/*
 * test_value_autoinit_numb.c
 *
 * Tests the CIF API's cif_value_autoinit_numb() function
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <math.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_value_autoinit_numb";
    cif_value_t *value;
    UChar *text;
    double d;
    U_STRING_DECL(v0, "0", 1);
    U_STRING_DECL(v1, "1", 1);
    U_STRING_DECL(vm17_5, "-17.5", 5);
    U_STRING_DECL(v1_234e10, "12340000000", 11);
    U_STRING_DECL(v081x, "8.11181962490081787109375e-07", 29);
    U_STRING_DECL(v1s1_9, "1(1)", 4);
    U_STRING_DECL(v0s2_9, "0(2)", 4);
    U_STRING_DECL(v12_346s3, "12.346(3)", 9);
    U_STRING_DECL(vm34_57s26, "-34.57(26)", 10);
    U_STRING_DECL(vm34_6s3, "-34.6(3)", 8);
    U_STRING_DECL(vm34_6s15, "-34.6(15)", 9);
    U_STRING_DECL(v1722s24, "1722(24)", 8);
    U_STRING_DECL(v1_72e3_s2, "1.72e+03(2)", 11);
    U_STRING_DECL(v0_00000120s10, "0.00000120(10)", 14);
    U_STRING_DECL(v1_2em7s10, "1.2e-07(10)", 11);

    U_STRING_INIT(v0, "0", 1);
    U_STRING_INIT(v1, "1", 1);
    U_STRING_INIT(vm17_5, "-17.5", 5);
    U_STRING_INIT(v1_234e10, "12340000000", 11);
    U_STRING_INIT(v081x, "8.11181962490081787109375e-07", 29);
    U_STRING_INIT(v1s1_9, "1(1)", 4);
    U_STRING_INIT(v0s2_9, "0(2)", 4);
    U_STRING_INIT(v12_346s3, "12.346(3)", 9);
    U_STRING_INIT(vm34_57s26, "-34.57(26)", 10);
    U_STRING_INIT(vm34_6s3, "-34.6(3)", 8);
    U_STRING_INIT(vm34_6s15, "-34.6(15)", 9);
    U_STRING_INIT(v1722s24, "1722(24)", 8);
    U_STRING_INIT(v1_72e3_s2, "1.72e+03(2)", 11);
    U_STRING_INIT(v0_00000120s10, "0.00000120(10)", 14);
    U_STRING_INIT(v1_2em7s10, "1.2e-07(10)", 11);

    INIT_USTDERR;

    TESTHEADER(test_name);

    /* Start with a value of kind CIF_UNK_KIND */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 3);

    /* Test negative su */
    TEST(cif_value_autoinit_numb(value, 1.0, -1.0, 9), CIF_ARGUMENT_ERROR, test_name, 4);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 5);

    /* Test bad su_rule */
    TEST(cif_value_autoinit_numb(value, 1.0, 0.5, 1), CIF_ARGUMENT_ERROR, test_name, 6);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 7);
    TEST(cif_value_autoinit_numb(value, 1.0, 0.5, 0), CIF_ARGUMENT_ERROR, test_name, 8);

    /* Test exact numbers */
    TEST(cif_value_autoinit_numb(value, 1.0, 0.0, 9), CIF_OK, test_name, 10);
    TEST((cif_value_as_double(value) != 1.0), 0, test_name, 11);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 12);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 13);
    TEST(u_strcmp(v1, text), 0, test_name, 14);
    free(text);

    TEST(cif_value_autoinit_numb(value, -17.5, 0.0, 9), CIF_OK, test_name, 15);
    TEST((cif_value_as_double(value) != -17.5), 0, test_name, 16);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 17);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 18);
    TEST(u_strcmp(vm17_5, text), 0, test_name, 19);
    free(text);

    TEST(cif_value_autoinit_numb(value, 1.234e10, 0.0, 9), CIF_OK, test_name, 20);
    TEST((cif_value_as_double(value) != 1.234e10), 0, test_name, 21);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 22);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 23);
    TEST(u_strcmp(v1_234e10, text), 0, test_name, 24);
    free(text);

    d = ldexp(1742.0, -31);
    TEST(cif_value_autoinit_numb(value, d, 0.0, 9), CIF_OK, test_name, 25);
    TEST((cif_value_as_double(value) != d), 0, test_name, 26);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 27);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 28);
    TEST(u_strcmp(v081x, text), 0, test_name, 29);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.0, 0.0, 9), CIF_OK, test_name, 30);
    TEST((cif_value_as_double(value) != 0.0), 0, test_name, 31);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 32);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 33);
    TEST(u_strcmp(v0, text), 0, test_name, 34);
    free(text);

    /* test measured numbers */

    TEST(cif_value_autoinit_numb(value, 1.0, 1.0, 9), CIF_OK, test_name, 35);
    TEST((cif_value_as_double(value) != 1.0), 0, test_name, 36);
    TEST((cif_value_su_as_double(value) != 1.0), 0, test_name, 37);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 38);
    TEST(u_strcmp(v1s1_9, text), 0, test_name, 39);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.4, 2.0, 9), CIF_OK, test_name, 40);
    TEST((cif_value_as_double(value) != 0.0), 0, test_name, 41);
    TEST((cif_value_su_as_double(value) != 2.0), 0, test_name, 42);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 43);
    TEST(u_strcmp(v0s2_9, text), 0, test_name, 44);
    free(text);

    TEST(cif_value_autoinit_numb(value, 12.3456, 0.003, 9), CIF_OK, test_name, 45);
    TEST((fabs(cif_value_as_double(value) - 12.346) > 0.0001), 0, test_name, 46);
    TEST((cif_value_su_as_double(value) != 0.003), 0, test_name, 47);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 48);
    TEST(u_strcmp(v12_346s3, text), 0, test_name, 49);
    free(text);

    TEST(cif_value_autoinit_numb(value, -34.567, 0.26, 27), CIF_OK, test_name, 50);
    TEST((fabs(cif_value_as_double(value) + 34.57) > 0.0001), 0, test_name, 51);
    TEST((cif_value_su_as_double(value) != 0.26), 0, test_name, 52);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 53);
    TEST(u_strcmp(vm34_57s26, text), 0, test_name, 54);
    free(text);

    TEST(cif_value_autoinit_numb(value, -34.567, 0.29, 27), CIF_OK, test_name, 55);
    TEST((fabs(cif_value_as_double(value) + 34.6) > 0.0001), 0, test_name, 56);
    TEST((fabs(cif_value_su_as_double(value) - 0.3) > 0.001), 0, test_name, 57);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 58);
    TEST(u_strcmp(vm34_6s3, text), 0, test_name, 59);
    free(text);

    TEST(cif_value_autoinit_numb(value, -34.567, 1.5, 27), CIF_OK, test_name, 60);
    TEST((fabs(cif_value_as_double(value) + 34.6) > 0.0001), 0, test_name, 61);
    TEST((cif_value_su_as_double(value) != 1.5), 0, test_name, 62);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 63);
    TEST(u_strcmp(vm34_6s15, text), 0, test_name, 64);
    free(text);

    TEST(cif_value_autoinit_numb(value, 1721.51, 24, 27), CIF_OK, test_name, 65);
    TEST((cif_value_as_double(value) != 1722.0), 0, test_name, 66);
    TEST((cif_value_su_as_double(value) != 24.0), 0, test_name, 67);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 68);
    TEST(u_strcmp(v1722s24, text), 0, test_name, 69);
    free(text);

    TEST(cif_value_autoinit_numb(value, 1721.51, 24, 19), CIF_OK, test_name, 70);
    TEST((cif_value_as_double(value) != 1720.0), 0, test_name, 71);
    TEST((cif_value_su_as_double(value) != 20.0), 0, test_name, 72);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 73);
    TEST(u_strcmp(v1_72e3_s2, text), 0, test_name, 74);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.0000012, 0.0000001, 19), CIF_OK, test_name, 75);
    TEST((fabs(cif_value_as_double(value) - 0.0000012) > 0.000000001), 0, test_name, 76);
    TEST((cif_value_su_as_double(value) != 0.0000001), 0, test_name, 77);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 78);
    TEST(u_strcmp(v0_00000120s10, text), 0, test_name, 79);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.00000012, 0.0000001, 19), CIF_OK, test_name, 80);
    TEST((fabs(cif_value_as_double(value) - 0.00000012) > 0.0000000001), 0, test_name, 81);
    TEST((cif_value_su_as_double(value) != 0.0000001), 0, test_name, 82);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 83);
    TEST(u_strcmp(v1_2em7s10, text), 0, test_name, 84);
    free(text);

    cif_value_free(value);

    return 0;
}

