#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# This script is free software; you can distribute it and/or modify it
# under the terms of the GNU GPL.  See the file COPYING.
#
# Copyright (C) 2003, 2004 Joost van Baal
# Process this file with autoconf to produce a configure script.


# AC_INIT(CRM114, [20031219-RC12.6], crm114-general@lists.sourceforge.net)
AC_INIT(CRM114, 20070807-BlameTheInterns, crm114-general@lists.sourceforge.net)

# Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR(m4)

AM_INIT_AUTOMAKE

# Minimum Autoconf version required.
AC_PREREQ(2.60)

# Where to generate output; srcdir location.
AC_CONFIG_SRCDIR(src/crm_main.c)
AC_CONFIG_HEADERS(src/config.h)

#REVISION=`date +%Y%m%dT%H%M`
#AC_DEFINE_UNQUOTED(REVISION, "$REVISION", [revision code of the software])
#AC_SUBST(REVISION)

# VER_SUFFIX=Ger-`date +%m%dT%H`
VER_SUFFIX=
AC_DEFINE_UNQUOTED(VER_SUFFIX, "$VER_SUFFIX", [version suffix code of the software])
AC_SUBST(VER_SUFFIX)

# one build every three hours is the fastest I can do... I guess ;-)
# At least this'll give me at least 3 years of build numbering before I run out of
# the 4 digit set. And then who cares when a fifth digit is required, eh?

let "buildnumber=(`date +%s` - 1185402255) / 10800"

TAR_FILENAME_POSTFIX=Ger-$buildnumber
AC_DEFINE_UNQUOTED(TAR_FILENAME_POSTFIX, "$TAR_FILENAME_POSTFIX", [distribution archive filename postfix code of the software])
AC_SUBST(TAR_FILENAME_POSTFIX)


# AC_DEFINE_UNQUOTED(SILENT, "@", [silent mode for make])
SILENT="@"
AC_SUBST(SILENT)




AC_PREFIX_DEFAULT(/usr/local)


# for crm114.lsm
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_COPYRIGHT([
Copyright (C) Copyright 2001-2007  William S. Yerazunis
])

AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC(gcc egcs cl cc lcc)
AC_GNU_SOURCE
AC_AIX
AC_MINIX
AC_C_PROTOTYPES



AC_PROG_AWK
AC_PROG_SED
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_CPP
AC_PROG_CPP_WERROR
AC_PROG_LD_GNU
AC_PROG_LD
AC_PROG_NM
AC_PATH_PROG([cvs2cl], [cvs2cl])
AC_PATH_PROG([uncrustify], [uncrustify])
AC_PATH_PROG([diff], [diff])
AC_PATH_PROG([sevenzip], [7z])
AC_PATH_PROG([zip], [zip])
AC_PATH_PROG([tar], [tar])

AX_DIFF_OPTION([E])
AX_DIFF_OPTION([b])
AX_DIFF_OPTION([w])
AX_DIFF_OPTION([B])
AX_DIFF_OPTION([d])
AX_DIFF_OPTION([-strip-trailing-cr])


#
# since AC_PROG_CC sets '-O2' in CFLAGS by default, while _we_'d like
# to have '-O3', we override the operation (for gcc only) right here.
# Of course, this is rather hackish, so if anyone knows about a cleaner
# solution, I'm all for it!
#                                               Ger Hobbelt
if test ! -z "$SED"; then
  CFLAGS=`echo $CFLAGS | $SED -e 's/ -O2//'`
fi
AC_ARG_ENABLE([debug], AC_HELP_STRING([--disable-debug],
  [Specify to create a so-called 'production' or 'release build'.
You can explicitly specify '--enable-debug' to create a standard
CRM (debuggable) build. (default: debugging turned ON)]),
  [AX_CFLAGS_GCC_OPTION([-O0])],
  [AX_CFLAGS_GCC_OPTION([-O3])]
  )

