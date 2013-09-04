/*
 * cif_walker.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 */

/** 
 * @file  cif_walker.h
 * @brief The CIF API public header file, declaring all data structures, functions, and macros intended for
 *        library users to manipulate.
 *
 * @author John C. Bollinger
 * @date   2012-2013
 */

#ifndef CIF_WALKER_H
#define CIF_WALKER_H

#include <cif.h>

/* Codes other than CIF_WALK_CONTINUE are negative to avoid colliding with error codes */
#define CIF_WALK_CONTINUE 0
#define CIF_WALK_SKIP_CHILDREN -1
#define CIF_WALK_SKIP_SIBLINGS -2
#define CIF_WALK_END -3

/**
 * @brief a set of functions definining a handler interface for directing and taking appropriate action in response
 *     to a traversal of a CIF.
 *
 * The walker will present each structural element of the CIF being traversed to the appropriate handler function,
 * along with the context object, if any, provided to the walker by its caller.  The handler may perform any action
 * it considers suitable, and is expected to return a code influencing the walker's direction, one of:
 * @li @c CIF_WALK_CONTINUE for the walker to continue on its default path (in most cases to descend to the current element's first child, or
 * @li @c CIF_WALK_SKIP_CHILDREN for the walker to skip traversal of the current element's children, if any, or
 * @li @c CIF_WALK_SKIP_SIBLINGS for the walker to skip the current element's children and all its siblings that have not yet been traversed, or
 * @li @c CIF_WALK_END for the walker to stop without traversing any more elements
 * A handler function may, alternatively, return a CIF API error code, which has the effect of CIF_WALK_END plus
 * causes the walker to forward the given code to its caller via its return value.
 */
typedef struct {
    /** @brief a handler function for the beginning of a top-level CIF object */
    int (*handle_cif_start)(cif_t *cif, void *context);
    /** @brief a handler function for the end of a top-level CIF object */
    int (*handle_cif_end)(cif_t *cif, void *context);
    /** @brief a handler function for the beginning of a data block */
    int (*handle_block_start)(cif_container_t *block, void *context);
    /** @brief a handler function for the end of a data block */
    int (*handle_block_end)(cif_container_t *block, void *context);
    /** @brief a handler function for the beginning of a save frame */
    int (*handle_frame_start)(cif_container_t *frame, void *context);
    /** @brief a handler function for the end of a save frame */
    int (*handle_frame_end)(cif_container_t *frame, void *context);
    /** @brief a handler function for the beginning of a loop */
    int (*handle_loop_start)(cif_loop_t *loop, void *context);
    /** @brief a handler function for the end of a loop */
    int (*handle_loop_end)(cif_loop_t *loop, void *context);
    /** @brief a handler function for the beginning of a loop packet */
    int (*handle_packet_start)(cif_packet_t *packet, void *context);
    /** @brief a handler function for the end of a loop packet */
    int (*handle_packet_end)(cif_packet_t *packet, void *context);
    /** @brief a handler function for data items (there are not separate beginning and end callbacks) */
    int (*handle_item)(UChar *name, cif_value_t *value, void *context);
} cif_walk_handler_t;

/**
 * @brief Performs a depth-first, natural-order traversal of a CIF, calling back to handler
 *        routines provided by the caller for each structural element
 *
 * Traverses the tree structure of a CIF, invoking caller-specified handler functions for each structural element
 * along the route.  Order is block -> (frame ->) loop -> packet.  Frames are traversed before loops belonging to the
 * same data block.  Handler callbacks can influence the walker's path via their return values, instructing it to
 * continue as normal (@c CIF_WALK_CONTINUE), to avoid traversing the children of the current element
 * (@c CIF_WALK_SKIP_CHILDREN), to avoid traversing subsequent siblings of the current element
 * (@c CIF_WALK_SKIP_SIBLINGS), or to terminate the walk altogether (@c CIF_WALK_END).  For the purposes of this
 * function, loops are not considered "siblings" of save frames belonging to the same data block.
 *
 * @param[in] cif the CIF to traverse
 * @param[in] handler a structure containing the callback functions handling each type of CIF structural element
 * @param[in] a context object to pass to each callback function; opaque to the walker itself
 *
 * @return Returns @c CIF_OK on success, or an error code (typically @c CIF_ERROR) on failure
 */
int cif_walk(cif_t *cif, cif_walk_handler_t *handler, void *context);

#endif

