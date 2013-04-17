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

int cif_block_create_frame(
        cif_block_t *block,
        const UChar *name,
        cif_frame_t **frame
        ) {
    FAILURE_HANDLING;
    cif_frame_t *temp;
    struct cif_s *cif;

    /* validate and normalize the frame name */
    if ((block == NULL) || (block->block_id >= 0)) return CIF_INVALID_HANDLE;

    cif = block->cif;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, create_frame, CREATE_FRAME_SQL);

    temp = (cif_frame_t *) malloc(sizeof(cif_frame_t));
    if (temp != NULL) {
        int result;

        /* the frame's pointer members must all be initialized, in case it ends up being passed to frame_free() */
        temp->cif = cif;
        temp->name_orig = NULL;
        /* as long as we're at it, initialize the block id, too */
        temp->block_id = block->id;

        result = cif_normalize_name(name, -1, &(temp->name), CIF_INVALID_FRAMENAME);
        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            temp->name_orig = cif_u_strdup(name);
            if (temp->name_orig != NULL) {
                if(BEGIN(cif->db) == SQLITE_OK) {
                    if(sqlite3_exec(cif->db, "insert into container values (null)", NULL, NULL, NULL) == SQLITE_OK) {
                        temp->id = sqlite3_last_insert_rowid(cif->db);

                        /* Bind the needed parameters to the statement and execute it */
                        if ((sqlite3_bind_int64(cif->create_frame_stmt, 1, temp->id) == SQLITE_OK)
                                && (sqlite3_bind_int64(cif->create_frame_stmt, 2, block->id) == SQLITE_OK)
                                && (sqlite3_bind_text16(cif->create_frame_stmt, 3, temp->name, -1,
                                        SQLITE_STATIC) == SQLITE_OK)
                                && (sqlite3_bind_text16(cif->create_frame_stmt, 4, temp->name_orig, -1,
                                        SQLITE_STATIC) == SQLITE_OK)) {
                            STEP_HANDLING;

                            switch (STEP_STMT(cif, create_frame)) {
                                case SQLITE_CONSTRAINT:
                                    /* must be a duplicate frame name */
                                    /* rollback the transaction and clean up, ignoring any further error */
                                    (void) sqlite3_reset(cif->create_frame_stmt);
                                    FAIL(soft, CIF_DUP_FRAMENAME);
                                case SQLITE_DONE:
                                    if (COMMIT(cif->db) == SQLITE_OK) {
                                        ASSIGN_TEMP_RESULT(frame, temp, cif_container_free);
                                        return CIF_OK;
                                    }
                                    /* else drop out the bottom */
                            }
                        }
    
                        /* discard the prepared statement */
                        DROP_STMT(cif, create_frame);
                    }
                    FAILURE_HANDLER(soft):
                    /* rollback the transaction, ignoring any further error */
                    ROLLBACK(cif->db);
                } /* else failed to begin a transaction */
            } /* else failed to dup the original name */
        } /* else failed to normalize the name */

        /* free the temporary container object and any resources associated with it */
        (void) cif_container_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_block_get_frame(
        cif_block_t *block,
        const UChar *name,
        cif_frame_t **frame
        ) {
    FAILURE_HANDLING;
    cif_frame_t *temp;
    struct cif_s *cif;

    /* validate, and normalize the frame name */
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

        temp->name_orig = NULL;
        result = cif_normalize_name(name, -1, &(temp->name), CIF_INVALID_FRAMENAME);
        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            /* Bind the needed parameters to the get frame statement and execute it */
            /* there is a uniqueness constraint on the search key, so at most one row can be returned */
            if ((sqlite3_bind_int64(cif->get_frame_stmt, 1, block->id) == SQLITE_OK)
                    && (sqlite3_bind_text16(cif->get_frame_stmt, 2, temp->name, -1, SQLITE_STATIC) == SQLITE_OK)) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, get_frame)) {
                    case SQLITE_ROW:
                        temp->id = sqlite3_column_int64(cif->get_frame_stmt, 0);
                        GET_COLUMN_STRING(cif->get_frame_stmt, 1, temp->name_orig, hard_fail);
                        temp->cif = cif;
                        temp->block_id = block->id;
                        ASSIGN_TEMP_RESULT(frame, temp, cif_container_free);
                        (void) sqlite3_reset(cif->get_frame_stmt);
                        /* There cannot be any more rows, as the DB enforces per-block frame name uniqueness */
                        return CIF_OK;
                    case SQLITE_DONE:
                        FAIL(soft, CIF_NOSUCH_BLOCK);
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

#ifdef __cplusplus
}
#endif

