#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# This script is free software; you can distribute it and/or modify it
# under the terms of the GNU GPL.  See the file COPYING.
#
# Copyright (C) 2003, 2004 Joost van Baal
# Copyright (C) 2007 Ger Hobbelt
#
# Process this file with autoconf to produce a configure script.





AC_INIT([CRM114], [20080630-BlameVT], [crm114-general@lists.sourceforge.net])

# Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 check-news dist-bzip2 dist-zip dist-tarZ]) 

# Minimum Autoconf version required.
AC_PREREQ([2.62])

# Where to generate output; srcdir location.
AC_CONFIG_SRCDIR([src/crm_main.c])
AC_CONFIG_HEADERS([src/config.h])




# #########################################################################
#
# Construct a build number to go with the major.minor release version.
#

# one build every three hours is the fastest I can do... I guess ;-)
# At least this'll give me at least 3 years of build numbering before I run out of
# the 4 digit set. And then who cares when a fifth digit is required, eh?

let "buildnumber=(`date +%s` - 1185402255) / 10800"

TAR_FILENAME_POSTFIX=Ger-$buildnumber
AC_DEFINE_UNQUOTED(TAR_FILENAME_POSTFIX, "$TAR_FILENAME_POSTFIX", [distribution archive filename postfix code of the software])
AC_SUBST(TAR_FILENAME_POSTFIX)

#REVISION=`date +%Y%m%dT%H%M`
REVISION=$buildnumber
AC_DEFINE_UNQUOTED(REVISION, "$REVISION", [revision number of software])
AC_SUBST(REVISION)

# VER_SUFFIX=Ger-`date +%m%dT%H`
VER_SUFFIX=
AC_DEFINE_UNQUOTED(VER_SUFFIX, "$VER_SUFFIX", [version suffix code of the software])
AC_SUBST(VER_SUFFIX)




AC_PREFIX_DEFAULT([/usr/local])



# for crm114.lsm
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_COPYRIGHT([
Copyright (C) Copyright 2001-2008  William S. Yerazunis
])



AC_ARG_VAR(ORIGINAL_BILLY_DISTRO_DIR,
[Points at a directory where the 'vanilla CRM114' source code resides;
this is the source code as available from Bill Yerazunis (a.k.a. BillY)
at crm114.sourceforge.net.
Generally, you don't want to bother with item at all, unless you wish
to generate distributions including diffs with the 'original'.])

if test -z "$ORIGINAL_BILLY_DISTRO_DIR" ; then
  ORIGINAL_BILLY_DISTRO_DIR="/home/ger/prj/1original/crm114/src/crm114.sourceforge.net/src"
fi
AC_DEFINE_UNQUOTED(ORIGINAL_BILLY_DISTRO_DIR, ["$ORIGINAL_BILLY_DISTRO_DIR"],
  [directory where BillY's original crm114 distro resides])
AC_SUBST(ORIGINAL_BILLY_DISTRO_DIR)










# ##################################################################
#
# include/exclude specific sections of CRM114 (e.g. experimental classifiers)
#


AX_ARG_ENABLE([experimental-classifiers],
  [CRM_PRODUCTION_CLASSIFIERS_ONLY],
  [any experimental classifiers],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use these experimental classifiers.
(default: experimental classifiers are ENABLED)])



AX_ARG_ENABLE([OSB-Bayes],
  [CRM_WITHOUT_OSB_BAYES],
  [the OSB-Bayes classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: OSB-Bayes classifier is build-in)])

AX_ARG_ENABLE([OSB-Winnow],
  [CRM_WITHOUT_OSB_WINNOW],
  [the OSB-Winnow classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: OSB-Winnow classifier is build-in)])

AX_ARG_ENABLE([OSBF],
  [CRM_WITHOUT_OSBF],
  [the OSBF classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: OSBF classifier is build-in)])

AX_ARG_ENABLE([OSB-Hyperspace],
  [CRM_WITHOUT_OSB_HYPERSPACE],
  [the OSB-Hyperspace classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: OSB-Hyperspace classifier is build-in)])

AX_ARG_ENABLE([Markov],
  [CRM_WITHOUT_MARKOV],
  [the Markov classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: Markov classifier is build-in)])

AX_ARG_ENABLE([Correlate],
  [CRM_WITHOUT_CORRELATE],
  [the Correlate classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: Correlate classifier is build-in)])

