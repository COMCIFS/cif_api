/*
 * test_parse_simple_loops.c
 *
 * Tests parsing simple CIF 2.0 looped data.
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unicode/ustring.h>
#include "../cif.h"
#include "assert_value.h"
#include "test.h"

#define BUFFER_SIZE 512
#define NUM_ITEMS     3
int main(void) {
    char test_name[80] = "test_parse_simple_loops";
    char local_file_name[] = "simple_loops.cif";
    char file_name[BUFFER_SIZE];
    FILE * cif_file;
    cif_tp *cif = NULL;
    cif_block_tp **block_list = NULL;
    cif_block_tp *block = NULL;
    cif_loop_tp **all_loops = NULL;
    cif_loop_tp **loop_p = NULL;
    cif_loop_tp *loop = NULL;
    cif_loop_tp *scalar_loop = NULL;
    cif_pktitr_tp *iterator = NULL;
    cif_packet_tp *packet = NULL;
    cif_value_tp *value = NULL;
    cif_value_tp *value2 = NULL;
    cif_value_tp *value3 = NULL;
    UChar **name_list = NULL;
    UChar **name_p = NULL;
    UChar *ustr;
    size_t count;
    unsigned int packet_flags;
    double d;
    U_STRING_DECL(code_simple_loops, "simple_loops", 13);
    U_STRING_DECL(name_col1,         "_col1", 6);
    U_STRING_DECL(name_col2,         "_col2", 6);
    U_STRING_DECL(name_col3,         "_col3", 6);
    U_STRING_DECL(name_single,       "_single", 8);
    U_STRING_DECL(name_scalar_a,     "_scalar_a", 10);
    U_STRING_DECL(name_scalar_b,     "_scalar_b", 10);
    U_STRING_DECL(value_v1,          "v1", 3);
    U_STRING_DECL(value_v2,          "v2", 3);
    U_STRING_DECL(value_v3,          "v3", 3);
    U_STRING_DECL(value_a,           "a", 2);
    U_STRING_DECL(value_b,           "b", 2);

    U_STRING_INIT(code_simple_loops, "simple_loops", 13);
    U_STRING_INIT(name_col1,         "_col1", 6);
    U_STRING_INIT(name_col2,         "_col2", 6);
    U_STRING_INIT(name_col3,         "_col3", 6);
    U_STRING_INIT(name_single,       "_single", 8);
    U_STRING_INIT(name_scalar_a,     "_scalar_a", 10);
    U_STRING_INIT(name_scalar_b,     "_scalar_b", 10);
    U_STRING_INIT(value_v1,          "v1", 3);
    U_STRING_INIT(value_v2,          "v2", 3);
    U_STRING_INIT(value_v3,          "v3", 3);
    U_STRING_INIT(value_a,           "a", 2);
    U_STRING_INIT(value_b,           "b", 2);

    /* Initialize data and prepare the test fixture */
    TESTHEADER(test_name);

    /* construct the test file name and open the file */
    RESOLVE_DATADIR(file_name, BUFFER_SIZE - strlen(local_file_name));
    TEST_NOT(file_name[0], 0, test_name, 1);
    strcat(file_name, local_file_name);
    cif_file = fopen(file_name, "rb");
    TEST(cif_file == NULL, 0, test_name, 2);

    /* parse the file */
    TEST(cif_parse(cif_file, NULL, &cif), CIF_OK, test_name, 3);

    /* check the parse result */
      /* check that there is exactly one block, and that it has the expected code */
    TEST(cif_get_all_blocks(cif, &block_list), CIF_OK, test_name, 4);
    TEST((*block_list == NULL), 0, test_name, 5);
    TEST_NOT((*(block_list + 1) == NULL), 0, test_name, 6);
    block = *block_list;
    TEST(cif_container_get_code(block, &ustr), CIF_OK, test_name, 7);
    TEST(u_strcmp(code_simple_loops, ustr), 0, test_name, 8);
    free(ustr);

      /* count the loops and check their categories */
    TEST(cif_container_get_all_loops(block, &all_loops), CIF_OK, test_name, 9);
    count = 0;
    for (loop_p = all_loops; *loop_p; loop_p += 1) {
        TEST(cif_loop_get_category(*loop_p, &ustr), CIF_OK, test_name, 2 * count + 10);
        if (ustr) {
            /* must be the scalar loop */
            TEST_NOT(*ustr == 0, 0, test_name, 2 * count + 11);
            scalar_loop = *loop_p;
            free(ustr);
        }
        cif_loop_free(*loop_p);
        count += 1;
    }

    if (scalar_loop) {
        /* This is not the expected case, but it is a consistent one */
        TEST_NOT(count == 4, 0, test_name, 2 * count + 9);  /* test number 17 if it passes */
        TEST(cif_loop_get_names(scalar_loop, &name_list), CIF_OK, test_name, 18);
        TEST_NOT(*name_list == NULL, 0, test_name, 19);
        free(name_list);
    } else {
        /* This is the expected case */
        TEST_NOT(count == 3, 0, test_name, 2 * count + 9);  /* test number 15 if it passes */
        /* skip two test numbers */
    }
    free(all_loops);

      /* check block contents: first loop */
    TEST(cif_container_get_item_loop(block, name_col1, &loop), CIF_OK, test_name, 20);
        /* check the number of names in the loop */
    TEST(cif_loop_get_names(loop, &name_list), CIF_OK, test_name, 21);
    for (name_p = name_list; *name_p; name_p += 1) {
        free(*name_p);
    }
    TEST_NOT((name_p - name_list) == 3, 0, test_name, 22);
    free(name_list);

        /* check the packets */
    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 23);

    count = 0;
    packet_flags = 0;
    #define TESTS_PER_IT 18
    while (1) {
        int index;

        TEST(cif_pktitr_next_packet(iterator, &packet), (count < 3) ? CIF_OK : CIF_FINISHED,
                test_name, 24 + count * TESTS_PER_IT);
        if (++count > 3) {
            break;
        }
        /* the order in which packets are iterated is not defined by the API (though insertion order is typical) */
        value = NULL;
        value2 = NULL;
        value3 = NULL;
        TEST(cif_packet_get_item(packet, name_col1, &value), CIF_OK, test_name, 25 + count * TESTS_PER_IT);
        TEST(cif_packet_get_item(packet, name_col2, &value2), CIF_OK, test_name, 26 + count * TESTS_PER_IT);
        TEST(cif_packet_get_item(packet, name_col3, &value3), CIF_OK, test_name, 27 + count * TESTS_PER_IT);
        /* TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 28 + count * TESTS_PER_IT); */
        TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 29 + count * TESTS_PER_IT);
        TEST_NOT(d == 0.0, 0, test_name,  30 + count * TESTS_PER_IT);
        TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 31 + count * TESTS_PER_IT);
        TEST_NOT(d == (index = (int) d), 0, test_name, 32 + count * TESTS_PER_IT);
        TEST(cif_value_kind(value2), CIF_CHAR_KIND, test_name, 33 + count * TESTS_PER_IT);
        TEST(cif_value_get_text(value2, &ustr), CIF_OK, test_name, 34 + count * TESTS_PER_IT);
        switch (index) {
            case 1:
                TEST(packet_flags & (1 << index), 0, test_name, 35 + count * TESTS_PER_IT);
                TEST(u_strcmp(value_v1, ustr), 0, test_name,  36 + count * TESTS_PER_IT);
                TEST(cif_value_kind(value3), CIF_UNK_KIND, test_name,  37 + count * TESTS_PER_IT);
                break;
            case 2:
                TEST(packet_flags & (1 << index), 0, test_name, 35 + count * TESTS_PER_IT);
                TEST(u_strcmp(value_v2, ustr), 0, test_name,  36 + count * TESTS_PER_IT);
                /* TEST(cif_value_kind(value3), CIF_NUMB_KIND, test_name,  37 + count * TESTS_PER_IT); */
                TEST(cif_value_get_su(value3, &d), CIF_OK, test_name, 38 + count * TESTS_PER_IT);
                TEST_NOT(d == 0.0, 0, test_name,  39 + count * TESTS_PER_IT);
                TEST(cif_value_get_number(value3, &d), CIF_OK, test_name, 40 + count * TESTS_PER_IT);
                TEST_NOT(d == 1.0, 0, test_name, 41 + count * TESTS_PER_IT);
                break;
            case 3:
                TEST(packet_flags & (1 << index), 0, test_name, 35 + count * TESTS_PER_IT);
                TEST(u_strcmp(value_v3, ustr), 0, test_name,  36 + count * TESTS_PER_IT);
                /* TEST(cif_value_kind(value3), CIF_NUMB_KIND, test_name,  37 + count * TESTS_PER_IT); */
                TEST(cif_value_get_su(value3, &d), CIF_OK, test_name, 38 + count * TESTS_PER_IT);
                TEST_NOT(abs(d - 0.2) < 1e-6, 0, test_name,  39 + count * TESTS_PER_IT);
                TEST(cif_value_get_number(value3, &d), CIF_OK, test_name, 40 + count * TESTS_PER_IT);
                TEST_NOT(d == 12.5, 0, test_name, 41 + count * TESTS_PER_IT);
                break;
            default:
                FAIL(36 + TESTS_PER_IT, test_name, index, "!=", 1);
        }
        packet_flags |= (1 << index);
    }

    /* next test is 25 + 3 * TESTS_PER_IT == 79 */
    TEST(packet_flags, 0xe, test_name, 79);
    cif_packet_free(packet);
    packet = NULL;
    TEST(cif_pktitr_abort(iterator), CIF_OK, test_name, 80);
    cif_loop_free(loop);

      /* check block contents: second loop */
    TEST(cif_container_get_item_loop(block, name_single, &loop), CIF_OK, test_name, 81);
        /* check the number of names in the loop */
    TEST(cif_loop_get_names(loop, &name_list), CIF_OK, test_name, 82);
    for (name_p = name_list; *name_p; name_p += 1) {
        free(*name_p);
    }
    TEST_NOT((name_p - name_list) == 1, 0, test_name, 83);
    free(name_list);

    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 84);
    count = 0;
    packet_flags = 0;
