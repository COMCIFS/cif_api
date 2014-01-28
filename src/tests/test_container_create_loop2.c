/*
 * test_container_create_loop2.c
 *
 * Tests general function of the CIF API's cif_container_create_loop() function.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_container_create_loop2";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_loop_t *loop = NULL;
    cif_block_t *block2;
#define NUM_NAMES 6
#define BUFLEN 64
    const char invalid_patterns[NUM_NAMES][BUFLEN] = {
        "no_leading_underscore",
        "_contains space",
        "_contains\\xanewline",
        "_contains_not_a_char_\\x10fffe",
        "_contains_\\udb17_unpaired_high_surrogate",
        "_contains_\\udead_unpaired_low_surrogate"
    };
    UChar *invalid_names[NUM_NAMES + 1];
    UChar *nonames = NULL;
    UChar *item_names[6];
    int counter;
    int subtest;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(name0, "_item0", 7);
    U_STRING_DECL(name1, "_item1", 7);
    U_STRING_DECL(name2, "_item2", 7);

    /* Initialize data and prepare the test fixture */

    INIT_USTDERR;
    TESTHEADER(test_name);
    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(name0, "_item0", 7);
    U_STRING_INIT(name1, "_item1", 7);
    U_STRING_INIT(name2, "_item2", 7);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);

    for (counter = 0; counter < NUM_NAMES; counter++) {
        invalid_names[counter] = (UChar *) malloc(BUFLEN * sizeof(UChar));
        u_unescape(invalid_patterns[counter], invalid_names[counter], BUFLEN - 1);
        *(invalid_names[counter] + (BUFLEN - 1)) = 0;
    }

    invalid_names[NUM_NAMES] = (UChar *) malloc((CIF_LINE_LENGTH + 2) * sizeof(UChar));
    if (invalid_names[NUM_NAMES] == NULL) return HARD_FAIL;

    for (counter = 0; counter <= CIF_LINE_LENGTH; counter++) {
        *(invalid_names[NUM_NAMES] + counter) = (UChar) '_';
    }
    *(invalid_names[NUM_NAMES] + counter) = 0;

    item_names[0] = name0;
    item_names[1] = name1;
    item_names[2] = name2;
    item_names[3] = NULL;
    item_names[4] = NULL;
    item_names[5] = NULL;

    TEST(cif_container_create_loop(block, NULL, &nonames, NULL), CIF_NULL_LOOP, test_name, 1);

    subtest = 2;

    /* subtests 2 - 19: test invalid item names of various types in various positions */
    for (counter = 0; counter <= NUM_NAMES; counter++) {
        item_names[0] = invalid_names[counter];
        TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_INVALID_ITEMNAME, test_name, subtest++);
        item_names[0] = name0;

        item_names[1] = invalid_names[counter];
        TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_INVALID_ITEMNAME, test_name, subtest++);
        item_names[1] = name1;

        item_names[2] = invalid_names[counter];
        TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_INVALID_ITEMNAME, test_name, subtest++);
        item_names[2] = name2;
    }

    for (counter = 0; counter <= NUM_NAMES; counter++) {
        free(invalid_names[counter]);
    }

    /* subtests 23 - 40: duplicating an item name that is already present in the container */
    for (counter = 0; counter < 3; counter++) {
        int counter2;

        item_names[4] = item_names[counter];

        /* verify the test item not already present */
        TEST(cif_container_get_item_loop(block, item_names[4], NULL), CIF_NOSUCH_ITEM, test_name, subtest++);

        /* put the item to dupe into the block.  no data are added for it; just being present should be sufficient.*/
        TEST(cif_container_create_loop(block, NULL, item_names + 4, &loop), CIF_OK, test_name, subtest++);

        /* try to create a loop with a duplicate item name */
        TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_DUP_ITEMNAME, test_name, subtest++);

        for (counter2 = 0; counter2 < 3; counter2++) {
            if (counter2 != counter)
                TEST(cif_container_get_item_loop(block, item_names[counter2], NULL), CIF_NOSUCH_ITEM, test_name, subtest++);
        }

        /* clean up */
        TEST(cif_loop_destroy(loop), CIF_OK, test_name, subtest++);
    }

    /* subtest 41: duplicate item names in the same (requested) loop */
    item_names[3] = name2;
    item_names[4] = NULL;
    TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_DUP_ITEMNAME, test_name, subtest++);

    /* subtests 42 - 43: duplicate scalar loops */
    item_names[1] = NULL;
    item_names[3] = NULL;
    TEST(cif_container_create_loop(block, CIF_SCALARS, item_names, NULL), CIF_OK, test_name, subtest++);
    TEST(cif_container_create_loop(block, CIF_SCALARS, item_names + 2, NULL), CIF_RESERVED_LOOP, test_name, subtest++);


    /* subtests 44 - 46: invalid container handle */
    TEST(cif_get_block(cif, block_code, &block2), CIF_OK, test_name, subtest++);
    TEST(cif_block_destroy(block), CIF_OK, test_name, subtest++);
    TEST(cif_container_create_loop(block2, NULL, item_names, NULL), CIF_INVALID_HANDLE, test_name, subtest++);

    cif_container_free(block2);

    DESTROY_CIF(test_name, cif);

    return 0;
}

