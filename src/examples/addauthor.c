/*
 * addauthor.c
 *
 * A toy program to add author information to a CIF.  The main purpose of this
 * program is to demonstrate CIF output, but you would probably want to
 * use a different approach to the problem in the real world because this one
 * loses all formatting and comments (they are not semantically significant).
 *
 * The input CIF is read from stdin, the result is written to stdout.
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unicode/ustdio.h>
#include "cif.h"

/*
 * A macro to check the truthiness of a value (generally a function return
 * code), exiting the program with an error message if the value is falsey.
 *
 * f: the value to check; typically a function call
 * m: a (C) string describing the action represented by f, for use in an
 *    error message if needed
 */
#define CHECK_CALL(f, m) do { \
    int result_ = (f); \
    if (result_ != CIF_OK) { \
        fprintf(stderr, "Failed to %s, returning code %d.\n", (m), result_); \
        exit(1); \
    } \
} while (0)

int main(int argc, char *argv[]) {
    struct cif_parse_opts_s *parse_opts;
    struct cif_write_opts_s *write_opts;
    cif_tp *cif = NULL;          /* it's important to initialize this pointer to NULL */
    cif_block_tp **all_blocks;
    cif_block_tp **current_block;
    cif_packet_tp *packet;
    cif_value_tp *value = NULL;  /* it's important to initialize this pointer to NULL */
    /*
     * NOTE: this sort of initialization works (only) as long as the source
     * character encoding is congruent with Unicode for the characters involved.
     */
    UChar authorname_name[]
            = { '_', 'p', 'u', 'b', 'l', '_', 'a', 'u', 't', 'h', 'o', 'r', '_', 'n', 'a', 'm', 'e', 0 };
    UChar *loop_names[2] = { NULL, NULL };
    UChar *name;
    UChar *ustr;

    /* handle command-line arguments */
    if (argc < 2) {
        fprintf(stderr, "warning: no author name specified\n");
        return 0;
    } else {
        CHECK_CALL(cif_cstr_to_ustr(argv[1], -1, &name), "convert the given name to an internal format");
    }

    loop_names[0] = authorname_name;

    CHECK_CALL(cif_parse_options_create(&parse_opts), "prepare parse options");
    CHECK_CALL(cif_write_options_create(&write_opts), "prepare write options");

    /* Parse CIF from the standard input */
    CHECK_CALL(cif_parse(stdin, parse_opts, &cif), "parse the input CIF");
    free(parse_opts);

    CHECK_CALL(cif_get_all_blocks(cif, &all_blocks), "retrieve data blocks");
    CHECK_CALL(cif_packet_create(&packet, NULL), "create a packet object");
    CHECK_CALL(cif_packet_set_item(packet, authorname_name, NULL), "create an author name element in a packet");
    CHECK_CALL(cif_packet_get_item(packet, authorname_name, &value), "retrieve the author name element");
    /* responsibility for 'value' remains with the packet */
    CHECK_CALL(cif_value_init_char(value, name), "Set the name in the packet");
    /* responsibility for 'name' passes to the value, which in turn belongs to the packet */

    /* add the specified author to each block */
    for (current_block = all_blocks; *current_block; current_block += 1) {
        cif_loop_tp *author_loop;
        int result = cif_container_get_item_loop(*current_block, authorname_name, &author_loop);

        switch (result) {
            default:
                CHECK_CALL(result, "retrieve the _publ_author_* loop");  /* will exit() */
                break;
            case CIF_NOSUCH_ITEM:
                /* create an author loop */
                CHECK_CALL(cif_container_create_loop(*current_block, NULL, loop_names, &author_loop),
                        "create an author loop");
                break;
            case CIF_OK:
                CHECK_CALL(cif_loop_get_category(author_loop, &ustr), "determine a loop's category");
                if (ustr && !u_strcmp(ustr, CIF_SCALARS)) {
                    fprintf(stderr, "Error: _publ_author_name is present as a scalar\n");
                    exit(1);
                }
                free(ustr);
                break;
        }

        /* add a packet to the loop */
        CHECK_CALL(cif_loop_add_packet(author_loop, packet), "add a packet to the author loop");

        /* clean up */
        cif_loop_free(author_loop);
        cif_block_free(*current_block);
        /* don't clean up the packet just yet -- it may be re-used */
    }
    free(all_blocks);

    /* no more need for the packet */
    cif_packet_free(packet);

    /* successfully updated the CIF; now write it out (all of the foregoing just for this) */
    CHECK_CALL(cif_write(stdout, write_opts, cif), "output the modified CIF");

    /* final cleanup */
    free(write_opts);
    CHECK_CALL(cif_destroy(cif), "clean up the in-memory CIF");

    return 0;
}
