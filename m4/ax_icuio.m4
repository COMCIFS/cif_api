#
# SYNOPSIS
#   AX_ICUIO
#
# DESCRIPTION
#
#   This macro attempts to determine the preprocessor flags and libraries
#   needed for a program that depends on the ICU I/O module.  ICU these days
#   recommends pkg-config for this purpose, but there are ICU packages in
#   currently-supported operating systems that have poor pkg-config data for
#   ICU.  This macro attempts to adapt.
#
#   The analysis is done assuming the C language.  Results are provided via
#   shell variables $ICU_CPPFLAGS and $ICU_LIBS.
#
#   As secondary results, the macro also sets shell variable $ICU_PKG to the
#   name of the pkg-config package for ICU, and sets shell variable
#   $ICU_NONPKG_LIBS to a list of libraries required for the ICU I/O
#   functions, but not in the pkg-config results for the chosen package.
#
# LICENSE
#
#   Copyright 2015 John C. Bollinger
#
#
#   This file is part of the CIF API.
#
#   The CIF API is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   The CIF API is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
#

AC_DEFUN([AX_ICUIO], [
  AC_LANG_PUSH([C])
  AC_REQUIRE([AC_PROG_CPP])dnl
  AC_REQUIRE([AC_PROG_SED])dnl
  AC_PATH_PROG([PKG_CONFIG], [pkg-config], [:])
  AS_IF([test "${PKG_CONFIG}" = :], [AC_MSG_ERROR([The 'pkg-config' command was not found])])

  ICU_PKG=
  ICU_NONPKG_LIBS=
  AC_MSG_CHECKING([libraries needed for ICU I/O])
  icuio_tags="icu-io icu"
  for tag in ${icuio_tags}; do
    ICU_LIBS=`${PKG_CONFIG} --libs ${tag} 2>/dev/null` && { ICU_PKG=${tag}; break; }
  done
  AS_IF([test "x${ICU_PKG}" = x], [AC_MSG_ERROR([pkg-config does not know about ICU])])
  AC_MSG_RESULT([pkg-config suggests ${ICU_LIBS}])

  AC_MSG_CHECKING([ICU CPP flags])
  ICU_CPPFLAGS=`${PKG_CONFIG} --cflags-only-I ${tag} 2>/dev/null` || {
    AC_MSG_RESULT([])
    AC_MSG_ERROR([Could not retrieve flags for package ${tag}])
  }
  AS_IF([test "x${ICU_CPPFLAGS}" = x], [AC_MSG_RESULT([@{:@none@:}@])], [AC_MSG_RESULT([${ICU_CPPFLAGS}])])

  _ax_icuio_CPPFLAGS_save="${CPPFLAGS}"
  CPPFLAGS="${CPPFLAGS} ${ICU_CPPFLAGS}"

  AC_CHECK_HEADER([unicode/ustring.h], [], [AC_MSG_FAILURE([Required header unicode/ustring.h was not found])])
  AC_CHECK_HEADER([unicode/ustdio.h], [], [AC_MSG_FAILURE([Required header unicode/ustdio.h was not found])])

  AC_MSG_CHECKING([the real name of u_fopen@{:@@:}@])
  AC_LANG_CONFTEST([
AC_LANG_DEFINES_PROVIDED
#include <unicode/ustdio.h>
CONF_LINE: u_fopen
  ])
  _ax_icuio_UFOPEN=`${CPP} conftest.c | ${SED} -n '/^CONF_LINE/ s/.*\s\(\w\+\)\s*$/\1/p'` || _ax_icuio_UFOPEN='error'
  rm conftest.c
  AC_MSG_RESULT([${_ax_icuio_UFOPEN}])
  AS_IF([test "x$_ax_icuio_UFOPEN" = xerror], [AC_MSG_ERROR([An error while determining the true name of u_fopen])])

  _ax_icuio_LIBS_save="${LIBS}"
  LIBS="${ICU_LIBS} ${LIBS}"
  AC_CHECK_FUNC([${_ax_icuio_UFOPEN}], [], [
    ICU_LIBS="-licuio ${ICU_LIBS}"
    LIBS="${ICU_LIBS} ${LIBS_save}"
    AC_MSG_NOTICE([trying again with ${ICU_LIBS}])
    AS_UNSET([AS_TR_SH([ac_cv_func_${_ax_icuio_UFOPEN}])])
    AC_CHECK_FUNC([${_ax_icuio_UFOPEN}], [ICU_NONPKG_LIBS='-licuio'],
      [AC_MSG_ERROR([Could not identify ICU I/O libraries])])
  ])
  LIBS="${_ax_icuio_LIBS_save}"
  CPPFLAGS="${_ax_icuio_CPPFLAGS_save}"
  AC_LANG_POP([C])
])

