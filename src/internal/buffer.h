/*
 * buffer.h
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 *
 * This is an internal CIF API header file, intended for use only in building the CIF API library.  It is not needed
 * to build client applications, and it is intended to *not* be installed on target systems.
 */

#ifndef INTERNAL_BUFFER_H
#define INTERNAL_BUFFER_H

#include <sys/types.h>

typedef struct {
    const char *start;
    size_t position;
    size_t limit;
    size_t capacity;
} read_buffer_tp;

typedef struct {
    char *start;
    size_t position;
    size_t limit;
    size_t capacity;
} write_buffer_tp;

/*
 * The point of separating read_buffer_tp from write_buffer_tp is to allow an
 * object of the former type to be wrapped around a const data source, and
 * the point of union buffer_tp is to support flipping a write buffer for
 * reading.  The result aliases for_writing.start with
 * (const) for_reading.start, however, and that presents a potential
 * (not realized anywhere as far as I know) for high levels of optimization
 * to break the code.  In addition, it opens the possibility for subtle
 * errors: a write buffer may be flipped for reading (once, at least), but
 * a read buffer must not be flipped for writing.
 */
typedef union {
    read_buffer_tp for_reading;
    write_buffer_tp for_writing;
} buffer_tp;

/*
 * Frees a read_buffer_tp or a write_buffer_tp structure WITHOUT freeing the raw
 * buffered data (member 'start' of either type).
 */
#define cif_buf_free_metadata(buf) do { \
  buffer_tp *b = (buf); \
  if (b) { \
    free(b); \
  } \
} while (0)

#endif

