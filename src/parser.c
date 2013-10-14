/*
 * parser.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

/**
 * It is important to understand that CIF 2.0 being UTF-8-only makes it a @i binary file type, albeit one that
 * can reliably pass for text on many modern systems.  Standard text parsing tools cannot portably be applied to
 * CIF 2.0 parsing, however, because implicitly, those are defined in terms of abstract characters.  The encoded
 * forms vary with the code page assumed by the compiler, whereas CIF 2.0's actual encoding does not.
 */

/*
 * The CIF parser implemented herein is a predictive recursive descent parser with full error recovery.
 */

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
#include "internal/utils.h"

/* plain macros */

#define SSIZE_T_MAX      (((size_t) -1) / 2)
#define BUF_SIZE_INITIAL (4 * (CIF_LINE_LENGTH + 2))
#define BUF_MIN_FILL          (CIF_LINE_LENGTH + 2)
#define CHAR_TABLE_MAX   160

/* specific character codes */
/* Note: numeric character codes avoid codepage dependencies associated with use of ordinary char literals */
#define LF_CHAR       0x0A
#define CR_CHAR       0x0D
#define DECIMAL_CHAR  0x2E
#define COLON_CHAR    0x3A
#define SEMI_CHAR     0x3B
#define QUERY_CHAR    0x3F
#define BKSL_CHAR     0x5C
#define CIF1_MAX_CHAR 0x7E
#define BOM_CHAR      0xFEFF
#define REPL_CHAR     0xFFFD
#define REPL1_CHAR    0x2A
#define EOF_CHAR      0xFFFF

/* character class codes */
/* code NO_CLASS must have value 0; the values of other codes may be changed as desired */
#define NO_CLASS      0
#define GENERAL_CLASS 1
#define WS_CLASS      2
#define EOL_CLASS     3
#define EOF_CLASS     4
#define HASH_CLASS    5
#define UNDERSC_CLASS 6
#define QUOTE_CLASS   7
#define SEMI_CLASS    9
#define OBRAK_CLASS  10
#define CBRAK_CLASS  11
#define OCURL_CLASS  12
#define CCURL_CLASS  13
/* class 14 is available */
#define DOLLAR_CLASS 15
#define OBRAK1_CLASS 16
#define CBRAK1_CLASS 17
#define A_CLASS      18
#define B_CLASS      19
#define D_CLASS      20
#define E_CLASS      21
#define G_CLASS      22
#define L_CLASS      23
#define O_CLASS      24
#define P_CLASS      25
#define S_CLASS      26
#define T_CLASS      27
#define V_CLASS      28

/* identifies the numerically-last class code, but does not itself directly represent a class: */
#define LAST_CLASS   V_CLASS

/* character metaclass codes */
#define NO_META       0
#define GENERAL_META  1
#define WS_META       2
#define OPEN_META     3
#define CLOSE_META    4

/* data types */

enum token_type {
    BLOCK_HEAD, FRAME_HEAD, FRAME_TERM, LOOPKW, NAME, OTABLE, CTABLE, OLIST, CLIST, KEY, TKEY, VALUE, QVALUE, TVALUE,
    END
};

struct scanner_s {
    UChar *buffer;                   /* A character buffer from which to scan characters */
    size_t buffer_size;              /* The total size of the buffer */
    size_t buffer_limit;             /* The size of the initial segment of the buffer containing valid characters */
    UChar *next_char;                /* A pointer into the buffer to the next character to be scanned */

    enum token_type ttype;           /* The grammatic category of the most recent token parsed */
    UChar *text_start;               /* The start position of the text from which the current token, if any, was parsed.
                                        This may differ from tvalue_start in some cases, such as for tokens representing
                                        delimited values. */
    UChar *tvalue_start;             /* The start position of the value of the current token, if any */
    size_t tvalue_length;            /* The length of the value of the current token, if any */

    size_t line;                     /* The current 1-based input line number */
    unsigned column;                 /* The number of characters so far scanned from the current line */

    unsigned int char_class[CHAR_TABLE_MAX];  /* Character class codes of the first Unicode code points */
    unsigned int meta_class[LAST_CLASS + 1];  /* Character metaclass codes for all the character classes */

    /* character source */
    read_chars_t read_func;
    void *char_source;
    int at_eof;

    /* cif version */
    int cif_version;

    /* custom parse options */
    int line_unfolding;
    int prefix_removing;
};

/* static data */

static const UChar eol_chars[3] = { LF_CHAR, CR_CHAR, 0 };

#define MAGIC_LENGTH 10
static const UChar CIF1_MAGIC[MAGIC_LENGTH]
        = { 0x23, 0x5c, 0x23, 0x43, 0x49, 0x46, 0x5f, 0x31, 0x2e, 0x31 }; /* #\#CIF_1.1 */
static const UChar CIF2_MAGIC[MAGIC_LENGTH]
        = { 0x23, 0x5c, 0x23, 0x43, 0x49, 0x46, 0x5f, 0x32, 0x2e, 0x30 }; /* #\#CIF_2.0 */


/* static function headers */

/* grammar productions */
static int parse_cif(struct scanner_s *scanner, cif_t *cifp);
static int parse_container(struct scanner_s *scanner, cif_container_t *container, int is_block);
static int parse_item(struct scanner_s *scanner, cif_container_t *container, UChar *name);
static int parse_loop(struct scanner_s *scanner, cif_container_t *container);
static int parse_list(struct scanner_s *scanner, cif_value_t **listp);
static int parse_table(struct scanner_s *scanner, cif_value_t **tablep);
static int parse_value(struct scanner_s *scanner, cif_value_t **valuep);

/* scanning functions */
static int next_token(struct scanner_s *scanner);
static int scan_ws(struct scanner_s *scanner);
static int scan_to_ws(struct scanner_s *scanner);
static int scan_to_eol(struct scanner_s *scanner);
static int scan_unquoted(struct scanner_s *scanner);
static int scan_delim_string(struct scanner_s *scanner);
static int scan_text(struct scanner_s *scanner);
static int get_more_chars(struct scanner_s *scanner);

/* other functions */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_t **dest);

/* function-like macros */

#define INIT_SCANNER(s) do { \
    int _i; \
    (s).line = 1; \
    (s).column = 0; \
    (s).ttype = END; \
    (s).cif_version = 2; \
    (s).line_unfolding = 1; \
    (s).prefix_removing = 1; \
    for (_i =   0; _i <  32;            _i += 1) (s).char_class[_i] = NO_CLASS; \
    for (_i =  32; _i < 127;            _i += 1) (s).char_class[_i] = GENERAL_CLASS; \
    for (_i = 128; _i < CHAR_TABLE_MAX; _i += 1) (s).char_class[_i] = NO_CLASS; \
    for (_i =   1; _i <= LAST_CLASS;    _i += 1) (s).meta_class[_i] = GENERAL_META; \
    (s).char_class[0x09] =   WS_CLASS; \
    (s).char_class[0x20] =   WS_CLASS; \
    (s).char_class[CR_CHAR] =  EOL_CLASS; \
    (s).char_class[LF_CHAR] =  EOL_CLASS; \
    (s).char_class[0x23] =   HASH_CLASS; \
    (s).char_class[0x95] =   UNDERSC_CLASS; \
    (s).char_class[0x22] =   QUOTE_CLASS; \
    (s).char_class[0x27] =   QUOTE_CLASS; \
    (s).char_class[SEMI_CHAR] = SEMI_CLASS; \
    (s).char_class[0x5B] =   OBRAK_CLASS; \
    (s).char_class[0x5D] =   CBRAK_CLASS; \
    (s).char_class[0x7B] =   OCURL_CLASS; \
    (s).char_class[0x7D] =   CCURL_CLASS; \
    (s).char_class[COLON_CHAR] = GENERAL_CLASS; \
    (s).char_class[0x24] =   DOLLAR_CLASS; \
    (s).char_class[0x41] =   A_CLASS; \
    (s).char_class[0x61] =   A_CLASS; \
    (s).char_class[0x42] =   B_CLASS; \
    (s).char_class[0x62] =   B_CLASS; \
    (s).char_class[0x44] =   D_CLASS; \
    (s).char_class[0x64] =   D_CLASS; \
    (s).char_class[0x45] =   E_CLASS; \
    (s).char_class[0x65] =   E_CLASS; \
    (s).char_class[0x47] =   G_CLASS; \
    (s).char_class[0x67] =   G_CLASS; \
    (s).char_class[0x4C] =   L_CLASS; \
    (s).char_class[0x6C] =   L_CLASS; \
    (s).char_class[0x4F] =   O_CLASS; \
    (s).char_class[0x6F] =   O_CLASS; \
    (s).char_class[0x50] =   P_CLASS; \
    (s).char_class[0x70] =   P_CLASS; \
    (s).char_class[0x53] =   S_CLASS; \
    (s).char_class[0x73] =   S_CLASS; \
    (s).char_class[0x54] =   T_CLASS; \
    (s).char_class[0x74] =   T_CLASS; \
    (s).char_class[0x56] =   V_CLASS; \
    (s).char_class[0x76] =   V_CLASS; \
    (s).char_class[0x7F] =   NO_CLASS; \
    (s).meta_class[NO_CLASS] =  NO_META; \
    (s).meta_class[WS_CLASS] =  WS_META; \
    (s).meta_class[EOL_CLASS] = WS_META; \
    (s).meta_class[EOF_CLASS] = WS_META; \
    (s).meta_class[OBRAK_CLASS] = OPEN_META; \
    (s).meta_class[OCURL_CLASS] = OPEN_META; \
    (s).meta_class[CBRAK_CLASS] = CLOSE_META; \
    (s).meta_class[CCURL_CLASS] = CLOSE_META; \
} while (CIF_FALSE)

