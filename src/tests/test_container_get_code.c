/*
 * test_container_get_code.c
 *
 * Tests the cif_container_get_code() function under a variety of circumstances.
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
#include "../cif.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     3
int main(void) {
    char test_name[80] = "test_container_get_code";
    char local_file_name[] = "simple_containers.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_tp *cif = NULL;
    cif_block_tp **block_list;
    cif_block_tp **block_p;
    cif_block_tp *block = NULL;
    cif_block_tp *block2 = NULL;
    cif_frame_tp **frame_list;
    cif_frame_tp **frame_p;
    cif_frame_tp *frame = NULL;
    cif_frame_tp *frame2 = NULL;
    UChar *ustr = NULL;
    size_t count;

    U_STRING_DECL(code_block1,       "block1", 7);
    U_STRING_DECL(code_block2,       "block2", 7);
    U_STRING_DECL(code_block3,       "block3", 7);
    U_STRING_DECL(code_s1,           "s1", 3);
    U_STRING_DECL(code_s2,           "s2", 3);

    U_STRING_INIT(code_block1,       "block1", 7);
    U_STRING_INIT(code_block2,       "block2", 7);
    U_STRING_INIT(code_block3,       "block3", 7);
    U_STRING_INIT(code_s1,           "s1", 3);
    U_STRING_INIT(code_s2,           "s2", 3);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /*
     * Test first against programmatically-created containers
     */
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, code_block1, block);
    CREATE_BLOCK(test_name, cif, code_block2, block2);
    CREATE_FRAME(test_name, block, code_s1, frame);
    CREATE_FRAME(test_name, block, code_s2, frame2);

    TEST(cif_container_get_code(block, &ustr), CIF_OK, test_name, 1);
    TEST(u_strcmp(ustr, code_block1), 0, test_name, 2);
    free(ustr);
    ustr = NULL;
    TEST(cif_container_get_code(block2, &ustr), CIF_OK, test_name, 3);
    TEST(u_strcmp(ustr, code_block2), 0, test_name, 4);
    free(ustr);
    ustr = NULL;
    TEST(cif_container_get_code(frame, &ustr), CIF_OK, test_name, 5);
    TEST(u_strcmp(ustr, code_s1), 0, test_name, 6);
    free(ustr);
    ustr = NULL;
    TEST(cif_container_get_code(frame2, &ustr), CIF_OK, test_name, 7);
    TEST(u_strcmp(ustr, code_s2), 0, test_name, 8);
    free(ustr);
    ustr = NULL;
    cif_container_free(frame2);
    cif_container_free(frame);
    cif_container_free(block2);
    cif_container_free(block);
    frame2 = NULL;
    frame = NULL;
    block2 = NULL;
    block = NULL;

    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 9);
    count = 9;
    for (block_p = block_list; *block_p; block_p += 1) {
        int is1;
        int is2;

        TEST(cif_container_get_code(*block_p, &ustr), CIF_OK, test_name, ++count);
        is1 = u_strcmp(ustr, code_block1);
        is2 = (!is1 && u_strcmp(ustr, code_block2));
        TEST_NOT((is1 || is2), 0, test_name, ++count);
        free(ustr);
        ustr = NULL;
        if (is1) {
            block = *block_p;
        } else {
            cif_block_free(*block_p);
        }
    }
    free(block_list);
    /* the last test was number 13 */

    TEST((block == NULL), 0, test_name, 14);
    TEST(cif_container_get_all_frames(block, &frame_list), CIF_OK, test_name, 15);
    count = 15;
    for (frame_p = frame_list; *frame_p; frame_p += 1) {
        int is1;
        int is2;

        TEST(cif_container_get_code(*frame_p, &ustr), CIF_OK, test_name, ++count);
        is1 = u_strcmp(ustr, code_s1);
        is2 = (!is1 && u_strcmp(ustr, code_s2));
        TEST_NOT((is1 || is2), 0, test_name, ++count);
        free(ustr);
        ustr = NULL;
        cif_frame_free(*frame_p);
    }
    free(frame_list);
    cif_block_free(block);
    block = NULL;
    /* the last test was number 19 */
    cif_destroy(cif);
    cif = NULL;

    /*
     * Test second against containers parsed from a file
     */

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 20);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 21);

    /* parse the file */
    TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, 22);

    /* check the parse result */

    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 23);
    count = 23;
    for (block_p = block_list; *block_p; block_p += 1) {
        int is1;
        int is2;
        int is3;

        TEST(cif_container_get_code(*block_p, &ustr), CIF_OK, test_name, ++count);
        is1 = u_strcmp(ustr, code_block1);
        is2 = (!is1 && u_strcmp(ustr, code_block2));
        is3 = (!is1 && !is2 && u_strcmp(ustr, code_block3));
        TEST_NOT((is1 || is2 || is3), 0, test_name, ++count);
        free(ustr);
        ustr = NULL;
        if (is1) {
            block = *block_p;
        } else {
            cif_block_free(*block_p);
        }
    }
    free(block_list);
    /* the last test was number 29 */

    TEST((block == NULL), 0, test_name, 30);
    TEST(cif_container_get_all_frames(block, &frame_list), CIF_OK, test_name, 31);
    count = 31;
    for (frame_p = frame_list; *frame_p; frame_p += 1) {
        int is1;
        int is2;

        TEST(cif_container_get_code(*frame_p, &ustr), CIF_OK, test_name, ++count);
        is1 = u_strcmp(ustr, code_s1);
        is2 = (!is1 && u_strcmp(ustr, code_s2));
        TEST_NOT((is1 || is2), 0, test_name, ++count);
        free(ustr);
        ustr = NULL;
        cif_frame_free(*frame_p);
    }
    free(frame_list);
    cif_block_free(block);
    block = NULL;

    /* clean up */

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
