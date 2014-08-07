/*
 * utils.h
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 */

#ifndef INTERNAL_UTILS_H
#define INTERNAL_UTILS_H

#include <string.h>
#include <unicode/ustring.h>
#include "cif.h"
#include "internal/ciftypes.h"
#include "internal/buffer.h"
#include "internal/value.h"

#ifdef __GNUC__
#define UNUSED   __attribute__ ((__unused__))
#define INTERNAL __attribute__ ((__visibility__ ("hidden"), __warn_unused_result__))
#define INTERNAL_VOID __attribute__ ((__visibility__ ("hidden")))
#else
#define UNUSED
#define INTERNAL
#define INTERNAL_VOID
#endif

/* simple macros */

/*
 * Macros for the compiler's sense of true and false values (we cannot use _bool
 * because we want to maintain code compatibility with strict C89 compilers).
 */
#define CIF_TRUE  (1 == 1)
#define CIF_FALSE (1 == 0)

/*
 * Macros for the boundaries of UTF-16 surrogate code units; use of these macros
 * assumes ints are wider than 16 bits.
 */
#define MIN_LEAD_SURROGATE   0xd800
#define MIN_TRAIL_SURROGATE  0xdc00
#define MAX_SURROGATE        0xdfff

/* debug macros and data */

/*
 * Framework code for debug messaging wrappers.  No problem with declaring
 * _debug_result as a static global for this purpose.
 */
#ifdef DEBUG
#include <stdio.h>
#include <sqlite3.h>
#include <unicode/ustdio.h>
/* If multithreaded debugging ever is wanted then these will need to be thead-local: */
static int _debug_result = SQLITE_OK;
static void *_debug_ptr = NULL;

extern UFILE *ustderr;
#define INIT_USTDERR do { if (ustderr == NULL) ustderr = u_finit(stderr, NULL, NULL); } while(0)

/*
 * A wrapper that, when enabled, intercepts the result codes of SQLite functions
 * and emits an explanatory message if it's different from SQLITE_OK (which is not
 * necessarily an error condition).  Evaluates in any case to (f).
 *
 * db: an sqlite3 * from which to extract an error message in the event that an error occurs
 * f: a SQLite function call or result to possibly be debugged
 */
#define DEBUG_WRAP(db,f) ( \
  (_debug_result = (f)), \
  ((_debug_result == SQLITE_OK) ? 0 \
    : fprintf(stderr, "At line %d in " __FILE__ ", SQLite error code %d: %s\n", __LINE__, \
              _debug_result, sqlite3_errmsg(db))), \
  _debug_result \
)
/*
 * A wrapper that, when enabled, intercepts the result codes of SQLite functions
 * and emits it if it differs from SQLITE_OK (which is not necessarily an error condition).
 * Evaluates in any case to (f).
 *
 * f: a SQLite function call or result to possibly be debugged
 */
#define DEBUG_WRAP2(f) ( \
  (_debug_result = (f)), \
  (_debug_result == SQLITE_OK) ? 0 \
    : fprintf(stderr, "At line %d in " __FILE__ ", SQLite error code %d\n", __LINE__, _debug_result), \
  _debug_result \
)

/*
 * A malloc wrapper that traps allocation errors
 */
#define malloc(size) ( \
  _debug_ptr = malloc(size), \
  (_debug_ptr != NULL) ? 0 \
    : fprintf(stderr, "At line %d in " __FILE__ ", memory allocation failure\n", __LINE__), \
  _debug_ptr \
)

/*
 * In a debugging build, emits the current file and line number to stderr
 */
#define TRACELINE fprintf(stderr, __FILE__ " line %d\n", __LINE__)

/*
 * In a debugging build, emits the file and line number, the lead string, and the specified message, and
 * evaluates to the message.
 * Argument 'msg' may be evaluated more than once.
 */
#define DEBUG_MSG(lead, msg) (fprintf(stderr, __FILE__ " line %d, %s: %s\n", __LINE__, (lead), (msg)), (msg))

#else  /* !DEBUG */
#define DEBUG_WRAP(c,f) (f)
#define DEBUG_WRAP2(f)  (f)
#define TRACELINE
#define INIT_USTDERR
#define DEBUG_MSG(lead, msg) (msg)
#endif /* !DEBUG */

