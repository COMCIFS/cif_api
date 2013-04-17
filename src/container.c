/*
 * container.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unicode/ustring.h>
#include "cif.h"
#include "internal/ciftypes.h"
#include "internal/utils.h"
#include "internal/sql.h"

/*
 * A common back-end for retrieving the loop of a given item in a given
 * container.  Relies on the caller to provide a normalized and valid item
 * name and a pre-allocated loop structure.  Does not validate the container
 * before using it, but does check for invalid item names.  Writes directly
 * to the provided loop structure, so a partial update may occur on failure.
 * Does not perform transaction management.
 *
 * The loop category and container are definitely assigned by this function;
 * the caller is responsible for releasing these when appropriate.
 */
static int cif_container_get_item_loop_internal (
        /*@temp@*/ cif_container_t *container,
        /*@temp@*/ const UChar *name,
        /*@temp@*/ cif_loop_t *loop
        );

/*
 * Adds a scalar item to the specified container, with the given name and value.  The name is assumed
 * normalized and valid.  The container and value are assumed valid.  No transaction management is performed.
 */
static int cif_container_add_scalar(
        /*@temp@*/ cif_container_t *container,
        /*@temp@*/ const UChar *item_name,
        /*@temp@*/ const UChar *name_orig,
        /*@temp@*/ cif_value_t *val
        );

static int cif_container_get_item_loop_internal (
        cif_container_t *container,
        const UChar *name,
        cif_loop_t *loop
        ) {
    FAILURE_HANDLING;
    /*@temp@*/ cif_t *cif = container->cif;

    /*
     * Create any needed prepared statements or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_item_loop, GET_ITEM_LOOP_SQL);

    /* Assign the container and category now to be sure that they have sensible values in the case of an error */
    loop->container = container;
    loop->category = NULL;

    if ((sqlite3_bind_text16(cif->get_item_loop_stmt, 2, name, -1, SQLITE_STATIC) == SQLITE_OK)
           && (sqlite3_bind_int64(cif->get_item_loop_stmt, 1, container->id) == SQLITE_OK)) {
        STEP_HANDLING;

        switch (STEP_STMT(cif, get_item_loop)) {
            case SQLITE_DONE:
                FAIL(soft, CIF_NOSUCH_ITEM);
            case SQLITE_ROW:
                GET_COLUMN_STRING(cif->get_item_loop_stmt, 2, loop->category, soft_fail);
                loop->loop_num = sqlite3_column_int(cif->get_item_loop_stmt, 1);

                /* verify that there was only one result row */
                switch (STEP_STMT(cif, get_item_loop)) {
                    case SQLITE_DONE:
                        return CIF_OK;
                    case SQLITE_ROW:
                        (void) sqlite3_reset(cif->get_item_loop_stmt);
                        FAIL(soft, CIF_INTERNAL_ERROR);
                }
        } /* else fall-through / fail */
    }

    DROP_STMT(cif, get_item_loop);

    FAILURE_HANDLER(soft):

    FAILURE_TERMINUS;
}

static int cif_container_add_scalar(
        cif_container_t *container,
        const UChar *item_name,
        const UChar *name_orig,
        cif_value_t *val
        ) {
    FAILURE_HANDLING;
    cif_loop_t *loop;
    const UChar *names[2] = { NULL, NULL };
    UChar *norm_names[2] = { NULL, NULL };
    UChar null_char = 0;
    int result;

    result = cif_container_get_category_loop(container, &null_char, &loop);
    switch(result) {
        case CIF_NOSUCH_LOOP:
            /* first create the scalar loop */
            /* FIXME: avoid re-normalizing the name */
            names[0] = name_orig;
            result = cif_container_create_loop(container, &null_char, names, &loop);
            if (result == CIF_OK) {
                cif_packet_t *packet;

                norm_names[0] = item_name;
                result = cif_packet_create_norm(&packet, norm_names, CIF_FALSE);
                if (result == CIF_OK) {
                    result = cif_packet_set_item(packet, item_name, val);
                    if (result == CIF_OK) {
                      result = cif_loop_add_packet(loop, packet);
                    }
                    (void) cif_packet_free(packet);
                }
                (void) cif_loop_free(loop);
            }
            break;

        case CIF_OK:
            /* add the item */
            /* FIXME: avoid re-normalizing the name */
            result = cif_loop_add_item(loop, name_orig, val);
            (void) cif_loop_free(loop);
            break;
    }
    SET_RESULT(result);

    FAILURE_TERMINUS;
}

