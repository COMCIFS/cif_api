/*
 * test_value_parse_numb.c
 *
 * Tests the CIF API's cif_value_parse_numb() function
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_value_parse_numb";
    cif_value_t *value;
    UChar *text;
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
    TEST(((text = cif_u_strdup(ve00s2)) == NULL), 0, test_name, 6);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 7);
    TEST(((text = cif_u_strdup(v1_0es2)) == NULL), 0, test_name, 8);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 9);
    TEST(((text = cif_u_strdup(v1_0e00s2x)) == NULL), 0, test_name, 10);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 11);
    TEST(((text = cif_u_strdup(v1_0e00sx2)) == NULL), 0, test_name, 12);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 13);
    TEST(((text = cif_u_strdup(v1_0e00s2b)) == NULL), 0, test_name, 14);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 15);
    TEST(((text = cif_u_strdup(vb1_0e00s2)) == NULL), 0, test_name, 16);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 17);
    TEST(((text = cif_u_strdup(v1_0be00s2)) == NULL), 0, test_name, 18);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 19);
    TEST(((text = cif_u_strdup(v7_0e00s2_0)) == NULL), 0, test_name, 20);
    TEST(cif_value_parse_numb(value, text), CIF_INVALID_NUMBER, test_name, 21);

    /* test exact numbers */
    TEST(((text = cif_u_strdup(v0)) == NULL), 0, test_name, 22);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 23);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 24);
    TEST((cif_value_as_double(value) != 0.0), 0, test_name, 25);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 26);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 27);
    TEST(u_strcmp(v0, text), 0, test_name, 28);

    TEST(((text = cif_u_strdup(v17)) == NULL), 0, test_name, 29);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 30);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 31);
    TEST((cif_value_as_double(value) != 17.0), 0, test_name, 32);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 33);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 34);
    TEST(u_strcmp(v17, text), 0, test_name, 35);

    TEST(((text = cif_u_strdup(v170_)) == NULL), 0, test_name, 36);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 37);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 38);
    TEST((cif_value_as_double(value) != 170.0), 0, test_name, 39);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 40);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 41);
    TEST(u_strcmp(v170_, text), 0, test_name, 42);

    TEST(((text = cif_u_strdup(v_32)) == NULL), 0, test_name, 43);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 44);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 45);
    TEST((fabs(cif_value_as_double(value) - 0.32) > 0.0001), 0, test_name, 46);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 47);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 48);
    TEST(u_strcmp(v_32, text), 0, test_name, 49);

    TEST(((text = cif_u_strdup(vm17_00)) == NULL), 0, test_name, 50);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 51);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 52);
    TEST((cif_value_as_double(value) != -17.0), 0, test_name, 53);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 54);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 55);
    TEST(u_strcmp(vm17_00, text), 0, test_name, 56);

    TEST(((text = cif_u_strdup(vm17_00e3)) == NULL), 0, test_name, 57);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 58);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 59);
    TEST((cif_value_as_double(value) != -17000.0), 0, test_name, 60);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 61);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 62);
    TEST(u_strcmp(vm17_00e3, text), 0, test_name, 63);

    TEST(((text = cif_u_strdup(vm17_00em4)) == NULL), 0, test_name, 64);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 65);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 66);
    TEST((fabs(cif_value_as_double(value) + 0.0017) > 0.000001) , 0, test_name, 67);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 68);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 69);
    TEST(u_strcmp(vm17_00em4, text), 0, test_name, 70);

    TEST(((text = cif_u_strdup(v2_142e104)) == NULL), 0, test_name, 71);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 72);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 73);
    TEST((fabs(cif_value_as_double(value) - 2.142e+104) > 1e+99), 0, test_name, 74);
    TEST((cif_value_su_as_double(value) != 0.0), 0, test_name, 75);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 76);
    TEST(u_strcmp(v2_142e104, text), 0, test_name, 77);

    /* test measured numbers */
    TEST(((text = cif_u_strdup(vm173s2)) == NULL), 0, test_name, 78);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 79);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 80);
    TEST((cif_value_as_double(value) != -173.0), 0, test_name, 81);
    TEST((cif_value_su_as_double(value) != 2.0), 0, test_name, 82);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 83);
    TEST(u_strcmp(vm173s2, text), 0, test_name, 84);

    TEST(((text = cif_u_strdup(v73_s120)) == NULL), 0, test_name, 85);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 86);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 87);
    TEST((cif_value_as_double(value) != 73.0), 0, test_name, 88);
    TEST((cif_value_su_as_double(value) != 120.0), 0, test_name, 89);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 90);
    TEST(u_strcmp(v73_s120, text), 0, test_name, 91);

    TEST(((text = cif_u_strdup(v1_0e00s2)) == NULL), 0, test_name, 92);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 93);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 94);
    TEST((cif_value_as_double(value) != 1.0), 0, test_name, 95);
    TEST((fabs(cif_value_su_as_double(value) - 0.2) > 0.001), 0, test_name, 96);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 97);
    TEST(u_strcmp(v1_0e00s2, text), 0, test_name, 98);

    TEST(((text = cif_u_strdup(v3_456e20s7)) == NULL), 0, test_name, 99);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 100);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 101);
    TEST((cif_value_as_double(value) != 3.456e+20), 0, test_name, 102);
    TEST((cif_value_su_as_double(value) != 7e+17), 0, test_name, 103);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 104);
    TEST(u_strcmp(v3_456e20s7, text), 0, test_name, 105);

    TEST(((text = cif_u_strdup(vm_00456e20s7)) == NULL), 0, test_name, 106);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 107);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 108);
    TEST((cif_value_as_double(value) != -4.56e+17), 0, test_name, 109);
    TEST((cif_value_su_as_double(value) != 7e+15), 0, test_name, 110);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 111);
    TEST(u_strcmp(vm_00456e20s7, text), 0, test_name, 112);

    cif_value_free(value);

    return 0;
}

