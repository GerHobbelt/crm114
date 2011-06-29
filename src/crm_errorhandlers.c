//  crm_errorhandlers.c  - Controllable Regex Mutilator,  version v1.0
// Copyright 2001-2007  William S. Yerazunis, all rights reserved.
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



//     Helper routines
//


#ifndef CRM_ASSERT_IS_UNTRAPPABLE
#error "config is corrupted; check config.h"
#endif
#if CRM_ASSERT_IS_UNTRAPPABLE
#define CRM_ASSERT_MESSENGER            untrappableerror_ex
#else
#define CRM_ASSERT_MESSENGER            fatalerror_ex
#endif



static void dump_error_script_line(char *dst, int maxdstlen, CSL_CELL *csl, int stmtnum)
{
    char *orig_dst = dst;
    int orig_maxdstlen = maxdstlen;

    //     Check to see - is there a trap available or is this a non-trap
    //     program?
    //
    if (!dst || maxdstlen <= 1)
        return;

    maxdstlen--;     // reserve space for the NUL sentinel at the end...

    // current statement where the error occurred:
    if (csl && csl->nstmts > 0 && csl->mct && csl->filetext
        && stmtnum >= 0 && stmtnum < csl->nstmts
        && stmtnum + 1 < csl->mct_size
        && csl->filetext)
    {
        MCT_CELL *stmt = csl->mct[stmtnum];
        MCT_CELL *next_stmt = csl->mct[stmtnum + 1];

        if (stmt && next_stmt
            && stmt->start >= 0
            && next_stmt->start >= 0)
        {
            int maxchar = csl->mct[stmtnum + 1]->fchar;
            int startchar = csl->mct[stmtnum]->fchar;
            unsigned char *sourcedata = (unsigned char *)(stmt->hosttxt ? stmt->hosttxt : csl->filetext);

            int ichar;
            int has_nonprintable_chars_on_board = 0;
            int last_was_LF = 0;

            // maxchar == 0: this may happen when there's a fatalerror in the compiler
            if (maxchar < startchar || !stmt->hosttxt || maxchar > csl->nchars)
            {
                // find EOL
                for (maxchar = startchar; maxchar < csl->nchars; maxchar++)
                {
                    switch (sourcedata[maxchar])
                    {
                    case 0:
                    case '\r':
                    case '\n':
                        break;

                    default:
                        continue;
                    }
                    break;
                }
                // now maxchar points to EOL or EOS
            }
            else
            {
                //
                // the compiler/preprocessor has a tendency to include the leading whitespace of the next line
                // with _this_ line, so we'll get rid of any trailing whitespace now to make sure it all'll
                // look mighty pretty right there.
                //
                for ( ; --maxchar >= startchar;)
                {
                    if (!crm_isspace(sourcedata[maxchar]))
                    {
                        maxchar++;
                        break;
                    }
                }
            }

            // try to preserve our sanity whn the line length numbers are weird: heuristical fix for something that's probably utterly FUBAR.
            if (maxchar > startchar + 255)
            {
                maxchar = startchar + 255;
            }

            for (ichar = startchar;
                 ichar < maxchar && maxdstlen > 0;
                 ichar++)
            {
                int c = sourcedata[ichar];
                switch (c)
                {
                case '\n':
                    last_was_LF = 1;

                case '\r':
                    *dst++ = c;
                    maxdstlen--;
                    break;

                case '\t':
                    *dst++ = ' ';
                    maxdstlen--;
                    if (maxdstlen >= 7 + 1)
                    {
                        dst = strmov(dst, "       ");
                        maxdstlen -= 7;
                    }
                    break;

                default:
                    if (crm_isprint(c))
                    {
                        *dst++ = c;
                        maxdstlen--;
                    }
                    else
                    {
                        has_nonprintable_chars_on_board = 1;
                        *dst++ = '.';
                        maxdstlen--;
                    }
                    break;
                }
            }
            if (!last_was_LF && maxdstlen > 0)
            {
                *dst++ = '\n';
                maxdstlen--;
            }

            if (has_nonprintable_chars_on_board
                && maxdstlen > WIDTHOF("The line was (in HEX bytes):\n-->") + 40 /* heuristic padding */)
            {
                dst = strmov(dst, "The line was (in HEX bytes):\n-->");
                maxdstlen -= WIDTHOF("The line was (in HEX bytes):\n-->") - 1;             // don't count the NUL sentinel in there.

                for (ichar = startchar;
                     ichar < maxchar && maxdstlen > 3;
                     ichar++)
                {
                    int c = sourcedata[ichar];
                    sprintf(dst, " %02x", c);
                    c = strlen(dst);
                    dst += c;
                    maxdstlen -= c;
                }
                if (maxdstlen > 0)
                {
                    *dst++ = '\n';
                    maxdstlen--;
                }
            }
        }
    }
    CRM_ASSERT(maxdstlen >= 0);
    CRM_ASSERT(maxdstlen == orig_maxdstlen - 1 - (dst - orig_dst));
    dst[0] = 0;

    if (!orig_dst[0] && orig_maxdstlen >= WIDTHOF("\n<<< no compiled statements yet >>>\n"))
    {
        strcpy(dst, "\n<<< no compiled statements yet >>>\n");
    }
}


