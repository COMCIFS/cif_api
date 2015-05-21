/*
 * loop.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "internal/compat.h"

#include <stdlib.h>
#include <assert.h>
#include "cif.h"
#include "internal/ciftypes.h"
#include "utlist.h"
#include "internal/utils.h"
#include "internal/sql.h"

static const char MULTIPLE_SCALAR_MESSAGE[49] = "Attempted to create multiple values for a scalar";

static int dup_ustrings(UChar ***dest, UChar *src[]);
static int cif_loop_get_names_internal(cif_loop_tp *loop, UChar ***item_names, int normalize);

static int dup_ustrings(UChar ***dest, UChar *src[]) {
    if (src == NULL) {
        *dest = NULL;
        return CIF_OK;
    } else {
        FAILURE_HANDLING;
        size_t string_count = 0;
        UChar **dest_temp;
        UChar **counter;

        /* count the strings (including if there are zero of them) */
        for (counter = src; *counter != NULL; counter += 1) {
            string_count += 1;
        }

        /* allocate space for the string pointers and a sentinel */
        dest_temp = (UChar **) malloc((string_count + 1) * sizeof(UChar *));
        if (dest_temp == NULL) {
            SET_RESULT(CIF_MEMORY_ERROR);
        } else {
            /* dupe all the individual strings */
            for (counter = dest_temp; *src != NULL; src += 1, counter += 1) {
                *counter = cif_u_strdup(*src);
                if (*counter == NULL) {
                    FAIL(soft, CIF_MEMORY_ERROR);
                }
            }

            /* terminate the array */
            *counter = NULL;

            /* success */
            *dest = dest_temp;
            return CIF_OK;

            FAILURE_HANDLER(soft):
            /* memory allocation failure for one of the member strings */
            while (counter > dest_temp) {
                free(--counter);
            }

            free(dest_temp);
        }

        FAILURE_TERMINUS;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

/* safe to be called by anyone */
void cif_loop_free(
        cif_loop_tp *loop
        ) {
    if (loop->category != NULL) free(loop->category);
    if (loop->names != NULL) {
        UChar **namep;
        for (namep = loop->names; *namep != NULL; namep += 1) {
            free(*namep);
        }
        free(loop->names);
    }
    free(loop);
}

/* safe to be called by anyone */
int cif_loop_destroy(
        cif_loop_tp *loop
        ) {
    FAILURE_HANDLING;
    cif_container_tp *container = loop->container;
    cif_tp *cif;

    if (container == NULL) {
        return CIF_INVALID_HANDLE;
    } else {
        cif = container->cif;
    }

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
                    cif_loop_free(loop);
                    return CIF_OK;
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

/* does not touch the database */
int cif_loop_get_category(cif_loop_tp *loop, UChar **category) {
    if (loop->category == NULL) {
        *category = NULL;
        return CIF_OK;
    } else {
        UChar *temp = cif_u_strdup(loop->category);

        if (temp != NULL) {
            *category = temp;
            return CIF_OK;
        } else {
            return CIF_MEMORY_ERROR;
        }
    }
}

int cif_loop_set_category(cif_loop_tp *loop, const UChar *category) {
    cif_container_tp *container = loop->container;
    UChar *category_temp;

    if (category == NULL) {
        category_temp = NULL;
    } else if (*category == 0) {
        return CIF_RESERVED_LOOP;
    } else {
        int temp = cif_loop_get_category(loop, &category_temp);

        if (temp != CIF_OK) {
            return temp;
        } else if (category_temp != NULL) {
            temp = *category_temp;
            free(category_temp);
            if (temp == 0) {
                return CIF_RESERVED_LOOP;
            }
        }

        category_temp = cif_u_strdup(category);
        if (category_temp == NULL) {
            return CIF_MEMORY_ERROR;
        }
    }

    if (container == NULL) {
        /* an unattached loop, such as may be synthesized temporarily during CIF parsing */
        if (loop->category != NULL) {
            free(loop->category);
        }
        loop->category = category_temp;

        return CIF_OK;
    } else {
        cif_tp *cif = container->cif;

        if (cif == NULL) {
            return CIF_ERROR;
        } else {
            FAILURE_HANDLING;
            STEP_HANDLING;

            /*
             * Create any needed prepared statements, or prepare the existing one(s)
             * for re-use, exiting this function with an error on failure.
             */
            PREPARE_STMT(cif, set_loop_category, SET_CATEGORY_SQL);

            /* set the category */
            if ((sqlite3_bind_int64(cif->set_loop_category_stmt, 2, container->id) == SQLITE_OK)
                    && (sqlite3_bind_int64(cif->set_loop_category_stmt, 3, loop->loop_num) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->set_loop_category_stmt, 1, category_temp, -1, SQLITE_STATIC)
                            == SQLITE_OK)
                    && (STEP_STMT(cif, set_loop_category) == SQLITE_DONE)) {

                if (loop->category != NULL) {
                    free(loop->category);
                }
                loop->category = category_temp;

                switch(sqlite3_changes(cif->db)) {
                    case 0:
                        /* The provided handle does not correspond to any existing loop */
                        return CIF_INVALID_HANDLE;
                    case 1:
                        /*
                         * Normal result
                         *
                         * NOTE: this relies on sqlite3_changes() to count all rows matching the selection predicate
                         * of an UPDATE statement as "changed", even if all the values set in a given row are actually
                         * equal to the corresponding values before the update.
                         */
                        return CIF_OK;
                    default:
                        /* should not happen because the query specifies the row to change by its full key */
                        return CIF_INTERNAL_ERROR;
                }
            }

            /* failed -- clean up */
            DROP_STMT(cif, get_loop_names);
            free(category_temp);

            FAILURE_TERMINUS;
        }
    }
}

