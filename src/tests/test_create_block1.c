/*
 * test_create_block1.c
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
    UChar *code;

    U_STRING_INIT(block_code, "block", 5);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    TEST(cif_get_block(cif, block_code, &block), CIF_NOSUCH_BLOCK, test_name, 1);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 2);
    TEST_NOT(block, NULL, test_name, 3);

    TEST(cif_container_get_code(block, &code), CIF_OK, test_name, 4);
    TEST(u_strcmp(block_code, code), 0, test_name, 5);

    /* not under test: */
    cif_block_free(block);
    block = NULL;

    TEST(cif_get_block(cif, block_code, &block), CIF_OK, test_name, 6);
    TEST_NOT(block, NULL, test_name, 7);
    cif_block_free(block);

    DESTROY_CIF(test_name, cif);

    return 0;
}

