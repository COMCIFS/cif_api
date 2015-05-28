/*
 * test_get_block.c
 *
 * Tests some details of the CIF API's cif_get_block() function that are not
 * covered by the cif_create_block() tests.
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
    char test_name[80] = "test_get_block";
    cif_tp *cif = NULL;
    cif_block_tp *block;
#define NUM_PAIRS 4
    char code_pairs[NUM_PAIRS][2][64] = {
        {"Block", "bLOck"},
        {"BlocK", "BLOCK"},
        {"bLoCk", "block"},
        {"me\\u0300\\u00df\\u00dd", "m\\u00C8sS\\u00fd"}
    };
    UChar buffer[CIF_LINE_LENGTH];
    int counter;


    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    for (counter = 0; counter < NUM_PAIRS; counter++) {
        block = NULL;
        TEST(
            cif_get_block(cif, TO_UNICODE(code_pairs[counter][1], buffer, CIF_LINE_LENGTH), &block),
            CIF_NOSUCH_BLOCK,
            test_name,
            HARD_FAIL
            );
        TEST(block != NULL, 0, test_name, 3 * counter);
        TEST(
            cif_create_block(cif, TO_UNICODE(code_pairs[counter][0], buffer, CIF_LINE_LENGTH), &block),
            CIF_OK,
            test_name,
            HARD_FAIL
            );
        /* no mechanism for checking the block name */
        cif_container_free(block);
        block = NULL;
        TEST(
            cif_get_block(cif, TO_UNICODE(code_pairs[counter][1], buffer, CIF_LINE_LENGTH), &block),
            CIF_OK,
            test_name,
            3 * counter + 1
            );
        TEST(block == NULL, 0, test_name, 3 * counter + 3);
        TEST(cif_block_destroy(block), CIF_OK, test_name, 3 * counter + 2);
    }

    DESTROY_CIF(test_name, cif);

    return 0;
}

