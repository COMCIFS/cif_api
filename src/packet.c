/*
 * packet.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
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
#include <assert.h>
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

    if (names == NULL) {
        element_count = 0;
    } else {
        for (next = names; *next; next += 1) ;
        element_count = (size_t) (next - names);
    }

    names_norm = (UChar **) malloc(sizeof(UChar *) * (element_count + 1));
    if (names_norm != NULL) {
        size_t counter;
        int result;

        for (counter = 0; counter < element_count; counter += 1) {
            result = cif_normalize_item_name(names[counter], -1, names_norm + counter, CIF_INVALID_ITEMNAME);
            if (result != CIF_OK) FAIL(soft, result);
        }
        names_norm[element_count] = NULL;

        /* avoid making unneeded key copies by allowing the packet to alias the normalized keys */
        result = cif_packet_create_norm(packet, names_norm, CIF_FALSE);
        if (result == CIF_OK) {
            struct entry_s *entry;
            size_t counter2;

            /* assign the original item names */
            /* iteration via hh.next is documented to proceed in insertion order */
            for (counter2 = 0, entry = (*packet)->map.head;
                    counter2 < element_count;
                    counter2 += 1, entry = (struct entry_s *) entry->hh.next) {
                next = names + counter2;
                assert(entry != NULL);
                if (u_strcmp(*next, entry->key) != 0) {
                    assert (entry->key_orig == entry->key);  /* implementation detail of cif_packet_create_norm() */
                    entry->key_orig = cif_u_strdup(*next);

                    if (entry->key_orig == NULL) {
                        cif_packet_free(*packet);
                        DEFAULT_FAIL(soft);
                    }
                }
            }

            /* we're about to abandon our copies of the name pointers; the names now belong exclusively to the packet */
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

/*
 * FIXME: the need for this is unclear, especially in its current form.  The
 * objective is to provide for avoiding unneeded multiple normalization, but
 * this function is hard to use because it does not record the original item
 * names; instead, it just sets them to the (provided) normalized names.
 */
int cif_packet_create_norm(cif_packet_t **packet, UChar **names, int avoid_aliasing) {
    FAILURE_HANDLING;
    cif_packet_t *temp_packet;

    assert(names != NULL);
    assert(packet != NULL);

    temp_packet = (cif_packet_t *) malloc(sizeof(cif_packet_t));
    if (temp_packet != NULL) {
        UChar **name;

        temp_packet->map.normalizer = cif_normalize_item_name;
        temp_packet->map.is_standalone = avoid_aliasing;
        temp_packet->map.head = NULL;
        for (name = names; *name; name += 1) {
            struct entry_s *scalar = (struct entry_s *) malloc(sizeof(struct entry_s));

            if (scalar == NULL) {
                DEFAULT_FAIL(soft);
            } else {
                scalar->as_value.kind = CIF_UNK_KIND;
                if (avoid_aliasing == 0) {
                    scalar->key = *name;
                } else {
                    scalar->key = cif_u_strdup(*name);
                    if (scalar->key == NULL) DEFAULT_FAIL(soft);
                }
                scalar->key_orig = scalar->key;
                HASH_ADD_KEYPTR(hh, temp_packet->map.head, scalar->key, U_BYTES(scalar->key), scalar);
            }
        }

        /* Packet is successfully initialized with dummy values */
        *packet = temp_packet;
        return CIF_OK;

        FAILURE_HANDLER(soft):
        cif_packet_free(temp_packet);
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
}
#endif

