/*
 * utils.c
 *
 * Copyright 2014, 2015 John C. Bollinger
 *
 *
 * This file is part of the CIF API.
 *
 * The CIF API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CIF API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "internal/compat.h"

#include <stdlib.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>
#include <unicode/unorm.h>
#include <unicode/ucnv.h>
#include "internal/utils.h"

#define MIN_HIGH_SURROGATE 0xd800
#define MIN_LOW_SURROGATE 0xdc00
#define MAX_LOW_SURROGATE 0xdfff
#define HIGH_SURROGATE_OFFSET 10

#ifdef DEBUG
UFILE *ustderr = NULL;
#define INIT_USTDERR do { if (ustderr == NULL) ustderr = u_finit(stderr, NULL, NULL); } while(0)
#else
#define INIT_USTDERR
#endif

const UChar cif11_chars[] = {
    UCHAR_TAB,
    UCHAR_CR,
    UCHAR_NL,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e,
    0
};

const size_t cif11_chars_elements = ((sizeof cif11_chars) / (sizeof cif11_chars[0])) - 1;

/*
 * Scans the provided Unicode string for characters disallowed in CIF, returning
 * CIF_TRUE if any are found or CIF_FALSE otherwise.  Surrogate pairs are
 * analyzed as the combined code point they jointly represent (but unpaired
 * surrogates are disallowed).  The argument must point to the first character
 * of a Unicode string terminated by a NUL character (U+0000), otherwise behavior
 * is undefined.
 *
 * The disallowed characters are
 *   those with code points less than U+0020, EXCEPT U+0009, U+000A, and U+000D;
 *   U+007F;
 *   U+FDD0 - U+FDEF;
 *   code points U+xxxxxx where (xxxxxx & 0xFFFE) is 0xFFFE
 */
static int cif_has_disallowed_chars(/*@temp@*/ const UChar *str) /*@*/;

/*
 * Scans the provided Unicode string for characters considered by CIF to be
 * whitespace.  Unicode character properties cannot be used for this purpose
 * because CIF's idea of whitespace is more restrictive than Unicode's.
 *
 * The argument must point to the first character of a Unicode string terminated
 * by a NUL character (U+0000), otherwise behavior is undefined.  This function
 * may report false positives if the provided string contains disallowed
 * characters.
 */
static int cif_has_whitespace(/*@temp@*/ const UChar *str) /*@*/;

/*
 * Determines whether the specified Unicode string is a valid CIF name
 *
 * name: the NUL-terminated Unicode string to evaluate
 * for_item: nonzero if the string is to be validated as an item name, otherwise zero to validate it as a block code or
 *     frame code
 *
 * Returns nonzero if the specified string is valid as a name of the specified kind, otherwise zero
 */
static int cif_is_valid_name(/*@temp@*/ const UChar *name, int for_item);

/*
 * Applies the specified Unicode normalization form to the (initial segment of the) specified string
 *
 * src: the Unicode string to normalize; assumed non-NULL
 * srclen: the maximum length of the input to normalize; if less than zero then the whole string is normalized up to
 *     the terminating NUL character (which otherwise does not need to be present)
 * mode: tyhe desired normalization mode
 * result: a pointer to the location where a pointer to the normalized string should be stored; modified only on success
 * result_length: a pointer by which to return the length of the normalized result; modified only on success
 * terminate: iff zero, the returned string is not required to be NUL-terminated
 *
 * Returns a CIF result code describing the success or failure of the operation
 */
static int cif_unicode_normalize(const UChar *src, int32_t srclen, UNormalizationMode mode, UChar **result,
        int32_t *result_length,  int terminate);

/*
 * Applies the Unicode case-folding algorithm (without Turkic dotless-i special option) to the (initial segment of the)
 * specified string.  The result is not nul-terminated if there is insufficient space for it.
 *
 * src: the Unicode string to fold; assumed non-NULL
 * srclen: the maximum length of the input to fold; if less than zero then the whole string is folded up to
 *     the terminating NUL character (which otherwise does not need to be present)
 * result: a pointer to the location where a pointer to the folded characters should be stored (not necessarily with
 *     a NUL terminator); modified only on success
 * result_length: a pointer by which to return the length of the folded result; modified only on success
 *
 * Returns a CIF result code describing the success or failure of the operation
 */
