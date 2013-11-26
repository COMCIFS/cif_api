/*
 * test_value_init_numb.c
 *
 * Tests the CIF API's cif_value_init_numb() function
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_value_init_numb";
    cif_value_t *value;
    UChar *text;
    double d;
    U_STRING_DECL(v0, "0", 2);
    U_STRING_DECL(v450_s1, "450.0", 6);
    U_STRING_DECL(v450_s0, "450", 4);
    U_STRING_DECL(v450_sm1, "4.5e+02", 8);
    U_STRING_DECL(v450_sm2, "4e+02", 6);
    U_STRING_DECL(v992_sm1, "9.9e+02", 8);
    U_STRING_DECL(v992_sm2, "1.0e+03", 8);
    U_STRING_DECL(vm12_345s_017_s3, "-12.345(17)", 12);
    U_STRING_DECL(vm12_345s_017_sm1, "-1e+01(0)", 10);
    U_STRING_DECL(vm0_5s_10_s0, "0(1)", 5);
    U_STRING_DECL(vm0_6s_10_s0, "1(1)", 5);
    U_STRING_DECL(vm0_00000042s_00000017_s8, "4.2e-07(17)", 12);

    U_STRING_INIT(v0, "0", 2);
    U_STRING_INIT(v450_s1, "450.0", 6);
    U_STRING_INIT(v450_s0, "450", 4);
    U_STRING_INIT(v450_sm1, "4.5e+02", 8);

    /* Note: ties round to even in the IEEE default rounding mode */
    U_STRING_INIT(v450_sm2, "4e+02", 6);

    U_STRING_INIT(v992_sm1, "9.9e+02", 8);
    U_STRING_INIT(v992_sm2, "1.0e+03", 8);
    U_STRING_INIT(vm12_345s_017_s3, "-12.345(17)", 12);
    U_STRING_INIT(vm12_345s_017_sm1, "-1e+01(0)", 10);
    U_STRING_INIT(vm0_5s_10_s0, "0(1)", 5);
    U_STRING_INIT(vm0_6s_10_s0, "1(1)", 5);
    U_STRING_INIT(vm0_00000042s_00000017_s8, "4.2e-07(17)", 12);

    TESTHEADER(test_name);

    /* Start with a value of kind CIF_UNK_KIND */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);

    /* reinitialize the value as kind NUMB, scale 1, exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, 1, 6), CIF_OK, test_name, 3);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 4);
    TEST((cif_value_as_double(value) != 450.0), 0, test_name, 5);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 6);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 7);
    TEST(u_strcmp(v450_s1, text), 0, test_name, 8);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, 0, 6), CIF_OK, test_name, 9);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 10);
    TEST((cif_value_as_double(value) != 450.0), 0, test_name, 11);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 12);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 13);
    TEST(u_strcmp(v450_s0, text), 0, test_name, 14);
    free(text);

    /* reinitialize the value as kind NUMB, scale -1, exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, -1, 6), CIF_OK, test_name, 15);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 16);
    d = cif_value_as_double(value);
    TEST((cif_value_as_double(value) != 450.0), 0, test_name, 17);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 18);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 19);
    TEST(u_strcmp(v450_sm1, text), 0, test_name, 20);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded-exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, -2, 6), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 22);
    TEST((cif_value_as_double(value) != 400.0), 0, test_name, 23);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 24);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 25);
    TEST(u_strcmp(v450_sm2, text), 0, test_name, 26);
    free(text);

    /* reinitialize the value as kind NUMB, scale -1, rounded-exact */
    TEST(cif_value_init_numb(value, 992.0, 0.0, -1, 6), CIF_OK, test_name, 27);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 28);
    TEST((cif_value_as_double(value) != 990.0), 0, test_name, 29);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 30);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 31);
    TEST(u_strcmp(v992_sm1, text), 0, test_name, 32);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded-exact, with rounding up */
    TEST(cif_value_init_numb(value, 992.0, 0.0, -2, 6), CIF_OK, test_name, 33);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 34);
    TEST((cif_value_as_double(value) != 1000.0), 0, test_name, 35);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 36);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 37);
    TEST(u_strcmp(v992_sm2, text), 0, test_name, 38);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded-exact, with rounding up */
    TEST(cif_value_init_numb(value, 992.0, 0.0, -2, 6), CIF_OK, test_name, 39);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 40);
    TEST((cif_value_as_double(value) != 1000.0), 0, test_name, 41);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 42);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 43);
    TEST(u_strcmp(v992_sm2, text), 0, test_name, 44);
    free(text);

    /* reinitialize the value as kind NUMB, scale 3, measured, negative */
    TEST(cif_value_init_numb(value, -12.345, 0.017, 3, 6), CIF_OK, test_name, 45);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 46);
    TEST((cif_value_as_double(value) != -12.345), 0, test_name, 47);
    TEST((cif_value_su_as_double(value) != 0.017), 0, test_name, 48);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 49);
    TEST(u_strcmp(vm12_345s_017_s3, text), 0, test_name, 50);
    free(text);

    /* reinitialize the value as kind NUMB, scale -1, measured, negative */
    TEST(cif_value_init_numb(value, -12.345, 0.017, -1, 6), CIF_OK, test_name, 51);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 52);
    TEST((cif_value_as_double(value) != -10), 0, test_name, 53);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 54);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 55);
    TEST(u_strcmp(vm12_345s_017_sm1, text), 0, test_name, 56);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, measured, non-zero sig-figs from rounding */
    TEST(cif_value_init_numb(value, 0.5, 1.0, 0, 6), CIF_OK, test_name, 57);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 58);
    TEST((cif_value_as_double(value) != 0.0), 0, test_name, 59);
    TEST((cif_value_su_as_double(value) != 1.0), 0, test_name, 60);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 61);
    TEST(u_strcmp(vm0_5s_10_s0, text), 0, test_name, 62);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, measured, non-zero sig-figs from rounding */
    TEST(cif_value_init_numb(value, 0.6, 1.0, 0, 6), CIF_OK, test_name, 63);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 64);
    TEST((cif_value_as_double(value) != 1.0), 0, test_name, 65);
    TEST((cif_value_su_as_double(value) != 1.0), 0, test_name, 66);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 67);
    TEST(u_strcmp(vm0_6s_10_s0, text), 0, test_name, 68);
    free(text);

    /* reinitialize the value as kind NUMB, scale 8, measured, excessive leading zeroes */
    TEST(cif_value_init_numb(value, 0.00000042, 0.00000017, 8, 5), CIF_OK, test_name, 69);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 70);
    TEST((cif_value_as_double(value) != 0.00000042), 0, test_name, 71);
    TEST((cif_value_su_as_double(value) != 0.00000017), 0, test_name, 72);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 73);
    TEST(u_strcmp(vm0_00000042s_00000017_s8, text), 0, test_name, 74);
    free(text);

    cif_value_free(value);

    return 0;
}

