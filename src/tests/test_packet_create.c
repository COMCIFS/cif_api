/*
 * test_packet_create.c
 *
 * Tests the CIF API's packet creation and inquiry functions
 * cif_packet_create(), cif_packet_get_names(), and cif_packet_get_item().
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
#include <assert.h>
#include "../cif.h"
#include "test.h"

static int test_name_list(UChar **expected, const UChar **observed);

int main(void) {
    char test_name[80] = "test_packet_create";
    cif_packet_tp *packet = NULL;
    cif_value_tp *value = NULL;
    UChar empty_name[1] = { 0 };
    UChar invalid_name1[5] = { 0x5F, 0x4B, 0xFFFF, 0x79, 0 };
    UChar invalid_name2[5] = { 0x5F, 0x4B, 0x20, 0x79, 0 };
    UChar name1[5] = { 0x5F, 0x4B, 0x45, 0x79, 0 };
    UChar name2[7] = { 0x5F, 0x56, 0x61, 0x6c, 0x75, 0x65, 0 };
    UChar uncomposed_name[6] = { 0x5F, 0x4B, 0x0073, 0x0307, 0x0323, 0 };
    UChar *all_names[4];
    const UChar **names;
    int counter;

    TESTHEADER(test_name);

    /* Create an empty packet */ 
    TEST(cif_packet_create(&packet, NULL), CIF_OK, test_name, 1);
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 2);
    TEST(names == NULL, 0, test_name, 3);
    TEST(names[0] != NULL, 0, test_name, 4);
    free(names);
    cif_packet_free(packet);

    /* Create an empty packet, version 2 */
    all_names[0] = NULL;
    TEST(cif_packet_create(&packet, all_names), CIF_OK, test_name, 5);
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 6);
    TEST(names == NULL, 0, test_name, 7);
    TEST(names[0] != NULL, 0, test_name, 8);
    free(names);
    cif_packet_free(packet);

    /* Test invalid names */
    all_names[0] = name1;
    all_names[1] = empty_name;
    all_names[2] = NULL;
    TEST(cif_packet_create(&packet, all_names), CIF_INVALID_ITEMNAME, test_name, 9);

    all_names[1] = name2;
    all_names[2] = invalid_name1;
    all_names[3] = NULL;
    TEST(cif_packet_create(&packet, all_names), CIF_INVALID_ITEMNAME, test_name, 10);

    all_names[0] = invalid_name2;
    all_names[2] = uncomposed_name;
    TEST(cif_packet_create(&packet, all_names), CIF_INVALID_ITEMNAME, test_name, 11);

    /* Create a non-empty packet */
    all_names[0] = name1;
    TEST(cif_packet_create(&packet, all_names), CIF_OK, test_name, 12);
    TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, 13);
    TEST(test_name_list(all_names, names), 0, test_name, 14);
    free(names);

    for (counter = 0; all_names[counter] != NULL; counter += 1) {
        TEST(cif_packet_get_item(packet, all_names[counter], &value), CIF_OK, test_name, 2 * counter + 15);
        TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 2 * counter + 16);
    }

    cif_packet_free(packet);

    return 0;
}

/*
 * tests whether the 'observed' argument points to the first element of a NULL-terminated array of the expected
 * number of NUL-terminated Unicode strings, each of which is a character-by-character match to the corresponding
 * 'expected' Unicode string.
 */
static int test_name_list(UChar **expected, const UChar **observed) {
    UChar **first_name = expected;
    assert(expected != NULL);
    if (observed == NULL) return -1;

    for (; ; expected += 1, observed += 1) {
        if (*expected == NULL) {
            return ((*observed == NULL) ? 0 : (expected - first_name) + 1);
        } else if ((*observed == NULL) || (u_strcmp(*expected, *observed) != 0)) {
            return (expected - first_name) + 1;
        }
    }

    /* not reached: */
    return 0;
}

