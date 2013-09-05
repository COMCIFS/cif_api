/*
 * ciffile.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cif.h"
#include "internal/utils.h"

#define CIF_NOWRAP    0
#define CIF_WRAP      1

#define PREFIX "> "
#define PREFIX_LENGTH 2

#ifdef __cplusplus
extern "C" {
#endif

static int write_cif_start(cif_t *cif, void *context);
static int write_cif_end(cif_t *cif, void *context);
static int write_block_start(cif_container_t *block, void *context);
static int write_block_end(cif_container_t *block, void *context);
static int write_frame_start(cif_container_t *frame, void *context);
static int write_frame_end(cif_container_t *frame, void *context);
static int write_loop_start(cif_loop_t *loop, void *context);
static int write_loop_end(cif_loop_t *loop, void *context);
static int write_packet_start(cif_packet_t *packet, void *context);
static int write_packet_end(cif_packet_t *packet, void *context);
static int write_item(UChar *name, cif_value_t *value, void *context);

static int write_list(void *context, cif_value_t *char_value);
static int write_table(void *context, cif_value_t *char_value);
static int write_char(void *context, cif_value_t *char_value, int allow_text);
static int write_text(void *context, UChar *text, int32_t length, int fold, int prefix);
static int fold_line(const UChar *line, int do_fold, int target_length, int window);
static int write_quoted(void *context, const UChar *text, int32_t length, char delimiter);
static int write_numb(void *context, cif_value_t *numb_value);
static int32_t write_literal(void *context, const char *text, int length, int wrap);
static int32_t write_uliteral(void *context, const UChar *text, int length, int wrap);
static int write_newline(void *context);

/*
 * Parses a CIF from the specified stream.  If the 'cif' argument is non-NULL
 * then a handle for the parsed CIF is written to its referrent.  Otherwise,
 * the parsed results are discarded, but the return code still indicates whether
 * parsing was successful.
 */
int cif_parse(FILE *stream, struct cif_parse_opts_s *options, cif_t **cif) {
    /* TODO */
    return CIF_NOT_SUPPORTED;
}

typedef struct {
    UFILE *file;
    int write_item_names;
    int last_column;
} write_context_t;

/* the true data type of the CIF walker context pointers used by the CIF-writing functions */
#define CONTEXT_S write_context_t
#define CONTEXT_T CONTEXT_S *
#define CONTEXT_INITIALIZE(c, f) do { c.file = f; c.write_item_names = CIF_FALSE; c.last_column = 0; } while (CIF_FALSE)
#define CONTEXT_UFILE(c) (((CONTEXT_T)(c))->file)
#define SET_WRITE_ITEM_NAMES(c,v) do { ((CONTEXT_T)(c))->write_item_names = (v); } while (CIF_FALSE)
#define IS_WRITE_ITEM_NAMES(c) (((CONTEXT_T)(c))->write_item_names)
#define SET_LAST_COLUMN(c,l) do { ((CONTEXT_T)(c))->last_column = (l); } while (CIF_FALSE)
#define LAST_COLUMN(c) (((CONTEXT_T)(c))->last_column)
#define LINE_LENGTH(c) (CIF_LINE_LENGTH)

/*
 * Formats the CIF data represented by the 'cif' handle to the specified
 * output.
 */