const char *skip_path(const char *srcfile)
{
    if (srcfile)
    {
        char *p = strrchr(srcfile, '/');
        if (p != NULL)
        {
            srcfile = p + 1;
        }
        p = strrchr(srcfile, '\\');
        if (p != NULL)
        {
            srcfile = p + 1;
        }
        p = strrchr(srcfile, ':');
        if (p != NULL)
        {
            srcfile = p + 1;
        }
    }
    return srcfile;
}

/*
 *      Returns the line + sourcefile + error message in a nicely formatted string.
 *
 *      (TODO: may even support multiple formats for easy debugging/code jumping)
 */
static void generate_err_reason_msg(
    char       *reason,
    int         reason_bufsize,
    int         srclineno,
    const char *srcfile_full,
    const char *funcname,
    const char *errortype_str,
    const char *encouraging_msg,
    CSL_CELL   *csl,
    int         script_codeline,
    const char *fmt,
    va_list     args
    )
{
    int widthleft = reason_bufsize;
    int has_newline;
    char *dst = reason;
    int encouragment_length;
    const int bill_style_errormessage = TRUE;

    /*
     *         some OS's include the complete path with the programname; we're not interested in that part here...
     */
    const char *progname = skip_path(prog_argv0);
    /* some compilers include the full path: strip if off! */
    const char *srcfile = skip_path(srcfile_full);

    //
    //   Create our reason string.  Note that some reason text can be VERY
    //   int, so we put out only the first 2048+40  characters
    //

    if (!reason || reason_bufsize <= 0)
    {
        return;
    }
    strncpy(reason, "\nERROR\n", reason_bufsize);      /* if we receive bogus parameters, make do as best we can */
    reason[reason_bufsize - 1] = 0;

    /* now anything is going to better than _that_ */


    if (widthleft > (12 + WIDTHOF("(truncated...)\n")                        /* see at the end of the code; buffer should be large enough to cope with it all */
                     + (srclineno > 0 ? (SIZEOF_LONG_INT * 12) / 4 : 0)      /* guestimate the worst case length upper limit for printf(%d) */
                     + (progname ? strlen(progname) : strlen("CRM114"))
                     + (srcfile ? strlen(srcfile) : 0)
                     + (funcname ? strlen(funcname) : 0)
                     + (errortype_str ? strlen(errortype_str) : strlen(" *UNIDENTIFIED ERROR*"))
                     ))
    {
        /*
         * only include what's specified. Format:
         *
         * \n<program>:<sourcefile>:<functionname>:<sourceline>: <error type>\n
         *
         * OR:
         *
         * \n<program>: <error type>\n(runtime system location: <sourcefile>(<sourceline>) in routine: <functionname>)\n
         */
        char *fname_pos;
        char *d;

        dst = strmov(dst, "\n");
        dst = strmov(dst, (progname && *progname) ? progname : "CRM114");
        dst = strmov(dst, ":");

        if (!bill_style_errormessage)
        {
            dst = strmov(dst, (srcfile && *srcfile) ? srcfile : "");
            dst = strmov(dst, ":");
            fname_pos = dst;
            dst = strmov(dst, (funcname && *funcname) ? funcname : "");
            // replace ':' and '::' with '.' in fully qualified function name (if it was passed along
            // like that, e.g. by using GCC's __PRETTY_FUNCTION__ predefined variable.
            for (d = fname_pos; ; fname_pos++)
            {
                switch (*fname_pos)
                {
                case 0:
                    *d = 0;
                    dst = d;
                    break;

                case ':':
                    if (fname_pos[1] == ':')
                        fname_pos++;
                    *d++ = '.';
                    continue;

                default:
                    *d++ = *fname_pos;
                    continue;
                }
                break;
            }
            dst = strmov(dst, ":");
            if (srclineno > 0)
            {
                sprintf(dst, "%d:", srclineno);
                dst += strlen(dst);
            }
            else
            {
                dst = strmov(dst, ":");
            }
        }
        dst = strmov(dst, (errortype_str && *errortype_str) ? errortype_str : " *UNIDENTIFIED ERROR*");
        dst = strmov(dst, "\n");
    }
    else
    {
        /* can't handle this error message in such a small buffer, so quit now you still can. */
        return;
    }
    widthleft = reason_bufsize - (dst - reason);

    /*
     * Now print the custom message section. Keep a guestimated room for the next section too: 256.
     */
#define MIN_BUFLEN_REQD   (100 + 256)
    if (widthleft > MIN_BUFLEN_REQD && fmt && args)
    {
        int len;

        vsnprintf(dst, widthleft, fmt, args);
        dst[widthleft - 1] = 0;
        len = strlen(dst);
        dst += len;
    }
    has_newline = (dst[-1] == '\n');
    widthleft = reason_bufsize - (dst - reason);

    /*
     * make sure there's still enough room for the last part of this message...
     */
    if (widthleft < MIN_BUFLEN_REQD)
    {
        if (reason_bufsize > MIN_BUFLEN_REQD + 512 && (dst - reason) > 2048 + 100)
        {
            /* truncate this section of the message... */
            int len = reason_bufsize - MIN_BUFLEN_REQD;
            dst = reason + len;
            widthleft = MIN_BUFLEN_REQD;

            dst = strmov(dst, "(...truncated in emergency)");
            has_newline = 0;
        }
    }
#undef MIN_BUFLEN_REQD
    widthleft = reason_bufsize - (dst - reason);

    if (!has_newline && widthleft > 1)
    {
        dst = strmov(dst, "\n");
    }
    widthleft = reason_bufsize - (dst - reason);

    if (!encouraging_msg || ! * encouraging_msg)
    {
        encouraging_msg = "Sorry, but this program is very sick and probably should be killed off.";
    }
    encouragment_length = strlen(encouraging_msg) + 1;

    if (widthleft > encouragment_length
        + WIDTHOF("This happened at line %d of file %s\n") + 40)
    {
        dst = strmov(dst, encouraging_msg);
        has_newline = (dst[-1] == '\n');
    }
    widthleft = reason_bufsize - (dst - reason);

    if (!has_newline && widthleft > 1)
    {
        dst = strmov(dst, "\n");
    }
    widthleft = reason_bufsize - (dst - reason);

    if (widthleft > (WIDTHOF("This happened at line %d of file %s:\n")
                     + (SIZEOF_LONG_INT * 12) / 4          /* guestimate the worst case length upper limit for printf(%d) */
                     + (csl && csl->filename ? strlen(csl->filename) : 0))
        && csl && csl->filename)
    {
        int len;

        snprintf(dst, widthleft, "This happened at line %d of file %s:\n    ",
            csl->cstmt, csl->filename);
        dst[widthleft - 1] = 0;
        len = strlen(dst);
        dst += len;
        widthleft -= len;

        dump_error_script_line(dst, widthleft, csl, script_codeline);
        len = strlen(dst);
        dst += len;
        widthleft -= len;
    }


    /*
     * If we want/have to, add the BillY style of source location error reporting here.
     *
     * Format:
     *  (runtime system location: <file>(<line>) in routine: <function>)
     */
    if (bill_style_errormessage)
    {
        if (widthleft > ((srclineno > 0 ? (SIZEOF_LONG_INT * 12) / 4 : 1)  /* guestimate the worst case length upper limit for printf(%d) */
                         + (srcfile ? strlen(srcfile) : 3)
                         + (funcname ? strlen(funcname) : 3)
                         + WIDTHOF("(runtime system location: X(X) in routine: X)\n")
                         ))
        {
            /*
             * (runtime system location: <sourcefile>(<sourceline>) in routine: <functionname>)\n
             */
            char *fname_pos;
            char *d;
            int len;

            snprintf(dst, widthleft, "(runtime system location: %s(%d) in routine: ",
                ((srcfile && *srcfile) ? srcfile : "---"),
                srclineno);
            dst[widthleft - 1] = 0;
            len = strlen(dst);
            dst += len;

            fname_pos = dst;
            dst = strmov(dst, (funcname && *funcname) ? funcname : "\?\?\?");
            // replace ':' and '::' with '.' in fully qualified function name (if it was passed along
            // like that, e.g. by using GCC's __PRETTY_FUNCTION__ predefined variable.
            for (d = fname_pos; ; fname_pos++)
            {
                switch (*fname_pos)
                {
                case 0:
                    *d = 0;
                    dst = d;
                    break;

                case ':':
                    if (fname_pos[1] == ':')
                        fname_pos++;
                    *d++ = '.';
                    continue;

                default:
                    *d++ = *fname_pos;
                    continue;
                }
                break;
            }
            dst = strmov(dst, ")\n");
        }
    }

    /* make sure the string ends with a newline! */
    if (dst[-1] != '\n')
    {
        dst = strmov(dst - WIDTHOF("(...truncated)\n"), "(...truncated)\n");
    }
}



