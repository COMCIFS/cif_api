/*
 * test_parse_simple_data.c
 *
 * Tests parsing simple CIF 2.0 data.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     8
int main(void) {
    char test_name[80] = "test_parse_simple_data";
    char local_file_name[] = "simple_data.cif";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_block_tp **block_list;
    cif_loop_tp **loop_list;
    cif_loop_tp *loop;
    cif_value_tp *value = NULL;
    UChar **name_list;
    UChar **name_p;
    UChar *ustr;
    double d;

    U_STRING_DECL(block_code,           "simple_data", 12);
    U_STRING_DECL(name_unknown_value,   "_unknown_value", 15);
    U_STRING_DECL(name_na_value,        "_na_value", 10);
    U_STRING_DECL(name_unquoted_string, "_unquoted_string", 17);
    U_STRING_DECL(name_sq_string,       "_sq_string", 11);
    U_STRING_DECL(name_dq_string,       "_dq_string", 11);
    U_STRING_DECL(name_text_string,     "_text_string", 13);
    U_STRING_DECL(name_numb_plain,      "_numb_plain", 12);
    U_STRING_DECL(name_numb_su,         "_numb_su", 9);
    U_STRING_DECL(value_unquoted_string, "unquoted", 9);
    U_STRING_DECL(value_sq_string,       "sq", 3);
    U_STRING_DECL(value_dq_string,       "dq", 3);
    U_STRING_DECL(value_text_string,     "text", 5);

    U_STRING_INIT(block_code,           "simple_data", 12);
    U_STRING_INIT(name_unknown_value,   "_unknown_value", 15);
    U_STRING_INIT(name_na_value,        "_na_value", 10);
    U_STRING_INIT(name_unquoted_string, "_unquoted_string", 17);
    U_STRING_INIT(name_sq_string,       "_sq_string", 11);
    U_STRING_INIT(name_dq_string,       "_dq_string", 11);
    U_STRING_INIT(name_text_string,     "_text_string", 13);
    U_STRING_INIT(name_numb_plain,      "_numb_plain", 12);
    U_STRING_INIT(name_numb_su,         "_numb_su", 9);
    U_STRING_INIT(value_unquoted_string, "unquoted", 9);
    U_STRING_INIT(value_sq_string,       "sq", 3);
    U_STRING_INIT(value_dq_string,       "dq", 3);
    U_STRING_INIT(value_text_string,     "text", 5);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, 3);

    /* check the parse result */
      /* check that there is exactly one block, and that it has the expected code */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 4);
    TEST((*block_list == NULL), 0, test_name, 5);
    TEST_NOT((*(block_list + 1) == NULL), 0, test_name, 6);
    block = *block_list;
    TEST(cif_container_get_code(block, &ustr), CIF_OK, test_name, 7);
    TEST(u_strcmp(block_code, ustr), 0, test_name, 8);
    free(ustr);

      /* check that there is exactly one loop in the block, and that it is the scalar loop */
    TEST(cif_container_get_all_loops(block, &loop_list), CIF_OK, test_name, 9);
    TEST((*loop_list == NULL), 0, test_name, 10);
    TEST_NOT((*(loop_list + 1) == NULL), 0, test_name, 11);
    loop = *loop_list;
    TEST(cif_loop_get_category(loop, &ustr), CIF_OK, test_name, 12);
    TEST(ustr == NULL, 0, test_name, 13);
    TEST(*ustr, 0, test_name, 14);
    free(ustr);

      /* check the number of data names in the loop */
    TEST(cif_loop_get_names(loop, &name_list), CIF_OK, test_name, 15);
    for (name_p = name_list; *name_p; name_p += 1) {
        /* We're responsible for freeing these anyway; might as well do it here and now */
        free(*name_p);
    }
    TEST((name_p - name_list), NUM_ITEMS, test_name, 16);
    free(name_list);

      /* check each expected item */
    TEST(cif_container_get_value(block, name_unknown_value, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_UNK_KIND, test_name, 18);

    TEST(cif_container_get_value(block, name_na_value, &value), CIF_OK, test_name, 19);
    TEST(cif_value_kind(value), CIF_NA_KIND, test_name, 20);

    TEST(cif_container_get_value(block, name_unquoted_string, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 22);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 23);
    TEST(u_strcmp(value_unquoted_string, ustr), 0, test_name, 24);
    free(ustr);

    TEST(cif_container_get_value(block, name_sq_string, &value), CIF_OK, test_name, 25);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 26);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 27);
    TEST(u_strcmp(value_sq_string, ustr), 0, test_name, 28);
    free(ustr);

    TEST(cif_container_get_value(block, name_dq_string, &value), CIF_OK, test_name, 29);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 30);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 31);
    TEST(u_strcmp(value_dq_string, ustr), 0, test_name, 32);
    free(ustr);

    TEST(cif_container_get_value(block, name_text_string, &value), CIF_OK, test_name, 33);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 34);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 35);
    TEST(u_strcmp(value_text_string, ustr), 0, test_name, 36);
    free(ustr);

    TEST(cif_container_get_value(block, name_numb_plain, &value), CIF_OK, test_name, 37);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 38);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 39);
    TEST_NOT(d == 1250.0, 0, test_name, 40);
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 41);
    TEST_NOT(d == 0.0, 0, test_name, 42);

    TEST(cif_container_get_value(block, name_numb_su, &value), CIF_OK, test_name, 43);
    TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 44);
    TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 45);
    TEST_NOT(d == 0.0625, 0, test_name, 46);  /* 0.0625 is exactly representable as an IEEE binary or decimal float */
    TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 47);
    TEST_NOT(abs(d - 0.0002) < 1e-10, 0, test_name, 48);

    /* clean up */
    cif_value_free(value);
    cif_loop_free(loop);
    free(loop_list);
    cif_block_free(block);
    free(block_list);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
