/*
 * test_parse_nested.c
 *
 * Tests parsing CIF 2.0 data containing nested save frames.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

#define BUFFER_SIZE 512
int main(void) {
    char test_name[80] = "test_parse_nested";
    char local_file_name[] = "nested.cif";
    struct cif_parse_opts_s *options;
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    cif_frame_t *frame = NULL;
    cif_frame_t *frame2 = NULL;
    cif_frame_t **frames = NULL;
    cif_frame_t **frame_p;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_value_t *value = NULL;
    UChar *ustr;
    U_STRING_DECL(nested_code,          "nested", 7);
    U_STRING_DECL(sibling_code,         "sibling", 8);
    U_STRING_DECL(name_nesting_level,   "_nesting_level", 15);
    UChar value_0[] = { '0', 0 };
    UChar value_1[] = { '1', 0 };
    UChar value_2[] = { '2', 0 };
    int count;

    U_STRING_INIT(nested_code,          "nested", 7);
    U_STRING_INIT(sibling_code,         "sibling", 8);
    U_STRING_INIT(name_nesting_level,   "_nesting_level", 15);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* prepare parse options */
    TEST(cif_parse_options_create(&options), CIF_OK, test_name, 1);
    options->max_frame_depth = -1;

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 2);
    strcat(file_name, local_file_name);
    fprintf(stderr, "test file is %s\n", file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 3);

    /* parse the file */
    TEST(cif_parse(cif_file, options, &cif), CIF_OK, test_name, 4);

    /* check the parse result */
      /* retrieve the expected data block */
    TEST(cif_get_block(cif, nested_code, &block), CIF_OK, test_name, 5);

      /* check the expected item */
    TEST(cif_container_get_value(block, name_nesting_level, &value), CIF_OK, test_name, 6);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 7);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 8);
    TEST(u_strcmp(ustr, value_0), 0, test_name, 9);
    free(ustr);

      /* check the save frame count */
    TEST(cif_container_get_all_frames(block, &frames), CIF_OK, test_name, 10);
    count = 0;
    for (frame_p = frames; *frame_p; frame_p += 1) {
        count += 1;
        cif_container_free(*frame_p);
    }
    free(frames);
    TEST(count, 1, test_name, 11);

      /* check the first-level save frame */
    TEST(cif_container_get_frame(block, nested_code, &frame), CIF_OK, test_name, 12);

      /* check the expected item */
    TEST(cif_container_get_value(frame, name_nesting_level, &value), CIF_OK, test_name, 13);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 14);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 15);
    TEST(u_strcmp(ustr, value_1), 0, test_name, 16);
    free(ustr);

      /* check the save frame count */
    TEST(cif_container_get_all_frames(frame, &frames), CIF_OK, test_name, 17);
    count = 0;
    for (frame_p = frames; *frame_p; frame_p += 1) {
        count += 1;
        cif_container_free(*frame_p);
    }
    free(frames);
    TEST(count, 2, test_name, 18);

      /* check the first second-level save frame */
    TEST(cif_container_get_frame(frame, nested_code, &frame2), CIF_OK, test_name, 19);

      /* check the expected item */
    TEST(cif_container_get_value(frame2, name_nesting_level, &value), CIF_OK, test_name, 20);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 21);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 22);
    TEST(u_strcmp(ustr, value_2), 0, test_name, 23);
    free(ustr);

      /* check the save frame count */
    TEST(cif_container_get_all_frames(frame2, &frames), CIF_OK, test_name, 24);
    count = 0;
    for (frame_p = frames; *frame_p; frame_p += 1) {
        count += 1;
        cif_container_free(*frame_p);
    }
    free(frames);
    TEST(count, 0, test_name, 25);
    cif_container_free(frame2);

      /* check the second second-level save frame */
    TEST(cif_container_get_frame(frame, sibling_code, &frame2), CIF_OK, test_name, 26);

      /* check the expected item */
    TEST(cif_container_get_value(frame2, name_nesting_level, &value), CIF_OK, test_name, 27);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 28);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 29);
    TEST(u_strcmp(ustr, value_2), 0, test_name, 30);
    free(ustr);

      /* check the save frame count */
    TEST(cif_container_get_all_frames(frame2, &frames), CIF_OK, test_name, 31);
    count = 0;
    for (frame_p = frames; *frame_p; frame_p += 1) {
        count += 1;
        cif_container_free(*frame_p);
    }
    free(frames);
    TEST(count, 0, test_name, 32);
    cif_container_free(frame2);

    /* clean up */
    cif_value_free(value);
    cif_frame_free(frame);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */
    free(options);

    return 0;
}
