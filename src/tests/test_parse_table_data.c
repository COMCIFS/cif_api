/*
 * test_parse_table_data.c
 *
 * Tests parsing simple CIF 2.0 table data.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     9
int main(void) {
    char test_name[80] = "test_parse_table_data";
    char local_file_name[] = "table_data.cif";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_block_t **block_list;
    cif_loop_t **loop_list;
    cif_loop_t *loop;
    cif_value_t *value = NULL;
    cif_value_t *element = NULL;
    UChar **name_list;
    UChar **name_p;
    UChar *ustr;
    double d;
    size_t count;

    U_STRING_DECL(block_code,           "table_data", 10);
    U_STRING_DECL(name_empty_table1,    "_empty_table1", 14);
    U_STRING_DECL(name_empty_table2,    "_empty_table2", 14);
    U_STRING_DECL(name_empty_table3,    "_empty_table3", 14);
    U_STRING_DECL(name_singleton_table1,"_singleton_table1", 18);
    U_STRING_DECL(name_singleton_table2,"_singleton_table2", 18);
    U_STRING_DECL(name_singleton_table3,"_singleton_table3", 18);
    U_STRING_DECL(name_digit3_map,      "_digit3_map", 12);
    U_STRING_DECL(name_space_keys,      "_space_keys", 12);
    U_STRING_DECL(name_type_examples,   "_type_examples", 15);
    U_STRING_DECL(key_zero,             "zero", 5);
    U_STRING_DECL(key_one,              "one", 4);
    U_STRING_DECL(key_two,              "two", 4);
    U_STRING_DECL(key_text,             "text", 5);
    U_STRING_DECL(key_0bl,              "", 1);
    U_STRING_DECL(key_1bl,              " ", 2);
    U_STRING_DECL(key_3bl,              "   ", 4);
    U_STRING_DECL(key_char,             "char", 5);
    U_STRING_DECL(key_unknown,          "unknown", 8);
    U_STRING_DECL(key_na,               "N/A", 4);
    U_STRING_DECL(key_numb,             "numb", 5);
    U_STRING_DECL(value_empty_key,      "empty_key", 10);

    U_STRING_INIT(block_code,           "table_data", 10);
    U_STRING_INIT(name_empty_table1,    "_empty_table1", 14);
    U_STRING_INIT(name_empty_table2,    "_empty_table2", 14);
    U_STRING_INIT(name_empty_table3,    "_empty_table3", 14);
    U_STRING_INIT(name_singleton_table1,"_singleton_table1", 18);
    U_STRING_INIT(name_singleton_table2,"_singleton_table2", 18);
    U_STRING_INIT(name_singleton_table3,"_singleton_table3", 18);
    U_STRING_INIT(name_digit3_map,      "_digit3_map", 12);
    U_STRING_INIT(name_space_keys,      "_space_keys", 12);
    U_STRING_INIT(name_type_examples,   "_type_examples", 15);
    U_STRING_INIT(key_zero,             "zero", 5);
    U_STRING_INIT(key_one,              "one", 4);
    U_STRING_INIT(key_two,              "two", 4);
    U_STRING_INIT(key_text,             "text", 5);
    U_STRING_INIT(key_0bl,              "", 1);
    U_STRING_INIT(key_1bl,              " ", 2);
    U_STRING_INIT(key_3bl,              "   ", 4);
    U_STRING_INIT(key_char,             "char", 5);
    U_STRING_INIT(key_unknown,          "unknown", 8);
    U_STRING_INIT(key_na,               "N/A", 4);
    U_STRING_INIT(key_numb,             "numb", 5);
    U_STRING_INIT(value_empty_key,      "empty_key", 10);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, 3);

    /* check the parse result */
      /* check that there is exactly one block, and that it has the expected code */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 4);
    TEST((*block_list == NULL), 0, test_name, 5);
    TEST_NOT((*(block_list + 1) == NULL), 0, test_name, 6);
    block = *block_list;
    TEST(cif_container_get_code(block, &ustr), CIF_OK, test_name, 7);
    TEST(u_strcmp(block_code, ustr), 0, test_name, 8);
    free(ustr);

      /* check that there is exactly one loop in the block, and that it is the scalar loop */
    TEST(cif_container_get_all_loops(block, &loop_list), CIF_OK, test_name, 9);
    TEST((*loop_list == NULL), 0, test_name, 10);
    TEST_NOT((*(loop_list + 1) == NULL), 0, test_name, 11);
    loop = *loop_list;
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 12);
    TEST(ustr == NULL, 0, test_name, 13);
    TEST(*ustr, 0, test_name, 14);
    free(ustr);

      /* check the number of data names in the loop */
    TEST(cif_loop_get_names(loop, &name_list), CIF_OK, test_name, 15);
    for (name_p = name_list; *name_p; name_p += 1) {
        /* We're responsible for freeing these anyway; might as well do it here and now */
        free(*name_p);
    }
    TEST((name_p - name_list), NUM_ITEMS, test_name, 16);
    free(name_list);

      /* check each expected item */
    TEST(cif_container_get_value(block, name_empty_table1, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 18);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 19);
    TEST(count, 0, test_name, 20);

    TEST(cif_container_get_value(block, name_empty_table2, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 22);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 23);
    TEST(count, 0, test_name, 24);

    TEST(cif_container_get_value(block, name_empty_table3, &value), CIF_OK, test_name, 25);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 26);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 27);
    TEST(count, 0, test_name, 28);

    TEST(cif_container_get_value(block, name_singleton_table1, &value), CIF_OK, test_name, 29);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 30);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 31);
    TEST(count, 1, test_name, 32);
    TEST(cif_value_get_item_by_key(value, key_zero, &element), CIF_OK, test_name, 33);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 34);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 35);
    TEST_NOT(d == 0, 0, test_name, 36);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 37);
    TEST_NOT(d == 0, 0, test_name, 38);

    TEST(cif_container_get_value(block, name_singleton_table2, &value), CIF_OK, test_name, 39);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 40);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 41);
    TEST(count, 1, test_name, 42);
    TEST(cif_value_get_item_by_key(value, key_text, &element), CIF_OK, test_name, 43);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 44);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 45);
    TEST(u_strcmp(ustr, key_text), 0, test_name, 46);
    free(ustr);

    TEST(cif_container_get_value(block, name_singleton_table3, &value), CIF_OK, test_name, 47);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 48);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 49);
    TEST(count, 1, test_name, 50);
    TEST(cif_value_get_item_by_key(value, key_0bl, &element), CIF_OK, test_name, 51);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 52);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 53);
    TEST(u_strcmp(ustr, value_empty_key), 0, test_name, 54);
    free(ustr);

    TEST(cif_container_get_value(block, name_digit3_map, &value), CIF_OK, test_name, 55);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 56);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 57);
    TEST(count, 3, test_name, 58);
    TEST(cif_value_get_item_by_key(value, key_zero, &element), CIF_OK, test_name, 59);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 60);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 61);
    TEST_NOT(d == 0, 0, test_name, 62);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 63);
    TEST_NOT(d == 0, 0, test_name, 64);
    TEST(cif_value_get_item_by_key(value, key_one, &element), CIF_OK, test_name, 65);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 66);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 67);
    TEST_NOT(d == 1, 0, test_name, 68);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 69);
    TEST_NOT(d == 0, 0, test_name, 70);
    TEST(cif_value_get_item_by_key(value, key_two, &element), CIF_OK, test_name, 71);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 72);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 73);
    TEST_NOT(d == 2, 0, test_name, 74);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 75);
    TEST_NOT(d == 0, 0, test_name, 76);

    TEST(cif_container_get_value(block, name_space_keys, &value), CIF_OK, test_name, 77);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 78);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 79);
    TEST(count, 3, test_name, 80);
    TEST(cif_value_get_item_by_key(value, key_0bl, &element), CIF_OK, test_name, 81);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 82);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 83);
    TEST_NOT(d == 0, 0, test_name, 84);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 85);
    TEST_NOT(d == 0, 0, test_name, 86);
    TEST(cif_value_get_item_by_key(value, key_1bl, &element), CIF_OK, test_name, 87);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 88);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 89);
    TEST_NOT(d == 1, 0, test_name, 90);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 91);
    TEST_NOT(d == 0, 0, test_name, 92);
    TEST(cif_value_get_item_by_key(value, key_3bl, &element), CIF_OK, test_name, 93);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 94);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 95);
    TEST_NOT(d == 3, 0, test_name, 96);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 97);
    TEST_NOT(d == 0, 0, test_name, 98);

    TEST(cif_container_get_value(block, name_type_examples, &value), CIF_OK, test_name, 99);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 100);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 101);
    TEST(count, 4, test_name, 102);
    TEST(cif_value_get_item_by_key(value, key_char, &element), CIF_OK, test_name, 103);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 104);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 105);
    TEST(u_strcmp(ustr, key_char), 0, test_name, 106);
    free(ustr);
    TEST(cif_value_get_item_by_key(value, key_unknown, &element), CIF_OK, test_name, 107);
    TEST(cif_value_kind(element), CIF_UNK_KIND, test_name, 108);
    TEST(cif_value_get_item_by_key(value, key_na, &element), CIF_OK, test_name, 109);
    TEST(cif_value_kind(element), CIF_NA_KIND, test_name, 110);
    TEST(cif_value_get_item_by_key(value, key_numb, &element), CIF_OK, test_name, 111);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 112);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 113);
    TEST_NOT(abs(d - -1.234e69) < 1e61, 0, test_name, 114);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 115);
    TEST_NOT(abs(d - 5e66) < 1e60, 0, test_name, 116);

    /* clean up */
    /* DON'T free 'element' */
    cif_value_free(value);
    cif_loop_free(loop);
    free(loop_list);
    cif_block_free(block);
    free(block_list);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
