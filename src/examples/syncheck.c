/*
 * syncheck.c
 *
 * A CIF 2.0 syntax checker demonstrating some of the capabilities of the
 * CIF API
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unicode/ustdio.h>
#include "cif.h"
#include "messages.h"

struct syntax_report_s {
    UFILE *ustderr;
    int error_count;
} ;

#define CHECK_CALL(f, m) do { \
    int result_ = (f); \
    if (result_ != CIF_OK) { \
        fprintf(stderr, "Failed to %s, returning code %d.\n", (m), result_); \
        exit(1); \
    } \
} while (0)

static int print_error(int code, size_t line, size_t column, const UChar *text, size_t length, void *data) {
    struct syntax_report_s *report_p = (struct syntax_report_s *) data;

    u_fprintf(report_p->ustderr, "  Error code %d at line %lu, column %lu, near \"%.*S\":\n    %s\n",
            code, (unsigned long) line, (unsigned long) column, (int32_t) length, text, messages[code]);
    report_p->error_count += 1;

    return 0;
}

/*
 * Initializes a parse options structure according to the specfied
 * command-line arguments.  Returns an index into the argv array one greater
 * than that of the last option argument (which is the index of the first
 * non-option argument if there are any).
 */
static int set_options(struct cif_parse_opts_s *options, int argc, char *argv[]) {
    options->error_callback = print_error;

    /* in the future, other options may be set according to command-line arguments */
    return 1;
}

int main(int argc, char *argv[]) {
    struct cif_parse_opts_s *options;
    int first_file;
    UFILE *ustderr;
    struct syntax_report_s report;
    int total_errors = 0;

    assert(argc > 0);

    CHECK_CALL(cif_parse_options_create(&options), "prepare parse options");
    first_file = set_options(options, argc, argv);

    ustderr = u_finit(stderr, NULL, NULL);
    if (ustderr == NULL) {
        fprintf(stderr, "Failed to open Unicode error stream.\n");
        exit(1);
    }
    report.ustderr = ustderr;
    options->user_data = &report;

    if (first_file == argc) {
        /* TODO: default to checking standard input */
        fprintf(stderr, "No CIF specified.\n");
    } else {
        int file_number;

        for (file_number = 1; file_number < argc; file_number += 1) {
            FILE *file;
            cif_t *cif = NULL;

            if (strcmp(argv[file_number], "-") == 0) {
                file = stdin;
            } else {
                file = fopen(argv[file_number], "rb");
                if (file == NULL) {
                    fprintf(stderr, "Failed to open input file '%s'.\n", argv[file_number]);
                    continue;
                }
            }

            printf("Parsing %s ...\n", argv[file_number]);
            report.error_count = 0;
            CHECK_CALL(cif_parse(file, options, &cif), "parse the input CIF");
            u_fflush(ustderr);
            printf("  %d errors.\n\n", report.error_count);
            total_errors += report.error_count;
            fflush(stdout);
            if (cif) {
                CHECK_CALL(cif_destroy(cif), "release in-memory CIF data");
            }
        }
    }

    u_fclose(ustderr);
    return (total_errors ? 1 : 0);
}