/* safe to be called by anyone */
int cif_loop_get_names(cif_loop_tp *loop, UChar ***item_names) {
    return cif_loop_get_names_internal(loop, item_names, CIF_FALSE);
}

/* safe to be called by anyone */
int cif_loop_add_item(
        cif_loop_tp *loop,
        const UChar *item_name,
        cif_value_tp *val
        ) {
    UChar *norm_name;
    int changes;  /* ignored */
    int result;

    if ((loop == NULL) || (loop->container == NULL) || (loop->container->cif == NULL)) {
        return CIF_INVALID_HANDLE;
    } else {
        cif_value_tp *default_val;

        if (val) {
            default_val = val;
        } else if ((result = cif_value_create(CIF_UNK_KIND, &default_val)) != CIF_OK) {
            return result;
        }
        if ((result = cif_normalize_item_name(item_name, -1, &norm_name, CIF_INVALID_ITEMNAME)) == CIF_OK) {
            result = cif_loop_add_item_internal(loop, item_name, norm_name, default_val, &changes);
            free(norm_name);
        }
        if (!val) {
            cif_value_free(default_val);
        }
        return result;
    }
}

int cif_loop_add_item_internal(
        cif_loop_tp *loop,
        const UChar *item_name,
        const UChar *norm_name,
        cif_value_tp *val,
        int *changes
        ) {
    FAILURE_HANDLING;
    NESTTX_HANDLING;
    cif_container_tp *container = loop->container;
    cif_tp *cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, add_loop_item, ADD_LOOP_ITEM_SQL);

    TRACELINE;
    assert(norm_name != NULL);
    if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {
        TRACELINE;
        if ((DEBUG_WRAP(cif->db, sqlite3_bind_int64(cif->add_loop_item_stmt, 1, container->id)) == SQLITE_OK)
                && (DEBUG_WRAP(cif->db, sqlite3_bind_text16(cif->add_loop_item_stmt, 2, norm_name, -1, SQLITE_STATIC))
                        == SQLITE_OK)
                && (DEBUG_WRAP(cif->db, sqlite3_bind_text16(cif->add_loop_item_stmt, 3, item_name, -1, SQLITE_STATIC))
                        == SQLITE_OK)
                && (DEBUG_WRAP(cif->db, sqlite3_bind_int(cif->add_loop_item_stmt, 4, loop->loop_num)) == SQLITE_OK)
                ) {
            STEP_HANDLING;

            TRACELINE;
            switch (STEP_STMT(cif, add_loop_item)) {
                case SQLITE_DONE:
                    TRACELINE;
                    if ((cif_container_set_all_values(container, norm_name, val) == CIF_OK)
                            /* NOTE: sqlite3_changes() is not thread-safe */
                            && ((*changes = sqlite3_changes(cif->db)) || CIF_TRUE)
                            && (COMMIT_NESTTX(cif->db) == SQLITE_OK)) {
                        return CIF_OK;
                    }
                    break;
                case SQLITE_ROW:
                    /* should not happen -- the insert statement should not return any rows */
                    TRACELINE;
                    sqlite3_reset(cif->add_loop_item_stmt);
                    break;
                case SQLITE_CONSTRAINT:
                    TRACELINE;
                    sqlite3_reset(cif->add_loop_item_stmt);
                    ROLLBACK_NESTTX(cif->db);
                    FAIL(soft, CIF_DUP_ITEMNAME);
                default:
                    TRACELINE;
                    sqlite3_reset(cif->add_loop_item_stmt);
                    /* fall out the bottom and fail */
            }
        }

        ROLLBACK_NESTTX(cif->db);
    }

    DROP_STMT(cif, add_loop_item);

    FAILURE_HANDLER(soft):

    FAILURE_TERMINUS;
}