/*
 * Adjusts an initialized scanner for parsing CIF 1 instead of CIF 2.  Should not be applied multiple times to the
 * same scanner.
 */
#define SET_V1(s) do { \
    (s).cif_version = 1; \
    (s).line_unfolding -= 1; \
    (s).prefix_removing -= 1; \
    (s).char_class[0x5B] =   OBRAK1_CLASS; \
    (s).char_class[0x5D] =   CBRAK1_CLASS; \
    (s).char_class[0x7B] =   GENERAL_CLASS; \
    (s).char_class[0x7D] =   GENERAL_CLASS; \
} while (CIF_FALSE)

/**
 * @brief Determines the class of the specified character based on the specified scanner ruleset.
 *
 * @param[in] c the character whose class is requested.  Must be non-negative.  May be evaluated more than once.
 * @param[in] s a struct scanner_s containing the rules defining the character's class.
 */
/*
 * There are additional code points that perhaps should be mapped to NO_CLASS in CIF 2: BMP not-a-character code points
 * 0xFDD0 - 0xFDEF, and per-plane not-a-character code points 0x??FFFE and 0x??FFFF except 0xFFFF (which is co-opted
 * as an EOF marker instead).  Also surrogate code units when not part of a surrogate pair.  At this time, however,
 * there appears little benefit to this macro making those comparatively costly distinctions, especially when it
 * cannot do so at all in the case of unpaired surrogates.
 */
#define CLASS_OF(c, s)  (((c) < CHAR_TABLE_MAX) ? (s)->char_class[c] : \
        (((c) == EOF_CHAR) ? EOF_CLASS : (((s)->cif_version >= 2) ? GENERAL_CLASS : NO_CLASS)))

/*
 * Evaluates the metaclass to which the class of the specified character belongs, with respect to the given scanner
 */
#define METACLASS_OF(c, s) ((s)->meta_class[CLASS_OF((c), (s))])

/* Expands to the length, in UChar code units, of the given scanner's current token */
#define TVALUE_LENGTH(s) ((s)->tvalue_length)

/* Sets the length, in UChar code units, of the given scanner's current token */
#define TVALUE_SETLENGTH(s, l) do { (s)->tvalue_length = (l); } while (CIF_FALSE)

/* Increments the length, in UChar code units, of the given scanner's current token */
#define TVALUE_INCLENGTH(s, n) do { (s)->tvalue_length += (n); } while (CIF_FALSE)

/* Expands to a pointer to the first UChar code unit in the given scanner's current token value */
#define TVALUE_START(s) ((s)->tvalue_start)

/* Sets a pointer to the first UChar code unit in the given scanner's current token value */
#define TVALUE_SETSTART(s, c) do { (s)->tvalue_start = (c); } while (CIF_FALSE)

/* Increments the given scanner's pointer to its current token value */
#define TVALUE_INCSTART(s, n) do { (s)->tvalue_start += (n); } while (CIF_FALSE)

/* expands to the value of the given scanner's current line number */
#define POSN_LINE(s) ((s)->line)

/* increments the given scanner's line number by the specified amount and resets its column number to zero */
#define POSN_INCLINE(s, n) do { (s)->line += (n); (s)->column = 0; } while (CIF_FALSE)

/* expands to the value of the given scanner's current column number */
#define POSN_COLUMN(s) ((s)->column)

/* increments the given scanner's column number by the specified amount */
#define POSN_INCCOLUMN(s, n) do { (s)->column += (n); } while (CIF_FALSE)

/* FIXME: consider dropping this macro in favor of calling get_more_chars() directly */
#define GET_MORE_CHARS(s, ev) do { ev = get_more_chars(s); } while (CIF_FALSE)

/*
 * Ensures that the buffer of the specified scanner has at least one character available to read; the available
 * character can be an EOF marker
 */
#define ENSURE_CHARS(s, ev) do { \
    struct scanner_s *_s_ec = (s); \
    if (_s_ec->next_char >= _s_ec->buffer + _s_ec->buffer_limit) { \
        GET_MORE_CHARS(_s_ec, ev); \
        if (ev != CIF_OK) break; \
    } else { \
        ev = CIF_OK; \
    } \
} while (CIF_FALSE)

/*
 * Inputs the next available character from the scanner source into the given character without changing the relative
 * token start position or recorded token length.  The absolute token start position may change, however. 
 */
#define NEXT_CHAR(s, c, ev) do { \
    struct scanner_s *_s_nc = (s); \
    ENSURE_CHARS(_s_nc, ev); \
    c = *(_s_nc->next_char++); \
} while (CIF_FALSE)

/*
 * Returns the character that will be returned (again) by the next invocation of NEXT_CHAR().  May cause additional
 * characters to be read into the buffer, but does not otherwise alter the scan state.
 */
#define PEEK_CHAR(s, c, ev) do { \
    struct scanner_s *_s_pc = (s); \
    ENSURE_CHARS(_s_pc, ev); \
    c = *(_s_pc->next_char); \
} while (CIF_FALSE)

/*
 * Consumes the current token, if any, from the scanner.  When next asked for a token, the scanner will provide
 * the subsequent one.
 */
#define CONSUME_TOKEN(s) do { \
    struct scanner_s *_s_ct = (s); \
    _s_ct->text_start = _s_ct->next_char; \
    TVALUE_SETSTART(_s_ct, _s_ct->next_char); \
    TVALUE_SETLENGTH(_s_ct, 0); \
} while (CIF_FALSE)

/*
 * Rejects the current token, if any, from the scanner, so that the same text is scanned again the next time a
 * token is requested.  This is useful only if the scanner is afterward modified so that the rejected token text
 * is scanned differently the next time.
 * s: the scanner
 * t: the type of the token immediately preceding the rejected one
 */
#define REJECT_TOKEN(s, t) do { \
    struct scanner_s *_s_rt = (s); \
    _s_rt->next_char = _s_rt->text_start; \
    TVALUE_SETSTART(_s_rt, _s_rt->next_char); \
    TVALUE_SETLENGTH(_s_rt, 0); \
    _s_rt->ttype = (t); \
} while (CIF_FALSE)

/*
 * Pushes back all but the specified number of initial characters of the current token to be scanned again as part of
 * the next token.  The result is undefined if more characters are pushed back than the current token contains.  The
 * length of the token value is adjusted so that the value extends to the new end of the remainder of the token.
 * s: the scanner in which to push back characters
 * n: the number of characters of the whole token text to keep (not just of the token value, if they differ)
 */
#define PUSH_BACK(s, n) do { \
    struct scanner_s *_s_pb = (s); \
    int32_t _n = (n); \
    _s_pb->next_char = _s_pb->text_start + _n; \
    TVALUE_SETLENGTH(_s_pb, _s_pb->next_char - TVALUE_START(_s_pb)); \
} while (CIF_FALSE)

/*
 * Advances the scanner by one code unit, tracking the current column number and watching for input malformations
 * associated with surrogate pairs: unpaired surrogates and pairs encoding Unicode code values not permitted in
 * CIF.  Surrogates have to be handled separately from character buffering in order to correctly handle surrogate
 * pairs that are split across buffer fills.
 * s: the scanner to manipulate
 * c: an lvalue in which to return the code unit read; evaluated multiple times
 * lead_fl: an lvalue of integral type wherein this macro can store state between runs; should evaluate to false
 *     before this macro's first run
 * ev: an lvalue in which an error code can be recorded
 */
