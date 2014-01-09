/*
 * test_list_elements.c
 *
 * Tests the CIF API's list value manipulation functions
 * cif_value_get_element_at(), cif_value_set_element_at,
 * cif_value_insert_element_at(), cif_value_remove_element_at(),
 * and cif_value_get_element_count().
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_list_elements";
    cif_value_t *value = NULL;
    cif_value_t *element1 = NULL;
    cif_value_t *element2 = NULL;
    cif_value_t *element3 = NULL;
    UChar *text1;
    UChar *text2;
    size_t count;
    double d1;
    double d2;
    U_STRING_DECL(value_text, "value text", 11);
    U_STRING_DECL(value_text2, "value text 2", 13);

    U_STRING_INIT(value_text, "value text", 11);
    U_STRING_INIT(value_text2, "value text 2", 13);

    TESTHEADER(test_name);

    /* Start with an empty list value */ 
    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 3);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 4);
    TEST(count, 0, test_name, 5);

    /* Test wrong-index actions on an empty list */
    TEST(cif_value_get_element_at(value, 0, &element1), CIF_INVALID_INDEX, test_name, 6);
    TEST(cif_value_get_element_at(value, -1, &element1), CIF_INVALID_INDEX, test_name, 7);
    TEST(cif_value_create(CIF_UNK_KIND, &element1), CIF_OK, test_name, 8);
    TEST(cif_value_set_element_at(value, 0, element1), CIF_INVALID_INDEX, test_name, 9);
    TEST(cif_value_set_element_at(value, -1, element1), CIF_INVALID_INDEX, test_name, 10);
    /* element1 is valid and independent */

    /* error results for wrong-type arguments */

    TEST(cif_value_kind(element1), CIF_UNK_KIND, test_name, 12);
    TEST(cif_value_get_element_at(element1, 0, &element2), CIF_ARGUMENT_ERROR, test_name, 13);
    TEST(cif_value_set_element_at(element1, 0, element2), CIF_ARGUMENT_ERROR, test_name, 14);
    cif_value_free(element1);

    TEST(cif_value_create(CIF_TABLE_KIND, &element1), CIF_OK, test_name, 15);
    TEST(cif_value_kind(element1), CIF_TABLE_KIND, test_name, 16);
    TEST(cif_value_get_element_at(element1, 0, &element2), CIF_ARGUMENT_ERROR, test_name, 17);
    TEST(cif_value_set_element_at(element1, 0, element2), CIF_ARGUMENT_ERROR, test_name, 18);
    cif_value_free(element1);

    TEST(cif_value_create(CIF_CHAR_KIND, &element1), CIF_OK, test_name, 19);
    TEST(cif_value_kind(element1), CIF_CHAR_KIND, test_name, 20);
    TEST(cif_value_get_element_at(element1, 0, &element2), CIF_ARGUMENT_ERROR, test_name, 21);
    TEST(cif_value_set_element_at(element1, 0, element2), CIF_ARGUMENT_ERROR, test_name, 22);
    cif_value_free(element1);

    TEST(cif_value_create(CIF_NUMB_KIND, &element1), CIF_OK, test_name, 23);
    TEST(cif_value_kind(element1), CIF_NUMB_KIND, test_name, 24);
    TEST(cif_value_get_element_at(element1, 0, &element2), CIF_ARGUMENT_ERROR, test_name, 25);
    TEST(cif_value_set_element_at(element1, 0, element2), CIF_ARGUMENT_ERROR, test_name, 26);
    cif_value_free(element1);

    TEST(cif_value_create(CIF_NA_KIND, &element1), CIF_OK, test_name, 27);
    TEST(cif_value_kind(element1), CIF_NA_KIND, test_name, 28);
    TEST(cif_value_get_element_at(element1, 0, &element2), CIF_ARGUMENT_ERROR, test_name, 29);
    TEST(cif_value_set_element_at(element1, 0, element2), CIF_ARGUMENT_ERROR, test_name, 30);
    cif_value_free(element1);

    /* insertion and retrieval */

    /* element 0 */
    TEST(cif_value_create(CIF_UNK_KIND, &element1), CIF_OK, test_name, 31);
    TEST(cif_value_init_numb(element1, 17.25, 0.125, 3, 5), CIF_OK, test_name, 32);
    TEST(cif_value_insert_element_at(value, 0, element1), CIF_OK, test_name, 33);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 34);
    TEST(count, 1, test_name, 35);
    TEST(cif_value_get_element_at(value, 0, &element2), CIF_OK, test_name, 36);
    /* element pointers should be unequal */
    TEST(element1 == element2, 0, test_name, 37);
    /* element values should be equal */
    TEST(cif_value_kind(element2), CIF_NUMB_KIND, test_name, 38);
    cif_value_get_number(element1, &d1);
    cif_value_get_number(element2, &d2);
    TEST(d1 != d2, 0, test_name, 39);
    cif_value_get_su(element1, &d1);
    cif_value_get_su(element2, &d2);
    TEST(d1 != d2, 0, test_name, 40);
    TEST(cif_value_get_text(element1, &text1), CIF_OK, test_name, 41);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 42);
    TEST(u_strcmp(text1, text2), 0, test_name, 43);
    free(text1);
    free(text2);
    cif_value_free(element1);
    /* cif_value_get_element() should be returning a pointer to its internal value object, not to a clone */
    TEST(cif_value_get_element_at(value, 0, &element1), CIF_OK, test_name, 44);
    TEST(element1 != element2, 0, test_name, 45);
    /* element1 and element2 both belong to the list value */

    /* element 1 */
    TEST(cif_value_create(CIF_NA_KIND, &element1), CIF_OK, test_name, 46);
    TEST(cif_value_insert_element_at(value, 1, element1), CIF_OK, test_name, 47);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 48);
    TEST(count, 2, test_name, 49);
    TEST(cif_value_get_element_at(value, 1, &element3), CIF_OK, test_name, 50);
    TEST(element3 == element1, 0, test_name, 51);
    TEST(element3 == element2, 0, test_name, 52);
    TEST(cif_value_kind(element3), CIF_NA_KIND, test_name, 53);
    TEST(cif_value_get_element_at(value, 0, &element3), CIF_OK, test_name, 54);
    TEST(cif_value_kind(element3), CIF_NUMB_KIND, test_name, 55);
    cif_value_get_number(element3, &d1);
    TEST(d1 != 17.25, 0, test_name, 56);
    /* element2 and element3 both belong to the list value */
    /* element1 is valid and independent */

    /* element 0.5 */
    TEST(cif_value_copy_char(element1, value_text), CIF_OK, test_name, 58);
    TEST(cif_value_insert_element_at(value, 1, element1), CIF_OK, test_name, 59);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 60);
    TEST(count, 3, test_name, 61);
    TEST(cif_value_get_element_at(value, 1, &element3), CIF_OK, test_name, 62);
    TEST(element3 == element1, 0, test_name, 63);
    TEST(element3 == element2, 0, test_name, 64);
    TEST(cif_value_kind(element3), CIF_CHAR_KIND, test_name, 65);
    TEST(cif_value_get_text(element3, &text1), CIF_OK, test_name, 66);
    TEST(u_strcmp(text1, value_text), 0, test_name, 67);
    cif_value_free(element1);
    free(text1);
    /* element2 and element3 both belong to the list value */
    /* element1 is invalid (freed) */

    /* element -0 */
    TEST(cif_value_insert_element_at(value, 0, NULL), CIF_OK, test_name, 68);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 69);
    TEST(count, 4, test_name, 70);
    TEST(cif_value_get_element_at(value, 0, &element3), CIF_OK, test_name, 71);
    TEST(cif_value_init_numb(element3, 42.0, 0.0, 0, 5), CIF_OK, test_name, 72);
    TEST(cif_value_kind(element3), CIF_NUMB_KIND, test_name, 73);
    cif_value_get_number(element3, &d1);
    TEST(d1 != 42.0, 0, test_name, 74);
    /* element2 and element3 both belong to the list value */
    /* element1 is invalid (freed) */

    /* test setting an element to itself */
    TEST(cif_value_get_element_at(value, 1, &element1), CIF_OK, test_name, 77);
    TEST(cif_value_set_element_at(value, 1, element1), CIF_OK, test_name, 78);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 79);
    TEST(count, 4, test_name, 80);
    TEST(cif_value_get_element_at(value, 1, &element2), CIF_OK, test_name, 81);
    TEST(element1 != element2, 0, test_name, 82);
    /* element1, element2, and element3 all belong to the list value */

    /* test shortcut for setting an unknown value */
    TEST(cif_value_set_element_at(value, 2, NULL), CIF_OK, test_name, 83);
    TEST(cif_value_get_element_at(value, 2, &element1), CIF_OK, test_name, 84);
    TEST(cif_value_kind(element1), CIF_UNK_KIND, test_name, 85);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 86);
    TEST(count, 4, test_name, 87);

    /* test setting an existing value */
    TEST(cif_value_create(CIF_UNK_KIND, &element3), CIF_OK, test_name, 88);
    TEST(cif_value_copy_char(element3, value_text2), CIF_OK, test_name, 89);
    TEST(cif_value_set_element_at(value, 2, element3), CIF_OK, test_name, 90);
    TEST(cif_value_get_element_at(value, 2, &element1), CIF_OK, test_name, 91);
    TEST(element1 == element3, 0, test_name, 92);
    TEST(cif_value_get_text(element1, &text1), CIF_OK, test_name, 93);
    TEST(u_strcmp(text1, value_text2), 0, test_name, 94);
    cif_value_free(element3);
    /* element1 and element2 both belong to the list value */
    /* element3 is invalid (freed) */

    /* test removing the last value */
    TEST(cif_value_get_element_at(value, 3, &element3), CIF_OK, test_name, 95);
    TEST(cif_value_remove_element_at(value, 3, &element2), CIF_OK, test_name, 96);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 97);
    TEST(count, 3, test_name, 98);
    TEST(element2 != element3, 0, test_name, 99);
    cif_value_free(element2);
    /* element1 belongs to the list value */
    /* element2 and element3 are invalid (freed) */

    /* test removing a middle value */
    TEST(cif_value_get_element_at(value, 0, &element1), CIF_OK, test_name, 100);
    TEST(cif_value_get_element_at(value, 2, &element2), CIF_OK, test_name, 101);
    TEST(cif_value_remove_element_at(value, 1, NULL), CIF_OK, test_name, 102);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 103);
    TEST(count, 2, test_name, 104);
    TEST(cif_value_get_element_at(value, 0, &element3), CIF_OK, test_name, 105);
    TEST(element1 != element3, 0, test_name, 106);
    TEST(cif_value_get_element_at(value, 1, &element3), CIF_OK, test_name, 107);
    TEST(element2 != element3, 0, test_name, 108);
    /* element1, element2, and element3 all belong to the list value */

    /* test removing the first value */
    TEST(cif_value_remove_element_at(value, 0, NULL), CIF_OK, test_name, 109);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 110);
    TEST(count, 1, test_name, 111);
    TEST(cif_value_get_element_at(value, 0, &element3), CIF_OK, test_name, 112);
    TEST(element2 != element3, 0, test_name, 113);

    /* test removing the only value */
    TEST(cif_value_remove_element_at(value, 0, NULL), CIF_OK, test_name, 114);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 115);
    TEST(count, 0, test_name, 116);

    cif_value_free(value);

    return 0;
}

