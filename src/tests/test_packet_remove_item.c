/*
 * test_packet_remove_item.c
 *
 * Tests the CIF API's cif_packet_remove_item() function, and incidentally
 * tests other packet functions including cif_packet_get_item() and
 * cif_packet_get_names().
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

static int test_name_list(UChar **expected, int num_expected, const UChar **observed);

int main(void) {
    char test_name[80] = "test_packet_remove_item";
    cif_packet_tp *packet = NULL;
    cif_value_tp *value = NULL;
    cif_value_tp *value2 = NULL;
    const UChar **item_names = NULL;
    UChar *used_names[5] = { NULL, NULL, NULL, NULL, NULL };
    UChar *text = NULL;
    UChar uncomposed_name[6] = { 0x5f, 'K', 0x0073, 0x0307, 0x0323, 0 };
    UChar equivalent_name[6] = { 0x5f, 'k', 0x0073, 0x0323, 0x0307, 0 };
    U_STRING_DECL(simple_name, "_name", 6);
    U_STRING_DECL(invalid_name, "name", 5);
    U_STRING_DECL(another_name, "_aNotheR.name", 14);
    U_STRING_DECL(third_name, "_a_#third#.$name", 17);
    U_STRING_DECL(text1, "one", 4);
    U_STRING_DECL(text2, "two", 4);
    U_STRING_DECL(text3, "three", 6);
    U_STRING_DECL(text4, "four", 5);

    U_STRING_INIT(simple_name, "_name", 6);
    U_STRING_INIT(invalid_name, "name", 5);
    U_STRING_INIT(another_name, "_aNotheR.name", 14);
    U_STRING_INIT(third_name, "_a_#third#.$name", 17);
    U_STRING_INIT(text1, "one", 4);
    U_STRING_INIT(text2, "two", 4);
    U_STRING_INIT(text3, "three", 6);
    U_STRING_INIT(text4, "four", 5);

    TESTHEADER(test_name);

    /* Start with an empty packet */ 
    TEST(cif_packet_create(&packet, used_names), CIF_OK, test_name, 1);

    /* Add several items to the packet */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 2);
    TEST(cif_value_copy_char(value, text1), CIF_OK, test_name, 3);
    TEST(cif_packet_set_item(packet, simple_name, value), CIF_OK, test_name, 4);
    used_names[0] = simple_name;

    TEST(cif_value_copy_char(value, text2), CIF_OK, test_name, 5);
    TEST(cif_packet_set_item(packet, another_name, value), CIF_OK, test_name, 6);
    used_names[1] = another_name;

    TEST(cif_value_copy_char(value, text3), CIF_OK, test_name, 7);
    TEST(cif_packet_set_item(packet, third_name, value), CIF_OK, test_name, 8);
    used_names[2] = third_name;

    TEST(cif_value_copy_char(value, text4), CIF_OK, test_name, 9);
    TEST(cif_packet_set_item(packet, equivalent_name, value), CIF_OK, test_name, 10);
    used_names[3] = equivalent_name;

    TEST(cif_packet_get_names(packet, &item_names), CIF_OK, test_name, 11);
    TEST(test_name_list(used_names, 4, item_names), 0, test_name, 12);
    free(item_names);

    /* Test removing via an invalid name */
    TEST(cif_packet_remove_item(packet, invalid_name, NULL), CIF_NOSUCH_ITEM, test_name, 13);

    /* Test removing the first item added */
    TEST(cif_packet_remove_item(packet, used_names[0], &value2), CIF_OK, test_name, 14);
    TEST(value2 == NULL, 0, test_name, 15);
    TEST(cif_value_get_text(value2, &text), CIF_OK, test_name, 16);
    TEST(text == NULL, 0, test_name, 17);
    TEST(u_strcmp(text1, text), 0, test_name, 18);
    free(text);
    cif_value_free(value2);
    TEST(cif_packet_remove_item(packet, used_names[0], NULL), CIF_NOSUCH_ITEM, test_name, 19);
    TEST(cif_packet_get_names(packet, &item_names), CIF_OK, test_name, 20);
    TEST(test_name_list(used_names + 1, 3, item_names), 0, test_name, 21);
    free(item_names);

    /* Test removing a middle item */
    TEST(cif_packet_remove_item(packet, used_names[2], &value2), CIF_OK, test_name, 22);
    TEST(value2 == NULL, 0, test_name, 23);
    TEST(cif_value_get_text(value2, &text), CIF_OK, test_name, 24);
    TEST(text == NULL, 0, test_name, 25);
    TEST(u_strcmp(text3, text), 0, test_name, 26);
    free(text);
    cif_value_free(value2);
    TEST(cif_packet_remove_item(packet, used_names[2], NULL), CIF_NOSUCH_ITEM, test_name, 27);

    /* Test removing the last-inserted item */
    TEST(cif_packet_remove_item(packet, uncomposed_name, &value2), CIF_OK, test_name, 28);
    TEST(value2 == NULL, 0, test_name, 29);
    TEST(value2 == value, 0, test_name, 30);
    TEST(cif_value_get_text(value2, &text), CIF_OK, test_name, 31);
    TEST(text == NULL, 0, test_name, 32);
    TEST(u_strcmp(text4, text), 0, test_name, 33);
    free(text);
    cif_value_free(value2);
    cif_value_free(value);
    TEST(cif_packet_remove_item(packet, used_names[3], NULL), CIF_NOSUCH_ITEM, test_name, 34);

    /* Test removing the only remaining item */
    TEST(cif_packet_remove_item(packet, used_names[1], &value2), CIF_OK, test_name, 35);
    TEST(value2 == NULL, 0, test_name, 36);
    TEST(cif_value_get_text(value2, &text), CIF_OK, test_name, 37);
    TEST(text == NULL, 0, test_name, 38);
    TEST(u_strcmp(text2, text), 0, test_name, 39);
    free(text);
    cif_value_free(value2);
    TEST(cif_packet_remove_item(packet, used_names[1], NULL), CIF_NOSUCH_ITEM, test_name, 40);
    TEST(cif_packet_get_names(packet, &item_names), CIF_OK, test_name, 41);
    TEST(test_name_list(used_names, 0, item_names), 0, test_name, 42);
    free(item_names);

    cif_packet_free(packet);

    return 0;
}

/*
 * tests whether the 'observed' argument points to the first element of a NULL-terminated array of the expected
 * number of NUL-terminated Unicode strings, each of which is a character-by-character match to one of the 'expected'
 * Unicode strings, but not necessarily in the same order.
 */
static int test_name_list(UChar **expected, int num_expected, const UChar **observed) {
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

