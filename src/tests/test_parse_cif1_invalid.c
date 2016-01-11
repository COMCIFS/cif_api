/*
 * test_parse_cif1_invalid.c
 *
 * Tests parsing simple CIF 2.0 data.
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

int main(void) {
    char test_name[80] = "test_parse_cif1_invalid";
    char local_file_name[] = "cif1_invalid.cif";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_value_tp *value = NULL;
    char file_name[BUFFER_SIZE];
    FILE *cif_file;
    struct cif_parse_opts_s *options;
    UChar *ustr = NULL;

    U_STRING_DECL(block_code, "d", 2);
    U_STRING_DECL(name,    "_name", 6);
    const UChar name_value[] = { 0x5b, 0x27, 0x6b, 0x27, 0x5d, 0 };

    U_STRING_INIT(block_code, "d", 2);
    U_STRING_INIT(name,    "_name", 6);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    TEST(cif_parse_options_create(&options), CIF_OK, test_name, 3);
    options->error_callback = cif_parse_error_ignore;

    /* parse the file */
    TEST(cif_parse(cif_file, options, &cif), CIF_OK, test_name, 4);

    /* validate the parsed content */

    TEST(cif_get_block(cif, block_code, &block), CIF_OK, test_name, 5);
    TEST(cif_container_get_value(block, name, &value), CIF_OK, test_name, 6);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 7);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 8);
    TEST(u_strcmp(ustr, name_value), 0, test_name, 9);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 10);

    /* clean up, ignoring any failures */

    free(ustr);
    cif_value_free(value);
    cif_container_free(block);
    cif_destroy(cif);
    fclose(cif_file);

    return 0;
}