#define SCAN_CHARACTER(s, c, lead_fl, ev) do { \
    struct scanner_s *_s = (s); \
    c = *(_s->next_char); \
    POSN_INCCOLUMN(_s, 1); \
    ev = CIF_OK; \
    switch (c & 0xfc00) { \
        case 0xd800: \
            lead_fl = CIF_TRUE; \
            break; \
        case 0xdc00: \
            if (lead_fl) { \
                if (((c & 0xfffe) == 0xdffe) && ((*(_s->next_char - 1) & 0xfc3f) == 0xd83f)) { \
                    /* TODO: error disallowed supplemental character */ \
                    /* recover by replacing with the replacement character and wiping out the lead surrogate */ \
                    memmove(_s->text_start + 1, _s->text_start, (_s->next_char - _s->text_start)); \
                    *(_s->next_char) = REPL_CHAR; \
                    c = REPL_CHAR; \
                    _s->text_start += 1; \
                    _s->tvalue_start += 1; \
                    POSN_INCCOLUMN(_s, -1); \
                } \
                lead_fl = CIF_FALSE; \
            } else { \
                /* TODO: error unpaired trail surrogate */ \
                /* recover by replacing with the replacement character */ \
                *(_s->next_char) = REPL_CHAR; \
                c = REPL_CHAR; \
            } \
            break; \
        default: \
            if (lead_fl) { \
                /* TODO: error unpaired lead surrogate preceding the current character */ \
                /* recover by replacing the surrogate with the replacement character */ \
                *(_s->next_char - 1) = REPL_CHAR; \
                lead_fl = CIF_FALSE; \
            } \
            break; \
    } \
    _s->next_char += 1; \
} while (CIF_FALSE)

/*
 * Expands to the lesser of its arguments.  The arguments may be evaluated more than once.
 */
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

/* function implementations */

/*
 * a parser error handler function that ignores all errors
 */
int cif_parse_error_ignore(int code UNUSED, size_t line UNUSED, size_t column UNUSED, const UChar *text UNUSED,
        size_t length UNUSED, void *data UNUSED) {
    return CIF_OK;
}

/*
 * a parser error handler function that rejects all erroneous CIFs with error code @c CIF_ERROR
 */
int cif_parse_error_die(int code UNUSED, size_t line UNUSED, size_t column UNUSED, const UChar *text UNUSED,
        size_t length UNUSED, void *data UNUSED) {
    return CIF_ERROR;
}

int cif_parse_internal(void *char_source, read_chars_t read_func, cif_t *dest, int version, int not_utf8,
        struct cif_parse_opts_s *options) {
    FAILURE_HANDLING;
    struct scanner_s scanner;

    if (read_func == NULL) {
        return CIF_ARGUMENT_ERROR;
    }

    scanner.buffer = (UChar *) malloc(BUF_SIZE_INITIAL * sizeof(UChar));
    scanner.buffer_size = BUF_SIZE_INITIAL;
    scanner.buffer_limit = 0;

    if (scanner.buffer != NULL) {
        UChar c;

        INIT_SCANNER(scanner);
        scanner.next_char = scanner.buffer;
        scanner.text_start = scanner.buffer;
        scanner.tvalue_start = scanner.buffer;
        scanner.tvalue_length = 0;
        scanner.char_source = char_source;
        scanner.read_func = read_func;
        scanner.line_unfolding += MIN(options->line_folding_modifier, 1);
        scanner.prefix_removing += MIN(options->text_prefixing_modifier, 1);

        NEXT_CHAR(&scanner, c, FAILURE_VARIABLE);
        if (FAILURE_VARIABLE == CIF_OK) {

            /* consume an initial BOM, if present */
            if (c == BOM_CHAR) {
                CONSUME_TOKEN(&scanner);
                NEXT_CHAR(&scanner, c, FAILURE_VARIABLE);
            }

            if (FAILURE_VARIABLE == CIF_OK) {
                /* If the CIF version is uncertain then use the CIF magic code, if any, to choose */
                if (version <= 0) {
                    if (CLASS_OF(c, &scanner) == HASH_CLASS) {
                        if ((FAILURE_VARIABLE = scan_to_ws(&scanner)) != CIF_OK) {
                            DEFAULT_FAIL(early);
                        }
                        if (TVALUE_LENGTH(&scanner) == MAGIC_LENGTH) {
                            if (u_strncmp(TVALUE_START(&scanner), CIF2_MAGIC, MAGIC_LENGTH) == 0) {
                                version = 2;
                                goto reset_scanner;
                            } else if (u_strncmp(TVALUE_START(&scanner), CIF1_MAGIC, MAGIC_LENGTH - 3) == 0) {
                                /* recognize magic codes for all CIF versions other than 2.0 as CIF 1 */
                                version = 1;
                                goto reset_scanner;
                            }
                        }
                    }
                    version = (version < 0) ? -version : 1;
                }
                reset_scanner:
                scanner.next_char = scanner.text_start;

                if (version == 1) {
                    SET_V1(scanner);
                } else if ((version == 2) && (not_utf8 != 0)) {
                    /* TODO: error CIF2 not UTF-8 */
                    /* recover by ignoring the problem */
                }

                SET_RESULT(parse_cif(&scanner, dest));
            }
        }

        FAILURE_HANDLER(early):
        free(scanner.buffer);
    }

    /* TODO: other cleanup ? */

    GENERAL_TERMINUS;
}

/*
 * Parse a while CIF via the provided scanner into the provided CIF object.  On success, all characters available from
 * the scanner will have been consumed.  The provided CIF object does not need to be empty, but semantic errors will
 * occur if it contains data blocks with block codes matching those of blocks read from the input.
 *
 * If the provided CIF handle is NULL then a syntax-only check is performed, but nothing is stored.  In that mode,
 * semantic constraints such as uniqueness of block codes, frame codes, and data names are not checked.
 */
static int parse_cif(struct scanner_s *scanner, cif_t *cif) {
    int result;

    do {
        cif_block_t *block = NULL;
        int32_t token_length;
        UChar *token_value;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        token_value = TVALUE_START(scanner);
        token_length = TVALUE_LENGTH(scanner);

        switch (scanner->ttype) {
            case BLOCK_HEAD:
                if (cif != NULL) {
                    /* insert a string terminator into the input buffer, after the current token */
                    UChar saved = *(token_value + token_length);
                    *(token_value + token_length) = 0;
    
                    result = cif_create_block(cif, token_value, &block);
                    switch (result) {
                        case CIF_INVALID_BLOCKCODE:
                            /* TODO: report syntax error */
                            /* recover by using the block code anyway */
                            if ((result = (cif_create_block_internal(cif, token_value, 1, &block)))
                                    != CIF_DUP_BLOCKCODE) {
                                break;
                            }
                            /* else result == CIF_DUP_BLOCKCODE, so fall through */
                        case CIF_DUP_BLOCKCODE:
                            /* TODO: report data error */
                            /* recover by using the existing block */
                            result = cif_get_block(cif, token_value, &block);
                            break;
                        /* default: do nothing */
                    }
        
                    /* restore the next character in the input buffer */
                    *(token_value + token_length) = saved;
                } /* else syntax check only; no actual block will be created */

                CONSUME_TOKEN(scanner);
    
                break;
            case END:
                /* it's more useful to leave the token than to consume it */
                goto cif_end;
            default:
                /* TODO: error missing data block header */
                /* recover by creating and installing an anonymous block context to eat up the content */
                if (cif != NULL) {
                    if ((result = cif_create_block_internal(cif, &cif_uchar_nul, 1, &block)) == CIF_DUP_BLOCKCODE) {
                        /*
                         * or fall back to using an anonymous context that already exists; this should happen only if
                         * the user provided a non-empty initial CIF
                         */
                        result = cif_get_block(cif, &cif_uchar_nul, &block);
                    }
                }
    
                break;
        }

        if (result != CIF_OK) {
            break;
        }

        result = parse_container(scanner, block, CIF_TRUE);
        if (block != NULL) {
            cif_block_free(block); /* ignore any error */
        }
    } while (result == CIF_OK);

    cif_end:
    return result;
}

