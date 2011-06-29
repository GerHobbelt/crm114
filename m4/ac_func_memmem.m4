#
# SYNOPSIS
#
#   AX_FUNC_MEMMEM
#
# DESCRIPTION
#
#   Checks for a memmem that works as defined in the latest standards
#   and has none of the caveats as mentioned in the Linux man pages, for instance.
#   If no working memmem is found, request a replacement and warn the user
#   about it.
#
# LAST MODIFICATION
#
#   2008-05-30
#
# COPYLEFT
#
#   Copyright (c) 2008 Ger Hobbelt <ger@hobbelt.com>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_FUNC_MEMMEM],
[AC_CHECK_FUNCS([memmem])
AC_MSG_CHECKING([for working memmem])
AC_CACHE_VAL([ac_cv_have_working_memmem],
[AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT([])],
[[
    char haystack[20];
    char needle[20];
    char *p;

    strcpy (haystack, "01234567345");
    strcpy (needle, "345");

    /* see if memmem() finds a needle as usual: */
    p = memmem(haystack, strlen(haystack) + 1, needle, strlen(needle));
    if (!p || strcmp(p, "34567345"))
        return 1;
    /* can find a match at start? */
    p = memmem(haystack + 3, strlen(haystack + 3) + 1, needle, strlen(needle));
    if (!p || strcmp(p, "34567345"))
        return 2;
    /* can find a match at end? */
    p = memmem(haystack, strlen(haystack) + 1, needle, strlen(needle) + 1);
    if (!p || strcmp(p, "345"))
        return 3;
    /* acts normal when needle length is zero: */
    p = memmem(haystack + 1, strlen(haystack) + 1, needle, 0);
    if (!p || strcmp(p, "1234567345"))
        return 4;
    /* acts normal when haystack length is zero: */
    p = memmem(haystack + 1, 0, needle, strlen(needle));
    if (p)
        return 5;
    /* acts normal when needle len > haystack len: */
    p = memmem(needle, strlen(needle), haystack, strlen(haystack));
    if (p)
        return 6;
    /* handles 8-bit input properly. */
	/* printf("needle[1] = %c, haystack[4] = %c, haystack[9] = %c\n", needle[1], haystack[4], haystack[9]); */
    needle[1] = 0;
    haystack[9] = 0;
    haystack[4] = 0;
    p = memmem(haystack, 12, needle, 3);
    if (!p || strcmp(p, "3"))
        return 7;
    return 0;
]])], 
  [ac_cv_have_working_memmem=yes], 
  [ac_cv_have_working_memmem=no], 
  [ac_cv_have_working_memmem=cross])])
AC_MSG_RESULT([$ac_cv_have_working_memmem])
if test x$ac_cv_have_working_memmem != "xyes"; then
  AC_LIBOBJ([memmem])
  AC_MSG_WARN([Replacing missing/broken memmem.])
  AC_DEFINE(PREFER_PORTABLE_MEMMEM, 1, [enable replacement memmem if system memmem is broken or missing])
fi])
