/*
 * cif.h
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
 * @file  cif.h
 * @brief The primary CIF API public header file, declaring most data structures, functions, and macros intended for
 *        library users to manipulate.
 *
 * @author John C. Bollinger
 * @date   2014, 2015
 */

#ifndef CIF_H
#define CIF_H

#include <stdio.h>
#include <unicode/ustring.h>

#ifdef __GNUC__
#define WARN_UNUSED __attribute__ ((__warn_unused_result__))
#else
#define WARN_UNUSED
#endif

#ifdef _WIN32
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#ifndef LIBCIF_STATIC
#define DECLSPEC __declspec(dllimport)
#endif
#endif
#endif

#ifndef DECLSPEC
#define DECLSPEC
#endif

#define CIF_FUNC_DECL(type, name, params) DECLSPEC type name params WARN_UNUSED
#define CIF_INTFUNC_DECL(name, params) CIF_FUNC_DECL(int, name, params)
#define CIF_VOIDFUNC_DECL(name, params) DECLSPEC void name params

/**
 * @page resource_mgmt Resource management with CIF API
 *
 * The core CIF API, being written in C, leaves basic responsibility for resource management in the hands of the
 * application programmer.  Nevertheless, the API is designed to make the resource
 * management burden as light as reasonably possible.  What follows is a discussion of many of the considerations
 * and specific API features that relate to resource management.
 *
 * @section basics Resource management basics
 * For the most part, CIF API objects are created dynamically via functions having @c _create in their names
 * (@c cif_create_block(), for example).  Each object type for which there is a creation function has one or more
 * functions for releasing that object and all its internal state:
 * @li Most types have a @c cif_*_free() function that cleans up objects of that type.  These are the
 *         workhorse resource releasing functions.  For objects that are handles on parts of a managed CIF, these
 *         functions release the handle without modifying the underlying managed CIF.
 * @li Most handle objects have a @c cif_*_destroy() function that removes the portion of the managed CIF represented
 *         by the handle.  Inasmuch as the handle would be useless after such an operation is performed, these
 *         functions also clean up the handle object as if by calling the appropriate free function.
 *
 * Use of the type-specific resource release functions ensures that all internal state is correctly released.
 * Application code must not pass CIF API objects to the C library's @c free() function, except as may be explicitly
 * described elsewhere in the API documentation.
 * 
 * Except where otherwise specified, CIF API functions that return a pointer to the caller via a pointer pointer
 * (as object creation functions in particular do) thereby assign responsibility for the returned pointer to the
 * calling function.  More generally, any function that allocates an object directly or indirectly bears initial
 * responsibility for releasing that object.  It must ensure the object is released before it returns, or
 * else it must pass responsibility for doing so to another context (typically the caller's).  Note, too, that API
 * functions that accept a pointer pointer argument vary with respect to what they do with the value, if any, that
 * the argument initially points to.  Many of them simply overwrite it, which will result in a memory leak if 
 * the caller passes a pointer to the only pointer to a live object.
 *
 * Except where otherwise specified (such as for the @c cif_*_free() and @c cif_*_destroy() functions), passing a
 * pointer as a CIF API function argument does not transfer responsibility for the pointed-to object to the called
 * function.
 * Furthermore, API functions do not create aliases to their pointer arguments that would make it unsafe for the
 * responsible function to clean them up at its sole discretion.  Where necessary, API functions copy objects in
 * order to avoid creating aliases to them.  These characteristics are intended to minimize confusion about whether
 * application code can or should release any particular CIF API object.
 *
 * Some API functions accept or return general objects such as arrays or Unicode strings.  For the most part, the same
 * rules apply to such objects as to CIF API objects, but the API does not provide resource management functions for
 * such objects.  See specific function documentation for details.  With respect to arrays in particular, consult
 * function docs for information about responsibility for array elements, as distinguished from responsibility for the
 * array itself.
 *
 * It should be borne in mind that CIF API handle objects, such as those of types @c cif_container_tp and
 * @c cif_loop_tp , are separate resources from the corresponding parts of managed CIFs.  At any given time, for any
 * given
 * part of a managed CIF, there may be any number -- including zero -- of extant, valid handles.  New handles can be
 * obtained for existing managed CIF pieces, and handles can be released without affecting the underlying managed CIF.
 * Destroying the underlying managed piece invalidates all outstanding handles, but releases only the handle, if any,
 * by which the destruction was performed.
 *
 * @section special Exceptions and special cases
 * The CIF API provides a handful of functions having resource management relevance that do not fit the general pattern:
 * @li Though it does not have "create" in its name, the cif_parse() function operates as an object creation function
 *         when its last argument is a pointer to NULL.
 * @li The creation function for loop packet iterators is cif_loop_get_packets() .  Its cleanup functions are
 *         cif_pktitr_close() and cif_pktitr_abort() .
 * @li The function cif_value_clean() is unusual in that it releases internal resources held by its argument without
 *         releasing the argument itself (converting it to a kind of value that does not require dynamically-allocated
 *         internal resources).
 * @li There is only one handle object for each overall managed CIF (of type @c cif_tp ).  There is therefore no
 *         function @c cif_free() ; use @c cif_destroy() instead.
 * @li There is no creation function for transparent data type @c cif_handler_tp .
 */

/**
 * @defgroup return_codes Function return codes
 * @{
 */

/** @brief A result code indicating successful completion of the requested operation */
#define CIF_OK                  0

/**
 * @brief A result code indicating that the requested operation completed successfully, but subsequent repetitions of
 *        the same operation can be expected to fail
 *
 * This code is used mainly by packet iterators to signal the user when the last available packet is returned.
 */
#define CIF_FINISHED            1

/**
 * @brief A result code indicating that the requested operation failed because an error occurred in one of the
 *        underlying libraries
 *
 * This is the library's general-purpose error code for faults that it cannot more specifically diagnose.  Pretty much
 * any API function can return this code under some set of circumstances.  Failed memory allocations or unexpected
 * errors emitted by the storage back end are typical triggers for this code.  In the event that this code is returned,
 * the C library's global @c errno variable @em may indicate the nature of the error more specifically.
 */
#define CIF_ERROR               2

/**
 * @brief A result code indicating that the requested operation could not be performed because of a dynamic
 *        memory allocation failure
 *
 * Few library clients will care about the distinction between this code and @c CIF_ERROR, but some future wrappers
 * may do.  For instance, a Python extension would be expected to distinguish memory allocation failures from other
 * errors so as to signal them to the host Python environment.
 */
#define CIF_MEMORY_ERROR        3

/**
 * @brief A result code returned on a best-effort basis to indicate that a user-provided object handle is invalid.
 *
 * The library does not guarantee to be able to recognize invalid handles.  If an invalid handle is provided then
 * a @c CIF_ERROR code may be returned instead, or really anything might happen -- generally speaking, library
 * behavior is undefined in these cases.
 *
 * Where it does detect invalidity, that result may be context dependent.  That is, the handle may be invalid for
 * the use for which it presented, but valid for different uses.  In particular, this may be the case if a save frame
 * handle is presented to one of the functions that requires specifically a data block handle.
 */
#define CIF_INVALID_HANDLE      4

/**
 * @brief A result code indicating that an internal error or inconsistency was encountered
 *
 * If this code is emitted then it means a bug in the library has been triggered (if for no other reason than that
 * this is the wrong code to return if an internal bug has @em not been triggered).
 */
#define CIF_INTERNAL_ERROR      5

/**
 * @brief A result code indicating generally that one or more arguments to the function do not satisfy the function's
 *        requirements.
 *
 * This is a fallback code -- a more specific code will be emitted when one is available.
 */
#define CIF_ARGUMENT_ERROR      6

/**
 * @brief A result code indicating that although the function was called with with substantially valid arguments, the
 *        context or conditions do not allow the call.
 *
 * This code is returned, for example, if @c cif_pktitr_remove_packet() is passed a packet iterator that has not yet
 * provided a packet that could be removed.
 */
#define CIF_MISUSE              7

/**
 * @brief A result code indicating that an optional feature was invoked and the library implementation in use does not
 *        support it.
 *
 * There are very few optional features defined at this point, so this code should rarely, if ever, be seen.
 */
#define CIF_NOT_SUPPORTED       8

/**
 * @brief A result code indicating that the operating environment is missing data or features required to complete the
 *        operation.
 *
 * For example, the @c cif_create() function of an SQLite-based implementation that depends on foreign keys might
 * emit this code if it determines that the external sqlite library against which it is running omits foreign key
 * support.
 */
#define CIF_ENVIRONMENT_ERROR   9

/**
 * @brief A result code indicating a synthetic error injected by client code via a callback function
 */
#define CIF_CLIENT_ERROR       10

/**
 * @brief A result code signaling an attempt to cause a CIF to contain blocks with duplicate (by CIF's criteria) block
 *        codes
 */
#define CIF_DUP_BLOCKCODE      11

/**
 * @brief A result code signaling an attempt to cause a CIF to contain a block with an invalid block code
 */
#define CIF_INVALID_BLOCKCODE  12

/**
 * @brief A result code signaling an attempt to retrieve a data block from a CIF (by reference to its block code)
 *        when that CIF does not contain a block bearing that code
 */
#define CIF_NOSUCH_BLOCK       13

/**
 * @brief A result code signaling an attempt to cause a data block to contain save frames with duplicate (by CIF's
 *        criteria) frame codes
 */
#define CIF_DUP_FRAMECODE      21

/**
 * @brief A result code signaling an attempt to cause a data block to contain a save frame with an invalid frame code
 */
#define CIF_INVALID_FRAMECODE  22

/**
 * @brief A result code signaling an attempt to retrieve a save frame from a data block (by reference to its frame code)
 *        when that block does not contain a frame bearing that code
 */
#define CIF_NOSUCH_FRAME       23

/**
 * @brief A result code signaling an attempt to retrieve a loop from a save frame or data block by category, when
 *        there is more than one loop tagged with the specified category
 */
#define CIF_CAT_NOT_UNIQUE     31

/**
 * @brief A result code signaling an attempt to retrieve a loop from a save frame or data block by category, when
 *        the requested category is invalid.
 *
 * The main invalid category is NULL (as opposed to the empty category name), which is invalid only for retrieval, not
 * loop creation.
 */
#define CIF_INVALID_CATEGORY   32

/**
 * @brief A result code signaling an attempt to retrieve a loop from a save frame or data block by category, when
 *        the container does not contain any loop tagged with the specified category.
 *
 * This code is related to loop references by @em category.  Contrast with @c CIF_NOSUCH_ITEM .
 */
#define CIF_NOSUCH_LOOP        33

/**
 * @brief A result code signaling an attempt to manipulate a loop having special significance to the library, in a
 *        manner that is not allowed
 *
 * The only reserved loops in this version of the library are those that hold items that do not / should not appear
 * in explicit CIF @c loop_ constructs.  There is at most one of these per data block or save frame, and they are
 * identified by empty (not NULL) category tags. (C code can use the @c CIF_SCALARS macro for this category).
 */
#define CIF_RESERVED_LOOP      34

/**
 * @brief A result code indicating that an attempt was made to add an item value to a different loop than the one
 *        containing the item.
 */
#define CIF_WRONG_LOOP         35

/**
 * @brief A result code indicating that a packet iterator was requested for a loop that contains no packets, or that
 *         a packet-less loop was encountered during parsing.
 */
#define CIF_EMPTY_LOOP         36

/**
 * @brief A result code indicating that an attempt was made to create a loop devoid of any data names
 */
#define CIF_NULL_LOOP          37

/**
 * @brief A result code indicating that an attempt was made to add an item to a data block or save frame that already
 *        contains an item of the same data name.
 *
 * "Same" in this sense is judged with the use of Unicode normalization and case folding.
 */
#define CIF_DUP_ITEMNAME       41

/**
 * @brief A result code indicating that an attempt was made to add an item with an invalid data name to a CIF.
 *
 * Note that attempts to @em retrieve items by invalid name, on the other hand, simply return @c CIF_NOSUCH_ITEM
 */
#define CIF_INVALID_ITEMNAME   42

/**
 * @brief A result code indicating that an attempt to retrieve an item by name failed as a result of no item bearing
 *        that data name being present in the target container
 */
#define CIF_NOSUCH_ITEM        43

/**
 * @brief A result code indicating that an attempt to retrieve a presumed scalar has instead returned one of multiple
 *        values found.
 */
#define CIF_AMBIGUOUS_ITEM     44

/**
 * @brief A result code indicating that the requested operation could not be performed because a packet object provided
 *        by the user was invalid.
 *
 * For example, the packet might have been empty in a context where that is not allowed.
 */
#define CIF_INVALID_PACKET     52

/**
 * @brief A result code indicating that during parsing, the last packet in a loop construct contained fewer values than
 *         the associated loop header had data names
 */
#define CIF_PARTIAL_PACKET     53

/**
 * @brief A result code indicating that an attempt was made to parse or write a value in a context that allows
 *        only values of kinds different from the given value's
 *
 * The only context in CIF 2.0 that allows values of some kinds but not others is table indices.  For this purpose,
 * "allowed" is defined in terms of CIF syntax, ignoring any other considerations such as validity with respect to
 * a data dictionary.
 */
#define CIF_DISALLOWED_VALUE   62

/**
 * @brief A result code indicating that a string provided by the user could not be parsed as a number.
 *
 * "Number" here should be interpreted in the CIF sense: an optionally-signed integer or floating-point number in
 * decimal format, with optional signed exponent and optional standard uncertainty.
 */
#define CIF_INVALID_NUMBER     72

/**
 * @brief A result code indicating that a (Unicode) string provided by the user as a table index is not valid for that
 *         use.
 */
#define CIF_INVALID_INDEX      73

/**
 * @brief A result code indicating that a bare value encountered while parsing CIF starts with a character that is not
 *         allowed for that purpose
 *
 * This error is recognized where the start character does not cause the "value" to be interpreted as something else
 * altogether, such as a data name or a comment.  In CIF 2.0 parsing mode, only the dollar sign (as the first character
 * of something that could be a value) will trigger this error; in CIF 1.1 mode, the opening and closing square
 * brackets will also trigger it.
 */
#define CIF_INVALID_BARE_VALUE 74

/**
 * @brief A result code indicating that an invalid code sequence has been detected during I/O: a source character
 *         representation is not a valid code sequence in the encoding with which it is being interpreted.
 *
 * This includes those code sequences ICU describes as "illegal" as well as those it describes as "irregular".
 */
#define CIF_INVALID_CHAR      102

/**
 * @brief A result code indicating that I/O fidelity cannot be maintained on account of there being no representation
 *         for a source character in the target form.
 *
 * Recovery normally involves mapping the source character to a substitution character.
 */
#define CIF_UNMAPPED_CHAR     103

