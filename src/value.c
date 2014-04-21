/*
 * value.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <assert.h>
#include <locale.h>

#ifdef HAVE_FEGETROUND
#ifdef HAVE_FENV_H
/*
 * This header is defined by C99, but not by C89.  We use it anyway if it is available, for the macros representing
 * floating-point rounding modes.
 */
#include <fenv.h>
#endif
#ifndef HAVE_DECL_FEGETROUND
/* fegetround() is available, but undeclared (even by fenv.h); assume the available version is C99-compliant */
int fegetround(void);
#endif
#endif

#include <unicode/utext.h>
#include <unicode/utypes.h>
#include <unicode/parseerr.h>
#include "cif.h"
#include "utlist.h"
#include "uthash.h"
#include "internal/utils.h"
#include "internal/value.h"

#define DEFAULT_SERIALIZATION_CAP 512

/**
 * @brief The base-10 logarithm of the smallest positive representable double (a de-normalized number),
 * truncated to an integer.
 */
#define LEAST_DBL_10_DIGIT (1 + DBL_MIN_10_EXP - DBL_DIG)

/**
 * @brief the number of leading zeroes permitted by default in the plain decimal text representation of a number value
 *
 * This is used by @c cif_value_autoinit_numb().
 */
#define DEFAULT_MAX_LEAD_ZEROES 5

/**
 * @brief Determines the index of the most significant decimal digit of the argument.
 *
 * The units digit has index 0.  Indices increase to the left of the decimal point, and decrease (taking negative
 * values) to the right.  The number zero has MSP zero.
 *
 * @param[in] the number whose MSP is requested; treated as a double
 *
 * @return the index of the most significant place, as an @c int
 */
#define MSP(v) ((int) floor((v == 0.0) ? 0 : log10(fabs(v))))

/**
 * @brief Serializes a NUL-terminated Unicode string to the provided buffer.
 *
 * The serialized representation is the bytes of a @c size_t @em character count followed by the raw bytes of the
 * string, excluding those of the terminating NUL character, all in machine order.  Special handling is provided
 * for NULL (not empty) strings.
 *
 * @param[in] string a pointer to the NUL-terminated Unicode string to serialize; may be NULL
 * @param[in,out] buffer a pointer to the @c write_buffer_t buffer to which the serialized form should be appended;
 *         must not be NULL.
 * @param[in] onerr the label to which execution should branch in the event that an error occurs; must be defined in the
 *         scope where this macro is used.
 */
#define SERIALIZE_USTRING(string, buffer, onerr) do { \
    const UChar *str = (string); \
    write_buffer_t *us_buf = (buffer); \
    ssize_t us_size = ((str == NULL) ? (ssize_t) -1 : (ssize_t) u_strlen(str)); \
    if (cif_buf_write(us_buf, &us_size, sizeof(ssize_t)) != CIF_OK) goto onerr; \
    if (cif_buf_write(us_buf, str, us_size * sizeof(UChar)) != CIF_OK) goto onerr; \
} while (0)

/**
 * @brief Deserializes a Unicode string from the given buffer, assuming the format produced by @c SERIALIZE_USTRING().
 *
 * A new Unicode string is allocated for the result, and on success, a pointer to it is assigned to @c dest (which
 * must therefore be an lvalue).  The @c dest pointer is unmodified on failure.
 *
 * @param[out] dest a Unicode string pointer to be pointed at the deserialized string; its initial value is ignored.
 *         may be set NULL
 * @param[in,out] a @c read_buffer_t from which the serialized string data should be read; on success its position is
 *         immediately after the last byte of the serialized string, but on failure its position becomes undefined;
 *         must not be NULL.
 * @param[in] onerr the label to which execution should branch in the event that an error occurs; must be defined in the
 *         scope where this macro is used.
 */
#define DESERIALIZE_USTRING(dest, buffer, onerr) do { \
    read_buffer_t *us_buf = (buffer); \
    ssize_t _size; \
    if (cif_buf_read(us_buf, &_size, sizeof(ssize_t)) == sizeof(ssize_t)) { \
        if (_size < 0) { \
            dest = NULL; \
        } else { \
            UChar *d = (UChar *) malloc((_size + 1) * sizeof(UChar)); \
            if (d != NULL) { \
                if (cif_buf_read(us_buf, d, (_size * sizeof(UChar))) == (_size * sizeof(UChar))) { \
                    d[_size] = 0; \
                    dest = d; \
                    break; \
                } else { \
                    free(d); \
                } \
            } \
        } \
    } \
    goto onerr; \
} while (0)

/**
 * @brief Serializes a value object and any component objects, appending the result to the specified buffer.
 *
 * @param[in] value a pointer to the @c cif_value_t object to serialize; must not be NULL.
 * @param[in,out] a @c read_buffer_t from which the serialized string data should be read; on success its position is
 *         immediately after the last byte of the serialized string, but on failure its position becomes undefined;
 *         must not be NULL.
 * @param[in] onerr the label to which execution should branch in the event that an error occurs; must be defined in the
 *         scope where this macro is used.
 */
#define SERIALIZE(value, buffer, onerr) do { \
    cif_value_t *val = (cif_value_t *) (value); \
    write_buffer_t *s_buf = (buffer); \
    if (cif_buf_write(s_buf, val, sizeof(cif_kind_t)) != CIF_OK) goto onerr; \
    switch (val->kind) { \
        case CIF_NUMB_KIND: \
        case CIF_CHAR_KIND: \
            SERIALIZE_USTRING(val->as_char.text, s_buf, onerr); \
            break; \
        case CIF_LIST_KIND: \
            if (cif_list_serialize(&(val->as_list), s_buf) != CIF_OK) goto onerr; \
            break; \
        case CIF_TABLE_KIND: \
            if (cif_table_serialize(&(val->as_table), s_buf) != CIF_OK) goto onerr; \
            break; \
        default: \
            break; \
    } \
} while (0)

/**
 * @brief Deserializes a value object from the provided buffer.
 *
 * If the @p value destination pointer is not NULL then the new value data is deserialized onto the value object to
 * which it points, overwriting anything already there; otherwise a new value object is allocated and (on success) a
 * pointer to it assigned (therefore @p value must be an lvalue).
 *
 * The type to which the value pointer points is canonically @c cif_value_t, but it can also be a struct having a
 * @c cif_value_t as its first member (note: an actual value object, not a pointer to one).  This affords a limited
 * form of polymorphism.
 *
 * As an implementation limitation, this macro must not appear more than once in any function.
 *
 * @param[in] value_type the type to which @p value points.
 * @param[in,out] value a pointer to receive the deserialized value, either into its existing referrent or into a newly-
 *         allocated referrent.
 * @param[in,out] buffer the @c read_buffer_t from which to obtain the serialized value
 * @param[in] onerr a label to which execution should branch in the event of error; must be defined in the scope where
 *         this macro is used.
 */
#define DESERIALIZE(value_type, value, buffer, onerr) do { \
    read_buffer_t *_buf = (buffer); \
    value_type *val = ((value != NULL) ? (value) : (value_type *) malloc(sizeof(value_type))); \
    if (val != NULL) { \
        cif_value_t *v = (cif_value_t *) val; \
        cif_kind_t _kind; \
        v->kind = CIF_UNK_KIND; \
        if (cif_buf_read(_buf, &_kind, sizeof(cif_kind_t)) == sizeof(cif_kind_t)) { \
            switch(_kind) { \
                case CIF_CHAR_KIND: \
                    v->kind = CIF_CHAR_KIND; \
                    /* fall through */ \
                case CIF_NUMB_KIND: \
                    DESERIALIZE_USTRING(v->as_char.text, _buf, vfail); \
                    if ((_kind == CIF_NUMB_KIND) \
                            && (cif_value_parse_numb(v, v->as_char.text) != CIF_OK)) goto vfail; \
                    break; \
                case CIF_LIST_KIND: \
                    if (cif_list_deserialize(&(v->as_list), _buf) != CIF_OK) goto vfail; \
                    assert(v->kind == CIF_LIST_KIND); \
                    break; \
                case CIF_TABLE_KIND: \
                    if (cif_table_deserialize(&(v->as_table), _buf) != CIF_OK) goto vfail; \
                    assert(v->kind == CIF_TABLE_KIND); \
                    break; \
                default: \
                    v->kind = _kind; \
                    break; \
            } \
            value = val; \
            break; \
        } \
        vfail: \
        if (val != value) free(val); \
    } \
    goto onerr; \
} while (0)

/**
 * @brief Copies a C string containing only ASCII characters into a Unicode character buffer.
 *
 * A pointer to the target location is provided as parameter @p u_dest; it must be an lvalue, as it is updated
 * to point to the next position after the last character copied.  This macro makes use of code-point equality
 * between ASCII and Unicode, therefore its behavior is undefined if the locale for which @p c_src is defined
 * uses a code page that is not congruent with Unicode for the characters contained in the source string.  It is
 * recommended that @p c_src use the C locale.
 *
 * @param[in,out] u_dest a pointer to the start of the destination region; must be an lvalue, as it is updated on
 *         completion to point to the new NUL terminator.  Behavior is undefined (and probably bad) if the target buffer
 *         does not have sufficient space for all the source characters, including the terminator.
 * @param[in] c_src a C string to copy to the destination.  
 */
#define USTRCPY_C(u_dest, c_src) do { \
    char *_s = (c_src); \
    while (*_s != '\0') *(u_dest++) = (UChar) *(_s++); \
    *u_dest = 0; \
} while (0)

/**
 * @brief Conditionally writes a standard uncertainty to the provided Unicode buffer.
 *
 * A NULL uncertainty is taken to indicate an exact number, in which case no uncertainty is written; otherwise the
 * provided digit string is copied to the destination on an equal code point basis, between parentheses.  In either
 * case, the destination buffer is ensured NUL terminated.
 *
 * @param[in,out] text a pointer to the beginning of the Unicode character buffer where the uncertainty, if any,
 *         should be written; it is updated to point to the terminating NUL character written by this macro, therefore
 *         it must be an lvalue
 * @param uncertainty A C string containing the decimal digits of the uncertainty to write, or NULL if the number is
 *         exact
 */
#define WRITE_SU(text, uncertainty) do { \
    UChar *_t = (text); \
    char *_su = (uncertainty); \
    if (_su != NULL) { \
        *(_t++) = UCHAR_OPEN; \
        USTRCPY_C(_t, _su); \
        *(_t++) = UCHAR_CLOSE; \
    } \
    *_t = 0; \
} while (0)

/* headers for internal functions */

/*
 * Creates a new dynamic buffer with the specified initial capacity.
 * Returns a pointer to the buffer structure, or NULL if buffer creation fails.
 */
static buffer_t *cif_buf_create(size_t cap);

/*
 * Releases a writeable dynamic buffer and all resources associated with it.  A no-op if the argument is NULL.
 */
static void cif_buf_free(write_buffer_t *buf);

