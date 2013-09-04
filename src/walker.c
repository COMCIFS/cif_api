/*
 * walker.c
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

#include "cif.h"
#include "cif_walker.h"
#include "internal/ciftypes.h"
#include "internal/utils.h"

static int walk_block(cif_container_t *block, cif_walk_handler_t *handler, void *context);
static int walk_frame(cif_container_t *frame, cif_walk_handler_t *handler, void *context);
static int walk_loops(cif_container_t *container, cif_walk_handler_t *handler, void *context);
static int walk_loop(cif_loop_t *loop, cif_walk_handler_t *handler, void *context);
static int walk_packet(cif_packet_t *packet, cif_walk_handler_t *handler, void *context);
static int walk_item(UChar *name, cif_value_t *value, cif_walk_handler_t *handler, void *context);

#define HANDLER_RESULT(handler_name, args, default_val) (handler->handle_ ## handler_name ? \
        handler->handle_ ## handler_name args : (default_val))

int cif_walk(cif_t *cif, cif_walk_handler_t *handler, void *context) {
    cif_container_t **blocks;

    /* call the handler for this element */
    int result = HANDLER_RESULT(cif_start, (cif, context), CIF_WALK_CONTINUE);

    switch (result) {
        case CIF_WALK_CONTINUE:
            /* traverse this element's children (its data blocks) */
            result = cif_get_all_blocks(cif, &blocks);
            if (result == CIF_OK) {
                int handle_blocks = CIF_TRUE;
                cif_container_t **current_block;

                for (current_block = blocks; *current_block; current_block += 1) {
                    if (handle_blocks) {
                        result = walk_block(*current_block, handler, context);

                        switch (walk_block(*current_block, handler, context)) {
                            case CIF_WALK_SKIP_SIBLINGS:
                            case CIF_WALK_END:
                                result = CIF_OK;
                            default:
                                /* Don't break out of the loop, because it cleans up the block handles as it goes */
                                handle_blocks = CIF_FALSE;
                                break;
                            case CIF_WALK_CONTINUE:
                            case CIF_WALK_SKIP_CHILDREN:
                                break;
                        }
                    }
                    cif_block_free(*current_block);
                }
                free(blocks);

                /* call the end handler if and only if we reached the end of the block list normally */
                if (handle_blocks) {
                    result = HANDLER_RESULT(cif_end, (cif, context), CIF_WALK_CONTINUE);
                    /* translate valid walk directions to CIF_OK for return from this function */
                    switch (result) {
                        case CIF_WALK_CONTINUE:
                        case CIF_WALK_SKIP_CHILDREN:
                        case CIF_WALK_SKIP_SIBLINGS:
                        case CIF_WALK_END:
                            return CIF_OK;
                    }
                }
            }

            break;
        case CIF_WALK_SKIP_CHILDREN:
        case CIF_WALK_SKIP_SIBLINGS:
        case CIF_WALK_END:
            /* valid start handler responses instructing us to return CIF_OK without doing anything further */
            return CIF_OK;
    }

    return result;
}

