/*
 * test_container_remove_item.c
 *
 * Tests behavior of the CIF API's cif_container_remove_item() function.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "uthash.h"

#include "test.h"

int main(void) {
    char test_name[80] = "test_container_remove_item";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp *loop = NULL;
    cif_loop_tp *loop2 = NULL;
    cif_packet_tp *packet = NULL;
    cif_packet_tp *packet2 = NULL;
    cif_pktitr_tp *iterator = NULL;
    cif_value_tp *value = NULL;
    cif_value_tp *value1 = NULL;
    cif_value_tp *value2 = NULL;
    cif_value_tp *value3 = NULL;
    UChar *names[5];
    int i;
    double d;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(name1l, "_name1", 7);
    U_STRING_DECL(name2l, "_name2", 7);
    U_STRING_DECL(name3l, "_name3", 7);
    U_STRING_DECL(name1u, "_Name1", 7);
    U_STRING_DECL(name2u, "_NAME2", 7);
    U_STRING_DECL(name3u, "_nAMe3", 7);
    U_STRING_DECL(scalar1l, "_scalar1", 9);
    U_STRING_DECL(scalar2l, "_scalar2", 9);
    U_STRING_DECL(scalar1u, "_sCaLar1", 9);
    U_STRING_DECL(scalar2u, "_SCaLar2", 9);
    U_STRING_DECL(invalid, "in valid", 9);
    U_STRING_DECL(category, "test", 5);

    /* Initialize data and prepare the test fixture */
    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(name1l, "_name1", 7);
    U_STRING_INIT(name2l, "_name2", 7);
    U_STRING_INIT(name3l, "_name3", 7);
    U_STRING_INIT(name1u, "_Name1", 7);
    U_STRING_INIT(name2u, "_NAME2", 7);
    U_STRING_INIT(name3u, "_nAMe3", 7);
    U_STRING_INIT(scalar1l, "_scalar1", 9);
    U_STRING_INIT(scalar2l, "_scalar2", 9);
    U_STRING_INIT(scalar1u, "_sCaLar1", 9);
    U_STRING_INIT(scalar2u, "_SCaLar2", 9);
    U_STRING_INIT(category, "test", 5);

    names[0] = name1l;
    names[1] = name2l;
    names[2] = name3l;
    names[3] = NULL;
    names[4] = NULL;

    INIT_USTDERR;
    TESTHEADER(test_name);

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    TEST(cif_container_create_loop(block, category, names, &loop), CIF_OK, test_name, 1);
    TEST(cif_container_create_loop(frame, category, names, &loop2), CIF_OK, test_name, 2);
    TEST(cif_container_get_item_loop(block, name3l, NULL), CIF_OK, test_name, 3);
    TEST(cif_container_get_item_loop(frame, name3l, NULL), CIF_OK, test_name, 4);
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 5);
    TEST(cif_loop_add_packet(loop2, packet), CIF_OK, test_name, 6);

    /* Test removing an invalid item name */
    TEST(cif_container_remove_item(block, invalid), CIF_NOSUCH_ITEM, test_name, 7);

    /* Test removing a valid item name that is not present */
    TEST(cif_container_remove_item(block, scalar1l), CIF_NOSUCH_ITEM, test_name, 8);

    /* Test removing an item for which there are no values */
    TEST(cif_container_remove_item(block, name3u), CIF_OK, test_name, 9);
    TEST(cif_container_get_value(block, name3l, NULL), CIF_NOSUCH_ITEM, test_name, 10);
    TEST(cif_container_get_value(frame, name3l, NULL), CIF_OK, test_name, 11);

    /* Add one packet to the target loop */
    TEST(cif_packet_get_item(packet, name1l, &value1), CIF_OK, test_name, 12);
    TEST(cif_packet_get_item(packet, name2l, &value2), CIF_OK, test_name, 13);
    TEST(cif_packet_get_item(packet, name3l, &value3), CIF_OK, test_name, 14);
    TEST(cif_value_autoinit_numb(value1, 1.0, 0.0, 19), CIF_OK, test_name, 15);
    TEST(cif_value_init(value2, CIF_LIST_KIND), CIF_OK, test_name, 16);
    TEST(cif_value_init(value3, CIF_NA_KIND), CIF_OK, test_name, 17);
    TEST(cif_loop_add_item(loop, name3l, value3), CIF_OK, test_name, 18);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 19);

    /* Test item removal from a single-packet loop */
    TEST(cif_container_remove_item(block, name2u), CIF_OK, test_name, 20);
    TEST(cif_container_get_value(block, name2l, NULL), CIF_NOSUCH_ITEM, test_name, 31);
    TEST(cif_container_get_value(frame, name2l, NULL), CIF_OK, test_name, 32);
        /* Still exactly one packet in loop1 if the following yields CIF_OK: */
    TEST(cif_container_get_value(block, name1l, &value), CIF_OK, test_name, 33);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 34);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 35);
    TEST(d != 1.0, 0, test_name, 36);
    cif_value_free(value);

    /* Add more packets */
        /* Preserves value2, that until now belonged to the packet: */
    TEST(cif_packet_remove_item(packet, name2l, &value2), CIF_OK, test_name, 37);
    TEST(cif_value_autoinit_numb(value1, 2.0, 0.0, 19), CIF_OK, test_name, 38);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 39);
    TEST(cif_value_autoinit_numb(value1, 3.0, 0.0, 19), CIF_OK, test_name, 40);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 41);

    /* test removal from a multi-packet loop */
    TEST(cif_container_remove_item(block, name3u), CIF_OK, test_name, 42);
    TEST(cif_container_get_value(block, name3l, NULL), CIF_NOSUCH_ITEM, test_name, 43);
    TEST(cif_container_get_value(frame, name3l, NULL), CIF_OK, test_name, 44);
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 45);
    for (i = 0; i < 3; i++) {
        TEST(cif_pktitr_next_packet(iterator, &packet2), CIF_OK, test_name, 46 + (5 * i));
        TEST(cif_packet_get_item(packet2, name1l, &value), CIF_OK, test_name, 47 + (5 * i));
        TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 48 + (5 * i));
        TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 49 + (5 * i));
        TEST(d != (double) (i + 1), 0, test_name, 50 + (5 * i));
    } /* last subtest == 60 */
    TEST(cif_pktitr_next_packet(iterator, NULL), CIF_FINISHED, test_name, 61);
    TEST(cif_pktitr_close(iterator), CIF_OK, test_name, 62);

    /* test removing the last item of its loop */
    TEST(cif_container_remove_item(block, name1u), CIF_OK, test_name, 63);
    TEST(cif_container_get_value(block, name1l, NULL), CIF_NOSUCH_ITEM, test_name, 64);
    TEST(cif_container_get_value(frame, name1l, NULL), CIF_OK, test_name, 65);
        /* The whole loop should be removed */
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_NOSUCH_LOOP, test_name, 66);
        /* The loop handle should be invalid */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_INVALID_HANDLE, test_name, 67);

    cif_loop_free(loop);
    cif_loop_free(loop2);

    /* test against the scalar loop */
    TEST(cif_container_set_value(block, scalar1l, value1), CIF_OK, test_name, 68);
    TEST(cif_container_set_value(block, scalar2l, value2), CIF_OK, test_name, 69);
    TEST(cif_container_get_value(block, scalar1l, NULL), CIF_OK, test_name, 70);
    TEST(cif_container_get_value(block, scalar2l, NULL), CIF_OK, test_name, 71);
    TEST(cif_container_get_category_loop(block, CIF_SCALARS, NULL), CIF_OK, test_name, 72);
    TEST(cif_container_remove_item(block, scalar1u), CIF_OK, test_name, 73);
    TEST(cif_container_get_value(block, scalar1l, NULL), CIF_NOSUCH_ITEM, test_name, 74);
    TEST(cif_container_get_value(block, scalar2l, NULL), CIF_OK, test_name, 75);
    TEST(cif_container_remove_item(block, scalar2u), CIF_OK, test_name, 76);
    TEST(cif_container_get_value(block, scalar2l, NULL), CIF_NOSUCH_ITEM, test_name, 77);
    TEST(cif_container_get_category_loop(block, CIF_SCALARS, NULL), CIF_NOSUCH_LOOP, test_name, 72);

    cif_packet_free(packet);
    DESTROY_FRAME(test_name, frame);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