/* function-like macros */

/*
 * Frees and nullifies the specified pointer, which must be a pointer lvalue
 * (of any referrent type).  The argument may be NULL; otherwise it must be a
 * valid pointer suitable to be passed to free().
 *
 * Because this macro sets the argument to NULL, it is safe to invoke it
 * multiple times on the same argument.
 */
#define CLEAN_PTR(ptr) do { \
    free(ptr); \
    ptr = NULL; \
} while (0)

/*
 * Records the specified pointer value at the specified location, or calls the
 * specified function to clean it up if the location is NULL
 *
 * temp: the pointer to assign
 * dest_p: a pointer to the target location
 * free_fn: the name of a one-arg function to which 'temp' can be passed, as
 *         necessary, to clean it up
 */
#define ASSIGN_TEMP_PTR(temp, dest_p, free_fn) do { \
    if (dest_p == NULL) { \
        free_fn(temp); \
    } else { \
        *dest_p = temp; \
    } \
} while (0)

/*
 * expands to the name of the variable wherein the current failure handler
 * error code is recorded; in some cases it may be clearer to set this
 * directly than to use SET_RESULT()
 */
#define FAILURE_VARIABLE _error_code

/*
 * Sets up failure handling for the containing function.  Must appear only
 * where variable declarations are allowed.  Not necessarily atomic.  Must
 * not be used more than once in the same scope.
 */
#define FAILURE_HANDLING int _error_code = CIF_ERROR

/*
 * Sets a result (failure) code without jumping.  Can be used to, among
 * other things, allow control to pass through the same cleanup path on
 * success as it does on failure.
 */
#ifdef DEBUG
#define SET_RESULT(code) do { \
  _error_code = (code); \
  if (_error_code != CIF_OK) \
    fprintf(stderr, "At line %d in " __FILE__ ", result set to code %d\n", __LINE__, _error_code); \
} while (0)
#else
#define SET_RESULT(code) _error_code = (code)
#endif

#define HANDLER_LABEL(type) type##_fail

/*
 * Branches to the failure handler for the specified failure type without
 * setting an error code.  Unless the error code has been changed since failure
 * handling was initialized for the current function, and unless the error code
 * is changed in the error handler, this will result in error code CIF_ERROR.
 */
#ifdef DEBUG
#define DEFAULT_FAIL(type) do { \
  fprintf(stderr, "At line %d in " __FILE__ ", executing " #type " failure with code %d\n", __LINE__, _error_code); \
  goto HANDLER_LABEL(type); \
} while (0)
#else
#define DEFAULT_FAIL(type) goto HANDLER_LABEL(type)
#endif

/*
 * Sets the specified error code in variable error_code, and branches to the
 * failure handler for the specified failure type
 */
#define FAIL(type, code) do { _error_code = code; DEFAULT_FAIL(type); } while (0)

/*
 * Tags the beginning of the handler code for the specified type of failure
 * in the current function.  Like a label, this macro should be terminated
 * by a colon instead of a semicolon.
 */
#define FAILURE_HANDLER(type) HANDLER_LABEL(type)

/*
 * Control flow from a FAILURE_HANDLER tag should eventually reach a
 * FAILURE_TERMINUS in the same function, which will return an error code.
 * Should not be assumed atomic.  Must appear in the same scope as a
 * a FAIL_HANDLING macro.
 */
#define FAILURE_TERMINUS return _error_code

/*
 * An alias for FAILURE_TERMINUS, for clarity where a success result is known
 * to have been achieved, but a variable result code can be returned.
 */
#define SUCCESS_TERMINUS FAILURE_TERMINUS

/*
 * An alias for FAILURE_TERMINUS, for clarity where control flow reaches the
 * same terminus in both success and failure cases
 */
#define GENERAL_TERMINUS FAILURE_TERMINUS

/*
 * Transaction management macros.  The argument to each should be an sqlite3
 * connection pointer.
 */
