/*
 * test_parse_complex_data.c
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
#define NUM_ITEMS     3
int main(void) {
    char test_name[80] = "test_parse_complex_data";
    char local_file_name[] = "complex_data.cif";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_block_t **block_list;
    cif_loop_t **loop_list;
    cif_loop_t *loop;
    cif_value_t *value = NULL;
    cif_value_t *element = NULL;
    cif_value_t *el_element = NULL;
    UChar **name_list;
    UChar **name_p;
    UChar *ustr;
    double d;
    size_t count;

    U_STRING_DECL(block_code,           "complex_data", 13);
    U_STRING_DECL(name_list_of_lists,   "_list_of_lists", 15);
    U_STRING_DECL(name_table_of_tables, "_table_of_tables", 17);
    U_STRING_DECL(name_hodge_podge,     "_hodge_podge", 13);
    U_STRING_DECL(key_English,          "English", 8);
    U_STRING_DECL(key_French,           "French", 7);
    U_STRING_DECL(key_one,              "one", 4);
    U_STRING_DECL(key_two,              "two", 4);
    U_STRING_DECL(key_alice,            "alice", 6);
    U_STRING_DECL(key_bob,              "bob", 4);
    U_STRING_DECL(key_charles,          "charles", 8);
    U_STRING_DECL(value_foo,            "foo", 4);
    U_STRING_DECL(value_bar,            "bar", 4);
    U_STRING_DECL(value_un,             "un", 3);
    U_STRING_DECL(value_deux,           "deux", 5);
    U_STRING_DECL(value_Cambridge,      "Cambridge", 10);
    U_STRING_DECL(value_Harvard,        "Harvard", 8);
    UChar key_a[] = { 'a', 0 };
    UChar key_b[] = { 'b', 0 };
    UChar key_c[] = { 'c', 0 };
    UChar value_x[] = { 'x', 0 };
    UChar value_y[] = { 'y', 0 };
    UChar value_z[] = { 'z', 0 };
    UChar *value_one;
    UChar *value_two;

    U_STRING_INIT(block_code,           "complex_data", 13);
    U_STRING_INIT(name_list_of_lists,   "_list_of_lists", 15);
    U_STRING_INIT(name_table_of_tables, "_table_of_tables", 17);
    U_STRING_INIT(name_hodge_podge,     "_hodge_podge", 13);
    U_STRING_INIT(key_English,          "English", 8);
    U_STRING_INIT(key_French,           "French", 7);
    U_STRING_INIT(key_one,              "one", 4);
    U_STRING_INIT(key_two,              "two", 4);
    U_STRING_INIT(key_alice,            "alice", 6);
    U_STRING_INIT(key_bob,              "bob", 4);
    U_STRING_INIT(key_charles,          "charles", 8);
    U_STRING_INIT(value_foo,            "foo", 4);
    U_STRING_INIT(value_bar,            "bar", 4);
    U_STRING_INIT(value_un,             "un", 3);
    U_STRING_INIT(value_deux,           "deux", 5);
    U_STRING_INIT(value_Cambridge,      "Cambridge", 10);
    U_STRING_INIT(value_Harvard,        "Harvard", 8);
    value_one = key_one;
    value_two = key_two;

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
    TEST(cif_container_get_value(block, name_list_of_lists, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 18);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 19);
    TEST(count, 3, test_name, 20);

    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 21);
    TEST(cif_value_kind(element), CIF_LIST_KIND, test_name, 22);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 23);
    TEST(count, 0, test_name, 24);

    TEST(cif_value_get_element_at(value, 1, &element), CIF_OK, test_name, 25);
    TEST(cif_value_kind(element), CIF_LIST_KIND, test_name, 26);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 27);
    TEST(count, 2, test_name, 28);
    TEST(cif_value_get_element_at(element, 0, &el_element), CIF_OK, test_name, 29);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 30);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 31);
    TEST(u_strcmp(ustr, value_foo), 0, test_name, 32);
    free(ustr);
    TEST(cif_value_get_element_at(element, 1, &el_element), CIF_OK, test_name, 33);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 34);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 35);
    TEST(u_strcmp(ustr, value_bar), 0, test_name, 36);
    free(ustr);

    TEST(cif_value_get_element_at(value, 2, &element), CIF_OK, test_name, 37);
    TEST(cif_value_kind(element), CIF_LIST_KIND, test_name, 38);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 39);
    TEST(count, 3, test_name, 40);
    TEST(cif_value_get_element_at(element, 0, &el_element), CIF_OK, test_name, 41);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 42);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 43);
    TEST(u_strcmp(ustr, value_x), 0, test_name, 44);
    free(ustr);
    TEST(cif_value_get_element_at(element, 1, &el_element), CIF_OK, test_name, 45);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 46);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 47);
    TEST(u_strcmp(ustr, value_y), 0, test_name, 48);
    free(ustr);
    TEST(cif_value_get_element_at(element, 2, &el_element), CIF_OK, test_name, 49);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 50);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 51);
    TEST(u_strcmp(ustr, value_z), 0, test_name, 52);
    free(ustr);

    TEST(cif_container_get_value(block, name_table_of_tables, &value), CIF_OK, test_name, 53);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 54);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 55);
    TEST(count, 2, test_name, 56);

    TEST(cif_value_get_item_by_key(value, key_English, &element), CIF_OK, test_name, 57);
    TEST(cif_value_kind(element), CIF_TABLE_KIND, test_name, 58);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 59);
    TEST(count, 2, test_name, 60);
    TEST(cif_value_get_item_by_key(element, key_one, &el_element), CIF_OK, test_name, 61);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 62);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 63);
    TEST(u_strcmp(ustr, value_one), 0, test_name, 64);
    free(ustr);
    TEST(cif_value_get_item_by_key(element, key_two, &el_element), CIF_OK, test_name, 65);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 66);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 67);
    TEST(u_strcmp(ustr, value_two), 0, test_name, 68);
    free(ustr);

    TEST(cif_value_get_item_by_key(value, key_French, &element), CIF_OK, test_name, 69);
    TEST(cif_value_kind(element), CIF_TABLE_KIND, test_name, 70);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 71);
    TEST(count, 2, test_name, 72);
    TEST(cif_value_get_item_by_key(element, key_one, &el_element), CIF_OK, test_name, 73);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 74);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 75);
    TEST(u_strcmp(ustr, value_un), 0, test_name, 76);
    free(ustr);
    TEST(cif_value_get_item_by_key(element, key_two, &el_element), CIF_OK, test_name, 77);
    TEST(cif_value_kind(el_element), CIF_CHAR_KIND, test_name, 78);
    TEST(cif_value_get_text(el_element, &ustr), CIF_OK, test_name, 79);
    TEST(u_strcmp(ustr, value_deux), 0, test_name, 80);
    free(ustr);

    TEST(cif_container_get_value(block, name_hodge_podge, &value), CIF_OK, test_name, 81);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 82);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 83);
    TEST(count, 3, test_name, 84);

    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 85);
    TEST(cif_value_kind(element), CIF_UNK_KIND, test_name, 86);

    TEST(cif_value_get_element_at(value, 1, &element), CIF_OK, test_name, 87);
    /* { 'a':10 'b':11 'c':[? 12] } */
    TEST(cif_value_kind(element), CIF_TABLE_KIND, test_name, 88);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 89);
    TEST(count, 3, test_name, 90);
    TEST(cif_value_get_item_by_key(element, key_a, &el_element), CIF_OK, test_name, 91);
    TEST(cif_value_kind(el_element), CIF_NUMB_KIND, test_name, 92);
    TEST(cif_value_get_number(el_element, &d), CIF_OK, test_name, 93);
    TEST_NOT(d == 10.0, 0, test_name, 94);
    TEST(cif_value_get_su(el_element, &d), CIF_OK, test_name, 95);
    TEST_NOT(d == 0, 0, test_name, 96);
    TEST(cif_value_get_item_by_key(element, key_b, &el_element), CIF_OK, test_name, 97);
    TEST(cif_value_kind(el_element), CIF_NUMB_KIND, test_name, 98);
    TEST(cif_value_get_number(el_element, &d), CIF_OK, test_name, 99);
    TEST_NOT(d == 11.0, 0, test_name, 100);
    TEST(cif_value_get_su(el_element, &d), CIF_OK, test_name, 101);
    TEST_NOT(d == 0, 0, test_name, 102);
    TEST(cif_value_get_item_by_key(element, key_c, &el_element), CIF_OK, test_name, 103);
    TEST(cif_value_kind(el_element), CIF_LIST_KIND, test_name, 104);
    TEST(cif_value_get_element_count(el_element, &count), CIF_OK, test_name, 105);
    TEST(count, 2, test_name, 106);
    TEST(cif_value_get_element_at(el_element, 0, &element), CIF_OK, test_name, 107);
    TEST(cif_value_kind(element), CIF_UNK_KIND, test_name, 108);
    TEST(cif_value_get_element_at(el_element, 1, &element), CIF_OK, test_name, 109);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 110);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 111);
    TEST_NOT(d == 12.0, 0, test_name, 112);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 113);
    TEST_NOT(d == 0, 0, test_name, 114);

    TEST(cif_value_get_element_at(value, 2, &element), CIF_OK, test_name, 115);
    /* [. . {} {'alice':Cambridge 'bob':Harvard 'charles':.}] */
    TEST(cif_value_kind(element), CIF_LIST_KIND, test_name, 116);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 117);
    TEST(count, 4, test_name, 118);
    TEST(cif_value_get_element_at(element, 0, &el_element), CIF_OK, test_name, 119);
    TEST(cif_value_kind(el_element), CIF_NA_KIND, test_name, 120);
    TEST(cif_value_get_element_at(element, 1, &el_element), CIF_OK, test_name, 121);
    TEST(cif_value_kind(el_element), CIF_NA_KIND, test_name, 122);
    TEST(cif_value_get_element_at(element, 2, &el_element), CIF_OK, test_name, 123);
    TEST(cif_value_kind(el_element), CIF_TABLE_KIND, test_name, 124);
    TEST(cif_value_get_element_count(el_element, &count), CIF_OK, test_name, 125);
    TEST(count, 0, test_name, 126);
    TEST(cif_value_get_element_at(element, 3, &el_element), CIF_OK, test_name, 127);
    TEST(cif_value_kind(el_element), CIF_TABLE_KIND, test_name, 128);
    TEST(cif_value_get_element_count(el_element, &count), CIF_OK, test_name, 129);
    TEST(count, 3, test_name, 130);
    TEST(cif_value_get_item_by_key(el_element, key_alice, &element), CIF_OK, test_name, 131);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 132);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 133);
    TEST(u_strcmp(ustr, value_Cambridge), 0, test_name, 134);
    free(ustr);
    TEST(cif_value_get_item_by_key(el_element, key_bob, &element), CIF_OK, test_name, 135);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 136);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 137);
    TEST(u_strcmp(ustr, value_Harvard), 0, test_name, 138);
    free(ustr);
    TEST(cif_value_get_item_by_key(el_element, key_charles, &element), CIF_OK, test_name, 139);
    TEST(cif_value_kind(element), CIF_NA_KIND, test_name, 140);

    /* clean up */
    /* DON'T free 'element' or 'el_element' */
    cif_value_free(value);
    cif_loop_free(loop);
    free(loop_list);
    cif_block_free(block);
    free(block_list);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
