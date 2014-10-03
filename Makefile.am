#
# Copyright (C) 2013, 2014 John C. Bollinger
#
# All rights reserved.
#

SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

# 'distcheck' should verify that building the examples and rebuilding the documentation both work
AM_DISTCHECK_CONFIGURE_FLAGS = --enable-doc-rebuild --with-examples

doxygen_examples = \
  misc/basic_parsing.c \
  misc/parse_errors.c \
  misc/parser_callbacks.c

EXTRA_DIST = \
  dox-html \
  dox-man \
  Doxyfile.in \
  README \
  extras/README \
  extras/cif2.ebnf \
  extras/cif_parser.y \
  extras/cif_scanner.l \
  misc/cif_schema.sql \
  misc/dox.css \
  misc/footer.html \
  misc/main.dox \
  test-data/bom.cif \
  test-data/bom_ver2.cif \
  test-data/cif_core.dic \
  test-data/comment_only.cif \
  test-data/complex_data.cif \
  test-data/empty.cif \
  test-data/list_data.cif \
  test-data/nested.cif \
  test-data/simple_containers.cif \
  test-data/simple_data.cif \
  test-data/simple_loops.cif \
  test-data/table_data.cif \
  test-data/text_fields.cif \
  test-data/triple.cif \
  test-data/unicode.cif \
  test-data/ver1.cif \
  test-data/ver2.cif \
  uthash/LICENSE \
  uthash/utarray.h \
  uthash/uthash.h \
  uthash/utlist.h \
  uthash/utstring.h \
  $(doxygen_examples)

if make_docs
doxygen_html_targets = dox-html
doxygen_man_targets = dox-man
dox_deps = Doxyfile src/cif.h src/parser.c misc/main.dox misc/footer.html misc/dox.css $(doxygen_examples)
else
doxygen_html_targets =
doxygen_man_targets =
dox_deps = 
endif

if install_docs
# The chmod in the below command must ensure the target files writable
# in order to work around weirdness in the behavior of the "distcheck" target
# when not configured to rebuild docs.
html_install = test -d dox-html && html_dir=dox-html || html_dir=$(srcdir)/dox-html; \
  $(MKDIR_P) $(DESTDIR)$(pkgdocdir); \
  cp -pR $${html_dir} $(DESTDIR)$(pkgdocdir)/html; \
  chmod -R u+w,go+rX $(DESTDIR)$(pkgdocdir)/html
else
html_install = :
endif

pkgdocdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

html-local: $(doxygen_html_targets)
	:

## This rule should not be necessary, but Automake seems otherwise to ignore the 
## install-html-local rule, perhaps because there are no targets with an HTML
## primary.
install-data-local: install-html-local

install-html-local:
	$(html_install)

uninstall-local:
	$(RM) -rf $(DESTDIR)$(pkgdocdir)

distclean-local:
	$(RM) -rf autom4te.cache

maintainer-clean-local:
	$(RM) -rf dox-html
	$(RM) -rf dox-man

dox-man: dox-html

dox-html: $(dox_deps)
	$(RM) -rf dox-html
	$(RM) -rf dox-man
	$(DOXYGEN) Doxyfile

Doxyfile: Doxyfile.in Makefile
	$(SED) -e 's,[@]PACKAGE@,$(PACKAGE),g' \
	       -e 's,[@]PACKAGE_VERSION@,$(PACKAGE_VERSION),g' \
	       -e 's,[@]abs_builddir@,$(abs_builddir),g' \
	       -e 's,[@]abs_srcdir@,$(abs_srcdir),g' \
	       < $< > $@