AX_ARG_ENABLE([Bit-Entropy],
  [CRM_WITHOUT_BIT_ENTROPY],
  [the Bit-Entropy classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: Bit-Entropy classifier is build-in)])

AX_ARG_ENABLE([SVM],
  [CRM_WITHOUT_SVM],
  [the SVM classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: SVM classifier is build-in)])

AX_ARG_ENABLE([SKS],
  [CRM_WITHOUT_SKS],
  [the SKS classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: SKS classifier is build-in)])

AX_ARG_ENABLE([FSCM],
  [CRM_WITHOUT_FSCM],
  [the FSCM classifier],
[This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: FSCM classifier is build-in)])

AX_ARG_ENABLE([Neural-Net],
  [CRM_WITHOUT_NEURAL_NET],
  [the Neural-Net classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: Neural-Net classifier is build-in)])

AX_ARG_ENABLE([CLUMP],
  [CRM_WITHOUT_CLUMP],
  [the CLUMP classifier],
  [This will produce a smaller executable, which will report an error
when your scripts attempt to use this classifier.
(default: CLUMP classifier is build-in)])




###################################################################

AX_ARG_ENABLE([analysis],
  [CRM_WITHOUT_BMP_ASSISTED_ANALYSIS],
  [BMP-assited analysis],
  [Enable this to create an 'instrumented' CRM114 binary which allows you to
perform several types of analysis on the script code execution, but most importantly
to have closer look at the performance and characteristics (behaviour) of the
selected feature hash method, the CSS feature storage and the classifiers themselves.
Multiple crm runs are automatically merged into a single 'profile collection'
when you so desire; use the included 'crm_bmp_prof' tool to process the collected
analysis data in various ways.

Disable this when you want to churn out the maximum performance out of CRM114.
(default: ENABLED)])





# ##################################################################
#
# Checks for programs.
#

AC_PROG_CC([gcc egcs cl lcc cc])
AC_PROG_CXX
AC_PROG_CC_STDC   # check for ISO/C99 options for the compilers, if available. Be hip, don't be square.
AC_USE_SYSTEM_EXTENSIONS
# AC_GNU_SOURCE -- obsolete
# AC_AIX -- obsolete
# AC_MINIX -- obsolete
# AC_C_PROTOTYPES -- obsoleted

AM_MAINTAINER_MODE

# check if and where emacs resides on this system:
AM_PATH_LISPDIR


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
AC_PATH_PROG([CVS2CL], [cvs2cl])
AC_PATH_PROG([UNCRUSTIFY], [uncrustify])
AC_PATH_PROG([DIFF], [diff], [])
AC_PATH_PROG([SEVEN_ZIP], [7z], [])
AC_PATH_PROG([LZMA], [lzma], [])
AC_PATH_PROG([ZIP], [zip], [])
AC_PATH_PROG([TAR], [tar], [])
AC_PATH_PROG([COMPRESS], [compress], [])
AC_PATH_PROG([SHAR], [shar], [])
AC_PATH_PROG([HTMLTIDY], [tidy], [])
AC_PATH_PROG([DB2X_XSLTPROC], [db2x_xsltproc], [])
AC_PATH_PROG([DB2X_MANXML], [db2x_manxml], [])
AC_PATH_PROG([DB2X_TEXIXML], [db2x_texixml], [])
AC_PATH_PROG([GZIP], [gzip], [])
AC_PATH_PROG([BZIP2], [bzip2], [])
AC_PATH_PROG([RMAN], [rman], [])
AX_PROG_GNU_TIME()

## Zoem is a an interpretive macro language, for creating mark-up languages, by
## Stijn van Dongen. Information about zoem can be found on the zoem webpage at
## http://micans.org/zoem/ .
## We need zoem >= 04-027
AC_PATH_PROG([ZOEM], [zoem], [])
AC_PATH_PROG([TROFF], [groff], [])
AC_PATH_PROG([COL], [col], [])




AX_CHECK_DIFF_OPTION([-E])
AX_CHECK_DIFF_OPTION([-b])
AX_CHECK_DIFF_OPTION([-w])
AX_CHECK_DIFF_OPTION([-B], [DIFF_FLAGS])
AX_CHECK_DIFF_OPTION([-d], [DIFF_FLAGS])
AX_CHECK_DIFF_OPTION([--strip-trailing-cr], [DIFF_FLAGS])



