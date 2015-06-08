/*
 * test_get_api_version.c
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

#include <stdlib.h>
#include <stdio.h>
#include "../cif.h"
#include "test.h"

#define VERSION_VAR "API_VERSION"

int main(void) {
    char test_name[80] = "test_get_api_version";
    const char *expected_version = getenv(VERSION_VAR);
    char *reported_version;
    int result;

    TESTHEADER(test_name);
    TEST(expected_version == NULL, 0, test_name, 1);
    TEST(cif_get_api_version(&reported_version), CIF_OK, test_name, 2);
    TEST(reported_version == NULL, 0, test_name, 3);
    TEST(strcmp(expected_version, reported_version), 0, test_name, 4);

    return 0;
}

