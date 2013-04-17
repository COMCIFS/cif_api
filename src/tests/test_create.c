/*
 * test_create.c
 *
 * Tests the CIF API's cif_create() and cif_destroy() functions, as much
 * as can be done in isolation.
 *
 * Copyright (C) 2013 John C. Bollinger.  All rights reserved.
 */

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(int argc, char *argv[]) {
    char test_name[80] = "test_create";
    cif_t *cif = NULL;
    int result;

    TESTHEADER(test_name);
    fprintf(stdout, "%s: Creating a managed CIF...\n", test_name);
    result = cif_create(&cif);
    if (result != CIF_OK) {
        fprintf(stdout, "%s: ... failed with code %d.\n", test_name, result);
        return 1;
    } else if (cif == NULL) {
        fprintf(stdout, "%s: ... did not set the CIF pointer.\n", test_name);
        return 2;
    }

    fprintf(stdout, "%s: Destroying the managed CIF...\n", test_name);
    result = cif_destroy(cif);
    if (result != CIF_OK) {
        fprintf(stdout, "%s: ... failed with code %d.\n", test_name, result);
        return 1;
    }

    return 0;
}