/**
 * @brief A result code indicating that a well-formed code sequence encountered during I/O decodes to a character that
 * is not allowed to appear in CIF.
 *
 * Recovery normally involves accepting the character.
 */
#define CIF_DISALLOWED_CHAR   104

/**
 * @brief A result code indicating that required whitespace was missing during CIF parsing.
 *
 * Recovery generally involves assuming the missing whitespace, which is fine as long as the reason for the error is
 * not some previous, unobserved error such as an omitted opening table delimiter.
 */
#define CIF_MISSING_SPACE     105

/**
 * @brief A result code indicating that an in-line quoted string started on the current line but was not closed before
 *         the end of the line.
 */
#define CIF_MISSING_ENDQUOTE  106

/**
 * @brief A result code indicating that a text field or triple-quoted string remained open when the end of the input
 *         was reached.
 *
 * If the closing delimiter of a text field or triple-quoted string is omitted then havoc ensues -- likely the entire
 * remainder of the file is parsed incorrectly -- possibly all as a single (unterminated) multiline value.
 */
#define CIF_UNCLOSED_TEXT    107

/**
 * @brief A result code indicating that input or output exceeded the relevant line-length limit
 */
#define CIF_OVERLENGTH_LINE   108

/**
 * @brief A result code indicating that a well-formed code sequence encountered at the beginning of CIF I/O decodes to
 * a character that is not allowed to appear as the initial character of a CIF.
 *
 * At the point where this error is encountered, it may not yet be known which version of CIF is being parsed.  The
 * normal recovery path is to leave the character alone, likely allowing it to trigger a different, more specific
 * error later.
 */
#define CIF_DISALLOWED_INITIAL_CHAR 109

/**
 * @brief A result code indicating that input is being parsed according to CIF-2 syntax, but being decoded according to
 *         a different encoding form than UTF-8
 */
#define CIF_WRONG_ENCODING    110

/**
 * @brief A result code indicating that during CIF parsing, something other than whitespace was encountered before the
 *         first block header
 */
#define CIF_NO_BLOCK_HEADER   113

/**
 * @brief A result code indicating that during CIF parsing, a save frame header was encountered but save frame support
 *         was disabled
 */
#define CIF_FRAME_NOT_ALLOWED 122

/**
 * @brief A result code indicating that during CIF parsing, a data block header or save frame header was encountered
 *         while a save frame was being parsed, thus indicating that a save frame terminator must have been omitted
 */
#define CIF_NO_FRAME_TERM     123

/**
 * @brief A result code indicating that during CIF parsing, a save frame terminator was encountered while no save frame
 *         was being parsed
 */
#define CIF_UNEXPECTED_TERM   124

/**
 * @brief A result code indicating that during CIF parsing, the end of input was encountered while parsing a save
 *         frame.
 *
 * This can indicate that the input was truncated or that the frame terminator was omitted.
 */
#define CIF_EOF_IN_FRAME      126

/**
 * @brief A result code indicating that an unquoted reserved word was encountered during CIF parsing
 */
#define CIF_RESERVED_WORD     132

/**
 * @brief A result code indicating that during CIF parsing, no data value was encountered where one was expected.
 */
#define CIF_MISSING_VALUE     133

/**
 * @brief A result code indicating that during CIF parsing, a data value was encountered where one was not expected.
 *
 * Typically this means that either the associated tag was omitted or the value itself was duplicated
 */
#define CIF_UNEXPECTED_VALUE  134

/**
 * @brief A result code indicating that during CIF parsing, a closing list or table delimiter was encountered outside
 *         the scope of any value.
 */
#define CIF_UNEXPECTED_DELIM  135

/**
 * @brief A result code indicating that during CIF parsing, a putative list or table value was encountered without a
 *         closing delimiter
 *
 * Alternatively, it is also conceivable that the previously-parsed opening delimiter was meant to be part of a value,
 * but was not so interpreted on account of the value being unquoted.
 */
#define CIF_MISSING_DELIM     136

/**
 * @brief A result code indicating that during parsing of a table value, an entry with no key was encountered
 *
 * Note that "no key" is different from "zero-length key" or "blank key"; the latter two are permitted.
 */
#define CIF_MISSING_KEY       137

/**
 * @brief A result code indicating that during parsing of a table value, an unquoted key was encountered
 *
 * The parser distinguishes this case from the "missing key" case by the presence of a colon in the bare string.
 */
#define CIF_UNQUOTED_KEY      138

/**
 * @brief A result code indicating that during parsing of a table value, text block was encountered as a table key
 *
 * The parser can handle that case just fine, but it is not allowed by the CIF 2.0 specifications.
 */
#define CIF_MISQUOTED_KEY     139

/**
 * @brief A result code indicating that during parsing of a table value, colon-delimited key/value "pair" was
 * encountered with no key at all (just colon, value)
 */
#define CIF_NULL_KEY          140

/**
 * @brief A result code indicating that during parsing, a text field putatively employing the text prefix protocol
 *         contained a line that did not begin with the prefix
 */
#define CIF_MISSING_PREFIX    141

/**
 * @brief A return code for CIF-handler functions (see @c cif_handler_tp) that indicates CIF traversal should continue
 *        along its normal path; has the same value as CIF_OK
 */
#define CIF_TRAVERSE_CONTINUE       0

/**
 * @brief A return code for CIF-handler functions (see @c cif_handler_tp) that indicates CIF traversal should bypass
 *        the current element, or at least any untraversed children, and thereafter proceed along the normal path
 */
#define CIF_TRAVERSE_SKIP_CURRENT  -1

/**
 * @brief A return code for CIF-handler functions (see @c cif_handler_tp) that indicates CIF traversal should bypass
 *        the current element, or at least any untraversed children, and any untraversed siblings, and thereafter
 *        proceed along the normal path
 */
#define CIF_TRAVERSE_SKIP_SIBLINGS -2

/**
 * @brief A return code for CIF-handler functions (see @c cif_handler_tp) that indicates CIF traversal should stop
 *        immediately
 */
#define CIF_TRAVERSE_END           -3

/**
 * @}
 *
 * @defgroup constants Other constants
 * @{
 */

/**
 * @brief The maximum number of characters in one line of a CIF
 */
#define CIF_LINE_LENGTH 2048

/**
 * @brief The maximimum number of characters in a CIF data name
 */
#define CIF_NAMELEN_LIMIT CIF_LINE_LENGTH

/**
 * @brief The category code with which the library tags the unique loop (if any) in each data block or save
 *        frame that contains items not associated with an explicit CIF @c loop_ construct
 *
 * @hideinitializer
 */
#define CIF_SCALARS (&cif_uchar_nul)

/**
 * @brief A static Unicode NUL character; a pointer to this variable constitutes an empty Unicode string.
 */
static const UChar cif_uchar_nul = 0;

/**
 * @}
 *
 * @defgroup datatypes Data types
 * @{
 */

/**
 * @brief An opaque handle on a managed CIF.
 */
typedef struct cif_s cif_tp;

/**
 * @brief An opaque handle on a managed CIF data block or save frame.
 *
 * From a structural perspective, save frames and data blocks are distinguished only by nesting level: data blocks are
 * (the only meaningful) top-level components of whole CIFs, whereas save frames are nested inside data blocks.  They
 * are otherwise exactly the same with respect to contents and allowed operations, and @c cif_container_tp models
 * that commonality.
 */
typedef struct cif_container_s cif_container_tp;

/**
 * @brief Equivalent to and interchangeable with @c cif_container_tp, but helpful for bookkeeping to track those
 *         containers that are supposed to be data blocks
 */
typedef struct cif_container_s cif_block_tp;

/**
 * @brief Equivalent to and interchangeable with @c cif_container_tp, but helpful for bookkeeping to track those
 *         containers that are supposed to be save frames
 */
typedef struct cif_container_s cif_frame_tp;

/**
 * @brief An opaque handle on a managed CIF loop
 */
typedef struct cif_loop_s cif_loop_tp;

/**
 * @brief An opaque data structure representing a CIF loop packet.
 *
 * This is not a "handle" in the sense of some of the other opaque data types, as instances have no direct connection
 * to a managed CIF.
 */
typedef struct cif_packet_s cif_packet_tp;

/**
 * @brief An opaque data structure encapsulating the state of an iteration through the packets of a loop in a
 *         managed CIF
 */
typedef struct cif_pktitr_s cif_pktitr_tp;

/**
 * @brief The type of all data value objects
 */
typedef union cif_value_u cif_value_tp;

/**
 * @brief The type used for codes representing the dynamic kind of the data in a @c cif_value_tp object
 */
typedef enum { 
    /**
     * @brief The kind code representing a character (Unicode string) data value
     */
    CIF_CHAR_KIND = 0, 

    /**
     * @brief The kind code representing a numeric or presumed-numeric data value
     */
    CIF_NUMB_KIND = 1, 

    /**
     * @brief The kind code representing a CIF 2.0 list data value
     */
    CIF_LIST_KIND = 2, 

    /**
     * @brief The kind code representing a CIF 2.0 table data value
     */
    CIF_TABLE_KIND = 3, 

    /**
     * @brief The kind code representing the not-applicable data value
     */
    CIF_NA_KIND = 4, 

    /**
     * @brief The kind code representing the unknown/unspecified data value
     */
    CIF_UNK_KIND = 5
} cif_kind_tp;

/**
 * @brief A set of functions defining a handler interface for directing and taking appropriate action in response
 *     to a traversal of a CIF.
 *
 * Each structural element of the CIF being traversed will be presented to the appropriate handler function,
 * along with the appropriate context object, if any.  When traversal is performed via the @c cif_walk() function,
 * the context object is the one passed to @c cif_walk() by its caller.  The handler may perform any action
 * it considers suitable, and it is expected to return a code influencing the traversal path, one of:
 * @li @c CIF_TRAVERSE_CONTINUE for the walker to continue on its default path (for instance, to descend to the current element's first child), or
 * @li @c CIF_TRAVERSE_SKIP_CURRENT for the walker to skip traversal of the current element's children, if any, and the current element itself if possible (meaningfully distinct from @c CIF_TRAVERSE_CONTINUE only for the @c *_start callbacks), or
 * @li @c CIF_TRAVERSE_SKIP_SIBLINGS for the walker to skip the current element if possible, its children, and all its siblings that have not yet been traversed, or
 * @li @c CIF_TRAVERSE_END for the walker to stop without traversing any more elements
 *
 * A handler function may, alternatively, return a CIF API error code, which has the effect of CIF_TRAVERSE_END plus
 * additional semantics specific to the traversal function (@c cif_walk() forwards the code to its caller as its
 * return value).
 */
typedef struct {

    /** @brief A handler function for the beginning of a top-level CIF object */
    int (*handle_cif_start)(cif_tp *cif, void *context);

    /** @brief A handler function for the end of a top-level CIF object */
    int (*handle_cif_end)(cif_tp *cif, void *context);

    /** @brief A handler function for the beginning of a data block */
    int (*handle_block_start)(cif_container_tp *block, void *context);

    /** @brief A handler function for the end of a data block */
    int (*handle_block_end)(cif_container_tp *block, void *context);

    /** @brief A handler function for the beginning of a save frame */
    int (*handle_frame_start)(cif_container_tp *frame, void *context);

    /** @brief A handler function for the end of a save frame */
    int (*handle_frame_end)(cif_container_tp *frame, void *context);

    /** @brief A handler function for the beginning of a loop */
    int (*handle_loop_start)(cif_loop_tp *loop, void *context);

    /** @brief A handler function for the end of a loop */
    int (*handle_loop_end)(cif_loop_tp *loop, void *context);

    /** @brief A handler function for the beginning of a loop packet */
    int (*handle_packet_start)(cif_packet_tp *packet, void *context);

    /** @brief A handler function for the end of a loop packet */
    int (*handle_packet_end)(cif_packet_tp *packet, void *context);

    /** @brief A handler function for data items (there are not separate beginning and end callbacks) */
    int (*handle_item)(UChar *name, cif_value_tp *value, void *context);

} cif_handler_tp;

/**
 * @brief The type of a callback function to be invoked when a parse error occurs.
 *
 * Note that this function receives the location where the error was @em detected , which is not necessarily the
 * location of the actual error.
 *
 * @param[in] code a parse error code indicating the nature of the error
 * @param[in] line the one-based line number at which the error was detected
 * @param[in] column the one-based column number at which the error was detected
 * @param[in] text if not @c NULL , a Unicode string -- not necessarily NUL terminated -- containing the specific
 *         CIF text being parsed where the error was detected
 * @param[in] length the number of @c UChar code units starting at the one @c text points to that may be relevant
 *         to the reported error.  Accessing @c text[length] or beyond produces undefined behavior.
 * @param[in,out] data a pointer to the user data object provided by the parser caller
 *
 * @return zero if the parse should continue (with implementation-dependent best-effort error recovery), or nonzero
 *         if the parse should be aborted, forwarding the return code to the caller of the parser 
 */
typedef int (*cif_parse_error_callback_tp)(int code, size_t line, size_t column, const UChar *text, size_t length, void *data);

/**
 * @brief The type of a callback function by which a client application can be notified of whitespace encountered
 *     during CIF parsing.
 *
 * CIF whitespace appearing outside any data value is not semantically significant from a CIF perspective, but it may
 * be useful to the calling application.  An application interested in being notified about whitespace as it is parsed
 * can register a callback function of this type with the parser.  Unlike a CIF handler or an error callback, whitespace
 * callbacks cannot directly influence CIF traversal or interrupt parsing, but they have access to the same user data
 * object that all other callbacks receive.
 *
 * The pointer @p ws and all subsequent values through @p ws @c + @p length are guaranteed to be valid pointer values
 * for comparison and arithmetic.  The result of dereferencing @p ws @c + @p length is undefined.
 *
 * Whitespace is generally reported in multi-character blocks, which may span lines, but the reported whitespace
 * sequences are not guaranteed to be maximal.  Physical runs of whitespace in the input CIF may be split across more
 * than one callback call.
 * 
 * @param[in] line the one-based line number of the start of the whitespace
 * @param[in] column the one-based column number of the first character of the whitespace run
 * @param[in] ws a pointer to the start of the whitespace sequence, which is @em not necessarily NUL-terminated
 * @param[in] length the number of characters in the whitespace sequence.  ( @p ws + @p length ) is guaranteed to be a
 *         valid pointer value for comparison and arithmetic, but the result of dereferencing it is undefined; whether
 *         incrementing it results in a valid pointer value is undefined.
 * @param[in,out] data a pointer to the user data object provided by the parser caller
 */
typedef void (*cif_whitespace_callback_tp)(size_t line, size_t column, const UChar *ws, size_t length, void *data);