/*
 * Writes bytes to a dynamic buffer, starting at its current position.
 * The buffer capacity is expanded as needed to accommodate the specified number of bytes, and possibly more.
 * The third argument may be zero, in which case the function is a no-op.
 * Returns CIF_OK on success or an error code on failure.  The buffer is unmodified on failure.
 * Behavior is undefined if buf does not point to a valid buffer_t object, or if start does not point to an
 * allocated block of memory at least len bytes long, or if the source is inside the buffer.
 */
static int cif_buf_write(write_buffer_t *buf, const void *src, size_t len);

/*
 * Reads up to the specified number of bytes from the specified dynamic buffer into the specified destination,
 * starting at the current position and not proceeding past the current limit.
 * Returns the number of bytes transferred to the destination (which will be zero when no more are available and
 * when the third argument is zero).  
 */
static size_t cif_buf_read(read_buffer_t *buf, void *dest, size_t max);

/*
 * (macro) Returns the buffer's internal position to the begining, leaving its limit intact.
 */
#define cif_buf_rewind(buf) do { buf->position = 0; } while (0)

/*
 * (macro) Returns the (write) buffer's internal position to the beginning and resets its limit to 0.
 */
#define cif_buf_reset(buf) do { buf->position = 0; buf->limit = 0; } while (0)

/*
 * Fully initializes a list value as an empty list.  Does not allocate any additional resources.  Should not be
 * invoked on an already-initialized value, as doing so may create a resource leak
 */
static void cif_list_init(struct list_value_s *list_value);

/*
 * Fully initializes a table value as an table list.  Does not allocate any additional resources.  Should not be
 * invoked on an already-initialized value, as doing so may create a resource leak
 */
static void cif_table_init(struct table_value_s *table_value);

/*
 * Value-type specific value cleaning functions
 */
static void cif_char_value_clean(struct char_value_s *char_value);
static void cif_numb_value_clean(struct numb_value_s *numb_value);
static void cif_list_value_clean(struct list_value_s *list_value);
static void cif_table_value_clean(struct table_value_s *table_value);

/*
 * Clones a list value, assuming all fields of the target object contain garbage
 */
static int cif_value_clone_list(struct list_value_s *value, struct list_value_s *clone);

/*
 * Composite-value serialization and deserialization functions
 */
static int cif_list_serialize(struct list_value_s *list, write_buffer_t *buf);
static int cif_table_serialize(struct table_value_s *table, write_buffer_t *buf);
static int cif_list_deserialize(struct list_value_s *list, read_buffer_t *buf);
static int cif_table_deserialize(struct table_value_s *table, read_buffer_t *buf);

/**
 * @brief produces an unsigned digit-string representation of the specified double, rounded to the specified scale
 *
 * Leading zeroes are omitted, except that the result will contain a single '0' digit if @p d rounds to exactly zero.
 * Although the sign of @p d is not directly represented in the resulting digit string, it may affect the rounding
 * applied.
 *
 * @param[in] d the double value for which a digit string representation is requested
 * @param[in] scale the index of the least-significant decimal digit in the result string, relative to the (implied)
 *         decimal point and increasing as place-values decrease
 * @return a pointer to the generated digit string; the caller is responsible for freeing this when it is no longer
 *         needed.  NULL is returned if the result cannot be computed -- which is only the case if @p d contains an
 *         infinity or an NaN -- or if not enough space can be allocated to store it.
 */
static char *to_digits(double d, int scale);

/**
 * @brief produces an appropriately-rounded type-double representation of the specified digit string, as interpreted
 *     scale according to the specified scale.
 *
 * If the digit string contains more digits than can fit on one line of CIF text then the trailing digits may be
 * ignored.  In perverse cases, this could make a difference of one unit in the last digit of the result relative to
 * rounding based on the full digit string.  On an IEEE-754 system , this function will raise floating-point overflow
 * and underflow exceptions for some combinations of arguments; if those are not trapped then the result returned in
 * those cases will depend in part on the FP rounding mode then in effect.
 *
 * @param[in] ddigits a C string containing the decimal digits to convert, from most- to least-significant, expressed
 *         in the C locale or onne that provides a substantially equivalent representation of decimal digit characters
 * @param[in] scale the number of digits in @p ddigits that follow the implied decimal point.  May be greater than the
 *         length of @p ddigits (indicating implicit leading zeroes between the decimal point and the first digit of
 *         @p ddigits) or less than zero (indicating trailing insignificant zeroes between the last digit of @p ddigits
 *         and the decimal point)
 * @return the representable @c double value closest to the number jointly represented by the arguments
 */
static double to_double(const char *ddigits, int scale);

/**
 * @brief Formats the text representation of a number value, in plain decimal form
 *
 * The caller is expected to manage the numeric locale, which should be the C locale or a functional equivalent.  The
 * caller takes repsonsibility for cleaning up the returned Unicode string.
 *
 * @param[in] sign_num a number having the same sign as the value to be represented by the desired text
 * @param[in] digit_buf a C string containing the significant decimal digits of the numeric value to format, relative to
 *         the specified scale
 * @param[in] su_buf a C string containing the significant decimal digits of the standard uncertainty of the value,
 *         relative to the specified scale, or @c NULL if the number is exact
 * @param[in] su_size the number of characters (expected) in @p su_buf, or zero for an exact number
 * @param[in] scale the number of decimal places to the right of the (implied) decimal point where the least significant
 *         digits of @p digit_buf and (if applicable) @p su_buf are each located; must not be less than zero
 * @return a NUL-terminated Unicode string containing the formatted value
 */
static UChar *format_text_decimal(double sign_num, char *digit_buf, char *su_buf, size_t su_size, int scale);

/**
 * @brief Formats the text representation of a number value, in scientific notation
 *
 * The caller is expected to manage the numeric locale, which should be the C locale or a functional equivalent.  The
 * caller takes repsonsibility for cleaning up the returned Unicode string.
 *
 * @param[in] sign_num a number having the same sign as the value to be represented by the desired text
 * @param[in] digit_buf a C string containing the significant decimal digits of the numeric value to format, relative to
 *         the specified scale
 * @param[in] su_buf a C string containing the significant decimal digits of the standard uncertainty of the value,
 *         relative to the specified scale, or @c NULL if the number is exact
 * @param[in] su_size the number of characters (expected) in @p su_buf, or zero for an exact number
 * @param[in] scale the number of decimal places to the right of the (implied) decimal point where the least significant
 *         digits of @p digit_buf and (if applicable) @p su_buf are each located
 * @return a NUL-terminated Unicode string containing the formatted value
 */
static UChar *format_text_sci(double sign_num, char *digit_buf, char *su_buf, size_t su_size, int scale);

/**
 * @brief Counts the number of significant mantissa bits in the specified double value
 *
 * "Significant" mantissa bits are all the bits from the most significant bit through the least-significant non-zero
 * bit.  That is, the maximal string of zero bits at the least-significant end of the mantissa is omitted from the
 * count.  Additionally, the radix-FLT_RADIX order of magnitude of the double argument is returned.
 *
 * @param[in] d the double value in which to count bits
 * @param[in,out] exp the location where the radix-FLT_RADIX place value, plus one, of the most significant mantissa
 *         bit shall be recorded.
 */
static int frac_bits(double d, int *exponent);

#ifdef HAVE_STRDUP
#ifndef HAVE_DECL_STRDUP
#ifdef __cplusplus
extern "C" {
#endif
char *strdup(const char *s)
#ifdef __GNUC__
     __THROW __attribute_malloc__ __nonnull ((1))
#endif
;
#ifdef __cplusplus
}
#endif
#endif
#else
/* function definition serves as a declaration */
static char *strdup(const char *s) {
    int length = strlen(s);
    char *dup = (char *) malloc(length + 1);

    if (dup) {
        char *d = dup;

        while (*s != '\0') {
            *(d++) = *(s++);
        }
        dup[length] = '\0';
    }

    return dup;
}
#endif

/* function implementations */

static void cif_list_init(struct list_value_s *list_value) {
    list_value->kind = CIF_LIST_KIND;
    list_value->elements = NULL;
    list_value->size = 0;
    list_value->capacity = 0;
}

static void cif_table_init(struct table_value_s *table_value) {
    table_value->kind = CIF_TABLE_KIND;
    table_value->map.head = NULL;
    table_value->map.is_standalone = 1;
    table_value->map.normalizer = cif_normalize_table_index;
}

/*
 * Frees the components of a character-type value, but not the value object itself
 */
static void cif_char_value_clean(struct char_value_s *char_value) {
    CLEAN_PTR(char_value->text);
}

/*
 * Frees the components of a number-type value, but not the value object itself
 */
static void cif_numb_value_clean(struct numb_value_s *numb_value) {
    CLEAN_PTR(numb_value->text);
    CLEAN_PTR(numb_value->digits);
    CLEAN_PTR(numb_value->su_digits);
}

/*
 * Frees the components of a list-type value, but not the value object itself
 */
static void cif_list_value_clean(struct list_value_s *list_value) {
    for (; list_value->size > 0; ) {
        list_value->size -= 1;
        cif_value_free(list_value->elements[list_value->size]);
    }
    CLEAN_PTR(list_value->elements);
    list_value->capacity = 0;
}

/*
 * Frees the components of a table-type value, but not the value object itself
 */
static void cif_table_value_clean(struct table_value_s *table_value) {
    struct entry_s *entry;
    struct entry_s *temp;

    HASH_ITER(hh, table_value->map.head, entry, temp) {
        HASH_DEL(table_value->map.head, entry);
        cif_map_entry_free_internal(entry, &(table_value->map));
    }
}

/**
 * @brief Clones a number value, assuming all fields of the target object initially contain garbage.
 *
 * On success, @p clone is an independent copy of @p value.  Some fields of the @p clone object may be modified on
 * failure, but they will not refer to live resources.  Its kind is changed (to @c CIF_NUMB_KIND ) only on success.
 *
 * @param[in] value a pointer to the number value object to clone; must not be NULL
 * @param[out] clone a pointer to an uninitialized number value object to initialize as a copy @p value; must not be
 *         NULL
 *
 * @return @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure
 */
static int cif_value_clone_numb(struct numb_value_s *value, struct numb_value_s *clone) {
    FAILURE_HANDLING;

    clone->text = cif_u_strdup(value->text);
    if (clone->text != NULL) {
        clone->digits = strdup(value->digits);
        if (clone->digits != NULL) {
            if (value->su_digits != NULL) {
                clone->su_digits = strdup(value->su_digits);
                if (clone->su_digits == NULL) DEFAULT_FAIL(su);
            } else {
                clone->su_digits = NULL;
            }

            /* all needed allocations are successful; copy the rest of the properties */
            clone->sign = value->sign;
            clone->scale = value->scale;
            clone->kind = CIF_NUMB_KIND;

            /* success */
            return CIF_OK;

            FAILURE_HANDLER(su):
            free(clone->digits);
        }

        free(clone->text);
    }

    FAILURE_TERMINUS;
}

