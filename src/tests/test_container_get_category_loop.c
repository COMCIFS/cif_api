/*
 * test_container_get_category_loop.c
 *
 * Tests behaviors of the CIF API's cif_container_get_category_loop() function that are not already tested in the
 * loop creation tests.
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
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_container_get_category_loop";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_block_tp *block2 = NULL;
    cif_loop_tp *loop = NULL;
    UChar *item_names[4] = { NULL, NULL, NULL, NULL };
    UChar **test_names;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block2_code, "block2", 7);
    U_STRING_DECL(category, "category", 9);
    U_STRING_DECL(category2, "category two", 13);
    UChar item1[] = { '_', 'i', 't', 'e', 'm', '1', 0 };
    UChar item2[] = { '_', 'i', 't', 'e', 'm', '2', 0 };
    UChar item3[] = { '_', 'i', 't', 'e', 'm', '3', 0 };

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);
    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block2_code, "block2", 7);
    U_STRING_INIT(category, "category", 9);
    U_STRING_INIT(category2, "category two", 13);

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block2_code, block2);

    /* Test when the requested category is NULL */
    item_names[0] = item1;
    TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_OK, test_name, 1);
    TEST(cif_container_get_category_loop(block, NULL, NULL), CIF_INVALID_CATEGORY, test_name, 2);

    /* Test when the requested category is present in multiple containers */
    item_names[1] = item2;
    item_names[2] = item3;
    TEST(cif_container_create_loop(block2, category, item_names, NULL), CIF_OK, test_name, 3);
    item_names[0] = item2;
    item_names[1] = NULL;
    TEST(cif_container_create_loop(block, category, item_names, NULL), CIF_OK, test_name, 4);
    TEST(cif_container_get_category_loop(block, category, &loop), CIF_OK, test_name, 5);
    TEST(cif_loop_get_names(loop, &test_names), CIF_OK, test_name, 6);
    cif_loop_free(loop);
    TEST((test_names == NULL), 0, test_name, 7);
    TEST((test_names[0] == NULL), 0, test_name, 8);
    TEST((test_names[1] != NULL), 0, test_name, 9);
    TEST(u_strcmp(item_names[0], test_names[0]), 0, test_name, 10);
    free(*test_names);
    free(test_names);

    /* Test when the requested category is present multiple times in the same container */
    item_names[0] = item3;
    TEST(cif_container_create_loop(block, category, item_names, NULL), CIF_OK, test_name, 11);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_CAT_NOT_UNIQUE, test_name, 12);

    /* Test a category name that is not present */
    TEST(cif_container_get_category_loop(block, category2, NULL), CIF_NOSUCH_LOOP, test_name, 13);

    DESTROY_BLOCK(test_name, block2);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

