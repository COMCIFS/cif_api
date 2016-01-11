/*
 * linguist.c
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

/* the package's config.h is not needed or used */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <unicode/ustdio.h>
#include <unicode/ustring.h>
#include "cif.h"
#include "cif_error.h"

/* data types */

typedef enum { CIF11 = 0, CIF20 = 1, STAR20 = 2, NONE } format;

struct context {
    const char *progname;
    UFILE *out;
    UFILE *ustderr;
    const char *out_encoding;
    const char *element_separator;
    const char *extra_eol;
    const char *extra_ws;
    int no_fold11_output;
    int prefix11_output;
    int quiet;
    int halt_on_error;
    format input_format;
    format output_format;
    unsigned error_count;
    int at_start;
    int in_container;
    int in_loop;
    int column;
};

/* macros */

#ifdef __GNUC__
#define UNUSED   __attribute__ ((__unused__))
#else
#define UNUSED
#endif

#define DEFAULT_OUTPUT_FORMAT "cif20"
#define MAX_LINE_LENGTH 2048

#define UCHAR_TAB   ((UChar) 0x09)
#define UCHAR_LF    ((UChar) 0x0a)
#define UCHAR_CR    ((UChar) 0x0d)
#define UCHAR_SP    ((UChar) 0x20)
#define UCHAR_COLON ((UChar) 0x3a)
#define UCHAR_SEMI  ((UChar) 0x3b)
#define UCHAR_OBRK  ((UChar) 0x5b)
#define UCHAR_BSL   ((UChar) 0x5c)
#define UCHAR_CBRK  ((UChar) 0x5d)
#define UCHAR_OBRC  ((UChar) 0x7b)
#define UCHAR_CBRC  ((UChar) 0x7d)

/* The text prefixed used by this program when one is required */
#define PREFIX "> "

/*
 * The halfwidth of the window within which the line-folding algorithm will look for a suitable location to fold long
 * lines.
 */
#define FOLD_WINDOW 8

/*
 * The maximum length of the data content of any physical line in a line-folded text field.  Such lines must also
 * contain a fold separator at the end (minimum one character, not including a line terminator.
 */
#define MAX_FOLD_LENGTH (MAX_LINE_LENGTH - 1)

#define SPACE_FORBIDDEN -1
#define SPACE_ALLOWED    0
#define SPACE_REQUIRED   1

#define CONTEXT_PROGNAME(c) (((struct context *)(c))->progname)
#define CONTEXT_OUT(c) (((struct context *)(c))->out)
#define CONTEXT_ERR(c) (((struct context *)(c))->ustderr)
#define CONTEXT_QUIET(c) (((struct context *)(c))->quiet)
#define CONTEXT_HALT_ON_ERROR(c) (((struct context *)(c))->halt_on_error)
#define CONTEXT_SET_OUTFORMAT(c, f) do { ((struct context *)(c))->output_format = (f); } while (0)
#define CONTEXT_OUTFORMAT(c) (((struct context *)(c))->output_format)
#define CONTEXT_SET_SEPARATOR(c, s) do { ((struct context *)(c))->element_separator = (s); } while (0)
#define CONTEXT_SEPARATOR(c) (((struct context *)(c))->element_separator)
#define CONTEXT_ERROR_COUNT(c) (((struct context *) (c))->error_count)
#define CONTEXT_INC_ERROR_COUNT(c) do { ((struct context *) (c))->error_count += 1; } while (0)
#define CONTEXT_IN_CONTAINER(c) (((struct context *) (c))->in_container)
#define CONTEXT_PUSH_CONTAINER(c) do { ((struct context *) (c))->in_container += 1; } while (0)
#define CONTEXT_POP_CONTAINER(c) do { ((struct context *) (c))->in_container -= 1; } while (0)
#define CONTEXT_IN_LOOP(c) (((struct context *) (c))->in_loop)
#define CONTEXT_TOGGLE_LOOP(c) do { ((struct context *) (c))->in_loop = !((struct context *) (c))->in_loop; } while (0)
#define CONTEXT_AT_START(c) (((struct context *) (c))->at_start)
#define CONTEXT_SET_STARTED(c) do { ((struct context *) (c))->at_start = 1; } while (0)
#define CONTEXT_EXTRA_WS(c) (((struct context *) (c))->extra_ws)
#define CONTEXT_EXTRA_EOL(c) (((struct context *) (c))->extra_eol)
#define CONTEXT_SET_COLUMN(c, col) do { ((struct context *)(c))->column = (col); } while (0)
#define CONTEXT_INC_COLUMN(c, amt) do { ((struct context *)(c))->column += (amt); } while (0)
#define CONTEXT_COLUMN(c) (((struct context *)(c))->column)

/*
 * A macro for handling short options that accept arguments
 */
#define PROCESS_ARGS(arg, opts, i, j) do {                  \
    if (argv[i][++j]) {                                     \
        process_args_ ## arg ((opts), argv[i] + j);         \
    } else if ((i + 1 < argc) && (argv[i + 1][0] != '-')) { \
        process_args_ ## arg ((opts), argv[++i]);           \
    } else {                                                \
        process_args_ ## arg ((opts), NULL);                \
    }                                                       \
} while (0)

#define DIE_UNLESS(p, n) do { \
    if (!(p)) {               \
        perror(n);            \
        exit(2);              \
    }                         \
} while(0)

/* global variables */

const char headers[][16] = {
        "#\\#CIF_1.1\n",  /* CIF11  */
        "#\\#CIF_2.0\n",  /* CIF20  */
        ""                /* STAR20 */
};

/* function prototypes */

