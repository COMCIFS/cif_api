/*
 * test_loop_misc.c
 *
 * Tests CIF API packet iterator and loop behaviors that are not covered by other tests.  Specifically, behavior
 * of a packet iterator with with the scalar loop is excercised, and the rollback facility provided by
 * cif_pktitr_abort() is verified.  Also, cif_loop_add_packet() is tested as it applies to the scalar loop.
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

static int assert_packets_equal(cif_packet_t *packet1, cif_packet_t *packet2);

int main(void) {
    char test_name[80] = "test_loop_misc";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_loop_t *loop;
    cif_pktitr_t *iterator;
    cif_packet_t *packet = NULL;
    cif_packet_t *packet2 = NULL;
    cif_value_t *value = NULL;
    cif_value_t *value1 = NULL;
    cif_value_t *value2 = NULL;
    cif_value_t *value3 = NULL;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(item1l, "_item1", 7);
    U_STRING_DECL(item2l, "_item2", 7);
    U_STRING_DECL(item3l, "_item3", 7);
    U_STRING_DECL(char_value1, "simple_Value", 13);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(item1l, "_item1", 7);
    U_STRING_INIT(item2l, "_item2", 7);
    U_STRING_INIT(item3l, "_item3", 7);
    U_STRING_INIT(char_value1, "simple_Value", 13);

    /* set up the test fixture */

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);

    /* Add some scalar values to the block */
    TEST(cif_value_create(CIF_UNK_KIND, &value1), CIF_OK, test_name, 1);
    TEST(cif_value_autoinit_numb(value1, 2.5, 0.25, 19), CIF_OK, test_name, 2);
    TEST(cif_value_create(CIF_UNK_KIND, &value2), CIF_OK, test_name, 3);
    TEST(cif_value_copy_char(value2, char_value1), CIF_OK, test_name, 4);
    TEST(cif_value_create(CIF_TABLE_KIND, &value3), CIF_OK, test_name, 5);
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 6);
    TEST(cif_container_set_value(block, item2l, value2), CIF_OK, test_name, 7);
    TEST(cif_container_set_value(block, item3l, value3), CIF_OK, test_name, 9);

    /* Obtain a handle on the block's scalar loop */
    TEST(cif_container_get_category_loop(block, CIF_SCALARS, &loop), CIF_OK, test_name, 10);

    /* Test modifying the scalar loop via a packet iterator */

    /* obtain an iterator and advance it across the first (only) packet */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 11);
    TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 12);

    /* verify values, and modify some of them */
    TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, 13);
    TEST(!assert_values_equal(value, value1), 0, test_name, 14);
    TEST(cif_packet_get_item(packet, item2l, &value), CIF_OK, test_name, 15);
    TEST(!assert_values_equal(value, value2), 0, test_name, 16);
    TEST(cif_value_init(value, CIF_NA_KIND), CIF_OK, test_name, 17);
    TEST(cif_value_clone(value, &value2), CIF_OK, test_name, 18);
    TEST(cif_packet_get_item(packet, item3l, &value), CIF_OK, test_name, 19);
    TEST(!assert_values_equal(value, value3), 0, test_name, 20);
    TEST(cif_value_copy_char(value, block_code), CIF_OK, test_name, 21);
    TEST(cif_value_clone(value, &value3), CIF_OK, test_name, 22);

    /* Apply the update */
    TEST(cif_pktitr_update_packet(iterator, packet), CIF_OK, test_name, 23);
    /* Verify that the update was applied (iterator still open) */
    TEST(cif_container_get_value(block, item2l, &value), CIF_OK, test_name, 24);
    TEST(!assert_values_equal(value, value2), 0, test_name, 25);
    /* Close the iterator, then reload the packet and test again */
    TEST(cif_pktitr_close(iterator), CIF_OK, test_name, 26);
    cif_packet_free(packet);
    packet = NULL;
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 27);
    TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 28);
    /* verify the revised values */
    TEST(cif_packet_get_item(packet, item1l, &value), CIF_OK, test_name, 29);
    TEST(!assert_values_equal(value, value1), 0, test_name, 30);
    TEST(cif_packet_get_item(packet, item2l, &value), CIF_OK, test_name, 31);
    TEST(!assert_values_equal(value, value2), 0, test_name, 32);
    TEST(cif_packet_get_item(packet, item3l, &value), CIF_OK, test_name, 33);
    TEST(!assert_values_equal(value, value3), 0, test_name, 34);
    /* iterator is still open */

    /* test aborting a modification */
    TEST(cif_value_init(value, CIF_LIST_KIND), CIF_OK, test_name, 35);
    TEST(cif_pktitr_update_packet(iterator, packet), CIF_OK, test_name, 36);
        /* 'value' currently belongs to the packet */
    value = NULL;
    TEST(cif_container_get_value(block, item3l, &value), CIF_OK, test_name, 37);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 38);
    TEST(cif_pktitr_abort(iterator), CIF_OK, test_name, 39);
    cif_value_free(value);
    value = NULL;
    TEST(cif_container_get_value(block, item3l, &value), CIF_OK, test_name, 40);
    TEST(!assert_values_equal(value, value3), 0, test_name, 41);
    cif_value_free(value);
    value = NULL;
    
    /* test aborting a deletion */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 42);
    TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 43);
    TEST(cif_pktitr_remove_packet(iterator), CIF_OK, test_name, 44);
        /* verify the deletion */
    TEST(cif_container_get_value(block, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 45);
        /* abort */
    TEST(cif_pktitr_abort(iterator), CIF_OK, test_name, 46);
    TEST(cif_container_get_value(block, item1l, NULL), CIF_OK, test_name, 47);

    /* test finalizing a deletion */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 48);
    TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 49);
    TEST(cif_pktitr_remove_packet(iterator), CIF_OK, test_name, 50);
        /* commit */
    TEST(cif_pktitr_close(iterator), CIF_OK, test_name, 51);
        /* verify */
    TEST(cif_container_get_value(block, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 52);
    TEST(cif_loop_get_packets(loop, &iterator), CIF_EMPTY_LOOP, test_name, 53);

    /* test adding a packet */
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 55);
        /* verify */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 56);
    TEST(cif_pktitr_next_packet(iterator, &packet2), CIF_OK, test_name, 57);
    TEST(!assert_packets_equal(packet2, packet), 0, test_name, 58);
    TEST(cif_pktitr_abort(iterator), CIF_OK, test_name, 59);
    cif_packet_free(packet2);

    /* test adding another packet */
    TEST(cif_loop_add_packet(loop, packet), CIF_RESERVED_LOOP, test_name, 60);

    cif_packet_free(packet);

    cif_value_free(value3);
    cif_value_free(value2);
    cif_value_free(value1);
    cif_loop_free(loop);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

