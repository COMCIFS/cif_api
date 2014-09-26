/*
 * container.c
 *
 * Copyright (C) 2013, 2014 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <unicode/ustring.h>
#include "cif.h"
#include "internal/ciftypes.h"
#include "internal/utils.h"
#include "internal/sql.h"

/*
 * The SQLite error message emitted when an attempt is made to cause one
 * container to contain more than one scalar loop, as expressed in the
 * database schema.
 */
static const char scalar_errmsg[22] = "duplicate scalar loop";

/*
 * Adds a scalar item to the specified container, with the given name and value.  The name is assumed
 * normalized and valid.  The container and value are assumed valid.  No transaction management is performed.
 */
static int cif_container_add_scalar(
        cif_container_t *container,
        const UChar *item_name,
        const UChar *name_orig,
        cif_value_t *val
        );

/*
 * Creates a new loop in the specified container, labeled with the specified category, and having original and
 * normalized item names as specified.  No argument checking or additional normalization is performed, not even
 * a check that there is at least one name.  The loop does not initially contain any packets.
 */
static int cif_container_create_loop_internal(
        cif_container_t *container,
        const UChar *category,
        UChar *names[],
        UChar *names_norm[],
        cif_loop_t **loop
        );

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
        cif_container_t *container,
        const UChar *name,
        cif_loop_t *loop
        );

/*
 * Tests whether the specified container handle is valid.  No transaction management is performed, so the test will
 * be performed in the scope of the current transaction if there is one, or in its own transaction otherwise.
 *
 * Returns CIF_OK if the handle is valid, CIF_INVALID_HANDLE if it is determined to be invalid, or CIF_ERROR if an
 * error occurs while determining the handle's validity.
 */
static int cif_container_validate(cif_container_t *container);

/*
 * Tests whether the specified container handle is valid.  No transaction management is performed, so the test will
 * be performed in the scope of the current transaction if there is one, or in its own transaction otherwise.
 *
 * Returns CIF_OK if the handle is valid, CIF_INVALID_HANDLE if it is determined to be invalid, or CIF_ERROR if an
 * error occurs while determining the handle's validity.
 */
static int cif_container_validate(cif_container_t *container) {
    FAILURE_HANDLING;
    cif_t *cif;

    if (container == NULL) {
        return CIF_INVALID_HANDLE;
    }

    cif = container->cif;

    /*
     * Create any needed prepared statements or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, validate_container, VALIDATE_CONTAINER_SQL);

    if ((sqlite3_bind_int64(cif->validate_container_stmt, 1, container->id) == SQLITE_OK)) {
        int result = sqlite3_step(cif->validate_container_stmt);

        sqlite3_reset(cif->validate_container_stmt);
        switch (result) {
            case SQLITE_ROW:
                return CIF_OK;
            case SQLITE_DONE:
                return CIF_INVALID_HANDLE;
            /* default: do nothing */
        }
    }

    DROP_STMT(cif, validate_container);

    FAILURE_TERMINUS;
}

