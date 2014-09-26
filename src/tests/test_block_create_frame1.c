/*
 * test_block_create_frame1.c
 *
 * Tests general function of the CIF API's cif_block_create_frame() function.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_block_create_frame1";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_block_t *block2 = NULL;
    cif_frame_t *frame = NULL;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block2_code, "block2", 7);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(alt_frame_code, "fRaME", 6);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block2_code, "block2", 7);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(alt_frame_code, "fRaME", 6);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block2_code, block2);

    /* Verify the result when the requested frame is absent */
    TEST(cif_block_get_frame(block, frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 1);
    TEST(frame != NULL, 0, test_name, 2);

    /* Verify that the test frame is absent from the other block, too */
    TEST(cif_block_get_frame(block2, frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 3);

    /* Verify that the alternative frame code is absent as well */
    TEST(cif_block_get_frame(block, alt_frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 4);

    /* Test creating the frame in one block */
    TEST(cif_block_create_frame(block, frame_code, &frame), CIF_OK, test_name, 5);
    TEST(frame == NULL, 0, test_name, 6);

    /* no mechanism for checking the frame code */

    /* not under test: */
    cif_frame_free(frame);
    frame = NULL;

    /* test retrieving the frame */
    TEST(cif_block_get_frame(block, frame_code, &frame), CIF_OK, test_name, 7);
    TEST(frame == NULL, 0, test_name, 8);

    /* not under test: */
    cif_frame_free(frame);
    frame = NULL;

    /* Verify that the frame was added only to the specified block */
    TEST(cif_block_get_frame(block2, frame_code, &frame), CIF_NOSUCH_FRAME, test_name, 9);

    /* Verify that the frame can be retrieved by an alternative, equivalent frame code */
    TEST(cif_block_get_frame(block, alt_frame_code, &frame), CIF_OK, test_name, 10);
    TEST(frame == NULL, 0, test_name, 11);

    /* Test creating a frame whose frame code is the same as the host block's block code */
    TEST(cif_block_create_frame(block, block_code, NULL), CIF_OK, test_name, 12);

    cif_frame_free(frame);
    DESTROY_BLOCK(test_name, block2);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

