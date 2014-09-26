/*
 * assert_cifs.h
 *
 * Functions for testing assertions about whole CIFs and their structural components.
 *
 * Copyright (C) 2014 John C. Bollinger.  All rights reserved.
 */

#ifndef ASSERT_CIFS_H
#define ASSERT_CIFS_H

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <assert.h>
#include "../cif.h"
#include "test.h"
#include "assert_value.h"
#include "uthash.h"

/*
 * A structure suitable for hashing with uthash that carries no data other than the hash metadata.
 */
struct generic_hashable_s {
    UT_hash_handle hh;
};

/*
 * A node in a linked list serving as a stack of context information entries informing
 * the cif handler
 */
struct context_stack_s {
    void *item;
    int children_remaining;
    int elements_remaining;
    struct context_stack_s *next;
};

/*
 * The overall context object by which the comparison handler tracks progress and results
 */
struct comparison_context_s {
    void *other_cif;
    struct context_stack_s *parent;
    int equal;
    int verbose;
};

/*
 * Asserts that the two provided CIFs are equivalent, meaning that they have the same block codes
 * and that their corresponding blocks are equivalent
 */
static int assert_cifs_equal(cif_t *cif1, cif_t *cif2);

/*
 * The following cif_handler callback functions jointly implement an equivalency comparison for whole CIFs.  The
 * equivalence condition tested is stronger than data-model equivalence, as it also tests that equivalent loop packets
 * appear in the same order in their loops, that simplifies the implementation and is satisfactory for the purposes
 * for which this is intended.
 */
static int handle_cif_comparison(cif_t *cif, void *context);
static int finish_cif_comparison(cif_t *cif, void *context);
static int handle_block_comparison(cif_container_t *block, void *context);
static int handle_frame_comparison(cif_container_t *frame, void *context);
static int finish_container_comparison(cif_container_t *container, void *context);
static int handle_container(cif_container_t *container, cif_container_t *other_container,
        struct comparison_context_s *context);
static int handle_loop_comparison(cif_loop_t *loop, void *context);
static int finish_loop_comparison(cif_loop_t *loop, void *context);
static int handle_packet_comparison(cif_packet_t *packet, void *context);
static int finish_packet_comparison(cif_packet_t *packet, void *context);
static int handle_item_comparison(UChar *name, cif_value_t *value, void *context);

static int assert_cifs_equal(cif_t *cif1, cif_t *cif2) {
    cif_handler_t comparison_handler = {
            handle_cif_comparison,       /* cif_start */
            finish_cif_comparison,       /* cif_end */
            handle_block_comparison,     /* block_start */
            finish_container_comparison, /* block_end */
            handle_frame_comparison,     /* frame_start */
            finish_container_comparison, /* frame_end */
            handle_loop_comparison,      /* loop_start */
            finish_loop_comparison,      /* loop_end */
            handle_packet_comparison,    /* packet_start */
            finish_packet_comparison,    /* packet_end */
            handle_item_comparison       /* item */
    };
    struct comparison_context_s context = { NULL, NULL, 1, 1 };
    struct context_stack_s *stack_p;
    int result;

    context.other_cif = cif2;
    result = cif_walk(cif1, &comparison_handler, &context);

    /* clean up the context stack if necessary */
    for (stack_p = context.parent; stack_p; stack_p = context.parent) {
        context.parent = stack_p->next;
        free(stack_p);
    }

    return ((result == CIF_OK) && context.equal);
}

static int handle_cif_comparison(cif_t *cif UNUSED, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    cif_t *other = (cif_t *) comp_context->other_cif;
    struct context_stack_s *stack_p;
    cif_block_t **blocks;
    int block_count = 0;

    if (cif_get_all_blocks(other, &blocks) == CIF_OK) {
        cif_block_t **next_block_p;

        for (next_block_p = blocks; *next_block_p; next_block_p += 1) {
            block_count += 1;
            cif_block_free(*next_block_p);
        }
        free(blocks);

        stack_p = (struct context_stack_s *) malloc(sizeof(struct context_stack_s));
        if (stack_p != NULL) {
            stack_p->item = other;
            stack_p->children_remaining = block_count;
            stack_p->elements_remaining = 0;
            stack_p->next = NULL;
            comp_context->parent = stack_p;

            return CIF_TRAVERSE_CONTINUE;
        }

        if (comp_context->verbose) {
            fprintf(stderr, "CIFs are unequal because their data block counts disagree.\n");
        }
    } else if (comp_context->verbose) {
        fprintf(stderr, "System error during CIF comparison.\n");
    }

    return CIF_TRAVERSE_END;
}

