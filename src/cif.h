/*
 * cif.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

/** 
 * @file  cif.h
 * @brief The primary CIF API public header file, declaring most data structures, functions, and macros intended for
 *        library users to manipulate.
 *
 * @author John C. Bollinger
 * @date   2012-2013
 */

#ifndef CIF_H
#define CIF_H

#include <stdio.h>
#include <unicode/ustring.h>

/**
 * @brief The maximum number of characters in one line of a CIF
 */
#define CIF_LINE_LENGTH 2048

/**
 * @brief The maximimum number of characters in a CIF data name
 */
#define CIF_NAMELEN_LIMIT CIF_LINE_LENGTH

/**
 * @defgroup return_codes Function return codes
 * @{
 */

/** @brief a result code indicating successful completion of the requested operation */
#define CIF_OK                  0

/**
 * @brief a result code indicating that the requested operation completed successfully, but subsequent repetitions of
 *        the same operation can be expected to fail
 *
 * This code is used mainly by packet iterators to signal the user when the last available packet is returned.
 */
#define CIF_FINISHED            1

/**
 * @brief a result code indicating that the requested operation failed because an error occurred in one of the
 *        underlying libraries
 *
 * This is the library's general-purpose error code for faults that it cannot more specifically diagnose.  Pretty much
 * any API function can return this code under some set of circumstances.  Failed memory allocations or unexpected
 * errors emitted by the storage back end are typical triggers for this code.  In the event that this code is returned,
 * the C library's global @c errno variable @em may indicate the nature of the error more specifically.
 */
#define CIF_ERROR               2

/**
 * @brief a result code returned on a best-effort basis to indicate that a user-provided object handle is invalid.
 *
 * The library does not guarantee to be able to recognize invalid handles.  If an invalid handle is provided then
 * a @c CIF_ERROR code may be returned instead, or really anything might happen -- generally speaking, library
 * behavior is undefined in these cases.
 *
 * Where it does detect invalidity, that result may be context dependent.  That is, the handle may be invalid for
 * the use for which it presented, but valid for different uses.  In particular, this may be the case if a save frame
 * handle is presented to one of the functions that requires specifically a data block handle.
 */
#define CIF_INVALID_HANDLE      3

/**
 * @brief a result code indicating that an internal error or inconsistency was encountered
 *
 * If this code is emitted then it means a bug in the library has been triggered.  (If for no other reason than that
 * this is the wrong code to return if an internal bug has @em not been triggered.)
 */
#define CIF_INTERNAL_ERROR      4

/**
 * @brief a result code indicating generally that one or more arguments to the function do not satisfy the function's
 *        requirements.
 *
 * This is a fallback code -- a more specific code will be emitted when one is available.
 */
#define CIF_ARGUMENT_ERROR      5

/**
 * @brief a result code indicating that although the function was called with with substantially valid arguments, the
 *        context or conditions do not allow the call.
 *
 * This code is returned, for example, if @c cif_pktitr_remove_packet() is passed a packet iterator that has not yet
 * provided a packet that could be removed.
 */
#define CIF_MISUSE              6

/**
 * @brief a result code indicating that an optional feature was invoked and the library implementation in use does not
 *        support it.
 *
 * There are very few optional features defined at this point, so this code should rarely, if even, be seen.
 */
#define CIF_NOT_SUPPORTED       7

/**
 * @brief A result code indicating that the operating environment is missing data or features required to complete the
 *        operation.
 *
 * For example, the @c cif_create() function of an SQLite-based implementation that depends on foreign keys might
 * emit this code if it determines that the external sqlite library against which it is running omits foreign key
 * support.
 */
#define CIF_ENVIRONMENT_ERROR   8

/**
 * @brief A result code indicating a synthetic error injected via a callback to the client
 */
#define CIF_CLIENT_ERROR        9

/**
 * @brief a result code signaling an attempt to cause a CIF to contain blocks with duplicate (by CIF's criteria) block
 *        codes
 */
#define CIF_DUP_BLOCKCODE      11

/**
 * @brief a result code signaling an attempt to cause a CIF to contain a block with an invalid block code
 */
#define CIF_INVALID_BLOCKCODE  12

/**
 * @brief a result code signaling an attempt to retrieve a data block from a CIF (by reference to its block code)
 *        when that CIF does not contain a block bearing that code
 */
#define CIF_NOSUCH_BLOCK       13

/**
 * @brief a result code signaling an attempt to cause a data block to contain save frames with duplicate (by CIF's
 *        criteria) frame codes
 */
#define CIF_DUP_FRAMECODE      21

/**
 * @brief a result code signaling an attempt to cause a data block to contain a save frame with an invalid frame code
 */
#define CIF_INVALID_FRAMECODE  22

/**
 * @brief a result code signaling an attempt to retrieve a save frame from a data block (by reference to its frame code)
 *        when that block does not contain a frame bearing that code
 */
#define CIF_NOSUCH_FRAME       23

/**
 * @brief a result code signaling an attempt to retrieve a loop from a save frame or data block by category, when
 *        there is more than one loop tagged with the specified category
 */
#define CIF_CAT_NOT_UNIQUE     31

/**
 * @brief a result code signaling an attempt to retrieve a loop from a save frame or data block by category, when
 *        the requested category is invalid.
 *
 * The main invalid category is NULL (as opposed to the empty category name), which is invalid only for retrieval, not
 * loop creation.
 */
#define CIF_INVALID_CATEGORY   32

/**
 * @brief a result code signaling an attempt to retrieve a loop from a save frame or data block by category, when
 *        the container does not contain any loop tagged with the specified category.
 *
 * This code is related to loop references by @em category.  Contrast with @c CIF_NOSUCH_ITEM .
 */
#define CIF_NOSUCH_LOOP        33

/**
 * @brief a result code signaling an attempt to manipulate a loop having special significance to the library in a
 *        manner that is not allowed
 *
 * The only reserved loops in this version of the library are those that hold items that do not / should not appear
 * in explicit CIF @c loop_ constructs.  There is at most one of these per data block or save frame, and they are
 * identified by empty (not NULL) category tags. (C code can use the @c CIF_SCALARS macro for this category).
 */
#define CIF_RESERVED_LOOP      34

/**
 * @brief a result code indicating that an attempt was made to add an item value to a different loop than the one
 *        containing the item.
 */
#define CIF_WRONG_LOOP         35

/**
 * @brief a result code indicating that a packet iterator was requested for a loop that contains no packets.
 */
#define CIF_EMPTY_LOOP         36

/**
 * @brief a result code indicating that an attempt was made to create a loop devoid of any data names
 */
#define CIF_NULL_LOOP          37

/**
 * @brief a result code indicating that an attempt was made to add an item to a data block or save frame that already
 *        contains an item of the same code.
 *
 * "Same" in this sense is judged with the use of Unicode normalization and case folding.
 */
#define CIF_DUP_ITEMNAME       41

/**
 * @brief a result code indicating that an attempt was made to add an item with an invalid data name to a CIF.
 *
 * Note that attempts to @em retrieve items by invalid name, on the other hand, simply return @c CIF_NOSUCH_ITEM
 */
#define CIF_INVALID_ITEMNAME   42

/**
 * @brief a result code indicating that an attempt to retrieve an item by name failed as a result of no item bearing
 *        that data name being present in the target container
 */
#define CIF_NOSUCH_ITEM        43

/**
 * @brief a result code indicating that an attempt to retrieve a presumed scalar has instead returned one of multiple
 *        values found.
 */
#define CIF_AMBIGUOUS_ITEM     44

