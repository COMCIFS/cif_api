/*
 * messages.h
 *
 * Copyright (C) 2014 John C. Bollinger
 *
 * All rights reserved.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

const char messages[][80] = {
    /* CIF_OK                  0 */ "no error",
    /* CIF_FINISHED            1 */ "iteration finished",
    /* CIF_ERROR               2 */ "unspecified error",
    /* CIF_INVALID_HANDLE      3 */ "invalid object handle provided",
    /* CIF_INTERNAL_ERROR      4 */ "CIF API internal error",
    /* CIF_ARGUMENT_ERROR      5 */ "invalid function argument",
    /* CIF_MISUSE              6 */ "improper CIF API use",
    /* CIF_NOT_SUPPORTED       7 */ "feature not supported",
    /* CIF_ENVIRONMENT_ERROR   8 */ "wrong or inadequate operating environment",
    /* CIF_CLIENT_ERROR        9 */ "application-directed error",
    "",
    /* CIF_DUP_BLOCKCODE      11 */ "duplicate data block code",
    /* CIF_INVALID_BLOCKCODE  12 */ "invalid data block code",
    /* CIF_NOSUCH_BLOCK       13 */ "no data block exists for the specified code",
    "", "", "", "", "", "", "",
    /* CIF_DUP_BLOCKCODE      21 */ "duplicate save frame code",
    /* CIF_INVALID_BLOCKCODE  22 */ "invalid save frame code",
    /* CIF_NOSUCH_BLOCK       23 */ "no save frame exists for the specified code and data block",
    "", "", "", "", "", "", "",
    /* CIF_CAT_NOT_UNIQUE     31 */ "the specified category does not uniquely identify a loop",
    /* CIF_INVALID_CATEGORY   32 */ "the specified category identifier is invalid",
    /* CIF_NOSUCH_LOOP        33 */ "no loop is assigned to the specified category",
    /* CIF_RESERVED_LOOP      34 */ "the scalar loop may not be manipulated as requested",
    /* CIF_WRONG_LOOP         35 */ "the specified item does not belong to the specified loop",
    /* CIF_EMPTY_LOOP         36 */ "loop with no data",
    /* CIF_NULL_LOOP          37 */ "loop with no data names",
    "", "", "",
    /* CIF_DUP_ITEMNAME       41 */ "duplicate item name",
    /* CIF_INVALID_ITEMNAME   42 */ "invalid item name",
    /* CIF_NOSUCH_ITEM        43 */ "no item bears the specified name",
    /* CIF_AMBIGUOUS_ITEM     44 */ "only one of the specified item's several values is provided",
    "", "", "", "", "", "", "",
    /* CIF_INVALID_PACKET     52 */ "the provided packet object is not valid for the requested operation",
    /* CIF_PARTIAL_PACKET     53 */ "too few values in the last packet of a loop",
    "", "", "", "", "", "", "", "",
    /*
     * Note: in principle, error code CIF_DISALLOWED_VALUE is more general than the message string describes, but
     * in practice, it only occurs in the narrower context described by the message.
     */
    /* CIF_DISALLOWED_VALUE   62 */ "wrong value type for a table index",
    "", "", "", "", "", "", "", "", "",
    /* CIF_INVALID_NUMBER     72 */ "the specified string could not be parsed as a number",
    /* CIF_INVALID_INDEX      73 */ "the specified string is not a valid table index",
    /* CIF_INVALID_BARE_VALUE 74 */ "a data value that must be quoted was encountered bare",
    "", "", "", "", "", "", /* 80 */
    "", "", "", "", "", "", "", "", "", "", /* 90 */
    "", "", "", "", "", "", "", "", "", "", /* 100 */
    "",
    /* CIF_INVALID_CHAR      102 */ "invalid encoded character",
    /* CIF_UNMAPPED_CHAR     103 */ "unmappable character",
    /* CIF_DISALLOWED_CHAR   104 */ "encountered a character that is not allowed to appear in CIF",
    /* CIF_MISSING_SPACE     105 */ "required whitespace separation was missing",
    /* CIF_MISSING_ENDQUOTE  106 */ "encountered an un-terminated inline quoted string",
    /* CIF_UNCLOSED_TEXT     107 */ "a multi-line string was not terminated before the end of the file",
    /* CIF_OVERLENGTH_LINE   108 */ "encountered an input line exceeding the length limit",
    /* CIF_DISALLOWED_INITIAL_CHAR 109 */ "the first character of the input is disallowed at that position",
    /* CIF_WRONG_ENCODING    110 */ "while parsing CIF 2.0, the character encoding is not UTF-8",
    "", "",
    /* CIF_NO_BLOCK_HEADER   113 */ "non-whitespace was encountered outside any data block",
    "", "", "", "", "", "", "", "",
    /* CIF_FRAME_NOT_ALLOWED 122 */ "a save frame was encountered, but save frame parsing is disabled",
    /* CIF_NO_FRAME_TERM     123 */ "a save frame terminator is missing at the apparent end of a frame",
    /* CIF_UNEXPECTED_TERM   124 */ "a save frame terminator was encountered where none was expected",
    "",
    /* CIF_EOF_IN_FRAME      126 */ "the end of the input was encountered inside a save frame",
    "", "", "", "", "",
    /* CIF_RESERVED_WORD     132 */ "a CIF reserved word was encountered as an unquoted data value",
    /* CIF_MISSING_VALUE     133 */ "missing data value",
    /* CIF_UNEXPECTED_VALUE  134 */ "unexpected data value",
    /* CIF_UNEXPECTED_DELIM  135 */ "misplaced list or table delimiter",
    /* CIF_MISSING_DELIM     136 */ "missing list or table delimiter",
    /* CIF_MISSING_KEY       137 */ "missing a key for a table entry",
    /* CIF_UNQUOTED_KEY      138 */ "an unquoted table key was encountered",
    /* CIF_MISQUOTED_KEY     139 */ "encountered a text block used as a table key",
    /* CIF_NULL_KEY          140 */ "a null table key was encountered",
    /* CIF_MISSING_PREFIX    141 */ "a line of a prefixed text field body omitted the prefix"
};

#endif
