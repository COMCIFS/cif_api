/*
 * test_container_get_value.c
 *
 * Tests behaviors of the CIF API's cif_container_get_value() function not
 * already adequately exercised via other tests.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_container_get_value";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp *loop;
    cif_packet_tp *packet;
    cif_value_tp *value1;
    cif_value_tp *value2;
    cif_value_tp *value3;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(item1l, "_item1", 7);
    U_STRING_DECL(item2l, "_item2", 7);
    U_STRING_DECL(item3l, "_item3", 7);
    U_STRING_DECL(item4l, "_item4", 7);
    U_STRING_DECL(item1u, "_Item1", 7);
    U_STRING_DECL(item2u, "_ITEM2", 7);
    U_STRING_DECL(item3u, "_iTeM3", 7);
    U_STRING_DECL(invalid, "in valid", 9);
    U_STRING_DECL(char_value1, "simple_Value", 13);
    UChar *names[4];
    size_t count;

    /* Initialize data and prepare the test fixture */

    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(item1l, "_item1", 7);
    U_STRING_INIT(item2l, "_item2", 7);
    U_STRING_INIT(item3l, "_item3", 7);
    U_STRING_INIT(item4l, "_item4", 7);
    U_STRING_INIT(item1u, "_Item1", 7);
    U_STRING_INIT(item2u, "_ITEM2", 7);
    U_STRING_INIT(item3u, "_iTeM3", 7);
    U_STRING_INIT(invalid, "in valid", 9);
    U_STRING_INIT(char_value1, "simple_Value", 13);
    names[0] = item1u;
    names[1] = item2u;
    names[2] = item3u;
    names[3] = NULL;
    

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    TEST(cif_container_create_loop(block, NULL, names, &loop), CIF_OK, test_name, 1);
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 2);

    /* Set a scalar value (of kind CIF_UNK_KIND) for item2 in the save frame */
    TEST(cif_container_set_value(frame, item2l, NULL), CIF_OK, test_name, 3);

    /* Test requesting a value from a zero-packet loop */
    value1 = NULL;
    TEST(cif_value_create(CIF_LIST_KIND, &value2), CIF_OK, test_name, 4);
    TEST(cif_container_get_value(block, item2l, NULL), CIF_NOSUCH_ITEM, test_name, 5);
    TEST(cif_container_get_value(block, item2l, &value1), CIF_NOSUCH_ITEM, test_name, 6);
    TEST(value1 != NULL, 0, test_name, 7);
    TEST(cif_container_get_value(block, item2l, &value2), CIF_NOSUCH_ITEM, test_name, 8);
    TEST(cif_value_kind(value2), CIF_LIST_KIND, test_name, 9);
    TEST(cif_value_get_element_count(value2, &count), CIF_OK, test_name, 10);
    TEST(count, 0, test_name, 11);

    /* Test requesting a value from a single-packet, non-scalar loop */
    TEST(cif_packet_get_item(packet, item1u, &value1), CIF_OK, test_name, 12);
    TEST(cif_value_init(value1, CIF_NA_KIND), CIF_OK, test_name, 13);
    TEST(cif_packet_get_item(packet, item2u, &value1), CIF_OK, test_name, 14);
    TEST(cif_value_copy_char(value1, char_value1), CIF_OK, test_name, 15);
    TEST(cif_value_clone(value1, &value2), CIF_OK, test_name, 16);  /* clone the value for item2 */
    TEST(cif_packet_get_item(packet, item3u, &value1), CIF_OK, test_name, 17);
    TEST(cif_value_copy_char(value1, item3u), CIF_OK, test_name, 18);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 19);
    value1 = NULL;
    TEST(cif_container_get_value(block, item2l, NULL), CIF_OK, test_name, 20);
    TEST(cif_container_get_value(block, item2l, &value1), CIF_OK, test_name, 21);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 22);
    TEST(cif_value_init(value1, CIF_UNK_KIND), CIF_OK, test_name, 23);
    TEST(cif_value_kind(value1), CIF_UNK_KIND, test_name, 24);
    TEST(cif_container_get_value(block, item2l, &value1), CIF_OK, test_name, 25);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 26);

    /* Test requesting a value for a valid but absent item name */
    cif_value_free(value1);
    value1 = NULL;
    TEST(cif_value_create(CIF_LIST_KIND, &value3), CIF_OK, test_name, 27);
    TEST(cif_container_get_value(block, item4l, NULL), CIF_NOSUCH_ITEM, test_name, 28);
    TEST(cif_container_get_value(block, item4l, &value1), CIF_NOSUCH_ITEM, test_name, 29);
    TEST(value1 != NULL, 0, test_name, 30);
    TEST(cif_container_get_value(block, item4l, &value3), CIF_NOSUCH_ITEM, test_name, 31);
    TEST(cif_value_kind(value3), CIF_LIST_KIND, test_name, 32);
    TEST(cif_value_get_element_count(value3, &count), CIF_OK, test_name, 33);
    TEST(count, 0, test_name, 34);

    /* Test requesting a value for an invalid (and therefore certainly absent) item name */
    TEST(cif_container_get_value(block, invalid, NULL), CIF_NOSUCH_ITEM, test_name, 35);
    TEST(cif_container_get_value(block, invalid, &value1), CIF_NOSUCH_ITEM, test_name, 36);
    TEST(value1 != NULL, 0, test_name, 37);
    TEST(cif_container_get_value(block, invalid, &value3), CIF_NOSUCH_ITEM, test_name, 38);
    TEST(cif_value_kind(value3), CIF_LIST_KIND, test_name, 39);
    TEST(cif_value_get_element_count(value3, &count), CIF_OK, test_name, 40);
    TEST(count, 0, test_name, 41);
    cif_value_free(value3);

    /* Test requesting a value from a multi-packet loop */
    TEST(cif_packet_get_item(packet, item1l, &value1), CIF_OK, test_name, 42);
    TEST(cif_value_init_numb(value1, 1.0, 0.0, 2, 1), CIF_OK, test_name, 43);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 44);
    TEST(cif_value_init_numb(value1, 2.0, 0.0, 2, 1), CIF_OK, test_name, 45);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 46);
    TEST(cif_value_init_numb(value1, 3.0, 0.0, 2, 1), CIF_OK, test_name, 47);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 48);
    value1 = NULL;

    TEST(cif_container_get_value(block, item2l, NULL), CIF_AMBIGUOUS_ITEM, test_name, 49);
    TEST(cif_container_get_value(block, item2l, &value1), CIF_AMBIGUOUS_ITEM, test_name, 50);
    TEST(value1 == NULL, 0, test_name, 51);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 52);
    TEST(cif_value_init(value1, CIF_NA_KIND), CIF_OK, test_name, 53);
    TEST(cif_container_get_value(block, item2l, &value1), CIF_AMBIGUOUS_ITEM, test_name, 54);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 55);

    cif_value_free(value2);
    cif_value_free(value1);
    cif_packet_free(packet);
    cif_loop_free(loop);
    cif_frame_free(frame);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

