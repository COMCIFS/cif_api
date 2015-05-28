/*
 * table1.c
 *
 * An example program to extract the data for a simple crystal data table
 * from a core CIF (v1.1 or v2.0 format) read from the standard input, and
 * to format it as XHTML (UTF-8) on the standard output.  This program
 * silently ignores all CIF syntax and parsing errors, recovering as best
 * it can if any are encountered.
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
#include "messages.h"

/*
 * A macro to check a value (generally a function return value), exiting
 * the program with an error message if the value is anything other than
 * CIF_OK.
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

static const UChar uchar_nul = 0;

static UFILE *ustderr;

/*
 * Initializes a parse options structure according to the specified
 * command-line arguments.  Returns an index into the argv array one greater
 * than that of the last option argument (which is the index of the first
 * non-option argument if there are any).
 */
static int set_options(struct cif_parse_opts_s *options, int argc, char *argv[]) {
    options->error_callback = cif_parse_error_ignore;

    /* in the future, additional options may be set according to command-line arguments */
    return 1;
}

/*
 * Prints the fixed beginning of an XHTML document
 */
static void print_header(UFILE *out) {
    u_fprintf(out,
            "<?xml version='1.1' encoding='UTF-8'?>\n<xhtml>\n<head/>\n<body>\n<h1>Chemical and Crystal Data</h1>\n");
}

/*
 * Prints a row of the data table for the specified block, given a heading and the corresponding data name.
 * If no such datum is present then the row is skipped.  Values of types other than NUMB or CHAR are output as
 * empty strings.
 */
static void print_simple_row(UFILE *out, cif_block_tp *block, UChar *heading, UChar *data_name) {
    /* retrieve the value, if any, for the specified item */
    cif_value_tp *value = NULL; /* it's important to initialize this pointer to NULL */
    int result = cif_container_get_value(block, data_name, &value);
    UChar *value_text = NULL;

    switch (result) {
        case CIF_AMBIGUOUS_ITEM:
            /* the item is (unexpectedly) in a multi-packet loop; use an unspecified one of its values */
            u_fprintf(ustderr, "Warning: using just one of multiple values available for item '%S'\n", data_name);
            u_fflush(ustderr);
            /* fall through */
        case CIF_OK:
            switch (cif_value_kind(value)) {
                case CIF_LIST_KIND:
                case CIF_TABLE_KIND:
                    /* list and table values are recognized but only minimally supported by this function */
                    u_fprintf(ustderr, "Warning: unexpected composite value for item '%S'\n", data_name);
                    u_fflush(ustderr);
                    break;
                default:
                    result = cif_value_get_text(value, &value_text);
                    if (result != CIF_OK) {
                        u_fprintf(ustderr,
                                "Error: Failed to retrieve text for the value of item %S, returning code %d.\n",
                                data_name, result);
                        exit(1);
                    }
                    break;
            }
            break;
        case CIF_NOSUCH_ITEM:
            /* the item is not present; skip the row */
            return;
        default:
            u_fprintf(ustderr, "Error (%d) while attempting to retrieve item '%S'\n", result, data_name);
            exit(1);
    }

    /* output the row */
    u_fprintf(out, "  <row><th>%S</th><td>%S</td></row>\n", heading, (value_text ? value_text : &uchar_nul));

    /* clean up */
    if (value_text) {
        free(value_text);
    }
    cif_value_free(value);
}

static void print_size_row(UFILE *out, cif_block_tp *block) {
    cif_value_tp *values[3] = { NULL, NULL, NULL }; /* it's important to initialize these pointers to NULL */
    UChar datanames[3][32] = {
            { '_', 'e', 'x', 'p', 't', 'l', '_', 'c', 'r', 'y', 's', 't', 'a', 'l', '_', 's', 'i', 'z', 'e', '_', 'm', 'a', 'x', 0 },
            { '_', 'e', 'x', 'p', 't', 'l', '_', 'c', 'r', 'y', 's', 't', 'a', 'l', '_', 's', 'i', 'z', 'e', '_', 'm', 'i', 'd', 0 },
            { '_', 'e', 'x', 'p', 't', 'l', '_', 'c', 'r', 'y', 's', 't', 'a', 'l', '_', 's', 'i', 'z', 'e', '_', 'm', 'i', 'n', 0 },
    };
    double numbers[3];
    int counter;

    /* retrieve the three dimensions from the CIF; abort if any is unavailable */
    for (counter = 0; counter < 3; counter += 1) {
        int result = cif_container_get_value(block, datanames[counter], values + counter);

        switch (result) {
            case CIF_AMBIGUOUS_ITEM:
                /* the item is (unexpectedly) in a multi-packet loop; use an unspecified one of its values */
                u_fprintf(ustderr, "Warning: using just one of multiple values available for item '%S'\n",
                        datanames[counter]);
                u_fflush(ustderr);
                /* fall through */
            case CIF_OK:
                switch (cif_value_kind(values[counter])) {
                    case CIF_NUMB_KIND:
                        result = cif_value_get_number(values[counter], numbers + counter);
                        if (result != CIF_OK) {
                            u_fprintf(ustderr,
                                    "Error: Failed to retrieve a numeric value for item %S, returning code %d.\n",
                                    datanames[counter], result);
                            exit(1);
                        }
                        break;
                    default:
                        /* the value is expected to be numeric; abort */
                        u_fprintf(ustderr, "Warning: non-numeric value for item '%S'\n", datanames[counter]);
                        u_fflush(ustderr);
                        goto cleanup;
                }
                break;
            case CIF_NOSUCH_ITEM:
                /* the item is not present; abort */
                goto cleanup;
            default:
                u_fprintf(ustderr, "Error (%d) while attempting to retrieve item '%S'\n", result, datanames[counter]);
                exit(1);
        }
    }

    /* output the row */
    u_fprintf(out, "  <row><th>Crystal size</th><td>%.2lf x %.2lf x %.2lf</td></row>\n", numbers[0], numbers[1], numbers[2]);

    cleanup:
    for (counter = 0; (counter < 3) && values[counter]; counter += 1) {
        cif_value_free(values[counter]);
    }
}

