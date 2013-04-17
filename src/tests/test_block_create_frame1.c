/*
 * test_block_create_frame1.c
 *
 * Tests general function of the CIF API's cif_block_create_frame() function.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_block_create_frame1";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_block_t *block2 = NULL;
    cif_frame_t *frame = NULL;
    int result;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block2_code, "block2", 7);
    U_STRING_DECL(frame_code, "frame", 6);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block2_code, "block2", 7);
    U_STRING_INIT(frame_code, "frame", 6);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block2_code, block2);

    TEST(cif_block_get_frame(block, frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 1);
    TEST(frame, NULL, test_name, 2);
    TEST(cif_block_get_frame(block2, frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 3);
    TEST(cif_block_create_frame(block, frame_code, &frame), CIF_OK, test_name, 4);
    TEST_NOT(frame, NULL, test_name, 5);

    /* no mechanism for checking the frame code */

    /* not under test: */
    cif_frame_free(frame);
    frame = NULL;

    TEST(cif_block_get_frame(block, frame_code, &frame), CIF_OK, test_name, 6);
    TEST_NOT(frame, NULL, test_name, 7);

    /* not under test: */
    cif_frame_free(frame);
    frame = NULL;

    TEST(cif_block_get_frame(block2, frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 8);

    DESTROY_CIF(test_name, cif);

    return 0;
}

