/*
 * test_container_create_loop1.c
 *
 * Tests general function of the CIF API's cif_block_create_frame() function.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_container_create_loop1";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_block_t *block2 = NULL;
    cif_frame_t *frame = NULL;
    cif_frame_t *frame2 = NULL;
    cif_loop_t *loop = NULL;
#define NUM_NAMES 8
#define BUFLEN 64
    const char name_patterns[NUM_NAMES][BUFLEN] = {
        "_item",
        "_category\\x2eitem",
        "__",
        "_\\x23_not_\\u2028a_\\u2029comment",
        "_\\x22not_a_string\\x22",
        "_\\x5bnot\\x2ca\\x2clist\\x5d",
        "_\\x7b\\x27not\\x27\\x3aa_table\\x7d",
        "_\\xeft\\xe9\\xa0\\u039c\\ud800\\udfba"
    };
    UChar *names[NUM_NAMES + 1];
    int counter;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block2_code, "block2", 7);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(frame2_code, "frame2", 7);
    U_STRING_DECL(category, "category", 9);
    U_STRING_DECL(category2, "category2", 10);
    U_STRING_DECL(category3, "", 1);
    U_STRING_DECL(category4, " ", 2);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);
    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block2_code, "block2", 7);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(frame2_code, "frame2", 7);
    U_STRING_INIT(category, "category", 9);
    U_STRING_INIT(category2, "category2", 10);
    U_STRING_INIT(category3, "", 1);
    U_STRING_INIT(category4, " ", 2);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block2_code, block2);
    CREATE_FRAME(test_name, block, frame_code, frame);
    CREATE_FRAME(test_name, block, frame2_code, frame2);

    for (counter = 0; counter < NUM_NAMES; counter++) {
        names[counter] = (UChar *) malloc(BUFLEN * sizeof(UChar));
        u_unescape(name_patterns[counter], names[counter], BUFLEN - 1);
        *(names[counter] + (BUFLEN - 1)) = 0;
    }
    names[counter] = NULL;

    INIT_USTDERR;

    /* Verify that none of the blocks or frames under test already (think they) have the target loops */

#define CHECK_CATEGORY_ABSENT(container, category, subtest) do { \
        cif_container_t *_cont = (container); \
        UChar *cat = (category); \
        int _st = (subtest); \
        if (cif_container_get_category_loop(_cont, cat, NULL) != CIF_NOSUCH_LOOP) { \
            u_fprintf(ustderr, "error: category %S reported already present\n", cat); \
            return _st; \
        } \
    } while (0)
#define CHECK_ITEM_ABSENT(container, item, subtest) do { \
        cif_container_t *_cont = (container); \
        UChar *itm = (item); \
        int _st = (subtest); \
        if (cif_container_get_item_loop(_cont, itm, NULL) != CIF_NOSUCH_ITEM) { \
            u_fprintf(ustderr, "error: item %S reported already present\n", itm); \
            return _st; \
        } \
    } while (0)
#define TEST_LOOP_ABSENT(container, subtest) do { \
        cif_container_t *cont = (container); \
        int st = (subtest); \
        CHECK_CATEGORY_ABSENT(cont, category,  st); \
        CHECK_CATEGORY_ABSENT(cont, category2, st); \
        CHECK_CATEGORY_ABSENT(cont, category3, st); \
        CHECK_CATEGORY_ABSENT(cont, category4, st); \
        for (counter = 0; counter < NUM_NAMES; counter++) { \
            CHECK_ITEM_ABSENT(cont, names[counter], st); \
        } \
    } while (0)
#define TEST_LOOP_PRESENT(container, category, names, subtest) do { \
        cif_container_t *cont = (container); \
        UChar *cat = (category); \
        UChar **name_strings = (names); \
        int st = (subtest); \
        UChar **n; \
        for (n = name_strings; *n != NULL; n += 1) { \
            if (cif_container_get_item_loop(cont, *n, NULL) != CIF_OK) { \
                u_fprintf(ustderr, "error: item %S expected to be present\n", *n); \
                return st; \
            } \
        } \
        if ((cat != NULL) && (cif_container_get_category_loop(cont, cat, NULL) != CIF_OK)){ \
            u_fprintf(ustderr, "error: category %S expected to be present\n", cat); \
            return st; \
        } \
    } while (0)

    TEST_LOOP_ABSENT(block, 1);
    TEST_LOOP_ABSENT(block2, 2);
    TEST_LOOP_ABSENT(frame, 3);
    TEST_LOOP_ABSENT(frame2, 4);

    /* Test creating a loop in a block */
    TEST(cif_container_create_loop(block, category, names, &loop), CIF_OK, test_name, 5);
    TEST_NOT((loop != NULL), 0, test_name, 6);
    cif_loop_free(loop);
    TEST_LOOP_PRESENT(block, category, names, 7);
    TEST_LOOP_ABSENT(block2, 8);
    TEST_LOOP_ABSENT(frame, 9);
    TEST_LOOP_ABSENT(frame2, 10);

    /* test creating the same loop in a frame inside the first block */
    loop = NULL;
    TEST(cif_container_create_loop(frame, category, names, &loop), CIF_OK, test_name, 11);
    TEST_NOT((loop != NULL), 0, test_name, 12);
    cif_loop_free(loop);
    TEST_LOOP_PRESENT(block, category, names, 13);
    TEST_LOOP_PRESENT(frame, category, names, 14);
    TEST_LOOP_ABSENT(block2, 15);
    TEST_LOOP_ABSENT(frame2, 16);

    /* test multiple loops with the same category */
    free(names[3]);
    names[3] = NULL;
    TEST(cif_container_create_loop(frame2, category2, names, NULL), CIF_OK, test_name, 17);
    TEST(cif_container_create_loop(frame2, category2, names + 4, NULL), CIF_OK, test_name, 16);

    /* test various categories */
    free(names[1]);
    free(names[5]);
    names[1] = NULL;
    names[5] = NULL;
    TEST(cif_container_create_loop(block2, category, names, NULL), CIF_OK, test_name, 18);
    TEST(cif_container_create_loop(block2, category2, names + 2, NULL), CIF_OK, test_name, 19);
    TEST(cif_container_create_loop(block2, category3, names + 4, NULL), CIF_OK, test_name, 20);
    TEST(cif_container_create_loop(block2, category4, names + 6, NULL), CIF_OK, test_name, 21);
    TEST_LOOP_PRESENT(block2, category, names, 22);
    TEST_LOOP_PRESENT(block2, category2, names + 2, 23);
    TEST_LOOP_PRESENT(block2, category3, names + 4, 24);
    TEST_LOOP_PRESENT(block2, category4, names + 6, 25);

    /* clean up */
    for (counter = 0; counter <= NUM_NAMES; counter++) {
        free(names[counter]);
    }
    DESTROY_FRAME(test_name, frame2);
    DESTROY_FRAME(test_name, frame);
    DESTROY_BLOCK(test_name, block2);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

