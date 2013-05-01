/*
 * ciftypes.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifndef INTERNAL_CIFTYPES_H
#define INTERNAL_CIFTYPES_H

#include <unicode/ustring.h>
#include <sqlite3.h>
#include "uthash.h"
#include "../cif.h"

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
typedef int (*name_normalizer_f)(/*@in@*/ /*@temp@*/ const UChar *name, int32_t namelen,
        /*@in@*/ /*@temp@*/ UChar **normalized_name, int invalidityCode) /*@modifies *normalized_name@*/;

/* a whole CIF */

struct cif_s {
   /*@only@*/ sqlite3 *db;
   /*@only@*/ /*@null@*/ sqlite3_stmt *create_block_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_block_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_all_blocks_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *create_frame_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_frame_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_all_frames_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *destroy_container_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *create_loop_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_loopnum_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *add_loopitem_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_cat_loop_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_item_loop_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_all_loops_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_value_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *set_all_values_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_loop_size_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *remove_item_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *destroy_loop_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *get_loop_names_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *add_loop_item_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *max_packet_num_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *check_item_loop_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *insert_value_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *update_packet_item_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *insert_packet_item_stmt;
   /*@only@*/ /*@null@*/ sqlite3_stmt *remove_packet_stmt;
};

/* data containers block and frame */

struct cif_container_s {
    /*@shared@*/ cif_t *cif;
    sqlite_int64 id;
    /*@only@*/ UChar *code;
    /*@only@*/ UChar *code_orig;
    sqlite_int64 block_id;
};

/* loops */

struct cif_loop_s {
  /*@shared@*/ cif_container_t *container;
  int loop_num;
  /*@only@*/ /*@null@*/ UChar *category;
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
    /*@only@*/ /*@null@*/ struct entry_s *head;  /* must be initialized to NULL! */
    int is_standalone; /* In (only) standalone maps, the keys belong to the entries */
    /*@shared@*/ name_normalizer_f normalizer;
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
    /*@shared@*/ cif_loop_t *loop;
    /*@only@*/ UChar **item_names;
    struct set_element_s *name_set;
    int previous_row_num;
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
    /*@null@*/ char *su_digits;
    int scale;
} cif_numb_t;

struct list_element_s;

typedef struct list_value_s {
    cif_kind_t kind;  /* expected: CIF_LIST_KIND */
    /*@only@*/ /*@null@*/ cif_value_t **elements;
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
    /*@shared@*/ /*@null@*/ struct list_element_s *next;
};

struct entry_s {
    cif_value_t as_value; /* not a pointer; MUST BE FIRST */
    UChar *key;
    UT_hash_handle hh;
};

#endif /* INTERNAL_CIFTYPES_H */

