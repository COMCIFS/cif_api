/*
 * parser.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 */

/**
 * @page parsing CIF parsing
 * @tableofcontents
 * The CIF API provides a parser for CIF 2.0, flexible enough to parse CIF 1.1 documents as well.  Inasmuch
 * as most CIFs conforming to the less formal conventions predating the CIF 1.1 specification can be successfully parsed
 * as CIF 1.1, the parser will handle substantially all CIFs conforming to any current or historic CIF specification.
 *
 * @section versions CIF versions
 * To date there have been two formal specifications for the CIF file format, v1.1 and v2.0, and a body of less formal
 * practice predating both.  Each specification so far has introduced incompatibilities with preceding practice, so
 * correct parsing depends on correctly identifying the version of CIF with which a given document is intended to
 * conform.
 *
 * @subsection version_0 Early CIF
 * Before the release of the CIF 1.1 specifications, CIF format was defined by the Hall, Allen, and Brown's 1991 paper
 * (Acta Cryst A47, 655-685).  It somewhat loosely defines CIF as being composed of "ASCII text", arranged in lines not
 * exceeding 80 characters, with a vague, mostly implicit, sense of some kind of whitespace separating syntactic units
 * (block headers, data names, etc.).  The paper makes a point that CIF files are easily readable and suitable for
 * creation or modification via a text editor.  There were no save frames, and there was some confusion about exactly
 * which characters were allowed (in particular, about which of the C0 controls were allowed, and what they meant).  The
 * vertical tab and form feed characters were accepted by most CIF processors, and the prevailing practice was to treat
 * the latter as a line terminator.
 *
 * During this time, followup papers refined some of the initial CIF ideas, and ultimately IUCr attempted to bring a
 * little more order to the early CIF world.  Significant during this time was limiting the allowed characters in a CIF
 * to the printable subset of those defined by (7-bit) US-ASCII, plus the tab, vertical tab, form feed, carriage
 * return, and line feed control characters (albeit not necessarilly @em encoded according to ASCII).  This period was
 * also marked by the development of semantic conventions, on top of basic CIF, for expressing information such as a
 * limited set of non-ASCII characters and logical lines longer than the 80-character limit.
 *
 * @subsection version_1 CIF 1.1
 * The CIF 1.1 specifications were initially drafted in 2002, as a more formal treatment of the language and to
 * address some of the issues that the first ten+ years of CIF practice had uncovered.  This revision deleted the
 * vertical tab and form feed characters from the allowed set and extended the line-length limit to 2048 characters.
 * It also, among other things, imposed a 75-character limit on data name, block code, and frame code lengths; allowed
 * save frames (which meanwhile had been added to STAR, along with @c global_ sections), but not save frame references;
 * formally excluded global_ sections (and reserved the "global_" keyword); and restricted the characters that may
 * start an unquoted data value.  Significantly, it also introduced the leading CIF version comment, though it is
 * optional in CIF 1.1.  With few exceptions, CIF 1.1 did not invalidate pre-existing CIFs.
 *
 * @subsection version_2 CIF 2.0
 * The primary impetus for another CIF revision arose from the concept of adding "methods" to CIF dictionaries, while
 * continuing to express those dictionaries in CIF format.  This necessitated support for new data types (list and
 * table) in CIF, and the opportunity was taken to also extend CIF's character repertoire to the whole Unicode space.
 * CIF 2.0 also solves the longstanding issue that even restricting characters to 7-bit ASCII, there are data that
 * cannot be expressed as CIF 1.1 values.
 *
 * The cost of these changes is a considerably higher level of incompatibility between CIF 2.0 and CIF 1.1 than between
 * CIF 1.1 and previous CIF practice, as reflected by incrementing CIF's major version number.  Some data value
 * representations known to appear in well-formed CIF 1.1 documents are not well-formed CIF 2.0; moreover, CIF 2.0
 * specifies that instance documents must have the form of Unicode text encoded according to UTF-8, which, ironically,
 * makes it a @em binary format -- albeit one that can do a good impression of a text format on many systems.  To
 * assist in sorting this out, CIF 2.0 requires an appropriate version comment to be used in well-formed documents
 * (itself a minor incompatibility).
 *
 * @section parsing-details The CIF API parsing interface
 * The main interface provided for CIF parsing is the @c cif_parse() function, which reads CIF text from a standard C
 * byte stream, checks and interprets the syntax, and optionally records the parsed results in an in-memory CIF object.
 * A variety of options and extension points are available via the @c options argument to the parse function, currently
 * falling into two broad categories:
 * @li options controlling the CIF dialect to be parsed, and
 * @li callback functions by which the caller can obtain dynamic information about the progress of the parse, and
 *         exert influence on parsing behavior
 *
 * These afford a great deal of parsing flexibility, some dimensions of which are explored in the following sections.
 *
 * @subsection basic-parsing Basic parsing
 * Historically, a majority of CIF 1.1 parsers have operated by parsing the input into some kind of in-memory
 * representation of the overall CIF, possibly, but not necessarily, independent of the original file.  The
 * @c cif_parse() function operates in this way when its third argument points to a location for a CIF handle: @code
 * void traditional(FILE *in) {
 *     cif_t *cif = NULL;
 *
 *     cif_parse(in, NULL, &cif);
 *     // results are available via 'cif' if anything was successfully parsed
 *     cif_destroy(cif);  // safe even if 'cif' is still NULL
 * }
 * @endcode
 *
 * By default, however, the parser stops at the first error it encounters.  Inasmuch as very many CIFs contain at least
 * minor errors, it may be desirable to instruct the parser to attempt to push past all or certain kinds of errors,
 * extracting a best-guess interpretation of the remainder of the input.  Such behavior can be obtained by providing an
 * error-handling callback function of type matching @c cif_parse_error_callback_t .  Such a function serves not only
 * to control which errors are bypassed, but also, if so written, to pass on details of each error to the caller.  For
 * example, this code counts the number of CIF syntax and semantic errors in the input CIF: @code
 * int record_error(int error_code, size_t line, size_t column, const UChar *text, size_t length, void *data) {
 *     ((int *) data) += 1;
 *     return CIF_OK;
 * }
 * void count_errors(FILE *in) {
 *     cif_t *cif = NULL;
 *     int num_errors = 0;
 *     struct cif_parse_opts_s *opts = NULL;
 *
 *     cif_parse_options_create(&opts);
 *     opts->error_callback = record_error;
 *     opts->user_data = &num_errors;
 *     cif_parse(in, opts, &cif);
 *     free(opts);
 *     // the parsed results are available via 'cif'
 *     // the number of errors is available in 'num_errors'
 *     // ...
 *     cif_destroy(cif);
 * }
 * @endcode
 * 
 * For convenience, the CIF API provides two default error handler callback functions, @c cif_parse_error_die() and
 * @c cif_parse_error_ignore().  As their names imply, the former causes the parse to be aborted on any error (the
 * default behavior), whereas the latter causes all errors to silently be ignored (to the extent that is possible).
 *
 * @subsection parser-callbacks Parser callbacks
 * Parsing a CIF from an external representation is in many ways analogous to performing a depth-first traversal of
 * a pre-parsed instance of the CIF data model, as the @c cif_walk() function does.  In view of this similarity, a @c
 * cif_handler_t object such as is also used with @c cif_walk() can be provided among the parse options to facilitate
 * caller monitoring and control of the parse process.  The handler callbacks can probe and to some
 * extent modify the CIF as it is parsed, including by instructing the parser to suppress (but not altogether skip)
 * some portions of the input.  This facility has applications from parse-time data selection to validation and beyond;
 * for example, here is a naive approach to assigning loop categories based on loop data names: @code
 * int assign_loop_category(cif_loop_t *loop, void *context) {
 *     UChar **names;
 *     UChar **next;
 *     UChar *dot_location;
 *     const UChar unicode_point = 0x2E;
 *
 *     // We can rely on at least one data name
 *     cif_loop_get_names(loop, &names);
 *     // Assumes the name contains a decimal point (Unicode code point U+002E), and
 *     // takes the category as everything preceding it.  Ignores case sensitivity considerations.
 *     dot_location = u_strchr(names[0] + 1, unicode_point);
 *     *dot_location = 0;
 *     cif_loop_set_category(loop, names[0]);
 *     // Clean up
 *     for (next = names; *next != NULL; next += 1) {
 *         free(*next);
 *     }
 *     free(names);
 * }
 *
 * void parse_with_categories(FILE *in) {
 *     cif_t *cif = NULL;
 *     struct cif_parse_opts_s *opts = NULL;
 *     cif_handler_t handler = { NULL, NULL, NULL, NULL, NULL, NULL, assign_loop_category, NULL, NULL, NULL, NULL };
 *
 *     cif_parse_options_create(&opts);
 *     opts->handler = &handler;
 *     cif_parse(in, opts, &cif);
 *     free(opts);
 *     // the parsed results are available via 'cif'
 *     // ...
 *     cif_destroy(cif);
 * }
 * @endcode
 *
 * Note that the parser traverses its input and issues callbacks in document order from start to end, so unlike
 * @c cif_walk(), it does not guarantee to traverse all of a data block's save frames before any of its data.
 *
 * @subsubsection validation CIF validation
 * The CIF API does not provide specific support for CIF validation because validation is is dependent on the DDL of
 * the dictionary to which a CIF purports to comply, whereas the CIF API is generic, not specific to any particular
 * DDL or dictionary.  To the extent that some validations can be performed during parsing, however, callback functions
 * provide a suitable means to engage such validation.
 *
 * @subsubsection comments Comments
 * For the most part, the parser ignores CIF comments other than for attempting to identify the CIF
 * version with which its input purports to comply.  In the event that the caller wants to be informed of comments and
 * other whitespace, however, there is among the parse options a pointer to a callback function for that purpose.  Its
 * use is analogous to the callbacks already discussed.
 *
 * @subsection syntax-only Syntax-only parsing
 * For some applications, it might not be desirable or even feasible to collect parsed CIF content into an
 * in-memory representation.  The simplest such application performs only a syntax check of the input -- perhaps to
 * test compliance with a particular CIF version and/or parse options.  The parser function operates in just
 * that way when its third argument is NULL (that is, when the caller provides no CIF handle location).
 * All provided callbacks are invoked as normal in this mode, including any error callback, and in the absence of any
 * callbacks the parser's return code indicates whether any errors were detected.  This syntax-only parsing mode does
 * have a few limitations, however -- primarily that because it does not retain an in-memory representation of its
 * input, it cannot check CIF semantic requirements for data name, frame code, and block code uniqueness within their
 * respective scopes.
 *
 * @subsection event-driven Event-driven parsing
 * The availability and scope of callback functions make the syntax-only mode described above a CIF analog of the
 * event-driven "SAX" XML parsing interface.  To use the parser in that mode, the caller provides callback functions
 * by which to be informed of parse "events" of interest -- recognition of entities and entity boundaries -- so as to
 * extract the desired information during the parse instead of by afterward analyzing the parse result.  Callbacks can
 * communicate with themselves and each other, and can memorialize data for the caller, via the @c user_data object
 * provided among the parse options (and demonstrated in the error-counting example).  Callbacks can be omitted
 * for events that are not of interest.
 */

/*
 * The CIF parser implemented herein is a predictive recursive descent parser with full error recovery.
 */

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
#include "internal/utils.h"

/* plain macros */

#define SSIZE_T_MAX      (((size_t) -1) / 2)
#define BUF_SIZE_INITIAL (4 * (CIF_LINE_LENGTH + 2))
#define BUF_MIN_FILL          (CIF_LINE_LENGTH + 2)

/* specific character codes */
/* Note: numeric character codes avoid codepage dependencies associated with use of ordinary char literals */
#define TAB_CHAR      0x09
#define LF_CHAR       0x0A
#define VT_CHAR       0x0B
#define FF_CHAR       0x0C
#define CR_CHAR       0x0D
#define DECIMAL_CHAR  0x2E
#define COLON_CHAR    0x3A
#define SEMI_CHAR     0x3B
#define QUERY_CHAR    0x3F
#define BKSL_CHAR     0x5C
#define CIF1_MAX_CHAR 0x7E
#define BOM_CHAR      0xFEFF
#define EOF_CHAR      0xFFFF

#if CHAR_TABLE_MAX <= CIF1_MAX_CHAR
#error "character class table is too small"
#endif

/* character metaclass codes */
#define NO_META       0
#define GENERAL_META  1
#define WS_META       2
#define OPEN_META     3
#define CLOSE_META    4

/* static data */

