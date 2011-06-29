#
# SYNOPSIS
#
#   AX_CHECK_DIFF_OPTION(optionflag, [shellvar], [A], [NA])
#
# DESCRIPTION
#
#   AX_CHECK_DIFF_OPTION([-E]) would show a message as like
#   "checking -E option for diff ... yes" and executes the [A] section
#   if it is understood, otherwise the [NA] section will be executed.
#
#   When 'shellvar' is specified, the 'optionflag' will be appended to it.
#
#     - $1 option-to-check-for : required
#     - $2 shellvar : optional / DIFF_FLAGS
#     - $3 action-if-found : optional / nothing
#     - $4 action-if-not-found : optional / nothing
#
# LAST MODIFICATION
#
#   2008-05-25
#
# COPYLEFT
#
#   Copyright (c) 2007-2009 Ger Hobbelt <ger@hobbelt.com>
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


AC_DEFUN([AX_CHECK_DIFF_OPTION],
[
  AS_VAR_PUSHDEF([optvar], [ac_cv_diff_opt_[]$1])
  AC_CACHE_CHECK([option $1 for ${DIFF}], [optvar],
  [
    # create two identical files to diff;
    # we're only interested in the validity of the commandline option here:
    cat >conftest.in1 <<_ACEOF
      XXX
_ACEOF
    cp conftest.in1 conftest.in2
    AS_IF([$DIFF $1 conftest.in1 conftest.in2 2> conftest.er1],
      [AS_VAR_SET([optvar], [yes])],
      [AS_VAR_SET([optvar], [no])])
    rm -f conftest.er1
    rm -f conftest.in1
    rm -f conftest.in2
  ])
  AS_IF([test AS_VAR_GET([optvar]) = yes],
  [
    m4_ifval([$3], [$3],
    [ 
      m4_ifval([$2], [$2="$$2 $1"],
      [
        DIFF_FLAGS="${DIFF_FLAGS} $1"
      ])
    ])
  ],[
    m4_ifval([$4], [$4], [:])
  ])
  AS_VAR_POPDEF([optvar])
])


