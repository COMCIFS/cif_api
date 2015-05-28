/*
 * test_block_create_frame2.c
 *
 * Tests general function of the CIF API's cif_block_create_frame() function.
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
    char test_name[80] = "test_block_create_frame2";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_block_tp *block2 = NULL;
    cif_frame_tp *frame = NULL;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(block2_code, "block2", 7);

#define NUM_PATTERNS 9
    char code_patterns[NUM_PATTERNS][64] = {
        "",
        "frame_with\ttab",
        "frame_with_LF\x0a",
        "\011blockwith\032noprint",
        "unpaired_hs_\\uda01foo",
        "unpaired_ls_\\udf17foo",
        "swapped_surrogates_\\udc00\\udbfffoo",
        "low_\\uffff_notchar",
        "high_\\udbff\\udffe_notchar",
    };
#define NUM_PAIRS 2
    char code_pairs[NUM_PAIRS][2][64] = {
      { "dupe", "Dupe" },
      { "A\\u030angstr\\u00f6m", "\\u00e5ngstr\\u00d6m" }
    };
    UChar buffer[CIF_LINE_LENGTH];
    int counter;

#define TO_U(s) TO_UNICODE((s), buffer, CIF_LINE_LENGTH)

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(block2_code, "block2", 7);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_BLOCK(test_name, cif, block2_code, block2);

    /* Test creating frames with invalid frame codes */
    for (counter = 0; counter < NUM_PATTERNS; counter++) {
        TEST(
            cif_block_create_frame(block, TO_U(code_patterns[counter]), NULL),
            CIF_INVALID_FRAMECODE,
            test_name,
            counter + 1);
    }

    /* Test creating frames with duplicate codes in the same block and in different blocks */
    for (counter = 0; counter < NUM_PAIRS; counter++) {
        cif_frame_tp *frame2 = NULL;

        TEST(
            cif_block_create_frame(block, TO_U(code_pairs[counter][0]), &frame),
            CIF_OK,
            test_name,
            3 * counter + NUM_PATTERNS + 1);
        TEST(
            cif_block_create_frame(block, TO_U(code_pairs[counter][1]), NULL),
            CIF_DUP_FRAMECODE,
            test_name,
            3 * counter + NUM_PATTERNS + 2);
        TEST(
            cif_block_create_frame(block2, TO_U(code_pairs[counter][1]), &frame2),
            CIF_OK,
            test_name,
            3 * counter + NUM_PATTERNS + 3);
        if (cif_container_destroy(frame) != CIF_OK) return HARD_FAIL;
        if (cif_container_destroy(frame2) != CIF_OK) return HARD_FAIL;
    }

    /* Test a different variety of invalid frame code */
    for (counter = 0; counter < CIF_LINE_LENGTH; counter++) buffer[counter] = (UChar) 'b';
    buffer[CIF_LINE_LENGTH - 4] = (UChar) 0;
    TEST(cif_block_create_frame(block, buffer, NULL), CIF_INVALID_FRAMECODE, test_name,
            NUM_PATTERNS + 3 * NUM_PAIRS + 1);

    /* Test creating a frame nested in a frame */

    /* This should not fail because we already tested it */
    if (cif_block_create_frame(block, TO_U(code_pairs[0][0]), &frame) != CIF_OK) return HARD_FAIL;

    /* done */

    DESTROY_BLOCK(test_name, block2);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

