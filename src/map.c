/*
 * map.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
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

static int cif_map_clean(cif_map_t *map) {
    if (map != NULL) {
        struct entry_s *entry;
        struct entry_s *temp;

        /* these hash operations do not use uthash_fatal(): */
        HASH_ITER(hh, map->head, entry, temp) {
            HASH_DEL(map->head, entry);
            cif_map_entry_free_internal(entry, map);
        }
    }

    return CIF_OK;
}

static int cif_map_get_keys(cif_map_t *map, const UChar ***names) {
    FAILURE_HANDLING;
    size_t name_count = (size_t) HASH_COUNT(map->head);
    const UChar **temp = (const UChar **) malloc(sizeof(const UChar *) * (name_count + 1));

    if (temp != NULL) {
        struct entry_s *item;
        struct entry_s *temp_item;
        const UChar **next = temp;

        HASH_ITER(hh, map->head, item, temp_item) {
            *next = item->key_orig;
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
         * Item key copies are initially recorded in a temporary array to avoid modifying the map until
         * complete success is certain.
         */
        size_t item_count = (size_t) HASH_COUNT(map->head);
        UChar **key_copies = (UChar **) malloc(sizeof(UChar *) * item_count);

        if (key_copies != NULL) {
            size_t next = 0;
            struct entry_s *item;
            struct entry_s *temp;

            /* Copy the names */
            HASH_ITER(hh, map->head, item, temp) {
                if (next >= item_count) FAIL(soft, CIF_INTERNAL_ERROR);
                key_copies[next] = cif_u_strdup(item->key_orig);

                if (key_copies[next] != NULL) {
                    next += 1;
                } else {
                    DEFAULT_FAIL(soft);
                }
            }

            /* Update the entries with the name copies */
            next = 0;
            HASH_ITER(hh, map->head, item, temp) {
                if (next >= item_count) FAIL(soft, CIF_INTERNAL_ERROR);
                item->key_orig = key_copies[next++];
            }

            /* success */
            map->is_standalone = 1;
            free(key_copies);
            return CIF_OK;

            FAILURE_HANDLER(soft):
            /* next points one past the last name copy that was successfully allocated */
            while (next > 0) {
                free(key_copies[--next]);
            }
            free(key_copies);
        }
    }

    FAILURE_TERMINUS;
}

static int cif_map_set_item(cif_map_t *map, const UChar *key, cif_value_t *value, int invalidity_code) {
    /*
     * In the event that the specified key is new to the given map and the map is not already standalone,
     * a necessary side effect of this function is to convert the map to standalone.  This follows from the need
     * to normalize the key and store the normalized form, and from the fact that it isn't viable to mix owned and
     * shared item keys in the same map.
     */
    FAILURE_HANDLING;
    UChar *key_norm;
    int result;

    result = (*map->normalizer)(key, -1, &key_norm, invalidity_code);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        /* must ensure key_norm is freed or kept */
        size_t key_bytes = (size_t) U_BYTES(key_norm);
        struct entry_s *item;
        int different_key;

        HASH_FIND(hh, map->head, key_norm, key_bytes, item);
        /*
         * If the provided key is not identical to one of the existing _internal_ item keys, then the map must be
         * (made) standalone to proceed.
         */
        different_key = ((item == NULL) || (u_strcmp(key, item->key_orig) != 0));
        if ((different_key == 0) || (convert_to_standalone(map) == CIF_OK)) {
            if (item != NULL) {
                /* There is an existing item for the given key */
                UChar *key_orig = ((different_key == 0) ? item->key_orig : cif_u_strdup(key));

                if (key_orig != NULL) {
                    /*
                     * Using a local pointer to the value enables the compiler (GCC, at least) to prove stronger
                     * assertions about aliasing than otherwise it could do (for otherwise, there is a need to cast
                     * &item to (cif_value_t **)).  This allows for stronger optimizations on the whole file.
                     */
                    cif_value_t *existing_value = &(item->as_value);

                    if (key_orig != item->key_orig) {
                        assert(map->is_standalone != 0);
                        free(item->key_orig);
                        item->key_orig = key_orig;
                    }

                    /*
                     * Except if the value presented is the same object as is already in the map,
                     * make the existing entry's value a copy of the one presented
                     */
                    if ((value == existing_value)
                            || ((value == NULL) && (cif_value_clean(existing_value), CIF_TRUE))
                            || (cif_value_clone(value, &existing_value) == CIF_OK)) {
                        free(key_norm);
                        return CIF_OK;
                    }
                }
            } else {
                /* This will be a new item for the map */

                assert(map->is_standalone != 0);
                item = (struct entry_s *) malloc(sizeof(struct entry_s));
                if (item != NULL) {
                    /* The map is standalone, so we need to copy the original key */
                    UChar *key_copy = cif_u_strdup(key);

                    if (key_copy != NULL) {
                        /*
                         * Using a local pointer to the value enables the compiler (GCC, at least) to prove stronger
                         * assertions about aliasing than otherwise it could do (see above):
                         */
                        cif_value_t *new_value = &(item->as_value);

                        new_value->kind = CIF_UNK_KIND;
                        if ((value == NULL) || cif_value_clone(value, &new_value) == CIF_OK) {
                            item->key = key_norm;
                            item->key_orig = key_copy;
                            HASH_ADD_KEYPTR(hh, map->head, item->key, key_bytes, item);
                            return CIF_OK;
                        }

                        /* referenced by the HASH_ADD_KEYPTR macro: */
                        FAILURE_HANDLER(soft):
                        free(key_copy);
                    }

                    free(item);
                }
            }
        }
        free(key_norm);
    }

    FAILURE_TERMINUS;
}

