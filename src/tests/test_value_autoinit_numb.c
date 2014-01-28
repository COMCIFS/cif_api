/*
 * test_value_autoinit_numb.c
 *
 * Tests the CIF API's cif_value_autoinit_numb() function
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
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
    double d1;
    U_STRING_DECL(v0, "0", 2);
    U_STRING_DECL(v1, "1", 2);
    U_STRING_DECL(vm17_5, "-17.5", 6);
    U_STRING_DECL(v1_234e10, "12340000000", 12);
    U_STRING_DECL(v081x, "8.11181962490081787109375e-07", 30);
    U_STRING_DECL(v1s1_9, "1(1)", 5);
    U_STRING_DECL(v0s2_9, "0(2)", 5);
    U_STRING_DECL(v12_346s3, "12.346(3)", 10);
    U_STRING_DECL(vm34_57s26, "-34.57(26)", 11);
    U_STRING_DECL(vm34_6s3, "-34.6(3)", 9);
    U_STRING_DECL(vm34_6s15, "-34.6(15)", 10);
    U_STRING_DECL(v1722s24, "1722(24)", 9);
    U_STRING_DECL(v1_72e3_s2, "1.72e+03(2)", 12);
    U_STRING_DECL(v0_00000120s10, "0.00000120(10)", 15);
    U_STRING_DECL(v1_2em7s10, "1.2e-07(10)", 12);

    U_STRING_INIT(v0, "0", 2);
    U_STRING_INIT(v1, "1", 2);
    U_STRING_INIT(vm17_5, "-17.5", 6);
    U_STRING_INIT(v1_234e10, "12340000000", 12);
    U_STRING_INIT(v081x, "8.11181962490081787109375e-07", 30);
    U_STRING_INIT(v1s1_9, "1(1)", 5);
    U_STRING_INIT(v0s2_9, "0(2)", 5);
    U_STRING_INIT(v12_346s3, "12.346(3)", 10);
    U_STRING_INIT(vm34_57s26, "-34.57(26)", 11);
    U_STRING_INIT(vm34_6s3, "-34.6(3)", 9);
    U_STRING_INIT(vm34_6s15, "-34.6(15)", 10);
    U_STRING_INIT(v1722s24, "1722(24)", 9);
    U_STRING_INIT(v1_72e3_s2, "1.72e+03(2)", 12);
    U_STRING_INIT(v0_00000120s10, "0.00000120(10)", 15);
    U_STRING_INIT(v1_2em7s10, "1.2e-07(10)", 12);

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
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 11);
    TEST((d1 != 1.0), 0, test_name, 12);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 13);
    TEST((d1 != 0.0), 0, test_name, 14);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 15);
    TEST(u_strcmp(v1, text), 0, test_name, 16);
    free(text);

    TEST(cif_value_autoinit_numb(value, -17.5, 0.0, 9), CIF_OK, test_name, 17);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 18);
    TEST((d1 != -17.5), 0, test_name, 19);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 20);
    TEST((d1 != 0.0), 0, test_name, 21);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 22);
    TEST(u_strcmp(vm17_5, text), 0, test_name, 23);
    free(text);

    TEST(cif_value_autoinit_numb(value, 1.234e10, 0.0, 9), CIF_OK, test_name, 24);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 25);
    TEST((d1 != 1.234e10), 0, test_name, 26);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 27);
    TEST((d1 != 0.0), 0, test_name, 28);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 29);
    TEST(u_strcmp(v1_234e10, text), 0, test_name, 30);
    free(text);

    d = ldexp(1742.0, -31);
    TEST(cif_value_autoinit_numb(value, d, 0.0, 9), CIF_OK, test_name, 31);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 32);
    TEST((d1 != d), 0, test_name, 33);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 34);
    TEST((d1 != 0.0), 0, test_name, 35);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 36);
    TEST(u_strcmp(v081x, text), 0, test_name, 37);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.0, 0.0, 9), CIF_OK, test_name, 38);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 39);
    TEST((d1 != 0.0), 0, test_name, 40);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 41);
    TEST((d1 != 0.0), 0, test_name, 42);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 43);
    TEST(u_strcmp(v0, text), 0, test_name, 44);
    free(text);

    /* test measured numbers */

    TEST(cif_value_autoinit_numb(value, 1.0, 1.0, 9), CIF_OK, test_name, 45);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 46);
    TEST((d1 != 1.0), 0, test_name, 47);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 48);
    TEST((d1 != 1.0), 0, test_name, 49);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 50);
    TEST(u_strcmp(v1s1_9, text), 0, test_name, 51);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.4, 2.0, 9), CIF_OK, test_name, 52);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 53);
    TEST((d1 != 0.0), 0, test_name, 54);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 55);
    TEST((d1 != 2.0), 0, test_name, 56);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 57);
    TEST(u_strcmp(v0s2_9, text), 0, test_name, 58);
    free(text);

    TEST(cif_value_autoinit_numb(value, 12.3456, 0.003, 9), CIF_OK, test_name, 59);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 60);
    TEST((fabs(d1 - 12.346) > 0.0001), 0, test_name, 61);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 62);
    TEST((d1 != 0.003), 0, test_name, 63);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 64);
    TEST(u_strcmp(v12_346s3, text), 0, test_name, 65);
    free(text);

    TEST(cif_value_autoinit_numb(value, -34.567, 0.26, 27), CIF_OK, test_name, 66);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 67);
    TEST((fabs(d1 + 34.57) > 0.0001), 0, test_name, 68);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 69);
    TEST((d1 != 0.26), 0, test_name, 70);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 71);
    TEST(u_strcmp(vm34_57s26, text), 0, test_name, 72);
    free(text);

    TEST(cif_value_autoinit_numb(value, -34.567, 0.29, 27), CIF_OK, test_name, 73);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 74);
    TEST((fabs(d1 + 34.6) > 0.0001), 0, test_name, 75);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 76);
    TEST((fabs(d1 - 0.3) > 0.001), 0, test_name, 77);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 78);
    TEST(u_strcmp(vm34_6s3, text), 0, test_name, 79);
    free(text);

    TEST(cif_value_autoinit_numb(value, -34.567, 1.5, 27), CIF_OK, test_name, 80);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 81);
    TEST((fabs(d1 + 34.6) > 0.0001), 0, test_name, 82);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 83);
    TEST((d1 != 1.5), 0, test_name, 84);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 85);
    TEST(u_strcmp(vm34_6s15, text), 0, test_name, 86);
    free(text);

    TEST(cif_value_autoinit_numb(value, 1721.51, 24, 27), CIF_OK, test_name, 87);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 88);
    TEST((d1 != 1722.0), 0, test_name, 89);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 90);
    TEST((d1 != 24.0), 0, test_name, 91);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 92);
    TEST(u_strcmp(v1722s24, text), 0, test_name, 93);
    free(text);

    TEST(cif_value_autoinit_numb(value, 1721.51, 24, 19), CIF_OK, test_name, 94);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 95);
    TEST((d1 != 1720.0), 0, test_name, 96);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 97);
    TEST((d1 != 20.0), 0, test_name, 98);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 99);
    TEST(u_strcmp(v1_72e3_s2, text), 0, test_name, 100);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.0000012, 0.0000001, 19), CIF_OK, test_name, 101);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 102);
    TEST((fabs(d1 - 0.0000012) > 0.000000001), 0, test_name, 103);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 104);
    TEST((d1 != 0.0000001), 0, test_name, 105);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 106);
    TEST(u_strcmp(v0_00000120s10, text), 0, test_name, 107);
    free(text);

    TEST(cif_value_autoinit_numb(value, 0.00000012, 0.0000001, 19), CIF_OK, test_name, 108);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 109);
    TEST((fabs(d1 - 0.00000012) > 0.0000000001), 0, test_name, 110);
    TEST(cif_value_get_su(value, &d1), CIF_OK, test_name, 111);
    TEST((d1 != 0.0000001), 0, test_name, 112);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 113);
    TEST(u_strcmp(v1_2em7s10, text), 0, test_name, 114);
    free(text);

    cif_value_free(value);

    return 0;
}