AM_CONDITIONAL([OS_HAS_DIFF], [test -n "$DIFF"])
AM_CONDITIONAL([OS_HAS_SEVEN_ZIP], [test -n "$SEVEN_ZIP"])
AM_CONDITIONAL([OS_HAS_LZMA], [test -n "$LZMA"])
AM_CONDITIONAL([OS_HAS_ZIP], [test -n "$ZIP"])
AM_CONDITIONAL([OS_HAS_TAR], [test -n "$TAR"])
AM_CONDITIONAL([OS_HAS_COMPRESS], [test -n "$COMPRESS"])
AM_CONDITIONAL([OS_HAS_SHAR], [test -n "$SHAR"])
AM_CONDITIONAL([OS_HAS_RMAN], [test -n "$RMAN"])
AM_CONDITIONAL([OS_HAS_HTMLTIDY], [test -n "$HTMLTIDY"])
AM_CONDITIONAL([OS_HAS_DB2X_XSLTPROC], [test -n "$DB2X_XSLTPROC"])
AM_CONDITIONAL([OS_HAS_DB2X_MANXML], [test -n "$DB2X_MANXML"])
AM_CONDITIONAL([OS_HAS_DB2X_TEXIXML], [test -n "$DB2X_TEXIXML"])
AM_CONDITIONAL([OS_HAS_GZIP], [test -n "$GZIP"])
AM_CONDITIONAL([OS_HAS_BZIP2], [test -n "$BZIP2"])
AM_CONDITIONAL([OS_HAS_GNU_TIME], [test -n "$GNU_TIME"])




AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(HOSTTYPE, "$host_os", [Set host type])







# ##################################################################
#
# any special build features?
#


#
# inline QSORT for extra speed?
#
AC_MSG_CHECKING([if we want to use the custom inline qsort by Michael Tokarev])
AC_ARG_ENABLE([inline-qsort],
  AC_HELP_STRING([--disable-inline-qsort],
    [do not use the custom inline qsort by Michael Tokarev but use
the slower qsort() C standard library routine instead.
(default: qsort is inlined)]),
  [
   AC_MSG_RESULT([$enable_inline_qsort])
   if eval "test x$enable_inline_qsort = xno"; then
     AC_DEFINE_UNQUOTED(CRM_WITHOUT_MJT_INLINED_QSORT, 1, [do not use the custom inline qsort by Michael Tokarev])
   fi
  ],
  [
   AC_MSG_RESULT([yes])
  ])




#
# no multithreading support for now; that's for when this thing can cough up a libcrm114 of sorts
#
WITH_THREADS=no


#
# all (script) debug features are available by default
#
WITH_DEBUG_FEATURES="full (script debugging)"



#
# debug or release build of the binary?
#
AC_MSG_CHECKING([if we want a debug or release build])
AC_ARG_ENABLE([debug], AC_HELP_STRING([--disable-debug],
  [Specify to create a so-called 'production' or 'release build'.
You can explicitly specify '--enable-debug' to create a standard
CRM (debuggable) build. (default: debugging turned OFF)]),
  [
   #
   # since AC_PROG_CC sets '-O2' in CFLAGS by default, while _we_'d like
   # to have '-O3', we override the operation (for gcc only) right here.
   # Of course, this is rather hackish, so if anyone knows about a cleaner
   # solution, I'm all for it!
   #                                               Ger Hobbelt
   if test ! -z "$SED"; then
     CFLAGS=`echo " $CFLAGS " | $SED -e 's:-O[[0-3]] : :' -e 's:-g : :' -e 's:  : :g'`
     CXXFLAGS=`echo " $CXXFLAGS " | $SED -e 's:-O[[0-3]] : :' -e 's:-g : :' -e 's:  : :g'`
   fi
   if eval "test x$enable_debug = xyes"; then
     AC_MSG_RESULT([debug])
     AX_CFLAGS_GCC_OPTION([-O0])
     AX_CFLAGS_GCC_OPTION([-g3],
       [],
       [],
       [AX_CFLAGS_GCC_OPTION([-g])]) # symbolic debugging
     AX_CXXFLAGS_GCC_OPTION([-O0])
     AX_CXXFLAGS_GCC_OPTION([-g3],
       [],
       [],
       [AX_CXXFLAGS_GCC_OPTION([-g])]) # symbolic debugging
     WITH_BUILD="debug"
     WITH_DEBUGGING=yes
   else
     AC_MSG_RESULT([release])
     AX_CFLAGS_GCC_OPTION([-O3],
       [],
       [],
       [AX_CFLAGS_GCC_OPTION([-O2])])
     AX_CXXFLAGS_GCC_OPTION([-O3],
       [],
       [],
       [AX_CXXFLAGS_GCC_OPTION([-O2])])
     WITH_BUILD="release (maximum optimizations)"
     WITH_DEBUGGING=no
   fi
  ],
  [
   AC_MSG_RESULT([default])
   if test ! -z "$SED"; then
     if test -z `echo " $CFLAGS " | $SED -e 's:.* \(-O[[0-3]]\) .*:\1:'`; then
       # only add '-O2' if there's no '-O expression custom defined yet (by the user before [s]he started ./configure)
       AX_CFLAGS_GCC_OPTION([-O2])
     fi
     if test -z `echo " $CXXFLAGS " | $SED -e 's:.* \(-O[[0-3]]\) .*:\1:'`; then
       # only add '-O2' if there's no '-O expression custom defined yet (by the user before [s]he started ./configure)
       AX_CXXFLAGS_GCC_OPTION([-O2])
     fi
   fi
   WITH_BUILD="default"
   WITH_DEBUGGING=no
  ])



