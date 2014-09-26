/*
 * test_multiple_cifs.c
 *
 * Tests to verify that CIFs created independently do not alias each other.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_multiple_cifs";
    cif_t *cif1 = NULL;
    cif_t *cif2 = NULL;
    U_STRING_DECL(block_code, "block", 6);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);

    /* create two distinct CIFs */
    TEST(cif_create(&cif1), CIF_OK, test_name, 1);
    TEST(cif_create(&cif2), CIF_OK, test_name, 2);

    /* verify their distinction */
    TEST(cif_create_block(cif1, block_code, NULL), CIF_OK, test_name, 3);
    TEST(cif_get_block(cif1, block_code, NULL), CIF_OK, test_name, 4);
    TEST(cif_get_block(cif2, block_code, NULL), CIF_NOSUCH_BLOCK, test_name, 5);
    TEST(cif_create_block(cif2, block_code, NULL), CIF_OK, test_name, 6);
    TEST(cif_get_block(cif2, block_code, NULL), CIF_OK, test_name, 7);
    TEST(cif_destroy(cif1), CIF_OK, test_name, 8);
    TEST(cif_get_block(cif2, block_code, NULL), CIF_OK, test_name, 9);
    TEST(cif_destroy(cif2), CIF_OK, test_name, 10);

    return 0;
}

