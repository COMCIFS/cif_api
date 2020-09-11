/*
 * test_parse_cif11_unquoted.c
 *
 * Tests parsing unquoted CIF 1.1 data that must be quoted in CIF 2
 *
 * Copyright 2020 John C. Bollinger
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
#define NUM_ITEMS     5
int main(void) {
    char test_name[80] = "test_parse_cif11_unquoted";
    char local_file_name[] = "cif11_unquoted.cif";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_block_tp **block_list;
    cif_loop_tp **loop_list;
    cif_loop_tp *loop;
    cif_value_tp *value = NULL;
    UChar **name_list;
    UChar **name_p;
    UChar *ustr;

    U_STRING_DECL(block_code,        "cif11_unquoted", 15);
    U_STRING_DECL(name_bracket_mid,  "_bracket_mid", 13);
    U_STRING_DECL(name_bracket_end,  "_bracket_end", 13);
    U_STRING_DECL(name_brace_begin,  "_brace_begin", 13);
    U_STRING_DECL(name_brace_mid,    "_brace_mid", 11);
    U_STRING_DECL(name_brace_end,    "_brace_end", 11);

    U_STRING_DECL(value_bracket_mid, "Fc^*^=kFc[1+0.001xFc^2^\\l^3^/sin(2\\q)]^-1/4^", 45);
    U_STRING_DECL(value_bracket_end, "a[42]", 6);
    U_STRING_DECL(value_brace_begin, "{foo}bar", 9);
    U_STRING_DECL(value_brace_mid,   "bar{foo}bar", 12);
    U_STRING_DECL(value_brace_end,   "bar{foo}", 9);

    U_STRING_INIT(block_code,        "cif11_unquoted", 15);
    U_STRING_INIT(name_bracket_mid,  "_bracket_mid", 13);
    U_STRING_INIT(name_bracket_end,  "_bracket_end", 13);
    U_STRING_INIT(name_brace_begin,  "_brace_begin", 13);
    U_STRING_INIT(name_brace_mid,    "_brace_mid", 11);
    U_STRING_INIT(name_brace_end,    "_brace_end", 11);

    U_STRING_INIT(value_bracket_mid, "Fc^*^=kFc[1+0.001xFc^2^\\l^3^/sin(2\\q)]^-1/4^", 45);
    U_STRING_INIT(value_bracket_end, "a[42]", 6);
    U_STRING_INIT(value_brace_begin, "{foo}bar", 9);
    U_STRING_INIT(value_brace_mid,   "bar{foo}bar", 12);
    U_STRING_INIT(value_brace_end,   "bar{foo}", 9);

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
    TEST(u_strcmp(block_code, ustr), 0, test_name, 8);
    free(ustr);

      /* check that there is exactly one loop in the block, and that it is the scalar loop */
    TEST(cif_container_get_all_loops(block, &loop_list), CIF_OK, test_name, 9);
    TEST((*loop_list == NULL), 0, test_name, 10);
    TEST_NOT((*(loop_list + 1) == NULL), 0, test_name, 11);
    loop = *loop_list;
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 12);
    TEST(ustr == NULL, 0, test_name, 13);
    TEST(*ustr, 0, test_name, 14);
    free(ustr);

      /* check the number of data names in the loop */
    TEST(cif_loop_get_names(loop, &name_list), CIF_OK, test_name, 15);
    for (name_p = name_list; *name_p; name_p += 1) {
        /* We're responsible for freeing these anyway; might as well do it here and now */
        free(*name_p);
    }
    TEST((name_p - name_list), NUM_ITEMS, test_name, 16);
    free(name_list);

      /* check each expected item */
    TEST(cif_container_get_value(block, name_bracket_mid, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 18);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 19);
    TEST(u_strcmp(value_bracket_mid, ustr), 0, test_name, 20);
    free(ustr);

    TEST(cif_container_get_value(block, name_bracket_end, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 22);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 23);
    TEST(u_strcmp(value_bracket_end, ustr), 0, test_name, 24);
    free(ustr);

    TEST(cif_container_get_value(block, name_brace_begin, &value), CIF_OK, test_name, 25);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 26);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 27);
    TEST(u_strcmp(value_brace_begin, ustr), 0, test_name, 28);
    free(ustr);

    TEST(cif_container_get_value(block, name_brace_mid, &value), CIF_OK, test_name, 29);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 30);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 31);
    TEST(u_strcmp(value_brace_mid, ustr), 0, test_name, 32);
    free(ustr);

    TEST(cif_container_get_value(block, name_brace_end, &value), CIF_OK, test_name, 33);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 34);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 35);
    TEST(u_strcmp(value_brace_end, ustr), 0, test_name, 36);
    free(ustr);

    /* clean up */
    cif_value_free(value);
    cif_loop_free(loop);
    free(loop_list);
    cif_block_free(block);
    free(block_list);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