static int parse_container(struct scanner_s *scanner, cif_container_t *container, int is_block) {
    int result;

    do {
        int32_t token_length;
        UChar *token_value;
        UChar *name;
        enum token_type alt_ttype = QVALUE;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        token_value = TVALUE_START(scanner);
        token_length = TVALUE_LENGTH(scanner);

        switch (scanner->ttype) {
            case BLOCK_HEAD:
                if (!is_block) {
                    /* TODO: error: unterminated save frame */
                    /* recover by continuing */
                }
                /* reject the token */
                result = CIF_OK;
                goto container_end;
            case FRAME_HEAD:
                if (!is_block) {
                    /* TODO: error: unterminated save frame */
                    /* recover by rejecting the token */
                    result = CIF_OK;
                    goto container_end;
                } else {
                    cif_container_t *frame = NULL;
                    UChar saved;
    
                    /* insert a string terminator into the input buffer, after the current token */
                    saved = *(token_value + token_length);
                    *(token_value + token_length) = 0;
   
                    if (container == NULL) {
                        result = CIF_OK;
                    } else { 
                        result = cif_block_create_frame(container, token_value, &frame);
                        switch (result) {
                            case CIF_INVALID_FRAMECODE:
                                /* TODO: report syntax error */
                                /* recover by using the frame code anyway */
                                if ((result = (cif_block_create_frame_internal(container, token_value, 1, &frame)))
                                        != CIF_DUP_FRAMECODE) {
                                    break;
                                }
                                /* else result == CIF_DUP_BLOCKCODE, so fall through */
                            case CIF_DUP_FRAMECODE:
                                /* TODO: report data error */
                                /* recover by using the existing frame */
                                result = cif_block_get_frame(container, token_value, &frame);
                                break;
                            /* default: do nothing */
                        }
                    }
    
                    /* restore the next character in the input buffer */
                    *(token_value + token_length) = saved;
                    CONSUME_TOKEN(scanner);
    
                    if (result == CIF_OK) {
                        result = parse_container(scanner, frame, CIF_FALSE);
                    }
                }
                break;
            case FRAME_TERM:
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                if (is_block) {
                    /* TODO: error unexpected frame terminator */
                    /* recover by dropping the token */
                } else {
                    /* close the context */
                    goto container_end;
                }
                break;
            case LOOPKW:
                CONSUME_TOKEN(scanner);
                result = parse_loop(scanner, container);
                break;
            case NAME:
                name = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                if (name == NULL) {
                    result = CIF_ERROR;
                    goto container_end;
                } else {
                    /* copy the data name to a separate Unicode string in the item context */
                    u_strncpy(name, token_value, token_length);
                    name[token_length] = 0;
                    CONSUME_TOKEN(scanner);
    
                    /* check for dupes */
                    result = ((container == NULL) ? CIF_NOSUCH_ITEM
                            : cif_container_get_item_loop(container, name, NULL));
                    if (result == CIF_OK) {
                        /* TODO: error duplicate name */
                        /* recover by rejecting the item (but still parsing the associated value) */
                        result = parse_item(scanner, container, NULL);
                    } else if (result == CIF_NOSUCH_ITEM) {
                        result = parse_item(scanner, container, name);
                    }

                    free(name);
                }
                break;
            case TKEY:
                alt_ttype = TVALUE;
                /* fall through */
            case KEY:
                /* TODO: error missing whitespace */
                /* recover by pushing back the colon */
                scanner->next_char -= 1;
                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                /* fall through */
            case TVALUE:
            case QVALUE:
            case VALUE:
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
                /* TODO: error unexpected value */
                /* recover by consuming and discarding the value */
                result = parse_item(scanner, container, NULL);
                break;
            case CTABLE:
            case CLIST:
                /* TODO: error unexpected closing delimiter */
                /* recover by dropping it */
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                break;
            case END:
                if (!is_block) {
                    /* TODO: error unterminated save frame */
                    /* recover by rejecting the token (as would happen anyway) */
                }
                /* no special action, just reject the token */
                result = CIF_OK;
                goto container_end;
            default:
                /* should not happen */
                result = CIF_INTERNAL_ERROR;
                break;
        }

    } while (result == CIF_OK);

    container_end:
    /* no cleanup needed here */
    return result;
}

static int parse_item(struct scanner_s *scanner, cif_container_t *container, UChar *name) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_t *value = NULL;
        enum token_type alt_ttype = QVALUE;
    
        switch (scanner->ttype) {
            case TKEY:
                alt_ttype = TVALUE;
                /* fall through */
            case KEY:
                /* TODO: error missing whitespace */
                /* recover by pushing back the colon */
                scanner->next_char -= 1;
                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                /* fall through */
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
            case TVALUE:
            case QVALUE:
            case VALUE:
                /* parse the value */
                result = parse_value(scanner, &value);
                break;
            default:
                /* TODO: error missing value */
                /* recover by inserting a synthetic unknown value */
                result = cif_value_create(CIF_UNK_KIND, &value);
                /* do not consume the token */
                break;
        }
    
        if (result == CIF_OK) {
            if ((name != NULL) && (container != NULL)) {
                /* _copy_ the value into the CIF */
                result = cif_container_set_value(container, name, value);
            }
            cif_value_free(value); /* ignore any error */
        }
    }

    return result;
}

static int parse_loop(struct scanner_s *scanner, cif_container_t *container) {
    string_element_t *first_name = NULL;          /* the first data name in the header */
    string_element_t **next_namep = &first_name;  /* a pointer to the pointer to the next data name in the header */
    int name_count = 0;
    int result;

    /* parse the header */
    while (((result = next_token(scanner)) == CIF_OK) && (scanner->ttype == NAME)) {
        int32_t token_length = TVALUE_LENGTH(scanner);
        UChar *token_value = TVALUE_START(scanner);

        *next_namep = (string_element_t *) malloc(sizeof(string_element_t));
        if (*next_namep == NULL) {
            result = CIF_ERROR;
            goto loop_end;
        } else {
            (*next_namep)->next = NULL;
            (*next_namep)->string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
            if ((*next_namep)->string == NULL) {
                result = CIF_ERROR;
                goto loop_end;
            } else {
                u_strncpy((*next_namep)->string, token_value, token_length);
                (*next_namep)->string[token_length] = 0;
                next_namep = &((*next_namep)->next);
                name_count += 1;
            }
        }

        CONSUME_TOKEN(scanner);
    }

    if (result == CIF_OK) {
        /* name_count is the number of data names parsed for the header */
        if (name_count == 0) {
            /* TODO: error empty loop header */
            /* recover by ignoring it */
        } else {
            /* an array of data names for creating the loop; elements belong to the linked list, not this array */
            UChar **names = (UChar **) malloc((name_count + 1) * sizeof(UChar *));

            if (names == NULL) {
                result = CIF_ERROR;
                goto loop_end;
            } else {
                cif_loop_t *loop = NULL;
                string_element_t *next_name;
                cif_packet_t *packet;
                int name_index = 0;
                
                /* validate the header and create the loop */

                name_index = 0;
                for(next_name = first_name; next_name != NULL; next_name = next_name->next) {
                    switch (result = ((container == NULL) ? CIF_NOSUCH_ITEM
                            : cif_container_get_item_loop(container, next_name->string, NULL))) {
                        case CIF_OK:
                            /* TODO: error duplicate item */
                            /* recover by ignoring the name and associated values in the loop to come */
                            /* a place-holder is retained in the list so that packet values are counted and assigned
                               correctly */
                            free(next_name->string);
                            next_name->string = NULL;
                            break;
                        case CIF_NOSUCH_ITEM:
                            /* the expected case */
                            names[name_index++] = next_name->string;
                            break;
                        default:
                            goto loop_body_end;
                    }
                }
                names[name_index] = NULL;

                if (container != NULL) {
                    /* create the loop */
                    switch (result = cif_container_create_loop(container, NULL, names, &loop)) {
                        case CIF_OK:         /* expected */
                        case CIF_NULL_LOOP:  /* tolerable */
                            break;
                        case CIF_INVALID_ITEMNAME:
                        case CIF_DUP_ITEMNAME:
                            /* these should not happen because the names were already validated */
                            result = CIF_INTERNAL_ERROR;
                            /* fall through */
                        default:
                            goto loop_body_end;
                    }
                }

                /* read packets */
                if ((result = cif_packet_create(&packet, names)) == CIF_OK) {
                    int have_packets = CIF_FALSE;

                    next_name = first_name;
                    while ((result = next_token(scanner)) == CIF_OK) {
                        UChar *name;
                        cif_value_t *value = NULL;
                        enum token_type alt_ttype = QVALUE;

                        switch (scanner->ttype) {
                            case TKEY:
                                alt_ttype = TVALUE;
                                /* fall through */
                            case KEY:
                                /* TODO: error missing whitespace */
                                /* recover by pushing back the colon */
                                scanner->next_char -= 1;
                                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                                /* fall through */
                            case OLIST:  /* opening delimiter of a list value */
                            case OTABLE: /* opening delimiter of a table value */
                            case TVALUE:
                            case QVALUE:
                            case VALUE:
                                name = next_name->string;
                                next_name = next_name->next;

                                if (name != NULL) {
                                    if (((result = cif_packet_set_item(packet, name, NULL)) != CIF_OK)
                                            || ((result = cif_packet_get_item(packet, name, &value)) != CIF_OK)) {
                                        goto packets_end;
                                    }
                                }

                                /* parse the value */
                                result = parse_value(scanner, &value);
                                if (name == NULL) {
                                    /* discard the value -- the associated name was a duplicate or failed validation */
                                    cif_value_free(value);  /* ignore any error */
                                } /* else the value is already in the packet and belongs to it */

                                if (result != CIF_OK) { /* this is the parse_value() result code */
                                    goto packets_end;
                                } else if (next_name == NULL) {  /* that was the last value in the packet */
                                    if ((loop != NULL) && ((result = cif_loop_add_packet(loop, packet)) != CIF_OK)) {
                                        goto packets_end;
                                    }
                                    have_packets = CIF_TRUE;
                                    next_name = first_name;
                                }

                                break;
                            case CLIST:
                            case CTABLE:
                                /* TODO: error unexpected list/table delimiter */
                                /* recover by dropping it */
                                CONSUME_TOKEN(scanner);
                                break;
                            default:
                                if (next_name != first_name) {
                                    /* TODO: error partial packet */
                                    /* recover by synthesizing unknown values to fill the packet, and saving it */
                                    if (loop != NULL) {
                                        for (; next_name != NULL; next_name = next_name->next) {
                                            if ((next_name->string != NULL)
                                                    && (result = cif_packet_set_item(packet, next_name->string, NULL))
                                                            != CIF_OK) {
                                                goto packets_end;
                                            }
                                        }
                                        if ((result = cif_loop_add_packet(loop, packet)) == CIF_OK) {
                                            goto packets_end;
                                        }
                                    }
                                } else if (!have_packets) {
                                    /* TODO: error no packets */
                                    /* recover by doing nothing for now */
                                    /*
                                     * JCB note: a loop without packets is not valid in the data model, but the data
                                     * names need to be retained until the container has been fully parsed to allow
                                     * duplicates to be detected correctly.  Probably some kind of pruning is needed
                                     * after the container is fully parsed.
                                     */
                                }
                                /* the token is not examined in any way */
                                goto packets_end;
                        } /* end switch */
                    } /* end while */

                    packets_end:
                    cif_packet_free(packet);
                }

                loop_body_end:
                free(names);
                /* elements of names belong to the linked list; they are freed later */
            } /* end ifelse (names) */
        } /* end ifelse (name_count) */
    } /* end if (header result) */

    loop_end:

    /* clean up the header name linked list */
    while (first_name != NULL) {
        string_element_t *next = first_name->next;

        if (first_name->string != NULL) {
            free(first_name->string);
        }
        free(first_name);
        first_name = next;
    }

    return result;
}

