/*
 * parser.c
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

/**
 * @page parsing CIF parsing
 * The CIF API provides a parser for CIF 2.0, flexible enough to parse CIF 1.1 documents and those complying with
 * earlier CIF conventions as well.
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
 * return, and line feed control characters (albeit not necessarily @em encoded according to ASCII).  This period was
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
 * A variety of options and extension points are available via the @c options argument to the parse function,
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
 * @c cif_parse() function operates in this way when its third argument points to a location for a CIF
 * handle: @include basic_parsing.c
 *
 * By default, however, the parser stops at the first error it encounters.  Inasmuch as historically, many CIFs
 * have contained at least minor errors, it may be desirable to instruct the parser to attempt to push past all or
 * certain kinds of errors, extracting @ref error_recovery "a best-guess interpretation of the remainder of the input".
 * Such behavior can be obtained by providing an
 * error-handling callback function of type matching @c cif_parse_error_callback_tp .  Such a function serves not only
 * to control which errors are bypassed, but also, if so written, to pass on details of each error to the caller.  For
 * example, this code counts the number of CIF syntax and semantic errors in the input CIF: @include parse_errors.c
 * 
 * For convenience, the CIF API provides two default error handler callback functions, @c cif_parse_error_die() and
 * @c cif_parse_error_ignore().  As their names imply, the former causes the parse to be aborted on any error (the
 * default behavior), whereas the latter causes all errors to silently be ignored (to the extent that is possible).
 *
 * @subsection parser-callbacks Parser callbacks
 * Parsing a CIF from an external representation is in many ways analogous to performing a depth-first traversal of
 * a pre-parsed instance of the CIF data model, as the @c cif_walk() function does.  In view of this similarity, a @c
 * cif_handler_tp object such as is also used with @c cif_walk() can be provided among the parse options to facilitate
 * caller monitoring and control of the parse process.  The handler callbacks can probe and to some
 * extent modify the CIF as it is parsed, including by instructing the parser to suppress (but not altogether skip)
 * some portions of the input.  This facility has applications from parse-time data selection to validation and beyond;
 * for example, here is a naive approach to assigning loop categories based on loop data
 * names: @include parser_callbacks.c
 *
 * Note that the parser traverses its input and issues callbacks in document order, from start to end, so unlike
 * @c cif_walk(), it does not guarantee to traverse all of a data block's save frames before any of its data.
 *
 * @subsubsection validation CIF validation
 * The CIF API does not provide specific support for CIF validation because validation is dependent on the DDL of
 * the dictionary to which a CIF purports to comply, whereas the CIF API is generic, not specific to any particular
 * DDL or dictionary.  To the extent that some validations can be performed during parsing, however, callback functions
 * provide a suitable means for interested applications to engage such validation.
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
 * All provided callbacks are invoked as normal in this mode, including any error callback, and regardless of any
 * callbacks, the parser's return code indicates whether any errors were detected.  This syntax-only parsing mode does
 * have a few limitations, however: primarily that because it does not retain an in-memory representation of its
 * input, it cannot check CIF semantic requirements for data name, frame code, and block code uniqueness within their
 * respective scopes.
 *
 * @subsection event-driven Event-driven parsing
 * The availability and scope of callback functions make the syntax-only mode described above a CIF analog of the
 * event-driven "SAX" XML parsing interface.  To use the parser in that mode, the caller provides callback functions
 * by which to be informed of parse "events" of interest -- recognition of entities and entity boundaries -- so as to
 * extract the desired information during the parse instead of by afterward analyzing the parsed result.  Callbacks can
 * communicate with themselves and each other, and can memorialize data for the caller, via the @c user_data object
 * provided among the parse options (and demonstrated in the error-counting example).  Callbacks can be omitted
 * for events that are not of interest.
 */