void usage(const char *progname);
void process_args(int argc, char *argv[], struct cif_parse_opts_s *parse_opts);
void process_args_input_encoding(struct cif_parse_opts_s *parse_opts, const char *encoding);
void process_args_input_format(struct cif_parse_opts_s *parse_opts, const char *format);
void process_args_input_folding(struct cif_parse_opts_s *parse_opts, const char *folding);
void process_args_input_prefixing(struct cif_parse_opts_s *parse_opts, const char *prefixing);
void process_args_output_encoding(struct cif_parse_opts_s *parse_opts, const char *encoding);
void process_args_output_format(struct cif_parse_opts_s *parse_opts, const char *format);
void process_args_output_folding(struct cif_parse_opts_s *parse_opts, const char *folding);
void process_args_output_prefixing(struct cif_parse_opts_s *parse_opts, const char *prefixing);
void process_args_quiet(struct cif_parse_opts_s *parse_opts);
void process_args_strict(struct cif_parse_opts_s *parse_opts);
int to_boolean(const char *val);
int print_header(cif_tp *cif, void *context);
int handle_cif_end(cif_tp *cif, void *context);
int open_block(cif_container_tp *block, void *context);
int open_frame(cif_container_tp *frame, void *context);
int finish_frame(cif_container_tp *container, void *context);
int flush_container(cif_container_tp *container, void *context);
int handle_loop_start(cif_loop_tp *loop, void *context);
int handle_loop_end(cif_loop_tp *loop, void *context);
int start_packet_on_new_line(cif_packet_tp *packet, void *context);
int discard_packet(cif_packet_tp *packet, void *context);
int print_item(UChar *name, cif_value_tp *value, void *context);
int print_list(cif_value_tp *value, void *context);
int print_table(cif_value_tp *value, void *context);
int print_value_text(cif_value_tp *value, void *context);
int print_text_field(UChar *str, int do_fold, int do_prefix, void *context);
ptrdiff_t compute_fold_length(UChar *fold_start, ptrdiff_t line_length, ptrdiff_t target_length, int window,
        int allow_folding_before_semi);
void whitespace_callback(size_t line, size_t column, const UChar *ws, size_t length, void *data);
int error_callback(int code, size_t line, size_t column, const UChar *text, size_t length, void *data);
void keyword_callback(size_t line, size_t column, const UChar *keyword, size_t length, void *data);
void dataname_callback(size_t line, size_t column, const UChar *dataname, size_t length, void *data);
int print_code(cif_container_tp *container, void *context, const char *type);
int flush_loops(cif_container_tp *container);
int ensure_space(int data_length, void *context);
int print_u_literal(int preceding_space, const UChar *str, int line1_length, void *context);
int print_delimited(UChar *str, UChar *delim, int delim_length, UFILE *out);
void translate_whitespace(UChar *text, size_t length, const UChar *extra_eol, const UChar *extra_ws);

/* function implementations */

void usage(const char *progname) {
    fprintf(stderr, "\nusage: %s [-f <input-format>] [-e <input-encoding>] [-l [1|0]] [-p [1|0]]\n"
                    "          [-F <output-format>] [-E <output-encoding>] [-L [1|0]] [-P [1|0]]\n"
                    "          [-q] [-s] [--] [<input-file> [<output-file>]]\n\n", progname);
    fputs(          "Description:\n", stderr);
    fputs(       /* "Transforms CIF data among CIF formats and dialects, or to STAR 2.0 format.\n" */
                    "Transforms CIF data among CIF formats and dialects.\n"
                    "If no input file is specified, or if input is specified as \"-\", then input\n"
                    "is read from the standard input, else it is from the specified file.  If no\n"
                    "output file is specified, or if output is specified as \"-\", then output is\n"
                    "directed to the standard output, else it goes to the specified file.\n\n",
                    stderr);
    fputs(          "Options that take boolean arguments (described as 1|0 in the synopsis and option\n"
                    "descriptions) will also accept arguments 'yes', 'true', 'no', and 'false'.\n\n",
                    stderr);
    fputs(          "Options:\n",
                    stderr);
    fputs(          "  -e <encoding>, --input-encoding=<encoding>\n"
                    "          Specifies the input character encoding.  If given as \"auto\" (the\n"
                    "          default) then the program attempts to determine the encoding from the\n"
                    "          input and falls back to a format- and system-specific default if it is\n",
                    stderr);
    fputs(          "          unable to do so.  Otherwise, the encoding names recognized are system-\n"
                    "          dependent, but they take the form of IANA names and aliases.  The specified\n"
                    "          encoding will be used, even for CIF 2.0 format input (even though the CIF 2.0\n"
                    "          specifications permit only UTF-8).\n\n",
                    stderr);
    fputs(          "  -E <encoding>, --output-encoding=<encoding>\n"
                    "          Specifies the output character encoding.  If given as \"auto\" (the\n"
                    "          default) then the program chooses an encoding in a format- and system-specific\n"
                    "          way.  Otherwise, the encoding names recognized are system-\n"
                    "          dependent, but they take the form of IANA names and aliases.  The specified\n"
                    "          encoding will be used, even for CIF 2.0 format output (even though the CIF 2.0\n"
                    "          specifications permit only UTF-8).\n\n",
                    stderr);
    fputs(          "  -f <format>, --input-format=<format>\n"
                    "          Specifies the input format.  The formats supported are \"auto\" (the\n"
                    "          program guesses; this is the default), \"cif10\" (the program assumes\n"
                    "          CIF 1.0), \"cif11\" (the program assumes CIF 1.1), and \"cif20\" (the\n"
                    "          program assumes CIF 2.0).  A format (other than auto) specified via this\n"
                    "          option overrides any contradictory indications in the file itself.\n\n",
                    stderr);
    fputs(          "  -F <format>, --output-format=<format>\n"
                    "          Specifies the output format.  The formats supported are \"cif11\" (the\n"
                    "          program emits CIF 1.1 format) and \"cif20\" (the program emits CIF 2.0\n"
                    "          format; this is the default).\n\n",
                    stderr);
    fputs(          "  -l 1|0, --input-line-folding=1|0\n"
                    "          Specifies whether to recognize and decode the CIF line-folding protocol\n"
                    "          in text fields in the the input.  Defaults to 1 (yes).\n\n",
                    stderr);
    fputs(          "  -L 1|0, --output-line-folding=1|0\n"
                    "          Specifies whether to allow line folding of text fields in the output.\n"
                    "          The program chooses automatically, on a field-by-field basis, whether\n"
                    "          to perform folding.  Defaults to 1 (yes).\n\n",
                    stderr);

    fputs(          "  -p 0|1, --input-text-prefixing=0|1\n"
                    "          Specifies whether to recognize and decode the CIF text-prefixing protocol\n"
                    "          in text fields in the the input.  Defaults to 1 (yes).\n\n",
                    stderr);
    fputs(          "  -P 0|1, --output-text-prefixing=0|1\n"
                    "          Specifies whether to allow line prefixing of text fields in the output.\n"
                    "          The program chooses automatically, on a field-by-field basis, whether\n"
                    "          to perform prefixing.  Defaults to 1 (yes).\n\n",
                    stderr);
    fputs(          "  -q      This option suppresses diagnostic output.  The exit status will still\n"
                    "          provide a general idea of the program's success.\n\n",
                    stderr);
    fputs(          "  -s      This option instructs the program to insist that the input data strictly\n"
                    "          conform to the chosen CIF format.  Any error will cause the program to\n"
                    "          terminate prematurely.  If this option is not given then the program will\n"
                    "          instead make a best effort at reading and processing the input despite\n"
                    "          any errors it may encounter.  Such error recovery efforts are inherently\n"
                    "          uncertain, however, and sometimes lossy.\n\n",
                    stderr);
    fputs(          "  --      Indicates the end of the option arguments.  Any subsequent arguments are\n"
                    "          interpreted as file names.\n\n",
                    stderr);

    fputs(          "Exit Status:\n"
                    "The program exits with status 0 if the input was parsed without any error and\n"
                    "successfully transformed.  It exits with status 1 if parse errors were detected,\n"
                    "but the program nevertheless consumed the entire input and produced a\n"
                    "transformation.  It exits with status 2 if no parse was attempted.  It exits with\n"
                    "status 3 if parse or transformation is interrupted prior to the full input being\n"
                    "consumed.\n\n"
                    , stderr);
    exit(2);
}

