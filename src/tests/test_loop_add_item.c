/*
 * test_loop_add_item.c
 *
 * Tests behavior of the CIF API's cif_loop_add_item() function.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "uthash.h"

#include "assert_value.h"
#define USE_USTDERR
#include "test.h"

#define CLEAN_NAMELIST(nl) do { \
  UChar **_l = (nl), **_n = _l; \
  while (*_n) free(*(_n++)); \
  free(_l); \
} while (0)

int compare_namelists(UChar *expected[], UChar *observed[]);

int main(void) {
    char test_name[80] = "test_loop_add_item";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_loop_t *loop = NULL;
    cif_loop_t *loop2 = NULL;
    cif_pktitr_t *iterator = NULL;
    cif_packet_t *packet = NULL;
    cif_packet_t *packet2 = NULL;
    cif_value_t *value = NULL;
    cif_value_t *value1 = NULL;
    cif_value_t *value2 = NULL;
    cif_value_t *value3 = NULL;
    UChar *names[5];
    UChar **names2;
    UChar *category = NULL;
    int i;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(name1l, "_name1", 7);
    U_STRING_DECL(name2l, "_name2", 7);
    U_STRING_DECL(name3l, "_name3", 7);
    U_STRING_DECL(name4l, "_name4", 7);
    U_STRING_DECL(name1u, "_Name1", 7);
    U_STRING_DECL(name2u, "_NAME2", 7);
    U_STRING_DECL(name3u, "_nAMe3", 7);
    U_STRING_DECL(scalar1l, "_scalar1", 9);
    U_STRING_DECL(scalar2l, "_scalar2", 9);
    U_STRING_DECL(scalar3l, "_scalar3", 9);
    U_STRING_DECL(scalar1u, "_sCaLar1", 9);
    U_STRING_DECL(scalar2u, "_SCaLar2", 9);
    U_STRING_DECL(scalar3u, "_scalaR3", 9);
    U_STRING_DECL(invalid, "in valid", 9);


    /* Initialize data and prepare the test fixture */
    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(name1l, "_name1", 7);
    U_STRING_INIT(name2l, "_name2", 7);
    U_STRING_INIT(name3l, "_name3", 7);
    U_STRING_INIT(name4l, "_name4", 7);
    U_STRING_INIT(name1u, "_Name1", 7);
    U_STRING_INIT(name2u, "_NAME2", 7);
    U_STRING_INIT(name3u, "_nAMe3", 7);
    U_STRING_INIT(scalar1l, "_scalar1", 9);
    U_STRING_INIT(scalar2l, "_scalar2", 9);
    U_STRING_INIT(scalar3l, "_scalar3", 9);
    U_STRING_INIT(scalar1u, "_sCaLar1", 9);
    U_STRING_INIT(scalar2u, "_SCaLar2", 9);
    U_STRING_INIT(scalar3u, "_scalaR3", 9);
    U_STRING_INIT(invalid, "in valid", 9);

    names[0] = name1l;
    names[1] = NULL;
    names[2] = NULL;
    names[3] = NULL;
    names[4] = NULL;

    INIT_USTDERR;
    TESTHEADER(test_name);

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);

    TEST(cif_container_create_loop(block, NULL, names, &loop), CIF_OK, test_name, 1);
        /* block/loop: name1l */
    TEST(cif_value_create(CIF_NA_KIND, &value), CIF_OK, test_name, 2);

    /* Test adding an item to a zero-packet loop */
    TEST(cif_loop_add_item(loop, invalid, value), CIF_INVALID_ITEMNAME, test_name, 3);
    TEST(cif_loop_add_item(loop, name2l, value), CIF_OK, test_name, 4);
    TEST(cif_loop_add_item(loop, name2u, value), CIF_DUP_ITEMNAME, test_name, 5);
    cif_value_free(value);
    names[1] = name2l;
        /* check the success result more fully */
    TEST(cif_loop_get_names(loop, &names2), CIF_OK, test_name, 6);
    TEST(compare_namelists(names, names2), 0, test_name, 7);
    CLEAN_NAMELIST(names2);
    TEST(cif_loop_get_packets(loop, &iterator), CIF_EMPTY_LOOP, test_name, 8);

    /* Test adding an item to a single-packet loop (but not the scalar loop) */
        /* add one packet to the loop */
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 9);
    TEST(cif_packet_get_item(packet, name1l, &value1), CIF_OK, test_name, 10);
    TEST(cif_value_init_numb(value1, 1.0, 0.0, 0, 1), CIF_OK, test_name, 11);
    TEST(cif_packet_get_item(packet, name2l, &value2), CIF_OK, test_name, 12);
    TEST(cif_value_copy_char(value2, name2u), CIF_OK, test_name, 13);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 14);
        /* test adding an item */
    TEST(cif_value_create(CIF_NA_KIND, &value3), CIF_OK, test_name, 15);
    TEST(cif_loop_add_item(loop, invalid, value3), CIF_INVALID_ITEMNAME, test_name, 16);
    TEST(cif_loop_add_item(loop, name2u, value3), CIF_DUP_ITEMNAME, test_name, 17);
    TEST(cif_loop_add_item(loop, name3l, value3), CIF_OK, test_name, 18);
    names[2] = name3l;
        /* check the success result more fully */
    TEST(cif_loop_get_names(loop, &names2), CIF_OK, test_name, 19);
    TEST(compare_namelists(names, names2), 0, test_name, 20);
    CLEAN_NAMELIST(names2);
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 21);
    TEST(cif_pktitr_next_packet(iterator, &packet2), CIF_OK, test_name, 22);
    TEST(cif_packet_get_item(packet2, name1u, &value), CIF_OK, test_name, 23);
    TEST(!assert_values_equal(value, value1), 0, test_name, 24);
    TEST(cif_packet_get_item(packet2, name2u, &value), CIF_OK, test_name, 25);
    TEST(!assert_values_equal(value, value2), 0, test_name, 26);
    TEST(cif_packet_get_item(packet2, name3u, &value), CIF_OK, test_name, 27);
    TEST(!assert_values_equal(value, value3), 0, test_name, 28);
    TEST(cif_pktitr_next_packet(iterator, NULL), CIF_FINISHED, test_name, 29);
    cif_packet_free(packet2);
    TEST(cif_pktitr_close(iterator), CIF_OK, test_name, 30);
    value = NULL;

    /* test adding to the scalar loop */
        /* Ensure the scalar loop exists (creating it via cif_loop_create() is not supported) */
    TEST(cif_container_set_value(block, scalar1l, value1), CIF_OK, test_name, 31);
    TEST(cif_container_set_value(block, scalar2l, value2), CIF_OK, test_name, 32);
    TEST(cif_container_get_item_loop(block, scalar1l, &loop2), CIF_OK, test_name, 33);
    TEST(cif_loop_get_category(loop2, &category), CIF_OK, test_name, 34);
    TEST(*category, 0, test_name, 35);
    free(category);
    cif_loop_free(loop2);
    TEST(cif_container_get_item_loop(block, scalar2l, &loop2), CIF_OK, test_name, 36);
    TEST(cif_loop_get_category(loop2, &category), CIF_OK, test_name, 37);
    TEST(*category, 0, test_name, 38);
    free(category);
        /* Add a new item via cif_loop_add_item(), and verify it via cif_container_get_value() */
    TEST(cif_loop_add_item(loop2, scalar3u, value3), CIF_OK, test_name, 39);
    TEST(cif_container_get_value(block, scalar3l, &value), CIF_OK, test_name, 40);
    TEST(!assert_values_equal(value, value3), 0, test_name, 41);
        /* Clean up */
    cif_value_free(value);
    cif_value_free(value3);
    cif_packet_free(packet); /* value1 and value2 belong to 'packet' */
    cif_loop_free(loop2);

    TEST(cif_loop_destroy(loop), CIF_OK, test_name, 42);
    TEST(cif_container_get_item_loop(block, name1l, NULL), CIF_NOSUCH_ITEM, test_name, 43);
    TEST(cif_container_get_value(block, name1l, NULL), CIF_NOSUCH_ITEM, test_name, 44);
    TEST(cif_container_get_value(block, name2l, NULL), CIF_NOSUCH_ITEM, test_name, 45);

    /* test adding to a multi-packet loop */
        /* Create and populate the loop */
    names[1] = NULL;
    TEST(cif_container_create_loop(block, NULL, names, &loop), CIF_OK, test_name, 46);
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 47);
    TEST(cif_packet_get_item(packet, names[0], &value), CIF_OK, test_name, 48);
    for (i = 0; i < 3; i++) {
        TEST(cif_value_autoinit_numb(value, i, 0, 19), CIF_OK, test_name, 49 + (2 * i));
        TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 50 + (2 * i));
    }   /* Last subtest number == 54 */
    cif_packet_free(packet);  /* also frees 'value', which belongs to the packet */
    packet = NULL;
        /* Add an item to the loop definition */
    TEST(cif_value_create(CIF_NA_KIND, &value), CIF_OK, test_name, 55);
    TEST(cif_loop_add_item(loop, name2u, value), CIF_OK, test_name, 56);
    cif_value_free(value);
        /* check the success result more fully */
    names[1] = name2l;
    names[2] = NULL;
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 57);
    for (i = 0; i < 3; i++) {
        double d;

        TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 58 + (9 * i));
        TEST(cif_packet_get_item(packet, name1l, &value1), CIF_OK, test_name, 59 + (9 * i));
        TEST(cif_value_kind(value1), CIF_NUMB_KIND, test_name, 60 + (9 * i));
        TEST(cif_value_get_number(value1, &d), CIF_OK, test_name, 61 + (9 * i));
        TEST(d != (double) i, 0, test_name, 62 + (9 * i));
        TEST(cif_value_get_su(value1, &d), CIF_OK, test_name, 63 + (9 * i));
        TEST(d != 0.0, 0, test_name, 64 + (9 * i));
        TEST(cif_packet_get_item(packet, name2l, &value2), CIF_OK, test_name, 65 + (9 * i));
        TEST(cif_value_kind(value2), CIF_NA_KIND, test_name, 66 + (9 * i));
        /* value1 and value2 belong to the packet */
    }   /* Last subtest number == 84 */
    TEST(cif_pktitr_next_packet(iterator, NULL), CIF_FINISHED, test_name, 85);
    TEST(cif_pktitr_close(iterator), CIF_OK, test_name, 86);
    cif_packet_free(packet);
    packet = NULL;

    /* test adding a loop item with a NULL default value */

        /* Add the name.  There was once a bug causing this call to produce a segmentation fault. */
    TEST(cif_loop_add_item(loop, name4l, NULL), CIF_OK, test_name, 87);

        /* Check the packet values */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 88);
    for (i = 0; i < 3; i++) {
        double d;

        TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 89 + (11 * i));
        TEST(cif_packet_get_item(packet, name1l, &value1), CIF_OK, test_name, 90 + (11 * i));
        TEST(cif_value_kind(value1), CIF_NUMB_KIND, test_name, 91 + (11 * i));
        TEST(cif_value_get_number(value1, &d), CIF_OK, test_name, 92 + (11 * i));
        TEST(d != (double) i, 0, test_name, 93 + (11 * i));
        TEST(cif_value_get_su(value1, &d), CIF_OK, test_name, 94 + (11 * i));
        TEST(d != 0.0, 0, test_name, 95 + (11 * i));
        TEST(cif_packet_get_item(packet, name2l, &value2), CIF_OK, test_name, 96 + (11 * i));
        TEST(cif_value_kind(value2), CIF_NA_KIND, test_name, 97 + (11 * i));
        TEST(cif_packet_get_item(packet, name4l, &value3), CIF_OK, test_name, 98 + (11 * i));
        TEST(cif_value_kind(value3), CIF_UNK_KIND, test_name, 99 + (11 * i));
        /* value1, value2, and value3 belong to the packet */
    }   /* Last subtest number == 121 */
    TEST(cif_pktitr_next_packet(iterator, NULL), CIF_FINISHED, test_name, 122);
    TEST(cif_pktitr_close(iterator), CIF_OK, test_name, 123);
    cif_packet_free(packet);

    cif_loop_free(loop);

    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

