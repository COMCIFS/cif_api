/*
 * test_value_get_number.c
 *
 * Tests error behavior of the CIF API's cif_value_get_number() and
 * cif_value_get_su() functions.  Ordinary behavior is tested elsewhere.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_value_get_number";
    cif_value_t *value;
    double d;

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
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 8);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 9);
    cif_value_free(value);

    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 10);
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 11);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 12);
    cif_value_free(value);

    TEST(cif_value_create(CIF_TABLE_KIND, &value), CIF_OK, test_name, 13);
    TEST(cif_value_get_number(value, &d), CIF_ARGUMENT_ERROR, test_name, 14);
    TEST(cif_value_get_su(value, &d), CIF_ARGUMENT_ERROR, test_name, 15);
    cif_value_free(value);

    return 0;
}

