/*
 * test_parse_cif1_quoting.c
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
#define NUM_ITEMS     2
int main(void) {
    char test_name[80] = "test_parse_cif1_quoting";
    char local_file_name[] = "cif1_quoting.cif";
    cif_t *cif = NULL;
    cif_block_t *block = NULL;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_block_t **block_list;
    cif_loop_t **loop_list;
    cif_loop_t *loop;
    cif_value_t *value = NULL;
    UChar **name_list;
    UChar **name_p;
    UChar *ustr;
    double d;

    U_STRING_DECL(block_code, "cif1_quoting", 13);
    U_STRING_DECL(name_sq,    "_sq", 4);
    U_STRING_DECL(name_dq,    "_dq", 4);
    U_STRING_DECL(value_sq,   "don't rock the boat", 20);
    U_STRING_DECL(value_dq,   "What's this ab\\\"out?", 21);

    U_STRING_INIT(block_code, "cif1_quoting", 13);
    U_STRING_INIT(name_sq,    "_sq", 4);
    U_STRING_INIT(name_dq,    "_dq", 4);
    U_STRING_INIT(value_sq,   "don't rock the boat", 20);
    U_STRING_INIT(value_dq,   "What's this ab\\\"out?", 21);

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
    TEST(cif_container_get_value(block, name_sq, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 18);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 19);
    TEST(u_strcmp(value_sq, ustr), 0, test_name, 20);
    free(ustr);

    TEST(cif_container_get_value(block, name_dq, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 22);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 23);
    TEST(u_strcmp(value_dq, ustr), 0, test_name, 24);
    free(ustr);

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
