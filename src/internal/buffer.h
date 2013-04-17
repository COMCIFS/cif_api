/*
 * buffer.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifndef INTERNAL_BUFFER_H
#define INTERNAL_BUFFER_H

#include <sys/types.h>

typedef struct {
    const char *start;
    size_t position;
    size_t limit;
    size_t capacity;
} read_buffer_t;

typedef struct {
    char *start;
    size_t position;
    size_t limit;
    size_t capacity;
} write_buffer_t;

/*
 * The point of separating read_buffer_t from write_buffer_t is to allow an
 * object of the former type to be wrapped around a const data source, and
 * the point of union buffer_t is to support flipping a write buffer for
 * reading.  The result aliases for_writing.start with
 * (const) for_reading.start, however, and that presents a potential
 * (not realized anywhere as far as I know) for high levels of optimization
 * to break the code.  In addition, it opens the possibility for subtle
 * errors: a write buffer may be flipped for reading (once, at least), but
 * a read buffer must not be flipped for writing.
 */
typedef union {
    read_buffer_t for_reading;
    write_buffer_t for_writing;
} buffer_t;

#define cif_buf_free_metadata(buf) do { \
  buffer_t *b = (buf); \
  if (b) { \
    free(b->for_writing.start); \
    free(b); \
  } \
} while (0)

#endif