/*
 * Clones a list value, assuming all fields of the target object contain garbage
 */
static int cif_value_clone_list(struct list_value_s *value, struct list_value_s *clone) {
    FAILURE_HANDLING;

    cif_list_init(clone);
    clone->elements = (cif_value_t **) malloc(sizeof(cif_value_t *) * value->size);
    if (clone->elements != NULL) {
        clone->capacity = value->size;
        for (; clone->size < value->size; clone->size += 1) {
            int result;
            cif_value_t **element = clone->elements + clone->size;

            *element = NULL;
            result = cif_value_clone(value->elements[clone->size], element);
            if (result != CIF_OK) {
                FAIL(soft, result);
            }
        }

        /* success */
        return CIF_OK;
    }

    FAILURE_HANDLER(soft):
    cif_list_value_clean(clone);

    FAILURE_TERMINUS;
}

/*
 * Clones a table value, assuming all fields of the target object contain garbage
 */
static int cif_value_clone_table(struct table_value_s *value, struct table_value_s *clone) {
    FAILURE_HANDLING;
    struct table_value_s temp;
    struct entry_s *entry;
    struct entry_s *tmp;

    cif_table_init(&temp);
    HASH_ITER(hh, value->map.head, entry, tmp) {
        struct entry_s *new_entry = (struct entry_s *) malloc(sizeof(struct entry_s));

        if (new_entry != NULL) {
            new_entry->key = cif_u_strdup(entry->key);
            if (new_entry->key != NULL) {
                new_entry->key_orig = cif_u_strdup(entry->key_orig);
                if (new_entry->key_orig != NULL) {
                    cif_value_t *new_value = &(new_entry->as_value);

                    new_value->kind = CIF_UNK_KIND;
                    if (cif_value_clone(&(entry->as_value), &new_value) == CIF_OK) {
#undef  uthash_fatal
#define uthash_fatal(msg) DEFAULT_FAIL(hash)
                        HASH_ADD_KEYPTR(hh, temp.map.head, new_entry->key, U_BYTES(new_entry->key), new_entry);
                        continue;
                    }

                    FAILURE_HANDLER(hash):
                    free(new_entry->key_orig);
                }
                free(new_entry->key);
            }
            free(new_entry);
        }

        cif_table_value_clean(&temp);
        FAILURE_TERMINUS;
    }

    /* success */
    cif_table_init(clone);
    clone->map.head = temp.map.head;

    return CIF_OK;
}

/*
 * Serializes a list value, not including the initial value-type code.  May recurse, directly or indirectly.
 */
static int cif_list_serialize(struct list_value_s *list, write_buffer_t *buf) {
    if (cif_buf_write(buf, &(list->size), sizeof(size_t)) == CIF_OK) {
        size_t i;
        for (i = 0; i < list->size; i++) {
            SERIALIZE(list->elements[i], buf, fail);
        }
    }

    return CIF_OK;

    fail:
    return CIF_ERROR;
}

/*
 * Serializes a table value, not including the initial value-type code.  May recurse, directly or indirectly.
 */
static int cif_table_serialize(struct table_value_s *table, write_buffer_t *buf) {
    FAILURE_HANDLING;
    struct entry_s *element;
    struct entry_s *temp;
    int flag;

    HASH_ITER(hh, table->map.head, element, temp) {
        int result;

        /* serialize a flag indicating that another entry follows */
        flag = 0;
        if ((result = cif_buf_write(buf, &flag, sizeof(int))) != CIF_OK) {
            FAIL(element, result);
        } else {
            /* serialize the key */
            SERIALIZE_USTRING(element->key, buf, HANDLER_LABEL(element));
            /* serialize the un-normalized key, or NULL if it is the same object as the key */
            SERIALIZE_USTRING(((element->key_orig == element->key) ? NULL : element->key_orig), buf,
                    HANDLER_LABEL(element));
            /* serialize the value */
            SERIALIZE(element, buf, HANDLER_LABEL(element));
        }
    }

    /* Serialize a flag indicating that no more entries follow */
    flag = -1;
    return cif_buf_write(buf, &flag, sizeof(int));

    FAILURE_HANDLER(element):
    FAILURE_TERMINUS;
}

/*
 * Deserializes a list value from the given buffer into the specified, existing value object.
 * Returns CIF_OK on success, or an error code on failure.  May recurse, directly or indirectly.
 */
static int cif_list_deserialize(struct list_value_s *list, read_buffer_t *buf) {
    FAILURE_HANDLING;
    size_t capacity;

    if (cif_buf_read(buf, &capacity, sizeof(size_t)) == sizeof(size_t)) { 
        if (capacity == 0) {
            /* an empty list */
            cif_list_init(list);
            list->elements = NULL;
            list->capacity = 0;
            list->size = 0;
            return CIF_OK;
        } else {
            cif_value_t **elements = (cif_value_t **) malloc(sizeof(cif_value_t *) * capacity);

            if (elements != NULL) {
                size_t size;

                for (size = 0; size < capacity; size += 1) {
                    elements[size] = NULL;  /* essential */
                    DESERIALIZE(cif_value_t, elements[size], buf, HANDLER_LABEL(element));
                }

                /* the 'list' argument is modified only now that we have successfully deserialized a whole list */
                cif_list_init(list);
                list->elements = elements;
                list->capacity = capacity;
                list->size = size;
                return CIF_OK;

                FAILURE_HANDLER(element):
                while (size > 0) {
                    size -= 1;
                    cif_value_free(elements[size]);
                }
                free(elements);
            }
        }
    }

    FAILURE_TERMINUS;
}

/*
 * Deserializes a table value from the given buffer into the specified, existing value object.  The value object
 * is assumed to hold no resources -- either it is uninitialized, or it is a valid value of kind CIF_UNK_KIND
 * or CIF_NA_KIND.
 *
 * Returns CIF_OK on success, or an error code on failure.  May recurse, directly or indirectly.
 */
static int cif_table_deserialize(struct table_value_s *table, read_buffer_t *buf) {
    FAILURE_HANDLING;
    cif_value_t temp;
    UChar *key;
    UChar *key_orig;
    struct entry_s *entry;

    cif_table_init(&(temp.as_table));
    for (;;) {
        int flag;

        if (cif_buf_read(buf, &flag, sizeof(int)) != sizeof(int)) {
            DEFAULT_FAIL(key);
        } else {
            switch (flag) {
                case -1:  /* no more entries */
                    cif_table_init(table);
                    table->map.head = temp.as_table.map.head;
                    return CIF_OK;
                case 0:  /* another entry is available */
                    key = NULL;
                    DESERIALIZE_USTRING(key, buf, HANDLER_LABEL(key));
                    key_orig = NULL;
                    DESERIALIZE_USTRING(key_orig, buf, HANDLER_LABEL(key_orig));
                    entry = NULL;
                    DESERIALIZE(struct entry_s, entry, buf, HANDLER_LABEL(value));
                    entry->key = key;
                    entry->key_orig = ((key_orig == NULL) ? key : key_orig);
#undef  uthash_fatal
#define uthash_fatal(msg) DEFAULT_FAIL(hash)
                    HASH_ADD_KEYPTR(hh, temp.as_table.map.head, entry->key, U_BYTES(entry->key), entry);
                    break;
                default:
                    FAIL(key, CIF_INTERNAL_ERROR);
            }
        }
    }

    FAILURE_HANDLER(hash):
    cif_value_free(&(entry->as_value));

    FAILURE_HANDLER(value):
    free(key_orig);

    FAILURE_HANDLER(key_orig):
    free(key);

    FAILURE_HANDLER(key):
    cif_value_clean(&temp);

    FAILURE_TERMINUS;
}

static UChar *format_text_decimal(double sign_num, char *digit_buf, char *su_buf, size_t su_size, int scale) {
    int val_digits = strlen(digit_buf);
    size_t total_chars = 
              ((sign_num < 0) ? 1 : 0)                            /* sign */
            + ((val_digits <= scale) ? (scale + 1) : val_digits)  /* value digits, including leading zeroes */
            + ((scale == 0) ? 0 : 1)                              /* decimal point */
            + ((su_size > 0) ? (su_size + 2) : 0)                 /* su, including parentheses */
            + 1;                                                  /* terminator */

    if (total_chars <= CIF_LINE_LENGTH + 1) {
        UChar *text = (UChar *) malloc(total_chars * sizeof(UChar));

        if (text != NULL) {
            UChar *c = text;
            char *next_digit = digit_buf;
            int whole_digits = val_digits - scale;

            /* The sign, if needed */
            if (sign_num < 0) *(c++) = UCHAR_MINUS;

            if (whole_digits <= 0) {
                /* leading zeroes */
                *(c++) = UCHAR_0;
                *(c++) = UCHAR_DECIMAL;
                while (whole_digits++ < 0) {
                    *(c++) = (UChar) '0';
                }
            } else {
                /* significant whole-number digits */
                do {
                    *(c++) = (UChar) *(next_digit++);
                } while (--whole_digits > 0);
                if (scale > 0) *(c++) = UCHAR_DECIMAL;
            }

            /* significant fraction digits and uncertainty */
            USTRCPY_C(c, next_digit);
            WRITE_SU(c, su_buf);

            return text;
        }
    } /* else formatted representation is too long; should be possible only for crazy scales */

    return NULL;
}

static UChar *format_text_sci(double sign_num, char *digit_buf, char *su_buf, size_t su_size, int scale) {
    int val_digits = strlen(digit_buf);
    int most_significant_place = (val_digits - 1) - scale;
    int exponent_digits = ((int) log10(abs(most_significant_place) + 0.5)) + 1;  /* adding 0.5 avoids log10(0) */
    size_t total_chars;

    assert(val_digits > 0);

    if (exponent_digits < 2) exponent_digits = 2;

    total_chars = 
              ((sign_num < 0) ? 1 : 0)                   /* sign */
            + ((val_digits > 1) ? (val_digits + 1) : 1)  /* value digits and decimal point */
            + exponent_digits + 2                        /* exponent sigil, sign, and digits */
            + ((su_size > 0) ? (su_size + 2) : 0)        /* su, including parentheses */
            + 1;                                         /* terminator */

    if (total_chars <= CIF_LINE_LENGTH + 1) {
        UChar *text = (UChar *) malloc(total_chars * sizeof(UChar));

        if (text != NULL) {
            UChar *c = text;
            char *next_digit = digit_buf;
            int i;

            /* The sign, if needed */
            if (sign_num < 0) *(c++) = UCHAR_MINUS;

            *(c++) = (UChar) *(next_digit++);

            if (*next_digit != '\0') {
                *(c++) = UCHAR_DECIMAL;
                USTRCPY_C(c, next_digit);
            }
            *(c++) = UCHAR_e;
            if (most_significant_place < 0) {
                *(c++) = UCHAR_MINUS;
                most_significant_place = -most_significant_place;
            } else {
                *(c++) = UCHAR_PLUS;
            }
            for (i = exponent_digits; i-- > 0; ) {
                *(c + i) = (UChar) ((most_significant_place % 10) + UCHAR_0);
                most_significant_place /= 10;
            }
            c += exponent_digits;

            WRITE_SU(c, su_buf);

            return text;
        }
    } /* else formatted representation is too long; should be possible only for crazy scales */

    return NULL;
}