/*
 * Returns 1 if the provided string represents truth, 0 if it represents falsehood, or -1 if it is unrecognized
 */
void process_args(int argc, char *argv[], struct cif_parse_opts_s *parse_opts) {
    struct context *context = (struct context *) parse_opts->user_data;
    int infile_index = 0;
    int outfile_index = 0;
    char *c;
    int i;

    assert(argc > 0);

    c = strrchr(argv[0], '/'); /* XXX: What about Windows? */
    context->progname = (c ? (c + 1) : argv[0]);
    parse_opts->whitespace_callback = whitespace_callback;
    parse_opts->error_callback = error_callback;
    process_args_output_format(parse_opts, DEFAULT_OUTPUT_FORMAT);

    for (i = 1; i < argc; i += 1) {
        if ((argv[i][0] != '-') || (!argv[i][1])) {
            /* a non-option argument */
            break;
        } else if (argv[i][1] == '-') {
            if (argv[i][2]) {
                /* a GNU-style long option */
                int j;

                for (j = 2; argv[i][j] && argv[i][j] != '='; ) {
                    j += 1;
                }
                if (!strncmp(&argv[i][2], "input-format", j - 2)) {
                    process_args_input_format(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "input-encoding", j - 2)) {
                    process_args_input_encoding(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "input-line-folding", j - 2)) {
                    process_args_input_folding(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "input-text-prefixing", j - 2)) {
                    process_args_input_prefixing(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "output-format", j - 2)) {
                    process_args_output_format(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "output-encoding", j - 2)) {
                    process_args_output_encoding(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "output-line-folding", j - 2)) {
                    process_args_output_folding(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strncmp(&argv[i][2], "output-text-prefixing", j - 2)) {
                    process_args_output_prefixing(parse_opts, (argv[i][j] ? argv[i] + j + 1 : NULL));
                } else if (!strcmp(&argv[i][2], "quiet")) {
                    process_args_quiet(parse_opts);
                } else if (!strcmp(&argv[i][2], "strict")) {
                    process_args_strict(parse_opts);
                } else {
                    usage(context->progname);
                }
            } else {
                /* explicit end of options */
                i += 1;
                break;
            }
        } else {
            int j;

            /* process short options */
            for (j = 1; ; j += 1) {
                switch (argv[i][j]) {
                    case '\0':
                        if (j == 1) {
                            /* '-' by itself is a file designator, not an option */
                            goto end_options;
                        }
                        goto end_arg;
                    case 'f':
                        PROCESS_ARGS(input_format, parse_opts, i, j);
                        goto end_arg;
                    case 'e':
                        PROCESS_ARGS(input_encoding, parse_opts, i, j);
                        goto end_arg;
                    case 'l':
                        PROCESS_ARGS(input_folding, parse_opts, i, j);
                        goto end_arg;
                    case 'p':
                        PROCESS_ARGS(input_prefixing, parse_opts, i, j);
                        goto end_arg;
                    case 'F':
                        PROCESS_ARGS(output_format, parse_opts, i, j);
                        goto end_arg;
                    case 'E':
                        PROCESS_ARGS(output_encoding, parse_opts, i, j);
                        goto end_arg;
                    case 'L':
                        PROCESS_ARGS(output_folding, parse_opts, i, j);
                        goto end_arg;
                    case 'P':
                        PROCESS_ARGS(output_prefixing, parse_opts, i, j);
                        goto end_arg;
                    case 'q': /* quiet mode */
                        process_args_quiet(parse_opts);
                        break;
                    case 's': /* strict parsing mode */
                        process_args_strict(parse_opts);
                        break;
                    default:
                        usage(context->progname);
                        break;
                } /* switch */
            } /* for j */
            end_arg:
            ;
        } /* else */
    } /* for i */

    end_options:

    if (i < argc) infile_index = i++;
    if (i < argc) outfile_index = i++;
    if (i < argc) usage(context->progname);

    if (infile_index && strcmp(argv[infile_index], "-")) {
        /* Connect the specified input file to stdin */
        /* For consistency, use text mode even for CIF 2 / STAR 2, because the standard input comes that way */
        FILE *in = freopen(argv[infile_index], "r", stdin);
        DIE_UNLESS(in, context->progname);
    } /* else use the standard input provided by the environment */

    if (outfile_index && strcmp(argv[outfile_index], "-")) {
        /* Connect the specified output file to stdout */
        /* For consistency, use text mode even for CIF 2 / STAR 2, because the standard output comes that way */
        FILE *out = freopen(argv[outfile_index], "w", stdout);
        DIE_UNLESS(out, context->progname);
    } /* else use the standard output provided by the environment */

    context->out = u_finit(stdout, "C", context->out_encoding);
    context->ustderr = u_finit(stderr, NULL, NULL);

    if (!(context->out && context->ustderr)) {
        perror(context->progname);
        fprintf(stderr, "%s: could not initialize Unicode output and/or error stream\n", argv[0]);
        exit(2);
    }

    /* final adjustments */
    context->extra_eol = parse_opts->extra_eol_chars;
    context->extra_ws = parse_opts->extra_ws_chars;
}

void process_args_input_encoding(struct cif_parse_opts_s *parse_opts, const char *encoding) {
    if (!encoding) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    }
    if (strcmp(encoding, "auto")) {
        parse_opts->default_encoding_name = NULL;
        parse_opts->force_default_encoding = 0;
    } else {
        parse_opts->default_encoding_name = encoding;
        parse_opts->force_default_encoding = 1;
    }
}