#
# include run-time profiling support (requires the use of specific development tools: prof/gprof)
#
WITH_GPROFILING="no"
WITH_PROFILING="no"

AC_MSG_CHECKING([if we want the instrument the build for profiling])
AC_ARG_ENABLE([profiling], AC_HELP_STRING([--enable-profiling],
  [You can explicitly specify '--enable-profiling' to 
create a CRM build which can be profiled with, for 
example, gprof. (default: profiling turned OFF)]),
  [
   AC_MSG_RESULT($enable_profiling)
   ac_profiling_cc_flags=""
   AX_CFLAGS_GCC_OPTION([-pg], [ac_profiling_cc_flags],
    	[
         WITH_GPROFILING="yes"
         AC_MSG_RESULT([$enable_profiling (gprof)])
        ],
   	[
   	 AX_CFLAGS_GCC_OPTION([-p], [ac_profiling_cc_flags],
   		[
   		WITH_PROFILING="yes"
   		AC_MSG_RESULT([$enable_profiling (prof)])
   		])
       ])
   AX_CXXFLAGS_GCC_OPTION([-pg], [ac_profiling_cc_flags],
    	[
         WITH_GPROFILING="yes"
         AC_MSG_RESULT([$enable_profiling (gprof)])
        ],
   	[
   	 AX_CXXFLAGS_GCC_OPTION([-p], [ac_profiling_cc_flags],
   		[
   		WITH_PROFILING="yes"
   		AC_MSG_RESULT([$enable_profiling (prof)])
   		])
       ])
   # delay copying into CFLAGS/CXXFLAGS/LDFLAGS; we don't want to influence the cc used to test stuff in this configure script
  ],
  [AC_MSG_RESULT([no])])

#
# maximum warning level for neurotic developers?
#
AC_MSG_CHECKING([if we want maximum compiler warnings])
AC_ARG_ENABLE([extended-compile-warnings],
  AC_HELP_STRING([--disable-extended-compile-warnings],
    [turn maximum compiler/linker warnings level off
(default: turn on all warnings)]),
  [
   AC_MSG_RESULT($enable_extended_compile_warnings)
   if eval "test x$enable_extended_compile_warnings = xyes"; then
     AX_CFLAGS_WARN_ALL([])
     AX_CXXFLAGS_WARN_ALL([])
   fi
  ],
  [
   AC_MSG_RESULT([yes])
   AX_CFLAGS_WARN_ALL([])
   AX_CXXFLAGS_WARN_ALL([])
  ])

#
# Do you want those CRM_ASSERT() calls active in your run-time or removed by the compiler?
#
AC_MSG_CHECKING([if we want run-time assertions])
AC_ARG_ENABLE([assertions],
  AC_HELP_STRING([--disable-assertions],
    [turn assertion checks off.
This will produce a faster running executable but such at the cost
of lacking a specific type of 'developer' checks. These SHOULD not
get triggered EVER, so you MAY assume specifying this option is
a 'free speedup'. (default: assertions are ENABLED)]),
  [
   AC_MSG_RESULT($enable_assertions)
   if eval "test x$enable_assertions = xno"; then
     AC_DEFINE_UNQUOTED(CRM_DONT_ASSERT, 1, [DISable ASSERT/VERIFY checks])
   fi
  ],
  [
   AC_MSG_RESULT([yes])
  ])