/*
 * The floor of the binary logarithm of an unsigned, 8-bit (or less) integer.  Although usable on its own, this is
 * intended mainly as a helper for the LOG2_16BIT macro, which is itself a helper for the LOG2_32BIT macro.
 *
 * This is where the magic happens.
 */
#define LOG2_8BIT(v)  (8 - 96/((((v)/4)|1)+16) - 10/(((v)|1)+2))

/*
 * The floor of the binary logarithm of an unsigned, 16-bit (or less) integer.  Although usable on its own, this is
 * intended mainly as a helper for the LOG2_32BIT macro.
 */
#define LOG2_16BIT(v) ((((v)>255U)?8:0) + LOG2_8BIT((v)>>(((v)>255U)?8:0)))

/**
 * @brief the floor of the binary logarithm of the argument, as a 32-bit unsigned integer
 *
 * The argument is cast to @c uint32_t, which may overflow if it is of a wider integral type or of a floating-point
 * type, and may be truncated or underflow if the argument is of a floating-point type.  This macro evaluates to
 * zero when the argument is zero.  The expression it expands to is a compile-time constant when the argument is
 * itself one.
 */
#define LOG2_32BIT(v) (((((uint32_t)(v))>65535L)?16:0) + LOG2_16BIT(((uint32_t)(v))>>((((uint32_t)(v))>65535L)?16:0)))

/* The bignum base, which must be a power of 10 */
#define BBASE 1000000000

/*
 * FIXME: Parts of the following assume FLT_RADIX == 2
 */

/*
 * The number of binary digits that can be completely covered by one bignum (base-BBASE) digit = floor(log[2](BBASE)) - 1
 */
#define BDIG_PER_DIG (LOG2_32BIT(BBASE) - 1)

/* The number of decimal digits represented by one bignum digit = log[10](BBASE) (assuming BBASE is a power of 10) */
#define DDIG_PER_DIG 9

/* The number of bignum digits needed for the largest possible integer part of a double */
#define INT_DIGITS ((DBL_MAX_10_EXP + DDIG_PER_DIG - 1) / DDIG_PER_DIG)

/* The number of bignum digits needed for the most precise possible fractional part of a double */
/* One decimal digit is needed for each binary fraction digit in order to represent the the binary fraction exactly */
#define FRAC_DIGITS (((DBL_MANT_DIG - DBL_MIN_EXP) + DDIG_PER_DIG - 1) / DDIG_PER_DIG)

/* The number of fixed-point bignum digits required to represent the full range of a double values */
#define DIG_PER_DBL (INT_DIGITS + FRAC_DIGITS)

/* The index of the bignum units digit */
#define UNITS_DIGIT (INT_DIGITS - 1)

/* The maximum value of the mantissa bits of a double, when interpreted as an unsigned integer */
#define MAX_MANTISSA  ((((uintmax_t) 1) << DBL_MANT_DIG) - 1)

/* The minimum value of the mantissa bits of a normalized, non-zero double, when interpreted as an unsigned integer */
#define MIN_MANTISSA  (((uintmax_t) 1) << (DBL_MANT_DIG - 1))

/*
 * The offset to the left of the units digit of the most significant bignum digit of an integer mantissa.  Although the
 * computation is approximate in principle, on account of the definition BDIG_PER_DIG, the result is nevertheless
 * exactly correct for all binary formats defined by IEEE 754-2008.
 */
#define MAX_MANT_MSD_OFFSET (((DBL_MANT_DIG + BDIG_PER_DIG - 1) / BDIG_PER_DIG) - 1)

/**
 * @brief A helper function for to_digits() that tests for an exact-zero tail to a base-BBASE bignum.
 *
 * @param[in] check_value a base-BBASE digit representing the most-significant portion of the insignificant digits
 *     to test, scaled by a power of 10 to a value between BBASE / 10 (inclusive) and BBASE (exclusive)
 * @param[in] work_dig a pointer to the bignum digit from which @p check_value is derived
 * @param[in] lsd a pointer to the least-significant digit of the full bignum value from which the other
 *     parameters are drawn; no bignum digits past this one will be considered
 * @return 1 if the digit tail represented by the arguments has all digits zero; otherwise zero
 */
static int is_zero(uint32_t check_value, uint32_t *work_dig, uint32_t *lsd) {
    if (check_value != 0) return 0;
    while (work_dig < lsd) if (*(++work_dig) != 0) return 0;
    return 1;
}

/**
 * @brief A helper function for to_digits() that compares the tail of a base-BBASE bignum with one half the value of
 *     the immediately preceding decimal digit.
 *
 * @param[in] check_value a base-BBASE digit representing the most-significant portion of the insignificant digits
 *     to test, scaled by a power of 10 to a value between BBASE / 10 (inclusive) and BBASE (exclusive)
 * @param[in] work_dig a pointer to the bignum digit from which @p check_value is derived
 * @param[in] lsd a pointer to the least-significant digit of the full bignum value from which the other
 *     parameters are drawn; no bignum digits past this one will be considered
 * @return a value less than, equal to, or greater than zero, corresponding to whether the digit tail represented
 *     by the arguments is less than, equal to, or greater than one half the value of the preceeding digit
 */
static int compare_half(uint32_t check_value, uint32_t *work_dig, uint32_t *lsd) {
    if (check_value < (BBASE / 2)) return -1;
    if ((check_value == (BBASE / 2)) && ((work_dig++ == lsd) || (is_zero(*work_dig, work_dig, lsd) != 0))) return 0;
    return 1;
}

/**
 * @brief Rounds the specified value based on an associated bignum tail
 *
 * The @p check_value and @p check_digit parameters are separate to allow clients to employ this function to round to
 * decimal digits that do not occur at the lower boundary of a bignum digit.  In that case, the @p round_value must be
 * scaled down by a power of ten to bring the desired decimal digit to the boundary, the result must be scaled back up
 * by the same power of 10, and the @p check_value must be scaled up by that power of 10, modulo BBASE.
 *
 * @param[in] negative nonzero if the bignum is negative, else zero
 * @param[in] round_value the least-significant bignum digit of the value to round
 * @param[in] check_value the value of the first bignum digit in the tail
 * @param[in] check_digit the index of the bignum digit from which @p check_value is drawn
 * @param[in] lsd a pointer to the least-significant digit in the bignum tail
 * @return the result of rounding @p round_value; no protection against overflow is provided
 */
static uint32_t round_it(int negative, uint32_t round_value, uint32_t check_value, uint32_t *check_digit,
        uint32_t *lsd) {
    int compare;

#ifdef HAVE_FEGETROUND
    /* C89 provides no standard way to do this, so we uses the C99 way if it is available */
    switch (fegetround()) {
        case FE_TOWARDZERO:
            /* truncate the tail; no attention is required to any of its digits */
            return round_value;
        case FE_DOWNWARD:
            /* round any fractional part downward, which is equivalent to truncation for non-negative numbers */
            return round_value + (((negative != 0) && (is_zero(check_value, check_digit, lsd) == 0)) ? 1 : 0);
        case FE_UPWARD:
            /* round any fractional part upward, which is equivalent to truncation for negative numbers */
            return round_value + (((negative == 0) && (is_zero(check_value, check_digit, lsd) == 0)) ? 1 : 0);
        default:
            /* unknown rounding modes are treated via the default mode, but these should not be encountered */
        case FE_TONEAREST:
#endif
            /*
             * This is the IEEE 754 default rounding mode, and the only one that will be used here if
             * fegetround() is not available to determine the actual rounding mode currently set.
             */
            compare = compare_half(check_value, check_digit, lsd);
            if (compare > 0) {
                return round_value + 1;
            } else if (compare == 0) {
                return ((round_value + 1) & ~((uint32_t) 1));
            } else {
                return round_value;
            }
#ifdef HAVE_FEGETROUND
    }
#endif
}

/**
 * @brief Applies a rounding correction to the specified integer value based on the fractional digits of the specified
 *         bignum
 *
 * @param[in] round_value the integer part of the number to round
 * @param[in] digits the bignum containing the fraction digits by which to compute a rounding correction
 * @param[in] units_digit the digit index in @p digits of the units digit
 * @param[in] lsd a pointer to the least-significant digit in @p digits
 * @return the correctly-rounded value, either @p round_value or @p round_value + 1, depending on the significant
 *         fractional digits of the bignum and the rounding mode currently in effect
 */
static uintmax_t round_to_int(uintmax_t round_value, uint32_t *digits, int units_digit, uint32_t *lsd) {
    uint32_t *work_digit = digits + units_digit + 1;

    return round_value + ((lsd < work_digit) ? 0 : round_it(0, 0, *work_digit, work_digit, lsd));
}