void process_args_input_format(struct cif_parse_opts_s *parse_opts, const char *fmt) {
    if (!fmt) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    }
    if (!strcmp(fmt, "auto")) {
        parse_opts->prefer_cif2 = 0;
    } else if (!strcmp(fmt, "cif20")) {
        parse_opts->prefer_cif2 = 20;
    } else if (!strcmp(fmt, "cif11")) {
        parse_opts->prefer_cif2 = -1;
    } else if (!strcmp(fmt, "cif10")) {
        parse_opts->prefer_cif2 = -1;
        parse_opts->extra_ws_chars = "\v";
        parse_opts->extra_eol_chars = "\f";
    } else {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    }
}

void process_args_input_folding(struct cif_parse_opts_s *parse_opts, const char *folding) {
    /* if the optional argument is not specified then it is taken as 1/true/yes */
    int argval = folding ? to_boolean(folding) : 1;

    if (argval < 0) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    } else {
        /* +1 for true, -1 for false */
        parse_opts->line_folding_modifier = 2 * argval - 1;
    }
}

void process_args_input_prefixing(struct cif_parse_opts_s *parse_opts, const char *prefixing) {
    /* if the optional argument is not specified then it is taken as 1/true/yes */
    int argval = prefixing ? to_boolean(prefixing) : 1;

    if (argval < 0) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    } else {
        /* +1 for true, -1 for false */
        parse_opts->text_prefixing_modifier = 2 * argval - 1;
    }
}

void process_args_output_encoding(struct cif_parse_opts_s *parse_opts, const char *encoding) {
    if (!encoding) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    }
    ((struct context *) (parse_opts->user_data))->out_encoding = encoding;
}

void process_args_output_format(struct cif_parse_opts_s *parse_opts, const char *fmt) {
    if (!fmt) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    } else if (!strcmp(fmt, "cif11")  || !strcmp(fmt, "cif1.1")) {
        CONTEXT_SET_OUTFORMAT(parse_opts->user_data, CIF11);
        CONTEXT_SET_SEPARATOR(parse_opts->user_data, NULL);
    } else if (!strcmp(fmt, "cif20")  || !strcmp(fmt, "cif2.0")) {
        CONTEXT_SET_OUTFORMAT(parse_opts->user_data, CIF20);
        CONTEXT_SET_SEPARATOR(parse_opts->user_data, "");
/*
    } else if (!strcmp(fmt, "star20") || !strcmp(fmt, "star2.0")) {
        CONTEXT_SET_OUTFORMAT(parse_opts->user_data, STAR20);
        CONTEXT_SET_SEPARATOR(parse_opts->user_data, ",");
 */
    } else {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    }
}

void process_args_output_folding(struct cif_parse_opts_s *parse_opts, const char *folding) {
    /* if the optional argument is not specified then it is taken as 1/true/yes */
    int argval = folding ? to_boolean(folding) : 1;

    if (argval < 0) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    } else {
        assert(argval < 2);
        ((struct context *) parse_opts->user_data)->no_fold11_output = 1 - argval;
    }
}

void process_args_output_prefixing(struct cif_parse_opts_s *parse_opts, const char *prefixing) {
    /* if the optional argument is not specified then it is taken as 1/true/yes */
    int argval = prefixing ? to_boolean(prefixing) : 1;

    if (argval < 0) {
        usage(CONTEXT_PROGNAME(parse_opts->user_data));
    } else {
        ((struct context *) parse_opts->user_data)->no_fold11_output = argval;
    }
}

void process_args_quiet(struct cif_parse_opts_s *parse_opts) {
    ((struct context *) parse_opts->user_data)->quiet = 1;
}

void process_args_strict(struct cif_parse_opts_s *parse_opts) {
    ((struct context *) parse_opts->user_data)->halt_on_error = 1;
}

/*
 * Translates the specified 'extra' end-of-line characters to to newlines and the specified 'extra' whitespace
 * characters to spaces in the provided text buffer.  The buffer is expected to be nul-terminated, but if its length is
 * already known then that can be specified as an argument for a minor efficiency improvement.  Otherwise, the length
 * should be passed as ((size_t) -1).
 */
int to_boolean(const char *val) {
    size_t len = strlen(val);
    char *dup = (char *) malloc((len + 1) * sizeof(char));
    int result;

    if (dup) {
        char *c;

        strcpy(dup, val);
        for (c = dup; *c; c += 1) {
            *c = tolower(*c);
        }
        val = dup;
    }

    if (len > 5) {
        /* the maximum-length recognized value is "false", with length 5.  Fail now if the specified value is longer */
        result = -1;
    } else if (!(strcmp(val, "1") && strcmp(val, "yes") && strcmp(val, "true"))) {
        result = 1;
    } else if (!(strcmp(val, "0") && strcmp(val, "no") && strcmp(val, "false"))) {
        result = 0;
    } else {
        result = -1;
    }

    free(dup);
    return result;
}

int print_header(cif_tp *cif UNUSED, void *context) {
    format fmt = CONTEXT_OUTFORMAT(context);
    int32_t nchars;

    assert(fmt < NONE);

    /*
     * No newline is printed because the only thing that can immediately follow is whitespace or a block header,
     * and a leading newline will alread be provided for the latter.
     */
    nchars = u_fprintf(CONTEXT_OUT(context), "%s", headers[fmt]);
    if (nchars < 0) {
        return CIF_ERROR;
    } else {
        CONTEXT_SET_COLUMN(context, 0);
        return CIF_OK;
    }
}

int handle_cif_end(cif_tp *cif UNUSED, void *context) {
    u_fputc(0x0aU, CONTEXT_OUT(context));  /* ignore the result */
    u_fclose(CONTEXT_OUT(context));
    u_fclose(CONTEXT_ERR(context));
    return 0;
}

/* function intended to handle block_start events */
int open_block(cif_container_tp *block, void *context) {
    CONTEXT_SET_STARTED(context);
    CONTEXT_PUSH_CONTAINER(context);
    return print_code(block, context, "data_");
}

/* function intended to handle frame_start events */
int open_frame(cif_container_tp *frame, void *context) {
    if (CONTEXT_IN_CONTAINER(context)) {
        CONTEXT_PUSH_CONTAINER(context);
        return print_code(frame, context, "save_");
    } else {
        return CIF_OK;
    }
}

