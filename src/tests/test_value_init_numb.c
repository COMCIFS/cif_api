/*
 * test_value_init_numb.c
 *
 * Tests the CIF API's cif_value_init_numb() function
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
#include "assert_doubles.h"
#include "test.h"

#ifndef DBL_TEST_ULPS
#define DBL_TEST_ULPS DEFAULT_ULPS
#endif

int main(void) {
    char test_name[80] = "test_value_init_numb";
    cif_value_tp *value;
    UChar *text;
    double d;
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
    U_STRING_DECL(v1_23e4, "1.23e+04", 9);
    U_STRING_DECL(v0s1, "0(1)", 5);
    U_STRING_DECL(v0e2s1, "0e+02(1)", 9);

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
    U_STRING_INIT(v1_23e4, "1.23e+04", 9);
    U_STRING_INIT(v0s1, "0(1)", 5);
    U_STRING_INIT(v0e2s1, "0e+02(1)", 9);

    TESTHEADER(test_name);

    /* Start with a value of kind CIF_UNK_KIND */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);

    /* reinitialize the value as kind NUMB, scale 1, exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, 1, 6), CIF_OK, test_name, 3);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 4);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 5);
    TEST((d != 450.0), 0, test_name, 6);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 7);
    TEST((d != 0.0), 0, test_name, 8);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 9);
    TEST(u_strcmp(v450_s1, text), 0, test_name, 10);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, 0, 6), CIF_OK, test_name, 11);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 12);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 13);
    TEST((d != 450.0), 0, test_name, 14);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 15);
    TEST((d != 0.0), 0, test_name, 16);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 17);
    TEST(u_strcmp(v450_s0, text), 0, test_name, 18);
    free(text);

    /* reinitialize the value as kind NUMB, scale -1, exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, -1, 6), CIF_OK, test_name, 19);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 20);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 21);
    TEST((d != 450.0), 0, test_name, 22);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 23);
    TEST((d != 0.0), 0, test_name, 24);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 25);
    TEST(u_strcmp(v450_sm1, text), 0, test_name, 26);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded-exact */
    TEST(cif_value_init_numb(value, 450.0, 0.0, -2, 6), CIF_OK, test_name, 27);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 28);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 29);
    TEST((d != 400.0), 0, test_name, 30);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 31);
    TEST((d != 0.0), 0, test_name, 32);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 33);
    TEST(u_strcmp(v450_sm2, text), 0, test_name, 34);
    free(text);

    /* reinitialize the value as kind NUMB, scale -1, rounded-exact */
    TEST(cif_value_init_numb(value, 992.0, 0.0, -1, 6), CIF_OK, test_name, 35);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 36);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 37);
    TEST((d != 990.0), 0, test_name, 38);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 39);
    TEST((d != 0.0), 0, test_name, 40);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 41);
    TEST(u_strcmp(v992_sm1, text), 0, test_name, 42);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded-exact, with rounding up */
    TEST(cif_value_init_numb(value, 992.0, 0.0, -2, 6), CIF_OK, test_name, 43);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 44);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 45);
    TEST((d != 1000.0), 0, test_name, 46);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 47);
    TEST((d != 0.0), 0, test_name, 48);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 49);
    TEST(u_strcmp(v992_sm2, text), 0, test_name, 50);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded-exact, with rounding up */
    TEST(cif_value_init_numb(value, 992.0, 0.0, -2, 6), CIF_OK, test_name, 51);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 52);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 53);
    TEST((d != 1000.0), 0, test_name, 54);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 55);
    TEST((d != 0.0), 0, test_name, 56);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 57);
    TEST(u_strcmp(v992_sm2, text), 0, test_name, 58);
    free(text);

    /* reinitialize the value as kind NUMB, scale 3, measured, negative */
    TEST(cif_value_init_numb(value, -12.345, 0.017, 3, 6), CIF_OK, test_name, 59);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 60);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 61);
    TEST(!assert_doubles_equal(d, -12.345, DBL_TEST_ULPS), 0, test_name, 62);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 63);
    TEST(!assert_doubles_equal(d, 0.017, DBL_TEST_ULPS), 0, test_name, 64);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 65);
    TEST(u_strcmp(vm12_345s_017_s3, text), 0, test_name, 66);
    free(text);

    /* reinitialize the value as kind NUMB, scale -1, measured, negative */
    /* Note: exact FP comparisons in this case */
    TEST(cif_value_init_numb(value, -12.345, 0.017, -1, 6), CIF_OK, test_name, 67);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 68);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 69);
    TEST((d != -10), 0, test_name, 70);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 71);
    TEST((d != 0.0), 0, test_name, 72);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 73);
    TEST(u_strcmp(vm12_345s_017_sm1, text), 0, test_name, 74);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, measured, non-zero sig-figs from rounding */
    /* Note: exact FP comparisons in this case */
    TEST(cif_value_init_numb(value, 0.5, 1.0, 0, 6), CIF_OK, test_name, 75);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 76);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 77);
    TEST((d != 0.0), 0, test_name, 78);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 79);
    TEST((d != 1.0), 0, test_name, 80);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 81);
    TEST(u_strcmp(vm0_5s_10_s0, text), 0, test_name, 82);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, measured, non-zero sig-figs from rounding */
    /* Note: exact FP comparisons in this case */
    TEST(cif_value_init_numb(value, 0.6, 1.0, 0, 6), CIF_OK, test_name, 83);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 84);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 85);
    TEST((d != 1.0), 0, test_name, 86);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 87);
    TEST((d != 1.0), 0, test_name, 88);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 89);
    TEST(u_strcmp(vm0_6s_10_s0, text), 0, test_name, 90);
    free(text);

    /* reinitialize the value as kind NUMB, scale 8, measured, excessive leading zeroes */
    TEST(cif_value_init_numb(value, 0.00000042, 0.00000017, 8, 5), CIF_OK, test_name, 91);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 92);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 93);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 94);
    TEST(!assert_doubles_equal(d, 0.00000042, DBL_TEST_ULPS), 0, test_name, 95);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 96);
    TEST(!assert_doubles_equal(d, 0.00000017, DBL_TEST_ULPS), 0, test_name, 97);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 98);
    TEST(u_strcmp(vm0_00000042s_00000017_s8, text), 0, test_name, 99);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded, uncertainty rounded to zero */
    /* Note: one exact FP comparison in this case */
    TEST(cif_value_init_numb(value, 12345.0, 1.0, -2, 1), CIF_OK, test_name, 100);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 101);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 102);
    TEST(!assert_doubles_equal(d, 12300.0, DBL_TEST_ULPS), 0, test_name, 103);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 104);
    TEST((d != 0.0), 0, test_name, 105);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 106);
    TEST(u_strcmp(v1_23e4, text), 0, test_name, 107);
    free(text);

    /* reinitialize the value as kind NUMB, scale 0, rounded to no significant digits */
    /* Note: exact FP comparisons in this case */
    TEST(cif_value_init_numb(value, 0.0625, 1.0, 0, 1), CIF_OK, test_name, 108);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 109);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 110);
    TEST((d != 0.0), 0, test_name, 111);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 112);
    TEST((d != 1.0), 0, test_name, 113);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 114);
    TEST(u_strcmp(v0s1, text), 0, test_name, 115);
    free(text);

    /* reinitialize the value as kind NUMB, scale -2, rounded to no significant digits */
    /* Note: exact FP comparisons in this case */
    TEST(cif_value_init_numb(value, 6.25, 100.0, -2, 1), CIF_OK, test_name, 116);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 117);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 118);
    TEST((d != 0.0), 0, test_name, 119);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 120);
    TEST((d != 100.0), 0, test_name, 121);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 122);
    TEST(u_strcmp(v0e2s1, text), 0, test_name, 123);
    free(text);

    cif_value_free(value);

    return 0;
}