static int finish_cif_comparison(cif_t *cif UNUSED, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;

    assert(stack_p);
    comp_context->parent = stack_p->next;
    if (stack_p->children_remaining != 0) {
        comp_context->equal = 0;
    }
    free(stack_p);

    return CIF_TRAVERSE_CONTINUE;
}

static int handle_block_comparison(cif_container_t *block, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    cif_t *other_cif;
    UChar *my_code;

    assert(stack_p);
    other_cif = (cif_t *) stack_p->item;
    if(cif_container_get_code(block, &my_code) == CIF_OK) {
        cif_container_t *other_block;
        int result = cif_get_block(other_cif, my_code, &other_block);

        free(my_code);
        if (result == CIF_OK) {
            return handle_container(block, other_block, comp_context);
        }
    } /* else no matching block in the other CIF */

    if (comp_context->verbose) {
        fprintf(stderr, "CIFs are unequal because data block codes don't match.\n");
    }

    comp_context->equal = 0;
    return CIF_TRAVERSE_END;
}

static int handle_frame_comparison(cif_container_t *frame, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    cif_container_t *other_container;
    UChar *my_code;

    assert(stack_p);
    other_container = (cif_container_t *) stack_p->item;
    if(cif_container_get_code(frame, &my_code) == CIF_OK) {
        cif_container_t *other_frame;
        int result = cif_container_get_frame(other_container, my_code, &other_frame);

        free(my_code);
        if (result == CIF_OK) {
            return handle_container(frame, other_frame, comp_context);
        }
    } /* else no matching save frame in the other CIF */

    if (comp_context->verbose) {
        fprintf(stderr, "CIFs are unequal because save frame codes don't match.\n");
    }

    comp_context->equal = 0;
    return CIF_TRAVERSE_END;
}

static int handle_container(cif_container_t *container, cif_container_t *other_container,
        struct comparison_context_s *context) {
    struct context_stack_s *stack_p;
    cif_container_t **frames;

    if (cif_container_get_all_frames(container, &frames) == CIF_OK) {
        cif_container_t **frame_p;
        cif_loop_t **loops;
        int frame_count = 0;

        for (frame_p = frames; *frame_p; frame_p += 1) {
            frame_count += 1;
            cif_container_free(*frame_p);
        }
        free(frames);

        if (cif_container_get_all_loops(container, &loops) == CIF_OK) {
            cif_loop_t **loop_p;
            int loop_count = 0;

            for (loop_p = loops; *loop_p; loop_p += 1) {
                loop_count += 1;
                cif_loop_free(*loop_p);
            }
            free(loops);

            stack_p = (struct context_stack_s *) malloc(sizeof(struct context_stack_s));
            if (stack_p != NULL) {
                stack_p->item = other_container;
                stack_p->children_remaining = frame_count;
                stack_p->elements_remaining = loop_count;
                stack_p->next = context->parent;
                context->parent = stack_p;

                return CIF_TRAVERSE_CONTINUE;
            }
        }
    }

    if (context->verbose) {
        fprintf(stderr, "System error during container comparison.\n");
    }

    context->equal = 0;
    return CIF_TRAVERSE_END;
}

static int finish_container_comparison(cif_container_t *container UNUSED, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    int rval;

    assert(stack_p);
    comp_context->parent = stack_p->next;
    comp_context->parent->children_remaining -= 1;
    if ((stack_p->children_remaining != 0) || (stack_p->elements_remaining != 0)) {
        if (comp_context->verbose) {
            fprintf(stderr, "CIFs container contents aren't fully matched.\n");
        }
        comp_context->equal = 0;
        rval = CIF_TRAVERSE_END;
    } else {
        rval = CIF_TRAVERSE_CONTINUE;
    }
    cif_container_free((cif_container_t *) stack_p->item);
    free(stack_p);

    return rval;
}