#define MAGIC_LENGTH 10
static const UChar CIF1_MAGIC[MAGIC_LENGTH]
        = { 0x23, 0x5c, 0x23, 0x43, 0x49, 0x46, 0x5f, 0x31, 0x2e, 0x31 }; /* #\#CIF_1.1 */
static const UChar CIF2_MAGIC[MAGIC_LENGTH]
        = { 0x23, 0x5c, 0x23, 0x43, 0x49, 0x46, 0x5f, 0x32, 0x2e, 0x30 }; /* #\#CIF_2.0 */


/* static function headers */

/* grammar productions */
static int parse_cif(struct scanner_s *scanner, cif_t *cifp);
static int parse_container(struct scanner_s *scanner, cif_container_t *container, int is_block);
static int parse_item(struct scanner_s *scanner, cif_container_t *container, UChar *name);
static int parse_loop(struct scanner_s *scanner, cif_container_t *container);
static int parse_loop_header(struct scanner_s *scanner, cif_container_t *container, string_element_t **name_list_head,
        int *name_countp);
static int parse_loop_packets(struct scanner_s *scanner, cif_loop_t *loop, string_element_t *first_name,
        UChar *names[]);
static int parse_list(struct scanner_s *scanner, cif_value_t **listp);
static int parse_table(struct scanner_s *scanner, cif_value_t **tablep);
static int parse_value(struct scanner_s *scanner, cif_value_t **valuep);

/* scanning functions */
static int next_token(struct scanner_s *scanner);
static int scan_ws(struct scanner_s *scanner);
static int scan_to_ws(struct scanner_s *scanner);
static int scan_to_eol(struct scanner_s *scanner);
static int scan_unquoted(struct scanner_s *scanner);
static int scan_delim_string(struct scanner_s *scanner);
static int scan_text(struct scanner_s *scanner);
static int get_first_char(struct scanner_s *scanner);
static int get_more_chars(struct scanner_s *scanner);

/* other functions */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_t **dest);

/* function-like macros */

#define INIT_V2_SCANNER(s) do { \
    struct scanner_s *_s = (s); \
    int _i; \
    _s->line = 1; \
    _s->column = 0; \
    _s->ttype = END; \
    _s->line_unfolding += 1; \
    _s->prefix_removing += 1; \
    _s->skip_depth = 0; \
    for (_i =   0; _i <  32;            _i += 1) _s->char_class[_i] = NO_CLASS; \
    for (_i =  32; _i < 127;            _i += 1) _s->char_class[_i] = GENERAL_CLASS; \
    for (_i = 128; _i < CHAR_TABLE_MAX; _i += 1) _s->char_class[_i] = NO_CLASS; \
    for (_i =   1; _i <= LAST_CLASS;    _i += 1) _s->meta_class[_i] = GENERAL_META; \
    _s->char_class[TAB_CHAR] =  WS_CLASS; \
    _s->char_class[LF_CHAR] =  EOL_CLASS; \
    _s->char_class[CR_CHAR] =  EOL_CLASS; \
    _s->char_class[0x20] =   WS_CLASS; \
    _s->char_class[0x22] =   QUOTE_CLASS; \
    _s->char_class[0x23] =   HASH_CLASS; \
    _s->char_class[0x24] =   DOLLAR_CLASS; \
    _s->char_class[0x27] =   QUOTE_CLASS; \
    _s->char_class[COLON_CHAR] = GENERAL_CLASS; \
    _s->char_class[SEMI_CHAR] = SEMI_CLASS; \
    _s->char_class[0x5B] =   OBRAK_CLASS; \
    _s->char_class[0x5D] =   CBRAK_CLASS; \
    _s->char_class[0x5F] =   UNDERSC_CLASS; \
    _s->char_class[0x7B] =   OCURL_CLASS; \
    _s->char_class[0x7D] =   CCURL_CLASS; \
    _s->char_class[0x41] =   A_CLASS; \
    _s->char_class[0x61] =   A_CLASS; \
    _s->char_class[0x42] =   B_CLASS; \
    _s->char_class[0x62] =   B_CLASS; \
    _s->char_class[0x44] =   D_CLASS; \
    _s->char_class[0x64] =   D_CLASS; \
    _s->char_class[0x45] =   E_CLASS; \
    _s->char_class[0x65] =   E_CLASS; \
    _s->char_class[0x47] =   G_CLASS; \
    _s->char_class[0x67] =   G_CLASS; \
    _s->char_class[0x4C] =   L_CLASS; \
    _s->char_class[0x6C] =   L_CLASS; \
    _s->char_class[0x4F] =   O_CLASS; \
    _s->char_class[0x6F] =   O_CLASS; \
    _s->char_class[0x50] =   P_CLASS; \
    _s->char_class[0x70] =   P_CLASS; \
    _s->char_class[0x53] =   S_CLASS; \
    _s->char_class[0x73] =   S_CLASS; \
    _s->char_class[0x54] =   T_CLASS; \
    _s->char_class[0x74] =   T_CLASS; \
    _s->char_class[0x56] =   V_CLASS; \
    _s->char_class[0x76] =   V_CLASS; \
    _s->char_class[0x7F] =   NO_CLASS; \
    _s->meta_class[NO_CLASS] =  NO_META; \
    _s->meta_class[WS_CLASS] =  WS_META; \
    _s->meta_class[EOL_CLASS] = WS_META; \
    _s->meta_class[EOF_CLASS] = WS_META; \
    _s->meta_class[OBRAK_CLASS] = OPEN_META; \
    _s->meta_class[OCURL_CLASS] = OPEN_META; \
    _s->meta_class[CBRAK_CLASS] = CLOSE_META; \
    _s->meta_class[CCURL_CLASS] = CLOSE_META; \
} while (CIF_FALSE)

/*
 * Adjusts an initialized scanner for parsing CIF 1 instead of CIF 2.  Should not be applied multiple times to the
 * same scanner.
 */
#define SET_V1(s) do { \
    struct scanner_s *_s = (s); \
    _s->line_unfolding -= 1; \
    _s->prefix_removing -= 1; \
    _s->char_class[0x5B] =   OBRAK1_CLASS; \
    _s->char_class[0x5D] =   CBRAK1_CLASS; \
    _s->char_class[0x7B] =   GENERAL_CLASS; \
    _s->char_class[0x7D] =   GENERAL_CLASS; \
} while (CIF_FALSE)

/*
 * @brief Determines the class of the specified character based on the specified scanner ruleset.
 *
 * @param[in] c the character whose class is requested.  Must be non-negative.  May be evaluated more than once.
 * @param[in] s a struct scanner_s containing the rules defining the character's class.
 */
/*
 * There are additional code points that perhaps should be mapped to NO_CLASS in CIF 2: BMP not-a-character code points
 * 0xFDD0 - 0xFDEF, and per-plane not-a-character code points 0x??FFFE and 0x??FFFF except 0xFFFF (which is co-opted
 * as an EOF marker instead).  Also surrogate code units when not part of a surrogate pair.  At this time, however,
 * there appears little benefit to this macro making those comparatively costly distinctions, especially when it
 * cannot do so at all in the case of unpaired surrogates.
 */
#define CLASS_OF(c, s)  (((c) < CHAR_TABLE_MAX) ? (s)->char_class[c] : \
        (((c) == EOF_CHAR) ? EOF_CLASS : (((s)->cif_version >= 2) ? GENERAL_CLASS : NO_CLASS)))

/*
 * Evaluates the metaclass to which the class of the specified character belongs, with respect to the given scanner
 */
#define METACLASS_OF(c, s) ((s)->meta_class[CLASS_OF((c), (s))])

/* Expands to the length, in UChar code units, of the given scanner's current token */
#define TVALUE_LENGTH(s) ((s)->tvalue_length)

/* Sets the length, in UChar code units, of the given scanner's current token */
#define TVALUE_SETLENGTH(s, l) do { (s)->tvalue_length = (l); } while (CIF_FALSE)

/* Increments the length, in UChar code units, of the given scanner's current token */
#define TVALUE_INCLENGTH(s, n) do { (s)->tvalue_length += (n); } while (CIF_FALSE)

/* Expands to a pointer to the first UChar code unit in the given scanner's current token value */
#define TVALUE_START(s) ((s)->tvalue_start)

/* Sets a pointer to the first UChar code unit in the given scanner's current token value */
#define TVALUE_SETSTART(s, c) do { (s)->tvalue_start = (c); } while (CIF_FALSE)

/* Increments the given scanner's pointer to its current token value */
#define TVALUE_INCSTART(s, n) do { (s)->tvalue_start += (n); } while (CIF_FALSE)

/* expands to the value of the given scanner's current line number */
#define POSN_LINE(s) ((s)->line)

/* increments the given scanner's line number by the specified amount and resets its column number to zero */
#define POSN_INCLINE(s, n) do { struct scanner_s *_s = (s); _s->line += (n); _s->column = 0; } while (CIF_FALSE)

/* expands to the value of the given scanner's current column number */
#define POSN_COLUMN(s) ((s)->column)

/* increments the given scanner's column number by the specified amount */
#define POSN_INCCOLUMN(s, n) do { (s)->column += (n); } while (CIF_FALSE)

/*
 * Ensures that the buffer of the specified scanner has at least one character available to read; the available
 * character can be an EOF marker
 */
#define ENSURE_CHARS(s, ev) do { \
    struct scanner_s *_s_ec = (s); \
    if (_s_ec->next_char >= _s_ec->buffer + _s_ec->buffer_limit) { \
        ev = get_more_chars(_s_ec); \
        if (ev != CIF_OK) break; \
    } else { \
        ev = CIF_OK; \
    } \
} while (CIF_FALSE)

/*
 * Inputs the next available character from the scanner source into the given character without changing the relative
 * token start position or recorded token length.  The absolute token start position may change, however. 
 */
#define NEXT_CHAR(s, c, ev) do { \
    struct scanner_s *_s_nc = (s); \
    ENSURE_CHARS(_s_nc, ev); \
    c = *(_s_nc->next_char++); \
    POSN_INCCOLUMN(_s_nc, 1); \
} while (CIF_FALSE)

/*
 * Returns the character that will be returned (again) by the next invocation of NEXT_CHAR().  May cause additional
 * characters to be read into the buffer, but does not otherwise alter the scan state.
 */
#define PEEK_CHAR(s, c, ev) do { \
    struct scanner_s *_s_pc = (s); \
    ENSURE_CHARS(_s_pc, ev); \
    c = *(_s_pc->next_char); \
} while (CIF_FALSE)

/*
 * Backs up the scanner by one code unit.  Assumes the previous code unit is still available in the buffer.
 * Performs column accounting, but no line-number or token start/length accounting.
 */
#define BACK_UP(s) do { \
    struct scanner_s *_s_pbc = (s); \
    assert(_s_pbc->next_char > _s_pbc->buffer); \
    _s_pbc->next_char -= 1; \
    POSN_INCCOLUMN(_s_pbc, -1); \
} while (0)

/*
 * Consumes the current token, if any, from the scanner.  When next asked for a token, the scanner will provide
 * the subsequent one.
 */
#define CONSUME_TOKEN(s) do { \
    struct scanner_s *_s_ct = (s); \
    _s_ct->text_start = _s_ct->next_char; \
    TVALUE_SETSTART(_s_ct, _s_ct->next_char); \
    TVALUE_SETLENGTH(_s_ct, 0); \
} while (CIF_FALSE)

/*
 * Rejects the current token, if any, from the scanner, so that the same text is scanned again the next time a
 * token is requested.  This is useful only if the scanner is afterward modified so that the rejected token text
 * is scanned differently the next time.
 * s: the scanner
 * t: the type of the token immediately preceding the rejected one
 */
#define REJECT_TOKEN(s, t) do { \
    struct scanner_s *_s_rt = (s); \
    _s_rt->next_char = _s_rt->text_start; \
    TVALUE_SETSTART(_s_rt, _s_rt->next_char); \
    TVALUE_SETLENGTH(_s_rt, 0); \
    _s_rt->ttype = (t); \
} while (CIF_FALSE)

/*
 * Pushes back all but the specified number of initial characters of the current token to be scanned again as part of
 * the next token.  The result is undefined if more characters are pushed back than the current token contains.  The
 * length of the token value is adjusted so that the value extends to the new end of the remainder of the token.
 * s: the scanner in which to push back characters
 * n: the number of characters of the whole token text to keep (not just of the token value, if they differ)
 */
