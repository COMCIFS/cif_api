/*
 * test_parse_simple_containers.c
 *
 * Tests parsing simple CIF 2.0 table data.
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
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     3
int main(void) {
    char test_name[80] = "test_parse_simple_containers";
    char local_file_name[] = "simple_containers.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_tp *cif = NULL;
    cif_block_tp **block_list;
    cif_block_tp **block_p;
    cif_block_tp *block = NULL;
    cif_frame_tp **frame_list;
    cif_frame_tp **frame_p;
    cif_frame_tp *frame = NULL;
    cif_value_tp *value = NULL;
    UChar *ustr;
    size_t count;

    U_STRING_DECL(code_block1,       "block1", 7);
    U_STRING_DECL(code_block2,       "block2", 7);
    U_STRING_DECL(code_block3,       "block3", 7);
    U_STRING_DECL(code_s1,           "s1", 3);
    U_STRING_DECL(code_s2,           "s2", 3);
    U_STRING_DECL(code_s3,           "s3", 3);
    U_STRING_DECL(name_location,     "_location", 10);
    U_STRING_DECL(value_block1_s1,   "block1/s1", 10);
    U_STRING_DECL(value_block1_s2,   "block1/s2", 10);
    U_STRING_DECL(value_block3_s1,   "block3/s1", 10);
    const UChar *value_block1;
    const UChar *value_block3;

    U_STRING_INIT(code_block1,       "block1", 7);
    U_STRING_INIT(code_block2,       "block2", 7);
    U_STRING_INIT(code_block3,       "block3", 7);
    U_STRING_INIT(code_s1,           "s1", 3);
    U_STRING_INIT(code_s2,           "s2", 3);
    U_STRING_INIT(code_s3,           "s3", 3);
    U_STRING_INIT(name_location,     "_location", 10);
    U_STRING_INIT(value_block1_s1,   "block1/s1", 10);
    U_STRING_INIT(value_block1_s2,   "block1/s2", 10);
    U_STRING_INIT(value_block3_s1,   "block3/s1", 10);
    value_block1 = code_block1;
    value_block3 = code_block3;

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
      /* check that there are exactly three blocks */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 4);
    TEST(block_list == NULL, 0, test_name, 5);
    for (count = 0, block_p = block_list; *block_p; count += 1, block_p += 1) {
        cif_container_free(*block_p);
    }
    TEST(count, 3, test_name, 6);
    free(block_list);

      /* check the block block1 */
    TEST(cif_get_block(cif, code_block1, &block), CIF_OK, test_name, 7);
    TEST(cif_container_get_value(block, name_location, &value), CIF_OK, test_name, 8);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 9);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 10);
    TEST(u_strcmp(ustr, value_block1), 0, test_name, 11);
    free(ustr);
    TEST(cif_block_get_all_frames(block, &frame_list), CIF_OK, test_name, 12);
    TEST(frame_list == NULL, 0, test_name, 13);
    for (count = 0, frame_p = frame_list; *frame_p; count += 1, frame_p += 1) {
        cif_container_free(*frame_p);
    }
    TEST(count, 2, test_name, 13);
    free(frame_list);
    TEST(cif_block_get_frame(block, code_s1, &frame), CIF_OK, test_name, 14);
    TEST(cif_container_get_value(frame, name_location, &value), CIF_OK, test_name, 15);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 16);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 17);
    TEST(u_strcmp(ustr, value_block1_s1), 0, test_name, 18);
    free(ustr);
    cif_frame_free(frame);
    TEST(cif_block_get_frame(block, code_s2, &frame), CIF_OK, test_name, 19);
    TEST(cif_container_get_value(frame, name_location, &value), CIF_OK, test_name, 20);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 21);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 22);
    TEST(u_strcmp(ustr, value_block1_s2), 0, test_name, 23);
    free(ustr);
    cif_frame_free(frame);
    cif_block_free(block);

      /* check the block block2 */
    TEST(cif_get_block(cif, code_block2, &block), CIF_OK, test_name, 24);
    TEST(cif_container_get_value(block, name_location, &value), CIF_NOSUCH_ITEM, test_name, 25);
    TEST(cif_block_get_all_frames(block, &frame_list), CIF_OK, test_name, 26);
    TEST(frame_list == NULL, 0, test_name, 27);
    TEST_NOT(*frame_list == NULL, 0, test_name, 28);
    free(frame_list);
    cif_block_free(block);

      /* check the block block3 */
    TEST(cif_get_block(cif, code_block3, &block), CIF_OK, test_name, 29);
    TEST(cif_container_get_value(block, name_location, &value), CIF_OK, test_name, 30);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 31);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 32);
    TEST(u_strcmp(ustr, value_block3), 0, test_name, 33);
    free(ustr);
    TEST(cif_block_get_all_frames(block, &frame_list), CIF_OK, test_name, 34);
    TEST(frame_list == NULL, 0, test_name, 35);
    for (count = 0, frame_p = frame_list; *frame_p; count += 1, frame_p += 1) {
        cif_container_free(*frame_p);
    }
    TEST(count, 2, test_name, 35);
    free(frame_list);
    TEST(cif_block_get_frame(block, code_s1, &frame), CIF_OK, test_name, 36);
    TEST(cif_container_get_value(frame, name_location, &value), CIF_OK, test_name, 37);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 38);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 39);
    TEST(u_strcmp(ustr, value_block3_s1), 0, test_name, 40);
    free(ustr);
    cif_frame_free(frame);
    TEST(cif_block_get_frame(block, code_s3, &frame), CIF_OK, test_name, 41);
    TEST(cif_container_get_value(frame, name_location, &value), CIF_NOSUCH_ITEM, test_name, 42);
    cif_frame_free(frame);
    cif_block_free(block);

    /* clean up */
    cif_value_free(value);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
