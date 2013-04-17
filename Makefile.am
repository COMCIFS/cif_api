#
# Copyright (C) 2013 John C. Bollinger
#
# All rights reserved.
#

SUBDIRS = src

EXTRA_DIST = \
  README \
  misc/cif_schema.sql \
  misc/footer.html \
  misc/main.dox \
  uthash/LICENSE \
  uthash/utarray.h \
  uthash/uthash.h \
  uthash/utlist.h \
  uthash/utstring.h

ACLOCAL_AMFLAGS = -I m4

if make_docs
doxygen_html_targets = dox-html
doxygen_man_targets = dox-man
else
doxygen_html_targets =
doxygen_man_targets =
endif

pkgdocdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

html-local: $(doxygen_html_targets)
	:

install-html-local: html-local
	test -d dox-html && { $(MKDIR_P) $(DESTDIR)$(pkgdocdir); cp -pR dox-html $(DESTDIR)$(pkgdocdir)/html; }

uninstall-local:
	$(RM) -rf $(DESTDIR)$(pkgdocdir)

clean-local:
	$(RM) -rf dox-html
	$(RM) -rf dox-man

distclean-local:
	$(RM) -rf autom4te.cache

dox-man: dox-html
	test -d $@ && $(RM) -rf $@
	$(DOXYGEN) Doxyfile

dox-html: Doxyfile src/cif.h misc/main.dox misc/footer.html
	$(RM) -rf dox-html
	$(RM) -rf dox-man
	$(DOXYGEN) Doxyfile

