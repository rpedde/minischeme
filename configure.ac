#-*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
#
# The 0MQ version number is extracted from include/zmq.h using
# the version.sh script. Hence, it should be updated there.
# The version in git should reflect the *next* version planned.
#
#AC_INIT([zeromq],[m4_esyscmd([./version.sh])],[zeromq-dev@lists.zeromq.org])
AC_INIT([minischeme], 0.0.1)


AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_HEADERS([src/platform.h])
AM_INIT_AUTOMAKE(foreign)
dnl m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# This lets us use PACKAGE_VERSION in Makefiles
AC_SUBST(PACKAGE_VERSION)

# Take a copy of original flags
MS_ORIG_CFLAGS="${CFLAGS:-none}"
MS_ORIG_CPPFLAGS="${CPPFLAGS:-none}"
MS_ORIG_CXXFLAGS="${CXXFLAGS:-none}"

CFLAGS="${CFLAGS} -D_GNU_SOURCE"

# Checks for programs.
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AM_PROG_LIBTOOL

# For host type checks
AC_CANONICAL_HOST

# Checks for header files.
AC_HEADER_STDC

AC_OUTPUT(Makefile src/Makefile)

                    
