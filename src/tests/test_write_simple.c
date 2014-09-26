/*
 * test_write_simple.c
 *
 * Tests writing simple CIF 2.0 data.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"

#define USE_USTDERR
#include "assert_cifs.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     8
int main(void) {
    char test_name[80] = "test_write_simple";
    FILE * cif_file;
    cif_t *cif = NULL;
    cif_t *cif_readback = NULL;
    cif_block_t *block = NULL;
    cif_value_t *value = NULL;
    UChar value_sq_string[] = { 'S', 'a', 'y', ' ', '"', 'B', 'o', 'o', '"', 0 };
    UChar value_dq_string[] = { 'D', 'r', '.', ' ', 'O', '\'', 'M', 'a', 'l', 'l', 'e', 'y', 0 };
    UChar value_text_string[] = { 'D', 'e', 'l', 'i', 'm', 's', ' ', 'a', 'r', 'e', ':', 0x0a, '\'', '\'', '\'', ' ',
            'a', 'n', 'd', ' ', '"', '"', '"', 0 };
    UChar value_sq3_string[] = { 'P', 'y', 't', 'h', 'o', 'n', ' ', 'u', 's', 'e', 's', ' ', '"', '"', '"', 0x0a,
            'f', 'o', 'r', ' ', 'm', 'u', 'l', 't', 'i', 'l', 'i', 'n', 'e', 's', 0 };
    UChar value_dq3_string[] = { 'T', 'r', 'y', ' ', 't', 'h', 'i', 's', ':', ' ', '\'', '\'', '\'',
            '_', 'n', 'a', 'm', 'e', 0x0a, ';', 0x0a, ';', '\'', '\'', '\'', 0 };
    U_STRING_DECL(block_code,           "simple_data", 12);
    U_STRING_DECL(name_unknown_value,   "_unknown_value", 15);
    U_STRING_DECL(name_na_value,        "_na_value", 10);
    U_STRING_DECL(name_sq_string,       "_sq_string", 11);
    U_STRING_DECL(name_dq_string,       "_dq_string", 11);
    U_STRING_DECL(name_text_string,     "_text_string", 13);
    U_STRING_DECL(name_sq3_string,      "_sq3_string", 12);
    U_STRING_DECL(name_dq3_string,      "_dq3_string", 12);
    U_STRING_DECL(name_numb_plain,      "_numb_plain", 12);
    U_STRING_DECL(name_numb_su,         "_numb_su", 9);

    U_STRING_INIT(block_code,           "simple_data", 12);
    U_STRING_INIT(name_unknown_value,   "_unknown_value", 15);
    U_STRING_INIT(name_na_value,        "_na_value", 10);
    U_STRING_INIT(name_sq_string,       "_sq_string", 11);
    U_STRING_INIT(name_dq_string,       "_dq_string", 11);
    U_STRING_INIT(name_sq3_string,      "_sq3_string", 12);
    U_STRING_INIT(name_dq3_string,      "_dq3_string", 12);
    U_STRING_INIT(name_text_string,     "_text_string", 13);
    U_STRING_INIT(name_numb_plain,      "_numb_plain", 12);
    U_STRING_INIT(name_numb_su,         "_numb_su", 9);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* Create the temp file */
    cif_file = tmpfile();
    /* cif_file = fopen("../write_simple.cif", "wb+"); */
    TEST(cif_file == NULL, 0, test_name, 1);

    /* Build the CIF data to test on */
    TEST(cif_create(&cif), CIF_OK, test_name, 2);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 3);

    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 4);
    TEST(cif_container_set_value(block, name_unknown_value, value), CIF_OK, test_name, 5);

    TEST(cif_value_init(value, CIF_NA_KIND), CIF_OK, test_name, 6);
    TEST(cif_container_set_value(block, name_na_value, value), CIF_OK, test_name, 7);

    TEST(cif_value_copy_char(value, value_sq_string), CIF_OK, test_name, 8);
    TEST(cif_container_set_value(block, name_sq_string, value), CIF_OK, test_name, 9);

    TEST(cif_value_copy_char(value, value_dq_string), CIF_OK, test_name, 10);
    TEST(cif_container_set_value(block, name_dq_string, value), CIF_OK, test_name, 11);

    TEST(cif_value_copy_char(value, value_text_string), CIF_OK, test_name, 12);
    TEST(cif_container_set_value(block, name_text_string, value), CIF_OK, test_name, 13);

    TEST(cif_value_copy_char(value, value_sq3_string), CIF_OK, test_name, 14);
    TEST(cif_container_set_value(block, name_sq3_string, value), CIF_OK, test_name, 15);

    TEST(cif_value_copy_char(value, value_dq3_string), CIF_OK, test_name, 16);
    TEST(cif_container_set_value(block, name_dq3_string, value), CIF_OK, test_name, 17);

    TEST(cif_value_init_numb(value, 17.125, 0, 4, 5), CIF_OK, test_name, 18);
    TEST(cif_container_set_value(block, name_numb_plain, value), CIF_OK, test_name, 19);

    TEST(cif_value_autoinit_numb(value, 43.53e06, 0.17e05, 19), CIF_OK, test_name, 20);
    TEST(cif_container_set_value(block, name_numb_su, value), CIF_OK, test_name, 21);

    cif_value_free(value);
    cif_container_free(block);

    /* write to the temp file */
    TEST(cif_write(cif_file, NULL, cif), CIF_OK, test_name, 22);
    fflush(cif_file);

    /* parse the file */
    rewind(cif_file);
    TEST(cif_parse(cif_file, NULL, &cif_readback), CIF_OK, test_name, 23);

    /* make sure everything matches */
    TEST_NOT(assert_cifs_equal(cif, cif_readback), 0, test_name, 24);

    /* clean up */
    TEST(cif_destroy(cif_readback), CIF_OK, test_name, 25);
    TEST(cif_destroy(cif), CIF_OK, test_name, 26);
    fclose(cif_file); /* ignore any error here */

    return 0;
}
