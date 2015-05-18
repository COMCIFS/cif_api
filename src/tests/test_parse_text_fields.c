/*
 * test_parse_text_fields.c
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
#define NUM_ITEMS 8
int main(void) {
    char test_name[80] = "test_parse_text_fields";
    char local_file_name[] = "text_fields.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_value_tp *value = NULL;
    UChar *ustr;

    U_STRING_DECL(block_code,           "text_fields", 12);
    U_STRING_DECL(name_plain1,          "_plain1", 8);
    U_STRING_DECL(name_plain2,          "_plain2", 8);
    U_STRING_DECL(name_terminators,     "_terminators", 13);
    U_STRING_DECL(name_folded1,         "_folded1", 9);
    U_STRING_DECL(name_folded2,         "_folded2", 9);
    U_STRING_DECL(name_prefixed1,       "_prefixed1", 13);
    U_STRING_DECL(name_prefixed2,       "_prefixed2", 13);
    U_STRING_DECL(name_pfx_folded,      "_pfx_folded", 12);
    U_STRING_DECL(name_folded_empty,    "_folded_empty", 14);
    U_STRING_DECL(name_prefixed_empty,  "_prefixed_empty", 16);
    U_STRING_DECL(name_pfx_fold_empty,  "_pfx_fold_empty", 16);
    UChar value_plain1[] = {
        '\\', '\\', 0x0a,
        'l', 'i', 'n', 'e', ' ', '2', '\\', 0x0a,
        'l', 'i', 'n', 'e', ' ', '3', ' ', ' ', ' ', ' ', 0
    };
    UChar value_plain2[] = { ';', '\\', 0 };
    UChar value_terminators[] = {
        'l', 'i', 'n', 'e', ' ', '1', 0x0a,
        'l', 'i', 'n', 'e', ' ', '2', 0x0a,
        'l', 'i', 'n', 'e', ' ', '3', 0x0a,
        'e', 'n', 'd', 0
    };
    UChar value_folded1[] = {
        'A', ' ', '(', 'n', 'o', 't', ' ', 's', 'o', ')', ' ', 'l', 'o', 'n', 'g', ' ', 'l', 'i', 'n', 'e', '.', 0x0a,
        'A', ' ', 'n', 'o', 'r', 'm', 'a', 'l', ' ', 'l', 'i', 'n', 'e', '.', 0x0a,
        'N', 'O', 'T', ' ', 'a', ' ', 'l', 'o', 'n', 'g', ' ', 'l', 'i', 'n', 'e', '.', '\\', 0
    };
    UChar value_folded2[] = {
        'l', 'i', 'n', 'e', ' ', '1', ' ', ' ', 0x0a,
        'l', 'i', 'n', 'e', ' ', '2', 0
    };
    UChar value_prefixed[] = {
        '_', 'e', 'm', 'b', 'e', 'd', 'd', 'e', 'd', 0x0a,
        ';', 0x0a,
        ';', 0
    };
    UChar value_pfx_folded[] = {
        'l', 'i', 'n', 'e', ' ', '1', ' ', 'i', 's', ' ', 'f', 'o', 'l', 'd', 'e', 'd', ' ', 't', 'w', 'i', 'c', 'e',
        '.', 0
    };

    U_STRING_INIT(block_code,           "text_fields", 12);
    U_STRING_INIT(name_plain1,          "_plain1", 8);
    U_STRING_INIT(name_plain2,          "_plain2", 8);
    U_STRING_INIT(name_terminators,     "_terminators", 13);
    U_STRING_INIT(name_folded1,         "_folded1", 9);
    U_STRING_INIT(name_folded2,         "_folded2", 9);
    U_STRING_INIT(name_prefixed1,       "_prefixed1", 13);
    U_STRING_INIT(name_prefixed2,       "_prefixed2", 13);
    U_STRING_INIT(name_pfx_folded,      "_pfx_folded", 12);
    U_STRING_INIT(name_folded_empty,    "_folded_empty", 14);
    U_STRING_INIT(name_prefixed_empty,  "_prefixed_empty", 16);
    U_STRING_INIT(name_pfx_fold_empty,  "_pfx_fold_empty", 16);

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

    /* check the parsed text field values */

    TEST(cif_get_block(cif, block_code, &block), CIF_OK, test_name, 4);

    TEST(cif_container_get_value(block, name_plain1, &value), CIF_OK, test_name, 5);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 6);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 7);
    TEST(u_strcmp(ustr, value_plain1), 0, test_name, 8);
    free(ustr);
    TEST(cif_container_get_value(block, name_plain2, &value), CIF_OK, test_name, 9);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 10);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 11);
    TEST(u_strcmp(ustr, value_plain2), 0, test_name, 12);
    free(ustr);
    TEST(cif_container_get_value(block, name_terminators, &value), CIF_OK, test_name, 13);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 14);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 15);
    TEST(u_strcmp(ustr, value_terminators), 0, test_name, 16);
    free(ustr);
    TEST(cif_container_get_value(block, name_folded1, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 18);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 19);
    TEST(u_strcmp(ustr, value_folded1), 0, test_name, 20);
    free(ustr);
    TEST(cif_container_get_value(block, name_folded2, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 22);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 23);
    TEST(u_strcmp(ustr, value_folded2), 0, test_name, 24);
    free(ustr);
    TEST(cif_container_get_value(block, name_prefixed1, &value), CIF_OK, test_name, 25);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 26);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 27);
    TEST(u_strcmp(ustr, value_prefixed), 0, test_name, 28);
    free(ustr);
    TEST(cif_container_get_value(block, name_prefixed2, &value), CIF_OK, test_name, 29);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 30);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 31);
    TEST(u_strcmp(ustr, value_prefixed), 0, test_name, 32);
    free(ustr);
    TEST(cif_container_get_value(block, name_pfx_folded, &value), CIF_OK, test_name, 33);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 34);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 35);
    TEST(u_strcmp(ustr, value_pfx_folded), 0, test_name, 36);
    free(ustr);
    TEST(cif_container_get_value(block, name_folded_empty, &value), CIF_OK, test_name, 37);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 38);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 39);
    TEST(*ustr, 0, test_name, 40);
    free(ustr);
    TEST(cif_container_get_value(block, name_prefixed_empty, &value), CIF_OK, test_name, 41);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 42);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 43);
    TEST(*ustr, 0, test_name, 44);
    free(ustr);
    TEST(cif_container_get_value(block, name_pfx_fold_empty, &value), CIF_OK, test_name, 45);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 46);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 47);
    TEST(*ustr, 0, test_name, 48);
    free(ustr);

    /* clean up */
    cif_value_free(value);
    cif_block_free(block);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