/* External-linkage functions */

#ifdef __cplusplus
extern "C" {
#endif

/* safe to be called by anyone */
int cif_container_free(
        cif_container_t *container
        ) {
    if (container != NULL) {
        /* don't free the 'cif' pointer */
        free(container->code);
        free(container->code_orig);
        free(container);
    }
    return CIF_OK;
}

/* safe to be called by anyone */
int cif_container_destroy(
        cif_container_t *container
        ) {
    if (container == NULL) {
        return CIF_INVALID_HANDLE;
    } else {
        STEP_HANDLING;
        cif_t *cif = container->cif;

        PREPARE_STMT(cif, destroy_container, DESTROY_CONTAINER_SQL);
        if ((sqlite3_bind_int64(cif->destroy_container_stmt, 1, container->id) == SQLITE_OK) 
                && (STEP_STMT(cif, destroy_container) == SQLITE_DONE)) {
            (void) cif_container_free(container);  /* the present implementation always succeeds */
            return (sqlite3_changes(cif->db) <= 0) ? CIF_INVALID_HANDLE : CIF_OK;
        }

        /* Do not destroy the handle, but do drop the prepared statement */
        DROP_STMT(cif, destroy_container);

        return CIF_ERROR;
    }
}

/* safe to be called by anyone */
int cif_container_create_loop(
        cif_container_t *container,
        const UChar *category,
        const UChar *names[],
        cif_loop_t **loop
        ) {
    FAILURE_HANDLING;
    cif_loop_t *temp;
    cif_t *cif;

    if (container == NULL) return CIF_INVALID_HANDLE;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, create_loop, CREATE_LOOP_SQL);
    PREPARE_STMT(cif, get_loopnum, GET_LOOPNUM_SQL);
    PREPARE_STMT(cif, add_loopitem, ADD_LOOPITEM_SQL);

    temp = (cif_loop_t *) malloc(sizeof(cif_loop_t));
    if (temp != NULL) {
        temp->category = cif_u_strdup(category);
        if ((category == NULL) || (temp->category != NULL)) {
            NESTTX_HANDLING;
            /* begin a transaction */
            if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {
                STEP_HANDLING;

                /* create the base loop entity and extract the container-specific loop number */
                if ((sqlite3_bind_int64(cif->create_loop_stmt, 1, container->id) == SQLITE_OK)
                        && (sqlite3_bind_text16(cif->create_loop_stmt, 2, category, -1, SQLITE_STATIC) == SQLITE_OK)
                        && (STEP_STMT(cif, create_loop) == SQLITE_DONE)) {
                    if ((sqlite3_bind_int64(cif->get_loopnum_stmt, 1, container->id) == SQLITE_OK)
                            && (STEP_STMT(cif, get_loopnum) == SQLITE_ROW)) {
                        temp->loop_num = sqlite3_column_int(cif->get_loopnum_stmt, 1);

                        /* Assign the specified item names to the new loop */
                        if ((sqlite3_reset(cif->get_loopnum_stmt) == SQLITE_OK)
                                && (sqlite3_bind_int64(cif->add_loopitem_stmt, 1, container->id) == SQLITE_OK)
                                && (sqlite3_bind_int(cif->add_loopitem_stmt, 2, temp->loop_num) == SQLITE_OK)) {
                            const UChar **name_orig_p;
    
                            for (name_orig_p = names; *name_orig_p; name_orig_p++) {
                                UChar *name;
                                int result;

                                /* validate and normalize the name; bind the normalized and original names */
                                result = cif_normalize_item_name(*name_orig_p, -1, &name, CIF_INVALID_ITEMNAME);
                                if (result != CIF_OK) {
                                    FAIL(soft, result);
                                } else if ((sqlite3_bind_text16(cif->add_loopitem_stmt, 3, name, -1, free) != SQLITE_OK)
                                        || (sqlite3_bind_text16(cif->add_loopitem_stmt, 4, *name_orig_p,
                                                -1, SQLITE_STATIC) != SQLITE_OK)) {
                                    /* name is free()d by sqlite3_bind_text16, even on error */
                                    DEFAULT_FAIL(hard);
                                }
    
                                /* execute the statement */
                                switch (STEP_STMT(cif, add_loopitem)) {
                                    case SQLITE_DONE:
                                        /* successful completion */
                                        break;
                                    case SQLITE_CONSTRAINT:
                                        if (sqlite3_reset(cif->add_loopitem_stmt) == SQLITE_OK) {
                                            FAIL(soft, CIF_DUP_ITEMNAME);
                                        }
                                        /*@fallthrough@*/
                                    default:
                                        DEFAULT_FAIL(hard);
                                }
                            } /* end for (each item name) */

                            /* NOTE: zero packets in the new loop at this point */
   
                            if (COMMIT_NESTTX(cif->db) == SQLITE_OK) {
                                temp->container = container;
                                ASSIGN_TEMP_RESULT(loop, temp, cif_loop_free);
    
                                /* success return */
                                return CIF_OK;
                            }
                        }
                    }
                }
    
                FAILURE_HANDLER(hard):
                /* discard the prepared statements */
                DROP_STMT(cif, add_loopitem);
                DROP_STMT(cif, get_loopnum);
                DROP_STMT(cif, create_loop);
    
                FAILURE_HANDLER(soft):
                /* rollback the transaction */
                ROLLBACK_NESTTX(cif->db);
            }
        }
        (void) cif_loop_free(temp);
    } /* else memory allocation failure */

    /* error return */
    FAILURE_TERMINUS;
}

