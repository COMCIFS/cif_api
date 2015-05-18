/*
 * test_loop_set_category.c
 *
 * Tests general function of the CIF API's cif_container_create_loop() function.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"

#include "test.h"

int main(void) {
    char test_name[80] = "test_loop_set_category";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_block_tp *block2 = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp *loop = NULL;
    cif_loop_tp *loop2 = NULL;
    UChar *temp;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block2_code, "block2", 7);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(category, "category", 9);
    U_STRING_DECL(category2, "category2", 10);
    U_STRING_DECL(empty, "", 1);
    U_STRING_DECL(category4, " ", 2);
    U_STRING_DECL(name1, "_1", 3);
    U_STRING_DECL(name2, "_two", 5);
    U_STRING_DECL(name3, "_III", 5);
    U_STRING_DECL(name4, "_other", 7);
    U_STRING_DECL(name5, "_five", 6);
    UChar *names[4];

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    INIT_USTDERR;

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block2_code, "block2", 7);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(category, "category", 9);
    U_STRING_INIT(category2, "category2", 10);
    U_STRING_INIT(empty, "", 1);
    U_STRING_INIT(category4, " ", 2);
    U_STRING_INIT(name1, "_1", 3);
    U_STRING_INIT(name2, "_two", 5);
    U_STRING_INIT(name3, "_III", 5);
    U_STRING_INIT(name4, "_other", 7);
    U_STRING_INIT(name5, "_five", 6);
    names[0] = name1;
    names[1] = name2;
    names[2] = name3;
    names[3] = NULL;

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block2_code, block2);
    CREATE_FRAME(test_name, block, frame_code, frame);

    TEST(cif_container_create_loop(block, category, names, &loop), CIF_OK, test_name, 1);
    TEST(cif_container_create_loop(block2, category, names, NULL), CIF_OK, test_name, 2);
    TEST(cif_container_create_loop(frame, category, names, NULL), CIF_OK, test_name, 3);

    TEST(cif_container_get_category_loop(block, category, NULL), CIF_OK, test_name, 4);
    TEST(cif_container_get_category_loop(block2, category, NULL), CIF_OK, test_name, 5);
    TEST(cif_container_get_category_loop(frame, category, NULL), CIF_OK, test_name, 6);
    TEST(cif_container_get_category_loop(block, category2, NULL), CIF_NOSUCH_LOOP, test_name, 7);
    TEST(cif_container_get_category_loop(block2, category2, NULL), CIF_NOSUCH_LOOP, test_name, 8);
    TEST(cif_container_get_category_loop(frame, category2, NULL), CIF_NOSUCH_LOOP, test_name, 9);

    /* Test changing the loop category to a different non-NULL category */
    TEST(cif_loop_set_category(loop, category2), CIF_OK, test_name, 10);
    TEST(cif_loop_get_category(loop, &temp), CIF_OK, test_name, 11);
    TEST(temp == NULL, 0, test_name, 12);
    TEST(u_strcmp(category2, temp), 0, test_name, 13);
    free(temp);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_NOSUCH_LOOP, test_name, 14);
    TEST(cif_container_get_category_loop(block2, category, NULL), CIF_OK, test_name, 15);
    TEST(cif_container_get_category_loop(frame, category, NULL), CIF_OK, test_name, 16);
    TEST(cif_container_get_category_loop(block, category2, NULL), CIF_OK, test_name, 17);
    TEST(cif_container_get_category_loop(block2, category2, NULL), CIF_NOSUCH_LOOP, test_name, 18);
    TEST(cif_container_get_category_loop(frame, category2, NULL), CIF_NOSUCH_LOOP, test_name, 19);

    /* Test changing the loop category to NULL */
    TEST(cif_loop_set_category(loop, NULL), CIF_OK, test_name, 20);
    TEST(cif_loop_get_category(loop, &temp), CIF_OK, test_name, 21);
    TEST(temp != NULL, 0, test_name, 22);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_NOSUCH_LOOP, test_name, 23);
    TEST(cif_container_get_category_loop(block2, category, NULL), CIF_OK, test_name, 24);
    TEST(cif_container_get_category_loop(frame, category, NULL), CIF_OK, test_name, 25);
    TEST(cif_container_get_category_loop(block, category2, NULL), CIF_NOSUCH_LOOP, test_name, 26);
    TEST(cif_container_get_category_loop(block2, category2, NULL), CIF_NOSUCH_LOOP, test_name, 27);
    TEST(cif_container_get_category_loop(frame, category2, NULL), CIF_NOSUCH_LOOP, test_name, 28);
    cif_loop_free(loop);
    TEST(cif_container_get_item_loop(block, name1, &loop), CIF_OK, test_name, 29);
    TEST(cif_loop_get_category(loop, &temp), CIF_OK, test_name, 30);
    TEST(temp != NULL, 0, test_name, 31);
    
    /* Test changing the NULL loop category to non-NULL */
    TEST(cif_loop_set_category(loop, category4), CIF_OK, test_name, 32);
    TEST(cif_loop_get_category(loop, &temp), CIF_OK, test_name, 33);
    TEST(temp == NULL, 0, test_name, 34);
    TEST(u_strcmp(category4, temp), 0, test_name, 35);
    free(temp);
    TEST(cif_container_get_category_loop(block, category4, NULL), CIF_OK, test_name, 36);
    TEST(cif_container_get_category_loop(block2, category4, NULL), CIF_NOSUCH_LOOP, test_name, 37);
    TEST(cif_container_get_category_loop(frame, category4, NULL), CIF_NOSUCH_LOOP, test_name, 38);

    /* Attempt to change to the reserved category */
    TEST(cif_container_get_category_loop(block, empty, NULL), CIF_NOSUCH_LOOP, test_name, 39);
    TEST(cif_loop_set_category(loop, empty), CIF_RESERVED_LOOP, test_name, 40);
    TEST(cif_container_get_category_loop(block, category4, NULL), CIF_OK, test_name, 41);
    TEST(cif_container_get_category_loop(block, empty, NULL), CIF_NOSUCH_LOOP, test_name, 42);

    /* Test changing to a duplicate category */
    names[0] = name5;
    names[1] = NULL;
    TEST(cif_container_create_loop(block, category, names, &loop2), CIF_OK, test_name, 43);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_OK, test_name, 44);
    TEST(cif_loop_set_category(loop, category), CIF_OK, test_name, 45);
    TEST(cif_container_get_category_loop(block, category4, NULL), CIF_NOSUCH_LOOP, test_name, 46);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_CAT_NOT_UNIQUE, test_name, 47);

    /* Test changing the category of the scalar loop */
    cif_loop_free(loop2);
    TEST(cif_container_set_value(block, name4, NULL), CIF_OK, test_name, 48);
    TEST(cif_container_get_category_loop(block, empty, &loop2), CIF_OK, test_name, 49);
    TEST(cif_loop_get_category(loop2, &temp), CIF_OK, test_name, 50);
    TEST(*temp, 0, test_name, 51);
    TEST(temp == NULL, 0, test_name, 52);
    free(temp);
    TEST(cif_loop_set_category(loop2, category2), CIF_RESERVED_LOOP, test_name, 53);

    /* Test "changing" the category to the value it already has */
    TEST(cif_loop_set_category(loop, category), CIF_OK, test_name, 54);

    /* Test changing the category of a deleted loop */
    cif_loop_free(loop2);
    TEST(cif_container_get_item_loop(block, name1, &loop2), CIF_OK, test_name, 55);
        /* 'loop2' is an independent handle on the same loop as 'loop' */
    TEST(cif_loop_destroy(loop2), CIF_OK, test_name, 56);
    TEST(cif_loop_set_category(loop, category2), CIF_INVALID_HANDLE, test_name, 57);

    /* clean up */
    cif_loop_free(loop);
    DESTROY_FRAME(test_name, frame);
    DESTROY_BLOCK(test_name, block2);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

