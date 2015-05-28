/*
 * test_container_set_value2.c
 *
 * Tests behaviors of the CIF API's cif_container_set_value() function for scalar values.
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
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_container_set_value2";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp *loop;
    cif_loop_tp *loop2;
    cif_pktitr_tp *pktitr;
    cif_packet_tp *packet;
    cif_value_tp *value1;
    cif_value_tp *value2;
    cif_value_tp *value3;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    UChar item1l[] = { '_', 'i', 't', 'e', 'm', '1', 0 };
    UChar item2l[] = { '_', 'i', 't', 'e', 'm', '2', 0 };
    UChar item3l[] = { '_', 'i', 't', 'e', 'm', '3', 0 };
    UChar item1u[] = { '_', 'I', 't', 'e', 'm', '1', 0 };
    UChar item2u[] = { '_', 'I', 'T', 'E', 'M', '2', 0 };
    UChar item3u[] = { '_', 'i', 'T', 'e', 'M', '3', 0 };
    UChar char_value1[] = { 's', 'i', 'm', 'p', 'l', 'e', '_', 'V', 'a', 'l', 'u', 'e', 0 };
    UChar *names[4];
    int i;
    int mask;

    /* Initialize data and prepare the test fixture */

    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    names[0] = item1u;
    names[1] = item2u;
    names[2] = item3u;
    names[3] = NULL;

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    TEST(cif_container_create_loop(block, NULL, names, &loop), CIF_OK, test_name, 1);
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 2);
    names[1] = NULL;
    TEST(cif_container_create_loop(frame, NULL, names, &loop2), CIF_OK, test_name, 3);

    TEST(cif_packet_get_item(packet, item1u, &value1), CIF_OK, test_name, 4);
    TEST(cif_value_init(value1, CIF_NA_KIND), CIF_OK, test_name, 5);
    TEST(cif_packet_get_item(packet, item2u, &value1), CIF_OK, test_name, 6);
    TEST(cif_value_copy_char(value1, char_value1), CIF_OK, test_name, 7);
    TEST(cif_packet_get_item(packet, item3u, &value1), CIF_OK, test_name, 8);
    TEST(cif_value_copy_char(value1, item3u), CIF_OK, test_name, 9);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 10);
    /* value1 belongs to the packet */

    TEST(cif_value_create(CIF_UNK_KIND, &value1), CIF_OK, test_name, 11);
    /* value1 is now independent */

    /* Test setting a value on a single-packet, non-scalar loop */
    TEST(cif_value_init_numb(value1, 1.0, 0.0, 1, 0), CIF_OK, test_name, 12);
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 13);
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 14);
    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 15);
    TEST(cif_packet_get_item(packet, item1l, &value2), CIF_OK, test_name, 16);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 17);
    value2 = NULL; /* value2 belongs to the packet */

    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_FINISHED, test_name, 18);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 19);
    TEST(cif_container_get_value(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 20);

    /* Test setting a value on an empty loop */
    TEST(cif_value_create(CIF_UNK_KIND, &value3), CIF_OK, test_name, 21);
    TEST(cif_value_init_numb(value3, 2.0, 0.0, 1, 0), CIF_OK, test_name, 22);
    TEST(cif_container_set_value(frame, item1l, value3), CIF_OK, test_name, 23);
    TEST(cif_loop_get_packets(loop2, &pktitr), CIF_EMPTY_LOOP, test_name, 24);
    TEST(cif_container_get_value(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 29);

    /* value1 and value3 are independent at this point */

    /* Test setting a value on a multi-packet loop */

    /* start by setting up a fresh loop */
    cif_packet_free(packet);
    cif_value_free(value3);
    cif_value_free(value1);
    names[1] = item2l;
    TEST(cif_loop_destroy(loop), CIF_OK, test_name, 31);
    TEST(cif_container_create_loop(block, NULL, names, &loop), CIF_OK, test_name, 32);
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 33);

    /* Add three packets */
    TEST(cif_packet_get_item(packet, item1u, &value1), CIF_OK, test_name, 34);
    TEST(cif_value_init_numb(value1, 0.0, 0.0, 0, 1), CIF_OK, test_name, 35);
    TEST(cif_packet_get_item(packet, item2u, &value2), CIF_OK, test_name, 36);
    TEST(cif_value_copy_char(value2, char_value1), CIF_OK, test_name, 37);
    TEST(cif_packet_get_item(packet, item3u, &value3), CIF_OK, test_name, 38);
    TEST(cif_value_copy_char(value3, item3u), CIF_OK, test_name, 39);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 40);
    TEST(cif_value_init_numb(value1, 1.0, 0.0, 0, 0), CIF_OK, test_name, 41);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 42);
    TEST(cif_value_init_numb(value1, 2.0, 0.0, 0, 0), CIF_OK, test_name, 43);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 44);

    /* perform cif_container_set_value() */
    value1 = NULL;
    TEST(cif_value_create(CIF_UNK_KIND, &value1), CIF_OK, test_name, 45);
    TEST(cif_value_init_numb(value1, 17.25, 0.25, 2, 1), CIF_OK, test_name, 46);
    TEST(cif_container_set_value(block, item2u, value1), CIF_OK, test_name, 47);
    /* value1 is the (independent) value that was set */

    /* clone value3 to make it independent of the packet */
    value2 = NULL;
    TEST(cif_value_clone(value3, &value2), CIF_OK, test_name, 48);
    value3 = value2;
    value2 = NULL;

    /* read back the packets and check them */
    cif_packet_free(packet);
    packet = NULL;
    mask = 0;
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 49);
    for (i = 0; i < 3; i += 1) {
        double d;

        TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 50 + 12 * i);
        TEST(cif_packet_get_item(packet, item2l, &value2), CIF_OK, test_name, 51 + 12 * i);
        TEST(!assert_values_equal(value1, value2), 0, test_name, 52 + 12 * i);
        TEST(cif_packet_get_item(packet, item3l, &value2), CIF_OK, test_name, 53 + 12 * i);
        TEST(!assert_values_equal(value3, value2), 0, test_name, 54 + 12 * i);
        TEST(cif_packet_get_item(packet, item1l, &value2), CIF_OK, test_name, 55 + 12 * i);
        TEST(cif_value_kind(value2), CIF_NUMB_KIND, test_name, 56 + 12 * i);
        TEST(cif_value_get_number(value2, &d), CIF_OK, test_name, 57 + 12 * i);
        TEST(d != ((int) d), 0, test_name, 58 + 12 * i);
        TEST(mask & (1 << ((int) d)), 0, test_name, 59 + 12 * i);
        mask |= (1 << ((int) d));
        TEST(cif_value_get_su(value2, &d), CIF_OK, test_name, 60 + 12 * i);
        TEST(d != 0, 0, test_name, 61 + 12 * i);
    }

    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_FINISHED, test_name, 86);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 87);

    cif_value_free(value3);
    cif_value_free(value1);
    cif_packet_free(packet);
    cif_loop_free(loop2);
    cif_loop_free(loop);
    cif_frame_free(frame);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