/* safe to be called by anyone */
int cif_container_get_category_loop(
        cif_container_t *container,
        const UChar *category,
        cif_loop_t **loop
        ) {
    FAILURE_HANDLING;
    cif_loop_t *temp;
    cif_t *cif;

    if (container == NULL) return CIF_INVALID_HANDLE;
    if (category == NULL) return CIF_INVALID_CATEGORY;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_cat_loop, GET_CAT_LOOP_SQL);

    temp = (cif_loop_t *) malloc(sizeof(cif_loop_t));
    if (temp != NULL) {
        temp->category = cif_u_strdup(category);
        if (temp->category != NULL) {
            if ((sqlite3_bind_int64(cif->get_cat_loop_stmt, 1, container->id) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->get_cat_loop_stmt, 2, category, -1, SQLITE_STATIC) == SQLITE_OK)) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, get_cat_loop)) {
                    case SQLITE_DONE:
                        FAIL(soft, CIF_NOSUCH_LOOP);
                    case SQLITE_ROW:
                        temp->loop_num = sqlite3_column_int(cif->get_cat_loop_stmt, 1);
                        temp->container = container;
                        switch (STEP_STMT(cif, get_cat_loop)) {
                            case SQLITE_DONE:
                                ASSIGN_TEMP_RESULT(loop, temp, cif_loop_free);
                                return CIF_OK;
                            case SQLITE_ROW:
                                (void) sqlite3_reset(cif->get_cat_loop_stmt);
                                FAIL(soft, CIF_CAT_NOT_UNIQUE);
                        }
                }
            }
            DROP_STMT(cif, get_cat_loop);
        }

        soft_fail:
        (void) cif_loop_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_container_get_item_loop(
        cif_container_t *container,
        const UChar *item_name,
        cif_loop_t **loop
        ) {
    FAILURE_HANDLING;
    cif_loop_t *temp;

    if (container == NULL) return CIF_INVALID_HANDLE;

    temp = (cif_loop_t *) malloc(sizeof(cif_loop_t));
    if (temp != NULL) {
        UChar *name;
        int result;

        temp->container = container;
        temp->category = NULL;

        result = cif_normalize_item_name(item_name, -1, &name, CIF_INVALID_ITEMNAME);
        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            result = cif_container_get_item_loop_internal(container, name, temp);
            free(name);
            if (result == CIF_OK) {
                ASSIGN_TEMP_RESULT(loop, temp, cif_loop_free);
                return CIF_OK;
            }

            SET_RESULT(result);
        }

        (void) cif_loop_free(temp);
    }

    FAILURE_TERMINUS;
}

/*
 * Does no transaction management (so either autocommits or works in the context of an existing TX)
 */
