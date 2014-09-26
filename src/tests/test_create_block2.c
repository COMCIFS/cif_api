/*
 * test_create_block2.c
 *
 * Tests error behaviors of the CIF API's cif_create_block() function.
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_create_block2";
    cif_t *cif = NULL;
#define NUM_PATTERNS 8
    char code_patterns[NUM_PATTERNS][64] = {
        "",
        "block with spaces",
        "block\001with\002noprint",
        "unpaired_hs_\\ud800foo",
        "unpaired_ls_\\udc42foo",
        "swapped_surrogates_\\udc17\\ud801foo",
        "low_\\ufffe_notchar",
        "high_\\udaff\\udfff_notchar",
    };
#define NUM_PAIRS 4
    char code_pairs[NUM_PAIRS][2][64] = {
      { "dupe", "DUpe" },
      { "\\u00c5ngstrom", "\\u00e5ngstrom" },
/*
 * NOTE: this pair should normalize to equal strings, but some versions of ICU
 * don't case-fold it properly.  Version 3.6, in particular, fails to fold
 * U+1E9E to to "ss", the full case-folding result specified in Unicode's
 * CaseFolding.txt table.  This is perhaps reflective of the version of Unicode
 * supported.
 *    { "v\\u00ca\\u0338\\u0328ry_Me\\u00dfy", "Ve\\u0328\\u0338\\u0302ry_me\\u1e9ey" },
 */
      { "v\\u00ca\\u0338\\u0328ry_Me\\u00dfy", "Ve\\u0328\\u0338\\u0302ry_mesSy" },
      { "\\u039daSt\\u1fc2", "\\u03bdast\\u0397\\u0345\\u0300" },
    };
    UChar buffer[CIF_LINE_LENGTH];
    int counter;
    cif_block_t *block;

#define TO_U(s) TO_UNICODE((s), buffer, CIF_LINE_LENGTH)

    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);

    for (counter = 0; counter < NUM_PATTERNS; counter++) {
        TEST(
            cif_create_block(cif, TO_U(code_patterns[counter]), NULL),
            CIF_INVALID_BLOCKCODE,
            test_name,
            counter + 1);
    }

    for (counter = 0; counter < NUM_PAIRS; counter++) {
        TEST(
            cif_create_block(cif, TO_U(code_pairs[counter][0]), &block),
            CIF_OK,
            test_name,
            2 * counter + NUM_PATTERNS + 1);
        TEST(
            cif_create_block(cif, TO_U(code_pairs[counter][1]), NULL),
            CIF_DUP_BLOCKCODE,
            test_name,
            2 * counter + NUM_PATTERNS + 2);
        if (cif_container_destroy(block) != CIF_OK) return HARD_FAIL;
    }

    for (counter = 0; counter < CIF_LINE_LENGTH; counter++) buffer[counter] = (UChar) 'a';
    buffer[CIF_LINE_LENGTH - 4] = (UChar) 0;

    TEST(cif_create_block(cif, buffer, NULL), CIF_INVALID_BLOCKCODE, test_name, NUM_PATTERNS + 2 * NUM_PAIRS + 1);

    DESTROY_CIF(test_name, cif);

    return 0;
}

