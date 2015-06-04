/*
 * cif.c
 *
 * Copyright 2014, 2015 John C. Bollinger
 *
 *
 * This file is part of the CIF API.
 *
 * The CIF API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CIF API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "internal/compat.h"

#include <stddef.h>
#include <sqlite3.h>

/* For UChar: */
#include <unicode/umachine.h>

#include "cif.h"
#include "internal/schema.h"
#include "internal/utils.h"
#include "internal/sql.h"
#include "internal/version.h"

#include "utlist.h"

#define INIT_STMT(cif, stmt_name) cif->stmt_name##_stmt = NULL

static int cif_create_callback(void *context, int n_columns, char **column_texts, char **column_names);
static int walk_container(cif_container_tp *container, int depth, cif_handler_tp *handler, void *context);
static int walk_loops(cif_container_tp *container, cif_handler_tp *handler, void *context);
static int walk_loop(cif_loop_tp *loop, cif_handler_tp *handler, void *context);
static int walk_packet(cif_packet_tp *packet, cif_handler_tp *handler, void *context);
static int walk_item(UChar *name, cif_value_tp *value, cif_handler_tp *handler, void *context);


#ifdef DEBUG
static void debug_sql(void *context, const char *text);

static void debug_sql(void *context UNUSED, const char *text) {
    fprintf(stderr, "debug: beginning to execute \"%s\"\n", text);
}

int total_queries = 0;
#endif

#ifdef PERFORM_QUERY_PROFILING
#include <stdio.h>

struct qp_s {
    FILE *output;
    sqlite3_uint64 cumulative_nanos;
};

static void cif_query_profile_callback(void *data, const char *query_text, sqlite3_uint64 time_nanos);

static void cif_query_profile_callback(void *data, const char *query_text, sqlite3_uint64 time_nanos) {
    struct qp_s *qp_params  = (struct qp_s *) data;

    qp_params->cumulative_nanos += time_nanos;
    fprintf(qp_params->output, "query: \"%s\"\nellapsed:%9lu us, cumulative:%9lu us\n\n", query_text,
            (unsigned long) (time_nanos / 1000), (unsigned long) (qp_params->cumulative_nanos / 1000));
}
#endif

/*
 * An SQLite callback function used by cif_create() at runtime to detect whether foreign key support is available
 * and successfully enabled:
 */