static int parse_list(struct scanner_s *scanner, cif_value_t **listp) {
    cif_value_t *list = NULL;
    size_t next_index = 0;
    int result;

    if (*listp == NULL) {
        /* create a new value object */
        result = cif_value_create(CIF_LIST_KIND, &list);
    } else {
        /* use the existing value object */
        list = *listp;
        result = cif_value_init(list, CIF_LIST_KIND);
    }

    while (result == CIF_OK) {
        cif_value_t *element = NULL;
        enum token_type alt_ttype = QVALUE;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }

        switch (scanner->ttype) {
            case TKEY:
                alt_ttype = TVALUE;
                /* fall through */
            case KEY:
                /* TODO: error missing whitespace */
                /* recover by pushing back the colon */
                scanner->next_char -= 1;
                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                /* fall through */
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
            case TVALUE:
            case QVALUE:
            case VALUE:
                /* Insert a dummy value, then update it with the parse result.  This minimizes copying. */
                result = cif_value_insert_element_at(list, next_index, NULL);
                switch ((result == CIF_OK) ? (result = cif_value_get_element_at(list, next_index++, &element))
                        : result) {
                    case CIF_OK:
                        /* element now points to a list element (not a copy). Parse the value into it. */
                        result = parse_value(scanner, &element);
                        break;
                    case CIF_ARGUMENT_ERROR:
                    case CIF_INVALID_INDEX:
                        /* should not happen */
                        result = CIF_INTERNAL_ERROR;
                        break;
                }
                break;
            case CLIST:
                /* accept the token and end the list */
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                goto list_end;
            default:
                /* TODO: error unterminated list */
                /* recover by synthetically closing the list */
                /* do not consume the token */
                result = CIF_OK;
                goto list_end;
        }
    } /* while */

    list_end:

    if (result == CIF_OK) {
        *listp = list;
    } else if (list != *listp) {
        cif_value_free(list);
    }

    return result;
}

static int parse_table(struct scanner_s *scanner, cif_value_t **tablep) {
    /*
     * Implementation note: this function is designed to minimize the damage caused by input malformation.  If a key or value is dropped, or if
     * a key is turned into a value by removing its colon or inserting whitespace before it, then only one table entry is lost.  Furthermore,
     * this function recognizes and handles unquoted keys, provided that the configured error callback does not abort parsing when notified
     * of the problem.
     */
    cif_value_t *table;
    int result;

    assert (tablep != NULL);
    if (*tablep == NULL) {
        result = cif_value_create(CIF_TABLE_KIND, &table);
    } else {
        table = *tablep;
        result = cif_value_init(table, CIF_TABLE_KIND);
    }

    while (result == CIF_OK) {
        /* parse (key, value) pairs */
        UChar *key = NULL;
        cif_value_t *value = NULL;

        /* scan the next key, if any */

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        switch (scanner->ttype) {
            case VALUE:
                if (*TVALUE_START(scanner) == COLON_CHAR) {
                    /* TODO error null key */
                    /* recover by handling it as a NULL (not empty) key */
                    if (TVALUE_LENGTH(scanner) > 1) {
                        PUSH_BACK(scanner, 1);
                    }
                    CONSUME_TOKEN(scanner);
                    break;
                } else {
                    size_t index;

                    /*
                     * an unquoted string is not valid here, but defer reporting the error until we know whether it is
                     * a stray value or an unquoted key.
                     */
                    for (index = 1; index < TVALUE_LENGTH(scanner); index += 1) {
                        if (TVALUE_START(scanner)[index] == COLON_CHAR) {
                            /* TODO: error unquoted key */
                            /* recover by accepting everything before the first colon as a key */
                            /* push back all characters following the first colon */
                            PUSH_BACK(scanner, index + 1);
                            TVALUE_SETLENGTH(scanner, index);
                            scanner->ttype = KEY;
                        }
                    }
                    if (scanner->ttype != KEY) {
                        /* TODO error missing key */
                        /* recover by dropping the value and continuing to the next entry */
                        CONSUME_TOKEN(scanner);
                        continue;
                    } /* else fall through */
                }
                /* fall through */
            case KEY:  /* this and CTABLE are the only genuinely valid token types here */
                /* copy the key to a NUL-terminated Unicode string */
                key = (UChar *) malloc((TVALUE_LENGTH(scanner) + 1) * sizeof(UChar));
                if (key != NULL) {
                    u_strncpy(key, TVALUE_START(scanner), TVALUE_LENGTH(scanner));
                    key[TVALUE_LENGTH(scanner)] = 0;
                    CONSUME_TOKEN(scanner);    
                    break;
                }
                result = CIF_ERROR;
                goto table_end;
            case TKEY:
                /* TODO error invalid key type */
                /* extract the text value as a NUL-terminated Unicode string */
                if ((result = decode_text(scanner, TVALUE_START(scanner), TVALUE_LENGTH(scanner), &value)) == CIF_OK) {
                    result = cif_value_get_text(value, &key);
                    cif_value_free(value); /* ignore any error */
                    CONSUME_TOKEN(scanner);
                    if (result == CIF_OK) {
                        break;
                    }
                }
                goto table_end;
            case QVALUE:
            case TVALUE:
                /* TODO error: unexpected value */
                /* recover by dropping it */
                CONSUME_TOKEN(scanner);
                continue;
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
                /* TODO: error unexpected value */
                /* recover by accepting and dropping the value */
                if ((result = parse_value(scanner, &value)) == CIF_OK) {
                    /* token is already consumed by parse_value() */
                    cif_value_free(value);  /* ignore any error */
                    continue; 
                }
                goto table_end;
            case CTABLE:
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                goto table_end;
            default:
                /* TODO: error unterminated table */
                /* recover by ending the table; the token is left for the calling context to handle */
                result = CIF_OK;
                goto table_end;
        }

        /* scan the value */

        /* obtain a value object into which to scan the value, so as to avoid copying the scanned value after the fact */
        if ((key != NULL)
                && (((result = cif_value_set_item_by_key(table, key, NULL)) != CIF_OK) 
                        || ((result = cif_value_get_item_by_key(table, key, &value)) != CIF_OK))) {
            break;
        }

        if ((result = next_token(scanner)) == CIF_OK) {
            switch (scanner->ttype) {
                case OLIST:  /* opening delimiter of a list value */
                case OTABLE: /* opening delimiter of a table value */
                case TVALUE:
                case QVALUE:
                case VALUE:
                    /* parse the incoming value into the existing value object */
                    result = parse_value(scanner, &value);
                    break;
                default:
                    /* TODO: error missing value */
                    /* recover by using an unknown value (already set) */
                    /* do not consume the token here */
                    break;
            }
        }

        if (key == NULL) {
            cif_value_free(value); /* ignore any error */
        } else {
            /* the value belongs to the table */
            free(key);
        }
    } /* while */

    table_end:
    return result;
}