static int cif_fold_case(const UChar *src, int32_t srclen, UChar **result, int32_t *result_length);

static int cif_has_disallowed_chars(const UChar *str) {
    const UChar *c;

    for (c = str; *c != 0; c++) {
        if (*c < MIN_HIGH_SURROGATE || *c > MAX_LOW_SURROGATE) {
            if (((*c < 0x20) && (*c != 0x9) && (*c != 0xa) && (*c != 0xd)) 
                    || (*c == 0x7f)
                    || ((*c > 0xfdcf) && (*c < 0xfdf0))
                    || (*c > 0xfffd)) {
                /* a disallowed BMP character */
                return 1;
            }
        } else if (*c >= MIN_LOW_SURROGATE) {
            /* an unpaired low surrogate */
            return 1;
        } else {
            const UChar high = *c;
            c++;
            if (*c < MIN_LOW_SURROGATE || *c > MAX_LOW_SURROGATE) {
                /* previous was an unpaired high surrogate */
                return 1;
            } else if (((*c & 0x3fe) == 0x3fe) && ((high & 0x3f) == 0x3f)) {
                /* surrogate pair encoding a "not a character" */
                return 1;
            }
        }
    }

    return 0;
}

#define UCHAR_UNDERSCORE 95
static int cif_has_whitespace(const UChar *src) {
    const UChar *c;

    for (c = src; *c != 0; c++) {
        if (*c <= 0x20) return 1;
    }

    return 0;
}

