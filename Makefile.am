#
# Copyright 2014, 2015 John C. Bollinger
#
#
# This file is part of the CIF API.
#
# The CIF API is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The CIF API is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
#

SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

# 'distcheck' should verify that building the examples works.
# It would be nice to test documentation rebuilding, too, but 'distcheck'
# doesn't like it that 'distclean' doesn't then remove the generarted docs.
AM_DISTCHECK_CONFIGURE_FLAGS = --with-examples

doxygen_examples = \
  misc/basic_parsing.c \
  misc/parse_errors.c \
  misc/parser_callbacks.c

EXTRA_DIST = \
  dox-html \
  dox-man \
  Doxyfile.in \
  IP_Notes \
  README \
  README_Windows \
  ReleaseNotes \
  extras/README \
  extras/cif_parser.y \
  extras/cif_scanner.l \
  misc/cif_schema.sql \
  misc/dox.css \
  misc/footer.html \
  misc/main.dox \
  test-data/10.cif \
  test-data/README \
  test-data/bom.cif \
  test-data/bom_ver2.cif \
  test-data/cif_core.dic \
  test-data/cif1_quoting.cif \
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
  
CLEANFILES = gmon.out
DISTCLEANFILES = Doxyfile

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

