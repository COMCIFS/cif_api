/*
 * value.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <locale.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>
#include <unicode/parseerr.h>
#include "cif.h"
#include "utlist.h"
#include "uthash.h"
#include "internal/utils.h"
#include "internal/value.h"

#define DEFAULT_SERIALIZATION_CAP 512
#define SERIAL_TABLE_TERMINATOR -1
#define SERIAL_ENTRY_SEPARATOR 0

/**
 * @brief The base-10 logarithm of the smallest positive representable double (a de-normalized number),
 * rounded toward zero to an integer.
 */
#define LEAST_DBL_10_DIGIT (1 + DBL_MIN_10_EXP - DBL_DIG)

#define DEFAULT_MAX_LEAD_ZEROES 5

/**
 * @brief Serializes a NUL-terminated Unicode string to the provided buffer.
 *
 * The serialized representation is the bytes of a @c size_t @em character count followed by the raw bytes of the
 * string, excluding those of the terminating NUL character, all in machine order.
 *
 * @param[in] string a pointer to the NUL-terminated Unicode string to serialize; must not be NULL.
 * @param[in,out] buffer a pointer to the @c write_buffer_t buffer to which the serialized form should be appended;
 *         must not be NULL.
 * @param[in] onerr the label to which execution should branch in the event that an error occurs; must be defined in the
 *         scope where this macro is used.
 */
#define SERIALIZE_USTRING(string, buffer, onerr) do { \
    const UChar *str = (string); \
    write_buffer_t *us_buf = (buffer); \
    size_t us_size = (size_t) u_strlen(str); \
    if (cif_buf_write(us_buf, &us_size, sizeof(size_t)) != CIF_OK) goto onerr; \
    if (cif_buf_write(us_buf, str, us_size * sizeof(UChar)) != CIF_OK) goto onerr; \
} while (0)

/**
 * @brief Deserializes a Unicode string from the given buffer, assuming the format produced by @c SERIALIZE_USTRING().
 *
 * A new Unicode string is allocated for the result, and on success, a pointer to it is assigned to @c dest (which
 * must therefore be an lvalue).  The @c dest pointer is unmodified on failure.
 *
 * @param[out] dest a Unicode string pointer to be pointed at the deserialized string; its initial value is ignored.
 * @param[in,out] a @c read_buffer_t from which the serialized string data should be read; on success its position is
 *         immediately after the last byte of the serialized string, but on failure its position becomes undefined;
 *         must not be NULL.
 * @param[in] onerr the label to which execution should branch in the event that an error occurs; must be defined in the
 *         scope where this macro is used.
 */
