/*
 * loop.c
 *
 * Copyright (C) 2013 John C. Bollinger
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
#include "utlist.h"
#include "internal/utils.h"
#include "internal/sql.h"

#ifdef __cplusplus
extern "C" {
#endif

/* safe to be called by anyone */
int cif_loop_free(
        cif_loop_t *loop
        ) {
    if (loop->category != NULL) free(loop->category);
    free(loop);
    return CIF_OK;
}

/* safe to be called by anyone */
int cif_loop_destroy(
        cif_loop_t *loop
        ) {
    FAILURE_HANDLING;
    cif_container_t *container = loop->container;
    cif_t *cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, destroy_loop, DESTROY_LOOP_SQL);

    if ((sqlite3_bind_int64(cif->destroy_loop_stmt, 1, container->id) == SQLITE_OK)
            && (sqlite3_bind_int64(cif->destroy_loop_stmt, 2, loop->loop_num) == SQLITE_OK)) {
        STEP_HANDLING;

        if (STEP_STMT(cif, destroy_loop) == SQLITE_DONE) {
            switch (sqlite3_changes(cif->db)) {
                case 0:
                    /* no such loop (now) exists */
                    FAIL(soft, CIF_INVALID_HANDLE);
                case 1:
                    return cif_loop_free(loop);
                default:
                    /* should not happen because the statement deletes by primary key */
                    SET_RESULT(CIF_INTERNAL_ERROR);
            }
        } /* else the executing the statement failed */
    }

    DROP_STMT(cif, destroy_loop);

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

/* safe to be called by anyone */
int cif_loop_get_names(
        cif_loop_t *loop,
        UChar ***item_names
        ) {
    FAILURE_HANDLING;

    if (loop == NULL) return CIF_INVALID_HANDLE;

    cif_container_t *container = loop->container;
    cif_t *cif = container->cif;

    if (item_names == NULL) {
        SET_RESULT(CIF_ARGUMENT_ERROR);
    } else {
        NESTTX_HANDLING;

        /*
         * Create any needed prepared statements, or prepare the existing one(s)
         * for re-use, exiting this function with an error on failure.
         */
        PREPARE_STMT(cif, get_loop_names, GET_LOOP_NAMES_SQL);

        if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {

            /* retrieve the names */
            if ((sqlite3_bind_int64(cif->get_loop_names_stmt, 1, container->id) == SQLITE_OK)
                    && (sqlite3_bind_int64(cif->get_loop_names_stmt, 2, loop->loop_num) == SQLITE_OK)) {
                STEP_HANDLING;
                int name_count = 0;
                string_element_t *name_list = NULL;
                string_element_t *next_name;
                string_element_t *temp_name;

                while (CIF_TRUE) {
                    UChar **temp_names;
 
                    /* read the names and copy them into application space */
                    switch (STEP_STMT(cif, get_loop_names)) {
                        case SQLITE_ROW:
                            next_name = (string_element_t *) malloc(sizeof(string_element_t));

                            if (next_name != NULL) {
                                GET_COLUMN_STRING(cif->get_loop_names_stmt, 0, next_name->string, name_fail);
                                LL_PREPEND(name_list, next_name);  /* prepending is O(1), appending would be O(n) */
                                name_count += 1;
                                continue;
                            } /* else fall through; go on to cleanup/fail */
                        default:
                            /* ignore any error: */
                            sqlite3_reset(cif->get_loop_names_stmt);
                            break;
                        case SQLITE_DONE:
                            temp_names = (UChar **) malloc(sizeof(UChar *) * (name_count + 1));
                            if (temp_names != NULL) {
                                ROLLBACK_NESTTX(cif->db);  /* no changes should have been made anyway */

                                temp_names[name_count] = NULL;
                                LL_FOREACH_SAFE(name_list, next_name, temp_name) {
                                    LL_DELETE(name_list, next_name);
                                    temp_names[--name_count] = next_name->string;
                                    free(next_name);
                                }

                                /* Success */
                                *item_names = temp_names;
                                return CIF_OK;
                            } /* else drop out the bottom and fail */
                            break;
                    }

                    name_fail:
                    /* release resources and roll back before reporting the failure */
                    LL_FOREACH_SAFE(name_list, next_name, temp_name) {
                        LL_DELETE(name_list, next_name);
                        free(next_name->string);
                        free(next_name);
                    }
                    ROLLBACK_NESTTX(cif->db);
                    DEFAULT_FAIL(soft);
                }
            }

            ROLLBACK_NESTTX(cif->db);
        }

        DROP_STMT(cif, get_loop_names);
    }

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

/* safe to be called by anyone */
int cif_loop_add_item(
        cif_loop_t *loop,
        const UChar *item_name,
        cif_value_t *val
        ) {
    FAILURE_HANDLING;
    UChar *norm_name;
    int result;

    result = cif_normalize_item_name(item_name, -1, &norm_name, CIF_INVALID_ITEMNAME);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        cif_container_t *container = loop->container;
        cif_t *cif = container->cif;
        NESTTX_HANDLING;

        /*
         * Create any needed prepared statements, or prepare the existing one(s)
         * for re-use, exiting this function with an error on failure.
         */
        PREPARE_STMT(cif, add_loopitem, ADD_LOOP_ITEM_SQL);

        if ((norm_name != NULL) && (BEGIN_NESTTX(cif->db) == SQLITE_OK)) {
            if ((sqlite3_bind_int64(cif->add_loop_item_stmt, 1, container->id) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->add_loop_item_stmt, 2, norm_name, -1, SQLITE_STATIC) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->add_loop_item_stmt, 3, item_name, -1, SQLITE_STATIC) == SQLITE_OK)
                    && (sqlite3_bind_int(cif->add_loop_item_stmt, 4, loop->loop_num) == SQLITE_OK)
                    ) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, add_loop_item)) {
                    case SQLITE_DONE:
                        if ((cif_container_set_all_values(container, norm_name, val) == CIF_OK)
                                && (COMMIT_NESTTX(cif->db) == SQLITE_OK)) {
                            return CIF_OK;
                        }
                        break;
                    case SQLITE_ROW:
                        /* should not happen -- the insert statement should not return any rows */
                        sqlite3_reset(cif->add_loop_item_stmt);
                        break;
                    case SQLITE_ERROR:
                        if (sqlite3_reset(cif->add_loop_item_stmt) == SQLITE_CONSTRAINT) {
                            FAIL(soft, CIF_DUP_ITEMNAME);
                        } /* else drop out the bottom and fail */
                }
            }

            ROLLBACK_NESTTX(cif->db);
        }

        DROP_STMT(cif, add_loopitem);
    }

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

