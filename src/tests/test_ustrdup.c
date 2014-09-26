/*
 * test_ustrdup.c
 *
 * Tests the CIF API's cif_u_strdup() function
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_ustrdup";
    UChar src0[1]  = { 0 };
    UChar ascii[7] = { 0x40, 0x41, 0x20, 0x72, 0x08, 0x20, 0 };
    UChar bmp[7]   = { 0x40, 0x1234, 0x20, 0x8531, 0xf00d, 0x1000, 0 };
    UChar upper[7] = { 0xdb0d, 0xdead, 0x20, 0xd800, 0xdfff, 0x1000, 0 };
    UChar bad[7]   = { 0xdb0d, 0x20, 0xdead, 0xdfff, 0xd9d9, 0xdada, 0 };
    UChar *dup;

    TESTHEADER(test_name);

    /* a NULL argument should be duplicated as NULL */
    TEST(cif_u_strdup(NULL) != NULL, 0, test_name, 1);

    dup = cif_u_strdup(src0);
    TEST(dup == NULL, 0, test_name, 2);
    TEST(dup == src0, 0, test_name, 3);
    TEST(u_strcmp(src0, dup), 0, test_name, 4);
    free(dup);

    dup = cif_u_strdup(ascii);
    TEST(dup == NULL, 0, test_name, 5);
    TEST(dup == ascii, 0, test_name, 6);
    TEST(u_strcmp(ascii, dup), 0, test_name, 7);
    free(dup);

    dup = cif_u_strdup(bmp);
    TEST(dup == NULL, 0, test_name, 8);
    TEST(dup == bmp, 0, test_name, 9);
    TEST(u_strcmp(bmp, dup), 0, test_name, 10);
    free(dup);

    dup = cif_u_strdup(upper);
    TEST(dup == NULL, 0, test_name, 11);
    TEST(dup == upper, 0, test_name, 12);
    TEST(u_strcmp(upper, dup), 0, test_name, 13);
    free(dup);

    dup = cif_u_strdup(bad);
    TEST(dup == NULL, 0, test_name, 14);
    TEST(dup == bad, 0, test_name, 15);
    TEST(u_strcmp(bad, dup), 0, test_name, 16);
    free(dup);

    return 0;
}

