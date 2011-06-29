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


#if defined(PREFER_PORTABLE_MEMMOVE) || !defined(HAVE_MEMMOVE)

void *crm_memmove(void *dst, const void *src, size_t len)
{
#if defined(HAVE_BCOPY)

    bcopy(src, dst, len);
    return dst;

#else

    if (len > 0 && dst && src)
    {
        uint8_t *d = (uint8_t *)dst;
        uint8_t *s = (uint8_t *)src;

        if (d > s)
        {
            d += len;
            s += len;
            switch (len & 0xF)                     // len mod 16: Duff's Device; start
            {
            case 15:
                *d-- = *s--;

            case 14:
                *d-- = *s--;

            case 13:
                *d-- = *s--;

            case 12:
                *d-- = *s--;

            case 11:
                *d-- = *s--;

            case 10:
                *d-- = *s--;

            case 9:
                *d-- = *s--;

            case 8:
                *d-- = *s--;

            case 7:
                *d-- = *s--;

            case 6:
                *d-- = *s--;

            case 5:
                *d-- = *s--;

            case 4:
                *d-- = *s--;

            case 3:
                *d-- = *s--;

            case 2:
                *d-- = *s--;

            case 1:
                *d-- = *s--;

            case 0:
                len >>= 4;                         // len DIV 16
                break;
            }
            for (; len-- > 0;)
            {
                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;

                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;

                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;

                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;
                *d-- = *s--;
            }
        }
        else
        {
            switch (len & 0xF)                     // len mod 16: Duff's Device; start
            {
            case 15:
                *d++ = *s++;

            case 14:
                *d++ = *s++;

            case 13:
                *d++ = *s++;

            case 12:
                *d++ = *s++;

            case 11:
                *d++ = *s++;

            case 10:
                *d++ = *s++;

            case 9:
                *d++ = *s++;

            case 8:
                *d++ = *s++;

            case 7:
                *d++ = *s++;

            case 6:
                *d++ = *s++;

            case 5:
                *d++ = *s++;

            case 4:
                *d++ = *s++;

            case 3:
                *d++ = *s++;

            case 2:
                *d++ = *s++;

            case 1:
                *d++ = *s++;

            case 0:
                len >>= 4;                         // len DIV 16
                break;
            }
            for (; len-- > 0;)
            {
                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;

                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;

                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;

                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;
                *d++ = *s++;
            }
        }
    }
    return dst;

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
#if defined(HAVE_UTIME) || defined(HAVE__UTIME)
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
int file_memset(FILE *dst, unsigned char val, size_t count)
{
    unsigned char buf[1024];

    memset(buf, val, 1024);

    while (count > 0)
    {
        ssize_t len = (ssize_t)fwrite(buf, 1, CRM_MIN(count, 1024), dst);
        if (len <= 0)
            return -1;

        count -= len;
    }
    return 0;
}



/*
 * ASCII/C dump a byte sequence to FILE* (probably stdout/stderr).
 *
 * This routine is a high-speed (buffered I/O) method for writing out
 * binary data as ASCII 'C' data.
 *
 * Use this, for instance, to dump variable names and other tidbits
 * to stderr while diagnosing CRM114 behaviour: it will prevent CRM114
 * from screwing up your console window config (which it would otherwise
 * be capable of doing by spewing binary data to it, including
 * undesirable ESC/byte sequences).
 *
 * Return value: the number of bytes written to FILE*.
 *
 * return -1 when an error occurred (check 'errno' for more info then)
 */
ssize_t fwrite_ASCII_Cfied(FILE *dst, const char *src, size_t len)
{
    char buf[2048 + 4];
    size_t i;
    ssize_t cnt = 0;
    size_t j;

    for (j = i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)src[i];

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
            if (j != fwrite4stdio(buf, j, dst))
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
        if (j != fwrite4stdio(buf, j, dst))
        {
            // error!
            return -1;
        }
        cnt += j;
    }
    return cnt;
}


#if !defined(HAVE_MEMMEM) || defined(PREFER_PORTABLE_MEMMEM)

void *my_memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len)
{
    const uint8_t *h;
    const uint8_t *n;

    if (needle_len == 0)
        return (void *)haystack;

    if (needle_len > haystack_len)
        return NULL;

    // could do a Boyer-Moore search here, but only for larger needle_len...
    //
    // aw, shucks, we'll do it the brute way:
    n = (const uint8_t *)needle;
    h = (const uint8_t *)haystack;

    do
    {
        const uint8_t *f = memchr(h, n[0], haystack_len);
        size_t i;
        const uint8_t *n0 = NULL;

        if (!f)
            return NULL;

        haystack_len -= (f - h);

        // see if we have a full match. Optimization: keep track of occurrences of n[0] too:
        do
        {
            for (i = 1; i < needle_len; i++)
            {
                if (n0 && f[i] == n[0])
                    n0 = &n[i];
                if (f[i] != n[i])
                    break;
            }
            if (i == needle_len)
            {
                // scored a hit!
                return (void *)f;
            }
            // n0 points to the next possibility, if it is !NULL
            if (!n0)
                break;

            haystack_len -= (n0 - f);
            f += (n0 - f);
        } while (haystack_len >= needle_len);
        // when we get here, either we've run out of haystack
        // OR the complete needle_len area of haystack didn't have another n[0]:
        // in the latter case, we can skip all those bytes as we scanned them already.
        if (haystack_len >= needle_len)
        {
            haystack_len -= needle_len;
            h = f + needle_len;
        }
    } while (haystack_len >= needle_len);      // still got a fightin' chance to find that needle?

    return NULL;
}

#endif


#if !defined(HAVE_STRNCHR)

//    a helper function that should be in the C runtime lib but isn't.
//
char *my_strnchr(const char *str, int c, size_t len)
{
    size_t i;

    i = 0;
    for (i = 0; i < len; i++)
    {
        if (str[i] == (char)c)
            return (char *)&str[i];
    }
    return NULL;
}

#endif


