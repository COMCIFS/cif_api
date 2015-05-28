/*
 * test_create_block1.c
 *
 * Tests general function of the CIF API's cif_create_block() and
 * cif_get_block() functions.
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
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_create_block1";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    U_STRING_DECL(block_code, "block", 6);
    UChar *code;

    U_STRING_INIT(block_code, "block", 6);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    TEST(cif_get_block(cif, block_code, &block), CIF_NOSUCH_BLOCK, test_name, 1);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 2);
    TEST(block == NULL, 0, test_name, 3);

    TEST(cif_container_get_code(block, &code), CIF_OK, test_name, 4);
    TEST(u_strcmp(block_code, code), 0, test_name, 5);
    free(code);

    /* not under test: */
    cif_block_free(block);
    block = NULL;

    TEST(cif_get_block(cif, block_code, &block), CIF_OK, test_name, 6);
    TEST(block == NULL, 0, test_name, 7);
    cif_block_free(block);

    DESTROY_CIF(test_name, cif);

    return 0;
}

