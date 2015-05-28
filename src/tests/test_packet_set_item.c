/*
 * test_packet_set_item.c
 *
 * Tests the CIF API's cif_packet_set_item() function, and incidentally
 * tests other packet functions including cif_packet_get_item() and
 * cif_packet_get_names().
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

static int test_name_list(UChar **expected, int num_expected, const UChar **observed);

int main(void) {
    char test_name[80] = "test_packet_set_item";
    cif_packet_tp *packet = NULL;
    cif_value_tp *value = NULL;
    cif_value_tp *value2 = NULL;
    const UChar **item_names = NULL;
    UChar *used_names[5] = { NULL, NULL, NULL, NULL, NULL };
    UChar *text = NULL;
    UChar uncomposed_name[6] = { 0x5f, 'K', 0x0073, 0x0307, 0x0323, 0 };
    UChar equivalent_name[6] = { 0x5f, 'K', 0x0073, 0x0323, 0x0307, 0 };
    UChar simple_name[] = { '_', 'n', 'a', 'm', 'e', 0 };
    UChar invalid_name[] = { 'n', 'a', 'm', 'e', 0 };
    UChar another_name[] = { '_', 'a', 'n', 'o', 't', 'h', 'e', 'r', '.', 'n', 'a', 'm', 'e', 0 };
    U_STRING_DECL(value_text, "Value teXt", 11);

    U_STRING_INIT(value_text, "Value teXt", 11);

    TESTHEADER(test_name);

    /* Start with an empty packet */ 
    TEST(cif_packet_create(&packet, used_names), CIF_OK, test_name, 1);

    /* Test setting the first item in an empty packet */
    TEST(cif_packet_set_item(packet, simple_name, NULL), CIF_OK, test_name, 2);
    used_names[0] = simple_name;
    TEST(cif_packet_get_names(packet, &item_names), CIF_OK, test_name, 3);
    TEST(item_names == NULL, 0, test_name, 4);
    TEST(test_name_list(used_names, 1, item_names), 0, test_name, 5);
    free(item_names);
    TEST(cif_packet_get_item(packet, simple_name, &value), CIF_OK, test_name, 6);
    TEST(value == NULL, 0, test_name, 7);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 8);
    /* value belongs to the packet */

    /* Test setting an invalid data name */
    TEST(cif_packet_set_item(packet, invalid_name, NULL), CIF_INVALID_ITEMNAME, test_name, 9);
    
    /* Test setting an un-normalized name */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 10);
    TEST(cif_value_copy_char(value, value_text), CIF_OK, test_name, 11);
    TEST(cif_packet_set_item(packet, uncomposed_name, value), CIF_OK, test_name, 12);
    used_names[1] = uncomposed_name;
    TEST(cif_packet_get_names(packet, &item_names), CIF_OK, test_name, 13);
    TEST(item_names == NULL, 0, test_name, 14);
    TEST(test_name_list(used_names, 2, item_names), 0, test_name, 15);
    free(item_names);
    TEST(cif_packet_get_item(packet, uncomposed_name, &value2), CIF_OK, test_name, 16);
    TEST(value2 == NULL, 0, test_name, 17);
    TEST(value2 == value, 0, test_name, 18);
    TEST(cif_value_kind(value2), CIF_CHAR_KIND, test_name, 19);
    TEST(cif_value_get_text(value2, &text), CIF_OK, test_name, 20);
    TEST(u_strcmp(value_text, text), 0, test_name, 21);
    cif_value_free(value);  /* free the _original_ value */
    free(text);
    /* value is invalid (freed); value2 belongs to the packet */

    /* one more item */
    TEST(cif_packet_set_item(packet, another_name, NULL), CIF_OK, test_name, 22);
    used_names[2] = another_name;

    /* test setting an item value to itself */
    TEST(cif_packet_set_item(packet, equivalent_name, value2), CIF_OK, test_name, 23);
    TEST(cif_packet_get_item(packet, uncomposed_name, &value), CIF_OK, test_name, 24);
    TEST(value != value2, 0, test_name, 25);
    TEST(cif_value_kind(value2), CIF_CHAR_KIND, test_name, 26);
    TEST(cif_value_get_text(value2, &text), CIF_OK, test_name, 27);
    TEST(u_strcmp(value_text, text), 0, test_name, 28);
    free(text);
    /* value and value2 both belong to the packet */

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

