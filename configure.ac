dnl
dnl Copyright (C) 2013 John C. Bollinger
dnl
dnl All rights reserved.
dnl

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([CIF API], [0.2.0], [John.Bollinger@StJude.org])
AC_CONFIG_SRCDIR([src/cif.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
dnl AM_INIT_AUTOMAKE([parallel-tests])
LT_INIT([disable-static])
AC_CONFIG_HEADERS([config.h])

# User options

AM_MAINTAINER_MODE([enable])

AC_ARG_ENABLE([extra-warnings],
  [AS_HELP_STRING([--enable-extra-warnings],
    [turn on extra compiler diagnostics intended to help developers identify questionable code.  Not all warnings emitted as a result indicate genuine problems. [default=no]])],
  [AS_CASE([$[]{enable_extra_warnings}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_extra_warnings}' for --enable-extra-warnings])]
    )],
  [enable_extra_warnings=no])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
    [cause the library to emit debug messages at runtime.  Desirable only for developers. [default=no]])],
  [AS_CASE([$[]{enable_debug}],
    [yes], [AC_DEFINE([DEBUG], [1], [Define to 1 to enable debug messages])],
    [no],  [],
           [AC_MSG_ERROR([Unrecognized value '$[]{enable_debug}' for --enable-debug])]
    )],
  [enable_debug=no])
AS_IF([test "x$[]{enable_debug}" = xno],
  [AC_DEFINE([NDEBUG], [1], [Define to any value to disable assertions])])

AC_ARG_ENABLE([doc-rebuild],
  [AS_HELP_STRING([--enable-doc-rebuild],
    [Rebuild the generated documentation.  Requires doxygen.  [default=no]])],
  [AS_CASE([$[]{enable_doc_rebuild}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_doc_rebuild}' for --enable-doc-rebuild])]
    )],
  [enable_doc_rebuild='no'])

AM_CONDITIONAL([make_docs], [test "x$[]{enable_doc_rebuild}" = xyes])

AC_ARG_WITH([docs],
  [AS_HELP_STRING([--with-docs], [Whether to install documentation. See also --enable-doc-rebuild. [default=yes]])],
  [AS_CASE([$[]with_docs],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{with_docs}' for --with-docs])]
    )],
  [with_docs=yes])

AM_CONDITIONAL([install_docs], [test "x$[]{with_docs}" = xyes])

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

# Programs used during the build process

AC_PROG_CC

# Verify that the chosen C compiler supports C89 code
AS_IF([test "x${ac_cv_prog_cc_c89}" = xno], [
  AC_MSG_ERROR([The C compiler @<:@$[]CC@:>@ does not support C89 code.])
  ])

AC_PROG_GREP
AC_PROG_SED
AC_PROG_MKDIR_P

AC_PATH_PROG([DOXYGEN], [doxygen], [])
AS_IF([test "x$[]{DOXYGEN}" = x && test "x$[]{enable_doc_rebuild}" = xyes], [
  AC_MSG_ERROR([Documentation regeneration is enabled, but doxygen is not found.])
])

AC_SUBST([DOXYGEN])

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

# Output C/cpp flags that define the C language standards that must be
# supported, if such flags are known.  Although not harmful for production
# builds, this is intended mainly for development, to ensure that project
# language requirements are satisfied.  Thus it's no big deal for a
# production build if we don't know what flags to use for this purpose.
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

# Headers
AC_CHECK_HEADERS([fenv.h])

# Libraries

AC_CHECK_LIB([m], [log10])

AC_SEARCH_LIBS([sqlite3_open_v2], [sqlite3], [], [AC_MSG_FAILURE([SQLite3 not found or not recent enough])])
# TODO: check SQLite version >= 3.6.19 (or otherwise test that it supports and enforces foreign key constraints) */

AS_IF([which pkg-config], [ICU_LIBS=`pkg-config --libs icu` || AC_MSG_FAILURE([No pkg-config information for icu])],
                          [AC_MSG_FAILURE([pkg-config not found])])
AS_IF([test "x$[]{enable_debug}" = xyes], [
    ICU_LIBS="-licuio $[]{ICU_LIBS}"
  ])
AC_SUBST([ICU_LIBS])

# Specific functions

AC_CHECK_FUNCS([strdup fegetround])

# We need to determine whether a declaration of strdup() is available, which
# might not be the case in some C89-compliant environments.  This is a separate
# question from that of whether the function itself is available; build options
# may suppress the declaration even though the function is in fact available.
# (For example, options "-std=c89 -pedantic" have that effect on gcc 4.)

AC_CHECK_DECL([strdup],
  [AC_DEFINE([HAVE_DECL_STRDUP], [1], [Define to 1 if a declaration of strdup() is visible in string.h])])

# Similarly for fegetround
AC_CHECK_DECL([fegetround],
  [AC_DEFINE([HAVE_DECL_FEGETROUND], [1], [Define to 1 if a declaration of fegetround[]() is visible in fenv.h])],
  [],
  [[#include <fenv.h>]])

# Headers

AC_CHECK_HEADERS([io.h stdint.h unistd.h])
AC_CHECK_HEADER([sqlite3.h], [], [AC_MSG_FAILURE([Required header sqlite3.h was not found])])
AC_CHECK_HEADER([unicode/ustring.h], [], [AC_MSG_FAILURE([Required header unicode/ustring.h was not found])])

# Data types
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTMAX_T

# These output variables are defined for documentary purposes.  The makefiles add
# them to the CPP flags, but the current configure script always leaves them empty:
AC_SUBST([SQLITE_CPPFLAGS])
AC_SUBST([ICU_CPPFLAGS])

# Reset the CPPFLAGS.  The changes we made for internal purposes must not be
# propagated to the build environment (at least, not directly).
CPPFLAGS="${CPPFLAGS_save}"

# (Over)write the Makefiles and other configure-generated files
AC_CONFIG_FILES([
  Doxyfile
  Makefile
  src/Makefile
])

AC_OUTPUT

