/*
 * ciftypes.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 *
 * This is an internal CIF API header file, intended for use only in building the CIF API library.  It is not needed
 * to build client applications, and it is intended to *not* be installed on target systems.
 */

#ifndef INTERNAL_CIFTYPES_H
#define INTERNAL_CIFTYPES_H

#include <unicode/ustring.h>
#include <sqlite3.h>
#include "uthash.h"
#include "../cif.h"

/* The number of entries in the scanner's character class table */
#define CHAR_TABLE_MAX   160

/* A replacement character for use with CIF2 */
#define REPL_CHAR     0xFFFD
/* A replacement character for use with CIF1 ('*') */
#define REPL1_CHAR    0x2A

/* scanner character class codes */
/* code NO_CLASS must have value 0; the values of other codes may be changed as desired */
#define NO_CLASS      0
#define GENERAL_CLASS 1
#define WS_CLASS      2
#define EOL_CLASS     3
#define EOF_CLASS     4
#define HASH_CLASS    5
#define UNDERSC_CLASS 6
#define QUOTE_CLASS   7
#define SEMI_CLASS    9
#define OBRAK_CLASS  10
#define CBRAK_CLASS  11
#define OCURL_CLASS  12
#define CCURL_CLASS  13
/* class 14 is available */
#define DOLLAR_CLASS 15
#define OBRAK1_CLASS 16
#define CBRAK1_CLASS 17
#define A_CLASS      18
#define B_CLASS      19
#define D_CLASS      20
#define E_CLASS      21
#define G_CLASS      22
#define L_CLASS      23
#define O_CLASS      24
#define P_CLASS      25
#define S_CLASS      26
#define T_CLASS      27
#define V_CLASS      28

/* identifies the numerically-last class code, but does not itself directly represent a class */
#define LAST_CLASS   V_CLASS

/*
 * A pointer to a function that normalizes names given in Unicode string form.  The specific nature and purpose of the
 * normalization performed is specific to the function.
 *
 * name: the name to be normalized, as a Unicode string
 * namelen: the number of characters of 'name' to use; if less than zero, then all characters are used up to the first
 *     NUL character; it is slightly more efficient to pass the actual length of the name instead of a negative number
 *     if the length is already known
 * normalized_name: if non-NULL, the location where a pointer to the normalized, NUL-terminated result should be
 *     recorded in the event of a successful normalization (including of an already-normalized input).  Any previous
 *     value is overwritten. If NULL then the result is discarded, but the return code still indicates whether
 *     normalization was successful
 * invalidityCode: the error code to return if the name is found invalid
 *
 * returns CIF_OK if normalization is successful, invalidityCode if the input name is found invalid, or CIF_ERROR if
 *     validation/normalization fails
 */
typedef int (*name_normalizer_f)(const UChar *name, int32_t namelen, UChar **normalized_name, int invalidityCode);

/* a whole CIF */

struct cif_s {
   sqlite3 *db;
   sqlite3_stmt *create_block_stmt;
   sqlite3_stmt *get_block_stmt;
   sqlite3_stmt *get_all_blocks_stmt;
   sqlite3_stmt *create_frame_stmt;
   sqlite3_stmt *get_frame_stmt;
   sqlite3_stmt *get_all_frames_stmt;
   sqlite3_stmt *destroy_container_stmt;
   sqlite3_stmt *validate_container_stmt;
   sqlite3_stmt *create_loop_stmt;
   sqlite3_stmt *get_loopnum_stmt;
   sqlite3_stmt *set_loop_category_stmt;
   sqlite3_stmt *add_loop_item_stmt;
   sqlite3_stmt *get_cat_loop_stmt;
   sqlite3_stmt *get_item_loop_stmt;
   sqlite3_stmt *get_all_loops_stmt;
   sqlite3_stmt *prune_container_stmt;
   sqlite3_stmt *get_value_stmt;
   sqlite3_stmt *set_all_values_stmt;
   sqlite3_stmt *get_loop_size_stmt;
   sqlite3_stmt *remove_item_stmt;
   sqlite3_stmt *destroy_loop_stmt;
   sqlite3_stmt *get_loop_names_stmt;
   sqlite3_stmt *max_packet_num_stmt;
   sqlite3_stmt *check_item_loop_stmt;
   sqlite3_stmt *insert_value_stmt;
   sqlite3_stmt *update_packet_item_stmt;
   sqlite3_stmt *insert_packet_item_stmt;
   sqlite3_stmt *remove_packet_stmt;
};

/* data containers block and frame */

struct cif_container_s {
    cif_t *cif;
    sqlite_int64 id;
    UChar *code;
    UChar *code_orig;
    sqlite_int64 block_id;
};

/* loops */

struct cif_loop_s {
    cif_container_t *container;
    int loop_num;
    UChar *category;
    UChar **names;
};

/* loop packets */

struct set_element_s {
    /* no data payload */
    UT_hash_handle hh;
};

struct entry_s;

/*
 * cif_map_t provides an internal shared implementation of packets and table values.  Separate
 * (and non-identical) types are exposed in the public API for those different uses of this
 * underlying structure.
 */
typedef struct cif_map_s {
    struct entry_s *head;  /* must be initialized to NULL! */
    int is_standalone; /* In (only) standalone maps, the keys belong to the entries */
    name_normalizer_f normalizer;
} cif_map_t;