/**
 * @brief a result code indicating that the requested operation could not be performed because a packet object provided
 *        by the user was invalid.
 *
 * For example, the packet might have been empty in a context where that is not allowed.
 */
#define CIF_INVALID_PACKET     52

/**
 * @brief a result code indicating that an attempt was made to parse or write a value in a context that allows
 *        only values of kinds different from the given value's
 *
 * The only context in CIF 2.0 that allows values of some kinds but not others is table indices.
 */
#define CIF_DISALLOWED_VALUE   62

/**
 * @brief a result code indicating that a string provided by the user could not be parsed as a number.
 *
 * "Number" here should be interpreted in the CIF sense: an optionally-signed integer or floating-point number in
 * decimal format, with optional signed exponent and optional standard uncertainty.
 */
#define CIF_INVALID_NUMBER     72

/**
 * @brief a result code indicating that a (Unicode) string provided by the user as a table index is not valid for that
 *        use
 */
#define CIF_INVALID_INDEX      73

/**
 * @brief a result code indicating that input or output exceeded the relevant line-length limit
 */
#define CIF_OVERLENGTH_LINE   108

/**
 * @brief a return code for CIF-handler functions (see @c cif_handler_t) that indicates CIF traversal should continue
 *        along its normal path
 */
#define CIF_TRAVERSE_CONTINUE       0

/**
 * @brief a return code for CIF-handler functions (see @c cif_handler_t) that indicates CIF traversal should bypass
 *        untraversed children of the current element, and thereafter proceed along the normal path
 */
#define CIF_TRAVERSE_SKIP_CHILDREN -1

/**
 * @brief a return code for CIF-handler functions (see @c cif_handler_t) that indicates CIF traversal should bypass
 *        untraversed children and siblings of the current element, and thereafter proceed along the normal path
 */
#define CIF_TRAVERSE_SKIP_SIBLINGS -2

/**
 * @brief a return code for CIF-handler functions (see @c cif_handler_t) that indicates CIF traversal should stop
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
 * @brief the category code with which the library tags the unique loop (if any) in each data block or save
 *        frame that contains items not associated with an explicit CIF @c loop_ construct
 *
 * @hideinitializer
 */
#define CIF_SCALARS (&cif_uchar_nul)

/**
 * @brief a static Unicode NUL character; a pointer to this variable constitutes an empty Unicode string.
 */
static const UChar cif_uchar_nul = 0;

/**
 * @}
 *
 * @defgroup datatypes Data types
 * @{
 */

/**
 * @brief the type of a callback function to be invoked when a parse error occurs.
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
 *
 * @return zero if the parse should continue (with implementation-dependent best-effort error recovery), or nonzero
 *         if the parse should be aborted, forwarding the return code to the caller of the parser 
 */
typedef int (*cif_parse_error_callback_t)(int code, size_t line, size_t column, const UChar *text, size_t length, void *data);

/**
 * @brief represents a collection of CIF parsing options.
 *
 * Unlike most data types defined by the CIF API, the parse options are not opaque.  This reflects the @c struct's
 * intended use for collecting (only) user-settable option values.
 */
struct cif_parse_opts_s {

    /**
     * @brief if non-zero, directs the parser to handle a CIF stream without any CIF-version code as CIF 2, instead
     *         of as CIF 1.
     *
     * Because the CIF-version code is @em required in CIF 2 but optional in CIF 1, it is most correct to assume CIF 1
     * when there is no version code.  Nevertheless, if a CIF is known or assumed to otherwise comply with CIF2, then
     * it is desirable to parse it that way regardless of the absence of a version code.
     *
     * CIF 2 streams that erroneously omit the version code will be parsed as CIF 2 if this option is enabled (albeit
     * with an error on account of the missing version code).  In that case, however, CIF 1 streams that (allowably)
     * omit the version code may be parsed incorrectly.
     */
    int default_to_cif2;

    /**
     * @brief if not @c NULL , names the coded character set with which the parser will attempt to interpret plain
     *         CIF 1.1 "text" files that do not bear CIF-recognized encoding information.
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
     * input CIF, then the parser will choose a default text encoding for CIF 1.1 documents that is appropriate to the
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
    char *default_encoding_name;

    /**
     * @brief If non-zero then the default encoding specified by @c default_encoding_name will be used to interpret the
     * CIF 1.1 or 2.0 input, regardless of any encoding signature or other appearance to the contrary.
     *
     * If @c default_encoding_name is NULL then it represents a system-dependent default encoding.  That's the norm
     * for CIF 1.1 input anyway, but if @c force_default_encoding is nonzero then the same system-dependent default will
     * be chosen for CIF 2.0 as well.
     *
     * This option is dangerous.  Enabling it can cause CIF parsing to fail, or in some cases cause CIF contents to
     * silently be misinterpreted if the specified default encoding is not in fact the correct encoding for the input.
     * On the other hand, use of this option is essential for correctly parsing CIF documents whose encoding cannot be
     * determined or guessed correctly.
     *
     * This option can be used to parse CIF 2.0 text that is encoded other than via UTF-8.  Such a file is not valid
     * CIF 2.0, and therefore will cause an error to be flagged, but if the error is ignored and the specified encoding
     * is in fact correct for the input then parsing will otherwise proceed normally.
     */
    int force_default_encoding;

    /**
     * @brief modifies whether line-folded text fields will be recognized and unfolded during parsing.
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
     * @brief modifies whether prefixed text fields will be recognized and de-prefixed during parsing.
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
     * @brief a callback function by which the client application can be notified about parse errors, affording it the
     *         option to interrupt the parse or allow it to continue.
     *
     * If not @c NULL, this function will be called by the parser whenever it encounters an error in the input CIF.
     * The parse will be terminated immediately with an error if this function returns non-zero; otherwise, it
     * serves informational purposes only.
     *
     * If @c NULL, or if parse options are not specified, then the parser will stop at the first error.
     */
    cif_parse_error_callback_t error_callback;

    /**
     * @brief a pointer to user data to be forwarded to all callback functions invoked by the parser; opaque to the
     *         parser itself, and may be @c NULL.
     */
    void *user_data;
};

/**
 * @brief an opaque handle on a managed CIF.
 */
typedef struct cif_s cif_t;

/**
 * @brief an opaque handle on a managed CIF data block or save frame.
 *
 * From a structural perspective, save frames and data blocks are distinguished only by nesting level: data blocks are
 * (the only meaningful) top-level components of whole CIFs, whereas save frames are nested inside data blocks.  They
 * are otherwise exactly the same with respect to contents and allowed operations, and @c cif_container_t models
 * that commonality.
 */
typedef struct cif_container_s cif_container_t;

/**
 * @brief equivalent to and interchangeable with @c cif_container_t, but helpful for bookkeeping to track those
 *         containers that are supposed to be data blocks
 */
typedef struct cif_container_s cif_block_t;

/**
 * @brief equivalent to and interchangeable with @c cif_container_t, but helpful for bookkeeping to track those
 *         containers that are supposed to be save frames
 */
typedef struct cif_container_s cif_frame_t;

/**
 * @brief an opaque handle on a managed CIF loop
 */
typedef struct cif_loop_s cif_loop_t;

/**
 * @brief an opaque data structure representing a CIF loop packet.
 *
 * This is not a "handle" in the sense of some of the other opaque data types, as instances have no direct connection
 * to a managed CIF.
 */
typedef struct cif_packet_s cif_packet_t;

/**
 * @brief an opaque data structure encapsulating the state of an iteration through the packets of a loop in an
 *         managed CIF
 */
typedef struct cif_pktitr_s cif_pktitr_t;

/**
 * @brief the type of all data value objects
 */
typedef union cif_value_u cif_value_t;

