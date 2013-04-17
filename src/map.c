/*
 * map.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
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

static int cif_map_clean(cif_map_t *map) {
    if (map) {
        struct entry_s *entry;
        struct entry_s *temp;

        /* these hash operations do not use uthash_fatal(): */
        HASH_ITER(hh, map->head, entry, temp) {
            HASH_DEL(map->head, entry);
            (void) cif_value_clean(&(entry->as_value));
            if (map->is_standalone != 0) {
                free(entry->key);
            }
            free(entry);
        }
    }

    return CIF_OK;
}

static int cif_map_get_names(
        cif_map_t *map,
        UChar ***names
        ) {
    FAILURE_HANDLING;
    size_t name_count = (size_t) HASH_COUNT(map->head);
    UChar **temp = (UChar **) malloc(sizeof(UChar *) * (name_count + 1));

    if (temp != NULL) {
        struct entry_s *item;
        struct entry_s *temp_item;
        UChar **next = temp;

        HASH_ITER(hh, map->head, item, temp_item) {
            *next = item->key;
            next += 1;
        }
        *next = NULL;

        *names = temp;
        return CIF_OK;
    }

    FAILURE_TERMINUS;
}

/*
 * Converts maps to standalone (when they aren't already so) by replacing each item's name with a newly-allocated
 * copy of itself.  On failure the map remains unchanged.
 */
static int convert_to_standalone(cif_map_t *map) {
    FAILURE_HANDLING;

    if (map->is_standalone != 0) {
        return CIF_OK;
    } else {
        /*
         * Item name copies are initially recorded in a temporary array to avoid modifying the map until
         * complete success is certain.
         */
        size_t item_count = (size_t) HASH_COUNT(map->head);
        UChar **name_copies = (UChar **) malloc(sizeof(UChar *) * item_count);

        if (name_copies != NULL) {
            size_t next = 0;
            struct entry_s *item;
            struct entry_s *temp;

            /* Copy the names */
            HASH_ITER(hh, map->head, item, temp) {
                if (next >= item_count) FAIL(soft, CIF_INTERNAL_ERROR);
                name_copies[next] = cif_u_strdup(item->key);

                if (name_copies[next] != NULL) {
                    next += 1;
                } else {
                    DEFAULT_FAIL(soft);
                }
            }

            /* Update the entries with the name copies */
            next = 0;
            HASH_ITER(hh, map->head, item, temp) {
                if (next >= item_count) FAIL(soft, CIF_INTERNAL_ERROR);
                item->key = name_copies[next++];
            }

            /* success */
            map->is_standalone = 1;
            free(name_copies);
            return CIF_OK;

            FAILURE_HANDLER(soft):
            /* next points one past the last name copy that was allocated */
            while (next > 0) {
                free(name_copies[--next]);
            }
            free(name_copies);
        }
    }

    FAILURE_TERMINUS;
}

static int cif_map_set_item(
        cif_map_t *map,
        const UChar *name, 
        cif_value_t *value,
        int invalidity_code
        ) {
    /*
     * In the event that the specified name is new to the given map and the map is not already standalone,
     * a necessary side effect of this function is to convert the map to standalone.  This follows from the need
     * to normalize the name and store the normalized form, and from the fact that it isn't viable to mix owned and
     * shared item names in the same map.
     */
    FAILURE_HANDLING;
    UChar *name_norm;
    int result;

    result = (*map->normalizer)(name, -1, &name_norm, invalidity_code);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        size_t name_bytes = (size_t) U_BYTES(name_norm);
        struct entry_s *item;

        HASH_FIND(hh, map->head, name_norm, name_bytes, item);
        if (item) {
            /* There is an existing item for the given name */

            /*
             * Using a local pointer to the value enables the compiler (GCC, at least) to prove stronger assertions
             * about aliasing than otherwise it could do (for otherwise, there is a need to cast &item to
             * (cif_value_t **)).  This allows for stronger optimizations on the whole file.
             */
            cif_value_t *existing_value = &(item->as_value);

            /* replace the old value with a clone of the new */
            if ((cif_value_clean(existing_value) == CIF_OK)
                    && (cif_value_clone(value, &existing_value) == CIF_OK)) {
                free(name_norm);
                return CIF_OK;
            }

        } else if (convert_to_standalone(map) == CIF_OK) {
            /* This will be a new item for the map */

            item = (struct entry_s *) malloc(sizeof(struct entry_s));
            if (item) {
                /*
                 * Using a local pointer to the value enables the compiler (GCC, at least) to prove stronger
                 * assertions about aliasing than otherwise it could do (see above):
                 */
                cif_value_t *new_value = &(item->as_value);

                if (cif_value_clone(value, &new_value) == CIF_OK) {
                    item->key = name_norm;
                    HASH_ADD_KEYPTR(hh, map->head, item->key, name_bytes, item);
                    return CIF_OK;
                }

                /* referenced by the HASH_ADD_KEYPTR macro: */
                FAILURE_HANDLER(soft):
                free(item);
            }
        }

        free(name_norm);
    }

    FAILURE_TERMINUS;
}

