/*
 * test_loop_modification.c
 *
 * Tests the CIF API's general functions for manipulating existing data,
 * cif_pktitr_update_packet() and cif_pktitr_remove_packet().
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

static cif_packet_tp *clone_packet(cif_packet_tp *packet);
static cif_packet_tp *lookup_packet(cif_packet_tp **packets, const UChar *key_name, cif_value_tp *find);
static int assert_packets_equal(cif_packet_tp *packet1, cif_packet_tp *packet2);
static int assert_packets(cif_pktitr_tp *pktitr, cif_packet_tp *expected[], const UChar *key_name);

int main(void) {
    char test_name[80] = "test_loop_modification";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_loop_tp *loop;
    cif_pktitr_tp *pktitr;
    cif_packet_tp *packet;
    cif_packet_tp *packet2;
    cif_packet_tp *packet3;
    cif_packet_tp *reference_packets[5] = { NULL, NULL, NULL, NULL, NULL };
    cif_value_tp *value;
    cif_value_tp *value1;
    cif_value_tp *value2;
    cif_value_tp *value3;
    U_STRING_DECL(block_code, "block", 6);
    UChar item1l[] = { '_', 'i', 't', 'e', 'm', '1', 0 };
    UChar item2l[] = { '_', 'i', 't', 'e', 'm', '2', 0 };
    UChar item3l[] = { '_', 'i', 't', 'e', 'm', '3', 0 };
    U_STRING_DECL(item1u, "_Item1", 7);
    U_STRING_DECL(item2u, "_ITEM2", 7);
    U_STRING_DECL(item3u, "_iTeM3", 7);
    U_STRING_DECL(char_value1, "simple_Value", 13);
    UChar *item_names[4];
    UChar cvalue[16] = { 0x56, 0 };
    UChar *ustr;
    int counter;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
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

    TEST(cif_container_create_loop(block, NULL, item_names, &loop), CIF_OK, test_name, 1);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 2);
    TEST((ustr != NULL), 0, test_name, 3);

    /* Verify that the loop initially has zero packets */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_EMPTY_LOOP, test_name, 4);

    /* Add several packets */

    TEST(cif_packet_create(&packet, NULL), CIF_OK, test_name, 5);

    TEST(cif_packet_set_item(packet, item1u, NULL), CIF_OK, test_name, 6);
    TEST(cif_packet_get_item(packet, item1u, &value1), CIF_OK, test_name, 7);  /* value1 belongs to the packet */
    TEST(cif_value_init_numb(value1, 1.0, 0.0, 0, 1), CIF_OK, test_name, 8);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 9);

    TEST(cif_value_init_numb(value1, 2.0, 0.0, 0, 1), CIF_OK, test_name, 10);
    TEST(cif_packet_set_item(packet, item2u, NULL), CIF_OK, test_name, 11);
    TEST(cif_packet_get_item(packet, item2u, &value2), CIF_OK, test_name, 12);  /* value2 belongs to the packet */
    TEST(u_sprintf(cvalue + 1, "%d", 2), 1, test_name, 13);
    TEST(cif_value_copy_char(value2, cvalue), CIF_OK, test_name, 14);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 15);

    TEST(cif_value_init_numb(value1, 3.0, 0.0, 0, 1), CIF_OK, test_name, 16);
    TEST(u_sprintf(cvalue + 1, "%d", 3), 1, test_name, 17);
    TEST(cif_value_copy_char(value2, cvalue), CIF_OK, test_name, 18);
    TEST(cif_packet_set_item(packet, item3l, NULL), CIF_OK, test_name, 19);
    TEST(cif_packet_get_item(packet, item3u, &value3), CIF_OK, test_name, 20);  /* value3 belongs to the packet */
    TEST(cif_value_init(value3, CIF_NA_KIND), CIF_OK, test_name, 21);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 22);

    TEST(cif_value_init_numb(value1, 4.0, 0.0, 0, 1), CIF_OK, test_name, 23);
    TEST(u_sprintf(cvalue + 1, "%d", 4), 1, test_name, 24);
    TEST(cif_value_copy_char(value2, cvalue), CIF_OK, test_name, 25);
    TEST(cif_value_init(value3, CIF_TABLE_KIND), CIF_OK, test_name, 26);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 27);

    /* The value pointers are about to become invalid; nullify them to avoid risk of confusion */
    value1 = NULL;
    value2 = NULL;
    value3 = NULL;

    /* Make reference copies of all four loop packets */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 28);
    for (counter = 0; counter < 4; counter += 1) {
        TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 29 + 2 * counter);
        reference_packets[counter] = clone_packet(packet);
        TEST((reference_packets[counter] == NULL), 0, test_name, 30 + 2 * counter);
    }
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_FINISHED, test_name, 37);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 38);

    /* test updating the first-iterated packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 39);
    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 40);
    TEST(cif_packet_get_item(packet, item3l, &value3), CIF_OK, test_name, 41);
    TEST((cif_value_kind(value3) == CIF_TABLE_KIND), 0, test_name, 42);
    TEST(cif_value_init(value3, CIF_TABLE_KIND), CIF_OK, test_name, 43);
    /* Update the reference list */
    TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, 44);
    packet2 = lookup_packet(reference_packets, item1l, value);
    TEST((packet2 == NULL), 0, test_name, 45);
    TEST(cif_packet_set_item(packet2, item3l, value3), CIF_OK, test_name, 46);
    /* update the loop */
    TEST(cif_pktitr_update_packet(pktitr, packet), CIF_OK, test_name, 47);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 48);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 49);
    TEST(!assert_packets(pktitr, reference_packets, item1l), 0, test_name, 50);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 51);

    /* test updating the second-iterated packet with a partial packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 52);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 53);
    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 54);
    TEST(cif_packet_get_item(packet, item3l, &value3), CIF_OK, test_name, 55);
    TEST((cif_value_kind(value3) == CIF_LIST_KIND), 0, test_name, 56);
    TEST(cif_value_init(value3, CIF_LIST_KIND), CIF_OK, test_name, 57);
    /* Update the reference list */
    TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, 58);
    packet2 = lookup_packet(reference_packets, item1l, value);
    TEST((packet2 == NULL), 0, test_name, 59);
    TEST(cif_packet_set_item(packet2, item3l, value3), CIF_OK, test_name, 60);
    /* update the loop via a partial packet */
    TEST(cif_packet_create(&packet3, NULL), CIF_OK, test_name, 61);
    TEST(cif_packet_set_item(packet3, item3u, value3), CIF_OK, test_name, 62);
    TEST(cif_pktitr_update_packet(pktitr, packet3), CIF_OK, test_name, 63);
    cif_packet_free(packet3);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 64);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 65);
    TEST(!assert_packets(pktitr, reference_packets, item1l), 0, test_name, 66);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 67);

    /* test updating the last-iterated packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 68);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 69);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 70);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 71);
    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 72);
    TEST(cif_packet_get_item(packet, item3l, &value3), CIF_OK, test_name, 73);
    TEST((cif_value_kind(value3) == CIF_CHAR_KIND), 0, test_name, 74);
    TEST(cif_value_copy_char(value3, char_value1), CIF_OK, test_name, 75);
    /* Update the reference list */
    TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, 76);
    packet2 = lookup_packet(reference_packets, item1l, value);
    TEST((packet2 == NULL), 0, test_name, 77);
    TEST(cif_packet_set_item(packet2, item3l, value3), CIF_OK, test_name, 78);
    /* update the loop */
    TEST(cif_pktitr_update_packet(pktitr, packet), CIF_OK, test_name, 79);
    /* verify that it was indeed the last packet updated */
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_FINISHED, test_name, 80);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 81);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 82);
    TEST(!assert_packets(pktitr, reference_packets, item1l), 0, test_name, 83);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 84);

    /* test removing the first-iterated packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 85);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 86);
    /* update the loop */
    TEST(cif_pktitr_remove_packet(pktitr), CIF_OK, test_name, 87);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 88);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 89);
    TEST(!assert_packets(pktitr, reference_packets + 1, item1l), 0, test_name, 90);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 91);

    /* test removing the second-iterated (middle) packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 92);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 93);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 94);
    /* Update the reference list */
    cif_packet_free(reference_packets[2]);
    reference_packets[2] = reference_packets[3];
    reference_packets[3] = NULL;
    /* update the loop */
    TEST(cif_pktitr_remove_packet(pktitr), CIF_OK, test_name, 95);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 96);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 97);
    TEST(!assert_packets(pktitr, reference_packets + 1, item1l), 0, test_name, 98);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 99);

    /* test removing the last-iterated packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 100);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 101);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 102);
    /* Update the reference list */
    cif_packet_free(reference_packets[2]);
    reference_packets[2] = NULL;
    /* update the loop */
    TEST(cif_pktitr_remove_packet(pktitr), CIF_OK, test_name, 103);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 104);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 105);
    TEST(!assert_packets(pktitr, reference_packets + 1, item1l), 0, test_name, 106);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 107);

    /* test updating the only remaining packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 108);
    TEST(cif_pktitr_next_packet(pktitr, &packet), CIF_OK, test_name, 109);
    TEST(cif_packet_get_item(packet, item3l, &value3), CIF_OK, test_name, 110);
    TEST((cif_value_kind(value3) == CIF_NUMB_KIND), 0, test_name, 111);
    TEST(cif_value_autoinit_numb(value3, 42.0, 0.125, 19), CIF_OK, test_name, 112);
    /* Update the reference list */
    TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, 113);
    packet2 = lookup_packet(reference_packets + 1, item1l, value);
    TEST((packet2 == NULL), 0, test_name, 114);
    TEST(cif_packet_set_item(packet2, item3l, value3), CIF_OK, test_name, 115);
    /* update the loop */
    TEST(cif_pktitr_update_packet(pktitr, packet), CIF_OK, test_name, 116);
    /* verify that that was the only packet */
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_FINISHED, test_name, 117);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 118);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 119);
    TEST(!assert_packets(pktitr, reference_packets + 1, item1l), 0, test_name, 120);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 121);

    /* test removing the only remaining packet */

    TEST(cif_loop_get_packets(loop, &pktitr), CIF_OK, test_name, 122);
    TEST(cif_pktitr_next_packet(pktitr, NULL), CIF_OK, test_name, 123);
    /* update the loop */
    TEST(cif_pktitr_remove_packet(pktitr), CIF_OK, test_name, 124);
    TEST(cif_pktitr_close(pktitr), CIF_OK, test_name, 125);
    /* check that the loop contents are as expected */
    TEST(cif_loop_get_packets(loop, &pktitr), CIF_EMPTY_LOOP, test_name, 126);

    for (counter = 0; counter < 4; counter += 1) {
        cif_packet_free(reference_packets[counter]);
    }
    cif_packet_free(packet);
    cif_loop_free(loop);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