/*
 *   Check to see - is there a trap available or is this a non-trap
 *   program?
 *
 *      if (csl->mct[csl->cstmt]->trap_index <  csl->nstmts)
 *
 *
 *   Perform all the sanity checks necessary to stay while handling this error.
 *       Assume the WORST!
 *
 *       Return 0 when the trap catched this one.
 *
 *       Return -1 when no trap whatsoever was found.
 */
static int check_for_trap_handler(CSL_CELL *csl, const char *reason)
{
    if (csl && csl->nstmts > 0 && csl->mct
        && csl->cstmt >= 0 && csl->cstmt < csl->nstmts && csl->mct[csl->cstmt]
        && csl->mct[csl->cstmt]->trap_index < csl->nstmts)
    {
        int ret;
        char *rbuf = strdup(reason);
        if (!rbuf)
        {
            fprintf(stderr,
                "Couldn't alloc rbuf in 'fatalerror()'!\nIt's really bad when the error fixup routine gets an error!\n");
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 3);
            }
            else
            {
                exit(CRM_EXIT_FATAL);
            }
        }

        ret = crm_trigger_fault(rbuf);
        free(rbuf);
        return ret;
    }

    return -1;
}


#ifndef CRM_DONT_ASSERT

int trigger_debugger = 0;

void crm_show_assert_msg(int lineno, const char *srcfile, const char *funcname, const char *msg)
{
    crm_show_assert_msg_ex(lineno, srcfile, funcname, msg, NULL);
}

