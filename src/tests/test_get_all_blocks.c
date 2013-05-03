/*
 * test_get_all_blocks.c
 *
 * Tests some details of the CIF API's cif_get_all_blocks() function
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"
#include "uthash.h"

struct set_el {
    UT_hash_handle hh;
};

int main(int argc, char *argv[]) {
    char test_name[80] = "test_get_all_blocks";
    cif_t *cif = NULL;
    cif_block_t **blocks = NULL;
    cif_block_t *block = NULL;
    UChar *code;
    U_STRING_DECL(block0, "b0", 2);
    U_STRING_DECL(block1, "b1", 2);
    U_STRING_DECL(block2, "b2", 2);
    U_STRING_DECL(frame1, "f1", 2);
    U_STRING_DECL(frame2, "f2", 2);
    UChar *codes[4];
    int subtest;

    U_STRING_INIT(block0, "b0", 2);
    U_STRING_INIT(block1, "b1", 2);
    U_STRING_INIT(block2, "b2", 2);
    U_STRING_INIT(frame1, "f1", 2);
    U_STRING_INIT(frame2, "f2", 2);
    codes[0] = block0;
    codes[1] = block1;
    codes[2] = block2;
    codes[3] = NULL;

    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    /* Test when there are zero blocks (subtests 1 - 3) */
    TEST(cif_get_all_blocks(cif, &blocks), CIF_OK, test_name, 1);
    TEST((blocks == NULL), 0, test_name, 2);
    TEST_NOT((blocks[0] == NULL), 0, test_name, 3);
    free(blocks);

    /* Test when there is exactly one block (subtests 4 - 10) */
    TEST(cif_create_block(cif, block0, &block), CIF_OK, test_name, 4);
    TEST(cif_get_all_blocks(cif, &blocks), CIF_OK, test_name, 5);
    TEST((blocks == NULL), 0, test_name, 6);
    TEST((blocks[0] == NULL), 0, test_name, 7);
    TEST_NOT((blocks[1] == NULL), 0, test_name, 8);
    TEST(cif_container_get_code(blocks[0], &code), CIF_OK, test_name, 9);
    TEST(u_strcmp(block0, code), 0, test_name, 10);

    free(code);
    cif_container_free(blocks[0]);
    free(blocks);

    /* Test when there are multiple blocks (subtests 11 - ?) */
    TEST(cif_block_create_frame(block, frame1, NULL), CIF_OK, test_name, 11);
    cif_container_free(block);
    TEST(cif_create_block(cif, block1, NULL), CIF_OK, test_name, 12);
    TEST(cif_create_block(cif, block2, &block), CIF_OK, test_name, 13);
    TEST(cif_block_create_frame(block, frame2, NULL), CIF_OK, test_name, 14);
    cif_container_free(block);
    TEST(cif_get_all_blocks(cif, &blocks), CIF_OK, test_name, 15);
    TEST((blocks == NULL), 0, test_name, 16);

    {
        struct set_el *head = NULL;
        int counter;

        /* Build a hash table of the expected block codes */
        subtest = 17;
        for (counter = 0; codes[counter] != NULL; counter += 1) {
            struct set_el *el = (struct set_el *) malloc(sizeof(struct set_el));

            if (el == NULL) return HARD_FAIL;
            HASH_ADD_KEYPTR(hh, head, codes[counter], u_strlen(codes[counter]) * sizeof(UChar), el);
        }

        /* Match each block to a block code; each code may be matched at most once */
        for (counter = 0; blocks[counter] != NULL; counter += 1) {
            UChar *code = NULL;
            struct set_el *el = NULL;

            TEST(cif_container_get_code(blocks[counter], &code), CIF_OK, test_name, subtest++);
            HASH_FIND(hh, head, code, u_strlen(code) * sizeof(UChar), el);
            TEST((el == NULL), 0, test_name, subtest++);
            HASH_DEL(head, el);
            free(code);
            free(el);
        }

        /* Make sure there are no unmatched block codes */
        TEST(HASH_COUNT(head), 0, test_name, subtest++);
    }
    
    DESTROY_CIF(test_name, cif);

    return 0;
}