static int walk_block(cif_container_t *block, cif_walk_handler_t *handler, void *context) {
    /* call the handler for this element */
    int result = HANDLER_RESULT(block_start, (block, context), CIF_WALK_CONTINUE);

    if (result != CIF_WALK_CONTINUE) {
        return result;
    } else {
        /* handle this block's save frames */
        cif_container_t **frames;

        result = cif_block_get_all_frames(block, &frames);
        if (result != CIF_OK) {
            return result;
        } else {
            cif_container_t **current_frame;
            int handle_frames = CIF_TRUE;
            int handle_loops = CIF_TRUE;

            for (current_frame = frames; *current_frame; current_frame += 1) {
                if (handle_frames) {
                    /* 'result' can only change within this loop while 'handle_frames' is true */
                    result = walk_frame(*current_frame, handler, context);
                    switch (result) {
                        case CIF_WALK_CONTINUE:
                        case CIF_WALK_SKIP_CHILDREN:
                            break;
                        case CIF_WALK_END:
                        default:
                            /* do not traverse this block's loops */
                            handle_loops = CIF_FALSE;
                            /* fall through */
                        case CIF_WALK_SKIP_SIBLINGS:
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
        result = walk_loops(block, handler, context);
        switch (result) {
            case CIF_WALK_CONTINUE:
            case CIF_WALK_SKIP_CHILDREN:
                return HANDLER_RESULT(block_end, (block, context), CIF_WALK_CONTINUE);
            case CIF_WALK_SKIP_SIBLINGS:
                return CIF_WALK_CONTINUE;
            default:
                return result;
        }
    }
}

static int walk_frame(cif_container_t *frame, cif_walk_handler_t *handler, void *context) {
    /* call the handler for this element */
    int result = HANDLER_RESULT(frame_start, (frame, context), CIF_WALK_CONTINUE);

    if (result != CIF_WALK_CONTINUE) {
        return result;
    } else {
        /* handle this frame's loops */
        result = walk_loops(frame, handler, context);
        switch (result) {
            case CIF_WALK_CONTINUE:
            case CIF_WALK_SKIP_CHILDREN:
                return HANDLER_RESULT(frame_end, (frame, context), CIF_WALK_CONTINUE);
            case CIF_WALK_SKIP_SIBLINGS:
                return CIF_WALK_CONTINUE;
            default:
                return result;
        }
    }
}

static int walk_loops(cif_container_t *container, cif_walk_handler_t *handler, void *context) {
    int result;
    cif_loop_t **loops;

    result = cif_container_get_all_loops(container, &loops);
    if (result == CIF_OK) {
        cif_loop_t **current_loop;
        int handle_loops = CIF_TRUE;

        for (current_loop = loops; *current_loop != NULL; current_loop += 1) {
            if (handle_loops) {
                result = walk_loop(*current_loop, handler, context);
                switch (result) {
                    case CIF_WALK_SKIP_CHILDREN:
                    case CIF_WALK_CONTINUE:
                        break;
                    default:
                        /* don't traverse any more loops; just release resources */
                        handle_loops = CIF_FALSE;
                        break;
                }
            }
            free(*current_loop);
        }

        free(loops);
    }

    return result;
}

static int walk_loop(cif_loop_t *loop, cif_walk_handler_t *handler, void *context) {
    int result = HANDLER_RESULT(loop_start, (loop, context), CIF_WALK_CONTINUE);

    if (result != CIF_WALK_CONTINUE) {
        return result;
    } else {
        cif_pktitr_t *iterator = NULL;

        result = cif_loop_get_packets(loop, &iterator);
        if (result != CIF_OK) {
            return result;
        } else {
            cif_packet_t *packet;

            while ((result = cif_pktitr_next_packet(iterator, &packet)) == CIF_OK) {
                int packet_result = walk_packet(packet, handler, context);

                switch (packet_result) {
                    case CIF_WALK_CONTINUE:
                    case CIF_WALK_SKIP_CHILDREN:
                        break;
                    case CIF_WALK_SKIP_SIBLINGS:
                        return CIF_WALK_CONTINUE;
                    default:  /* CIF_WALK_END or error code */
                        return packet_result;
                }
            }

            if (result != CIF_FINISHED) {
                return result;
            }

            return HANDLER_RESULT(loop_end, (loop, context), CIF_WALK_CONTINUE);
        }
    }
}

static int walk_packet(cif_packet_t *packet, cif_walk_handler_t *handler, void *context) {
    int handler_result = HANDLER_RESULT(packet_start, (packet, context), CIF_WALK_CONTINUE);

    if (handler_result != CIF_WALK_CONTINUE) {
        return handler_result;
    } else {
        struct entry_s *item;

        for (item = packet->map.head; item != NULL; item = (struct entry_s *) item->hh.next) {
            int item_result;

            item_result = walk_item(item->key, &(item->as_value), handler, context);
            switch (item_result) {
                case CIF_WALK_CONTINUE:
                case CIF_WALK_SKIP_CHILDREN:
                    break;
                case CIF_WALK_SKIP_SIBLINGS:
                    return CIF_WALK_CONTINUE;
                default:  /* CIF_WALK_END or error code */
                    return item_result;
            }
        }

        return HANDLER_RESULT(packet_end, (packet, context), CIF_WALK_CONTINUE);
    }
}

static int walk_item(UChar *name, cif_value_t *value, cif_walk_handler_t *handler, void *context) {
    return HANDLER_RESULT(item, (name, value, context), CIF_WALK_CONTINUE);
}