/*
 * Outputs a save frame terminator and cleans out the frame contents
 * Intended to handle frame_end events.
 */
int finish_frame(cif_container_tp *container, void *context) {
    if (CONTEXT_IN_CONTAINER(context)) {
        if (u_fprintf(CONTEXT_OUT(context), "\nsave_\n") != 7) {
            return CIF_ERROR;
        } else {
            CONTEXT_SET_COLUMN(context, 0);
        }
    }
    return flush_container(container, context);
}

/*
 * Removes all save frames and data from the specified container.
 * Intended to handle block_end events, and to participate in handling frame_end events.
 */
int flush_container(cif_container_tp *container, void *context UNUSED) {
    int result;
    cif_container_tp **frames;

    if ((result = cif_container_get_all_frames(container, &frames)) == CIF_OK) {
        cif_container_tp **frame;

        for (frame = frames; *frame; frame += 1) {
            if ((result = cif_container_destroy(*frame)) != CIF_OK) {

                /* clean up remaining frame handles */
                for (; *frame; frame += 1) {
                    cif_container_free(*frame);
                }

                goto done;
            }
        }

        /* all contained save frames were successfully destroyed; now destroy the loops */
        result = flush_loops(container);

        done:
        free(frames);
    }

    CONTEXT_POP_CONTAINER(context);

    return result;
}

/*
 * Prints a loop header to the output
 */
int handle_loop_start(cif_loop_tp *loop, void *context) {
    if (CONTEXT_IN_CONTAINER(context)) {
        int result;
        UChar **names;

        CONTEXT_TOGGLE_LOOP(context);

        if (u_fprintf(CONTEXT_OUT(context), "\nloop_\n") != 7) {
            result = CIF_ERROR;
        } else if ((result = cif_loop_get_names(loop, &names)) == CIF_OK) {
            UChar **name;

            for (name = names; *name; name += 1) {
                if (u_fprintf(CONTEXT_OUT(context), "%S\n", *name) != u_strlen(*name) + 2) {
                    result = CIF_ERROR;
                }
                free(*name);
            }

            free(names);
        }

        CONTEXT_SET_COLUMN(context, 0);
        return result;
    } else {
        return 0;
    }
}

/*
 * Tracks that the parser has left a loop, and ends the current line
 */
int handle_loop_end(cif_loop_tp *loop UNUSED, void *context) {
    if (CONTEXT_IN_CONTAINER(context)) {
        CONTEXT_TOGGLE_LOOP(context);

        if (u_fputc(0x0aU, CONTEXT_OUT(context)) != 0x0aU) {
            return CIF_ERROR;
        }
        CONTEXT_SET_COLUMN(context, 0);
    }
    return 0;
}

/*
 * Causes each loop packet to start on a new line
 * Intended to handle packet_start events.
 */
int start_packet_on_new_line(cif_packet_tp *packet UNUSED, void *context) {
    if (CONTEXT_IN_CONTAINER(context)) {
        if (CONTEXT_COLUMN(context)) {
            if (u_fputc(0x0aU, CONTEXT_OUT(context)) != 0x0aU) {
                return CIF_ERROR;
            }
            CONTEXT_SET_COLUMN(context, 0);
        }
    }
    return CIF_TRAVERSE_CONTINUE;
}

/*
 * Suppresses recording looped data
 * Intended to handle packet_end events.
 */
int discard_packet(cif_packet_tp *packet UNUSED, void *context UNUSED) {
    return CIF_TRAVERSE_SKIP_CURRENT;
}

/* Intended to handle item events. */
int print_item(UChar *name, cif_value_tp *value, void *context) {
    U_STRING_DECL(unk_value_literal, "?", 2);
    U_STRING_DECL(na_value_literal, ".", 2);

    U_STRING_INIT(unk_value_literal, "?", 2);
    U_STRING_INIT(na_value_literal, ".", 2);

    if (CONTEXT_IN_CONTAINER(context)) {
        if (name && !CONTEXT_IN_LOOP(context)) {
            int32_t n_printed = u_fprintf(CONTEXT_OUT(context), "\n%S", name);

            if (n_printed < 3) {
                return CIF_ERROR;
            } else {
                CONTEXT_SET_COLUMN(context, n_printed - 1);
            }
        }

        /* write the value */
        switch(cif_value_kind(value)) {
            case CIF_CHAR_KIND:
            case CIF_NUMB_KIND:
                return print_value_text(value, context);
            case CIF_UNK_KIND:
                return print_u_literal(SPACE_REQUIRED, unk_value_literal, 1, context);
            case CIF_NA_KIND:
                return print_u_literal(SPACE_REQUIRED, na_value_literal, 1, context);
            case CIF_LIST_KIND:
                return print_list(value, context);
            case CIF_TABLE_KIND:
                return print_table(value, context);
            default:
                /* unknown kind */
                return CIF_INTERNAL_ERROR;
        }
    } /* else ignore it altogether */

    return CIF_TRAVERSE_CONTINUE;
}

/*
 * A callback function by which whitespace (including comments) in the input CIF is handled.  This version echoes
 * input whitespace to the output.
 */
int print_list(cif_value_tp *value, void *context) {
    static const UChar list_open[] = { UCHAR_OBRK, 0 };
    static const UChar list_close[] = { UCHAR_SP, UCHAR_CBRK, 0 };
    size_t count;
    int result;

    if (CONTEXT_OUTFORMAT(context) == CIF11) {
        result = CIF_DISALLOWED_VALUE;
    } else if ((result = cif_value_get_element_count(value, &count)) == CIF_OK) {
        if ((result = print_u_literal(SPACE_REQUIRED, list_open, 1, context)) == CIF_OK) {
            size_t index;

            for (index = 0; index < count; index += 1) {
                cif_value_tp *element;

                if (cif_value_get_element_at(value, index, &element) != CIF_OK) {
                    return CIF_INTERNAL_ERROR;
                } else if ((result = print_item(NULL, element, context)) != CIF_TRAVERSE_CONTINUE) {
                    return result;
                }
            }

            result = print_u_literal(SPACE_ALLOWED, list_close, 2, context);
        }
    }

    return result;
}