/*
 * Looks up an item in the specified map, and optionally removes it.  The
 * item is returned through the 'value' pointer if that is not NULL.  In any
 * case, CIF_OK is returned if the named item was initially present; otherwise,
 * CIF_NOSUCH_ITEM is returned.
 */
static int cif_map_retrieve_item(
        cif_map_t *map, 
        const UChar *name, 
        cif_value_t **value, 
        int do_remove) {
    FAILURE_HANDLING;
    UChar *name_norm;
    int result;

    result = (*map->normalizer)(name, -1, &name_norm, CIF_INVALID_ITEMNAME);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        struct entry_s *item = NULL;

        HASH_FIND(hh, map->head, name_norm, U_BYTES(name_norm), item);
        free(name_norm);

        if (item) {
            if (do_remove != 0) {
                HASH_DEL(map->head, item);
                if (map->is_standalone != 0) {
                    free(item->key);
                }
            }
            if (value != NULL) {
                *value = &(item->as_value);
            } else if (do_remove) {
                (void) cif_value_free(&(item->as_value));
            }

            return CIF_OK;
        } else {
            SET_RESULT(CIF_NOSUCH_ITEM);
        }
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
extern "C" {
#endif

int cif_packet_free(cif_packet_t *packet) {
    if (packet) {
        (void) cif_map_clean(&(packet->map));
        free(packet);
    }
    return CIF_OK;
}

int cif_packet_get_names(
        cif_packet_t *packet,
        UChar ***names
        ) {
    return cif_map_get_names(&(packet->map), names);
}

int cif_packet_set_item(
        cif_packet_t *packet,
        const UChar *name, 
        cif_value_t *value
        ) {
    return cif_map_set_item(&(packet->map), name, value, CIF_INVALID_ITEMNAME);
}

int cif_packet_get_item(
        cif_packet_t *packet, 
        const UChar *name, 
        cif_value_t **value
        ) {
    return cif_map_retrieve_item(&(packet->map), name, value, 0);
}

int cif_packet_remove_item(
        cif_packet_t *packet, 
        const UChar *name, 
        cif_value_t **value
        ) {
    return cif_map_retrieve_item(&(packet->map), name, value, 1);
}

int cif_value_get_keys(
        cif_value_t *table,
        UChar ***keys
        ) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_get_names(&(table->as_table.map), keys);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_set_item_by_key(
        cif_value_t *table,
        const UChar *key, 
        cif_value_t *item
        ) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_set_item(&(table->as_table.map), key, item, CIF_ARGUMENT_ERROR);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_get_item_by_key(
        cif_value_t *table, 
        const UChar *name, 
        cif_value_t **value
        ) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_retrieve_item(&(table->as_table.map), name, value, 0);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_remove_item_by_key(
        cif_value_t *table, 
        const UChar *name, 
        cif_value_t **value
        ) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_retrieve_item(&(table->as_table.map), name, value, 1);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

#ifdef __cplusplus
}
#endif

