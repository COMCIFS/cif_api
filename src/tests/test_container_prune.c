/*
 * test_container_prune.c
 *
 * Tests general function of the CIF API's cif_container_prune() function.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_container_prune";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_block_t *block2 = NULL;
    cif_loop_t *loop = NULL;
    cif_packet_t *packet = NULL;
#define MAX_NAMES 5
    UChar *names[MAX_NAMES + 1];
    UChar *names2[MAX_NAMES + 1];
    int counter;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block_code2, "block2", 6);
    U_STRING_DECL(category, "category", 9);
    U_STRING_DECL(category2, "category2", 10);
    U_STRING_DECL(name1, "_name1", 7);
    U_STRING_DECL(name2, "_name2", 7);
    U_STRING_DECL(name3, "_name3", 7);
    U_STRING_DECL(name4, "_name4", 7);
    U_STRING_DECL(name5, "_name5", 7);

    /* Initialize data and prepare the test fixture */
    INIT_USTDERR;
    TESTHEADER(test_name);
    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block_code2, "block2", 6);
    U_STRING_INIT(category, "category", 9);
    U_STRING_INIT(category2, "category2", 10);
    U_STRING_INIT(name1, "_name1", 7);
    U_STRING_INIT(name2, "_name2", 7);
    U_STRING_INIT(name3, "_name3", 7);
    U_STRING_INIT(name4, "_name4", 7);
    U_STRING_INIT(name5, "_name5", 7);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block_code2, block2);

    /* Test pruning a simple loop from an otherwise-empty block */
      /* create it */
    names[0] = name1;
    names[1] = NULL;
    TEST(cif_container_create_loop(block, category, names, NULL), CIF_OK, test_name, 1);
      /* verify it's present */
    TEST(cif_container_get_item_loop(block, name1, NULL), CIF_OK, test_name, 2);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_OK, test_name, 3);
      /* prune it out, and verify it's gone */
    TEST(cif_container_prune(block), CIF_OK, test_name, 4);
    TEST(cif_container_get_item_loop(block, name1, NULL), CIF_NOSUCH_ITEM, test_name, 5);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_NOSUCH_LOOP, test_name, 6);

    /* Test pruning when there is no empty loop */
      /* when there are no loops at all */
    TEST(cif_container_prune(block), CIF_OK, test_name, 7);
      /* when there is a loop, but it's not empty */
    names[1] = name2;
    names[2] = NULL;
    TEST(cif_container_create_loop(block, category, names, &loop), CIF_OK, test_name, 8);
    TEST(cif_packet_create(&packet, names), CIF_OK, test_name, 9);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 10);
    cif_packet_free(packet);
    cif_loop_free(loop);
    loop = NULL;
    TEST(cif_container_prune(block), CIF_OK, test_name, 11);
        /* verify it's still present */
    TEST(cif_container_get_item_loop(block, name1, NULL), CIF_OK, test_name, 12);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_OK, test_name, 13);
      /* when there's an empty loop, but it's in a different container */
    TEST(cif_container_create_loop(block2, category, names, NULL), CIF_OK, test_name, 14);
    TEST(cif_container_prune(block), CIF_OK, test_name, 15);
        /* verify it's still present in both containers*/
    TEST(cif_container_get_item_loop(block, name1, NULL), CIF_OK, test_name, 16);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_OK, test_name, 17);
    TEST(cif_container_get_item_loop(block2, name1, NULL), CIF_OK, test_name, 18);
    TEST(cif_container_get_category_loop(block2, category, NULL), CIF_OK, test_name, 19);

    /* test pruning one loop among several */
      /* create the needed additional loops */
    names2[0] = name3;
    names2[1] = name4;
    names2[2] = NULL;
        /* an empty loop */
    TEST(cif_container_create_loop(block, category2, names2, NULL), CIF_OK, test_name, 20);
        /* another nonempty loop (the scalar loop) */
    TEST(cif_container_set_value(block, name5, NULL), CIF_OK, test_name, 21);
      /* prune and check */
    TEST(cif_container_prune(block), CIF_OK, test_name, 22);
    TEST(cif_container_get_item_loop(block, name1, NULL), CIF_OK, test_name, 23);
    TEST(cif_container_get_category_loop(block, category, NULL), CIF_OK, test_name, 24);
    TEST(cif_container_get_value(block, name5, NULL), CIF_OK, test_name, 25);

    /* clean up */
    DESTROY_BLOCK(test_name, block2);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

