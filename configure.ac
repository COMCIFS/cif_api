dnl
dnl Copyright (C) 2013 John C. Bollinger
dnl
dnl All rights reserved.
dnl

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([CIF API], [0.1.0], [John.Bollinger@StJude.org])
AC_CONFIG_SRCDIR([src/cif.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
#AM_INIT_AUTOMAKE([parallel-tests])
LT_INIT([disable-static])
AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE([enable])

AC_ARG_ENABLE([extra-warnings],
  [AS_HELP_STRING([--enable-extra-warnings],
    [turn on extra compiler diagnostics intended to help developers identify questionable code.  Not all warnings emitted as a result indicate genuine problems. [default=no]])],
  [AS_CASE([$[]{enable_extra_warnings}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_extra_warnings}' for --enable-extra-warnings])]
    )],
  [enable_extra_warnings=no])

AC_ARG_ENABLE([docs],
  [AS_HELP_STRING([--enable-docs],
    [Generate and install documentation.  Requires doxygen.  @<:@default=yes if doxygen is found, otherwise no@:>@])],
  [AS_CASE([$[]{enable_docs}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_docs}' for --enable-docs])]
    )],
  [enable_docs=''])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
    [cause the library to emit debug messages at runtime.  Desirable only for developers. [default=no]])],
  [AS_CASE([$[]{enable_debug}],
    [yes], [AC_DEFINE([DEBUG], [1], [Define to 1 to enable debug messages])],
    [no],  [],
           [AC_MSG_ERROR([Unrecognized value '$[]{enable_debug}' for --enable-debug])]
    )],
  [enable_debug=no])

