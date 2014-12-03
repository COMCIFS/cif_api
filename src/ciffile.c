/*
 * ciffile.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <limits.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>

#include "cif.h"
#include "internal/utils.h"
#include "internal/value.h"

#define CIF_NOWRAP    0
#define CIF_WRAP      1

#define PREFIX "> "
#define PREFIX_LENGTH 2

/*
 * Expands to the lesser of its arguments.  The arguments may be evaluated more than once.
 */
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

typedef struct {
    FILE *byte_stream;
    unsigned char *byte_buffer;
    size_t buffer_size;
    unsigned char *buffer_position;
    unsigned char *buffer_limit;
    UConverter *converter;
    /*
     *   0 if EOF has not yet been detected;
     * < 0 if EOF has been detected, but data may not all have been converted;
     * > 1 if EOF has been detected and all data have been converted
     */
    int eof_status;
    int last_error;
} uchar_stream_t;

typedef struct {
    UFILE *file;
    int write_item_names;
    int separate_values;
    int last_column;
    int depth;
} write_context_t;

/* Unicode strings important for determining possible output formats for char values */
static const UChar dq3[4] = { UCHAR_DQ, UCHAR_DQ, UCHAR_DQ, 0 };
static const UChar sq3[4] = { UCHAR_SQ, UCHAR_SQ, UCHAR_SQ, 0 };
static const char header_type[2][10] = { "\ndata_%S\n", "\nsave_%S\n" };

/* the true data type of the CIF walker context pointers used by the CIF-writing functions */
#define CONTEXT_S write_context_t
#define CONTEXT_T CONTEXT_S *
#define CONTEXT_INITIALIZE(c, f) do { \
    c.file = f; c.write_item_names = CIF_FALSE; c.separate_values = 1; c.last_column = 0; c.depth = 0; \
} while (CIF_FALSE)
#define CONTEXT_UFILE(c) (((CONTEXT_T)(c))->file)
#define SET_WRITE_ITEM_NAMES(c,v) do { ((CONTEXT_T)(c))->write_item_names = (v); } while (CIF_FALSE)
#define IS_WRITE_ITEM_NAMES(c) (((CONTEXT_T)(c))->write_item_names)
#define SET_SEPARATE_VALUES(c,v) do { ((CONTEXT_T)(c))->separate_values = (v); } while (CIF_FALSE)
#define IS_SEPARATE_VALUES(c) (((CONTEXT_T)(c))->separate_values)
#define SET_LAST_COLUMN(c,l) do { ((CONTEXT_T)(c))->last_column = (l); } while (CIF_FALSE)
#define LAST_COLUMN(c) (((CONTEXT_T)(c))->last_column)
#define LINE_LENGTH(c) (CIF_LINE_LENGTH)
#define CONTEXT_DEPTH(c) (((CONTEXT_T)(c))->depth)
#define CONTEXT_INC_DEPTH(c, inc) do { ((CONTEXT_T)(c))->depth += (inc); } while (CIF_FALSE)

/*
 * A value-returning macro that ensures the current output position is preceded by whitespace, outputting appropriate
 * whitespace if necessary.  Evaluates to true if successful, else to false.
 *
 * @param[in]  c a pointer to the write context object describing the current writing context
 * @param[out] t an lvalue of a signed integral type for use as temporary storage
 */
#define ENSURE_SPACED(c, t) ( \
    (LAST_COLUMN(c) <= 0) || ( \
        (t = write_literal((c), " ", 1, CIF_NOWRAP)), \
        ((t == -CIF_OVERLENGTH_LINE) ? write_newline(c) : (t == 1)) \
    ) \
)

static void ustream_to_unicode_callback(const void *context, UConverterToUnicodeArgs *args, const char *codeUnits,
        int32_t length, UConverterCallbackReason reason, UErrorCode *error_code);
static ssize_t ustream_read_chars(void *char_source, UChar *dest, ssize_t count, int *error_code);

/*
 * CIF handler functions used by write_cif()
 */

/*
 * Handles the beginning of a CIF by outputting a CIF version comment
 */
static int write_cif_start(cif_t *cif, void *context);

/*
 * Handles the end of a CIF by outputting a newline
 */
static int write_cif_end(cif_t *cif, void *context);

/*
 * Handles the beginning of a CIF data block by outputting a block header
 */
static int write_container_start(cif_container_t *block, void *context);

/*
 * Handles the end of a CIF data block by outputting a newline
 */
static int write_container_end(cif_container_t *block, void *context);

/*
 * Handles the beginning of a loop by outputting a loop header
 * (unless it is the scalar loop)
 */
static int write_loop_start(cif_loop_t *loop, void *context);

/*
 * Handles the end of a loop by outputting a newline
 */
static int write_loop_end(cif_loop_t *loop, void *context);

/*
 * Handles the beginning of a loop packet by doing nothing
 */
static int write_packet_start(cif_packet_t *packet, void *context);

/*
 * Handles the end of a loop packet by outputting a newline
 */
static int write_packet_end(cif_packet_t *packet, void *context);

/*
 * Handles a data item by outputting it, possibly preceded by its data name
 */
static int write_item(UChar *name, cif_value_t *value, void *context);

/*
 * An internal function for writing a list value.  Returns a CIF API result code.
 */
static int write_list(void *context, cif_value_t *char_value);

/*
 * An internal function for writing a table value.  Returns a CIF API result code.
 */
static int write_table(void *context, cif_value_t *char_value);

/*
 * An internal function for writing a char value in an appropriate form.  Returns a CIF API result code.
 */
static int write_char(void *context, cif_value_t *char_value, int allow_text);

/*
 * An internal function for writing a text block.  Returns a CIF API result code.
 */
static int write_text(void *context, UChar *text, int32_t length, int fold, int prefix);

