/*
 * test_value_copy_char.c
 *
 * Tests the CIF API's cif_value_copy_char() function
 *
 * Copyright 2014, 2015 John C. Bollinger
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
    char test_name[80] = "test_value_copy_char";
    cif_value_tp *value;
    UChar buffer[240];
    UChar *copy_text;
    UChar *text;

    TESTHEADER(test_name);

    TO_UNICODE("\\nSome text.\\n"
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