int cif_write(FILE *stream, cif_t *cif) {
    cif_handler_t handler = {
        write_cif_start,
        write_cif_end,
        write_block_start,
        write_block_end,
        write_frame_start,
        write_frame_end,
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

static int write_cif_start(cif_t *cif UNUSED, void *context) {
    int num_written = u_fprintf(CONTEXT_UFILE(context), "#\\#CIF_2.0\n");

    SET_LAST_COLUMN(context, 0);
    return ((num_written > 0) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_cif_end(cif_t *cif UNUSED, void *context) {
    return (write_newline(context) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_block_start(cif_container_t *block, void *context) {
    UChar *code;
    int result = cif_container_get_code(block, &code);

    if (result == CIF_OK) {
        result = ((u_fprintf(CONTEXT_UFILE(context), "\ndata_%S\n", code) > 7) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
        SET_LAST_COLUMN(context, 0);
        free(code);
    }
    return result;
}

static int write_block_end(cif_container_t *block UNUSED, void *context) {
    return (write_newline(context) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
}

static int write_frame_start(cif_container_t *frame, void *context) {
    UChar *code;
    int result = cif_container_get_code(frame, &code);

    if (result == CIF_OK) {
        result = ((u_fprintf(CONTEXT_UFILE(context), "\nsave_%S\n", code) > 7) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
        SET_LAST_COLUMN(context, 0);
        free(code);
    }
    return result;
}

static int write_frame_end(cif_container_t *frame UNUSED, void *context) {
    SET_LAST_COLUMN(context, 0);  /* anticipates the next line */
    return ((u_fprintf(CONTEXT_UFILE(context), "\nsave_\n") > 6) ? CIF_TRAVERSE_CONTINUE : CIF_ERROR);
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
                            if (u_fprintf(CONTEXT_UFILE(context), " %S\n", *next_name) < 4) result = CIF_ERROR;
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

    /* output the data name if the context so indicates */
    if (IS_WRITE_ITEM_NAMES(context)) {
        if ((LAST_COLUMN(context) > 0) && !write_newline(context)) {
            FAIL(soft, CIF_ERROR);
        }

        if (write_uliteral(context, name, -1, CIF_NOWRAP) < 2) {
            FAIL(soft, CIF_ERROR);
        }
    }

    if (LAST_COLUMN(context) > 0) {
        switch (write_literal(context, " ", 1, CIF_NOWRAP)) {
            case 1:
                break;
            case -CIF_OVERLENGTH_LINE:
                write_newline(context);
                break;
            default:
                FAIL(soft, CIF_ERROR);
        }
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
        size_t index;

        SET_WRITE_ITEM_NAMES(context, CIF_FALSE);
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
            SET_WRITE_ITEM_NAMES(context, write_names_save);
            return CIF_OK;
        }
    }

    return CIF_ERROR;
}

static int write_table(void *context, cif_value_t *table_value) {
    FAILURE_HANDLING;
    UChar **keys;
    int result;

    if ((result = cif_value_get_keys(table_value, &keys)) != CIF_OK) {
        SET_RESULT(result);
    } else {
        if (write_literal(context, "{", 1, CIF_WRAP) == 1) {
            int write_names_save = IS_WRITE_ITEM_NAMES(context);
            UChar **key;

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
                    /* copying the key is inefficient, but necessitated by the external API */
                    if (cif_value_copy_char(kv, *key) != CIF_OK) {
                        SET_RESULT(CIF_ERROR);
                    } else if ((result = write_char(context, kv, CIF_FALSE)) != CIF_OK) {
                        SET_RESULT(result);
                    } else if (write_literal(context, ":", 1, CIF_NOWRAP) != 1) {
                        SET_RESULT(CIF_ERROR);
                    } else if ((result = write_item(NULL, value, context)) > 0) {
                        /* an error code */
                        SET_RESULT(result);
                    } else {
                        /* entry successfully written */
                        cif_value_free(kv);
                        continue;
                    }

                    /* entry not successfully written -- clean up before failing */
                    cif_value_free(kv);
                    DEFAULT_FAIL(soft);
                }
            }

            if (write_literal(context, " }", 2, CIF_WRAP) == 2) {
                /* success */
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
        /* choose delimiters */
        int32_t length = u_countChar32(text, -1);
        int fold = (length > (LINE_LENGTH(context) - 2));
        UChar *nl = u_strchr(text, '\n');

        if (nl != NULL) {
            if (allow_text) {
                /* write as a text block, possibly with line-folding and/or prefixing  */

                int prefix = CIF_FALSE;

                /* scan the value for embedded newline + semicolon */
                do {
                    if (*(++nl) == ';') {
                        prefix = CIF_TRUE;
                        break;
                    }
                    nl = u_strchr(nl, '\n');
                } while (nl);

                result = write_text(context, text, length, fold, prefix);
            } else {
                result = CIF_DISALLOWED_VALUE;
            }
        } else if (fold) {
            /* write the value as a line-folded text block */
            result = (allow_text ? write_text(context, text, length, CIF_TRUE, CIF_FALSE) : CIF_DISALLOWED_VALUE);
        } else if (u_strchr(text, '\'') == NULL) {
            /* write the value as an apostrophe-delimited string */
            result = write_quoted(context, text, length, '\'');
        } else if (u_strchr(text, '"') == NULL) {
            /* write the value as a quote-delimited string */
            result = write_quoted(context, text, length, '"');
        } else {
            /* write the value as a text block */
            result = (allow_text ? write_text(context, text, length, CIF_FALSE, CIF_FALSE) : CIF_DISALLOWED_VALUE);
        }

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

        if (!write_newline(context)) {
            return CIF_ERROR;
        }
        if (!prefix) {
            prefix_text[0] = '\0';
        }

        /* each logical line, delimited by newlines */
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
                    nchars = u_fprintf(CONTEXT_UFILE(context), "%s%*.*S%s\n", prefix_text, len, len, tok,
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

/**
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
 * @return the number of code units in the first folded segment; zero for an empty string
 */
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

    for (len += 1; len <= target_length + window; len += 1) {
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

static int write_numb(void *context, cif_value_t *numb_value) {
    int32_t result;
    UChar *text;

    if (cif_value_get_text(numb_value, &text) == CIF_OK) {
        result = write_uliteral(context, text, -1, CIF_WRAP);
        free(text);
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


#ifdef __cplusplus
}
#endif

