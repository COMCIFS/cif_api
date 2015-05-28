/*
 * test_value_create.c
 *
 * Tests the CIF API's cif_value_create() function
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
    char test_name[80] = "test_value_create";
    cif_value_tp *value;
    UChar *text;
    size_t count;
    U_STRING_DECL(zero, "0", 2);

    U_STRING_INIT(zero, "0", 2);
    TESTHEADER(test_name);

    /* Test creating a value of kind CIF_UNK_KIND */ 
    value = NULL;
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 3);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 4);
    TEST((text != NULL), 0, test_name, 5);
    cif_value_free(value);

    /* Test creating a value of kind CIF_NA_KIND */ 
    value = NULL;
    TEST(cif_value_create(CIF_NA_KIND, &value), CIF_OK, test_name, 6);
    TEST((value == NULL), 0, test_name, 7);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 8);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 9);
    TEST((text != NULL), 0, test_name, 10);
    cif_value_free(value);

    /* Test creating a value of kind CIF_CHAR_KIND */ 
    value = NULL;
    TEST(cif_value_create(CIF_CHAR_KIND, &value), CIF_OK, test_name, 11);
    TEST((value == NULL), 0, test_name, 12);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 13);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 14);
    TEST((text == NULL), 0, test_name, 15);
    TEST((*text != 0), 0, test_name, 16);
    cif_value_free(value);
    free(text);

    /* Test creating a value of kind CIF_NUMB_KIND */ 
    value = NULL;
    TEST(cif_value_create(CIF_NUMB_KIND, &value), CIF_OK, test_name, 17);
    TEST((value == NULL), 0, test_name, 18);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 19);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 20);
    TEST((text == NULL), 0, test_name, 21);
    TEST(u_strcmp(zero, text), 0, test_name, 22);
    cif_value_free(value);
    free(text);

    /* Test creating a value of kind CIF_LIST_KIND */ 
    value = NULL;
    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 23);
    TEST((value == NULL), 0, test_name, 24);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 25);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 26);
    TEST((text != NULL), 0, test_name, 27);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 28);
    TEST((count != 0), 0, test_name, 29);
    cif_value_free(value);

    /* Test creating a value of kind CIF_TABLE_KIND */ 
    value = NULL;
    TEST(cif_value_create(CIF_TABLE_KIND, &value), CIF_OK, test_name, 29);
    TEST((value == NULL), 0, test_name, 30);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 31);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 32);
    TEST((text != NULL), 0, test_name, 33);
    TEST(cif_value_get_element_count(value, &count), CIF_OK, test_name, 34);
    TEST((count != 0), 0, test_name, 35);
    cif_value_free(value);

    /* Test creating a value of invalid kind */ 
    value = NULL;
    TEST(cif_value_create((cif_kind_tp) 42, &value), CIF_ARGUMENT_ERROR, test_name, 36);
    TEST((value != NULL), 0, test_name, 37);

    return 0;
}

