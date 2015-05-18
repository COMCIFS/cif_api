/*
 * test_nesting.c
 *
 * Tests storing and retrieving nested composite CIF values.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_nesting";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_value_tp *value;
    cif_value_tp *value1;
    cif_value_tp *value2;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(item1l, "_item1", 7);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(item1l, "_item1", 7);

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);

    /* Prepare a list value with nested list elements */
    TEST(cif_value_create(CIF_LIST_KIND, &value1), CIF_OK, test_name, 1);
    TEST(cif_value_create(CIF_LIST_KIND, &value2), CIF_OK, test_name, 2);
    TEST(cif_value_insert_element_at(value1, 0, value2), CIF_OK, test_name, 3);
    TEST(cif_value_insert_element_at(value1, 1, value2), CIF_OK, test_name, 4);
    TEST(cif_value_copy_char(value2, item1l), CIF_OK, test_name, 5);
    TEST(cif_value_insert_element_at(value1, 2, value2), CIF_OK, test_name, 6);
    TEST(cif_value_init(value2, CIF_LIST_KIND), CIF_OK, test_name, 7);
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 8);
    TEST(cif_value_insert_element_at(value2, 0, value), CIF_OK, test_name, 9);
    TEST(cif_value_copy_char(value, item1l), CIF_OK, test_name, 10);
    TEST(cif_value_insert_element_at(value2, 1, value), CIF_OK, test_name, 11);
    TEST(cif_value_autoinit_numb(value, 2.0, 0.0, 19), CIF_OK, test_name, 12);
    TEST(cif_value_insert_element_at(value2, 2, value), CIF_OK, test_name, 13);
    TEST(cif_value_insert_element_at(value1, 3, value2), CIF_OK, test_name, 14);
    cif_value_free(value);
    value = NULL;
    cif_value_free(value2);
    value2 = NULL;

    /* Test recording and re-reading the value */
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 15);
    TEST(cif_container_get_value(block, item1l, &value2), CIF_OK, test_name, 16);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 17);
    cif_value_free(value2);
    value2 = NULL;

    /* Prepare a list value with nested table elements */
    TEST(cif_value_get_element_at(value1, 1, &value2), CIF_OK, test_name, 18);
    TEST(cif_value_init(value2, CIF_TABLE_KIND), CIF_OK, test_name, 19);
    TEST(cif_value_get_element_at(value1, 2, &value2), CIF_OK, test_name, 20);
    TEST(cif_value_init(value2, CIF_TABLE_KIND), CIF_OK, test_name, 21);
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 22);
    TEST(cif_value_set_item_by_key(value2, CIF_SCALARS, value), CIF_OK, test_name, 23);
    TEST(cif_value_autoinit_numb(value, 17.0, 2.5, 19), CIF_OK, test_name, 24);
    TEST(cif_value_set_item_by_key(value2, item1l, value), CIF_OK, test_name, 25);
    cif_value_free(value);
    value = NULL;
    /* value2 belongs to value1 */
    value2 = NULL;

    /* Test recording and re-reading the value */
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 26);
    TEST(cif_container_get_value(block, item1l, &value2), CIF_OK, test_name, 27);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 28);
    cif_value_free(value2);
    value2 = NULL;

    /* Prepare a table value with nested list elements */
    TEST(cif_value_init(value1, CIF_TABLE_KIND), CIF_OK, test_name, 29);
    TEST(cif_value_set_item_by_key(value1, CIF_SCALARS, NULL), CIF_OK, test_name, 30);
    TEST(cif_value_get_item_by_key(value1, CIF_SCALARS, &value2), CIF_OK, test_name, 31);
    TEST(cif_value_init(value2, CIF_LIST_KIND), CIF_OK, test_name, 32);
    TEST(cif_value_set_item_by_key(value1, item1l, NULL), CIF_OK, test_name, 33);
    TEST(cif_value_get_item_by_key(value1, item1l, &value2), CIF_OK, test_name, 34);
    TEST(cif_value_init(value2, CIF_LIST_KIND), CIF_OK, test_name, 35);
    TEST(cif_value_insert_element_at(value2, 0, NULL), CIF_OK, test_name, 36);
    TEST(cif_value_insert_element_at(value2, 1, NULL), CIF_OK, test_name, 37);
    TEST(cif_value_get_element_at(value2, 1, &value), CIF_OK, test_name, 38);
    TEST(cif_value_copy_char(value, CIF_SCALARS), CIF_OK, test_name, 39);
    TEST(cif_value_insert_element_at(value2, 2, NULL), CIF_OK, test_name, 40);
    TEST(cif_value_get_element_at(value2, 2, &value), CIF_OK, test_name, 41);
    TEST(cif_value_autoinit_numb(value, -1.0, 0.5, 19), CIF_OK, test_name, 42);
    value2 = NULL;
    value = NULL;

    /* Test recording and re-reading the value */
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 43);
    TEST(cif_container_get_value(block, item1l, &value2), CIF_OK, test_name, 44);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 45);
    cif_value_free(value2);
    value2 = NULL;

    /* Prepare a table value with nested table elements */
    TEST(cif_value_get_item_by_key(value1, CIF_SCALARS, &value2), CIF_OK, test_name, 46);
    TEST(cif_value_init(value2, CIF_TABLE_KIND), CIF_OK, test_name, 47);
    TEST(cif_value_set_item_by_key(value2, CIF_SCALARS, NULL), CIF_OK, test_name, 48);
    TEST(cif_value_set_item_by_key(value2, item1l, NULL), CIF_OK, test_name, 49);
    TEST(cif_value_get_item_by_key(value2, item1l, &value), CIF_OK, test_name, 50);
    TEST(cif_value_autoinit_numb(value, 17.0, 0.5, 19), CIF_OK, test_name, 51);
    value2 = NULL;
    value = NULL;

    /* Test recording and re-reading the value */
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 52);
    TEST(cif_container_get_value(block, item1l, &value2), CIF_OK, test_name, 53);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 54);
    cif_value_free(value2);
    value2 = NULL;

    /* clean up */
    cif_value_free(value1);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

