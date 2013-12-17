/*
 * test_loop_get_names.c
 *
 * Tests behaviors of the CIF API's cif_loop_get_names() function that are not already tested in the
 * loop creation tests.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
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
int main(int argc, char *argv[]) {
    char test_name[80] = "test_loop_get_names";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_loop_t *loop1;
    cif_loop_t *loop2;
    U_STRING_DECL(block_code, "block1", 7);
    U_STRING_DECL(item1, "_item1", 7);
    UChar *items[NUM_ITEMS + 1];
    UChar **names;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block1", 7);
    U_STRING_INIT(item1, "_item1", 7);

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
    TEST(cif_loop_get_names(loop1, *names), CIF_INVALID_HANDLE, test_name, 8);
    TEST(cif_loop_free(loop1), CIF_OK, test_name, 9);
    free(names);

    /* Final cleanup */
    DESTROY_CIF(test_name, cif);

    return 0;
}