/*
 * Parses a value of any of the supported types.  The next token must represent a value, or the start of one.
 */
static int parse_value(struct scanner_s *scanner, cif_value_t **valuep) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_t *value = *valuep;

        /* build a value object or modify the provided one, as appropriate */
        if ((value != NULL) || ((result = cif_value_create(CIF_UNK_KIND, &value)) == CIF_OK)) {
            UChar *token_value = TVALUE_START(scanner);
            size_t token_length = TVALUE_LENGTH(scanner);

            switch (scanner->ttype) {
                case OLIST: /* opening delimiter of a list value */
                    CONSUME_TOKEN(scanner);
                    result = parse_list(scanner, &value);
                    break;
                case OTABLE: /* opening delimiter of a table value */
                    CONSUME_TOKEN(scanner);
                    result = parse_table(scanner, &value);
                    break;
                case TVALUE:
                    /* parse text block contents into a value */
                    result = decode_text(scanner, token_value, token_length, &value);
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    break;
                case QVALUE:
                    /* overwrite the closing delimiter with a string terminator */
                    token_value[token_length - 1] = 0;
                    /* advance token start past the opening delimiter */
                    token_value += 1;
                    /* copy the input string into the value object */
                    result = cif_value_copy_char(value, token_value);
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                   break;
                case VALUE:
                    /* special cases for unquoted question mark (?) and period (.) */
                    if (token_length == 1) {
                        if (*token_value == QUERY_CHAR) {
                            result = cif_value_init(value, CIF_UNK_KIND);
                            goto value_end;
                        }  else if (*token_value == DECIMAL_CHAR) {
                            result = cif_value_init(value, CIF_NA_KIND);
                            goto value_end;
                        } /* else an ordinary value */
                    }
        
                    if ((result = cif_value_parse_numb(value, token_value)) == CIF_INVALID_NUMBER) {
                        /* failed to parse the value as a number; record it as a string */
                        int32_t temp_length = token_length + 1;
                        UChar *temp = (UChar *) malloc((temp_length) * sizeof(UChar));

                        if (temp == NULL) {
                            result = CIF_ERROR;
                        } else {
                            /* copy the token value into a separate buffer, with terminator */
                            *temp = 0;
                            u_strncat(temp, token_value, temp_length);  /* ensures NUL termination */
                            /* assign the terminated string into the value object */
                            if ((result = cif_value_init_char(value, temp)) != CIF_OK) {
                                /* assignment failed; free the string */
                                free(temp);
                            }
                        }
                    }

                    value_end:
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    break;
                default:
                    /* This parse function should be called only when the incoming token is or starts a value */
                    result = CIF_INTERNAL_ERROR;
                    break;
            }

            if (result == CIF_OK) {
                *valuep = value;
            } else if (*valuep != value) {
                cif_value_free(value);  /* ignore any error */
            }
        }
    }

    return result;
}


/*
 * Decodes the contents of a text block by un-prefixing and unfolding lines as appropriate, and standardizing line
 * terminators to a single newline character.  Records the result in a CIF value object.
 *
 * If dest points to a non-NULL value handle then the associated value object is modified
 * (on success) to be of kind CIF_CHAR_KIND, holding the parsed text.  Otherwise, a new value
 * object is created and its handle recorded where dest points.
 */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_t **dest) {
    FAILURE_HANDLING;
    cif_value_t *value = *dest;
    int result = (value == NULL) ? cif_value_create(CIF_UNK_KIND, &value) : CIF_OK;

    if (result != CIF_OK) {
        /* failed to create a value object */
        return result;
    } else {
        if (text_length <= 0) {
            /* update the value object with an empty char value */
            if ((result = cif_value_init(value, CIF_CHAR_KIND)) != CIF_OK) {
                FAIL(early, CIF_ERROR);
            } else {
                *dest = value;
                return CIF_OK;
            }
        } else {
            UChar *buffer = (UChar *) malloc((text_length + 1) * sizeof(UChar));

            if (buffer != NULL) {
                /* analyze the text and copy its logical contents into the buffer */
                UChar *buf_pos = buffer;
                UChar *in_pos = text;
                UChar *in_limit = text + text_length; /* input boundary pointer */
                int32_t prefix_length;
                int folded;

                if ((*text == SEMI_CHAR) || ((scanner->line_unfolding <= 0) && (scanner->prefix_removing <= 0))) {
                    /* text beginning with a semicolon is neither prefixed nor folded */
                    prefix_length = 0;
                    folded = CIF_FALSE;
                } else {
                    /* check for text prefixing and/or line-folding */
                    int backslash_count = 0;
                    int nonws = CIF_FALSE;  /* whether there is any non-whitespace between the last backslash and EOL */

                    /* scan the first line of the text for a prefix and/or line-folding signature */
                    prefix_length = -1;
                    for (; in_pos < in_limit; in_pos += 1) {
                        UChar c = *(in_pos++);

                        /* prospectively copy the input character to the buffer */
                        *(buf_pos++) = c;

                        /* update statistics */
                        switch (c) {
                            case BKSL_CHAR:
                                prefix_length = (in_pos - text) - 1; /* the number of characters preceding this one */
                                backslash_count += 1;                /* the total number of backslashes on this line */
                                nonws = CIF_FALSE;                   /* reset the nonws flag */
                                break;
                            case CR_CHAR:
                                /* convert CR and CR LF to LF */
                                *(buf_pos - 1) = LF_CHAR;
                                if ((in_pos < in_limit) && (*in_pos == LF_CHAR)) {
                                    in_pos += 1;
                                }
                                /* fall through */
                            case LF_CHAR:
                                goto end_of_line;
                            default:
                                if (CLASS_OF(c, scanner) != WS_CLASS) {
                                    nonws = CIF_TRUE;
                                }
                                break;
                        }
                    }

                    end_of_line:
                    /* analyze the results of the scan of the first line */
                    prefix_length = ((scanner->prefix_removing > 0) ? (prefix_length + 1 - backslash_count) : 0);
                    if (!nonws && ((backslash_count == 1)
                            || ((backslash_count == 2) && (prefix_length > 0) && (text[prefix_length] == BKSL_CHAR)))) {
                        /* prefixed or folded or both */
                        folded = (((prefix_length == 0) || (backslash_count == 2)) && (scanner->line_unfolding > 0));

                        /* discard the text already copied to the buffer */
                        buf_pos = buffer;
                    } else {
                        /* neither prefixed nor folded */
                        prefix_length = 0;
                        folded = CIF_FALSE;
                        /* keep the buffered text */
                    }
                }

                if (!folded && (prefix_length == 0)) {
                    /* copy the rest of the text, performing line terminator conversion as appropriate */
                    UChar *in_start = in_pos;
                    int32_t length;

                    while (in_pos < in_limit) {
                        UChar c = *in_pos;

                        if (c == CR_CHAR) {
                            length = in_pos++ - in_start;
                            u_strncpy(buf_pos, in_start, length);
                            buf_pos += length;
                            *(buf_pos++) = LF_CHAR;
                            if ((in_pos < in_limit) && (*in_pos == LF_CHAR)) {
                                in_pos += 1;
                            }
                            in_start = in_pos;
                        } else {
                            in_pos += 1;
                        }
                    }

                    length = in_limit - in_start;
                    u_strncpy(buf_pos, in_start, length);
                    buf_pos[length] = 0;
                } else {

                    /* process the text line-by-line, confirming and consuming prefixes and unfolding as appropriate */
                    while (in_pos < in_limit) { /* each line */
                        UChar *buf_temp = NULL;

                        /* consume the prefix, if any */
                        if (((in_limit - in_pos) <= prefix_length) && (u_strncmp(text, in_pos, prefix_length) == 0)) {
                            in_pos += prefix_length;
                        } else {
                            /* TODO: error missing prefix */
                            /* recover by copying the un-prefixed text (no special action required for that) */
                        }

                        /* copy from input to buffer, up to and including EOL, accounting for folding if applicable */
                        while (in_pos < in_limit) { /* each UChar unit */
                            UChar c = *(in_pos++);

                            /* copy the character unconditionally */
                            *(buf_pos++) = c;

                            /* handle line folding where appropriate */
                            if ((c == BKSL_CHAR) && folded) {
                                /* buf_temp tracks the position of the backslash in the output buffer */
                                buf_temp = buf_pos - 1;
                            } else {
                                int class = CLASS_OF(c, scanner);

                                if (class == EOL_CLASS) {
                                    /* convert CR and CR LF to LF */
                                    if (c == CR_CHAR) {
                                        *(buf_pos - 1) = LF_CHAR;
                                        if ((in_pos < in_limit) && (*in_pos == LF_CHAR)) {
                                            in_pos += 1;
                                        }
                                    }

                                    /* if appropriate, rewind the output buffer to achieve line folding */
                                    if (buf_temp != NULL) {
                                        buf_pos = buf_temp;
                                        buf_temp = NULL;
                                    }

                                    break;
                                } else if (class != WS_CLASS) {
                                    buf_temp = NULL;
                                }
                            }
                        }
                    }

                    /* add a string terminator */
                    *buf_pos = 0;
                }

                /* assign the buffer to the value object */
                result = cif_value_init_char(value, buffer);
                if (result == CIF_OK) {
                    /* The value is successfully parsed / initialized; assign it to the output variable */
                    *dest = value;
                    return CIF_OK;
                } else {
                    SET_RESULT(result);
                }

                /* clean up the buffer */
                free(buffer);
            } /* else failed to allocate the text buffer */
        }

        FAILURE_HANDLER(early):
        /* clean up the value */
        if (value != *dest) cif_value_free(value); /* ignore any error */
    }

    FAILURE_TERMINUS;
}

