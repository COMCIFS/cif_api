/*
 * pktitr.c
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
#include "internal/value.h"
#include "internal/sql.h"
#include "utlist.h"
#include "uthash.h"

#ifdef __cplusplus
extern "C" {
#endif

int cif_pktitr_close(
        cif_pktitr_t *iterator
        ) {
    int result = CIF_OK;
    cif_t *cif = iterator->loop->container->cif;

    if (COMMIT(cif->db) != SQLITE_OK) {
        result = CIF_ERROR;
        (void) ROLLBACK(cif->db);
    }

    cif_pktitr_free(iterator);

    return result;
}

int cif_pktitr_abort(
        cif_pktitr_t *iterator
        ) {
    /* This implementation never returns CIF_NOT_SUPPORTED */

    int result = CIF_OK;
    cif_t *cif = iterator->loop->container->cif;

    if (ROLLBACK(cif->db) != SQLITE_OK) {
        result = CIF_ERROR;
    }

    cif_pktitr_free(iterator);

    return result;
}

void cif_pktitr_free(
        cif_pktitr_t *iterator
        ) {
    struct set_element_s *element;
    struct set_element_s *temp;

    if (iterator->item_names != NULL) {
        /*@temp@*/ UChar **name;

        for (name = iterator->item_names; *name; name += 1) {
            free(*name);
        }
        free(iterator->item_names);
    }

    HASH_ITER(hh, iterator->name_set, element, temp) {
        HASH_DEL(iterator->name_set, element);
        free(element);
    }

    sqlite3_finalize(iterator->stmt); /* harmless if the stmt is NULL */

    free(iterator);
}

int cif_pktitr_next_packet(
        cif_pktitr_t *iterator,
        cif_packet_t **packet
        ) {
    FAILURE_HANDLING;
    sqlite3_stmt *stmt = iterator->stmt;
    int current_row = sqlite3_column_int(stmt, 0);
    cif_packet_t *temp_packet;

    assert (iterator->item_names != NULL);
    if (sqlite3_get_autocommit(iterator->loop->container->cif->db) != 0) {
        /* no transaction is active -- the provided iterator is stale */
        return CIF_INVALID_HANDLE;
    }

    /* create a new packet for the expected items, with all unknown values */
    if (cif_packet_create_norm(&temp_packet, iterator->item_names, CIF_FALSE) == CIF_OK) {
        /* populate the packet with values read from the DB */
        while (CIF_TRUE) {
            const UChar *name;
            struct entry_s *entry;
            int next_row;

            /* For which item is this value? */

            /* will be freed automatically by SQLite: */
            name = (const UChar *) sqlite3_column_text16(stmt, 1);

            if (!name) {
                DEFAULT_FAIL(soft);
            }
            HASH_FIND(hh, temp_packet->map.head, name, U_BYTES(name), entry);
            if ((entry == NULL) || entry->as_value.kind != CIF_UNK_KIND) {
                /* The item was expected to have a dummy value pre-recorded in the packet */
                FAIL(soft, CIF_INTERNAL_ERROR);
            }

            /* set value properties from the DB */
            GET_VALUE_PROPS(stmt, 2, &(entry->as_value), soft);

            /* check whether there are any more values for the current packet */
            switch (sqlite3_step(stmt)) {
                case SQLITE_ROW:
                    next_row = sqlite3_column_int(stmt, 0);
                    if (next_row == current_row) {
                        /* there is another value for this packet; loop back to handle it */
                        continue;
                    } /* else that was the last value for the packet, but there is another packet after it */
                    SET_RESULT(CIF_OK);
                    break;
                case SQLITE_DONE:
                    /* that was the last value for the last packet */
                    SET_RESULT(CIF_FINISHED);
                    break;
                default:
                    DEFAULT_FAIL(soft);
            }

            /* the current packet has been fully read from the DB */
            iterator->previous_row_num = current_row;

            /* (Optionally) set the packet (or just its contents) in the result */
            if (packet == NULL) {
                /* do nothing -- the packet is simply dropped */
            } else if (*packet == NULL) {
                /* easy case: just give the caller a pointer to the packet we constructed */
                *packet = temp_packet;
            } else {
                /* copy values into the existing packet */
                struct entry_s *temp;

                HASH_ITER(hh, temp_packet->map.head, entry, temp) {
                    size_t name_len = (size_t) U_BYTES(entry->key);
                    struct entry_s *target;

                    HASH_FIND(hh, (*packet)->map.head, entry->key, name_len, target);
                    HASH_DEL(temp_packet->map.head, entry);
                    if (target != NULL) {
                        /* we don't cif_value_clone() the value because we want a shallow copy instead of a deep one */

                        /* release any resources held by the current value */
                        (void) cif_value_clean(&(target->as_value));

                        /* make a shallow copy of the value: */
                        memcpy(&(target->as_value), &(entry->as_value), sizeof(cif_value_t));

                        /* release the temporary entry itself, but not any resources it refers to */
                        free(entry);
                    } else if ((*packet)->map.is_standalone != 0) {
                        /* convert the entry to standalone, for compatibility with the packet */
                        entry->key = cif_u_strdup(entry->key);

                        if (entry->key != NULL) {
                            /* add the entry to the packet */
                            HASH_ADD_KEYPTR(hh, (*packet)->map.head, entry->key, name_len, entry);
                        } else {
                            DEFAULT_FAIL(soft);
                        }
                    } else {
                        FAIL(soft, CIF_ARGUMENT_ERROR);
                    }
                }

                /* we copied the packet instead of assigning it out, so free whatever is left */
                (void) cif_packet_free(temp_packet);
            }

            SUCCESS_TERMINUS;
        }

        FAILURE_HANDLER(soft):
        (void) cif_packet_free(temp_packet);
    }

    FAILURE_TERMINUS;
}

