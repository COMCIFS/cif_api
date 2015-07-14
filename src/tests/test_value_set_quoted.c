/*
 * test_value_set_quoted.c
 *
 * Tests the behavior of the cif_value_is_quoted() and cif_value_set_quoted() functions.
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

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_value_set_quoted";
    cif_value_tp *value = NULL;
    U_STRING_DECL(value_text, "value text", 11);
    U_STRING_DECL(numb_text, "1.234(5)", 9);
    U_STRING_DECL(query_text, "?", 2);
    U_STRING_DECL(dot_text, ".", 2);
    UChar *text;
    double d1, d2, su1, su2;

    U_STRING_INIT(value_text, "value text", 11);
    U_STRING_INIT(numb_text, "1.234(5)", 9);
    U_STRING_INIT(query_text, "?", 2);
    U_STRING_INIT(dot_text, ".", 2);

    TESTHEADER(test_name);

    /* Test values of kind 'unk' */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 3);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 4);
    TEST(cif_value_set_quoted(value, CIF_QUOTED), CIF_OK, test_name, 5);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 6);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 7);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 8);
    TEST(u_strcmp(text, query_text), 0, test_name, 9);
    free(text);
    TEST(cif_value_set_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 10);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 11);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 12);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 13);
    TEST(text != NULL, 0, test_name, 14);
    cif_value_free(value);

    /* Test values of kind 'na' */
    TEST(cif_value_create(CIF_NA_KIND, &value), CIF_OK, test_name, 15);
    TEST((value == NULL), 0, test_name, 16);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 17);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 18);
    TEST(cif_value_set_quoted(value, CIF_QUOTED), CIF_OK, test_name, 19);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 20);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 21);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 22);
    TEST(u_strcmp(text, dot_text), 0, test_name, 23);
    free(text);
    TEST(cif_value_set_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 24);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 25);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 26);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 27);
    TEST(text != NULL, 0, test_name, 28);
    cif_value_free(value);

    /* Test values of kind 'list' */
    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 29);
    TEST((value == NULL), 0, test_name, 30);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 31);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 32);
    TEST(cif_value_set_quoted(value, CIF_QUOTED), CIF_ARGUMENT_ERROR, test_name, 33);
    cif_value_free(value);

    /* Test values of kind 'table' */
    TEST(cif_value_create(CIF_TABLE_KIND, &value), CIF_OK, test_name, 34);
    TEST((value == NULL), 0, test_name, 35);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 36);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 37);
    TEST(cif_value_set_quoted(value, CIF_QUOTED), CIF_ARGUMENT_ERROR, test_name, 38);
    cif_value_free(value);

    /* Test values of kind 'char' */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 39);
    TEST(cif_value_copy_char(value, value_text), CIF_OK, test_name, 40);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 41);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 42);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 43);
    TEST(u_strcmp(text, value_text), 0, test_name, 44);
    free(text);
    TEST(cif_value_set_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 45);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 46);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 47);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 48);
    TEST(u_strcmp(text, value_text), 0, test_name, 49);
    free(text);
    cif_value_free(value);

    /* Test values of kind 'numb' */
    text = (UChar *) malloc(16 * sizeof(*text));
    TEST(text == NULL, 0, test_name, 50);
    *text = 0;
    u_strcpy(text, numb_text);
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 51);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 52);
    text = NULL;
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 53);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 54);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 55);
    TEST(cif_value_get_su(value, &su1), CIF_OK, test_name, 56);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 57);
    TEST(u_strcmp(text, numb_text), 0, test_name, 58);
    free(text);
    TEST(cif_value_set_quoted(value, CIF_QUOTED), CIF_OK, test_name, 59);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 60);
    TEST(cif_value_get_number(value, &d2), CIF_OK, test_name, 61);
    TEST(cif_value_get_su(value, &su2), CIF_OK, test_name, 62);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 63);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 64);
    TEST(u_strcmp(text, numb_text), 0, test_name, 65);
    free(text);
    TEST(d1 != d2, 0, test_name, 66);
    TEST(su1 != su2, 0, test_name, 67);
    cif_value_free(value);

    return 0;
}