#
# Do you want those CRM_ASSERT() calls to be trappable by CRM script or just fail dramtically at run-time
# every time?
#
AC_MSG_CHECKING([if we want run-time trappable assertions])
AC_ARG_ENABLE([trappable-asserts],
  AC_HELP_STRING([--enable-trappable-assertions],
    [make assertion checks trappable (instead of being a
untrappable error report). This means any triggered assertion 
checks will show up in your CRM trap handlers, but BEWARE: under
specific (rather theoretical) circumstances you may now be greeted
by a coredump due to an inifinite loop caused by assertion failures
within the CRM114 trap handler. Can't say you haven't been warned!
(default: assertions are 'untrappable')]),
  [
   AC_MSG_RESULT($enable_trappable_assertions)
   if eval "test x$enable_trappable_assertions = xyes"; then
     AC_DEFINE_UNQUOTED(CRM_ASSERT_IS_UNTRAPPABLE, 0, [TRAPPABLE(!) ASSERT/VERIFY checks - (this is dangerous!)])
   else
     AC_DEFINE_UNQUOTED(CRM_ASSERT_IS_UNTRAPPABLE, 1, [untrappable ASSERT/VERIFY checks])
   fi
  ],
  [
   AC_MSG_RESULT([no])
   AC_DEFINE_UNQUOTED(CRM_ASSERT_IS_UNTRAPPABLE, 1, [untrappable ASSERT/VERIFY checks])
  ])





AC_SYS_LONG_FILE_NAMES

# Checks for header files.

# this test might fail when runtime is broken, e.g. due to ld which
# fails to find proper libtre
AC_HEADER_STDC

AC_HEADER_MAJOR
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
# AC_HEADER_ASSERT
AC_HEADER_STAT
AC_CHECK_HEADERS(fcntl.h float.h stdlib.h stddef.h string.h unistd.h)
AC_CHECK_HEADERS(stdio.h ctype.h errno.h sys/types.h sys/wait.h)
AC_CHECK_HEADERS(dirent.h unistd.h sys/mman.h sys/stat.h math.h)
AC_CHECK_HEADERS(locale.h sys/times.h signal.h time.h sys/time.h)
AC_CHECK_HEADERS(inttypes.h arpa/inet.h netinet/in.h)
AC_CHECK_HEADERS(stdarg.h varargs.h float.h limits.h)
AC_CHECK_HEADERS(libintl.h sys/utime.h utime.h wchar.h)
AC_CHECK_HEADERS(crtdbg.h signal.h io.h direct.h process.h lmcons.h)
AC_CHECK_HEADERS(getopt.h getopt_ex.h)
AC_CHECK_HEADERS(endian.h sys/param.h)
AC_CHECK_HEADERS(pwd.h)
AC_CHECK_HEADERS(pshpack1.h pshpack2.h pshpack4.h poppack.h)


# see: http://www.gnu-pascal.de/crystal/gpc/en/mail11031.html
AC_CHECK_HEADERS(crt_externs.h)


# Checks for typedefs, structures, and compiler characteristics.
AX_C_PREDEF___FUNCTION__
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_BIGENDIAN(
  [AC_DEFINE([MACHINE_IS_BIG_ENDIAN], 1,
	[Define to 1 if your processor stores words with the most significant
	 byte first (like Motorola and SPARC, unlike Intel and VAX).])],
  [AC_DEFINE([MACHINE_IS_LITTLE_ENDIAN], 1,
	[Define to 1 if your processor stores words with the least significant
	 byte first (like Intel and VAX).])]
  )
AC_C_CHAR_UNSIGNED
AX_CC_PRAGMA_PACK
AC_TYPE_LONG_LONG_INT
AC_C_STRINGIZE
AC_CHECK_SIZEOF([long int])
AC_CHECK_SIZEOF([int])
if test $ac_cv_type_long_long_int = yes; then
  AC_CHECK_SIZEOF([long long int])
fi
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_OFF_T
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([[clock_t], [pid_t], [mode_t], [struct stat], [struct timespec], [struct timeval], [struct tms]],
 [], [], [
AC_INCLUDES_DEFAULT[

/* extra includes for these */
@%:@ifdef HAVE_SYS_TIMES_H
@%:@include <sys/times.h>
@%:@endif
@%:@if defined (TIME_WITH_SYS_TIME)
@%:@include <sys/time.h>
@%:@include <time.h>
@%:@else
@%:@if defined (HAVE_SYS_TIME_H)
@%:@include <sys/time.h>
@%:@elif defined (HAVE_TIME_H)
@%:@include <time.h>
@%:@endif
@%:@endif
@%:@ifdef HAVE_PROCESS_H
@%:@include <process.h>
@%:@endif
]])
AX_NSEC_STAT_FIELDS





