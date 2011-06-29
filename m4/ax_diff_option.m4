##### derived from http://autoconf-archive.cryp.to/ax_diff_flags_option.html
#
# SYNOPSIS
#
#   AX_DIFF_OPTION (optionflag [,[shellvar][,[A][,[NA]]])
#
# DESCRIPTION
#
#   AX_DIFF_OPTION(-E) would show a message as like
#   "checking -E option for diff ... yes" and adds the
#   optionflag to DIFF_FLAGS if it is understood. You can override the
#   shellvar-default of DIFF_FLAGS of course.
#
#     - $1 option-to-check-for : required ("-option" as non-value)
#     - $2 shell-variable-to-add-to : DIFF_FLAGS
#     - $3 action-if-found : add value to shellvariable
#     - $4 action-if-not-found : nothing
#
# LAST MODIFICATION
#
#   2007-08-05
#
# COPYLEFT
#
#   Copyright (c) 2007 Ger Hobbelt <ger@hobbelt.com>
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


AC_DEFUN([AX_DIFF_OPTION], [dnl
AS_VAR_PUSHDEF([FLAGS],[DIFF_FLAGS])dnl
AS_VAR_PUSHDEF([VAR],[ac_cv_diff_flag_[]$1])dnl
AC_CACHE_CHECK([option m4_ifval(-$1,-$1,[?]) for diff],
VAR,[VAR="no"
 ac_save_[]FLAGS="$[]FLAGS"
 FLAGS="$ac_save_[]FLAGS -m4_ifval($1,$1)"
 cat >conftest.in1 <<_ACEOF
XXX
_ACEOF
 cp conftest.in1 conftest.in2
 if test -z "X$diff" ; then
   ac_tool=diff
 else
   ac_tool=$diff
 fi
 $ac_tool $FLAGS conftest.in1 conftest.in2 2> conftest.er1
 ac_status=$?
 grep -v '^ *+' conftest.er1 > conftest.err
 rm -f conftest.er1
 rm -f conftest.in1
 rm -f conftest.in2
 cat conftest.err >&AS_MESSAGE_LOG_FD
 _AS_ECHO_LOG([\$? = $ac_status])
 if test $ac_status == 0 ; then
   VAR="yes"
 fi
 FLAGS="$ac_save_[]FLAGS"
 _AS_ECHO_LOG([status = $ac_status, ] VAR [ = $VAR])
 rm -f conftest.err
])
case ".$VAR" in
   .no*) m4_ifvaln($4,$4,[
         m4_ifval($1,[
           AC_RUN_LOG([: m4_ifval($2,$2,DIFF_FLAGS)=$m4_ifval($2,$2,DIFF_FLAGS) -- diff does NOT support option -$1])
         ])
         ]) ;;
   *) m4_ifvaln($3,$3,[
   if echo " $[]m4_ifval($2,$2,DIFF_FLAGS) " | grep " $VAR " 2>&1 >/dev/null
   then AC_RUN_LOG([: m4_ifval($2,$2,DIFF_FLAGS) does contain -$1])
   else AC_RUN_LOG([: m4_ifval($2,$2,DIFF_FLAGS)="$m4_ifval($2,$2,DIFF_FLAGS) -$1"])
                      m4_ifval($2,$2,DIFF_FLAGS)="$m4_ifval($2,$2,DIFF_FLAGS) -$1"
   fi ]) ;;
esac
AS_VAR_POPDEF([VAR])dnl
AS_VAR_POPDEF([FLAGS])dnl
])