void crm_show_assert_msg_ex(int lineno, const char *srcfile, const char *funcname, const char *msg, const char *extra_msg)
{
    if (trigger_debugger)
    {
        /* trigger a debugger breakpoint for easy debugging... */
#if defined HAVE__CRTDBGBREAK
        _CrtDbgBreak();
#elif defined HAVE___DEBUGBREAK
        __debugbreak();
#else
        /* don't know how so it's going to be the 'last ditch effort' then */
#endif
        /* last ditch effort: make sure a core dump is about so the debugger may pop up */
        {
            char *p = NULL;
            *p = 0;
        }
    }
    CRM_ASSERT_MESSENGER(lineno, srcfile, funcname,
        "\nBetter start screaming, guv', since the software's just gone critical:\n"
        "assertion '%s' failed!%s%s\n",
        msg, (extra_msg ? "\n" : ""), (extra_msg ? extra_msg : ""));
}

#endif





const char *errno_descr(int errno_number)
{
    const char *ret = strerror(errno_number);

    if (!ret || ! * ret)
    {
        return "\?\?\?";
    }
    return ret;
}


#if 0
const char *syserr_descr(int errno_number)
{
    char *ret = strerror(errno_number);

    if (!ret || ! * ret)
    {
        return "\?\?\?";
    }
    return ret;
}
#endif