/**
 * @brief the type used for codes representing the dynamic kind of the data in a @c cif_value_t object
 */
typedef enum { 
    /**
     * @brief the kind code representing a character (Unicode string) data value
     */
    CIF_CHAR_KIND, 

    /**
     * @brief the kind code representing a numeric or presumed-numeric data value
     */
    CIF_NUMB_KIND, 

    /**
     * @brief the kind code representing a CIF 2.0 list data value
     */
    CIF_LIST_KIND, 

    /**
     * @brief the kind code representing a CIF 2.0 table data value
     */
    CIF_TABLE_KIND, 

    /**
     * @brief the kind code representing the not-applicable data value
     */
    CIF_NA_KIND, 

    /**
     * @brief the kind code representing the unknown/unspecified data value
     */
    CIF_UNK_KIND
} cif_kind_t;

/**
 * @brief a set of functions definining a handler interface for directing and taking appropriate action in response
 *     to a traversal of a CIF.
 *
 * Each structural element of the CIF being traversed will be presented to the appropriate handler function,
 * along with the appropriate context object, if any.  When traversal is performed via the @c cif_walk() function,
 * the context object is the one passed to @c cif_walk() by its caller.  The handler may perform any action
 * it considers suitable, and it is expected to return a code influencing the traversal path, one of:
 * @li @c CIF_TRAVERSE_CONTINUE for the walker to continue on its default path (for instance, to descend to the current element's first child), or
 * @li @c CIF_TRAVERSE_SKIP_CHILDREN for the walker to skip traversal of the current element's children, if any (meaningfully distinct from @c CIF_TRAVERSE_CONTINUE only for the @c *_start callbacks), or
 * @li @c CIF_TRAVERSE_SKIP_SIBLINGS for the walker to skip the current element's children and all its siblings that have not yet been traversed, or
 * @li @c CIF_TRAVERSE_END for the walker to stop without traversing any more elements
 *
 * A handler function may, alternatively, return a CIF API error code, which has the effect of CIF_TRAVERSE_END plus
 * additional semantics specific to the traversal function (@c cif_walk() forwards the code to its caller as its
 * return value).
 */
typedef struct {
    /** @brief a handler function for the beginning of a top-level CIF object */
    int (*handle_cif_start)(cif_t *cif, void *context);
    /** @brief a handler function for the end of a top-level CIF object */
    int (*handle_cif_end)(cif_t *cif, void *context);
    /** @brief a handler function for the beginning of a data block */
    int (*handle_block_start)(cif_container_t *block, void *context);
    /** @brief a handler function for the end of a data block */
    int (*handle_block_end)(cif_container_t *block, void *context);
    /** @brief a handler function for the beginning of a save frame */
    int (*handle_frame_start)(cif_container_t *frame, void *context);
    /** @brief a handler function for the end of a save frame */
    int (*handle_frame_end)(cif_container_t *frame, void *context);
    /** @brief a handler function for the beginning of a loop */
    int (*handle_loop_start)(cif_loop_t *loop, void *context);
    /** @brief a handler function for the end of a loop */
    int (*handle_loop_end)(cif_loop_t *loop, void *context);
    /** @brief a handler function for the beginning of a loop packet */
    int (*handle_packet_start)(cif_packet_t *packet, void *context);
    /** @brief a handler function for the end of a loop packet */
    int (*handle_packet_end)(cif_packet_t *packet, void *context);
    /** @brief a handler function for data items (there are not separate beginning and end callbacks) */
    int (*handle_item)(UChar *name, cif_value_t *value, void *context);
} cif_handler_t;

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
 * @param[in] stream a @c FILE @c * from which to read the raw CIF data; must be a non-NULL pointer to a readable
 *         stream, which will typically be read to its end
 *
 * @param[in] options a pointer to a @c struct @c cif_parse_opts_s object describing options to use while parsing, or
 *         @c NULL to use default values for all options
 *
 * @param[in,out] cif controls the disposition of the parsed data.  If NULL, then the parsed data are discarded, else
 *         they added to the CIF to which this pointer refers, creating a new one for the purpose if the pointer
 *         dereferences to a NULL pointer.
 * 
 * @return Returns @c CIF_OK on a successful parse, even if the results are discarded, or an error code
 *         (typically @c CIF_ERROR ) on failure.  In the event of a failure, a new CIF object may still be created
 *         and returned via the @c cif argument or the provided CIF object may still be modified by addition of
 *         one or more data blocks.
 */
int cif_parse(
        /*@in@*/ /*@temp@*/ FILE *stream,
        /*@in@*/ /*@temp@*/ struct cif_parse_opts_s *options,
        /*@out@*/ /*@null@*/ cif_t **cif
        ) /*@modifies *cif@*/;

/**
 * @brief a CIF parse error handler function that ignores all errors
 */
int cif_parse_error_ignore(int code, size_t line, size_t column, const UChar *text, size_t length, void *data);

/**
 * @brief a CIF parse error handler function that interrupts the parse on any error, causing @c CIF_ERROR to be returned
 */
int cif_parse_error_die(int code, size_t line, size_t column, const UChar *text, size_t length, void *data);

/**
 * @brief Formats the CIF data represented by the @c cif handle to the specified output stream.
 *
 * @param[in,out] stream a @c FILE @c * to which to write the CIF format output; must be a non-NULL pointer to a
 *         writable stream.
 *
 * @param[in] cif a handle on the managed CIF object to output; must be non-NULL and valid.
 *
 * @return Returns @c CIF_OK if the data are fully written, or else an error code (typically @c CIF_ERROR ).
 *         The stream state is undefined after a failure.
 */
int cif_write(
        /*@in@*/ /*@temp@*/ FILE *stream,
        /*@in@*/ /*@temp@*/ cif_t *cif
        );

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
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
int cif_create(
        /*@temp@*/ cif_t **cif
        ) /*@modifies *cif@*/;

/**
 * @brief Removes the specified managed CIF, releasing all resources it holds.
 *
 * The @c cif handle and any outstanding handles on its contents are invalidated by this function.  The effects of
 * any further use of them are undefined, though the functions of this library @em may return code
 * @c CIF_INVALID_HANDLE if such a handle is passed to them.  Any resources belonging to the provided handle itself
 * are also released, but not those belonging to any other handle.  The original external source (if any) of the CIF
 * data is not affected.
 *
 * @param[in] cif the handle on the CIF to destroy; must not be NULL
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
int cif_destroy(
        /*@only@*/ cif_t *cif
        );

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
 * @param[in] cif a handle on the managed CIF object to which a new block should be added; must be non-NULL and
 *        valid.
 *
 * @param[in] code the block code of the block to add, as a NUL-terminated Unicode string; the block code must
 *        comply with CIF constraints on block codes.
 *
 * @param[in,out] block if not NULL, then a location where a handle on the new block should be recorded.  A handle is
 *        recorded only on success.
 *
 * @return @c CIF_OK on success or an error code on failure, normally one of:
 *        @li @c CIF_INVALID_BLOCKCODE  if the provided block code is invalid
 *        @li @c CIF_DUP_BLOCKCODE if the specified CIF already contains a block having the given code (note that
 *                block codes are compared in a Unicode-normalized and caseless form)
 *        @li @c CIF_ERROR for most other failures
 */
int cif_create_block(
        /*@in@*/ /*@temp@*/ cif_t *cif,
        /*@in@*/ /*@temp@*/ const UChar *code,
        /*@out@*/ /*@null@*/ cif_block_t **block
        ) /*@modifies *block@*/;

