//  crm_win32_port.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
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


#if (defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(WIN64))


#ifndef HAVE_TRUNCATE
/*
 * Native Win32 doesn't come with a truncate() call so we supply one instead.
 *
 * The wicked thing is that Win32 doesn't have a system-level truncate()
 * either, so we have to do this a bit different. See also:
 *
 * http://mail.python.org/pipermail/python-dev/2003-September/037946.html
 *
 * for a discussion about this in a different setting. Quoting:
 *
 *   [...] Windows has no way to say "here's a file, change
 *   the size to such-and-such"; the only way is to set the file pointer to the
 *   desired size, and then call the no-argument Win32 SetEndOfFile(); Python
 *   *used* to use the MS C _chsize() function, but that did insane things when
 *   passed a "large" size; the SetEndOfFile() code was introduced as part of
 *   fixing Python's Windows largefile support.
 *
 *
 * function: truncate file to the spcified filesize. Return 0 on success, otherwise
 *          return failure code in errno.
 */
int truncate(const char *filepath, size_t filesize)
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
        ret = (S_OK != CloseHandle(f));
		if (ret)
		{
		    errno = EINVAL;     /* fake errno code; too lazy to translate those GetLastError() numbers to errno's now... */
		}
    }
    else
    {
        ret = !0;
	    errno = EINVAL;     /* fake errno code; too lazy to translate those GetLastError() numbers to errno's now... */
    }
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





#if defined (_DEBUG)

_CrtMemState crm_memdbg_state_snapshot1;
int trigger_memdump = 0;



/*
 * Define our own reporting function.
 * We'll hook it into the debug reporting
 * process later using _CrtSetReportHook.
 */
int crm_dbg_report_function(int report_type, char *usermsg, int *retval)
{
    /*
     * By setting retVal to zero, we are instructing _CrtDbgReport
     * to continue with normal execution after generating the report.
     * If we wanted _CrtDbgReport to start the debugger, we would set
     * retVal to one.
     */
    *retval = !!trigger_debugger;

    /*
     * When the report type is for an ASSERT,
     * we'll report some information, but we also
     * want _CrtDbgReport to get called -
     * so we'll return TRUE.
     *
     * When the report type is a WARNing or ERROR,
     * we'll take care of all of the reporting. We don't
     * want _CrtDbgReport to get called -
     * so we'll return FALSE.
     */
    switch (report_type)
    {
    default:
    case _CRT_WARN:
    case _CRT_ERROR:
    case _CRT_ERRCNT:
        fputs(usermsg, stderr);
        fflush(stderr);
        return FALSE;

    case _CRT_ASSERT:
        fputs(usermsg, stderr);
        fflush(stderr);
        break;
    }
    return TRUE;
}


void crm_report_mem_analysis(void)
{
    _CrtMemState msNow;

    if (!_CrtCheckMemory())
    {
        fprintf(stderr, ">>>Failed to validate memory heap<<<\n");
    }

    /* only dump leaks when there are in fact leaks */
    _CrtMemCheckpoint(&msNow);

    if (msNow.lCounts[_CLIENT_BLOCK] != 0
        || msNow.lCounts[_NORMAL_BLOCK] != 0
        || (_crtDbgFlag & _CRTDBG_CHECK_CRT_DF
            && msNow.lCounts[_CRT_BLOCK] != 0)
    )
    {
        /* difference detected: dump objects since start. */
        _RPT0(_CRT_WARN, "============== Detected memory leaks! ====================\n");

        _CrtMemDumpAllObjectsSince(&crm_memdbg_state_snapshot1);
        _CrtMemDumpStatistics(&crm_memdbg_state_snapshot1);
    }
}

#endif



int getpagesize(void)
    {
        SYSTEM_INFO info;
                int pagesize;
        GetSystemInfo(&info);
        pagesize = info.dwPageSize;

return pagesize;
    }



#endif /* (defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(WIN64)) */