#define DESERIALIZE_USTRING(dest, buffer, onerr) do { \
    read_buffer_t *us_buf = (buffer); \
    size_t _size; \
    if (cif_buf_read(us_buf, &_size, sizeof(size_t)) == sizeof(size_t)) { \
        UChar *d = (UChar *) malloc((_size + 1) * sizeof(UChar)); \
        if (d != NULL) { \
            if (cif_buf_read(us_buf, d, _size) == _size) { \
                d[_size] = 0; \
                dest = d; \
                break; \
            } else { \
                free(d); \
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
            /*@fallthrough@*/ \
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
                    /*@fallthrough@*/ \
                case CIF_NUMB_KIND: \
                    DESERIALIZE_USTRING(v->as_char.text, _buf, vfail); \
                    v->kind = CIF_CHAR_KIND; \
                    if ((_kind == CIF_NUMB_KIND) \
                            && (cif_value_parse_numb(v, v->as_char.text) != CIF_OK)) goto vfail; \
                    break; \
                case CIF_LIST_KIND: \
                    if (cif_list_deserialize(&(v->as_list), _buf) != CIF_OK) goto vfail; \
                    break; \
                case CIF_TABLE_KIND: \
                    if (cif_table_deserialize(&(v->as_table), _buf) != CIF_OK) goto vfail; \
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
 * @brief Formats the digits of a numeric standard uncertainty value according to the specified scale, providing both
 * the formatted representation and its size (including the null terminator).
 *
 * For positive uncertainties, the formatting performed is to round the absolute uncertainty value to @p scale
 * digits after the decimal point and then remove any decimal point, insignificant zeroes, or exponent indicator
 * to yield a pure decimal digit string.  For other uncertainties, the "formatted" digit string is NULL and its
 * length is zero. On success, a pointer to the digit string is written in @p su_buf and its size (including the
 * terminating NUL) is written in @p su_size.  Those parameters must therefore by lvalues.
 *
 * The input su_val is assumed contain an unsigned, decimal, floating-point number, formatted in a manner consistent
 * with the C locale.
 * 
 * If the su is a negative number then control branches to the specified failure label, @p fail_tag .
 *
 * @param[in] su_val the uncertainty value to format, a number (generally a double).
 * @param[out] su_buf a @c char @c * to receive a pointer to the formatted su
 * @param[out] su_size the name of a variable in which to record the length of the formatted digit string; must be
 *         assignment-compatible with @c ptrdiff_t
 * @param[in] scale the number of significant digits to the right of the decimal point in the uncertainty; must not
 *         be negative
 * @param[in] fail_tag the label to which execution should branch in the event of an error
 */
/* TODO: check the constraint that the scale must not be negative */
#define FORMAT_SU(su_val, su_buf, su_size, scale, fail_tag) do { \
    double f_su = (su_val); \
    if (f_su > 0.0) { \
        su_buf = format_as_decimal(su, (scale)); \
        if (su_buf == NULL) { \
            DEFAULT_FAIL(fail_tag); \
        } else { \
            char *f_c = su_buf; \
            char *f_d = su_buf; \
            for (; ((*f_c == '0') || (*f_c == '.')); f_c += 1) /* nothing */ ; \
            for (;;) { \
                *(f_d++) = *(f_c); \
                if (*(f_c++) == '\0') break; \
                while (*f_c == '.') f_c += 1; \
            } \
            su_size = (f_d - su_buf); \
            if (su_size == 1) { \
                free(su_buf); \
                su_buf = NULL; \
                su_size = 0; \
            } \
        } \
    } else { \
        su_buf = NULL; \
        su_size = 0; \
    } \
} while (0)

/**
 * @brief Writes mantissa and uncertainty digit strings into a pre-allocated Unicode character buffer.
 *
 * The provided buffer (@p text ) must be pre-allocated and of sufficient length; no bounds checking is performed.
 * A leading '-' sign is prepended if the given numeric value @p val is negative.  The uncertainty is omitted if
 * NULL; otherwise, it is parenthesized and appended to the mantissa without any whitespace.
 *
 * @param[in,out] text a pointer to the Unicode character buffer into which the digit strings are to be formatted.
 * @param[in] a number whose sign is transferred to the result (implicitly if the value is non-negative); in principle,
 *         this is the numeric value represented by the (scaled) mantissa, but any number having the correct sign will
 *         do.
 * @param[in] mantissa a NUL-terminated C string of the decimal digits of the mantissa of the number to format, in a
 *         form consistent with the C locale; must not be NULL.
 * @param[in] uncertainty a NUL-terminated C string of the decimal digits of the uncertainty associated with the given
 *         mantissa, expressed to the same scale and consistent with the C locale, or NULL if the mantissa is exact.
 */
#define WRITE_NUMBER_TEXT(text, val, mantissa, uncertainty) do { \
    UChar *u = (text); \
    char *wnt_su = (uncertainty); \
    char *_c; \
    if ((val) < 0.0) *(u++) = UCHAR_MINUS; \
    for (_c = (mantissa); *_c != '\0'; ) *(u++) = (UChar) *(_c++); \
    if (wnt_su != NULL) { \
        *(u++) = UCHAR_OPEN; \
        for (_c = wnt_su; *_c != '\0'; ) *(u++) = (UChar) *(_c++); \
        *(u++) = UCHAR_CLOSE; \
    } \
    *u = 0; \
} while (0)

/* headers for internal functions */

/*
 * Creates a new dynamic buffer with the specified initial capacity.
 * Returns a pointer to the buffer structure, or NULL if buffer creation fails.
 */
static /*@only@*/ /*@null@*/ buffer_t *cif_buf_create(size_t cap);

/*
 * Releases a writeable dynamic buffer and all resources associated with it.  A no-op if the argument is NULL.
 */
static void cif_buf_free(/*@only@*/ write_buffer_t *buf);

/*
 * Writes bytes to a dynamic buffer, starting at its current position.
 * The buffer capacity is expanded as needed to accommodate the specified number of bytes, and possibly more.
 * The third argument may be zero, in which case the function is a no-op.
 * Returns CIF_OK on success or an error code on failure.  The buffer is unmodified on failure.
 * Behavior is undefined if buf does not point to a valid buffer_t object, or if start does not point to an
 * allocated block of memory at least len bytes long, or if the source is inside the buffer.
 */
static int cif_buf_write(/*@temp@*/ write_buffer_t *buf, /*@temp@*/ const void *src, size_t len);

/*
 * Reads up to the specified number of bytes from the specified dynamic buffer into the specified destination,
 * starting at the current position and not proceeding past the current limit.
 * Returns the number of bytes transferred to the destination (which will be zero when no more are available and
 * when the third argument is zero).  
 */
static size_t cif_buf_read(/*@temp@*/ read_buffer_t *buf, /*@out@*/ void *dest, size_t max);

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
static void cif_list_init(/*@out@*/ struct list_value_s *list_value);

/*
 * Fully initializes a table value as an table list.  Does not allocate any additional resources.  Should not be
 * invoked on an already-initialized value, as doing so may create a resource leak
 */
static void cif_table_init(/*@out@*/ struct table_value_s *table_value);

static void cif_char_value_clean(/*@temp@*/ struct char_value_s *char_value);
static void cif_numb_value_clean(/*@temp@*/ struct numb_value_s *numb_value);
static void cif_list_value_clean(/*@temp@*/ struct list_value_s *list_value);
static void cif_table_value_clean(/*@temp@*/ struct table_value_s *table_value);

/*
 * Clones a list value, assuming all fields of the target object contain garbage
 */
static int cif_value_clone_list(/*@in@*/ /*@temp@*/ struct list_value_s *value, /*@out@*/ struct list_value_s *clone);

static int cif_list_serialize(struct list_value_s *list, write_buffer_t *buf);
static int cif_table_serialize(struct table_value_s *table, write_buffer_t *buf);
static int cif_list_deserialize(/*@out@*/ struct list_value_s *list, read_buffer_t *buf);
static int cif_table_deserialize(/*@out@*/ struct table_value_s *table, read_buffer_t *buf);

/**
 * @brief Formats a decimal representation of the specified number, with the specified number of digits following the
 *         decimal point.
 *
 * Uses the current numeric locale.
 *
 * @param[in] v the number to format
 * @param[in] scale the number of digits to format following the decimal point; must not be negative
 * @return a newly-allocated C string containing the formatted number
 */
static /*@only@*/ /*@null@*/ char *format_as_decimal(double v, int scale) /*@*/;

/**
 * @brief Formats a value and su into the specified number value object, in plain decimal format, assuming that the
 *         target's current contents are all garbage.
 *
 * The caller is expected to manage the numeric locale, which should be the C locale or a functional equivalent.
 *
 * @param[in] val The numeric value to format
 * @param[in] su The uncertainty associated with @p val
 * @param[in] scale the number of digits to output after the decimal point; must not be negative
 * @param[out] a number value structure to initialize with the formatted number
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
static int format_value_decimal(double val, double su, int scale, /*@out@*/ struct numb_value_s *numb);

