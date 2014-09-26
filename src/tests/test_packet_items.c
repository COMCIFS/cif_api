/*
 * test_packet_items.c
 *
 * Tests the CIF API's packet creation and manipulation functions
 * cif_packet_create(), cif_packet_get_names(), cif_packet_set_item(),
 * cif_packet_get_item(), and cif_packet_remove_item().
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

static int test_name_list(UChar **expected, const UChar **observed);

int main(void) {
    char test_name[80] = "test_packet_items";
    cif_packet_t *packet = NULL;
    cif_value_t *value = NULL;
    cif_value_t *value1 = NULL;
    cif_value_t *value2 = NULL;
    cif_value_t *value3 = NULL;
    UChar invalid_name1[5] = { 0x5F, 0x4B, 0xFFFF, 0x79, 0 };
    UChar invalid_name2[5] = { 0x5F, 0x4B, 0x20, 0x79, 0 };
    UChar name1[5] = { 0x5F, 0x4B, 0x45, 0x79, 0 };
    UChar name2[7] = { 0x5F, 0x56, 0x61, 0x6c, 0x75, 0x65, 0 };
    UChar uncomposed_name[6] = { 0x5F, 0x4B, 0x0073, 0x0307, 0x0323, 0 };
    UChar equivalent_name[5] = { 0x5F, 0x4B, 0x1E61, 0x323, 0 };
    UChar *all_names[4] = { NULL, NULL, NULL, NULL };
    const UChar **names;
    U_STRING_DECL(char_value, "I am a value", 13);

    U_STRING_INIT(char_value, "I am a value", 13);

    TESTHEADER(test_name);

    /* Create an empty packet */ 
    TEST(cif_packet_create(&packet, NULL), CIF_OK, test_name, 1);

    /* set a new item and read it back (total 1) */
    TEST(cif_value_create(CIF_UNK_KIND, &value1), CIF_OK, test_name, 2);
    TEST(cif_value_copy_char(value1, char_value), CIF_OK, test_name, 3);
    TEST(cif_packet_set_item(packet, name1, value1), CIF_OK, test_name, 4);
    all_names[0] = name1;
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 5);
    TEST(test_name_list(all_names, names), 0, test_name, 6);
    free(names);
    TEST(cif_packet_get_item(packet, name1, &value), CIF_OK, test_name, 7);
    TEST((value1 == value), 0, test_name, 8);
    TEST(!assert_values_equal(value1, value), 0, test_name, 9);

    /* set a new item and read it back (total 1) */
    TEST(cif_value_create(CIF_UNK_KIND, &value2), CIF_OK, test_name, 10);
    TEST(cif_value_init_numb(value2, 42.0, 0.25, 2, 2), CIF_OK, test_name, 11);
    TEST(cif_packet_set_item(packet, name2, value2), CIF_OK, test_name, 12);
    all_names[1] = name2;
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 13);
    TEST(test_name_list(all_names, names), 0, test_name, 14);
    free(names);
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 15);
    TEST((value2 == value), 0, test_name, 16);
    TEST(!assert_values_equal(value2, value), 0, test_name, 17);
      /* verify that the previous value is unchanged */
    TEST(cif_packet_get_item(packet, name1, &value), CIF_OK, test_name, 18);
    TEST(!assert_values_equal(value1, value), 0, test_name, 19);

    /* set a new item and read it back (total 3) */
    TEST(cif_value_create(CIF_LIST_KIND, &value3), CIF_OK, test_name, 20);
    TEST(cif_packet_set_item(packet, uncomposed_name, value3), CIF_OK, test_name, 21);
    all_names[2] = uncomposed_name;
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 22);
    TEST(test_name_list(all_names, names), 0, test_name, 23);
    free(names);
    TEST(cif_packet_get_item(packet, equivalent_name, &value), CIF_OK, test_name, 24);
    TEST((value3 == value), 0, test_name, 25);
    TEST(!assert_values_equal(value3, value), 0, test_name, 26);
      /* verify that the previous values are unchanged */
    TEST(cif_packet_get_item(packet, name1, &value), CIF_OK, test_name, 27);
    TEST(!assert_values_equal(value1, value), 0, test_name, 28);
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 29);
    TEST(!assert_values_equal(value2, value), 0, test_name, 30);

    /* modify an existing item and read it back (total 3) */
    TEST(cif_value_init(value1, CIF_TABLE_KIND), CIF_OK, test_name, 31);
    TEST(cif_packet_set_item(packet, name1, value1), CIF_OK, test_name, 32);
      /* the name list shouldn't change in any way */
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 33);
    TEST(test_name_list(all_names, names), 0, test_name, 34);
    free(names);
    TEST(cif_packet_get_item(packet, name1, &value), CIF_OK, test_name, 35);
    TEST((value1 == value), 0, test_name, 36);
    TEST(!assert_values_equal(value1, value), 0, test_name, 37);
      /* verify that the other values are unchanged */
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 38);
    TEST(!assert_values_equal(value2, value), 0, test_name, 39);
    TEST(cif_packet_get_item(packet, uncomposed_name, &value), CIF_OK, test_name, 40);
    TEST(!assert_values_equal(value3, value), 0, test_name, 41);

    /* modify another item and read it back (total 3) */
    TEST(cif_value_init(value3, CIF_NA_KIND), CIF_OK, test_name, 42);
    TEST(cif_packet_set_item(packet, equivalent_name, value3), CIF_OK, test_name, 43);
      /* the alternative name form should now be listed among the item names */
    all_names[2] = equivalent_name;
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 44);
    TEST(test_name_list(all_names, names), 0, test_name, 45);
    free(names);
    TEST(cif_packet_get_item(packet, equivalent_name, &value), CIF_OK, test_name, 46);
    TEST((value3 == value), 0, test_name, 47);
    TEST(!assert_values_equal(value3, value), 0, test_name, 48);
      /* verify that the other values are unchanged */
    TEST(cif_packet_get_item(packet, name1, &value), CIF_OK, test_name, 49);
    TEST(!assert_values_equal(value1, value), 0, test_name, 50);
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 51);
    TEST(!assert_values_equal(value2, value), 0, test_name, 52);

    /* test invalid names */
    TEST(cif_packet_set_item(packet, invalid_name1, value1), CIF_INVALID_ITEMNAME, test_name, 53);
    TEST(cif_packet_set_item(packet, invalid_name2, value1), CIF_INVALID_ITEMNAME, test_name, 54);
    TEST(cif_packet_get_item(packet, invalid_name1, &value), CIF_NOSUCH_ITEM, test_name, 55);
    TEST(cif_packet_get_item(packet, invalid_name2, &value), CIF_NOSUCH_ITEM, test_name, 56);
    TEST(cif_packet_remove_item(packet, invalid_name1, NULL), CIF_NOSUCH_ITEM, test_name, 57);
    TEST(cif_packet_remove_item(packet, invalid_name2, NULL), CIF_NOSUCH_ITEM, test_name, 58);
      /* verify that the values are unchanged */
    TEST(cif_packet_get_item(packet, name1, &value), CIF_OK, test_name, 59);
    TEST(!assert_values_equal(value1, value), 0, test_name, 60);
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 61);
    TEST(!assert_values_equal(value2, value), 0, test_name, 62);
    TEST(cif_packet_get_item(packet, uncomposed_name, &value), CIF_OK, test_name, 63);
    TEST(!assert_values_equal(value3, value), 0, test_name, 64);

    /* remove the first item (2 left) */
    value = NULL;
    TEST(cif_packet_remove_item(packet, name1, &value), CIF_OK, test_name, 65);
    TEST(!assert_values_equal(value1, value), 0, test_name, 66);
    cif_value_free(value);
    cif_value_free(value1);
    TEST(cif_packet_get_item(packet, name1, &value), CIF_NOSUCH_ITEM, test_name, 67);
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 68);
    TEST(test_name_list(all_names + 1, names), 0, test_name, 69);
    free(names);
      /* verify that the other values are unchanged */
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 70);
    TEST(!assert_values_equal(value2, value), 0, test_name, 71);
    TEST(cif_packet_get_item(packet, uncomposed_name, &value), CIF_OK, test_name, 72);
    TEST(!assert_values_equal(value3, value), 0, test_name, 73);

    /* remove the last-added item (1 left) */
    value = NULL;
    TEST(cif_packet_remove_item(packet, uncomposed_name, NULL), CIF_OK, test_name, 74);
    all_names[2] = NULL;
    cif_value_free(value3);
    TEST(cif_packet_get_item(packet, uncomposed_name, &value), CIF_NOSUCH_ITEM, test_name, 75);
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 76);
    TEST(test_name_list(all_names + 1, names), 0, test_name, 77);
    free(names);
      /* verify that the other value is unchanged */
    TEST(cif_packet_get_item(packet, name2, &value), CIF_OK, test_name, 78);
    TEST(!assert_values_equal(value2, value), 0, test_name, 79);

    /* remove the final item */
    TEST(cif_packet_remove_item(packet, name2, NULL), CIF_OK, test_name, 80);
    all_names[1] = NULL;
    cif_value_free(value2);
    TEST(cif_packet_get_item(packet, name2, &value), CIF_NOSUCH_ITEM, test_name, 81);
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 82);
    TEST(test_name_list(all_names + 1, names), 0, test_name, 83);
    free(names);

    cif_packet_free(packet);

    return 0;
}

/*
 * tests whether the 'observed' argument points to the first element of a NULL-terminated array of the expected
 * number of NUL-terminated Unicode strings, each of which is a character-by-character match to the corresponding
 * 'expected' Unicode string.
 */
static int test_name_list(UChar **expected, const UChar **observed) {
    assert(expected != NULL);
    if (observed == NULL) return -1;

    for (; ; expected += 1, observed += 1) {
        if (*expected == NULL) {
            return ((*observed == NULL) ? 0 : 1);
        } else if ((*observed == NULL) || (u_strcmp(*expected, *observed) != 0)) {
            return 1;
        }
    }

    /* not reached: */
    return 0;
}

