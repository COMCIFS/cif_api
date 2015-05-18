/*
 * test_parse_triple.c
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
    char test_name[80] = "test_parse_triple";
    char local_file_name[] = "triple.cif";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_value_tp *value = NULL;
    UChar *ustr;

    U_STRING_DECL(block_code,           "triple", 12);
    U_STRING_DECL(name_empty1,          "_empty1", 8);
    U_STRING_DECL(name_empty2,          "_empty2", 8);
    U_STRING_DECL(name_simple,          "_simple", 8);
    U_STRING_DECL(name_tricky1,         "_tricky1", 9);
    U_STRING_DECL(name_tricky2,         "_tricky2", 9);
    U_STRING_DECL(name_embedded,        "_embedded", 10);
    U_STRING_DECL(name_multiline1,      "_multiline1", 12);
    U_STRING_DECL(name_multiline2,      "_multiline2", 12);
    U_STRING_DECL(name_ml_embed,        "_ml_embed", 10);
    U_STRING_DECL(value_simple,         "simple", 7);
    U_STRING_DECL(value_tricky1,        "'tricky", 8);
    U_STRING_DECL(value_tricky2,        "\"\"tricky", 9);
    U_STRING_DECL(value_embedded,       "\"\"\"embedded\"\"\"", 15);
    U_STRING_DECL(value_multiline1,     "first line\nsecond line", 23);
    U_STRING_DECL(value_multiline2,     "\nsecond line [of 3]\n", 21);
    U_STRING_DECL(value_ml_embed,       "\n_not_a_name\n;embedded\n;\n", 26);

    U_STRING_INIT(block_code,           "triple", 12);
    U_STRING_INIT(name_empty1,          "_empty1", 8);
    U_STRING_INIT(name_empty2,          "_empty2", 8);
    U_STRING_INIT(name_simple,          "_simple", 8);
    U_STRING_INIT(name_tricky1,         "_tricky1", 9);
    U_STRING_INIT(name_tricky2,         "_tricky2", 9);
    U_STRING_INIT(name_embedded,        "_embedded", 10);
    U_STRING_INIT(name_multiline1,      "_multiline1", 12);
    U_STRING_INIT(name_multiline2,      "_multiline2", 12);
    U_STRING_INIT(name_ml_embed,        "_ml_embed", 10);
    U_STRING_INIT(value_simple,         "simple", 7);
    U_STRING_INIT(value_tricky1,        "'tricky", 8);
    U_STRING_INIT(value_tricky2,        "\"\"tricky", 9);
    U_STRING_INIT(value_embedded,       "\"\"\"embedded\"\"\"", 15);
    U_STRING_INIT(value_multiline1,     "first line\nsecond line", 23);
    U_STRING_INIT(value_multiline2,     "\nsecond line [of 3]\n", 21);
    U_STRING_INIT(value_ml_embed,       "\n_not_a_name\n;embedded\n;\n", 26);

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
      /* retrieve the expected data block */
    TEST(cif_get_block(cif, block_code, &block), CIF_OK, test_name, 4);

      /* check each expected item */
    TEST(cif_container_get_value(block, name_empty1, &value), CIF_OK, test_name, 5);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 6);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 7);
    TEST(*ustr, 0, test_name, 8);
    free(ustr);

    TEST(cif_container_get_value(block, name_empty2, &value), CIF_OK, test_name, 9);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 10);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 11);
    TEST(*ustr, 0, test_name, 12);
    free(ustr);

    TEST(cif_container_get_value(block, name_simple, &value), CIF_OK, test_name, 13);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 14);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 15);
    TEST(u_strcmp(value_simple, ustr), 0, test_name, 16);
    free(ustr);

    TEST(cif_container_get_value(block, name_tricky1, &value), CIF_OK, test_name, 17);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 18);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 19);
    TEST(u_strcmp(value_tricky1, ustr), 0, test_name, 20);
    free(ustr);

    TEST(cif_container_get_value(block, name_tricky2, &value), CIF_OK, test_name, 21);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 22);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 23);
    TEST(u_strcmp(value_tricky2, ustr), 0, test_name, 24);
    free(ustr);

    TEST(cif_container_get_value(block, name_embedded, &value), CIF_OK, test_name, 25);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 26);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 27);
    TEST(u_strcmp(value_embedded, ustr), 0, test_name, 28);
    free(ustr);

    TEST(cif_container_get_value(block, name_multiline1, &value), CIF_OK, test_name, 29);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 30);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 31);
    TEST(u_strcmp(value_multiline1, ustr), 0, test_name, 32);
    free(ustr);

    TEST(cif_container_get_value(block, name_multiline2, &value), CIF_OK, test_name, 33);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 34);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 35);
    TEST(u_strcmp(value_multiline2, ustr), 0, test_name, 36);
    free(ustr);

    TEST(cif_container_get_value(block, name_ml_embed, &value), CIF_OK, test_name, 37);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 38);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 39);
    TEST(u_strcmp(value_ml_embed, ustr), 0, test_name, 40);
    free(ustr);

    /* clean up */
    cif_value_free(value);
    cif_block_free(block);
    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