static int handle_loop_comparison(cif_loop_t *loop, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    cif_container_t *other_container;
    UChar **my_data_names;
    int is_equal = 0;

    assert(stack_p);
    other_container = (cif_container_t *) stack_p->item;

    /* ignores loop categories */

    if (cif_loop_get_names(loop, &my_data_names) == CIF_OK) {
        UChar **my_name_p = my_data_names;
        cif_loop_t *other_loop;

        /* Retrieve the analogous loop */
        assert(*my_data_names);
        if (cif_container_get_item_loop(other_container, *my_data_names, &other_loop) == CIF_OK) {
            UChar **other_data_names;

            if (cif_loop_get_names(other_loop, &other_data_names) == CIF_OK) {
                /* match datanames without normalization */
                UChar **other_name_p = other_data_names;
                struct generic_hashable_s *other_datanames_head = NULL;
                struct generic_hashable_s *other_dataname_entry;
                struct generic_hashable_s *temp_dataname_entry;
                int dataname_count = 0;

                for (; *other_name_p; other_name_p += 1) {
                    other_dataname_entry = (struct generic_hashable_s *) malloc(sizeof(struct generic_hashable_s));

                    if (other_dataname_entry) {
                        HASH_ADD_KEYPTR(hh, other_datanames_head, *other_name_p,
                                (u_strlen(*other_name_p) * sizeof(UChar)), other_dataname_entry);
                        dataname_count += 1;
                    } else {
                        break;
                    }
                }

                if (! *other_name_p) { /* successfully hashed all the datanames */
                    /* match data names against the hash */
                    for (; *my_name_p; my_name_p += 1) {
                        HASH_FIND(hh, other_datanames_head, *my_name_p,
                                (u_strlen(*my_name_p) * sizeof(UChar)), other_dataname_entry);
                        free(*my_name_p);
                        if (other_dataname_entry) {
                            /* a match; clean it up */
                            HASH_DELETE(hh, other_datanames_head, other_dataname_entry);
                            free(other_dataname_entry);
                        } else {
                            /* no match */
                            break;
                        }
                    }

                    if ((*my_name_p == NULL) && (HASH_COUNT(other_datanames_head) == 0)) {
                        /* all names matched with none left over */
                        cif_pktitr_t *other_packets;

                        if (cif_loop_get_packets(other_loop, &other_packets) == CIF_OK) {
                            int ignored;

                            /* set up appropriate grandparent and parent contexts on the context stack */
                            stack_p = (struct context_stack_s *) malloc(sizeof(struct context_stack_s));
                            if (stack_p != NULL) {
                                stack_p->item = other_loop;
                                stack_p->children_remaining = 0;
                                stack_p->elements_remaining = 0;
                                stack_p->next = comp_context->parent;
                                comp_context->parent = stack_p;

                                stack_p = (struct context_stack_s *) malloc(sizeof(struct context_stack_s));
                                if (stack_p != NULL) {
                                    stack_p->item = other_packets;
                                    stack_p->children_remaining = 1;  /* flags whether iteration has finished */
                                    stack_p->elements_remaining = dataname_count;
                                    stack_p->next = comp_context->parent;
                                    comp_context->parent = stack_p;

                                    /* raise a flag to signal a completely successful match */
                                    is_equal = 1;
                                } else {
                                    if (comp_context->verbose) {
                                        fprintf(stderr, "System error while comparing CIFs.\n");
                                    }
                                    ignored = cif_pktitr_abort(other_packets); /* ignore any failure */
                                }
                            } else {
                                if (comp_context->verbose) {
                                    fprintf(stderr, "System error while comparing CIFs.\n");
                                }
                                ignored = cif_pktitr_abort(other_packets); /* ignore any failure */
                            }
                        }
                    } else if (comp_context->verbose) {
                        fprintf(stderr, "CIFs are unequal because loop headers are mismatched.\n");
                    }

                } else if (comp_context->verbose) {
                    fprintf(stderr, "System error while comparing CIFs.\n");
                }

                /* clean up any entries remaining in the dataname hash */
                HASH_ITER(hh, other_datanames_head, other_dataname_entry, temp_dataname_entry) {
                    HASH_DELETE(hh, other_datanames_head, other_dataname_entry);
                    free(other_dataname_entry);
                }

                /* clean up the list of other datanames */
                for (other_name_p = other_data_names; *other_name_p; other_name_p += 1) {
                    free(*other_name_p);
                }
                free(other_data_names);
            } else if (comp_context->verbose) {
                fprintf(stderr, "System error while comparing CIFs.\n");
            }

            /* mustn't free other_loop while an iterator depending on it is open */
        } else if (comp_context->verbose) {
            fprintf(stderr, "CIFs are unequal because a data name cannot be matched.\n");
        }

        /* clean up this loop's data name list (as much as is left of it) */
        while (*my_name_p) {
            free(*(my_name_p++));
        }
        free(my_data_names);
    }

    comp_context->equal = (comp_context->equal && is_equal);
    return (is_equal ? CIF_TRAVERSE_CONTINUE : CIF_TRAVERSE_END);
}

