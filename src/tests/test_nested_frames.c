/*
 * test_nested_frames.c
 *
 * Tests the CIF API's cif_block_create_frame() function as applied to creating
 * nested save frames.  The function is more broadly tested under its alias,
 * cif_block_create_frame().
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
    char test_name[80] = "test_nested_frames";
    cif_tp *cif = NULL;
    cif_block_tp *block = NULL;
    cif_frame_tp *frame = NULL;
    cif_frame_tp *frame2 = NULL;
    U_STRING_DECL(block_code, "block", 6);
    U_STRING_DECL(frame_code, "frame", 6);
    U_STRING_DECL(frame2_code, "frame2", 7);

    U_STRING_INIT(block_code, "block", 6);
    U_STRING_INIT(frame_code, "frame", 6);
    U_STRING_INIT(frame2_code, "frame2", 7);
    TESTHEADER(test_name);
    CREATE_CIF(test_name, cif);
    CREATE_BLOCK(test_name, cif, block_code, block);
    CREATE_FRAME(test_name, block, frame_code, frame);

    /* Test creating a nested frame */
    TEST(cif_container_create_frame(frame, frame2_code, &frame2), CIF_OK, test_name, 1);
    TEST(frame2 == NULL, 0, test_name, 2);

    /* no mechanism for checking the frame code */

    /* not under test: */
    cif_frame_free(frame2);
    frame2 = NULL;

    /* test retrieving the frame */
    TEST(cif_container_get_frame(frame, frame2_code, &frame2), CIF_OK, test_name, 3);
    TEST(frame2 == NULL, 0, test_name, 4);

    /* not under test: */
    cif_frame_free(frame2);
    frame2 = NULL;

    /* Verify that the frame was added only to the specified parent frame */
    TEST(cif_block_get_frame(block, frame2_code, &frame2), CIF_NOSUCH_FRAME, test_name, 5);

    /* Test creating a frame whose frame code is the same as the host block's block code */
    TEST(cif_container_create_frame(frame, frame_code, NULL), CIF_OK, test_name, 6);

    DESTROY_FRAME(test_name, frame);
    DESTROY_BLOCK(test_name, block);
    DESTROY_CIF(test_name, cif);

    return 0;
}