# Check for library functions
AC_SEARCH_LIBS([sqrt], [m])
AC_SEARCH_LIBS([clock_gettime], [rt])




AC_MSG_CHECKING([wether your runtime environment is ok])
# Check to be sure not to get some AC_FUNC_ 's fail miserably.
AC_RUN_IFELSE([AC_LANG_PROGRAM([], [exit (0);])],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_FAILURE([It seems your runtime environment is broken.  Does your TRE installation need an explicitly set LD_LIBRARY_PATH?])]
)



dnl *************************
dnl regex libraries tests
dnl *************************
AC_ARG_WITH([regex-includes],
  AC_HELP_STRING([--with-regex-includes=DIR], [where you have regex includes]),
  WITH_REGEX_INC="$withval")
if test -d "$WITH_REGEX_INC"; then
  CFLAGS="$CFLAGS -I$WITH_REGEX_INC"
  CXXFLAGS="$CXXFLAGS -I$WITH_REGEX_INC"
fi

AC_ARG_WITH([regex-libraries],
  AC_HELP_STRING([--with-regex-libraries=DIR], [where you have regex libraries]),
  WITH_REGEX_LIB="$withval")
if test -d "$WITH_REGEX_LIB"; then
  LDFLAGS="$LDFLAGS -L$WITH_REGEX_LIB"
fi

dnl ********************************
dnl all the various REGEX tests
dnl ********************************
AC_ARG_WITH([regex], 
  AC_HELP_STRING([[--with-regex=[none/auto/posix/gnu/v8/bsd/pcre/lregex/tre]]],
                 [select type of regular expressions you want to use]),
  [WITH_REGEX="$withval"],
  [WITH_REGEX="auto"])

WITH_SELECTED_REGEX=""

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
    AC_CHECK_HEADERS(tre/regex.h)
    AC_CHECK_PROG(TRE_CONFIG, tre-config, "yes", "no")
    if eval "test x$TRE_CONFIG = xyes"; then
      tre_cflags=`tre-config --cflags`
      tre_libs=`tre-config --libs`
      x_cflags="$x_cflags $tre_cflags"
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_TRE_REGEX, 1, [TRE REs])
      LIBS="$LIBS $tre_libs"
      want_regex_impl=no
      WITH_SELECTED_REGEX=TRE
    else
      AC_SEARCH_LIBS([tre_compile], [tre], [
        AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
        AC_DEFINE_UNQUOTED(HAVE_TRE_REGEX, 1, [TRE REs])
        want_regex_impl="no"
        WITH_SELECTED_REGEX=TRE
        ])
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
      LIBS="$LIBS -lregex"
      want_regex_impl="no"
      WITH_SELECTED_REGEX="POSIX (libregex)"
      ])
  fi
fi


dnl ********************
dnl test for POSIX regex
dnl ********************

if eval "test x$want_regex_impl = xyes"; then
  if eval "test x$WITH_REGEX = xauto -o x$WITH_REGEX = xposix"; then
    AC_MSG_CHECKING([for POSIX regcomp()])
    AC_CHECK_HEADERS(regex.h)
    ac_pregcomp="no"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    @%:@include <sys/types.h>
    @%:@include <regex.h>
    ]], [[
      regex_t pr;
      regcomp(&pr , "" , 0);
    ]])],[ac_pregcomp="yes"],[])
    AC_MSG_RESULT($ac_pregcomp)
    if eval "test x$ac_pregcomp = xyes"; then
      AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
      AC_DEFINE_UNQUOTED(HAVE_POSIX_REGEX, 1, [POSIX REs])
      want_regex_impl=no
      WITH_SELECTED_REGEX=POSIX
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
      WITH_SELECTED_REGEX=PCRE
    else
      AC_CHECK_LIB(pcre, pcre_compile, [
        AC_DEFINE_UNQUOTED(HAVE_REGEX, 1, [REs support])
        AC_DEFINE_UNQUOTED(HAVE_PCRE_REGEX, 1,
          [PCRE REs])
        LIBS="$LIBS -lpcre"
        want_regex_impl=no
        WITH_SELECTED_REGEX=PCRE
        ])
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
      WITH_SELECTED_REGEX=GNU
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
      WITH_SELECTED_REGEX="Bell V8"
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
      WITH_SELECTED_REGEX=BSD
    fi
  fi
