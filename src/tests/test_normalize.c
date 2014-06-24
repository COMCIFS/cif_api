/*
 * test_normalize.c
 *
 * Tests the CIF API's cif_normalize() function
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

#define NCASES 7
int main(int argc, char *argv[]) {
    char test_name[80] = "test_normalize";
    UChar input[NCASES][16]    = {
        { 0 }, /* empty string */

        /* ASCII */
        { 0x63, 0x69, 0x66, 0x20, 0x66, 0x6f, 0x72, 0x65, 0x76, 0x65, 0x72, 0x21, 0 },
        { 0x57, 0x68, 0x6f, 0x20, 0x6e, 0x65, 0x65, 0x64, 0x73, 0x20, 0x58, 0x4d, 0x4c, 0x3f, 0 },

        /* BMP +- normalization, w/ pre-composed characters */
        { 0x0174, 0x0151, 0x014c, 0x0166, 0x0051, 0x0300, 0x0323, 0x212b, 0x03d4, 0 },
        { 0x0300, 0x0301, 0x330, 0x0327, 0x004e, 0x0303, 0x0069, 0x0302, 0x00dc, 0x0315, 0x030c, 0 },
        { 0x0020, 0x1ea5, 0x0328, 0x1ec4, 0x0330, 0 },

        /* upper +- normalization */
        { 0xd81b, 0xdf15, 0xd81b, 0xdf51, 0xd81b, 0xdf5a, 0xd81b, 0xdf1d, 0xd801, 0xdc00, 0xd801, 0xdc1d, 0 }
    };
    UChar expected[NCASES][16] = {
        { 0 }, /* empty string */

        /* ASCII */
        { 0x63, 0x69, 0x66, 0x20, 0x66, 0x6f, 0x72, 0x65, 0x76, 0x65, 0x72, 0x21, 0 },
        { 0x77, 0x68, 0x6f, 0x20, 0x6e, 0x65, 0x65, 0x64, 0x73, 0x20, 0x78, 0x6d, 0x6c, 0x3f, 0 },

        /* BMP +- normalization, w/ pre-composed characters */
        { 0x0175, 0x0151, 0x014d, 0x0167, 0x0071, 0x0323, 0x0300, 0x00e5, 0x03d4, 0 },
        { 0x0327, 0x0330, 0x0300, 0x0301, 0x00f1, 0x00ee, 0x01da, 0x0315, 0 },
        { 0x0020, 0x0105, 0x0302, 0x0301, 0x1e1b, 0x302, 0x0303, 0 },  /* longer than the original */

        /* upper +- normalization */
        { 0xd81b, 0xdf15, 0xd81b, 0xdf51, 0xd81b, 0xdf5a, 0xd81b, 0xdf1d, 0xd801, 0xdc28, 0xd801, 0xdc45, 0 }
    };
    UChar expected_2_6[16] = { 0x77, 0x68, 0x6f, 0x20, 0x6e, 0x65, 0 };
    UChar expected_5_4[16] = { 0x0020, 0x0105, 0x0302, 0x0301, 0x1ec5, 0 };
    UChar *result;
    int i;

    TESTHEADER(test_name);

    /* srclen == 0 should always yield an empty string */
    TEST(cif_normalize(input[0], 0, &result), CIF_OK, test_name, 1);
    TEST(result == NULL, 0, test_name, 4);
    TEST(*result != 0, 0, test_name, 5);
    free(result);

    /* test srclen < actual length; only the first srclen characters should be considered */
    TEST(cif_normalize(input[2], 6, &result), CIF_OK, test_name, 6);
    TEST(result == NULL, 0, test_name, 7);
    TEST(u_strcmp(result, expected_2_6), 0, test_name, 8);
    free(result);
    TEST(cif_normalize(input[5], 4, &result), CIF_OK, test_name, 9);
    TEST(result == NULL, 0, test_name, 10);
    TEST(u_strcmp(result, expected_5_4), 0, test_name, 11);
    free(result);

    #define START 12
    #define NTESTS 10
    for (i = 0; i < NCASES; i += 1) {
        /* Check that normalization produces the expected result */
        TEST(cif_normalize(input[i], -1, NULL), CIF_OK, test_name, START + NTESTS * i);  /* NULL result */
        TEST(cif_normalize(input[i], -1, &result), CIF_OK, test_name, START + NTESTS * i + 1);
        TEST(result == input[i], 0, test_name, START + NTESTS * i + 2);
        TEST(u_strcmp(result, expected[i]), 0, test_name, START + NTESTS * i + 3);
        free(result);
        /* check with length limit equal to the actual length */
        TEST(cif_normalize(input[i], u_strlen(input[i]), &result), CIF_OK, test_name, START + NTESTS * i + 4);
        TEST(result == input[i], 0, test_name, START + NTESTS * i + 5);
        TEST(u_strcmp(result, expected[i]), 0, test_name, START + NTESTS * i + 6);
        free(result);
        /* Check that re-normalization does not produce a different result */
        TEST(cif_normalize(expected[i], -1, &result), CIF_OK, test_name, START + NTESTS * i + 7);
        TEST(result == expected[i], 0, test_name, START + NTESTS * i + 8);
        TEST(u_strcmp(result, expected[i]), 0, test_name, START + NTESTS * i + 9);
        free(result);
    } /* next is START + NTESTS * NCASES */

    return 0;
}