int cif_container_set_all_values(
        cif_container_t *container,
        const UChar *item_name,
        cif_value_t *val
        ) {
    FAILURE_HANDLING;
    cif_t *cif;

    if (container == NULL) return CIF_INVALID_HANDLE;
    if (item_name == NULL) return CIF_INVALID_ITEMNAME;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, set_all_values, SET_ALL_VALUES_SQL);
    if ((sqlite3_bind_int64(cif->set_all_values_stmt, 7, container->id) == SQLITE_OK)
            && (sqlite3_bind_text16(cif->set_all_values_stmt, 8, item_name, -1, SQLITE_STATIC) == SQLITE_OK)
            && (sqlite3_bind_int(cif->set_all_values_stmt, 1, val->kind) == SQLITE_OK)) {
        STEP_HANDLING;

        SET_VALUE_PROPS(cif->set_all_values_stmt, 1, val, hard, soft);

        /* Execute in an implicit transaction: */
        if (STEP_STMT(cif, set_all_values) == SQLITE_DONE) {
            /* NOTE: sqlite3_changes() is non-transactional and not thread-safe! */
            if (sqlite3_changes(cif->db) > 0) {
                return CIF_OK;
            } else {
                FAIL(soft, CIF_NOSUCH_ITEM);
            }
        }
    }

    FAILURE_HANDLER(hard):
    DROP_STMT(cif, set_all_values);

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

int cif_container_get_value(
        cif_container_t *container,
        const UChar *name,
        cif_value_t **val
        ) {
    FAILURE_HANDLING;
    cif_t *cif = container->cif;
    UChar *name_norm;
    int result;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_value, GET_VALUE_SQL);

    result = cif_normalize_item_name(name, -1, &name_norm, CIF_INVALID_ITEMNAME);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        /* bind the statement parameters */
        if ((sqlite3_bind_text16(cif->get_value_stmt, 2, name_norm, -1, free) == SQLITE_OK)
                && (sqlite3_bind_int64(cif->get_value_stmt, 1, container->id) == SQLITE_OK)) {
            STEP_HANDLING;

            /* start executing the statement (in an implicit transaction) */
            switch (STEP_STMT(cif, get_value)) {
                case SQLITE_DONE:
                    /* no item by the given name in the specified container */
                    FAIL(soft, CIF_NOSUCH_ITEM);
                case SQLITE_ROW:
                    /* a value was found */
                    while (val != NULL) {
                        /* load the value from the DB */
                        cif_value_t *temp = (cif_value_t *) malloc(sizeof(cif_value_t));

                        if (temp != NULL) {
                            GET_VALUE_PROPS(cif->get_value_stmt, 0, temp, inner);

                            /* hand the value off to the caller */
                            if (*val == NULL) {
                                *val = temp;
                                break;
                            } else if ((cif_value_clean(*val) == CIF_OK) 
                                    && (cif_value_clone(temp, val) == CIF_OK)) {
                                free(temp);
                                break;
                            }

                            FAILURE_HANDLER(inner):
                            free(temp);
                        }

                        (void) sqlite3_reset(cif->get_value_stmt);
                        DEFAULT_FAIL(soft);
                    }
                   
                    /* check whether there are any more values */ 
                    switch (STEP_STMT(cif, get_value)) {
                        case SQLITE_ROW:
                            (void) sqlite3_reset(cif->get_value_stmt);
                            FAIL(soft, CIF_AMBIGUOUS_ITEM);
                        case SQLITE_DONE:
                            return CIF_OK;
                    }
            }
        }

        DROP_STMT(cif, get_value);
    }

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