static int finish_loop_comparison(cif_loop_t *loop UNUSED, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    cif_pktitr_t *other_packets;
    cif_loop_t *other_loop;
    int ignored;
    int rval;

    assert(stack_p);
    comp_context->parent = stack_p->next;
    other_packets = (cif_pktitr_t *) stack_p->item;
    if ((stack_p->children_remaining == 0) || (cif_pktitr_next_packet(other_packets, NULL) != CIF_FINISHED)) {
        /* differing numbers of loop packets */
        if (comp_context->verbose) {
            fprintf(stderr, "CIFs are unequal because corresponding loops have different packet counts.\n");
        }
        comp_context->equal = 0;
        rval = CIF_TRAVERSE_END;
    } else {
        rval = CIF_TRAVERSE_CONTINUE;
    }
    ignored = cif_pktitr_abort(other_packets);  /* ignore any error */
    free(stack_p);

    stack_p = comp_context->parent;
    assert(stack_p);
    comp_context->parent = stack_p->next;
    comp_context->parent->elements_remaining -= 1;
    other_loop = (cif_loop_t *) stack_p->item;
    cif_loop_free(other_loop);
    free(stack_p);

    return rval;
}

static int handle_packet_comparison(cif_packet_t *packet UNUSED, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    cif_pktitr_t *other_packets;
    cif_packet_t *other_packet = NULL;

    assert(stack_p);
    other_packets = (cif_pktitr_t *) stack_p->item;
    switch (cif_pktitr_next_packet(other_packets, &other_packet)) {
        case CIF_OK:
            stack_p = (struct context_stack_s *) malloc(sizeof(struct context_stack_s));
            if (stack_p != NULL) {
                stack_p->item = other_packet;
                stack_p->children_remaining = 0;
                stack_p->elements_remaining = comp_context->parent->elements_remaining;
                stack_p->next = comp_context->parent;
                comp_context->parent = stack_p;

                return CIF_TRAVERSE_CONTINUE;
            }
            break;
        case CIF_FINISHED:
            if (comp_context->verbose) {
                fprintf(stderr, "CIFs are unequal because corresponding loops have different packet counts.\n");
            }
            stack_p->children_remaining = 0;
            break;
        default:
            if (comp_context->verbose) {
                fprintf(stderr, "System error while comparing loop packets\n");
            }
            break;
    }

    comp_context->equal = 0;
    return CIF_TRAVERSE_END;
}

static int finish_packet_comparison(cif_packet_t *packet UNUSED, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    int rval;

    assert(stack_p);
    comp_context->parent = stack_p->next;
    if (stack_p->elements_remaining != 0) {
        if (comp_context->verbose) {
            fprintf(stderr, "CIFs are unequal because corresponding packets have differing item counts.\n");
        }
        comp_context->equal = 0;
        rval = CIF_TRAVERSE_END;
    } else {
        rval = CIF_TRAVERSE_CONTINUE;
    }
    cif_packet_free((cif_packet_t *) stack_p->item);
    free(stack_p);

    return rval;
}

static int handle_item_comparison(UChar *name, cif_value_t *value, void *context) {
    struct comparison_context_s *comp_context = (struct comparison_context_s *) context;
    struct context_stack_s *stack_p = comp_context->parent;
    cif_packet_t *other_packet;
    cif_value_t *other_value = NULL;

    assert(stack_p);
    other_packet = (cif_packet_t *) stack_p->item;
    if (cif_packet_get_item(other_packet, name, &other_value) == CIF_OK) {
        stack_p->elements_remaining -= 1;
        if (assert_values_equal(value, other_value)) {
            return CIF_TRAVERSE_CONTINUE;
        } else if (comp_context->verbose) {
            fprintf(stderr, "CIFs are unequal because corresponding values differ.\n");
        }
    }

    comp_context->equal = 0;
    return CIF_TRAVERSE_END;
}

#endif