#define PUSH_BACK(s, n) do { \
    struct scanner_s *_s_pb = (s); \
    int32_t _n = (n); \
    _s_pb->next_char = _s_pb->text_start + _n; \
    TVALUE_SETLENGTH(_s_pb, _s_pb->next_char - TVALUE_START(_s_pb)); \
} while (CIF_FALSE)

/*
 * Advances the scanner by one code unit, tracking the current column number and watching for input malformations
 * associated with surrogate pairs: unpaired surrogates and pairs encoding Unicode code values not permitted in
 * CIF.  Surrogates have to be handled separately from character buffering in order to correctly handle surrogate
 * pairs that are split across buffer fills.
 * s: the scanner to manipulate
 * c: an lvalue in which the next code unit scanned will be returned
 * lead_fl: an lvalue of integral type wherein this macro can store state between runs; should evaluate to false
 *     before this macro's first run
 * ev: an lvalue in which an error code can be recorded
 */
#define SCAN_UCHAR(s, c, lead_fl, ev) do { \
    struct scanner_s *_s = (s); \
    int _surrogate_mask; \
    c = *(_s->next_char); \
    POSN_INCCOLUMN(_s, 1); \
    ev = CIF_OK; \
    _surrogate_mask = (c & 0xfc00); \
    if (_surrogate_mask == 0xdc00) { /* a trail surrogate */ \
        if (lead_fl) { \
            if (((c & 0xfffe) == 0xdffe) && ((*(_s->next_char - 1) & 0xfc3f) == 0xd83f)) { \
                /* error: disallowed supplemental character */ \
                ev = _s->error_callback(CIF_DISALLOWED_CHAR, _s->line, _s->column, _s->next_char - 1, 1, _s->user_data); \
                if (ev != 0) break; \
                /* recover by replacing with the replacement character and wiping out the lead surrogate */ \
                memmove(_s->text_start + 1, _s->text_start, (_s->next_char - _s->text_start)); \
                c = ((_s->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR); \
                *(_s->next_char) = c; \
                _s->text_start += 1; \
                _s->tvalue_start += 1; \
            } \
            POSN_INCCOLUMN(_s, -1); \
            lead_fl = CIF_FALSE; \
        } else { \
            /* error: unpaired trail surrogate */ \
            ev = _s->error_callback(CIF_INVALID_CHAR, _s->line, _s->column, _s->next_char, 1, _s->user_data); \
            if (ev != 0) break; \
            /* recover by replacing with the replacement character */ \
            c = ((_s->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR); \
            *(_s->next_char) = c; \
        } \
    } else { \
        if (lead_fl) { \
            /* error: unpaired lead surrogate preceding the current character */ \
            ev = _s->error_callback(CIF_INVALID_CHAR, _s->line, _s->column, _s->next_char - 1, 1, _s->user_data); \
            if (ev != 0) break; \
            /* recover by replacing the surrogate with the replacement character */ \
            *(_s->next_char - 1) = ((_s->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR); \
        } \
        lead_fl = (_surrogate_mask == 0xd800); \
    } \
    _s->next_char += 1; \
} while (CIF_FALSE)

/* function implementations */

/*
 * a parser error handler function that ignores all errors
 */
int cif_parse_error_ignore(int code UNUSED, size_t line UNUSED, size_t column UNUSED, const UChar *text UNUSED,
        size_t length UNUSED, void *data UNUSED) {
    return CIF_OK;
}

/*
 * a parser error handler function that rejects all erroneous CIFs.  Always returtns @c code.
 */
int cif_parse_error_die(int code, size_t line UNUSED, size_t column UNUSED, const UChar *text UNUSED,
        size_t length UNUSED, void *data UNUSED) {
    return code;
}

int cif_parse_internal(struct scanner_s *scanner, int not_utf8, cif_t *dest) {
    FAILURE_HANDLING;

    scanner->buffer = (UChar *) malloc(BUF_SIZE_INITIAL * sizeof(UChar));
    scanner->buffer_size = BUF_SIZE_INITIAL;
    scanner->buffer_limit = 0;

    if (scanner->buffer != NULL) {
        UChar c;

        INIT_V2_SCANNER(scanner);
        scanner->next_char = scanner->buffer;
        scanner->text_start = scanner->buffer;
        scanner->tvalue_start = scanner->buffer;
        scanner->tvalue_length = 0;

        /*
         * We must avoid get_more_chars() here (and also NEXT_CHAR, which uses it, because we want -- here only -- to
         * accept a byte-order mark.
         */
        FAILURE_VARIABLE = get_first_char(scanner);

        if (FAILURE_VARIABLE == CIF_OK) {
            int scanned_bom;

            c = *(scanner->next_char++);

            /* consume an initial BOM, if present, regardless of the actual source encoding */
            /* NOTE: assumes that the character decoder, if any, does not also consume an initial BOM */
            if ((scanned_bom = (c == BOM_CHAR))) {
                CONSUME_TOKEN(scanner);
                NEXT_CHAR(scanner, c, FAILURE_VARIABLE);
            }

            if (FAILURE_VARIABLE == CIF_OK) {
                /* If the CIF version is uncertain then use the CIF magic code, if any, to choose */
                if (scanner->cif_version <= 0) {
                    scanner->cif_version = (scanner->cif_version < 0) ? -scanner->cif_version : 1;
                    if (CLASS_OF(c, scanner) == HASH_CLASS) {
                        if ((FAILURE_VARIABLE = scan_to_ws(scanner)) != CIF_OK) {
                            DEFAULT_FAIL(early);
                        }
                        if (TVALUE_LENGTH(scanner) == MAGIC_LENGTH) {
                            if (u_strncmp(TVALUE_START(scanner), CIF2_MAGIC, MAGIC_LENGTH) == 0) {
                                scanner->cif_version = 2;
                            } else if (u_strncmp(TVALUE_START(scanner), CIF1_MAGIC, MAGIC_LENGTH - 3) == 0) {
                                /* recognize magic codes for all CIF versions other than 2.0 as CIF 1 */
                                scanner->cif_version = 1;
                            }
                        }
                    }
                }

                /* reset the scanner */
                scanner->next_char = scanner->text_start;
                scanner->column = 0;

                if (scanner->cif_version == 1) {
                    if (scanned_bom) {
                        /* error: disallowed CIF 1 character */
                        FAILURE_VARIABLE = scanner->error_callback(CIF_DISALLOWED_CHAR, 1, 0,
                                scanner->next_char - 1, 1, scanner->user_data);
                        /* recover, if necessary, by ignoring the problem */
                    }
                    SET_V1(scanner);
                } else if ((scanner->cif_version == 2) && (not_utf8 != 0)) {
                    /* error: CIF2 but not UTF-8 */
                    FAILURE_VARIABLE = scanner->error_callback(CIF_WRONG_ENCODING, 1, 1, scanner->next_char, 0,
                            scanner->user_data);
                    /* recover, if necessary, by ignoring the problem */
                }
                if (FAILURE_VARIABLE == CIF_OK) {
                    SET_RESULT(parse_cif(scanner, dest));
                }
            }
        }

        FAILURE_HANDLER(early):
        free(scanner->buffer);
    }

    GENERAL_TERMINUS;
}

/*
 * Calls a function via a function pointer, provided that the pointer is not NULL.  In that case, this macro evaluates
 * to the function return value.  Otherwise, it evaluates to the specified default return value.
 *
 * f: a pointer to the function to call; may be NULL
 * args: a parenthesized argument list for the function
 * default_return: the logical return value of the function when the pointer is NULL
 */
#define OPTIONAL_CALL(f,args,default_return) ((f == NULL) ? default_return : f args)

/*
 * Calls a function via a function pointer, provided that the pointer is not NULL.  The function's return value, if
 * any, is ignored.
 *
 * f: a pointer to the function to call; may be NULL
 * args: a parenthesized argument list for the function
 */
#define OPTIONAL_VOIDCALL(f,args) do { if (f != NULL) f args; } while (CIF_FALSE)

/*
 * Parse a while CIF via the provided scanner into the provided CIF object.  On success, all characters available from
 * the scanner will have been consumed.  The provided CIF object does not need to be empty, but semantic errors will
 * occur if it contains data blocks with block codes matching those of blocks read from the input.
 *
 * If the provided CIF handle is NULL then a syntax-only check is performed, but nothing is stored.  In that mode,
 * semantic constraints such as uniqueness of block codes, frame codes, and data names are not checked.
 */
static int parse_cif(struct scanner_s *scanner, cif_t *cif) {
    int result;

    result = OPTIONAL_CALL(scanner->handler->handle_cif_start, (cif, scanner->user_data), CIF_OK);
    switch (result) {
        case CIF_TRAVERSE_SKIP_CURRENT:
        case CIF_TRAVERSE_SKIP_SIBLINGS:
            scanner->skip_depth = 1;
            /* fall through */
        case CIF_TRAVERSE_CONTINUE:
            result = CIF_OK;
            break;
        case CIF_TRAVERSE_END:
            return CIF_OK;
    }
    while (result == CIF_OK) {
        cif_block_t *block = NULL;
        int32_t token_length;
        UChar *token_value;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        token_value = TVALUE_START(scanner);
        token_length = TVALUE_LENGTH(scanner);

        switch (scanner->ttype) {
            case BLOCK_HEAD:
                if ((cif != NULL) && (scanner->skip_depth <= 0)) {
                    /* insert a string terminator into the input buffer, after the current token */
                    UChar saved = *(token_value + token_length);
                    *(token_value + token_length) = 0;
    
                    result = cif_create_block(cif, token_value, &block);
                    switch (result) {
                        case CIF_INVALID_BLOCKCODE:
                            /* syntax error: invalid block code */
                            result = scanner->error_callback(result, scanner->line,
                                    scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                    TVALUE_LENGTH(scanner), scanner->user_data);
                            if (result != CIF_OK) {
                                goto cif_end;
                            }
                            /* recover by using the block code anyway */
                            if ((result = (cif_create_block_internal(cif, token_value, 1, &block)))
                                    != CIF_DUP_BLOCKCODE) {
                                break;
                            }
                            /* else result == CIF_DUP_BLOCKCODE, so fall through */
                        case CIF_DUP_BLOCKCODE:
                            /* error: duplicate block code */
                            result = scanner->error_callback(result, scanner->line,
                                    scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                    TVALUE_LENGTH(scanner), scanner->user_data);
                            if (result != CIF_OK) {
                                goto cif_end;
                            }
                            /* recover by using the existing block */
                            result = cif_get_block(cif, token_value, &block);
                            break;
                        /* default: do nothing */
                    }
        
                    /* restore the next character in the input buffer */
                    *(token_value + token_length) = saved;
                } /* else syntax check only; no actual block will be created */

                CONSUME_TOKEN(scanner);
    
                break;
            case END:
                /* it's more useful to leave the token than to consume it */
                goto cif_end;
            default:
                /* error: missing data block header */
                result = scanner->error_callback(CIF_NO_BLOCK_HEADER, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner), TVALUE_LENGTH(scanner),
                        scanner->user_data);
                if (result != CIF_OK) {
                    goto cif_end;
                }
                /* recover by creating and installing an anonymous block context to eat up the content */
                if ((cif != NULL) && (scanner->skip_depth <= 0)) {
                    if ((result = cif_create_block_internal(cif, &cif_uchar_nul, 1, &block)) == CIF_DUP_BLOCKCODE) {
                        /*
                         * or fall back to using an anonymous context that already exists; this should happen only if
                         * the user provided a non-empty initial CIF
                         */
                        result = cif_get_block(cif, &cif_uchar_nul, &block);
                    }
                }
    
                break;
        }

        if (result != CIF_OK) {
            break;
        }

        result = parse_container(scanner, block, CIF_TRUE);
        if (block != NULL) {
            cif_block_free(block); /* ignore any error */
        }
    }

    cif_end:
    if (scanner->skip_depth > 0) {
        scanner->skip_depth -= 1;
    }
    if (result == CIF_OK) {
        result = OPTIONAL_CALL(scanner->handler->handle_cif_end, (cif, scanner->user_data), CIF_OK);
    }

    /* codes less than 0 are interpreted as CIF traversal navigation signals */
    return ((result > CIF_OK) ? result : CIF_OK);
}

static int parse_container(struct scanner_s *scanner, cif_container_t *container, int is_block) {
    int result;

    if (scanner->skip_depth > 0) {
        scanner->skip_depth += 1;
        result = CIF_OK;
    } else {
        if (is_block) {
            result = OPTIONAL_CALL(scanner->handler->handle_block_start, (container, scanner->user_data), CIF_OK);
        } else {
            result = OPTIONAL_CALL(scanner->handler->handle_frame_start, (container, scanner->user_data), CIF_OK);
        }
        switch (result) {
            case CIF_TRAVERSE_CONTINUE:
                result = CIF_OK;
                break;
            case CIF_TRAVERSE_SKIP_CURRENT:
                scanner->skip_depth = 1;
                result = CIF_OK;
                break;
            case CIF_TRAVERSE_SKIP_SIBLINGS:
                scanner->skip_depth = 2;
                result = CIF_OK;
                break;
        }
    }

    while (result == CIF_OK) {
        int32_t token_length;
        UChar *token_value;
        UChar *name;
        enum token_type alt_ttype = QVALUE;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        token_value = TVALUE_START(scanner);
        token_length = TVALUE_LENGTH(scanner);

        switch (scanner->ttype) {
            case BLOCK_HEAD:
                if (!is_block) {
                    /* error: unterminated save frame */
                    result = scanner->error_callback(CIF_NO_FRAME_TERM, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner), TVALUE_LENGTH(scanner),
                            scanner->user_data);
                    /* recover, if so directed, by continuing */
                } /* else result == CIF_OK, as tested before this switch */
                /* do not consume the token */
                goto container_end;
            case FRAME_HEAD:
                if (!is_block) {
                    /* error: unterminated save frame */
                    result = scanner->error_callback(CIF_NO_FRAME_TERM, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner), TVALUE_LENGTH(scanner),
                            scanner->user_data);
                    /* recover, if so directed, by forcing closure of the current frame without consuming the token */
                    goto container_end;
                } else {
                    cif_container_t *frame = NULL;

                    if ((container == NULL) || (scanner->skip_depth > 0)) {
                        result = CIF_OK;
                    } else { 
                        UChar saved = *(token_value + token_length);

                        /* insert a string terminator into the input buffer, after the current token */
                        *(token_value + token_length) = 0;

                        result = cif_block_create_frame(container, token_value, &frame);
                        switch (result) {
                            case CIF_INVALID_FRAMECODE:
                                /* syntax error: invalid frame code */
                                result = scanner->error_callback(result, scanner->line,
                                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                        TVALUE_LENGTH(scanner), scanner->user_data);
                                if (result != CIF_OK) {
                                    goto container_end;
                                }
                                /* recover by using the frame code anyway */
                                if ((result = (cif_block_create_frame_internal(container, token_value, 1, &frame)))
                                        != CIF_DUP_FRAMECODE) {
                                    break;
                                }
                                /* else result == CIF_DUP_BLOCKCODE, so fall through */
                            case CIF_DUP_FRAMECODE:
                                /* error: duplicate frame code */
                                result = scanner->error_callback(result, scanner->line,
                                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                        TVALUE_LENGTH(scanner), scanner->user_data);
                                if (result != CIF_OK) {
                                    goto container_end;
                                }
                                /* recover by using the existing frame */
                                result = cif_block_get_frame(container, token_value, &frame);
                                break;
                            /* default: do nothing */
                        }

                        /* restore the next character in the input buffer */
                        *(token_value + token_length) = saved;
                    }
    
                    CONSUME_TOKEN(scanner);
    
                    if (result == CIF_OK) {
                        result = parse_container(scanner, frame, CIF_FALSE);
                        if (frame != NULL) {
                            cif_frame_free(frame);  /* ignore any error */
                        }
                    }
                }
                break;
            case FRAME_TERM:
                /* consume the token in all cases */
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                if (is_block) {
                    /* error: unexpected frame terminator */
                    result = scanner->error_callback(CIF_UNEXPECTED_TERM, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), scanner->user_data);
                    if (result != CIF_OK) {
                        goto container_end;
                    }
                    /* recover by dropping the token */
                } else {
                    /* close the context */
                    goto container_end;
                }
                break;
            case LOOPKW:
                CONSUME_TOKEN(scanner);
                result = parse_loop(scanner, container);
                break;
            case NAME:
                if (scanner->skip_depth > 0) {
                    CONSUME_TOKEN(scanner);
                    result = parse_item(scanner, container, NULL);
                } else {
                    name = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                    if (name == NULL) {
                        result = CIF_ERROR;
                        goto container_end;
                    } else {
                        /* copy the data name to a separate Unicode string */
                        u_strncpy(name, token_value, token_length);
                        name[token_length] = 0;
                        CONSUME_TOKEN(scanner);
    
                        /* check for dupes */
                        result = ((container == NULL) ? CIF_NOSUCH_ITEM
                                : cif_container_get_item_loop(container, name, NULL));

                        if (result == CIF_NOSUCH_ITEM) {
                            result = parse_item(scanner, container, name);
                        } else if (result == CIF_OK) {
                            /* error: duplicate data name */
                            result = scanner->error_callback(CIF_DUP_ITEMNAME, scanner->line,
                                    scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                    TVALUE_LENGTH(scanner), scanner->user_data);
                            if (result != CIF_OK) {
                                goto container_end;
                            }
                            /* recover by rejecting the item (but still parsing the associated value) */
                            result = parse_item(scanner, container, NULL);
                        }

                        free(name);
                    }
                }
                break;
            case TKEY:
                alt_ttype = TVALUE;
                /* fall through */
            case KEY:
                /* error: missing whitespace */
                result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line, scanner->column - 1,
                        scanner->next_char - 1, 0, scanner->user_data);
                if (result != CIF_OK) {
                    goto container_end;
                }
                /* recover by pushing back the colon */
                scanner->next_char -= 1;
                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                /* fall through */
            case TVALUE:
            case QVALUE:
            case VALUE:
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
                /* error: unexpected value */
                result = scanner->error_callback(CIF_UNEXPECTED_VALUE, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                if (result != CIF_OK) {
                    goto container_end;
                }
                /* recover by consuming and discarding the value */
                result = parse_item(scanner, container, NULL);
                break;
            case CTABLE:
            case CLIST:
                /* error: unexpected closing delimiter */
                result = scanner->error_callback(CIF_UNEXPECTED_DELIM, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                if (result != CIF_OK) {
                    goto container_end;
                }
                /* recover by dropping it */
                CONSUME_TOKEN(scanner);
                break;
            case END:
                if (!is_block) {
                    /* error: unterminated save frame at EOF */
                    result = scanner->error_callback(CIF_EOF_IN_FRAME, scanner->line, scanner->column,
                            TVALUE_START(scanner), 0, scanner->user_data);
                    /* recover by rejecting the token (as would happen anyway) */
                } /* else result == CIF_OK */
                /* no special action, just reject the token */
                goto container_end;
            default:
                /* should not happen */
                result = CIF_INTERNAL_ERROR;
                break;
        }

    }

    container_end:
    if (scanner->skip_depth > 0) {
        scanner->skip_depth -= 1;
    }
    if ((result == CIF_OK) && (scanner->skip_depth <= 0)
            && ((container == NULL) || ((result = cif_container_prune(container)) == CIF_OK))) {
        if (is_block) {
            result = OPTIONAL_CALL(scanner->handler->handle_block_end, (container, scanner->user_data), CIF_OK);
        } else {
            result = OPTIONAL_CALL(scanner->handler->handle_frame_end, (container, scanner->user_data), CIF_OK);
        }
        switch (result) {
            case CIF_TRAVERSE_SKIP_SIBLINGS:
                scanner->skip_depth = 1;
                /* fall through */
            case CIF_TRAVERSE_CONTINUE:
            case CIF_TRAVERSE_SKIP_CURRENT:
                result = CIF_OK;
                break;
        }
    }

    return result;
}

