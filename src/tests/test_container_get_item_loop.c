/*
 * test_container_get_item_loop.c
 *
 * Tests behaviors of the CIF API's cif_container_get_item_loop() function that are not already tested in the
 * loop creation tests.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_container_get_item_loop";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_frame_t *frame = NULL;
    cif_loop_t *loop;
    UChar *item_names[4] = { NULL, NULL, NULL, NULL };
    UChar **item_names_p;
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

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    item_names[0] = item1u;
    TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_OK, test_name, 1);

    item_names[0] = item2u;
    item_names[1] = item3u;
    TEST(cif_container_create_loop(block, NULL, item_names, NULL), CIF_OK, test_name, 2);

    /* Test with a couple of types of invalid item names */
    loop = NULL;
    TEST(cif_container_get_item_loop(block, NULL, &loop), CIF_NOSUCH_ITEM, test_name, 3);
    TEST((loop != NULL), 0, test_name, 4);
    TEST(cif_container_get_item_loop(block, invalid, &loop), CIF_NOSUCH_ITEM, test_name, 5);
    TEST((loop != NULL), 0, test_name, 6);

    /* Test retrieval by exact name match */
    TEST(cif_container_get_item_loop(block, item1u, &loop), CIF_OK, test_name, 7);
    TEST((loop == NULL), 0, test_name, 8);
    TEST(cif_loop_get_names(loop, &item_names_p), CIF_OK, test_name, 9);
    TEST((item_names_p == NULL), 0, test_name, 10);
    TEST((item_names_p[0] == NULL), 0, test_name, 11);
    TEST((item_names_p[1] != NULL), 0, test_name, 12);
    TEST(u_strcmp(item1u, item_names_p[0]), 0, test_name, 13);
    free(item_names_p[0]);
    free(item_names_p);
    cif_loop_free(loop);

    TEST(cif_container_get_item_loop(block, item3u, &loop), CIF_OK, test_name, 14);
    TEST((loop == NULL), 0, test_name, 15);
    TEST(cif_loop_get_names(loop, &item_names_p), CIF_OK, test_name, 16);
    TEST((item_names_p == NULL), 0, test_name, 17);
    TEST((item_names_p[0] == NULL), 0, test_name, 18);
    TEST((item_names_p[1] == NULL), 0, test_name, 19);
    TEST((item_names_p[2] != NULL), 0, test_name, 20);
    TEST((u_strcmp(item2u, item_names_p[0]) * u_strcmp(item2u, item_names_p[1])), 0, test_name, 21);
    TEST((u_strcmp(item3u, item_names_p[0]) * u_strcmp(item3u, item_names_p[1])), 0, test_name, 22);
    free(item_names_p[0]);
    free(item_names_p[1]);
    free(item_names_p);
    cif_loop_free(loop);

    /* Create other-container loops */
    item_names[1] = NULL;
    TEST(cif_container_create_loop(frame, NULL, item_names, NULL), CIF_OK, test_name, 23);
    item_names[0] = item3u;
    item_names[1] = item1u;
    TEST(cif_container_create_loop(frame, NULL, item_names, NULL), CIF_OK, test_name, 24);

    /* Test case-insensitive matching with other-container loops present */
    TEST(cif_container_get_item_loop(block, item1l, &loop), CIF_OK, test_name, 25);
    TEST((loop == NULL), 0, test_name, 26);
    TEST(cif_loop_get_names(loop, &item_names_p), CIF_OK, test_name, 27);
    TEST((item_names_p == NULL), 0, test_name, 28);
    TEST((item_names_p[0] == NULL), 0, test_name, 29);
    TEST((item_names_p[1] != NULL), 0, test_name, 30);
    TEST(u_strcmp(item1u, item_names_p[0]), 0, test_name, 31);
    free(item_names_p[0]);
    free(item_names_p);
    cif_loop_free(loop);

    TEST(cif_container_get_item_loop(block, item3l, &loop), CIF_OK, test_name, 32);
    TEST((loop == NULL), 0, test_name, 33);
    TEST(cif_loop_get_names(loop, &item_names_p), CIF_OK, test_name, 34);
    TEST((item_names_p == NULL), 0, test_name, 35);
    TEST((item_names_p[0] == NULL), 0, test_name, 36);
    TEST((item_names_p[1] == NULL), 0, test_name, 37);
    TEST((item_names_p[2] != NULL), 0, test_name, 38);
    TEST((u_strcmp(item2u, item_names_p[0]) * u_strcmp(item2u, item_names_p[1])), 0, test_name, 39);
    TEST((u_strcmp(item3u, item_names_p[0]) * u_strcmp(item3u, item_names_p[1])), 0, test_name, 40);
    free(item_names_p[0]);
    free(item_names_p[1]);
    free(item_names_p);
    cif_loop_free(loop);

    /* Test with a valid but missing item name */
    loop = NULL;
    TEST(cif_container_get_item_loop(block, item4l, &loop), CIF_NOSUCH_ITEM, test_name, 41);
    TEST((loop != NULL), 0, test_name, 42);

    DESTROY_CIF(test_name, cif);

    return 0;
}

