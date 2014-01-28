/*
 * test_value_create.c
 *
 * Tests the CIF API's cif_value_create() function
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_value_create";
    cif_value_t *value;
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
    TEST(cif_value_create(42, &value), CIF_ARGUMENT_ERROR, test_name, 36);
    TEST((value != NULL), 0, test_name, 37);

    return 0;
}

