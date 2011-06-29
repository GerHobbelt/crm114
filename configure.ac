#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# $Id: configure.ac,v 1.40 2004/08/18 13:52:04 vanbaal Exp $, maintained in crm114 SF CVS at
# $Source: /cvsroot/crm114/crm114/configure.ac,v $
#
# This script is free software; you can distribute it and/or modify it
# under the terms of the GNU GPL.  See the file COPYING.
#
# Copyright (C) 2003, 2004 Joost van Baal
# Process this file with autoconf to produce a configure script.

# AC_INIT(CRM114, [20031219-RC12.6], crm114-general@lists.sourceforge.net)
AC_INIT(CRM114, [20040627-BlameSeifkes-auto.1], crm114-general@lists.sourceforge.net)

# Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR(m4)

AM_INIT_AUTOMAKE

# Minimum Autoconf version required.
AC_PREREQ(2.60)

# Where to generate output; srcdir location.
AC_CONFIG_SRCDIR([src/crm_main.c])
AC_CONFIG_HEADERS([src/config.h])

# REVISION=`date +%Y-%m-%dT%H:%M`
# AC_DEFINE_UNQUOTED(REVISION, "$REVISION", [revision number of software])
# AC_SUBST(REVISION)

PACKAGE=$PACKAGE_NAME
VERSION=$PACKAGE_VERSION
PACKAGE=crm114
AC_SUBST(VERSION)
AC_SUBST(PACKAGE)

AC_PREFIX_DEFAULT(/usr/local)


# for crm114.lsm
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_COPYRIGHT([
Copyright (C) Copyright 2001-2007  William S. Yerazunis
])


# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AM_MAINTAINER_MODE
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LN_S


AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOSTTYPE, "$host_os", [Set host type])


# Checks for libraries.
AC_CHECK_LIB([m], [matherr])

## of course, this checking for Tre and stuff could better be handled using pkg-config
## possibly, tre might need to get patched for this: it should ship a .pc file.
crm_gnu_regex=no
AC_CHECK_LIB([tre], [regcomp])
if test "$ac_cv_lib_tre_regcomp" = "no"
then
  AC_MSG_NOTICE([no TRE found, let's see if your libc delivers regexp functionality.])
  AC_CHECK_FUNCS([regcomp], [crm_gnu_regex=yes], AC_MSG_ERROR([TRE nor GNU libc regex found.]))
fi
AM_CONDITIONAL(CRM_GNU_REGEX, test x$crm_gnu_regex = xyes)
## mutt's configure.in has a possibly inspiring AC_ARG_WITH(regex) hack.

AC_MSG_CHECKING([wether your runtime environment is ok])
# Check to be sure not to get some AC_FUNC_ 's fail miserably.
AC_RUN_IFELSE([
  int main () { exit (0); }],
[crm_run_time=yes],
[crm_run_time=no]
)
if test "$crm_run_time" = "no"
then
  AC_MSG_FAILURE([[It seems your runtime environment is broken.  Does your TRE installation need an explicitly set LD_LIBRARY_PATH?]])
fi
AC_MSG_RESULT([yes])

# Checks for header files.
AC_HEADER_DIRENT

# this test might fail when runtime is broken, e.g. due to ld which
# fails to find proper libtre
AC_HEADER_STDC

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h float.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
#
# failure of this test is pretty disastrous. be sure it's not due to
# broken runtime
AC_FUNC_MALLOC
#
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_CHECK_FUNCS([dup2 floor memchr memmove munmap strcasecmp strcspn strdup strncasecmp strstr])
AC_CHECK_FUNCS(strmov)

# Enable expanding of shebang in .crm scripts: get a usable BIN_PATH
# should include auto-archive snippet here, instead of quoting it.
AC_DEFUN([AC_DEFINE_DIR], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_define_dir=`eval echo [$]$2`
  ac_define_dir=`eval echo [$]ac_define_dir`
  $1="$ac_define_dir"
  AC_SUBST($1)
])
AC_DEFINE_DIR([BIN_PATH], [bindir])
# usage: set ``#! @BIN_PATH@/crm'' in top of foo.crm.in

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(INSTALLFLAGS)


AC_CONFIG_FILES([Makefile
                 crm114.lsm
                 docs/Makefile
                 examples/Makefile
                 examples/pad.crm
                 examples/shroud.crm
                 mailfilter/Makefile
                 mailfilter/classifymail.crm
                 mailfilter/mailfilter.crm
                 mailfilter/rewriteutil.crm
                 mailfilter/unionintersecttest.crm
                 man/Makefile
                 man/include.zmm
                 src/Makefile
                 tests/Makefile
                 tests/aliustest.crm
                 tests/approxtest.crm
                 tests/argtest.crm
                 tests/backwardstest.crm
                 tests/beeptest.crm
                 tests/bracktest.crm
                 tests/classifytest.crm
                 tests/escapetest.crm
                 tests/eval_infiniteloop.crm
                 tests/exectest.crm
                 tests/fataltraptest.crm
                 tests/inserttest_a.crm
                 tests/inserttest_b.crm
                 tests/inserttest_c.crm
                 tests/learntest.crm
                 tests/match_isolate_test.crm
                 tests/matchtest.crm
                 tests/mathalgtest.crm
                 tests/mathrpntest.crm
                 tests/nestaliustest.crm
                 tests/overalterisolatedtest.crm
                 tests/paolo_overvars.crm
                 tests/randomiotest.crm
                 tests/rewritetest.crm
                 tests/skudtest.crm
                 tests/statustest.crm
                 tests/traptest.crm
                 tests/uncaughttraptest.crm
                 tests/unionintersecttest.crm
                 tests/userdirtest.crm
                 tests/windowtest.crm
                 tests/windowtest_fromvar.crm
])

AC_OUTPUT


cat <<EOF

Congratulations, your $PACKAGE_NAME $PACKAGE_VERSION is configured now.
You can now build and install $PACKAGE_NAME using the Makefile, by calling

  $ make
  # make install

.
EOF

