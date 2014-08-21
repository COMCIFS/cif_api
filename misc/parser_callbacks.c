int assign_loop_category(cif_loop_t *loop, void *context) {
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
    cif_t *cif = NULL;
    struct cif_parse_opts_s *opts = NULL;
    cif_handler_t handler = { NULL, NULL, NULL, NULL, NULL, NULL, assign_loop_category, NULL, NULL, NULL, NULL };

    cif_parse_options_create(&opts);
    opts->handler = &handler;
    cif_parse(in, opts, &cif);
    free(opts);
    /* the parsed results are available via 'cif' */
    /* ... */
    cif_destroy(cif);
}