struct cif_packet_s {
    cif_map_t map;
};

/*
 * A packet iterator encapsulates the internal state involved in stepping through the
 * packets of a loop
 */
struct cif_pktitr_s {
    sqlite3_stmt *stmt;
    cif_loop_t *loop;
    UChar **item_names;
    struct set_element_s *name_set;
    int previous_row_num;
    int finished;
};

/* values */

/* IMPORTANT: the order of the members of the following value structures is significant. */

typedef struct char_value_s {
    cif_kind_t kind;  /* expected: CIF_CHAR_KIND */
    UChar *text;
} cif_char_t;

typedef struct numb_value_s {
    cif_kind_t kind;  /* expected: CIF_NUMB_KIND */
    UChar *text;
    int sign;         /* expected: +-1 */
    /*
     * digit strings are expressed in the C locale.   They are expressed without leading
     * zeroes, except that exact zero values and sus correspond to the digit string
     * consisting of a single zero.
     */
    char *digits;
    char *su_digits;
    int scale;
} cif_numb_t;

struct list_element_s;

typedef struct list_value_s {
    cif_kind_t kind;  /* expected: CIF_LIST_KIND */
    cif_value_t **elements;
    size_t size;
    size_t capacity;
} cif_list_t;

typedef struct table_value_s {
    cif_kind_t kind;  /* expected: CIF_TABLE_KIND */
    cif_map_t map;
} cif_table_t;

union cif_value_u {
    cif_kind_t kind;
    cif_char_t as_char;
    cif_numb_t as_numb;
    cif_list_t as_list;
    cif_table_t as_table;
    /* nothing (else) for kinds CIF_NA_KIND and CIF_UNK_KIND */
};

struct list_element_s {
    cif_value_t as_value; /* not a pointer */
    struct list_element_s *next;
};

struct entry_s {
    cif_value_t as_value; /* not a pointer; MUST BE FIRST */
    UChar *key;
    UChar *key_orig;
    UT_hash_handle hh;
};

/*
 * A node in a singly-linked list of Unicode strings
 */
typedef struct string_el {
    struct string_el *next;
    UChar *string;
} string_element_t;

/*
 * reads characters from the specified source, of a runtime type appropriate for the pointed-to function, into the
 * specified destination buffer.  Up to 'count' characters are read.
 *
 * Returns the number of characters transferred to the destination buffer, or a number less than zero if an error
 * occurs. In the event of a negative return code, a suggested CIF return code to pass to the caller is recorded
 * where 'error_code' points.
 *
 * If 'count' is nonpositive then zero is returned; otherwise, zero is returned only when no more characters are
 * available from the specified source.
 */
typedef ssize_t (*read_chars_f)(void *char_source, UChar *dest, ssize_t count, int *error_code);

/* parser semantic token types */
enum token_type {
    BLOCK_HEAD, FRAME_HEAD, FRAME_TERM, LOOPKW, NAME, OTABLE, CTABLE, OLIST, CLIST, KEY, TKEY, VALUE, QVALUE, TVALUE,
    END, ERROR
};

/*
 * Tracks state of the built-in CIF scanner as it progresses through a CIF
 */
struct scanner_s {
    UChar *buffer;          /* A character buffer from which to scan characters */
    size_t buffer_size;     /* The total size of the buffer */
    size_t buffer_limit;    /* The size of the initial segment of the buffer containing valid characters */
    UChar *next_char;       /* A pointer into the buffer to the next character to be scanned */

    enum token_type ttype;  /* The grammatic category of the most recent token parsed */
    UChar *text_start;      /* The start position of the text from which the current token, if any, was parsed.
                               This may differ from tvalue_start in some cases, such as for tokens representing
                               delimited values. */
    UChar *tvalue_start;    /* The start position of the value of the current token, if any */
    size_t tvalue_length;   /* The length of the value of the current token, if any */

    size_t line;            /* The current 1-based input line number */
    unsigned column;        /* The number of characters so far scanned from the current line */

    /* character classification tables */
    unsigned int char_class[CHAR_TABLE_MAX];
    unsigned int meta_class[LAST_CLASS + 1];

    /* character source */
    void *char_source;
    read_chars_f read_func;
    int at_eof;

    /* cif version */
    int cif_version;

    /* custom parse options */
    int line_unfolding;
    int prefix_removing;

    /* user callback support */
    cif_handler_t *handler;
    cif_parse_error_callback_t error_callback;
    cif_whitespace_callback_t whitespace_callback;
    void *user_data;

    /*
     * Used internally to supports navigational control via caller-provided CIF handlers.
     *
     * skip_depth == 0 means all elements parsed will be handled normally
     * skip_depth == 1 means the current element's children will be skipped (and maybe the current element itself,
     *     depending on when this depth is assigned)
     * skip_depth == 2 means the current element's children and subsequent siblings will be skipped (and maybe the
     *     current element itself, depending on when this depth is assigned)
     * skip_depth == 3 means the current element's children and subsequent siblings will be skipped, as well as any
     *     of its parent's siblings that have not yet been parsed; should only be encountered when the current element
     *     is itself being skipped.
     * etc.
     */
    int skip_depth;
};

#endif /* INTERNAL_CIFTYPES_H */

