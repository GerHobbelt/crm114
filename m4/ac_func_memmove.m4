##### http://autoconf-archive.cryp.to/ac_func_memmove.html
#
# SYNOPSIS
#
#   AX_FUNC_MEMMOVE
#
# DESCRIPTION
#
#   Checks for a memmove that can handle overlaps correctly. If no
#   working memmove is found, request a replacement and warn the user
#   about it.
#
# LAST MODIFICATION
#
#   2002-09-26
#
# COPYLEFT
#
#   Copyright (c) 2002 R�diger Kuhlmann <info@ruediger-kuhlmann.de>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_FUNC_MEMMOVE],
[
  AC_CHECK_FUNCS([memmove])
  AC_MSG_CHECKING([for working memmove])
  AC_CACHE_VAL([ac_cv_have_working_memmove],
  [
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT([])[

@%:@include <stdio.h>
@%:@include <stdlib.h>
@%:@include <string.h>

]],[[
    char buf[10];

    strcpy (buf, "01234567");
    memmove (buf, buf + 2, 3);
    if (strcmp (buf, "23434567"))
        exit(1);
    strcpy (buf, "01234567");
    memmove (buf + 2, buf, 3);
    if (strcmp (buf, "01012567"))
        exit(1);
    exit(0);
]]
    )],
    [ac_cv_have_working_memmove=yes],
    [ac_cv_have_working_memmove=no],
    [ac_cv_have_working_memmove=cross])
  ])
  AC_MSG_RESULT([$ac_cv_have_working_memmove])
  if test x$ac_cv_have_working_memmove != "xyes"; then
    AC_LIBOBJ([memmove])
    AC_MSG_WARN([Replacing missing/broken memmove.])
    AC_DEFINE(PREFER_PORTABLE_MEMMOVE, 1, [enable replacement memmove if system memmove is broken or missing])
  fi
])