#define BEGIN(db) DEBUG_WRAP((db), sqlite3_exec((db), "begin", NULL, NULL, NULL))
#define COMMIT(db) DEBUG_WRAP((db), sqlite3_exec((db), "commit", NULL, NULL, NULL))
#define ROLLBACK(db) DEBUG_WRAP((db), sqlite3_exec((db), "rollback", NULL, NULL, NULL))

#define SAVE(db) DEBUG_WRAP((db), sqlite3_exec((db), "savepoint s", NULL, NULL, NULL))
#define RELEASE(db) DEBUG_WRAP((db), sqlite3_exec((db), "release s", NULL, NULL, NULL))
#define ROLLBACK_TO(db) DEBUG_WRAP((db), sqlite3_exec((db), "rollback to s", NULL, NULL, NULL))

#define NESTTX_HANDLING int _top_tx
#define BEGIN_NESTTX(db) ( \
  (_top_tx = sqlite3_get_autocommit(db)), \
  ((_top_tx == 0) ? SAVE(db) : BEGIN(db)) )
#define COMMIT_NESTTX(db) ((_top_tx == 0) ? RELEASE(db) : COMMIT(db))
#define ROLLBACK_NESTTX(db) ((_top_tx == 0) ? ROLLBACK_TO(db) : ROLLBACK(db))

/*
 * A macro expression evaluating to zero if the specified error code
 * reflects a transient or data-related condition, or nonzero otherwise.
 *
 * code: the int error code; evaluated once only
 * t: an int temporary lvalue; its contents will be overwritten with the
 *     result of evaluating the 'code' argument
 */
#define IS_HARD_ERROR(code, t) ( \
  t = (code), \
  ((t != SQLITE_OK) \
          && (t != SQLITE_ROW) \
          && (t != SQLITE_DONE) \
          && ((t & 0xff) != SQLITE_CONSTRAINT) \
          && ((t & 0xff) != SQLITE_BUSY) \
          && ((t & 0xff) != SQLITE_LOCKED)) \
)

/*
 * Releases any resources associated with the named prepared statement of the
 * specified CIF, and clears the pointer to it.  Any error is ignored.
 *
 * c: an expression of type cif_t or, equivalently, cif_t *,
 *    representing the CIF whose prepared statement is to be dropped.
 *    It is an error for the expression to evaluate to NULL at runtime.
 * stmt_name: the name of the statement to drop, less the trailing "_stmt"
 *    tag, as plain text.
 */
#define DROP_STMT(c, stmt_name) do { \
    cif_t *d_cif = (c); \
    sqlite3_finalize(d_cif->stmt_name##_stmt); \
    d_cif->stmt_name##_stmt = NULL; \
} while (0)

/*
 * Prepares an existing SQL statement of the specified CIF for reuse, or
 * prepares a new one if it cannot do so.  Causes CIF_ERROR to
 * be returned from the context function if a presumed-usable statement is
 * not ultimately prepared.
 *
 * c: an expression of type cif_t or, equivalently, cif_t *,
 *    representing the CIF for which to prepare a statement.
 *    It is an error for the expression to evaluate to NULL at runtime.
 * stmt_name: the name of the statement to prepare, less the trailing "_stmt"
 *    tag, as plain text.
 * sql: the SQL text of the (single) statement to prepare, as a
 *    null-terminated UTF-8 byte string
 */
