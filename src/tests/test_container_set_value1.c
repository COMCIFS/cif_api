/*
 * test_container_set_value1.c
 *
 * Tests behaviors of the CIF API's cif_container_set_value() function for scalar values.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"
#include "assert_value.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_container_set_value1";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_frame_t *frame = NULL;
    cif_loop_t *loop;
    cif_value_t *value1;
    cif_value_t *value2;
    cif_value_t *value3;
    UChar key0[1] = { 0 };
    UChar key1[3] = { 0x20, 0x20, 0 };
    UChar key2[4] = { 0x41, 0x7b, 0x7d, 0 };
    UChar key3[4] = { 0x61, 0x7b, 0x7d, 0 };
    UChar key4[7] = { 0x23, 0xd800, 0xdc01, 0x20, 0x09, 0x27, 0 };
    UChar *ustr;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(item1l, "_item1", 7);
    U_STRING_DECL(item2l, "_item2", 7);
    U_STRING_DECL(item3l, "_item3", 7);
    U_STRING_DECL(item4l, "_item4", 7);
    U_STRING_DECL(item5l, "_item5", 7);
    U_STRING_DECL(item6l, "_item6", 7);
    U_STRING_DECL(item1u, "_Item1", 7);
    U_STRING_DECL(item2u, "_ITEM2", 7);
    U_STRING_DECL(item3u, "_iTeM3", 7);
    U_STRING_DECL(invalid, "in valid", 9);
    U_STRING_DECL(char_value1, "simple_Value", 13);
    U_STRING_DECL(psuedo_numb, "1", 2);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(item1l, "_item1", 7);
    U_STRING_INIT(item2l, "_item2", 7);
    U_STRING_INIT(item3l, "_item3", 7);
    U_STRING_INIT(item4l, "_item4", 7);
    U_STRING_INIT(item5l, "_item5", 7);
    U_STRING_INIT(item6l, "_item6", 7);
    U_STRING_INIT(item1u, "_Item1", 7);
    U_STRING_INIT(item2u, "_ITEM2", 7);
    U_STRING_INIT(item3u, "_iTeM3", 7);
    U_STRING_INIT(invalid, "in valid", 9);
    U_STRING_INIT(char_value1, "simple_Value", 13);
    U_STRING_INIT(psuedo_numb, "1", 2);

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    TEST(cif_value_create(CIF_UNK_KIND, &value1), CIF_OK, test_name, 1);
    TEST(cif_value_copy_char(value1, char_value1), CIF_OK, test_name, 2);
    TEST(cif_container_get_item_loop(block, item1u, NULL), CIF_NOSUCH_ITEM, test_name, 3);
    TEST(cif_container_get_item_loop(frame, item1u, NULL), CIF_NOSUCH_ITEM, test_name, 4);

    /* test setting a value in an empty container (char) */
    TEST(cif_container_set_value(block, item1u, value1), CIF_OK, test_name, 5);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item1u, NULL), CIF_NOSUCH_ITEM, test_name, 6);
    TEST(cif_container_get_item_loop(block, item1u, &loop), CIF_OK, test_name, 7);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 8);
    TEST(ustr == NULL, 0, test_name, 9);
    TEST(ustr[0], 0, test_name, 10);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    value2 = NULL;
    TEST(cif_container_get_value(block, item1u, &value2), CIF_OK, test_name, 11);
    TEST(value2 == value1, 0, test_name, 12);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 13);
    cif_value_free(value2);
    value2 = NULL;

    /* test setting a second value in the same container (numb) */
    TEST(cif_value_init_numb(value1, 42.0, 0.5, 1, 6), CIF_OK, test_name, 16);
    TEST(cif_container_get_item_loop(block, item2u, NULL), CIF_NOSUCH_ITEM, test_name, 17);
    TEST(cif_container_get_item_loop(frame, item2u, NULL), CIF_NOSUCH_ITEM, test_name, 18);
    TEST(cif_container_set_value(block, item2u, value1), CIF_OK, test_name, 19);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item2u, NULL), CIF_NOSUCH_ITEM, test_name, 20);
    TEST(cif_container_get_item_loop(block, item2u, &loop), CIF_OK, test_name, 21);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 22);
    TEST(ustr == NULL, 0, test_name, 23);
    TEST(ustr[0], 0, test_name, 24);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(block, item2u, &value2), CIF_OK, test_name, 25);
    TEST(value2 == value1, 0, test_name, 26);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 27);
    cif_value_free(value2);
    value2 = NULL;

    /* test setting a third value in the same container (na) */
    TEST(cif_value_init(value1, CIF_NA_KIND), CIF_OK, test_name, 32);
    TEST(cif_container_get_item_loop(block, item3u, NULL), CIF_NOSUCH_ITEM, test_name, 33);
    TEST(cif_container_get_item_loop(frame, item3u, NULL), CIF_NOSUCH_ITEM, test_name, 34);
    TEST(cif_container_set_value(block, item3u, value1), CIF_OK, test_name, 35);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item3u, NULL), CIF_NOSUCH_ITEM, test_name, 36);
    TEST(cif_container_get_item_loop(block, item3u, &loop), CIF_OK, test_name, 37);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 38);
    TEST(ustr == NULL, 0, test_name, 39);
    TEST(ustr[0], 0, test_name, 40);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(block, item3u, &value2), CIF_OK, test_name, 41);
    TEST(value2 == value1, 0, test_name, 42);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 43);
    cif_value_free(value2);
    value2 = NULL;

    /* test setting a fourth value in the same container (unk) */
    TEST(cif_value_clean(value1), CIF_OK, test_name, 44);
    TEST(cif_container_get_item_loop(block, item4l, NULL), CIF_NOSUCH_ITEM, test_name, 45);
    TEST(cif_container_get_item_loop(frame, item4l, NULL), CIF_NOSUCH_ITEM, test_name, 46);
    TEST(cif_container_set_value(block, item4l, value1), CIF_OK, test_name, 47);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item4l, NULL), CIF_NOSUCH_ITEM, test_name, 48);
    TEST(cif_container_get_item_loop(block, item4l, &loop), CIF_OK, test_name, 49);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 50);
    TEST(ustr == NULL, 0, test_name, 51);
    TEST(ustr[0], 0, test_name, 52);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(block, item4l, &value2), CIF_OK, test_name, 53);
    TEST(value2 == value1, 0, test_name, 54);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 55);
    cif_value_free(value2);
    value2 = NULL;

    /* test setting a fifth value in the same container (list) */
    TEST(cif_value_init(value1, CIF_LIST_KIND), CIF_OK, test_name, 56);
    TEST(cif_value_create(CIF_UNK_KIND, &value3), CIF_OK, test_name, 57);
    TEST(cif_value_copy_char(value3, psuedo_numb), CIF_OK, test_name, 58);
    TEST(cif_value_insert_element_at(value1, 0, value3), CIF_OK, test_name, 59);
    TEST(cif_value_init_numb(value3, 2.0, 1.0, 1, 1), CIF_OK, test_name, 60);
    TEST(cif_value_insert_element_at(value1, 1, value3), CIF_OK, test_name, 61);
    TEST(cif_value_init_numb(value3, 3.0, 0.0, 1, 1), CIF_OK, test_name, 62);
    TEST(cif_value_insert_element_at(value1, 2, value3), CIF_OK, test_name, 63);
    TEST(cif_value_init(value3, CIF_UNK_KIND), CIF_OK, test_name, 64);
    TEST(cif_value_insert_element_at(value1, 3, value3), CIF_OK, test_name, 65);
    TEST(cif_value_init(value3, CIF_NA_KIND), CIF_OK, test_name, 66);
    TEST(cif_value_insert_element_at(value1, 4, value3), CIF_OK, test_name, 67);
    cif_value_free(value3);
    TEST(cif_container_get_item_loop(block, item5l, NULL), CIF_NOSUCH_ITEM, test_name, 68);
    TEST(cif_container_get_item_loop(frame, item5l, NULL), CIF_NOSUCH_ITEM, test_name, 69);
    TEST(cif_container_set_value(block, item5l, value1), CIF_OK, test_name, 70);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item5l, NULL), CIF_NOSUCH_ITEM, test_name, 71);
    TEST(cif_container_get_item_loop(block, item5l, &loop), CIF_OK, test_name, 72);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 73);
    TEST(ustr == NULL, 0, test_name, 74);
    TEST(ustr[0], 0, test_name, 75);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(block, item5l, &value2), CIF_OK, test_name, 76);
    TEST(value2 == value1, 0, test_name, 77);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 78);
    cif_value_free(value2);
    value2 = NULL;

    /* test setting a sixth value in the same container (table) */
    TEST(cif_value_init(value1, CIF_TABLE_KIND), CIF_OK, test_name, 79);
    TEST(cif_value_create(CIF_UNK_KIND, &value3), CIF_OK, test_name, 80);
    TEST(cif_value_copy_char(value3, psuedo_numb), CIF_OK, test_name, 81);
    TEST(cif_value_set_item_by_key(value1, key0, value3), CIF_OK, test_name, 82);
    TEST(cif_value_init_numb(value3, 2.0, 1.0, 1, 1), CIF_OK, test_name, 83);
    TEST(cif_value_set_item_by_key(value1, key1, value3), CIF_OK, test_name, 84);
    TEST(cif_value_init_numb(value3, 3.0, 0.0, 1, 1), CIF_OK, test_name, 85);
    TEST(cif_value_set_item_by_key(value1, key2, value3), CIF_OK, test_name, 86);
    TEST(cif_value_init(value3, CIF_UNK_KIND), CIF_OK, test_name, 87);
    TEST(cif_value_set_item_by_key(value1, key3, value3), CIF_OK, test_name, 88);
    TEST(cif_value_init(value3, CIF_NA_KIND), CIF_OK, test_name, 89);
    TEST(cif_value_set_item_by_key(value1, key4, value3), CIF_OK, test_name, 90);
    cif_value_free(value3);
    value3 = NULL;
    TEST(cif_container_get_item_loop(block, item6l, NULL), CIF_NOSUCH_ITEM, test_name, 91);
    TEST(cif_container_get_item_loop(frame, item6l, NULL), CIF_NOSUCH_ITEM, test_name, 92);
    TEST(cif_container_set_value(block, item6l, value1), CIF_OK, test_name, 93);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item6l, NULL), CIF_NOSUCH_ITEM, test_name, 94);
    TEST(cif_container_get_item_loop(block, item6l, &loop), CIF_OK, test_name, 95);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 96);
    TEST(ustr == NULL, 0, test_name, 97);
    TEST(ustr[0], 0, test_name, 98);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(block, item6l, &value2), CIF_OK, test_name, 99);
    TEST(value2 == value1, 0, test_name, 100);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 101);
    cif_value_free(value2);
    value2 = NULL;

    /* test modifying a (scalar) value already set in the container */
    TEST(cif_value_init_numb(value1, 17.50, 0.25, 2, 6), CIF_OK, test_name, 102);
    TEST(cif_container_get_item_loop(block, item1l, NULL), CIF_OK, test_name, 103);
    TEST(cif_container_get_item_loop(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 104);
    TEST(cif_container_set_value(block, item1l, value1), CIF_OK, test_name, 105);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item1l, NULL), CIF_NOSUCH_ITEM, test_name, 106);
    TEST(cif_container_get_item_loop(block, item1l, &loop), CIF_OK, test_name, 107);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 108);
    TEST(ustr == NULL, 0, test_name, 109);
    TEST(ustr[0], 0, test_name, 110);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(block, item1l, &value2), CIF_OK, test_name, 111);
    TEST(value2 == value1, 0, test_name, 112);
    TEST(!assert_values_equal(value1, value2), 0, test_name, 113);
    cif_value_free(value2);
    value2 = NULL;

    /* test a different container for cross-container bleed on add or modify */
        /* get current value of item5l from block */
    TEST(cif_container_get_value(block, item5l, &value3), CIF_OK, test_name, 114);
        /* verify that the value in the block is not what we are about to set in the frame */
    TEST(assert_values_equal(value1, value3), 0, test_name, 115);
        /* set item5l in frame */
    TEST(cif_container_set_value(frame, item5l, value1), CIF_OK, test_name, 116);
        /* get newly-set value of item5l from frame */
    TEST(cif_container_get_value(frame, item5l, &value2), CIF_OK, test_name, 117);
        /* verify that the value was set correctly in the frame */
    TEST(!assert_values_equal(value1, value2), 0, test_name, 118);
    cif_value_free(value2);
    value2 = NULL;
        /* verify that the value of item5l did not change in the block */
    TEST(cif_container_get_value(block, item5l, &value2), CIF_OK, test_name, 119);
    TEST(!assert_values_equal(value3, value2), 0, test_name, 120);

    TEST(cif_value_init(value1, CIF_LIST_KIND), CIF_OK, test_name, 121);
        /* modify item5l in frame */
    TEST(cif_container_set_value(frame, item5l, value1), CIF_OK, test_name, 122);
        /* get modified value of item5l from frame */
    TEST(cif_container_get_value(frame, item5l, &value2), CIF_OK, test_name, 123);
        /* verify that the value was updated correctly in the frame */
    TEST(!assert_values_equal(value1, value2), 0, test_name, 124);
    cif_value_free(value2);
    value2 = NULL;
        /* verify that the value of item5l did not change in the block */
    TEST(cif_container_get_value(block, item5l, &value2), CIF_OK, test_name, 119);
    TEST(!assert_values_equal(value3, value2), 0, test_name, 120);
    cif_value_free(value3);
    cif_value_free(value2);
    value2 = NULL;

    /* test setting a NULL value */
    TEST(cif_value_clean(value1), CIF_OK, test_name, 121);
    TEST(cif_container_get_item_loop(frame, item6l, NULL), CIF_NOSUCH_ITEM, test_name, 122);
    TEST(cif_container_set_value(frame, item6l, NULL), CIF_OK, test_name, 123);
    /* verify that the item went into the scalar loop, in (only) the right container */
    TEST(cif_container_get_item_loop(frame, item6l, &loop), CIF_OK, test_name, 124);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 125);
    TEST(ustr == NULL, 0, test_name, 126);
    TEST(ustr[0], 0, test_name, 127);
    free(ustr);
    cif_loop_free(loop);
    /* read back the value and check it */
    TEST(cif_container_get_value(frame, item6l, &value2), CIF_OK, test_name, 128);
    TEST(cif_value_kind(value2), CIF_UNK_KIND, test_name, 129);
    cif_value_free(value2);
    value2 = NULL;
    cif_value_free(value1);

    /* test removing values */
    TEST(cif_container_remove_item(frame, item5l), CIF_OK, test_name, 130);
    TEST(cif_container_get_value(frame, item5l, &value2), CIF_NOSUCH_ITEM, test_name, 131);
    TEST(cif_container_get_value(block, item5l, &value2), CIF_OK, test_name, 132);
    cif_value_free(value2);
    value2 = NULL;
    TEST(cif_container_remove_item(frame, item6l), CIF_OK, test_name, 133);
    TEST(cif_container_get_value(frame, item6l, &value2), CIF_NOSUCH_ITEM, test_name, 134);
    TEST(cif_container_get_value(block, item6l, &value2), CIF_OK, test_name, 135);
    cif_value_free(value2);

    /* The item-less loop should be destroyed (?) */
    TEST(cif_container_get_category_loop(frame, CIF_SCALARS, &loop), CIF_NOSUCH_LOOP, test_name, 136);
    TEST(cif_container_get_category_loop(block, CIF_SCALARS, &loop), CIF_OK, test_name, 137);
    cif_loop_free(loop);

    TEST(cif_container_get_item_loop(block, invalid, NULL), CIF_NOSUCH_ITEM, test_name, 138);
    TEST(cif_container_set_value(block, invalid, NULL), CIF_INVALID_ITEMNAME, test_name, 139);
    TEST(cif_container_get_item_loop(block, invalid, NULL), CIF_NOSUCH_ITEM, test_name, 140);

    cif_frame_free(frame);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