/**
 * @brief Looks up and optionally returns the data block bearing the specified block code, if any, in the specified CIF.
 *
 * Note that CIF block codes are matched in caseless and Unicode-normalized form.
 *
 * @param[in] cif a handle on the managed CIF object in which to look up the specified block code; must be non-NULL
 *         and valid.
 *
 * @param[in] code the block code to look up, as a NUL-terminated Unicode string.
 *
 * @param[in,out] block if not NULL on input and a block matching the specified code is found, then a handle on it is
 *         written where this parameter points.  The caller assumes responsibility for releasing this handle.
 *       
 * @return @c CIF_OK on a successful lookup (even if @c block is NULL), @c CIF_NOSUCH_BLOCK if there is no data block
 *         bearing the given code in the given CIF, or an error code (typically @c CIF_ERROR ) if an error occurs.
 */
int cif_get_block(
        /*@in@*/ /*@temp@*/ cif_t *cif,
        /*@in@*/ /*@temp@*/ const UChar *code,
        /*@out@*/ /*@null@*/ cif_block_t **block
        ) /*@modifies *block@*/;

/**
 * @brief Provides a null-terminated array of data block handles, one for each block in the specified CIF.
 *
 * The caller takes responsibility for cleaning up the provided block handles and the dynamic array containing them.
 *
 * @param[in] cif a handle on the managed CIF object from which to draw block handles; must be non-NULL and valid.
 *
 * @param[in,out] blocks a pointer to the location where a pointer to the resulting array of block handles should be
 *         recorded.  Must not be NULL.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_get_all_blocks(
        /*@in@*/ /*@temp@*/ cif_t *cif,
        /*@out@*/ cif_block_t ***blocks
        );

/**
 * @brief Performs a depth-first, natural-order traversal of a CIF, calling back to handler
 *        routines provided by the caller for each structural element
 *
 * Traverses the tree structure of a CIF, invoking caller-specified handler functions for each structural element
 * along the route.  Order is block -> [frame ->] loop -> packet -> item, with save frames are traversed before loops
 * belonging to the same data block.  Handler callbacks can influence the walker's path via their return values,
 * instructing it to continue as normal (@c CIF_TRAVERSE_CONTINUE), to avoid traversing the children of the current
 * element (@c CIF_TRAVERSE_SKIP_CHILDREN), to avoid traversing subsequent siblings of the current element
 * (@c CIF_TRAVERSE_SKIP_SIBLINGS), or to terminate the walk altogether (@c CIF_TRAVERSE_END).  For the purposes of
 * this function, loops are not considered "siblings" of save frames belonging to the same data block.
 *
 * @param[in] cif the CIF to traverse
 * @param[in] handler a structure containing the callback functions handling each type of CIF structural element
 * @param[in] context a context object to pass to each callback function; opaque to the walker itself
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR) on failure
 */
int cif_walk(cif_t *cif, cif_handler_t *handler, void *context);

/**
 * @}
 *
 * @defgroup block_funcs Data block specific functions
 *
 * @{
 */

/**
 * @brief an alias for @c cif_container_free
 *
 * @hideinitializer
 */
#define cif_block_free cif_container_free

/**
 * @brief an alias for @c cif_container_destroy
 *
 * @hideinitializer
 */
#define cif_block_destroy cif_container_destroy

/**
 * @brief Creates a new save frame bearing the specified frame code in the specified data block.
 *
 * This function can optionally return a handle on the new frame via the @c frame argument.  When that option
 * is used, the caller assumes responsibility for cleaning up the provided handle via either @c cif_container_free()
 * (to clean up only the handle) or @c cif_container_destroy() (to also remove the frame from the managed CIF).
 *
 * @param[in] block a handle on the managed data block object to which a new block should be added; must be
 *        non-NULL and valid.
 *
 * @param[in] code the frame code of the frame to add, as a NUL-terminated Unicode string; the frame code must
 *        comply with CIF constraints on frame codes.
 *
 * @param[in,out] frame if not NULL, then a location where a handle on the new frame should be recorded.
 *
 * @return @c CIF_OK on success or an error code on failure, normally one of:
 *        @li @c CIF_INVALID_FRAMECODE  if the provided frame code is invalid
 *        @li @c CIF_DUP_FRAMECODE if the specified CIF already contains a frame having the given code (note that
 *                frame codes are compared in a Unicode-normalized and caseless form)
 *        @li @c CIF_ERROR for most other failures
 */
int cif_block_create_frame(
        /*@in@*/ /*@temp@*/ cif_block_t *block,
        /*@in@*/ /*@temp@*/ const UChar *code,
        /*@out@*/ /*@null@*/ cif_frame_t **frame
        ) /*@modifies *frame@*/;

/**
 * @brief Looks up and optionally returns the save frame bearing the specified code, if any, in the specified CIF.
 *
 * Note that CIF frame codes are matched in caseless and Unicode-normalized form.
 *
 * @param[in] block a handle on the managed data block object in which to look up the specified frame code; must be
 *         non-NULL and valid.
 *
 * @param[in] code the frame code to look up, as a NUL-terminated Unicode string.
 *
 * @param[in,out] frame if not NULL and a frame matching the specified code is found, then a handle on it is
 *         written where this parameter points.  The caller assumes responsibility for releasing this handle.
 *       
 * @return @c CIF_OK on a successful lookup (even if @c frame is NULL), @c CIF_NOSUCH_FRAME if there is no save frame
 *         bearing the given code in the given CIF, or an error code (typically @c CIF_ERROR ) if an error occurs.
 */
int cif_block_get_frame(
        /*@in@*/ /*@temp@*/ cif_block_t *block,
        /*@in@*/ /*@temp@*/ const UChar *code,
        /*@out@*/ /*@null@*/ cif_frame_t **frame
        ) /*@modifies *frame@*/;

/**
 * @brief Provides a null-terminated array of save frame handles, one for each frame in the specified data block.
 *
 * The caller takes responsibility for cleaning up the provided frame handles.
 *
 * @param[in] block a handle on the managed data block object from which to draw save frame handles; must be non-NULL and
 *         valid.
 *
 * @param[in,out] frames a pointer to the location where a pointer to the resulting array of frame handles should be
 *         recorded.  Must not be NULL.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_block_get_all_frames(
        /*@in@*/ /*@temp@*/ cif_block_t *block,
        /*@out@*/ cif_frame_t ***frames
        );

/**
 * @}
 *
 * @defgroup frame_funcs Save frame specific functions
 *
 * @{
 */

/**
 * @brief an alias for @c cif_container_free
 *
 * @hideinitializer
 */
#define cif_frame_free cif_container_free

/**
 * @brief an alias for @c cif_container_destroy
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
 * @brief Frees any resources associated with the specified container handle without
 * modifying the managed CIF to which its associated data block or save frame
 * belongs.
 *
 * @param[in] container a handle on the container object to free; must be non-NULL and valid on entry, but is
 *         invalidated by this function
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_container_free(
        /*@only@*/ /*@null@*/ cif_container_t *container
        );

/**
 * @brief Frees any resources associated with the specified container handle, and furthermore removes the associated
 *         save frame and all its contents from its managed CIF.
 *
 * @param[in,out] container a handle on the container to destroy; must be non-NULL and valid on entry, but is
 *         invalidated by this function
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_container_destroy(
        /*@only@*/ cif_container_t *container
        );

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
int cif_container_get_code(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ UChar **code
        );

/**
 * @brief Asserts that the specified container represents a data block, as opposed to a save frame
 *
 * @param[in] container a handle on the container that is asserted to be a data block
 *
 * @return @c CIF_OK if the container is a data block, @c CIF_ARGUMENT_ERROR if it is a save frame, or @c CIF_ERROR if
 *         it is @c NULL
 */