/**
 * @brief Represents a collection of CIF parsing options.
 *
 * Unlike most data types defined by the CIF API, the parse options are not opaque.  This reflects the @c struct's
 * intended use for collecting (only) user-settable option values.  There is nevertheless still an object creation
 * function, @c cif_parse_options_create() ; applications allocating parse options only via that function will thereby
 * insulate themselves against changes to the struct's size arising from additions to the option list in future
 * versions of the API.
 */
struct cif_parse_opts_s {

    /**
     * @brief If non-zero, directs the parser to handle a CIF stream without any CIF-version code as CIF 2, instead
     *         of as CIF 1.
     *
     * Because the CIF-version code is @em required in CIF 2 but optional in CIF 1, it is most correct to assume CIF 1
     * when there is no version code.  Nevertheless, if a CIF is known or assumed to otherwise comply with CIF2, then
     * it is desirable to parse it that way regardless of the absence of a version code.
     *
     * CIF 2 streams that erroneously omit the version code will be parsed as CIF 2 if this option is enabled (albeit
     * with an error on account of the missing version code).  On the other hand, CIF 1 streams that (allowably)
     * omit the version code may be parsed incorrectly if this option is enabled.
     */
    int default_to_cif2;

    /**
     * @brief If not @c NULL , names the coded character set with which the parser will attempt to interpret plain
     *         CIF 1.1 "text" files that do not bear recognized encoding information.
     *
     * Inasmuch as CIF 1 is a text format and CIF 2.0 is a text-like format, it is essential for the parser to interpret
     * them according to the text encodings with which they are written.  Well-formed CIF 2.0 is Unicode text encoded
     * via UTF-8, but the CIF 1 specifications are intentionally vague about the terms "text" and "text file", intending
     * them to be interpreted in a system-dependent manner.  Additionally, documents that conform to CIF 2.0 except with
     * respect to encoding can still be parsed successfully (though an error will be flagged) if the correct encoding
     * can be determined.
     *
     * The parser will recognize UTF-8, UTF-16 (either byte order), and UTF-32 (either of two byte orders) for CIFs that
     * begin with a Unicode byte-order mark (BOM).  In most cases it will recognize UTF-16 and UTF-32 encodings even
     * without a BOM, as well.  In some cases it may recognize UTF-8 or other encodings when there is no BOM, and
     * well-formed CIF 2.0 documents will always be recognized as UTF-8 (also wrongly-encoded CIF 2.0 documents may be
     * interpreted as UTF-8).
     *
     * If no encoding is specified via this option (i.e. it is @c NULL ) and no encoding signature is recognized in the
     * input CIF, then for CIF 1.1 documents the parser will choose a default text encoding that is appropriate to the
     * system on which it is running, and it will attempt to parse according to that encoding.  How the default encoding
     * is chosen is implementation dependent in this case.
     *
     * If the correct encoding of the input CIF is known, however, then that encoding can be specified by its IANA name
     * via this option.  Implementations may also recognize aliases and / or unregistered encoding names.  If it is
     * supported, the named encoding will be used in the event that no encoding signature is detected, bypassing the
     * library's usual method for choosing a default encoding.  This allows a CIF written in a localized encoding to be
     * parsed correctly on a system with a different default locale.  It should be noted, however, that such a CIF can
     * reasonably be considered erroneous (on the system where it is being parsed) on account of its encoding.
     *
     * The names supported by this option are those recognized by ICU's "converter" API as converter / code page names.
     */
    const char *default_encoding_name;

    /**
     * @brief If non-zero then the default encoding specified by @c default_encoding_name will be used to interpret the
     * CIF 1.1 or 2.0 input, regardless of any encoding signature or other appearance to the contrary.
     *
     * If @c default_encoding_name is NULL then it represents a system-dependent default encoding.  That's the norm
     * for CIF 1.1 input anyway, but if @c force_default_encoding is nonzero then the same system-dependent default will
     * be chosen for CIF 2.0 as well.
     *
     * This option is dangerous.  Enabling it can cause CIF parsing to fail, or in some cases cause CIF contents to
     * silently be misinterpreted, if the specified default encoding is not in fact the correct encoding for the input.
     * On the other hand, use of this option is essential for correctly parsing CIF documents whose encoding cannot be
     * determined or guessed correctly.
     *
     * This option can be used to parse CIF 2.0 text that is encoded other than via UTF-8.  Such a file is not valid
     * CIF 2.0, and therefore will cause an error to be flagged, but if the error is ignored and the specified encoding
     * is in fact correct for the input then parsing will otherwise proceed normally.
     */
    int force_default_encoding;

    /**
     * @brief Modifies whether line-folded text fields will be recognized and unfolded during parsing.
     *
     * The line-folding protocol for text fields is part of the CIF 2.0 specification, but it is only a common
     * convention for CIF 1.  By default, therefore, the parser will recognize and unfold line-folded text fields
     * when it operates in CIF 2.0 mode, but it will pass them through as-is when it operates in CIF 1.0 mode.  This
     * option influences that behavior: if greater than zero then the parser will unfold line-folded text fields
     * regardless of CIF version, and if less than zero then it will @em not recognize or unfold line-folded text
     * fields even in CIF 2 mode.
     *
     * Note that where a text field has been both line-folded and prefixed, the line-folding can be recognized only
     * if line unfolding and text de-prefixing are @em both enabled.
     */
    int line_folding_modifier;

    /**
     * @brief Modifies whether prefixed text fields will be recognized and de-prefixed during parsing.
     *
     * The prefix protocol for text fields is part of the CIF 2.0 specification, but for CIF 1 it is only a local
     * convention of certain organizations.  By default, therefore, the parser will recognize and de-prefix prefixed
     * text fields when it operates in CIF 2.0 mode, but it will pass them through as-is when it operates in CIF 1.0
     * mode. This option influences that behavior: if greater than zero then the parser will de-prefix prefixed text
     * fields regardless of CIF version, and if less than zero then it will @em not recognize or de-prefix prefixed
     * text fields even in CIF 2 mode.
     *
     * Note that where a text field has been both line-folded and prefixed, the prefixing can be recognized only
     * if line unfolding and text de-prefixing are @em both enabled.
     */
    int text_prefixing_modifier;

    /**
     * @brief The maximum save frame depth.
     *
     * If 1, then one level of save frames will be accepted (i.e. save frames are allowed, but must not be nested).
     * If 0 then all save frames will be rejected as erroneous.  That might be used to ensure that CIF data files (as
     * opposed to dictionaries) do not contain save frames.
     * If negative, then save frames are allowed, and may be nested without limit.
     * Values greater than 1 are @b reserved for possible future use as a bound on save frame nesting depth.
     *
     * The current version of STAR allows nested save frames, and their use was proposed for CIF 2, especially for
     * DDLm dictionaries.  Save frame nesting was not accepted into the CIF 2.0 standard, however, even for
     * dictionary files.
     *
     * The default is 1.
     */
    int max_frame_depth;

    /**
     * @brief ASCII characters that should be interpreted as CIF inline whitespace
     *
     * If not @c NULL, specifies additional characters from the 7-bit ASCII set and / or from among the C1 controls that
     * should be accepted as representing CIF inline whitespace.  Ordinarily, only the space character and the tab
     * character have this function, and the C1 controls and most of the C0 controls are disallowed altogether.  This
     * option is mainly intended to allow successful parsing of pre-v1.1 CIFs, some of which used the vertical tab
     * character as inline whitespace, but it is not inherently limited to that use.
     *
     * The space or tab may be specified among these characters, but it has no additional effect.
     *
     * The carriage return and newline characters may be specified among these characters, but it does not change
     * their role as end-of-line characters.
     *
     * The string is terminated by a null character (@c '\\0'); no mechanism is provided for treating that character as
     * whitespace.
     */
    const char *extra_ws_chars;

    /**
     * @brief ASCII characters that should be interpreted as CIF end-of-line [whitespace] characters
     *
     * If not @c NULL, specifies additional characters from the 7-bit ASCII set and / or from among the C1 controls that
     * should be accepted as representing CIF end-of-line characters.  Ordinarily, only the carriage return and newline
     * characters have this function, and the C1 controls and most of the C0 controls are disallowed altogether.  This
     * option is mainly intended to allow successful parsing of pre-v1.1 CIFs, some of which used the form feed
     * character as an end-of-line character, but it is not inherently limited to that use.
     *
     * Extra end-of-line characters appearing within data values are subject to the same conversion to newlines as
     * carriage return characters and carriage return / line feed pairs.
     *
     * The space and tab characters cannot be given end-of-line significance via this option; if they appear in the
     * provided string then they are ignored.
     *
     * The carriage return and newline characters may be specified among these characters, but it has no additional
     * effect.
     *
     * The string is terminated by a null character (@c '\\0'); no mechanism is provided for treating that character as
     * end-of-line.
     */
    const char *extra_eol_chars;

    /**
     * @brief A set of handler functions by which the application can be notified of details of the parse progress as
     *         they occur, and through which it can influence the data recorded; may be @c NULL.
     *
     * If not itself NULL, then any non-null handler functions in the @c cif_handler_tp to which this option points will
     * be invoked by the parser at appropriate times as it traverses the input CIF text.  The handlers' return codes
     * @c CIF_TRAVERSE_SKIP_CURRENT and @c CIF_TRAVERSE_SKIP_SIBLINGS are interpreted as directing which data to record
     * in the target CIF, if indeed the user has provided one.  (The parser cannot altogether skip parsing any part
     * of the input if it must identify the end of that part and there resume normal operation.)  Handler callbacks are
     * not invoked for entities thereby passed over.  Handlers may modify the CIF under construction, subject to the
     * limitations inherent in the CIF being incompletely constructed when they are called.
     */
    cif_handler_tp *handler;

    /**
     * @brief A callback function by which the client application can be notified about whitespace encountered outside
     *         data values
     *
     * If not @c NULL, this function will be called by the parser whenever it encounters a run of insignificant
     * whitespace (including comments) in the input CIF.  Whitespace is insignificant if it serves only to separate
     * other elements appearing in the CIF.  The parser does not guarantee to collect @em maximal whitespace runs;
     * it may at times split consecutive whitespace into multiple runs, performing a callback for each one.
     */
    cif_whitespace_callback_tp whitespace_callback;

    /**
     * @brief A callback function by which the client application can be notified about parse errors, affording it the
     *         option to interrupt the parse or allow it to continue.
     *
     * If not @c NULL, this function will be called by the parser whenever it encounters an error in the input CIF.
     * The parse will be aborted immediately with an error if this function returns non-zero; otherwise, it
     * serves informational purposes only.  Two pre-built error callbacks are provided: @c cif_parse_error_ignore()
     * and @c cif_parse_error_die().
     *
     * If @c NULL, or if parse options are not specified, then the parser will operate as if the error handler were
     * @c cif_parse_error_die().
     */
    cif_parse_error_callback_tp error_callback;

    /**
     * @brief A pointer to user data to be forwarded to all callback functions invoked by the parser; opaque to the
     *         parser itself, and may be @c NULL.
     */
    void *user_data;
};

/**
 * @brief Represents a collection of CIF writing options.
 *
 * This is a place-holder for future options.  No CIF write options are defined for the present version of the CIF API.
 */
struct cif_write_opts_s {

