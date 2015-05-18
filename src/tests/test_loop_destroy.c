/*
 * test_loop_destroy.c
 *
 * Tests behavior of the CIF API's functions for deleting (destroying)
 * whole loops, together with all their packets
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_loop_destroy";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp *loop;
    cif_loop_tp *loop1;
    cif_loop_tp *loop2;
    cif_packet_tp *packet;
    cif_value_tp *value1;
    cif_value_tp *value2;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(item1l, "_item1", 7);
    U_STRING_DECL(item2l, "_item2", 7);
    U_STRING_DECL(item3l, "_item3", 7);
    UChar *item_names[4];

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(item1l, "_item1", 7);
    U_STRING_INIT(item2l, "_item2", 7);
    U_STRING_INIT(item3l, "_item3", 7);

    item_names[0] = item1l;
    item_names[1] = item2l;
    item_names[2] = item3l;
    item_names[3] = NULL;

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    /* Verify that the test loops do not initially exist */
    TEST(cif_container_get_item_loop(block, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 1);
    TEST(cif_container_get_item_loop(block, item2l, NULL), CIF_NOSUCH_ITEM, test_name, 2);
    TEST(cif_container_get_item_loop(block, item3l, NULL), CIF_NOSUCH_ITEM, test_name, 3);
    TEST(cif_container_get_item_loop(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 4);
    TEST(cif_container_get_item_loop(frame, item2l, NULL), CIF_NOSUCH_ITEM, test_name, 5);
    TEST(cif_container_get_item_loop(frame, item3l, NULL), CIF_NOSUCH_ITEM, test_name, 6);

    /* Create the loops, and verify that they exist */
    TEST(cif_container_create_loop(frame, NULL, item_names + 1, &loop2), CIF_OK, test_name, 7);
    item_names[2] = NULL;
    TEST(cif_container_create_loop(block, NULL, item_names, &loop1), CIF_OK, test_name, 8);
    TEST(cif_container_get_item_loop(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 9);
    TEST(cif_container_get_item_loop(frame, item2l, NULL), CIF_OK, test_name, 10);
    TEST(cif_container_get_item_loop(frame, item3l, NULL), CIF_OK, test_name, 11);
    TEST(cif_container_get_item_loop(block, item1l, NULL), CIF_OK, test_name, 12);
    TEST(cif_container_get_item_loop(block, item2l, NULL), CIF_OK, test_name, 13);
    TEST(cif_container_get_item_loop(block, item3l, NULL), CIF_NOSUCH_ITEM, test_name, 14);

    /* Destroy loop1 (belonging to the block); check that it is destroyed and the other is not */
    TEST(cif_loop_destroy(loop1), CIF_OK, test_name, 15);
    TEST(cif_container_get_item_loop(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 16);
    TEST(cif_container_get_item_loop(frame, item2l, NULL), CIF_OK, test_name, 17);
    TEST(cif_container_get_item_loop(frame, item3l, NULL), CIF_OK, test_name, 18);
    TEST(cif_container_get_item_loop(block, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 19);
    TEST(cif_container_get_item_loop(block, item2l, NULL), CIF_NOSUCH_ITEM, test_name, 20);
    TEST(cif_container_get_item_loop(block, item3l, NULL), CIF_NOSUCH_ITEM, test_name, 21);

    /* Recreate loop1, this time with some data, then delete it again */
    TEST(cif_container_create_loop(block, NULL, item_names, &loop1), CIF_OK, test_name, 22);
    TEST(cif_packet_create(&packet, NULL), CIF_OK, test_name, 23);
    TEST(cif_packet_set_item(packet, item_names[0], NULL), CIF_OK, test_name, 24);
    TEST(cif_packet_set_item(packet, item_names[1], NULL), CIF_OK, test_name, 25);
    TEST(cif_packet_get_item(packet, item_names[0], &value1), CIF_OK, test_name, 26);
    TEST(cif_packet_get_item(packet, item_names[1], &value2), CIF_OK, test_name, 27);
    TEST(cif_value_autoinit_numb(value1, 1.0, 0.0, 19), CIF_OK, test_name, 28);
    TEST(cif_value_init(value2, CIF_NA_KIND), CIF_OK, test_name, 29);
    TEST(cif_loop_add_packet(loop1, packet), CIF_OK, test_name, 30);
    TEST(cif_value_autoinit_numb(value1, 2.0, 0.0, 19), CIF_OK, test_name, 31);
    TEST(cif_loop_add_packet(loop1, packet), CIF_OK, test_name, 32);

    /* Destroy the loop again */
    TEST(cif_loop_destroy(loop1), CIF_OK, test_name, 33);
    TEST(cif_container_get_item_loop(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 34);
    TEST(cif_container_get_item_loop(frame, item2l, NULL), CIF_OK, test_name, 35);
    TEST(cif_container_get_item_loop(frame, item3l, NULL), CIF_OK, test_name, 36);
    TEST(cif_container_get_item_loop(block, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 37);
    TEST(cif_container_get_item_loop(block, item2l, NULL), CIF_NOSUCH_ITEM, test_name, 38);
    TEST(cif_container_get_item_loop(block, item3l, NULL), CIF_NOSUCH_ITEM, test_name, 39);

    /* add some scalars to the block */
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 40);
    TEST(cif_value_init(value1, CIF_NA_KIND), CIF_OK, test_name, 41);
    TEST(cif_container_set_value(block, item2l, value1), CIF_OK, test_name, 42);
    TEST(cif_value_init(value1, CIF_LIST_KIND), CIF_OK, test_name, 43);
    TEST(cif_container_set_value(block, item3l, value1), CIF_OK, test_name, 44);

    /* destroy the scalar loop */
    TEST(cif_container_get_category_loop(block, CIF_SCALARS, &loop), CIF_OK, test_name, 45);
    TEST(cif_loop_destroy(loop), CIF_OK, test_name, 46);
    TEST(cif_container_get_item_loop(block, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 47);
    TEST(cif_container_get_item_loop(block, item2l, NULL), CIF_NOSUCH_ITEM, test_name, 48);
    TEST(cif_container_get_item_loop(block, item3l, NULL), CIF_NOSUCH_ITEM, test_name, 49);

    cif_packet_free(packet);
    cif_loop_free(loop2);
    cif_frame_free(frame);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