int cif_container_assert_block(cif_container_t *container);

/**
 * @brief Creates a new loop in the specified container, and optionally returns a handle on it.
 *
 * There must be at least one item name, and all item names given must initially be absent from the container.  All item
 * names must be valid, per the CIF specifications.
 * The category name may be NULL, in which case the loop has no explicit category, but the empty category name is
 * reserved for the loop containing all the scalar data in the container.  The preprocessor macro @c CIF_SCALARS
 * expands to that category name; it is provided to make the meaning clearer.  Other categories are not required to be
 * unique, but they are less useful when there are duplicates in the same container.  New loops initially contain
 * zero packets, which is acceptable only as a transient state.
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
int cif_container_create_loop(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ const UChar *category,
        /*@in@*/ /*@temp@*/ UChar *names[],
        /*@out@*/ /*@null@*/ cif_loop_t **loop
        ) /*@modifies *loop@*/;

/**
 * @brief Looks up the loop, if any, in the specified container that is assigned to the specified category.
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
int cif_container_get_category_loop(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ const UChar *category,
        /*@out@*/ /*@null@*/ cif_loop_t **loop
        ) /*@modifies *loop@*/;

/**
 * @brief Looks up the loop, if any, in the specified container that contains the specified item, and optionally returns
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
int cif_container_get_item_loop(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ const UChar *item_name,
        /*@out@*/ /*@null@*/ cif_loop_t **loop
        ) /*@modifies *loop@*/;

/**
 * @brief Provides a null-terminated array of loop handles, one for each loop in the specified container.
 *
 * The caller takes responsibility for cleaning up the provided loop handles and the array containing them.
 *
 * @param[in] container a handle on the managed container object for which to draw loop handles; must be non-NULL and valid.
 *
 * @param[in,out] loops a pointer to the location where a pointer to the resulting array of loop handles should be
 *         recorded.  Must not be NULL.
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_container_get_all_loops(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@out@*/ /*@null@*/ cif_loop_t ***loops
        );

/**
 * @brief Looks up an item by name in a data block or save frame, and optionally returns (one of) its value(s)
 *
 * The return value indicates whether the item is present with a single value (@c CIF_OK), present with multiple
 * values (@c CIF_AMBIGUOUS_ITEM), or absent (@c CIF_NOSUCH_ITEM).  Being present in a zero-packet loop is treated
 * the same as being absent altogether.  If there are any values for the item and the @c val parameter is non-NULL,
 * then one of the values is returned via @c val.  Name matching is performed via a caseless, Unicode-normalized
 * algorithm.
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
int cif_container_get_value(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ const UChar *item_name,
        /*@in@*/ cif_value_t **val
        ) /*@modifies *val@*/;

/**
 * @brief Sets the value of the specified item in the specified container, or adds it
 * as a scalar if it's not already present in the container.
 *
 * Care is required with this function: if the named item is in a multi-packet loop then the
 * given value is set for the item in every loop packet.
 *
 * @param[in] container a handle on the container to modify; must be non-NULL and valid
 *
 * @param[in] item_name the name of the item to set, as a NUL-terminated Unicode string; must be a valid item name
 *
 * @param[in] val a pointer to the value to set for the specified item
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_container_set_value(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ const UChar *item_name,
        /*@in@*/ /*@temp@*/ cif_value_t *val
        );

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
int cif_container_remove_item(
        /*@in@*/ /*@temp@*/ cif_container_t *container,
        /*@in@*/ /*@temp@*/ const UChar *item_name
        );

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
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure.
 */
int cif_loop_free(
        /*@only@*/ cif_loop_t *loop
        );

/**
 * @brief Releases any resources associated with the given loop handle, and furthermore removes the entire associated
 *         loop from its managed CIF, including all items and values belonging to it, discarding them.
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
int cif_loop_destroy(
        /*@only@*/ cif_loop_t *loop
        );

/**
 * @brief Retrieves this loop's category
 *
 * Responsibility for cleaning up the category string belongs to the caller.  Note that loops are not required to
 * have categories assigned, therefore the category returned may be a NULL pointer.
 *
 * @param[in] loop a handle on the loop whose item names are requested
 *
 * @param[in,out] category a pointer to the location where a pointer to a NUL-terminated Unicode string containing the
 *         requested category (or NULL) should be recorded
 *
 * @return @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_loop_get_category(
        /*@in@*/ /*@temp@*/ cif_loop_t *loop,
        /*@in@*/ /*@temp@*/ UChar **category
        );

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
int cif_loop_get_names(
        /*@in@*/ /*@temp@*/ cif_loop_t *loop,
        /*@in@*/ /*@temp@*/ UChar ***item_names
        ) /*@modifies *item_names@*/;

/**
 * @brief Adds the CIF data item identified by the specified name to the specified managed loop, with the given
 *         initial value in every existing loop packet.
 *
 * Note that there is an asymmetry here, in that items are added to specific @em loops, but removed from 
 * overall @em containers (data blocks and save frames).
 *
 * @param[in] loop a handle on the loop to which the item should be added
 *
 * @param[in] item_name the name of the item to add as a NUL-terminated Unicode string; must be non-NULL and a valid
 *         CIF item name
 *
 * @param[in] val a pointer to the value object that serves as the the initial value for the item
 *
 * @return Returns @c CIF_OK on success or a characteristic error code on failure, normally one of:
 *         @li @c CIF_INVALID_ITEMNAME if the item name is not valid
 *         @li @c CIF_DUP_ITEMNAME if the specified item name is already present in the loop's container
 *         @li @c CIF_ERROR in most other cases
 */
int cif_loop_add_item(
        /*@in@*/ /*@temp@*/ cif_loop_t *loop,
        /*@in@*/ /*@temp@*/ const UChar *item_name,
        /*@in@*/ /*@temp@*/ cif_value_t *val
        );

/* items are removed directly from containers, not from loops */

/**
 * @brief Adds a packet to the specified loop.
 *
 * The packet is not required to provide data for all items in the loop (items without specified values get the
 * explicit unknown value), but it must specify at least one value, and it must not provide data for any items not
 * in the loop.
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
int cif_loop_add_packet(
        /*@in@*/ /*@temp@*/ cif_loop_t *loop,
        /*@in@*/ /*@temp@*/ cif_packet_t *packet
        );

/**
 * @brief Creates an iterator over the packets in the specified loop.
 *
 * After successful return and before the iterator is closed or aborted, behavior is undefined if the underlying
 * loop is modified other than via the iterator.  Furthermore, other modifications to the same managed CIF made
 * while an iterator is active may be sensitive to the iterator, in that it is undefined whether they will be
 * retained if the iterator is aborted or closed unsuccessfully.  As a result of these constraints, users are advised
 * to maintain as few active iterators as feasible for any given CIF, and to minimize non-iterator operations performed
 * while any iterator is active on the same target CIF.
 *
 * @param[in] loop a handle on the loop whose packets are requested; must be non-NULL and valid
 *
 * @param[in,out] iterator the location where a pointer to the iterator object should be written; must not be NULL
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_loop_get_packets(
        /*@in@*/ /*@temp@*/ cif_loop_t *loop,
        /*@in@*/ /*@temp@*/ cif_pktitr_t **iterator
        ) /*@modifies *iterator@*/;

/**
 * @}
 *
 * @defgroup pktitr_funcs Functions for (loop) packet iteration
 *
 * @{
 */

