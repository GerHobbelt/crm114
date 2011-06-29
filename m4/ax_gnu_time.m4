#
# SYNOPSIS
#
#   AX_PROG_GNU_TIME()
#
# DESCRIPTION
#
#   Checks for a GNU time version in $PATH, i.e. a 'time' which
#   support the '-v' (verbose) commandline switch.
#
#   When none is available, a warning message is displayed and
#   the search continues, now for a regular 'time'.
#   If, again, none is found, it is assumed 'time' is a shell
#   built-in command.
#
#   When a GNU time is found, the 'GNU_TIME' variable is set
#   to point to the absolute path.
#
#   At all times, the 'TIME' variable is also set, either to point
#   at the absolute path of GNU time, if one was found, or the
#   regular 'time' or shell built-in.
#
# LAST MODIFICATION
#
#   2008-06-04
#
# COPYLEFT
#
#   Copyright (c) 2008 Ger Hobbelt <ger@hobbelt.com>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#   02111-1307, USA.
#
#   As a special exception, the respective Autoconf Macro's copyright
#   owner gives unlimited permission to copy, distribute and modify the
#   configure scripts that are the output of Autoconf when processing
#   the Macro. You need not follow the terms of the GNU General Public
#   License when using or distributing such scripts, even though
#   portions of the text of the Macro appear in them. The GNU General
#   Public License (GPL) does govern all other use of the material that
#   constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the
#   Autoconf Macro released by the Autoconf Macro Archive. When you
#   make and distribute a modified version of the Autoconf Macro, you
#   may extend this special exception to the GPL to apply to your
#   modified version as well.


AC_DEFUN([AX_PROG_GNU_TIME],
[
  AC_ARG_VAR([GNU_TIME], [Location of GNU time.  Defaults to the first
    'time' program on PATH which supports the '-v' commandline switch.])
  AC_CACHE_CHECK([for GNU time that supports '-v'], [ac_cv_path_GNU_TIME],
  [
    AC_PATH_PROGS_FEATURE_CHECK([GNU_TIME], [time],
    [
      `${ac_path_GNU_TIME} -v echo "hello" >/dev/null 2>&1` \
        && `${ac_path_GNU_TIME} --version >/dev/null 2>&1` \
        && ac_cv_path_GNU_TIME=${ac_path_GNU_TIME} ac_path_GNU_TIME_found=:
    ],
    [
      AC_MSG_WARN([no acceptable time could be found in \$PATH.
'GNU time' is recommended])
    ])
  ])
  if test -z "${ac_cv_path_GNU_TIME}"; then
    AC_PATH_PROGS_FEATURE_CHECK([TIME], [time],
    [
      `${ac_path_TIME echo} "hello" >/dev/null 2>&1` \
        && ac_cv_path_TIME=${ac_path_TIME} ac_path_TIME_found=:
    ],
    [
      ac_cv_path_TIME=time
    ])
    AC_SUBST([TIME], [$ac_cv_path_TIME])
  else
    AC_SUBST([TIME], [$ac_cv_path_GNU_TIME])
    AC_SUBST([GNU_TIME], [$ac_cv_path_GNU_TIME])
  fi
])



