/*
 * test_value_copy_char.c
 *
 * Tests the CIF API's cif_value_copy_char() function
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_value_copy_char";
    cif_value_t *value;
    UChar buffer[240];
    UChar *copy_text;
    UChar *text;
    UChar *unused;

    TESTHEADER(test_name);

    unused = TO_UNICODE("\\nSome text.\\n"
            "  With multiple lines and U\\u0308nicode characters \\tfrom various planes (\\U0001F649: \\U0010DEAF).\\n"
            "  There's also a hyphen or two, and \"quoted\" text.", buffer, 240);

    copy_text = (UChar *) malloc((u_strlen(buffer) + 1) * sizeof(UChar));
    u_strcpy(copy_text, buffer);

    /* Start with a value of kind CIF_UNK_KIND */ 
    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 1);
    TEST((value == NULL), 0, test_name, 2);

    /* reinitialize the value as kind CHAR */
    TEST(cif_value_copy_char(value, buffer), CIF_OK, test_name, 3);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 4);

    /* check that the value carries a dependent reference to the initialization text */
    buffer[0] = (UChar) 'X';
    TEST(cif_value_get_text(value, &text), CIF_OK, test_name, 6);
    TEST((text == NULL), 0, test_name, 7);
    TEST(u_strcmp(text, copy_text), 0, test_name, 8);

    cif_value_free(value);
    free(copy_text);

    return 0;
}