/*
 * Prints an XHTML heading and data table for the given CIF data block
 */
#define BUFSIZE 80
static void print_table1(UFILE *out, cif_block_tp *block) {
    UChar *code;
    UChar heading_buffer[BUFSIZE];
    UChar dataname_buffer[BUFSIZE];
    struct pair {
        const char *heading;
        const char *dataname;
    };
    struct pair part1[] = {
            { "Chemical formula",       "_chemical_formula_sum" },
            { "Formula weight",         "_chemical_formula_weight" },
            { "Temperature",            "_diffrn_ambient_temperature" },
            { "Crystal color",          "_exptl_crystal_colour" },
            { "Crystal description",    "_exptl_crystal_description"},
            { NULL, NULL }
    };
    struct pair part2[] = {
            { "Crystal system",         "_space_group_crystal_system" },
            { "Space group",            "_space_group_name_H-M_alt"},
            { "a",                      "_cell_length_a" },
            { "b",                      "_cell_length_b" },
            { "c",                      "_cell_length_c" },
            { "alpha",                  "_cell_angle_alpha" },
            { "beta",                   "_cell_angle_beta" },
            { "gamma",                  "_cell_angle_gamma" },
            { "Volume",                 "_cell_volume" },
            { "Z",                      "_cell_formula_units_Z" },
            { "Density (calculated)",   "_exptl_crystal_density_diffrn" },
            { "Absorption coefficient", "_exptl_absorpt_coefficient_mu" },
            { "F(000)",                 "_exptl_crystal_F_000" },
            { NULL, NULL }
    };
    struct pair *pair;
    UErrorCode error_code = U_ZERO_ERROR;

    CHECK_CALL(cif_container_get_code(block, &code), "retrieve a data block's code");
    u_fprintf(out, "<h2>Chemical and crystal data for %S</h2>\n<table>\n", code);
    free(code);

    for (pair = part1; pair->heading; pair += 1) {
        print_simple_row(out, block,
                u_strFromUTF8(heading_buffer, BUFSIZE, NULL, pair->heading, -1, &error_code),
                u_strFromUTF8(dataname_buffer, BUFSIZE, NULL, pair->dataname, -1, &error_code)
                );
    }

    print_size_row(out, block);

    for (pair = part2; pair->heading; pair += 1) {
        print_simple_row(out, block,
                u_strFromUTF8(heading_buffer, BUFSIZE, NULL, pair->heading, -1, &error_code),
                u_strFromUTF8(dataname_buffer, BUFSIZE, NULL, pair->dataname, -1, &error_code)
                );
    }

    u_fprintf(out, "</table>\n");
}

/*
 * Prints the fixed end of an XHTML document
 */
static void print_trailer(UFILE *out) {
    u_fprintf(out, "</body>\n</xhtml>\n");
}

/*
 * The main program
 */
int main(int argc, char *argv[]) {
    struct cif_parse_opts_s *options;
    cif_tp *cif = NULL;  /* it's important to initialize this pointer to NULL */
    int first_file;
    UFILE *ustdout;
    cif_block_tp **all_blocks;
    cif_block_tp **current_block;

    /* handle command-line arguments */
    CHECK_CALL(cif_parse_options_create(&options), "prepare parse options");
    first_file = set_options(options, argc, argv);

    /* Parse CIF from the standard input */
    CHECK_CALL(cif_parse(stdin, options, &cif), "parse the input CIF");
    free(options);

    ustderr = u_finit(stderr, NULL, NULL);
    if (ustderr == NULL) {
        fprintf(stderr, "Failed to open Unicode error stream.\n");
        exit(1);
    }

    /* prepare a Unicode output stream wrapping stdout */
    ustdout = u_finit(stdout, "en_US", "UTF_8");
    if (ustdout == NULL) {
        fprintf(stderr, "Failed to open Unicode output stream.\n");
        exit(1);
    }

    CHECK_CALL(cif_get_all_blocks(cif, &all_blocks), "retrieve data blocks");

    /* output the invariant file header */
    print_header(ustdout);

    /* output a data table for each block */
    for (current_block = all_blocks; *current_block; current_block += 1) {
        print_table1(ustdout, *current_block);
        cif_block_free(*current_block);
    }
    free(all_blocks);

    /* output the invariant file trailer */
    print_trailer(ustdout);

    /* clean up */
    u_fclose(ustdout);
    CHECK_CALL(cif_destroy(cif), "release in-memory CIF data");

    return 0;
}