#define SET_ID_PROPS(stmt, ofs, container_id, item_name, row_num, onerr) do { \
    sqlite3_stmt *s = (stmt); \
    if ((sqlite3_bind_int64(s, ofs + 1, (container_id)) != SQLITE_OK) \
            || (sqlite3_bind_text16(s, ofs + 2, (item_name), -1, SQLITE_STATIC) != SQLITE_OK) \
            || (sqlite3_bind_int(s, ofs + 3, (row_num)) != SQLITE_OK)) DEFAULT_FAIL(onerr); \
} while (0)

int cif_pktitr_update_packet(
        cif_pktitr_t *iterator,
        cif_packet_t *packet
        ) {
    FAILURE_HANDLING;

    if (sqlite3_get_autocommit(iterator->loop->container->cif->db) != 0) {
        /* no transaction is active -- the provided iterator is stale */
        SET_RESULT(CIF_INVALID_HANDLE);
    } else if (iterator->previous_row_num <= 0) {
        /* no packet has yet been returned, or the last returned has been removed */
        SET_RESULT(CIF_MISUSE);
    } else {
        cif_container_t *container = iterator->loop->container;
        cif_t *cif = container->cif;
        struct entry_s *scalar;
        struct entry_s *temp;

        /*
         * Create any needed prepared statements, or prepare the existing one(s)
         * for re-use, exiting this function with an error on failure.
         */
        PREPARE_STMT(cif, update_packet_item, UPDATE_PACKET_ITEM_SQL);
        PREPARE_STMT(cif, insert_packet_item, INSERT_PACKET_ITEM_SQL);

        if (SAVE(cif->db) == CIF_OK) {
            /* step through the items in the provided packet */
            HASH_ITER(hh, packet->map.head, scalar, temp) {
                struct set_element_s *element;

                HASH_FIND(hh, iterator->name_set, scalar->key, U_BYTES(scalar->key), element);
                if (element) { /* an item for the iterator's subject loop */
                    STEP_HANDLING;

                    SET_VALUE_PROPS(cif->update_packet_item_stmt, 1, &(scalar->as_value), hard, soft);
                    SET_ID_PROPS(cif->update_packet_item_stmt, 6, container->id, scalar->key,
                            iterator->previous_row_num, hard);

                    if ((STEP_STMT(cif, update_packet_item) == SQLITE_DONE)
                            && (sqlite3_clear_bindings(cif->update_packet_item_stmt) == SQLITE_OK)) {
                        /* working here */
                        switch (sqlite3_changes(cif->db)) {
                            case 0:  /* no rows updated -- item must be missing for the row */
                                SET_VALUE_PROPS(cif->insert_packet_item_stmt, 1, &(scalar->as_value), hard, soft);
                                SET_ID_PROPS(cif->insert_packet_item_stmt, 6, container->id, scalar->key,
                                        iterator->previous_row_num, hard);
				if ((STEP_STMT(cif, insert_packet_item) != SQLITE_DONE)
                                        || (sqlite3_clear_bindings(cif->insert_packet_item_stmt) != SQLITE_OK)) {
                                    break;
                                }
                            case 1:
                                /* on to the next packet element */
                                continue;
                            default: /* multiple rows updated */
                                /* should not happen because we identify the row to update by its full primary key */
                                FAIL(soft, CIF_INTERNAL_ERROR);
                        }
                    }

                    DEFAULT_FAIL(hard);
                } else {
                    /* not an item belonging to the iterator's subject loop */
                    FAIL(soft, CIF_WRONG_LOOP);
                }
            }

            if (RELEASE(cif->db) == SQLITE_OK) {
                return CIF_OK;
            }

            FAILURE_HANDLER(hard):
            DROP_STMT(cif, update_packet_item);
            DROP_STMT(cif, insert_packet_item);

            FAILURE_HANDLER(soft):
            ROLLBACK_TO(cif->db);
        }
    }

    FAILURE_TERMINUS;
}

int cif_pktitr_remove_packet(
        cif_pktitr_t *iterator
        ) {
    FAILURE_HANDLING;

    if (sqlite3_get_autocommit(iterator->loop->container->cif->db)) {
        /* no transaction is active -- the provided iterator is stale */
        SET_RESULT(CIF_INVALID_HANDLE);
    } else if (iterator->previous_row_num <= 0) {
        /* no packet has yet been returned, or the last returned has already been removed */
        SET_RESULT(CIF_MISUSE);
    } else {
        cif_loop_t *loop = iterator->loop;
        cif_container_t *container = iterator->loop->container;
        cif_t *cif = container->cif;

        /*
         * Create any needed prepared statements, or prepare the existing one(s)
         * for re-use, exiting this function with an error on failure.
         */
        PREPARE_STMT(cif, remove_packet, REMOVE_PACKET_SQL);

        if ((sqlite3_bind_int64(cif->remove_packet_stmt, 1, container->id) == SQLITE_OK)
                && (sqlite3_bind_int(cif->remove_packet_stmt, 2, loop->loop_num) == SQLITE_OK)
                && (sqlite3_bind_int(cif->remove_packet_stmt, 3, iterator->previous_row_num) == SQLITE_OK)
                && (SAVE(cif->db) == SQLITE_OK)) {
            STEP_HANDLING;

            if ((STEP_STMT(cif, remove_packet) == SQLITE_DONE) 
                    && (RELEASE(cif->db) == SQLITE_OK)) {
                /* Success */
                iterator->previous_row_num = -1;
                return CIF_OK;
            }

            (void) ROLLBACK_TO(cif->db);
        }
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
}
#endif

