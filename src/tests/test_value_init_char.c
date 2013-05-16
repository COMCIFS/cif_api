/*
 * test_value_init_char.c
 *
 * Tests the CIF API's cif_value_init_char() function
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_value_init_char";
    cif_value_t *value;
    UChar buffer[240];
    UChar *init_text;
    UChar *text;

    TESTHEADER(test_name);

    TO_UNICODE("\\nSome text.\\n"
            "  With multiple lines and U\\u0308nicode characters \\tfrom various planes (\\U0001F649: \\U0010DEAF).\\n"
            "  There's also a hyphen or two, and \"quoted\" text.", buffer, 240);

    init_text = (UChar *) malloc((u_strlen(buffer) + 1) * sizeof(UChar));
    u_strcpy(init_text, buffer);

    /* Start with a value of kind CIF_UNK_KIND */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);

    /* reinitialize the value as kind CHAR */
    TEST(cif_value_init_char(value, init_text), CIF_OK, test_name, 3);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 4);

    /* check that the value carries a dependent reference to the initialization text */
    *init_text = (UChar) 'X';
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 6);
    TEST((text == NULL), 0, test_name, 7);
    TEST(u_strcmp(text, init_text), 0, test_name, 8);

    cif_value_free(value);

    return 0;
}

