void traditional(FILE *in) {
    cif_tp *cif = NULL;

    cif_parse(in, NULL, &cif);
    /* results are available via 'cif' if anything was successfully parsed */
    cif_destroy(cif);  /* safe even if 'cif' is still NULL */
}