    /**
     * @brief Indicates the CIF major version with which the output must comply.
     *
     * Valid values include @c 1 for CIF 1.1 and @c 2 for CIF 2.0; value `0` means the default for the version of
     * the CIF API in use; for the present version, the default is CIF 2.0.
     *
     * Note that the text-prefix protocol will be applied in CIF 1.1 mode for values that otherwise could not be
     * represented.
     */
    int cif_version;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @}
 *
 * @defgroup cifio_funcs CIF input / output functions
 *
 * @{
 */

/**
 * @brief Parses a CIF from the specified stream using the library's built-in parser.
 *
 * The data are interpreted as a standalone CIF providing zero or more data blocks to add to the provided or a new
 * managed CIF object.  The caller asserts that the new and any pre-existing data are part of the same logical CIF
 * data set, therefore it constitutes a parse error for any data blocks in the provided CIF to have block codes that
 * match that of one of the existing blocks.
 *
 * If a new CIF object is created via this function then the caller assumes responsibility for releasing its
 * resources at an appropriate time via @c cif_destroy().
 *
 * @b CIF @b handler @b callbacks.  The parse options allow the caller to register a variety of callback functions by
 * which the parse can be tracked and influenced.  Most of these are wrapped together in the @c handler option, by
 * which a @c cif_handler_tp object can be provided.  Handler functions belonging to such a handler will be called when
 * appropriate during CIF parsing, and the returned navigational signals direct which parts of the input data are
 * included in the in-memory CIF rpresentation, if any, constructed by the parse.  To some extent, the CIF can be
 * modified during parse time, too; for example, loop categories may be assigned via callback.
 *
 * @b Error @b handler @b callback.  The parse options also afford the caller an opportunity to specify an error-handler
 * callback.  The provided function will be invoked whenever any error is detected in the CIF input, and any return
 * code other than @c CIF_OK will be immediately returned to the @c cif_parse() caller, aborting the remainder of the
 * parse.  This feature can be used to record or display information about the errors encountered, and / or to
 * selectively ignore some errors (invoking the built-in recovery approach).  Two pre-built error callbacks are
 * provided: @c cif_parse_error_ignore() and @c cif_parse_error_die().  If no error handler is specified by the caller
 * then @c cif_parse_error_die() is used.
 *
 * @b Syntax-only @b mode.  If the @a cif argument is NULL then the parse is performed in syntax-only mode.  Errors
 * in CIF syntax will be detected as normal, but some semantic errors, such as duplicate data names, frame codes, or
 * block codes will not be detected.  Also, the handles provided to CIF handler functions during the parse may be NULL
 * or may yield less information in this mode.
 *
 * @param[in,out] stream a @c FILE @c * from which to read the raw CIF data; must be a non-NULL pointer to a readable
 *         stream, which will typically be read to its end.  This stream should be open in @b BINARY mode
 *         (e.g. fopen() mode "rb") on any system where that makes a difference.  The caller retains
 *         ownership of this stream.
 *
 * @param[in] options a pointer to a @c struct @c cif_parse_opts_s object describing options to use while parsing, or
 *         @c NULL to use default values for all options
 *
 * @param[in,out] cif controls the disposition of the parsed data.  If NULL, then the parsed data are discarded.
 *         Otherwise, they are added to the CIF to which this pointer refers.  If a NULL handle is initially recorded
 *         at the referenced location then a new CIF is created to receive the data and its handle is recorded here.
 *         In the latter case, ownership of the new CIF goes to the caller.
 * 
 * @return Returns @c CIF_OK on a successful parse, even if the results are discarded, or an error code
 *         (typically @c CIF_ERROR ) on failure.  In the event of a failure, a new CIF object may still be created
 *         and returned via the @c cif argument, or the provided CIF object may still be modified.
 */
CIF_INTFUNC_DECL(cif_parse, (
        FILE *stream,
        struct cif_parse_opts_s *options,
        cif_tp **cif
        ));

/**
 * @brief Allocates a parse options structure and initializes it with default values.
 *
 * Obtaining a parse options structure via this function insulates programs against additions to the option list in
 * future versions of the library.  Programs may create @c cif_parse_opts_s objects by other means -- for example, by
 * allocating them on the stack -- but the behavior of programs that do may be undefined if they are dynamically linked
 * against a future version of the library that adds fields to the structure definition, perhaps even if they are
 * re-compiled against revised library headers.  This is not an issue for statically-linked programs that fully
 * initialize their options.
 *
 * On successful return, the provided options object belongs to the caller.  An options structure as provided by this
 * function may safely be freed, or its members overwritten, without concern for freeing memory referenced by the
 * members.  The user is responsible for accommodating any deviation from those characteristics that are introduced to
 * the structure after this function provides it.
 *
 * @param[in,out] opts a the location where a pointer to the new parse options structure should be recorded.  The
 *         initial value of @p *opts is ignored, and is overwritten on success.
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_parse_options_create, (
        struct cif_parse_opts_s **opts
        ));

/**
 * @brief A CIF parse error handler function that ignores all errors
 */
CIF_INTFUNC_DECL(cif_parse_error_ignore, (
        int code,
        size_t line,
        size_t column,
        const UChar *text,
        size_t length,
        void *data
        ));

/**
 * @brief A CIF parse error handler function that interrupts the parse on any error, returning @c code
 */
CIF_INTFUNC_DECL(cif_parse_error_die, (
        int code,
        size_t line,
        size_t column,
        const UChar *text,
        size_t length,
        void *data
        ));

/**
 * @brief Formats the CIF data represented by the @c cif handle to the specified output stream.
 *
 * Ownership of the arguments does not transfer to the function.
 *
 * @param[in,out] stream a @c FILE @c * to which to write the CIF format output; must be a non-NULL pointer to a
 *         writable stream.  In the event that the write options request CIF 1.1 output, this file should be open in
 *         @em text mode (on those systems that have distinct text and binary modes)
 *
 * @param[in] options a pointer to a @c struct @c cif_write_opts_s object describing options to use for writing, or
 *         @c NULL to use default values for all options
 *
 * @param[in] cif a handle on the CIF object to serialize to the specified stream
 *
 * @return Returns @c CIF_OK if the data are fully written, or else an error code (typically @c CIF_ERROR ).
 *         The stream state is undefined after a failure.
 */
CIF_INTFUNC_DECL(cif_write, (
        FILE *stream,
        struct cif_write_opts_s *options,
        cif_tp *cif
        ));

/**
 * @brief Allocates a write options structure and initializes it with default values.
 *
 * Obtaining a write options structure via this function insulates programs against additions to the option list in
 * future versions of the library.  Programs may create @c cif_write_opts_s objects by other means -- for example, by
 * allocating them on the stack -- but the behavior of programs that do is undefined if they are dynamically linked
 * against a future version of this API that adds fields to the structure definition.  Statically-linked programs
 * that fully initialize their write options structures are not subject to such issues.
 *
 * On successful return, the provided options object belongs to the caller.  An options structure as provided by this
 * function may safely be freed, or its members overwritten, without concern for freeing memory referenced by the
 * members.  The user is responsible for accommodating any deviation from those characteristics that are introduced to
 * the structure after this function provides it.
 *
 * @param[in,out] opts a the location where a pointer to the new write options structure should be recorded.  The
 *         initial value of @p *opts is ignored, and is overwritten on success.
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_write_options_create, (
        struct cif_write_opts_s **opts
        ));

/**
 * @}
 *
 * @defgroup whole_cif_funcs Whole-CIF functions 
 *
 * @{
 */

/**
 * @brief Creates a new, empty, managed CIF.
 *
 * If the function succeeds then the caller assumes responsibility for releasing the resources associated with the
 * managed CIF via the @c cif_destroy() function.
 *
 * @param[out] cif a pointer to the location where a handle on the managed CIF should be recorded; must not be NULL.
 *         The initial value of @p *cif is ignored, and is overwritten on success.
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_create, (
        cif_tp **cif
        ));

/**
 * @brief Removes the specified managed CIF, releasing all resources it holds.
 *
 * The @c cif handle and any outstanding handles on its contents are invalidated by this function.  The effects of
 * any further use of them (except release via the @c cif_*_free() functions) are undefined, though the functions of
 * this library @em may return code @c CIF_INVALID_HANDLE if such a handle is passed to them.  Any resources belonging
 * to the provided handle itself are also released, but not those belonging to any other handle.  The original external
 * source (if any) of the CIF data is not affected.
 *
 * @param[in] cif the handle on the CIF to destroy; must not be NULL.  On success, this handle ceases to be valid.
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_destroy, (
        cif_tp *cif
        ));

/*
 * There is no cif_clean() or cif_free() -- these would inherently cause
 * resource leakage if they did anything less than cif_destroy() does.
 */

/**
 * @brief Creates a new data block bearing the specified code in the specified CIF.
 *
 * This function can optionally return a handle on the new block via the @c block argument.  When that option
 * is used, the caller assumes responsibility for cleaning up the provided handle via either @c cif_container_free()
 * (to clean up only the handle) or @c cif_container_destroy() (to also remove the block from the managed CIF).
 *
 * The caller retains ownership of all the arguments.
 *
 * @param[in] cif a handle on the managed CIF object to which a new block should be added; must be non-NULL and
 *        valid.
 *
 * @param[in] code the block code of the block to add, as a NUL-terminated Unicode string; the block code must
 *        comply with CIF constraints on block codes.
 *
 * @param[in,out] block if not NULL, then a location where a handle on the new block should be recorded, overwriting
 *        any previous value.  A handle is recorded only on success.
 *
 * @return @c CIF_OK on success or an error code on failure, normally one of:
 *        @li @c CIF_ARGUMENT_ERROR  if the provided block code is NULL
 *        @li @c CIF_INVALID_BLOCKCODE  if the provided block code is invalid
 *        @li @c CIF_DUP_BLOCKCODE if the specified CIF already contains a block having the given code (note that
 *                block codes are compared in a Unicode-normalized and caseless form)
 *        @li @c CIF_ERROR for most other failures
 */
CIF_INTFUNC_DECL(cif_create_block, (
        cif_tp *cif,
        const UChar *code,
        cif_block_tp **block
        ));

/**
 * @brief Looks up and optionally returns the data block bearing the specified block code, if any, in the specified CIF.
 *
 * Note that CIF block codes are matched in caseless and Unicode-normalized form.
 *
 * The caller retains ownership of all arguments.
 *
 * @param[in] cif a handle on the managed CIF object in which to look up the specified block code; must be non-NULL
 *         and valid.
 *
 * @param[in] code the block code to look up, as a NUL-terminated Unicode string.
 *
 * @param[in,out] block if not NULL on input, and if a block matching the specified code is found, then a handle on it
 *         is written where this argument points.  The caller assumes responsibility for releasing this handle.
 *       
 * @return @c CIF_OK on a successful lookup (even if @c block is NULL), @c CIF_NOSUCH_BLOCK if there is no data block
 *         bearing the given code in the given CIF, or an error code (typically @c CIF_ERROR ) if an error occurs.
 */
CIF_INTFUNC_DECL(cif_get_block, (
        cif_tp *cif,
        const UChar *code,
        cif_block_tp **block
        ));

/**
 * @brief Provides a null-terminated array of data block handles, one for each block in the specified CIF.
 *
 * The caller takes responsibility for cleaning up the provided block handles and the dynamic array containing them.
 *
 * @param[in] cif a handle on the managed CIF object from which to draw block handles; must be non-NULL and valid.
 *         The caller retains ownership of this argument.
 *
 * @param[in,out] blocks a pointer to the location where a pointer to the resulting array of block handles should be
 *         recorded.  Must not be NULL.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_get_all_blocks, (
        cif_tp *cif,
        cif_block_tp ***blocks
        ));

/**
 * @brief Performs a depth-first, natural-order traversal of a CIF, calling back to handler
 *        routines provided by the caller for each structural element
 *
 * Traverses the tree structure of a CIF, invoking caller-specified handler functions for each structural element
 * along the route.  Order is block -> [frame -> ...] loop -> packet -> item, with save frames being traversed before
 * loops within each data block.  Handler callbacks can influence the walker's path via their return values,
 * instructing it to continue as normal (@c CIF_TRAVERSE_CONTINUE), to avoid traversing the children of the current
 * element (@c CIF_TRAVERSE_SKIP_CURRENT), to avoid traversing subsequent siblings of the current element
 * (@c CIF_TRAVERSE_SKIP_SIBLINGS), or to terminate the walk altogether (@c CIF_TRAVERSE_END).  For the purposes of
 * this function, loops are not considered "siblings" of save frames.
 *
 * @param[in] cif a handle on the CIF to traverse
 * @param[in] handler a structure containing the callback functions handling each type of CIF structural element
 * @param[in] context a context object to pass to each callback function; opaque to the walker itself
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR) on failure
 */
CIF_INTFUNC_DECL(cif_walk, (
        cif_tp *cif,
        cif_handler_tp *handler,
        void *context
        ));

/**
 * @}
 *
 * @defgroup block_funcs Data block specific functions
 *
 * @{
 */

/**
 * @brief An alias for @c cif_container_free
 *
 * @hideinitializer
 */
#define cif_block_free cif_container_free

/**
 * @brief An alias for @c cif_container_destroy
 *
 * @hideinitializer
 */
#define cif_block_destroy cif_container_destroy

/**
 * @brief An alias for cif_container_create_frame
 *
 * @hideinitializer
 */
#define cif_block_create_frame cif_container_create_frame

/**
 * @brief An alias for cif_container_get_frame
 *
 * @hideinitializer
 */
#define cif_block_get_frame cif_container_get_frame

/**
 * @brief An alias for cif_container_get_all_frames
 *
 * @hideinitializer
 */
#define cif_block_get_all_frames cif_container_get_all_frames

/**
 * @}
 *
 * @defgroup frame_funcs Save frame specific functions
 *
 * @{
 */

/**
 * @brief An alias for @c cif_container_free
 *
 * @hideinitializer
 */
#define cif_frame_free cif_container_free

/**
 * @brief An alias for @c cif_container_destroy
 *
 * @hideinitializer
 */
#define cif_frame_destroy cif_container_destroy

/**
 * @}
 *
 * @defgroup container_funcs Data container general functions
 *
 * @{
 */

/**
 * @brief Creates a new save frame bearing the specified frame code in the specified container.
 *
 * This function can optionally return a handle on the new frame via the @c frame argument.  When that option
 * is used, the caller assumes responsibility for cleaning up the provided handle via either @c cif_container_free()
 * (to clean up only the handle) or @c cif_container_destroy() (to also remove the frame from the managed CIF).
 *
 * Note that this function exhibits an extension to the CIF 2.0 data model in that CIF 2.0 does not permit save
 * frames to contain other save frames, whereas this function will happily create just such nested
 * frames if asked to do so.
 *
 * @param[in] container a handle on the managed data block (or save frame) object to which a new block should be
 *        added; must be non-NULL and valid.
 *
 * @param[in] code the frame code of the frame to add, as a NUL-terminated Unicode string; the frame code must
 *        comply with CIF constraints on frame codes.
 *
 * @param[in,out] frame if not NULL, then a location where a handle on the new frame should be recorded.
 *
 * @return @c CIF_OK on success or an error code on failure, normally one of:
 *        @li @c CIF_INVALID_FRAMECODE  if the provided frame code is invalid
 *        @li @c CIF_DUP_FRAMECODE if the specified data block already contains a frame having the given code (note that
 *                frame codes are compared in a Unicode-normalized and caseless form)
 *        @li @c CIF_ERROR for most other failures
 */
CIF_INTFUNC_DECL(cif_container_create_frame, (
        cif_container_tp *container,
        const UChar *code,
        cif_frame_tp **frame
        ));

/**
 * @brief Frees any resources associated with the specified container handle without
 * modifying the associated managed CIF
 *
 * @param[in] container a handle on the container object to free; must be non-NULL and valid on entry, but is
 *         invalidated by this function
 */
CIF_VOIDFUNC_DECL(cif_container_free, (
        cif_container_tp *container
        ));

/**
 * @brief Frees any resources associated with the specified container handle, and furthermore removes the associated
 *         container and all its contents from its managed CIF.
 *
 * @param[in,out] container a handle on the container to destroy; must be non-NULL and valid on entry, but is
 *         invalidated by this function
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_destroy, (
        cif_container_tp *container
        ));

/**
 * @brief Looks up and optionally returns the save frame bearing the specified code, if any, in the specified data
 *         block or save frame.
 *
 * Note that CIF frame codes are matched in caseless and Unicode-normalized form.
 *
 * @param[in] container a handle on the managed data block or save frame object
 *         in which to look up the specified frame code; must be non-NULL and valid.
 *
 * @param[in] code the frame code to look up, as a NUL-terminated Unicode string.
 *
 * @param[in,out] frame if not NULL and a frame matching the specified code is found, then a handle on it is
 *         written where this parameter points.  The caller assumes responsibility for releasing this handle.
 *       
 * @return @c CIF_OK on a successful lookup (even if @c frame is NULL), @c CIF_NOSUCH_FRAME if there is no save frame
 *         bearing the given code in the given CIF, or an error code (typically @c CIF_ERROR ) if an error occurs.
 */
CIF_INTFUNC_DECL(cif_container_get_frame, (
        cif_container_tp *container,
        const UChar *code,
        cif_frame_tp **frame
        ));

/**
 * @brief Provides a null-terminated array of save frame handles, one for each frame in the specified data block or save frame.
 *
 * The caller takes responsibility for cleaning up the provided frame handles and the dynamic array containing them.
 *
 * @param[in] container a handle on the managed data block or save frame object from which to draw save frame
 *         handles; must be non-NULL and valid.
 *
 * @param[in,out] frames a pointer to the location where a pointer to the resulting array of frame handles should be
 *         recorded.  Must not be NULL.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_get_all_frames, (
        cif_container_tp *container,
        cif_frame_tp ***frames
        ));

/**
 * @brief Retrieves the identifying code (block code or frame code) of the specified container.
 *
 * Responsibility for cleaning up the block or frame codes retrieved via this function belongs to the caller.
 *
 * @param[in] container a handle on the container whose code is requested
 *
 * @param[in,out] code a pointer to the location where a pointer to a NUL-terminated Unicode string containing the
 *         requested code should be recorded
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_get_code, (
        cif_container_tp *container,
        UChar **code
        ));

/**
 * @brief Asserts that the specified container represents a data block, as opposed to a save frame
 *
 * @param[in] container a handle on the container that is asserted to be a data block
 *
 * @return @c CIF_OK if the container is a data block, @c CIF_ARGUMENT_ERROR if it is a save frame, or @c CIF_ERROR if
 *         it is @c NULL
 */
CIF_INTFUNC_DECL(cif_container_assert_block, (
        cif_container_tp *container
        ));

/**
 * @brief Creates a new loop in the specified container, and optionally returns a handle on it.
 *
 * There must be at least one item name, and all item names given must initially be absent from the container.
 * All item names must be valid, per the CIF specifications.
 *
 * The category name may be NULL, in which case the loop has no explicit category, but the empty category name is
 * reserved for the loop containing all the scalar data in the container.  The preprocessor macro @c CIF_SCALARS
 * expands to that category name; it is provided to make the meaning clearer.  Other categories are not required to be
 * unique, but they are less useful when there are duplicates in the same container.
 *
 * New loops initially contain zero packets, which is acceptable only as a transient state.
 *
 * @param[in] container a handle on the container in which to create a loop.
 *
 * @param[in] category the loop's category, as a NUL-terminated Unicode string.  May be NULL.
 *
 * @param[in] names an array of pointers to the item names that should appear in the loop, as NUL-terminated Unicode
 *         strings, with the end of the list indicated by a NULL pointer; ownership of the array and its contents
 *         does not transfer
 *
 * @param[in,out] loop if not NULL, points to the location where a handle on the newly-created loop should be recorded.
 *
 * @return Returns @c CIF_OK on success, or an error code on failure, among them:
 *        @li @c CIF_NULL_LOOP if an empty list of item names is specified
 *        @li @c CIF_INVALID_ITEMNAME if one or more of the provided item names does not satisfy CIF's criteria
 *        @li @c CIF_DUP_ITEMNAME if one or more of the item names is already present in the target container
 *        @li @c CIF_ERROR in most other failure cases
 */
CIF_INTFUNC_DECL(cif_container_create_loop, (
        cif_container_tp *container,
        const UChar *category,
        UChar *names[],
        cif_loop_tp **loop
        ));

/**
 * @brief Looks up the loop in the specified container that is assigned to the specified category.
 *
 * Unlike block code, frame code, or item name lookup, category lookup works by exact, character-by-character matching.
 *
 * @param[in] container a handle on the data block or save frame in which to look for a loop, must be non-NULL and
 *         valid
 *
 * @param[in] category the category to look up, as a NUL-terminated Unicode string; must not be NULL
 *
 * @param[in,out] loop if not NULL, the location where a handle on the discovered loop, if any, should be written; in
 *         that case, the caller is responsible for cleaning up the returned handle
 *
 * @return a status code indicating the nature of the result:
 *         @li @c CIF_OK if a single matching loop was found; only in this case can a handle be returned
 *         @li @c CIF_INVALID_CATEGORY if the specified category name is invalid (primarily, if it is NULL)
 *         @li @c CIF_NOSUCH_LOOP if no loop having the specified category is found
 *         @li @c CIF_CAT_NOT_UNIQUE if multiple loops having the specified category are found
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_container_get_category_loop, (
        cif_container_tp *container,
        const UChar *category,
        cif_loop_tp **loop
        ));

/**
 * @brief Looks up the loop in the specified container that contains the specified item, and optionally returns
 *        a handle on it.
 *
 * Note that in the underlying data model, @em all items belong to loops.  To determine whether an item is logically
 * a scalar, either count the packets in its loop, or check whether the loop's category is @c CIF_SCALARS (depending
 * on what, exactly, you mean by "scalar").
 *
 * @param[in] container a handle on the container in which to look up the item's loop
 *
 * @param[in] item_name the item name to look up, as a NUL-terminated Unicode string.  Matching is performed via a
 *            caseless, Unicode-normalized algorithm.
 *
 * @param[in,out] loop if not NULL, the location where a handle on the discovered loop, if any, is written
 *
 * @return Returns a status code indicating the nature of the result:
 *         @li @c CIF_OK if the item was found; only in this case can a handle be returned
 *         @li @c CIF_NOSUCH_ITEM if the item is not present in the container
 *         @li @c CIF_ERROR in most other cases
 *         The return code does not depend on whether @c loop is NULL.
 */
CIF_INTFUNC_DECL(cif_container_get_item_loop, (
        cif_container_tp *container,
        const UChar *item_name,
        cif_loop_tp **loop
        ));

/**
 * @brief Provides a null-terminated array of loop handles, one for each loop in the specified container.
 *
 * The caller takes responsibility for cleaning up the provided loop handles and the array containing them.
 *
 * @param[in] container a handle on the managed container object from which to draw loop handles; must be non-NULL and
 *         valid.
 *
 * @param[in,out] loops a pointer to the location where a pointer to the resulting array of loop handles should be
 *         recorded.  Must not be NULL.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_get_all_loops, (
        cif_container_tp *container,
        cif_loop_tp ***loops
        ));

/**
 * @brief Removes all data-less loops from the specified container
 *
 * The @c cif_container_create_loop() function creates loops that initially contain no data (which is intended to be
 * a transitory state), and it is possible via the packet iterator interface to remove all packets from a previously
 * non-empty loop.  The CIF data model does not accommodate loops without data, so by whatever path they are
 * introduced, empty loops must be removed or filled in order to achieve a valid CIF.  This convenience function serves
 * that purpose by efficiently removing all empty loops belonging directly to the specified container.
 *
 * @param[in] container a handle on the data block or save frame from which to prune empty loops
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_prune, (
        cif_container_tp *container
        ));

/**
 * @brief Looks up an item by name in a data block or save frame, and optionally returns (one of) its value(s)
 *
 * The return value indicates whether the item is present with a single value (@c CIF_OK), present with multiple
 * values (@c CIF_AMBIGUOUS_ITEM), or absent (@c CIF_NOSUCH_ITEM).  Being present in a zero-packet loop is treated
 * the same as being absent altogether.  If there are any values for the item and the @c val parameter is non-NULL,
 * then one of the values is returned via @c val.  Name matching is performed via a caseless, Unicode-normalized
 * algorithm, and invalid names are handled as absent.
 *
 * @param[in] container a handle on the data block or save frame in which to look up the item; must be non-NULL and
 *         valid
 *
 * @param[in] item_name the name of the item to look up, as a NUL-terminated Unicode string; must not be NULL
 *
 * @param[in,out] val if non-NULL then the location where a pointer to (one of) the item's value(s) should be written.
 *         If a non-NULL pointer is already at the referenced location then its contents are cleaned as if by
 *         @c cif_value_clean() and then overwritten; otherwise a new value object is created and a pointer to it
 *         provided.
 *
 * @return Returns a result code as described above, or @c CIF_ERROR if an error occurs
 */
CIF_INTFUNC_DECL(cif_container_get_value, (
        cif_container_tp *container,
        const UChar *item_name,
        cif_value_tp **val
        ));

/**
 * @brief Sets the value of the specified item in the specified container, or adds it
 * as a scalar if it's not already present in the container.
 *
 * The given value is set for the item in every packet of the loop to which it belongs.  To set a value in just one
 * packet of a multi-packet loop, use a packet iterator.  Note that zero-packet loops are no exception: if an item
 * belongs to a zero-packet loop, then setting its value(s) via this method updates all zero packets for no net effect.
 *
 * @param[in] container a handle on the container to modify; must be non-NULL and valid
 *
 * @param[in] item_name the name of the item to set, as a NUL-terminated Unicode string; must be a valid item name
 *
 * @param[in] val a pointer to the value to set for the specified item; if NULL then a value of kind @c CIF_UNK_KIND
 *         is set
 *
 * @return @c CIF_OK on success, @c CIF_INVALID_ITEMNAME if the item name is invalid, or another error code (typically
 *         @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_set_value, (
        cif_container_tp *container,
        const UChar *item_name,
        cif_value_tp *val
        ));

/**
 * @brief Removes the specified item and all its values from the specified container.
 *
 * If the removed item was the last in its loop, then the whole loop is removed, invalidating any outstanding handle
 * on it.
 *
 * @param[in] container a handle on the container from which to remove the item
 *
 * @param[in] item_name the name of the item to set, as a NUL-terminated Unicode string; must not be NULL
 *
 * @return Returns @c CIF_OK if the item was successfully removed, @c CIF_NOSUCH_ITEM if it was not present in the
 *         container, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_container_remove_item, (
        cif_container_tp *container,
        const UChar *item_name
        ));

/**
 * @}
 *
 * @defgroup loop_funcs Functions for manipulating loops
 *
 * @{
 */

/**
 * @brief Releases any resources associated with the given loop handle without modifying the managed CIF with which
 *         it is associated.
 *
 * @param[in] loop the loop handle to free; must be non-NULL and valid
 */
CIF_VOIDFUNC_DECL(cif_loop_free, (
        cif_loop_tp *loop
        ));

/**
 * @brief Releases any resources associated with the given loop handle, and furthermore removes the entire associated
 *         loop from its managed CIF, including all items and values belonging to it.
 *
 * Provided that they are initially valid, the loop handle and any outstanding iterators over its contents are
 * invalidated by successful execution of this function.
 *
 * On failure, the loop handle is not freed, so that the caller can interrogate it for error reporting purposes.  The
 * caller remains responsible to release it, possibly via @c cif_loop_free().
 *
 * @param[in,out] loop a handle on the loop to destroy
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_loop_destroy, (
        cif_loop_tp *loop
        ));

/**
 * @brief Retrieves the specified loop's category.
 *
 * Responsibility for cleaning up the category string belongs to the caller.  Note that loops are not required to
 * have categories assigned, therefore the category returned may be a NULL pointer.
 *
 * @param[in] loop a handle on the loop whose item names are requested
 *
 * @param[in,out] category a pointer to the location where a pointer to a NUL-terminated Unicode string containing the
 *         requested category (or NULL) should be recorded; must not be NULL
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_loop_get_category, (
        cif_loop_tp *loop,
        UChar **category
        ));

/**
 * @brief Sets the specified loop's category.
 *
 * The zero-character string (as distinguished from NULL) is RESERVED as a loop category.  No loop's category may be
 * set to a zero-character string (unless that's what it already is), nor may a loop's category be changed if it is
 * the zero-character string.
 *
 * @param[in,out] loop a handle on the loop whose category is to be set
 *
 * @param[in] category the category string to set as a NUL-terminated Unicode string; does not need to be unique and
 *         may be NULL, but must not be a zero-length string.  Ownership of this string is not transferred by passing
 *         it to this function.
 *
 * @return @c CIF_OK on success, or an error code on failure, typically either @c CIF_RESERVED_LOOP or @c CIF_ERROR
 */
CIF_INTFUNC_DECL(cif_loop_set_category, (
        cif_loop_tp *loop,
        const UChar *category
        ));

/**
 * @brief Retrieves the item names belonging to the specified loop.
 *
 * The resulting name list takes the form of a NULL-terminated array of NUL-terminated Unicode strings.  The caller
 * assumes responsibility for freeing the individual names and the array containing them.
 *
 * @param[in] loop a handle on the loop whose item names are requested
 *
 * @param[in,out] item_names the location where a pointer to the resulting NULL-terminated item name array should be
 *         written; must not be NULL
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_loop_get_names, (
        cif_loop_tp *loop,
        UChar ***item_names
        ));

/**
 * @brief Adds the CIF data item identified by the specified name to the specified managed loop, with the given
 *         initial value in every existing loop packet.
 *
 * The caller retains ownership of all arguments.
 *
 * Note that there is an asymmetry here, in that items are added to specific @em loops, but removed from 
 * overall @em containers (data blocks and save frames).  This reflects the CIF architectural asymmetry that item
 * names must be unique within their entire container, yet within it they each belong exclusively to a specific loop.
 *
 * @param[in] loop a handle on the loop to which the item should be added
 *
 * @param[in] item_name the name of the item to add as a NUL-terminated Unicode string; must be non-NULL and a valid
 *         CIF item name
 *
 * @param[in] val a pointer to the value object that serves as the the initial value for the item, or NULL to use a
 *         CIF unknown value
 *
 * @return Returns @c CIF_OK on success or a characteristic error code on failure, normally one of:
 *         @li @c CIF_INVALID_ITEMNAME if the item name is not valid
 *         @li @c CIF_DUP_ITEMNAME if the specified item name is already present in the loop's container
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_loop_add_item, (
        cif_loop_tp *loop,
        const UChar *item_name,
        cif_value_tp *val
        ));

/* items are removed directly from containers, not from loops */

/**
 * @brief Adds a packet to the specified loop.
 *
 * The packet object is not required to provide data for all items in the loop (items without specified values get the
 * explicit unknown value), but it must specify at least one value, and it must not provide data for any items not
 * in the loop.  The caller retains ownership of the packet abd its contents.
 *
 * @param[in] loop a handle on the loop to which to add a packet; mist be non-NULL and valid
 *
 * @param[in] packet a pointer to the packet object specifying the values for the new packet; must be non-NULL,
 *         specifying at least one item value, and not specifying any that do not belong to the designated loop
 *
 * @return Returns @c CIF_OK on success, or a characteristic error code on failure, normally one of:
 *         @li @c CIF_INVALID_PACKET if the packet contains no items
 *         @li @c CIF_WRONG_LOOP if the packet specifies any items that do not belong to the target loop
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_loop_add_packet, (
        cif_loop_tp *loop,
        cif_packet_tp *packet
        ));

/**
 * @brief Creates an iterator over the packets in the specified loop.
 *
 * After successful return and before the iterator is closed or aborted, behavior is undefined if the underlying
 * loop is accessed (even just for reading) other than via the iterator.  Furthermore, other modifications to the
 * same managed CIF made while an iterator is active may be sensitive to the iterator, in that it is undefined
 * whether they will be retained if the iterator is aborted or closed unsuccessfully.  As a result of these constraints,
 * users are advised to maintain as few active iterators as feasible for any given CIF, to minimize non-iterator
 * operations performed while any iterator is active on the same target CIF, and to close or abort iterators as
 * soon as possible after they cease to be required.
 *
 * @param[in] loop a handle on the loop whose packets are requested; must be non-NULL and valid
 *
 * @param[in,out] iterator the location where a pointer to the iterator object should be written; must not be NULL
 *
 * @return @c CIF_OK on success or an error code on failure, normally one of:
 *         @li @c CIF_INVALID_HANDLE if the loop handle represents a loop that does not (any longer) exist;
 *         @li @c CIF_EMPTY_LOOP if the target loop contains no packets; or
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_loop_get_packets, (
        cif_loop_tp *loop,
        cif_pktitr_tp **iterator
        ));

/**
 * @}
 *
 * @defgroup pktitr_funcs Functions for (loop) packet iteration
 *
 * @{
 *
 * A packet iterator, obtained via @c cif_loop_get_packets(), represents the current state of a particular iteration
 * of the packets of a particular loop (of a particular container).  Based on that state, the user controls the
 * progression of the iteration and optionally accesses the data and / or modifies the underlying loop.
 *
 * The life cycle of an iterator contains several states:
 * @li @b NEW - The initial state of every iterator.  An iterator remains in this state until it is destroyed via
 *         @c cif_pktitr_close() or @c cif_pktitr_abort(), or it is moved to the @b ITERATED state (or, under special
 *         circumstances, directly to the @b FINISHED state; see below) via @c cif_pktitr_next_packet(); no other
 *         operations are valid for a packet iterator in this state.
 * @li @b ITERATED - The state of an iterator that has a valid previous packet.  An iterator enters this state
 *         via @c cif_pktitr_next_packet() (when that function returns @c CIF_OK), and leaves it via
 *         @c cif_pktitr_remove_packet() [to the @b REMOVED state] or via @c cif_pktitr_next_packet() [to the
 *         @b FINISHED state] when that function returns @c CIF_FINISHED.  All packet iterator functions are valid
 *         for an iterator in this state; some leave it in this state.
 * @li @b REMOVED - The state of an iterator after its most recently iterated packet is removed via
 *         @c cif_pktitr_remove_packet().  From a forward-looking perspective, this state is nearly equivalent
 *         to the @b NEW state; the primary difference is that it is not extraordinary for an iterator to
 *         proceed directly from this state to the @b FINISHED state.
 * @li @b FINISHED - The state of an iterator after @c cif_pktitr_next_packet() returns @c CIF_FINISHED to signal
 *         that no more packets are available.  The only valid operations on such an iterator are to destroy it via
 *         @c cif_pktitr_close() or @c cif_pktitr_abort(), and one of those operations should be performed in a timely
 *         manner.
 *
 * The CIF data model does not accommodate loops without any packets, but the CIF API definition requires such
 * loops to be accommodated in memory, at least transiently.  It is for such packetless loops alone that
 * a packet iterator can proceed directly from @b NEW to @b FINISHED.
 *
 * Inasmuch as CIF loops are only incidentally ordered, the sequence in which loop packets are presented by an iterator
 * is not defined by these specifications.
 *
 * While iteration of a given loop is underway, it is implementation-defined what effect, if any, modifications
 * to that loop other than by the iterator itself (including by a different iterator) have on the iteration results.
 *
 * An open packet iterator potentially places both hard and soft constraints on use of the CIF with which it is
 * associated.  In particular, open iterators may interfere with other accesses to the same underlying data -- even
 * for reading -- and it is undefined how closing or aborting an iterator affects other open iterators.  When multiple
 * iterators must be held open at the same time, it is safest (but not guaranteed safe) to close / abort them in
 * reverse order of their creation.  Additionally, and secondarily to the preceding, packet iterators should be closed
 * or aborted as soon as they cease to be needed.  More specific guidance may be associated with particular API
 * implementations.
 */

/**
 * @brief Finalizes any changes applied via the specified iterator and releases any resources associated with it.  The
 *         iterator ceases to be active.
 *
 * Failure of this function means that changes applied by the iterator could not be made permanent; its resources
 * are released regardless, and it ceases being active.
 *
 * @param[in,out] iterator a pointer to the iterator to close; must be a non-NULL pointer to an active iterator object
 *
 * @return @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_pktitr_close, (
        cif_pktitr_tp *iterator
        ));

/**
 * @brief If possible, reverts any changes applied via the provided iterator to its CIF, and, in all cases, releases any
 *         resources associated with the iterator itself.
 *
 * Whether changes can be reverted or rolled back depends on the library implementation.
 *
 * @param[in,out] iterator a pointer to the packet iterator object to abort; must be a non-NULL pointer to an active
 *         iterator
 *
 * @return Returns @c CIF_OK on success, @c CIF_NOT_SUPPORTED if aborting is not supported, or an error code (typically
 *         @c CIF_ERROR ) in all other cases
 */
CIF_INTFUNC_DECL(cif_pktitr_abort, (
        cif_pktitr_tp *iterator
        ));

/**
 * @brief Advances a packet iterator to the next packet, if any, and optionally returns the contents of that packet.
 *
 * If @c packet is not NULL then the packet data are recorded where it points, either replacing the contents of a
 * packet provided that way by the caller (when @c *packet is not NULL) or providing a new packet to the caller.
 * "Replacing the contents" includes removing items that do not belong to the iterated loop.
 * A new packet provided to the caller in this way (when @p packet is not NULL and @p *packet is NULL on call) becomes
 * the responsibility of the caller; when no longer needed, its resources should be released via @c cif_packet_free().
 *
 * If no more packets are available from the iterator's loop then @c CIF_FINISHED is returned.
 *
 * @param[in,out] iterator a pointer to the packet iterator from which the next packet is requested
 *
 * @param[in,out] packet if non-NULL, the location where the contents of the next packet should be provided, either
 *         as the address of a new packet, or by replacing the contents of an existing one whose address is already
 *         recorded there.
 *
 * @return On success returns @c CIF_OK if the iterator successfully advanced, @c CIF_FINISHED if there were no
 *         more packets available.  Returns an error code on failure (typically @c CIF_ERROR ), in which case the
 *         contents of any pre-existing packet provided to the function are undefined (but valid)
 */
CIF_INTFUNC_DECL(cif_pktitr_next_packet, (
        cif_pktitr_tp *iterator,
        cif_packet_tp **packet
        ));

/**
 * @brief Updates the last packet iterated by the specified iterator with the values from the provided packet.
 *
 * It is an error to pass an iterator to this function for which @c cif_pktitr_next_packet() has not been called
 * since it was obtained or since it was most recently passed to @c cif_pktitr_remove_packet(), or to pass one for
 * which cif_pktitr_next_packet() has returned @c CIF_FINISHED. Items in the iterator's target loop for which the
 * packet does not provide a value are left unchanged.  It is an error for the provided packet to provide a value for
 * an item not included in the iterator's loop.
 *
 * @param[in] iterator a pointer to the packet iterator defining the loop and packet to update; must be a non-NULL
 *         pointer to an active packet iterator
 *
 * @param[in] packet a pointer to a packet object defining the updates to apply; must not be NULL
 *
 * @return @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one of:
 *         @li @c CIF_MISUSE if the iterator has no current packet (because it has not yet provided one, or because the
 *              one most recently provided has been removed
 *         @li @c CIF_WRONG_LOOP if the packet provides an item that does not belong to the iterator's loop
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_pktitr_update_packet, (
        cif_pktitr_tp *iterator,
        cif_packet_tp *packet
        ));

/**
 * @brief Removes the last packet iterated by the specified iterator from its managed loop.
 *
 * It is an error to pass an iterator to this function for which @c cif_pktitr_next_packet() has not been called
 * since it was obtained or since it was most recently passed to @c cif_pktitr_remove_packet().
 *
 * @param[in,out] iterator a pointer to the packet iterator object whose most recently provided packet is to be removed
 *
 * @return Returns @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one
 *         of:
 *         @li @c CIF_MISUSE if the iterator has no current packet (because it has not yet provided one, or because the
 *              one most recently provided has been removed via this function
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_pktitr_remove_packet, (
        cif_pktitr_tp *iterator
        ));

/**
 * @}
 *
 * @defgroup packet_funcs Functions for manipulating packets
 *
 * @{
 */

/**
 * @brief Creates a new packet for the given item names
 *
 * The resulting packet does not retain any references to the item name strings provided by the caller.  It will
 * contain for each data name a value object representing the explicit "unknown" value.  The caller is responsible for
 * freeing the packet via @c cif_packet_free() when it is no longer needed.
 * 
 * @param[in,out] packet the location were the address of the new packet should be recorded.  Must not be NULL.  The
 *         value initially at @p *packet is ignored.
 *
 * @param[in] names a pointer to a NULL-terminated array of Unicode string pointers, each element containing one data
 *         name to be represented in the new packet; may contain zero names.  The responsibility for these resources
 *         is not transferred.  May be NULL, which is equivalent to containing zero names.
 *
 * @return Returns @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one
 *         of:
 *         @li @c CIF_INVALID_ITEMNAME if one of the provided strings is not a valid CIF data name
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_packet_create, (
        cif_packet_tp **packet, 
        UChar *names[]
        ));

/**
 * @brief Retrieves the names of all the items in the current packet.
 *
 * It is the caller's responsibility to release the resulting array, but its elements @b MUST @b NOT be modified or
 * freed.  Names are initially in the order in which they appear in the array passed to @c cif_packate_create().
 * If a @em new item is added via @c cif_packet_set_item() then its name is appended to the packet's name list.
 *
 * @param[in] packet a pointer to the packet whose data names are requested
 *
 * @param[in,out] names  the location where a pointer to the array of names should be written.  The array will be
 *         NULL-terminated.
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_packet_get_names, (
        cif_packet_tp *packet,
        const UChar ***names
        ));

/**
 * @brief Sets the value of the specified CIF item in the specified packet, including if the packet did not previously
 *         provide any value for the specified data name.
 *
 * The provided value object is copied into the packet; the packet does not assume responsibility for either the
 * specified value object or the specified name.  If the value handle is @c NULL then an explicit unknown-value object
 * is recorded.  The copied (or created) value can subsequently be obtained via @c cif_packet_get_item().
 *
 * When the specified name matches an item already present in the packet, the existing value is first cleaned as if
 * by @c cif_value_clean(), then the new value is copied onto it.  That will be visible to code that holds a reference
 * to the value (obtained via @c cif_packet_get_item()).  Exception: if @p value is the same object as the
 * current internal one for the given name then this function succeeds without changing anything.
 *
 * @param[in,out] packet a pointer to the packet to modify
 *
 * @param[in] name the name of the item within the packet to modify, as a NUL-terminated Unicode string; must be
 *         non-null and valid for a CIF data name
 *
 * @param[in] value the value object to copy into the packet, or @c NULL for an unknown-value value object to be added
 *
 * @return Returns @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one
 *         of:
 *         @li @c CIF_INVALID_ITEMNAME if @p name is not a valid CIF data name
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_packet_set_item, (
        cif_packet_tp *packet, 
        const UChar *name, 
        cif_value_tp *value
        ));

/**
 * @brief Determines whether a packet contains a value for a specified data name, and optionally provides that value.
 *
 * The value, if any, is returned via parameter @p value, provided that that parameter is not NULL.  In contrast to
 * @c cif_packet_set_item(), the value is not copied out; rather a pointer to the packet's own internal object is
 * provided.  Therefore, callers must not free the value object themselves, but they may freely @em modify it, and such
 * modifications will subsequently be visible via the packet.
 *
 * @param[in] packet a pointer to the packet object from which to retrieve a value
 *
 * @param[in] name the data name requested from the packet
 *
 * @param[in,out] value if not NULL, gives the location where a pointer to the requested value should be written.
 *
 * @return Returns @c CIF_OK if the packet contains an item having the specified data name, or @c CIF_NOSUCH_ITEM
 *         otherwise.
 */
CIF_INTFUNC_DECL(cif_packet_get_item, (
        cif_packet_tp *packet, 
        const UChar *name, 
        cif_value_tp **value
        ));

/**
 * @brief Removes the value, if any, associated with the specified name in the specified packet, optionally returning
 *         it to the caller.
 *
 * The value object is returned via the @p value parameter if that is not NULL, in which case the caller assumes
 * responsibility for the value.  Otherwise the removed value and all resources belonging to it are released.
 *
 * @param[in,out] packet a pointer to the packet from which to remove an item.
 *
 * @param[in] name the name of the item to remove, as a NUL-terminated Unicode string.
 *
 * @param[in,out] value if not NULL, the location where a pointer to the removed item should be written
 *
 * @return Returns @c CIF_OK if the packet initially contained an item having the given name, or @c CIF_NOSUCH_ITEM
 *         otherwise; the case where the given name is not a valid CIF item name is not distinguished as a separate case.
 */
CIF_INTFUNC_DECL(cif_packet_remove_item, (
        cif_packet_tp *packet, 
        const UChar *name, 
        cif_value_tp **value
        ));

/**
 * @brief Releases the specified packet and all resources associated with it, including those associated with any
 * values within.
 *
 * This function is a safe no-op when the argument is NULL.
 *
 * @param[in] packet a pointer to the packet to free
 */
CIF_VOIDFUNC_DECL(cif_packet_free, (
        cif_packet_tp *packet
        ));

/**
 * @}
 *
 * @defgroup value_funcs Functions for manipulating value objects
 *
 * @{
 * CIF data values are represented by and to CIF API functions via the opaque data type @c cif_value_tp .  Because this
 * type is truly opaque, @c cif_value_tp objects cannot directly be declared.  Independent value objects must instead
 * be created via function @c cif_value_create() , and @em independent ones should be released via @c cif_value_free()
 * when they are no longer needed.  Unlike the "handles" on CIF structural components that are used in several other
 * parts of the API, @c cif_value_tp objects are complete data objects, independent from the backing CIF storage
 * mechanism, albeit sometimes parts of larger aggregate value objects.
 *
 * The API classifies values into several distinct "kinds": character (Unicode string) values, apparently-numeric
 * values, list values, table values, unknown-value place holders, and not-applicable/default-value place holders.
 * These alternatives are represented by the enumeration @c cif_kind_tp.  Value kinds are assigned when values are
 * created, but may be changed by re-initialization.  Several functions serve this purpose: @c cif_value_init(), of
 * course, but also @c cif_value_init_char(), @c cif_value_copy_char(), @c cif_value_parse_numb(),
 * @c cif_value_init_numb(), and @c cif_value_autoinit_numb().  If it is unknown, the kind of a value object can be
 * determined via @c cif_value_kind(); this is important, because many of the value manipulation functions are
 * useful only for values of certain kinds.
 *
 * Values of list and table kind aggregate other value objects.  The aggregates do not accept independent value objects
 * directly into themselves; instead they make copies of values entered into them so as to reduce confusion
 * about object ownership.  Such internal copies can still be exposed outside their container objects, however, to
 * reduce copying and facilitate value manipulation.  Value objects that belong to a list or table aggregate are
 * @em dependent on their container object, and must not be directly freed.
 *
 * Table values associate other values with Unicode string keys.  Unlike CIF data values and keywords, table keys are
 * not "case insensitive".  They do, however, take Unicode canonical equivalence into account, thus Unicode strings
 * containing different sequences of characters and yet canonically equivalent to each other can be used
 * interchangeably as table keys.  When keys are enumerated via
 * @c cif_value_get_keys(), the form of the key most recently used to enter a value into the table is the form
 * provided.  Table keys may contain whitespace, including leading and trailing whitespace, which is significant.
 * The Unicode string consisting of zero characters is a valid table key.
 */

/**
 * @brief Allocates and initializes a new value object of the specified kind.
 *
 * The new object is initialized with a kind-dependent default value:
 * @li an empty string for @c CIF_CHAR_KIND
 * @li an exact zero for @c CIF_NUMB_KIND
 * @li an empty list or table for @c CIF_LIST_KIND or @c CIF_TABLE_KIND, respectively
 * @li (there is only one possible distinct value each for @c CIF_UNK_KIND and @c CIF_NA_KIND)
 * 
 * For @c CIF_CHAR_KIND and @c CIF_NUMB_KIND, however, it is somewhat more efficient to create a value object of kind
 * @c CIF_UNK_KIND and then initialize it with the appropriate kind-specific function.
 *
 * @param[in] kind the value kind to create
 *
 * @param[in,out] value the location where a pointer to the new value should be recorded; must not be NULL.  The
 *         initial value of @p *value is ignored.
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_value_create, (
        cif_kind_tp kind,
        cif_value_tp **value
        ));

/**
 * @brief Frees any resources associated with the provided value object without freeing the object itself,
 *         changing it into an instance of the explicit unknown value.
 *
 * This function's primary uses are internal, but it is available for anyone to use.  External code more often
 * wants @c cif_value_free() instead.  This function does not directly affect any managed CIF.
 *
 * @param[in,out] value a valid pointer to the value object to clean
 *
 * @return Returns @c CIF_OK on success.  In principle, an error code such as @c CIF_ERROR is returned on failure, but
 *         no specific failure conditions are currently defined.
 */
CIF_VOIDFUNC_DECL(cif_value_clean, (
        cif_value_tp *value
        ));

/**
 * @brief Frees the specified value object and all resources associated with it.
 *
 * This provides only resource management; it has no effect on any managed CIF.  This function is a safe no-op
 * when its argument is NULL.
 *
 * @param[in,out] value a valid pointer to the value object to free, or NULL
 */
CIF_VOIDFUNC_DECL(cif_value_free, (
        cif_value_tp *value
        ));

/**
 * @brief Creates an independent copy of a CIF value object.
 *
 * For composite values (LIST and TABLE), the result is a deep clone -- all members of the original value are
 * themselves cloned, recursively, to create the members of the cloned result.  If the referent of the @p clone
 * parameter is not NULL, then the object it points to is first cleaned up as if by @c cif_value_clean(), then
 * overwritten with the cloned data; otherwise, a new value object is allocated and a pointer to it is written where
 * @p clone points.
 *
 * @param[in] value a pointer to the value object to clone
 *
 * @param[in,out] clone the location where a pointer to the cloned value should be made to reside, either by replacing
 *         the contents of an existing value object or by recording a pointer to a new one; must not be NULL.  If
 *         @p *clone is not NULL then it must point to a valid cif value object.
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_value_clone, (
        cif_value_tp *value,
        cif_value_tp **clone
        ));

/**
 * @brief Reinitializes the provided value object to a default value of the specified kind.
 *
 * The value referenced by the provided handle should have been allocated via @c cif_value_create().  Any unneeded
 * resources it holds are released.  The specified value kind does not need to be the same as the value's present
 * kind.  Kind-specific default values are documented as with @c cif_value_create().
 *
 * This function is equivalent to @c cif_value_clean() when the specified kind is @c CIF_UNK_KIND, and it is less useful
 * than the character- and number-specific (re)initialization functions.  It is the only means available to change an
 * existing value in-place to a list, table, or N/A value, however, and it can be used to efficently empty a list or
 * table value.
 *
 * On failure, the value is left in a valid but otherwise unspecified state.
 *
 * @param[in,out] value a handle on the value to reinitialize; must not be @c NULL
 * @param[in] kind the cif value kind as which the value should be reinitialized
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_value_init, (
        cif_value_tp *value,
        cif_kind_tp kind
        ));

/**
 * @brief (Re)initializes the specified value object as being of kind @c CIF_CHAR_KIND, with the specified text
 *
 * Any previous contents of the provided value object are first cleaned as if by @c cif_value_clean().
 * <strong><em>Responsibility for the provided string passes to the value object</em></strong>; it should not
 * subsequently be managed directly by the caller.  This implies that the text must be dynamically allocated.  The
 * value object will become sensitive to text changes performed afterward via the @c text pointer.  This behavior
 * could be described as wrapping an existing Unicode string in a CIF value object.
 *
 * @param[in,out] value a pointer to the value object to be initialized; must not be NULL
 *
 * @param[in] text the new text content for the specified value object; must not be NULL
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 *
 * @sa cif_value_copy_char()
 */
CIF_INTFUNC_DECL(cif_value_init_char, (
        cif_value_tp *value,
        UChar *text
        ));

/**
 * @brief (Re)initializes the specified value object as being of kind CIF_CHAR_KIND, with a copy of the specified text.
 *
 * This function performs the same job as @c cif_value_init_char(), except that it makes a copy of the value text.
 * <strong><em>Responsibility for the @c text argument does not change</em></strong>, and the value object does not
 * become sensitive to change via the @c text pointer.  Unlike @c cif_value_init_char(), this function is thus suitable
 * for use with initialization text that resides on the stack (e.g. in a local array variable) or in read-only memory.
 *
 * @param[in,out] value a pointer to the value object to be initialized; must not be NULL
 *
 * @param[in] text the new text content for the specified value object; must not be NULL
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 *
 * @sa cif_value_init_char()
 */
CIF_INTFUNC_DECL(cif_value_copy_char, (
        cif_value_tp *value,
        const UChar *text
        ));

/**
 * @brief Parses the specified Unicode string to produce a floating-point number and its standard uncertainty,
 *         recording them in the provided value object.
 *
 * For success, the entire input string must be successfully parsed.  On success (only), any previous contents
 * of the value object are released as if by @c cif_value_clean() ; otherwise, the value object is not modified.
 * <strong><em>The value object takes ownership of the parsed text on success.</em></strong>  The caller is responsible
 * for ensuring that the @p text pointer is not afterward used to modify its referrent.  The representable values
 * that can be initialized in this manner are not limited to those representable the machine's native
 * floating-point format.
 *
 * This behavior could be described as wrapping an existing Unicode string in a (numeric) CIF value object.
 *
 * @param[in,out] numb the value object into which to parse the text
 *
 * @param[in] text the text to parse as a NUL-terminated Unicode string, in one of the forms documented forms
 *         documented for numbers in CIF text
 *
 * @return Returns @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one
 *         of:
 *         @li @c CIF_INVALID_NUMBER if the text cannot be fully parsed as a number
 *         @li @c CIF_ERROR in most other cases
 */
CIF_INTFUNC_DECL(cif_value_parse_numb, (
        cif_value_tp *numb, 
        UChar *text
        ));

/**
 * @brief (Re)initializes the given value as a number with the specified value and uncertainty, with a number of
 *     significant figures determined by the specified @p scale
 *
 * Any previous contents of the provided value are released, as if by @c cif_value_clean() .
 * 
 * The value's text representation will be formatted with a standard uncertainty if and only if the given standard
 * uncertainty (@p su ) is greater than zero.  It is an error for the uncertainty to be less than zero, but it may
 * be exactly equal to zero.
 *
 * The @p scale gives the number of significant digits to the right of the units digit; the value and uncertainty
 * will be rounded or extended to this number of decimal places as needed.  Any rounding is performed according to the
 * floating-point rounding mode in effect at that time.  The scale may be less than zero, indicating that the units
 * digit and possibly some of the digits to its left are insignificant and not to be recorded.  If the given su rounds
 * to exactly zero at the specified scale, then the resulting number object represents an exact number, whether the su
 * given was itself exactly zero or not.
 *
 * If the scale is greater than zero or if pure decimal notation would require more than @p max_leading_zeroes
 * leading zeroes after the decimal point, then the number's text representation is formatted in scientific notation
 * (d.ddde+-dd; the number of digits of mantissa and exponent may vary).  Otherwise, it is formatted in decimal
 * notation.
 *
 * It is an error if a text representation consistent with the arguments would require more characters than the
 * CIF 2.0 maximum line length (2048 characters).
 *
 * The behavior of this function is constrained by the characteristics of the data type (@c double ) of the 
 * @p value and @p su parameters.  Behavior is undefined if a scale is specified that exceeds the maximum possible
 * precision of a value of type double, or such that @c 10^(-scale) is greater than the greatest representable finite
 * double.  Behavior is undefined if @p val or @p su has a value that does not correspond to a finite real number (i.e.
 * if one of those values represents an infinite value or does not represent any number at all).
 *
 * @param[in,out] numb a pointer to the value object to initialize
 *
 * @param[in] val the numeric value to assign to the number
 *
 * @param[in] su the absolute numeric standard uncertainty in the specified value
 *
 * @param[in] scale the number of significant digits to the right of the decimal point in both @p val and @p su
 *
 * @param[in] max_leading_zeroes the maximimum number of leading zeroes with which the text representation of numbers
 *         having absolute value less than 1 will be formatted; must not be less than zero.
 *
 * @return Returns CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_value_init_numb, (
        cif_value_tp *numb, 
        double val, 
        double su, 
        int scale, 
        int max_leading_zeroes
        ));

/**
 * @brief (Re)initializes the given value as a number with the specified value and uncertainty, automatically
 *         choosing the number of significant digits to keep based on the specified @p su and @p su_rule.
 *
 * Any previous contents of the provided value are released, as if by @c cif_value_clean().
 * 
 * The number's text representation will be formatted with a standard uncertainty if and only if the given su is
 * greater than zero.  In that case, the provided su_rule governs the number of decimal digits with which the value
 * text is formatted, and the number of digits of value and su that are recorded.  The largest scale is chosen such
 * that the significant digits of the rounded su, interpreted as an integer, are less than or equal to the su_rule.
 * The most commonly used su_rule is probably 19, but others are used as well, including at least 9, 27, 28, 29, and 99.
 * The su_rule must be at least 2, and behavior is undefined if its decimal representation has more significant digits
 * than the implementation-defined maximum decimal precision of a double.  If su_rule is less than 6 then some standard
 * uncertainties may be foratted as (0) in the value's text representation.
 * 
 * If the @p su is exactly zero then all significant digits are recorded to represent the exact value of @p val.  The
 * @p su_rule is ignored in this case, and no su is included in the text representation.
 * 
 * The value's text representation is expressed in scientific notation if it would otherwise have more than five
 * leading zeroes or any trailing insignificant zeroes.  Otherwise, it is expressed in plain decimal notation.
 *
 * Behavior is undefined if @p val or @p su has a value that does not correspond to a finite real number (i.e.
 * if one of those values represents an infinite value or does not represent any number at all).
 *
 * @param[in,out] numb a pointer to the number value object to initialize
 *
 * @param[in] val the numeric value to assign to the number
 *
 * @param[in] su the absolute numeric standard uncertainty in the specified value; must not be negative
 *
 * @param[in] su_rule governs rounding of the value and uncertainty; it is a "rule" in the sense that the rounding
 *         conventions it governs are sometimes referred to by names of form "rule of 19"
 *
 * @return Returns CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure.
 */
CIF_INTFUNC_DECL(cif_value_autoinit_numb, (
        cif_value_tp *numb, 
        double val, 
        double su, 
        unsigned int su_rule
        ));

/**
 * @brief Returns the kind code of the specified value.
 *
 * Unlike most CIF functions, the return value is not an error code, as no error conditions are defined for or
 * detectable by this function.
 *
 * @param[in] value a pointer to the value object whose kind is requested; must point at an initialized value
 *
 * @return Returns the kind code of the specified value
 */
CIF_FUNC_DECL(cif_kind_tp, cif_value_kind, (
        cif_value_tp *value
        ));

/**
 * @brief Returns the value represented by the given number value object.
 *
 * Behavior is implementation-defined if the represented numeric value is outside the representable range of type
 * @c double .  Some of the possible behaviors include raising a floating-point trap and returning a special value,
 * such as an IEEE infinity.
 *
 * @param[in] numb a pointer the the value object whose numeric value is requested
 * @param[in,out] val a pointer to the location where the numeric value should be recorded; must be a valid pointer
 *         to suitably-aligned storage large enough to accommodate a @c double.
 *
 * @return Returns @c CIF_OK on success, @c CIF_ARGUMENT_ERROR if the provided object is not of numeric kind, or
 *         another code, typically @c CIF_ERROR, if an error occurs.
 */
CIF_INTFUNC_DECL(cif_value_get_number, (
        cif_value_tp *numb,
        double *val
        ));

/**
 * @brief Returns the uncertainty of the value represented by the given number value object.
 *
 * The uncertainty is zero for an exact number.  Behavior is implementation-defined if the uncertainty is outside the
 * representable range of type @c double .
 *
 * @param[in] numb a pointer the the value object whose numeric standard uncertainty is requested
 * @param[in,out] su a pointer to the location where the uncertainty should be recorded; must be a valid pointer
 *         to suitably aligned storage large enough to accommodate a @c double.
 *
 * @return Returns @c CIF_OK on success, @c CIF_ARGUMENT_ERROR if the provided object is not of numeric kind, or
 *         another code, typically @c CIF_ERROR, if an error occurs.
 */
CIF_INTFUNC_DECL(cif_value_get_su, (
        cif_value_tp *numb,
        double *su
        ));

/**
 * @brief Retrieves the text representation of the specified value.
 *
 * It is important to understand that the "text representation" provided by this function is not the same as the
 * CIF format representation in which the same value might be serialized to a CIF file.  Instead, it is the @em parsed
 * text representation, so, among other things, it omits any CIF delimiters and might not adhere to CIF line length
 * limits.
 * 
 * The value text, if any, obtained via this function belongs to then caller.
 *
 * This function is natural for values of kind @c CIF_CHAR_KIND.  It is also fairly natural for values of kind
 * @c CIF_NUMB_KIND, as those carry a text representation with them to allow for undelimited number-like values
 * that are intended to be interpreted as text.  It is natural, but trivial, for kinds @c CIF_NA_KIND
 * and @c CIF_UNK_KIND, for which the text representation is NULL (as opposed to empty).  It is decidedly unnatural,
 * however, for the composite value kinds @c CIF_LIST_KIND and @c CIF_TABLE_KIND.  As such, the text for values of
 * those kinds is also NULL, though that is subject to change in the future.
 *
 * @param[in] value a pointer to the value object whose text is requested
 *
 * @param[in,out] text the location where a pointer to copy of the value text (or NULL, as appropriate) should be
 *         recorded; must not be NULL
 *
 * @return Returns @c CIF_OK on success, or @c CIF_ERROR on failure
 */
CIF_INTFUNC_DECL(cif_value_get_text, (
        cif_value_tp *value,
        UChar **text
        ));

/**
 * @brief Determines the number of elements of a composite data value.
 *
 * Composite values are those having kind @c CIF_LIST_KIND or @c CIF_TABLE_KIND .  For the purposes of this function,
 * the "elements" of a table are its key/value @em pairs .  Only the direct elements of a list or table are counted,
 * not the elements of any lists or tables nested within.
 *
 * @param[in] value a pointer to the value object whose elements are to be counted; must not be NULL
 *
 * @param[out] count a pointer to a location where the number of elements should be recorded
 *
 * @return @c CIF_OK if the value has kind @c CIF_LIST_KIND or @c CIF_TABLE_KIND, otherwise @c CIF_ARGUMENT_ERROR
 */
CIF_INTFUNC_DECL(cif_value_get_element_count, (
        cif_value_tp *value,
        size_t *count
        ));

/**
 * @brief Retrieves an element of a list value by its index.
 *
 * <em>A pointer to the actual value object contained in the list is returned, so modifications to the value will be
 * reflected in the list.</em>  If that is not desired then the caller can make a copy via @c cif_value_clone() .  The
 * caller must not free the returned value, but may modify it in any other way.
 *
 * It is unsafe for the caller to retain the provided element pointer if and when the list that owns the element ceases
 * to be under its exclusive control, for the lifetime of the referenced value object is then uncertain.  It will not
 * outlive the list that owns it, but various operations on the list may cause it to be discarded while the list is
 * still live.  Note in particular that a context switch to another thread that has access to the list object
 * constitutes passing out of the caller's control.  If multiple threads have unsynchronized concurrent access to
 * the list then no element retrieved from it via this function is @em ever safe to use, not even to clone it.
 *
 * @param[in] value a pointer to the @c CIF_LIST_KIND value from which to retrieve an element
 *
 * @param[in] index the zero-based index of the requested element; must be less than the number of elements in the list
 *
 * @param[out] element the location where a pointer to the requested value should be written; must not be NULL
 *
 * @return Returns @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_LIST_KIND; otherwise returns
 *     @c CIF_OK if @c index is less than the number of elements in the list, else @c CIF_INVALID_INDEX .
 */
CIF_INTFUNC_DECL(cif_value_get_element_at, (
        cif_value_tp *value,
        size_t index,
        cif_value_tp **element
        ));

/**
 * @brief Replaces an existing element of a list value with a different value.
 *
 * The provided value is @em copied into the list (if not NULL); responsibility for the original object is not
 * transferred.  The replaced value is first cleaned as if by @c cif_value_clean(), then the new value is copied onto
 * it.  That will be visible to code that holds a reference to the value (obtained via @c cif_value_get_element_at()).
 * 
 * Special case: if the replacement value is the same object as the one currently at the specified position in the
 * list, then this function succeeds without changing anything.
 *
 * @param[in,out] value a pointer to the @c CIF_LIST_KIND value to modify
 *
 * @param[in] index the zero-based index of the element to replace; must be less than the number of elements in the
 *         list
 *
 * @param[in] element a pointer to the replacement value, or @c NULL to set a @c CIF_UNK_KIND value
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_LIST_KIND , otherwise
 *         @li @c CIF_INVALID_INDEX if the index is greater than or equal to the number of list elements, or
 *         @li @c CIF_OK if a value was successfully set, or
 *         @li an error code, typically @c CIF_ERROR , if setting the value fails for reasons other than those already
 *             described
 *
 * @sa cif_value_insert_element_at()
 * @sa cif_value_set_item_by_key()
 */
CIF_INTFUNC_DECL(cif_value_set_element_at, (
        cif_value_tp *value,
        size_t index,
        cif_value_tp *element
        ));

/**
 * @brief Inserts an element into the specified list value at the specified position, pushing back the elements
 *         (if any) initially at that and following positions.
 *
 * The provided value is @em copied into the list (if not NULL); responsibility for the original object is not
 * transferred.  The list may be extended by inserting at the index one past its last element.
 *
 * @param[in,out] value a pointer to the list value to modify
 *
 * @param[in] index the zero-based index of the element to replace; must be less than or equal to the number of
 *         elements in the list
 *
 * @param[in] element a pointer to the value to insert, or NULL to insert a @c CIF_UNK_KIND value
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_LIST_KIND , otherwise
 *         @li @c CIF_INVALID_INDEX if the index is greater than the initial number of list elements, or
 *         @li @c CIF_OK if a value was successfully inserted, or
 *         @li an error code, typically @c CIF_ERROR , if inserting the value fails for reasons other than those already
 *             described
 *
 * @sa cif_value_set_element_at()
 */
CIF_INTFUNC_DECL(cif_value_insert_element_at, (
        cif_value_tp *value,
        size_t index,
        cif_value_tp *element
        ));

/**
 * @brief Removes an existing element from a list value, optionally returning it to the caller.
 *
 * Any elements following the removed one in the list are moved forward to fill the gap.  The removed element is
 * returned via the @p element parameter if that parameter is not NULL; in that case the caller assumes responsibility
 * for freeing that value when it is no longer needed.  Otherwise, the value is freed by this function.
 *
 * @param[in,out] value the list value to modify
 *
 * @param[in] index the zero-based index of the element to remove; must be less than the initial number of elements
 *         in the list
 *
 * @param[in,out] element if not NULL, then a pointer to the removed element is written at this location
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_LIST_KIND , otherwise
 *         @li @c CIF_INVALID_INDEX if the index is greater than the initial number of list elements, else
 *         @li @c CIF_OK if a value was successfully inserted
 *         @li in principle, a general error code such as @c CIF_ERROR may be returned if removing the value fails for
 *             reasons other than those already described, but no such failure conditions are currently defined
 *
 * @sa cif_value_remove_item_by_key()
 */
CIF_INTFUNC_DECL(cif_value_remove_element_at, (
        cif_value_tp *value,
        size_t index,
        cif_value_tp **element
        ));

/**
 * @brief Retrieves an array of the keys of a table value.
 *
 * Although tables are insensitive to differences between canonically equivalent keys, this function always provides
 * keys in the form most recently entered into the table.
 *
 * The caller assumes responsibility for freeing the returned array itself, but not the keys in it.
 * <strong><em>The keys remain the responsibility of the table, and MUST NOT be freed or modified in any
 * way.</em></strong>  Each key is a NUL-terminated Unicode string, and the end of the list is marked by a NULL
 * element.
 *
 * @param[in] table a pointer to the table value whose keys are requested
 *
 * @param[out] keys the location where a pointer to the array of keys should be written; must not be NULL.
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @p table has kind different from @c CIF_TABLE_KIND , otherwise
 *         @li @c CIF_OK if the keys were successfully retrieved, or
 *         @li an error code, typically @c CIF_ERROR , if retrieving the keys fails for reasons other than those already
 *             described
 */
CIF_INTFUNC_DECL(cif_value_get_keys, (
        cif_value_tp *table,
        const UChar ***keys
        ));

/**
 * @brief Associates a specified value and key in the provided table value.
 *
 * The value and key are copied into the table; responsibility for the originals does not transfer.  If a value
 * was already associated with the given key then it is first cleaned as if by @c cif_value_clean(), then the new value
 * is copied onto it.  That will be visible to code that holds a reference to the value (obtained via @c
 * cif_value_get_item_by_key()).
 * 
 * Special case: if the item value is the same object as one currently associated with the given key in the table, then
 * this function succeeds without changing anything.
 *
 * @param[in,out] table a pointer to the table value to modify
 *
 * @param[in] key the key of the table entry to set or modify, as a NUL-terminated Unicode string; must meet CIF
 *         validity criteria
 *
 * @param[in] item a pointer to the value object to enter into the table, or @c NULL to enter a @c CIF_UNK_KIND value
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @p table has kind different from @c CIF_TABLE_KIND , otherwise
 *         @li @c CIF_INVALID_INDEX if the given key does not meet CIF validity criteria
 *         @li @c CIF_OK if the entry was successfully set / updated, or
 *         @li an error code, typically @c CIF_ERROR , if setting or updating the entry fails for reasons other than
 *             those already described
 *
 * @sa cif_value_set_element_at()
 */
CIF_INTFUNC_DECL(cif_value_set_item_by_key, (
        cif_value_tp *table, 
        const UChar *key, 
        cif_value_tp *item
        ));

/**
 * @brief Looks up a table entry by key and optionally returns the associated value.
 *
 * If the key is present in the table and the @p value argument is non-NULL, then a pointer to the value associated
 * with the key is written where @p value points.  Otherwise, this function simply indicates via its return value
 * whether the specified key is present in the table.  <em>Any value object provided via the @p value parameter
 * continues to be owned by the table.</em>  It must not be freed, and any modifications to it will be reflected
 * in the table.  If that is not desired then the caller can make a copy of the value via @c cif_value_clone() .
 *
 * @param[in] table a pointer to the table value in which to look up the specified key
 *
 * @param[in] key the key to look up, as a NUL-terminated Unicode string.
 *
 * @param[in,out] value if not NULL, the location where a pointer to the discovered value, if any, should be
 *         written
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_TABLE_KIND , otherwise
 *         @li @c CIF_OK if the key was found in the table or
 *         @li @c CIF_NOSUCH_ITEM if the key was not found.
 *         @li in principle, an error code such as @c CIF_ERROR is returned in the event of a failure other than those
 *             already described, but no such failure conditions are currently defined.
 *
 * @sa cif_value_get_element_at()
 */
CIF_INTFUNC_DECL(cif_value_get_item_by_key, (
        cif_value_tp *table, 
        const UChar *key, 
        cif_value_tp **value
        ));

/**
 * @brief Removes an item from a table value, optionally returning the value.
 *
 * If the key is present in the table and the @p value parameter is non-NULL, then a pointer to the value associated
 * with the key is written where @p value points, and the caller assumes responsibility for freeing the value when
 * it is no longer needed.  Otherwise, this function automatically frees the value object, if any, of the removed
 * entry.
 *
 * @param[in,out] table a pointer to the table value to modify
 *
 * @param[in] key the key of the entry to remove, as a NUL-terminated Unicode string
 *
 * @param[in,out] value if not NULL, the location where a pointer to the removed value, if any, should be written
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_TABLE_KIND , otherwise
 *         @li @c CIF_OK if the key was found in the table (and the entry removed) or
 *         @li @c CIF_NOSUCH_ITEM if the key was not found.
 *         @li in principle, an error code such as @c CIF_ERROR is returned in the event of a failure other than those
 *             already described, but no such failure conditions are currently defined.
 *
 * @sa cif_value_remove_element_at()
 */
CIF_INTFUNC_DECL(cif_value_remove_item_by_key, (
        cif_value_tp *table, 
        const UChar *key, 
        cif_value_tp **value
        ));

/**
 * @}
 *
 * @defgroup utility_funcs CIF utility functions
 *
 * @{
 */

/**
 * @brief Provides a string representation of the CIF API's version number
 *
 * Creates a character array, fills it with a string representation of the CIF API's version number, and records a
 * pointer to it where the argument points.  The caller is responsible for freeing the version string when it is no
 * longer needed.
 *
 * @param[in,out] version a pointer to location where a pointer to the version string should be recorded; must
 *         not be NULL
 *
 * @return @c CIF_OK on success, @c CIF_ARGUMENT_ERROR if @p version is @c NULL, or @c CIF_MEMORY_ERROR if space cannot
 *         be allocated for the version string
 */
CIF_INTFUNC_DECL(cif_get_api_version, (
        char **version
        ));

/**
 * @brief Creates and returns a duplicate of a Unicode string.
 *
 * It is sometimes useful to duplicate a Unicode string, but ICU does not provide an analog of @c strdup() for that
 * purpose.  The CIF API therefore provides its own, and makes it available for general use.
 *
 * Behavior is undefined if the argument is not terminated by a NUL (Unicode) character.
 *
 * @param[in] str the NUL-terminated Unicode string to duplicate; the caller retains ownership of this object
 *
 * @return Returns a pointer to the duplicate, or NULL on failure or if the argument is NULL.  Responsibility for the
 *         duplicate, if any, belongs to the caller.
 */
CIF_FUNC_DECL(UChar *, cif_u_strdup, (
        const UChar *str
        ));

/**
 * @brief Converts (the initial part of) the specified Unicode string to a case-folded normalized form.
 *
 * The normalized form is that obtained by converting to Unicode normalization form NFD, applying the Unicode
 * case-folding algorithm to the result (with default handling of Turkic dotless i), and renormalizing the case-folded
 * form to Unicode normalization form NFC.  The result string, if provided, becomes the responsibility of the caller.
 * If not NULL, it is guaranteed to be NUL-terminated.
 *
 * The normalized form output by this function is suitable for comparing CIF "case-insensitive" strings for equivalence,
 * as equivalent strings will have identical normalized forms.  This accounts not only for case folding itself, but also
 * for combining marks, including sequences thereof.  It does not, however, erase distinctions between different Unicode
 * characters that are typically rendered similarly (so-called "compatibility equivalents"), as that would constitute
 * a semantic change.
 *
 * @param[in] src the Unicode string to normalize; must not be NULL
 * @param[in] srclen the maximum length of the input to normalize; if less than zero then the whole string is
 *     normalized up to the first NUL character (which otherwise does not need to be present); must not exceed
 *     the actual number of UChars in the source string
 * @param[in,out] normalized a pointer to a location to record the result; if NULL then the result is discarded, but
 *     the return code still indicates whether normalization was successful.  If non-NULL, then the pointer at the
 *     specified location is overwritten, and the caller assumes responsibility for freeing the memory to which the
 *     new value points.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_normalize, (
        const UChar *src,
        int32_t srclen,
        UChar **normalized
        ));

/**
 * @brief Converts (the initial part of) a C String to a Unicode string via an ICU default converter.
 *
 * This function is most applicable to C strings obtained from external input, rather than to strings appearing in C
 * source code.  ICU will normally try to guess what converter is appropriate for default text, but the converter it
 * will use can be influenced via @c ucnv_setDefaultName() (warning: the default converter name is @em global).  On
 * successful conversion, the output Unicode string will be NUL terminated.
 *
 * @param[in] cstr the C string to convert; may be NULL, in which case the conversion result is likewise NULL; if
 *     not NULL and  @p srclen is -1, then must be terminated by a NUL byte, else termination is optional
 * @param[in] srclen the input string length, or -1 if the string consists of all bytes up to a NUL terminator
 * @param[in,out] ustr a pointer to a location to record the result; must not be NULL.  If a non-NULL pointer is
 *     written here by this function (which can happen only on success), then the caller assumes ownership of the
 *     memory it references.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
CIF_INTFUNC_DECL(cif_cstr_to_ustr, (
        const char *cstr,
        int32_t srclen,
        UChar **ustr
        ));

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CIF_H */
