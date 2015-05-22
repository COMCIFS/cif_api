#
# Copyright (C) 2013, 2014 John C. Bollinger
#
# All rights reserved.
#

dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.63])
dnl quoting delimiters in the bug report address does not affect the generated
dnl configure script, but may offer some protection against the address being
dnl scraped from this Autoconf file:
AC_INIT([CIF API], [0.3.0], [John[.]Bollinger[@]StJude[.]org])
AC_CONFIG_SRCDIR([src/cif.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([parallel-tests])

# Option win32-dll indicates that the package *can* produce clean Win32 DLLs,
# not that those are necessarily the target for the current build.
LT_INIT([disable-static win32-dll])

AC_CONFIG_HEADERS([config.h])

# User options

AM_MAINTAINER_MODE([enable])

AC_ARG_ENABLE([doc-rebuild],
  [AS_HELP_STRING([--enable-doc-rebuild],
    [Rebuild the generated documentation.  Requires doxygen.  [default=no]])],
  [AS_CASE([$[]{enable_doc_rebuild}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_doc_rebuild}' for --enable-doc-rebuild])]
    )],
  [enable_doc_rebuild='no'])

AM_CONDITIONAL([make_docs], [test "x$[]{enable_doc_rebuild}" = xyes])

AC_ARG_ENABLE([c89-enforcement],
  [AS_HELP_STRING([--enable-c89-enforcement],
    [configure normally tries to verify that the chosen C compiler supports C89.  That behavior can be diasabled via this option, as may be necessary in certain environments, such as when a C++ compiler is chosen. [default=yes]])],
  [AS_CASE([$[]{enable_c89_enforcement}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_c89_enforcement}' for --enable-c89-enforcement])]
    )],
  [enable_c89_enforcement=yes])

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

AC_ARG_ENABLE([extra-warnings],
  [AS_HELP_STRING([--enable-extra-warnings],
    [turn on extra compiler diagnostics intended to help developers identify questionable code.  Not all warnings emitted as a result indicate genuine problems. [default=no]])],
  [AS_CASE([$[]{enable_extra_warnings}],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{enable_extra_warnings}' for --enable-extra-warnings])]
    )],
  [enable_extra_warnings=no])

AC_ARG_ENABLE([profiling],
  [AS_HELP_STRING([--enable-profiling@<:@=type@:>@],
    [build the library and example programs with profiling support (of the specified flavor).  If no profiling flavor
     is given then configure will attempt to guess based on the chosen C compiler.  The profiling types currently known
     to configure are: gnu. [default=no]])],
  [],
  [enable_profiling=no])

AC_ARG_ENABLE([query-profiling],
  [AS_HELP_STRING([--enable-query-profiling], [make the library emit information at runtime pertaining to the time
    consumed by queries performed against the internal database.  Appropriate only for debugging CIF API performance
    issues; do not enable for a production build.])],
  [
    AS_CASE([$enableval],
      [yes|no], [],
                [AC_MSG_ERROR([Unrecognized option value in --enable-query-profiling=$enableval])])
  ],
  [enable_query_profiling='no'])
AS_IF([test "x$enable_query_profiling" = xyes], [
  AC_DEFINE([PERFORM_QUERY_PROFILING], [1], [Define to 1 to enable query profiling])
])

AC_ARG_WITH([examples],
  [AS_HELP_STRING([--with-examples], [build and install the example programs])], [
    AS_CASE([$[]withval],
      [yes|no], [],
                [AC_MSG_ERROR([--with-examples=$[]withval is not supported])])
  ], [
    with_examples='no'
  ])
  
AM_CONDITIONAL([build_examples], [test "x$[]with_examples" = xyes])

AC_ARG_WITH([docs],
  [AS_HELP_STRING([--without-docs], [do not install the documentation. See also --enable-doc-rebuild. [default=yes]])],
  [AS_CASE([$[]with_docs],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '$[]{with_docs}' for --with-docs])]
    )],
  [with_docs=yes])

AM_CONDITIONAL([install_docs], [test "x$[]{with_docs}" = xyes])

AC_ARG_WITH([test-fuzz],
  [AS_HELP_STRING([--with-test-fuzz=<fuzz ulps>],
    [specifies a relative fuzz factor for the tests to use in inexact floating-point comparisons. [default=2]])],
  [
    AS_CASE([$[]withval],
      [no],  [
        AC_MSG_WARN([option --without-test-fuzz is non-sensical; using the default fuzz factor])
        with_test_fuzz=2
      ],
      [yes], [
        AC_MSG_WARN([option --with-test-fuzz is meaningless without a specified fuzz factor])
        with_test_fuzz=2
      ],
      [*@<:@^0-9@:>@*], [AC_MSG_ERROR([--with-test-fuzz must specify a non-negative integer fuzz factor, not "$withval"])]
    )
  ], [with_test_fuzz=2])