/**
 * @brief Finalizes any changes applied via the specified iterator and releases any resources associated with it.  The
 *         iterator ceases to be active.
 *
 * Note that failure means that changes applied by the iterator could not be made permanent; the iterator's resources
 * are released regardless, and it ceases being active.
 *
 * @param[in,out] iterator a pointer to the iterator to close; must be a non-NULL pointer to an active iterator object
 *
 * @return @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_pktitr_close(
        /*@only@*/ cif_pktitr_t *iterator
        );

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
int cif_pktitr_abort(
        /*@only@*/ cif_pktitr_t *iterator
        );

/**
 * @brief Advances a packet iterator to the next packet, and optionally returns the contents of that packet.
 *
 * If @c packet is not NULL then the packet data are recorded where it points, either replacing the contents of a
 * packet provided that way by the caller (when @c *packet is not NULL) or providing a new packet to the caller.
 * A new packet provided to the caller in this way becomes the responsibility of the caller; when no longer needed
 * its resources should be released via @c cif_packet_free().  Notwithstanding the foregoing, behavior of this
 * function is undefined if it has previously returned @c CIF_FINISHED for the given iterator.
 *
 * @param[in,out] iterator a pointer to the packet iterator from which the next packet is requested
 *
 * @param[in,out] packet if non-NULL, the location where the contents of the next packet should be provided, either
 *         as the address of a new packet, or by replacing the contents of an existing one whose address is already
 *         recorded there.
 *
 * @return On success returns @c CIF_OK if subsequent packets are also available, or @c CIF_FINISHED otherwise.
 *         Returns an error code on failure (typically @c CIF_ERROR ), in which case the contents of any pre-existing
 *         packet provided to the function are undefined (but valid)
 */
int cif_pktitr_next_packet(
        /*@in@*/ /*@temp@*/ cif_pktitr_t *iterator,
        /*@in@*/ /*@temp@*/ /*@null@*/ cif_packet_t **packet
        ) /*@modifies *packet@*/;

/**
 * @brief Updates the last packet iterated by the specified iterator with the values from the provided packet.
 *
 * It is an error to pass an iterator to this function for which @c cif_pktitr_next_packet() has not been called
 * since it was obtained or since it was most recently passed to @c cif_pktitr_remove_packet(). Items in the
 * iterator's target loop for which the packet does not provide a value are left unchanged.  It is an error for the
 * provided packet to provide a value for an item not included in the iterator's loop.
 *
 * @param[in] iterator a pointer to the packet iterator defining the loop and packet to update; must be a non-NULL
 *         pointer to an active packet iterator
 *
 * @param[in] packet a pointer to the packet object defining the updates to apply; must not be NULL
 *
 * @return @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one of:
 *         @li @c CIF_MISUSE if the iterator has no current packet (because it has not yet provided one, or because the
 *              one most recently provided has been removed
 *         @li @c CIF_WRONG_LOOP if the packet provides an item that does not belong to the iterator's loop
 *         @li @c CIF_ERROR in most other cases
 */
int cif_pktitr_update_packet(
        /*@in@*/ /*@temp@*/ cif_pktitr_t *iterator,
        /*@in@*/ /*@temp@*/ cif_packet_t *packet
        );

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
 *              one most recently provided has been removed
 *         @li @c CIF_ERROR in most other cases
 */
int cif_pktitr_remove_packet(
        /*@in@*/ /*@temp@*/ cif_pktitr_t *iterator
        );

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
 * The resulting packet does not retain any references to the item name strings provided by the caller.  The new
 * packet will contain for each data name a value object representing the explicit "unknown" value.  The caller is
 * responsible for freeing the packet when it is no longer needed.
 * 
 * @param[in,out] packet the location were the address of the new packet should be recorded.  Must not be NULL.
 *
 * @param[in] names a pointer to a NULL-terminated array of Unicode string pointers, each element containing one data
 *         names to be represented in the new packet; may contain zero names.  The responsibility for these resources
 *         is not transferred.
 *
 * @return Returns @c CIF_OK on success, or else an error code characterizing the nature of the failure, normally one
 *         of:
 *         @li @c CIF_INVALID_ITEMNAME if one of the provided strings is not a valid CIF data name
 *         @li @c CIF_ERROR in most other cases
 */
int cif_packet_create(
        cif_packet_t **packet, 
        UChar **names
        );

/**
 * @brief Retrieves the names of all the items in the current packet.
 *
 * It is the caller's responsibility to release the resulting array, but its elements @b MUST @b NOT be modified or
 * freed.
 *
 * @param[in] packet a pointer to the packet whose data names are requested
 *
 * @param[in,out] names  the location where a pointer to the array of names should be written.  The array will be
 *         NULL-terminated.
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_packet_get_names(
        /*@temp@*/ cif_packet_t *packet,
        UChar ***names
        );

/**
 * @brief Sets the value of the specified CIF item in the specified packet, including if the packet did not previously
 *         provide any value for the specified data name.
 *
 * The provided value object is copied into the packet; the packet does not assume responsibility for either the
 * specified value object or the specified name.  If the value handle is @c NULL then an explicit unknown-value object is
 * recorded.  The copied (or created) value can subsequently be obtained via @c cif_packet_get_item().
 *
 * @param[in,out] packet a pointer to the packet to modify
 *
 * @param[in] name the name of the item within the packet to modify, as a NUL-terminated Unicode string; must be
 *         non-null and valid for a CIF data name
 *
 * @param[in] value the value object to copy into the packet, or @c NULL for an unknown-value value object to be added
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_packet_set_item(
        /*@temp@*/ cif_packet_t *packet, 
        /*@temp@*/ const UChar *name, 
        /*@temp@*/ cif_value_t *value
        );

/**
 * @brief Determines whether a packet contains a value for a specified data name, and optionally provides that value.
 *
 * The value, if any, is returned via parameter @p value, provided that that parameter is not NULL.  In contrast to
 * @c cif_packet_set_item(), the value is not copied out; rather a pointer to the packet's own copy is provided.
 * Therefore, callers must not free the value object themselves, but they may freely @em modify it, and such
 * modifications will be visible via the packet.
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
int cif_packet_get_item(
        /*@temp@*/ cif_packet_t *packet, 
        /*@temp@*/ const UChar *name, 
        cif_value_t **value
        );

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
 *         otherwise.
 */
int cif_packet_remove_item(
        /*@temp@*/ cif_packet_t *packet, 
        /*@temp@*/ const UChar *name, 
        cif_value_t **value
        );

/**
 * @brief Releases the specified packet and all resources associated with it, including those associated with any
 * values within.
 *
 * This function is a safe no-op when the argument is NULL.
 *
 * @param[in] packet a pointer to the packet to free
 *
 * @return Returns @c CIF_OK on success.  In principle, an error code such as @c CIF_ERROR is returned on failure, but
 *         no specific failure conditions are currently defined.
 */
int cif_packet_free(
        /*@only@*/ cif_packet_t *packet
        );

/**
 * @}
 *
 * @defgroup value_funcs Functions for manipulating value objects
 *
 * @{
 */

/**
 * @brief Allocates and initializes a new value object of the specified kind.
 *
 * The new object is initialized with a kind-dependent default value:
 * @li an empty string for @c CIF_CHAR_KIND
 * @li an exact zero for @c CIF_NUMB_KIND
 * @li an empty list or table for @c CIF_LIST_KIND and @c CIF_TABLE_KIND, respectively
 * @li (there is only one possible distinct value each for @c CIF_UNK_KIND and @c CIF_NA_KIND)
 * 
 * For @c CIF_CHAR_KIND and @c CIF_NUMB_KIND, however, it is somewhat more efficient to create a value object of kind
 * @c CIF_UNK_KIND and then initialize it with the appropriate kind-specific function.
 *
 * @param[in] kind the value kind to create
 *
 * @param[in,out] value the location where a pointer to the new value should be recorded; must not be NULL
 *
 * @return Returns @c CIF_OK on success or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_value_create(
        cif_kind_t kind,
        cif_value_t **value
        );

/**
 * @brief Frees any resources associated with the provided value object without freeing the object itself,
 *         changing it into the explicit unknown value.
 *
 * This function's primary uses are internal, though it is available for anyone to use.  External code more often
 * wants @c cif_value_free() instead.  This function does not directly affect any managed CIF.
 *
 * @param[in,out] value a pointer to the value object to clean
 *
 * @return Returns @c CIF_OK on success.  In principle, an error code such as @c CIF_ERROR is returned on failure, but
 *         no specific failure conditions are currently defined.
 */