/* safe to be called by anyone */
int cif_loop_add_packet(
        cif_loop_t *loop,
        cif_packet_t *packet
        ) {
    FAILURE_HANDLING;
    NESTTX_HANDLING;
    cif_container_t *container = loop->container;
    cif_t *cif = container->cif;

    if (!packet->map.head) return CIF_INVALID_PACKET;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, max_packet_num, MAX_PACKET_NUM_SQL);
    PREPARE_STMT(cif, check_item_loop, CHECK_ITEM_LOOP_SQL);
    PREPARE_STMT(cif, insert_value, INSERT_VALUE_SQL);

    if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {
        if ((sqlite3_bind_int64(cif->max_packet_num_stmt, 1, container->id) == SQLITE_OK)
                && (sqlite3_bind_int(cif->max_packet_num_stmt, 2, loop->loop_num) == SQLITE_OK)) {
            STEP_HANDLING;
            int row_num;
            struct entry_s *item;

            /* determine the packet number to use */
            switch (STEP_STMT(cif, max_packet_num)) {
                case SQLITE_ROW:
                    row_num = sqlite3_column_int(cif->max_packet_num_stmt, 1) + 1;
                    if ((sqlite3_reset(cif->max_packet_num_stmt) == SQLITE_OK)
                            && (sqlite3_clear_bindings(cif->max_packet_num_stmt) == SQLITE_OK)
                            && (sqlite3_bind_int64(cif->check_item_loop_stmt, 1, container->id) == SQLITE_OK)
                            && (sqlite3_bind_int(cif->check_item_loop_stmt, 3, loop->loop_num) == SQLITE_OK)) {
                        /* step through the entries in the packet */
                        for (item = packet->map.head; ; item = (struct entry_s *) item->hh.next) {
                            if (!item) { /* no more entries */
                                if (COMMIT_NESTTX(cif->db) == SQLITE_OK) {
                                    return CIF_OK;
                                } else {
                                    DEFAULT_FAIL(hard);
                                }
                            }

                            /* check that the item belongs to the present loop */
                            if (sqlite3_bind_text16(cif->check_item_loop_stmt, 2, item->key, -1, SQLITE_STATIC)
                                    != SQLITE_OK) {
                                DEFAULT_FAIL(hard);
                            } else {
                                switch(STEP_STMT(cif, check_item_loop)) {
                                    case SQLITE_DONE:
                                        /* the item does not belong to this loop */
                                        FAIL(rb, CIF_WRONG_LOOP);
                                    case SQLITE_ROW:
                                        if (sqlite3_reset(cif->check_item_loop_stmt) != SQLITE_OK) {
                                            DEFAULT_FAIL(rb);
                                        }
                                        break;  /* break from switch */
                                    default:
                                        DEFAULT_FAIL(hard);
                                }
                            }

                            if ((sqlite3_bind_int64(cif->insert_value_stmt, 1, container->id) != SQLITE_OK)
                                    || (sqlite3_bind_text16(cif->insert_value_stmt, 2, item->key, -1, SQLITE_STATIC)
                                            != SQLITE_OK)
                                    || (sqlite3_bind_int(cif->insert_value_stmt, 3, row_num) != SQLITE_OK)
                                    || (sqlite3_bind_int(cif->insert_value_stmt, 4, item->as_value.kind)
                                            != SQLITE_OK)) {
                                SET_VALUE_PROPS(cif->insert_value_stmt, 4, &(item->as_value), hard, rb);
                                switch (STEP_STMT(cif, insert_value)) {
                                    case SQLITE_DONE:
                                        /* one value recorded; move on to the next, if any */
                                        if (sqlite3_clear_bindings(cif->insert_value_stmt) != SQLITE_OK) {
                                            DEFAULT_FAIL(hard);
                                        }
                                        continue;
                                    case SQLITE_ROW:
                                        /* should not happen: insert statements do not return rows */
                                        FAIL(rb, CIF_INTERNAL_ERROR);
                                    default:
                                        break; /* falls out of the switch, ultimately to fail hard */
                                }
                            }

                            break; /* break out of the loop */

                            FAILURE_HANDLER(rb):
                            ROLLBACK_NESTTX(cif->db);
                            DEFAULT_FAIL(soft);
                        }
                    }
                    break;
                case SQLITE_DONE:
                    /* should not happen: an aggregate selection must always return a row */
                    (void) ROLLBACK_NESTTX(cif->db);
                    FAIL(soft, CIF_INTERNAL_ERROR);
            }
        }

        FAILURE_HANDLER(hard):
        (void) ROLLBACK_NESTTX(cif->db);
    }

    DROP_STMT(cif, insert_value);
    DROP_STMT(cif, check_item_loop);
    DROP_STMT(cif, max_packet_num);

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