/*
 * @brief computes the best length for the next segment of a folded text block line
 *
 * This function attempts to fold before a word boundary, but will split words if there are no suitable boundaries
 * in the target window.  Will not split surrogate pairs.
 *
 * @param[in] line a pointer to the beginning of the line to fold
 * @param[in] do_fold indicates whether to actually perform folding.  If this argument evaluates to false, then the
 *         full number of code units in @c line is returned
 * @param[in] target_length the desired length of the folded segment, in code @i units (not code @i points )
 * @param[in] window the variance allowed in the length of folded segments other than the last, in code units
 *
 * @return the number of code units in the first folded segment; zero if @c line is an empty Unicode string
 */
static int fold_line(const UChar *line, int do_fold, int target_length, int window);

/*
 * An internal function for writing a character value in quoted form.  Returns a CIF API result code.
 */
static int write_quoted(void *context, const UChar *text, int32_t length, char delimiter);

/*
 * An internal function for writing a character value in triple-quoted form.  Returns a CIF API result code.
 */
static int write_triple_quoted(void *context, const UChar *text, int32_t line1_length, int32_t last_line_length,
        char delimiter);

/*
 * An internal function for writing a NUMB value.  Returns a CIF API result code.
 */
static int write_numb(void *context, cif_value_t *numb_value);

/*
 * An internal function for outputting a literal byte string.  Returns the number of Unicode characters written,
 * or -1 times the numeric value of a CIF error code if an error occurs.
 */
static int32_t write_literal(void *context, const char *text, int length, int wrap);

/*
 * An internal function for outputting a Unicode string literal.  Returns the number of Unicode characters written,
 * or -1 times the numeric value of a CIF error code if an error occurs.
 */
static int32_t write_uliteral(void *context, const UChar *text, int length, int wrap);

/*
 * An internal function for outputting a newline; helps keep track of the current column to which output is being
 * directed.  Returns a truthy (nonzero) int on success, or a falsey (0) int on failure.
 */
static int write_newline(void *context);

/* A CIF handler that handles nothing */
static cif_handler_t DEFAULT_CIF_HANDLER = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/* The CIF parsing options used when none are provided by the caller */
static struct cif_parse_opts_s DEFAULT_OPTIONS =
        { 0, NULL, 0, 0, 0, 1, &DEFAULT_CIF_HANDLER, NULL, cif_parse_error_die, NULL };

/* The length of the basic magic code identifying many CIFs (including all well-formed CIF 2.0 CIFs): "#\#CIF_" */
#define MAGIC_LENGTH 7

/* The number of additional characters in a CIF 2.0 magic code that are not covered by the basic magic code */
#define MAGIC_EXTRA 3

/* A CIF2 magic number in the platform default encoding */
static const char CIF2_DEFAULT_MAGIC[MAGIC_LENGTH + MAGIC_EXTRA + 1] = "#\\#CIF_2.0";

/* A CIF2 magic number encoded in UTF-8; in many cases, the contents will match CIF2_DEFAULT_MAGIC */
static const char CIF2_UTF8_MAGIC[MAGIC_LENGTH + MAGIC_EXTRA + 1] = "\x23\x5c\x23\x43\x49\x46\x5f\x32\x2e\x30";

static const char UTF8[6] = "UTF-8";