fi







# check for a environ/__environ global var
AX_ENVIRON

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
AX_FUNC_SNPRINTF
AX_FUNC_MEMMOVE
AX_FUNC_MEMMEM
VL_LIB_READLINE
AC_CHECK_FUNCS(dup2 bcopy)
AC_CHECK_FUNCS(memchr memset)
AC_CHECK_FUNCS(munmap msync)
AC_CHECK_FUNCS(strcasecmp strncasecmp strcspn strdup strstr strchr strnchr)
AC_CHECK_FUNCS(strnicmp stricmp)
AC_CHECK_FUNCS(strncpy _snprintf _vsnprintf)
AC_CHECK_FUNCS(strmov)
AC_CHECK_FUNCS(times)
AC_CHECK_FUNCS(stat _stat utime _utime)
AC_CHECK_FUNCS(strerror)
AC_CHECK_FUNCS(getpid getppid waitpid system)
AC_CHECK_FUNCS(truncate)
AC_CHECK_FUNCS(pipe)
AC_CHECK_FUNCS(floor fabs pow log logl log10 sqrt log2 isnan _isnan)
AC_CHECK_FUNCS(isalnum isalpha isascii isblank iscntrl isdigit isgraph islower isprint ispunct isspace isupper isxdigit)
AC_CHECK_FUNCS(setlocale)
AC_CHECK_FUNCS(getopt getopt_long getopt_long_only)
AC_CHECK_FUNCS(getopt_ex getopt_long_ex getopt_long_only_ex)
AC_CHECK_FUNCS(__debugbreak _set_output_format _setmode _fileno _set_errno)
AC_CHECK_FUNCS(sysconf)
AC_CHECK_FUNCS(madvise posix_madvise)
AC_CHECK_FUNCS(getcwd GetFullPathNameA)
AC_CHECK_FUNCS(getuid getpwuid getpwuid_r GetUserNameA)
AC_CHECK_FUNCS(QueryPerformanceFrequency QueryPerformanceCounter clock_gettime clock_getres clock gettimeofday)




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


# AC_DEFINE_UNQUOTED(SILENT, "@", [silent mode for make])
SILENT="@"
AC_SUBST(SILENT)

# setting default install flags: install rights = -rwxr-xr-x
INSTALLFLAGS="-m 755"



AC_SUBST(INSTALLFLAGS)
AC_SUBST(DIFF_FLAGS)

# see 
#   http://www.gnu.org/software/libtool/manual/automake/Rebuilding.html
# for the why: I want any change in those .in files to cause an automatic
# update, e.g. when running
#   make dist
#
AC_SUBST([CONFIG_STATUS_DEPENDENCIES], ['$(top_srcdir)/src/config_vanilla_UNIX_sys_defaults.h.in $(top_srcdir)/win32/config_win32.h.in'])