static int cif_is_valid_name(const UChar *name, int for_item) {
    return ((name != NULL)
            && ((for_item == 0) ? (*name != 0) : (*name == UCHAR_UNDERSCORE))
            && (u_countChar32(name, -1) <= (int32_t) CIF_LINE_LENGTH - ((for_item == 0) ? 5 : 0))
            && (cif_has_whitespace(name) == 0)
            && (cif_has_disallowed_chars(name) == 0)) ? 1 : 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int cif_normalize(const UChar *src, int32_t srclen, UChar **normalized) {
    FAILURE_HANDLING;
    int32_t result_length;
    UChar *buf;
    int result_code;

    INIT_USTDERR;
    if ((result_code = cif_unicode_normalize(src, srclen, UNORM_NFD, &buf, &result_length, 0)) != CIF_OK) {
        SET_RESULT(result_code);
#ifdef DEBUG
        u_fprintf(ustderr, "error: Could not normalize to form NFD: %S\n", src);
        u_fflush(ustderr);
#endif
    } else {
        UChar *buf2;

        if ((result_code = cif_fold_case(buf, result_length, &buf2, &result_length)) != CIF_OK) {
#ifdef DEBUG
            u_fprintf(ustderr, "error: Could not fold case: %S\n", buf);
            u_fflush(ustderr);
#endif
            free(buf);
            SET_RESULT(result_code);
        } else {
            free(buf);
            if ((result_code = cif_unicode_normalize(buf2, result_length, UNORM_NFC, &buf, &result_length, 1)) != CIF_OK) {
                SET_RESULT(result_code);
                free(buf2);
#ifdef DEBUG
                u_fprintf(ustderr, "error: Could not normalize to form NFC: %S\n", buf2);
                u_fflush(ustderr);
#endif
            } else {
                free(buf2);
                if (normalized) {
                    *normalized = buf;
                } else {
                    free(buf);
                }

                return CIF_OK;
            }
        }
    }

    FAILURE_TERMINUS;
}


UChar *cif_u_strdup(const UChar *src) {
    if (src) {
        int32_t len = u_strlen(src) + 1;
        UChar *dest = (UChar *) malloc((size_t) len * sizeof(UChar));

        return (dest == NULL) ? NULL : u_strncpy(dest, src, len);
    } else {
        return NULL;
    }
}

int cif_cstr_to_ustr(const char *cstr, int32_t srclen, UChar **ustr) {
    if (ustr) {
        if (!cstr) {
            *ustr = NULL;
            return CIF_OK;
        } else {
            int32_t eff_srclen = ((srclen < 0) ? (int32_t) strlen(cstr) : srclen);
            int32_t capacity = 2 * eff_srclen + 1;  /* capacity is measured in UChar units */
            UChar *tmp = (UChar *) malloc(capacity * sizeof(UChar));

            if (!tmp) {
                return CIF_MEMORY_ERROR;
            } else {
                UErrorCode error = U_ZERO_ERROR;
                UConverter *converter = ucnv_open(NULL, &error);
                int32_t result = ucnv_toUChars(converter, tmp, capacity, cstr, eff_srclen, &error);

                ucnv_close(converter);
                if (U_SUCCESS(error)) {
                    /* reallocate to the (almost surely smaller) size of the actual result */
                    *ustr = (UChar *) realloc(tmp, (result + 1) * sizeof(UChar));
                    if (!*ustr) {
                        /*
                         * Reallocation failed.
                         *
                         * This should not happen because the new size should always be less than the original size,
                         * but if it were to happen then the original 'tmp' is still valid and its value can be
                         * returned to the caller.
                         */
                        *ustr = tmp;
                    }

                    return CIF_OK;
                }

                free(tmp);
            }
        }
    }

    return CIF_ERROR;
}

int cif_normalize_name(const UChar *name, int32_t namelen, UChar **normalized_name, int invalidityCode) {
    if (cif_is_valid_name(name, 0) == 0) {
        return invalidityCode;
    } else {
        return cif_normalize(name, namelen, normalized_name);
    }
}

int cif_normalize_item_name(const UChar *name, int32_t namelen, UChar **normalized_name, int invalidityCode) {
    if (cif_is_valid_name(name, 1) == 0) {
        return invalidityCode;
    } else {
        return cif_normalize(name, namelen, normalized_name);
    }
}

int cif_normalize_table_index(const UChar *name, int32_t namelen, UChar **normalized_name, int invalidityCode) {
    if ((name != NULL) && (cif_has_disallowed_chars(name) == 0)) {
        int32_t dummy;
        UChar *buf;
        int result;

        if ((result = cif_unicode_normalize(name, namelen, UNORM_NFC, &buf, &dummy, 1)) == CIF_OK) {
            if (normalized_name) {
                *normalized_name = buf;
            } else {
                free(buf);
            }
        }

        return result;
    } else {
        return invalidityCode;
    }
}

int cif_is_reserved_string(const UChar *str) {
#define UCHAR_A 0x41
#define UCHAR_a 0x61
#define UCHAR_B 0x42
#define UCHAR_b 0x62
#define UCHAR_D 0x44
#define UCHAR_d 0x64
#define UCHAR_E 0x45
#define UCHAR_e 0x65
#define UCHAR_G 0x47
#define UCHAR_g 0x67
#define UCHAR_L 0x4C
#define UCHAR_l 0x6C
#define UCHAR_O 0x4F
#define UCHAR_o 0x6f
#define UCHAR_P 0x50
#define UCHAR_p 0x70
#define UCHAR_S 0x53
#define UCHAR_s 0x73
#define UCHAR_T 0x54
#define UCHAR_t 0x74
#define UCHAR_V 0x56
#define UCHAR_v 0x76

    switch (str[0]) {
        case UCHAR_UNDER:
        case UCHAR_HASH:
        case UCHAR_DOLLAR:
        case UCHAR_SQ:
        case UCHAR_DQ:
            /* reserved first character */
            return 1;
        case UCHAR_D:
        case UCHAR_d:
            /* check for data_* */
            return (((str[1] == UCHAR_A) || (str[1] == UCHAR_a))
                    && ((str[2] == UCHAR_T) || (str[2] == UCHAR_t))
                    && ((str[3] == UCHAR_A) || (str[3] == UCHAR_a))
                    && (str[4] == UCHAR_UNDER)
                    );
        case UCHAR_G:
        case UCHAR_g:
            /* check for global_ (exact) */
            return (((str[1] == UCHAR_L) || (str[1] == UCHAR_l))
                    && ((str[2] == UCHAR_O) || (str[2] == UCHAR_o))
                    && ((str[3] == UCHAR_B) || (str[3] == UCHAR_b))
                    && ((str[4] == UCHAR_A) || (str[4] == UCHAR_a))
                    && ((str[5] == UCHAR_L) || (str[5] == UCHAR_l))
                    && (str[6] == UCHAR_UNDER) && (str[7] == 0)
                    );
        case UCHAR_L:
        case UCHAR_l:
            /* check for loop_ (exact) */
            return (((str[1] == UCHAR_O) || (str[1] == UCHAR_o))
                    && ((str[2] == UCHAR_O) || (str[2] == UCHAR_o))
                    && ((str[3] == UCHAR_P) || (str[3] == UCHAR_p))
                    && (str[4] == UCHAR_UNDER) && (str[5] == 0)
                    );
        case UCHAR_S:
        case UCHAR_s:
            switch (str[1]) {
                case UCHAR_A:
                case UCHAR_a:
                    /* check for save_* */
                    return (((str[2] == UCHAR_V) || (str[2] == UCHAR_v))
                            && ((str[3] == UCHAR_E) || (str[3] == UCHAR_e))
                            && (str[4] == UCHAR_UNDER)
                            );
                case UCHAR_T:
                case UCHAR_t:
                    /* check for stop_ (exact) */
                    return (((str[2] == UCHAR_O) || (str[2] == UCHAR_o))
                            && ((str[3] == UCHAR_P) || (str[3] == UCHAR_p))
                            && (str[4] == UCHAR_UNDER) && (str[5] == 0)
                            );
            }  /* default: nothing */
            break;
    } /* default: nothing */

    return 0;
}

int cif_analyze_string(const UChar *str, int allow_unquoted, int allow_triple_quoted, int32_t length_limit,
        struct cif_string_analysis_s *result) {
    static const UChar apos_delim[] = { UCHAR_SQ, 0 };
    static const UChar quot_delim[] = { UCHAR_DQ, 0 };
    static const UChar apos3_delim[] = { UCHAR_SQ, UCHAR_SQ, UCHAR_SQ, 0 };
    static const UChar quot3_delim[] = { UCHAR_DQ, UCHAR_DQ, UCHAR_DQ, 0 };
    static const UChar text_delim[] = { UCHAR_NL, UCHAR_SEMI, 0 };

    /* uses signed 32-bit integer character counters -- sufficient for very (very!) long values */
    int32_t char_counts[128] = { 0 };
    int32_t first_line = 0;
    int32_t this_line = 0;
    int32_t max_line = 0;
    int32_t length = 0;
    int32_t consec_semis = 0;
    int32_t most_semis = 0;
    int32_t crlf_count = 0;
    int has_nl_semi = CIF_FALSE;
    int has_trailing_ws = CIF_FALSE;
    int32_t num_lines;
    UChar ch;

    result->has_reserved_start = CIF_FALSE;

    /* Analyze the text to inform the choice of delimiters */
    for (ch = str[0]; ch; ch = str[++length]) {
        char_counts[(ch < 127) ? ch : 127] += 1;
        switch (ch) {
            case UCHAR_CR:
                if (str[length + 1] == UCHAR_NL) {
                    crlf_count += 1;
                    break;
                } /* else fall through */
            case UCHAR_NL:
                has_nl_semi = (has_nl_semi || (str[length + 1] == UCHAR_SEMI));
                has_trailing_ws = (has_trailing_ws || ((length > 0) && (
                        (str[length - 1] == UCHAR_SP) || (str[length - 1] == UCHAR_TAB)
                        || (str[length - 1] == UCHAR_VT))));
                if (char_counts[UCHAR_NL] + char_counts[UCHAR_CR] == 1) {
                    first_line = this_line;
                    max_line = this_line;
                } else if (this_line > max_line) {
                    max_line = this_line;
                }
                if (consec_semis > most_semis) {
                    most_semis = consec_semis;
                }
                consec_semis = 0;
                this_line = 0;
                break;
            case UCHAR_SEMI:
                consec_semis += 1;
                this_line += 1;
                break;
            default:
                consec_semis = 0;
                this_line += 1;
                break;
        }
    }

    /* handle the stats for the last line */
    if (consec_semis > most_semis) {
        most_semis = consec_semis;
    }
    num_lines = 1 + char_counts[UCHAR_NL] + char_counts[UCHAR_CR] - crlf_count;
    if (num_lines == 1) {
        first_line = this_line;
        max_line = this_line;
    } else if (this_line > max_line) {
        max_line = this_line;
    }

    /* If the longest line surpasses the length limit then we cannot use anything other than a text block */
    if (max_line <= length_limit) {

        /* If the value is contained within a single line then all options are on the table */
        if (num_lines == 1) {

            /* Maybe whitespace-delimited */
            if (allow_unquoted
                    && ((char_counts[UCHAR_SP] + char_counts[UCHAR_TAB] + char_counts[UCHAR_OBRK]
                        + char_counts[UCHAR_CBRK] + char_counts[UCHAR_OBRC] + char_counts[UCHAR_CBRC]) == 0)
                    && (str[0] != UCHAR_SQ) && (str[0] != UCHAR_DQ) && (str[0] != UCHAR_HASH)
                    && (str[0] != UCHAR_DOLLAR) && (str[0] != UCHAR_UNDER) && (str[0] != UCHAR_SEMI)
                    && !cif_is_reserved_string(str)
                    ) {
                /* Flagged as unquoted and having valid form for being presented unquoted */
                result->delim[0] = 0; /* = write_unquoted(context, text, max_line); */
                result->delim_length = 0;
                goto done;
            }

            /* Maybe single-delimited */
            if (max_line <= (length_limit - 2)) {
                if (char_counts[UCHAR_SQ] == 0) {
                    /* recommend apostrophe delimiters */
                    u_strcpy(result->delim, apos_delim);
                    result->delim_length = 1;
                    goto done;
                } else if (char_counts[UCHAR_DQ] == 0) {
                    /* recommend quote delimiters */
                    u_strcpy(result->delim, quot_delim);
                    result->delim_length = 1;
                    goto done;
                } /* else neither (single) apostrophe or quotation mark is suitable */
            }

            /* maybe triple-delimited */
            if (allow_triple_quoted && (max_line <= (length_limit - 6))) {
                if (u_strstr(str, apos3_delim) == NULL) {
                    u_strcpy(result->delim, apos3_delim);
                    result->delim_length = 3;
                    goto done;
                } else if (u_strstr(str, quot3_delim) == NULL) {
                    u_strcpy(result->delim, quot3_delim);
                    result->delim_length = 3;
                    goto done;
                }
            }

        } else
        /*
         * We can use triple quotes if neither the first line nor the last is too long, and if the text does
         * not contain both triple delimiters, provided that triple-quoting is permitted at all in the dialect
         * we are writing.
         */
        if (allow_triple_quoted && (this_line < (length_limit - 3)) && (first_line < (length_limit - 3))) {
            if (u_strstr(str, apos3_delim) == NULL) {
                u_strcpy(result->delim, apos3_delim);
                result->delim_length = 3;
                goto done;
            } else if (u_strstr(str, quot3_delim) == NULL) {
                u_strcpy(result->delim, quot3_delim);
                result->delim_length = 3;
                goto done;
            }
        }
    }

    /* if control reaches this point then all alternatives other than a text block have been ruled out */
    u_strcpy(result->delim, text_delim);
    result->delim_length = 2;

    /* check whether the beginning of the value emulates a prefix or a fold separator */
    if (str[0] != UCHAR_SEMI) {
        int32_t index;

        for (index = first_line - 1; index >= 0; index -= 1) {
            switch (str[index]) {
                case UCHAR_TAB:
                case UCHAR_SP:
                    /* trailing spaces and tabs do not affect the determination */
                    break;
                case UCHAR_BSL:
                    /*
                     * The last non-whitespace character is a backslash, so the beginning of the text
                     * looks like a prefixing and/or line-folding marker.  It can be handled via line folding.
                     */
                    result->has_reserved_start = CIF_TRUE;

                    /* fall through */
                default:
                    goto done;
            }
        }
    }

    done:
    result->num_lines = num_lines;
    result->length = length;
    result->length_first = first_line;
    result->length_last = this_line;
    result->length_max = max_line;
    result->contains_text_delim = has_nl_semi;
    result->max_semi_run = consec_semis;
    result->has_trailing_ws = has_trailing_ws;

    return CIF_OK;
}

#ifdef __cplusplus
}
#endif

