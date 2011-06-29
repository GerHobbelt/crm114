##### http://autoconf-archive.cryp.to/ac_func_snprintf.html
#
# SYNOPSIS
#
#   AX_NSEC_STAT_FIELDS
#
# DESCRIPTION
#
#   Checks if 'struct stat' carries nanosecond ctime/mtime/atime
#   elements, and if it does, what those are named.
#   Each of these will set their corresponding HAVE_NSEC_STAT_xxx #define
#   to 1.
#
# LAST MODIFICATION
#
#   2007-08-15
#
# COPYLEFT
#
#   Copyright (c) 2007 Ger Hobbelt <ger@hobbelt.com>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_NSEC_STAT_FIELDS],
[AC_CHECK_TYPES([struct stat])
AC_MSG_CHECKING([for nanosecond 'struct stat' timestamps: mtimensec])
AC_CACHE_VAL(ac_cv_have_nsec_stat_mtimensec,
[AC_RUN_IFELSE(
  [AC_LANG_SOURCE([[#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

int main(void)
{
  struct stat st;
  st.st_ctimensec = 0;  
  st.st_mtimensec = 0;  
  st.st_atimensec = 0;  
  exit(0);
}]])], 
  [ac_cv_have_nsec_stat_mtimensec=yes], 
  [ac_cv_have_nsec_stat_mtimensec=no], 
  [ac_cv_have_nsec_stat_mtimensec=cross])])
AC_MSG_RESULT([$ac_cv_have_nsec_stat_mtimensec])
AC_MSG_CHECKING([for nanosecond 'struct stat' timestamps: mtime_nsec])
AC_CACHE_VAL(ac_cv_have_nsec_stat_mtime_nsec,
[AC_RUN_IFELSE(
  [AC_LANG_SOURCE([[#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

int main(void)
{
  struct stat st;
  st.st_ctime_nsec = 0;  
  st.st_mtime_nsec = 0;  
  st.st_atime_nsec = 0;  
  exit(0);
}]])], 
  [ac_cv_have_nsec_stat_mtime_nsec=yes], 
  [ac_cv_have_nsec_stat_mtime_nsec=no], 
  [ac_cv_have_nsec_stat_mtime_nsec=cross])])
AC_MSG_RESULT([$ac_cv_have_nsec_stat_mtime_nsec])
AC_MSG_CHECKING([for nanosecond 'struct stat' timestamps: mtim.tv_nsec])
AC_CACHE_VAL(ac_cv_have_nsec_stat_mtim_tv_nsec,
[AC_RUN_IFELSE(
  [AC_LANG_SOURCE([[#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(void)
{
  struct stat st;
  st.st_ctim.tv_nsec = 0;  
  st.st_mtim.tv_nsec = 0;  
  st.st_atim.tv_nsec = 0;  
  exit(0);
}]])], 
  [ac_cv_have_nsec_stat_mtim_tv_nsec=yes], 
  [ac_cv_have_nsec_stat_mtim_tv_nsec=no],
  [ac_cv_have_nsec_stat_mtim_tv_nsec=cross])])
AC_MSG_RESULT([$ac_cv_have_nsec_stat_mtim_tv_nsec])

if test x$ac_cv_have_nsec_stat_mtimensec == "xyes"; then
  AC_DEFINE(HAVE_NSEC_STAT_TIMENSEC, 1, [Define if run-time library offers nanosecond time interval in struct stat:c/m/atimensec.])
fi
if test x$ac_cv_have_nsec_stat_mtime_nsec == "xyes"; then
  AC_DEFINE(HAVE_NSEC_STAT_TIME_NSEC, 1, [Define if run-time library offers nanosecond time interval in struct stat:c/m/atime_nsec.])
fi
if test x$ac_cv_have_nsec_stat_mtim_tv_nsec == "xyes"; then
  AC_DEFINE(HAVE_NSEC_STAT_TIM_TV_NSEC, 1, [Define if run-time library offers nanosecond time interval in struct stat:c/m/atim.tv_nsec.])
fi
])

