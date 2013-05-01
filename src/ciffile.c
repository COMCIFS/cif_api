/*
 * ciffile.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cif.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a CIF from the specified stream.  If the 'cif' argument is non-NULL
 * then a handle for the parsed CIF is written to its referrent.  Otherwise,
 * the parsed results are discarded, but the return code still indicates whether
 * parsing was successful.
 */
int cif_parse(FILE *stream, struct cif_parse_opts_s *options, cif_t **cif) {
    /* TODO */
    return CIF_NOT_SUPPORTED;
}

/*
 * Formats the CIF data represented by the 'cif' handle to the specified
 * output.
 */
int cif_write(FILE *stream, cif_t *cif) {
    /* TODO */
    return CIF_NOT_SUPPORTED;
}

#ifdef __cplusplus
}
#endif