/**
 * @brief Formats a value and su into the specified number value object, in scientific notation format, assuming that
 *         the target's current contents are all garbage.
 *
 * The caller is expected to manage the numeric locale, which should be the C locale or a functional equivalent.
 *
 * @param[in] val The numeric value to format
 * @param[in] su The uncertainty associated with @p val
 * @param[in] scale the position of the least significant digit of @p val, expressed as number of decimal digits after
 *         the 10^0 position; may be negative
 * @param[out] a number value structure to initialize with the formatted number
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
static int format_value_sci(double val, double su, int scale, /*@out@*/ struct numb_value_s *numb);
static double digits_as_double(const char *digits, int scale);

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
        (void) cif_value_free(list_value->elements[list_value->size]);
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
        CLEAN_PTR(entry->key);
        (void) cif_value_free((cif_value_t *) entry);
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
                if (clone->su_digits != NULL) DEFAULT_FAIL(su);
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
    (void) cif_list_value_clean(clone);

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
        struct entry_s *new_entry = (struct entry_s *) malloc(sizeof(struct entry_s *));

        if (new_entry != NULL) {
           new_entry->key = cif_u_strdup(entry->key);
           if (new_entry->key != NULL) {
               cif_value_t *new_value = &(new_entry->as_value);

               new_value->kind = CIF_UNK_KIND;
               if (cif_value_clone(&(entry->as_value), &new_value) == CIF_OK) {
#undef uthash_fatal
#define uthash_fatal(msg) DEFAULT_FAIL(hash)
                   HASH_ADD_KEYPTR(hh, temp.map.head, new_entry->key, U_BYTES(new_entry->key), new_entry);
                   continue;

                   FAILURE_HANDLER(hash):
                   (void) cif_value_clean(new_value);
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

    fail:
    return CIF_ERROR;
}

/*
 * Serializes a table value, not including the initial value-type code.  May recurse, directly or indirectly.
 */
static int cif_table_serialize(struct table_value_s *table, write_buffer_t *buf) {
    FAILURE_HANDLING;
    int separator = SERIAL_ENTRY_SEPARATOR;
    int terminator = SERIAL_TABLE_TERMINATOR;
    struct entry_s *element;
    struct entry_s *temp;

    HASH_ITER(hh, table->map.head, element, temp) {
        /*
         * a separator isn't really needed, especially before the first entry, but using one simplifies
         * deserialization by reserving the same amount of space before each entry that is consumed by the
         * final terminator.
         */
        if(cif_buf_write(buf, &separator, sizeof(int)) != CIF_OK) DEFAULT_FAIL(element);
        SERIALIZE_USTRING(element->key, buf, HANDLER_LABEL(element));
        SERIALIZE(element, buf, HANDLER_LABEL(element));
    }
    return cif_buf_write(buf, &terminator, sizeof(int));

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
        cif_value_t **elements = (cif_value_t **) malloc(sizeof(cif_value_t *) * capacity);

        if (elements != NULL) {
            size_t size;

            for (size = 0; size < capacity; size += 1) {
                elements[size] = NULL;  /* TODO: check whether this initialization can be avoided */
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
    struct entry_s *entry;

    cif_table_init(&(temp.as_table));
    for (;;) {
        int flag;

        if (cif_buf_read(buf, &flag, sizeof(int)) != CIF_OK) {
            DEFAULT_FAIL(key);
        } else {
            switch (flag) {
                case SERIAL_TABLE_TERMINATOR:
                    cif_table_init(table);
                    table->map.head = temp.as_table.map.head;
                    return CIF_OK;
                case SERIAL_ENTRY_SEPARATOR:
                    key = NULL;
                    DESERIALIZE_USTRING(key, buf, HANDLER_LABEL(key));
                    entry = NULL;
                    DESERIALIZE(struct entry_s, entry, buf, HANDLER_LABEL(value));
                    entry->key = key;
#undef uthash_fatal
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
    free(key);

    FAILURE_HANDLER(key):
    cif_value_clean(&temp);

    FAILURE_TERMINUS;
}

static char *format_as_decimal(double v, int scale) {
    double abs_val = fabs(v);
    int most_significant_place = (int) ((v == 0.0) ? 0 : log10(abs_val));
    /* one or more digits before the decimal point + (maybe) a decimal point and additional digits: */
    int char_count = ((most_significant_place > 0) ? (most_significant_place + 1) : 1)
            + ((scale > 0) ? (scale + 1) : 0);
    char *digit_buf;

    assert(scale >= 0);
    /*
     * Allows for the terminator, for an extra leading digit possibly introduced by rounding up, and for an
     * (unexpected) trailing decimal point when the scale is exactly zero.
     */
    digit_buf = (char *) malloc((size_t) char_count + 3);

    if (digit_buf != NULL) {
        /*
         * C89 does not define snprintf(), so it is not used here.  The space needed was carefully computed, however,
         * so it is reasonably safe to use ordinary sprintf().  If there is nevertheless an overflow, that will be
         * caught by the subsequent assertion (supposing assertions are enabled, and variable char_count is not
         * clobbered).
         */
        /* FIXME: test that this works with large (but valid) scales: */
        int chars_written = sprintf(digit_buf, "%.*f", scale, abs_val);

        assert (chars_written <= char_count);
        chars_written -= 1;
        if (digit_buf[chars_written] == '.') {
            digit_buf[chars_written] = '\0';
        }
    }

    return digit_buf;
}

static int format_value_decimal(double val, double su, int scale, struct numb_value_s *numb) {
    FAILURE_HANDLING;
    char *digit_buf = format_as_decimal(val, scale);

    if (digit_buf != NULL) {
        UChar *text;
        size_t total_chars;

        /* the size of the su digit string, INCLUDING the NULL terminator */
        size_t su_size;
        char *su_buf;

        FORMAT_SU(su, su_buf, su_size, scale, su);

        total_chars = strlen(digit_buf) + ((val < 0.0) ? 1 : 0) + ((su_size > 0) ? (su_size + 2) : 0);
        if (total_chars <= CIF_LINE_LENGTH) {
            text = (UChar *) malloc(total_chars * sizeof(UChar));

            if (text != NULL) {
                char *c;

                WRITE_NUMBER_TEXT(text, val, digit_buf, su_buf);

                /* Convert the formatted number to a plain digit string */
                c = strchr(digit_buf, '.');
                if (c != NULL) {
                    do {
                        *c = *(c + 1);
                        c = c + 1;
                    } while (*c != '\0');
                }

                /* assign the formatted results to the value object */
                numb->text = text;
                numb->digits = digit_buf;
                numb->su_digits = su_buf;

                return CIF_OK;
            }
        } else {
            /* Formatted representation is too long */
            /* should not happen: no combination of arguments should produce a result longer than the limit */
            SET_RESULT(CIF_INTERNAL_ERROR);
        }

        if (su_buf != NULL) free(su_buf);

        FAILURE_HANDLER(su):
        free(digit_buf);
    }

    FAILURE_TERMINUS;
}

static int format_value_sci(double val, double su, int scale, struct numb_value_s *numb) {
    FAILURE_HANDLING;
    double abs_val = fabs(val);
    int most_significant_place = (int) ((val == 0.0) ? 0 : log10(abs_val));
    int exponent_digits = ((int) log10(abs(most_significant_place) + 1.0)) + 1;
    int mantissa_digits;
    int chars_needed;
    char *buf;

    if (exponent_digits < 2) exponent_digits = 2;
    if (-scale > most_significant_place) {
        most_significant_place = -scale;
        mantissa_digits = 1;
    } else {
        mantissa_digits = 1 + most_significant_place + scale;
    }

    /*
     * If the mantissa needs more than one digit (and maybe if not), then it also needs space for a decimal point.
     * Leave space also for the exponent indicator (e) and sign, an extra digit of mantissa possibly introduced by
     * rounding up, and a terminator
     */
    chars_needed = (mantissa_digits + exponent_digits + 5);

    buf = (char *) malloc((size_t) chars_needed);
    if (buf != NULL) {
        int chars_used = sprintf(buf, "%.*e", mantissa_digits - 1, abs_val);

        if (chars_used <= chars_needed) {  /* this case should always be exercised */
            /* check the exponent for possible rounding up */
            char *c = strchr(buf, 'e');

            if (c != NULL) {
                char *su_buf;
                size_t su_size;
                size_t total_chars;
                UChar *text;

                if (atoi(c + 1) != most_significant_place) {
                    /* the value was rounded up such that its scale changed. */
                    /* insert an extra zero of mantissa to restore the requested scale (space is already reserved) */
                    if (mantissa_digits == 1) {
                        memmove(c + 2, c, strlen(c));
                        *c = '.';
                        *(c + 1) = '0';
                    } else {
                        memmove(c + 1, c, strlen(c));
                        *c = '0';
                    }
                }

                FORMAT_SU(su, su_buf, su_size, scale, exponent);

                total_chars = strlen(buf) + ((val < 0.0) ? 1 : 0) + ((su_size > 0) ? (su_size + 2) : 0);
                if (total_chars <= CIF_LINE_LENGTH + 1) {
                    text = (UChar *) malloc(total_chars * sizeof(UChar));
                    if (text != NULL) {
                        WRITE_NUMBER_TEXT(text, val, buf, su_buf);

                        /* convert the formatted number to a digit string (of the mantissa) */
                        if (*(buf + 1) == '.') {
                            c = buf + 1;
                            do {
                                *c = *(c + 1);
                                c = c + 1;
                            } while (*c != 'e');
                        } else {
                            c = strchr(buf, 'e');
                            if (c == NULL) {
                                free(text);
                                DEFAULT_FAIL(text);
                            }
                        }
                        *c = '\0';

                        /* assign the formatted results to the value object */
                        numb->text = text;
                        numb->digits = buf;
                        numb->su_digits = su_buf;

                        return CIF_OK;
                    }
                } else {
                    /* Formatted representation is too long */
                    /* should not happen: no combination of arguments should produce a result longer than the limit */
                    SET_RESULT(CIF_INTERNAL_ERROR);
                }

                FAILURE_HANDLER(text):
                if (su_buf != NULL) free(su_buf);
            } /* else number formatting failure */
        } /* else formatted number is too long */

        FAILURE_HANDLER(exponent):
        free(buf);
    } /* else buffer allocation failure */

    FAILURE_TERMINUS;
}

#undef BUF_SIZE
/* (mantissa digits +- fudge) + sign + point + e + sign + (exponent + fudge) + term */
/* => DBL_DIG + 1 + 1 + 1 + 1 + DBL_DIG + 1 */
#define BUF_SIZE (2 * DBL_DIG + 5)
static double digits_as_double(const char *digits, int scale) {
    char buf[BUF_SIZE];
    char *c = buf;
    const char *d = digits;
    int digit_count;
    double result;
    int exponent;

    /* be sure to parse in the C locale */
    char *locale = setlocale(LC_NUMERIC, "C");

    /* copy the first digit to the buffer */
    *(c++) = *(d++);

    /* add any further digits after a decimal point */
    if (*d != '\0') {
        *(c++) = '.';
        digit_count = (int) strlen(d);
        if (digit_count >= DBL_DIG) digit_count = (DBL_DIG - 1);
        strncpy(c, d, (size_t) digit_count);
        c += digit_count;
    } else {
        digit_count = 0;
    }

    /*
     * calculate the appropriate exponent based on the scale and number of digits,
     * clamping the result to the limits of representable doubles.
     */
    exponent = digit_count - scale;
    if (exponent > DBL_MAX_EXP) {
        exponent = DBL_MAX_EXP;
    } else if (exponent <= (DBL_MIN_EXP - DBL_DIG)) {
        exponent = (DBL_MIN_EXP - DBL_DIG) + 1;
    }
  
    /* append the exponent designator */ 
    c += sprintf(c, "e%+.2d", exponent);

    /* swallow any trailing decimal point (though there shouldn't be one) and ensure the string is terminated */
    if (*(c - 1) == '.') c -= 1;
    *c = '\0';

    /* parse the formatted number */
    (void) sscanf(buf, "%lf", &result);

    /* restore the original locale */
    (void) setlocale(LC_NUMERIC, locale);

    return result;
}
#undef BUF_SIZE

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
    /*@only@*/ cif_value_t *temp = (cif_value_t *) malloc(sizeof(cif_value_t));

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
                cif_list_init(&(temp->as_list));   /* no additional resource allocation here */
                break;
            case CIF_TABLE_KIND:
                cif_table_init(&(temp->as_table)); /* no additional resource allocation here */
                break;
            default:
                break;
        }

        *value = temp;
        return CIF_OK;

        FAILURE_HANDLER(soft):
        free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_value_clean(union cif_value_u *value) {
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

    return CIF_OK;
}

int cif_value_free(union cif_value_u *value) {
    if (value != NULL) {
        cif_value_clean(value);
        free(value);
    }

    return CIF_OK;
}

int cif_value_clone(cif_value_t *value, cif_value_t **clone) {
    FAILURE_HANDLING;
    cif_value_t *temp;
    cif_value_t *to_free = NULL;

    if (*clone != NULL) {
        if (cif_value_clean(*clone) != CIF_OK) DEFAULT_FAIL(soft);
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
            /* TODO: this looks wrong; should we really be converting from digit chars to digit values here? */
            *(suc++) = (text[su_start++] - UCHAR_0);
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
    n_temp.digits = (char *) malloc((digit_end + 1) - (digit_start + num_decimal));
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
        if (cif_value_clean(n) == CIF_OK) {
            numb->kind = CIF_NUMB_KIND;
            numb->text = text;
            numb->sign = n_temp.sign;
            numb->digits = n_temp.digits;
            numb->su_digits = n_temp.su_digits;
            numb->scale = n_temp.scale;
            return CIF_OK;
        }

        free(n_temp.digits);
    }

    FAILURE_HANDLER(late):
    free(n_temp.su_digits); /* safe when n_temp.su_digits is NULL */

    FAILURE_HANDLER(early):
    FAILURE_TERMINUS;
}

int cif_value_init_char(cif_value_t *value, UChar *text) {
    int clean_result = cif_value_clean(value);

    if (clean_result == CIF_OK) {
        value->as_char.text = text;
        value->kind = CIF_CHAR_KIND;
        return CIF_OK;
    } else {
        return clean_result;
    }
}

int cif_value_init_numb(cif_value_t *n, double val, double su, int scale, int max_leading_zeroes) {
    if ((su >= 0.0) && (-scale >= LEAST_DBL_10_DIGIT) && (-scale <= DBL_MAX_10_EXP) && (max_leading_zeroes >= 0)) {
        /* Arguments appear valid */

        FAILURE_HANDLING;
        struct numb_value_s *numb = &(n->as_numb);
        double abs_val = fabs(val);
        int most_significant_place = (int) ((val == 0.0) ? 0 : log10(abs_val));
        char *locale = setlocale(LC_NUMERIC, "C");

        if (locale != NULL) {
            if (cif_value_clean(n) == CIF_OK) {
                if ((scale >= 0) && (-(most_significant_place + 1) <= max_leading_zeroes)) {
                    /* use decimal notation */
                    if (format_value_decimal(val, su, scale, numb) != CIF_OK) DEFAULT_FAIL(soft);
                } else {
                    /* use scientific notation */
                    if (format_value_sci(val, su, scale, numb) != CIF_OK) DEFAULT_FAIL(soft);
                }

                numb->kind = CIF_NUMB_KIND;
                numb->sign = (val < 0) ? -1 : 1;
                numb->scale = scale;
                SET_RESULT(CIF_OK);
            }

            FAILURE_HANDLER(soft):
            /* restore the original locale */
            (void) setlocale(LC_NUMERIC, locale);
        }
        FAILURE_TERMINUS;
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

#define BUF_SIZE 50
int cif_value_autoinit_numb(cif_value_t *numb, double val, double su, unsigned int su_rule) {
    if ((su >= 0.0) && (su_rule >= 9) && (cif_value_clean(numb) == CIF_OK)) {
        /* Arguments appear valid */

        if (su == 0.0) {
            int most_significant_place = (int) ((val == 0.0) ? 0.0 : log10(fabs(val)));
            int scale = -most_significant_place + (DBL_DIG - 1);

            return cif_value_init_numb(numb, val, su, scale, DEFAULT_MAX_LEAD_ZEROES);
        } else {
            int result_code = CIF_INTERNAL_ERROR;
            int rule_digits;
            char buf[BUF_SIZE];

            /* number formatting and parsing must be done in the C locale to ensure portability */
            char *locale = setlocale(LC_NUMERIC, "C");

            if (locale != NULL) {
#ifdef CIF_MAX_10_EXP_DIG
                int extra_digits = CIF_MAX_10_EXP_DIG;
#else
                int exp_max = ((DBL_MAX_10_EXP >= -(DBL_MIN_10_EXP)) ? DBL_MAX_10_EXP : -(DBL_MIN_10_EXP));
                int extra_digits;

                for (extra_digits = 1; exp_max > 9; exp_max /= 10) extra_digits += 1;
#endif

                /* determine the number of significant digits in the su_rule */
                for (rule_digits = 1; su_rule > 9; su_rule /= 10) rule_digits += 1;

                /*
                 * Assuming that the buffer is large enough (which it very much should be), format the su using the same
                 * number of total digits as the su_rule has sig-figs to determine the needed scale.  This approach is a
                 * bit inefficient, but it's straightforward, reliable, and portable.
                 */
                if ((rule_digits + extra_digits + 4 < BUF_SIZE)
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

double cif_value_as_double(cif_value_t *n) {
    struct numb_value_s *numb = &(n->as_numb);
    double d;

    assert(numb->kind == CIF_NUMB_KIND);

    d = digits_as_double(numb->digits, numb->scale);
    return ((numb->sign < 0) ? -d : d);
}

double cif_value_su_as_double(cif_value_t *n) {
    struct numb_value_s *numb = &(n->as_numb);

    assert(numb->kind == CIF_NUMB_KIND);

    if (numb->su_digits == NULL) {
        return 0.0;
    } else {
        /* always non-negative */
        return digits_as_double(numb->su_digits, numb->scale);
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
        int result = cif_value_clone(element, &clone);

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