int print_table(cif_value_tp *value, void *context) {
    static const UChar table_open[] =  { UCHAR_OBRC, 0 };
    static const UChar table_close[] = { UCHAR_SP, UCHAR_CBRC, 0 };
    static const UChar entry_colon[] = { UCHAR_COLON, 0 };
    const UChar **keys;
    int result;

    if (CONTEXT_OUTFORMAT(context) == CIF11) {
        result = CIF_DISALLOWED_VALUE;
    } else if ((result = cif_value_get_keys(value, &keys)) == CIF_OK) {
        if ((result = print_u_literal(SPACE_REQUIRED, table_open, 1, context)) == CIF_OK) {
            const UChar **key;

            for (key = keys; *key; key += 1) {
                cif_value_tp *kv;
                cif_value_tp *ev;

                /* key and value are both printed via the machinery for printing values */
                if (((result = cif_value_get_item_by_key(value, *key, &ev)) != CIF_OK)
                        || ((result = cif_value_create(CIF_UNK_KIND, &kv)) != CIF_OK)) {
                    goto cleanup;
                } else {
                    /* copying the key is inefficient, but the original belongs to the table value */
                    if (((result = cif_value_copy_char(kv, *key)) != CIF_OK)
                            || ((result = print_value_text(kv, context)) != CIF_OK)
                            || ((result = print_u_literal(SPACE_FORBIDDEN, entry_colon, 1, context)) != CIF_OK)
                            || ((result = print_item(NULL, ev, context)) != CIF_TRAVERSE_CONTINUE)
                            ) {
                        cif_value_free(kv);
                        goto cleanup;
                    }

                    cif_value_free(kv);
                    /* ev (the entry's value) must not be freed -- it belongs to the table */
                }
            }

            result = print_u_literal(SPACE_ALLOWED, table_close, 2, context);
        }

        cleanup:

        /* free only the key array, not the individual keys, as required by cif_value_get_keys() */
        free(keys);
    }

    return result;
}

int print_value_text(cif_value_tp *value, void *context) {
    struct cif_string_analysis_s analysis;
    UChar *text;
    int result;

    if ((result = cif_value_get_text(value, &text)) == CIF_OK) {
        if ((result = cif_analyze_string(text, !cif_value_is_quoted(value), CONTEXT_OUTFORMAT(context) != CIF11,
                MAX_LINE_LENGTH, &analysis)) == CIF_OK) {
            int32_t length;

            switch (analysis.delim_length) {
                case 3: /* triple-quoted */
                    if (analysis.num_lines > 1) {
                        if (((result = ensure_space(analysis.length_first + 3, context)) == CIF_OK)
                            && ((result = print_delimited(text, analysis.delim, 3, CONTEXT_OUT(context))) == CIF_OK)) {
                            CONTEXT_SET_COLUMN(context, analysis.length_last + 3);
                        }
                        break;
                    } /* else fall through */
                case 1: /* single-quoted */
                case 0: /* whitespace-delimited */
                    length = analysis.length_first + 2 * analysis.delim_length;
                    if (((result = ensure_space(length, context)) == CIF_OK)
                        && ((result = print_delimited(text, analysis.delim, analysis.delim_length,
                                CONTEXT_OUT(context))) == CIF_OK)) {
                        CONTEXT_SET_COLUMN(context, length);
                    }
                    break;
                case 2:
                    return print_text_field(text,
                            /* whether to fold: */
                            (analysis.length_max > MAX_LINE_LENGTH)
                                    || (analysis.length_first >= MAX_LINE_LENGTH)
                                    || analysis.has_reserved_start
                                    || analysis.has_trailing_ws
                                    || (analysis.max_semi_run >= (MAX_FOLD_LENGTH - 1)),
                            /* whether to prefix: */
                            analysis.contains_text_delim
                                    || (analysis.max_semi_run >= (MAX_FOLD_LENGTH - 1)),
                            context
                            );
                default:
                    result = CIF_INTERNAL_ERROR;
                    break;
            }
        }
        free(text);
    }

    return result;
}

/* prints a string in text-field form, applying line-folding and / or text prefixing as directed */
int print_text_field(UChar *str, int do_fold, int do_prefix, void *context) {
    /* CIF line termination characters */
    static const UChar line_term[] = { 0xa, 0xd, 0 };

    int result;

    if (!(do_prefix || do_fold)) {
        result = (u_fprintf(CONTEXT_OUT(context), "\n;%S\n;", str) < 4) ? CIF_ERROR : CIF_OK;
    } else {
        if (u_fprintf(CONTEXT_OUT(context), "\n;%s%s%s", (do_prefix ? PREFIX "\\" : ""), (do_fold ? "\\" : ""),
                ((do_prefix || do_fold) ? "\n" : "")) < 2) {
            result = CIF_ERROR;
        } else {
            UChar *line_start;  /* points to the start of the current logical line */
            UChar *line_end;    /* points to the terminator of the current logical line (line term or string term) */
            int32_t prefix_len = do_prefix ? 2 : 0;

            for (line_start = str; *line_start; line_start = line_end + 1) {
                /* each logical line */
                int32_t line_len = u_strcspn(line_start, line_term);

                line_end = line_start + line_len;

                if (!do_fold) {
                    assert(do_prefix);
                    if (u_fprintf(CONTEXT_OUT(context), PREFIX "%.*S\n", line_len, line_start)
                            < (prefix_len + line_len + 1)) {
                        return CIF_ERROR;
                    }
                } else {
                    UChar *fold_start = line_start;
                    ptrdiff_t fold_len;

                    do {
                        /* each folded segment (even if there's only one; even if it's empty) */
                        ptrdiff_t limit = line_end - fold_start;

                        fold_len = compute_fold_length(fold_start, limit,
                                MAX_FOLD_LENGTH - FOLD_WINDOW - prefix_len , FOLD_WINDOW, do_prefix);
                        assert(0 < fold_len && fold_len <= limit);

                        if (fold_len == limit) {

                            /* whether trailing whitespace or literal backslashes need to be protected */
                            int protect = (fold_len > 0) && (
                                    (fold_start[fold_len - 1] == UCHAR_SP)
                                    || (fold_start[fold_len - 1] == UCHAR_TAB)
                                    || (fold_start[fold_len - 1] == UCHAR_BSL));

                            if (u_fprintf(CONTEXT_OUT(context), "%s%.*S%s\n", (do_prefix ? PREFIX : ""),
                                    fold_len, fold_start, (protect ? "\\\n" : "")) < (prefix_len + fold_len)) {
                                return CIF_ERROR;
                            }
                        } else {
                            if (u_fprintf(CONTEXT_OUT(context), "%s%.*S\\\n", (do_prefix ? PREFIX : ""),
                                    fold_len, fold_start) < (prefix_len + fold_len)) {
                                return CIF_ERROR;
                            }
                        }
                        fold_start += fold_len;
                    } while (fold_start < line_end);
                }

                /* CR/LF line termination provides an extra character to eat */
                if ((*line_end == UCHAR_CR) && (*(line_end + 1) == UCHAR_LF)) {
                    line_end += 1;
                }

                if (!*line_end) {
                    /* lest the loop control expression attempt to dereference an out-of-bounds pointer: */
                    break;
                }
            }

            /* closing delimiter */
            /* the leading newline will already have been output */
            result = (u_fputc(UCHAR_SEMI, CONTEXT_OUT(context)) == UCHAR_SEMI) ? CIF_OK : CIF_ERROR;
        }
    }

    if (result == CIF_OK) {
        CONTEXT_SET_COLUMN(context, 1);
    }

    return result;
}