#ifdef __cplusplus
extern "C" {
#endif

int cif_parse_options_create(struct cif_parse_opts_s **opts) {
    struct cif_parse_opts_s *opts_temp = (struct cif_parse_opts_s *) calloc(1, sizeof(struct cif_parse_opts_s));

    if (opts_temp == NULL) {
        return CIF_ERROR;
    } else {
        /* explicitly initialize all pointer members */
        opts_temp->default_encoding_name = NULL;
        opts_temp->handler = NULL;
        opts_temp->whitespace_callback = NULL;
        opts_temp->error_callback = NULL;
        opts_temp->user_data = NULL;
        /* members having integral types are pre-initialized to zero because calloc() clears the memory it allocates */
        opts_temp->max_frame_depth = 1;

        *opts = opts_temp;
        return CIF_OK;
    }
}

int cif_write_options_create(struct cif_write_opts_s **opts) {
    struct cif_write_opts_s *opts_temp = (struct cif_write_opts_s *) calloc(1, sizeof(struct cif_parse_opts_s));

    if (opts_temp == NULL) {
        return CIF_ERROR;
    } else {
        /* explicitly initialize all pointer members */

        /* members having integral types are pre-initialized to zero because calloc() clears the memory it allocates */

        *opts = opts_temp;
        return CIF_OK;
    }
}

/*
 * Parses a CIF from the specified stream.  If the 'cif' argument is non-NULL
 * then a handle for the parsed CIF is written to its referrent.  Otherwise,
 * the parsed results are discarded, but the return code still indicates whether
 * parsing was successful.
 *
 * Considerations:
 * (1) cannot rely on seeking or rewinding the stream (not all streams support it)
 * (2) cannot use fileno() or an equivalent, because the provided stream may have data already buffered
 * (3) cannot initially be certain, in general, which encoding is used
 *
 * IMPORTANT: The implementation of this function necessarily involves some implementation-defined (but usually
 * reliable) behavior.  This arises from ICU's reliance on the 'char' data type for binary data (encoded characters)
 * relative to the implementation freedom C allows for that type, and from the fact that C I/O is ultimately based on
 * units of type 'unsigned char'.  It is not guaranteed that type 'char' can represent as many distinct values as
 * 'unsigned char', or that the bit pattern representing an arbitrary 'unsigned char' is also a valid 'char'
 * representation, or that interpreting an 'unsigned char' bit pattern as a 'char' value results in the same value
 * that (for example) fgets() would store to its target string when it processes that bit pattern as input.  The
 * following code nevertheless assumes all those things to be true.
 */
#define BUFFER_SIZE  2048
int cif_parse(FILE *stream, struct cif_parse_opts_s *options, cif_t **cifp) {
    FAILURE_HANDLING;
    unsigned char buffer[BUFFER_SIZE];
    size_t count;
    cif_t *cif;
    const char *encoding_name;
    uchar_stream_t ustream;
    UErrorCode error_code = U_ZERO_ERROR;
    int cif_version = 0;
    struct scanner_s scanner;
    int result;

    if (options == NULL) {
        options = &DEFAULT_OPTIONS;
    }

    if (cifp == NULL) {
        cif = NULL;
    } else if ((result = ((*cifp == NULL) ? cif_create(cifp) : CIF_OK)) == CIF_OK) {
        cif = *cifp;
    } else {
        return result;
    }

    if (options->force_default_encoding != 0) {
        encoding_name = options->default_encoding_name;
        count = 0;
        if (options->default_to_cif2 != 0) {
            cif_version = -2;
        }
    } else {
        /* attempt to guess the character encoding based on the first few bytes of the stream */

        char *char_buffer = (char *) buffer;

        count = fread(char_buffer, 1, BUFFER_SIZE, stream);  /* returns a short count only if there isn't enough data */
        if ((count < BUFFER_SIZE) && ferror(stream)) {
            DEFAULT_FAIL(early);
        } else if (count == 0) {
            /* simplest possible case: empty file --> empty CIF */
            return CIF_OK;
        } else {
            int32_t sig_length;

            /* Look for a Unicode signature (a BOM encoded at the beginning of the stream) */
            encoding_name = ucnv_detectUnicodeSignature(char_buffer, count, &sig_length, &error_code);
            if (U_FAILURE(error_code)) {
                /* FIXME: verify that ICU's idea of failure is what we really want here */
                DEFAULT_FAIL(early);
            } else if (encoding_name != NULL) {
                /* a Unicode encoding signature is successfully detected */
                /* no further action here */
            } else if ((count >= MAGIC_LENGTH + MAGIC_EXTRA)
                    && (memcmp(char_buffer, CIF2_UTF8_MAGIC, MAGIC_LENGTH + MAGIC_EXTRA) == 0)) {
                /* FIXME: should really test whether the magic number is followed by whitespace (which is required) */
                /* the input carries a CIF2 binary magic number, so choose UTF8 */
                encoding_name = UTF8;
                cif_version = 2;
            } else if ((options->default_to_cif2 != 0) && ((count < MAGIC_LENGTH + MAGIC_EXTRA)
                    || ((memcmp(char_buffer, CIF2_DEFAULT_MAGIC, MAGIC_LENGTH) != 0)
                            && (memcmp(char_buffer, CIF2_UTF8_MAGIC, MAGIC_LENGTH) != 0)))) {
                /*
                 * There is no CIF magic code expressed in either of the candidate encodings, and the user has opted to
                 * default to CIF2 in such cases (contrary to the CIF 2 specifications), yet the user has NOT opted
                 * to override the default encoding of CIF2 (UTF-8)
                 */
                encoding_name = UTF8;
                cif_version = 2;
            } else {
                /*
                 * There is a magic code for a CIF version other than 2.0, or there is no magic code and the caller
                 * has not opted to treat the input as CIF 2.0 in that case.
                 */
                encoding_name = NULL;  /* use the default encoding */
                cif_version = 1;
            }
        }
    }

    /* encoding identified, or knowingly defaulted */

    ustream.converter = ucnv_open(encoding_name, &error_code); /* FIXME: is any other customization needed? */
    if (U_SUCCESS(error_code)) {
        const char *converter_name = ucnv_getName(ustream.converter, &error_code);  /* belongs to ustream.converter */

        ucnv_setToUCallBack(ustream.converter, ustream_to_unicode_callback, &scanner, NULL, NULL, &error_code);

        if (U_FAILURE(error_code)) {
            result = CIF_ERROR;
        } else {
            /* FIXME: this test is probably too simplistic: */
            int not_utf8 = strcmp("UTF-8", converter_name);

            /* set up those properties of the scanner that derive from caller input */

            /* character source */
            ustream.byte_stream = stream;
            ustream.byte_buffer = buffer;
            ustream.buffer_size = BUFFER_SIZE;
            ustream.buffer_position = buffer;
            ustream.buffer_limit = buffer + count;
            ustream.eof_status = 0;
            ustream.last_error = 0; /* this is a _user_ error code, not necessarily a CIF code */

            /* scanner details */
            scanner.char_source = &ustream;
            scanner.read_func = ustream_read_chars;
            scanner.at_eof = CIF_FALSE;
            scanner.cif_version = cif_version;
            scanner.line_unfolding = MIN(options->line_folding_modifier, 1);
            scanner.prefix_removing = MIN(options->text_prefixing_modifier, 1);
            scanner.max_frame_depth = MIN(options->max_frame_depth, 1);
            scanner.handler = ((options->handler == NULL) ? DEFAULT_OPTIONS.handler : options->handler);
            scanner.error_callback
                    = ((options->error_callback == NULL) ? DEFAULT_OPTIONS.error_callback : options->error_callback);
            scanner.whitespace_callback = ((options->whitespace_callback == NULL) ? DEFAULT_OPTIONS.whitespace_callback
                    : options->whitespace_callback);
            scanner.user_data = options->user_data;  /* may be NULL */

            /* perform the actual parse */
            result = cif_parse_internal(&scanner, not_utf8, cif);
        }

        ucnv_close(ustream.converter);

        return result;
    }

    FAILURE_HANDLER(early):
    FAILURE_TERMINUS;
}
#undef BUFFER_SIZE

/*
 * Formats the CIF data represented by the 'cif' handle to the specified
 * output.
 */
int cif_write(FILE *stream, struct cif_write_opts_s *options UNUSED, cif_t *cif) {
    cif_handler_t handler = {
        write_cif_start,
        write_cif_end,
        write_container_start,
        write_container_end,
        write_container_start,
        write_container_end,
        write_loop_start,
        write_loop_end,
        write_packet_start,
        write_packet_end,
        write_item
    };
    UFILE *u_stream = u_finit(stream, "C", "UTF_8");
    int result;

    if (u_stream != NULL) {
        CONTEXT_S context;

        CONTEXT_INITIALIZE(context, u_stream);
        result = cif_walk(cif, &handler, &context);

        /* TODO: handle errors */
        u_fclose(u_stream);
        return result;
    } else {
        return CIF_ERROR;
    }
}

#ifdef __cplusplus
}
#endif