#define PREPARE_STMT(c, stmt_name, sql) do { \
    cif_t *p_cif = (c); \
    int _t; \
    if ((p_cif->stmt_name##_stmt == NULL) \
            || (IS_HARD_ERROR(DEBUG_WRAP(p_cif->db, sqlite3_reset(p_cif->stmt_name##_stmt)), _t) != 0) \
            || (DEBUG_WRAP(p_cif->db, sqlite3_clear_bindings(p_cif->stmt_name##_stmt)) != SQLITE_OK)) { \
        DROP_STMT(p_cif, stmt_name); \
        if (DEBUG_WRAP(p_cif->db, sqlite3_prepare_v2(p_cif->db, sql, -1, &(p_cif->stmt_name##_stmt), NULL)) != SQLITE_OK) { \
            return CIF_ERROR; \
        } \
    } \
} while (0)

/*
 * Declares temporary variables used transiently by the STEP_STMT macro
 */
#define STEP_HANDLING sqlite3_stmt *_step_stmt; int _step_result

/*
 * Steps the statement of the specified name in the specified cif, resetting
 * the statement if its evaluation is complete (without error).  The expansion
 * is an expression that evaluates to the return code of the sqlite3_step()
 * call.
 *
 * This macro may only be used in where the declarations made by a preceding
 * STEP_HANDLING macro are in-scope.
 */
#define STEP_STMT(c, stmt_name) DEBUG_WRAP((c)->db, ( \
    _step_stmt = (c)->stmt_name##_stmt, \
    _step_result = sqlite3_step(_step_stmt), \
    (_step_result == SQLITE_DONE) ? sqlite3_reset(_step_stmt) : SQLITE_OK, \
    _step_result \
))

/*
 * Copies a a result value out of the specified column (col) of the specified
 * prepared statement (stmt) into newly-allocated space.  The copy is ensured
 * null-terminated, and a pointer to it is recorded in 'dest'.  If an error
 * occurs then jumps to the specified label (onerr).  Only if execution
 * does not branch to 'onerr' may dest afterward point to memory that needs
 * to be managed (but dest may be NULL in any case).
 */
#define GET_COLUMN_STRING(stmt, col, dest, onerr) do { \
    const UChar *string_val = (const UChar *) sqlite3_column_text16(stmt, col); \
    if (string_val == NULL) { \
        dest = NULL; \
    } else { \
        size_t value_bytes = (size_t) sqlite3_column_bytes16(stmt, col); \
        int32_t value_chars; \
        dest = (UChar *) malloc(value_bytes + sizeof(UChar)); \
        if (dest == NULL) goto onerr; \
        value_chars = (int32_t) (value_bytes / 2); \
        u_strncpy(dest, string_val, value_chars); \
        dest[value_chars] = 0; \
    } \
} while (0)

/*
 * Copies a a result value out of the specified column (col) of the specified
 * prepared statement (stmt) into newly-allocated space.  The copy is ensured
 * null-terminated, and a pointer to it is recorded in 'dest'.  If an error
 * occurs then jumps to the specified label (onerr).  Only if execution
 * does not branch to 'onerr' may dest afterward point to memory that needs
 * to be managed (but dest may be NULL in any case).
 */
#define GET_COLUMN_BYTESTRING(stmt, col, dest, onerr) do { \
    const char *string_val = (const char *) sqlite3_column_text(stmt, col); \
    if (string_val == NULL) { \
        dest = NULL; \
    } else { \
        size_t value_bytes = (size_t) sqlite3_column_bytes(stmt, col); \
        dest = (char *) malloc(value_bytes + 1); \
        if (dest == NULL) goto onerr; \
        strncpy(dest, string_val, value_bytes); \
        dest[value_bytes] = '\0'; \
    } \
} while (0)

/*
 * Binds the fields of a value object to the parameters of a prepared statement in a manner appropriate to the
 * value's kind (but does not assign the kind itself).  Reseting the statement and / or clearing its bindings is
 * the responsibility of the macro user.  Bindings should be cleared at least before updating a value with one
 * of a different kind.
 *
 * stmt: a pointer to the sqlite3_stmt object whose parameters are to be updated.  It must have a consecutive
 *   sequence of parameters corresponding, respectively, to these columns of table item_value:
 *   kind, val_text, val, val_digits, su_digits, scale
 * col_ofs: one less than the index of the prepared statement parameter corresponding to item_value.kind
 * val: a pointer to the value object from which to fill statement parameters
 * onsqlerr: the code for the failure handler to invoke in the event that any of the parameter bindings fails
 * ondataerr: the code for the failure handler to invoke in the event of a data malformation (can only happen
 * for LIST and TABLE values)
 */
#define SET_VALUE_PROPS(stmt, col_ofs, val, onsqlerr, ondataerr) do { \
    sqlite3_stmt *s = (stmt); \
    int ofs = (col_ofs); \
    cif_value_t *v = (val); \
    double svp_d; \
    buffer_t *buf; \
    if (sqlite3_bind_int(s, 1 + ofs, v->kind) != SQLITE_OK) { \
        DEFAULT_FAIL(onsqlerr); \
    } \
    switch (v->kind) { \
        case CIF_CHAR_KIND: \
            if ((sqlite3_bind_text16(s, 2 + ofs, v->as_char.text, -1, SQLITE_STATIC) != SQLITE_OK) \
                    || (sqlite3_bind_text16(s, 3 + ofs, v->as_char.text, -1, SQLITE_STATIC)) != SQLITE_OK) { \
                DEFAULT_FAIL(onsqlerr); \
            } \
            break; \
        case CIF_NUMB_KIND: \
            if ((sqlite3_bind_text16(s, 2 + ofs, v->as_numb.text, -1, SQLITE_STATIC) != SQLITE_OK) \
                    || (cif_value_get_number(v, &svp_d) != CIF_OK) \
                    || (sqlite3_bind_double(s, 3 + ofs, svp_d) != SQLITE_OK) \
                    || (sqlite3_bind_text(s, 4 + ofs, v->as_numb.digits, -1, SQLITE_STATIC) != SQLITE_OK) \
                    || (sqlite3_bind_text(s, 5 + ofs, v->as_numb.su_digits, -1, SQLITE_STATIC) != SQLITE_OK) \
                    || (sqlite3_bind_int(s, 6 + ofs, v->as_numb.scale) != SQLITE_OK)) { \
                DEFAULT_FAIL(onsqlerr); \
            } \
            break; \
        case CIF_LIST_KIND: \
        case CIF_TABLE_KIND: \
            buf = cif_value_serialize(v); \
            if (buf == NULL) { \
                DEFAULT_FAIL(ondataerr); \
            } else { \
                char *blob = buf->for_writing.start; \
                int limit = (int) buf->for_writing.limit; \
                cif_buf_free_metadata(buf); \
                if (sqlite3_bind_blob(s, 3 + ofs, blob, limit, free) != SQLITE_OK) { \
                    DEFAULT_FAIL(onsqlerr); \
                } \
            } \
            break; \
        default: \
            break; \
    }  \
} while (0)

/* column order: kind, val, val_text, val_digits, su_digits, scale */
#define GET_VALUE_PROPS(_s, _ofs, _val, errlabel) do { \
    sqlite3_stmt *_stmt = (_s); \
    cif_value_t *_value = (_val); \
    int _col_ofs = (_ofs); \
    const void *_blob; \
    _value->kind = (cif_kind_t) sqlite3_column_int(_stmt, _col_ofs); \
    switch (_value->kind) { \
        case CIF_CHAR_KIND: \
            GET_COLUMN_STRING(_stmt, _col_ofs + 2, _value->as_char.text, HANDLER_LABEL(errlabel)); \
            if (_value->as_char.text != NULL) break; \
            FAIL(errlabel, CIF_INTERNAL_ERROR); \
        case CIF_NUMB_KIND: \
            GET_COLUMN_STRING(_stmt, _col_ofs + 2, _value->as_numb.text, HANDLER_LABEL(errlabel)); \
            GET_COLUMN_BYTESTRING(_stmt, _col_ofs + 3, _value->as_numb.digits, HANDLER_LABEL(errlabel)); \
            if ((_value->as_numb.text != NULL) && (*(_value->as_numb.text) != 0) && (_value->as_numb.digits != NULL) \
                    && (*(_value->as_numb.digits) != '\0')) { \
                GET_COLUMN_BYTESTRING(_stmt, _col_ofs + 4, _value->as_numb.su_digits, HANDLER_LABEL(errlabel)); \
                _value->as_numb.scale = sqlite3_column_int(_stmt, _col_ofs + 5); \
                _value->as_numb.sign = (*(_value->as_numb.text) == UCHAR_MINUS) ? -1 : 1; \
                break; \
            } \
            FAIL(errlabel, CIF_INTERNAL_ERROR); \
        case CIF_LIST_KIND: \
        case CIF_TABLE_KIND: \
            _blob = (const void *) sqlite3_column_blob(_stmt, _col_ofs + 1); \
            if ((_blob != NULL) && (cif_value_deserialize( \
                    _blob, (size_t) sqlite3_column_bytes(_stmt, _col_ofs + 1), _value) != NULL)) { \
                break; \
            } \
            FAIL(errlabel, CIF_INTERNAL_ERROR); \
        case CIF_UNK_KIND: \
        case CIF_NA_KIND: \
            break; \
        default: \
            FAIL(errlabel, CIF_INTERNAL_ERROR); \
    } \
} while (0)

/* The number of _bytes_ in the given null(-character)-terminated Unicode string */
#define U_BYTES(s) (u_strlen(s) * sizeof(UChar))

/*
 * function headers for private, non-static functions
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * An internal version of cif_create_block() that allows block code validation to be suppressed (when 'lenient' is
 * nonzero)
 */
int cif_create_block_internal(
        cif_t *cif,
        const UChar *code,
        int lenient,
        cif_block_t **block
        ) INTERNAL ;

/*
 * An internal version of cif_container_create_frame() that allows frame code
 * validation to be suppressed (when 'lenient' is nonzero)
 */
int cif_container_create_frame_internal(
        cif_container_t *container,
        const UChar *code,
        int lenient,
        cif_frame_t **frame
        ) INTERNAL ;

/*
 * An internal version of cif_loop_add_item that performs no validation or normalization and provides the number
 * of changes (== the number of loop packets) back to the caller
 */
int cif_loop_add_item_internal(
        cif_loop_t *loop,
        const UChar *item_name,
        const UChar *norm_name,
        cif_value_t *val,
        int *changes
        ) INTERNAL ;

/*
 * Creates a new packet for the given item names, and records a pointer to it where the given pointer points.  The
 * names are assumed already normalized, as if by cif_normalize_name()
 *
 * packet: a pointer to the location were the address of the new packet should be written
 * names:  a pointer to a NULL-terminated array of Unicode string pointers containing the data names to be
 *         represented in the new packet; may contain zero names
 * avoid_aliasing: if this argument evaluates to true, then the given names must be copied into the new packet;
 *         otherwise, the pointers to them may be copied.  In the latter case, this function acquires responsibility
 *         for the provided names.
 */
int cif_packet_create_norm(
        cif_packet_t **packet,
        UChar **names,
        int avoid_aliasing
        ) INTERNAL;

/*
 * Validates a CIF block code or frame code, or similar "case insensitive" name,
 * and creates a normalized version suitable for use as a database or hash key,
 * or for equivalency comparisons.
 *
 * name: a pointer to the Unicode string to normalize; not modified, must not be
 *     NULL
 * namelen: the maximum number of characters to normalize; if less than zero then
 *     the whole string is normalized, up to the first NUL character
 * normalized_name: if non-NULL, a pointer to the normalized string is written
 *     where this pointer points; otherwise the normalized result is discarded,
 *     but the return code still indicates whether normalization succeeded
 * invalidityCode: the return code to use in the event that the specified name
 *     does not meet the validity criteria for a frame or block code
 *
 * The normalization applied by this function is to convert the input to Unicode
 * normalization form NFD, then case-fold the result, and finally to transform
 * the case-folded string to Unicode normalization form NFC.
 */
int cif_normalize_name(
        const UChar *name,
        int32_t namelen,
        UChar **normalized_name,
        int invalidityCode
        ) INTERNAL;

/*
 * Validates a CIF data name, and creates a normalized version suitable for use
 * as a database or hash key, or for equivalency comparisons.
 *
 * name: a pointer to the Unicode string to normalize; not modified, must not be
 *     NULL
 * namelen: the maximum number of characters to normalize; if less than zero then
 *     the whole string is normalized, up to the first NUL character
 * normalized_name: if non-NULL, a pointer to the normalized string is written
 *     where this pointer points; otherwise the normalized result is discarded,
 *     but the return code still indicates whether normalization succeeded
 * invalidityCode: the return code to use in the event that the specified name
 *     does not meet the validity criteria for a data name
 *
 * The normalization applied by this function is to convert the input to Unicode
 * normalization form NFD, then case-fold the result, and finally to transform
 * the case-folded string to Unicode normalization form NFC.
 */
int cif_normalize_item_name(
        const UChar *name,
        int32_t namelen,
        UChar **normalized_name,
        int invalidityCode
        ) INTERNAL;

/*
 * Validates a CIF table index, and creates a normalized version suitable for use
 * as a hash key or for equivalency comparisons.
 *
 * name: a pointer to the Unicode string to normalize; not modified, must not be
 *     NULL
 * namelen: the maximum number of characters to normalize; if less than zero then
 *     the whole string is normalized, up to the first NUL character
 * normalized_name: if non-NULL, a pointer to the normalized string is written
 *     where this pointer points; otherwise the normalized result is discarded,
 *     but the return code still indicates whether normalization succeeded
 * invalidityCode: the return code to use in the event that the specified name
 *     does not meet the validity criteria for a data name
 *
 * The normalization applied by this function is simply to convert the input to
 * Unicode normalization form NFC
 */
int cif_normalize_table_index(
        const UChar *name,
        int32_t namelen,
        UChar **normalized_name,
        int invalidityCode
        ) INTERNAL;

/*
 * Releases the map metadata of an entry structure.  This allows the entry to be used as a fat value after removal from
 * its map, without leaking the resources previously bound up in the metadata.
 *
 * entry: the entry object whose metadata are to be released
 *
 * map: the map from which the entry was extracted; this is necessary to determine which metadata belong to the entry
 */
CIF_VOIDFUNC_DECL(cif_map_entry_clean_metadata_internal, (
        struct entry_s *entry,
        cif_map_t *map
        )) INTERNAL_VOID;

/*
 * Frees a map entry and all resources associated with it, including the value.  Should be used only after the entry
 * is removed from its map.
 *
 * entry: the entry object to free
 *
 * map: the map from which the entry was extracted; this is necessary to determine which metadata belong to the entry
 */
CIF_VOIDFUNC_DECL(cif_map_entry_free_internal, (
        struct entry_s *entry,
        cif_map_t *map
        )) INTERNAL_VOID;

/*
 * Serializes a value to this library's internal serialization format.
 * Returns NULL if serialization fails (most likely because of insufficient memory).
 */
buffer_t *cif_value_serialize(
        cif_value_t *value
        ) INTERNAL;

/*
 * Deserializes a value from this library's internal serialization format.
 * If dest is non-NULL then the deserialized value will be recorded in the value
 * object it points to, overwriting any data already there; otherwise a new
 * value will be allocated. Returns a pointer to the deserialized value object,
 * or NULL if deserialization fails (most likely because of insufficient memory
 * or a format error).
 */
cif_value_t *cif_value_deserialize(
        const void *src,
        size_t len,
        cif_value_t *dest
        ) INTERNAL;

/*
 * Sets any and all (possibly looped) values for the specified item in the
 * specified container; no values are set if the item does not appear in the
 * container.  The item name is assumed already normalized.  No (explicit)
 * transaction management is performed.
 */
int cif_container_set_all_values(
        cif_container_t *container,
        const UChar *item_name,
        cif_value_t *val
        ) INTERNAL;

/*
 * Releases all resources associated with the specified packet iterator.  This is intended for internal
 * use by the library -- client code should instead call cif_pkitr_close() or cif_pktitr_abort().
 */
void cif_pktitr_free(
        cif_pktitr_t *iterator
        ) INTERNAL_VOID;

/*
 * A common parser back end (potentially) serving multiple front ends that accept CIF data in different
 * forms.
 *
 * @param[in,out] scanner a pointer to a scanner structure initialized with character source properties, user options,
 *         and an initial CIF version assertion / evaluation
 * @param[in] not_utf8 if non-zero, indicates that the characters provided by the scanner's character source are known
 *         to be derived from an encoded byte sequence via an encoding different from UTF-8
 * @param[in,out] dest a pointer to a @c cif_t object to update with the CIF data read from the provided source.  May
 *         be NULL, in which case a semantic-free syntax check of the provided character stream is performed without
 *         recording anything
 */
int cif_parse_internal(
        struct scanner_s *scanner,
        int not_utf8,
        cif_t *dest
        );

#ifdef __cplusplus
}
#endif

#endif

