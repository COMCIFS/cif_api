/*
 * test_write_complex.c
 *
 * Tests writing complex CIF 2.0 data.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"
#include "assert_cifs.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     8
int main(int argc, char *argv[]) {
    char test_name[80] = "test_write_complex";
    FILE * cif_file;
    cif_t *cif = NULL;
    cif_t *cif_readback = NULL;
    cif_block_t *block = NULL;
    cif_value_t *value = NULL;
    cif_value_t *value2 = NULL;
    cif_value_t *value3 = NULL;
    UChar name_list1[]  = { '_', 'l', 'i', 's', 't', '1', 0 };
    UChar name_list2[]  = { '_', 'l', 'i', 's', 't', '2', 0 };
    UChar name_list3[]  = { '_', 'l', 'i', 's', 't', '3', 0 };
    UChar name_table1[] = { '_', 't', 'a', 'b', 'l', 'e', '1', 0 };
    UChar name_table2[] = { '_', 't', 'a', 'b', 'l', 'e', '2', 0 };
    UChar name_table3[] = { '_', 't', 'a', 'b', 'l', 'e', '3', 0 };
    UChar value_empty[] = { 0 };
    UChar value_blank[] = { ' ', '\t', ' ', 0 };
    UChar value_text[]  = { '"', '"', '"', ' ', 'a', 'n', 'd', ' ', '\'', '\'', '\'', '?', 0x0a,
            'O', 'o', 'p', 's', '.', 0 };
    UChar key1[]        = { 'k', 'e', 'y', 0 };
    UChar key2[]        = { '\'', '\'', '\'', 0x0a, 0x0a, ';', 0 };
    U_STRING_DECL(block_code,           "complex_data", 13);

    U_STRING_INIT(block_code,           "complex_data", 13);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* Create the temp file */
    cif_file = tmpfile();
    /* cif_file = fopen("../write_complex.cif", "wb+"); */
    TEST(cif_file == NULL, 0, test_name, 1);

    /* Build the CIF data to test on */
    TEST(cif_create(&cif), CIF_OK, test_name, 2);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 3);

    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 4);
    TEST(cif_container_set_value(block, name_list1, value), CIF_OK, test_name, 5);

    /* Note: inserts a _copy_ of 'value' into 'value' */
    TEST(cif_value_insert_element_at(value, 0, value), CIF_OK, test_name, 6);

    TEST(cif_value_create(CIF_UNK_KIND, &value2), CIF_OK, test_name, 7);
    TEST(cif_value_copy_char(value2, value_empty), CIF_OK, test_name, 8);
    TEST(cif_value_insert_element_at(value, 1, value2), CIF_OK, test_name, 9);
    TEST(cif_value_copy_char(value2, value_text), CIF_OK, test_name, 10);
    TEST(cif_value_insert_element_at(value, 2, value2), CIF_OK, test_name, 11);
    TEST(cif_value_autoinit_numb(value2, 13.1, 0.0625, 19), CIF_OK, test_name, 12);
    TEST(cif_value_insert_element_at(value, 3, value2), CIF_OK, test_name, 13);
    TEST(cif_container_set_value(block, name_list2, value), CIF_OK, test_name, 14);

    TEST(cif_value_create(CIF_TABLE_KIND, &value3), CIF_OK, test_name, 15);
    TEST(cif_container_set_value(block, name_table1, value3), CIF_OK, test_name, 16);

    TEST(cif_value_set_item_by_key(value3, value_empty, value3), CIF_OK, test_name, 17);
    TEST(cif_value_set_item_by_key(value3, value_blank, NULL), CIF_OK, test_name, 18);
    TEST(cif_value_set_item_by_key(value3, key2, value2), CIF_OK, test_name, 19);
    TEST(cif_container_set_value(block, name_table2, value3), CIF_OK, test_name, 20);

    TEST(cif_value_set_item_by_key(value3, key1, value), CIF_OK, test_name, 21);
    TEST(cif_value_set_item_by_key(value3, value_blank, value3), CIF_OK, test_name, 22);
    TEST(cif_container_set_value(block, name_table3, value3), CIF_OK, test_name, 23);

    TEST(cif_value_set_element_at(value, 1, value3), CIF_OK, test_name, 24);
    TEST(cif_value_copy_char(value2, name_list3), CIF_OK, test_name, 25);
    TEST(cif_value_insert_element_at(value, 0, value2), CIF_OK, test_name, 26);
    TEST(cif_container_set_value(block, name_list3, value), CIF_OK, test_name, 27);

    cif_value_free(value);
    cif_value_free(value2);
    cif_value_free(value3);
    cif_container_free(block);

    /* write to the temp file */
    TEST(cif_write(cif_file, NULL, cif), CIF_OK, test_name, 28);
    fflush(cif_file);

    /* parse the file */
    rewind(cif_file);
    TEST(cif_parse(cif_file, NULL, &cif_readback), CIF_OK, test_name, 29);

    /* make sure everything matches */
    TEST_NOT(assert_cifs_equal(cif, cif_readback), 0, test_name, 30);

    /* clean up */
    TEST(cif_destroy(cif_readback), CIF_OK, test_name, 31);
    TEST(cif_destroy(cif), CIF_OK, test_name, 32);
    fclose(cif_file); /* ignore any error here */

    return 0;
}