static ssize_t ustream_read_chars(void *char_source, UChar *dest, ssize_t count, int *error_code) {
    uchar_stream_t *ustream = (uchar_stream_t *) char_source;

    if ((count <= 0) || (ustream->eof_status > 0)) {
        return 0;
    } else {
        UErrorCode icu_error_code = U_ZERO_ERROR;
        UChar *dest_temp = dest;
        ssize_t num_read;

        do {
            char *pos;

            if ((ustream->buffer_position >= ustream->buffer_limit) && (ustream->eof_status == 0)) {
                /* fill the byte buffer; assumes the buffer size is nonzero */

                /* FIXME: replace FILE/fread() with descriptor/read() when the latter is available */
                size_t bytes_read = fread(ustream->byte_buffer, 1, ustream->buffer_size, ustream->byte_stream);

                if (bytes_read < ustream->buffer_size) {
                    if (ferror(ustream->byte_stream) != 0) {
                        /* I/O error */
                        return -1;
                    } else {
                        /* end-of-file encountered */
                        ustream->eof_status = -1;
                    }
                }

                /* record the boundaries of the valid buffered bytes */
                ustream->buffer_position = ustream->byte_buffer;
                ustream->buffer_limit = ustream->byte_buffer + bytes_read;
            }

            /* convert to Unicode via the associated converter */
            pos = (char *) ustream->buffer_position;
            ucnv_toUnicode(ustream->converter, &dest_temp, dest + count, (const char **) &pos,
                    (char *) ustream->buffer_limit, NULL, (ustream->eof_status != 0), &icu_error_code);
            ustream->buffer_position = (unsigned char *) pos;
            num_read = (dest_temp - dest);

            /* catch conversion errors and end of data */
            if (icu_error_code == U_BUFFER_OVERFLOW_ERROR) {
                break;
            } else if (U_FAILURE(icu_error_code)) {
                /* usually set via a callback from the converter; */
                *error_code = ((ustream->last_error == 0) ? CIF_ERROR : ustream->last_error);
                return -1;
            } else if (ustream->eof_status != 0) {
                /* end of the character stream */
                ustream->eof_status = 1;
                break;
            }
        } while (num_read == 0);

        return num_read;
    }
}

/*
 * An ICU converter callback for the to-Unicode direction that wraps a CIF API error callback
 */