static int parse_item(struct scanner_s *scanner, cif_container_t *container, UChar *name) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_t *value = NULL;
        enum token_type alt_ttype = QVALUE;
    
        if (scanner->skip_depth > 0) {
            scanner->skip_depth += 1;
        }

        switch (scanner->ttype) {
            case TKEY:
                alt_ttype = TVALUE;
                /* fall through */
            case KEY:
                /* error: missing whitespace */
                result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line, scanner->column - 1,
                        scanner->next_char - 1, 0, scanner->user_data);
                if (result != CIF_OK) {
                    break;
                }
                /* recover by pushing back the colon */
                scanner->next_char -= 1;
                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                /* fall through */
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
            case TVALUE:
            case QVALUE:
            case VALUE:
                /* parse the value */
                result = parse_value(scanner, &value);
                break;
            default:
                /* error: missing value */
                result = scanner->error_callback(CIF_MISSING_VALUE, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                if (result == CIF_OK) {
                    /* recover by inserting a synthetic unknown value */
                    result = cif_value_create(CIF_UNK_KIND, &value);
                }
                /* do not consume the token */
                break;
        }
    
        if (result == CIF_OK) {
            if ((name != NULL) && (container != NULL)) {
                assert(scanner->skip_depth <= 0);

                result = OPTIONAL_CALL(scanner->handler->handle_item, (name, value, scanner->user_data), CIF_OK);
                switch (result) {
                    case CIF_TRAVERSE_CONTINUE:
                        /* _copy_ the value into the CIF */
                        result = cif_container_set_value(container, name, value);
                        break;
                    case CIF_TRAVERSE_SKIP_CURRENT:
                        /* no need to set the skip depth because we don't go any deeper from here */
                        result = CIF_OK;
                        break;
                    case CIF_TRAVERSE_SKIP_SIBLINGS:
                        scanner->skip_depth = 2;
                        result = CIF_OK;
                        break;
                }
                
            }
            cif_value_free(value); /* ignore any error */
        }

        if (scanner->skip_depth > 0) {
            scanner->skip_depth -= 1;
        }
    }

    return result;
}

static int parse_loop(struct scanner_s *scanner, cif_container_t *container) {
    string_element_t *first_name = NULL;          /* the first data name in the header */
    int name_count = 0;
    cif_loop_t *loop = NULL;
    int result;

    if (scanner->skip_depth > 0) {
        scanner->skip_depth += 1;
    }

    /* parse the header */
    result = parse_loop_header(scanner, container, &first_name, &name_count);
    
    if (result == CIF_OK) {
        /* name_count is the number of data names in the header, including dupes and invalid ones */
        if (name_count == 0) {
            /* error: empty loop header */
            result = scanner->error_callback(CIF_NULL_LOOP, scanner->line, scanner->column - TVALUE_LENGTH(scanner),
                    TVALUE_START(scanner), 0, scanner->user_data);
            if (result != CIF_OK) {
                goto loop_end;
            }
            /* recover by ignoring it */
        } else {
            /* an array of data names for creating the loop; elements belong to the linked list, not this array */
            UChar **names = (UChar **) malloc((name_count + 1) * sizeof(UChar *));

            if (names == NULL) {
                result = CIF_ERROR;
                goto loop_end;
            } else {
                string_element_t *next_name;
                int name_index = 0;
               
                /* dummy_loop is a static adapter; of its elements, it owns only 'category' */
                cif_loop_t dummy_loop = { NULL, -1, NULL, NULL };

                dummy_loop.container = container;
                dummy_loop.names = names; 

                /* prepare for the loop body */

                name_index = 0;
                for (next_name = first_name; next_name != NULL; next_name = next_name->next) {
                    if (next_name->string != NULL) {
                        names[name_index++] = next_name->string;
                    } /* else a previously-detected duplicate name */
                }
                names[name_index] = NULL;

                if (scanner->skip_depth <= 0) {  /* this loop is not being skipped */
                    result = OPTIONAL_CALL(scanner->handler->handle_loop_start, (&dummy_loop, scanner->user_data), CIF_OK);
                    switch (result) {
                        case CIF_TRAVERSE_CONTINUE:
                            if (container != NULL) {
                                /* create the loop */
                                switch (result = cif_container_create_loop(container, dummy_loop.category, names,
                                        &loop)) {
                                    case CIF_NULL_LOOP:  /* tolerable */
                                    case CIF_OK:         /* expected */
                                        break;
                                    case CIF_INVALID_ITEMNAME:
                                    case CIF_DUP_ITEMNAME:
                                        /* these should not happen because the names were already validated */
                                        result = CIF_INTERNAL_ERROR;
                                        /* fall through */
                                    default:
                                        goto loop_body_end;
                                }
                            }
                            break;
                        case CIF_TRAVERSE_SKIP_CURRENT:
                            scanner->skip_depth = 1;
                            break;
                        case CIF_TRAVERSE_SKIP_SIBLINGS:
                            scanner->skip_depth = 2;
                            break;
                        case CIF_TRAVERSE_END:
                            goto loop_body_end;
                    }
                }  /* else loop == NULL from its initialization */

                /* read packets */
                result = parse_loop_packets(scanner, loop, first_name, names);

                loop_body_end:
                if (dummy_loop.category != NULL) {
                    free(dummy_loop.category);
                }
                free(names);
                /* elements of names belong to the linked list; they are freed later */
            } /* end ifelse (names) */
        } /* end ifelse (name_count) */
    } /* end if (header result) */

    loop_end:

    /* clean up the header name linked list */
    while (first_name != NULL) {
        string_element_t *next = first_name->next;

        if (first_name->string != NULL) {
            free(first_name->string);
        }
        free(first_name);
        first_name = next;
    }

    if (scanner->skip_depth > 0) {
        scanner->skip_depth -= 1;
    } else if (result == CIF_OK) {
        /* Call the loop end handler */
        switch (result = OPTIONAL_CALL(scanner->handler->handle_loop_end, (loop, scanner->user_data), CIF_OK)) {
            case CIF_TRAVERSE_SKIP_CURRENT:
                result = CIF_OK;
                break;
            case CIF_TRAVERSE_SKIP_SIBLINGS:
                scanner->skip_depth = 1;
                result = CIF_OK;
                break;
        }
    }

    return result;
}

