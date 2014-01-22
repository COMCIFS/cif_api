/*
 * assert_value.h
 *
 * Functions for testing assertions about CIF values
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unicode/ustring.h>
#include <assert.h>
#include "../cif.h"
#include "test.h"

static int assert_values_equal(cif_value_t *value1, cif_value_t *value2);
static int assert_lists_equal(cif_value_t *list1, cif_value_t *list2);
static int assert_tables_equal(cif_value_t *table1, cif_value_t *table2);

static int assert_values_equal(cif_value_t *value1, cif_value_t *value2) {
    int rval = 0;
    cif_kind_t kind1 = cif_value_kind(value1);
    cif_kind_t kind2 = cif_value_kind(value2);

    INIT_USTDERR;
    assert(value1 != NULL);
    assert(value2 != NULL);
    if (kind1 == kind2) {
        UChar *text1;
        UChar *text2;

#ifdef DEBUG
        fprintf(stderr, "kinds match (%d)\n", kind1);
#endif
        switch (kind1) {
            case CIF_CHAR_KIND:
            case CIF_NUMB_KIND:
                /* number and character values are both compared with other values of the same kind via their text */
                if (cif_value_get_text(value1, &text1) == CIF_OK) {
                    assert(text1 != NULL);
                    if (cif_value_get_text(value2, &text2) == CIF_OK) {
                        assert(text2 != NULL);
                        rval = (u_strcmp(text1, text2) == 0);
#ifdef DEBUG
                        if (rval == 0) {
                            u_fprintf(ustderr, "Text value '%S' != '%S'\n", text1, text2);
                        } else {
                            u_fprintf(ustderr, "Text values match (%S)\n", text1);
                        }
#endif
                        free(text2);
                    }
                    free(text1);
                }
                break;
            case CIF_LIST_KIND:
                rval = assert_lists_equal(value1, value2);
                break;
            case CIF_TABLE_KIND:
                rval = assert_tables_equal(value1, value2);
                break;
            default:
                /* nothing further to check */
                rval = 1;
                break;
        }
#ifdef DEBUG
    } else {
        fprintf(stderr, "expected kind %d, got %d\n", kind1, kind2);
#endif
    }

    return rval;
}

static int assert_lists_equal(cif_value_t *list1, cif_value_t *list2) {
    size_t count1, count2;
    
    if ((cif_value_get_element_count(list1, &count1) != CIF_OK)
            || (cif_value_get_element_count(list2, &count2) != CIF_OK)
            || (count1 != count2)) {
#ifdef DEBUG
        fprintf(stderr, "List size mismatch\n");
#endif
        return 0;
    } else {
        for (count2 = 0; count2 < count1; count2 += 1) {
            cif_value_t *value1;
            cif_value_t *value2;

            if ((cif_value_get_element_at(list1, count2, &value1) != CIF_OK)
                    || (cif_value_get_element_at(list2, count2, &value2) != CIF_OK)
                    || !assert_values_equal(value1, value2)) {
#ifdef DEBUG
                fprintf(stderr, "List element mismatch\n");
#endif
                return 0;
            }
            /* value1 and value2 should not be freed; they belong to their respective lists */
        }

#ifdef DEBUG
        fprintf(stderr, "List match\n");
#endif
        return 1;
    }
}

static int assert_tables_equal(cif_value_t *table1, cif_value_t *table2) {
    size_t count1, count2;
    const UChar **keys;
    
    if ((cif_value_get_element_count(table1, &count1) != CIF_OK)
            || (cif_value_get_element_count(table2, &count2) != CIF_OK)
            || (count1 != count2)
            || (cif_value_get_keys(table1, &keys) != CIF_OK)) {
#ifdef DEBUG
        fprintf(stderr, "Table size mismatch\n");
#endif
        return 0;
    } else {
        int rval = 1;

        for (count2 = 0; count2 < count1; count2 += 1) {
            cif_value_t *value1;
            cif_value_t *value2;

            assert(keys[count2] != NULL);
            if ((cif_value_get_item_by_key(table1, keys[count2], &value1) != CIF_OK)
                    || (cif_value_get_item_by_key(table2, keys[count2], &value2) != CIF_OK)
                    || !assert_values_equal(value1, value2)) {
                rval = 0;
                break;
            }
            /* value1 and value2 should not be freed; they belong to their respective lists */
        }

        /* clean up the key array (but not the individual keys!) regardless of the comparison result */
        free(keys);

#ifdef DEBUG
        fprintf(stderr, (rval == 0) ? "Table mismatch" : "Table match\n");
#endif
        return rval;
    }
}

