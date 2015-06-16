/*
 * test_write_11.c
 *
 * Tests writing simple data in CIF 1.1 format.
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

#include "assert_cifs.h"
#define TEXT_TEMPFILE 1
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     8
int main(void) {
    char test_name[80] = "test_write_11";
    FILE * cif_file;
    char c;
    cif_tp *cif = NULL;
    cif_tp *cif_readback = NULL;
    cif_block_tp *block = NULL;
    cif_value_tp *value = NULL;
    struct cif_write_opts_s *options;
    UChar value_sq_string[] = { 'S', 'a', 'y', ' ', '"', 'B', 'o', 'o', '"', 0 };
    UChar value_dq_string[] = { 'D', 'r', '.', ' ', 'O', '\'', 'M', 'a', 'l', 'l', 'e', 'y', 0 };
    UChar value_text_string[] = { 'D', 'e', 'l', 'i', 'm', 's', ' ', 'a', 'r', 'e', ':', 0x0a, '\'', ' ',
            'a', 'n', 'd', ' ', '"', 0 };
    UChar value_disallowed[] = { 'a', 'b', '\n', ';', 'c', 0 };
    char buffer[16];
    U_STRING_DECL(block_code,           "11_data", 12);
    U_STRING_DECL(name_unknown_value,   "_unknown_value", 15);
    U_STRING_DECL(name_na_value,        "_na_value", 10);
    U_STRING_DECL(name_sq_string,       "_sq_string", 11);
    U_STRING_DECL(name_dq_string,       "_dq_string", 11);
    U_STRING_DECL(name_text_string,     "_text_string", 13);
    U_STRING_DECL(name_numb_plain,      "_numb_plain", 12);
    U_STRING_DECL(name_numb_su,         "_numb_su", 9);

    U_STRING_INIT(block_code,           "simple_data", 12);
    U_STRING_INIT(name_unknown_value,   "_unknown_value", 15);
    U_STRING_INIT(name_na_value,        "_na_value", 10);
    U_STRING_INIT(name_sq_string,       "_sq_string", 11);
    U_STRING_INIT(name_dq_string,       "_dq_string", 11);
    U_STRING_INIT(name_text_string,     "_text_string", 13);
    U_STRING_INIT(name_numb_plain,      "_numb_plain", 12);
    U_STRING_INIT(name_numb_su,         "_numb_su", 9);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* Create the temp file */
    cif_file = tmpfile();
    /* cif_file = fopen("../write_simple.cif", "wb+"); */
    TEST(cif_file == NULL, 0, test_name, 1);

    /* Build the CIF data to test on */
    TEST(cif_create(&cif), CIF_OK, test_name, 2);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 3);

    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 4);
    TEST(cif_container_set_value(block, name_unknown_value, value), CIF_OK, test_name, 5);

    TEST(cif_value_init(value, CIF_NA_KIND), CIF_OK, test_name, 6);
    TEST(cif_container_set_value(block, name_na_value, value), CIF_OK, test_name, 7);

    TEST(cif_value_copy_char(value, value_sq_string), CIF_OK, test_name, 8);
    TEST(cif_container_set_value(block, name_sq_string, value), CIF_OK, test_name, 9);

    TEST(cif_value_copy_char(value, value_dq_string), CIF_OK, test_name, 10);
    TEST(cif_container_set_value(block, name_dq_string, value), CIF_OK, test_name, 11);

    TEST(cif_value_copy_char(value, value_text_string), CIF_OK, test_name, 12);
    TEST(cif_container_set_value(block, name_text_string, value), CIF_OK, test_name, 13);

    TEST(cif_value_init_numb(value, 17.125, 0, 4, 5), CIF_OK, test_name, 14);
    TEST(cif_container_set_value(block, name_numb_plain, value), CIF_OK, test_name, 15);

    TEST(cif_value_autoinit_numb(value, 43.53e06, 0.17e05, 19), CIF_OK, test_name, 16);
    TEST(cif_container_set_value(block, name_numb_su, value), CIF_OK, test_name, 17);

    TEST(cif_write_options_create(&options), CIF_OK, test_name, 18);
    options->cif_version = 1;

    /* write to the temp file */
    TEST(cif_write(cif_file, options, cif), CIF_OK, test_name, 19);
    TEST(fflush(cif_file), 0, test_name, 20);

    /* parse the file */
    rewind(cif_file);
    TEST(cif_parse(cif_file, NULL, &cif_readback), CIF_OK, test_name, 21);

    /* make sure everything matches */
    TEST_NOT(assert_cifs_equal(cif, cif_readback), 0, test_name, 22);

    /* check for a v1.1 signature (without BOM) */
    rewind(cif_file);
    TEST(fscanf(cif_file, "%15[^\n\r\t ]%c", buffer, &c), 2, test_name, 23);
    TEST(((c == '\r') && (c != '\n') && (c != ' ') && (c != '\t')), 0, test_name, 24);
    TEST(strcmp(buffer, "#\\#CIF_1.1"), 0, test_name, 25);

    /* test writing an unsupported value */
    rewind(cif_file);
    TEST(cif_value_copy_char(value, value_disallowed), CIF_OK, test_name, 26);
    TEST(cif_container_set_value(block, name_text_string, value), CIF_OK, test_name, 27);

    TEST(cif_write(cif_file, options, cif), CIF_DISALLOWED_VALUE, test_name, 28);

    /* clean up */
    cif_value_free(value);
    cif_container_free(block);

    TEST(cif_destroy(cif_readback), CIF_OK, test_name, 29);
    TEST(cif_destroy(cif), CIF_OK, test_name, 30);
    fclose(cif_file); /* ignore any error here */

    return 0;
}