/*
 * Clones a packet and returns a pointer to the clone.  Returns NULL on failure.
 */
static cif_packet_t *clone_packet(cif_packet_t *packet) {
    cif_packet_t *clone = NULL;
    const UChar **names;

    if (cif_packet_get_names(packet, &names) == CIF_OK) {
        if (cif_packet_create(&clone, NULL) == CIF_OK) {
            const UChar **name_p;

            for (name_p = names; *name_p != NULL; name_p += 1) {
                cif_value_t *value;

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
 * Tests the assertion that the specified packets contain values for identical
 * sets of item names, with all pairs of values associated with the same name
 * being equal.  Returns a nonzero value if the assertion is true, or 0 if it
 * is false.
 */
static int assert_packets_equal(cif_packet_t *packet1, cif_packet_t *packet2) {
    const UChar **item_names;
    int rval = 0;

    if (cif_packet_get_names(packet1, &item_names) == CIF_OK) {
        /*
         * packet2 is cloned to create a scratch value that can be destroyed.
         * That's convenient, but we really need only a name set, so cloning
         * the whole thing is overkill.
         */
        cif_packet_t *clone = clone_packet(packet2);

        if (clone != NULL) {
            const UChar **name_p;

            /* check each item in packet1 to see whether it matches an item in the clone of packet2 */
            for (name_p = item_names; *name_p != NULL; name_p += 1) {
                cif_value_t *value1;
                cif_value_t *value2;

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
