#
# Copyright (C) 2013, 2014 John C. Bollinger
#
# All rights reserved.
#

SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
  dox-html \
  dox-man \
  README \
  extras/README \
  extras/cif_parser.y \
  extras/cif_scanner.l \
  misc/cif_schema.sql \
  misc/dox.css \
  misc/footer.html \
  misc/main.dox \
  misc/third_party_lib32.supp \
  misc/third_party_lib64.supp \
  test-data/bom.cif \
  test-data/bom_ver2.cif \
  test-data/cif_expanded_oct09.dic \
  test-data/complex_data.cif \
  test-data/empty.cif \
  test-data/list_data.cif \
  test-data/simple_containers.cif \
  test-data/simple_data.cif \
  test-data/simple_loops.cif \
  test-data/table_data.cif \
  test-data/unicode.cif \
  test-data/ver1.cif \
  test-data/ver2.cif \
  uthash/LICENSE \
  uthash/utarray.h \
  uthash/uthash.h \
  uthash/utlist.h \
  uthash/utstring.h

if make_docs
doxygen_html_targets = dox-html
doxygen_man_targets = dox-man
dox_deps = Doxyfile src/cif.h src/parser.c misc/main.dox misc/footer.html misc/dox.css
else
doxygen_html_targets =
doxygen_man_targets =
dox_deps = 
endif

if install_docs
# The chmod in the below command is needed to work around weirdness in the behavior of the
# "distcheck" target
html_install = test -d dox-html && html_dir=dox-html || html_dir=$(srcdir)/dox-html; \
  $(MKDIR_P) $(DESTDIR)$(pkgdocdir); \
  cp -pR $${html_dir} $(DESTDIR)$(pkgdocdir)/html; \
  chmod -R u+w $(DESTDIR)$(pkgdocdir)/html
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
	test -d $@ && $(RM) -rf $@
	$(DOXYGEN) Doxyfile

dox-html: $(dox_deps)
	$(RM) -rf dox-html
	$(RM) -rf dox-man
	$(DOXYGEN) Doxyfile

