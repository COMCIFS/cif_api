/**
 * basic_parsing.c
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
int assign_loop_category(cif_loop_tp *loop, void *context) {
    UChar **names;
    UChar **next;
    UChar *dot_location;
    const UChar unicode_dot = 0x2E;

    /* We can rely on at least one data name */
    cif_loop_get_names(loop, &names);
    /*
     * Assumes the name contains a decimal point (Unicode code point U+002E), and
     * takes the category as everything preceding it.  Ignores case sensitivity considerations.
     */
    dot_location = u_strchr(names[0] + 1, unicode_dot);
    *dot_location = 0;
    cif_loop_set_category(loop, names[0]);
    /* Clean up */
    for (next = names; *next != NULL; next += 1) {
        free(*next);
    }
    free(names);
}

void parse_with_categories(FILE *in) {
    cif_tp *cif = NULL;
    struct cif_parse_opts_s *opts = NULL;
    cif_handler_tp handler = { NULL, NULL, NULL, NULL, NULL, NULL, assign_loop_category, NULL, NULL, NULL, NULL };

    cif_parse_options_create(&opts);
    opts->handler = &handler;
    cif_parse(in, opts, &cif);
    free(opts);
    /* the parsed results are available via 'cif' */
    /* ... */
    cif_destroy(cif);
}
