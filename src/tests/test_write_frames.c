/*
 * test_write_frames.c
 *
 * Tests writing CIFs containing save frames.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include "../cif.h"

#define USE_USTDERR
#include "assert_cifs.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     8
int main(void) {
    char test_name[80] = "test_write_frames";
    FILE * cif_file;
    cif_t *cif = NULL;
    cif_t *cif_readback = NULL;
    cif_block_t *block = NULL;
    cif_frame_t *frame1 = NULL;
    cif_frame_t *frame2 = NULL;
    cif_loop_t *loop = NULL;
    cif_packet_t *packet = NULL;
    cif_value_t *value = NULL;

    UChar frame1_code[] = { 'f', 'r', 'a', 'm', 'e', '1', 0 };
    UChar frame2_code[] = { 'f', 'r', 'a', 'm', 'e', '2', 0 };

    UChar names[][8]  = {
        { '_', 'k', 'e', 'y', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '1', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '2', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '3', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '4', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '5', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '6', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '7', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '8', 0 },
        { '_', 'v', 'a', 'l', 'u', 'e', '9', 0 }
    };

    UChar *item_names[4];

    UChar value_text[]  = { '"', '"', '"', ' ', 'a', 'n', 'd', ' ', '\'', '\'', '\'', '?', 0x0a,
            'O', 'o', 'p', 's', '.', 0 };
    UChar value_text2[]  = { '%', '\\', 0x0a, ' ', '#', 'n', 'o', 't', ' ', 'a', ' ',
            'c', 'o', 'm', 'm', 'e', 'n', 't', 0x0a, '"', '"', '"', '\'', '\'', '\'', 0 };
    UChar value_bksl1[] = { '\\', '\'', 'e', 0 };

    U_STRING_DECL(block_code,           "framed_data", 13);

    U_STRING_INIT(block_code,           "framed_data", 13);
    item_names[0] = names[0];
    item_names[1] = names[1];
    item_names[2] = names[2];
    item_names[3] = NULL;

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* Create the temp file */
    cif_file = tmpfile();
    /* cif_file = fopen("../write_frames.cif", "wb+"); */
    TEST(cif_file == NULL, 0, test_name, 1);

    /* Build the CIF data to test on */
    TEST(cif_create(&cif), CIF_OK, test_name, 2);
    TEST(cif_create_block(cif, block_code, &block), CIF_OK, test_name, 3);

    TEST(cif_container_create_loop(block, NULL, item_names, &loop), CIF_OK, test_name, 4);
    TEST(cif_packet_create(&packet, item_names), CIF_OK, test_name, 5);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 6);
    TEST(cif_packet_get_item(packet, item_names[1], &value), CIF_OK, test_name, 7);
    TEST(cif_value_autoinit_numb(value, 17.0, 1.0, 19), CIF_OK, test_name, 8);
    TEST(cif_packet_get_item(packet, item_names[2], &value), CIF_OK, test_name, 9);
    TEST(cif_value_init(value, CIF_NA_KIND), CIF_OK, test_name, 10);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 11);
    TEST(cif_value_copy_char(value, value_text2), CIF_OK, test_name, 12);
    TEST(cif_packet_get_item(packet, item_names[0], &value), CIF_OK, test_name, 13);
    TEST(cif_value_copy_char(value, value_text), CIF_OK, test_name, 14);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 15);
    cif_packet_free(packet);
    cif_loop_free(loop);
    loop = NULL;

    TEST(cif_container_create_frame(block, frame1_code, &frame1), CIF_OK, test_name, 16);
    TEST(cif_container_create_loop(frame1, NULL, item_names, &loop), CIF_OK, test_name, 17);
    TEST(cif_packet_create(&packet, item_names), CIF_OK, test_name, 18);
    TEST(cif_packet_get_item(packet, item_names[1], &value), CIF_OK, test_name, 19);
    TEST(cif_value_autoinit_numb(value, 21.0, 1.0, 19), CIF_OK, test_name, 20);
    TEST(cif_packet_get_item(packet, item_names[2], &value), CIF_OK, test_name, 21);
    TEST(cif_value_init(value, CIF_NA_KIND), CIF_OK, test_name, 22);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 23);
    TEST(cif_value_copy_char(value, value_bksl1), CIF_OK, test_name, 24);
    TEST(cif_packet_get_item(packet, item_names[0], &value), CIF_OK, test_name, 25);
    TEST(cif_value_init(value, CIF_UNK_KIND), CIF_OK, test_name, 26);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 27);
    cif_packet_free(packet);
    cif_loop_free(loop);
    loop = NULL;

    item_names[2] = names[5];
    TEST(cif_container_create_frame(block, frame2_code, &frame2), CIF_OK, test_name, 28);
    TEST(cif_container_create_loop(frame2, NULL, item_names, &loop), CIF_OK, test_name, 29);
    TEST(cif_packet_create(&packet, item_names), CIF_OK, test_name, 30);
    TEST(cif_packet_get_item(packet, item_names[0], &value), CIF_OK, test_name, 31);
    TEST(cif_value_autoinit_numb(value, 1.0, 1.0, 19), CIF_OK, test_name, 32);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 33);
    cif_packet_free(packet);
    cif_loop_free(loop);
    loop = NULL;

    item_names[0] = names[3];
    item_names[1] = names[4];
    item_names[2] = NULL;
    TEST(cif_container_create_loop(frame1, NULL, item_names, &loop), CIF_OK, test_name, 34);
    TEST(cif_packet_create(&packet, item_names), CIF_OK, test_name, 35);
    TEST(cif_packet_get_item(packet, item_names[0], &value), CIF_OK, test_name, 36);
    TEST(cif_value_copy_char(value, value_bksl1), CIF_OK, test_name, 37);
    TEST(cif_packet_get_item(packet, item_names[1], &value), CIF_OK, test_name, 38);
    TEST(cif_value_init_numb(value, 12.5, 0, 1, 5), CIF_OK, test_name, 39);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 40);
    TEST(cif_value_init_numb(value, 0.00033333, 0.0000002, 7, 1), CIF_OK, test_name, 41);
    TEST(cif_loop_add_packet(loop, packet), CIF_OK, test_name, 42);
    cif_packet_free(packet);
    cif_loop_free(loop);

    TEST(cif_value_create(CIF_UNK_KIND, &value), CIF_OK, test_name, 43);
    TEST(cif_container_set_value(frame2, names[5], value), CIF_OK, test_name, 44);
    TEST(cif_value_copy_char(value, names[1]), CIF_OK, test_name, 45);
    TEST(cif_container_set_value(block, names[5], value), CIF_OK, test_name, 46);
    cif_value_free(value);
    cif_container_free(frame2);
    cif_container_free(frame1);
    cif_container_free(block);

    /* write to the temp file */
    TEST(cif_write(cif_file, NULL, cif), CIF_OK, test_name, 47);
    fflush(cif_file);

    /* parse the file */
    rewind(cif_file);
    TEST(cif_parse(cif_file, NULL, &cif_readback), CIF_OK, test_name, 48);

    /* make sure everything matches */
    TEST_NOT(assert_cifs_equal(cif, cif_readback), 0, test_name, 49);

    /* clean up */
    TEST(cif_destroy(cif_readback), CIF_OK, test_name, 50);
    TEST(cif_destroy(cif), CIF_OK, test_name, 51);
    fclose(cif_file); /* ignore any error here */

    return 0;
}