static void ustream_to_unicode_callback(const void *context, UConverterToUnicodeArgs *args,
        const char *codeUnits UNUSED, int32_t length UNUSED, UConverterCallbackReason reason, UErrorCode *error_code) {
    const struct scanner_s *scanner = (const struct scanner_s *) context;
    uchar_stream_t *ustream = (uchar_stream_t *) scanner->char_source;

    if (reason <= UCNV_IRREGULAR) {
        ustream->last_error = scanner->error_callback(
                ((reason == UCNV_UNASSIGNED) ? CIF_UNMAPPED_CHAR : CIF_INVALID_CHAR), scanner->line, scanner->column,
                NULL, 0, scanner->user_data);
        if (ustream->last_error == 0) { /* Clear the ICU error and output a substitution character */
            UChar repl = ((scanner->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR);

            *error_code = U_ZERO_ERROR;
            ucnv_cbToUWriteUChars(args, &repl, 1, 0, error_code);
            /* if a new error is generated by the above call then it will be propagated outward from ICU */
        }
    } /* else it's a lifecycle signal, which we can safely ignore */
}


static int write_cif_start(cif_t *cif UNUSED, void *context) {
    int num_written = u_fprintf(CONTEXT_UFILE(context), "#\\#CIF_2.0\n");

    SET_LAST_COLUMN(context, 0);
    return ((num_written > 0) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_cif_end(cif_t *cif UNUSED, void *context) {
    return (write_newline(context) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_container_start(cif_container_t *block, void *context) {
    UChar *code;
    int result = cif_container_get_code(block, &code);
    const char *this_header_type = header_type[(CONTEXT_DEPTH(context) == 0) ? 0 : 1];

    if (result == CIF_OK) {
        result = ((u_fprintf(CONTEXT_UFILE(context), this_header_type, code) > 7) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
        SET_LAST_COLUMN(context, 0);
        if (result == CIF_TRAVERSE_CONTINUE) {
            CONTEXT_INC_DEPTH(context, 1);
        }
        free(code);
    }
    return result;
}

static int write_container_end(cif_container_t *block UNUSED, void *context) {
    CONTEXT_INC_DEPTH(context, -1);
    SET_LAST_COLUMN(context, 0);  /* anticipates the next line */
    if (CONTEXT_DEPTH(context) == 0) {
        return (write_newline(context) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
    } else {
        return ((u_fprintf(CONTEXT_UFILE(context), "\nsave_\n") > 6) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
    }
}

static int write_loop_start(cif_loop_t *loop, void *context) {
    UChar *category;
    int result;

    result = cif_loop_get_category(loop, &category);
    if (result == CIF_OK) {
        if ((category != NULL) && (u_strcmp(category, CIF_SCALARS) == 0)) {
            /* the scalar loop for this container */
            if (write_newline(context)) {
                SET_WRITE_ITEM_NAMES(context, CIF_TRUE);
                result = CIF_TRAVERSE_CONTINUE;
            } else {
                result = CIF_ERROR;
            }
        } else {
            /* an ordinary loop */
            SET_WRITE_ITEM_NAMES(context, CIF_FALSE);

            /* write a loop header */
            if (u_fprintf(CONTEXT_UFILE(context), "\nloop_\n") > 6) {
                UChar **item_names;

                SET_LAST_COLUMN(context, 0);
                result = cif_loop_get_names(loop, &item_names);
                if (result == CIF_OK) {
                    UChar **next_name;

                    result = CIF_TRAVERSE_CONTINUE;
                    for (next_name = item_names; *next_name != NULL; next_name += 1) {
                        if (result == CIF_TRAVERSE_CONTINUE) {
                            if (u_fprintf(CONTEXT_UFILE(context), " %S\n", *next_name) < 4) {
                                result = CIF_ERROR;
                            }
                            SET_LAST_COLUMN(context, 0);
                        }
                        /* need to free all item names even after an error is detected */
                        free(*next_name);
                    }

                    /* always need to free the item name array itself */
                    free(item_names);
                }
            } else {
                result = CIF_ERROR;
            }
        }
    }

    free(category);

    return result;
}

static int write_loop_end(cif_loop_t *loop UNUSED, void *context) {
    return (write_newline(context) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_packet_start(cif_packet_t *packet UNUSED, void *context UNUSED) {
    /* No particular action */
    return CIF_TRAVERSE_CONTINUE;
}

static int write_packet_end(cif_packet_t *packet UNUSED, void *context) {
    return (write_newline(context) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_item(UChar *name, cif_value_t *value, void *context) {
    FAILURE_HANDLING;
    int temp;

    /* output the data name if the context so indicates */
    if (IS_WRITE_ITEM_NAMES(context)) {
        /* write the name at the beginning of a line */
        if ((LAST_COLUMN(context) > 0) && !write_newline(context)) {
            FAIL(soft, CIF_ERROR);
        }
        if (write_uliteral(context, name, -1, CIF_NOWRAP) < 2) {
            FAIL(soft, CIF_ERROR);
        }
    }

    /* Precede the value with a single space or newline if required */
    if (IS_SEPARATE_VALUES(context) && !ENSURE_SPACED(context, temp)) {
        FAIL(soft, CIF_ERROR);
    }

    /* output the value in a manner determined by its kind */
    switch (cif_value_kind(value)) {
        /* these result value computations rely on the fact that CIF_OK == CIF_TRAVERSE_CONTINUE == 0 */
        case CIF_CHAR_KIND:
            SET_RESULT(write_char(context, value, CIF_TRUE));
            break;
        case CIF_NUMB_KIND:
            SET_RESULT(write_numb(context, value));
            break;
        case CIF_LIST_KIND:
            SET_RESULT(write_list(context, value));
            break;
        case CIF_TABLE_KIND:
            SET_RESULT(write_table(context, value));
            break;
        case CIF_NA_KIND:
            SET_RESULT((write_literal(context, ".", 1, CIF_WRAP) == 1) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
            break;
        case CIF_UNK_KIND:
            SET_RESULT((write_literal(context, "?", 1, CIF_WRAP) == 1) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
            break;
        default:
            /* unknown kind */
            SET_RESULT(CIF_INTERNAL_ERROR);
            break;
    }

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

static int write_list(void *context, cif_value_t *list_value) {
    size_t count;
    
    if (cif_value_get_element_count(list_value, &count) != CIF_OK) {
        return CIF_INTERNAL_ERROR;
    }

    if (write_literal(context, "[", 1, CIF_WRAP) == 1) {
        int write_names_save = IS_WRITE_ITEM_NAMES(context);
        int separate_values_save = IS_SEPARATE_VALUES(context);
        size_t index;

        SET_WRITE_ITEM_NAMES(context, CIF_FALSE);
        SET_SEPARATE_VALUES(context, CIF_TRUE);
        for (index = 0; index < count; index += 1) {
            cif_value_t *element;
            int result;

            if (cif_value_get_element_at(list_value, index, &element) != CIF_OK) {
                return CIF_INTERNAL_ERROR;
            } else if ((result = write_item(NULL, element, context)) > 0) {
                /* an error code */
                return result;
            }
        }

        if (write_literal(context, " ]", 2, CIF_WRAP) == 2) {
            /* success */
            SET_SEPARATE_VALUES(context, separate_values_save);
            SET_WRITE_ITEM_NAMES(context, write_names_save);
            return CIF_OK;
        }
    }

    return CIF_ERROR;
}

static int write_table(void *context, cif_value_t *table_value) {
    FAILURE_HANDLING;
    const UChar **keys;
    int result;

    if ((result = cif_value_get_keys(table_value, &keys)) != CIF_OK) {
        SET_RESULT(result);
    } else {
        int separate_values_save = IS_SEPARATE_VALUES(context);

        if (write_literal(context, "{", 1, separate_values_save) == 1) {
            int write_names_save = IS_WRITE_ITEM_NAMES(context);
            const UChar **key;

            SET_WRITE_ITEM_NAMES(context, CIF_FALSE);
            for (key = keys; *key; key += 1) {
                cif_value_t *kv = NULL;
                cif_value_t *value = NULL;

                if (cif_value_get_item_by_key(table_value, *key, &value) != CIF_OK) {
                    FAIL(soft, CIF_INTERNAL_ERROR);
                }

                if (u_strHasMoreChar32Than(*key, -1, LINE_LENGTH(context) - (LAST_COLUMN(context) + 4))
                        && !write_newline(context)) {
                    FAIL(soft, CIF_ERROR);
                }

                if (cif_value_create(CIF_UNK_KIND, &kv) != CIF_OK) {
                    FAIL(soft, CIF_ERROR);
                } else {
                    SET_SEPARATE_VALUES(context, CIF_FALSE);

                    /* copying the key is inefficient, but necessitated by the external API */
                    if (!ENSURE_SPACED(context, result) || (cif_value_copy_char(kv, *key) != CIF_OK)) {
                        SET_RESULT(CIF_ERROR);
                    } else if ((result = write_char(context, kv, CIF_FALSE)) != CIF_OK) {
                        SET_RESULT(result);
                    } else if (write_literal(context, ":", 1, CIF_NOWRAP) != 1) {
                        SET_RESULT(CIF_ERROR);
                    } else {
                        if ((result = write_item(NULL, value, context)) > 0) {
                            /* an error code */
                            SET_RESULT(result);
                        } else {
                            /* entry successfully written */
                            cif_value_free(kv);
                            continue;
                        }
                    }

                    /* entry not successfully written -- clean up before failing */
                    cif_value_free(kv);
                    DEFAULT_FAIL(soft);
                }
            }

            if (write_literal(context, " }", 2, CIF_WRAP) == 2) {
                /* success */
                SET_SEPARATE_VALUES(context, separate_values_save);
                SET_WRITE_ITEM_NAMES(context, write_names_save);
                SET_RESULT(CIF_OK);
            }
        }

        FAILURE_HANDLER(soft):
        /* free only the key array, not the individual keys, as required by cif_value_get_keys() */
        free(keys);
    }

    /* failure and success cases both end up here */
    FAILURE_TERMINUS;
}

static int write_char(void *context, cif_value_t *char_value, int allow_text) {
    int result;
    UChar *text;

    if (cif_value_get_text(char_value, &text) == CIF_OK) {
        /* uses signed 32-bit integer character counters -- could overflow for very (very!) long text blocks */
        int32_t char_counts[129];
        int32_t first_line = 0;
        int32_t this_line = 0;
        int32_t max_line = 0;
        int32_t length = 0;
        int32_t consec_semis = 0;
        int32_t most_semis = 0;
        int32_t extra_space = (IS_SEPARATE_VALUES(context) ? 0 : LAST_COLUMN(context));
        int has_nl_semi = CIF_FALSE;
        int emulates_prefix = CIF_FALSE;
        UChar ch;

        /* Analyze the text to inform the choice of delimiters */
        memset(char_counts, 0, sizeof(char_counts));
        for (ch = text[length]; ch; ch = text[++length]) {
            char_counts[(ch < 128) ? ch : 128] += 1;
            if (ch == UCHAR_NL) {
                has_nl_semi = (has_nl_semi || (text[length + 1] == UCHAR_SEMI));
                if (char_counts[UCHAR_NL] == 1) {
                    first_line = this_line;
                }
                if (this_line > max_line) {
                    max_line = this_line;
                }
                if (consec_semis > most_semis) {
                    most_semis = consec_semis;
                }
                consec_semis = 0;
                this_line = 0;
            } else {
                if (ch == UCHAR_SEMI) {
                    consec_semis += 1;
                } else {
                    if (consec_semis > most_semis) {
                        most_semis = consec_semis;
                    }
                    consec_semis = 0;
                }
                this_line += 1;
            }
        }
        if (char_counts[UCHAR_NL] == 0) {
            first_line = this_line;
        }
        if (this_line > max_line) {
            max_line = this_line;
        }

        /* If the longest line surpasses the length limit then we cannot use anything other than a text block */
        if (max_line <= (LINE_LENGTH(context))) {
            if (char_counts[UCHAR_NL] == 0) {
                /* Maybe single-delimited */
                if (char_counts[UCHAR_SQ] == 0) {
                    /* write the value as an apostrophe-delimited string */
                    result = write_quoted(context, text, length, UCHAR_SQ);
                    goto done;
                } else if (char_counts[UCHAR_DQ] == 0) {
                    result = write_quoted(context, text, length, UCHAR_DQ);
                    goto done;
                } /* else neither (single) apostrophe or quotation mark is suitable */

                /* maybe triple-delimited */
                /*
                 * If we are forbidden to precede the value with whitespace then it must fit on the remainder of the
                 * current line.
                 */
                if (max_line <= (LINE_LENGTH(context)) - (6 + extra_space)) {
                    /* 
                     * line 1 lengths expressed to write_triple_quoted() here include the closing delimiter, because it
                     * will appear on the first line.  Line 1 lengths NEVER include the opening delimiter.
                     */
                    if (u_strstr(text, sq3) == NULL) {
                        result = write_triple_quoted(context, text, length + 3, this_line, UCHAR_SQ);
                        goto done;
                    } else if (u_strstr(text, dq3) == NULL) {
                        result = write_triple_quoted(context, text, length + 3, this_line, UCHAR_DQ);
                        goto done;
                    }
                }

            } else 

            /*
             * We can use triple quotes if neither the first line nor the last is too long, and if the text does not
             * contain both triple delimiters.
             */
            if ((this_line < (LINE_LENGTH(context) - 3))
                            && (first_line < (LINE_LENGTH(context) - (3 + extra_space)))) {
                if (u_strstr(text, sq3) == NULL) {
                    result = write_triple_quoted(context, text, first_line, this_line, UCHAR_SQ);
                    goto done;
                } else if (u_strstr(text, dq3) == NULL) {
                    result = write_triple_quoted(context, text, first_line, this_line, UCHAR_DQ);
                    goto done;
                }
            }

        }

        /* if control reaches this point then all alternatives other than a text block have been ruled out */
        if (allow_text) {
            /* write as a text block, possibly with line-folding and/or prefixing  */
            int fold = ((max_line > LINE_LENGTH(context))
                    || (most_semis >= LINE_LENGTH(context))
                    || ((!has_nl_semi) && ((first_line + 1) > LINE_LENGTH(context))));

            if (!has_nl_semi) { /* otherwise it's redundant to perform the following test */
                /* scan backward through the first line to check whether it mimics a prefix marker */
                int32_t index;

                for (index = first_line - 1; index >= 0; index -= 1) {
                    switch (text[index]) {
                        case UCHAR_TAB:
                        case UCHAR_SP:
                            /* trailing spaces and tabs do not affect the determination */
                            break;
                        case UCHAR_BSL:
                            /* the last non-space character is a backslash -- looks like a prefix/folding marker */
                            emulates_prefix = CIF_TRUE;
                            /* fall through */
                        default:
                            /* the last non-space character is not a backslash -- not a prefix-mark mimic */
                            goto end_scan;
                    }
                }
            }

            end_scan:
            result = write_text(context, text, length, fold, (has_nl_semi || emulates_prefix));
        } else {
            result = CIF_DISALLOWED_VALUE;
        }

        done:
        free(text);
    } else {
        result = CIF_ERROR;
    }

    return result;
}

/**
 * @brief Writes a text block according to the specified context, applying the line-folding protocol and/or the
 *        line prefix protocol as indicated.
 *
 * The contents of the text are destroyed by this function.  The caller is responsible for determining whether
 * line-folding or prefixing should be applied.
 *
 * @param[in,out] context the handler context describing the output sink and details
 * @param[in,out] text a pointer to the content of the text block; the text is destroyed by this function, but the
 *         caller retains responsibility for managing the space
 * @param[in] length the length of the text, in @c UChar units; must be consistent with the content of @c text
 * @param[in] fold if true, the line-folding protocol should be applied to the block contents
 * @param[in] prefix if true, the prefix fold protocol should be applied to the block contents
 */
static int write_text(void *context, UChar *text, int32_t length, int fold, int prefix) {
    /* TODO: test on text containing surrogate pairs -- the expected lengths may be wrong */
    int expected = 2 + (prefix ? (PREFIX_LENGTH + 1) : 0) + (fold ? 1 : 0);
    int32_t nchars;

    /* opening delimiter and flags */
    nchars = u_fprintf(CONTEXT_UFILE(context), "\n;%s%s", (prefix ? PREFIX "\\" : ""), (fold ? "\\" : ""));
    if (nchars != expected) {
        return CIF_ERROR;
    }

    /* body */
    if (!fold && !prefix) {
        /* shortcut when neither line-folding nor prefixing: */
        if (u_fprintf(CONTEXT_UFILE(context), "%S", text) != length) {
            return CIF_ERROR;
        }
    } else {
        int prefix_length = (prefix ? PREFIX_LENGTH : 0);
        int target_length = LINE_LENGTH(context) - 8;
        char prefix_text[3] = PREFIX;
        UChar *saved;
        UChar *tok;

        if (!prefix) {
            prefix_text[0] = '\0';
        }

        /* each logical line, delimited from the previous one by a newline */
        for (tok = text; tok != NULL; tok = saved) {
            int len;

            saved = u_strchr(tok, '\n');
            if (saved != NULL) {
                *saved = 0;
                saved += 1;
            }

            if (*tok == 0) {
                if (!write_newline(context)) {
                    return CIF_ERROR;
                }
            } else {
                int printed = CIF_FALSE;

                /* each folded segment */
                do {
                    len = fold_line(tok, fold, target_length, 6);

                    if ((len == 0) && printed) {
                        break;
                    }

                    expected = len + prefix_length + 1;
                    nchars = u_fprintf(CONTEXT_UFILE(context), "\n%s%*.*S%s", prefix_text, len, len, tok,
                            ((tok[len] == 0) ? "" : (expected += 1, "\\")));
                    if (nchars != expected) {
                        return CIF_ERROR;
                    }

                    printed = CIF_TRUE;
                    tok += len;
                } while (CIF_TRUE);
            }
        }
    }

    /* closing delimiter */
    if (u_fprintf(CONTEXT_UFILE(context), "\n;") != 2) {
        return CIF_ERROR;
    }
    SET_LAST_COLUMN(context, 1);

    return CIF_OK;
}

static int fold_line(const UChar *line, int do_fold, int target_length, int window) {
    int low_candidate = -1;
    int len;

    if (!do_fold) {
        return u_strlen(line);
    }

    for (len = 0; len <= target_length; len += 1) {
        if (line[len] == 0) {
            /* can't fold wider than the whole string */
            return len;
        } else if ((line[len] == ' ') || (line[len] == '\t')) {
            /* track the best folding boundary below the target length */
            low_candidate = len;
        }
    }

    for (; len <= target_length + window; len += 1) {
        if (line[len] == 0) {
            /* the whole string can fit in this fold */
            return len;
        } else if ((line[len] == ' ') || (line[len] == '\t')) {
            int high_candidate = len;

            /* scan forward to see whether the whole line will fit */
            while (++len <= target_length + window) {
                if (line[len] == 0) {
                    return len;
                }
            }
           
            /* choose a folding point inside the window */ 
            if (low_candidate < target_length - window) {
                /* there is no candidate in the bottom half of the window */
                return high_candidate;
            } else {
                /* len and low_candidate are both in the window; return the one closer to target_length */
                return (((high_candidate + low_candidate) / 2) < target_length ? high_candidate : low_candidate);
            }
        }
    }

    /* No better candidate was found, so fold at the target length, avoiding splitting surrogate pairs */
    /* TODO: avoid folding immediately before a semicolon (unless prefixing is in effect, too) */
    if ((line[target_length] >= MIN_TRAIL_SURROGATE) && (line[target_length] <= MAX_SURROGATE)
            && (line[target_length - 1] >= MIN_LEAD_SURROGATE) && (line[target_length - 1] < MIN_TRAIL_SURROGATE)) {
        target_length -= 1;
    }

    return target_length;
}

static int write_quoted(void *context, const UChar *text, int32_t length, char delimiter) {
    int32_t nchars;
    int last_column = LAST_COLUMN(context);

    if ((last_column + length + 2) > LINE_LENGTH(context)) {
        if (write_newline(context)) {
            last_column = 0;
        } else {
            return CIF_ERROR;
        }
    }

    nchars = u_fprintf(CONTEXT_UFILE(context), "%c%*.*S%c", delimiter, length, length, text, delimiter);

    SET_LAST_COLUMN(context, last_column + nchars);

    return (nchars == (length + 2)) ? CIF_OK : CIF_ERROR;
}

static int write_triple_quoted(void *context, const UChar *text, int32_t line1_length, int32_t last_line_length,
        char delimiter) {
    int32_t nchars;
    int last_column = LAST_COLUMN(context);

    if ((last_column + line1_length + 3) > LINE_LENGTH(context)) {
        if (write_newline(context)) {
            last_column = 0;
        } else {
            return CIF_ERROR;
        }
    } else if (text[line1_length]) {
        assert(text[line1_length] == '\n');
        last_column = 0;  /* as-of before writing the last line */
    }

    nchars = u_fprintf(CONTEXT_UFILE(context), "%c%c%c%S%c%c%c", delimiter, delimiter, delimiter,
            text, delimiter, delimiter, delimiter);

    SET_LAST_COLUMN(context, last_column + last_line_length + 3);

    return (nchars >= (line1_length + 6)) ? CIF_OK : CIF_ERROR;
}

static int write_numb(void *context, cif_value_t *numb_value) {
    int result;
    UChar *text;

    if (cif_value_get_text(numb_value, &text) == CIF_OK) {
        int32_t nchars = write_uliteral(context, text, -1, IS_SEPARATE_VALUES(context));
        free(text);
        result = ((nchars < 0) ? -nchars : ((nchars > 0) ? 0 : CIF_ERROR));
    } else {
        result = CIF_ERROR;
    }

    return result;
}

/**
 * @brief Writes a literal C string to the destination associated with the provided context
 *
 * @param[in,out] context a pointer to the context object describing the output destination and all needed state of
 *         the current writing process
 * @param[in] text a C string containing the text to write
 * @param[in] length the number of characters of @c text to write, or a negative number to signal the whole string
 *         should be written.  If the string length is known by the caller, then it is slightly more efficient to
 *         pass the known length than to pass a negative number.  Behavior is undefined if @c length is greater than
 *         the length of @c text.  May be zero, in which case this function does nothing and returns zero.
 * @param[in] wrap if @c true, and the specified text is too long for the remainder of the current output line, then
 *         the output will be written on a new line; otherwise, if the specified text is too long for the current line
 *         then no output is performed and an error code is returned
 * @return the number of characters written on success, or the negative of an error code on failure, normally one of:
 *        @li @c -CIF_OVERLENGTH_LINE if the text will not fit on the current line and wrapping is disabled
 *        @li @c -CIF_ERROR for most other failures
 */
static int32_t write_literal(void *context, const char *text, int length, int wrap) {
    if (length < 0) {
        length = strlen(text);
    }

    if (length == 0) {
        return 0;
    } else {
        int last_column = LAST_COLUMN(context);
        int32_t nchars;

        if ((length + last_column) > LINE_LENGTH(context)) {
            if (wrap) {
                if (write_newline(context)) {
                    last_column = 0;
                } else {
                    /* Failed to write a newline */
                    return -CIF_ERROR;
                }
            } else {
                return -CIF_OVERLENGTH_LINE;
            }
        }

        nchars = u_fprintf(CONTEXT_UFILE(context), "%*.*s", length, length, text);
        if (nchars > 0) {
            SET_LAST_COLUMN(context, last_column + nchars);
        }

        return nchars;
    }
}

/**
 * @brief Writes a literal Unicode string to the destination associated with the provided context
 *
 * Warning: an unfortunate choice of @c length could cause a surrogate pair to be split.
 *
 * @param[in,out] context a pointer to the context object describing the output destination and all needed state of
 *         the current writing process
 * @param[in] text a Unicode string containing the text to write
 * @param[in] length the number of @c UChar units of @c text to write, or a negative number to signal the whole string
 *         should be written.  If the string length is known by the caller, then it is slightly more efficient to
 *         pass the known length than to pass a negative number.  Behavior is undefined if @c length is greater than
 *         the length of @c text.  May be zero, in which case this function does nothing and returns zero.
 * @param[in] wrap if @c true, and the specified text is too long for the remainder of the current output line, then
 *         the output will be written on a new line; otherwise, if the specified text is too long for the current line
 *         then no output is performed and an error code is returned
 * @return the number of characters written on success, or the negative of an error code on failure, normally one of:
 *        @li @c -CIF_OVERLENGTH_LINE if the text will not fit on the current line and wrapping is disabled
 *        @li @c -CIF_ERROR for most other failures
 */
static int32_t write_uliteral(void *context, const UChar *text, int length, int wrap) {
    if (length < 0) {
        length = u_countChar32(text, -1);
    }

    if (length == 0) {
        return 0;
    } else {
        int last_column = LAST_COLUMN(context);
        int32_t nchars;

        if ((length + last_column) > LINE_LENGTH(context)) {
            if (wrap == CIF_WRAP) {
                if (write_newline(context)) {
                    last_column = 0;
                } else {
                    return -CIF_ERROR;
                }
            } else {
                return -CIF_OVERLENGTH_LINE;
            }
        }

        nchars = u_fprintf(CONTEXT_UFILE(context), "%*.*S", length, length, text);
        if (nchars > 0) {
            SET_LAST_COLUMN(context, last_column + nchars);
        }

        return nchars;
    }
}

static int write_newline(void *context) {
    if (u_fputc('\n', CONTEXT_UFILE(context)) == '\n') {
        SET_LAST_COLUMN(context, 0);
        return CIF_TRUE;
    } else {
        return CIF_FALSE;
    }
}
