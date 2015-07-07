/*
 * test_value_get_number.c
 *
 * Tests error behavior of the CIF API's cif_value_get_number() and
 * cif_value_get_su() functions.  Ordinary behavior is tested elsewhere.
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
#include "assert_value.h"

UChar *u_strdup(UChar *src) {
    UChar *dest = malloc((u_strlen(src) + 1) * sizeof(UChar));

    if (dest) {
        u_strcpy(dest, src);
    }

    return dest;
}

int main(void) {
    U_STRING_DECL(val_str1, "-10.250(125)", 13);
    U_STRING_DECL(val_str2, "1742E+02", 9);
    U_STRING_DECL(val_str3, "1 ", 4);
    char test_name[80] = "test_value_get_number";
    cif_value_tp *value;
    cif_value_tp *value2;
    UChar *tmp;
    double d;

    U_STRING_INIT(val_str1, "-10.250(125)", 13);
    U_STRING_INIT(val_str2, "1742E+02", 9);
    U_STRING_INIT(val_str3, "1 ", 4);

    TESTHEADER(test_name);

    /* Test with values of various wrong kinds */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 2);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 3);
    cif_value_free(value);

    TEST(cif_value_create(CIF_NA_KIND, &value), CIF_OK, test_name, 4);
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 5);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 6);
    cif_value_free(value);

    TEST(cif_value_create(CIF_CHAR_KIND, &value), CIF_OK, test_name, 7);
    TEST(cif_value_get_number(value, &d), CIF_INVALID_NUMBER, test_name, 8);
    TEST(cif_value_get_su(value, &d), CIF_INVALID_NUMBER, test_name, 9);
    cif_value_free(value);

    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 10);
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 11);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 12);
    cif_value_free(value);

    TEST(cif_value_create(CIF_TABLE_KIND, &value), CIF_OK, test_name, 13);
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 14);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 15);
    cif_value_free(value);

    /* Test char-to-numb coercion */
    tmp = u_strdup(val_str1);
    TEST(tmp == NULL, 0, test_name, 16);
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 17);
    TEST(cif_value_copy_char(value, val_str1), CIF_OK, test_name, 18);
    TEST(cif_value_create(CIF_UNK_KIND, &value2), CIF_OK, test_name, 19);
    TEST(cif_value_parse_numb(value2, tmp), CIF_OK, test_name, 20); /* responsibility for tmp passes to value2*/
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 21);
    TEST(assert_values_equal(value, value2), 0, test_name, 22);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 23);
    TEST(!assert_values_equal(value, value2), 0, test_name, 24);
    cif_value_free(value);
    cif_value_free(value2);

    tmp = u_strdup(val_str2);
    TEST(tmp == NULL, 0, test_name, 25);
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 26);
    TEST(cif_value_copy_char(value, val_str2), CIF_OK, test_name, 27);
    TEST(cif_value_create(CIF_UNK_KIND, &value2), CIF_OK, test_name, 28);
    TEST(cif_value_parse_numb(value2, tmp), CIF_OK, test_name, 29); /* responsibility for tmp passes to value2*/
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 30);
    TEST(assert_values_equal(value, value2), 0, test_name, 31);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 32);
    TEST(!assert_values_equal(value, value2), 0, test_name, 33);
    cif_value_free(value);
    cif_value_free(value2);

    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 34);
    TEST(cif_value_copy_char(value, val_str3), CIF_OK, test_name, 35);
    value2 = NULL;
    TEST(cif_value_clone(value, &value2), CIF_OK, test_name, 36);
    TEST(cif_value_get_number(value, &d), CIF_INVALID_NUMBER, test_name, 37);
    TEST(!assert_values_equal(value, value2), 0, test_name, 38);
    cif_value_free(value);
    cif_value_free(value2);

    return 0;
}

