//  crm_win32_port.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2006  William S. Yerazunis, all rights reserved.
//  
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.  
//
//  Other licenses may be negotiated; contact the 
//  author for details.  
//
//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"


/*
  strmov(dst, src) moves all the  characters  of  src  (including  the
  closing NUL) to dst, and returns a pointer to the new closing NUL in
  dst.

  strmov() will generate undefined results when used with overlapping 
  strings.
*/

#ifndef HAVE_STRMOV

char *strmov(char *dst, const char *src)
{
	assert(dst != NULL);
	assert(src != NULL);

	while ((*dst++ = *src++) != 0)
	  ;
	return dst - 1;
}

#endif

