/*
 * test_parse_unicode.c
 *
 * Tests parsing CIF 2.0 data containing characters outside the CIF 1.0 repertoire
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

#define BUFFER_SIZE 512
int main(void) {
    char test_name[80] = "test_parse_unicode";
    char local_file_name[] = "unicode.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_tp *cif = NULL;
    cif_block_tp **block_list = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp **frame_list = NULL;
    cif_frame_tp *frame = NULL;
    cif_loop_tp **all_loops = NULL;
    cif_loop_tp **loop_p = NULL;
    cif_loop_tp *loop = NULL;
    cif_loop_tp *scalar_loop = NULL;
    cif_value_tp *value = NULL;
    UChar *ustr;
    size_t count;
    UChar code_unicode[] = { 0x16c, 'n', 'i', 'c', 0xf6, 'd', 'e', 0x2192, 0 };
    UChar code_s1[] =      { 0xa7, '1', 0 };
    UChar name_deltaHf[] = { '_', 0x394, 'H', 'f', 0 };
    UChar value_deltaHf[] = { 0x2212, '3', '9', '3', '.', '5', '0', '9' , 0 };
    UChar value_uvalue[] =  { 0xd801, 0xde3e, 0x16a0, 0x2820, 0 };
    U_STRING_DECL(name_formula,      "_formula", 9);
    U_STRING_DECL(name_uvalue,       "_uvalue", 8);
    U_STRING_DECL(value_formula,     "C O2", 5);

    U_STRING_INIT(name_formula,      "_formula", 9);
    U_STRING_INIT(name_uvalue,       "_uvalue", 8);
    U_STRING_INIT(value_formula,     "C O2", 5);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, 3);

    /* check the parse result */
      /* check that there is exactly one block, and that it has the expected code */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 4);
    TEST((*block_list == NULL), 0, test_name, 5);
    TEST_NOT((*(block_list + 1) == NULL), 0, test_name, 6);
    block = *block_list;
    TEST(cif_container_get_code(block, &ustr), CIF_OK, test_name, 7);
    TEST(u_strcmp(code_unicode, ustr), 0, test_name, 8);
    free(ustr);

      /* count the loops and check their categories */
    TEST(cif_container_get_all_loops(block, &all_loops), CIF_OK, test_name, 9);
    TEST_NOT(*all_loops == NULL, 0, test_name, 10);  /* no loops directly in this block */

      /* now check the frames */
    TEST(cif_block_get_all_frames(block, &frame_list), CIF_OK, test_name, 11);
    TEST(frame_list[0] == NULL, 0, test_name, 12);
    TEST_NOT(frame_list[1] == NULL, 0, test_name, 13);
    frame = frame_list[0];
    free(frame_list);
      /* there is exactly one save frame; check its contents */
    TEST(cif_container_get_code(frame, &ustr), CIF_OK, test_name, 14);
    TEST(u_strcmp(ustr, code_s1), 0, test_name, 15);
    free(ustr);
      /* count the loops and check their categories */
    TEST(cif_container_get_all_loops(frame, &all_loops), CIF_OK, test_name, 16);
    count = 0;
    for (loop_p = all_loops; *loop_p; loop_p += 1) {
        TEST(cif_loop_get_category(*loop_p, &ustr), CIF_OK, test_name, 2 * count + 17);
        if (ustr) {
            /* must be the scalar loop */
            TEST_NOT(*ustr == 0, 0, test_name, 2 * count + 18);
            scalar_loop = *loop_p;
            free(ustr);
        }
        cif_loop_free(*loop_p);
        count += 1;
    }

    TEST_NOT(count == 2, 0, test_name, 2 * count + 19);  /* test number 23 if it passes */
    free(all_loops);

      /* check frame contents: item _formula */
    TEST(cif_container_get_item_loop(frame, name_formula, &loop), CIF_OK, test_name, 24);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 25);
    TEST_NOT(ustr == NULL, 0, test_name, 26);  /* no category */
    cif_loop_free(loop);
    TEST(cif_container_get_value(frame, name_formula, &value), CIF_OK, test_name, 27);  /* only one loop packet */
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 28);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 29);
    TEST(u_strcmp(ustr, value_formula), 0, test_name, 30);

      /* check frame contents: item _\u0394Hf */
    TEST(cif_container_get_item_loop(frame, name_deltaHf, &loop), CIF_OK, test_name, 31);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 32);
    TEST_NOT(ustr == NULL, 0, test_name, 33);  /* no category */
    cif_loop_free(loop);
    TEST(cif_container_get_value(frame, name_deltaHf, &value), CIF_OK, test_name, 34);  /* only one loop packet */
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 35);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 36);
    TEST(u_strcmp(ustr, value_deltaHf), 0, test_name, 37);

      /* check frame contents: item _uvalue */
    TEST(cif_container_get_item_loop(frame, name_uvalue, &loop), CIF_OK, test_name, 40);
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 41);
    TEST(ustr == NULL, 0, test_name, 42);
    TEST_NOT(*ustr == 0, 0, test_name, 43);
    cif_loop_free(loop);
    TEST(cif_container_get_value(frame, name_uvalue, &value), CIF_OK, test_name, 44);  /* a scalar */
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 45);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 46);
    TEST(u_strcmp(ustr, value_uvalue), 0, test_name, 47);

    /* clean up */
    cif_value_free(value);
    cif_block_free(block);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
