/*
 * test_table_elements.c
 *
 * Tests the CIF API's table value manipulation functions
 * cif_value_get_item_by_key(), cif_value_set_item_by_key(),
 * cif_value_remove_item_by_key(), cif_value_get_keys(),
 * and cif_value_get_element_count().
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

static int test_key_list(UChar **expected, int num_expected, const UChar **observed);

int main(void) {
    char test_name[80] = "test_table_elements";
    cif_value_tp *value = NULL;
    cif_value_tp *element1 = NULL;
    cif_value_tp *element2 = NULL;
    cif_value_tp *element3 = NULL;
    UChar empty_key[] = { 0 };
    UChar invalid_key[] = { 'K', 0xFFFF, 'y', 0 };
    UChar uncomposed_key[] = { 'K', 0x0073, 0x0307, 0x0323, 0 };
    UChar equivalent_key[] = { 'K', 0x0073, 0x0323, 0x0307, 0 };
    UChar equiv2_key[] =     { 'K', 0x1e61, 0x0323, 0 };
    UChar normalized_key[] = { 'K', 0x1e69, 0 };
    UChar folded_key[] =     { 'k', 0x1e69, 0 };
    UChar *all_keys[4];
    const UChar **keys;
    UChar *text1;
    UChar *text2;
    size_t count;
    double d1;
    UChar value_text[] =  { 'v', 'a', 'l', 'u', 'e', ' ', 't', 'e', 'x', 't', 0 };
    UChar value_text2[] = { 'v', 'a', 'l', 'u', 'e', ' ', 't', 'e', 'x', 't', ' ', '2', 0 };
    UChar value_text3[] = { 'v', 'A', 'L', 'u', 'E', '_', 'T', 'E', 'X', 't', ' ', '3', 0 };
    UChar key1[] =        { 'k', 'e', 'y', ' ', '1', 0 };
    UChar blank_key[] =   { ' ', ' ', 0 };

    TESTHEADER(test_name);

    all_keys[0] = key1;
    all_keys[1] = empty_key;
    all_keys[2] = blank_key;
    all_keys[3] = uncomposed_key;

    /* Start with an empty table value */ 
    TEST(cif_value_create(CIF_TABLE_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 3);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 4);
    TEST(count, 0, test_name, 5);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 6);
    TEST(test_key_list(all_keys, 0, keys), 0, test_name, 7);
    free(keys);

    /* Test wrong-key actions on an empty table */
    TEST(cif_value_get_item_by_key(value, key1, &element1), CIF_NOSUCH_ITEM, test_name, 8);
    TEST(cif_value_get_item_by_key(value, empty_key, &element1), CIF_NOSUCH_ITEM, test_name, 9);
    TEST(cif_value_remove_item_by_key(value, key1, &element1), CIF_NOSUCH_ITEM, test_name, 10);
    TEST(cif_value_remove_item_by_key(value, empty_key, &element1), CIF_NOSUCH_ITEM, test_name, 11);

    /* error results for wrong-type arguments */

    TEST(cif_value_create(CIF_UNK_KIND, &element1), CIF_OK, test_name, 12);
    TEST(cif_value_kind(element1), CIF_UNK_KIND, test_name, 13);
    TEST(cif_value_get_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 14);
    TEST(cif_value_set_item_by_key(element1, key1, value), CIF_ARGUMENT_ERROR, test_name, 15);
    TEST(cif_value_remove_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 16);
    TEST(cif_value_get_keys(element1, &keys), CIF_ARGUMENT_ERROR, test_name, 17);

    TEST(cif_value_init(element1, CIF_LIST_KIND), CIF_OK, test_name, 18);
    TEST(cif_value_kind(element1), CIF_LIST_KIND, test_name, 17);
    TEST(cif_value_get_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 18);
    TEST(cif_value_set_item_by_key(element1, key1, value), CIF_ARGUMENT_ERROR, test_name, 19);
    TEST(cif_value_remove_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 20);
    TEST(cif_value_get_keys(element1, &keys), CIF_ARGUMENT_ERROR, test_name, 21);

    TEST(cif_value_init(element1, CIF_CHAR_KIND), CIF_OK, test_name, 22);
    TEST(cif_value_kind(element1), CIF_CHAR_KIND, test_name, 23);
    TEST(cif_value_get_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 24);
    TEST(cif_value_set_item_by_key(element1, key1, value), CIF_ARGUMENT_ERROR, test_name, 25);
    TEST(cif_value_remove_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 26);
    TEST(cif_value_get_keys(element1, &keys), CIF_ARGUMENT_ERROR, test_name, 27);

    TEST(cif_value_init(element1, CIF_NUMB_KIND), CIF_OK, test_name, 28);
    TEST(cif_value_kind(element1), CIF_NUMB_KIND, test_name, 29);
    TEST(cif_value_get_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 30);
    TEST(cif_value_set_item_by_key(element1, key1, value), CIF_ARGUMENT_ERROR, test_name, 31);
    TEST(cif_value_remove_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 32);
    TEST(cif_value_get_keys(element1, &keys), CIF_ARGUMENT_ERROR, test_name, 33);

    TEST(cif_value_init(element1, CIF_NA_KIND), CIF_OK, test_name, 34);
    TEST(cif_value_kind(element1), CIF_NA_KIND, test_name, 35);
    TEST(cif_value_get_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 36);
    TEST(cif_value_set_item_by_key(element1, key1, value), CIF_ARGUMENT_ERROR, test_name, 37);
    TEST(cif_value_remove_item_by_key(element1, key1, &element2), CIF_ARGUMENT_ERROR, test_name, 38);
    TEST(cif_value_get_keys(element1, &keys), CIF_ARGUMENT_ERROR, test_name, 39);

    /* test with an invalid key */
    TEST(cif_value_set_item_by_key(value, invalid_key, element1), CIF_INVALID_INDEX, test_name, 40);
    TEST(cif_value_get_item_by_key(value, invalid_key, &element2), CIF_NOSUCH_ITEM, test_name, 41);
    TEST(cif_value_remove_item_by_key(value, invalid_key, &element2), CIF_NOSUCH_ITEM, test_name, 42);
    cif_value_free(element1);

    /* insertion and retrieval */

    /* item key1 */
    TEST(cif_value_create(CIF_UNK_KIND, &element1), CIF_OK, test_name, 43);
    TEST(cif_value_init_numb(element1, 17.25, 0.125, 3, 5), CIF_OK, test_name, 44);
    TEST(cif_value_kind(element1), CIF_NUMB_KIND, test_name, 45);
    TEST(cif_value_set_item_by_key(value, all_keys[0], element1), CIF_OK, test_name, 46);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 47);
    TEST(count, 1, test_name, 48);
    TEST(cif_value_get_item_by_key(value, all_keys[0], &element2), CIF_OK, test_name, 49);
    /* element pointers should be unequal */
    TEST(element1 == element2, 0, test_name, 50);
    /* element values should be equal */
    TEST(!assert_values_equal(element1, element2), 0, test_name, 51);
    cif_value_free(element1);
    /* cif_value_get_item_by_key() should be returning a pointer to its internal value object, not to a clone */
    TEST(cif_value_get_item_by_key(value, all_keys[0], &element1), CIF_OK, test_name, 52);
    TEST(element1 != element2, 0, test_name, 53);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 54);
    TEST(test_key_list(all_keys, 1, keys), 0, test_name, 55);
    free(keys);
    /* element1 and element2 both belong to the table value */

    /* item empty_key */
    TEST(cif_value_create(CIF_NA_KIND, &element1), CIF_OK, test_name, 56);
    TEST(cif_value_kind(element1), CIF_NA_KIND, test_name, 57);
    TEST(cif_value_set_item_by_key(value, all_keys[1], element1), CIF_OK, test_name, 58);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 59);
    TEST(count, 2, test_name, 60);
    TEST(cif_value_get_item_by_key(value, all_keys[1], &element3), CIF_OK, test_name, 61);
    TEST(element3 == element1, 0, test_name, 62);
    TEST(element3 == element2, 0, test_name, 63);
    TEST(cif_value_kind(element3), CIF_NA_KIND, test_name, 64);
    TEST(cif_value_get_item_by_key(value, all_keys[0], &element3), CIF_OK, test_name, 65);
    TEST(cif_value_kind(element3), CIF_NUMB_KIND, test_name, 66);
    TEST(cif_value_get_number(element3, &d1), CIF_OK, test_name, 67);
    TEST(d1 != 17.25, 0, test_name, 72);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 73);
    TEST(test_key_list(all_keys, 2, keys), 0, test_name, 74);
    free(keys);
    /* element2 and element3 both belong to the list value */
    /* element1 is valid and independent */

    /* item blank_key */
    TEST(cif_value_copy_char(element1, value_text), CIF_OK, test_name, 75);
    TEST(cif_value_kind(element1), CIF_CHAR_KIND, test_name, 76);
    TEST(cif_value_set_item_by_key(value, all_keys[2], element1), CIF_OK, test_name, 77);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 78);
    TEST(count, 3, test_name, 79);
    TEST(cif_value_get_item_by_key(value, all_keys[2], &element3), CIF_OK, test_name, 80);
    TEST(element3 == element1, 0, test_name, 81);
    TEST(element3 == element2, 0, test_name, 82);
    TEST(cif_value_kind(element3), CIF_CHAR_KIND, test_name, 83);
    TEST(cif_value_get_text(element3, &text1), CIF_OK, test_name, 84);
    TEST(u_strcmp(text1, value_text), 0, test_name, 85);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 86);
    TEST(test_key_list(all_keys, 3, keys), 0, test_name, 87);
    cif_value_free(element1);
    free(text1);
    free(keys);
    /* element2 and element3 both belong to the list value */
    /* element1 is invalid (freed) */

    /* test setting an element to itself */
    TEST(cif_value_get_item_by_key(value, all_keys[0], &element1), CIF_OK, test_name, 88);
    TEST(cif_value_set_item_by_key(value, all_keys[0], element1), CIF_OK, test_name, 89);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 90);
    TEST(count, 3, test_name, 91);
    TEST(cif_value_get_item_by_key(value, all_keys[0], &element2), CIF_OK, test_name, 92);
    TEST(element1 != element2, 0, test_name, 93);
    /* element1, element2, and element3 all belong to the list value */

    /* test shortcut for setting an unknown value */
    TEST(cif_value_set_item_by_key(value, empty_key, NULL), CIF_OK, test_name, 94);
    TEST(cif_value_get_item_by_key(value, empty_key, &element1), CIF_OK, test_name, 95);
    TEST(cif_value_kind(element1), CIF_UNK_KIND, test_name, 96);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 97);
    TEST(count, 3, test_name, 98);
    /* element1, element2, and element3 all belong to the list value */

    /* test setting an existing value */
    TEST(cif_value_create(CIF_UNK_KIND, &element3), CIF_OK, test_name, 99);
    TEST(cif_value_copy_char(element3, value_text2), CIF_OK, test_name, 100);
    TEST(cif_value_set_item_by_key(value, blank_key, element3), CIF_OK, test_name, 101);
    TEST(cif_value_get_item_by_key(value, blank_key, &element1), CIF_OK, test_name, 102);
    TEST(element1 == element3, 0, test_name, 103);
    TEST(cif_value_get_text(element1, &text1), CIF_OK, test_name, 104);
    TEST(u_strcmp(text1, value_text2), 0, test_name, 105);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 106);
    TEST(test_key_list(all_keys, 3, keys), 0, test_name, 107);
    cif_value_free(element3);
    free(text1);
    free(keys);
    /* element1 and element2 both belong to the list value */
    /* element3 is invalid (freed) */

    /* test removing a value */
    TEST(cif_value_get_item_by_key(value, all_keys[0], &element3), CIF_OK, test_name, 108);
    TEST(cif_value_remove_item_by_key(value, all_keys[0], &element2), CIF_OK, test_name, 109);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 110);
    TEST(count, 2, test_name, 111);
    TEST(element2 != element3, 0, test_name, 112);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 113);
    TEST(test_key_list(all_keys + 1, 2, keys), 0, test_name, 114);
    free(keys);
    cif_value_free(element2);
    /* element1 belongs to the list value; element2 and element3 are invalid (freed) */

    /* test removing another value */
    TEST(cif_value_remove_item_by_key(value, all_keys[2], NULL), CIF_OK, test_name, 115);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 116);
    TEST(count, 1, test_name, 117);
    TEST(cif_value_get_item_by_key(value, all_keys[1], NULL), CIF_OK, test_name, 118);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 119);
    TEST(test_key_list(all_keys + 1, 1, keys), 0, test_name, 120);
    free(keys);
    /* element1 belongs to the list value; element2 and element3 are invalid (freed) */

    /* test removing the only value */
    TEST(cif_value_remove_item_by_key(value, all_keys[1], &element1), CIF_OK, test_name, 121);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 122);
    TEST(count, 0, test_name, 123);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 124);
    TEST(test_key_list(all_keys, 0, keys), 0, test_name, 125);
    free(keys);
    /* element1 belongs to the list value; element2 and element3 are invalid (freed) */

    /* test re-inserting the value, with a different key */
    TEST(cif_value_copy_char(element1, value_text3), CIF_OK, test_name, 126);
    TEST(cif_value_set_item_by_key(value, all_keys[2], element1), CIF_OK, test_name, 127);
    TEST(cif_value_get_item_by_key(value, all_keys[2], &element2), CIF_OK, test_name, 128);
    TEST(cif_value_get_text(element1, &text1), CIF_OK, test_name, 129);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 130);
    TEST(u_strcmp(text1, text2), 0, test_name, 131);
    free(text1);
    free(text2);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 132);
    TEST(count, 1, test_name, 133);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 134);
    TEST(test_key_list(all_keys + 2, 1, keys), 0, test_name, 135);
    free(keys);
    cif_value_free(element1);
    /* element2 belongs to the table */
    /* element1 and element3 are invalid (freed) */

    /* test key normalization */
    TEST(cif_value_create(CIF_UNK_KIND, &element1), CIF_OK, test_name, 136);
    TEST(cif_value_copy_char(element1, value_text), CIF_OK, test_name, 137);
    TEST(cif_value_set_item_by_key(value, uncomposed_key, element1), CIF_OK, test_name, 138);
    TEST(cif_value_get_item_by_key(value, uncomposed_key, &element2), CIF_OK, test_name, 139);
    TEST(cif_value_get_text(element2, &text2), CIF_OK, test_name, 140);
    TEST(u_strcmp(value_text, text2), CIF_OK, test_name, 141);
    free(text2);
    TEST(cif_value_get_keys(value, &keys), CIF_OK, test_name, 142);
    TEST(test_key_list(all_keys + 2, 2, keys), 0, test_name, 143);
    free(keys);
    TEST(cif_value_get_item_by_key(value, normalized_key, &element3), CIF_OK, test_name, 144);
    TEST(element2 != element3, 0, test_name, 145);
    TEST(cif_value_get_item_by_key(value, equivalent_key, &element3), CIF_OK, test_name, 146);
    TEST(element2 != element3, 0, test_name, 147);
    TEST(cif_value_get_item_by_key(value, equiv2_key, &element3), CIF_OK, test_name, 148);
    TEST(element2 != element3, 0, test_name, 149);
    /* test non-case-folding */
    TEST(cif_value_get_item_by_key(value, folded_key, &element3), CIF_NOSUCH_ITEM, test_name, 150);

    /* test NULL value pointer */
    TEST(cif_value_get_item_by_key(value, normalized_key, NULL), CIF_OK, test_name, 151);

    cif_value_free(element1);

    cif_value_free(value);

    return 0;
}

/*
 * tests whether the 'observed' argument points to the first element of a NULL-terminated array of the expected
 * number of NUL-terminated Unicode strings, each of which is a character-by-character match to one of the 'expected'
 * Unicode strings, but not necessarily in the same order.
 */
static int test_key_list(UChar **expected, int num_expected, const UChar **observed) {
    int seen = 0;
    int i;
    int j;

    for (i = 0; i < num_expected; i += 1) {
        if (observed[i] == NULL) return -(i + 1);
    }
    if (observed[i] != NULL) return -(i + 1);

    for (i = 0; i < num_expected; i += 1) {
        for (j = 0; j < num_expected; j += 1) {
            if (u_strcmp(expected[i], observed[j]) == 0) goto found;
        }

        /* not found */
        return (i + 1);

        found:
        /* make sure this isn't a duplicate */
        if ((seen & (1 << i)) != 0) {
            return (i + 1);
        } else {
            seen |= (1 << i);
        }
    }

    return 0;
}

