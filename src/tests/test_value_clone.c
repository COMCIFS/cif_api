/*
 * test_value_clone.c
 *
 * Tests the behavior of the cif_value_clone() function.
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

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_value_clone";
    cif_value_tp *value = NULL;
    cif_value_tp *clone = NULL;
    cif_value_tp *element = NULL;
    cif_value_tp *element2 = NULL;
    U_STRING_DECL(value_text, "value text", 11);
    U_STRING_DECL(numb_text, "1.234(5)", 9);
    U_STRING_DECL(one, "1.000", 6);
    U_STRING_DECL(two, "2e-00", 6);
    U_STRING_DECL(five, "5", 2);
    U_STRING_DECL(three_sir, "Three, sir.", 12);
    UChar *text;
    UChar *text2;
    size_t count;
    size_t extra;
    int i;

    U_STRING_INIT(value_text, "value text", 11);
    U_STRING_INIT(numb_text, "1.234(5)", 9);
    U_STRING_INIT(one, "1.000", 6);
    U_STRING_INIT(two, "2e-00", 6);
    U_STRING_INIT(five, "5", 2);
    U_STRING_INIT(three_sir, "Three, sir.", 12);

    TESTHEADER(test_name);

    /* Test cloning values of kind 'unknown' */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 3);
    TEST(cif_value_clone(value, &clone), CIF_OK, test_name, 4);
    TEST(value == clone, 0, test_name, 5);
    TEST(cif_value_kind(clone), CIF_UNK_KIND, test_name, 6);
    TEST(cif_value_is_quoted(clone), CIF_NOT_QUOTED, test_name, 7);
    cif_value_free(clone);
    clone = NULL;
    
    /* Test cloning values of kind 'NA' */ 
    TEST(cif_value_init(value, CIF_NA_KIND), CIF_OK, test_name, 8);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 9);
    TEST(cif_value_clone(value, &clone), CIF_OK, test_name, 10);
    TEST(value == clone, 0, test_name, 11);
    TEST(cif_value_kind(clone), CIF_NA_KIND, test_name, 12);
    TEST(cif_value_is_quoted(clone), CIF_NOT_QUOTED, test_name, 13);
    cif_value_free(clone);
    clone = NULL;
  
    /* Test cloning values of kind 'char' */ 
    text = cif_u_strdup(value_text);
    TEST(text == NULL, 0, test_name, 14);
    TEST(cif_value_init_char(value, text), CIF_OK, test_name, 15);  /* 'value' takes responsibility for 'text' */
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 16);
    TEST(cif_value_get_text(value, &text2), CIF_OK, test_name, 17);
    TEST(u_strcmp(text, text2), 0, test_name, 18);
    free(text2);
    TEST(cif_value_set_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 19);
    TEST(cif_value_clone(value, &clone), CIF_OK, test_name, 20);
    TEST(value == clone, 0, test_name, 21);
    TEST(cif_value_kind(clone), CIF_CHAR_KIND, test_name, 22);
    TEST(cif_value_is_quoted(clone), CIF_NOT_QUOTED, test_name, 23);
    TEST(cif_value_get_text(clone, &text2), CIF_OK, test_name, 24);
    TEST(u_strcmp(text, text2), 0, test_name, 25);
    free(text2);
    text[0] = 'Q';  /* modifies value, but should not affect clone */
    TEST(cif_value_get_text(value, &text2), CIF_OK, test_name, 26);
    TEST(u_strcmp(text, text2), 0, test_name, 27);
    free(text2);
    TEST(cif_value_get_text(clone, &text2), CIF_OK, test_name, 28);
    TEST(u_strcmp(text, text2) == 0, 0, test_name, 29);
    free(text2);
    /* do not free 'text' because it belongs to 'value' */ 
    cif_value_free(clone);
    clone = NULL;

    /* Test cloning values of kind 'numb' */
    text = cif_u_strdup(numb_text);
    TEST(text == NULL, 0, test_name, 30);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 31);  /* 'value' takes responsibility for 'text' */
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 32);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 33);
    TEST(text == NULL, 0, test_name, 34);
    free(text);
    TEST(cif_value_set_quoted(value, CIF_QUOTED), CIF_OK, test_name, 35);
    TEST(cif_value_clone(value, &clone), CIF_OK, test_name, 36);
    TEST(value == clone, 0, test_name, 37);
    TEST(cif_value_kind(clone), CIF_NUMB_KIND, test_name, 38);
    TEST(cif_value_is_quoted(clone), CIF_QUOTED, test_name, 39);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 40);
    TEST(cif_value_get_text(clone, &text2), CIF_OK, test_name, 41);
    TEST(u_strcmp(text, text2), 0, test_name, 42);
    free(text2);
    free(text);
    cif_value_free(clone);
    clone = NULL;

    /* Test cloning values of kind 'list' */
    TEST(cif_value_init(value, CIF_LIST_KIND), CIF_OK, test_name, 43);
    TEST(cif_value_insert_element_at(value, 0, NULL), CIF_OK, test_name, 44);
    TEST(cif_value_insert_element_at(value, 0, NULL), CIF_OK, test_name, 45);
    TEST(cif_value_insert_element_at(value, 0, NULL), CIF_OK, test_name, 46);
    TEST(cif_value_insert_element_at(value, 0, NULL), CIF_OK, test_name, 47);
    TEST(cif_value_get_element_at(value, 0, &element), CIF_OK, test_name, 48);
    TEST(cif_value_copy_char(element, one), CIF_OK, test_name, 49);  /* 0 */
    TEST(cif_value_get_element_at(value, 1, &element), CIF_OK, test_name, 50);
    TEST(cif_value_copy_char(element, two), CIF_OK, test_name, 51);  /* 1 */
    TEST(cif_value_get_element_at(value, 2, &element), CIF_OK, test_name, 52);
    TEST(cif_value_copy_char(element, five), CIF_OK, test_name, 53); /* 2 */
    TEST(cif_value_get_element_at(value, 3, &element), CIF_OK, test_name, 54);
    TEST(cif_value_init(element, CIF_LIST_KIND), CIF_OK, test_name, 55); /* 3 */
    TEST(cif_value_insert_element_at(element, 0, NULL), CIF_OK, test_name, 56);
    TEST(cif_value_get_element_at(element, 0, &element), CIF_OK, test_name, 57);
    TEST(cif_value_copy_char(element, three_sir), CIF_OK, test_name, 58);
    TEST(cif_value_clone(value, &clone), CIF_OK, test_name, 59);
    TEST(value == clone, 0, test_name, 60);
    TEST(cif_value_kind(clone), CIF_LIST_KIND, test_name, 61);
    TEST(cif_value_get_element_count(clone, &count), CIF_OK, test_name, 62);
    TEST(count, 4, test_name, 63);

    for (i = 0, extra = 0; i < 4; i += 1) {
        TEST(cif_value_get_element_at(value, i, &element), CIF_OK, test_name, 64 + i * 6);
        TEST(cif_value_get_element_at(clone, i, &element2), CIF_OK, test_name, 65 + i * 6);
        TEST(element == element2, 0, test_name, 66 + i * 6);
        if (i >= 3) {
            TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 66 + i * 6 + ++extra);
            TEST(count, 1, test_name, 66 + i * 6 + ++extra);
            TEST(cif_value_get_element_count(element2, &count), CIF_OK, test_name, 66 + i * 6 + ++extra);
            TEST(count, 1, test_name, 66 + i * 6 + ++extra);
            TEST(cif_value_get_element_at(element, 0, &element), CIF_OK, test_name, 66 + i * 6 + ++extra);
            TEST(cif_value_get_element_at(element2, 0, &element2), CIF_OK, test_name, 66 + i * 6 + ++extra);
        }
        TEST(cif_value_get_text(element, &text), CIF_OK, test_name, 67 + i * 6 + extra);
        TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 68 + i * 6 + extra);
        TEST(u_strcmp(text, text2), 0, test_name, 69 + i * 6 + extra);
        free(text);
        free(text2);
    }
    cif_value_free(clone);
    clone = NULL;
    /* maximimum test number to this point == 93 */

    /* Test cloning values of kind 'table' */
    TEST(cif_value_init(value, CIF_TABLE_KIND), CIF_OK, test_name, 94);
    TEST(cif_value_set_item_by_key(value, one, NULL), CIF_OK, test_name, 95);
    TEST(cif_value_set_item_by_key(value, two, NULL), CIF_OK, test_name, 96);
    TEST(cif_value_set_item_by_key(value, five, NULL), CIF_OK, test_name, 97);
    TEST(cif_value_set_item_by_key(value, three_sir, NULL), CIF_OK, test_name, 98);
    TEST(cif_value_get_item_by_key(value, one, &element), CIF_OK, test_name, 99);
    TEST(cif_value_copy_char(element, one), CIF_OK, test_name, 100);
    TEST(cif_value_get_item_by_key(value, two, &element), CIF_OK, test_name, 101);
    TEST(cif_value_copy_char(element, two), CIF_OK, test_name, 102);
    TEST(cif_value_get_item_by_key(value, five, &element), CIF_OK, test_name, 103);
    TEST(cif_value_copy_char(element, five), CIF_OK, test_name, 104);
    TEST(cif_value_get_item_by_key(value, three_sir, &element), CIF_OK, test_name, 105);
    TEST(cif_value_init(element, CIF_TABLE_KIND), CIF_OK, test_name, 106);
    TEST(cif_value_set_item_by_key(element, three_sir, NULL), CIF_OK, test_name, 107);
    TEST(cif_value_get_item_by_key(element, three_sir, &element2), CIF_OK, test_name, 108);
    TEST(cif_value_copy_char(element2, three_sir), CIF_OK, test_name, 109);
    TEST(cif_value_clone(value, &clone), CIF_OK, test_name, 110);
    TEST(value == clone, 0, test_name, 111);

    TEST(cif_value_get_item_by_key(value, one, &element), CIF_OK, test_name, 112);
    TEST(cif_value_get_item_by_key(clone, one, &element2), CIF_OK, test_name, 113);
    TEST(element == element2, 0, test_name, 114);
    TEST(cif_value_get_text(element, &text), CIF_OK, test_name, 115);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 116);
    TEST(u_strcmp(text, text2), 0, test_name, 117);
    free(text);
    free(text2);

    TEST(cif_value_get_item_by_key(value, two, &element), CIF_OK, test_name, 118);
    TEST(cif_value_get_item_by_key(clone, two, &element2), CIF_OK, test_name, 119);
    TEST(element == element2, 0, test_name, 120);
    TEST(cif_value_get_text(element, &text), CIF_OK, test_name, 121);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 122);
    TEST(u_strcmp(text, text2), 0, test_name, 123);
    free(text);
    free(text2);

    TEST(cif_value_get_item_by_key(value, five, &element), CIF_OK, test_name, 124);
    TEST(cif_value_get_item_by_key(clone, five, &element2), CIF_OK, test_name, 125);
    TEST(element == element2, 0, test_name, 126);
    TEST(cif_value_get_text(element, &text), CIF_OK, test_name, 127);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 128);
    TEST(u_strcmp(text, text2), 0, test_name, 129);
    free(text);
    free(text2);

    TEST(cif_value_get_item_by_key(value, three_sir, &element), CIF_OK, test_name, 130);
    TEST(cif_value_get_item_by_key(clone, three_sir, &element2), CIF_OK, test_name, 131);
    TEST(element == element2, 0, test_name, 132);
    TEST(cif_value_get_element_count(element, &count), CIF_OK, test_name, 133);
    TEST(count, 1, test_name, 134);
    TEST(cif_value_get_element_count(element2, &count), CIF_OK, test_name, 135);
    TEST(count, 1, test_name, 136);
    TEST(cif_value_get_item_by_key(element, three_sir, &element), CIF_OK, test_name, 137);
    TEST(cif_value_get_item_by_key(element2, three_sir, &element2), CIF_OK, test_name, 138);
    TEST(cif_value_get_text(element, &text), CIF_OK, test_name, 139);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 140);
    TEST(u_strcmp(text, text2), 0, test_name, 141);
    free(text);
    free(text2);
    TEST(cif_value_get_element_count(clone, &count), CIF_OK, test_name, 142);
    TEST(count, 4, test_name, 143);
    cif_value_free(clone);

    cif_value_free(value);

    return 0;
}

