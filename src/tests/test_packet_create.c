/*
 * test_packet_create.c
 *
 * Tests the CIF API's table value manipulation functions
 * cif_value_get_item_by_key(), cif_value_set_item_by_key(),
 * cif_value_remove_item_by_key(), cif_value_get_keys(),
 * and cif_value_get_element_count().
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_packet_create";
    cif_packet_t *packet = NULL;
    cif_value_t *value = NULL;
    UChar *names[5];
    UChar **names2;
    int counter;
    U_STRING_DECL(name0, "_name1", 7);
    U_STRING_DECL(name1, "_cATegory.Item2", 16);
    U_STRING_DECL(name2, "_Cat_NAME_3#.item2", 19);
    U_STRING_DECL(name3, "__", 3);
    U_STRING_DECL(name1_norm, "_category.item2", 16);
    U_STRING_DECL(invalid_name, "no_initial_undersc", 19);

    U_STRING_INIT(name0, "_name1", 7);
    U_STRING_INIT(name1, "_cATegory.Item2", 16);
    U_STRING_INIT(name2, "_Cat_NAME_3#.item2", 19);
    U_STRING_INIT(name3, "__", 3);
    U_STRING_INIT(name1_norm, "_category.item2", 16);
    U_STRING_INIT(invalid_name, "no_initial_undersc", 19);

    TESTHEADER(test_name);

    /* Test creating a packet with zero names */
    names[0] = NULL;
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 1);
    TEST(packet == NULL, 0, test_name, 2);
    TEST(cif_packet_get_names(packet, &names2), CIF_OK, test_name, 3);
    TEST(names2 == NULL, 0, test_name, 4);
    TEST(names2[0] != NULL, 0, test_name, 5);
    free(names2);
    cif_packet_free(packet);

    /* Test creating a packet with several names */
    names[0] = name0;
    names[1] = name1;
    names[2] = name2;
    names[3] = name3;
    names[4] = NULL;
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 6);
    TEST(packet == NULL, 0, test_name, 7);
    TEST(cif_packet_get_names(packet, &names2), CIF_OK, test_name, 8);
    TEST(names2 == NULL, 0, test_name, 9);
    for (counter = 0; counter < 4; counter += 1) {
        TEST(names2[counter] == NULL, 0, test_name, 10 + 2 * counter);
        TEST(u_strcmp(names[counter], names2[counter]), 0, test_name, 11 + 2 * counter);
    }  /* last test number was 17 */
    TEST(names2[4] != NULL, 0, test_name, 18);
    /* per the docs, elements of the names2 array must not be modified or freed */
    free(names2);                         /* but the array itself must be freed */
    for (counter = 0; counter < 4; counter += 1) {
        TEST(cif_packet_get_item(packet, names[counter], &value), CIF_OK, test_name, 19 + 3 * counter);
        TEST(value == NULL, 0, test_name, 20 + 3 * counter);
        TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 21 + 3 * counter);
    }  /* last test number was 30 */

    /* Test creation-time name normalization */
    TEST(cif_packet_get_item(packet, name1_norm, NULL), CIF_OK, test_name, 31);

    cif_packet_free(packet);

    /* Test creating a packet containing an invalid name */
    names[1] = invalid_name;
    TEST(cif_packet_create(&packet, names), CIF_INVALID_ITEMNAME, test_name, 32);

    return 0;
}

