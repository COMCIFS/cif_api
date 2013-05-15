/*
 * cif.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>
#include <sqlite3.h>

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
#include "internal/schema.h"
#include "internal/utils.h"
#include "internal/sql.h"
#include "utlist.h"

#define INIT_STMT(cif, stmt_name) cif->stmt_name##_stmt = NULL

/*
 * A callback function used by cif_create() at runtime to detect whether foreign key support is available
 * and successfully enabled:
 */
static int cif_create_callback(void *context, /*@unused@*/ int n_columns UNUSED, char **column_texts,
        /*@unused@*/ char **column_names UNUSED) {
    *((int *) context) = (((*column_texts != NULL) && (**column_texts == '1')) ? 1 : 0);
    return 0;
}

#ifdef DEBUG
static void debug_sql(void *context UNUSED, const char *text) {
    fprintf(stderr, "debug: beginning to execute \"%s\"\n", text);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

int cif_create(cif_t **cif) {
    FAILURE_HANDLING;
    cif_t *temp;

    if (cif == NULL) return CIF_ARGUMENT_ERROR;

    temp = (cif_t *) malloc(sizeof(cif_t));
    if (temp != NULL) {
        if (
                /*
                 * Per the SQLite docs, it is recommended and safe, albeit currently
                 * unnecessary, to initialize the library prior to first use.
                 */
                (DEBUG_WRAP2(sqlite3_initialize()) == SQLITE_OK)

                /*
                 * Open a connection to a temporary SQLite database.  The database will
                 * use UTF-16 as its default character encoding (and also the function
                 * requires the filename to be encoded in UTF-16).
                 */
                && (DEBUG_WRAP2(sqlite3_open16(&cif_uchar_nul, &(temp->db))) == SQLITE_OK)) {
            int fks_enabled = 0;

            /* TODO: probably other DB setup / configuration */

            if (DEBUG_WRAP(temp->db, sqlite3_exec(temp->db, ENABLE_FKS_SQL, cif_create_callback, &fks_enabled, NULL))
                    == SQLITE_OK) {
                if (fks_enabled == 0) {
                    SET_RESULT(CIF_ENVIRONMENT_ERROR);
                } else if (BEGIN(temp->db) == SQLITE_OK) {
                    int i;

                    /* Execute each statement in the 'schema_statements' array */
                    for (i = 0; i < DDL_STMT_COUNT; i++) {
                        if (DEBUG_WRAP(temp->db, sqlite3_exec(temp->db, schema_statements[i], NULL, NULL, NULL)) != SQLITE_OK) {
#ifdef DEBUG
                            fprintf(stderr, "Error occurs in DDL statement %d:\n%s\n", i, schema_statements[i]);
#endif
                            DEFAULT_FAIL(hard);
                        }
                    }

                    if (COMMIT(temp->db) == SQLITE_OK) {
                        /* The database is set up; now initialize the other fields of the cif object */
                        INIT_STMT(temp, create_block);
                        INIT_STMT(temp, get_block);
                        INIT_STMT(temp, get_all_blocks);
                        INIT_STMT(temp, create_frame);
                        INIT_STMT(temp, get_frame);
                        INIT_STMT(temp, get_all_frames);
                        INIT_STMT(temp, destroy_container);
                        INIT_STMT(temp, validate_container);
                        INIT_STMT(temp, create_loop);
                        INIT_STMT(temp, get_loopnum);
                        INIT_STMT(temp, add_loopitem);
                        INIT_STMT(temp, get_cat_loop);
                        INIT_STMT(temp, get_item_loop);
                        INIT_STMT(temp, get_all_loops);
                        INIT_STMT(temp, get_value);
                        INIT_STMT(temp, set_all_values);
                        INIT_STMT(temp, get_loop_size);
                        INIT_STMT(temp, remove_item);
                        INIT_STMT(temp, destroy_loop);
                        INIT_STMT(temp, get_loop_names);
                        INIT_STMT(temp, add_loop_item);
                        INIT_STMT(temp, max_packet_num);
                        INIT_STMT(temp, check_item_loop);
                        INIT_STMT(temp, insert_value);
                        INIT_STMT(temp, update_packet_item);
                        INIT_STMT(temp, insert_packet_item);
                        INIT_STMT(temp, remove_packet);

#ifdef DEBUG
                        sqlite3_trace(temp->db, debug_sql, NULL);
#endif

                        /* success */
                        *cif = temp;
                        return CIF_OK;
                    }
                }

                FAILURE_HANDLER(hard):
                ROLLBACK(temp->db);  /* ignore any error */
            }

            DEBUG_WRAP(temp->db, sqlite3_close(temp->db)); /* ignore any error */
        }
        free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_destroy(cif_t *cif) {
    sqlite3_stmt *stmt = NULL;

    /* ensure that there is no open transaction; will fail harmlessly if not */
    ROLLBACK(cif->db);

    /* clean up any outstanding prepared statements */
    while (CIF_TRUE) {
        stmt = sqlite3_next_stmt(cif->db, stmt);
        if (stmt == NULL) break;
        DEBUG_WRAP(cif->db,sqlite3_finalize(stmt));
    }

    /* close the DB */
    if (DEBUG_WRAP(cif->db,sqlite3_close(cif->db)) == SQLITE_OK) {
        cif->db = NULL;
        free(cif);
        return CIF_OK;
    } else {
        free(cif);
        return CIF_ERROR;
    }
}

int cif_create_block(cif_t *cif, const UChar *code, cif_block_t **block) {
    FAILURE_HANDLING;
    cif_block_t *temp;
    int result;

    if (cif == NULL) return CIF_INVALID_HANDLE;

    INIT_USTDERR;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, create_block, CREATE_BLOCK_SQL);

    temp = (cif_block_t *) malloc(sizeof(cif_block_t));
    if (temp != NULL) {
        /*
         * the block's pointer members must all be initialized, in case it ends up being passed to
         * cif_container_free()
         */
        temp->cif = cif;
        temp->code = NULL;
        temp->code_orig = NULL;
        temp->block_id = -1; /* ensure initialized, but this is meaningful only for save frames */

        /* validate and normalize the block code */
        if ((result = cif_normalize_name(code, -1, &(temp->code), CIF_INVALID_BLOCKCODE)) != CIF_OK) {
            SET_RESULT(result);
        } else {
            temp->code_orig = cif_u_strdup(code);
            if (temp->code_orig != NULL) {
                if(BEGIN(cif->db) == SQLITE_OK) {
                    if(DEBUG_WRAP2(sqlite3_exec(cif->db, "insert into container(id) values (null)", NULL, NULL, NULL))
                            == SQLITE_OK) {
                        temp->id = sqlite3_last_insert_rowid(cif->db);

                        /* Bind the needed parameters to the statement and execute it */
                        if ((sqlite3_bind_int64(cif->create_block_stmt, 1, temp->id) == SQLITE_OK)
                                && (sqlite3_bind_text16(cif->create_block_stmt, 2, temp->code, -1,
                                        SQLITE_STATIC) == SQLITE_OK)
                                && (sqlite3_bind_text16(cif->create_block_stmt, 3, temp->code_orig, -1,
                                        SQLITE_STATIC) == SQLITE_OK)) {
                            STEP_HANDLING;

                            switch (STEP_STMT(cif, create_block)) {
                                case SQLITE_CONSTRAINT:
                                    /* must be a duplicate block code */
                                    /* rollback the transaction and clean up, ignoring any further error */
                                    FAIL(soft, CIF_DUP_BLOCKCODE);
                                case SQLITE_DONE:
                                    if (COMMIT(cif->db) == SQLITE_OK) {
                                        ASSIGN_TEMP_PTR(temp, block, cif_container_free);
                                        return CIF_OK;
                                    }
                                    /* else drop out the bottom */
                            }
                        }

                        /* discard the prepared statement */
                        DROP_STMT(cif, create_block);
                    }
                    FAILURE_HANDLER(soft):
                    /* rollback the transaction, ignoring any further error */
                    ROLLBACK(cif->db);
                } /* else failed to begin a transaction */
            } /* else failed to dup the original code */
        }
        /* free the container object and any resources associated with it */
        (void) cif_container_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_get_block(cif_t *cif, const UChar *code, cif_block_t **block) {
    FAILURE_HANDLING;
    cif_block_t *temp;

    if (cif == NULL) return CIF_INVALID_HANDLE;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_block, GET_BLOCK_SQL);

    temp = (cif_block_t *) malloc(sizeof(cif_block_t));
    if (temp != NULL) {
        int result;

        temp->code_orig = NULL;
        temp->block_id = -1; /* ensure initialized, but this is meaningful only for save frames */
        result = cif_normalize_name(code, -1, &(temp->code), CIF_INVALID_BLOCKCODE);
        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            /* Bind the needed parameters to the create block statement and execute it */
            /* there is a uniqueness constraint on the search key, so at most one row can be returned */
            if (sqlite3_bind_text16(cif->get_block_stmt, 1, temp->code, -1, SQLITE_STATIC) == SQLITE_OK) {
                STEP_HANDLING;

                switch (STEP_STMT(cif, get_block)) {
                    case SQLITE_ROW:
                        temp->cif = cif;
                        temp->id = sqlite3_column_int64(cif->get_block_stmt, 0);
                        GET_COLUMN_STRING(cif->get_block_stmt, 1, temp->code_orig, hard_fail);
                        /* ignore any error here: */
                        sqlite3_reset(cif->get_block_stmt);
                        ASSIGN_TEMP_PTR(temp, block, cif_container_free);
                        return CIF_OK;
                    case SQLITE_DONE:
                        FAIL(soft, CIF_NOSUCH_BLOCK);
                }
            }

            FAILURE_HANDLER(hard):
            /* discard the prepared statement */
            DROP_STMT(cif, get_block);
        }

        FAILURE_HANDLER(soft):
        /* free the container object */
        cif_container_free(temp);
    }

    FAILURE_TERMINUS;
}

int cif_get_all_blocks(cif_t *cif, cif_block_t ***blocks) {
    FAILURE_HANDLING;
    STEP_HANDLING;
    struct block_el {
        cif_block_t block;  /* must be first */
        struct block_el *next;
    } *head = NULL;
    struct block_el **next_block_p = &head;
    struct block_el *next_block;
    cif_block_t **temp_blocks;
    size_t block_count = 0;
    int result;

    if (cif == NULL) return CIF_INVALID_HANDLE;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_all_blocks, GET_ALL_BLOCKS_SQL);

    while (IS_HARD_ERROR(STEP_STMT(cif, get_all_blocks), result) == 0) {
        switch (result) {
            default:
                /* ignore the error code: */
                DEBUG_WRAP(cif->db, sqlite3_reset(cif->get_all_blocks_stmt));
                DEFAULT_FAIL(soft);
            case SQLITE_ROW:
                *next_block_p = (struct block_el *) malloc(sizeof(struct block_el));
                if (*next_block_p == NULL) {
                    DEFAULT_FAIL(soft);
                } else {
                    cif_block_t *temp = &((*next_block_p)->block);

                    /* handle the linked-list next pointer */
                    next_block_p = &((*next_block_p)->next);
                    *next_block_p = NULL;

                    /* initialize the new block with defaults, in case it gets passed to cif_container_free() */
                    temp->cif = cif;
                    temp->code = NULL;
                    temp->code_orig = NULL;
                    temp->block_id = -1;

                    /* read results into the current block object */
                    temp->id = sqlite3_column_int64(cif->get_all_blocks_stmt, 0);
                    GET_COLUMN_STRING(cif->get_all_blocks_stmt, 1, temp->code, HANDLER_LABEL(hard));
                    GET_COLUMN_STRING(cif->get_all_blocks_stmt, 2, temp->code_orig, HANDLER_LABEL(hard));

                    /* maintain a count of blocks read */
                    block_count += 1;
                }
                break;
            case SQLITE_DONE:
                /* aggregate the results into the needed form, and return it */
                temp_blocks = (cif_block_t **) malloc((block_count + 1) * sizeof(cif_block_t *));
                if (temp_blocks == NULL) {
                    DEFAULT_FAIL(soft);
                } else {
                    size_t block_index = 0;

                    for (next_block = head;
                            block_index < block_count;
                            block_index += 1, next_block = next_block->next) {
                        if (next_block == NULL) {
                            free(temp_blocks);
                            FAIL(soft, CIF_INTERNAL_ERROR);
                        }
                        temp_blocks[block_index] = &(next_block->block);
                    }
                    temp_blocks[block_count] = NULL;
                    *blocks = temp_blocks;
                    return CIF_OK;
                }
        }
    }

    FAILURE_HANDLER(hard):
    DROP_STMT(cif, get_all_blocks);

    FAILURE_HANDLER(soft):
    while (head != NULL) {
        next_block = head->next;
        cif_block_free(&(head->block));
        head = next_block;
    }

    FAILURE_TERMINUS;
}

#ifdef __cplusplus
}
#endif