/*
 * Looks up an item in the specified map, and optionally removes it.  The
 * item is returned through the 'value' pointer if that is not NULL.  In any
 * case, CIF_OK is returned if the named item was initially present; otherwise,
 * CIF_NOSUCH_ITEM is returned.
 */
static int cif_map_retrieve_item(cif_map_t *map, const UChar *key, cif_value_t **value, int do_remove,
        int invalidity_code) {
    FAILURE_HANDLING;
    UChar *key_norm;
    int result;

    result = (*map->normalizer)(key, -1, &key_norm, invalidity_code);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        struct entry_s *item = NULL;

        HASH_FIND(hh, map->head, key_norm, U_BYTES(key_norm), item);
        free(key_norm);

        if (item == NULL) {
            SET_RESULT(CIF_NOSUCH_ITEM);
        } else {
            if (value != NULL) {
                *value = &(item->as_value);
            }

            if (do_remove != 0) {
                HASH_DEL(map->head, item);
                if (value != NULL) {
                    cif_map_entry_clean_metadata_internal(item, map);
                } else {
                    cif_map_entry_free_internal(item, map);
                }
            }

            return CIF_OK;
        }
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
extern "C" {
#endif

void cif_map_entry_clean_metadata_internal(struct entry_s *entry, cif_map_t *map) {
    if (entry->key != entry->key_orig) {
        free(entry->key);
    }
    if (map->is_standalone != 0) {
        free(entry->key_orig);
    }
}

void cif_map_entry_free_internal(struct entry_s *entry, cif_map_t *map) {
    cif_map_entry_clean_metadata_internal(entry, map);
    cif_value_free(&(entry->as_value));
}

void cif_packet_free(cif_packet_t *packet) {
    if (packet != NULL) {
        cif_map_clean(&(packet->map));
        free(packet);
    }
}

int cif_packet_get_names(cif_packet_t *packet, const UChar ***names) {
    return cif_map_get_keys(&(packet->map), names);
}

int cif_packet_set_item(cif_packet_t *packet, const UChar *name, cif_value_t *value) {
    return cif_map_set_item(&(packet->map), name, value, CIF_INVALID_ITEMNAME);
}

int cif_packet_get_item(cif_packet_t *packet, const UChar *name, cif_value_t **value) {
    return cif_map_retrieve_item(&(packet->map), name, value, 0, CIF_NOSUCH_ITEM);
}

int cif_packet_remove_item(cif_packet_t *packet, const UChar *name, cif_value_t **value) {
    return cif_map_retrieve_item(&(packet->map), name, value, 1, CIF_NOSUCH_ITEM);
}

int cif_value_get_keys(cif_value_t *table, const UChar ***keys) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_get_keys(&(table->as_table.map), keys);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_set_item_by_key(cif_value_t *table, const UChar *key, cif_value_t *item) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_set_item(&(table->as_table.map), key, item, CIF_INVALID_INDEX);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_get_item_by_key(cif_value_t *table, const UChar *name, cif_value_t **value) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_retrieve_item(&(table->as_table.map), name, value, 0, CIF_NOSUCH_ITEM);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

int cif_value_remove_item_by_key(cif_value_t *table, const UChar *name, cif_value_t **value) {
    if (table->kind == CIF_TABLE_KIND) {
        return cif_map_retrieve_item(&(table->as_table.map), name, value, 1, CIF_NOSUCH_ITEM);
    } else {
        return CIF_ARGUMENT_ERROR;
    }
}

#ifdef __cplusplus
}
#endif