/*
 * Clones a packet and returns a pointer to the clone.  Returns NULL on failure.
 */
static cif_packet_tp *clone_packet(cif_packet_tp *packet) {
    cif_packet_tp *clone = NULL;
    const UChar **names;

    if (cif_packet_get_names(packet, &names) == CIF_OK) {
        if (cif_packet_create(&clone, NULL) == CIF_OK) {
            const UChar **name_p;

            for (name_p = names; *name_p != NULL; name_p += 1) {
                cif_value_tp *value;

                /* 'value' does not need to be released because it belongs to 'packet' */
                /* 'value' is automatically cloned into 'clone'; no need to clone it explicitly */
                if ((cif_packet_get_item(packet, *name_p, &value) != CIF_OK)
                        || (cif_packet_set_item(clone, *name_p, value) != CIF_OK)) {
                    /* something went wrong -- clean up and exit */
                    cif_packet_free(clone);
                    clone = NULL;
                    break;
                }
            }
        }

        free(names);
    }

    return clone;
}

/*
 * Searches a NULL-terminated array of packets for the first one having the
 * specified 'find' value for item 'key_name'.  Returns a pointer to the first
 * such packet in the array, or NULL if none is found or if an error occurs.
 *
 * All packets should have an item of the specified key_name.
 */
