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


/*
 * strmov(dst, src) moves all the  characters  of  src  (including  the
 * closing NUL) to dst, and returns a pointer to the new closing NUL in
 * dst.
 *
 * strmov() will generate undefined results when used with overlapping
 * strings.
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


#if defined (PREFER_PORTABLE_MEMMOVE) || !defined (HAVE_MEMMOVE)

void *crm_memmove(void *dst, const void *src, size_t len)
{
#if defined (HAVE_BCOPY)
    bcopy(src, dst, len);
    return dst;

#else
#error "provide a proper memmove() implementation, please"
#endif
}

#endif


/*
 * equivalent to touch(3).
 *
 * Because mmap/munmap doesn't set atime, nor set the "modified"
 * flag, some network filesystems will fail to mark the file as
 * modified and so their cacheing will make a mistake.
 *
 * The fix is to call this function on the .css file, to force
 * the filesystem to repropagate it's caches.
 *
 * The utime code has been gleaned off the touch tool itself.
 */
void crm_touch(const char *filename)
{
#if defined (HAVE_UTIME) || defined (HAVE__UTIME)
    /* Pass NULL to utime so it will not fail if we just have
     * write access to the file, but don't own it.  */
    if (utime(filename, NULL))
    {
		fatalerror_ex(SRC_LOC(), "Unable to touch file '%s'; it might be that this file is used by another application - system error: %d(%s)\n",
			filename, errno, errno_descr(errno));
    }
#else
    /*
     *      The fix is to do a trivial read/write on the .css ile, to force
     *      the filesystem to repropagate it's caches.
     */
    int hfd;                  //  hashfile fd
    unsigned char foo[1];

    hfd = open(filename, O_RDWR | O_BINARY); /* [i_a] on MSwin/DOS, open() opens in CRLF text mode by default; this will corrupt those binary values! */
    if (hfd < 0)
    {
        fatalerror_ex(SRC_LOC(), "Couldn't touch file '%s'; it might be that this file is used by another application - system error: %d(%s)\n",
			filename, errno, errno_descr(errno));
    }
    else
    {
        read(hfd, foo, sizeof(foo));
        lseek(hfd, 0, SEEK_SET);
        write(hfd, foo, sizeof(foo));
        close(hfd);
    }
#endif
}




/*
 * write the specified number of byte c to file f.
 *
 * Return 0 on success, otherwise the error will be available in
 * errno.
 */
int file_memset(FILE *dst, unsigned char val, int count)
{
    unsigned char buf[1024];

    memset(buf, val, 1024);

    while (count > 0)
    {
        int len = (int)fwrite(buf, 1, CRM_MIN(count, 1024), dst);
        if (len <= 0)
            return -1;

        count -= len;
    }
    return 0;
}



/*
   ASCII/C dump a byte sequence to FILE* (probably stdout/stderr).

   This routine is a high-speed (buffered I/O) method for writing out
   binary data as ASCII 'C' data.

   Use this, for instance, to dump variable names and other tidbits
   to stderr while diagnosing CRM114 behaviour: it will prevent CRM114
   from screwing up your console window config (which it would otherwise
   be capable of doing by spewing binary data to it, including 
   undesirable ESC/byte sequences).

   Return value: the number of bytes written to FILE*.

   return -1 when an error occurred (check 'errno' for more info then)
*/
int fwrite_ASCII_Cfied(FILE *dst, const char *src, int len)
{
	char buf[2048+4];
	int i;
	int cnt = 0;
	int j;

	for (j = i = 0; i < len; i++)
{
	int c = (unsigned char)src[i];
	
		switch (c)
{
case '\\':
	buf[j++] = '\\';
	buf[j++] = '\\';
	break;

	
case '\n':
	buf[j++] = '\\';
	buf[j++] = 'n';
	break;

case '\r':
	buf[j++] = '\\';
	buf[j++] = 'r';
	break;

case '\t':
	buf[j++] = '\\';
	buf[j++] = 't';
	break;

case '\a':
	buf[j++] = '\\';
	buf[j++] = 'a';
	break;

case '\b':
	buf[j++] = '\\';
	buf[j++] = 'b';
	break;

case '\v':
	buf[j++] = '\\';
	buf[j++] = 'v';
	break;

case '\f':
	buf[j++] = '\\';
	buf[j++] = 'f';
	break;

default:
	if (crm_isascii(c) && crm_isprint(c))
	{
		buf[j++] = c;
	}
	else
	{
		buf[j++] = '\\';
		buf[j++] = 'x';
		CRM_ASSERT((c >> 4) >= 0 && (c >> 4) <= 0xF);
		buf[j++] = "0123456789abcdef"[c >> 4];
		buf[j++] = "0123456789abcdef"[c & 0xF];
	}
break;
}

 // biggest chunk to be dumped per char is the HEX escape @ 4 chars
if (j > WIDTHOF(buf) - 4)
{
	if (1 != fwrite(buf, 1, j, dst))
	{
		// error!
		return -1;
}
	cnt += j;
j = 0;
}
}
 // dump remainder of buffer to dst
if (j > 0)
{
	if (1 != fwrite(buf, 1, j, dst))
	{
		// error!
		return -1;
}
	cnt += j;
}
return cnt;
}

