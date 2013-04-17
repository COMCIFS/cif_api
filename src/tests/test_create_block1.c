/*
 * test_create_block.c
 *
 * Tests general function of the CIF API's cif_create_block() and
 * cif_get_block() functions.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_create_block1";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    int result;
    U_STRING_DECL(block_code, "block", 5);

    U_STRING_INIT(block_code, "block", 5);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    TEST(cif_get_block(cif, block_code, &block), CIF_NOSUCH_BLOCK, test_name, 1);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 2);
    TEST_NOT(block, NULL, test_name, 3);

    /* no mechanism for checking the block name */

    /* not under test: */
    cif_block_free(block);
    block = NULL;

    TEST(cif_get_block(cif, block_code, &block), CIF_OK, test_name, 4);
    TEST_NOT(block, NULL, test_name, 4);

    DESTROY_CIF(test_name, cif);

    return 0;
}