AC_ARG_ENABLE([extended-compile-warnings],
  AC_HELP_STRING([--disable-extended-compile-warnings],
    [turn maximum compiler/linker warnings level off
(default: turn on all warnings)]),
  [],
  [AX_CFLAGS_WARN_ALL(AM_CFLAGS)
dnl  AX_CFLAGS_WARN_ALL_ANSI(AM_CFLAGS)
  ])

AC_ARG_ENABLE([assertions],
  AC_HELP_STRING([--disable-assertions],
    [turn assertion checks off.
This will produce a faster running executable but such at the cost
of lacking a specific type of 'developer' checks. These SHOULD not
get triggered EVER, so you MAY assume specifying this option is
a 'free speedup'. (default: assertions are ENABLED)]),
  AC_DEFINE_UNQUOTED(CRM_DONT_ASSERT, 1, [DISable ASSERT/VERIFY checks])
  )

AC_ARG_ENABLE([untrappable-asserts],
  AC_HELP_STRING([--enable-untrappable-assertions],
    [assertion checks are untrappable (instead of merely being a
trappable error report). This means any triggered assertion checks
will NOT show up in your CRM trap handlers. (default: assertions are 'trappable')]),
  AC_DEFINE_UNQUOTED(CRM_ASSERT_IS_UNTRAPPABLE, 1, [untrappable ASSERT/VERIFY checks])
  )




AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOSTTYPE, "$host_os", [Set host type])


# Checks for libraries.
AC_CHECK_LIB([m], [matherr])

# ## of course, this checking for Tre and stuff could better be handled using pkg-config
# ## possibly, tre might need to get patched for this: it should ship a .pc file.
# crm_gnu_regex=no
# AC_CHECK_LIB([tre], [regcomp])
# if test "$ac_cv_lib_tre_regcomp" = "no"
# then
#   AC_MSG_NOTICE([no TRE found, let's see if your libc delivers regexp functionality.])
#   AC_CHECK_FUNCS([regcomp], [crm_gnu_regex=yes], AC_MSG_ERROR([TRE nor GNU libc regex found.]))
# fi
# AM_CONDITIONAL(CRM_GNU_REGEX, test x$crm_gnu_regex = xyes)
# ## mutt's configure.in has a possibly inspiring AC_ARG_WITH(regex) hack.


dnl *************************
dnl regex libraries tests
dnl *************************
AC_ARG_WITH(regex-includes,
  [  --with-regex-includes=DIR    where you have regex includes],
  WITH_REGEX_INC="$withval")
if test -n "$WITH_REGEX_INC"; then
  CPPFLAGS="$CPPFLAGS -I$WITH_REGEX_INC"
fi

AC_ARG_WITH(regex-libraries,
  [  --with-regex-libraries=DIR   where you have regex libraries],
  WITH_REGEX_LIB="$withval")
if test -n "$WITH_REGEX_LIB"; then
  LDFLAGS="$LDFLAGS -L$WITH_REGEX_LIB"
fi

dnl ********************************
dnl all the various REGEX tests
dnl ********************************
AC_ARG_WITH(regex, [  --with-regex=[none/auto/posix/gnu/v8/bsd/pcre/lregex/tre]
                             select type of regular expressions you want
                             to use],
  WITH_REGEX="$withval", WITH_REGEX="auto")

case "$WITH_REGEX" in
  none | auto | posix | gnu | v8 | bsd | pcre | lregex | tre) ;;
  *) AC_MSG_ERROR("Bad selection in --with-regex option !");;
esac


if eval "test x$WITH_REGEX = xnone"; then
  want_regex_impl=no
else
  want_regex_impl=yes
  AC_CHECK_HEADERS(regex.h)
fi

