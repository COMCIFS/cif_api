/*
 * test_loop_packets.c
 *
 * Tests behavior of the CIF API's functions for adding and reading back
 * loop packets, primarily cif_loop_add_packet(), cif_loop_get_packets(),
 * and cif_pktitr_next_packet().
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

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

static int count_names(const UChar **names);

int main(void) {
    char test_name[80] = "test_loop_packets";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp *loop;
    cif_pktitr_tp *pktitr;
    cif_packet_tp *packet;
    cif_value_tp *value;
    cif_value_tp *value1;
    cif_value_tp *value2;
    cif_value_tp *value3;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    UChar item1l[] = { '_', 'i', 't', 'e', 'm', '1', 0 };
    UChar item2l[] = { '_', 'i', 't', 'e', 'm', '2', 0 };
    UChar item3l[] = { '_', 'i', 't', 'e', 'm', '3', 0 };
    UChar item4l[] = { '_', 'i', 't', 'e', 'm', '4', 0 };
    U_STRING_DECL(item1u, "_Item1", 7);
    U_STRING_DECL(item2u, "_ITEM2", 7);
    U_STRING_DECL(item3u, "_iTeM3", 7);
    U_STRING_DECL(char_value1, "simple_Value", 13);
    UChar *item_names[4];
    UChar cvalue[16] = { 0x56, 0 };
    UChar *ustr;
    int test_counter;
    unsigned packet_mask;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(item1u, "_Item1", 7);
    U_STRING_INIT(item2u, "_ITEM2", 7);
    U_STRING_INIT(item3u, "_iTeM3", 7);
    U_STRING_INIT(char_value1, "simple_Value", 13);

    item_names[0] = item1l;
    item_names[1] = item2l;
    item_names[2] = item3l;
    item_names[3] = NULL;

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    TEST(cif_container_create_loop(block, NULL, item_names, &loop), CIF_OK, test_name, 1);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 2);
    TEST((ustr != NULL), 0, test_name, 3);

    /* Verify that the loop initially has zero packets */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_EMPTY_LOOP, test_name, 4);

    /* Test adding an empty packet */
    TEST(cif_packet_create(&packet, NULL), CIF_OK, test_name, 5);
    TEST(cif_loop_add_packet(loop, packet), CIF_INVALID_PACKET, test_name, 6);

    /* Test adding a packet with only a non-existent name */
    TEST(cif_packet_set_item(packet, item4l, NULL), CIF_OK, test_name, 7);
    TEST(cif_packet_get_item(packet, item4l, &value2), CIF_OK, test_name, 8);
    TEST(cif_value_copy_char(value2, char_value1), CIF_OK, test_name, 9);
    TEST(cif_loop_add_packet(loop, packet), CIF_WRONG_LOOP, test_name, 10);
    /* value2 belongs to the packet */
    value2 = NULL;

    /* Test adding a packet with a name belonging to a different loop */
    TEST(cif_container_set_value(block, item4l, NULL), CIF_OK, test_name, 11);
    TEST(cif_loop_add_packet(loop, packet), CIF_WRONG_LOOP, test_name, 12);
    TEST(cif_packet_set_item(packet, item1u, NULL), CIF_OK, test_name, 13);
    TEST(cif_packet_get_item(packet, item1u, &value1), CIF_OK, test_name, 14);  /* value1 belongs to the packet */
    TEST(cif_value_init_numb(value1, 1.0, 0.0, 0, 1), CIF_OK, test_name, 15);
    TEST(cif_loop_add_packet(loop, packet), CIF_WRONG_LOOP, test_name, 16);

    /* Test adding and reading back several packets */
    TEST(cif_packet_remove_item(packet, item4l, NULL), CIF_OK, test_name, 17);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 18);

    TEST(cif_value_init_numb(value1, 2.0, 0.0, 0, 1), CIF_OK, test_name, 19);
    TEST(cif_packet_set_item(packet, item2u, NULL), CIF_OK, test_name, 20);
    TEST(cif_packet_get_item(packet, item2u, &value2), CIF_OK, test_name, 21);  /* value2 belongs to the packet */
    TEST(u_sprintf(cvalue + 1, "%d", 2), 1, test_name, 22);
    TEST(cif_value_copy_char(value2, cvalue), CIF_OK, test_name, 23);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 24);

    TEST(cif_value_init_numb(value1, 3.0, 0.0, 0, 1), CIF_OK, test_name, 25);
    TEST(u_sprintf(cvalue + 1, "%d", 3), 1, test_name, 26);
    TEST(cif_value_copy_char(value2, cvalue), CIF_OK, test_name, 27);
    TEST(cif_packet_set_item(packet, item3l, NULL), CIF_OK, test_name, 28);
    TEST(cif_packet_get_item(packet, item3u, &value3), CIF_OK, test_name, 29);  /* value3 belongs to the packet */
    TEST(cif_value_init(value3, CIF_NA_KIND), CIF_OK, test_name, 30);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 31);

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 32);
    TEST(cif_pktitr_remove_packet(pktitr), CIF_MISUSE, test_name, 33);
    TEST(cif_pktitr_update_packet(pktitr, packet), CIF_MISUSE, test_name, 34);
    TEST(cif_packet_set_item(packet, item4l, NULL), CIF_OK, test_name, 35);
    TEST(cif_packet_get_item(packet, item4l, NULL), CIF_OK, test_name, 36);

    /*
     * NOTE: packets are not guaranteed to be iterated in insertion order, but each one should be iterated
     *       exactly once.
     */
    test_counter = 37;
    for (packet_mask = 0; packet_mask < 7; ) {
        const UChar **names;
        int key;
        double key_double;

        TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, test_counter++);
        TEST(cif_packet_get_names(packet, &names), CIF_OK, test_name, test_counter++);
        TEST(count_names(names), 3, test_name, test_counter++);
        free(names);

        TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, test_counter++);
        TEST(cif_value_get_number(value, &key_double), CIF_OK, test_name, test_counter++);
        key = (int) key_double;
        TEST((key != key_double), 0, test_name, test_counter++);
        TEST((key <= 0), 0, test_name, test_counter++);
        TEST((key > 3), 0, test_name, test_counter++);
        TEST((packet_mask & (1 << (key - 1))), 0, test_name, test_counter++);
        packet_mask |= (1 << (key - 1));

        TEST(cif_packet_get_item(packet, item3l, &value), CIF_OK, test_name, test_counter++);
        TEST(cif_value_kind(value), ((key > 2) ? CIF_NA_KIND : CIF_UNK_KIND), test_name, test_counter++);
        TEST(cif_packet_get_item(packet, item2l, &value), CIF_OK, test_name, test_counter++);
        if (key < 2) {
            TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, test_counter++);
        } else {
            TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, test_counter++);
            TEST(u_sprintf(cvalue + 1, "%d", key), 1, test_name, test_counter++);
            TEST(u_strcmp(cvalue, ustr), 0, test_name, test_counter++);
            free(ustr);
        }
    }

    /* assert(test_counter == 80); */
    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_FINISHED, test_name, 80);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 81);

    /* test iterating packets without returning them */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 82);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 83);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 84);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 85);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_FINISHED, test_name, 86);
    TEST(cif_pktitr_abort(pktitr), CIF_OK, test_name, 87);

    cif_packet_free(packet);
    cif_loop_free(loop);
    cif_frame_free(frame);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

static int count_names(const UChar **names) {
    int count;

    for (count = 0; names[count] != NULL; count += 1) ;

    return count;
}