/*
 * Chooses how much of the given line of text should be included in the next folded segment
 */
ptrdiff_t compute_fold_length(UChar *fold_start, ptrdiff_t line_length, ptrdiff_t target_length, int window,
        int allow_folding_before_semi) {
    assert(target_length > window);

    if (line_length <= target_length + window) {
        /* the line fits without folding */
        return line_length;
    } else {

        /*
         * prefer to fold at a transition from whitespace to non-whitespace, as close as possible to the target length
         */

        int best_category = 0; /* category 0 = no good; 1 = between non-space; 2 = between space; 3 = transition */
        int best_diff = -(window + 1);
        int diff;
        UChar this_char = fold_start[target_length - (window + 1)];
        int is_space = ((this_char == UCHAR_SP) || (this_char == UCHAR_TAB));
        int was_space;

        /* identify the best fold location in the bottom half of the window */
        for (diff = -window; diff; diff += 1) {
            int category;

            was_space = is_space;
            this_char = fold_start[target_length + diff];
            is_space = ((this_char == UCHAR_SP) || (this_char == UCHAR_TAB));

            category = (allow_folding_before_semi || (this_char != UCHAR_SEMI))
                    ? ((was_space * 2) + !is_space)
                    : 0;

            if (category >= best_category) {
                best_diff = diff;
                best_category = category;
            }
        }

        /* look for a better fold location in the top half of the window */
        for (; diff <= window; diff += 1) {
            int category;

            was_space = is_space;
            this_char = fold_start[target_length + diff];
            is_space = ((this_char == UCHAR_SP) || (this_char == UCHAR_TAB));

            category = (allow_folding_before_semi || (this_char != UCHAR_SEMI))
                    ? ((was_space * 2) + !is_space)
                    : 0;

            if (category == 3) {
                /* it doesn't get any better than this */
                best_diff = diff;
                break;
            } else if (category > best_category) {
                best_diff = diff;
                best_category = category;
            } else if ((category == best_category) && (diff <= -best_diff)) {
                best_diff = diff;
                best_category = category;
            }
        }

        if (best_category) {
            /* A viable fold location was found */
            return target_length + best_diff;
        } else {
            /*
             * All characters in the target window are semicolons, and we must not fold before a semicolon.
             * Scan backward in the string to find a viable fold location
             */
            ptrdiff_t best_length;

            for (best_length = target_length - (window + 1); best_length > 0; best_length -= 1) {
                if (fold_start[best_length] != UCHAR_SEMI) {
                    break;
                }
            }

            return best_length;
        }
    }
}

void whitespace_callback(size_t line UNUSED, size_t column UNUSED, const UChar *ws, size_t length, void *data) {
    /* TODO: preserve comments, possibly strip other whitespace */
#if 0
    /* Note: this initializer may be subject to the selected source character set: */
    static const UChar cif_header_start[] = { '#', '\\', '#', 'C', 'I', 'F', '_', 0 };
    static const UChar standard_eol[] = { UCHAR_LF, UCHAR_CR, 0 };
    const UChar *extra_eol = CONTEXT_EXTRA_EOL(data);
    const UChar *extra_ws = CONTEXT_EXTRA_WS(data);
    int32_t n_written;
    int32_t tail_start;
    int32_t tail_len;
    UChar *ws_buf;

    /* suppress echoing any CIF header comment that appears at the beginning of the file */
    if (CONTEXT_AT_START(data)) {
        /*
         * Note: this implementation depends on undocumented CIF API behavior of providing the whole header
         * comment in one callback call.  It might one day break.
         */
        UChar *substr = u_strstr(ws, cif_header_start);

        if (!substr || (substr == ws)) {
            return;
        }
    }

    /* make a nul-terminated copy of the data */
    ws_buf = (UChar *) malloc((length + 1) * sizeof(*ws_buf));
    u_strncpy(ws_buf, ws, length);
    ws_buf[length] = 0;

    /*
     * CIF 1.0 output is not supported, so convert any *expected* unsupported characters to others with similar
     * semantics.
     */
    if (extra_eol || extra_ws) {
        translate_whitespace(ws_buf, length, extra_eol, extra_ws);
    }

    n_written = u_fprintf(CONTEXT_OUT(data), "%*S", (int32_t) length, ws_buf);
    assert(n_written == length);

    /* track position */
    for (tail_start = 0, tail_len = length; tail_len; ) {
        int32_t head_len = u_strcspn(ws_buf + tail_start, standard_eol);

        if (head_len == tail_len) {
            break;
        } else {
            tail_start += (head_len + 1);
            tail_len -= (head_len + 1);
        }
    }

    if (tail_len == length) {
        /* no line terminator */
        CONTEXT_INC_COLUMN(data, tail_len);
    } else {
        CONTEXT_SET_COLUMN(data, tail_len);
    }

    /* clean up */
    free(ws_buf);
#endif

    CONTEXT_SET_STARTED(data);
}

/*
 * A callback function by which whitespace (including comments) in the input CIF is handled.  This version echoes
 * input whitespace to the output
 */
int error_callback(int code, size_t line, size_t column, const UChar *text, size_t length, void *data) {
    UFILE *ustderr = CONTEXT_ERR(data);

    CONTEXT_INC_ERROR_COUNT(data);

    if (!CONTEXT_QUIET(data)) {
        if ((0 <= code) && (code <= cif_nerr)) {
            u_fprintf(ustderr, "CIF error %d at line %lu, column %lu, (near '%*S'): %s\n", code, (uint32_t) line,
                    (uint32_t) column, (int32_t) length, text, cif_errlist[code]);
        } else {
            u_fprintf(ustderr, "CIF error %d at line %lu, column %lu, (near '%*S'): (unknown error code)\n", code,
                    (uint32_t) line, (uint32_t) column, (int32_t) length, text);
        }
    }

    if (CONTEXT_HALT_ON_ERROR(data)) {
        return code;
    } else {
        /* TODO: corrective output in some cases (?) */
        return CIF_OK;
    }
}

