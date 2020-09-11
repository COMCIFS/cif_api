/*
 * test_value_try_quoted.c
 *
 * Tests the behavior of the cif_value_is_quoted() and cif_value_try_quoted() functions.
 *
 * Copyright 2014, 2015, 2020 John C. Bollinger
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
#include "test.h"

int main(void) {
    char test_name[80] = "test_value_try_quoted";
    cif_value_tp *value = NULL;
    U_STRING_DECL(value_text, "value_text", 11);
    U_STRING_DECL(numb_text, "1.234(5)", 9);
    U_STRING_DECL(query_text, "?", 2);
    U_STRING_DECL(dot_text, ".", 2);
    U_STRING_DECL(dataname, "_data1", 7);
    U_STRING_DECL(comment,  "# nope", 7);
    U_STRING_DECL(ref,      "$frame", 7);
    U_STRING_DECL(squoted,  "'oops", 6);
    U_STRING_DECL(dquoted,  "\"oops", 6);
    U_STRING_DECL(ndataname, "data1_", 7);
    U_STRING_DECL(ncomment,  "nope#", 7);
    U_STRING_DECL(nref,      "fr$ame", 7);
    U_STRING_DECL(nsquoted,  "oo'ps", 6);
    U_STRING_DECL(ndquoted,  "oops\"", 6);
    U_STRING_DECL(datahead1,  "data_", 6);
    U_STRING_DECL(datahead2,  "data_d", 7);
    U_STRING_DECL(savehead1,  "save_", 6);
    U_STRING_DECL(savehead2,  "save_s", 7);
    U_STRING_DECL(loop1,  "loop_", 6);
    U_STRING_DECL(loop2,  "loop_1", 7);
    U_STRING_DECL(stop1,  "stop_", 6);
    U_STRING_DECL(stop2,  "stop_s", 7);
    U_STRING_DECL(global1,  "global_", 8);
    U_STRING_DECL(global2,  "global_g", 9);
    U_STRING_DECL(wbrak, "brack]", 7);
    U_STRING_DECL(wbrac, "bra}ce", 7);
    U_STRING_DECL(wspace1, "has space", 10);
    U_STRING_DECL(wspace2, " hasspace", 10);
    U_STRING_DECL(wspace3, "hasspace ", 10);
    UChar *text;
    double d1, d2, su1, su2;

    U_STRING_INIT(value_text, "value_text", 11);
    U_STRING_INIT(numb_text, "1.234(5)", 9);
    U_STRING_INIT(query_text, "?", 2);
    U_STRING_INIT(dot_text, ".", 2);
    U_STRING_INIT(dataname, "_data1", 7);
    U_STRING_INIT(comment,  "# nope", 7);
    U_STRING_INIT(ref,      "$frame", 7);
    U_STRING_INIT(squoted,  "'oops", 6);
    U_STRING_INIT(dquoted,  "\"oops", 6);
    U_STRING_INIT(ndataname, "data1_", 7);
    U_STRING_INIT(nref,      "frame$", 7);
    U_STRING_INIT(ncomment,  "nope#", 7);
    U_STRING_INIT(nref,      "fr$ame", 7);
    U_STRING_INIT(nsquoted,  "oo'ps", 6);
    U_STRING_INIT(ndquoted,  "oops\"", 6);
    U_STRING_INIT(datahead1,  "data_", 6);
    U_STRING_INIT(datahead2,  "data_d", 7);
    U_STRING_INIT(savehead1,  "save_", 6);
    U_STRING_INIT(savehead2,  "save_s", 7);
    U_STRING_INIT(loop1,  "loop_", 6);
    U_STRING_INIT(loop2,  "loop_1", 7);
    U_STRING_INIT(stop1,  "stop_", 6);
    U_STRING_INIT(stop2,  "stop_s", 7);
    U_STRING_INIT(global1,  "global_", 8);
    U_STRING_INIT(global2,  "global_g", 9);
    U_STRING_INIT(wbrak, "brack]", 7);
    U_STRING_INIT(wbrac, "bra}ce", 7);
    U_STRING_INIT(wspace1, "has space", 10);
    U_STRING_INIT(wspace2, " hasspace", 10);
    U_STRING_INIT(wspace3, "hasspace ", 10);

    TESTHEADER(test_name);

    /* Test values of kind 'unk' */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 3);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 4);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_OK, test_name, 5);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 6);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 7);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 8);
    TEST(u_strcmp(text, query_text), 0, test_name, 9);
    free(text);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 10);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 11);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 12);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 13);
    TEST(text != NULL, 0, test_name, 14);
    cif_value_free(value);

    /* Test values of kind 'na' */
    TEST(cif_value_create(CIF_NA_KIND, &value), CIF_OK, test_name, 15);
    TEST((value == NULL), 0, test_name, 16);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 17);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 18);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_OK, test_name, 19);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 20);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 21);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 22);
    TEST(u_strcmp(text, dot_text), 0, test_name, 23);
    free(text);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 24);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 25);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 26);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 27);
    TEST(text != NULL, 0, test_name, 28);
    cif_value_free(value);

    /* Test values of kind 'list' */
    TEST(cif_value_create(CIF_LIST_KIND, &value), CIF_OK, test_name, 29);
    TEST((value == NULL), 0, test_name, 30);
    TEST(cif_value_kind(value), CIF_LIST_KIND, test_name, 31);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 32);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_ARGUMENT_ERROR, test_name, 33);
    cif_value_free(value);

    /* Test values of kind 'table' */
    TEST(cif_value_create(CIF_TABLE_KIND, &value), CIF_OK, test_name, 34);
    TEST((value == NULL), 0, test_name, 35);
    TEST(cif_value_kind(value), CIF_TABLE_KIND, test_name, 36);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 37);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_ARGUMENT_ERROR, test_name, 38);
    cif_value_free(value);

    /* Test values of kind 'char' */
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 39);
    TEST(cif_value_copy_char(value, value_text), CIF_OK, test_name, 40);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 41);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 42);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 43);
    TEST(u_strcmp(text, value_text), 0, test_name, 44);
    free(text);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 45);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 46);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 47);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 48);
    TEST(u_strcmp(text, value_text), 0, test_name, 49);
    free(text);
    cif_value_free(value);

    /* Test values of kind 'numb' */
    text = (UChar *) malloc(16 * sizeof(*text));
    TEST(text == NULL, 0, test_name, 50);
    *text = 0;
    u_strcpy(text, numb_text);
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 51);
    TEST(cif_value_parse_numb(value, text), CIF_OK, test_name, 52);
    text = NULL;
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 53);
    TEST(cif_value_is_quoted(value), CIF_NOT_QUOTED, test_name, 54);
    TEST(cif_value_get_number(value, &d1), CIF_OK, test_name, 55);
    TEST(cif_value_get_su(value, &su1), CIF_OK, test_name, 56);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_OK, test_name, 57);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 58);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 59);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 60);
    TEST(u_strcmp(text, numb_text), 0, test_name, 61);
    free(text);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_OK, test_name, 62);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 63);
    TEST(cif_value_get_number(value, &d2), CIF_OK, test_name, 64);
    TEST(cif_value_get_su(value, &su2), CIF_OK, test_name, 65);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 66);
    TEST(cif_value_try_quoted(value, CIF_QUOTED), CIF_OK, test_name, 67);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 68);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 69);
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 70);
    TEST(u_strcmp(text, numb_text), 0, test_name, 71);
    free(text);
    TEST(d1 != d2, 0, test_name, 72);
    TEST(su1 != su2, 0, test_name, 73);

    /* test reserved (and some unreserved) strings */
    TEST(cif_value_copy_char(value, dataname), CIF_OK, test_name, 74);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 75);
    TEST(cif_value_copy_char(value, comment), CIF_OK, test_name, 76);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 77);

    TEST(cif_value_copy_char(value, ref), CIF_OK, test_name, 78);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 79);
    TEST(cif_value_copy_char(value, squoted), CIF_OK, test_name, 80);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 81);
    TEST(cif_value_copy_char(value, dquoted), CIF_OK, test_name, 82);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 83);
    TEST(cif_value_copy_char(value, datahead1), CIF_OK, test_name, 84);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 85);
    TEST(cif_value_copy_char(value, datahead2), CIF_OK, test_name, 86);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 87);
    TEST(cif_value_copy_char(value, savehead1), CIF_OK, test_name, 88);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 89);
    TEST(cif_value_copy_char(value, savehead2), CIF_OK, test_name, 90);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 91);
    TEST(cif_value_copy_char(value, loop1), CIF_OK, test_name, 92);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 93);
    TEST(cif_value_copy_char(value, loop2), CIF_OK, test_name, 94);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 95);
    TEST(cif_value_copy_char(value, stop1), CIF_OK, test_name, 96);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 97);
    TEST(cif_value_copy_char(value, stop2), CIF_OK, test_name, 98);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 99);
    TEST(cif_value_copy_char(value, global1), CIF_OK, test_name, 100);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 101);
    TEST(cif_value_copy_char(value, global2), CIF_OK, test_name, 102);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 103);

    /*
     * If there are braces or brackets then the function returns CIF_OK without
     * marking the value unquoted.
     */
    TEST(cif_value_copy_char(value, wbrak), CIF_OK, test_name, 104);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 105);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 106);
    TEST(cif_value_copy_char(value, wbrac), CIF_OK, test_name, 107);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 108);
    TEST(cif_value_is_quoted(value), CIF_QUOTED, test_name, 109);

    TEST(cif_value_copy_char(value, ndataname), CIF_OK, test_name, 110);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 111);
    TEST(cif_value_copy_char(value, nref), CIF_OK, test_name, 112);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 113);
    TEST(cif_value_copy_char(value, ncomment), CIF_OK, test_name, 114);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 115);
    TEST(cif_value_copy_char(value, nref), CIF_OK, test_name, 116);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 117);
    TEST(cif_value_copy_char(value, nsquoted), CIF_OK, test_name, 118);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 119);
    TEST(cif_value_copy_char(value, ndquoted), CIF_OK, test_name, 120);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_OK, test_name, 121);
    TEST(cif_value_copy_char(value, wspace1), CIF_OK, test_name, 122);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 123);
    TEST(cif_value_copy_char(value, wspace2), CIF_OK, test_name, 124);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 125);
    TEST(cif_value_copy_char(value, wspace3), CIF_OK, test_name, 126);
    TEST(cif_value_try_quoted(value, CIF_NOT_QUOTED), CIF_ARGUMENT_ERROR, test_name, 127);

    cif_value_free(value);
    return 0;
}