/**
 * @page error_recovery Parse error recovery
 * Since the built-in parser allows syntax and grammar errors to be ignored, it must provide a mechanism for recovering
 * from such errors to continue the parse.  In general, there is no single, clear approach for recovering from any
 * given error, so the following table documents the approach taken by the parser in each case.
 * <table>
 * <tr><th>Condition</th><th>Code</th><th>Recovery action</th></tr>
 * <tr><td>Wrong character encoding</td><td>@c CIF_WRONG_ENCODING</td><td>ignore the problem</td></tr>
 * <tr><td colspan='3'>CIF 2.0 must consist of Unicode character data encoded in UTF-8, and this error is emitted if the
 * input is recognized to be encoded differently.  That this error is emitted at all generally means that the parser has
 * identified the signature of a different (known) encoding, so it can recover by reading the data according to the
 * detected encoding, even though such input does not comply with the CIF 2.0 specifications.</td></tr>
 * <tr><td>Disallowed input character</td><td>@c CIF_DISALLOWED_CHAR</td><td>substitute a replacement character</td></tr>
 * <tr><td colspan='3'>This error indicates that an input character outside the allowed set was read.  The parser can
 *     recover by accepting the character.  Which characters are allowed depends on which version of CIF is being
 *     parsed.</td></tr>
 * <tr><td>Missing whitespace</td><td>@c CIF_MISSING_SPACE</td><td>assume the omitted whitespace</td></tr>
 * <tr><td colspan='3'>Whitespace separation is required between most CIF grammatic units.  In some cases, the omission
 *     of such whitespace can be recognized by the parser, resulting in this error.  In particular, this is the error
 *     that will be reported when a CIF1-style string with embedded delimiter is encountered when parsing in CIF 2 mode.
 *     If the opening delimiter of a table is omitted then this error will occur at trailing colon of each table key.</td></tr>
 * <tr><td>Invalid block code</td><td>@c CIF_INVALID_BLOCKCODE</td><td>use the block code anyway</td></tr>
 * <tr><td colspan='3'>Although the API's CIF manipulation functions will not allow blocks with invalid codes to be
 *     created directly by client programs, the parser can and will create such blocks to accommodate inputs that
 *     use such codes.  The result is not a valid instance of the CIF data model.</td></tr>
 * <tr><td>Duplicate block code</td><td>@c CIF_DUP_BLOCKCODE</td><td>reopen the specified block</td></tr>
 * <tr><td colspan='3'>Block codes must be unique within a given CIF.  To handle a duplicate block code, the parser
 *     reopens the specified block and parses the following contents into it.  This may well lead to additional errors
 *     being reported.</td></tr>
 * <tr><td>Missing block header</td><td>@c CIF_NO_BLOCK_HEADER</td><td>parse into an anonymous block</td></tr>
 * <tr><td colspan='3'>To handle data that appear prior to any block header, the parser creates a data block with an
 *     empty name and parses into that.  The data are available via that name, but the result is not a valid instance of
 *     the CIF data model.</td></tr>
 * <tr><td>Invalid frame code</td><td>@c CIF_INVALID_FRAMECODE</td><td>use the frame code anyway</td></tr>
 * <tr><td colspan='3'>Although the API's CIF manipulation functions will not allow save frames with invalid codes to be
 *     created directly by client programs, the parser can and will create such frames to accommodate inputs that
 *     use such codes.  The result is not a valid instance of the CIF data model.</td></tr>
 * <tr><td>Duplicate frame code</td><td>@c CIF_DUP_FRAMECODE</td><td>reopen the specified frame</td></tr>
 * <tr><td colspan='3'>Save frame codes must be unique within a their containing block or save frame.  To handle a
 *     duplicate frame code, the parser reopens the specified frame and parses the following contents into it.  This may
 *     well lead to additional errors being reported.</td></tr>
 * <tr><td>Disallowed save frame</td><td>@c CIF_FRAME_NOT_ALLOWED</td><td>accept the save frame</td></tr>
 * <tr><td colspan='3'>This error occurs when a save frame header is encountered while parsing with save frame support
 *     completely disabled.  The parser recovers by parsing the frame as if save frame support were enabled at the
 *     default level.</td></tr>
 * <tr><td>Unterminated save frame</td><td>@c CIF_NO_FRAME_TERM</td><td>assume the missing terminator</td></tr>
 * <tr><td colspan='3'>This error occurs when a data block header is encountered while parsing a save frame, or when a
 *     save frame header is encountered while parsing a save frame with nested frames disabled (the default).  The
 *     parser recovers by assuming the missing save frame terminator at the position where the error is detected.</td></tr>
 * <tr><td>Unterminated save frame at end-of-file</td><td>@c CIF_EOF_IN_FRAME</td><td>assume the missing terminator</td></tr>
 * <tr><td colspan='3'>This is basically the same as the CIF_NO_FRAME_TERM case, but triggered when the end of input
 *     occurs while parsing a save frame.  This case is distinguished in part because it may indicate a truncated
 *     input.</td></tr>
 * <tr><td>Unexpected save frame terminator</td><td>@c CIF_UNEXPECTED_TERM</td><td>ignore</td></tr>
 * <tr><td colspan='3'>If a save frame terminator is encountered outside the scope of a save frame, the parser recovers
 *     by ignoring it.  This condition cannot be distinguished from the alternative that a save frame header is given
 *     without any frame code.</td></tr>
 * <tr><td>Duplicate data name</td><td>@c CIF_DUP_ITEMNAME</td><td>parse and drop the item</td></tr>
 * <tr><td colspan='3'>If a duplicate item name is encountered then it and its associated value(s) are dropped,
 *     including when the duplicate appears in a loop.</td></tr>
 * <tr><td>Unexpected value</td><td>@c CIF_UNEXPECTED_VALUE</td><td>ignore the value</td></tr>
 * <tr><td colspan='3'>This error occurs when a value appears outside a list or loop without being paired with a
 *     dataname or (in a table) a key.</td></tr>
 * <tr><td>Unexpected (closing) delimiter</td><td>@c CIF_UNEXPECTED_DELIM</td><td>ignore the delimiter</td></tr>
 * <tr><td colspan='3'>This error occurs when a list or table closing delimiter appears without matching opening
 *     delimiter preceding it.  This can happen when such a delimiter appears in the middle of a whitespace-delimited
 *     data value.</td></tr>
 * <tr><td>Missing data value</td><td>@c CIF_MISSING_VALUE</td><td>use a synthetic unknown value</td></tr>
 * <tr><td colspan='3'>This error occurs when a data name or table key appears without a paired value.  The parser
 *     recovers by synthesizing unknown-value placeholder value.</td></tr>
 * <tr><td>Empty loop header</td><td>@c CIF_NULL_LOOP</td><td>ignore</td></tr>
 * <tr><td colspan='3'>This occurs when the @c loop_ keyword appears without at least one subsequent data name.  The
 *     parser recovers by ignoring it.</td></tr>
 * <tr><td>Truncated loop packet</td><td>@c CIF_PARTIAL_PACKET</td><td>fill out the packet with unknown values</td></tr>
 * <tr><td colspan='3'>This error occurs when the number of data values in a loop is not an integral multiple of the
 *     number of data names.  In such cases, the parser can recover by filling in the missing values with out with
 *     unknown-value placeholder values.</td></tr>
 * <tr><td>Empty loop</td><td>@c CIF_EMPTY_LOOP</td><td>accept</td></tr>
 * <tr><td colspan='3'>This occurs when a valid loop header is not followed by any values.  The parser recovers by
 *     accepting the empty loop, which can be accommodated by the API's internal CIF representation.  The result is not
 *     a valid instance of the CIF data model, however.</td></tr>
 * <tr><td>Unterminated list or table</td><td>@c CIF_MISSING_DELIM</td><td>assume the missing delimiter</td></tr>
 * <tr><td colspan='3'>If the closing delimiter of a list or table is omitted, then the parser recovers by assuming the
 *     terminator to appear at the point where its absence is recognized.</td></tr>
 * <tr><td>Missing table key</td><td>@c CIF_MISSING_KEY</td><td>drop the value</td></tr>
 * <tr><td colspan='3'>If a value appears inside a table without an associated key, then the parser recovers by
 *     dropping it.</td></tr>
 * <tr><td>Missing table key</td><td>@c CIF_NULL_KEY</td><td>use a NULL key</td></tr>
 * <tr><td colspan='3'>If a table entry contains a (colon, value) without any key representation at all (not even an
 *     empty string), then the parser can recover by using a NULL key.  The result is not a valid instance of the
 *     CIF data model.</td></tr>
 * <tr><td>Unquoted table key</td><td>@c CIF_UNQUOTED_KEY</td><td>accept</td></tr>
 * <tr><td colspan='3'>This case is distinguished from the @c CIF_MISSING_KEY case by the presence of a colon in
 *     the unquoted value.  The key is taken as everything up to the first colon, and the value as everything
 *     after</td></tr>
 * <tr><td>Text block as a table key</td><td>@c CIF_MISQUOTED_KEY</td><td>accept</td></tr>
 * <tr><td colspan='3'>CIF 2.0 does not allow text blocks to be used as table keys, but this is a somewhat artificial
 *     restriction.  If the parser encounters a table key quoted with newline/semicolon delimiters then it can recover
 *     by accepting that key as valid.</td></tr>
 * <tr><td>Missing text prefix</td><td>@c CIF_MISSING_PREFIX</td><td>accept</td></tr>
 * <tr><td colspan='3'>The text prefixing protocol requires every line of a prefixed text field to start with the
 *     chosen prefix.  If any line fails to do so then the parser can typically recover by simply accepting that
 *     line verbatim.</td></tr>
 * <tr><td>Invalid unquoted value</td><td>@c CIF_INVALID_BARE_VALUE</td><td>accept</td></tr>
 * <tr><td colspan='3'>A whitespace-delimited data value has a restricted character repertoire and a more-restricted
 *     first character.  When the parser recognizes that one of these restrictions has not been obeyed, it can recover
 *     by accepting the value as-is.</td></tr>
 * <tr><td>Unquoted reserved word</td><td>@c CIF_RESERVED_WORD</td><td>drop</td></tr>
 * <tr><td colspan='3'>The the strings 'data_' (without a block code), 'stop_', and 'global_' are reserved and must
 *     not appear as unquoted complete words in CIFs.  If the parser encounters one, it can recover by dropping
 *     it.</td></tr>
 * <tr><td>Overlength line</td><td>@c CIF_OVERLENGTH_LINE</td><td>drop</td></tr>
 * <tr><td colspan='3'>If a CIF input line exceeds the allowed number of @em characters (2048 in CIF 1.1 and CIF 2.0)
 *     then the parser can recover by ignoring the problem.  Note that the limit is expressed in Unicode characters --
 *     not bytes, nor even @c UChar code units -- and it does not include line terminators.</td></tr>
 * <tr><td>Missing endquote</td><td>@c CIF_MISSING_ENDQUOTE</td><td>assume the quote</td></tr>
 * <tr><td colspan='3'>When a (single-) apostrophe-quoted or quotation-mark-quoted string is not terminated before the end of
 *     the line on which it begins, the parser can recover by assuming the missing delimiter at the end of the line.
 *     </td></tr>
 * <tr><td>Unterminated multiline string</td><td>@c CIF_UNCLOSED_TEXT</td><td>assume the closing delimiter</td></tr>
 * <tr><td colspan='3'>When a text block or triple-apostrophe-quoted or triple-quotation-mark-quoted string is not terminated
 *     before the end of the end of the input, the parser can recover by assuming the missing delimiter at the end of
 *     the input.  In such cases, that is often much more text than the value was meant to include, but there is no
 *     reliable way to determine where it was supposed to end.</td></tr>
 * <tr><td>Disallowed first character</td><td>@c CIF_DISALLOWED_INITIAL_CHAR</td><td>accept</td></tr>
 * <tr><td colspan='3'>There are slightly different rules for the first character of a CIF than for others, in that a Unicode
 *     byte-order mark (U+FEFF) is allowed there.  Moreover, an unexpected character at that position can be an
 *     indication of a mis-identified character encoding.  The parser can recover by accepting the character, but
 *     that @em will result in at least one subsequent error.</td></tr>
 * </table>
 */