static cif_packet_tp *lookup_packet(cif_packet_tp **packets, const UChar *key_name, cif_value_tp *find) {
    cif_packet_tp **packet_p;

    for (packet_p = packets; *packet_p != NULL; packet_p += 1) {
        cif_value_tp *value;

        if (cif_packet_get_item(*packet_p, key_name, &value) != CIF_OK) {
            return NULL;
        } else if (assert_values_equal(value, find)) {
            break;
        }
    }

    return *packet_p; 
}

/*
 * Tests the assertion that the specified packets contain values for identical
 * sets of item names, with all pairs of values associated with the same name
 * being equal.  Returns a nonzero value if the assertion is true, or 0 if it
 * is false.
 */
static int assert_packets_equal(cif_packet_tp *packet1, cif_packet_tp *packet2) {
    const UChar **item_names;
    int rval = 0;

    if (cif_packet_get_names(packet1, &item_names) == CIF_OK) {
        /*
         * packet2 is cloned to create a scratch value that can be destroyed.
         * That's convenient, but we really need only a name set, so cloning
         * the whole thing is overkill.
         */
        cif_packet_tp *clone = clone_packet(packet2);

        if (clone != NULL) {
            const UChar **name_p;

            /* check each item in packet1 to see whether it matches an item in the clone of packet2 */
            for (name_p = item_names; *name_p != NULL; name_p += 1) {
                cif_value_tp *value1;
                cif_value_tp *value2;

                if ((cif_packet_get_item(packet1, *name_p, &value1) != CIF_OK)
                        || (cif_packet_get_item(clone, *name_p, &value2) != CIF_OK)
                        || (!assert_values_equal(value1, value2))
                           /* match successful.  remove the matching item from the clone: */
                        || (cif_packet_remove_item(clone, *name_p, NULL) != CIF_OK)) {
                    goto done;
                }
            }

            /* check whether there are any unmatched items left in the erstwhile clone of packet2 */
            free(item_names);
            item_names = NULL;  /* in case the following fails, to avoid doubly freeing 'item_names' */
            rval = ((cif_packet_get_names(clone, &item_names) == CIF_OK) && (*item_names == NULL));

            done:
            cif_packet_free(clone);
        }

        free(item_names);
    }

    return rval;
}