#if defined (WIN32)

/*
 * return a static string containing the errorcode description.
 */
void Win32_syserr_descr(char **dstptr, size_t max_dst_len, DWORD errorcode, const char *arg)
{
    DWORD fmtret;
    LPSTR dstbuf = NULL;
    char *dst;

    if (dstptr == NULL || max_dst_len <= WIDTHOF("(...truncated)") + 1)
        return;

    if (*dstptr == NULL)
    {
        if (max_dst_len < MAX_PATTERN)
        {
            max_dst_len = MAX_PATTERN;
        }
        *dstptr = calloc(max_dst_len, sizeof(*dst));
        if (*dstptr == NULL)
        {
            return;
        }
    }
    CRM_ASSERT(max_dst_len > WIDTHOF("(...truncated)"));
    dst = *dstptr;
    dst[0] = 0;
    max_dst_len--;            // lazy, so we don't have to offset by '-1' every time when we while a NUL sentinel below.
    dst[max_dst_len] = 0;     // this now does NOT write out-of-bounds :-)

    fmtret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, errorcode, 0, (LPSTR)&dstbuf, 0, NULL);
    if (dstbuf != NULL && fmtret > 0 && *dstbuf)
    {
        const char *s = dstbuf;
        const char *p = strchr(s, '%');
        size_t len;

        while (p && max_dst_len > 0)
        {
            strncpy(dst, s, CRM_MIN(p - s, max_dst_len));
            dst[max_dst_len] = 0;
            len = strlen(dst);
            max_dst_len -= len;
            dst += len;
            if (max_dst_len <= 3)
                break;

            if (p[1] == '1')
            {
                // replace '%1' with ''arg''
                strncpy(dst, "'", max_dst_len);
                strncpy(dst + 1, (arg ? arg : "\?\?\?"), max_dst_len - 1);
                dst[max_dst_len] = 0;
                len = strlen(dst);
                max_dst_len -= len;
                dst += len;
                if (max_dst_len <= 2)
                    break;
                strncpy(dst, "'", max_dst_len);
                dst++;
                max_dst_len--;

                p += 2;
            }
            else
            {
                strncpy(dst, "%", max_dst_len);
                dst++;
                max_dst_len--;

                p++;
            }
            s = p;
            p = strchr(s, '%');
        }
        strncpy(dst, s, max_dst_len);
        dst[max_dst_len] = 0;
        len = strlen(dst);
        max_dst_len -= len;
        dst += len;
        s += len;
        if (*s)
        {
            dst -= WIDTHOF("(...truncated)");
            strcpy(dst, "(...truncated)");
        }
    }
    if (!dstptr[0][0])
    {
        snprintf(*dstptr, max_dst_len, "\?\?\?");
        *dstptr[max_dst_len] = 0;
    }
    if (dstbuf)
    {
        LocalFree(dstbuf);
    }
}

#endif




