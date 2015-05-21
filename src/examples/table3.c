/*
 * table3.c
 *
 * An example program to extract the data for a simple atomic coordinate table
 * from a core CIF (v1.1 or v2.0 format) read from the standard input, and
 * to format it as XHTML (UTF-8) on the standard output.  This program
 * silently ignores all CIF syntax and parsing errors, recovering as best
 * it can if any are encountered.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
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
            "<?xml version='1.1' encoding='UTF-8'?>\n<xhtml>\n<head/>\n<body>\n"
            "<h1>Atomic coordinates and thermal parameters</h1>\n");
}

/*
 * Prints an XHTML heading and coordinate table for the given CIF data block
 */
#define BUFSIZE 80
static void print_table3(UFILE *out, cif_block_tp *block) {
    UChar *code;
    cif_loop_tp *coordinate_loop;
    cif_pktitr_tp *iterator;
    cif_packet_tp *packet = NULL;  /* it is important that this pointer be initialized to NULL */
    /* declare data name variables the ICU way: */
    U_STRING_DECL(label_name, "_atom_site_label", 17);
    U_STRING_DECL(x_name, "_atom_site_fract_x", 19);
    U_STRING_DECL(y_name, "_atom_site_fract_y", 19);
    U_STRING_DECL(z_name, "_atom_site_fract_z", 19);
    U_STRING_DECL(u_name, "_atom_site_U_iso_or_equiv", 26);
    const UChar *names[5];

    /* initialize the data name variables the ICU way: */
    U_STRING_INIT(label_name, "_atom_site_label", 17);
    U_STRING_INIT(x_name, "_atom_site_fract_x", 19);
    U_STRING_INIT(y_name, "_atom_site_fract_y", 19);
    U_STRING_INIT(z_name, "_atom_site_fract_z", 19);
    U_STRING_INIT(u_name, "_atom_site_U_iso_or_equiv", 26);
    names[0] = label_name;
    names[1] = x_name;
    names[2] = y_name;
    names[3] = z_name;
    names[4] = u_name;

    /* retrieve the block code and output the table title and headings */
    CHECK_CALL(cif_container_get_code(block, &code), "retrieve a data block's code");
    u_fprintf(out, "<h2>Atomic coordinates and equivalent isotropic thermal parameters for %S</h2>\n<table>\n", code);
    u_fprintf(out, "<tr><th>Atom</th><th>x</th><th>y</th><th>z</th><th>U(eq)</th></tr>\n");
    free(code);

    /* Obtain an iterator over the _atom_site_* loop */
    CHECK_CALL(cif_container_get_item_loop(block, label_name, &coordinate_loop), "retrieve the atom site loop");
    CHECK_CALL(cif_loop_get_packets(coordinate_loop, &iterator), "obtain a loop packet iterator");

    /* output one row for each packet */
    for (;;) {
        int result = cif_pktitr_next_packet(iterator, &packet);
        int counter;

        if (result == CIF_FINISHED) {
            break;
        } else {
            CHECK_CALL(result, "obtain the next atom site packet");
        }
        /* if control reaches here then 'result' is CIF_OK */

        u_fprintf(out, "<tr>");
        for (counter = 0; counter < 5; counter += 1) {
            cif_value_tp *value;
            UChar *text;

            CHECK_CALL(cif_packet_get_item(packet, names[counter], &value),
                    "retrieve a value from a loop packet");  /* the retrieved value belongs to the packet */
            CHECK_CALL(cif_value_get_text(value, &text), "retrieve a value's text representation");
            if (text) {
                u_fprintf(out, "<td>%S</td>", text);
                free(text);  /* the text belongs to us */
            } else {
                u_fprintf(out, "<td>?</td>");
            }
        }
        u_fprintf(out, "</tr>\n");
    }

    u_fprintf(out, "</table>\n");

    cif_packet_free(packet);     /* safe even if 'packet' is still NULL */
    cif_pktitr_abort(iterator);  /* ignore any error -- it doesn't matter at this point */
    cif_loop_free(coordinate_loop);
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
        print_table3(ustdout, *current_block);
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
