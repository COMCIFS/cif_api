/*
 * test_container_assert_block.c
 *
 * Tests general function of the CIF API's cif_create_block() and
 * cif_get_block() functions.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_container_assert_block";
    cif_t *cif = NULL;
    cif_container_t *block = NULL;
    cif_container_t *frame = NULL;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 1);
    TEST(cif_block_create_frame(block, frame_code, &frame), CIF_OK, test_name, 2);
    TEST(cif_container_assert_block(NULL), CIF_ERROR, test_name, 3);
    TEST(cif_container_assert_block(block), CIF_OK, test_name, 4);
    TEST(cif_container_assert_block(frame), CIF_ARGUMENT_ERROR, test_name, 5);


    /* clean up */
    cif_container_free(frame);
    frame = NULL;
    cif_container_free(block);
    block = NULL;

    DESTROY_CIF(test_name, cif);

    return 0;
}