/*
 * The CIF parser implemented herein is a predictive recursive descent parser with full error recovery.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "internal/compat.h"

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
#include "internal/utils.h"
#include "internal/value.h"

/* plain macros */

#define SSIZE_T_MAX   (((size_t) -1) / 2)

/* a result code used internally to this file to signal that no more input characters are available */
#define CIF_EOF       -1

/*
 * It is now common for CIFs to have one or more multi-kilobyte values, so start with a largish buffer to minimize
 * the number of buffer expansions likely to be needed
 */
#define BUF_SIZE_INITIAL (64 * (CIF_LINE_LENGTH + 2))
#define BUF_MIN_FILL          (CIF_LINE_LENGTH + 2)

/* special character codes */
#define CIF1_MAX_CHAR 0x7E
#define EOF_CHAR      0xFFFF

#if (CHAR_TABLE_MAX <= CIF1_MAX_CHAR)
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
static int parse_cif(struct scanner_s *scanner, cif_tp *cifp);
static int parse_container(struct scanner_s *scanner, cif_container_tp *container, int is_block);
static int parse_item(struct scanner_s *scanner, cif_container_tp *container, UChar *name);
static int parse_loop(struct scanner_s *scanner, cif_container_tp *container);
static int parse_loop_header(struct scanner_s *scanner, cif_container_tp *container, string_element_tp **name_list_head,
        int *name_countp);
static int parse_loop_packets(struct scanner_s *scanner, cif_loop_tp *loop, string_element_tp *first_name,
        UChar *names[], int column_count);
static int parse_list(struct scanner_s *scanner, cif_value_tp **listp);
static int parse_table(struct scanner_s *scanner, cif_value_tp **tablep);
static int parse_value(struct scanner_s *scanner, cif_value_tp **valuep);

/* scanning functions */
static int next_token(struct scanner_s *scanner);
static int scan_ws(struct scanner_s *scanner);
static int scan_to_ws(struct scanner_s *scanner);
static int scan_to_eol(struct scanner_s *scanner);
static int scan_unquoted(struct scanner_s *scanner);
static int scan_delim_string(struct scanner_s *scanner);
static int scan_triple_delim_string(struct scanner_s *scanner);
static int scan_text(struct scanner_s *scanner);
static int get_first_char(struct scanner_s *scanner);
static int get_more_chars(struct scanner_s *scanner);

/* other functions */
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_tp **dest);

/* function-like macros */

#define INIT_V2_SCANNER(s, ws, eol) do { \
    struct scanner_s *_s = (s); \
    int _i; \
    const char * _c; \
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
    if (ws) { \
        for (_c = ws; *_c; _c += 1) { \
            unsigned char uc = *_c; \
            if (uc < CHAR_TABLE_MAX) _s->char_class[uc] = WS_CLASS; \
        } \
    } \
    if (eol) { \
        for (_c = eol; *_c; _c += 1) { \
            unsigned char uc = *_c; \
            if (uc < CHAR_TABLE_MAX) _s->char_class[uc] = EOL_CLASS; \
        } \
    } \
    _s->char_class[UCHAR_TAB] =   WS_CLASS; \
    _s->char_class[UCHAR_NL] =    EOL_CLASS; \
    _s->char_class[UCHAR_CR] =    EOL_CLASS; \
    _s->char_class[0x20] =        WS_CLASS; \
    _s->char_class[0x22] =        QUOTE_CLASS; \
    _s->char_class[0x23] =        HASH_CLASS; \
    _s->char_class[0x24] =        DOLLAR_CLASS; \
    _s->char_class[0x27] =        QUOTE_CLASS; \
    _s->char_class[UCHAR_COLON] = GENERAL_CLASS; \
    _s->char_class[UCHAR_SEMI] =  SEMI_CLASS; \
    _s->char_class[0x5B] =        OBRAK_CLASS; \
    _s->char_class[0x5D] =        CBRAK_CLASS; \
    _s->char_class[0x5F] =        UNDERSC_CLASS; \
    _s->char_class[0x7B] =        OCURL_CLASS; \
    _s->char_class[0x7D] =        CCURL_CLASS; \
    _s->char_class[0x41] =        A_CLASS; \
    _s->char_class[0x61] =        A_CLASS; \
    _s->char_class[0x42] =        B_CLASS; \
    _s->char_class[0x62] =        B_CLASS; \
    _s->char_class[0x44] =        D_CLASS; \
    _s->char_class[0x64] =        D_CLASS; \
    _s->char_class[0x45] =        E_CLASS; \
    _s->char_class[0x65] =        E_CLASS; \
    _s->char_class[0x47] =        G_CLASS; \
    _s->char_class[0x67] =        G_CLASS; \
    _s->char_class[0x4C] =        L_CLASS; \
    _s->char_class[0x6C] =        L_CLASS; \
    _s->char_class[0x4F] =        O_CLASS; \
    _s->char_class[0x6F] =        O_CLASS; \
    _s->char_class[0x50] =        P_CLASS; \
    _s->char_class[0x70] =        P_CLASS; \
    _s->char_class[0x53] =        S_CLASS; \
    _s->char_class[0x73] =        S_CLASS; \
    _s->char_class[0x54] =        T_CLASS; \
    _s->char_class[0x74] =        T_CLASS; \
    _s->char_class[0x56] =        V_CLASS; \
    _s->char_class[0x76] =        V_CLASS; \
    _s->char_class[0x7F] =        NO_CLASS; \
    _s->meta_class[NO_CLASS] =    NO_META; \
    _s->meta_class[WS_CLASS] =    WS_META; \
    _s->meta_class[EOL_CLASS] =   WS_META; \
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
    _s->line_unfolding  -= 1; \
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
 * 0xFDD0 - 0xFDEF, and per-plane not-a-character code points 0x??FFFE and 0x??FFFF (except 0xFFFF, which is co-opted
 * as an EOF marker instead).  Also surrogate code units when not part of a surrogate pair.  At this time, however,
 * there appears little benefit to this macro making those comparatively costly distinctions, especially when it
 * cannot do so at all in the case of unpaired surrogates.
 */
#define CLASS_OF(c, s)  (((c) < CHAR_TABLE_MAX) ? (s)->char_class[c] : \
        (((s)->cif_version >= 2) ? GENERAL_CLASS : NO_CLASS))

/* Evaluates the metaclass to which the class of the specified character belongs, with respect to the given scanner */
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
 * Ensures that the buffer of the specified scanner has at least one character available to read, or that the scanner's
 * end-of-file flag has been raised.  Sets 'ev' to 'CIF_OK' if there is already at least one character to read or if
 * additional characters are successfully buffered; to 'CIF_EOF' if the scanner has reached the end of the file; or
 * else to a CIF error code.
 */
