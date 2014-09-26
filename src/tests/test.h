/*
 * test.h
 *
 * Macros for CIF test fixture setup and teardown.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#ifndef TESTS_TEST_H
#define TESTS_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include <sqlite3.h>
#include "uthash.h"
#include "../cif.h"

#ifdef __GNUC__
#define UNUSED   __attribute__ ((__unused__))
#else
#define UNUSED
#endif

/*
 * The name of the directory containing the test files it is evaluated relative to the working directory unless the
 * environment variable CIFAPI_SRC is set, in which case it is evaluated relative to the directory named by that
 * environment variable
 */
#define DATA_DIR "test-data"

/*
 * Standardized result codes.  Any other return code represents a normal failure.
 */
#define SUCCESS    0
#define SKIP      77
#define HARD_FAIL 99

struct set_el {
    UT_hash_handle hh;
};

#ifdef USE_USTDERR
static UFILE *ustderr = NULL;

#define INIT_USTDERR do { if (ustderr == NULL) ustderr = u_finit(stderr, NULL, NULL); } while (0)
#endif

#define TESTHEADER(name) do { \
  fprintf(stderr, "\n-- %s --\n", (name)); \
} while (0)

#define RETURN(code) do { \
  sqlite3_shutdown(); \
  return (code); \
} while (0)

/*
 * Emits a failure message to stderr and returns the specified code.
 */
#define FAIL(ret, name, code, sense, compare) do { \
  int _ret = (ret); \
  fprintf(stderr, "%s(%d): ... failed with code %d " sense " %d at line %d in " __FILE__ ".\n", \
          (name), _ret, (code), (compare), __LINE__); \
  return (ret); \
} while (0)

/*
 * Evaluates (expr) and compares the result to (expect).  If they differ then a test failure is triggered
 * with failure code (fail_code).  In the event of failure, a message emitted to stderr will identify the
 * failing test as (name).
 */
#define TEST(expr, expect, name, fail_code) do { \
  int _result = (expr); \
  int _expect = (expect); \
  int _code = (fail_code); \
  if (_result != _expect) FAIL(_code, (name), _result, "!=", _expect); \
  fprintf(stderr, "  subtest %d passed\n", _code); \
} while (0)

/*
 * Like TEST, but with the sense of the success/failure criterion reversed.
 */
#define TEST_NOT(expr, expect_not, name, fail_code) do { \
  int _result = (expr); \
  int _expect_not = (expect_not); \
  int _code = (fail_code); \
  if (_result == _expect_not) FAIL((fail_code),(name),_result,"==",_expect_not); \
  fprintf(stderr, "  subtest %d passed\n", _code); \
} while (0)

#define TO_UNICODE(s, buffer, buf_len) ( \
    u_unescape((s), buffer, buf_len), \
    buffer \
)

/*
 * Creates a new managed cif, recording a handle on it in cif, which must therefore be an lvalue.
 * Generates a hard failure if unsuccessful.
 */
#define CREATE_CIF(n, cif) do { \
    const char *_test_name = (n); \
    int _result; \
    fprintf(stderr, "%s: Creating a managed CIF...\n", _test_name); \
    _result = cif_create(&cif); \
    if (_result != CIF_OK) { \
        fprintf(stderr, "error: %s: ... failed with code %d.\n", _test_name, _result); \
        return HARD_FAIL; \
    } else if (cif == NULL) { \
        fprintf(stderr, "error: %s: ... did not set the CIF pointer.\n", _test_name); \
        return HARD_FAIL; \
    } \
} while (0)

/*
 * Destroys the specified managed cif, or emits a warning if it fails to do so
 */
#define DESTROY_CIF(n, cif) do { \
    const char *_test_name = (n); \
    int _result; \
    fprintf(stderr, "%s: Destroying a managed CIF...\n", _test_name); \
    _result = cif_destroy(cif); \
    if (_result != CIF_OK) { \
        fprintf(stderr, "warning: %s: ... failed with code %d.\n", _test_name, _result); \
    } \
} while (0)

/*
 * Creates a new data block bearing the specified code in the specified cif, recording a handle on it
 * in 'block', which must therefore be an lvalue.  Generates a hard failure if unsuccessful.
 */
#define CREATE_BLOCK(n, cif, code, block) do { \
    const char *_test_name = (n); \
    int _result; \
    UChar *_code = (code); \
    fprintf(stderr, "%s: Creating a managed data block...\n", _test_name); \
    _result = cif_create_block((cif), _code, &block); \
    if (_result != CIF_OK) { \
        fprintf(stderr, "error: %s: ... failed with code %d.\n", _test_name, _result); \
        return HARD_FAIL; \
    } else if (block == NULL) { \
        fprintf(stderr, "error: %s: ... did not set the block pointer.\n", _test_name); \
        return HARD_FAIL; \
    } \
} while (0)

/*
 * Destroys the specified managed data block, or emits a warning if it fails to do so
 */
#define DESTROY_BLOCK(n, block) do { \
    const char *_test_name = (n); \
    int _result; \
    fprintf(stderr, "%s: Destroying a managed data block...\n", _test_name); \
    _result = cif_container_destroy(block); \
    if (_result != CIF_OK) { \
        fprintf(stderr, "warning: %s: ... failed with code %d.\n", _test_name, _result); \
    } \
} while (0)

/*
 * Creates a new save frame bearing the specified code in the specified block, recording a handle on it
 * in 'frame', which must therefore be an lvalue.  Generates a hard failure if unsuccessful.
 */
#define CREATE_FRAME(n, block, code, frame) do { \
    const char *_test_name = (n); \
    int _result; \
    UChar *_code = (code); \
    fprintf(stderr, "%s: Creating a managed save frame...\n", _test_name); \
    _result = cif_block_create_frame((block), _code, &frame); \
    if (_result != CIF_OK) { \
        fprintf(stderr, "error: %s: ... failed with code %d.\n", _test_name, _result); \
        return HARD_FAIL; \
    } else if (frame == NULL) { \
        fprintf(stderr, "error: %s: ... did not set the frame pointer.\n", _test_name); \
        return HARD_FAIL; \
    } \
} while (0)

/*
 * Destroys the specified managed save frame, or emits a warning if it fails to do so
 */
#define DESTROY_FRAME(n, frame) do { \
    const char *_test_name = (n); \
    int _result; \
    fprintf(stderr, "%s: Destroying a managed save frame...\n", _test_name); \
    _result = cif_container_destroy(frame); \
    if (_result != CIF_OK) { \
        fprintf(stderr, "warning: %s: ... failed with code %d.\n", _test_name, _result); \
    } \
} while (0)

/*
 * Records a path to the test data directory.  Despite the term "resolve" in the macro name, this may still be a
 * relative path.  'dest' is a pointer to a char buffer into which the result should be written, and 'len' is the space
 * available in the buffer.  len must be greater than zero, else the behavior of this macro is undefined.  All the
 * storage from dest[0] to dest[len - 1] must belong to the same object, else the behavior of this macro is undefined.
 * There must be sufficient space for the full resolution, else an empty string is recorded.
 */
#define RESOLVE_DATADIR(dest, len) do { \
    const char *_cifapi_src = getenv("CIFAPI_SRC"); \
    char *_d = (dest); \
    ssize_t _l = (len); \
    if (!_cifapi_src) { _cifapi_src = "."; } \
    if (strlen(_cifapi_src) >= (_l - (strlen(DATA_DIR) + 2))) { _d[0] = 0; } \
    else { sprintf(_d, "%s/" DATA_DIR "/", _cifapi_src); } \
} while (0)

#endif
