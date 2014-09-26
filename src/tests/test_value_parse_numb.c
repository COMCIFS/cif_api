/*
 * test_value_parse_numb.c
 *
 * Tests the CIF API's cif_value_parse_numb() function
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"

#define USE_USTDERR
#include "test.h"

int main(void) {
    char test_name[80] = "test_value_parse_numb";
    cif_value_t *value;
    UChar *text;
    double d;
    UChar v[1] = { 0 };  /* malformed */
    U_STRING_DECL(ve00s2, "e+00(2)", 8);  /* malformed */
    U_STRING_DECL(v1_0es2, "1.0e(2)", 8);  /* malformed */
    U_STRING_DECL(v1_0e00s2x, "1.0e+00(2", 10);  /* malformed */
    U_STRING_DECL(v1_0e00sx2, "1.0e+002)", 10);  /* malformed */
    U_STRING_DECL(v1_0e00s2b, "1.0e+00(2) ", 12);  /* malformed */
    U_STRING_DECL(vb1_0e00s2, " 1.0e+00(2)", 12);  /* malformed */
    U_STRING_DECL(v1_0be00s2, "1.0 e+00(2)", 12);  /* malformed */
    U_STRING_DECL(v7_0e00s2_0, "7.0e+00(2.0)", 13);  /* malformed */
    U_STRING_DECL(v0, "0", 2);
    U_STRING_DECL(v17, "17", 3);
    U_STRING_DECL(v170_, "170.", 5);
    U_STRING_DECL(v_32, ".32", 4);
    U_STRING_DECL(vm17_00, "-17.00", 7);
    U_STRING_DECL(vm17_00e3, "-17.00e+3", 10);
    U_STRING_DECL(vm17_00em4, "-17.00e-04", 11);
    U_STRING_DECL(v2_142e104, "2.142e+104", 11);
    U_STRING_DECL(vm173s2, "-173(2)", 8);
    U_STRING_DECL(v73_s120, "73.(120)", 9);
    U_STRING_DECL(v1_0e00s2, "1.0e+00(2)", 11);
    U_STRING_DECL(v3_456e20s7, "3.456e+20(7)", 13);
    U_STRING_DECL(vm_00456e20s7, "-.00456e+20(7)", 15);

    U_STRING_INIT(ve00s2, "e+00(2)", 8);  /* malformed */
    U_STRING_INIT(v1_0es2, "1.0e(2)", 8);  /* malformed */
    U_STRING_INIT(v1_0e00s2x, "1.0e+00(2", 10);  /* malformed */
    U_STRING_INIT(v1_0e00sx2, "1.0e+002)", 10);  /* malformed */
    U_STRING_INIT(v1_0e00s2b, "1.0e+00(2) ", 12);  /* malformed */
    U_STRING_INIT(vb1_0e00s2, " 1.0e+00(2)", 12);  /* malformed */
    U_STRING_INIT(v1_0be00s2, "1.0 e+00(2)", 12);  /* malformed */
    U_STRING_INIT(v7_0e00s2_0, "7.0e+00(2.0)", 13);  /* malformed */
    U_STRING_INIT(v0, "0", 2);
    U_STRING_INIT(v17, "17", 3);
    U_STRING_INIT(v170_, "170.", 5);
    U_STRING_INIT(v_32, ".32", 4);
    U_STRING_INIT(vm17_00, "-17.00", 7);
    U_STRING_INIT(vm17_00e3, "-17.00e+3", 10);
    U_STRING_INIT(vm17_00em4, "-17.00e-04", 11);
    U_STRING_INIT(v2_142e104, "2.142e+104", 11);
    U_STRING_INIT(vm173s2, "-173(2)", 8);
    U_STRING_INIT(v73_s120, "73.(120)", 9);
    U_STRING_INIT(v1_0e00s2, "1.0e+00(2)", 11);
    U_STRING_INIT(v3_456e20s7, "3.456e+20(7)", 13);
    U_STRING_INIT(vm_00456e20s7, "-.00456e+20(7)", 15);

    INIT_USTDERR;

    TESTHEADER(test_name);

    /* Start with a value of kind CIF_UNK_KIND */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 3);

    /* Test various malformations */
    TEST(((text = cif_u_strdup(v)) == NULL), 0, test_name, 4);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 5);
    free(text);
    TEST(((text = cif_u_strdup(ve00s2)) == NULL), 0, test_name, 6);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 7);
    free(text);
    TEST(((text = cif_u_strdup(v1_0es2)) == NULL), 0, test_name, 8);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 9);
    free(text);
    TEST(((text = cif_u_strdup(v1_0e00s2x)) == NULL), 0, test_name, 10);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 11);
    free(text);
    TEST(((text = cif_u_strdup(v1_0e00sx2)) == NULL), 0, test_name, 12);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 13);
    free(text);
    TEST(((text = cif_u_strdup(v1_0e00s2b)) == NULL), 0, test_name, 14);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 15);
    free(text);
    TEST(((text = cif_u_strdup(vb1_0e00s2)) == NULL), 0, test_name, 16);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 17);
    free(text);
    TEST(((text = cif_u_strdup(v1_0be00s2)) == NULL), 0, test_name, 18);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 19);
    free(text);
    TEST(((text = cif_u_strdup(v7_0e00s2_0)) == NULL), 0, test_name, 20);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 21);
    free(text);

    /* test exact numbers */
    TEST(((text = cif_u_strdup(v0)) == NULL), 0, test_name, 22);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 23);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 24);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 25);
    TEST((d != 0.0), 0, test_name, 26);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 27);
    TEST((d != 0.0), 0, test_name, 28);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 29);
    TEST(u_strcmp(v0, text), 0, test_name, 30);
    free(text);

    TEST(((text = cif_u_strdup(v17)) == NULL), 0, test_name, 31);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 32);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 33);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 34);
    TEST((d != 17.0), 0, test_name, 35);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 36);
    TEST((d != 0.0), 0, test_name, 37);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 38);
    TEST(u_strcmp(v17, text), 0, test_name, 39);
    free(text);

    TEST(((text = cif_u_strdup(v170_)) == NULL), 0, test_name, 40);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 41);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 42);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 43);
    TEST((d != 170.0), 0, test_name, 44);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 45);
    TEST((d != 0.0), 0, test_name, 46);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 47);
    TEST(u_strcmp(v170_, text), 0, test_name, 48);
    free(text);

    TEST(((text = cif_u_strdup(v_32)) == NULL), 0, test_name, 49);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 50);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 51);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 52);
    TEST((fabs(d - 0.32) > 0.0001), 0, test_name, 53);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 54);
    TEST((d != 0.0), 0, test_name, 55);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 56);
    TEST(u_strcmp(v_32, text), 0, test_name, 57);
    free(text);

    TEST(((text = cif_u_strdup(vm17_00)) == NULL), 0, test_name, 58);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 59);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 60);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 61);
    TEST((d != -17.0), 0, test_name, 62);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 63);
    TEST((d != 0.0), 0, test_name, 64);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 65);
    TEST(u_strcmp(vm17_00, text), 0, test_name, 66);
    free(text);

    TEST(((text = cif_u_strdup(vm17_00e3)) == NULL), 0, test_name, 67);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 68);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 69);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 70);
    TEST((d != -17000.0), 0, test_name, 71);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 72);
    TEST((d != 0.0), 0, test_name, 73);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 74);
    TEST(u_strcmp(vm17_00e3, text), 0, test_name, 75);
    free(text);

    TEST(((text = cif_u_strdup(vm17_00em4)) == NULL), 0, test_name, 76);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 77);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 78);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 79);
    TEST((fabs(d + 0.0017) > 0.000001) , 0, test_name, 80);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 81);
    TEST((d != 0.0), 0, test_name, 82);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 83);
    TEST(u_strcmp(vm17_00em4, text), 0, test_name, 84);
    free(text);

    TEST(((text = cif_u_strdup(v2_142e104)) == NULL), 0, test_name, 85);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 86);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 87);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 88);
    TEST((fabs(d - 2.142e+104) > 1e+99), 0, test_name, 89);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 90);
    TEST((d != 0.0), 0, test_name, 91);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 92);
    TEST(u_strcmp(v2_142e104, text), 0, test_name, 93);
    free(text);

    /* test measured numbers */
    TEST(((text = cif_u_strdup(vm173s2)) == NULL), 0, test_name, 94);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 95);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 96);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 97);
    TEST((d != -173.0), 0, test_name, 98);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 99);
    TEST((d != 2.0), 0, test_name, 100);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 101);
    TEST(u_strcmp(vm173s2, text), 0, test_name, 102);
    free(text);

    TEST(((text = cif_u_strdup(v73_s120)) == NULL), 0, test_name, 103);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 104);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 105);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 106);
    TEST((d != 73.0), 0, test_name, 107);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 108);
    TEST((d != 120.0), 0, test_name, 109);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 110);
    TEST(u_strcmp(v73_s120, text), 0, test_name, 111);
    free(text);

    TEST(((text = cif_u_strdup(v1_0e00s2)) == NULL), 0, test_name, 112);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 113);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 114);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 115);
    TEST((d != 1.0), 0, test_name, 116);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 117);
    TEST((fabs(d - 0.2) > 0.001), 0, test_name, 118);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 119);
    TEST(u_strcmp(v1_0e00s2, text), 0, test_name, 120);
    free(text);

    TEST(((text = cif_u_strdup(v3_456e20s7)) == NULL), 0, test_name, 121);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 122);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 123);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 124);
    TEST((d != 3.456e+20), 0, test_name, 125);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 126);
    TEST((d != 7e+17), 0, test_name, 127);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 128);
    TEST(u_strcmp(v3_456e20s7, text), 0, test_name, 129);
    free(text);

    TEST(((text = cif_u_strdup(vm_00456e20s7)) == NULL), 0, test_name, 130);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 131);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 132);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 133);
    TEST((d != -4.56e+17), 0, test_name, 134);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 135);
    TEST((d != 7e+15), 0, test_name, 136);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 137);
    TEST(u_strcmp(vm_00456e20s7, text), 0, test_name, 138);
    free(text);

    cif_value_free(value);

    return 0;
}

