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
        CRM_ASSERT(dst != NULL);
        CRM_ASSERT(src != NULL);

        while ((*dst++ = *src++) != 0)
          ;
        return dst - 1;
}

#endif

#if defined(PREFER_PORTABLE_MEMMOVE) || !defined(HAVE_MEMMOVE)

void *crm_memmove(void *dst, const void *src, size_t len)
{
#if defined(HAVE_BCOPY)
        bcopy(src, dst, len);
        return dst;
#else
#error "provide a proper memmove() implementation, please"
#endif
}

#endif


/*
   equivalent to touch(3).

  Because mmap/munmap doesn't set atime, nor set the "modified"
  flag, some network filesystems will fail to mark the file as
  modified and so their cacheing will make a mistake.

  The fix is to call this function on the .css ile, to force
  the filesystem to repropagate it's caches.

  The utime code has been gleaned off the touch tool itself.
*/
void crm_touch(const char *filename)
{
#if defined(HAVE_UTIME) || defined(HAVE__UTIME)
      /* Pass NULL to utime so it will not fail if we just have
         write access to the file, but don't own it.  */
      if (utime (filename, NULL))
        {
      fatalerror_ex(SRC_LOC(), "Unable to touch file %s\n", filename);
        }
#else
        /*
                The fix is to do a trivial read/write on the .css ile, to force
                the filesystem to repropagate it's caches.
        */
    int hfd;                  //  hashfile fd
    unsigned char foo[1];

    hfd = open (filename, O_RDWR | O_BINARY); /* [i_a] on MSwin/DOS, open() opens in CRLF text mode by default; this will corrupt those binary values! */
    if (hfd < 0)
    {
      fatalerror_ex(SRC_LOC(), "Couldn't touch file %s\n", filename);
    }
    else
        {
    read (hfd, foo, sizeof(foo));
    lseek (hfd, 0, SEEK_SET);
    write (hfd, foo, sizeof(foo));
    close (hfd);
        }
#endif
}