/*
 * Ensures that the provided scanner has the next available token identified and classified.
 */
static int next_token(struct scanner_s *scanner) {
    int result = CIF_OK;
    int last_ttype = scanner->ttype;

    /* any token may follow these without intervening whitespace: */
    int after_ws = ((last_ttype == END) || (last_ttype == OLIST) || (last_ttype == OTABLE) || (last_ttype == KEY) || (last_ttype == TKEY));

    while (scanner->text_start >= scanner->next_char) { /* else there is already a token waiting */
        enum token_type ttype;
        UChar c;
        int clazz;

        TVALUE_SETSTART(scanner, scanner->text_start);
        TVALUE_SETLENGTH(scanner, 0);
        NEXT_CHAR(scanner, c, result);
        if (result != CIF_OK) {
            break;
        }

        /* Require whitespace separation between most tokens, but not between keys and values (where it is forbidden) */
        clazz = CLASS_OF(c, scanner);
        if (!after_ws) {
            switch (scanner->meta_class[clazz]) {
                case WS_META:
                case CLOSE_META:
                    /* do nothing: whitespace is never required before whitespace or closing brackets / braces */
                    break;
                default:
                    /* TODO: error missing whitespace */
                    /* recover by assuming the missing whitespace */
                    break;
            }
        }

        switch (clazz) {
            case WS_CLASS:
            case EOL_CLASS:
                result = scan_ws(scanner);
                /* FIXME: report to listener, if any */
                if (result == CIF_OK) {
                    CONSUME_TOKEN(scanner);
                    after_ws = CIF_TRUE;
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
            case HASH_CLASS:
                result = scan_to_eol(scanner);
                /* FIXME: report to listener, if any */
                if (result == CIF_OK) {
                    CONSUME_TOKEN(scanner);
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
            case EOF_CLASS:
                ttype = END;
                result = CIF_OK;
                break;
            case UNDERSC_CLASS:
                ttype = NAME;
                result = scan_to_ws(scanner);
                break;
            case OBRAK_CLASS:
                ttype = OLIST;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case CBRAK_CLASS:
                ttype = CLIST;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case OBRAK1_CLASS:
            case CBRAK1_CLASS:
                /* TODO error: disallowed start char for a ws-delimited value */
                /* recover by scanning it as the start of a ws-delimited value anyway */
                result = scan_unquoted(scanner);
                ttype = VALUE;
                break;
            case OCURL_CLASS:
                ttype = OTABLE;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case CCURL_CLASS:
                ttype = CTABLE;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case QUOTE_CLASS:
                result = scan_delim_string(scanner);

                if (result == CIF_OK) {
                    /* determine whether this is a value or a table key */

                    /* peek ahead at the next character to see whether this looks like a table key */
                    PEEK_CHAR(scanner, c, result);

                    if (result == CIF_OK) {
                        if (c == COLON_CHAR) {
                            /* can only happen in CIF 2 mode, for in CIF 1 mode the character after a closing quote is always whitespace */
                            scanner->next_char += 1;
                            ttype = KEY;
                        } else {
                            ttype = QVALUE;
                        }

                        /* either way, the logical token value starts after the opening quote */
                        TVALUE_INCSTART(scanner, 1);
                    }
                }
                break;
            case SEMI_CLASS:
                if (POSN_COLUMN(scanner) == 1) {  /* semicolon was in column 1 */
                    if ((result = scan_text(scanner)) == CIF_OK) {
                        TVALUE_INCSTART(scanner, 1);
                        ttype = TVALUE;

                        if (scanner->cif_version >= 2) {
                            /* peek ahead at the next character to see whether this looks like a (bogus) table key */
                            PEEK_CHAR(scanner, c, result);

                            if (result == CIF_OK) {
                                if (c == COLON_CHAR) {
                                    scanner->next_char += 1;
                                    ttype = TKEY;
                                }
                            }
                        }
                    }
                } else {
                    /* exactly as the default case, but we can't fall through */
                    result = scan_unquoted(scanner);
                    ttype = VALUE;
                }
                break;
            case DOLLAR_CLASS:
                /* TODO: error frame reference */
                /* recover by accepting it as a ws-delimited string */
                /* fall through */
            default:
                result = scan_unquoted(scanner);
                ttype = VALUE;
                break;
        }

        if (result == CIF_OK) {    
            if (ttype == VALUE) {

                /*
                 * Case-insensitive and locale- and codepage-independent detection of reserved words.
                 * It's a bit verbose, but straightforward.
                 */

                UChar *token = TVALUE_START(scanner);
                size_t token_length = scanner->tvalue_length;

                if ((token_length > 4) && (CLASS_OF(token[4], scanner) == UNDERSC_CLASS)) {
                    /* check for data_*, save_*, loop_, and stop_ */
                    if (         (CLASS_OF(*token, scanner) == D_CLASS)
                            && (CLASS_OF(token[1], scanner) == A_CLASS)
                            && (CLASS_OF(token[2], scanner) == T_CLASS)
                            && (CLASS_OF(token[3], scanner) == A_CLASS)) {
                        if (token_length == 5) {
                            /* TODO: error missing block code / reserved word */
                            /* recover by dropping it */
                            CONSUME_TOKEN(scanner);
                        } else {
                            ttype = BLOCK_HEAD;
                            TVALUE_INCSTART(scanner, 5);
                            TVALUE_INCLENGTH(scanner, -5);
                        }
                    } else if (  (CLASS_OF(*token, scanner) == S_CLASS)
                            && (CLASS_OF(token[1], scanner) == A_CLASS)
                            && (CLASS_OF(token[2], scanner) == V_CLASS)
                            && (CLASS_OF(token[3], scanner) == E_CLASS)) {
                        ttype = ((token_length == 5) ? FRAME_TERM : FRAME_HEAD);
                        TVALUE_INCSTART(scanner, 5);
                        TVALUE_INCLENGTH(scanner, - 5);
                    } else if ((token_length == 5)
                            && (CLASS_OF(token[2], scanner) == O_CLASS)
                            && (CLASS_OF(token[3], scanner) == P_CLASS)) {
                        if (         (CLASS_OF(*token, scanner) == L_CLASS)
                                && (CLASS_OF(token[1], scanner) == O_CLASS)) {
                            ttype = LOOPKW;
                            TVALUE_INCSTART(scanner, 5);
                            TVALUE_INCLENGTH(scanner, - 5);
                        } else if (  (CLASS_OF(*token, scanner) == S_CLASS)
                                && (CLASS_OF(token[1], scanner) == T_CLASS)) {
                            /* TODO: error 'stop' reserved word */
                            /* recover by dropping it */
                            CONSUME_TOKEN(scanner);
                        } 
                    }
                } else if ((token_length == 7) && (CLASS_OF(token[6], scanner) == UNDERSC_CLASS)
                        &&   (CLASS_OF(*token, scanner) == G_CLASS)
                        && (CLASS_OF(token[1], scanner) == L_CLASS)
                        && (CLASS_OF(token[2], scanner) == O_CLASS)
                        && (CLASS_OF(token[3], scanner) == B_CLASS)
                        && (CLASS_OF(token[4], scanner) == A_CLASS)
                        && (CLASS_OF(token[5], scanner) == L_CLASS)) {
                    /* TODO: error 'global' reserved word */
                    /* recover by ignoring it */
                    CONSUME_TOKEN(scanner);
                }
            } /* endif ttype == VALUE */
        } /* endif result == CIF_OK */
    } /* end while */

    return result;
}

static int scan_ws(struct scanner_s *scanner) {
    int sol = 0;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);

            switch (CLASS_OF(c, scanner)) {
                case WS_CLASS:
                    /* increment the column number; c is assumed to not be a surrogate code value */
                    POSN_INCCOLUMN(scanner, 1);
                    sol = 0;
                    break;
                case EOL_CLASS:
                    if (POSN_COLUMN(scanner) > CIF_LINE_LENGTH) {
                        /* TODO: error long line */
                        /* recover by accepting it as-is */
                    }

                    /* the next character is at the start of a line; sol encodes data about the preceding terminators */
                    sol = ((sol << 2) + ((c == CR_CHAR) ? 2 : 1)) & 0xf;
                    /*
                     * sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
                     * the line number was incremented for the CR, and should not be incremented again for the LF.
                     */
                    POSN_INCLINE(scanner, ((sol == 0x9) ? 0 : 1));
                    break;
                default:
                    goto end_of_token;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

static int scan_to_ws(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);

            if (METACLASS_OF(c, scanner) == WS_META) {
                goto end_of_token;
            }

            /* Validate the character and advance the scanner position, incrementing the column number as appropriate */
            SCAN_CHARACTER(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

static int scan_to_eol(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);

            switch (CLASS_OF(c, scanner)) {
                case EOL_CLASS:
                case EOF_CLASS:
                    goto end_of_token;
            }

            /* Validate the character and advance the scanner position, incrementing the column number as appropriate */
            SCAN_CHARACTER(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

static int scan_unquoted(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);

            switch (METACLASS_OF(c, scanner)) {
                case OPEN_META:
                    /* TODO: error missing whitespace between values */
                    /* fall through */
                case CLOSE_META:
                case WS_META:
                    goto end_of_token;
            }

            /* Validate the character and advance the scanner position, incrementing the column number as appropriate */
            SCAN_CHARACTER(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

/*
 * Scans a string delimited by matching characters, the opening one being at the current token start position.  The
 * scan will stop with a recoverable error if it reaches the end of the initial line.  The token start position is
 * advanced to immediately past the opening delimiter, and the resulting token length accounts for only the content,
 * not the delimiters.
 */
static int scan_delim_string(struct scanner_s *scanner) {
    UChar delim = *(scanner->text_start);
    int lead_surrogate = CIF_FALSE;
    int delim_size;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Read and validate the next character, incrementing the column number as appropriate */
            SCAN_CHARACTER(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }

            if (c == delim) {
                if (scanner->cif_version < 2) {
                    /* look ahead for whitespace */
                    PEEK_CHAR(scanner, c, result);
                    if (result != CIF_OK) {
                        return result;
                    } else if (METACLASS_OF(c, scanner) != WS_META) {
                        /* the current character is part of the value */
                        continue;
                    }
                }
                delim_size = 1;
                goto end_of_token;
            } else {
                switch (CLASS_OF(c, scanner)) {
                    case EOL_CLASS:
                    case EOF_CLASS:
                        /* handle error: unterminated quoted string */
                        /* recover by assuming a trailing close-quote */
                        delim_size = 0;
                        goto end_of_token;
                }
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_INCSTART(scanner, 1);
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)) - delim_size);
    return CIF_OK;
}


/*
 * Scans a text block.  Sets the token parameters to mark the block contents, exclusive of delimiters, assuming the
 * initial token start position to be at the leading semicolon.
 */
static int scan_text(struct scanner_s *scanner) {
    int delim_size = 0;
    int sol = 0;
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Read and validate the next character, incrementing the column number as appropriate */
            SCAN_CHARACTER(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }

            switch (CLASS_OF(c, scanner)) {
                case SEMI_CLASS:
                    if (sol != 0) {
                        delim_size = ((*(scanner->next_char - 2) == LF_CHAR)
                                && (*(scanner->next_char - 3) == CR_CHAR)) ? 3 : 2;
                        goto end_of_token;
                    }
                    break;
                case EOL_CLASS:
                    if (POSN_COLUMN(scanner) > CIF_LINE_LENGTH) {
                        /* TODO: error long line */
                        /* recover by accepting it as-is */
                    }

                    /* the next character is at the start of a line */
                    sol = ((sol << 2) + ((c == CR_CHAR) ? 2 : 1)) & 0xf;
                    /*
                     * sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
                     * the line number was incremented for the CR, and should not be incremented again for the LF.
                     */
                    POSN_INCLINE(scanner, ((sol == 0x9) ? 0 : 1));
                    break;
                case EOF_CLASS:
                    /* TODO: error unterminated text block */
                    /* recover by reporting out the whole tail as the token */
                    goto end_of_token;
                default:
                    sol = 0;
                    break;
            }
        }

        GET_MORE_CHARS(scanner, result);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_INCSTART(scanner, 1);
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)) - delim_size);
    return CIF_OK;
}