#define ENSURE_CHARS(s, ev) do { \
    struct scanner_s *_s_ec = (s); \
    if (_s_ec->next_char >= _s_ec->buffer + _s_ec->buffer_limit) { \
        ev = ((_s_ec->at_eof == 0) ? get_more_chars(_s_ec) : CIF_EOF); \
    } else { \
        ev = CIF_OK; \
    } \
} while (CIF_FALSE)

/*
 * Provides the character that will be returned (again) by the next invocation of NEXT_CHAR().  May cause additional
 * characters to be read into the buffer and / or the cause the scanner's end-of-file flag to be set, but does not
 * otherwise alter the scan state.  Sets 'ev' to 'CIF_OK' if a valid character is provided, to 'CIF_EOF' if the
 * scanner has already reached the end of the input, or to a CIF error code.
 */
#define PEEK_CHAR(s, c, ev) do { \
    struct scanner_s *_s_pc = (s); \
    ENSURE_CHARS(_s_pc, ev); \
    if (ev == CIF_OK) { \
        c = *(_s_pc->next_char); \
        break; \
    } \
} while (CIF_FALSE)

/*
 * Inputs the next available character from the scanner source into the given character without changing the relative
 * token start position or recorded token length.  The absolute token start position may change, however, and column
 * (but not line) accounting is performed. Sets 'ev' to 'CIF_OK' if a valid character is provided, to 'CIF_EOF' if the
 * scanner has already reached the end of the input, or to a CIF error code.
 */
#define NEXT_CHAR(s, c, ev) do { \
    struct scanner_s *_s_nc = (s); \
    PEEK_CHAR(_s_nc, c, ev); \
    if (ev == CIF_OK) { \
        _s_nc->next_char += 1; \
        POSN_INCCOLUMN(_s_nc, 1); \
    } \
} while (CIF_FALSE)

/*
 * Backs up the scanner by one code unit.  Assumes the previous code unit is still available in the buffer.
 * Performs column accounting, but no line-number or token start/length accounting.
 */
#define BACK_UP(s) do { \
    struct scanner_s *_s_bu = (s); \
    assert(_s_bu->next_char > _s_bu->buffer); \
    _s_bu->next_char -= 1; \
    POSN_INCCOLUMN(_s_bu, -1); \
} while (CIF_FALSE)

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
#define TRIM_TOKEN(s, n) do { \
    struct scanner_s *_s_pb = (s); \
    int32_t _n = (n); \
    _s_pb->next_char = _s_pb->text_start + _n; \
    TVALUE_SETLENGTH(_s_pb, _s_pb->next_char - TVALUE_START(_s_pb)); \
} while (CIF_FALSE)

/*
 * Tests whether the specified flag indicates that a lead surrogate was scanned; if so, raises an invalid character
 * error.  If the error is suppressed, then recovery is to replace the character preceding the current scan position
 * with a replacement character appropriate to the version of CIF being scanned
 * s: the scanner to manipulate; must have at least one code unit available in its buffer
 * lead_fl: an lvalue of integral type wherein this macro can store state between runs; should be set to false
 *     before this macro's first run and whenever a character other than a lead surrogate is scanned
 * ev: an lvalue in which an error code can be recorded
 */
#define HANDLE_UNPAIRED_LEAD(s, lead_fl, ev) do { \
    if (lead_fl) { \
        struct scanner_s *_s_hls = (s); \
        /* error: unpaired lead surrogate preceding the current character */ \
        ev = _s_hls->error_callback(CIF_INVALID_CHAR, _s_hls->line, _s_hls->column, _s_hls->next_char - 1, 1, \
                _s_hls->user_data); \
        if (ev != 0) break; \
        /* recover by replacing the surrogate with the replacement character */ \
        *(_s_hls->next_char - 1) = ((_s_hls->cif_version >= 2) ? REPL_CHAR : REPL1_CHAR); \
    } \
} while (CIF_FALSE)

/*
 * A wrapper around HANDLE_UNPAIRED_LEAD() that returns an appropriate error code in the event that an unpaired lead
 * surrogate is detected, and the error is not suppressed.
 */
#define HANDLE_UNPAIRED_LEAD_OR_FAIL(s, lead_fl, ev) do { \
    ev = CIF_OK; HANDLE_UNPAIRED_LEAD(s, lead_fl, ev); if (ev != CIF_OK) return ev; \
} while (CIF_FALSE)

/*
 * Advances the scanner by one code unit, tracking the current column number and watching for input malformations
 * associated with surrogate pairs: unpaired surrogates and pairs encoding Unicode code values not permitted in
 * CIF.  Surrogates have to be handled separately from character buffering in order to correctly handle surrogate
 * pairs that are split across buffer fills.
 * s: the scanner to manipulate; must have at least one code unit available in its buffer
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
                ev = _s->error_callback(CIF_DISALLOWED_CHAR, _s->line, _s->column, _s->next_char - 1, 2, _s->user_data); \
                if (ev != 0) break; \
                /* recover by accepting the character */ \
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
        if ((c < CHAR_TABLE_MAX) ? (scanner->char_class[c] == NO_CLASS) \
                : (((c & 0xFFFEu) == 0xFFFEu) || (c == UCHAR_BOM) || ((c >= 0xFDD0u) && (c <= 0xFDEFu)))) { \
            /* error: disallowed BMP character */ \
            ev = _s->error_callback(CIF_DISALLOWED_CHAR, _s->line, _s->column, _s->next_char, 1, _s->user_data); \
            if (ev != CIF_OK) break; \
            /* recover by accepting the character */ \
        } \
        if ((_s->cif_version < 2) && (c > CIF1_MAX_CHAR)) { \
            /* error: disallowed CIF 1 character */ \
            ev = _s->error_callback(CIF_DISALLOWED_CHAR, _s->line, _s->column, _s->next_char, 1, _s->user_data); \
            if (ev != CIF_OK) break; \
            /* recover by accepting the character */ \
        } \
        HANDLE_UNPAIRED_LEAD(s, lead_fl, ev); \
        lead_fl = (_surrogate_mask == 0xd800); \
    } \
    _s->next_char += 1; \
} while (CIF_FALSE)

/*
 * Handles line number accounting when an EOL character is scanned, incrementing the line number unless the EOL
 * character is a line feed, and it was immediately preceded by a carriage return.  Raises a line length error
 * if the line is overlong, *and returns from the host function if it is not suppressed*.  Arguments are
 *   s: the scanner from which the EOL character was read
 *   ch: the EOL character that was read
 *   state: a modifiable lvalue in which this macro can store state between invocations; should be externally set to
 *     zero when a non-EOL character is scanned
 */
#define HANDLE_EOL(s, ch, state) do { \
    struct scanner_s *_s_eol = (s); \
    UChar _c = (ch); \
    if (POSN_COLUMN(scanner) > CIF_LINE_LENGTH) { \
        /* error: long line */ \
        int _ev = _s_eol->error_callback(CIF_OVERLENGTH_LINE, _s_eol->line, \
                scanner->column, _s_eol->next_char - 1, 0, _s_eol->user_data); \
        if (_ev != CIF_OK) return _ev; \
        /* recover by accepting it as-is */ \
    } \
    /* the next character is at the start of a line */ \
    state = (((state) << 2) + ((_c == UCHAR_NL) ? 1 : ((_c == UCHAR_CR) ? 2 : 3))) & 0xf; \
    /* sol code 0x9 == 1001b indicates that the last two characters were CR and LF.  In that case
     * the line number was incremented for the CR, and should not be incremented again for the LF.
     */ \
    POSN_INCLINE(_s_eol, (((state) == 0x9) ? 0 : 1)); \
} while (CIF_FALSE)

/* function implementations */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * a parser error handler function that ignores all errors
 */
int cif_parse_error_ignore(int code UNUSED, size_t line UNUSED, size_t column UNUSED, const UChar *text UNUSED,
        size_t length UNUSED, void *data UNUSED) {
    return CIF_OK;
}

