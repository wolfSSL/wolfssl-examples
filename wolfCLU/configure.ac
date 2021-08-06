# configure.ac
#
# Copyright (C) 2006-2020 wolfSSL Inc.
# All rights reserved.
#
# This file is part of wolfssl command line utility.
#
#

#requires user to have AutoConf version 2.63 or greater.
AC_PREREQ([2.63])

AC_INIT([wolfssl_clu], [0.0.5], [http://www.wolfssl.com])

#a helpful directory to keep clutter out of root
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADER([src/config.h])
AC_CANONICAL_HOST
AC_CANONICAL_BUILD


AM_INIT_AUTOMAKE([1.11 -Wall -Werror -Wno-portability foreign tar-ustar subdir-objects no-define color-tests])

AC_ARG_PROGRAM
AC_DEFUN([PROTECT_AC_USE_SYSTEM_EXTENSIONS],
         [AX_SAVE_FLAGS
          AC_LANG_PUSH([C])
          AC_USE_SYSTEM_EXTENSIONS
          AC_LANG_POP([C])
          AX_RESTORE_FLAGS
          ])
#PROTECT_AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_MACRO_DIR([m4])

#requires user to have LibTool version 2.2 or greater
LT_PREREQ([2.2])
LT_INIT([disable-static],[win32-dll])
LT_LANG([C++])
LT_LANG([C])

gl_VISIBILITY
AS_IF([ test -n "$CFLAG_VISIBILITY" ], [
       AM_CPPFLAGS="$AM_CPPFLAGS $CFLAG_VISIBILITY"
       CPPFLAGS="$CPPFLAGS $CFLAG_VISIBILITY"
       ])

#silent
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

# Checks for headers/libraries
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/time.h string.h termios.h unistd.h])
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_FUNCS([gettimeofday memset alarm])
AC_FUNC_MALLOC
AC_TYPE_INT64_T

# Requirements
TAO_REQUIRE_LIBWOLFSSL
# Have John or Todd assist in writing have_opensslextra.m4
#TAO_REQUIRE_OPENSSLEXTRA
# Have John or Todd assist in writing have_pwdbased.m4
#TAO_REQUIRE_PWDBASED

# since we have autoconf available, we can use wolfssl options header
AM_CFLAGS="$AM_CFLAGS -DHAVE_WOLFSSL_OPTIONS"

# capture user C_EXTRA_FLAGS from ./configure line, CFLAGS may hold -g -O2 even
# if user doesn't override, no way to tell
USER_C_EXTRA_FLAGS="$C_EXTRA_FLAGS"
CFLAGS=""

###############################################
#                                             #
#     Need a genKey check                     #
#                                             #
#     Need aes-cbc-[128][192][256] check      #
#     Need aes-ctr-[128][192][256] check      #
#     Need 3des-cbc-[056][112][168] check     #
#     Need Camellia-cbc-[128][192][256] check #
#                                             #
###############################################

# Checks for typedefs, structures, and compiler characteristics.
if test "$ac_cv_sizeof_long" = "8"; then
   AM_CFLAGS="$AM_CFLAGS -DSIZEOF_LONG=8"
else
    if test "$ac_cv_sizeof_long_long" = "8"; then
        AM_CFLAGS="$AM_CFLAGS -DSIZEOF_LONG_LONG=8"
    fi
fi

# add user C_EXTRA_FLAGS back
CFLAGS="$CFLAGS $USER_C_EXTRA_FLAGS"

CREATE_HEX_VERSION
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

# FINAL
AC_CONFIG_FILES([Makefile])

#AX_CREATE_GENERIC_CONFIG
AX_AM_JOBSERVER([yes])

AC_OUTPUT()

# force make clean
echo "---"
echo "Running make clean..."
make clean >/dev/null 2>&1
echo


# output config summary
echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * C Flags:                   $CFLAGS"
echo "   * CPP Flags:                 $CPPFLAGS"
echo "   * LIB Flags:                 $LIB"