/*
 * Transfers characters from the provided scanner's character source into its working character buffer, provided that
 * any are available.  May move unconsumed data within the buffer (adjusting the rest of the scanner's internal state
 * appropriately) and/or may increase the size of the buffer.  Will insert an EOF_CHAR into the buffer if called when
 * there are no more characters are available.  Returns CIF_OK if anny characters are transferred (including an EOF
 * marker), otherwise CIF_ERROR.
 */
static int get_more_chars(struct scanner_s *scanner) {
    size_t chars_read = scanner->next_char - scanner->buffer;
    size_t chars_consumed = scanner->text_start - scanner->buffer;
    ssize_t nread;

    assert(chars_read < SSIZE_T_MAX);
    assert(chars_consumed < chars_read);
    if (chars_consumed >= scanner->buffer_limit) {
        /* the buffer is empty; reset it to the beginning */
        assert(scanner->next_char == scanner->text_start);
        scanner->text_start = scanner->buffer;
        TVALUE_SETSTART(scanner, scanner->buffer);
        scanner->next_char = scanner->buffer;
        scanner->buffer_limit = 0;
    } else if (scanner->buffer_size < (scanner->buffer_limit + BUF_MIN_FILL)) {
        /* need to make room at the top of the buffer */
        size_t current_chars = chars_read - chars_consumed;
        size_t tvalue_diff = TVALUE_START(scanner) - scanner->text_start;

        if (current_chars * 2 < scanner->buffer_size) {
            /* The current data occupy less than half the buffer; move them to the front of the buffer */
            memmove(scanner->buffer, scanner->text_start, current_chars * sizeof(UChar));
        } else {
            /*
             * extend the buffer ( == allocate a new one and copy what's needed from the old )
             *
             * malloc() is chosen over realloc() here because it is expected that in-place extension will rarely be
             * possible, and in most other cases it will be unnecessary to copy the full contents of the original
             * buffer.  Moreover, this affords the opportunity to relocate the valid buffered data to the beginning
             * of the new buffer without unneeded overhead.
             */
            size_t new_size = scanner->buffer_size * 2;
            UChar *new_buffer = (UChar *) malloc(new_size * sizeof(UChar));

            if (new_buffer == NULL) {
                return CIF_ERROR;
            } else {
                /* preserve only the needed data */
                memcpy(new_buffer, scanner->text_start, current_chars * sizeof(UChar));

                /* assign the expanded buffer to the scanner */
                free(scanner->buffer);
                scanner->buffer = new_buffer;
                scanner->buffer_size = new_size;
            }
        }

        /* either way, update scanner state to reflect the changes */
        scanner->text_start = scanner->buffer;
        TVALUE_SETSTART(scanner, scanner->buffer + tvalue_diff);
        scanner->next_char = scanner->buffer + current_chars;
        scanner->buffer_limit = current_chars;
    }

    /* once EOF has been detected, don't attempt to read from the character source any more */
    nread = scanner->at_eof ? 0 : scanner->read_func(scanner->char_source, scanner->buffer + scanner->buffer_limit,
                scanner->buffer_size - scanner->buffer_limit);

    if (nread < 0) {
        return CIF_ERROR;
    } else if (nread == 0) {
        scanner->buffer[scanner->buffer_limit] = EOF_CHAR;
        scanner->buffer_limit += 1;
        scanner->at_eof = CIF_TRUE;
        return CIF_OK;
    } else {
        UChar *start = scanner->buffer + scanner->buffer_limit;
        UChar *end = start + nread;

        for (; start < end; start += 1) {
            if ((*start < CHAR_TABLE_MAX) ? (scanner->char_class[*start] == NO_CLASS)
                    : ((*start > 0xFFFD) || (*start == 0xFEFF) || ((*start >= 0xFDD0) && (*start <= 0xFDEF)))) {
                /* TODO: error disallowed BMP character */
                /* recover by replacing with a replacement char */
                *start = ((scanner->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR);
            }
        }

        if (scanner->cif_version < 2) {
            for (start = scanner->buffer + scanner->buffer_limit; start < end; start += 1) {
                if (*start > CIF1_MAX_CHAR) {
                    /* TODO: error disallowed CIF 1 character */
                    /* recover by accepting the character */
                }
            }
        }

        scanner->buffer_limit += nread;
        return CIF_OK;
    }
}

