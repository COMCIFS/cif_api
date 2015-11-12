/*
 * test_analyze_string.c
 *
 * Tests the CIF API's cif_analyze_string() function
 *
 * Copyright 2015 John C. Bollinger
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

#define HASH   0x23
#define DOLR   0x24
#define OPAREN 0x28
#define CPAREN 0x29
#define OBRACK 0x5B
#define CBRACK 0x5D
#define OBRACE 0x7B
#define CBRACE 0x7D
#define BACKSL 0x5C
#define UNDERS 0x5F

#define CIF_TRUE  (1 == 1)
#define CIF_FALSE (1 == 0)

const UChar empty[] = { 0 };
const UChar bare[] = { 'b', 'a', 'r', 'e', HASH, DOLR, OPAREN, CPAREN, '.',
        '?', UNDERS, ';', BACKSL, '"', '\'', 'e', 'r', 'a', 'b', 0 };
const UChar not_comment[] = { HASH, 'n', 'o', 't', 'a', 'c', 'o', 'm', 'm', 'e', 'n', 't', 0 };
const UChar not_dname[] = { UNDERS, 'n', 'o', 't', UNDERS, 'a', UNDERS, 'n', 'a', 'm', 'e', 0 };
const UChar not_list[] = { 'a', 'b', 'c', OBRACK, CBRACK, 'd', 'e', 'f', 0 };
const UChar not_table[] = { 'a', 'b', 'c', OBRACE, CBRACE, 'd', 'e', 'f', 0 };

int main(void) {
    U_STRING_DECL(not_squoted, "'apostrophes!'", 15);
    U_STRING_DECL(not_dquoted, "\"quotes\"", 10);
    U_STRING_DECL(not_bothquoted, "\"'quotes!'\"", 12);
    U_STRING_DECL(not_unknown, "?", 2);
    U_STRING_DECL(not_na, ".", 2);
    U_STRING_DECL(init_semi, ";not-text", 10);
    U_STRING_DECL(wspace, "A quick brown fox", 18);
    U_STRING_DECL(wspace_end, "Outer ", 7);
    U_STRING_DECL(semis, "semicolons:;;;;;;;+more:;;;\n;;;;;", 35);
    U_STRING_DECL(apos3_line, "Triple apostrophes (''')", 25);
    U_STRING_DECL(apos3_lines, "Like this: \r  '''", 18);
    U_STRING_DECL(apos3_text, "Like this:\n  ''' \"", 19);
    U_STRING_DECL(quote3_line, "Triple quotes (\"\"\")", 20);
    U_STRING_DECL(quote3_lines, "Like that: \n  \"\"\"", 18);
    U_STRING_DECL(quote3_text, "Or that:\n  \"\"\" '", 19);
    U_STRING_DECL(potpourri, "notpfx\\\nA bit of \"\"\", \r\n a bit of ''', and a bit of\n;", 54);
    U_STRING_DECL(reserved1, "data_", 6);
    U_STRING_DECL(reserved2, "lOop_", 6);
    U_STRING_DECL(reserved3, "savE_foo", 9);
    U_STRING_DECL(reserved4, "Global_", 8);
    U_STRING_DECL(reserved5, "stoP_", 6);

    struct cif_string_analysis_s analysis = { { 0 } };
    char test_name[80] = "test_analyze_string";
    int len;

    U_STRING_INIT(not_squoted, "'apostrophes!'", 15);
    U_STRING_INIT(not_dquoted, "\"quotes\"", 10);
    U_STRING_INIT(not_bothquoted, "\"'quotes!'\"", 12);
    U_STRING_INIT(not_unknown, "?", 2);
    U_STRING_INIT(not_na, ".", 2);
    U_STRING_INIT(init_semi, ";not-text", 10);
    U_STRING_INIT(wspace, "A quick brown fox", 18);
    U_STRING_INIT(wspace_end, "Outer ", 7);
    U_STRING_INIT(semis, "semicolons:;;;;;;;+more:;;;\n;;;;;", 35);
    U_STRING_INIT(apos3_line, "Triple apostrophes (''')", 25);
    U_STRING_INIT(apos3_lines, "Like this: \r  '''", 18);
    U_STRING_INIT(apos3_text, "Like this:\n  ''' \"", 19);
    U_STRING_INIT(quote3_line, "Triple quotes (\"\"\")", 20);
    U_STRING_INIT(quote3_lines, "Like that: \n  \"\"\"", 18);
    U_STRING_INIT(quote3_text, "Or that:\n  \"\"\" '", 19);
    U_STRING_INIT(potpourri, "notpfx\\\nA bit of \"\"\", \r\n a bit of ''', and a bit of\n;", 54);
    U_STRING_INIT(reserved1, "data_", 6);
    U_STRING_INIT(reserved2, "lOop_", 6);
    U_STRING_INIT(reserved3, "savE_foo", 9);
    U_STRING_INIT(reserved4, "Global_", 8);
    U_STRING_INIT(reserved5, "stoP_", 6);


    TESTHEADER(test_name);

    /* test analyzing an empty string */
    TEST(cif_analyze_string(empty, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 1);
    TEST(analysis.delim_length, 1, test_name, 2);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 3);
    TEST(analysis.length, 0, test_name, 4);
    TEST(analysis.length_first, 0, test_name, 5);
    TEST(analysis.length_last, 0, test_name, 6);
    TEST(analysis.length_max, 0, test_name, 7);
    TEST(analysis.num_lines, 1, test_name, 8);
    TEST(analysis.max_semi_run, 0, test_name, 9);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 10);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 11);

    /* analyze a string that can be presented bare, despite odd contents */
    len = u_strlen(bare);
    TEST(cif_analyze_string(bare, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 12);
    TEST(analysis.delim_length, 0, test_name, 13);
    TEST(analysis.delim[0], 0, test_name, 14);
    TEST(analysis.length, len, test_name, 15);
    TEST(analysis.length_first, len, test_name, 16);
    TEST(analysis.length_last, len, test_name, 17);
    TEST(analysis.length_max, len, test_name, 18);
    TEST(analysis.num_lines, 1, test_name, 19);
    TEST(analysis.max_semi_run, 1, test_name, 20);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 21);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 22);

    /* analyze a string that looks like a no-whitespace comment */
    len = u_strlen(not_comment);
    TEST(cif_analyze_string(not_comment, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 23);
    TEST(analysis.delim_length, 1, test_name, 24);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 25);
    TEST(analysis.length, len, test_name, 26);
    TEST(analysis.length_first, len, test_name, 27);
    TEST(analysis.length_last, len, test_name, 28);
    TEST(analysis.length_max, len, test_name, 29);
    TEST(analysis.num_lines, 1, test_name, 30);
    TEST(analysis.max_semi_run, 0, test_name, 31);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 32);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 33);

    /* analyze a string that looks like a data name */
    len = u_strlen(not_dname);
    TEST(cif_analyze_string(not_dname, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 34);
    TEST(analysis.delim_length, 1, test_name, 35);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 36);
    TEST(analysis.length, len, test_name, 37);
    TEST(analysis.length_first, len, test_name, 38);
    TEST(analysis.length_last, len, test_name, 39);
    TEST(analysis.length_max, len, test_name, 40);
    TEST(analysis.num_lines, 1, test_name, 41);
    TEST(analysis.max_semi_run, 0, test_name, 42);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 43);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 44);

    /* analyze a string that contains list delimiters */
    len = u_strlen(not_list);
    TEST(cif_analyze_string(not_list, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 45);
    TEST(analysis.delim_length, 1, test_name, 46);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 47);
    TEST(analysis.length, len, test_name, 48);
    TEST(analysis.length_first, len, test_name, 49);
    TEST(analysis.length_last, len, test_name, 50);
    TEST(analysis.length_max, len, test_name, 51);
    TEST(analysis.num_lines, 1, test_name, 52);
    TEST(analysis.max_semi_run, 0, test_name, 53);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 54);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 55);

    /* analyze a string that contains table delimiters */
    len = u_strlen(not_table);
    TEST(cif_analyze_string(not_table, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 56);
    TEST(analysis.delim_length, 1, test_name, 57);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 58);
    TEST(analysis.length, len, test_name, 59);
    TEST(analysis.length_first, len, test_name, 60);
    TEST(analysis.length_last, len, test_name, 61);
    TEST(analysis.length_max, len, test_name, 62);
    TEST(analysis.num_lines, 1, test_name, 63);
    TEST(analysis.max_semi_run, 0, test_name, 64);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 65);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 66);

    /* analyze a string that contains apostrophes */
    len = u_strlen(not_squoted);
    TEST(cif_analyze_string(not_squoted, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 67);
    TEST(analysis.delim_length, 1, test_name, 68);
    TEST(analysis.delim[0], '"', test_name, 69);
    TEST(analysis.length, len, test_name, 70);
    TEST(analysis.length_first, len, test_name, 71);
    TEST(analysis.length_last, len, test_name, 72);
    TEST(analysis.length_max, len, test_name, 73);
    TEST(analysis.num_lines, 1, test_name, 74);
    TEST(analysis.max_semi_run, 0, test_name, 75);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 76);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 77);

    /* analyze a string that contains quotation marks */
    len = u_strlen(not_dquoted);
    TEST(cif_analyze_string(not_dquoted, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 78);
    TEST(analysis.delim_length, 1, test_name, 79);
    TEST(analysis.delim[0], '\'', test_name, 80);
    TEST(analysis.length, len, test_name, 81);
    TEST(analysis.length_first, len, test_name, 82);
    TEST(analysis.length_last, len, test_name, 83);
    TEST(analysis.length_max, len, test_name, 84);
    TEST(analysis.num_lines, 1, test_name, 85);
    TEST(analysis.max_semi_run, 0, test_name, 86);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 87);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 88);

    /* analyze a string that contains apostrophes and ends with a quotation mark */
    len = u_strlen(not_bothquoted);
    TEST(cif_analyze_string(not_bothquoted, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 89);
    TEST(analysis.delim_length, 3, test_name, 90);
    TEST(analysis.delim[0], '\'', test_name, 91);
    TEST(analysis.length, len, test_name, 92);
    TEST(analysis.length_first, len, test_name, 93);
    TEST(analysis.length_last, len, test_name, 94);
    TEST(analysis.length_max, len, test_name, 95);
    TEST(analysis.num_lines, 1, test_name, 96);
    TEST(analysis.max_semi_run, 0, test_name, 97);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 98);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 99);
    TEST(cif_analyze_string(not_bothquoted, CIF_TRUE, CIF_FALSE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 100);
    TEST(analysis.delim_length, 2, test_name, 101);

    /* analyze a string that looks like an unknown-value placeholder */
    len = u_strlen(not_unknown);
    TEST(cif_analyze_string(not_unknown, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 102);
    TEST(analysis.delim_length, 1, test_name, 103);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 104);
    TEST(analysis.length, len, test_name, 105);
    TEST(analysis.length_first, len, test_name, 106);
    TEST(analysis.length_last, len, test_name, 107);
    TEST(analysis.length_max, len, test_name, 108);
    TEST(analysis.num_lines, 1, test_name, 109);
    TEST(analysis.max_semi_run, 0, test_name, 110);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 111);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 112);

    /* analyze a string that looks like an not-applicable placeholder */
    len = u_strlen(not_na);
    TEST(cif_analyze_string(not_na, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 113);
    TEST(analysis.delim_length, 1, test_name, 114);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 115);
    TEST(analysis.length, len, test_name, 116);
    TEST(analysis.length_first, len, test_name, 117);
    TEST(analysis.length_last, len, test_name, 118);
    TEST(analysis.length_max, len, test_name, 119);
    TEST(analysis.num_lines, 1, test_name, 120);
    TEST(analysis.max_semi_run, 0, test_name, 121);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 122);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 123);

    /* analyze a string that starts with a semicolon */
    len = u_strlen(init_semi);
    TEST(cif_analyze_string(init_semi, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 113);
    TEST(analysis.delim_length, 1, test_name, 114);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 115);
    TEST(analysis.length, len, test_name, 116);
    TEST(analysis.length_first, len, test_name, 117);
    TEST(analysis.length_last, len, test_name, 118);
    TEST(analysis.length_max, len, test_name, 119);
    TEST(analysis.num_lines, 1, test_name, 120);
    TEST(analysis.max_semi_run, 1, test_name, 121);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 122);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 123);

    /* analyze a string that starts with a semicolon */
    len = u_strlen(init_semi);
    TEST(cif_analyze_string(init_semi, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 113);
    TEST(analysis.delim_length, 1, test_name, 114);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 115);
    TEST(analysis.length, len, test_name, 116);
    TEST(analysis.length_first, len, test_name, 117);
    TEST(analysis.length_last, len, test_name, 118);
    TEST(analysis.length_max, len, test_name, 119);
    TEST(analysis.num_lines, 1, test_name, 120);
    TEST(analysis.max_semi_run, 1, test_name, 121);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 122);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 123);

    /* analyze a string that contains spaces */
    len = u_strlen(wspace);
    TEST(cif_analyze_string(wspace, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 124);
    TEST(analysis.delim_length, 1, test_name, 125);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 126);
    TEST(analysis.length, len, test_name, 127);
    TEST(analysis.length_first, len, test_name, 128);
    TEST(analysis.length_last, len, test_name, 129);
    TEST(analysis.length_max, len, test_name, 130);
    TEST(analysis.num_lines, 1, test_name, 131);
    TEST(analysis.max_semi_run, 0, test_name, 132);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 133);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 134);

    /* analyze a string that ends with a space */
    len = u_strlen(wspace_end);
    TEST(cif_analyze_string(wspace_end, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 135);
    TEST(analysis.delim_length, 1, test_name, 136);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 137);
    TEST(analysis.length, len, test_name, 138);
    TEST(analysis.length_first, len, test_name, 139);
    TEST(analysis.length_last, len, test_name, 140);
    TEST(analysis.length_max, len, test_name, 141);
    TEST(analysis.num_lines, 1, test_name, 142);
    TEST(analysis.max_semi_run, 0, test_name, 143);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 144);
    TEST(analysis.has_trailing_ws, CIF_TRUE, test_name, 145);

    /* analyze a string that contains a text-block delimiter and other semicolons */
    len = u_strlen(semis);
    TEST(cif_analyze_string(semis, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 146);
    TEST(analysis.delim_length, 3, test_name, 147);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 148);
    TEST(analysis.length, len, test_name, 149);
    TEST(analysis.length_first, 27, test_name, 150);
    TEST(analysis.length_last, 5, test_name, 151);
    TEST(analysis.length_max, 27, test_name, 152);
    TEST(analysis.num_lines, 2, test_name, 153);
    TEST(analysis.max_semi_run, 7, test_name, 154);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 155);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 156);

    /* analyze a string that contains a trebled apostrophe */
    len = u_strlen(apos3_line);
    TEST(cif_analyze_string(apos3_line, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 157);
    TEST(analysis.delim_length, 1, test_name, 158);
    TEST(analysis.delim[0], '"', test_name, 159);
    TEST(analysis.length, len, test_name, 160);
    TEST(analysis.length_first, len, test_name, 161);
    TEST(analysis.length_last, len, test_name, 162);
    TEST(analysis.length_max, len, test_name, 163);
    TEST(analysis.num_lines, 1, test_name, 164);
    TEST(analysis.max_semi_run, 0, test_name, 165);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 166);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 167);

    /* analyze a string that contains a trebled quotation mark */
    len = u_strlen(quote3_line);
    TEST(cif_analyze_string(quote3_line, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 168);
    TEST(analysis.delim_length, 1, test_name, 169);
    TEST(analysis.delim[0], '\'', test_name, 170);
    TEST(analysis.length, len, test_name, 171);
    TEST(analysis.length_first, len, test_name, 172);
    TEST(analysis.length_last, len, test_name, 173);
    TEST(analysis.length_max, len, test_name, 174);
    TEST(analysis.num_lines, 1, test_name, 175);
    TEST(analysis.max_semi_run, 0, test_name, 176);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 177);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 178);

    /* analyze a multi-line string that contains a trebled apostrophe */
    len = u_strlen(apos3_lines);
    TEST(cif_analyze_string(apos3_lines, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 179);
    TEST(analysis.delim_length, 3, test_name, 180);
    TEST(analysis.delim[0], '"', test_name, 181);
    TEST(analysis.length, len, test_name, 182);
    TEST(analysis.length_first, 11, test_name, 183);
    TEST(analysis.length_last, 5, test_name, 184);
    TEST(analysis.length_max, 11, test_name, 185);
    TEST(analysis.num_lines, 2, test_name, 186);
    TEST(analysis.max_semi_run, 0, test_name, 187);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 188);
    TEST(analysis.has_trailing_ws, CIF_TRUE, test_name, 189);

    /* analyze a multi-line string that contains a trebled apostrophe and ends with a quotation mark */
    len = u_strlen(apos3_text);
    TEST(cif_analyze_string(apos3_text, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 190);
    TEST(analysis.delim_length, 2, test_name, 191);
    TEST(analysis.delim[1], ';', test_name, 192);
    TEST(analysis.length, len, test_name, 193);
    TEST(analysis.length_first, 10, test_name, 194);
    TEST(analysis.length_last, 7, test_name, 195);
    TEST(analysis.length_max, 10, test_name, 196);
    TEST(analysis.num_lines, 2, test_name, 197);
    TEST(analysis.max_semi_run, 0, test_name, 198);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 199);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 200);

    /* analyze a multi-line string that contains a trebled quotation mark */
    len = u_strlen(quote3_lines);
    TEST(cif_analyze_string(quote3_lines, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 201);
    TEST(analysis.delim_length, 3, test_name, 202);
    TEST(analysis.delim[0], '\'', test_name, 203);
    TEST(analysis.length, len, test_name, 204);
    TEST(analysis.length_first, 11, test_name, 205);
    TEST(analysis.length_last, 5, test_name, 206);
    TEST(analysis.length_max, 11, test_name, 207);
    TEST(analysis.num_lines, 2, test_name, 208);
    TEST(analysis.max_semi_run, 0, test_name, 209);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 210);
    TEST(analysis.has_trailing_ws, CIF_TRUE, test_name, 211);

    /* analyze a multi-line string that contains a trebled quotation mark and ends with an apostrophe */
    len = u_strlen(quote3_text);
    TEST(cif_analyze_string(quote3_text, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 212);
    TEST(analysis.delim_length, 2, test_name, 213);
    TEST(analysis.delim[1], ';', test_name, 214);
    TEST(analysis.length, len, test_name, 215);
    TEST(analysis.length_first, 8, test_name, 216);
    TEST(analysis.length_last, 7, test_name, 217);
    TEST(analysis.length_max, 8, test_name, 218);
    TEST(analysis.num_lines, 2, test_name, 219);
    TEST(analysis.max_semi_run, 0, test_name, 220);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 221);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 222);

    /* analyze a multi-line string that contains all varieties of string delimiters */
    len = u_strlen(potpourri);
    TEST(cif_analyze_string(potpourri, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 223);
    TEST(analysis.delim_length, 2, test_name, 224);
    TEST(analysis.delim[1], ';', test_name, 225);
    TEST(analysis.length, len, test_name, 226);
    TEST(analysis.length_first, 7, test_name, 227);
    TEST(analysis.length_last, 1, test_name, 228);
    TEST(analysis.length_max, 27, test_name, 229);
    TEST(analysis.num_lines, 4, test_name, 230);
    TEST(analysis.max_semi_run, 1, test_name, 231);
    TEST(analysis.has_reserved_start, CIF_TRUE, test_name, 232);
    TEST(analysis.has_trailing_ws, CIF_TRUE, test_name, 233);

    /* analyze a string that is reserved from presentation as a whitespace-delimited value */
    len = u_strlen(reserved1);
    TEST(cif_analyze_string(reserved1, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 234);
    TEST(analysis.delim_length, 1, test_name, 235);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 236);
    TEST(analysis.length, len, test_name, 237);
    TEST(analysis.length_first, len, test_name, 238);
    TEST(analysis.length_last, len, test_name, 239);
    TEST(analysis.length_max, len, test_name, 240);
    TEST(analysis.num_lines, 1, test_name, 241);
    TEST(analysis.max_semi_run, 0, test_name, 242);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 243);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 244);

    /* analyze a string that is reserved from presentation as a whitespace-delimited value */
    len = u_strlen(reserved2);
    TEST(cif_analyze_string(reserved2, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 245);
    TEST(analysis.delim_length, 1, test_name, 246);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 247);
    TEST(analysis.length, len, test_name, 248);
    TEST(analysis.length_first, len, test_name, 249);
    TEST(analysis.length_last, len, test_name, 250);
    TEST(analysis.length_max, len, test_name, 251);
    TEST(analysis.num_lines, 1, test_name, 252);
    TEST(analysis.max_semi_run, 0, test_name, 253);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 254);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 255);

    /* analyze a string that is reserved from presentation as a whitespace-delimited value */
    len = u_strlen(reserved3);
    TEST(cif_analyze_string(reserved3, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 256);
    TEST(analysis.delim_length, 1, test_name, 257);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 258);
    TEST(analysis.length, len, test_name, 259);
    TEST(analysis.length_first, len, test_name, 260);
    TEST(analysis.length_last, len, test_name, 261);
    TEST(analysis.length_max, len, test_name, 262);
    TEST(analysis.num_lines, 1, test_name, 263);
    TEST(analysis.max_semi_run, 0, test_name, 264);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 265);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 266);

    /* analyze a string that is reserved from presentation as a whitespace-delimited value */
    len = u_strlen(reserved4);
    TEST(cif_analyze_string(reserved4, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 267);
    TEST(analysis.delim_length, 1, test_name, 268);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 269);
    TEST(analysis.length, len, test_name, 270);
    TEST(analysis.length_first, len, test_name, 271);
    TEST(analysis.length_last, len, test_name, 272);
    TEST(analysis.length_max, len, test_name, 273);
    TEST(analysis.num_lines, 1, test_name, 274);
    TEST(analysis.max_semi_run, 0, test_name, 275);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 276);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 277);

    /* analyze a string that is reserved from presentation as a whitespace-delimited value */
    len = u_strlen(reserved5);
    TEST(cif_analyze_string(reserved5, CIF_TRUE, CIF_TRUE, CIF_LINE_LENGTH, &analysis), CIF_OK, test_name, 278);
    TEST(analysis.delim_length, 1, test_name, 279);
    TEST(analysis.delim[0] != '\'' && analysis.delim[0] != '"', CIF_FALSE, test_name, 280);
    TEST(analysis.length, len, test_name, 281);
    TEST(analysis.length_first, len, test_name, 282);
    TEST(analysis.length_last, len, test_name, 283);
    TEST(analysis.length_max, len, test_name, 284);
    TEST(analysis.num_lines, 1, test_name, 285);
    TEST(analysis.max_semi_run, 0, test_name, 286);
    TEST(analysis.has_reserved_start, CIF_FALSE, test_name, 287);
    TEST(analysis.has_trailing_ws, CIF_FALSE, test_name, 288);

    return 0;
}