/*
 * Tests the assertion that the packets available for iteration via 'pktitr'
 * correspond exactly to those in the NULL-terminated 'expected' array, albeit
 * not necessarily in the same order.  'key_name' is the name of an item to be
 * used to match iterated packets with putative equal packets in the 'expected'
 * array.
 *
 * Returns a non-zero value if the assertion is true, or zero if it is false.
 *
 * This function is inherently destructive in that it iterates over some or all
 * of the packets available from the given iterator.
 */
static int assert_packets(cif_pktitr_tp *pktitr, cif_packet_tp *expected[], const UChar *key_name) {
    unsigned int count;
    unsigned int expected_mask;
    cif_packet_tp *packet = NULL;
    int rval = 0;

    /*
     * Count the expected packets, and prepare a bitmask by which to track
     * which ones have been matched.
     */
    for (count = 0; expected[count] != NULL; count += 1) {}
    expected_mask = ~((~0) << count);  /* one bit set for each expected packet */

    /* iterate over the available packets */
    for (;;) {
        cif_value_tp *key;
        cif_packet_tp *expected_packet;

        switch (cif_pktitr_next_packet(pktitr, &packet)) {
            default:            /* error */
                goto cleanup;
            case CIF_FINISHED:  /* no more packets */
                goto end_packets;
            case CIF_OK:        /* retrieved a packet */
                /* compare the retrieved packet to the expected one (if any) bearing the same key value */
                if ((cif_packet_get_item(packet, key_name, &key) != CIF_OK)
                        || ((expected_packet = lookup_packet(expected, key_name, key)) == NULL)
                        || !assert_packets_equal(packet, expected_packet)) {
                    /* no match */
                    goto cleanup;
                } else {
                    /* update the mask to mark the packet seen */
                    unsigned int bit;

                    for (count = 0; expected[count] != expected_packet; count += 1) {
                        if (expected[count] == NULL) goto cleanup;
                    }
                    bit = (1 << count);
                    if ((expected_mask & bit) == bit) {
                        expected_mask ^= bit;
                    } else {
                        /* the matching expected packet was matched previously */
                        goto cleanup;
                    }
                }
        }
    }

    end_packets:  /* normal end of packet iteration */
    /* The assertion is satisfied if and only if 'expected_mask' is zero */
    rval = (expected_mask == 0);

    cleanup:
    if (packet != NULL) {
        cif_packet_free(packet);
    }

    return rval;
}

