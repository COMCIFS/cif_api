/*
 * test_parse_minimal.c
 *
 * Tests parsing various dataless CIFs, including the smallest possible
 * compliant CIF 2.0 document and a completely empty document (technically CIF 1).
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
#define NUM_FILES 5
#define TESTS_PER_FILE 6
int main(int argc, char *argv[]) {
    char test_name[80] = "test_parse_minimal";
    char local_file_name[NUM_FILES][17] = {
        "ver2.cif",
        "bom_ver2.cif",
        "empty.cif",
        "ver1.cif",
        "comment_only.cif"
    };
    char file_name[BUFFER_SIZE];
    cif_t *cif = NULL;
    cif_block_t **block_list;
    int count;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    for (count = 0; count < NUM_FILES; count += 1) {
        FILE * cif_file;

        /* construct the test file name and open the file */
        RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name[count]));
        TEST_NOT(file_name[0], 0, test_name, count * TESTS_PER_FILE + 1);
        strcat(file_name, local_file_name[count]);
        cif_file = fopen(file_name, "rb");
        TEST(cif_file == NULL, 0, test_name, count * TESTS_PER_FILE + 2);

        /* parse the file */
        TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, count * TESTS_PER_FILE + 3);

        /* close the file, ignoring any failure here */
        fclose(cif_file);

        /* check the parse result */
          /* check that there are no blocks */
        TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, count * TESTS_PER_FILE + 4);
        TEST(block_list == NULL, 0, test_name, count * TESTS_PER_FILE + 5);
        TEST_NOT(*block_list == NULL, 0, test_name, count * TESTS_PER_FILE + 6);
        free(block_list);
    }

    /* clean up */
    DESTROY_CIF(test_name, cif);

    return 0;
}