static int cif_container_get_item_loop_internal (
        cif_container_t *container,
        const UChar *name,
        cif_loop_t *loop
        ) {
    FAILURE_HANDLING;
    cif_t *cif = container->cif;

    /*
     * Create any needed prepared statements or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_item_loop, GET_ITEM_LOOP_SQL);

    /* Assign the container and category now to be sure that they have sensible values in the case of an error */
    loop->container = container;
    loop->category = NULL;
    loop->names = NULL;

    if ((sqlite3_bind_text16(cif->get_item_loop_stmt, 2, name, -1, SQLITE_STATIC) == SQLITE_OK)
           && (sqlite3_bind_int64(cif->get_item_loop_stmt, 1, container->id) == SQLITE_OK)) {
        STEP_HANDLING;

        switch (STEP_STMT(cif, get_item_loop)) {
            case SQLITE_DONE:
                FAIL(soft, CIF_NOSUCH_ITEM);
            case SQLITE_ROW:
                GET_COLUMN_STRING(cif->get_item_loop_stmt, 1, loop->category, soft_fail);
                loop->loop_num = sqlite3_column_int(cif->get_item_loop_stmt, 0);

                /* verify that there was only one result row */
                switch (STEP_STMT(cif, get_item_loop)) {
                    case SQLITE_DONE:
                        return CIF_OK;
                    case SQLITE_ROW:
                        sqlite3_reset(cif->get_item_loop_stmt);
                        FAIL(soft, CIF_INTERNAL_ERROR);
                    /* otherwise do nothing */
                }
            /* default: do nothing */
        }
    }  /* else fall-through / fail */

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
    UChar null_char = 0;
    UChar *none = NULL;
    cif_packet_t *packet = NULL;
    int result;
    int num_packets;

    TRACELINE;
    result = cif_container_get_category_loop(container, &null_char, &loop);
    switch(result) {
        case CIF_NOSUCH_LOOP:
            /* first create the scalar loop */
            TRACELINE;
            if ((result = cif_container_create_loop_internal(container, &null_char, &none, &none, &loop)) != CIF_OK) {
                break;
            }
            /* fall through to add the item */
        case CIF_OK:
            /* add the item */
            TRACELINE;
            result = cif_loop_add_item_internal(loop, name_orig, item_name, val, &num_packets);

            if ((result == CIF_OK) && (num_packets == 0) && ((result = cif_packet_create(&packet, NULL)) == CIF_OK)) {
                if ((result = cif_packet_set_item(packet, item_name, val)) == CIF_OK) {
                    /* No need to specify values for any other scalars here, even if the loop defines some */
                    result = cif_loop_add_packet(loop, packet);
                }
                cif_packet_free(packet);
            }

            cif_loop_free(loop);
            break;
        /* default: do nothing */
    }
    SET_RESULT(result);

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
extern "C" {
#endif

int cif_container_create_frame(cif_container_t *container, const UChar *code, cif_frame_t **frame) {
    return cif_container_create_frame_internal(container, code, 0, frame);
}

int cif_container_create_frame_internal(cif_container_t *container, const UChar *code, int lenient, cif_frame_t **frame) {
    FAILURE_HANDLING;
    cif_frame_t *temp;
    struct cif_s *cif;

    TRACELINE;

    /* validate and normalize the frame code */
    if (container == NULL) return CIF_INVALID_HANDLE;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, create_frame, CREATE_FRAME_SQL);

    TRACELINE;

    temp = (cif_frame_t *) malloc(sizeof(cif_frame_t));
    if (temp != NULL) {
        int result;

        /* the frame object must be initialized in case it ends up being passed to cif_container_free() */
        temp->cif = cif;
        temp->parent_id = container->id;
        temp->code = NULL;
        temp->code_orig = NULL;

        result = ((lenient == 0) ? cif_normalize_name(code, -1, &(temp->code), CIF_INVALID_FRAMECODE)
                                 : cif_normalize(code, -1, &(temp->code)));

        TRACELINE;

        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            temp->code_orig = cif_u_strdup(code);
            if (temp->code_orig != NULL) {
                if(BEGIN(cif->db) == SQLITE_OK) {
                    TRACELINE;
                    if(sqlite3_exec(cif->db, "insert into container(id) values (null)", NULL, NULL, NULL)
                            == SQLITE_OK) {
                        temp->id = sqlite3_last_insert_rowid(cif->db);

                        TRACELINE;

                        /* Bind the needed parameters to the statement and execute it */
                        if ((sqlite3_bind_int64(cif->create_frame_stmt, 1, temp->id) == SQLITE_OK)
                                && (sqlite3_bind_int64(cif->create_frame_stmt, 2, container->id) == SQLITE_OK)
                                && (sqlite3_bind_text16(cif->create_frame_stmt, 3, temp->code, -1,
                                        SQLITE_STATIC) == SQLITE_OK)
                                && (sqlite3_bind_text16(cif->create_frame_stmt, 4, temp->code_orig, -1,
                                        SQLITE_STATIC) == SQLITE_OK)) {
                            STEP_HANDLING;

                            TRACELINE;

                            switch (STEP_STMT(cif, create_frame)) {
                                case SQLITE_CONSTRAINT:
                                    /* must be a duplicate frame code */
                                    /* rollback the transaction and clean up, ignoring any further error */
                                    (void) sqlite3_reset(cif->create_frame_stmt);
                                    FAIL(soft, CIF_DUP_FRAMECODE);
                                case SQLITE_DONE:
                                    if (COMMIT(cif->db) == SQLITE_OK) {
                                        ASSIGN_TEMP_PTR(temp, frame, cif_container_free);
                                        return CIF_OK;
                                    }
                                    /* fall through */
                                /* default: do nothing */
                            }

                            TRACELINE;
                        }
    
                        /* discard the prepared statement */
                        DROP_STMT(cif, create_frame);
                    }
                    FAILURE_HANDLER(soft):
                    /* rollback the transaction, ignoring any further error */
                    ROLLBACK(cif->db);
                } /* else failed to begin a transaction */
            } /* else failed to dup the original code */
        } /* else failed to normalize the code */

        /* free the temporary container object and any resources associated with it */
        cif_container_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_container_get_frame(
        cif_container_t *container,
        const UChar *code,
        cif_frame_t **frame
        ) {
    FAILURE_HANDLING;
    cif_frame_t *temp;
    struct cif_s *cif;

    /* validate, and normalize the frame code */
    if (container == NULL) return CIF_INVALID_HANDLE;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_frame, GET_FRAME_SQL);

    temp = (cif_frame_t *) malloc(sizeof(cif_frame_t));
    if (temp != NULL) {
        int result;

        temp->code_orig = NULL;
        result = cif_normalize_name(code, -1, &(temp->code), CIF_INVALID_FRAMECODE);
        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            /* Bind the needed parameters to the get frame statement and execute it */
            /* there is a uniqueness constraint on the search key, so at most one row can be returned */
            if ((sqlite3_bind_int64(cif->get_frame_stmt, 1, container->id) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->get_frame_stmt, 2, temp->code, -1, SQLITE_STATIC) == SQLITE_OK)) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, get_frame)) {
                    case SQLITE_ROW:
                        temp->id = sqlite3_column_int64(cif->get_frame_stmt, 0);
                        GET_COLUMN_STRING(cif->get_frame_stmt, 1, temp->code_orig, hard_fail);
                        temp->cif = cif;
                        temp->parent_id = container->id;
                        ASSIGN_TEMP_PTR(temp, frame, cif_container_free);
                        sqlite3_reset(cif->get_frame_stmt);
                        /* There cannot be any more rows, as the DB enforces per-container frame code uniqueness */
                        return CIF_OK;
                    case SQLITE_DONE:
                        FAIL(soft, CIF_NOSUCH_FRAME);
                        /* fall through */
                    /* default: do nothing */
                }
            }

            FAILURE_HANDLER(hard):
            /* discard the prepared statement */
            DROP_STMT(cif, get_frame);
        }

        FAILURE_HANDLER(soft):
        /* free the container object */
        cif_container_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_container_get_all_frames(cif_container_t *container, cif_frame_t ***frames) {
    FAILURE_HANDLING;
    struct frame_el {
        cif_frame_t frame; /* must be first */
        struct frame_el *next;
    } *head = NULL;
    struct frame_el **next_frame_p = &head;
    struct frame_el *next_frame;
    cif_t *cif;

    if (container == NULL) return CIF_INVALID_HANDLE;

    cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_all_frames, GET_ALL_FRAMES_SQL);

    if (sqlite3_bind_int64(cif->get_all_frames_stmt, 1, container->id) == SQLITE_OK) {
        STEP_HANDLING;
        int result;
        size_t frame_count = 0;
        cif_frame_t **temp_frames;

        while (IS_HARD_ERROR(STEP_STMT(cif, get_all_frames), result) == 0) {
            switch (result) {
                default:
                    /* ignore the error code: */
                    DEBUG_WRAP(cif->db, sqlite3_reset(cif->get_all_frames_stmt));
                    DEFAULT_FAIL(soft);
                case SQLITE_ROW:
                    *next_frame_p = (struct frame_el *) malloc(sizeof(struct frame_el));
                    if (*next_frame_p == NULL) {
                        DEFAULT_FAIL(soft);
                    } else {
                        cif_frame_t *temp = &((*next_frame_p)->frame);

                        /* handle the linked-list next pointer */
                        next_frame_p = &((*next_frame_p)->next);
                        *next_frame_p = NULL;

                        /* initialize the new frame with defaults, in case it gets passed to cif_container_free() */
                        temp->cif = cif;
                        temp->code = NULL;
                        temp->code_orig = NULL;
                        temp->parent_id = container->id;

                        /* read the results into the current frame object */
                        temp->id = sqlite3_column_int64(cif->get_all_frames_stmt, 0);
                        GET_COLUMN_STRING(cif->get_all_frames_stmt, 1, temp->code, HANDLER_LABEL(hard));
                        GET_COLUMN_STRING(cif->get_all_frames_stmt, 2, temp->code_orig, HANDLER_LABEL(hard));

                        /* maintain a count of containers read */
                        frame_count += 1;
                    }
                    break;
                case SQLITE_DONE:
                    /* aggregate the results into the needed form, and return it */
                    temp_frames = (cif_frame_t **) malloc((frame_count + 1) * sizeof(cif_frame_t *));
                    if (temp_frames == NULL) {
                        DEFAULT_FAIL(soft);
                    } else {
                        size_t frame_index = 0;

                        for (next_frame = head;
                                frame_index < frame_count;
                                frame_index += 1, next_frame = next_frame->next) {
                            if (next_frame == NULL) {
                                free(temp_frames);
                                FAIL(soft, CIF_INTERNAL_ERROR);
                            }
                            temp_frames[frame_index] = &(next_frame->frame);
                        }
                        temp_frames[frame_count] = NULL;
                        *frames = temp_frames;
                        return CIF_OK;
                    }
            }
        }
    }

    FAILURE_HANDLER(hard):
    DROP_STMT(cif, get_all_frames);

    FAILURE_HANDLER(soft):
    while (head != NULL) {
        next_frame = head->next;
        cif_frame_free(&(head->frame));
        head = next_frame;
    }

    FAILURE_TERMINUS;
}

