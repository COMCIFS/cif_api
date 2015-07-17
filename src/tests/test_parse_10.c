/*
 * test_parse_10.c
 *
 * Tests parsing simple CIF 2.0 looped data.
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
    char test_name[80] = "test_parse_10";
    char local_file_name[] = "10.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    struct cif_parse_opts_s *options;
    cif_tp *cif = NULL;
    cif_block_tp **block_list = NULL;
    cif_block_tp *block = NULL;
    cif_value_tp *value = NULL;
    UChar *ustr;

    double d;

    U_STRING_DECL(code_10,           "10", 3);
    U_STRING_DECL(name_sq_string,    "_sq_string", 11);
    U_STRING_DECL(name_dq_string,    "_dq_string", 11);
    U_STRING_DECL(name_text_string,  "_text_string", 13);
    U_STRING_DECL(name_xlat_eol,     "_translated_eol", 16);
    U_STRING_DECL(name_numb_plain,   "_numb_plain", 12);
    U_STRING_DECL(value_sq,          "sq", 3);
    U_STRING_DECL(value_dq,          "dq", 3);
    U_STRING_DECL(value_text,        "text", 5);
    UChar value_xlat_eol[] = { 'l', 'i', 'n', 'e', '1', ' ', ';', '\n', 'l', 'i', 'n', 'e', '2', 0 };
    const char extra_ws_chars[] = "\v\n";
    const char extra_eol_chars[] = "\f ";

    U_STRING_INIT(code_10,           "10", 3);
    U_STRING_INIT(name_sq_string,    "_sq_string", 11);
    U_STRING_INIT(name_dq_string,    "_dq_string", 11);
    U_STRING_INIT(name_text_string,  "_text_string", 13);
    U_STRING_INIT(name_xlat_eol,     "_translated_eol", 16);
    U_STRING_INIT(name_numb_plain,   "_numb_plain", 12);
    U_STRING_INIT(value_sq,          "sq", 3);
    U_STRING_INIT(value_dq,          "dq", 3);
    U_STRING_INIT(value_text,        "text", 5);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse_options_create(&options), CIF_OK, test_name, 3);
    options->extra_ws_chars = extra_ws_chars;
    options->extra_eol_chars = extra_eol_chars;

    TEST(cif_parse(cif_file, options, &cif), CIF_OK, test_name, 4);

    /* check the parse result */
      /* check that there is exactly one block, and that it has the expected code */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 5);
    TEST((*block_list == NULL), 0, test_name, 6);
    TEST_NOT((*(block_list + 1) == NULL), 0, test_name, 7);
    block = *block_list;
    free(block_list);
    TEST(cif_container_get_code(block, &ustr), CIF_OK, test_name, 8);
    TEST(u_strcmp(code_10, ustr), 0, test_name, 8);
    free(ustr);

      /* check block contents */
    TEST(cif_container_get_value(block, name_sq_string, &value), CIF_OK, test_name, 9);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 10);
    TEST(u_strcmp(ustr, value_sq), 0, test_name, 11);
    free(ustr);

    TEST(cif_container_get_value(block, name_dq_string, &value), CIF_OK, test_name, 12);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 13);
    TEST(u_strcmp(ustr, value_dq), 0, test_name, 14);
    free(ustr);

    TEST(cif_container_get_value(block, name_text_string, &value), CIF_OK, test_name, 15);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 16);
    TEST(u_strcmp(ustr, value_text), 0, test_name, 17);
    free(ustr);

    TEST(cif_container_get_value(block, name_xlat_eol, &value), CIF_OK, test_name, 18);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 19);
    TEST(u_strcmp(ustr, value_xlat_eol), 0, test_name, 20);
    free(ustr);

    TEST(cif_container_get_value(block, name_numb_plain, &value), CIF_OK, test_name, 21);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 22);
    TEST_NOT(d == 1250.0, 0, test_name, 23);

    /* clean up */
    cif_block_free(block);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