static char *to_digits(double d, int scale) {
    int negative;

    if (d != d) {
        /* not-a-number */
        return NULL;
    } else if (d < 0) {
        negative = 1;
        d = -d;
    } else {
        negative = 0;
    }

    if (d == 0.0) {
        return strdup("0");
    } else if (d > DBL_MAX) {
        /* infinite */
        return NULL;
    } else {
        uint32_t digits[DIG_PER_DBL + 1];

        /* uintmax_t is assumed at least DBL_MANT_DIGITS wide */
        uintmax_t fraction;

        uint32_t *msd = digits + UNITS_DIGIT;
        uint32_t *lsd;
        uint32_t *work_dig;
        uint32_t check_value;
        uint32_t p10;
        int round_digit;
        int extra_ddigits;
        char *result;
        int i;

        /* exponent is the base-2 exponent of d when the mantissa is expressed as an integer */
        int exponent;

        /* clear the work space */
        for (i = 0; i <= DIG_PER_DBL; i += 1) {
            digits[i] = 0;
        }

        /* assumes uintmax_t is at least DBL_MANT_DIG bits wide */
        /* assumes DBL_MAX_EXP > DBL_MANT_DIG */
        /* assumes ldexp() and frexp() introduce no rounding error */
        d = frexp(d, &exponent);
        for (fraction = (uintmax_t) ldexp(d, DBL_MANT_DIG); fraction > 0; ) {
            *(msd--) = fraction % BBASE;
            fraction /= BBASE;
        }
        /* correct the exponent for the bias we introduced via ldexp() */
        exponent -= DBL_MANT_DIG;

        /* ensure msd points at the most-significant bignum digit */
        msd += 1;
        /* ensure lsd points at the least-significant bignum digit */
        for (lsd = digits + UNITS_DIGIT; *lsd == 0; lsd -= 1);

        /* apply the binary exponent */

        while (exponent < 0) {
            /* This is the usual case because exponent is biased by -DBL_MANT_DIG (-53 for the IEEE 754 64-bit format) */

            /* Limit the shifts to simplify bookkeeping, spreading the division over multiple cycles if necessary */
            uint32_t shift = ((BDIG_PER_DIG < (uint32_t) -exponent) ? BDIG_PER_DIG : (uint32_t) -exponent);
            uint64_t remainder = 0;

            /* perform a digit-by-digit division via bit shift, from most- to least-significant digit */
            /* this is a variant of long division */
            work_dig = msd;
            do {
                uint64_t dividend = remainder + *work_dig;
                
                *work_dig = (uint32_t) (dividend >> shift);
                remainder = (dividend & ((((uint64_t) 1) << shift) - 1)) * BBASE;
            } while ((work_dig++ <= lsd) || (remainder != 0));

            /* track the least-significant nonzero bignum digit */
            lsd = work_dig - 1;
            /* track the most-significant nonzero bignum digit */
            while (*msd == 0) msd += 1;
            exponent += shift;
        }

        while (exponent > 0) {
            /* This case will be triggered only for very large numbers */
            uint32_t shift = ((BDIG_PER_DIG < (uint32_t) exponent) ? BDIG_PER_DIG : (uint32_t) exponent);
            uint64_t carry = 0;

            /* perform a digit-by-digit multiplication via bit shift, from least- to most-significant digit */
            /* this is a variant of long multiplication */
            work_dig = lsd;
            do {
                /* This will not overflow as long as BDIG_PER_DIG < 32: */
                uint64_t product = (((uint64_t) *work_dig) << shift) + carry;

                *work_dig = (uint32_t) (product % BBASE);
                carry =                (product / BBASE);
            } while ((--work_dig >= msd) || (carry != 0));

            /* track the most-significant nonzero bignum digit */
            msd = work_dig + 1;
            /* track the least-significant nonzero bignum digit */
            while (*lsd == 0) lsd -= 1;
            exponent -= shift;
        }

        /*
         * Now round it
         */

        /* Take care: C integer division is partially implementation-dependent when the dividend is negative */
        round_digit = UNITS_DIGIT
                + ((scale <= 0) ? -((-scale) / DDIG_PER_DIG) : ((scale + DDIG_PER_DIG - 1) / DDIG_PER_DIG));

        p10 = 1;
        if (round_digit < DIG_PER_DBL) {
            int round_pos = ((-scale) % DDIG_PER_DIG);

            /* C does not define the sign of the modulus result when any argument is negative, as round_to may be */
            if (round_pos < 0) round_pos += DDIG_PER_DIG;

            /* Capture the first few insignificant digits, and clear them from the result if necessary */
            if (round_pos == 0) {
                work_dig = digits + round_digit + 1;
                check_value = *work_dig;
            } else {
                for (; round_pos > 0; round_pos -= 1) p10 *= 10;
                work_dig = digits + round_digit;
                check_value = (*work_dig % p10);
                /* clear the check value from the result */
                *work_dig -= check_value;
                /* scale the check value to be comparable with the other case's */
                check_value *= (BBASE / p10);
            }

            digits[round_digit] = p10 * round_it(negative, digits[round_digit] / p10, check_value, work_dig, lsd);

            /* Set the new lsd according to the rounding */
            lsd = digits + round_digit;

            if (lsd < msd) {
                msd = lsd;
                /* no carry needed */
                assert(*msd <= 1);
                p10 = 1;
            } else {
                /* Complete the rounding by applying any carry digit(s) -- iteratively, if necessary */
                for (work_dig = lsd; *work_dig >= BBASE; ) {
                    /* This can overflow 'digits' only if DBL_MAX_10_EXP is divisible by DDIG_PER_DIG */
                    uint32_t carry = *(work_dig--) / BBASE;

                    *work_dig += carry;
                }

                /* update the most-significant digit if necessary */
                if (work_dig < msd) msd = work_dig;
            }

            extra_ddigits = 0;
            result = (char *) malloc((1 + lsd - msd) * DDIG_PER_DIG + 1);
        } else { /* extra precision */
            /* This is a fallback case.  Exercising this code probably indicates user error. */
            int int_digits = 1 + UNITS_DIGIT - (int) (msd - digits);
            int ddigit_count = scale + ((int_digits < 0) ? 0 : (int_digits * DDIG_PER_DIG));

            lsd = digits + DIG_PER_DBL - 1;
            extra_ddigits = scale - ((DIG_PER_DBL - (UNITS_DIGIT + 1)) * DDIG_PER_DIG);
            result = (char *) malloc(ddigit_count + 1);
        }

        /* generate and return the digit string */
        if (result != NULL) {
            char *work = result;

            /* count the number of significant decimal digits in the most-significant bignum digit, forcing at least one */
            for (i = 1, check_value = (*msd) / 10; check_value > 0; check_value /= 10) {
                i += 1;
            }

            for (work_dig = msd; work_dig <= lsd; work_dig += 1, i = DDIG_PER_DIG) {
                int j;

                for (j = i; j-- > 0; ) {
                    /* assumes the 'C' locale or one sufficiently similar: */
                    *(work + j) = (char) ((*work_dig % 10) + '0');
                    *work_dig /= 10;
                }
                work += i;
            }

            if (extra_ddigits > 0) {
                /* extend the digit string with extra zeroes */
                do {
                    *(work++) = '0';
                } while (--extra_ddigits > 0);
            } else {
                /* truncate the digit string after the last significant digit */
                while (p10 > 1) {
                    work -= 1;
                    p10 /= 10;
                }
            }

            /* add the string terminator */
            *work = '\0';
        }

        return result;
    }
}

/* FIXME: parts of the following assume DBL_MANT_DIG is not more than 64 and that FLT_RADIX is 2 */
static double to_double(const char *ddigits, int scale) {
    /* skip leading zeroes: */
    while (*ddigits == '0') ddigits++;

    if (*ddigits == '\0') {
        /* all digits are zero */
        return 0.0;
    } else {
        /* the least-significant decimal place in the input */
        int lsp = -scale;
        /* the most-significant decimal place in the input */
        int msp;
        const char *last_ddig;
       
        /* Determine the most significant place by counting digits */ 
        msp = lsp;
        for (last_ddig = ddigits + 1; *last_ddig != '\0'; last_ddig += 1) msp += 1;
        /* ignore trailing zeroes: */
        while (*(--last_ddig) == '0') lsp += 1;

        assert (msp >= lsp);

        /*
         * Truncate super-long digit strings.  This may introduce up to 1 ULP of rounding error for such digit strings,
         * but the limit is set so that any number expressible as a numeric literal in a CIF document is rounded
         * correctly.  This goes far beyond the precision of any binary numeric format available on any hardware known
         * to the author at the time of this writing.
         */
        if ((msp - lsp) >= CIF_LINE_LENGTH) {
            lsp = 1 + msp - CIF_LINE_LENGTH;
            last_ddig = ddigits + (msp - lsp);
        }

        /*
         * Handle inputs that defy normal representation in the system's 'double' format
         */
        if (msp > DBL_MAX_10_EXP) {
            /*
             * This should raise an 'Overflow' FP exception (which is sensible under the circumstances).  If it is not
             * trapped then the result depends on the current FP rounding mode: in some modes, including the default
             * mode, the result should be positive infinity; in the others, it should be DBL_MAX, the maximum
             * representable double.
             */
            return DBL_MAX * FLT_RADIX;
        } else if (msp <= (DBL_MIN_10_EXP - DBL_DIG)) {
            /*
             * This should raise an 'Underflow' FP exception (which is sensible under the circumstances); if it is not
             * trapped then the result should be zero.
             *
             * Note: other inputs can also raise 'Underflow' FP exceptions, but those ultimately yield denormalized FP
             * results if the exception is not trapped.
             */
            return DBL_MIN / pow(FLT_RADIX, DBL_MAX_EXP - 1);
        } else {
            /*
             * We know (DBL_MIN_10_EXP - DBL_DIG) < msp <= DBL_MAX_10_EXP, because of the conditions above.
             * Also, we ensured above that the logical digit string has msp - CIF_LINE_LENGTH < lsp.
             * 
             * It is necessary to consider additional significand digits required when right-shifting the value:
             * Each bit of right shift requires one additional decimal digit.  Thus, for significands that exceed
             * the maximimum integral mantissa value, each additional integer digit produces an ultimate requirement
             * of up to four fractional digits in the scaled significand. Therefore, the most decimal digits that
             * can be required to be able to accommodate all computations are those required for the maximum msp
             * and maximum-length digit string of all '9'.  The algorithm employed is slightly sloppy, however,and
             * may require one more decimal digit than is absolutely necessary.
             *
             * if msp == DBL_MAX_10_EXP (its maximum possible value here) and the digit string has CIF_LINE_LENGTH
             * digits (its maximum), and DBL_MAX_10_EXP < CIF_LINE_LENGTH (true for IEEE 754 doubles), then the lsp is
             * 1 + DBL_MAX_10_EXP - CIF_LINE_LENGTH.  The number of additional decimal digits needed is then
             *
             * floor(log2(10^(DBL_MAX_10_EXP + 1) - 1)) - DBL_MANT_DIG
             *     <= floor(log2(10^(DBL_MAX_10_EXP + 1))) - DBL_MANT_DIG
             *      = floor((DBL_MAX_10_EXP + 1) * log2(10)) - DBL_MANT_DIG
             *     <= floor((DBL_MAX_10_EXP + 1) * 3.322) - DBL_MANT_DIG
             *      = floor(((DBL_MAX_10_EXP + 1) * 3322.) / 1000.) - DBL_MANT_DIG
             *      = (((DBL_MAX_10_EXP + 1) * 3322) / 1000) - DBL_MANT_DIG
             *
             * That provides a reliable upper bound for the number of decimal digits required in the event that the
             * significand needs to be right shifted for its most-significant bits to fit into the number of mantissa
             * bits.  On the other hand, however, it is conceivable that for some values of the constants, the initial,
             * unshifted value may have a lesser lsp, but no less than:
             * (2 + DBL_MIN_10_EXP - DBL_DIG) - CIF_LINE_LENGTH
             */
#define     DBL_MANT_10_DIG (3 * (DBL_MANT_DIG  / 10))
#define     ULT_LSP_ALT1 ((DBL_MANT_10_DIG + DBL_MANT_DIG - CIF_LINE_LENGTH) \
                    - (((DBL_MAX_10_EXP + 1) * 2322) / 1000))
#define     ULT_LSP_ALT2 ((2 + DBL_MIN_10_EXP - DBL_DIG) - CIF_LINE_LENGTH)
#if (ULT_LSP_ALT1 < ULT_LSP_ALT2)
#define     BIGNUM_DIGITS (((DBL_MAX_10_EXP + DDIG_PER_DIG - 1) / DDIG_PER_DIG) \
                    + ((DDIG_PER_DIG - (ULT_LSP_ALT1 + 1)) / DDIG_PER_DIG))
#else
#define     BIGNUM_DIGITS (((DBL_MAX_10_EXP + DDIG_PER_DIG - 1) / DDIG_PER_DIG) \
                    + ((DDIG_PER_DIG - (ULT_LSP_ALT2 + 1)) / DDIG_PER_DIG))
