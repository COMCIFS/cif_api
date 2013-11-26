/*
 * test_block_get_all_frames.c
 *
 * Tests some details of the CIF API's cif_block_get_all_frames() function
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"
#include "uthash.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_block_get_all_frames";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_block_t *block2 = NULL;
    cif_frame_t **frames = NULL;
    cif_frame_t *frame = NULL;
    UChar *code;
    U_STRING_DECL(block0, "b0", 3);
    U_STRING_DECL(b2, "b2", 3);
    U_STRING_DECL(frame0, "f0", 3);
    U_STRING_DECL(frame1, "f1", 3);
    U_STRING_DECL(frame2, "f2", 3);
    U_STRING_DECL(frame3, "f3", 3);
    UChar *codes[4];
    int subtest;

    U_STRING_INIT(block0, "b0", 3);
    U_STRING_INIT(b2, "b2", 3);
    U_STRING_INIT(frame0, "f0", 3);
    U_STRING_INIT(frame1, "f1", 3);
    U_STRING_INIT(frame2, "f2", 3);
    U_STRING_INIT(frame3, "f3", 3);
    codes[0] = frame0;
    codes[1] = frame1;
    codes[2] = frame2;
    codes[3] = NULL;

    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block0, block);
    CREATE_BLOCK(test_name, cif, b2, block2);

    /* Test when there are zero frames (subtests 1 - 3) */
    TEST(cif_block_get_all_frames(block, &frames), CIF_OK, test_name, 1);
    TEST((frames == NULL), 0, test_name, 2);
    TEST_NOT((frames[0] == NULL), 0, test_name, 3);
    free(frames);

    /* Test when there is exactly one frame (subtests 4 - 10) */
    TEST(cif_block_create_frame(block, frame0, NULL), CIF_OK, test_name, 4);
    TEST(cif_block_get_all_frames(block, &frames), CIF_OK, test_name, 5);
    TEST((frames == NULL), 0, test_name, 6);
    TEST((frames[0] == NULL), 0, test_name, 7);
    TEST_NOT((frames[1] == NULL), 0, test_name, 8);
    TEST(cif_container_get_code(frames[0], &code), CIF_OK, test_name, 9);
    TEST(u_strcmp(frame0, code), 0, test_name, 10);

    free(code);
    cif_container_free(frames[0]);
    free(frames);

    /* Test when there are multiple frames, in multiple blocks (subtests 11 - ?) */
    TEST(cif_block_create_frame(block2, frame1, NULL), CIF_OK, test_name, 11);
    TEST(cif_block_create_frame(block, frame1, NULL), CIF_OK, test_name, 12);
    TEST(cif_block_create_frame(block, frame2, NULL), CIF_OK, test_name, 13);
    TEST(cif_block_create_frame(block2, frame3, NULL), CIF_OK, test_name, 14);
    TEST(cif_block_get_all_frames(block, &frames), CIF_OK, test_name, 15);
    TEST((frames == NULL), 0, test_name, 16);

    {
        struct set_el *head = NULL;
        int counter;

        /* Build a hash table of the expected frame codes */
        subtest = 17;
        for (counter = 0; codes[counter] != NULL; counter += 1) {
            struct set_el *el = (struct set_el *) malloc(sizeof(struct set_el));

            if (el == NULL) return HARD_FAIL;
            HASH_ADD_KEYPTR(hh, head, codes[counter], u_strlen(codes[counter]) * sizeof(UChar), el);
        }

        /* Match each block to a block code; each code may be matched at most once */
        for (counter = 0; frames[counter] != NULL; counter += 1) {
            UChar *code = NULL;
            struct set_el *el = NULL;

            TEST(cif_container_get_code(frames[counter], &code), CIF_OK, test_name, subtest++);
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