int cif_value_clean(
        /*@temp@*/ cif_value_t *value
        );

/**
 * @brief Frees the specified value object and all resources associated with it.
 *
 * This provides only resource management; it has no effect on any managed CIF.  This function is a safe no-op
 * when its argument is NULL.
 *
 * @param[in,out] value a pointer to the value object to free
 *
 * @return Returns @c CIF_OK on success.  In principle, an error code such as @c CIF_ERROR is returned on failure, but
 *         no specific failure conditions are currently defined.
 */
int cif_value_free(
        /*@only@*/ cif_value_t *value
        );

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
 *         the contents of an existing value object or by recording a pointer to a new one
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_value_clone(
        /*@in@*/ /*@temp@*/ cif_value_t *value,
        /*@in@*/ /*@temp@*/ cif_value_t **clone
        ) /*@modifies *clone@*/;

/**
 * @brief Reinitializes the provided value object to a default value of the specified kind
 *
 * The value referenced by the provided handle should have been allocated via @c cif_value_create().  Any unneeded
 * resources it holds are released.  The specified value kind does not need to be the same as the value's present
 * kind.  Kind-specific default values are documented with @c cif_value_create().
 *
 * This function is equivalent to cif_value_clean() when the specified kind is @c CIF_UNK_KIND, and it is less useful
 * than the character- and number-specific (re)initialization functions for those value kinds.  It is the only
 * means available to change an existing value to a list, table, or N/A value in-place, however.
 *
 * On failure, the value is left in a valid but undefined state.
 *
 * @param[in,out] value a handle on the value to reinitialize; must not be @c NULL
 * @param[in] kind the cif value kind as which the value should be reinitialized
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_value_init(
        cif_value_t *value,
        cif_kind_t kind);

/**
 * @brief (Re)initializes the specified value object as being of kind CIF_CHAR_KIND, with the specified text
 *
 * Any previous contents of the provided value object are first cleaned as if by @c cif_value_clean().  Responsibility
 * for the provided string passes to the value object; it should not subsequently be managed directly by the caller.
 * Note that this implies that the text must be dynamically allocated.  The value object will become sensitive to text
 * changes performed afterward via the @c text pointer.
 *
 * @param[in,out] value a pointer to the value object to be initialized; must not be NULL
 *
 * @param[in] text the new text content for the specified value object; must not be NULL
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_value_init_char(
        /*@in@*/ /*@temp@*/ cif_value_t *value,
        /*@in@*/ /*@keep@*/ UChar *text
        );

/**
 * @brief (Re)initializes the specified value object as being of kind CIF_CHAR_KIND, with a copy of the specified text.
 *
 * This function performs the same job as @c cif_value_init_char(), except that it makes a copy of the value text.
 * Responsibility for the @c text argument does not change, and the value object does not become sensitive to change
 * via the @c text pointer.  Unlike @c cif_value_init_char(), this function is suitable for for initialization text
 * that resides on the stack (e.g. in a local array variable) or in read-only memory.
 *
 * @param[in,out] value a pointer to the value object to be initialized; must not be NULL
 *
 * @param[in] text the new text content for the specified value object; must not be NULL
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR ) on failure
 */
int cif_value_copy_char(
        /*@in@*/ /*@temp@*/ cif_value_t *value,
        /*@in@*/ /*@keep@*/ UChar *text
        );

/**
 * @brief Parses the specified Unicode string to produce a floating-point number and its standard uncertainty,
 *         recording them in the provided value object.
 *
 * For success, the entire input string must be successfully parsed.  On success (only), any previous contents
 * of the value object are released as if by @c cif_value_clean() ; otherwise the value object is not modified.
 * The value object takes ownership of the parsed text on success.
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
int cif_value_parse_numb(
        /*@in@*/ /*@temp@*/ cif_value_t *numb, 
        /*@in@*/ /*@keep@*/ UChar *text
        );

/**
 * @brief (Re-)initializes the given value as a number with the specified value and uncertainty, with a number of
 *     significant figures determined by the specified @p scale
 *
 * Any previous contents of the provided value are released, as if by @c cif_value_clean() .
 * 
 * The value's text representation will be formatted with a standard uncertainty if and only if the given standard
 * uncertainty (@p su ) is greater than zero.  It is an error for the uncertainty to be less than zero.
 *
 * The @p scale gives the number of significant digits to the right of the decimal point; the value and uncertainty
 * will be rounded or extended to this number of decimal places as needed.  Any rounding is performed according to the
 * floating-point rounding mode in effect at that time.  The scale may be less than zero, indicating that some of the
 * digits to the left of the decimal point are insignificant and not to be recorded.  Care is required with this
 * parameter: if the given su rounds to exactly zero at the specified scale, then the resulting number object is
 * indistinguishable from one representing an exact number.
 *
 * If the scale is greater than zero or if pure decimal notation would require more than the specified number of
 * leading zeroes after the decimal point, then the number is formatted in scientific notation (d.ddde+-dd).  (The
 * number of digits of exponent may vary.)  Otherwise, it is formatted in decimal notation.
 *
 * It is an error if a text representation consistent with the arguments would require more characters than the
 * CIF 2.0 maximum line length (2048 characters).
 *
 * The behavior of this function is constrained by the characteristics of the data type (@c double ) of the 
 * @p value and @p su arguments.  Behavior is undefined if a scale is specified that exceeds the maximum possible
 * precision of a value of type double, or such that @c 10^(-scale) is greater than the greatest representable finite
 * double.
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
int cif_value_init_numb(
        /*@in@*/ /*@temp@*/ cif_value_t *numb, 
        double val, 
        double su, 
        int scale, 
        int max_leading_zeroes
        );

/**
 * @brief (Re-)initializes the given value as a number with the specified value and uncertainty, automatically
 *         choosing the number of significant digits to keep based on the specified @p su and @p su_rule.
 *
 * Any previous contents of the provided value are released, as if by cif_value_clean().
 * 
 * The number's text representation will be formatted with a standard uncertainty if and only if the given su is
 * greater than zero.  In that case, the provided su_rule governs the number of decimal digits with which the value
 * text is formatted, and the number of digits of value and su that are recorded.  The largest scale is chosen such
 * that the significant digits of the rounded su, interpreted as an integer, are less than or equal to the su_rule.
 * The most commonly used su_rule is probably 19, but others are used as well, including at least 9, 27, 28, 29, and 99.
 * The su_rule must be at least 9, and behavior is undefined if its decimal representation has more significant digits
 * than the implementation-defined maximum decimal precision of a double.
 * 
 * If the su is exactly zero then all significant digits of the value (and possibly more) are formatted.  The su_rule
 * is ignored in this case, and no su is included in the text representation.
 * 
 * The value's text representation is expressed in scientific notation if it would otherwise have more than five
 * leading zeroes or any trailing insignificant zeroes.  Otherwise, it is expressed in plain decimal notation.
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
int cif_value_autoinit_numb(
        /*@in@*/ /*@temp@*/ cif_value_t *numb, 
        double val, 
        double su, 
        unsigned int su_rule
        );

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
cif_kind_t cif_value_kind(
        /*@in@*/ /*@temp@*/ cif_value_t *value
        );

