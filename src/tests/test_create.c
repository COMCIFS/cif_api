/*
 * test_create.c
 *
 * Tests the CIF API's cif_create() and cif_destroy() functions, as much
 * as can be done in isolation.
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

#include <stdio.h>
#include "../cif.h"
#include "test.h"

int main(void) {
    char test_name[80] = "test_create";
    cif_tp *cif = NULL;
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