static int parse_loop_header(struct scanner_s *scanner, cif_container_t *container, string_element_t **name_list_head,
        int *name_countp) {
    string_element_t **next_namep = name_list_head;  /* a pointer to the pointer to the next data name in the header */
    int result;

    /* parse the header */
    while (((result = next_token(scanner)) == CIF_OK) && (scanner->ttype == NAME)) {
        int32_t token_length = TVALUE_LENGTH(scanner);
        UChar *token_value = TVALUE_START(scanner);

        *next_namep = (string_element_t *) malloc(sizeof(string_element_t));
        if (*next_namep == NULL) {
            return CIF_ERROR;
        } else {
            (*next_namep)->next = NULL;
            (*next_namep)->string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
            if ((*next_namep)->string == NULL) {
                return CIF_ERROR;
            } else {
                u_strncpy((*next_namep)->string, token_value, token_length);
                (*next_namep)->string[token_length] = 0;

                /* check for data name duplication */
                switch (result = ((container == NULL) ? CIF_NOSUCH_ITEM
                            : cif_container_get_item_loop(container, (*next_namep)->string, NULL))) {
                    case CIF_NOSUCH_ITEM:
                        /* the expected case */
                        break;
                    case CIF_OK:
                        /* error: duplicate item name */
                        if ((result = scanner->error_callback(CIF_DUP_ITEMNAME, scanner->line,
                                scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                TVALUE_LENGTH(scanner), scanner->user_data)) == CIF_OK) {
                            /* recover by ignoring the name, and later its associated values in the loop body */
                            /* a place-holder is retained in the list so that packet values can be counted and assigned
                               correctly */
                            free((*next_namep)->string);
                            (*next_namep)->string = NULL;
                            break;
                        }
                        /* fall through */
                    default:
                        return result;
                }

                next_namep = &((*next_namep)->next);
                *name_countp += 1;
            }
        }

        CONSUME_TOKEN(scanner);
    }

    return result;
}

static int parse_loop_packets(struct scanner_s *scanner, cif_loop_t *loop, string_element_t *first_name,
        UChar *names[]) {
    cif_packet_t *packet;
    int result = cif_packet_create(&packet, names);

    /* read packets */
    if (result == CIF_OK) {
        int have_packets = CIF_FALSE;
        string_element_t *next_name = first_name;

        while ((result = next_token(scanner)) == CIF_OK) {
            UChar *name;
            cif_value_t *value = NULL;
            enum token_type alt_ttype = QVALUE;

            switch (scanner->ttype) {
                case TKEY:
                    alt_ttype = TVALUE;
                    /* fall through */
                case KEY:
                    /* error: missing whitespace (or that's how we interpret it, anyway) */
                    result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), scanner->user_data);
                    if (result != CIF_OK) {
                        goto packets_end;
                    }
                    /* recover by pushing back the colon */
                    scanner->next_char -= 1;
                    scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                    /* fall through */
                case OLIST:  /* opening delimiter of a list value */
                case OTABLE: /* opening delimiter of a table value */
                case TVALUE:
                case QVALUE:
                case VALUE:
                    if (next_name == first_name) {
                        /* first value of a new packet */
                        if (scanner->skip_depth > 0) {
                            scanner->skip_depth += 1;
                        } else {
                            /* there are no packet values yet, so dispatch a NULL packet to the handler */
                            result = OPTIONAL_CALL(scanner->handler->handle_packet_start,
                                    (NULL, scanner->user_data), CIF_OK);
                            switch (result) {
                                case CIF_TRAVERSE_SKIP_CURRENT:
                                    scanner->skip_depth = 1;
                                    break;
                                case CIF_TRAVERSE_SKIP_SIBLINGS:
                                    scanner->skip_depth = 2;
                                    break;
                                case CIF_TRAVERSE_END:
                                    goto packets_end;
                            }
                        }
                    }
                    name = next_name->string;

                    if (name != NULL) {
                        /* create a value object IN THE PACKET to receive the parsed value */
                        if (((result = cif_packet_set_item(packet, name, NULL)) != CIF_OK)
                                || ((result = cif_packet_get_item(packet, name, &value)) != CIF_OK)) {
                            goto packets_end;
                        }
                    } /* else the name is just a placeholder */

                    /* parse the value */
                    if ((result = parse_value(scanner, &value)) == CIF_OK) {
                        result = OPTIONAL_CALL(scanner->handler->handle_item,
                                (name, value, scanner->user_data), CIF_OK);
                        switch (result) {
                            case CIF_TRAVERSE_SKIP_CURRENT:
                                result = CIF_OK;
                                break;
                            case CIF_TRAVERSE_SKIP_SIBLINGS:
                                scanner->skip_depth = 1;
                                result = CIF_OK;
                                break;
                        }
                    }
                    if (name == NULL) {
                        /* discard the value -- the associated name was a duplicate or failed validation */
                        cif_value_free(value);  /* ignore any error */
                    } /* else the value is already in the packet and belongs to it */

                    next_name = next_name->next;
                    if (result != CIF_OK) { /* this is the parse_value() or item handler result code */
                        goto packets_end;
                    } else if (next_name == NULL) {  /* that was the last value in the packet */
                        if (scanner->skip_depth > 0) {
                            scanner->skip_depth -= 1;
                        } else {
                            result = OPTIONAL_CALL(scanner->handler->handle_packet_end,
                                    (packet, scanner->user_data), CIF_OK);
                            switch (result) {
                                case CIF_TRAVERSE_CONTINUE:  /* == CIF_OK */
                                    /* record the packet, if appropriate */
                                    if ((loop != NULL)
                                            && ((result = cif_loop_add_packet(loop, packet)) != CIF_OK)) {
                                        goto packets_end;
                                    }
                                    break;
                                case CIF_TRAVERSE_SKIP_CURRENT:
                                    /* do not record the current packet */
                                    result = CIF_OK;
                                    break;
                                case CIF_TRAVERSE_SKIP_SIBLINGS:
                                    /* do not record the current packet or any others in this loop */
                                    scanner->skip_depth = 1;
                                    result = CIF_OK;
                                    break;
                                default:
                                    /* abort the parse */
                                    goto packets_end;
                            }
                        }
                        have_packets = CIF_TRUE;
                        next_name = first_name;
                    }

                    break;
                case CLIST:
                case CTABLE:
                    /* error: unexpected list/table delimiter */
                    result = scanner->error_callback(CIF_UNEXPECTED_DELIM, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), scanner->user_data);
                    if (result != CIF_OK) {
                        goto packets_end;
                    }
                    /* recover by dropping it; the loop body is not terminated */
                    CONSUME_TOKEN(scanner);
                    break;
                default: /* any other token type terminates the loop body */
                    if (next_name != first_name) {
                        /* error: partial packet */
                        result = scanner->error_callback(CIF_PARTIAL_PACKET, scanner->line,
                                scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner), 0,
                                scanner->user_data);
                        if (result != CIF_OK) {
                            goto packets_end;
                        }
                        /* recover by synthesizing unknown values to fill the packet, and saving it */
                        for (; next_name != NULL; next_name = next_name->next) {
                            if ((next_name->string != NULL)
                                    && (result = cif_packet_set_item(packet, next_name->string, NULL))
                                            != CIF_OK) {
                                goto packets_end;
                            }
                        }

                        if (scanner->skip_depth > 0) {
                            scanner->skip_depth -= 1;
                        } else {
                            result = OPTIONAL_CALL(scanner->handler->handle_packet_end,
                                    (packet, scanner->user_data), CIF_OK);
                            switch (result) {
                                case CIF_TRAVERSE_CONTINUE:  /* == CIF_OK */
                                    if (loop != NULL) {
                                        result = cif_loop_add_packet(loop, packet);
                                        /* will fall through to "goto packets_end" */
                                    }
                                    break;
                                case CIF_TRAVERSE_SKIP_CURRENT:
                                    result = CIF_OK;
                                    break;
                                case CIF_TRAVERSE_SKIP_SIBLINGS:
                                    scanner->skip_depth = 1;
                                    result = CIF_OK;
                                    break;
                                default:
                                    goto packets_end;
                            }
                        }
                    } else if (!have_packets) {
                        /* error: no packets */
                        result = scanner->error_callback(CIF_EMPTY_LOOP, scanner->line,
                                scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                TVALUE_LENGTH(scanner), scanner->user_data);
                        /* recover by doing nothing for now */
                        /*
                         * JCB note: a loop without packets is not valid in the data model, but the data
                         * names need to be retained until the container has been fully parsed to allow
                         * duplicates to be detected correctly.  Maybe some kind of pruning is needed
                         * after the container is fully parsed.
                         */
                    }

                    /* the token is not consumed or examined in any way */
                    goto packets_end;
            } /* end switch(scanner->ttype) */
        } /* end while(next_token()) */

        packets_end:
        cif_packet_free(packet);
    } /* end if (cif_packet_create()) */

    return result;
}

static int parse_list(struct scanner_s *scanner, cif_value_t **listp) {
    cif_value_t *list = NULL;
    size_t next_index = 0;
    int result;

    if (*listp == NULL) {
        /* create a new value object */
        result = cif_value_create(CIF_LIST_KIND, &list);
    } else {
        /* use the existing value object */
        list = *listp;
        result = cif_value_init(list, CIF_LIST_KIND);
    }

    while (result == CIF_OK) {
        cif_value_t *element = NULL;
        enum token_type alt_ttype = QVALUE;

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }

        switch (scanner->ttype) {
            case TKEY:
                alt_ttype = TVALUE;
                /* fall through */
            case KEY:
                /* error: missing whitespace */
                result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                if (result != CIF_OK) {
                    goto list_end;
                }
                /* recover by pushing back the colon */
                scanner->next_char -= 1;
                scanner->ttype = alt_ttype;  /* TVALUE or QVALUE */
                /* fall through */
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
            case TVALUE:
            case QVALUE:
            case VALUE:
                /* Insert a dummy value, then update it with the parse result.  This minimizes copying. */
                result = cif_value_insert_element_at(list, next_index, NULL);
                switch ((result == CIF_OK) ? (result = cif_value_get_element_at(list, next_index++, &element))
                        : result) {
                    case CIF_OK:
                        /* element now points to a list element (not a copy). Parse the value into it. */
                        result = parse_value(scanner, &element);
                        break;
                    case CIF_ARGUMENT_ERROR:
                    case CIF_INVALID_INDEX:
                        /* should not happen */
                        result = CIF_INTERNAL_ERROR;
                        break;
                }
                break;
            case CLIST:
                /* accept the token and end the list */
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                goto list_end;
            default:
                /* error: unterminated list */
                result = scanner->error_callback(CIF_MISSING_DELIM, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                /* recover by synthetically closing the list without consuming the token */
                goto list_end;
        }
    } /* while */

    list_end:

    if (result == CIF_OK) {
        *listp = list;
    } else if (list != *listp) {
        cif_value_free(list);
    }

    return result;
}

