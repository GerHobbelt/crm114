#
# SYNOPSIS
#
#   AX_ARG_ENABLE(optionflag, ......)
#
# DESCRIPTION
#
#   AX_ARG_ENABLE(opt) will check for the ./configure option --opt
#   and set the specified option accordingly.
#
#     - $1 commandline-option-to-check-for : required
#     - $2 shellvar : optional / nothing
#     - $3
#     - $4
#
# LAST MODIFICATION
#
#   2008-05-25
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


AC_DEFUN([AX_ARG_ENABLE],
[
  AS_VAR_PUSHDEF([opt], [enable_$1])
  AC_MSG_CHECKING([if we want $3])
  AC_ARG_ENABLE([$1],
    [AC_HELP_STRING([--disable-$1], [do not include $3. $4])],
    [
      AC_MSG_RESULT([AS_VAR_GET([opt])])
      AS_IF([test x[]AS_VAR_GET([opt]) = xno],
      [
        AC_DEFINE([$2], [1], [#define as 1: do NOT include $3 in the build.
#define as 0: explicitly request INclusion in the build.
#undef: assume INclusion in the build (i.e. use the default set in crm114_config.h)])
      ],
      [
        AC_DEFINE([$2], [0], [#define as 1: do NOT include $3 in the build.
#define as 0: explicitly request INclusion in the build.
#undef: assume INclusion in the build (i.e. use the default set in crm114_config.h)])
      ])
    ],
    [
      AC_MSG_RESULT([yes])
    ])
  AS_VAR_POPDEF([opt])
])


