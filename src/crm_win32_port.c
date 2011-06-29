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


#if defined(WIN32)


#ifndef HAVE_TRUNCATE
/*
   Native Win32 doesn't come with a truncate() call so we supply one instead.

   The wicked thing is that Win32 doesn't have a system-level truncate()
   either, so we have to do this a bit different. See also:

   http://mail.python.org/pipermail/python-dev/2003-September/037946.html

   for a discussion about this in a different setting. Quoting:

                [...] Windows has no way to say "here's a file, change
                the size to such-and-such"; the only way is to set the file pointer to the
                desired size, and then call the no-argument Win32 SetEndOfFile(); Python
                *used* to use the MS C _chsize() function, but that did insane things when
                passed a "large" size; the SetEndOfFile() code was introduced as part of
                fixing Python's Windows largefile support.


  function: truncate file to the spcified filesize. Return 0 on success, otherwise
            return failure code in errno.
 */
int truncate(const char *filepath, long filesize)
{
        int ret = 0;
        HANDLE f = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (f != INVALID_HANDLE_VALUE)
        {
                LARGE_INTEGER pos;

                pos.QuadPart = filesize;
                if (SetFilePointerEx(f, pos, NULL, FILE_BEGIN))
                {
                        ret = !SetEndOfFile(f);
                }
                CloseHandle(f);
        }
        else
        {
                ret = !0;
        }
        errno = EINVAL; /* fake errno code; too lazy to translate those GetLastError() numbers to errno's now... */
        return ret;
}
#endif



#ifndef HAVE_TIMES
clock_t times(struct tms *buf)
{
  FILETIME create, exit, kern, user;
  if (GetProcessTimes(GetCurrentProcess(), &create, &exit, &kern, &user))
    {
      buf->tms_utime = user.dwLowDateTime;
      buf->tms_stime = kern.dwLowDateTime;
      buf->tms_cutime = 0;
      buf->tms_cstime = 0;
      return GetTickCount();
    }
  return -1;
}
#endif



#endif /* WIN32 */