static int parse_table(struct scanner_s *scanner, cif_value_t **tablep) {
    /*
     * Implementation note: this function is designed to minimize the damage caused by input malformation.  If a
     * key or value is dropped, or if a key is turned into a value by removing its colon or inserting whitespace
     * before it, then only one table entry is lost.  Furthermore, this function recognizes and handles unquoted
     * keys, provided that the configured error callback does not abort parsing when notified of the problem.
     */
    cif_value_t *table;
    int result;

    assert (tablep != NULL);
    if (*tablep == NULL) {
        result = cif_value_create(CIF_TABLE_KIND, &table);
    } else {
        table = *tablep;
        result = cif_value_init(table, CIF_TABLE_KIND);
    }

    while (result == CIF_OK) {
        /* parse (key, value) pairs */
        UChar *key = NULL;
        cif_value_t *value = NULL;

        /* scan the next key, if any */

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        switch (scanner->ttype) {
            case VALUE:
                if (*TVALUE_START(scanner) == COLON_CHAR) {
                    /* error: null key */
                    result = scanner->error_callback(CIF_MISSING_KEY, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), scanner->user_data);
                    if (result != CIF_OK) {
                        goto table_end;
                    }
                    /* recover by handling it as a NULL (not empty) key */
                    if (TVALUE_LENGTH(scanner) > 1) {
                        PUSH_BACK(scanner, 1);
                    }
                    CONSUME_TOKEN(scanner);
                    break;
                } else {
                    size_t index;

                    /*
                     * an unquoted string is not valid here, but defer reporting the error until we know whether it is
                     * a stray value or an unquoted key.
                     */
                    for (index = 1; index < TVALUE_LENGTH(scanner); index += 1) {
                        if (TVALUE_START(scanner)[index] == COLON_CHAR) {
                            /* error: unquoted key */
                            result = scanner->error_callback(CIF_UNQUOTED_KEY, scanner->line,
                                    scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                    TVALUE_LENGTH(scanner), scanner->user_data);
                            if (result != CIF_OK) {
                                goto table_end;
                            }
                            /* recover by accepting everything before the first colon as a key */
                            /* push back all characters following the first colon */
                            PUSH_BACK(scanner, index + 1);
                            TVALUE_SETLENGTH(scanner, index);
                            scanner->ttype = KEY;
                        }
                    }
                    if (scanner->ttype != KEY) {
                        /* error: missing key */
                        result = scanner->error_callback(CIF_MISSING_KEY, scanner->line,
                                scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                TVALUE_LENGTH(scanner), scanner->user_data);
                        if (result != CIF_OK) {
                            goto table_end;
                        }
                        /* recover by dropping the value and continuing to the next entry */
                        CONSUME_TOKEN(scanner);
                        continue;
                    } /* else fall through */
                }
                /* fall through */
            case KEY:  /* this and CTABLE are the only genuinely valid token types here */
                /* copy the key to a NUL-terminated Unicode string */
                key = (UChar *) malloc((TVALUE_LENGTH(scanner) + 1) * sizeof(UChar));
                if (key != NULL) {
                    u_strncpy(key, TVALUE_START(scanner), TVALUE_LENGTH(scanner));
                    key[TVALUE_LENGTH(scanner)] = 0;
                    CONSUME_TOKEN(scanner);    
                    break;
                }
                result = CIF_ERROR;
                goto table_end;
            case TKEY:
                /* error invalid key type */
                result = scanner->error_callback(CIF_MISQUOTED_KEY, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                if (result == CIF_OK) {
                    /* recover by using the text field contents as a key */
                    if ((result = decode_text(scanner, TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), &value)) == CIF_OK) {
                        result = cif_value_get_text(value, &key);
                        cif_value_free(value); /* ignore any error */
                        CONSUME_TOKEN(scanner);
                        if (result == CIF_OK) {
                            break;
                        }
                    }
                }
                goto table_end;
            case QVALUE:
            case TVALUE:
            case OLIST:  /* opening delimiter of a list value */
            case OTABLE: /* opening delimiter of a table value */
                /* error: missing key / unexpected value */
                result = scanner->error_callback(CIF_MISSING_KEY, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                if (result == CIF_OK) {
                    /* recover by accepting and dropping the value */
                    if ((result = parse_value(scanner, &value)) == CIF_OK) {
                        /* token is already consumed by parse_value() */
                        cif_value_free(value);  /* ignore any error */
                        continue; 
                    }
                }
                goto table_end;
            case CTABLE:
                CONSUME_TOKEN(scanner);
                result = CIF_OK;
                goto table_end;
            default:
                /* error: unterminated table */
                result = scanner->error_callback(CIF_MISSING_DELIM, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner), 0, scanner->user_data);
                /* recover by ending the table; the token is left for the caller to handle */
                goto table_end;
        }

        /* scan the value */

        /* obtain a value object into which to scan the value, to avoid copying the scanned value after the fact */
        if ((key != NULL)
                && (((result = cif_value_set_item_by_key(table, key, NULL)) != CIF_OK) 
                        || ((result = cif_value_get_item_by_key(table, key, &value)) != CIF_OK))) {
            free(key);
            break;
        }

        if ((result = next_token(scanner)) == CIF_OK) {
            switch (scanner->ttype) {
                case OLIST:  /* opening delimiter of a list value */
                case OTABLE: /* opening delimiter of a table value */
                case TVALUE:
                case QVALUE:
                case VALUE:
                    /* parse the incoming value into the existing value object */
                    result = parse_value(scanner, &value);
                    break;
                default:
                    /* error: missing value */
                    result = scanner->error_callback(CIF_MISSING_VALUE, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), scanner->user_data);
                    /* recover by using an unknown value (already set) */
                    /* do not consume the token here */
                    /* will proceed to table_end if result != CIF_OK */
                    break;
            }
        }

        if (key == NULL) {
            cif_value_free(value); /* ignore any error */
        } else {
            /* the value belongs to the table */
            free(key);
        }
    } /* while */

    table_end:
    return result;
}

/*
 * Parses a value of any of the supported types.  The next token must represent a value, or the start of one.
 */
static int parse_value(struct scanner_s *scanner, cif_value_t **valuep) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_t *value = *valuep;

        /* build a value object or modify the provided one, as appropriate */
        if ((value != NULL) || ((result = cif_value_create(CIF_UNK_KIND, &value)) == CIF_OK)) {
            UChar *token_value = TVALUE_START(scanner);
            size_t token_length = TVALUE_LENGTH(scanner);
            UChar *string;

            switch (scanner->ttype) {
                case OLIST: /* opening delimiter of a list value */
                    CONSUME_TOKEN(scanner);
                    result = parse_list(scanner, &value);
                    break;
                case OTABLE: /* opening delimiter of a table value */
                    CONSUME_TOKEN(scanner);
                    result = parse_table(scanner, &value);
                    break;
                case TVALUE:
                    /* parse text block contents into a value */
                    result = decode_text(scanner, token_value, token_length, &value);
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    break;
                case QVALUE:
                    /* overwrite the closing delimiter (not included in the length) with a string terminator */
                    string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                    if (string != NULL) {
                        /* copy the input string into the value object */
                        u_strncpy(string, token_value, token_length);
                        string[token_length] = 0;
                        result = cif_value_init_char(value, string);
                        /* ownership of 'string' is transferred to 'value' */
                        CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    } else {
                        result = CIF_ERROR;
                    }
                   break;
                case VALUE:
                    /* special cases for unquoted question mark (?) and period (.) */
                    if (token_length == 1) {
                        if (*token_value == QUERY_CHAR) {
                            result = cif_value_init(value, CIF_UNK_KIND);
                            goto value_end;
                        }  else if (*token_value == DECIMAL_CHAR) {
                            result = cif_value_init(value, CIF_NA_KIND);
                            goto value_end;
                        } /* else an ordinary value */
                    }
        
                    string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                    if (string == NULL) {
                        result = CIF_ERROR;
                    } else {
                        /* copy the token value into a separate buffer, with terminator */
                        *string = 0;
                        u_strncat(string, token_value, token_length);  /* ensures NUL termination */

                        /* Record the value as a number then it parses that way; otherwise as a string */
                        /* In either case, the value object takes ownership of the string */
                        if (((result = cif_value_parse_numb(value, string)) == CIF_INVALID_NUMBER) &&
                                ((result = cif_value_init_char(value, string)) != CIF_OK)) {
                            /* assignment failed; free the string */
                            free(string);
                        }
                    }

                    value_end:
                    CONSUME_TOKEN(scanner);  /* consume the token _after_ parsing its value */
                    break;
                default:
                    /* This parse function should be called only when the incoming token is or starts a value */
                    result = CIF_INTERNAL_ERROR;
                    break;
            }

            if (result == CIF_OK) {
                *valuep = value;
            } else if (*valuep != value) {
                cif_value_free(value);  /* ignore any error */
            }
        }
    }

    return result;
}


/*
 * Decodes the contents of a text block by un-prefixing and unfolding lines as appropriate, and standardizing line
 * terminators to a single newline character.  Records the result in a CIF value object.
 *
 * If dest points to a non-NULL value handle then the associated value object is modified
 * (on success) to be of kind CIF_CHAR_KIND, holding the parsed text.  Otherwise, a new value
 * object is created and its handle recorded where dest points.
 */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_t **dest) {
    FAILURE_HANDLING;
    cif_value_t *value = *dest;
    int result = (value == NULL) ? cif_value_create(CIF_UNK_KIND, &value) : CIF_OK;

    if (result != CIF_OK) {
        /* failed to create a value object */
        return result;
    } else {
        if (text_length <= 0) {
            /* update the value object with an empty char value */
            if ((result = cif_value_init(value, CIF_CHAR_KIND)) != CIF_OK) {
                FAIL(early, CIF_ERROR);
            } else {
                *dest = value;
                return CIF_OK;
            }
        } else {
            UChar *buffer = (UChar *) malloc((text_length + 1) * sizeof(UChar));

            if (buffer != NULL) {
                /* analyze the text and copy its logical contents into the buffer */
                UChar *buf_pos = buffer;
                UChar *in_pos = text;
                UChar *in_limit = text + text_length; /* input boundary pointer */
                int32_t prefix_length;
                int folded;

                if ((*text == SEMI_CHAR) || ((scanner->line_unfolding <= 0) && (scanner->prefix_removing <= 0))) {
                    /* text beginning with a semicolon is neither prefixed nor folded */
                    prefix_length = 0;
                    folded = CIF_FALSE;
                } else {
                    /* check for text prefixing and/or line-folding */
                    int backslash_count = 0;
                    int nonws = CIF_FALSE;  /* whether there is any non-whitespace between the last backslash and EOL */

                    /* scan the first line of the text for a prefix and/or line-folding signature */
                    prefix_length = -1;
                    for (; in_pos < in_limit; ) {
                        UChar c = *(in_pos++);

                        /* prospectively copy the input character to the buffer */
                        *(buf_pos++) = c;

                        /* update statistics */
                        switch (c) {
                            case BKSL_CHAR:
                                prefix_length = (in_pos - text) - 1; /* the number of characters preceding this one */
                                backslash_count += 1;                /* the total number of backslashes on this line */
                                nonws = CIF_FALSE;                   /* reset the nonws flag */
                                break;
                            case CR_CHAR:
                                /* convert CR and CR LF to LF */
                                *(buf_pos - 1) = LF_CHAR;
                                if ((in_pos < in_limit) && (*in_pos == LF_CHAR)) {
                                    in_pos += 1;
                                }
                                /* fall through */
                            case LF_CHAR:
                                goto end_of_line;
                            default:
                                if (CLASS_OF(c, scanner) != WS_CLASS) {
                                    nonws = CIF_TRUE;
                                }
                                break;
                        }
                    }

                    end_of_line:
                    /* analyze the results of the scan of the first line */
                    prefix_length = ((scanner->prefix_removing > 0) ? (prefix_length + 1 - backslash_count) : 0);
                    if (!nonws && ((backslash_count == 1)
                            || ((backslash_count == 2) && (prefix_length > 0) && (text[prefix_length] == BKSL_CHAR)))) {
                        /* prefixed or folded or both */
                        folded = (((prefix_length == 0) || (backslash_count == 2)) && (scanner->line_unfolding > 0));

                        /* discard the text already copied to the buffer */
                        buf_pos = buffer;
                    } else {
                        /* neither prefixed nor folded */
                        prefix_length = 0;
                        folded = CIF_FALSE;
                        /* keep the buffered text */
                    }
                }

                if (!folded && (prefix_length == 0)) {
                    /* copy the rest of the text, performing line terminator conversion as appropriate */
                    UChar *in_start = in_pos;
                    int32_t length;

                    while (in_pos < in_limit) {
                        UChar c = *in_pos;

                        if (c == CR_CHAR) {
                            length = in_pos++ - in_start;
                            u_strncpy(buf_pos, in_start, length);
                            buf_pos += length;
                            *(buf_pos++) = LF_CHAR;
                            if ((in_pos < in_limit) && (*in_pos == LF_CHAR)) {
                                in_pos += 1;
                            }
                            in_start = in_pos;
                        } else {
                            in_pos += 1;
                        }
                    }

                    length = in_limit - in_start;
                    u_strncpy(buf_pos, in_start, length);
                    buf_pos[length] = 0;
                } else {

                    /* process the text line-by-line, confirming and consuming prefixes and unfolding as appropriate */
                    while (in_pos < in_limit) { /* each line */
                        UChar *buf_temp = NULL;

                        /* consume the prefix, if any */
                        if (prefix_length > 0) {
                            if (((in_limit - in_pos) >= prefix_length)
                                    && (u_strncmp(text, in_pos, prefix_length) == 0)) {
                                in_pos += prefix_length;
                            } else {
                                /* error: missing prefix */
                                result = scanner->error_callback(CIF_MISSING_PREFIX, scanner->line,
                                        1, TVALUE_START(scanner), 0, scanner->user_data);
                                if (result != CIF_OK) {
                                    FAIL(late, result);
                                }
                                /* recover by copying the un-prefixed text (no special action required for that) */
                            }
                        }

                        /* copy from input to buffer, up to and including EOL, accounting for folding if applicable */
                        while (in_pos < in_limit) { /* each UChar unit */
                            UChar c = *(in_pos++);

                            /* copy the character unconditionally */
                            *(buf_pos++) = c;

                            /* handle line folding where appropriate */
                            if ((c == BKSL_CHAR) && folded) {
                                /* buf_temp tracks the position of the backslash in the output buffer */
                                buf_temp = buf_pos - 1;
                            } else {
                                int class = CLASS_OF(c, scanner);

                                if (class == EOL_CLASS) {
                                    /* convert CR and CR LF to LF */
                                    if (c == CR_CHAR) {
                                        *(buf_pos - 1) = LF_CHAR;
                                        if ((in_pos < in_limit) && (*in_pos == LF_CHAR)) {
                                            in_pos += 1;
                                        }
                                    }

                                    /* if appropriate, rewind the output buffer to achieve line folding */
                                    if (buf_temp != NULL) {
                                        buf_pos = buf_temp;
                                        buf_temp = NULL;
                                    }

                                    break;
                                } else if (class != WS_CLASS) {
                                    buf_temp = NULL;
                                }
                            }
                        }
                    }

                    /* add a string terminator */
                    *buf_pos = 0;
                }

                /* assign the buffer to the value object */
                result = cif_value_init_char(value, buffer);
                if (result == CIF_OK) {
                    /* The value is successfully parsed / initialized; assign it to the output variable */
                    *dest = value;
                    return CIF_OK;
                } else {
                    SET_RESULT(result);
                }

                FAILURE_HANDLER(late):

                /* clean up the buffer */
                free(buffer);
            } /* else failed to allocate the text buffer */
        }

        FAILURE_HANDLER(early):
        /* clean up the value */
        if (value != *dest) cif_value_free(value); /* ignore any error */
    }

    FAILURE_TERMINUS;
}