void keyword_callback(size_t line UNUSED, size_t column UNUSED, const UChar *keyword, size_t length, void *data) {
    /* TODO: implementing this callback will help with preserving comments in the input  */
}

void dataname_callback(size_t line UNUSED, size_t column UNUSED, const UChar *dataname, size_t length, void *data) {
    /* TODO: implementing this callback will help with preserving comments in the input  */
}

/* function intended to handle cif_start events */
int print_code(cif_container_tp *container, void *context, const char *type) {
    UChar *code;
    uint32_t nchars;
    int result;

    if ((result = cif_container_get_code(container, &code)) == CIF_OK) {
        if ((nchars = u_fprintf(CONTEXT_OUT(context), "\n%s%S", type, code)) < 7) {
            result = CIF_ERROR;
        }
        CONTEXT_SET_COLUMN(context, nchars - 1);
        free(code);
        result = CIF_OK;
    }

    return result;
}

/*
 * Removes all data from the specified container
 */
int flush_loops(cif_container_tp *container) {
    int result;
    cif_loop_tp **loops;

    if ((result = cif_container_get_all_loops(container, &loops)) == CIF_OK) {
        cif_loop_tp **loop;

        for (loop = loops; *loop; loop += 1) {
            if ((result = cif_loop_destroy(*loop)) != CIF_OK) {
                break;
            }
        }

        /* clean up in case of error */
        for (; *loop; loop += 1) {
            cif_loop_free(*loop);
        }

        free(loops);
    }

    return result;
}

int ensure_space(int data_length, void *context) {
    int result = CIF_ERROR;

    if (CONTEXT_COLUMN(context) > 0) {
        if ((1 + data_length + CONTEXT_COLUMN(context)) > MAX_LINE_LENGTH) {
            if (u_fprintf(CONTEXT_OUT(context), "\n") == 1) {
                CONTEXT_SET_COLUMN(context, 0);
                result = CIF_OK;
            }
        } else {
            if (u_fprintf(CONTEXT_OUT(context), " ") == 1) {
                CONTEXT_INC_COLUMN(context, 1);
                result = CIF_OK;
            }
        }
    } else {
        result = CIF_OK;
    }

    return result;
}

/*
 * Prints a literal string to the output, possibly preceded by a newline or space.  Updates the context's current
 * column according to the specified length of the first line; callers will need to correct the column after printing
 * a multi-line string.
 */
int print_u_literal(int preceding_space, const UChar *str, int line1_length, void *context) {
    int32_t nprinted;

    if (CONTEXT_COLUMN(context)) {
        int32_t nspace = ((preceding_space > 0) ? preceding_space : 0);
        int need_newline = (line1_length + CONTEXT_COLUMN(context) + nspace) > MAX_LINE_LENGTH;

        if (need_newline) {
            if (preceding_space < 0) {
                return CIF_OVERLENGTH_LINE;
            } else {
                if ((nprinted = u_fprintf(CONTEXT_OUT(context), "\n%S", str)) < 1) {
                    return CIF_ERROR;
                }
                CONTEXT_SET_COLUMN(context, nprinted - 1);
            }
        } else {
            if ((nprinted = u_fprintf(CONTEXT_OUT(context), "%*s%S", nspace, "", str)) < nspace) {
                return CIF_ERROR;
            }
            CONTEXT_INC_COLUMN(context, nprinted);
        }
    } else {
        /* already at the beginning of a line */
        if ((nprinted = u_fprintf(CONTEXT_OUT(context), "%S", str)) < 0) {
            return CIF_ERROR;
        }
        CONTEXT_SET_COLUMN(context, nprinted);
    }

    return CIF_OK;
}

/*
 * A program to convert among various dialects of CIF
 */
int print_delimited(UChar *str, UChar *delim, int delim_length, UFILE *out) {
    return (u_fprintf(out, "%S%S%S", delim, str, delim) >= delim_length * 2) ? CIF_OK : CIF_ERROR;
}

void translate_whitespace(UChar *text, size_t length, const UChar *extra_eol, const UChar *extra_ws) {
    if (length == (size_t) -1) {
        length = u_strlen(text);
    }
    if (extra_eol) {
        size_t span_start = 0;

        for (;;) {
            span_start += u_strcspn(text + span_start, extra_eol);
            if (span_start >= length) {
                break;
            } else {
                text[span_start++] = (UChar) 0x0a; /* newline */
            }
        }
    }

    if (extra_ws) {
        size_t span_start = 0;

        for (;;) {
            span_start += u_strcspn(text + span_start, extra_ws);
            if (span_start >= length) {
                break;
            } else {
                text[span_start++] = (UChar) 0x20; /* space */
            }
        }
    }
}

int main(int argc, char *argv[]) {
    struct cif_parse_opts_s *parse_opts;
    struct context context;
    cif_handler_tp handler = {
        print_header,      /* cif start */
        handle_cif_end,    /* cif end */
        open_block,        /* block start */
        flush_container,   /* block end */
        open_frame,        /* frame start */
        finish_frame,      /* frame end */
        handle_loop_start, /* loop_start */
        handle_loop_end,   /* loop end */
        start_packet_on_new_line, /* packet start */
        discard_packet,    /* packet end */
        print_item         /* item */
    };
    cif_tp *cif = NULL;
    int result;
    int ignored;

    memset(&context, 0, sizeof(context));

    if (cif_parse_options_create(&parse_opts) != CIF_OK) {
        exit(2);
    }

    parse_opts->handler = &handler;
    parse_opts->user_data = &context;
    process_args(argc, argv, parse_opts);
    assert(context.out && context.ustderr);

    result = cif_parse(stdin, parse_opts, &cif);

    if (result != CIF_OK) {
        handle_cif_end(cif, &context);
    }
    ignored = cif_destroy(cif);
    free(parse_opts);

    /*
     * The handler already closed the Unicode output and error streams
     */

    /*
     * Exit codes:
     *  3 - parse aborted because of errors
     *  2 - parse skipped (from function usage())
     *  1 - parse completed with errors
     *  0 - parse completed without errors
     */
    return (result != CIF_OK) ? 3 : (context.error_count ? 1 : 0);
}