/*
 * Computes a normalization of the given Unicode string.  Returns a CIF error code on failure; otherwise, sets *result
 * to the normalized Unicode string, and the caller assumes responsibility for cleaning up that string.  The original
 * string is not modified.
 */
static int cif_unicode_normalize(const UChar *src, int32_t srclen, UNormalizationMode mode, UChar **result,
        int32_t *result_length,  int terminate) {
    FAILURE_HANDLING;
    int32_t src_chars = ((srclen >= 0) ? srclen : u_strlen(src));

    /* prepare an output buffer, reserving space for a trailing NUL character */
    int32_t buffer_chars = src_chars + 1;
    UChar *buf = (UChar *) malloc(((size_t) buffer_chars) * sizeof(UChar));

    while (buf) {
        /* perform the normalization */
        UErrorCode code = U_ZERO_ERROR;
        int32_t normalized_chars = unorm_normalize(src, src_chars, mode, 0, buf, buffer_chars, &code);

        if ((code == U_STRING_NOT_TERMINATED_WARNING) && terminate) {
            /* (only) the terminator did not fit in the buffer */
            /* enlarge and terminate the buffer; no need to re-normalize */
            UChar *temp = (UChar *) realloc(buf, ((size_t) normalized_chars + 1) * sizeof (UChar));

            if (temp != NULL) {
                /* buf is no longer a valid pointer */
                temp[normalized_chars] = 0;
                *result = temp;
                *result_length = normalized_chars;
                return CIF_OK;
            } /* else reallocation failure; buf remains valid */
            FAIL(soft, CIF_MEMORY_ERROR);
        } else if (U_SUCCESS(code)) { /* Note: ICU warning codes are accounted successful */
            /* output captured and terminator written -- it's all good */
            *result = buf;
            *result_length = normalized_chars;
            return CIF_OK;
        } else if (code == U_BUFFER_OVERFLOW_ERROR) {
            /* a larger output buffer was needed */

            /* allocate a sufficient buffer; do not realloc() because it is useless to preserve the current content */
            free(buf);
            buffer_chars = (size_t) normalized_chars + 1;
            buf = (UChar *) malloc(buffer_chars * sizeof (UChar));  /* no need for a NULL check here */

            /* run the normalization routine again to capture the missing output */
            continue;
        } /* else normalization failed */

        FAILURE_HANDLER(soft):
#ifdef DEBUG
        fprintf(stderr, "error: Unicode normalization failure: %s\n", u_errorName(code));
#endif

        free(buf);
        FAILURE_TERMINUS;
    }

    return CIF_MEMORY_ERROR;
}

