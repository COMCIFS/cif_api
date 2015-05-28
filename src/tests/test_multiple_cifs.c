/*
 * test_multiple_cifs.c
 *
 * Tests to verify that CIFs created independently do not alias each other.
 *
 * Copyright 2014, 2015 John C. Bollinger
 *
 *
 * This file is part of the CIF API.
 *
 * The CIF API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CIF API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_multiple_cifs";
    cif_tp *cif1 = NULL;
    cif_tp *cif2 = NULL;
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

