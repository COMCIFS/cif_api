/*
 * test_parse_core.c
 *
 * Tests parsing the DDLm version of the CIF Core dictionary
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

static int error_callback(int code, size_t line, size_t column, const UChar *text UNUSED, size_t length UNUSED,
        void *data UNUSED) {
    fprintf(stderr, "error code %d at line %lu, column %lu\n", code, (unsigned long) line, (unsigned long) column);
    return code;
}


#define BUFFER_SIZE 512
#define NUM_ITEMS     3
int main(void) {
    char test_name[80] = "test_parse_core";
    char local_file_name[] = "cif_core.dic";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    struct cif_parse_opts_s options = {
        0,    /* do not default to CIF 2 */
        NULL, /* no specified default encoding */
        0,    /* do not force the default encoding */
        0,    /* unmodified line-folding rules */
        0,    /* unmodified text-prefixing rules */
        -1,   /* Allow unlimited save frame nesting */
        NULL, /* no CIF handler */
        NULL, /* no whitespace handler */
        error_callback, /* error callback function */
        NULL  /* no user data */
    };
    cif_tp *cif = NULL;
    U_STRING_DECL(code_block1,       "block1", 7);

    U_STRING_INIT(code_block1,       "block1", 7);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse(cif_file, &options, &cif), CIF_OK, test_name, 3);

    /* check the parse result */
      /* TODO */

    /* clean up */

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
