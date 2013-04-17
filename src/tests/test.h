/*
 * test.h
 *
 * Macros for CIF test fixture setup and teardown.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"

/*
 * Standardized result codes.  Any other return code represents a normal failure.
 */
#define SUCCESS    0
#define SKIP      77
#define HARD_FAIL 99

#define TESTHEADER(name) do { \
  fprintf(stderr, "\n-- %s --\n", (name)); \
} while (0)

/*
 * Emits a failure message to stderr and returns the specified code.
 */
#define FAIL(ret, name, code, sense, compare) do { \
  int _ret = (ret); \
  fprintf(stderr, "%s(%d): ... failed with code %d " sense " %d.\n", (name), _ret, (code), (compare)); \
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


/*
 * Creates a new managed cif, recording a pointer to it in cif, which must therefore be an lvalue.
 * Generates a hard failure if unsuccessful.
 */
#define CREATE_CIF(n, cif) do { \
    const char *_test_name = (n); \
    int _result; \
    fprintf(stdout, "%s: Creating a managed CIF...\n", _test_name); \
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
    fprintf(stdout, "%s: Destroying a managed CIF...\n", _test_name); \
    _result = cif_destroy(cif); \
    if (result != CIF_OK) { \
        fprintf(stderr, "warning: %s: ... failed with code %d.\n", _test_name, _result); \
    } \
} while (0)