/* Not safe to be called by other library functions */
int cif_container_set_value(
        cif_container_t *container,
        const UChar *name_orig,
        cif_value_t *val
        ) {
    FAILURE_HANDLING;
    cif_loop_t item_loop;
    UChar *name;
    sqlite3 *db = container->cif->db;
    int result;

    if (container == NULL) return CIF_INVALID_HANDLE;
    if (name_orig == NULL) return CIF_INVALID_ITEMNAME;

    result = cif_normalize_item_name(name_orig, -1, &name, CIF_INVALID_ITEMNAME);
    if (result != CIF_OK) {
        SET_RESULT(result);
    } else {
        if (BEGIN(db) == SQLITE_OK) {
            result = cif_container_get_item_loop_internal(container, name, &item_loop);
            switch (result) {
                case CIF_NOSUCH_ITEM:
                    /* nothing to clean up in item_loop in this case */
                    result = cif_container_add_scalar(container, name, name_orig, val);
                    break;
                case CIF_OK:
                    free(item_loop.category);
                    result = cif_container_set_all_values(container, name, val);
                    break;
            }

            if ((result == CIF_OK) && (COMMIT(db) != SQLITE_OK)) {
                result = CIF_ERROR;
            }

            if (result != CIF_OK) {
                (void) ROLLBACK(db);
            }

            SET_RESULT(result);
        }

        free(name);
    }

    FAILURE_TERMINUS;  /* and success terminus, too */
}

/* not safe to be called by other library functions */
int cif_container_remove_item(
        cif_container_t *container,
        const UChar *item_name
        ) {
    FAILURE_HANDLING;
    cif_t *cif;

    if (container == NULL) return CIF_INVALID_HANDLE;
    if (item_name == NULL) return CIF_INVALID_ITEMNAME;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_loop_size, GET_LOOP_SIZE_SQL);
    PREPARE_STMT(cif, remove_item, REMOVE_ITEM_SQL);
    PREPARE_STMT(cif, destroy_loop, DESTROY_LOOP_SQL);

    if ((sqlite3_bind_int64(cif->get_loop_size_stmt, 1, container->id) == SQLITE_OK) 
            && (sqlite3_bind_text16(cif->get_loop_size_stmt, 2, item_name, -1, SQLITE_STATIC) == SQLITE_OK)) {
        int size;

        /*
         * even though only one statement modifies the data, an explicit transaction is used to ensure that the
         * the results of the queries used to choose that statement are still valid when it is executed.  That may be
         * overkill as long as the library does not support multithreaded access to the same CIF, but it's the right
         * thing to do.  Among other things, if the library ever is extended to provide concurrent access support then
         * this will already be up to it.
         */
        if (BEGIN(cif->db) == SQLITE_OK) {
            STEP_HANDLING;
            int loop_num;

            switch (STEP_STMT(cif, get_loop_size)) {
                case SQLITE_DONE:
                    /* The container does not have the specified item */
                    (void) ROLLBACK(cif->db);
                    FAIL(soft, CIF_NOSUCH_ITEM);
                case SQLITE_ROW:
                    size = sqlite3_column_int(cif->get_loop_size_stmt, 2);
                    loop_num = sqlite3_column_int(cif->get_loop_size_stmt, 1);
                    (void) sqlite3_reset(cif->get_loop_size_stmt); 

                    if (size == 1) {
                        /* the item is the only one in its loop, so remove the loop altogether */
                        if ((sqlite3_bind_int64(cif->destroy_loop_stmt, 1, container->id) != SQLITE_OK)
                                || (sqlite3_bind_int(cif->destroy_loop_stmt, 2, loop_num) != SQLITE_OK)
                                || (STEP_STMT(cif, destroy_loop) != SQLITE_DONE)) {
                            (void) sqlite3_reset(cif->destroy_loop_stmt);
                            DEFAULT_FAIL(hard);
                        }
                    } else {
                        /* there are other items in the same loop, so remove just the target item */
                        if ((sqlite3_bind_int64(cif->remove_item_stmt, 1, container->id) != SQLITE_OK) 
                                || (sqlite3_bind_text16(cif->remove_item_stmt, 2, item_name, -1, SQLITE_STATIC)
                                        != SQLITE_OK)
                                || (STEP_STMT(cif, remove_item) != SQLITE_DONE)) {
                            (void) sqlite3_reset(cif->remove_item_stmt);
                            DEFAULT_FAIL(hard);
                        }
                    }
                    if (COMMIT(cif->db) == SQLITE_OK) {
                        return CIF_OK;
                    }
            }
            FAILURE_HANDLER(hard):
            (void) ROLLBACK(cif->db);
        }
    }

    DROP_STMT(cif, destroy_loop);
    DROP_STMT(cif, remove_item);
    DROP_STMT(cif, get_loop_size);

    FAILURE_HANDLER(soft):
    FAILURE_TERMINUS;
}

#ifdef __cplusplus
}
#endif