//     apocalyptic error - an error that can't be serviced on a TRAP - forces
//     exit, not a prayer of survival.
void untrappableerror_std(int lineno, const char *srcfile, const char *funcname, const char *text1, const char *text2)
{
    untrappableerror_ex(lineno, srcfile, funcname,
        (text2 && strlen(text2) <= 1024
         ? " %.1024s %.1024s\n"
         : " %.1024s %.1024s(...truncated)\n"),
        text1, text2);
}

void untrappableerror_ex(int lineno, const char *srcfile, const char *funcname, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    untrappableerror_va(lineno, srcfile, funcname, fmt, args);
    va_end(args);
}

void untrappableerror_va(int lineno, const char *srcfile, const char *funcname, const char *fmt, va_list args)
{
    char reason[MAX_PATTERN];

    generate_err_reason_msg(
        reason,
        WIDTHOF(reason),
        lineno,
        srcfile,
        funcname,
        " *UNTRAPPABLE ERROR*",
        NULL,
        csl,
        csl->cstmt,
        fmt,
        args);
    fputs(reason, stderr);

    if (engine_exit_base != 0)
    {
        exit(engine_exit_base + 2);
    }
    else
    {
        exit(CRM_EXIT_APOCALYPSE);
    }
}


//     fatalerror - print a fatal error on stdout, trap if we can, else exit
int fatalerror_std(int lineno, const char *srcfile, const char *funcname, const char *text1, const char *text2)
{
    //
    //   Note that some reason text2's can be VERY
    //   int, so we put out only the first 1024 characters
    //

    return fatalerror_ex(lineno, srcfile, funcname,
        (text2 && strlen(text2) <= 1024 ?
         " %.1024s %.1024s\n" :
         " %.1024s %.1024s(...truncated)\n"),
        text1, text2);
}

int fatalerror_ex(int lineno, const char *srcfile, const char *funcname, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = fatalerror_va(lineno, srcfile, funcname, fmt, args);
    va_end(args);
    return ret;
}

int fatalerror_va(int lineno, const char *srcfile, const char *funcname, const char *fmt, va_list args)
{
    char reason[MAX_PATTERN];
    int trap_catch;
    int original_statement_line = (csl != NULL ? csl->cstmt : -1);

    generate_err_reason_msg(
        reason,
        WIDTHOF(reason),
        lineno,
        srcfile,
        funcname,
        " *ERROR*",
        NULL,
        csl,
        original_statement_line,
        fmt,
        args);

    trap_catch = check_for_trap_handler(csl, reason);
    if (trap_catch == 0)
    {
        /* handled! */
        return 0;
    }

    fputs(reason, stderr);

    if (engine_exit_base != 0)
    {
        exit(engine_exit_base + 4);
    }
    else
    {
        exit(CRM_EXIT_FATAL);
    }
}

int nonfatalerror_std(int lineno, const char *srcfile, const char *funcname, const char *text1, const char *text2)
{
    return nonfatalerror_ex(lineno, srcfile, funcname,
        (text2 && strlen(text2) <= 1024 ?
         " %.1024s %.1024s\n" :
         " %.1024s %.1024s(...truncated)\n"),
        text1, text2);
}

int nonfatalerror_ex(int lineno, const char *srcfile, const char *funcname, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = nonfatalerror_va(lineno, srcfile, funcname, fmt, args);
    va_end(args);
    return ret;
}

static int nonfatalerrorcount = 0;
static int nonfatalerrorcount_max_reported = 0;