/* safe to be called by anyone */
int cif_loop_add_packet(
        cif_loop_tp *loop,
        cif_packet_tp *packet
        ) {
    FAILURE_HANDLING;
    NESTTX_HANDLING;
    cif_container_tp *container = loop->container;
    cif_tp *cif;

    if (container == NULL) {
        return CIF_INVALID_HANDLE;
    } else if (!packet->map.head) {
        return CIF_INVALID_PACKET;
    } else {
        cif = container->cif;
    }

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, update_packet_num, UPDATE_PACKET_NUM_SQL);
    PREPARE_STMT(cif, get_packet_num, GET_PACKET_NUM_SQL);
    PREPARE_STMT(cif, check_item_loop, CHECK_ITEM_LOOP_SQL);
    PREPARE_STMT(cif, insert_value, INSERT_VALUE_SQL);

    if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {
        STEP_HANDLING;
        int result = -1;

        if ((sqlite3_bind_int64(cif->update_packet_num_stmt, 1, container->id) == SQLITE_OK)
                && (sqlite3_bind_int(cif->update_packet_num_stmt, 2, loop->loop_num) == SQLITE_OK)
                && ((result = STEP_STMT(cif, update_packet_num)) == SQLITE_DONE)
                && (sqlite3_bind_int64(cif->get_packet_num_stmt, 1, container->id) == SQLITE_OK)
                && (sqlite3_bind_int(cif->get_packet_num_stmt, 2, loop->loop_num) == SQLITE_OK)) {
            int row_num;
            struct entry_s *item;

            /* determine the packet number to use */
            switch (STEP_STMT(cif, get_packet_num)) {
                case SQLITE_ROW:
                    TRACELINE;
                    /* If the result is NULL then the retrieval function returns it as 0: */
                    row_num = sqlite3_column_int(cif->get_packet_num_stmt, 0);
                    if ((sqlite3_reset(cif->get_packet_num_stmt) == SQLITE_OK)
                            && (sqlite3_clear_bindings(cif->get_packet_num_stmt) == SQLITE_OK)
                            && (sqlite3_bind_int64(cif->check_item_loop_stmt, 1, container->id) == SQLITE_OK)
                            && (sqlite3_bind_int(cif->check_item_loop_stmt, 3, loop->loop_num) == SQLITE_OK)) {

                        /* step through the entries in the packet */
                        for (item = packet->map.head; ; item = (struct entry_s *) item->hh.next) {
                            if (item == NULL) { /* no more entries */
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
                                        TRACELINE;
                                        FAIL(rb, CIF_WRONG_LOOP);
                                    case SQLITE_ROW:
                                        TRACELINE;
                                        if (sqlite3_reset(cif->check_item_loop_stmt) != SQLITE_OK) {
                                            DEFAULT_FAIL(rb);
                                        }
                                        break;  /* break from switch */
                                    default:
                                        TRACELINE;
                                        DEFAULT_FAIL(hard);
                                }
                            }

                            /* insert this item's value for this packet */
                            TRACELINE;
                            if ((sqlite3_bind_int64(cif->insert_value_stmt, 1, container->id) == SQLITE_OK)
                                    && (sqlite3_bind_text16(cif->insert_value_stmt, 2, item->key, -1, SQLITE_STATIC)
                                            == SQLITE_OK)
                                    && (sqlite3_bind_int(cif->insert_value_stmt, 3, row_num) == SQLITE_OK)) {
                                SET_VALUE_PROPS(cif->insert_value_stmt, 3, &(item->as_value), hard, rb);
                                TRACELINE;
                                switch (STEP_STMT(cif, insert_value)) {
                                    case SQLITE_DONE:
                                        /* one value recorded; move on to the next, if any */
                                        TRACELINE;
                                        if (sqlite3_clear_bindings(cif->insert_value_stmt) != SQLITE_OK) {
                                            DEFAULT_FAIL(hard);
                                        }
                                        continue;
                                    case SQLITE_ROW:
                                        /* should not happen: insert statements do not return rows */
                                        TRACELINE;
                                        FAIL(rb, CIF_INTERNAL_ERROR);
                                    default:
                                        TRACELINE;
                                        break; /* falls out of the switch, ultimately to fail hard */
                                }
                            }

                            TRACELINE;
                            break; /* break out of the loop */

                            FAILURE_HANDLER(rb):
                            TRACELINE;
                            ROLLBACK_NESTTX(cif->db);
                            DEFAULT_FAIL(soft);
                        }
                    }
                    break;
                case SQLITE_DONE:
                    /* should not happen: an aggregate selection must always return a row */
                    TRACELINE;
                    ROLLBACK_NESTTX(cif->db);
                    FAIL(soft, CIF_INTERNAL_ERROR);
                /* default: do nothing */
            }
        } else if (result == SQLITE_CONSTRAINT) {
            TRACELINE;
            sqlite3_reset(cif->update_packet_num_stmt);
            /* Note: sqlite3_errmsg() is not thread-safe */
            if (strcmp(DEBUG_MSG("sqlite3 error message", sqlite3_errmsg(cif->db)), MULTIPLE_SCALAR_MESSAGE) == 0) {
                SET_RESULT(CIF_RESERVED_LOOP);
            }
        }

        FAILURE_HANDLER(hard):
        (void) ROLLBACK_NESTTX(cif->db);
    }

    DROP_STMT(cif, insert_value);
    DROP_STMT(cif, check_item_loop);
    DROP_STMT(cif, get_packet_num);
    DROP_STMT(cif, update_packet_num);

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

