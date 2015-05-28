/*
 * buffer.h
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

/*
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

