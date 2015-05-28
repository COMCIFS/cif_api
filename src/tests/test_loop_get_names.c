/*
 * test_loop_get_names.c
 *
 * Tests behaviors of the CIF API's cif_loop_get_names() function that are not already tested in the
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
#include <unicode/ustdio.h>
#include "../cif.h"
#include "test.h"

/*
 * test invalid loop
 * test NULL item_names
 */

#define NAME_UCHARS 8
#define NUM_ITEMS 1
#define EXPECTED_LOOPS 4
int main(void) {
    char test_name[80] = "test_loop_get_names";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_loop_tp *loop1;
    cif_loop_tp *loop2;
    U_STRING_DECL(block_code, "block1", 7);
    UChar item1[] = { '_', 'i', 't', 'e', 'm', '1', 0 };
    UChar *items[NUM_ITEMS + 1];
    UChar **names;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block1", 7);

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);

    /*
     * The loops we will test retrieving:
     */

    items[0] = item1;
    items[1] = NULL;

    /* create the test loop and record a handle in loop1 */
    TEST(cif_container_create_loop(block, NULL, items, &loop1), CIF_OK, test_name, 1);

    /* retrieve a second handle on the same loop as loop2 */
    TEST(cif_container_get_item_loop(block, item1, &loop2), CIF_OK, test_name, 2);

    /* verify loop1's data names */
    TEST(cif_loop_get_names(loop1, &names), CIF_OK, test_name, 3);
    TEST((*names == NULL), 0, test_name, 4);
    TEST((*(names + 1) != NULL), 0, test_name, 5);
    TEST(u_strcmp(item1, *names), 0, test_name, 6);
    free(*names);

    /* destroy the loop via handle loop2 */
    TEST(cif_loop_destroy(loop2), CIF_OK, test_name, 7);

    /* attempt to retrieve the data names again */
    TEST(cif_loop_get_names(loop1, &names), CIF_INVALID_HANDLE, test_name, 8);
    cif_loop_free(loop1);
    free(names);

    /* Final cleanup */
    DESTROY_CIF(test_name, cif);

    return 0;
}