static int cif_fold_case(const UChar *src, int32_t srclen, UChar **result, int32_t *result_length) {
    int32_t src_chars = ((srclen < 0) ? u_strlen(src) : srclen);

    /* initial guess: the folded version will not contain more code units than the unfolded */
    int32_t buffer_chars = src_chars + 1;
    UChar *buf = (UChar *) malloc(((size_t) buffer_chars) * sizeof(UChar));

    while (buf) {
        UErrorCode code = U_ZERO_ERROR;
        int32_t folded_chars = u_strFoldCase(buf, buffer_chars, src, src_chars, U_FOLD_CASE_DEFAULT, &code);

        if (U_SUCCESS(code)) { /* Note: ICU warning codes are accounted successful */
            /* case folding was successful, but result is not necessarily NUL-terminated */
            *result = buf;
            *result_length = folded_chars;
            return CIF_OK;
        } else if (code == U_BUFFER_OVERFLOW_ERROR) {
            /* the destination buffer was too small */

            /* allocate a sufficient buffer */
            free(buf);
            buffer_chars = (size_t) folded_chars + 1;
            buf = (UChar *) malloc(buffer_chars * sizeof(UChar));  /* no need for a NULL check here */

            /* re-fold */
            continue;
        }  /* else case-folding failure */

        /* clean up and fail */
        free(buf);
        return CIF_ERROR;
    }

    return CIF_MEMORY_ERROR;
}
