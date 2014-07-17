/*
 * test_parse_list_data.c
 *
 * Tests parsing simple CIF 2.0 list data.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"
#include "assert_value.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS    15
int main(int argc, char *argv[]) {
    char test_name[80] = "test_parse_list_data";
    char local_file_name[] = "list_data.cif";
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

    U_STRING_DECL(block_code,           "list_data", 10);
    U_STRING_DECL(name_empty_list1,     "_empty_list1", 13);
    U_STRING_DECL(name_empty_list2,     "_empty_list2", 13);
    U_STRING_DECL(name_empty_list3,     "_empty_list3", 13);
    U_STRING_DECL(name_single_na1,      "_single_na1", 12);
    U_STRING_DECL(name_single_na2,      "_single_na2", 12);
    U_STRING_DECL(name_single_na3,      "_single_na3", 12);
    U_STRING_DECL(name_single_unk,      "_single_unk", 12);
    U_STRING_DECL(name_single_string1,  "_single_string1", 16);
    U_STRING_DECL(name_single_string2,  "_single_string2", 16);
    U_STRING_DECL(name_single_string3,  "_single_string3", 16);
    U_STRING_DECL(name_single_numb1,    "_single_numb1", 14);
    U_STRING_DECL(name_single_numb2,    "_single_numb2", 14);
    U_STRING_DECL(name_digit_list,      "_digit_list", 12);
    U_STRING_DECL(name_string_list,     "_string_list", 13);
    U_STRING_DECL(name_mixed_list,      "_mixed_list", 12);
    U_STRING_DECL(value_bare,           "bare", 5);
    U_STRING_DECL(value_sq,             "sq", 3);
    U_STRING_DECL(value_not_list,       "[ not a list ]", 15);
    U_STRING_DECL(value_one,            "one", 4);
    U_STRING_DECL(value_two,            "two", 4);
    U_STRING_DECL(value_three,          "\"three\"", 8);
    U_STRING_DECL(value_mary1,          "Mary", 5);
    U_STRING_DECL(value_mary2,          "had", 4);
    U_STRING_DECL(value_mary4,          "little", 7);
    U_STRING_DECL(value_mary6,          "Its fleece....", 15);

    U_STRING_INIT(block_code,           "list_data", 10);
    U_STRING_INIT(name_empty_list1,     "_empty_list1", 13);
    U_STRING_INIT(name_empty_list2,     "_empty_list2", 13);
    U_STRING_INIT(name_empty_list3,     "_empty_list3", 13);
    U_STRING_INIT(name_single_na1,      "_single_na1", 12);
    U_STRING_INIT(name_single_na2,      "_single_na2", 12);
    U_STRING_INIT(name_single_na3,      "_single_na3", 12);
    U_STRING_INIT(name_single_unk,      "_single_unk", 12);
    U_STRING_INIT(name_single_string1,  "_single_string1", 16);
    U_STRING_INIT(name_single_string2,  "_single_string2", 16);
    U_STRING_INIT(name_single_string3,  "_single_string3", 16);
    U_STRING_INIT(name_single_numb1,    "_single_numb1", 14);
    U_STRING_INIT(name_single_numb2,    "_single_numb2", 14);
    U_STRING_INIT(name_digit_list,      "_digit_list", 12);
    U_STRING_INIT(name_string_list,     "_string_list", 13);
    U_STRING_INIT(name_mixed_list,      "_mixed_list", 12);
    U_STRING_INIT(value_bare,           "bare", 5);
    U_STRING_INIT(value_sq,             "sq", 3);
    U_STRING_INIT(value_not_list,       "[ not a list ]", 15);
    U_STRING_INIT(value_one,            "one", 4);
    U_STRING_INIT(value_two,            "two", 4);
    U_STRING_INIT(value_three,          "\"three\"", 8);
    U_STRING_INIT(value_mary1,          "Mary", 5);
    U_STRING_INIT(value_mary2,          "had", 4);
    U_STRING_INIT(value_mary4,          "little", 7);
    U_STRING_INIT(value_mary6,          "Its fleece....", 15);

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
    TEST(cif_container_get_value(block, name_empty_list1, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 18);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 19);
    TEST(count, 0, test_name, 20);

    TEST(cif_container_get_value(block, name_empty_list2, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 22);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 23);
    TEST(count, 0, test_name, 24);

    TEST(cif_container_get_value(block, name_empty_list3, &value), CIF_OK, test_name, 25);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 26);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 27);
    TEST(count, 0, test_name, 28);

    TEST(cif_container_get_value(block, name_single_na1, &value), CIF_OK, test_name, 29);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 30);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 31);
    TEST(count, 1, test_name, 32);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 33);
    TEST(cif_value_kind(element), CIF_NA_KIND, test_name, 34);

    TEST(cif_container_get_value(block, name_single_na2, &value), CIF_OK, test_name, 35);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 36);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 37);
    TEST(count, 1, test_name, 38);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 39);
    TEST(cif_value_kind(element), CIF_NA_KIND, test_name, 40);

    TEST(cif_container_get_value(block, name_single_na3, &value), CIF_OK, test_name, 41);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 42);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 43);
    TEST(count, 1, test_name, 44);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 45);
    TEST(cif_value_kind(element), CIF_NA_KIND, test_name, 46);

    TEST(cif_container_get_value(block, name_single_unk, &value), CIF_OK, test_name, 47);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 48);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 49);
    TEST(count, 1, test_name, 50);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 51);
    TEST(cif_value_kind(element), CIF_UNK_KIND, test_name, 52);

    TEST(cif_container_get_value(block, name_single_string1, &value), CIF_OK, test_name, 53);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 54);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 55);
    TEST(count, 1, test_name, 56);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 57);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 58);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 59);
    TEST(u_strcmp(ustr, value_bare), 0, test_name, 60);
    free(ustr);

    TEST(cif_container_get_value(block, name_single_string2, &value), CIF_OK, test_name, 61);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 62);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 63);
    TEST(count, 1, test_name, 64);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 65);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 66);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 67);
    TEST(u_strcmp(ustr, value_sq), 0, test_name, 68);
    free(ustr);

    TEST(cif_container_get_value(block, name_single_string3, &value), CIF_OK, test_name, 69);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 70);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 71);
    TEST(count, 1, test_name, 72);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 73);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 74);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 75);
    TEST(u_strcmp(ustr, value_not_list), 0, test_name, 76);
    free(ustr);

    TEST(cif_container_get_value(block, name_single_numb1, &value), CIF_OK, test_name, 77);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 78);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 79);
    TEST(count, 1, test_name, 80);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 81);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 82);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 83);
    TEST_NOT(d == 0, 0, test_name, 84);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 85);
    TEST_NOT(d == 0, 0, test_name, 86);

    TEST(cif_container_get_value(block, name_single_numb2, &value), CIF_OK, test_name, 87);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 88);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 89);
    TEST(count, 1, test_name, 90);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 91);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 92);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 93);
    TEST_NOT(d == -10.0, 0, test_name, 94);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 95);
    TEST_NOT(abs(d - 0.2) < 1e6, 0, test_name, 96);

    TEST(cif_container_get_value(block, name_digit_list, &value), CIF_OK, test_name, 97);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 98);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 99);
    TEST(count, 10, test_name, 100);
#define TESTNUM_BASE     100
#define TESTS_PER_CYCLE    6
    for (count = 0; count < 10; count += 1) {
        TEST(cif_value_get_element_at(value, count, &element), CIF_OK, test_name,
                TESTNUM_BASE + 1 + count * TESTS_PER_CYCLE);
        TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, TESTNUM_BASE + 2 + count * TESTS_PER_CYCLE);
        TEST(cif_value_get_number(element, &d), CIF_OK, test_name, TESTNUM_BASE + 3 + count * TESTS_PER_CYCLE);
        TEST_NOT(d == (double) count, 0, test_name, TESTNUM_BASE + 4 + count * TESTS_PER_CYCLE);
        TEST(cif_value_get_su(element, &d), CIF_OK, test_name, TESTNUM_BASE + 5 + count * TESTS_PER_CYCLE);
        TEST_NOT(d == 0.0, 0, test_name, TESTNUM_BASE + 6 + count * TESTS_PER_CYCLE);
    }
    /* next test is TESTNUM_BASE + 10 * TESTS_PER_CYCLE + 1 = 161 */

    TEST(cif_container_get_value(block, name_string_list, &value), CIF_OK, test_name, 161);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 162);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 163);
    TEST(count, 3, test_name, 164);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 165);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 166);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 167);
    TEST(u_strcmp(ustr, value_one), 0, test_name, 168);
    free(ustr);
    TEST(cif_value_get_element_at(value, 1, &element), CIF_OK, test_name, 169);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 170);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 171);
    TEST(u_strcmp(ustr, value_two), 0, test_name, 172);
    free(ustr);
    TEST(cif_value_get_element_at(value, 2, &element), CIF_OK, test_name, 173);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 174);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 175);
    TEST(u_strcmp(ustr, value_three), 0, test_name, 176);
    free(ustr);

    TEST(cif_container_get_value(block, name_mixed_list, &value), CIF_OK, test_name, 177);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 178);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 179);
    TEST(count, 6, test_name, 180);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 181);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 182);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 183);
    TEST(u_strcmp(ustr, value_mary1), 0, test_name, 184);
    free(ustr);
    TEST(cif_value_get_element_at(value, 1, &element), CIF_OK, test_name, 185);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 186);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 187);
    TEST(u_strcmp(ustr, value_mary2), 0, test_name, 188);
    free(ustr);
    TEST(cif_value_get_element_at(value, 2, &element), CIF_OK, test_name, 189);
    TEST(cif_value_kind(element), CIF_NUMB_KIND, test_name, 190);
    TEST(cif_value_get_number(element, &d), CIF_OK, test_name, 191);
    TEST_NOT(d == 1.0, 0, test_name, 192);
    TEST(cif_value_get_su(element, &d), CIF_OK, test_name, 193);
    TEST_NOT(d == 0.0, 0, test_name, 194);
    TEST(cif_value_get_element_at(value, 3, &element), CIF_OK, test_name, 195);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 196);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 197);
    TEST(u_strcmp(ustr, value_mary4), 0, test_name, 198);
    free(ustr);
    TEST(cif_value_get_element_at(value, 4, &element), CIF_OK, test_name, 199);
    TEST(cif_value_kind(element), CIF_UNK_KIND, test_name, 200);
    TEST(cif_value_get_element_at(value, 5, &element), CIF_OK, test_name, 201);
    TEST(cif_value_kind(element), CIF_CHAR_KIND, test_name, 202);
    TEST(cif_value_get_text(element, &ustr), CIF_OK, test_name, 203);
    TEST(u_strcmp(ustr, value_mary6), 0, test_name, 204);
    free(ustr);

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