int nonfatalerror_va(int lineno, const char *srcfile, const char *funcname, const char *fmt, va_list args)
{
    char reason[MAX_PATTERN];
    int trap_catch;
    int original_statement_line = (csl != NULL ? csl->cstmt : -1);

    generate_err_reason_msg(
        reason,
        WIDTHOF(reason),
        lineno,
        srcfile,
        funcname,
        " *WARNING*",
        "I'll try to keep working.\n",
        csl,
        original_statement_line,
        fmt,
        args);

    trap_catch = check_for_trap_handler(csl, reason);
    if (trap_catch == 0)
    {
        /* handled! */
        return 0;
    }

    fputs(reason, stderr);

    nonfatalerrorcount++;

    if (nonfatalerrorcount > MAX_NONFATAL_ERRORS)
    {
        if (!nonfatalerrorcount_max_reported)
        {
            trap_catch = fatalerror_ex(lineno, srcfile, funcname,
                "Too many untrapped warnings; your program is very likely unrecoverably broken.\n"
                "\n\n  'Better shut her down, Scotty.  She's sucking mud again.'\n");
            nonfatalerrorcount_max_reported = 1; /* don't keep on yakking about too many whatever... */
        }
    }
    return trap_catch;
}



void reset_nonfatalerrorreporting(void)
{
    nonfatalerrorcount = 0;
    nonfatalerrorcount_max_reported = 0;
}



//     print out timings of each statement
void crm_output_profile(CSL_CELL *csl)
{
    int i;

    fprintf(stderr,
        "\n         Execution Profile Results\n");
    fprintf(stderr,
        "\n  Memory usage at completion: %10d window, %10d isolated\n",
        cdw->nchars, tdw->nchars);
    fprintf(stderr,
        "\n  Statement Execution Time Profiling (0 times suppressed)");
    fprintf(stderr,
        "\n  line:      usertime   systemtime    totaltime\n");
    for (i = 0; i < csl->nstmts; i++)
    {
        if (csl->mct[i]->stmt_utime + csl->mct[i]->stmt_stime > 0)
        {
            fprintf(stderr, " %5d:   %10ld   %10ld   %10ld\n",
                i,
                (long int)csl->mct[i]->stmt_utime,
                (long int)csl->mct[i]->stmt_stime,
                (long int)(csl->mct[i]->stmt_utime + csl->mct[i]->stmt_stime));
        }
    }
}



// crm_trigger_fault - whenever there's a fault, this routine Does The
//  Right Thing, in terms of looking up the next TRAP statement and
//  turning control over to it.
//
//  Watch out, this routine must return cleanly with the CSL top set up
//  appropriately so execution can continue at the chosen statement.
//  That next statement executed will be a TRAP statement whose regex
//  matches the fault string.  All other intervening statements will
//  be ignored.
//
//  This routine returns 0 if execution should continue, or 1 if there
//  was no applicable trap to catch the fault and we should take the
//  default fault action (which might be to exit).
//
//  Routines that get here must be careful to NOT trash the constructed
//  csl frame that causes the next statement to be the TRAP statement.
//  In particular, we act like the MATCH and CLASSIFY and FAIL statments
//  and branch to the chosen location -1 (due to the increment in the
//  main invocation loop)

