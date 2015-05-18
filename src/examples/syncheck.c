/*
 * syncheck.c
 *
 * A CIF 2.0 syntax checker demonstrating some of the capabilities of the
 * CIF API.  Pretty much all the real work is done by the API; this driver
 * program just sets up the parser and reports out the results.
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
#include "messages.h"

/*
 * Pointers to a struct of this type will be passed to the error callback
 * function when the parser calls it
 */
struct syntax_report_s {
    UFILE *ustderr;
    int error_count;
} ;

/*
 * A macro to check the truthiness of a value (generally a function return
 * value), exiting the program with an error message if the value is falsey.
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

/*
 * An error-handling function for use as a parser callback.  When called,
 * this function prints an explanatory message describing the nature and
 * location of the error, increments an error counter, and indicates via
 * its return value that the parser should attempt to recover and continue
 * parsing.
 *
 * Details of the meanings of the arguments and return value are as documented
 * for the CIF API function pointer type cif_parse_error_callback_t.
 */
static int print_error(int code, size_t line, size_t column, const UChar *text, size_t length, void *data) {
    struct syntax_report_s *report_p = (struct syntax_report_s *) data;

    u_fprintf(report_p->ustderr, "  Error code %d at line %lu, column %lu, near \"%.*S\":\n    %s\n",
            code, (unsigned long) line, (unsigned long) column, (int32_t) length, text, messages[code]);
    report_p->error_count += 1;

    return 0;
}

/*
 * Initializes a parse options structure according to the specified
 * command-line arguments.  Returns an index into the argv array one greater
 * than that of the last option argument (which is the index of the first
 * non-option argument if there are any).
 */
static int set_options(struct cif_parse_opts_s *options, int argc, char *argv[]) {
    options->error_callback = print_error;

    /* in the future, additional options may be set according to command-line arguments */
    return 1;
}

/*
 * The main program
 */
int main(int argc, char *argv[]) {
    struct cif_parse_opts_s *options;
    int first_file;
    UFILE *ustderr;
    struct syntax_report_s report;
    int total_errors = 0;

    /* handle command-line arguments */
    CHECK_CALL(cif_parse_options_create(&options), "prepare parse options");
    first_file = set_options(options, argc, argv);

    /* prepare data for the callback function, especially a Unicode stream wrapping stderr */
    ustderr = u_finit(stderr, NULL, NULL);
    if (ustderr == NULL) {
        fprintf(stderr, "Failed to open Unicode error stream.\n");
        exit(1);
    }
    report.ustderr = ustderr;
    options->user_data = &report;

    if (first_file == argc) {
        fprintf(stderr, "No CIF specified.\n");
    } else {

        /*
         * Parse and report on each file specified on the command line
         */

        int file_number;

        for (file_number = first_file; file_number < argc; file_number += 1) {
            FILE *file;
            cif_tp *cif = NULL; /* it's important to initialize this pointer to NULL */

            if (strcmp(argv[file_number], "-") == 0) {
                /* File name "-" is taken to mean the program's standard input */
                file = stdin;
            } else {
                file = fopen(argv[file_number], "rb");
                if (file == NULL) {
                    fprintf(stderr, "Failed to open input file '%s'.\n", argv[file_number]);
                    continue;
                }
            }

            printf("Parsing %s ...\n", argv[file_number]);
            fflush(stdout);

            /* perform the actual parse */
            report.error_count = 0;
            /*
             * Even though this program is loosely described as a "syntax checker", it does intend to check the
             * semantic constraints around block code, frame code, and data name uniqueness.  For that to work, the
             * parser must not be invoked in normal mode (by passing a non-NULL third argument), rather than in
             * "syntax only".  If the required memory footprint for checking large files were a concern, then additional
             * callbacks could be installed to truncate unneeded data.
             */
            CHECK_CALL(cif_parse(file, options, &cif), "parse the input CIF");
            u_fflush(ustderr);  /* flush now in case stderr and stdout point to the same device */

            /* summarize and clean up */
            printf("  %d errors.\n\n", report.error_count);
            fflush(stdout);
            total_errors += report.error_count;
            if (cif) {
                CHECK_CALL(cif_destroy(cif), "release in-memory CIF data");
            }
            if (file != stdin) {
                fclose(file);
            }
        }
    }

    free(options);
    u_fclose(ustderr);

    return (total_errors ? 1 : 0);
}
