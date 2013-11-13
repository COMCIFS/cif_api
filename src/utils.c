/*
 * utils.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>
#include <unicode/unorm.h>
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

#ifdef __cplusplus
extern "C" {
#endif

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
 * result_length: a pointer by which to return the length of the normalized result
 * terminate: iff zero, the returned string is not required to be NUL-terminated
 *
 * Returns a pointer to the normalized string, or NULL if normalization fails
 */
static UChar *cif_unicode_normalize(const UChar *src, int32_t srclen, UNormalizationMode mode, int32_t *result_length,
        int terminate);

/*
 * Applies the Unicode case-folding algorithm (without Turkic dotless-i special option) to the (initial segment of the)
 * specified string
 *
 * src: the Unicode string to fold; assumed non-NULL
 * srclen: the maximum length of the input to fold; if less than zero then the whole string is folded up to
 *     the terminating NUL character (which otherwise does not need to be present)
 * result_length: a pointer by which to return the length of the folded result
 *
 * Returns a pointer to the folded string, or NULL if folding fails; the caller is responsible for cleaning up; the
 * result is not necessarilly NUL-terminated
 */
static UChar *cif_fold_case(const UChar *src, int32_t srclen, int32_t *result_length);


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

int cif_normalize(const UChar *src, int32_t srclen, UChar **normalized) {
    int32_t result_length;
    UChar *buf = cif_unicode_normalize(src, srclen, UNORM_NFD, &result_length, 0);

    INIT_USTDERR;
    if (buf == NULL) {
#ifdef DEBUG
        u_fprintf(ustderr, "error: Could not normalize to form NFD: %S\n", src);
        u_fflush(ustderr);
#endif
    } else {
        UChar *buf2 = cif_fold_case(buf, result_length, &result_length);

        if (buf2 == NULL) {
#ifdef DEBUG
            u_fprintf(ustderr, "error: Could not fold case: %S\n", buf);
            u_fflush(ustderr);
#endif
            free(buf);
        } else {
            free(buf);
            buf = cif_unicode_normalize(buf2, result_length, UNORM_NFC, &result_length, 1);
            if (buf == NULL) {
#ifdef DEBUG
                u_fprintf(ustderr, "error: Could not normalize to form NFC: %S\n", buf2);
                u_fflush(ustderr);
#endif
                free(buf2);
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

    return CIF_ERROR;
}


/*
 * Computes the Unicode normalization form NFC equivalent of the given Unicode string.  Returns NULL on failure;
 * otherwise, the caller assumes responsibility for cleaning up the returned Unicode string.  The original string is
 * not modified.
 */
static UChar *cif_unicode_normalize(const UChar *src, int32_t srclen, UNormalizationMode mode, int32_t *result_length,
        int terminate) {
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
            UChar *temp = (UChar *) realloc(buf, ((size_t) normalized_chars + 1) * sizeof (UChar));

            if (temp != NULL) {
                /* buf is no longer a valid pointer */
                temp[normalized_chars] = 0;
                *result_length = normalized_chars;
                return temp; 
            } /* else reallocation failure; buf remains valid */
        } else if (U_SUCCESS(code)) {
            /* output captured and terminator written -- it's all good */
            *result_length = normalized_chars;
            return buf;
        } else if (code == U_BUFFER_OVERFLOW_ERROR) {
            /* a larger output buffer was needed */

            /* allocate a sufficient buffer; do not realloc() because it is useless to preserve the current content */
            free(buf);
            buffer_chars = (size_t) normalized_chars + 1;
            buf = (UChar *) malloc(buffer_chars * sizeof (UChar));  /* no need for a NULL check here */

            /* run the normalization routine again to capture the missing output */
            continue;
        } /* else normalization failed */

#ifdef DEBUG
        fprintf(stderr, "error: Unicode normalization failure: %s\n", u_errorName(code));
#endif

        free(buf);
        return NULL;
    }

    return buf;
}

static UChar *cif_fold_case(const UChar *src, int32_t srclen, int32_t *result_length) {
    int32_t src_chars = ((srclen < 0) ? u_strlen(src) : srclen);

    /* initial guess: the folded version will not contain more code units than the unfolded */
    int32_t buffer_chars = src_chars + 1;
    UChar *buf = (UChar *) malloc(((size_t) buffer_chars) * sizeof(UChar));

    while (buf) {
        UErrorCode code = U_ZERO_ERROR;
        int32_t folded_chars = u_strFoldCase(buf, buffer_chars, src, src_chars, U_FOLD_CASE_DEFAULT, &code);

        if (U_SUCCESS(code)) {
            /* case folding was successful, but result is not necessarily NUL-terminated */
            *result_length = folded_chars;
            return buf;
        } else if (code == U_BUFFER_OVERFLOW_ERROR) {
            /* the destination buffer was too small */

            /* allocate a sufficient buffer */
            free(buf);
            buffer_chars = (size_t) folded_chars + 1;
            buf = (UChar *) realloc(buf, buffer_chars * sizeof(UChar));  /* no need for a NULL check here */

            /* re-fold */
            continue;
        }  /* else case-folding failure */

        /* clean up and fail */
        free(buf);
        return NULL;
    }

    return buf;
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
        UChar *buf = cif_unicode_normalize(name, namelen, UNORM_NFC, &dummy, 1);

        if (buf != NULL) {
            if (normalized_name) {
                *normalized_name = buf;
            } else {
                free(buf);
            }

            return CIF_OK;
        }

        return CIF_ERROR;
    } else {
        return invalidityCode;
    }
}


#ifdef __cplusplus
}
#endif