AC_DEFINE_UNQUOTED([DBL_TEST_ULPS], [$with_test_fuzz],
  [A relative fuzz factor for the tests to use in floating-point comparisons that should not require exact equality])

# Programs used during the build process

AC_PROG_CC

# Verify that the chosen C compiler supports C89 code
AS_IF([test "x${enable_c89_enforcement}" = xyes && test "x${ac_cv_prog_cc_c89}" = xno], [
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
      [WARNING_FLAGS="${WARNING_FLAGS} -Wswitch -Wswitch-default -Wcast-qual -Wbad-function-cast"])
  ],
  [WARNING_FLAGS=""])
AC_SUBST([WARNING_FLAGS])

AS_IF([test "x${enable_profiling}" = xyes], [
  AS_CASE([${CC}],
          [gcc*|g++*], [enable_profiling='gnu'],
                       [enable_profiling='no'; AC_MSG_WARN([I don't know what profiling type to use for the '${CC}' compiler])])
])

# Output flags appropriate for the chosen type of profiling support
PROFILING_CPPFLAGS=
PROFILING_CFLAGS=
PROFILING_LDFLAGS=
AS_CASE([$enable_profiling],
  [gnu|gprof], [PROFILING_CFLAGS='-pg'; PROFILING_LDFLAGS='-pg'],
  [no],  [:],
         [AC_MSG_WARN([I don't know profiling type '${enable_profiling}'])]
  )
AC_SUBST([PROFILING_CPPFLAGS])
AC_SUBST([PROFILING_CFLAGS])
AC_SUBST([PROFILING_LDFLAGS])

AC_EGREP_CPP([windows], [
#ifdef _WIN32
  windows
#endif
], [is_windows=yes], [is_windows=no])

AM_CONDITIONAL([win32], [test "x${is_windows}" = xyes])

# Headers
AC_CHECK_HEADERS([fenv.h])

# Libraries

AC_CHECK_LIB([m], [log10])

AC_SEARCH_LIBS([sqlite3_open_v2], [sqlite3], [], [AC_MSG_FAILURE([SQLite3 not found or not recent enough])])
# TODO: check SQLite version >= 3.6.19 (or otherwise test that it supports and enforces foreign key constraints) */

AS_IF([which pkg-config >/dev/null], [
    AC_MSG_CHECKING([for the ICU I/O library])
    icuio_tags="icu-io icu"
    ICU_LIBS=
    for tag in ${icuio_tags}; do
      ICU_LIBS=`pkg-config --libs ${tag} 2>/dev/null`" -licuio" && break
    done
    AC_MSG_RESULT([${ICU_LIBS}])
    AS_IF([test "x${ICU_LIBS}" = x], [AC_MSG_FAILURE([No pkg-config information for any of: ${icuio_tags}])])
    AC_MSG_CHECKING([for ICU CPP flags])
    ICU_CPPFLAGS=`pkg-config --cflags-only-I ${tag} 2>/dev/null` || {
      AC_MSG_RESULT([])
      AC_MSG_FAILURE([Could not retrieve flags for package $[]{tag}])
    }
    AC_MSG_RESULT([${ICU_CPPFLAGS}])
    CPPFLAGS="${CPPFLAGS} ${ICU_CPPFLAGS}"
  ],
  [AC_MSG_FAILURE([pkg-config not found])])
AC_SUBST([ICU_CPPFLAGS])
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
  [AC_DEFINE([HAVE_DECL_FEGETROUND], [1], [Define to 1 if a declaration of fegetround() is visible in fenv.h])],
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
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# These output variables are defined for documentary purposes.  The makefiles add
# them to the CPP flags, but the current configure script always leaves them empty:
AC_SUBST([SQLITE_CPPFLAGS])
AC_SUBST([ICU_CPPFLAGS])

# Reset the CPPFLAGS.  The changes we made for internal purposes must not be
# propagated to the build environment (at least, not directly).
CPPFLAGS="${CPPFLAGS_save}"

# (Over)write the Makefiles and other configure-generated files
AC_CONFIG_FILES([
  Makefile
  src/Makefile
])

AC_OUTPUT