/*
 * Ensures that the provided scanner has the next available token identified and classified.
 */
static int next_token(struct scanner_s *scanner) {
    int result = CIF_OK;
    enum token_type last_ttype = scanner->ttype;
    enum token_type ttype = last_ttype;

    /* any token may follow these without intervening whitespace: */
    int after_ws = ((last_ttype == END) || (last_ttype == OLIST) || (last_ttype == OTABLE) || (last_ttype == KEY)
            || (last_ttype == TKEY));

    while ((scanner->text_start >= scanner->next_char) /* else there is a token ready */
            && (result == CIF_OK)) {
        UChar c;
        int clazz;

        ttype = ERROR;  /* it would be an internal error for this token type to escape this function */
        TVALUE_SETSTART(scanner, scanner->text_start);
        TVALUE_SETLENGTH(scanner, 0);
        NEXT_CHAR(scanner, c, result);
        if (result != CIF_OK) {
            break;
        }

        /* Require whitespace separation between most tokens, but not between keys and values (where it is forbidden) */
        clazz = CLASS_OF(c, scanner);
        if (!after_ws) {
            switch (scanner->meta_class[clazz]) {
                case WS_META:
                case CLOSE_META:
                    /* do nothing: whitespace is never required before whitespace or before closing brackets / braces */
                    break;
                default:
                    /* error: missing whitespace */
                    result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line, scanner->column - 1,
                            scanner->next_char - 1, 0, scanner->user_data);
                    if (result != CIF_OK) {
                        return result;
                    }
                    /* recover by assuming the missing whitespace */
                    break;
            }
        }

        switch (clazz) {
            case EOL_CLASS:
                /* back up the scanner to ensure that scan_ws() performs correct line and column accounting */
                BACK_UP(scanner);
                /* fall through */
            case WS_CLASS:
                result = scan_ws(scanner);
                if (result == CIF_OK) {
                    /* notify the configured whitespace callback, if any */
                    OPTIONAL_VOIDCALL(scanner->whitespace_callback, (scanner->line, scanner->column,
                            TVALUE_START(scanner), TVALUE_LENGTH(scanner), scanner->user_data));

                    /* move on */
                    CONSUME_TOKEN(scanner); /* FIXME: is this redundant? */
                    after_ws = CIF_TRUE;
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
            case HASH_CLASS:
                result = scan_to_eol(scanner);
                if (result == CIF_OK) {
                    /* notify the configured whitespace callback, if any */
                    OPTIONAL_VOIDCALL(scanner->whitespace_callback, (scanner->line, scanner->column,
                            TVALUE_START(scanner), TVALUE_LENGTH(scanner), scanner->user_data));

                    /* move on */
                    CONSUME_TOKEN(scanner); /* FIXME: is this redundant? */
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
            case EOF_CLASS:
                ttype = END;
                result = CIF_OK;
                break;
            case UNDERSC_CLASS:
                ttype = NAME;
                result = scan_to_ws(scanner);
                break;
            case OBRAK_CLASS:
                ttype = OLIST;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case CBRAK_CLASS:
                ttype = CLIST;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case OCURL_CLASS:
                ttype = OTABLE;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case CCURL_CLASS:
                ttype = CTABLE;
                TVALUE_SETLENGTH(scanner, 1);
                break;
            case QUOTE_CLASS:
                result = scan_delim_string(scanner);

                if (result == CIF_OK) {
                    /* determine whether this is a value or a table key */

                    /* peek ahead at the next character to see whether this looks like a table key */
                    PEEK_CHAR(scanner, c, result);

                    if (result == CIF_OK) {
                        if (c == COLON_CHAR) {
                            /*
                             * can only happen in CIF 2 mode, for in CIF 1 mode the character after a closing quote is
                             * always whitespace 
                             */
                            scanner->next_char += 1;
                            ttype = KEY;
                        } else {
                            ttype = QVALUE;
                        }
                    }
                }
                break;
            case SEMI_CLASS:
                if (POSN_COLUMN(scanner) == 1) {  /* semicolon was in column 1 */
                    if ((result = scan_text(scanner)) == CIF_OK) {
                        ttype = TVALUE;

                        if (scanner->cif_version >= 2) {
                            /* peek ahead at the next character to see whether this looks like a (bogus) table key */
                            PEEK_CHAR(scanner, c, result);

                            if (result == CIF_OK) {
                                if (c == COLON_CHAR) {
                                    /* FIXME: is it necessary to diagnose this as an error? */
                                    scanner->next_char += 1;
                                    ttype = TKEY;
                                }
                            }
                        }
                    }
                } else {
                    /* exactly as the default case, but we can't fall through */
                    result = scan_unquoted(scanner);
                    ttype = VALUE;
                }
                break;
            case OBRAK1_CLASS:
            case CBRAK1_CLASS:
                /* CIF 1 treatment of opening and closing square brackets */
            case DOLLAR_CLASS:
                /* error: disallowed start char for a ws-delimited value */
                result = scanner->error_callback(CIF_INVALID_BARE_VALUE, scanner->line,
                        scanner->column, TVALUE_START(scanner), 1, scanner->user_data);
                if (result != CIF_OK) {
                    break;
                }
                /* recover by scanning it as the start of a ws-delimited value anyway */
                /* fall through */
            default:
                if ((c & 0xf800) == 0xd800) {
                    /* a surrogate code unit; back up to ensure proper handling in scan_unquoted() */
                    BACK_UP(scanner);
                }
                result = scan_unquoted(scanner);
                ttype = VALUE;
                break;
        }

        if ((result == CIF_OK) && (ttype == VALUE)) {

            /*
             * Case-insensitive and locale- and codepage-independent detection of reserved words.
             * It's a bit verbose, but straightforward.
             */

            UChar *token = TVALUE_START(scanner);
            size_t token_length = scanner->tvalue_length;

            if ((token_length > 4) && (CLASS_OF(token[4], scanner) == UNDERSC_CLASS)) {
                /* check for data_*, save_*, loop_, and stop_ */
                if (         (CLASS_OF(*token, scanner) == D_CLASS)
                        && (CLASS_OF(token[1], scanner) == A_CLASS)
                        && (CLASS_OF(token[2], scanner) == T_CLASS)
                        && (CLASS_OF(token[3], scanner) == A_CLASS)) {
                    if (token_length == 5) {
                        /* error: 'data_' reserved word / missing block code */
                        result = scanner->error_callback(CIF_RESERVED_WORD, scanner->line,
                                scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                TVALUE_LENGTH(scanner), scanner->user_data);
                        /* prepare to recover by dropping it */
                        CONSUME_TOKEN(scanner);
                    } else {
                        ttype = BLOCK_HEAD;
                        TVALUE_INCSTART(scanner, 5);
                        TVALUE_INCLENGTH(scanner, -5);
                    }
                } else if (  (CLASS_OF(*token, scanner) == S_CLASS)
                        && (CLASS_OF(token[1], scanner) == A_CLASS)
                        && (CLASS_OF(token[2], scanner) == V_CLASS)
                        && (CLASS_OF(token[3], scanner) == E_CLASS)) {
                    ttype = ((token_length == 5) ? FRAME_TERM : FRAME_HEAD);
                    TVALUE_INCSTART(scanner, 5);
                    TVALUE_INCLENGTH(scanner, - 5);
                } else if ((token_length == 5)
                        && (CLASS_OF(token[2], scanner) == O_CLASS)
                        && (CLASS_OF(token[3], scanner) == P_CLASS)) {
                    if (         (CLASS_OF(*token, scanner) == L_CLASS)
                            && (CLASS_OF(token[1], scanner) == O_CLASS)) {
                        ttype = LOOPKW;
                        TVALUE_INCSTART(scanner, 5);
                        TVALUE_INCLENGTH(scanner, - 5);
                    } else if (  (CLASS_OF(*token, scanner) == S_CLASS)
                            && (CLASS_OF(token[1], scanner) == T_CLASS)) {
                        /* error: 'stop_' reserved word */
                        result = scanner->error_callback(CIF_RESERVED_WORD, scanner->line,
                                scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                TVALUE_LENGTH(scanner), scanner->user_data);
                        /* prepare to recover by dropping it */
                        CONSUME_TOKEN(scanner);
                    } 
                }
            } else if ((token_length == 7) && (CLASS_OF(token[6], scanner) == UNDERSC_CLASS)
                    &&   (CLASS_OF(*token, scanner) == G_CLASS)
                    && (CLASS_OF(token[1], scanner) == L_CLASS)
                    && (CLASS_OF(token[2], scanner) == O_CLASS)
                    && (CLASS_OF(token[3], scanner) == B_CLASS)
                    && (CLASS_OF(token[4], scanner) == A_CLASS)
                    && (CLASS_OF(token[5], scanner) == L_CLASS)) {
                /* error: 'global_' reserved word */
                result = scanner->error_callback(CIF_RESERVED_WORD, scanner->line,
                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                        TVALUE_LENGTH(scanner), scanner->user_data);
                /* prepare to recover by dropping it */
                CONSUME_TOKEN(scanner);
            }

            /* Set the (possibly-corrected) token type in the scanner */
        } /* endif result == CIF_OK && ttype == VALUE */
    } /* end while */

    scanner->ttype = ttype;

    return result;
}

/*
 * Scans a run of whitespace.  Proper line and column counting depends on all line terminators being seen by this
 * function.
 */
static int scan_ws(struct scanner_s *scanner) {
    int sol = 0;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        for (; scanner->next_char < top; scanner->next_char += 1) {
            UChar c = *(scanner->next_char);

            switch (CLASS_OF(c, scanner)) {
                case WS_CLASS:
                    /* increment the column number; c is assumed to not be a surrogate code value */
                    POSN_INCCOLUMN(scanner, 1);
                    sol = 0;
                    break;
                case EOL_CLASS:
                    if (POSN_COLUMN(scanner) > CIF_LINE_LENGTH) {
                        /* error: long line */
                        result = scanner->error_callback(CIF_OVERLENGTH_LINE, scanner->line,
                                scanner->column, scanner->next_char, 0, scanner->user_data);
                        if (result != CIF_OK) {
                            return result;
                        }
                        /* recover by accepting it as-is */
                    }

                    /* the next character is at the start of a line; sol encodes data about the preceding terminators */
                    sol = ((sol << 2) + ((c == CR_CHAR) ? 2 : 1)) & 0xf;
                    /*
                     * sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
                     * the line number was incremented for the CR, and should not be incremented again for the LF.
                     */
                    POSN_INCLINE(scanner, ((sol == 0x9) ? 0 : 1));
                    break;
                default:
                    goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

static int scan_to_ws(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            } else if (METACLASS_OF(c, scanner) == WS_META) {
                goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    /* We've scanned one code unit too far; back off */
    TVALUE_SETLENGTH(scanner, (--scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

static int scan_to_eol(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }
            switch (CLASS_OF(c, scanner)) {
                case EOL_CLASS:
                case EOF_CLASS:
                    goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    /* We've scanned one code unit too far; back off */
    TVALUE_SETLENGTH(scanner, (--scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

static int scan_unquoted(struct scanner_s *scanner) {
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }
            switch (METACLASS_OF(c, scanner)) {
                case OPEN_META:
                    /* error: missing whitespace between values */
                    /* this doesn't occur in CIF 1 mode because no characters are mapped to the OPEN_META there */
                    result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line,
                            scanner->column, scanner->next_char - 1, 0, scanner->user_data);
                    if (result != CIF_OK) {
                        return result;
                    }
                    /* fall through */
                case CLOSE_META:
                case WS_META:
                    goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    /* We've scanned one code unit too far; back off */
    TVALUE_SETLENGTH(scanner, (--scanner->next_char - TVALUE_START(scanner)));
    return CIF_OK;
}

/*
 * Scans a string delimited by matching characters, the opening one being at the current token start position.  The
 * scan will stop with a recoverable error if it reaches the end of the initial line.  The token start position is
 * advanced to immediately past the opening delimiter, and the resulting token length accounts for only the content,
 * not the delimiters.
 */
static int scan_delim_string(struct scanner_s *scanner) {
    UChar delim = *(scanner->text_start);
    int lead_surrogate = CIF_FALSE;
    int delim_size;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }

            if (c == delim) {
                if (scanner->cif_version < 2) {
                    /* look ahead for whitespace */
                    PEEK_CHAR(scanner, c, result);
                    if (result != CIF_OK) {
                        return result;
                    } else if (METACLASS_OF(c, scanner) != WS_META) {
                        /* the current character is part of the value */
                        continue;
                    }
                }
                delim_size = 1;
                goto end_of_token;
            } else {
                switch (CLASS_OF(c, scanner)) {
                    case EOL_CLASS:
                    case EOF_CLASS:
                        /* error: unterminated quoted string */
                        result = scanner->error_callback(CIF_MISSING_ENDQUOTE, scanner->line,
                                scanner->column, scanner->next_char - 1, 0, scanner->user_data);
                        if (result != CIF_OK) {
                            return result;
                        }
                        /* recover by assuming a trailing close-quote */
                        delim_size = 0;
                        goto end_of_token;
                }
            }
        }

        result = get_more_chars(scanner);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    /* The token value starts after the opening delimiter */
    TVALUE_INCSTART(scanner, 1);
    /* The token length excludes the closing delimiter (if any) */
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)) - delim_size);

    return CIF_OK;
}


/*
 * Scans a text block.  Sets the token parameters to mark the block contents, exclusive of delimiters, assuming the
 * initial token start position to be at the leading semicolon.
 */
static int scan_text(struct scanner_s *scanner) {
    int delim_size = 0;
    int sol = 0;
    int lead_surrogate = CIF_FALSE;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;
        int result;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }

            switch (CLASS_OF(c, scanner)) {
                case SEMI_CLASS:
                    if (sol != 0) {
                        delim_size = ((*(scanner->next_char - 2) == LF_CHAR)
                                && (*(scanner->next_char - 3) == CR_CHAR)) ? 3 : 2;
                        goto end_of_token;
                    }
                    break;
                case EOL_CLASS:
                    if (POSN_COLUMN(scanner) > CIF_LINE_LENGTH) {
                        /* error: long line */
                        result = scanner->error_callback(CIF_OVERLENGTH_LINE, scanner->line,
                                scanner->column, scanner->next_char - 1, 0, scanner->user_data);
                        if (result != CIF_OK) {
                            return result;
                        }
                        /* recover by accepting it as-is */
                    }

                    /* the next character is at the start of a line */
                    sol = ((sol << 2) + ((c == CR_CHAR) ? 2 : 1)) & 0xf;
                    /*
                     * sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
                     * the line number was incremented for the CR, and should not be incremented again for the LF.
                     */
                    POSN_INCLINE(scanner, ((sol == 0x9) ? 0 : 1));
                    break;
                case EOF_CLASS:
                    /* error: unterminated text block */
                    result = scanner->error_callback(CIF_UNCLOSED_TEXT, scanner->line,
                            scanner->column, scanner->next_char - 1, 0, scanner->user_data);
                    if (result != CIF_OK) {
                        return result;
                    }
                    /* recover by reporting out the whole tail as the token */
                    goto end_of_token;
                default:
                    sol = 0;
                    break;
            }
        }

        result = get_more_chars(scanner);
        if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_INCSTART(scanner, 1);
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)) - delim_size);
    return CIF_OK;
}