# We need a program that will count lines and report the result to stdout:
AC_ARG_WITH([line-counter],
  [AS_HELP_STRING([--with-line-counter=<line count command>],
    [specifies a command that counts lines read from its standard input and writes the count to its standard output.  If not specified, configure will attempt to guess.])], [
    AS_CASE([$[]withval],
      [yes|no], [AC_MSG_ERROR([--with-line-counter must specify a line-counting command])],
      [AS_IF([test "x$[]withval" = x], [AC_MSG_ERROR([the line-counter command must not be empty])])]
    )
    COUNT_LINES="$[]withval"
  ], [
    AC_PATH_PROGS([COUNT_LINES], [wc], [none])
    AS_CASE([$[]{COUNT_LINES}],
      [*/wc], [COUNT_LINES="$[]{COUNT_LINES} --lines"],
      [AC_MSG_ERROR([I don't know how to count lines.  Try telling me via --with-line-counter.])])
  ])

AC_PROG_CC
AC_PROG_GREP
AC_PROG_SED
AC_PROG_MKDIR_P

AS_IF([test "x$[]{enable_docs}" != xno], [
  AC_PATH_PROG([DOXYGEN], [doxygen], [])
  AS_IF([test "x$[]{DOXYGEN}" = x && test "x$[]{enable_docs}" = xyes], [
    AC_MSG_ERROR([Documentation generation is enabled, but doxygen is not found.])
  ])
], [DOXYGEN=''])

AC_SUBST([DOXYGEN])

AM_CONDITIONAL([make_docs], [test "x$[]{DOXYGEN}" != x])

# test the line counter
AC_MSG_CHECKING([whether '$[]{COUNT_LINES}' works])
line_count=`$[]{COUNT_LINES} 2>/dev/null <<EOF || echo failed
line 1
line 2
line 3
EOF
`
AS_IF([test "x$[]{line_count}" = x3],
  [AC_MSG_RESULT([yes])],
  [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([command '$[]{COUNT_LINES}' does not count lines correctly.  Try specifying a different command via --with-line-counter])
  ])
AC_SUBST([COUNT_LINES])

# Output CFLAGS that define the C language standards that must be supported,
# if such flags are known.  Although not harmful for production builds, this
# is intended mainly for development, to ensure that project language
# requirements are satisfied.  Thus it's no big deal for a production build
# if we don't know what flags to use.
#
# Note that this is typically different than what we would achieve via macro
# AC_PROG_CC_C89 -- that ensures an environment that supports all C89-compliant
# code, but prefers to allow extensions as well.  What we want here is something
# that will at least warn us about code that a C89-compliant compiler might not
# understand or is permitted to reject.
AS_CASE([$[]{CC}],
  [gcc*], [C_STD_FLAGS="-std=c89 -pedantic"],
  [C_STD_FLAGS=""])
AC_SUBST([C_STD_FLAGS])

# Need to use the C_STD_FLAGS in our tests so that they see the same environment
# that the real build will see, but we shouldn't make permanent changes to the
# CPPFLAGS variable because it belongs to the user.
CPPFLAGS_save="${CPPFLAGS}"
CPPFLAGS="${C_STD_FLAGS} $CPPFLAGS"

# Output C[PP]FLAGS that cause the compiler to emit warnings about all conditions
# we care to detect, if such flags are known.  Although not harmful for
# production builds, this is intended mainly for development, to help detect
# bugs and reduce possible portability issues and other strangeness.  Thus it's
# no big deal for a production build if we don't know what flags to use.
AS_CASE([$[]{CC}],
  [gcc*], [
    WARNING_FLAGS="-Wall -Wextra -Wformat=2 -Winit-self -Wno-switch -Wno-switch-default -Wstrict-aliasing=2 -Wshadow -Wpointer-arith -Wc++-compat -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls -Wnested-externs"
    AS_IF([test $[]{enable_extra_warnings} = 'yes'],
      [WARNING_FLAGS="${WARNING_FLAGS} -Wcast-qual -Wbad-function-cast"])
  ],
  [WARNING_FLAGS=""])
AC_SUBST([WARNING_FLAGS])

# Libraries

AC_SEARCH_LIBS([sqlite3_open_v2], [sqlite3], [], [AC_MSG_FAILURE([SQLite3 not found or not recent enough])])
# TODO: check SQLite version >= 3.6.19 (or otherwise test that it supports and enforces foreign key constraints) */

AS_IF([which pkg-config], [ICU_LIBS=`pkg-config --libs icu` || AC_MSG_FAILURE([No pkg-config information for icu])],
                          [AC_MSG_FAILURE([pkg-config not found])])
AS_IF([test "x$[]{enable_debug}" = xyes], [
    ICU_LIBS="-licuio $[]{ICU_LIBS}"
  ])
AC_SUBST([ICU_LIBS])

# Specific functions

AC_CHECK_FUNCS([strdup])

# We need to determine whether a declaration of strdup() is available, which
# might not be the case in some C89-compliant environments.  This is a separate
# question from that of whether the function itself is available; build options
# may suppress the declaration even though the function is in fact available.
# (For example, options "-std=c89 -pedantic" have that effect on gcc 4.)

AC_CHECK_DECL([strdup], [AC_DEFINE([HAVE_DECL_STRDUP], [1], [Define to 1 if a declaration of strdup() is visible in string.h])])

# Headers

AC_CHECK_HEADER([sqlite3.h], [], [AC_MSG_FAILURE([Required header sqlite3.h was not found])])
AC_CHECK_HEADER([unicode/ustring.h], [], [AC_MSG_FAILURE([Required header unicode/ustring.h was not found])])

# These output variables are defined for documentary purposes.  The makefiles add
# them to the CPP flags, but the current configure script always leaves them empty:
AC_SUBST([SQLITE_CPPFLAGS])
AC_SUBST([ICU_CPPFLAGS])

# Reset the CPPFLAGS.  The changes we made must not be propagated out into
# the build environment (at least, not directly).
CPPFLAGS="${CPPFLAGS_save}"

# (Over)write the Makefiles

AC_CONFIG_FILES([
  Makefile
  Doxyfile
  src/Makefile
])
AC_OUTPUT

