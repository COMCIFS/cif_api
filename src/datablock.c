/*
 * datablock.c
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
#include "internal/utils.h"
#include "internal/sql.h"

#ifdef __cplusplus
extern "C" {
#endif

int cif_block_create_frame(cif_block_t *block, const UChar *code, cif_frame_t **frame) {
    return cif_block_create_frame_internal(block, code, 0, frame);
}

int cif_block_create_frame_internal(cif_block_t *block, const UChar *code, int lenient, cif_frame_t **frame) {
    FAILURE_HANDLING;
    cif_frame_t *temp;
    struct cif_s *cif;

    TRACELINE;

    /* validate and normalize the frame code */
    if ((block == NULL) || (block->block_id >= 0)) return CIF_INVALID_HANDLE;

    cif = block->cif;

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
        temp->block_id = block->id;
        temp->code = NULL;
        temp->code_orig = NULL;

        result = ((lenient == 0) ? cif_normalize_name(code, -1, &(temp->code), CIF_INVALID_FRAMECODE)
                                 : cif_normalize_common(code, -1, &(temp->code)));

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
                                && (sqlite3_bind_int64(cif->create_frame_stmt, 2, block->id) == SQLITE_OK)
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
                                    /* else drop out the bottom */
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

int cif_block_get_frame(
        cif_block_t *block,
        const UChar *code,
        cif_frame_t **frame
        ) {
    FAILURE_HANDLING;
    cif_frame_t *temp;
    struct cif_s *cif;

    /* validate, and normalize the frame code */
    if ((block == NULL) || (block->block_id >= 0))return CIF_INVALID_HANDLE;

    cif = block->cif;

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
            if ((sqlite3_bind_int64(cif->get_frame_stmt, 1, block->id) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->get_frame_stmt, 2, temp->code, -1, SQLITE_STATIC) == SQLITE_OK)) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, get_frame)) {
                    case SQLITE_ROW:
                        temp->id = sqlite3_column_int64(cif->get_frame_stmt, 0);
                        GET_COLUMN_STRING(cif->get_frame_stmt, 1, temp->code_orig, hard_fail);
                        temp->cif = cif;
                        temp->block_id = block->id;
                        ASSIGN_TEMP_PTR(temp, frame, cif_container_free);
                        sqlite3_reset(cif->get_frame_stmt);
                        /* There cannot be any more rows, as the DB enforces per-block frame code uniqueness */
                        return CIF_OK;
                    case SQLITE_DONE:
                        FAIL(soft, CIF_NOSUCH_FRAME);
                    /* else fall out the bottom */
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

int cif_block_get_all_frames(cif_block_t *block, cif_frame_t ***frames) {
    FAILURE_HANDLING;
    struct frame_el {
        cif_frame_t frame; /* must be first */
        struct frame_el *next;
    } *head = NULL;
    struct frame_el **next_frame_p = &head;
    struct frame_el *next_frame;
    cif_t *cif;

    if (block == NULL) return CIF_INVALID_HANDLE;

    cif = block->cif;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_all_frames, GET_ALL_FRAMES_SQL);

    if (sqlite3_bind_int64(cif->get_all_frames_stmt, 1, block->id) == SQLITE_OK) {
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
                        temp->block_id = block->id;

                        /* read the results into the current frame object */
                        temp->id = sqlite3_column_int64(cif->get_all_frames_stmt, 0);
                        GET_COLUMN_STRING(cif->get_all_frames_stmt, 1, temp->code, HANDLER_LABEL(hard));
                        GET_COLUMN_STRING(cif->get_all_frames_stmt, 2, temp->code_orig, HANDLER_LABEL(hard));

                        /* maintain a count of blocks read */
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

#ifdef __cplusplus
}
#endif