/*
 * a parser error handler function that rejects all erroneous CIFs.  Always returns @c code.
 */
int cif_parse_error_die(int code, size_t line UNUSED, size_t column UNUSED, const UChar *text UNUSED,
        size_t length UNUSED, void *data UNUSED) {
    return code;
}

int cif_parse_internal(struct scanner_s *scanner, int not_utf8, const char *extra_ws, const char *extra_eol,
        cif_tp *dest) {
    FAILURE_HANDLING;

    scanner->buffer = (UChar *) malloc(BUF_SIZE_INITIAL * sizeof(UChar));
    scanner->buffer_size = BUF_SIZE_INITIAL;
    scanner->buffer_limit = 0;

    if (scanner->buffer == NULL) {
        SET_RESULT(CIF_MEMORY_ERROR);
    } else {
        UChar c;

        INIT_V2_SCANNER(scanner, extra_ws, extra_eol);
        scanner->next_char = scanner->buffer;
        scanner->text_start = scanner->buffer;
        scanner->tvalue_start = scanner->buffer;
        scanner->tvalue_length = 0;

        /*
         * We must avoid get_more_chars() here (and also NEXT_CHAR, which uses it) because we want -- here only -- to
         * accept a byte-order mark.
         */
        FAILURE_VARIABLE = get_first_char(scanner);

        if (FAILURE_VARIABLE == CIF_EOF) {
            /* empty CIF */
            FAILURE_VARIABLE = CIF_OK;
        } else if (FAILURE_VARIABLE == CIF_OK) {
            int scanned_bom;

            c = *(scanner->next_char++);

            /* consume an initial BOM, if present, regardless of the actual source encoding */
            /* NOTE: assumes that the character decoder, if any, does not also consume an initial BOM */
            if ((scanned_bom = (c == UCHAR_BOM))) {
                CONSUME_TOKEN(scanner);
                NEXT_CHAR(scanner, c, FAILURE_VARIABLE);
            }

            if (FAILURE_VARIABLE == CIF_EOF) {
                /* BOM-only CIF; nothing else to do */
                FAILURE_VARIABLE = CIF_OK;
            } else if (FAILURE_VARIABLE == CIF_OK) {
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

#ifdef __cplusplus
}
#endif

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
static int parse_cif(struct scanner_s *scanner, cif_tp *cif) {
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
        /* default: do nothing */
    }
    while (result == CIF_OK) {
        cif_block_tp *block = NULL;
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

static int parse_container(struct scanner_s *scanner, cif_container_tp *container, int is_block) {
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
            /* default: do nothing */
        }
    }

    while (result == CIF_OK) {
        int32_t token_length;
        UChar *token_value;
        UChar *name;
        enum token_type alt_ttype = QVALUE;
        cif_container_tp *frame;

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
                frame = NULL;

                if ((container == NULL) || (scanner->skip_depth > 0)) {
                    result = CIF_OK;
                } else { 
                    UChar saved = *(token_value + token_length);

                    if (scanner->max_frame_depth == 0) {
                        /* save frames are not permitted */
                        result = scanner->error_callback(CIF_FRAME_NOT_ALLOWED, scanner->line,
                             scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                             TVALUE_LENGTH(scanner), scanner->user_data);
                        /* recover, if so directed, by acting as if max_frame_depth were 1 */
                        if (!is_block) {
                            goto container_end;
                        }
                    } else if ((scanner->max_frame_depth == 1) && !is_block) {
                        /* nested save frames are not permitted */
                        result = scanner->error_callback(CIF_NO_FRAME_TERM, scanner->line,
                             scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                             TVALUE_LENGTH(scanner), scanner->user_data);
                        /* recover, if so directed, by assuming the missing terminator */
                        /* do not consume the token */
                        goto container_end;
                    }

                    /* insert a string terminator into the input buffer, after the current token */
                    *(token_value + token_length) = 0;

                    result = cif_container_create_frame(container, token_value, &frame);
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
                            if ((result = (cif_container_create_frame_internal(container, token_value, 1, &frame)))
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
                            result = cif_container_get_frame(container, token_value, &frame);
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
                if (scanner->skip_depth <= 0) {
                    OPTIONAL_VOIDCALL( scanner->keyword_callback, (scanner->line, scanner->column,
                            TVALUE_START(scanner), TVALUE_LENGTH(scanner), scanner->user_data) );
                }
                CONSUME_TOKEN(scanner);
                result = parse_loop(scanner, container);
                break;
            case NAME:
                if (scanner->skip_depth > 0) {
                    CONSUME_TOKEN(scanner);
                    result = parse_item(scanner, container, NULL);
                } else {
                    OPTIONAL_VOIDCALL( scanner->dataname_callback, (scanner->line, scanner->column, token_value,
                            token_length, scanner->user_data) );
                    name = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                    if (name == NULL) {
                        result = CIF_MEMORY_ERROR;
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
                /* error: missing whitespace (between a quoted value and a subsequent colon)*/
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
                        1 + scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
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
            /* default: do nothing */
        }
    }

    return result;
}

static int parse_item(struct scanner_s *scanner, cif_container_tp *container, UChar *name) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_tp *value = NULL;
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
                    /* default: do nothing */
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

static int parse_loop(struct scanner_s *scanner, cif_container_tp *container) {
    string_element_tp *first_name = NULL;          /* the first data name in the header */
    int name_count = 0;
    cif_loop_tp *loop = NULL;
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
                result = CIF_MEMORY_ERROR;
                goto loop_end;
            } else {
                string_element_tp *next_name;
                int name_index = 0;
                int column_count = 0;
               
                /* dummy_loop is a static adapter; of its elements, it owns only 'category' */
                cif_loop_tp dummy_loop = { NULL, -1, NULL, NULL };

                dummy_loop.container = container;
                dummy_loop.names = names; 

                /* prepare for the loop body */

                name_index = 0;
                for (next_name = first_name; next_name != NULL; next_name = next_name->next) {
                    if (next_name->string != NULL) {
                        names[name_index++] = next_name->string;
                    } /* else a previously-detected duplicate name */
                    column_count += 1;
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
                        /* default: do nothing */
                    }
                }  /* else loop == NULL from its initialization */

                /* read packets */
                result = parse_loop_packets(scanner, loop, first_name, names, column_count);

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
        string_element_tp *next = first_name->next;

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
            /* default: do nothing */
        }
    }

    if (loop != NULL) {
        cif_loop_free(loop);
    }

    return result;
}

static int parse_loop_header(struct scanner_s *scanner, cif_container_tp *container, string_element_tp **name_list_head,
        int *name_countp) {
    string_element_tp **next_namep = name_list_head;  /* a pointer to the pointer to the next data name in the header */
    int result;

    /* parse the header */
    while (((result = next_token(scanner)) == CIF_OK) && (scanner->ttype == NAME)) {
        int32_t token_length = TVALUE_LENGTH(scanner);
        UChar *token_value = TVALUE_START(scanner);

        if (scanner->skip_depth <= 0) {
            OPTIONAL_VOIDCALL( scanner->dataname_callback, (scanner->line, scanner->column,
                    token_value, token_length, scanner->user_data) );
        }

        *next_namep = (string_element_tp *) malloc(sizeof(string_element_tp));
        if (*next_namep == NULL) {
            return CIF_MEMORY_ERROR;
        } else {
            (*next_namep)->next = NULL;
            (*next_namep)->string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
            if ((*next_namep)->string == NULL) {
                return CIF_MEMORY_ERROR;
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

static int parse_loop_packets(struct scanner_s *scanner, cif_loop_tp *loop, string_element_tp *first_name,
        UChar *names[], int column_count) {
    cif_packet_tp *packet;
    int result = cif_packet_create(&packet, names);

    /* read packets */
    if (result == CIF_OK) {
        cif_value_tp **packet_values = (cif_value_tp **) malloc(column_count * sizeof(cif_value_tp *));

        if (packet_values == NULL) {
            result = CIF_MEMORY_ERROR;
        } else {
            cif_value_tp *dummy_value = NULL;

            result = cif_value_create(CIF_UNK_KIND, &dummy_value);
            if (result == CIF_OK) {
                int have_packets = CIF_FALSE;
                int column_index = 0;
                string_element_tp *next_name;

                /*
                 * Extract pointers to the packet's values once, for access by index, to avoid a normalizing and hashing
                 * many times.
                 */
                for (next_name = first_name; next_name; next_name = next_name->next) {
                    assert(column_index < column_count);
                    if (next_name->string == NULL) {
                        packet_values[column_index] = dummy_value;
                    } else {
                        result = cif_packet_get_item(packet, next_name->string, packet_values + column_index);
                        if (result == CIF_NOSUCH_ITEM) {
                            result = CIF_INTERNAL_ERROR;
                        }
                        if (result != CIF_OK) {
                            goto packets_end;
                        }
                    }
                    column_index += 1;
                }

                column_index = 0;
                next_name = first_name;
                while ((result = next_token(scanner)) == CIF_OK) {
                    UChar *name;
                    cif_value_tp *value = NULL;
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
                            if (column_index == 0) {
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
                                        /* default: do nothing */
                                    }
                                }
                            }

                            name = next_name->string;
                            value = packet_values[column_index];  /* it is safe to re-use the existing value object */

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
                                    /* default: do nothing */
                                }
                            }

                            column_index = (column_index + 1) % column_count;
                            if (result != CIF_OK) { /* this is the parse_value() or item handler result code */
                                goto packets_end;
                            } else if (column_index == 0) {  /* that was the last value in the packet */
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
                            }
                            next_name = next_name->next ? next_name->next : first_name;

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
                            if (column_index != 0) {
                                /* error: partial packet */
                                result = scanner->error_callback(CIF_PARTIAL_PACKET, scanner->line,
                                        scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner), 0,
                                        scanner->user_data);
                                if (result != CIF_OK) {
                                    goto packets_end;
                                }
                                /* recover by synthesizing unknown values to fill the packet, and saving it */
                                for (; column_index < column_count; column_index += 1) {
                                    if ((names[column_index] != NULL)
                                            && (result = cif_value_init(packet_values[column_index], CIF_UNK_KIND))
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
                cif_value_free(dummy_value);
            } /* end if (result == CIF_OK) [of cif_value_create()] */
            free(packet_values);  /* free only the array; its elements belong to the packet */
        } /* end if (packet_values != NULL) */
        cif_packet_free(packet);
    } /* end if (cif_packet_create()) */

    return result;
}

static int parse_list(struct scanner_s *scanner, cif_value_tp **listp) {
    cif_value_tp *list = NULL;
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
        cif_value_tp *element = NULL;
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
                    /* default: do nothing */
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

static int parse_table(struct scanner_s *scanner, cif_value_tp **tablep) {
    /*
     * Implementation note: this function is designed to minimize the damage caused by input malformation.  If a
     * key or value is dropped, or if a key is turned into a value by removing its colon or inserting whitespace
     * before it, then only one table entry is lost.  Furthermore, this function recognizes and handles unquoted
     * keys, provided that the configured error callback does not abort parsing when notified of the problem.
     */
    cif_value_tp *table;
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
        cif_value_tp *value = NULL;

        /* scan the next key, if any */

        if ((result = next_token(scanner)) != CIF_OK) {
            break;
        }
        switch (scanner->ttype) {
            case VALUE:
                if (*TVALUE_START(scanner) == UCHAR_COLON) {
                    /* error: null key */
                    result = scanner->error_callback(CIF_NULL_KEY, scanner->line,
                            scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                            TVALUE_LENGTH(scanner), scanner->user_data);
                    if (result != CIF_OK) {
                        goto table_end;
                    }
                    /* recover by handling it as a NULL (not empty) key */
                    if (TVALUE_LENGTH(scanner) > 1) {
                        TRIM_TOKEN(scanner, 1);
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
                        if (TVALUE_START(scanner)[index] == UCHAR_COLON) {
                            /* error: unquoted key */
                            result = scanner->error_callback(CIF_UNQUOTED_KEY, scanner->line,
                                    scanner->column - TVALUE_LENGTH(scanner), TVALUE_START(scanner),
                                    TVALUE_LENGTH(scanner), scanner->user_data);
                            if (result != CIF_OK) {
                                goto table_end;
                            }
                            /* recover by accepting everything before the first colon as a key */
                            /* push back all characters following the first colon */
                            TRIM_TOKEN(scanner, index + 1);
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
                result = CIF_MEMORY_ERROR;
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
static int parse_value(struct scanner_s *scanner, cif_value_tp **valuep) {
    int result = next_token(scanner);

    if (result == CIF_OK) {
        cif_value_tp *value = *valuep;

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
                        result = CIF_MEMORY_ERROR;
                    }
                    break;
                case VALUE:
                    /* special cases for unquoted question mark (?) and period (.) */
                    if (token_length == 1) {
                        if (*token_value == UCHAR_QUERY) {
                            result = cif_value_init(value, CIF_UNK_KIND);
                            goto value_end;
                        }  else if (*token_value == UCHAR_DECIMAL) {
                            result = cif_value_init(value, CIF_NA_KIND);
                            goto value_end;
                        } /* else an ordinary value */
                    }
        
                    string = (UChar *) malloc((token_length + 1) * sizeof(UChar));
                    if (string == NULL) {
                        result = CIF_MEMORY_ERROR;
                    } else {
                        /* copy the token value into a separate buffer, with terminator */
                        *string = 0;
                        u_strncat(string, token_value, token_length);  /* ensures NUL termination */

                        /*
                         * Record the value as a string; it will later be coerced to a number automatically
                         * if requested as a number and if possible.  The value object takes ownership of the string.
                         */
                        if ((result = cif_value_init_char(value, string)) != CIF_OK) {
                            /* initialization failed; free the string */
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
static int decode_text(struct scanner_s *scanner, UChar *text, int32_t text_length, cif_value_tp **dest) {
    FAILURE_HANDLING;
    cif_value_tp *value = *dest;
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

            if (buffer == NULL) {
                SET_RESULT(CIF_MEMORY_ERROR);
            } else {
                /* analyze the text and copy its logical contents into the buffer */
                UChar *buf_pos = buffer;
                UChar *in_pos = text;
                UChar *in_limit = text + text_length; /* input boundary pointer */
                int32_t prefix_length;
                int folded;

                if ((*text == UCHAR_SEMI) || ((scanner->line_unfolding <= 0) && (scanner->prefix_removing <= 0))) {
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
                            case UCHAR_BSL:
                                prefix_length = (in_pos - text) - 1; /* the number of characters preceding this one */
                                backslash_count += 1;                /* the total number of backslashes on this line */
                                nonws = CIF_FALSE;                   /* reset the nonws flag */
                                break;
                            case UCHAR_CR:
                                /* convert CR and CR LF to LF */
                                *(buf_pos - 1) = UCHAR_NL;
                                if ((in_pos < in_limit) && (*in_pos == UCHAR_NL)) {
                                    in_pos += 1;
                                }
                                /* fall through */
                            case UCHAR_NL:
                                goto end_of_line;
                            default:
                                switch(CLASS_OF(c, scanner)) {
                                    case EOL_CLASS: /* a non-standard EOL character */
                                        goto end_of_line;
                                    case WS_CLASS:
                                        break;
                                    default:
                                        nonws = CIF_TRUE;
                                        break;
                                }
                                break;
                        }
                    }

                    end_of_line:
                    /* analyze the results of the scan of the first line */
                    prefix_length = ((scanner->prefix_removing > 0) ? (prefix_length + 1 - backslash_count) : 0);
                    if (!nonws && ((backslash_count == 1)
                            || ((backslash_count == 2) && (prefix_length > 0) && (text[prefix_length] == UCHAR_BSL)))) {
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

                        if ((CLASS_OF(c, scanner) == EOL_CLASS) && (c != UCHAR_NL)) {
                            length = in_pos++ - in_start;
                            u_strncpy(buf_pos, in_start, length);
                            buf_pos += length;
                            *(buf_pos++) = UCHAR_NL;
                            if ((in_pos < in_limit) && (c == UCHAR_CR) && (*in_pos == UCHAR_NL)) {
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
                                /* FIXME: this is not an error! */
                                /*
                                result = scanner->error_callback(CIF_MISSING_PREFIX, scanner->line,
                                        1, TVALUE_START(scanner), 0, scanner->user_data);
                                if (result != CIF_OK) {
                                    FAIL(late, result);
                                }
                                */
                                /* recover by copying the un-prefixed text (no special action required for that) */
                            }
                        }

                        /* copy from input to buffer, up to and including EOL, accounting for folding if applicable */
                        while (in_pos < in_limit) { /* each UChar unit */
                            UChar c = *(in_pos++);

                            /* copy the character unconditionally */
                            *(buf_pos++) = c;

                            /* handle line folding where appropriate */
                            if ((c == UCHAR_BSL) && folded) {
                                /* buf_temp tracks the position of the backslash in the output buffer */
                                buf_temp = buf_pos - 1;
                            } else {
                                int klass = CLASS_OF(c, scanner);

                                if (klass == EOL_CLASS) {
                                    /* convert line terminators other than LF to LF */
                                    if (c != UCHAR_NL) {
                                        *(buf_pos - 1) = UCHAR_NL;
                                        if ((c == UCHAR_CR) && (in_pos < in_limit) && (*in_pos == UCHAR_NL)) {
                                            in_pos += 1;
                                        }
                                    }

                                    /* if appropriate, rewind the output buffer to achieve line folding */
                                    if (buf_temp != NULL) {
                                        buf_pos = buf_temp;
                                        buf_temp = NULL;
                                    }

                                    break;
                                } else if (klass != WS_CLASS) {
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
            if (result == CIF_EOF) {
                ttype = END;
                result = CIF_OK;
            }
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
                    if (scanner->skip_depth <= 0) {
                        /* notify the configured whitespace callback, if any */
                        OPTIONAL_VOIDCALL(scanner->whitespace_callback, (scanner->line, scanner->column,
                                TVALUE_START(scanner), TVALUE_LENGTH(scanner), scanner->user_data));
                    }

                    /* move on */
                    CONSUME_TOKEN(scanner);
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
                    CONSUME_TOKEN(scanner);
                    continue;  /* cycle the LOOP */
                } else {
                    break;     /* break from the SWITCH */
                }
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

                    switch (result) {
                        case CIF_OK:
                            if (c == UCHAR_COLON) {
                                /*
                                 * can only happen in CIF 2 mode, for in CIF 1 mode the character after a closing quote is
                                 * always whitespace
                                 */
                                scanner->next_char += 1;
                                ttype = KEY;
                                break;
                            }
                            /* fall through */
                        case CIF_EOF:
                            result = CIF_OK;
                            ttype = QVALUE;
                            break;
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
                                if (c == UCHAR_COLON) {
                                    /* Not diagnosed as an error _here_ */
                                    scanner->next_char += 1;
                                    ttype = TKEY;
                                }
                            } else if (result == CIF_EOF) {
                                result = CIF_OK;
                            }
                        }
                    }
                } else {
                    /* as the default case, but we can't fall through */
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
                /* back up to ensure proper character validation in scan_unquoted() */
                BACK_UP(scanner);
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
                    HANDLE_EOL(scanner, c, sol);
                    break;
                default:
                    goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            goto end_of_token;
        } else if (result != CIF_OK) {
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
                /* We've scanned one code unit too far; back off */
                BACK_UP(scanner);
                goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            HANDLE_UNPAIRED_LEAD_OR_FAIL(scanner, lead_surrogate, result);
            goto end_of_token;
        } if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
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
            if (CLASS_OF(c, scanner) == EOL_CLASS) {
                /* We've scanned one code unit too far; back off */
                BACK_UP(scanner);
                goto end_of_token;
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            HANDLE_UNPAIRED_LEAD_OR_FAIL(scanner, lead_surrogate, result);
            goto end_of_token;
        } else if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
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
                    /* this doesn't occur in CIF 1 mode because no characters are mapped to OPEN_META there */
                    result = scanner->error_callback(CIF_MISSING_SPACE, scanner->line,
                            scanner->column, scanner->next_char - 1, 0, scanner->user_data);
                    if (result != CIF_OK) {
                        return result;
                    }
                    /* fall through */
                case CLOSE_META:
                case WS_META:
                    if (c != EOF_CHAR) {
                        /* We've scanned one code unit too far; back off */
                        BACK_UP(scanner);
                    }
                    goto end_of_token;
                /* default: do nothing */
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            HANDLE_UNPAIRED_LEAD_OR_FAIL(scanner, lead_surrogate, result);
            goto end_of_token;
        } else if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)));
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
    int result;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }

            if (c == delim) {
                PEEK_CHAR(scanner, c, result);

                if (result != CIF_EOF) {
                    if (result != CIF_OK) {
                        return result;
                    } else if (scanner->cif_version < 2) {
                        /* look ahead for whitespace */
                        if (METACLASS_OF(c, scanner) != WS_META) {
                            /* the current character is part of the value */
                            continue;
                        }
                    } else if (scanner->next_char - scanner->text_start == 2) {
                        /* test for a third delimiter character */
                        if (c == delim) {
                            scanner->next_char += 1;
                            return scan_triple_delim_string(scanner);
                        }
                    }
                }
                delim_size = 1;
                goto end_of_token;
            } else if (CLASS_OF(c, scanner) == EOL_CLASS) {
                /* back up the scanner by one position in preparation for recovery */
                BACK_UP(scanner);
                goto end_of_line;
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            HANDLE_UNPAIRED_LEAD_OR_FAIL(scanner, lead_surrogate, result);
            goto end_of_line;
        } else if (result != CIF_OK) {
            return result;
        }
    }

    end_of_line:
    /* error: unterminated quoted string */
    result = scanner->error_callback(CIF_MISSING_ENDQUOTE, scanner->line,
            scanner->column, scanner->next_char, 0, scanner->user_data);
    if (result != CIF_OK) {
        return result;
    }
    delim_size = 0;

    end_of_token:
    /* The token value starts after the opening delimiter */
    TVALUE_INCSTART(scanner, 1);
    /* The token length excludes the closing delimiter (if any) */
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)) - delim_size);

    return CIF_OK;
}

/*
 * Scans a string delimited by a tripled delimiter character.  The scanned string may contain
 * any character or character sequence, including whitespace, other than the closing
 * triple-delimiter.
 *
 * On entry, the text start position is assumed to be at the first character of the opening
 * delimiter, and the scan position is assumed to be immediately after the last character of
 * the opening delimiter.
 */
static int scan_triple_delim_string(struct scanner_s *scanner) {
    UChar delim = *(scanner->text_start);
    int delim_count = 0;
    int lead_surrogate = CIF_FALSE;
    int delim_size;
    int result;
    int sol = 0;

    for (;;) {
        UChar *top = scanner->buffer + scanner->buffer_limit;

        while (scanner->next_char < top) {
            UChar c;

            /* Scan and validate the next code unit, incrementing the column number as appropriate */
            SCAN_UCHAR(scanner, c, lead_surrogate, result);
            if (result != CIF_OK) {
                return result;
            }

            if (c == delim) {
                if (++delim_count >= 3) {
                    delim_size = 3;
                    goto end_of_token;
                }
            } else {
                delim_count = 0;
                if (CLASS_OF(c, scanner) == EOL_CLASS) {
                    HANDLE_EOL(scanner, c, sol);
                } else {
                    sol = 0;
                }
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            HANDLE_UNPAIRED_LEAD_OR_FAIL(scanner, lead_surrogate, result);
            goto end_of_file;
        } else if (result != CIF_OK) {
            return result;
        }
    }

    end_of_file:
    /* error: unterminated quoted string */
    result = scanner->error_callback(CIF_UNCLOSED_TEXT, scanner->line,
            scanner->column, scanner->next_char - 1, 0, scanner->user_data);
    if (result != CIF_OK) {
        return result;
    }
    /* recover by assuming a trailing close-quote */
    delim_size = 0;

    end_of_token:
    /* The token value starts after the opening delimiter */
    TVALUE_INCSTART(scanner, 3);
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
                        delim_size = ((*(scanner->next_char - 2) == UCHAR_NL)
                                && (*(scanner->next_char - 3) == UCHAR_CR)) ? 3 : 2;
                        goto end_of_token;
                    }
                    break;
                case EOL_CLASS:
                    /* HANDLE_EOL(scanner, c, sol); */
do {
    struct scanner_s *_s_eol = (scanner);
    UChar _c = (c);

    if (POSN_COLUMN(scanner) > CIF_LINE_LENGTH) {
        int _ev = _s_eol->error_callback(CIF_OVERLENGTH_LINE, _s_eol->line,
                scanner->column, _s_eol->next_char - 1, 0, _s_eol->user_data);
        if (_ev != CIF_OK) return _ev;
    }
    sol = (((sol) << 2) + ((_c == UCHAR_NL) ? 1 : ((_c == UCHAR_CR) ? 2 : 3))) & 0xf;
    POSN_INCLINE(_s_eol, (((sol) == 0x9) ? 0 : 1));
} while (CIF_FALSE);
                    break;
                default:
                    sol = 0;
                    break;
            }
        }

        result = get_more_chars(scanner);
        if (result == CIF_EOF) {
            HANDLE_UNPAIRED_LEAD_OR_FAIL(scanner, lead_surrogate, result);
            /* error: unterminated text block */
            result = scanner->error_callback(CIF_UNCLOSED_TEXT, scanner->line,
                    scanner->column, scanner->next_char - 1, 0, scanner->user_data);
            if (result != CIF_OK) {
                return result;
            }
            /* recover by reporting out the whole tail as the token */
            goto end_of_token;
        } else if (result != CIF_OK) {
            return result;
        }
    }

    end_of_token:
    TVALUE_INCSTART(scanner, 1);
    TVALUE_SETLENGTH(scanner, (scanner->next_char - TVALUE_START(scanner)) - delim_size);
    return CIF_OK;
}

/*
 * Transfers one or possibly two character from the provided scanner's character source into its working character
 * buffer, provided that any are available.  Assumes that no characters have yet been transferred, and that the CIF
 * version being parsed may not yet be known.  Will raise the end-of-file flag if called when there are no characters
 * are available.  Returns CIF_OK if any characters are transferred, CIF_EOF if the EOF flag is raised without
 * transferring any characters, or CIF_ERROR otherwise.
 *
 * Two characters are transferred
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
        scanner->at_eof = CIF_TRUE;
        return CIF_EOF;
    } else {
        UChar ch = *scanner->buffer;
        int result;

        if ((ch > CIF1_MAX_CHAR) ? (ch != UCHAR_BOM) : (scanner->char_class[ch] == NO_CLASS)) {
            /* error: disallowed character */
            result = scanner->error_callback(CIF_DISALLOWED_INITIAL_CHAR, 1, 0,
                    scanner->buffer, 1, scanner->user_data);
            if (result != CIF_OK) {
                return result;
            }
            /* recover by accepting the character (for the moment) */
        } else if (ch == UCHAR_CR) { /* convert CR and CRLF to LF */
            *scanner->buffer = UCHAR_NL;

            /* try to convert one more character, to check for CRLF */
            nread = scanner->read_func(scanner->char_source, scanner->buffer + 1, scanner->buffer_size - 1,
                    &read_error);
            if (nread < 0) {
                return read_error;
            } else if (nread == 0) {
                scanner->at_eof = CIF_TRUE;  /* but don't return CIF_EOF, because we do provide one character */
            } else if (*(scanner->buffer + 1) != UCHAR_NL) {
                scanner->buffer_limit += 1;
            } /* else the buffer limit will overall be increased by 1 only, effectively consuming the NL */

        }

        scanner->buffer_limit += 1;
        return CIF_OK;
    }
}