int crm_trigger_fault(char *reason)
{
    //
    //    and if the fault is correctly trapped, this is the fixup.
    //
    char trap_pat[MAX_PATTERN];
    int pat_len;
    regex_t preg;
    int i;
    ARGPARSE_BLOCK apb;
    int slen;
    int done;
    int trapline;
    int original_statement;

    CRM_ASSERT(csl != NULL);

    //  Non null fault_text rstring -we'll take the trap
    if (user_trace)
    {
        fprintf(stderr, "Catching FAULT generated on line %d\n",
            csl->cstmt);
        fprintf(stderr, "FAULT reason:\n%s\n",
            reason);
    }

    original_statement = csl->cstmt;
    trapline = csl->cstmt;

    done = 0;
    while (!done)
    {
        CRM_ASSERT(csl->mct[trapline]->trap_index != trapline);
        if (csl->mct[trapline]->trap_index >= csl->nstmts
            || (trapline = csl->mct[trapline]->trap_index) == -1)
        {
            if (user_trace)
            {
                fprintf(stderr, "     ... no applicable TRAP.\n");
            }
            return 1;
        }

        //      trapline = csl->mct[trapline]->trap_index;
        //
        //        make sure we're really on a trap statement.
        //
        if (csl->mct[trapline]->stmt_type != CRM_TRAP)
            return 1;

        // [i_a] fixup for a trap cycle if the trap statement to be parsed is at fault itself (blowuptrapbugtest.crm)
        csl->cstmt = trapline;

        //       OK, we're here, with a live trap.
        slen = (csl->mct[trapline + 1]->fchar)
               - (csl->mct[trapline]->fchar);

        i = crm_statement_parse(
            &(csl->filetext[csl->mct[trapline]->fchar]),
            slen,
            &apb);
        if (user_trace)
        {
            fprintf(stderr, "Trying trap at line %d:\n", trapline);
#if 0
            {
                int j;

                for (j =  (csl->mct[trapline]->fchar);
                     j < (csl->mct[trapline + 1]->fchar);
                     j++)
                {
                    fprintf(stderr, "%c", csl->filetext[j]);
                }
            }
#else
            fwrite_ASCII_Cfied(stderr,
                csl->filetext + csl->mct[trapline]->fchar,
                csl->mct[trapline + 1]->fchar - csl->mct[trapline]->fchar);
#endif
            fprintf(stderr, "\n");
        }

        //  Get the trap pattern and  see if we match.
        crm_get_pgm_arg(trap_pat, MAX_PATTERN,
            apb.s1start, apb.s1len);
        //
        //      Do variable substitution on the pattern
        pat_len = crm_nexpandvar(trap_pat, apb.s1len, MAX_PATTERN);

        //
        if (user_trace)
        {
            fprintf(stderr, "This TRAP will trap anything matching =%s= .\n",
                trap_pat);
        }
        //       compile the regex
        i = crm_regcomp(&preg, trap_pat, pat_len, REG_EXTENDED);
        if (i == 0)
        {
            i = crm_regexec(&preg,
                reason,
                strlen(reason),
                0, NULL, 0, NULL);
            crm_regfree(&preg);
        }
        else
        {
            crm_regerror(i, &preg, tempbuf, data_window_size);
            // CRM_ASSERT(csl->cstmt == trapline); previous code can have called fatalerror[_ex] multiple times,
            // causing the traphandler line to move forward multiple times. So reassign the traphandler and
            // go from there:
            fatalerror_ex(SRC_LOC(),
                "Regular Expression Compilation Problem in TRAP pattern '%s' while processing the trappable error '%s'",
                tempbuf,
                reason);
            // trapline = csl->cstmt; // [i_a] the call to fatalerror[_ex] will have found a new trapline!
        }


        //    trap_regcomp_error:


        //    if i != 0, we didn't match - kick the error further
        if (i == 0)
        {
            if (user_trace)
            {
                fprintf(stderr, "TRAP matched.\n");
                fprintf(stderr, "Next statement will be %d\n",
                    trapline);
            }
            //
            //   set the next statement to execute to be
            //   the TRAP statement itself.


            // CRM_ASSERT(csl->cstmt == trapline); previous code can have called fatalerror[_ex] multiple times,
            // causing the traphandler line to move forward multiple times. So reassign the traphandler and
            // go from there:
            csl->cstmt = trapline;
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = 1;
            //
            //     If there's a trap variable, modify it.
            {
                char reasonname[MAX_VARNAME];
                int rnlen;
                crm_get_pgm_arg(reasonname, MAX_VARNAME, apb.p1start, apb.p1len);
                if (apb.p1len > 2)
                {
                    rnlen = crm_nexpandvar(reasonname, apb.p1len, MAX_VARNAME);
                    //   crm_nexpandvar null-terminates for us so we can be
                    //   8-bit-unclean here
                    crm_set_temp_nvar(reasonname,
                        reason,
                        strlen(reason));
                }
                done = 1;
            }
        }
        else
        {
            //   The trap pattern didn't match - move outward to
            //   the surrounding trap (if it exists)
            if (user_trace)
            {
                fprintf(stderr,
                    "TRAP didn't match - trying next trap in line.\n");
            }
        }
        //      and note that we haven't set "done" == 1 yet, so
        //      we will go through the loop again.
    }
    return 0;
}