#define TESTS_PER_IT2 8
    while (1) {
        int index;

        TEST(cif_pktitr_next_packet(iterator, &packet), (count < 3) ? CIF_OK : CIF_FINISHED,
                test_name, 85 + count * TESTS_PER_IT2);
        if (++count > 3) {
            break;
        }
        /* the order in which packets are iterated is not defined by the API (though insertion order is typical) */
        value = NULL;
        TEST(cif_packet_get_item(packet, name_single, &value), CIF_OK, test_name, 86 + count * TESTS_PER_IT2);
        /* TEST(cif_value_kind(value), CIF_NUMB_KIND, test_name, 87 + count * TESTS_PER_IT2); */
        TEST(cif_value_get_su(value, &d), CIF_OK, test_name, 88 + count * TESTS_PER_IT2);
        TEST_NOT(d == 0.0, 0, test_name,  89 + count * TESTS_PER_IT2);
        TEST(cif_value_get_number(value, &d), CIF_OK, test_name, 90 + count * TESTS_PER_IT2);
        TEST_NOT(d == (index = (int) d), 0, test_name, 91 + count * TESTS_PER_IT2);
        TEST(packet_flags & (1 << index), 0, test_name, 92 + count * TESTS_PER_IT2);
        packet_flags |= (1 << index);
    }

        /* next test is 86 + 3 * TESTS_PER_IT2 == 110 */
    TEST(packet_flags, 0xe, test_name, 110);
    cif_packet_free(packet);
    packet = NULL;
    TEST(cif_pktitr_abort(iterator), CIF_OK, test_name, 111);
    cif_loop_free(loop);

      /* check block contents: third loop */
    TEST(cif_container_get_item_loop(block, name_scalar_a, &loop), CIF_OK, test_name, 112);
        /* check the number of names in the loop */
    TEST(cif_loop_get_names(loop, &name_list), CIF_OK, test_name, 113);
    for (name_p = name_list; *name_p; name_p += 1) {
        free(*name_p);
    }
    TEST_NOT((name_p - name_list) == 2, 0, test_name, 114);
    free(name_list);

    TEST(cif_loop_get_packets(loop, &iterator), CIF_OK, test_name, 115);
    TEST(cif_pktitr_next_packet(iterator, &packet), CIF_OK, test_name, 116);
    value = NULL;
    TEST(cif_packet_get_item(packet, name_scalar_a, &value), CIF_OK, test_name, 117);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 118);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 119);
    TEST(u_strcmp(value_a, ustr), 0, test_name, 120);
    free(ustr);
    TEST(cif_packet_get_item(packet, name_scalar_b, &value), CIF_OK, test_name, 121);
    TEST(cif_value_kind(value), CIF_CHAR_KIND, test_name, 122);
    TEST(cif_value_get_text(value, &ustr), CIF_OK, test_name, 123);
    TEST(u_strcmp(value_b, ustr), 0, test_name, 124);
    free(ustr);
    cif_packet_free(packet);
    TEST(cif_pktitr_next_packet(iterator, NULL), CIF_FINISHED, test_name, 125);
    TEST(cif_pktitr_abort(iterator), CIF_OK, test_name, 126);
    cif_loop_free(loop);

    /* clean up */
    cif_block_free(block);

    DESTROY_CIF(test_name, cif);
    fclose(cif_file);  /* ignore any failure here */

    return 0;
}