static int cif_create_callback(void *context, int n_columns UNUSED, char **column_texts, char **column_names UNUSED) {
    *((int *) context) = (((*column_texts != NULL) && (**column_texts == '1')) ? 1 : 0);
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int cif_get_api_version(char **version) {
    if (version == NULL) return CIF_ARGUMENT_ERROR;

    *version = strdup(CIF_API_VERSION);

    return (*version) ? CIF_OK : CIF_MEMORY_ERROR;
}

int cif_create(cif_tp **cif) {
    FAILURE_HANDLING;
    cif_tp *temp;

    if (cif == NULL) return CIF_ARGUMENT_ERROR;

    temp = (cif_tp *) malloc(sizeof(cif_tp));
    if (temp == NULL) {
        SET_RESULT(CIF_MEMORY_ERROR);
    } else {
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

#ifdef PERFORM_QUERY_PROFILING
            static struct qp_s query_params;

            query_params.output = stderr;
            query_params.cumulative_nanos = 0;

            sqlite3_profile(temp->db, cif_query_profile_callback, &query_params);
#endif

            /* Any other DB setup / configuration needed in the future should go here */

            if (DEBUG_WRAP(temp->db, sqlite3_exec(temp->db, ENABLE_FKS_SQL, cif_create_callback, &fks_enabled, NULL))
                    == SQLITE_OK) {
                if (fks_enabled == 0) {
                    SET_RESULT(CIF_ENVIRONMENT_ERROR);
                } else if (BEGIN(temp->db) == SQLITE_OK) {
                    const char * const *stmt_p;

                    /* Execute each statement in the 'schema_statements' array */
                    for (stmt_p = schema_statements; *stmt_p; stmt_p += 1) {
                        if (DEBUG_WRAP(temp->db, sqlite3_exec(temp->db, *stmt_p, NULL, NULL, NULL)) != SQLITE_OK) {
#ifdef DEBUG
                            fprintf(stderr, "Error occurs in DDL statement %d:\n%s\n",
                                    (int)(stmt_p - schema_statements), *stmt_p);
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
                        INIT_STMT(temp, set_loop_category);
                        INIT_STMT(temp, add_loop_item);
                        INIT_STMT(temp, get_cat_loop);
                        INIT_STMT(temp, get_item_loop);
                        INIT_STMT(temp, get_all_loops);
                        INIT_STMT(temp, prune_container);
                        INIT_STMT(temp, get_value);
                        INIT_STMT(temp, set_all_values);
                        INIT_STMT(temp, get_loop_size);
                        INIT_STMT(temp, remove_item);
                        INIT_STMT(temp, destroy_loop);
                        INIT_STMT(temp, get_loop_names);
                        INIT_STMT(temp, get_packet_num);
                        INIT_STMT(temp, update_packet_num);
                        INIT_STMT(temp, reset_packet_num);
                        INIT_STMT(temp, check_item_loop);
                        INIT_STMT(temp, insert_value);
                        INIT_STMT(temp, update_value);
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

int cif_destroy(cif_tp *cif) {
    sqlite3_stmt *stmt;

    /* ensure that there is no open transaction; will fail harmlessly if there already is none */
    ROLLBACK(cif->db);

    /* Clean up any outstanding prepared statements */
    while ((stmt = sqlite3_next_stmt(cif->db, NULL))) {
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

int cif_create_block(cif_tp *cif, const UChar *code, cif_block_tp **block) {
    return code ? cif_create_block_internal(cif, code, 0, block) : CIF_ARGUMENT_ERROR;
}

int cif_create_block_internal(cif_tp *cif, const UChar *code, int lenient, cif_block_tp **block) {
    FAILURE_HANDLING;
    cif_block_tp *temp;
    int result;

    if (cif == NULL) return CIF_INVALID_HANDLE;

    INIT_USTDERR;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, create_block, CREATE_BLOCK_SQL);

    temp = (cif_block_tp *) malloc(sizeof(cif_block_tp));
    if (temp == NULL) {
        SET_RESULT(CIF_MEMORY_ERROR);
    } else {
        /*
         * the block's pointer members must all be initialized, in case it ends up being passed to
         * cif_container_free()
         */
        temp->cif = cif;
        temp->code = NULL;
        temp->code_orig = NULL;
        temp->parent_id = -1; /* ensure initialized, but this is meaningful only for save frames */

        /* validate (if non-lenient) and normalize the block code */
        result = ((lenient == 0) ? cif_normalize_name(code, -1, &(temp->code), CIF_INVALID_BLOCKCODE)
                                 : cif_normalize(code, -1, &(temp->code)));
        if (result != CIF_OK) {
            SET_RESULT(result);
        } else {
            temp->code_orig = cif_u_strdup(code);
            if (temp->code_orig == NULL) {
                SET_RESULT(CIF_MEMORY_ERROR);
            } else {
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
                                    /* fall through */
                                /* default: do nothing */
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

int cif_get_block(cif_tp *cif, const UChar *code, cif_block_tp **block) {
    FAILURE_HANDLING;
    cif_block_tp *temp;

    if (cif == NULL) return CIF_INVALID_HANDLE;

    /*
     * Create any needed prepared statements, or prepare the existing ones for
     * re-use, exiting this function with an error on failure.
     */
    PREPARE_STMT(cif, get_block, GET_BLOCK_SQL);

    temp = (cif_block_tp *) malloc(sizeof(cif_block_tp));
    if (temp == NULL) {
        SET_RESULT(CIF_MEMORY_ERROR);
    } else {
        int result;

        temp->code_orig = NULL;
        temp->parent_id = -1; /* ensure initialized, but this is meaningful only for save frames */
        result = cif_normalize(code, -1, &(temp->code));
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
                    /* default: do nothing */
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

int cif_get_all_blocks(cif_tp *cif, cif_block_tp ***blocks) {
    FAILURE_HANDLING;
    STEP_HANDLING;
    struct block_el {
        cif_block_tp block;  /* must be first */
        struct block_el *next;
    } *head = NULL;
    struct block_el **next_block_p = &head;
    struct block_el *next_block;
    cif_block_tp **temp_blocks;
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
                    FAIL(soft, CIF_MEMORY_ERROR);
                } else {
                    cif_block_tp *temp = &((*next_block_p)->block);

                    /* handle the linked-list next pointer */
                    next_block_p = &((*next_block_p)->next);
                    *next_block_p = NULL;

                    /* initialize the new block with defaults, in case it gets passed to cif_container_free() */
                    temp->cif = cif;
                    temp->code = NULL;
                    temp->code_orig = NULL;
                    temp->parent_id = -1;

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
                temp_blocks = (cif_block_tp **) malloc((block_count + 1) * sizeof(cif_block_tp *));
                if (temp_blocks == NULL) {
                    FAIL(soft, CIF_MEMORY_ERROR);
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

#define HANDLER_RESULT(handler_name, args, default_val) (handler->handle_ ## handler_name ? \
        handler->handle_ ## handler_name args : (default_val))

int cif_walk(cif_tp *cif, cif_handler_tp *handler, void *context) {
    cif_container_tp **blocks;

    /* call the handler for this element */
    int result = HANDLER_RESULT(cif_start, (cif, context), CIF_TRAVERSE_CONTINUE);

    switch (result) {
        case CIF_TRAVERSE_CONTINUE:
            /* traverse this element's children (its data blocks) */
            result = cif_get_all_blocks(cif, &blocks);
            if (result == CIF_OK) {
                int handle_blocks = CIF_TRUE;
                cif_container_tp **current_block;

                for (current_block = blocks; *current_block; current_block += 1) {
                    if (handle_blocks) {
                        result = walk_container(*current_block, 0, handler, context);

                        switch (result) {
                            case CIF_TRAVERSE_SKIP_SIBLINGS:
                            case CIF_TRAVERSE_END:
                                result = CIF_OK;
                            default:
                                /* Don't break out of the loop, because it cleans up the block handles as it goes */
                                handle_blocks = CIF_FALSE;
                                break;
                            case CIF_TRAVERSE_CONTINUE:
                            case CIF_TRAVERSE_SKIP_CURRENT:
                                break;
                        }
                    }
                    cif_block_free(*current_block);
                }
                free(blocks);

                /* call the end handler if and only if we reached the end of the block list normally */
                if (handle_blocks) {
                    result = HANDLER_RESULT(cif_end, (cif, context), CIF_TRAVERSE_CONTINUE);
                    /* translate valid walk directions to CIF_OK for return from this function */
                    switch (result) {
                        case CIF_TRAVERSE_CONTINUE:
                        case CIF_TRAVERSE_SKIP_CURRENT:
                        case CIF_TRAVERSE_SKIP_SIBLINGS:
                        case CIF_TRAVERSE_END:
                            return CIF_OK;
                        /* default: do nothing */
                    }
                }
            }

            break;
        case CIF_TRAVERSE_SKIP_CURRENT:
        case CIF_TRAVERSE_SKIP_SIBLINGS:
        case CIF_TRAVERSE_END:
            /* valid start handler responses instructing us to return CIF_OK without doing anything further */
            return CIF_OK;
        /* default: do nothing */
    }

    return result;
}

#ifdef __cplusplus
}
#endif

static int walk_container(cif_container_tp *container, int depth, cif_handler_tp *handler, void *context) {
    /* call the handler for this element */
    int result = (depth ? HANDLER_RESULT(frame_start, (container, context), CIF_TRAVERSE_CONTINUE)
                       : HANDLER_RESULT(block_start, (container, context), CIF_TRAVERSE_CONTINUE));

    if (result != CIF_TRAVERSE_CONTINUE) {
        return result;
    } else {
        /* handle this container's save frames */
        cif_container_tp **frames;

        result = cif_container_get_all_frames(container, &frames);
        if (result != CIF_OK) {
            return result;
        } else {
            cif_container_tp **current_frame;
            int handle_frames = CIF_TRUE;
            int handle_loops = CIF_TRUE;

            for (current_frame = frames; *current_frame; current_frame += 1) {
                if (handle_frames) {
                    /* 'result' can only change within this loop while 'handle_frames' is true */
                    result = walk_container(*current_frame, depth + 1, handler, context);
                    switch (result) {
                        case CIF_TRAVERSE_CONTINUE:
                        case CIF_TRAVERSE_SKIP_CURRENT:
                            break;
                        case CIF_TRAVERSE_END:
                        default:
                            /* do not traverse this block's loops */
                            handle_loops = CIF_FALSE;
                            /* fall through */
                        case CIF_TRAVERSE_SKIP_SIBLINGS:
                            /* do not process subsequent frames */
                            handle_frames = CIF_FALSE;
                            break;
                    }
                }
                cif_frame_free(*current_frame);  /* ignore any error */
            }
            free(frames);

            if (!handle_loops) {
                return result;
            }
        }

        /* handle this block's loops */
        result = walk_loops(container, handler, context);
        switch (result) {
            case CIF_TRAVERSE_CONTINUE:
            case CIF_TRAVERSE_SKIP_CURRENT:
                return (depth ? HANDLER_RESULT(frame_end, (container, context), CIF_TRAVERSE_CONTINUE)
                              : HANDLER_RESULT(block_end, (container, context), CIF_TRAVERSE_CONTINUE));
            case CIF_TRAVERSE_SKIP_SIBLINGS:
                return CIF_TRAVERSE_CONTINUE;
            default:
                return result;
        }
    }
}

static int walk_loops(cif_container_tp *container, cif_handler_tp *handler, void *context) {
    int result;
    cif_loop_tp **loops;

    result = cif_container_get_all_loops(container, &loops);
    if (result == CIF_OK) {
        cif_loop_tp **current_loop;
        int handle_loops = CIF_TRUE;

        for (current_loop = loops; *current_loop != NULL; current_loop += 1) {
            if (handle_loops) {
                result = walk_loop(*current_loop, handler, context);
                switch (result) {
                    case CIF_TRAVERSE_SKIP_CURRENT:
                    case CIF_TRAVERSE_CONTINUE:
                        break;
                    default:
                        /* don't traverse any more loops; just release resources */
                        handle_loops = CIF_FALSE;
                        break;
                }
            }
            cif_loop_free(*current_loop);
        }

        free(loops);
    }

    return result;
}

static int walk_loop(cif_loop_tp *loop, cif_handler_tp *handler, void *context) {
    int result = HANDLER_RESULT(loop_start, (loop, context), CIF_TRAVERSE_CONTINUE);

    if (result != CIF_TRAVERSE_CONTINUE) {
        return result;
    } else {
        cif_pktitr_tp *iterator = NULL;

        result = cif_loop_get_packets(loop, &iterator);
        if (result != CIF_OK) {
            return result;
        } else {
            cif_packet_tp *packet = NULL;
            int close_result;

            while ((result = cif_pktitr_next_packet(iterator, &packet)) == CIF_OK) {
                int packet_result = walk_packet(packet, handler, context);

                switch (packet_result) {
                    case CIF_TRAVERSE_CONTINUE:
                    case CIF_TRAVERSE_SKIP_CURRENT:
                        continue;
                    case CIF_TRAVERSE_SKIP_SIBLINGS:
                        packet_result = CIF_TRAVERSE_CONTINUE;
                        break;
                    /* default: CIF_TRAVERSE_END or error code -- do nothing */
                }

                /* control reaches this point only on error */
                result = packet_result;
                break;
            }

            /* Clean up the packet */
            cif_packet_free(packet);

            /* The iterator must be closed or aborted; we choose to close in case the walker modified the CIF */
            if (((close_result = cif_pktitr_close(iterator)) != CIF_OK) && (result == CIF_FINISHED)) {
                result = close_result;
            } /* else suppress any second error in favor of a first one */

            if (result != CIF_FINISHED) {
                return result;
            }

            return HANDLER_RESULT(loop_end, (loop, context), CIF_TRAVERSE_CONTINUE);
        }
    }
}

static int walk_packet(cif_packet_tp *packet, cif_handler_tp *handler, void *context) {
    int handler_result = HANDLER_RESULT(packet_start, (packet, context), CIF_TRAVERSE_CONTINUE);

    if (handler_result != CIF_TRAVERSE_CONTINUE) {
        return handler_result;
    } else {
        struct entry_s *item;

        for (item = packet->map.head; item != NULL; item = (struct entry_s *) item->hh.next) {
            int item_result;

            item_result = walk_item(item->key, &(item->as_value), handler, context);
            switch (item_result) {
                case CIF_TRAVERSE_CONTINUE:
                case CIF_TRAVERSE_SKIP_CURRENT:
                    break;
                case CIF_TRAVERSE_SKIP_SIBLINGS:
                    return CIF_TRAVERSE_CONTINUE;
                default:  /* CIF_TRAVERSE_END or error code */
                    return item_result;
            }
        }

        return HANDLER_RESULT(packet_end, (packet, context), CIF_TRAVERSE_CONTINUE);
    }
}

static int walk_item(UChar *name, cif_value_tp *value, cif_handler_tp *handler, void *context) {
    return HANDLER_RESULT(item, (name, value, context), CIF_TRAVERSE_CONTINUE);
}
