/*
 * test_container_get_all_loops.c
 *
 * Tests behaviors of the CIF API's cif_container_get_all_loops() function that are not already tested in the
 * loop creation tests.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>
#include "../cif.h"
#include "test.h"

#define NAME_UCHARS 8
#define NUM_NAMES 8
#define EXPECTED_LOOPS 4
int main(int argc, char *argv[]) {
    char test_name[80] = "test_container_get_all_loops";
    cif_t *cif = NULL;
    cif_block_t *block1 = NULL;
    cif_block_t *block2 = NULL;
    cif_frame_t *frame = NULL;
    cif_loop_t **loops;
    cif_loop_t **loop_ctr;
    UChar item_names[NUM_NAMES][NAME_UCHARS];
    UChar *loop_items[NUM_NAMES + 2];
    UChar *expected_items[EXPECTED_LOOPS][NUM_NAMES + 1];
    int counter;
    int offset;
    int loop_mask;
    int subtest;
    U_STRING_DECL(block1_code, "block1", 7);
    U_STRING_DECL(block2_code, "block2", 7);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(category, "category", 9);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    U_STRING_INIT(block1_code, "block1", 7);
    U_STRING_INIT(block2_code, "block2", 7);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(category, "category", 9);
    for (counter = 0; counter < NUM_NAMES; counter += 1) {
        u_sprintf(item_names[counter], "_item%d", counter);
        loop_items[counter] = NULL;
    }
    loop_items[NUM_NAMES] = NULL;

    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block1_code, block1);
    CREATE_BLOCK(test_name, cif, block2_code, block2);
    CREATE_FRAME(test_name, block1, frame_code, frame);

    /*
     * The loops we will test retrieving:
     */

    loop_items[0] = item_names[0];
    loop_items[1] = NULL;
    TEST(cif_container_create_loop(block1, NULL, loop_items, NULL), CIF_OK, test_name, 1);
    memcpy(expected_items[0], loop_items, 2 * sizeof(UChar *));

    loop_items[0] = item_names[NUM_NAMES - 1];
    TEST(cif_container_create_loop(block1, CIF_SCALARS, loop_items, NULL), CIF_OK, test_name, 2);
    memcpy(expected_items[1], loop_items, 2 * sizeof(UChar *));

    /* The middle names are split between two loops */
    offset = (NUM_NAMES - 2) / 2;
    for (counter = 0; counter < offset; counter += 1) {
        loop_items[counter] = item_names[counter + 1];
        loop_items[counter + 1 + offset] = item_names[counter + 1 + offset];
    }
    loop_items[counter] = NULL;
    loop_items[counter + offset] = NULL;
    TEST(cif_container_create_loop(block1, category, loop_items, NULL), CIF_OK, test_name, 3);
    TEST(cif_container_create_loop(block1, category, loop_items + 1 + offset, NULL), CIF_OK, test_name, 4);
    memcpy(expected_items[2], loop_items, (offset + 1) * sizeof(UChar *));
    memcpy(expected_items[3], loop_items + offset + 1, (offset + 1) * sizeof(UChar *));

    /*
     * Loops that we will verify are NOT retrieved:
     */

    offset = (NUM_NAMES / 2) + 1;
    for (counter = 0; counter < NUM_NAMES; counter += 2) {
        loop_items[(counter % 2) * offset + (counter / 2)] = item_names[counter];
    }
    loop_items[offset - 1] = NULL;
    loop_items[NUM_NAMES + 1] = NULL;
    TEST(cif_container_create_loop(block2, category, loop_items, NULL), CIF_OK, test_name, 5);
    TEST(cif_container_create_loop(block2, CIF_SCALARS, loop_items + offset, NULL), CIF_OK, test_name, 6);
  
    loop_items[2] = NULL; 
    for (counter = 0; counter < NUM_NAMES; counter += 1) {
      loop_items[counter % 2] = item_names[counter];
      if ((counter % 2) == 1) {
        TEST(cif_container_create_loop(frame, NULL, loop_items, NULL), CIF_OK, test_name, (counter / 2) + 7);
      }
    }

    TEST(cif_container_get_all_loops(NULL, &loops), CIF_INVALID_HANDLE, test_name, 11);
    TEST(cif_container_get_all_loops(block1, &loops), CIF_OK, test_name, 13);

    /* test that all the correct loops were returned (and only those) */
    loop_mask = (1 << EXPECTED_LOOPS) - 1;
    subtest = 14;
    INIT_USTDERR;
    for (loop_ctr = loops; *loop_ctr != NULL; loop_ctr += 1) {
        UChar **observed_items = NULL;
        UChar **name_ctr;

        /* Retrieve the names for the loop, and record them in a set */
        TEST(cif_loop_get_names(*loop_ctr, &observed_items), CIF_OK, test_name, subtest++); {
            struct set_el *head = NULL;

            for (counter = 0; observed_items[counter] != NULL; counter += 1) {
                struct set_el *el = (struct set_el *) malloc(sizeof(struct set_el));

                if (el == NULL) return HARD_FAIL;
                HASH_ADD_KEYPTR(hh, head, observed_items[counter], u_strlen(observed_items[counter]) * sizeof(UChar),
                                el);
            }
            u_fflush(ustderr);
            TEST(HASH_COUNT(head), counter, test_name, subtest++); {
                /* Attempt to match up the observed names to one of the as-yet unmatched expected lists */
                for (counter = 0; counter < EXPECTED_LOOPS; counter += 1) {
                    if ((loop_mask & (1 << counter)) != 0) {
                        struct set_el *el;

                        name_ctr = expected_items[counter];
                        HASH_FIND(hh, head, *name_ctr, u_strlen(*name_ctr) * sizeof(UChar), el);
                        if (el != NULL) { /* matched one name; the rest in this list should also match */
                            loop_mask ^= (1 << counter);
                            do {
                                HASH_DEL(head, el);
                                free(el);
                                name_ctr += 1;
                                if (*name_ctr == NULL) {
                                    if (head == NULL) {
                                        goto matched_it;
                                    } else {
                                        HASH_CLEAR(hh, head);
                                        FAIL(subtest, test_name, (int) head, "!=", 0);
                                    }
                                } else {
                                    HASH_FIND(hh, head, *name_ctr, u_strlen(*name_ctr) * sizeof(UChar), el);
                                }
                            } while (el != NULL);
                            HASH_CLEAR(hh, head);
                            FAIL(subtest, test_name, 1, "!=", 0);
                        }
                    }
                }

                FAIL(subtest, test_name, 1, "!=", 0);

                matched_it:
                /* clean up the observed loop elements */
                for (name_ctr = observed_items; *name_ctr != NULL; name_ctr += 1) {
                    free(*name_ctr);
                }
                free(observed_items);
            }
        }
    }

    /* Verify that all the expected loops were accounted for */
    TEST(loop_mask, 0, test_name, subtest++);

    /* clean up the returned loops before re-using the 'loops' variable */
    for (loop_ctr = loops; *loop_ctr != NULL; loop_ctr += 1) {
        cif_loop_free(*loop_ctr);
    }
    free(loops);

    /* Test with a valid pointer that is not a valid block handle */
    cif_block_free(block2);
    TEST(cif_get_block(cif, block1_code, &block2), CIF_OK, test_name, subtest++);
    TEST(cif_block_destroy(block2), CIF_OK, test_name, subtest++);
    TEST(cif_container_get_all_loops(block1, &loops), CIF_INVALID_HANDLE, test_name, subtest++);

    /* Final cleanup */
    cif_container_free(block1);
    DESTROY_CIF(test_name, cif);

    return 0;
}