# write delayed options into CFLAGS/LDFLAGS:
CFLAGS="$CFLAGS $ac_profiling_cc_flags"
CXXFLAGS="$CXXFLAGS $ac_profiling_cc_flags"
LDFLAGS="$LDFLAGS $ac_profiling_cc_flags"



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
                 src/config_vanilla_UNIX_sys_defaults.h
                 tests/Makefile
                 tests/megatest.sh
                 tests/megatest_ng.sh
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
                 tests/exectimingtest1.crm
                 tests/exectimingtest2.crm
                 tests/fataltraptest.crm
                 tests/fataltrapprogramerr.crm
                 tests/inserttest_a.crm
                 tests/inserttest_b.crm
                 tests/inserttest_c.crm
                 tests/learntest.crm
                 tests/match_isolate_test.crm
                 tests/isolatephenomenontest.crm
                 tests/matchtest.crm
                 tests/mathalgtest.crm
                 tests/mathrpntest.crm
                 tests/nestaliustest.crm
                 tests/overalterisolatedtest.crm
                 tests/paolo_overvars.crm
                 tests/paolo_ov2.crm
                 tests/paolo_ov3.crm
                 tests/paolo_ov4.crm
                 tests/paolo_ov5.crm
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
                 tests/blowuptrapbugtest.crm
                 tests/indirecttest.crm
                 tests/exec_async_leaktest1.crm
                 tests/isolate_reclaim_test.crm
                 tests/match_isolate_reclaim.crm
                 tests/slashbugtest.crm
                 tests/trapseqbugtest.crm
                 tests/translate_tr.crm
                 tests/call_return_test.crm
                 tests/alternating_example_neural.crm
                 tests/alius_w_comment.crm
                 tests/zz_translate_test.crm
                 tests/quine.crm
                 tests/defaulttest.crm
                 tests/print_binary2decimal_int32.crm
                 tests/match_re_fringe1.crm
                 tests/legal_eval_to_null_var1.crm
                 tests/legal_show_null_var1.crm
                 tests/illegal_stmt1.crm
                 tests/illegal_stmt2.crm
                 tests/illegal_alter_null_var1.crm
                 tests/illegal_isolate_null_var1.crm
                 tests/illegal_isolate_null_var2.crm
                 tests/illegal_isolate_var1.crm
                 tests/illegal_isolate_var2.crm
                 tests/illegal_CSS_files_w_decent_error_message2.crm
                 tests/illegal_empty_alter2.crm
                 tests/illegal_alter_var1.crm
                 tests/illegal_CSS_files_w_decent_error_message3.crm
                 tests/illegal_eval_var1.crm
                 tests/illegal_eval_var2.crm
                 tests/illegal_empty_isolate1.crm
                 tests/illegal_CSS_files_w_decent_error_message1.crm
                 tests/illegal_isolate_var3.crm
                 tests/illegal_empty_alter1.crm
                 tests/illegal_empty_isolate2.crm
                 tests/illegal_indirect_isolate1.crm
                 tests/legal_indirect_isolate1.crm
                 tests/legal_indirect_alter1.crm
                 tests/legal_eval_var1.crm
                 tests/testscript.sh
                 tests/crm114_statustest.filter.sh
                 tests/crm114_error_report.filter.sh
                 tests/crm114_versiontest.filter.sh
                 tests/crm114_classify.filter.sh
                 tests/crm114_isolate_test.filter.sh
                 examples/pad.crm
                 examples/shroud.crm
                 mailfilter/classifymail.crm
                 mailfilter/mailfilter.crm
                 mailfilter/mailfilter.cf
                 mailfilter/maillib.crm
                 mailfilter/mailreaver.crm
                 mailfilter/mailtrainer.crm
                 examples/rewriteutil.crm
                 win32/config_win32.h
],
[
	#
	# and when we finally have produced the files, make sure the generated shell scripts are executable:
	#
	chmod a+x src/*.sh          2> /dev/null
	chmod a+x tests/*.sh        2> /dev/null
	chmod a+x tests/*.crm       2> /dev/null
	chmod a+x examples/*.crm    2> /dev/null
	chmod a+x mailfilter/*.crm  2> /dev/null
])

AC_OUTPUT



dnl Print configuration summary

my_installpath=`eval echo $exec_prefix`

cat <<EOF

Configuration summary
=====================

CRM114 is now configured as follows:

* Compilation environment

  CC           = $CC
  CFLAGS       = $CFLAGS
  CPP          = $CPP
  CPPFLAGS     = $CPPFLAGS
  CXX          = $CXX
  CXXFLAGS     = $CXXFLAGS
  LD           = $LD
  LDFLAGS      = $LDFLAGS
  LIBS         = $LIBS
  INSTALLFLAGS = $INSTALLFLAGS
  BINDIR       = $BIN_PATH

* CRM114 options

  Development-time debugging:             $WITH_DEBUGGING
  Allow profiling using gprof:            $WITH_GPROFILING
  Allow profiling using prof:             $WITH_PROFILING
  Include debug features:                 $WITH_DEBUG_FEATURES
  Build:                                  $WITH_BUILD

  multithreading support:                 $WITH_THREADS

  regular expressions library in use:     $WITH_REGEX ($WITH_SELECTED_REGEX)

-----------------------------------------------------------------------

Congratulations, your $PACKAGE_NAME $PACKAGE_VERSION is configured now.
You can now build and install $PACKAGE_NAME using the Makefile, by calling

  \$ make

Before running 

  \$ make install

you may wish to test the newly build binary by running

  \$ make check

which is an extended version or Bill Yerazunis' megatest. Any unacceptable
mistakes will be reported and cause the 'check' run to abort.

EOF