/*
 * Transfers one character from the provided scanner's character source into its working character buffer, provided
 * that any are available.  Assumes that no characters have yet been transferred, and that the CIF version being parsed
 * may not yet be known.  Will insert an EOF_CHAR into the buffer if called when there no more characters are available.
 * Returns CIF_OK if any characters are transferred (including an EOF marker), otherwise CIF_ERROR.
 *
 * Unlike get_more_chars(), this function accepts a Unicode byte-order mark, U+FEFF.
 */
static int get_first_char(struct scanner_s *scanner) {
    ssize_t nread;
    int read_error;

    assert(scanner->next_char == scanner->buffer);
    assert(scanner->text_start == scanner->buffer);
    assert(scanner->buffer_limit == 0);
    TVALUE_SETSTART(scanner, scanner->buffer);

    /* convert one character */
    nread = scanner->read_func(scanner->char_source, scanner->buffer, 1, &read_error);

    if (nread < 0) {
        return read_error;
    } else if (nread == 0) {
        *scanner->buffer = EOF_CHAR;
        scanner->buffer_limit += 1;
        scanner->at_eof = CIF_TRUE;
        return CIF_OK;
    } else {
        UChar ch = *scanner->buffer;
        int result;

        assert (nread == 1);
        if ((ch > CIF1_MAX_CHAR) ? (ch != BOM_CHAR) : (scanner->char_class[ch] == NO_CLASS)) {
            /* error: disallowed character */
            result = scanner->error_callback(CIF_DISALLOWED_INITIAL_CHAR, 1, 0,
                    scanner->buffer, 1, scanner->user_data);
            if (result != CIF_OK) {
                return result;
            }
            /* recover by accepting the character (for the moment) */
        }

        scanner->buffer_limit += 1;
        return CIF_OK;
    }
}

/*
 * Transfers characters from the provided scanner's character source into its working character buffer, provided that
 * any are available.  May move unconsumed data within the buffer (adjusting the rest of the scanner's internal state
 * appropriately) and/or may increase the size of the buffer.  Will insert an EOF_CHAR into the buffer if called when
 * there no more characters are available.  Returns CIF_OK if any characters are transferred (including an EOF
 * marker), otherwise CIF_ERROR.
 */
static int get_more_chars(struct scanner_s *scanner) {
    size_t chars_read = scanner->next_char - scanner->buffer;
    size_t chars_consumed = scanner->text_start - scanner->buffer;
    ssize_t nread;
    int read_error;

    assert(chars_read < SSIZE_T_MAX);
    assert(chars_consumed <= chars_read); /* chars_consumed == chars_read only at the beginning of a parse */
    if (chars_consumed >= scanner->buffer_limit) {
        /* the buffer is empty; reset it to the beginning */
        assert(scanner->next_char == scanner->text_start);
        scanner->text_start = scanner->buffer;
        TVALUE_SETSTART(scanner, scanner->buffer);
        scanner->next_char = scanner->buffer;
        scanner->buffer_limit = 0;
    } else if (scanner->buffer_size < (scanner->buffer_limit + BUF_MIN_FILL)) {
        /* need to make room at the top of the buffer */
        size_t current_chars = chars_read - chars_consumed;
        size_t tvalue_diff = TVALUE_START(scanner) - scanner->text_start;

        if (current_chars * 2 < scanner->buffer_size) {
            /* The current data occupy less than half the buffer; move them to the front of the buffer */
            memmove(scanner->buffer, scanner->text_start, current_chars * sizeof(UChar));
        } else {
            /*
             * extend the buffer ( == allocate a new one and copy what's needed from the old )
             *
             * malloc() is chosen over realloc() here because it is expected that in-place extension will rarely be
             * possible, and in most other cases it will be unnecessary to copy the full contents of the original
             * buffer.  Moreover, this affords the opportunity to relocate the valid buffered data to the beginning
             * of the new buffer without unneeded overhead.
             */
            size_t new_size = scanner->buffer_size * 2;
            UChar *new_buffer = (UChar *) malloc(new_size * sizeof(UChar));

            if (new_buffer == NULL) {
                return CIF_ERROR;
            } else {
                /* preserve only the needed data */
                memcpy(new_buffer, scanner->text_start, current_chars * sizeof(UChar));

                /* assign the expanded buffer to the scanner */
                free(scanner->buffer);
                scanner->buffer = new_buffer;
                scanner->buffer_size = new_size;
            }
        }

        /* either way, update scanner state to reflect the changes */
        scanner->text_start = scanner->buffer;
        TVALUE_SETSTART(scanner, scanner->buffer + tvalue_diff);
        scanner->next_char = scanner->buffer + current_chars;
        scanner->buffer_limit = current_chars;
    }

    /* once EOF has been detected, don't attempt to read from the character source any more */
    nread = scanner->at_eof ? 0 : scanner->read_func(scanner->char_source, scanner->buffer + scanner->buffer_limit,
                scanner->buffer_size - scanner->buffer_limit, &read_error);

    if (nread < 0) {
        return read_error;
    } else if (nread == 0) {
        scanner->buffer[scanner->buffer_limit] = EOF_CHAR;
        scanner->buffer_limit += 1;
        scanner->at_eof = CIF_TRUE;
        return CIF_OK;
    } else {
        UChar *start = scanner->buffer + scanner->buffer_limit;
        UChar *end = start + nread;
        int result;

        for (; start < end; start += 1) {
            if ((*start < CHAR_TABLE_MAX) ? (scanner->char_class[*start] == NO_CLASS)
                    : ((*start > 0xFFFD) || (*start == BOM_CHAR) || ((*start >= 0xFDD0) && (*start <= 0xFDEF)))) {
                /* error: disallowed BMP character */
                result = scanner->error_callback(CIF_DISALLOWED_CHAR, scanner->line, scanner->column,
                        scanner->next_char - 1, 1, scanner->user_data);
                if (result != CIF_OK) {
                    return result;
                }
                /* recover by replacing with a replacement char */
                *start = ((scanner->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR);
            }
        }

        if (scanner->cif_version < 2) {
            for (start = scanner->buffer + scanner->buffer_limit; start < end; start += 1) {
                if (*start > CIF1_MAX_CHAR) {
                    /* error: disallowed CIF 1 character */
                    result = scanner->error_callback(CIF_DISALLOWED_CHAR, scanner->line, scanner->column,
                            scanner->next_char - 1, 1, scanner->user_data);
                    if (result != CIF_OK) {
                        return result;
                    }
                    /* recover by accepting the character */
                }
            }
        }

        scanner->buffer_limit += nread;
        return CIF_OK;
    }
}
