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