#define U_BYTES(s) (u_strlen(s) * sizeof(UChar))
/*
 * Compares two NULL-terminated arrays of NUL-terminated Unicode strings to
 * determine whether their elements are equivalent under CIF name normalization
 * rules, albeit not necessarily in the same order.
 *
 * Returns 0 if the name lists are equivalent, otherwise nonzero
 */
int compare_namelists(UChar *expected[], UChar *observed[]) {
    struct set_element {
        UChar *normalized_name;
        int position;
        UT_hash_handle hh;
    } *name_set = NULL, *element, *temp_el;
    int rval;
    int counter;
    int num_names;

    /* load up a set of normalized item names for subsequent comparison */
    for (counter = 0; expected[counter] != NULL; counter += 1) {
        element = (struct set_element *) malloc(sizeof(struct set_element));
        if (cif_normalize(expected[counter], -1, &(element->normalized_name)) != CIF_OK) {
            rval = -(counter + 1);
            goto cleanup;
        }
        HASH_ADD_KEYPTR(hh, name_set, element->normalized_name, U_BYTES(element->normalized_name), element);
    }
    num_names = counter;

    /* compare the observed names to the expected ones */
    for (counter = 0; observed[counter] != NULL; counter += 1) {
        UChar *normalized;

        if (cif_normalize(observed[counter], -1, &normalized) != CIF_OK) {
            rval = -(num_names + 1);
            goto cleanup;
        }
        HASH_FIND(hh, name_set, normalized, U_BYTES(normalized), element);
        free(normalized);

        if (element == NULL) {
            /* not found */
            rval = counter + 1;
            goto cleanup;
        } else {
            /* this name must not be matched again */
            HASH_DEL(name_set, element);
            free(element->normalized_name);
            free(element);
        }
    }

    /* Check whether there are any expected names left over */
    if (name_set == NULL) {
        return 0;
    } else {
        rval = num_names + 1;
        /* fall through to cleanup: */
    }

    /* clean up any leftovers */
    cleanup:
    HASH_ITER(hh, name_set, element, temp_el) {
        free(element->normalized_name);
        free(element);
    }

    return rval;
}

