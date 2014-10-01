#
# Copyright (C) 2013, 2014 John C. Bollinger
#
# All rights reserved.
#

lib_LTLIBRARIES = libcif.la
include_HEADERS = cif.h

## Library versioning (LibTool style):
##
## Whenever the implementation is modified without changing the outward-facing
## interface or its documented or generally-assumed behavior, increment
## LIB_REVISION by one.
## 
## Whenever the outward-facing interface is modified, whether by addition of a
## feature, removal of a feature, or change to a documented or generally-assumed
## behavior, increment LIB_CURRENT by one and set LIB_REVISION to zero.
##
## Furthermore, when LIB_CURRENT is changed exclusively because of the addition
## of a feature, increment LIB_AGE by one.  Whenever else LIB_CURRENT is changed,
## set LIB_AGE to zero (or to any other integer exceeding neither the previous
## value of LIB_AGE nor the difference between the new value of LIB_CURRENT and
## the greatest prior value of LIB_CURRENT when any of the removed or modified
## features was added or previously modified).  Basically, LIB_AGE describes
## how far back the library's binary compatibility extends, in terms of
## LIB_CURRENT numbers.
##
## Those rules imply that LIB_CURRENT never decreases, and that the difference
## between LIB_CURRENT and LIB_AGE never decreases.  LIB_REVISION is never
## negative, but its values are not directly tied to those of the other two
## version parameters.
##
## These decisions should be made once per software release, based on changes from
## one release to the next (not, for example, every time the library is built).
LIB_CURRENT = 0
LIB_REVISION = 0
LIB_AGE = 0
LIB_VERSION_FLAGS = -version-info $(LIB_CURRENT):$(LIB_REVISION):$(LIB_AGE)

EXTRA_DIST = notes.txt style.txt

# For valgrind tests, compile at optimization level -O (no higher):
# TODO: find a cleaner way to do this
#CFLAGS += -O

AM_CPPFLAGS = $(C_STD_FLAGS) $(ICU_CPPFLAGS) $(SQLITE_CPPFLAGS) -I $(top_srcdir)/uthash $(WARNING_FLAGS)

BUILT_SOURCES = internal/schema.h

CLEANFILES = $(BUILT_SOURCES)

include tests.am

libcif_la_SOURCES = \
  cif.c \
  ciffile.c \
  container.c \
  loop.c \
  map.c \
  packet.c \
  parser.c \
  pktitr.c \
  utils.c \
  value.c \
  cif.h \
  internal/buffer.h \
  internal/ciftypes.h \
  internal/sql.h \
  internal/utils.h \
  internal/value.h
nodist_libcif_la_SOURCES = $(BUILT_SOURCES)
libcif_la_LIBADD = $(ICU_LIBS)
libcif_la_LDFLAGS = $(LIB_VERSION_FLAGS)

LDADD = libcif.la

# A sed script that processes a slightly-restricted dialect of SQL into a
# comma-delimited list of C string literals, each containing the text of
# one statement:
SED_SQL_MANGLER = \
  s/^  *//; \
  s/  *$$//; \
  $$ s/\([^;]\)$$/\1;/; \
  /^[Bb][Ee][Gg][Ii][Nn][^;]*$$/,/^[Ee][Nn][Dd] *;/ { \
    /^[Ee][Nn][Dd] *;/ b t; \
    b i; \
  }; \
  /;$$/ b t; \
  :i; \
  s/.*/"& "/; H; d; \
  :t; \
  s/^\(.*\);$$/"\1"/; H; s/.*/,/; x

internal/schema.h: $(top_srcdir)/misc/cif_schema.sql Makefile.am
	@$(MKDIR_P) internal
	echo "#ifndef INTERNAL_SCHEMA_H" > $@
	echo "#define INTERNAL_SCHEMA_H" >> $@
	echo "#define DDL_STMT_COUNT (`$(SED) -e '$(SED_SQL_MANGLER)' < $< | grep '^,$$' | $(COUNT_LINES)` + 1)" >> $@
	echo "static const char * const schema_statements[DDL_STMT_COUNT] = {" >> $@
	$(GREP) '[^[:space:]]' $< \
	  | $(GREP) -v '\s*--' \
	  | $(SED) -e '$(SED_SQL_MANGLER)' \
	  >> $@
	echo "};" >> $@
	echo "#endif" >> $@

