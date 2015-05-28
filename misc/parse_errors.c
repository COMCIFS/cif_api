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
int record_error(int error_code, size_t line, size_t column, const UChar *text, size_t length, void *data) {
    *((int *) data) += 1;
    return CIF_OK;
}

void count_errors(FILE *in) {
    cif_tp *cif = NULL;
    int num_errors = 0;
    struct cif_parse_opts_s *opts = NULL;

    cif_parse_options_create(&opts);
    opts->error_callback = record_error;
    opts->user_data = &num_errors;
    cif_parse(in, opts, &cif);
    free(opts);
    /*
     * the parsed results are available via 'cif'
     * the number of errors is available in 'num_errors'
     */
    /* ...  */
    cif_destroy(cif);
}
