/*
 * Bison definitions for a CIF 2.0 parser skeleton
 *
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Bison options specify a reentrant (pure) parser accepting an input stream and a (dummy) scanner pointer as arguments
 * to its main function.  It will interface with a lexical analyzer that takes an additional "scanner" argument of type
 * void *, and it will produce verbose error messages and track location within the input.
 *
 * The "scanner" parse parameter is a bit of a hack -- its value on entry to the parse routine is ignored and can be
 * NULL -- it is the only means Bison provides to inject a local variable scoped to the whole parse function.
 */
%pure-parser
%parse-param { FILE *input_file }
%parse-param { void *scanner }
%lex-param   { void *scanner }
%error-verbose
%locations

%{
#include <stdio.h>

/* function prototypes, most of them for the lexical analyzer interface */
int yylex(void *lvalp, void *llocp, void *scanner);
int yylex_init(void **);
int yylex_destroy(void *);
void yyset_in(FILE *in, void *scanner);
void yyerror(void *locp, FILE *input_file, void *scanner, const char *message);
%}

%initial-action {
    /* initialize a reentrant scanner and point it at the provided input stream */
    int lex_init_result = yylex_init(&scanner);

    if (lex_init_result != 0) return lex_init_result;
    yyset_in(input_file, scanner);
}

/* terminal tokens in the generated parser's grammar */
%token BLOCK_HEADER
%token FRAME_HEADER
%token FRAME_TERMINATOR
%token LOOP
%token DATANAME
%token LIST_OPEN
%token LIST_CLOSE
%token TABLE_OPEN
%token TABLE_CLOSE
%token KEY
%token TEXT_FIELD
%token SIMPLE_VALUE
%token VCOMMENT
%token WS

/*
 * The grammar's start symbol -- that is, the non-terminal symbol that represents the complete grammatical construct
 * the parser will attempt to construct.
 */
%start cif

%%

/*
 * An overall CIF (v2.0) starts with a version-identification comment, and may thereafter contain zero or more data
 * blocks delimited from the initial comment by whitespace.
 */
cif: VCOMMENT delim_blocks { yylex_destroy(scanner); }
  ;

/*
 * either nothing or whitespace followed by zero or more data blocks
 */
delim_blocks: /* empty */
    | whitespace
    | whitespace data_blocks
  ;

/*
 * One or more data blocks.  Because data blocks always end with whitespace, there is no need to provide for explicit
 * whitespace separation or to make special allowance for trailing whitespace.
 */
data_blocks: data_block
    | data_blocks data_block
  ;

/*
 * A data block consists of a block header and zero or more subsequent save frames, items, and loops, all separated
 * from each other by whitespace, with trailing whitespace.
 * Note: the requirement for trailing whitespace is artificial, though very commonly fulfilled in practice.  The
 * lexical analyzer is responsible for ensuring that it reports whitespace at the end of the input, whether there
 * actually is any or not, unless there are no data blocks.
 */
data_block: BLOCK_HEADER whitespace
    | data_block save_frame whitespace
    | data_block item whitespace
    | data_block loop                  /* loops already provide trailing whitespace */
  ;

/*
 * A save frame is constructed much the same as a data block, but may not contain nested save frames and has an
 * explicit terminator.
 */
save_frame:  frame_body FRAME_TERMINATOR
  ;

frame_body: FRAME_HEADER whitespace
    | frame_body save_frame whitespace
    | frame_body item whitespace
    | frame_body loop                  /* loops already provide trailing whitespace */
  ;

/*
 * Data items consist of a tag (data name) and a value, separated from each other by whitespace.  If the value is a
 * text field, then the initial newline of its opening delimiter can do double duty as the separator.
 */
item:  DATANAME delim_value
  ;

/*
 * A loop consists of a header followed by a body with trailing whitespace
 */
loop: loop_header delim_lb
  ;

/*
 * A loop header consists of a loop_ keyword followed by a sequence of one or more tags, all separated from each other
 * by whitespace.
 */
loop_header: LOOP whitespace DATANAME
    | loop_header whitespace DATANAME
  ;

/*
 * A loop body consists of a sequence of data values, each separated from whatever precedes it by whitespace, with 
 * trailing whitespace.
 */
delim_lb: loop_body whitespace
  ;

loop_body: delim_value
    | loop_body TEXT_FIELD
    | delim_lb TEXT_FIELD
    | delim_lb non_delim_value
  ;

/*
 * A value of type LIST consists of a sequence of zero or more whitespace-separated values (of any type) delimited by
 * matching square brackets. Whitespace is optional between brackets and the values within, and between the brackets of
 * an empty list.
 */
list: LIST_OPEN list_values lclose
    | LIST_OPEN lclose
  ;

lclose: LIST_CLOSE
    | whitespace LIST_CLOSE
  ;

list_values: any_value
    | list_values delim_value
  ;

/*
 * A value of type TABLE consists of a sequence of zero or more whitespace-separated entries delimited by matching
 * curly braces. Whitespace is optional between brackets and the entries within, and between the braces of an empty
 * table.
 */
table: topen entries tclose
    |  topen TABLE_CLOSE
  ;

topen: TABLE_OPEN
    | TABLE_OPEN whitespace
  ;

tclose: TABLE_CLOSE
    | whitespace TABLE_CLOSE
  ;

entries: entry
    | entries whitespace entry
  ;

/*
 * Table entries consist of a key and a value (of any type), optionally separated by whitespace.
 */
entry: KEY any_value
  ;

any_value: delim_value
    | non_delim_value
;

/*
 * In most contexts, values need to be delimited from whatever precedes them by whitespace.  The initial newline of
 * a text field's leading delimiter can do double duty as separating whitespace, but additional whitespace is allowed
 * before a text field, too.
 */
delim_value: whitespace non_delim_value
    | whitespace TEXT_FIELD
    | TEXT_FIELD
  ;

/*
 * Most values do not have inherent leading separators.  These include bare values, quoted string values, lists, and
 * tables.
 */
non_delim_value: SIMPLE_VALUE
    | list
    | table
  ;

/*
 * The lexical analyzer is permitted to split runs of whitespace into multiple tokens
 */
whitespace: WS
    | whitespace WS
  ;

%%

/*
 * A function called by the parser to report grammar errors.  It is non-static, so it is available for routines to call
 * as well -- for instance, the lexical analyzer.
 *
 * The 'input_file' and 'scanner' arguments are ignored by this implementation; the parse routine expects to provide
 * them as a result of having them declared among its own parameters.  The 'lp' parameter is ignored if NULL, and
 * otherwise is expected to be a pointer to a structure describing the location of the error within the input, as
 * maintained by the parser's location-tracking code.
 */
void yyerror(void *lp, FILE *input_file, void *scanner, const char *msg) {
    YYLTYPE *locp = (YYLTYPE *) lp;

    if (locp == NULL) {
        fprintf(stderr, "%s\n", msg);
    } else {
        fprintf(stderr, "near line %d, column %d: %s\n", locp->first_line, locp->first_column, msg);
    }
}

