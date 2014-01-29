/*
 * test_block_get_frame.c
 *
 * Tests operation of the CIF API's cif_block_get_frame() function
 *
 * Copyright (C) 2013, 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_block_get_frame";
    cif_t *cif = NULL;
    cif_block_t *block;
    cif_frame_t *frame;
#define NUM_PAIRS 3
    char code_pairs[NUM_PAIRS][2][64] = {
        {"Frame", "frAme"},
        {"fraME", "FRAME"},
        {"me\\u0300\\u00df\\u00dd", "m\\u00C8sS\\u00fd"}
    };
    UChar buffer[CIF_LINE_LENGTH];
    int counter;
    U_STRING_DECL(block_code, "block", 6);

    U_STRING_INIT(block_code, "block", 6);

    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);

    for (counter = 0; counter < NUM_PAIRS; counter++) {
        frame = NULL;
        TEST(
            cif_block_get_frame(block, TO_UNICODE(code_pairs[counter][1], buffer, CIF_LINE_LENGTH), &frame),
            CIF_NOSUCH_FRAME,
            test_name,
            HARD_FAIL
            );
        TEST(frame != NULL, 0, test_name, 3 * counter + 1);
        TEST(
            cif_block_create_frame(block, TO_UNICODE(code_pairs[counter][0], buffer, CIF_LINE_LENGTH), &frame),
            CIF_OK,
            test_name,
            HARD_FAIL
            );
        /* no mechanism for checking the frame code */
        cif_container_free(frame);
        frame = NULL;
        TEST(
            cif_block_get_frame(block, TO_UNICODE(code_pairs[counter][1], buffer, CIF_LINE_LENGTH), &frame),
            CIF_OK,
            test_name,
            3 * counter + 2
            );
        TEST(frame == NULL, 0, test_name, 3 * counter + 3);
        cif_frame_destroy(frame);
    }

    TEST(
        cif_block_get_frame(block, block_code, NULL),
        CIF_NOSUCH_FRAME,
        test_name,
        3 * NUM_PAIRS + 1
        );

    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