/**
 * @brief Returns the value represented by the given number value object.
 *
 * Behavior is undefined if the provided object is not a valid, initialized number value object.  Unlike most CIF
 * functions, the return value is not an error code -- no errors are detected.
 *
 * @param[in] numb a pointer the the value object whose numeric value is requested
 *
 * @return Returns the numeric value of the value object, as a @c double
 */
double cif_value_as_double(
        /*@in@*/ /*@temp@*/ cif_value_t *numb
        );

/**
 * @brief Returns the uncertainty of the value represented by the given number value object.
 *
 * The uncertainty is zero for an exact number.  Behavior is undefined if the provided object is not a valid,
 * initialized number value object.  Unlike most CIF functions, the return value is not an error code -- no errors are
 * detected.
 *
 * @param[in] numb a pointer the the value object whose numeric standard uncertainty is requested
 *
 * @return Returns the standard uncertainty of the value object, as a @c double
 */
double cif_value_su_as_double(
        /*@in@*/ /*@temp@*/ cif_value_t *numb
        );

/**
 * @brief Retrieves the text representation of the specified value.
 *
 * It is important to understand that the "text representation" provided by this function is not the same as the
 * CIF format representation in which the same value might be serialized to a CIF file.  Instead, it is the @em parsed
 * text representation, so, among other things, it omits any CIF delimiters and might not adhere to CIF line length
 * limits.
 * 
 * This function is natural for values of kind @c CIF_CHAR_KIND.  It is also fairly natural for values of kind
 * @c CIF_NUMB_KIND, for those carry a text representation with them to allow for undelimited number-like values
 * that nevertheless are intended to be interpreted as text.  It is natural, but trivial, for kinds @c CIF_NA_KIND
 * and @c CIF_UNK_KIND, for which the text representation is NULL (as opposed to empty).  It is decidedly unnatural,
 * however, for the composite value kinds @c CIF_LIST_KIND and @c CIF_TABLE_KIND.  As such, the text for values of
 * those kinds is also NULL, though that is subject to change in the future.
 *
 * @param[in] value a pointer to the value object whose text is requested
 *
 * @param[in,out] text the location where a copy of the value text (or NULL, as appropriate) should be recorded; must
 *         not be NULL
 *
 * @return Returns @c CIF_OK on success, or @c CIF_ERROR on failure
 */
int cif_value_get_text(
        /*@temp@*/ cif_value_t *value,
        /*@out@*/ UChar **text
        );

/**
 * @brief Determines the number of elements of a composite data value.
 *
 * Composite values are those having kind @c CIF_LIST_KIND or @c CIF_TABLE_KIND .  For the purposes of this function,
 * the "elements" of a table are its key/value @em pairs .  Only the direct elements of a list or table are counted,
 * not the elements of any lists or tables nested within.
 *
 * @param[in] value a pointer to the value object whose elements are to be counted; must not be NULL
 *
 * @param[out] count the number of elements
 *
 * @return @c CIF_OK if the value has kind @c CIF_LIST_KIND or @c CIF_TABLE_KIND, otherwise @c CIF_ARGUMENT_ERROR
 */
int cif_value_get_element_count(
        /*@temp@*/ cif_value_t *value,
        /*@out@*/ size_t *count);

/**
 * @brief Retrieves an element of a list value by its index.
 *
 * A pointer to the actual value object contained in the list is returned, so modifications to the value will be
 * reflected in the list.  If that is not desired then the caller can make a copy via @c cif_value_clone() .  The
 * caller must not free the returned value, but may modify it in any other way.
 *
 * @param[in] value a pointer to the value from which to retrieve an element; must not be a pointer to a value of
 *         kind @c CIF_LIST_KIND
 *
 * @param[in] index the zero-based index of the requested element; must be less than the number of elements in the list
 *
 * @param[out] element the location where a pointer to the requested value should be written; must not be NULL
 *
 * @return Returns @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_LIST_KIND; otherwise returns
 *     @c CIF_OK if @c index is less than the number of elements in the list, else @c CIF_INVALID_INDEX .
 */
int cif_value_get_element_at(cif_value_t *value, size_t index, cif_value_t **element);

/**
 * @brief Replaces an existing element of a list value with a different value.
 *
 * The provided value is @em copied into the list (if not NULL); responsibility for the original object is not
 * transferred.  The replaced value is freed and discarded, except as described next.
 * 
 * Special case: if the replacement value is the same object as the one currently at the specified position in the
 * list, then this function succeeds without changing anything.
 *
 * @param[in,out] value a pointer to the list value to modify
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
 */
int cif_value_set_element_at(cif_value_t *value, size_t index, cif_value_t *element);

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
 */
int cif_value_insert_element_at(cif_value_t *value, size_t index, cif_value_t *element);

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
 */
int cif_value_remove_element_at(cif_value_t *value, size_t index, cif_value_t **element);

/**
 * @brief Retrieves an array of the keys of a table value.
 *
 * The caller assumes responsibility for freeing the returned array itself, but @em not the keys in it.  The keys
 * remain the responsibility of the table, and @b MUST @b NOT be freed or modified in any way.  Each key is a
 * NUL-terminated Unicode string, and the end of the list is marked by a NULL element.
 *
 * @param[in] table a pointer to the table value whose keys are requested
 *
 * @param[out] keys the location where a pointer to the array of keys should be written; must not be NULL.
 *
 * @return Returns a status code characteristic of the result:
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_TABLE_KIND , otherwise
 *         @li @c CIF_OK if the keys were successfully retrieved, or
 *         @li an error code, typically @c CIF_ERROR , if retrieving the keys fails for reasons other than those already
 *             described
 */
int cif_value_get_keys(
        /*@temp@*/ cif_value_t *table,
        UChar ***keys
        );

/**
 * @brief Associates a specified value and key in the provided table value.
 *
 * The value and key are copied into the table; responsibility for the originals does not transfer.  Any value
 * previously associated with the given key is automatically freed, except as described next.
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
 *         @li @c CIF_ARGUMENT_ERROR if the @c value has kind different from @c CIF_TABLE_KIND , otherwise
 *         @li @c CIF_INVALID_INDEX if the given key does not meet CIF validity criteria
 *         @li @c CIF_OK if the entry was successfully set / updated, or
 *         @li an error code, typically @c CIF_ERROR , if setting or updating the entry fails for reasons other than
 *             those already described
 */
int cif_value_set_item_by_key(
        /*@temp@*/ cif_value_t *table, 
        /*@temp@*/ const UChar *key, 
        /*@temp@*/ cif_value_t *item
        );

/**
 * @brief Looks up a table entry by key and optionally returns the associated value.
 *
 * If the key is present in the table and the @p value parameter is non-NULL, then a pointer to the value associated
 * with the key is written where @p value points.  Otherwise, this function simply indicates via its return value
 * whether the specified key is present in the table.  Any value object provided via the @p value parameter continues
 * to be owned by the table.  It must not be freed, and any modifications to it will be reflected in the table.  If
 * that is not desired then the caller can make a copy of the value via @c cif_value_clone() .
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
 */
int cif_value_get_item_by_key(
        /*@temp@*/ cif_value_t *table, 
        /*@temp@*/ const UChar *key, 
        cif_value_t **value
        );

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
 */
int cif_value_remove_item_by_key(
        /*@temp@*/ cif_value_t *table, 
        /*@temp@*/ const UChar *key, 
        cif_value_t **value
        );

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* CIF_H */