/* not safe to be called by other library functions */
int cif_loop_get_packets(
        cif_loop_t *loop,
        cif_pktitr_t **iterator
        ) {
    FAILURE_HANDLING;
    cif_container_t *container = loop->container;
    cif_t *cif = container->cif;
    cif_pktitr_t *temp_it;

    if (iterator == NULL) return CIF_ARGUMENT_ERROR;

    temp_it = (cif_pktitr_t *) malloc(sizeof(cif_pktitr_t));
    if (temp_it) {
        /* initialize to NULL so we can later recognize where cleanup is needed */
        temp_it->stmt = NULL;
        temp_it->item_names = NULL;
        temp_it->name_set = NULL;

        if (cif_loop_get_names(loop, &(temp_it->item_names)) == CIF_OK) {
            UChar **name;

            /* construct a set of item names for quick name-inclusion tests */
#undef uthash_fatal
#define uthash_fatal(msg) DEFAULT_FAIL(soft)
            for (name = temp_it->item_names; *name; name += 1) {
                struct set_element_s *element = (struct set_element_s *) malloc(sizeof(struct set_element_s));

                if (element) {
                    HASH_ADD_KEYPTR(hh, temp_it->name_set, *name, U_BYTES(*name), element);
                } else {
                    DEFAULT_FAIL(soft);
                }
            }

            /* prepare the SQL statement by which the values will be retrieved, and fetch the first row */
            if (sqlite3_prepare_v2(cif->db, GET_LOOP_VALUES_SQL, -1, &(temp_it->stmt), NULL) == SQLITE_OK) {
                if ((sqlite3_bind_int64(temp_it->stmt, 1, container->id) == SQLITE_OK)
                        && (sqlite3_bind_int(temp_it->stmt, 2, loop->loop_num) == SQLITE_OK)) {
                    if (BEGIN(cif->db) == SQLITE_OK) {
                        switch (sqlite3_step(temp_it->stmt)) {
                            case SQLITE_ROW:
                                temp_it->previous_row_num = -1;
                                temp_it->loop = loop;
                                *iterator = temp_it;
                                /* transaction is left open */
                                return CIF_OK;
                            case SQLITE_DONE:
                                SET_RESULT(CIF_EMPTY_LOOP);
                        }
                        (void) ROLLBACK(cif->db);
                    }
                }
            }
        }

        FAILURE_HANDLER(soft):
        /* clean up everything */
        cif_pktitr_free(temp_it);
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
}
#endif