/* safe to be called by anyone */
void cif_container_free(
        cif_container_t *container
        ) {
    if (container != NULL) {
        /* don't free the 'cif' pointer */
        free(container->code);
        free(container->code_orig);
        free(container);
    }
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
            cif_container_free(container);
            return (sqlite3_changes(cif->db) <= 0) ? CIF_INVALID_HANDLE : CIF_OK;
        }

        /* Do not destroy the handle, but do drop the prepared statement */
        DROP_STMT(cif, destroy_container);

        return CIF_ERROR;
    }
}

/* does not touch the database */
int cif_container_get_code(
        cif_container_t *container,
        UChar **code) {
    UChar *temp = cif_u_strdup(container->code_orig);

    if (temp != NULL) {
        *code = temp;
        return CIF_OK;
    } else {
        return CIF_ERROR;
    }
}

/* does not touch the database */
int cif_container_assert_block(cif_container_t *container) {
    return ((container == NULL) ? CIF_ERROR : (container->parent_id < 0) ? CIF_OK : CIF_ARGUMENT_ERROR);
}

/*
 * Safe to be called by anyone; this is an argument-checking, name-normalizing
 * wrapper around cif_container_create_loop_internal
 */
int cif_container_create_loop(
        cif_container_t *container,
        const UChar *category,
        UChar *names[],
        cif_loop_t **loop
        ) {
    FAILURE_HANDLING;
    UChar **name_orig_p;
    UChar **names_norm;

    TRACELINE;

    if (container == NULL) return CIF_INVALID_HANDLE;
    if (names == NULL)     return CIF_ARGUMENT_ERROR;
    if (*names == NULL)    return CIF_NULL_LOOP;

    /* count the names */
    for (name_orig_p = names; *name_orig_p; name_orig_p += 1) {
        /* do nothing */
    }

    names_norm = (UChar **) malloc((1 + (name_orig_p - names)) * sizeof(UChar *));
    if (names_norm != NULL) {
        UChar **name_norm_p = names_norm;
        int result;

        /* validate and normalize the names */
        for (name_orig_p = names; *name_orig_p; name_orig_p += 1) {
            UChar *name;

            /* validate and normalize the name */
            if ((result = cif_normalize_item_name(*name_orig_p, -1, &name, CIF_INVALID_ITEMNAME)) != CIF_OK) {
                FAIL(cleanup, result);
            }
            *(name_norm_p++) = name;
        }
        *(name_norm_p++) = NULL;

        SET_RESULT(cif_container_create_loop_internal(container, category, names, names_norm, loop));

        FAILURE_HANDLER(cleanup):  /* Reached on success, too */
        while (name_norm_p > names_norm) {
            free(*(--name_norm_p));
        }
        free(names_norm);
    }

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
            temp->names = NULL;
            if ((sqlite3_bind_int64(cif->get_cat_loop_stmt, 1, container->id) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->get_cat_loop_stmt, 2, category, -1, SQLITE_STATIC) == SQLITE_OK)) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, get_cat_loop)) {
                    case SQLITE_DONE:
                        FAIL(soft, CIF_NOSUCH_LOOP);
                    case SQLITE_ROW:
                        temp->loop_num = sqlite3_column_int(cif->get_cat_loop_stmt, 0);
                        temp->container = container;
                        switch (STEP_STMT(cif, get_cat_loop)) {
                            case SQLITE_DONE:
                                ASSIGN_TEMP_PTR(temp, loop, cif_loop_free);
                                return CIF_OK;
                            case SQLITE_ROW:
                                (void) sqlite3_reset(cif->get_cat_loop_stmt);
                                FAIL(soft, CIF_CAT_NOT_UNIQUE);
                                /* fall through */
                            /* default: do nothing */
                        }
                        /* fall through */
                    /* default: do nothing */
                }
            }
            DROP_STMT(cif, get_cat_loop);
        }

        soft_fail:
        cif_loop_free(temp);
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
        temp->names = NULL;

        result = cif_normalize_item_name(item_name, -1, &name, CIF_INVALID_ITEMNAME);
        if (result == CIF_INVALID_ITEMNAME) {
            SET_RESULT(CIF_NOSUCH_ITEM);
        } else if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            result = cif_container_get_item_loop_internal(container, name, temp);
            free(name);
            if (result == CIF_OK) {
                ASSIGN_TEMP_PTR(temp, loop, cif_loop_free);
                return CIF_OK;
            }

            SET_RESULT(result);
        }

        cif_loop_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_container_get_all_loops(cif_container_t *container, cif_loop_t ***loops) {
    FAILURE_HANDLING;
    NESTTX_HANDLING;
    cif_t *cif;

    if (container == NULL) return CIF_INVALID_HANDLE;

    cif = container->cif;

    if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {
        int result = cif_container_validate(container);

        if (result == CIF_OK) {        
            struct loop_el {
                cif_loop_t loop; /* must be first */
                struct loop_el *next;
            } *head = NULL;
            struct loop_el **next_loop_p = &head;
            struct loop_el *next_loop;

            /*
             * Create any needed prepared statements, or prepare the existing one(s)
             * for re-use, exiting this function with an error on failure.
             */
            PREPARE_STMT(cif, get_all_loops, GET_ALL_LOOPS_SQL);

            if (sqlite3_bind_int64(cif->get_all_loops_stmt, 1, container->id) == SQLITE_OK) {
                STEP_HANDLING;
                cif_loop_t **temp_loops;
                size_t loop_count = 0;

                while (IS_HARD_ERROR(STEP_STMT(cif, get_all_loops), result) == 0) {
                    switch (result) {
                        default:
                            DEBUG_WRAP(cif->db, sqlite3_reset(cif->get_all_loops_stmt));
                            DEFAULT_FAIL(soft);
                        case SQLITE_ROW:
                            /* add a new loop object to the linked list */
                            *next_loop_p = (struct loop_el *) malloc(sizeof(struct loop_el));
                            if (*next_loop_p == NULL) {
                                DEFAULT_FAIL(soft);
                            } else {
                                cif_loop_t *temp = &((*next_loop_p)->loop);

                                /* handle the linked-list next pointer */
                                next_loop_p = &((*next_loop_p)->next);
                                *next_loop_p = NULL;

                                /* initialize the new loop object */
                                temp->container = container;
                                temp->loop_num = sqlite3_column_int(cif->get_all_loops_stmt, 0);
                                temp->names = NULL;
                                GET_COLUMN_STRING(cif->get_all_loops_stmt, 1, temp->category, HANDLER_LABEL(hard));
                                loop_count += 1;
                            }
                            break;
                        case SQLITE_DONE:
                            /* aggregate the results into the needed form, and return it */
                            temp_loops = (cif_loop_t **) malloc((loop_count + 1) * sizeof(cif_loop_t *));

                            if (temp_loops == NULL) {
                                DEFAULT_FAIL(soft);
                            } else {
                                size_t loop_index;

                                for (loop_index = 0, next_loop = head;
                                        loop_index < loop_count;
                                        loop_index += 1, next_loop = next_loop->next) {
                                    if (next_loop == NULL) {
                                        while (loop_index > 0) {
                                            cif_loop_free(temp_loops[--loop_index]);
                                        }
                                        free(temp_loops);
                                        FAIL(soft, CIF_INTERNAL_ERROR);
                                    }
                                    temp_loops[loop_index] = &(next_loop->loop);
                                }
                                temp_loops[loop_count] = NULL;
                                *loops = temp_loops;
                                ROLLBACK_NESTTX(cif->db);
                                return CIF_OK;
                            }
                    }
                }
            }

            FAILURE_HANDLER(hard):
            DROP_STMT(cif, get_all_loops);

            FAILURE_HANDLER(soft):
            while (head != NULL) {
                next_loop = head->next;
                free(head);
                head = next_loop;
            }
        } else {
            SET_RESULT(result);
        }

        ROLLBACK_NESTTX(cif->db);
    }

    FAILURE_TERMINUS;
}