#endif

            /* The internal bignum representation that will serve as an intermediate representation */
            uint32_t digits[BIGNUM_DIGITS];
            int units_digit;

            int right_shift_min;
            int right_shift_max;

            /* a binary exponent tracking the internal scaling performed during this computation */
            int exponent = 0;

            /* pointers tracking the most- and least-significant bignum digits in the number */
            uint32_t *msd;
            uint32_t *lsd;

            /* The extreme *decimal* places that may need to be supported in this computation */
            int lsp_min;
            int msp_max;

            /* data tracking the state of processing of the input digit string */
            uint32_t *next_dig;
            int ddigits_left;
            const char *next_ddig;

            /*
             * The C89 math library does not define log2(); we work around using log10(), and for that purpose compute
             * and retain log10(2.0).
             */
            static int is_init = 0;
            static double log_2 = 0;

            /* a workspace for assembling the integer value of the mantissa */
            uintmax_t mantissa_bits;

            int i;

            if (is_init == 0) {
                log_2 = log10(2.0);
                is_init = 1;
            }

            /* Note: log(x)/log(b) == log_base_b(x) */
            right_shift_min = 1 + floor((log10(*ddigits - '0') + msp) / log_2) - DBL_MANT_DIG;
            right_shift_max = 1 + floor((log10(1 + *ddigits - '0') + msp) / log_2) - DBL_MANT_DIG;

            assert ((right_shift_max - right_shift_min) < 2);

            /* convert to base-BBASE bignum */

            /* Determine which decimal digit positions may be needed */
            if (right_shift_max > 0) {
                /* Each right shift by one bit requires an additional decimal digit. */
                lsp_min = lsp - right_shift_max;
                msp_max = msp;
            } else if (right_shift_min < 0) {
                /*
                 * Each left shift by log2(10) bits, or fraction thereof, requires an additional decimal digit.
                 * We approximate with a few more than may be needed, by (under)estimating log2(10) as 3.
                 */
                lsp_min = lsp;
                msp_max = msp + ((2 - right_shift_min) / 3);
            } else {
                lsp_min = lsp;
                msp_max = msp;
            }

            assert(msp_max >= 0);

            /* compute the units digit position in the bignum digit string */
            units_digit = msp_max / DDIG_PER_DIG;

            /* clear the bignum digits */
            for(i = 0; i < BIGNUM_DIGITS; i += 1) {
                digits[i] = 0;
            }

            next_dig = digits + units_digit
                    + ((msp >= 0) ? -(msp / DDIG_PER_DIG) : (((DDIG_PER_DIG - 1) - msp) / DDIG_PER_DIG));
            msd = next_dig;

            /* read digits into the bignum */
            ddigits_left = ((msp >= 0) ? ((msp % DDIG_PER_DIG) + 1) : (DDIG_PER_DIG - ((-msp - 1) % DDIG_PER_DIG)));
            for (next_ddig = ddigits; next_ddig <= last_ddig; next_ddig += 1) {
                assert(ddigits_left > 0);
                *next_dig = (*next_dig * 10) + (*next_ddig - '0');
                if (--ddigits_left == 0) {
                    next_dig += 1;
                    ddigits_left = DDIG_PER_DIG;
                }
            }
            /* add trailing decimal zeroes as necessary to fill out the bignum digit */
            assert(ddigits_left > 0);
            if (ddigits_left < DDIG_PER_DIG) {
                do {
                    *next_dig *= 10;
                } while (--ddigits_left > 0);
            } /* else the last bignum digit was already exactly filled */
            lsd = next_dig;

            /*
             * scale the significand to use DBL_MANT_DIG radix-FLT_RADIX integer digits.
             *
             * Between right_shift_min and right_shift_max (if they differ), right_shift_max is chosen as the target
             * shift, because even if it is off by one (it cannot be off by more), a correction may still be unneeded
             * if there are no nonzero fractional digits after the initial scaling.  The same would not be true if
             * right_shift_min were chosen and was off by one.
             */

            if (right_shift_max > 0) {
                /* shift right */
                while (exponent < right_shift_max) {
                    uint32_t shift = (right_shift_max - exponent);
                    uint64_t remainder = 0;

                    if (shift > BDIG_PER_DIG) {
                        shift = BDIG_PER_DIG;
                    }
                    for (next_dig = msd; (next_dig <= lsd) || (remainder != 0); next_dig += 1) {
                        uint64_t dividend = remainder + *next_dig;

                        *next_dig = (dividend >> shift);
                        remainder = (dividend & ((((uint64_t) 1) << shift) - 1)) * BBASE;
                    }
                    lsd = next_dig - 1;
                    while (*msd == 0) msd += 1;
                    exponent += shift;
                }
            } else if (right_shift_max < 0) {
                /* shift left -- the usual case */
                while ((exponent > right_shift_max) && ((lsd - digits) > units_digit)) {
                    uint32_t shift = (exponent - right_shift_max);
                    uint64_t carry = 0;

                    if (shift > BDIG_PER_DIG) {
                        shift = BDIG_PER_DIG;
                    }
                    for (next_dig = lsd; (next_dig >= msd) || (carry != 0); next_dig -= 1) {
                        uint64_t product = (((uint64_t) *next_dig) << shift) + carry;

                        *next_dig = (product % BBASE);
                        carry = (product / BBASE);
                    }
                    msd = next_dig + 1;
                    while (*lsd == 0) lsd -= 1;
                    exponent -= shift;
                }
            } /* else no scaling is needed */

            /* compute the integer mantissa, applying a one-bit left shift if it turns out to be needed */
            while (CIF_TRUE) {  /* at most two iterations are expected */
                for (mantissa_bits = 0, next_dig = msd; next_dig <= (digits + units_digit); next_dig += 1) {
                    mantissa_bits = (mantissa_bits * BBASE) + *next_dig;
                }
                assert(mantissa_bits <= MAX_MANTISSA);

                if ((mantissa_bits >= MIN_MANTISSA) || ((lsd - digits) <= units_digit)) {
                    break;
                } else {  /* this alternative should be exercised at most once */
                    uint64_t carry = 0;

                    for (next_dig = lsd; (next_dig >= msd) || (carry != 0); next_dig -= 1) {
                        uint64_t product = (((uint64_t) *next_dig) << 1) + carry;

                        *next_dig = (product % BBASE);
                        carry = (product / BBASE);
                    }
                    msd = next_dig + 1;
                    while (*lsd == 0) lsd -= 1;
                    exponent -= 1;
                }
            }

            mantissa_bits = round_to_int(mantissa_bits, digits, units_digit, lsd);

            /* account for overflow during rounding */
            if (mantissa_bits > MAX_MANTISSA) {
                mantissa_bits = 1;
                exponent += DBL_MANT_DIG;
            }

            /* compute and return the result */
            return ldexp((double) mantissa_bits, exponent);
        }
    }
}

static buffer_t *cif_buf_create(size_t cap) {
    buffer_t *buf = (buffer_t *) malloc(sizeof(buffer_t));

    if (buf != NULL) {
        buf->for_writing.start = (char *) malloc(cap);
        if (buf->for_writing.start != NULL) {
            buf->for_writing.capacity = cap;
            buf->for_writing.limit = 0;
            buf->for_writing.position = 0;
            return buf;
        }
        free(buf);
    }
    return NULL;
}

static void cif_buf_free(write_buffer_t *buf) {
    if (buf != NULL) {
        if (buf->start != NULL) {
            free(buf->start);
        }
        free(buf);
    }
}

static int cif_buf_write(write_buffer_t *buf, const void *src, size_t len) {
    size_t needed_cap = buf->position + len;

    if (needed_cap < buf->position) { /* overflow */
        return CIF_ERROR;
    } else if (needed_cap > buf->capacity) {
        /* expand capacity by a factor 1.5, as many times as necessary */
        size_t working_cap = buf->capacity;
        size_t proposed_cap;
        char *new_start;

        do {
            proposed_cap = (working_cap * 3) >> 1;

            if (proposed_cap < working_cap) { /* overflow */
                /* fall back to requesting only what is imminently needed */
                proposed_cap = needed_cap;
            }
        } while (proposed_cap < needed_cap);

        /* reallocate the buffer space */
        new_start = (char *) realloc(buf->start, proposed_cap);
        if ((new_start == NULL) && (needed_cap < proposed_cap)) {
            new_start = (char *) realloc(buf->start, needed_cap);
        }

        if (new_start == NULL) {
            return CIF_ERROR;
        } else {
            buf->start = new_start;
        }
    }

    memcpy(buf->start + buf->position, src, len);
    buf->position += len;
    if (buf->position > buf->limit) buf->limit = buf->position;

    return CIF_OK;
}

