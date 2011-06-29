/*!
 \file
 */

/*
 * Getopt for GNU.
 *
 * NOTE: getopt is now part of the C library, so if you don't know what
 * "Keep this file name-space clean" means, talk to drepper@gnu.org
 * before changing it!
 *
 * Copyright (C) 1987, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98
 *  Free Software Foundation, Inc.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * [i_a] if you get weird 'redefinition' errors and such, turn OFF
 *       using precompiled headers in MSVC6. No need for #include stdafx.h
 *       then.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"


int getopt(int argc, char *const *argv, const char *optstring)
{
    return getopt_ex(argc, argv, optstring, fprintf, stderr);
}


/* getopt_long and getopt_long_only entry points for GNU getopt.
 * Copyright (C) 1987,88,89,90,91,92,93,94,96,97,98
 *   Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  */

int getopt_long(int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index)
{
    return getopt_long_ex(argc, argv, options, long_options, opt_index, fprintf, stderr);
}


/* Like getopt_long, but '-' as well as '--' can indicate a long option.
 * If an option that starts with '-' (not '--') doesn't match a long option,
 * but does match a short option, it is parsed as a short option
 * instead.  */
int getopt_long_only(int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index)
{
    return getopt_long_only_ex(argc, argv, options, long_options, opt_index, fprintf, stderr);
}