dnl ********************
dnl test for TRE regex --- this configure section needs to be FIXED
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xtre"; then
    AC_CHECK_PROG(TRE_CONFIG, tre-config, "yes", "no")
    if eval "test x$TRE_CONFIG = xyes"; then
      tre_cflags=`tre-config --cflags`
      tre_libs=`tre-config --libs`
      x_cflags="$x_cflags $tre_cflags"
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_TRE_REGEX, 1, [TRE REs])
      AC_CHECK_HEADERS(tre/regex.h)
      LIBS="$LIBS $tre_libs"
      want_regex_impl=no
    else
      AC_CHECK_LIB(tre, tre_compile, [
        AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
        AC_DEFINE_UNQUOTED(HAVE_TRE_REGEX, 1,
          [TRE REs])
        AC_CHECK_HEADERS(tre/regex.h)
        LIBS="$LIBS -ltre";want_regex_impl="no"])
    fi
  fi
fi

dnl ********************************
dnl test for POSIX regex in libregex
dnl ********************************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xlregex"; then
    AC_CHECK_LIB(regex, regcomp, [
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_POSIX_REGEX, 1, [POSIX REs])
      LIBS="$LIBS -lregex"; want_regex_impl="no"])
  fi
fi


dnl ********************
dnl test for POSIX regex
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xposix"; then
    AC_MSG_CHECKING([for POSIX regcomp()])
    ac_pregcomp="no"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #include <sys/types.h>
    #include <regex.h>
    ]], [[
      regex_t pr;
      regcomp(&pr , "" , 0);
    ]])],[ac_pregcomp="yes"],[])
    AC_MSG_RESULT($ac_pregcomp)
    if eval "test x$ac_pregcomp = xyes"; then
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_POSIX_REGEX, 1, [POSIX REs])
      want_regex_impl=no
    fi
  fi
fi

dnl ********************
dnl test for PCRE regex
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xpcre"; then
    AC_CHECK_PROG(PCRE_CONFIG, pcre-config, "yes", "no")
    if eval "test x$PCRE_CONFIG = xyes"; then
      pcre_cflags=`pcre-config --cflags`
      pcre_libs=`pcre-config --libs`
      x_cflags="$x_cflags $pcre_cflags"
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_PCRE_REGEX, 1, [PCRE REs])
      LIBS="$LIBS $pcre_libs"
      want_regex_impl=no
    else
      AC_CHECK_LIB(pcre, pcre_compile, [
        AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
        AC_DEFINE_UNQUOTED(HAVE_PCRE_REGEX, 1,
          [PCRE REs])
        LIBS="$LIBS -lpcre";want_regex_impl="no"])
    fi
  fi
fi

dnl ********************
dnl test for GNU regex
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xgnu"; then
    AC_MSG_CHECKING([for GNU regular expressions])
    ac_gnu_re="no"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[
      re_compile_pattern();
    ]])],[ac_gnu_re="yes"],[])
    AC_MSG_RESULT($ac_gnu_re)
    if eval "test x$ac_gnu_re = xyes"; then
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_GNU_REGEX, 1, [GNU REs])
      want_regex_impl=no
    fi
  fi
fi

dnl ********************
dnl test for Bell V8 regex
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xv8"; then
    AC_MSG_CHECKING([for Bell V8 regcomp()])
    ac_v8regcomp="no"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <regexp.h>]], [[
      regexp *pr;
      pr = regcomp("");
    ]])],[ac_v8regcomp="yes"],[])
    AC_MSG_RESULT($ac_v8regcomp)
    if eval "test x$ac_v8regcomp = xyes"; then
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_V8_REGEX, 1, [SYSV 8 REs])
      AC_CHECK_FUNC(regsub,
        AC_DEFINE_UNQUOTED(HAVE_V8_REGSUB, 1,
          [SYSV 8 RE exports regsub]))
      want_regex_impl=no
    fi
  fi
fi

dnl ********************
dnl test for BSD regex
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xbsd"; then
    AC_MSG_CHECKING([for BSD regular expressions])
    ac_bsd_re="no"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[
      re_comp(".*");
      re_exec(".*");
    ]])],[ac_bsd_re="yes"],[])
    AC_MSG_RESULT($ac_bsd_re)
    if eval "test x$ac_bsd_re = xyes"; then
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_BSD_REGEX, 1, [BSD REs])
      want_regex_impl=no
    fi
  fi