static size_t cif_buf_read(read_buffer_t *buf, void *dest, size_t max) {
    if ((buf->position >= buf->limit) || (max == 0)) {
        return 0;
    } else {
        size_t available = buf->limit - buf->position;

        if (max > available) {
            max = available;
        }

        memcpy(dest, buf->start + buf->position, max);
        buf->position += max;

        return max;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

int cif_value_create(cif_kind_t kind, cif_value_t **value) {
    FAILURE_HANDLING;
    cif_value_t *temp = (cif_value_t *) malloc(sizeof(cif_value_t));

    if (temp != NULL) {
        switch (kind) {
            case CIF_CHAR_KIND:
                temp->as_char.text = (UChar *) malloc(sizeof(UChar));
                if (temp->as_char.text == NULL) DEFAULT_FAIL(soft);
                *(temp->as_char.text) = 0;
                temp->kind = CIF_CHAR_KIND;
                break;
            case CIF_NUMB_KIND:
                temp->kind = CIF_UNK_KIND; /* lest the init function try to clean up */
                if (cif_value_init_numb(temp, 0.0, 0.0, 0, 1) != CIF_OK) DEFAULT_FAIL(soft);
                break;
            case CIF_LIST_KIND:
                temp->kind = CIF_UNK_KIND; /* lest the init function try to clean up */
                cif_list_init(&(temp->as_list));   /* no additional resource allocation here */
                break;
            case CIF_TABLE_KIND:
                temp->kind = CIF_UNK_KIND; /* lest the init function try to clean up */
                cif_table_init(&(temp->as_table)); /* no additional resource allocation here */
                break;
            case CIF_NA_KIND:
                /* fall through */
            case CIF_UNK_KIND:
                temp->kind = kind;
                break;
            default:
                FAIL(soft, CIF_ARGUMENT_ERROR);
        }

        *value = temp;
        return CIF_OK;

        FAILURE_HANDLER(soft):
        free(temp);
    }

    FAILURE_TERMINUS;
}

void cif_value_clean(union cif_value_u *value) {
    switch (value->kind) {
        case CIF_CHAR_KIND:
            cif_char_value_clean((struct char_value_s *) value);
            break;
        case CIF_NUMB_KIND:
            cif_numb_value_clean((struct numb_value_s *) value);
            break;
        case CIF_LIST_KIND:
            cif_list_value_clean((struct list_value_s *) value);
            break;
        case CIF_TABLE_KIND:
            cif_table_value_clean((struct table_value_s *) value);
            break;
        default:
            /* nothing to do for CIF_UNK_KIND or CIF_NA_KIND */
            break;
    }

    /* Mark the value as kind CIF_UNK_KIND in case it is cleaned again or freed */
    value->kind = CIF_UNK_KIND;
}

void cif_value_free(union cif_value_u *value) {
    if (value != NULL) {
        cif_value_clean(value);
        free(value);
    }
}

int cif_value_clone(cif_value_t *value, cif_value_t **clone) {
    FAILURE_HANDLING;
    cif_value_t *temp;
    cif_value_t *to_free = NULL;

    if (*clone != NULL) {
        cif_value_clean(*clone);
        temp = *clone;
    } else {
        if (cif_value_create(CIF_UNK_KIND, &temp) != CIF_OK) DEFAULT_FAIL(soft);
        to_free = temp;
    }

    switch (value->kind) {
        case CIF_CHAR_KIND:
            temp->as_char.text = cif_u_strdup(value->as_char.text);
            if (!temp->as_char.text) DEFAULT_FAIL(soft);
            temp->kind = CIF_CHAR_KIND;
            break;
        case CIF_NUMB_KIND:
            if (cif_value_clone_numb(&(value->as_numb), &(temp->as_numb)) != CIF_OK) DEFAULT_FAIL(soft);
            break;
        case CIF_LIST_KIND:
            if (cif_value_clone_list(&(value->as_list), &(temp->as_list)) != CIF_OK) DEFAULT_FAIL(soft);
            break;
        case CIF_TABLE_KIND:
            if (cif_value_clone_table(&(value->as_table), &(temp->as_table)) != CIF_OK) DEFAULT_FAIL(soft);
            break;
        case CIF_UNK_KIND:
        case CIF_NA_KIND:
            temp->kind = value->kind;
            break;
        default:
            FAIL(soft, CIF_ARGUMENT_ERROR);
    }

    *clone = temp;
    return CIF_OK;

    FAILURE_HANDLER(soft):
    free(to_free);

    FAILURE_TERMINUS;
}

/*
 * Note: in addition to its publicly-documented use, this function may be used to initialize value objects allocated on
 * the stack, provided that their kind is pre-initialized to CIF_UNK_KIND.
 */
int cif_value_init(cif_value_t *value, cif_kind_t kind) {
    if (kind == CIF_NUMB_KIND) {
        return cif_value_init_numb(value, 0.0, 0.0, 0, 1);
    } else {
        int result = CIF_OK;

        cif_value_clean(value);
        switch (kind) {
            case CIF_CHAR_KIND:
                value->as_char.text = (UChar *) malloc(sizeof(UChar));
                if (value->as_char.text == NULL) {
                    result = CIF_ERROR;
                } else {
                    *(value->as_char.text) = 0;
                    value->kind = CIF_CHAR_KIND;
                }
                break;
            case CIF_LIST_KIND:
                cif_list_init(&(value->as_list));    /* no resource allocation here */
                break;
            case CIF_TABLE_KIND:
                cif_table_init(&(value->as_table));  /* no resource allocation here */
                break;
            case CIF_NA_KIND:
                value->kind = CIF_NA_KIND;
                break;
            case CIF_UNK_KIND:
                /* do nothing */
                break;
            default:
                result = CIF_ARGUMENT_ERROR;
                break;
        }

        return result;
    }
}

buffer_t *cif_value_serialize(cif_value_t *value) {
    buffer_t *buf = cif_buf_create(DEFAULT_SERIALIZATION_CAP);

    if (buf != NULL) {
        write_buffer_t *wbuf = &(buf->for_writing);
        SERIALIZE(value, wbuf, fail);
        return buf;

        fail:
        cif_buf_free(wbuf);
        buf = NULL;
    }

    return buf;
}

cif_value_t *cif_value_deserialize(const void *src, size_t len, cif_value_t *dest) {
    read_buffer_t buf;

    buf.start = (const char *) src;
    buf.capacity = len;
    buf.limit = len;
    buf.position = 0;
    DESERIALIZE(cif_value_t, dest, &buf, fail);

    return dest;

    fail:
    return NULL;
}

int cif_value_parse_numb(cif_value_t *n, UChar *text) {
    FAILURE_HANDLING;
    struct numb_value_s *numb = &(n->as_numb);
    size_t pos = 0;

    /* details of the main digit string */
    size_t digit_start;
    size_t digit_end;
    int num_decimal = 0;
    size_t decimal_pos = 0;

    struct numb_value_s n_temp;

    n_temp.sign = 1;

    /* optional leading sign */
    switch (text[pos]) {
        case UCHAR_MINUS:
            n_temp.sign = -1;
            /*@fallthrough@*/
        case UCHAR_PLUS:
            pos += 1;
        /* else do nothing */
    }

    /* mandatory digit string with optional single decimal point */
    digit_start = pos;
    while (((text[pos] >= UCHAR_0) && (text[pos] <= UCHAR_9)) || ((text[pos] == UCHAR_DECIMAL) && (num_decimal == 0))) {
        if (text[pos] == UCHAR_DECIMAL) {
            num_decimal = 1;
            decimal_pos = pos;
        }
        pos += 1;
    }
    if (pos <= (digit_start + num_decimal)) {
        /* no digits */
        FAIL(early, CIF_INVALID_NUMBER);
    }

    /* Consume a trailing decimal point and any leading insignificant zeroes */
    if (text[pos - 1] == UCHAR_DECIMAL) {
        num_decimal = 0;
        digit_end = pos - 1;
    } else {
        digit_end = pos;
    }
    while (((text[digit_start] == UCHAR_0) || (text[digit_start] == UCHAR_DECIMAL))
            && (digit_start < (digit_end - 1))) {
        digit_start += 1;
    }

    /* optional exponent */
    if ((text[pos] == UCHAR_E) || (text[pos] == UCHAR_e)) {
        int exponent = 0;
        int exp_sign = 1;
        size_t exp_start;

        /* optional sign */
        switch (text[++pos]) {
            case UCHAR_MINUS:
                exp_sign = -1;
                /*@fallthrough@*/
            case UCHAR_PLUS:
                pos += 1;
            /* else do nothing */
        }

        exp_start = pos;
        while ((text[pos] >= UCHAR_0) && (text[pos] <= UCHAR_9)) {
            exponent = (int) ((exponent * 10) + (text[pos] - UCHAR_0));
            pos += 1;
        }
        if (pos <= exp_start) {
            /* no exponent digits */
            FAIL(early, CIF_INVALID_NUMBER);
        }

        /* update the initial scale based on the exponent */
        n_temp.scale = exponent * (-exp_sign);
    } else {
        n_temp.scale = 0;
    }

    if (num_decimal == 1) {
        n_temp.scale += (digit_end - (decimal_pos + 1));
    }

    /* optional uncertainty */
    if (text[pos] == UCHAR_OPEN) {
        size_t su_start = ++pos;
        char *suc;

        while ((text[pos] >= UCHAR_0) && (text[pos] <= UCHAR_9)) {
            pos += 1;
        }
        if ((pos <= su_start) || (text[pos] != UCHAR_CLOSE)) {
            /* no su digits or missing the closing parenthesis */
            FAIL(early, CIF_INVALID_NUMBER);
        }
        /* consume leading, insignificant zeroes */
        while ((text[su_start] == UCHAR_0) && (su_start < (pos - 1))) {
            su_start += 1;
        }
        n_temp.su_digits = (char *) malloc(1 + pos - su_start);
        if (n_temp.su_digits == NULL) {
            DEFAULT_FAIL(early);
        }
        suc = n_temp.su_digits;
        while (su_start < pos) {
            /* digits are expressed in the C locale */
            *(suc++) = (text[su_start++]);
        }
        *suc = '\0';

        pos += 1;
    } else {
        n_temp.su_digits = NULL;
    }

    if (text[pos] != 0) {
        /* the string has an unparsed tail */
        FAIL(late, CIF_INVALID_NUMBER);
    }

    /* write the digit string to the value object */
    n_temp.digits = (char *) malloc((digit_end + 2) - (digit_start + num_decimal));
    if (n_temp.digits != NULL) {
        char *c = n_temp.digits;

        /* to simplify logic in the following loop: */
        if (num_decimal == 0) decimal_pos = pos;

        /* copy the digits, skipping over the decimal point, if any */
        for (pos = digit_start; pos < digit_end; pos += 1) {
            if (pos != decimal_pos) {
                /* digits are expressed in the C locale */
                 *c = (char) text[pos];
                 c += 1;
            }
        }
        *c = '\0';

        /* Successful parse; copy results to the target value object */
        cif_value_clean(n);
        numb->kind = CIF_NUMB_KIND;
        numb->text = text;
        numb->sign = n_temp.sign;
        numb->digits = n_temp.digits;
        numb->su_digits = n_temp.su_digits;
        numb->scale = n_temp.scale;

        return CIF_OK;
    }

    FAILURE_HANDLER(late):
    free(n_temp.su_digits); /* safe when n_temp.su_digits is NULL */

    FAILURE_HANDLER(early):
    FAILURE_TERMINUS;
}

int cif_value_init_char(cif_value_t *value, UChar *text) {
    if (text == NULL) {
        return CIF_ARGUMENT_ERROR;
    } else {
        cif_value_clean(value);
        value->as_char.text = text;
        value->kind = CIF_CHAR_KIND;
        return CIF_OK;
    }
}

int cif_value_copy_char(cif_value_t *value, const UChar *text) {
    if (text == NULL) {
        return CIF_ARGUMENT_ERROR;
    } else {
        UChar *copy = cif_u_strdup(text);

        if (copy == NULL) {
            return CIF_ERROR;
        } else {
            int result = cif_value_init_char(value, copy);

            if (result != CIF_OK) {
                free(copy);
            }

            return result;
        }
    }
}

int cif_value_init_numb(cif_value_t *n, double val, double su, int scale, int max_leading_zeroes) {
    if ((su < 0.0) || (-scale < LEAST_DBL_10_DIGIT) || (-scale > DBL_MAX_10_EXP) || (max_leading_zeroes < 0)) {
        return CIF_ARGUMENT_ERROR;
    } else {
        FAILURE_HANDLING;
        struct numb_value_s *numb = &(n->as_numb);
        int most_significant_place = MSP(val);
        char *locale = setlocale(LC_NUMERIC, "C");

        if (locale != NULL) {
            char *digit_buf = to_digits(val, scale);

            cif_value_clean(n);
            if (digit_buf != NULL) {
                char *su_buf;

                /* the size of the su digit string, excluding the terminator; 0 for no uncertainty */
                ssize_t su_size;

                if (su > 0) {
                    su_buf = to_digits(su, scale);
                    su_size = ((su_buf == NULL) ? -1 : (ssize_t) strlen(su_buf));
                } else {
                    su_buf = NULL;
                    su_size = 0;
                }

                if ((su_size == 0) || (su_buf != NULL)) {
                    UChar *text;

                    /* casting 'su_size' (type ssize_t) to type 'size_t' is safe because we know it is > 0 */
                    if ((scale >= 0) && (-(most_significant_place + 1) <= max_leading_zeroes)) {
                        /* use decimal notation */
                        text = format_text_decimal(val, digit_buf, su_buf, (size_t) su_size, scale);
                    } else {
                        /* use scientific notation */
                        text = format_text_sci(val, digit_buf, su_buf, (size_t) su_size, scale);
                    }

                    if (text != NULL) {
                        /* assign the formatted results to the value object */
                        numb->kind = CIF_NUMB_KIND;
                        numb->sign = (val < 0) ? -1 : 1;
                        numb->text = text;
                        numb->digits = digit_buf;
                        numb->su_digits = su_buf;
                        numb->scale = scale;

                        /* restore the original locale */
                        setlocale(LC_NUMERIC, locale);

                        return CIF_OK;
                    }

                    if (su_buf != NULL) free(su_buf);
                }
                free(digit_buf);
            }

            /* restore the original locale */
            setlocale(LC_NUMERIC, locale);
        }

        FAILURE_TERMINUS;
    }
}

static int frac_bits(double d, int *exponent) {
    if (d == 0.0) {
        *exponent = 0;
        return 0;
    } else {
        double t = frexp(d, exponent);
        /* FIXME: assumes DBL_MANT_DIG <= 64 */
        uint64_t bits = (uint64_t) ldexp(fabs(t), DBL_MANT_DIG);
        int zeroes = 0;

        while ((bits % 16) == 0) {
            zeroes += 4;
            bits /= 16;
        }
        while ((bits % 2) == 0) {
            zeroes += 1;
            bits /= 2;
        }

        return (DBL_MANT_DIG - zeroes);
    }
}

/*
 * BUF_SIZE must be sufficient to accommodate the number of decimal digits in UINT_MAX, plus the number of decimal
 * digits in max(2, floor(log10(UINT_MAX))), plus a decimal point, exponent sigil, exponent sign, and terminator.  Where
 * 'unsigned int' is a 32-bit, twos-complement binary integer, that would be 16 chars.  The 50-char buffer size alotted
 * here is sufficient for a 128-bit integer, so more than enough for any currently-known implementation.
 */
#define BUF_SIZE 50
int cif_value_autoinit_numb(cif_value_t *numb, double val, double su, unsigned int su_rule) {
    if ((su >= 0.0) && (su_rule >= 2)) {
        /* Arguments appear valid */

        cif_value_clean(numb);

        if (su == 0.0) { /* an exact number */
            int exponent;
            int bit_count = frac_bits(val, &exponent);
            int scale;

            if (bit_count >= exponent) {
                scale = (bit_count - exponent);
            } else {
                int most_significant_place = MSP(val);

                scale = ((most_significant_place < DBL_DIG) ? 0 : ((DBL_DIG - 1) - most_significant_place));
            }

            return cif_value_init_numb(numb, val, su, scale, DEFAULT_MAX_LEAD_ZEROES);
        } else {
            int result_code = CIF_INTERNAL_ERROR;

            /* number formatting and parsing must be done in the C locale to ensure portability */
            char *locale = setlocale(LC_NUMERIC, "C");

            if (locale != NULL) {
                char buf[BUF_SIZE];
                int rule_digits;

                /* The maximum number of decimal digits in the scientific-notation exponent of a formatted double */
                int exponent_digits
#ifdef CIF_MAX_10_EXP_DIG
                        = CIF_MAX_10_EXP_DIG;
#else
                        ;
                {
                    int exp_max = ((DBL_MAX_10_EXP >= -(DBL_MIN_10_EXP)) ? DBL_MAX_10_EXP : -(DBL_MIN_10_EXP));

                    /* Assumes formatting with minimum two-(decimal-)digit exponents */
                    for (exponent_digits = 2; exp_max > 99; exp_max /= 10) exponent_digits += 1;
                }
#endif

                /* determine the number of significant digits in the su_rule (which is known to be positive here) */
                rule_digits = (int) log10(su_rule + 0.5) + 1;

                /*
                 * Assuming that the buffer is large enough (which it very much should be), format the su using the same
                 * number of total digits as the su_rule has sig-figs to determine the needed scale.  This approach is a
                 * bit inefficient, but it's straightforward, reliable, and portable.
                 */
                if ((rule_digits + exponent_digits + 4 < BUF_SIZE)
                        && (sprintf(buf, "%.*e", rule_digits - 1, su) < BUF_SIZE)) {
                    char *v;
                    char *exponent;
                    size_t su_digits;
                    int scale;

                    if (buf[1] == '.') {
                        v = buf + 1;
                        *v = buf[0];
                    } else {
                        v = buf;
                    }

                    su_digits = (size_t) strtol(v, &exponent, 10);

                    /* The exponent and number of digits yield the scale */
                    assert((exponent != NULL) && (*exponent == 'e'));
                    scale = -atoi(exponent + 1) + rule_digits - 1;

                    /* Reduce the scale by 1 if the su needs to be rounded to fewer digits */
                    if (su_digits > (size_t) su_rule) scale -= 1;

                    result_code = cif_value_init_numb(numb, val, su, scale, DEFAULT_MAX_LEAD_ZEROES);
                } /* else the formatted su overflowed, despite our checks.  The su_rule must be very large. */

                (void) setlocale(LC_NUMERIC, locale);
            }

            return result_code;
        }
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}
cif_kind_t cif_value_kind(cif_value_t *value) {
    return value->kind;
}

int cif_value_get_number(cif_value_t *n, double *val) {
    if (n->kind != CIF_NUMB_KIND) {
        return CIF_ARGUMENT_ERROR;
    } else {
        struct numb_value_s *numb = &(n->as_numb);
        double d = to_double(numb->digits, numb->scale);

        *val = ((numb->sign < 0) ? -d : d);
        return CIF_OK;
    }
}

int cif_value_get_su(cif_value_t *n, double *su) {
    if (n->kind != CIF_NUMB_KIND) {
        return CIF_ARGUMENT_ERROR;
    } else {
        struct numb_value_s *numb = &(n->as_numb);

        *su = (numb->su_digits == NULL)
                ? 0.0
                : to_double(numb->su_digits, numb->scale);

        return CIF_OK;
    }
}

int cif_value_get_text(cif_value_t *value, UChar **text) {
    UChar *text_copy;

    switch (value->kind) {
        case CIF_CHAR_KIND:
            /* fall through */
        case CIF_NUMB_KIND:
            text_copy = cif_u_strdup(value->as_char.text);
            if (text_copy) {
                *text = text_copy;
            } else {
                return CIF_ERROR;
            } 
            break;
        default:
            *text = NULL;
            break;
    }

    return CIF_OK;
}

int cif_value_get_element_count(
        cif_value_t *value,
        size_t *count) {
    switch (value->kind) {
        case CIF_LIST_KIND:
            *count = value->as_list.size;
            return CIF_OK;
        case CIF_TABLE_KIND:
            *count = (size_t) HASH_COUNT(value->as_table.map.head);
            return CIF_OK;
        default:
            return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_get_element_at(
        cif_value_t *value,
        size_t index,
        cif_value_t **element) {
    if (value->kind != CIF_LIST_KIND) {
        return CIF_ARGUMENT_ERROR;
    } else if (index >= value->as_list.size) {
        return CIF_INVALID_INDEX;
    } else {
        *element = value->as_list.elements[index];
        return CIF_OK;
    }
}

int cif_value_set_element_at(
        cif_value_t *value,
        size_t index,
        cif_value_t *element) {
    if (value->kind != CIF_LIST_KIND) {
        return CIF_ARGUMENT_ERROR;
    } else if (index >= value->as_list.size) {
        return CIF_INVALID_INDEX;
    } else {
        cif_value_t *target = value->as_list.elements[index];

        if (target == element) {
            return CIF_OK;
        } else if (element == NULL) {
            cif_value_clean(target);
            return CIF_OK;
        } else {
            /* TODO: check safety in case of failure: */
            return cif_value_clone(element, &target);
        }
    }
}

int cif_value_insert_element_at(
        cif_value_t *value,
        size_t index,
        cif_value_t *element) {
    if (value->kind != CIF_LIST_KIND) {
        return CIF_ARGUMENT_ERROR;
    } else if (index > value->as_list.size) {
        return CIF_INVALID_INDEX;
    } else {
        FAILURE_HANDLING;
        cif_value_t *clone = NULL;
        int result = ((element == NULL) ? cif_value_create(CIF_UNK_KIND, &clone) : cif_value_clone(element, &clone));

        if (result == CIF_OK) {
            size_t index2;

            if (value->as_list.size >= value->as_list.capacity) {
                /* expand list capacity */
                size_t new_cap;
                cif_value_t **new_elements;

                assert (value->as_list.size == value->as_list.capacity);

                new_cap = value->as_list.capacity
                        + ((value->as_list.capacity < 10) ?  4 : (value->as_list.capacity / 2));
                new_elements = (cif_value_t **) realloc(value->as_list.elements, sizeof(cif_value_t *) * new_cap);
                if (new_elements != NULL) {
                    value->as_list.elements = new_elements;
                    value->as_list.capacity = new_cap;
                } else {
                    DEFAULT_FAIL(soft);
                }
            }

            /* move trailing elements back to make room */
            for (index2 = value->as_list.size; index2 > index; index2 -= 1) {
                value->as_list.elements[index2] = value->as_list.elements[index2 - 1];
            }

            /* set the inserted value (clone) in place and finish up */
            value->as_list.elements[index] = clone;
            value->as_list.size += 1;
            return CIF_OK;
            
            FAILURE_HANDLER(soft):
            cif_value_free(clone);
        } else {
            SET_RESULT(result);
        }

        FAILURE_TERMINUS;
    }
}

int cif_value_remove_element_at(
        cif_value_t *value,
        size_t index,
        cif_value_t **element) {
    if (value->kind != CIF_LIST_KIND) {
        return CIF_ARGUMENT_ERROR;
    } else if (index >= value->as_list.size) {
        return CIF_INVALID_INDEX;
    } else {
        /* save or free the removed element */
        if (element != NULL) {
            *element = value->as_list.elements[index];
        } else {
            cif_value_free(value->as_list.elements[index]);
        }

        /* move trailing elements forward to fill in */
        while (++index < value->as_list.size) {
            value->as_list.elements[index - 1] = value->as_list.elements[index];
        }

        /* update list size */
        value->as_list.size -= 1;

        return CIF_OK;
    }
}


#ifdef __cplusplus
}
#endif