/*
 * Transfers characters from the provided scanner's character source into its working character buffer, provided that
 * any are available.  May move unconsumed data within the buffer (adjusting the rest of the scanner's internal state
 * appropriately) and/or may increase the size of the buffer.  Will raise the end-of-file flag if called when there are
 * no characters are available.
 *
 * Returns CIF_OK if any characters are transferred, CIF_EOF if the EOF flag is raised, or CIF_ERROR otherwise.
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
                return CIF_MEMORY_ERROR;
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
    } /* else just append to the currently buffered data */

    /* once EOF has been detected, don't attempt to read from the character source any more */
    nread = scanner->at_eof ? 0 : scanner->read_func(scanner->char_source, scanner->buffer + scanner->buffer_limit,
                scanner->buffer_size - scanner->buffer_limit, &read_error);

    if (nread < 0) {
        return read_error;
    } else if (nread == 0) {
        scanner->at_eof = CIF_TRUE;
        return CIF_EOF;
    } else {
        /* convert line terminators */
        UChar *lead = scanner->buffer + scanner->buffer_limit; /* a pointer to the character being probed */
        UChar *bound = lead + nread;
        UChar *trail;
        UChar *dest;

        do {
            lead = u_memchr(lead, UCHAR_CR, bound - lead);
            if ((!lead) || ((lead + 1 < bound) && (*(lead + 1) == UCHAR_NL))) {
                break;
            } else {
                *lead = UCHAR_NL;
            }
        } while (CIF_TRUE);

        dest = lead;
        while (lead) {
            ptrdiff_t length;

            trail = ++lead;  /* trail points to the LF of the latest-read CRLF terminator */
            do {
                assert(lead <= bound);
                lead = u_memchr(lead, UCHAR_CR, bound - lead);  /* look for the next CR */
                if (!lead) {
                    /* end of input */
                    length = bound - trail;
                    break;
                } else if ((lead + 1 < bound) && (*(lead + 1) == UCHAR_NL)) {
                    /* end of CRLF-terminated line */
                    nread -= 1; /* CRLF will be converted to just LF */
                    length = lead - trail;
                    break;
                } else {
                    /* bare CR is translated to LF without any need to move other data */
                    *lead = UCHAR_NL;
                }
            } while (CIF_TRUE);

            /* convert one line terminator */
            u_memmove(dest, trail, length);
            dest += length;
        }

        /* bookkeeping */
        scanner->buffer_limit += nread;

        return CIF_OK;
    }
}