fi





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
AC_HEADER_TIME
# AC_HEADER_ASSERT
AC_HEADER_STAT
AC_CHECK_HEADERS(fcntl.h float.h stdlib.h string.h unistd.h)
AC_CHECK_HEADERS(stdio.h ctype.h errno.h sys/types.h sys/wait.h)
AC_CHECK_HEADERS(dirent.h unistd.h sys/mman.h sys/stat.h math.h)
AC_CHECK_HEADERS(locale.h sys/times.h signal.h time.h sys/time.h)
AC_CHECK_HEADERS(inttypes.h arpa/inet.h netinet/in.h)
AC_CHECK_HEADERS(stdarg.h varargs.h float.h limits.h)
AC_CHECK_HEADERS(libintl.h sys/utime.h utime.h wchar.h)
AC_CHECK_HEADERS(crtdbg.h signal.h io.h direct.h process.h lmcons.h)
AC_CHECK_HEADERS(getopt.h getopt_ex.h)
AC_CHECK_HEADERS(endian.h sys/param.h)


# see: http://www.gnu-pascal.de/crystal/gpc/en/mail11031.html
AC_CHECK_HEADERS(crt_externs.h)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_BIGENDIAN
AC_C_CHAR_UNSIGNED
AC_C_LONG_LONG
AC_C_STRINGIZE
AC_CHECK_SIZEOF([long int])
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES(clock_t)
AC_CHECK_TYPES(pid_t)
AC_CHECK_TYPES(mode_t)

# check for a environ/__environ global var
AC_ENVIRON

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
AC_FUNC_REALLOC
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_STRFTIME
AC_FUNC_SNPRINTF
AC_FUNC_MEMMOVE
VL_LIB_READLINE
AC_CHECK_FUNCS(dup2 bcopy)
AC_CHECK_FUNCS(memchr memset)
AC_CHECK_FUNCS(munmap msync)
AC_CHECK_FUNCS(strcasecmp strncasecmp strcspn strdup strstr strchr)
AC_CHECK_FUNCS(strnicmp stricmp)
AC_CHECK_FUNCS(strncpy _snprintf _vsnprintf)
AC_CHECK_FUNCS(strmov)
AC_CHECK_FUNCS(times)
AC_CHECK_FUNCS(stat _stat utime _utime)
AC_CHECK_FUNCS(strerror)
AC_CHECK_FUNCS(getpid getppid waitpid system)
AC_CHECK_FUNCS(truncate)
AC_CHECK_FUNCS(pipe)
AC_CHECK_FUNCS(floor fabs pow log logl log10 sqrt log2)
AC_CHECK_FUNCS(isalnum isalpha isascii isblank iscntrl isdigit isgraph islower isprint ispunct isspace isupper isxdigit)
AC_CHECK_FUNCS(setlocale)
AC_CHECK_FUNCS(getopt getopt_long)
AC_CHECK_FUNCS(__debugbreak)



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
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(INSTALLFLAGS)
AC_SUBST(DIFF_FLAGS)


CRM=":|crm"
AC_SUBST(CRM)


AC_CONFIG_FILES([Makefile
                 crm114.lsm
                 docs/Makefile
                 examples/Makefile
                 mailfilter/Makefile
                 man/Makefile
                 man/include.zmm
                 src/Makefile
                 src/diff_em.sh
                 src/indent.sh
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
                 tests/tenfold_validate.crm
                 tests/tokendelimiterbugtest.crm
                 tests/bracesbugtest.crm
                 examples/pad.crm
                 examples/shroud.crm
                 mailfilter/classifymail.crm
                 mailfilter/mailfilter.crm
                 mailfilter/rewriteutil.crm
])

AC_OUTPUT


cat <<EOF

Congratulations, your $PACKAGE_NAME $PACKAGE_VERSION is configured now.
You can now build and install $PACKAGE_NAME using the Makefile, by calling

  $ make
  $ make install

You may wish to test the newly build binary by running

  $ make megatest
.
EOF

