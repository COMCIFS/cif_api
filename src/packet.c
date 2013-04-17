/*
 * packet.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

/*
 * Note: most packet manipulation functions are defined in map.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "cif.h"
#include "internal/ciftypes.h"
#include "internal/utils.h"

#undef uthash_fatal
#define uthash_fatal(msg) DEFAULT_FAIL(soft)

#ifdef __cplusplus
extern "C" {
#endif

int cif_packet_create(cif_packet_t **packet, UChar **names) {
    /* This is just a name-normalizing wrapper around cif_packet_create_norm() */
    FAILURE_HANDLING;
    UChar **names_norm;
    UChar **next;
    size_t element_count;

    for (next = names; *next; next += 1) ;

    element_count = (size_t) (next - names);
    names_norm = (UChar **) malloc(sizeof(UChar *) * (element_count + 1));
    if (names_norm) {
        size_t counter;
        int result;

        for (counter = 0; counter < element_count; counter += 1) {
            result = cif_normalize_item_name(names[counter], -1, names_norm + counter, CIF_INVALID_ITEMNAME);
            if (result != CIF_OK) FAIL(soft, result);
        }
        names_norm[element_count] = NULL;

        result = cif_packet_create_norm(packet, names_norm, 0);
        if (result == CIF_OK) {
            /* we're about to abandon our copies of the name pointers; they now belong to the packet */
            (*packet)->map.is_standalone = 1;
            /* free the array, but not its elements */
            free(names_norm);
            return CIF_OK;
        }

        SET_RESULT(result);

        FAILURE_HANDLER(soft):
        /* counter points one past the last allocated element of names_norm */
        while (counter > 0) {
            free(names_norm[--counter]);
        }
        free(names_norm);
    }

    FAILURE_TERMINUS;
}

int cif_packet_create_norm(cif_packet_t **packet, UChar **names, int standalone) {
    FAILURE_HANDLING;

    if (!names || !packet) {
        SET_RESULT(CIF_ARGUMENT_ERROR);
    } else {
        cif_packet_t *temp_packet = (cif_packet_t *) malloc(sizeof(cif_packet_t));

        if (temp_packet) {
            /*@temp@*/ UChar **name;

            temp_packet->map.is_standalone = standalone;
            temp_packet->map.head = NULL;
            for (name = names; *name; name += 1) {
                struct entry_s *scalar = (struct entry_s *) malloc(sizeof(struct entry_s));

                if (!scalar) {
                    DEFAULT_FAIL(soft);
                } else {
                    scalar->as_value.kind = CIF_UNK_KIND;
                    if (standalone != 0) {
                        scalar->key = cif_u_strdup(*name);
                        if (scalar->key == NULL) DEFAULT_FAIL(soft);
                    } else {
                        scalar->key = *name;
                    }
                    HASH_ADD_KEYPTR(hh, temp_packet->map.head, scalar->key, U_BYTES(scalar->key), scalar);
                }
            }

            /* Packet is successfully initialized with dummy values */
            *packet = temp_packet;
            return CIF_OK;

            FAILURE_HANDLER(soft):
            (void) cif_packet_free(temp_packet);
        }
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
}
#endif

