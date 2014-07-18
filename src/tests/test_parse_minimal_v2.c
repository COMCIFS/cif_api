/*
 * test_parse_minimal_v2.c
 *
 * Tests parsing the smallest possible compliant CIF 2.0 document.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"
#include "assert_value.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     3
int main(int argc, char *argv[]) {
    char test_name[80] = "test_parse_minimal_v2";
    char local_file_name[] = "ver2.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_t *cif = NULL;
    cif_block_t **block_list;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, 3);

    /* check the parse result */
      /* check that there are no blocks */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 4);
    TEST(block_list == NULL, 0, test_name, 5);
    TEST_NOT(*block_list == NULL, 0, test_name, 6);
    free(block_list);

    /* clean up */
    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
