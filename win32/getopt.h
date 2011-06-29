/*!
 \file
 */
/* Declarations for getopt.
 * Copyright (C) 1989,90,91,92,93,94,96,97,98 Free Software Foundation, Inc.
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

#ifndef __GETOPT_C_H__
#define __GETOPT_C_H__

#include "getopt_ex.h"



#ifdef __cplusplus
extern "C"
{
#endif


/*!
 * Get definitions and prototypes for functions to process the
 * arguments in ARGV (ARGC of them, minus the program name) for
 * options given in OPTS.
 *
 * Return the option character from OPTS just read.  Return -1 when
 * there are no more options.  For unrecognized options, or options
 * missing arguments, `optopt' is set to the option letter, and '?' is
 * returned.
 *
 * The OPTS string is a list of characters which are recognized option
 * letters, optionally followed by colons, specifying that that letter
 * takes an argument, to be placed in `optarg'.
 *
 * If a letter in OPTS is followed by two colons, its argument is
 * optional.  This behavior is specific to the GNU `getopt'.
 *
 * The argument `--' causes premature termination of argument
 * scanning, explicitly telling `getopt' that there are no more
 * options.
 *
 * If OPTS begins with `--', then non-option arguments are treated as
 * arguments to the option 0.  This behavior is specific to the GNU
 * `getopt'.
 */
int getopt(int argc, char *const *argv, const char *optstring);
int getopt_long(int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index);
int getopt_long_only(int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index);


#ifdef __cplusplus
}
#endif

#endif /* getopt.h */