int cif_container_prune(cif_container_t *container) {
    FAILURE_HANDLING;
    cif_t *cif = container->cif;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, prune_container, PRUNE_SQL);
    if (sqlite3_bind_int64(cif->prune_container_stmt, 1, container->id) == SQLITE_OK) {
        STEP_HANDLING;

        switch (STEP_STMT(cif, prune_container)) {
            case SQLITE_DONE:
                return CIF_OK;
            case SQLITE_MISUSE:
                FAIL(soft, CIF_MISUSE);
                /* fall through */
            /* default: do nothing */
        }
    }

    FAILURE_HANDLER(soft):
    DROP_STMT(cif, prune_container);

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
    TRACELINE;
    if ((sqlite3_bind_int64(cif->set_all_values_stmt, 7, container->id) == SQLITE_OK)
            && (sqlite3_bind_text16(cif->set_all_values_stmt, 8, item_name, -1, SQLITE_STATIC) == SQLITE_OK)) {
        STEP_HANDLING;

        SET_VALUE_PROPS(cif->set_all_values_stmt, 0, val, hard, soft);

        /* Execute in an implicit transaction: */
        TRACELINE;
        if (STEP_STMT(cif, set_all_values) == SQLITE_DONE) {
            /* OK even if zero rows changed => zero-packet loop */
            return CIF_OK;
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

    TRACELINE;
    result = cif_normalize_item_name(name, -1, &name_norm, CIF_NOSUCH_ITEM);
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
                    TRACELINE;
                    while (val != NULL) {
                        /* load the value from the DB */
                        cif_value_t *temp = (cif_value_t *) malloc(sizeof(cif_value_t));

                        if (temp != NULL) {
                            GET_VALUE_PROPS(cif->get_value_stmt, 0, temp, inner);

                            /* hand the value off to the caller */
                            if (*val == NULL) {
                                *val = temp;
                                break;
                            } else {
                                cif_value_clean(*val);
                                /* make a _shallow_ copy of 'temp' where 'val' points */
                                memcpy(*val, temp, sizeof(cif_value_t));
                                free(temp);
                                break;
                            }

                            FAILURE_HANDLER(inner):
                            free(temp);
                        }

                        sqlite3_reset(cif->get_value_stmt);
                        DEFAULT_FAIL(soft);
                    }
                   
                    /* check whether there are any more values */ 
                    switch (STEP_STMT(cif, get_value)) {
                        case SQLITE_ROW:
                            sqlite3_reset(cif->get_value_stmt);
                            FAIL(soft, CIF_AMBIGUOUS_ITEM);
                        case SQLITE_DONE:
                            return CIF_OK;
                        /* default: do nothing */
                    }
                    /* fall through */
                /* default: do nothing */
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
            cif_value_t temp_val;

            if (val == NULL) {
                temp_val.kind = CIF_UNK_KIND;
                val = &temp_val;
            }
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
                /* default: do nothing */
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
    UChar *normalized_name;
    int result;

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

    /* Note: this code assumes that items bearing invalid names cannot be introduced into the DB */
    result = cif_normalize_item_name(item_name, -1, &normalized_name, CIF_NOSUCH_ITEM);
    if (result != CIF_OK) {
        FAIL(soft, result);
    } else if ((sqlite3_bind_int64(cif->get_loop_size_stmt, 1, container->id) == SQLITE_OK) 
            && (sqlite3_bind_text16(cif->get_loop_size_stmt, 2, normalized_name, -1, SQLITE_STATIC) == SQLITE_OK)) {
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
                    ROLLBACK(cif->db);
                    FAIL(soft, CIF_NOSUCH_ITEM);
                case SQLITE_ROW:
                    size = sqlite3_column_int(cif->get_loop_size_stmt, 1);
                    loop_num = sqlite3_column_int(cif->get_loop_size_stmt, 0);
                    sqlite3_reset(cif->get_loop_size_stmt); 

                    if (size == 1) {
                        /* the item is the only one in its loop, so remove the loop altogether */
                        if ((sqlite3_bind_int64(cif->destroy_loop_stmt, 1, container->id) != SQLITE_OK)
                                || (sqlite3_bind_int(cif->destroy_loop_stmt, 2, loop_num) != SQLITE_OK)
                                || (STEP_STMT(cif, destroy_loop) != SQLITE_DONE)) {
                            sqlite3_reset(cif->destroy_loop_stmt);
                            DEFAULT_FAIL(hard);
                        }
                    } else {
                        /* there are other items in the same loop, so remove just the target item */
                        if ((sqlite3_bind_int64(cif->remove_item_stmt, 1, container->id) != SQLITE_OK) 
                                || (sqlite3_bind_text16(cif->remove_item_stmt, 2, normalized_name, -1, SQLITE_STATIC)
                                        != SQLITE_OK)
                                || (STEP_STMT(cif, remove_item) != SQLITE_DONE)) {
                            sqlite3_reset(cif->remove_item_stmt);
                            DEFAULT_FAIL(hard);
                        }
                    }
                    if (COMMIT(cif->db) == SQLITE_OK) {
                        return CIF_OK;
                    }
                    /* fall through */
                /* default: do nothing */
            }
            FAILURE_HANDLER(hard):
            ROLLBACK(cif->db);
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

/*
 * The main implementation of loop creation; assumes arguments are valid and
 * consistent.
 */
static int cif_container_create_loop_internal(
        cif_container_t *container,
        const UChar *category,
        UChar *names[],
        UChar *names_norm[],
        cif_loop_t **loop
        ) {
    FAILURE_HANDLING;
    cif_loop_t *temp;
    cif_t *cif;

    cif = container->cif;

    TRACELINE;

    /*
     * Create any needed prepared statements, or prepare the existing one(s)
     * for re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, create_loop, CREATE_LOOP_SQL);
    PREPARE_STMT(cif, get_loopnum, GET_LOOPNUM_SQL);
    PREPARE_STMT(cif, add_loop_item, ADD_LOOP_ITEM_SQL);

    TRACELINE;

    temp = (cif_loop_t *) malloc(sizeof(cif_loop_t));
    if (temp != NULL) {

        temp->category = cif_u_strdup(category);
        if ((category == NULL) || (temp->category != NULL)) {
            NESTTX_HANDLING;

            TRACELINE;
            temp->names = NULL;

            /* begin a transaction */
            if (BEGIN_NESTTX(cif->db) == SQLITE_OK) {
                STEP_HANDLING;
                int result;

                /* create the base loop entity and extract the container-specific loop number */
                if ((sqlite3_bind_int64(cif->create_loop_stmt, 1, container->id) == SQLITE_OK)
                        && (sqlite3_bind_text16(cif->create_loop_stmt, 2, category, -1, SQLITE_STATIC) == SQLITE_OK)
                        && (IS_HARD_ERROR(STEP_STMT(cif, create_loop), result) == 0)) {
                    TRACELINE;
                    /* 'result' is expected to be SQLITE_DONE on success */
                    switch (result) {
                        default:
                            /* Error code ignored, but it should reiterate the code recorded in 'result': */
                            sqlite3_reset(cif->create_loop_stmt);
                            DEFAULT_FAIL(soft);
                        case SQLITE_CONSTRAINT:
                            /*
                             * The statement must be reset before the error message is retrieved, else only the generic
                             * message associated with the error code is obtained, not the message emitted by raise()
                             * (when that's applicable).  [SQLite 3.6.20]
                             */
                            /* Error code ignored, but it should reiterate SQLITE_COINSTRAINT: */
                            sqlite3_reset(cif->create_loop_stmt);
                            if (strcmp(DEBUG_MSG("sqlite3 error message", sqlite3_errmsg(cif->db)), scalar_errmsg) == 0) {
                                /* duplicate scalar loop */
                                FAIL(soft, CIF_RESERVED_LOOP);
                            } else {
                                /* apparently, the container id does not refer to an existing container */
                                FAIL(soft, CIF_INVALID_HANDLE);
                            }
                        case SQLITE_DONE:
                            if ((sqlite3_bind_int64(cif->get_loopnum_stmt, 1, container->id) == SQLITE_OK)
                                    && (STEP_STMT(cif, get_loopnum) == SQLITE_ROW)) {
                                int t;

                                TRACELINE;

                                temp->loop_num = sqlite3_column_int(cif->get_loopnum_stmt, 0);

                                TRACELINE;

                                /* Assign the specified item names to the new loop */
                                if ((IS_HARD_ERROR(sqlite3_reset(cif->get_loopnum_stmt), t) == 0)
                                        && (sqlite3_bind_int64(cif->add_loop_item_stmt, 1, container->id) == SQLITE_OK)
                                        && (sqlite3_bind_int(cif->add_loop_item_stmt, 4, temp->loop_num) == SQLITE_OK)) {
                                    int name_index;

                                    TRACELINE;

                                    for (name_index = 0; names[name_index] != NULL; name_index += 1) {
                                        if ((sqlite3_bind_text16(cif->add_loop_item_stmt, 2, names_norm[name_index],
                                                -1, SQLITE_STATIC) != SQLITE_OK)
                                                || (sqlite3_bind_text16(cif->add_loop_item_stmt, 3, names[name_index],
                                                        -1, SQLITE_STATIC) != SQLITE_OK)) {
                                            DEFAULT_FAIL(hard);
                                        }

                                        /* execute the statement */
                                        switch (STEP_STMT(cif, add_loop_item)) {
                                            case SQLITE_DONE:
                                                /* successful completion */
                                                break;
                                            case SQLITE_CONSTRAINT:
                                                /* reseting the statement should recapitulate the error code */
                                                if (sqlite3_reset(cif->add_loop_item_stmt) == SQLITE_CONSTRAINT) {
                                                    FAIL(soft, CIF_DUP_ITEMNAME);
                                                } /* else a new error ocurred */
                                                /* fall through */
                                            default:
                                                DEFAULT_FAIL(hard);
                                        }
                                    } /* end for (each item name) */

                                    /* NOTE: zero packets in the new loop at this point */

                                    if (COMMIT_NESTTX(cif->db) == SQLITE_OK) {
                                        temp->container = container;
                                        ASSIGN_TEMP_PTR(temp, loop, cif_loop_free);

                                        /* success return */
                                        return CIF_OK;
                                    }
                                }
                            }
                        }
                }

                FAILURE_HANDLER(hard):
                /* discard the prepared statements */
                DROP_STMT(cif, add_loop_item);
                DROP_STMT(cif, get_loopnum);
                DROP_STMT(cif, create_loop);

                FAILURE_HANDLER(soft):
                /* rollback the transaction */
                TRACELINE;
                ROLLBACK_NESTTX(cif->db);
            }
        }
        cif_loop_free(temp);
    } /* else memory allocation failure */

    /* error return */
    FAILURE_TERMINUS;
}