/* not safe to be called by other library functions */
int cif_loop_get_packets(
        cif_loop_tp *loop,
        cif_pktitr_tp **iterator
        ) {
    FAILURE_HANDLING;
    cif_container_tp *container = loop->container;
    cif_tp *cif;
    cif_pktitr_tp *temp_it;

    if (container == NULL) {
        return CIF_INVALID_HANDLE;
    } else if (iterator == NULL) {
        return CIF_ARGUMENT_ERROR;
    } else {
        cif = container->cif;
    }

    temp_it = (cif_pktitr_tp *) malloc(sizeof(cif_pktitr_tp));
    if (!temp_it) {
        SET_RESULT(CIF_MEMORY_ERROR);
    } else {
        int result;

        /* initialize to NULL so we can later recognize where cleanup is needed */
        temp_it->stmt = NULL;
        temp_it->item_names = NULL;
        temp_it->name_set = NULL;
        temp_it->finished = 0;

        if ((result = cif_loop_get_names_internal(loop, &(temp_it->item_names), CIF_TRUE)) != CIF_OK) {
            SET_RESULT(result);
        } else {
            UChar **name;

/* All uthash fatal errors arise from memory allocation failure */
#undef uthash_fatal
#define uthash_fatal(msg) FAIL(soft, CIF_MEMORY_ERROR)
            for (name = temp_it->item_names; *name; name += 1) {
                struct set_element_s *element = (struct set_element_s *) malloc(sizeof(struct set_element_s));

                if (element) {
                    HASH_ADD_KEYPTR(hh, temp_it->name_set, *name, U_BYTES(*name), element);
                } else {
                    FAIL(soft, CIF_MEMORY_ERROR);
                }
            }

            /* prepare the SQL statement by which the values will be retrieved, and fetch the first row */
            if (sqlite3_prepare_v2(cif->db, GET_LOOP_VALUES_SQL, -1, &(temp_it->stmt), NULL) == SQLITE_OK) {
                if ((sqlite3_bind_int64(temp_it->stmt, 1, container->id) == SQLITE_OK)
                        && (sqlite3_bind_int(temp_it->stmt, 2, loop->loop_num) == SQLITE_OK)) {
                    if (BEGIN(cif->db) == SQLITE_OK) {
                        /* intentionally not using STEP_STMT(): */
                        switch (sqlite3_step(temp_it->stmt)) {
                            case SQLITE_ROW:
                                temp_it->previous_row_num = -1;
                                temp_it->loop = loop;
                                *iterator = temp_it;
                                /* transaction is left open */
                                return CIF_OK;
                            case SQLITE_DONE:
                                SET_RESULT(CIF_EMPTY_LOOP);
                                /* fall through */
                            /* default: do nothing */
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

static int cif_loop_get_names_internal(cif_loop_tp *loop, UChar ***item_names, int normalize) {
    cif_container_tp *container = loop->container;

    if (item_names == NULL) {
        return CIF_ARGUMENT_ERROR;
    } else if (loop->loop_num < 0) {
        /* return the cached names, if any */
        return dup_ustrings(item_names, loop->names);
    } else if (container == NULL) {
        return CIF_INVALID_HANDLE;
    } else {
        FAILURE_HANDLING;
        NESTTX_HANDLING;
        cif_tp *cif = container->cif;

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
                string_element_tp *name_list = NULL;
                string_element_tp *next_name;
                string_element_tp *temp_name;

                while (CIF_TRUE) {
                    UChar **temp_names;

                    /* read the names and copy them into application space */
                    switch (STEP_STMT(cif, get_loop_names)) {
                        case SQLITE_ROW:
                            next_name = (string_element_tp *) malloc(sizeof(string_element_tp));

                            if (next_name != NULL) {
                                GET_COLUMN_STRING(cif->get_loop_names_stmt, 0, next_name->string, HANDLER_LABEL(name));
                                LL_PREPEND(name_list, next_name);  /* prepending is O(1), appending would be O(n) */
                                name_count += 1;
                                continue;
                            }

                            SET_RESULT(CIF_MEMORY_ERROR);
                            /* fall through; go on to cleanup/fail */
                        default:
                            /* ignore any error: */
                            sqlite3_reset(cif->get_loop_names_stmt);
                            break;
                        case SQLITE_DONE:
                            if (name_count <= 0) FAIL(name, CIF_INVALID_HANDLE);
                            temp_names = (UChar **) malloc(sizeof(UChar *) * (name_count + 1));
                            if (temp_names == NULL) {
                                SET_RESULT(CIF_MEMORY_ERROR);
                            } else {
                                ROLLBACK_NESTTX(cif->db);  /* no changes should have been made anyway */

                                temp_names[name_count] = NULL;
                                LL_FOREACH_SAFE(name_list, next_name, temp_name) {
                                    LL_DELETE(name_list, next_name);
                                    name_count -= 1;
                                    if (normalize) {
                                        int result = cif_normalize_item_name(
                                                next_name->string, -1, temp_names + name_count, CIF_INTERNAL_ERROR);

                                        free(next_name->string);
                                        free(next_name);
                                        if (result != CIF_OK) {
                                            FAIL(normalization, result);
                                        }
                                    } else {
                                        temp_names[name_count] = next_name->string;
                                        free(next_name);
                                    }
                                }

                                /* Success */
                                *item_names = temp_names;
                                return CIF_OK;

                                FAILURE_HANDLER(normalization):
                                while (temp_names[++name_count] != NULL) {
                                    free(temp_names[name_count]);
                                }
                            } /* else drop out the bottom and fail */
                            break;
                    }

                    FAILURE_HANDLER(name):
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

        FAILURE_HANDLER(soft):
        FAILURE_TERMINUS;
    }
}
