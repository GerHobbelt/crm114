//  crm114_.c  - Controllable Regex Mutilator,  version v1.0
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




//         If we got to here, we need to run some user-interfacing
//         (i.e. debugging).
//
//         possible return values:
//         1: reparse and continue execution
//         -1: exit immediately
//         0: continue

long crm_debugger(void)
{
    long ichar;
    static int firsttime = 1;
    static FILE *mytty;

    if (firsttime)
    {
        fprintf(crm_stderr, "CRM114 Debugger - type \"h\" for help.  ");
        fprintf(crm_stderr, "User trace turned on.\n");
        user_trace = 1;
        firsttime = 0;
        if (user_trace)
            fprintf(crm_stderr, "Opening the user terminal for debugging I/O\n");
#if defined (WIN32)
        mytty = fopen("CON", "rb");
#else
        mytty = fopen("/dev/tty", "rb");
#endif
        clearerr(mytty);
    }

    if (csl->mct[csl->cstmt]->stmt_break > 0)
        fprintf(crm_stderr, "Breakpoint tripped at statement %ld\n", csl->cstmt);
debug_top:
    //    let the user know they're in the debugger
    //
    fprintf(crm_stderr, "\ncrm-dbg> ");

    //    find out what they want to do
    //
    ichar = 0;

    while (!feof(mytty)
           && ichar < data_window_size - 1
           && (inbuf[ichar - 1] != '\n'))
    {
        inbuf[ichar] = fgetc(mytty);
        ichar++;
    }
    inbuf[ichar] = '\000';

    if (feof(mytty))
    {
        fprintf(crm_stderr, "Quitting\n");
        if (engine_exit_base != 0)
        {
            exit(engine_exit_base + 8);
        }
        else
            exit(EXIT_SUCCESS);
    }


    //   now a big siwtch statement on the first character of the command
    //
    switch (inbuf[0])
    {
    case 'q':
    case 'Q':
        {
            if (user_trace)
                fprintf(crm_stderr, "Quitting.\n");
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 9);
            }
            else
                exit(EXIT_SUCCESS);
        }
        break;

    case 'n':
    case 'N':
        {
            debug_countdown = 0;
            return 0;
        }
        break;

    case 'c':
    case 'C':
        {
            debug_countdown = 0;
            if (1 != sscanf(&inbuf[1], "%ld", &debug_countdown))
            {
                fprintf(crm_stderr, "Failed to decode the debug 'C' "
                                "command countdown number '%s'. "
                                "Assume 0: 'continue'.\n", &inbuf[1]);
            }
            if (debug_countdown <= 0)
            {
                debug_countdown = -1;
                fprintf(crm_stderr, "continuing execution...\n");
            }
            else
            {
                fprintf(crm_stderr, "continuing %ld cycles...\n", debug_countdown);
            }
            return 0;
        }
        break;

    case 't':
        if (user_trace == 0)
        {
            user_trace = 1;
            fprintf(crm_stderr, "User tracing on");
        }
        else
        {
            user_trace = 0;
            fprintf(crm_stderr, "User tracing off");
        }
        break;

    case 'T':
        if (internal_trace == 0)
        {
            internal_trace = 1;
            fprintf(crm_stderr, "Internal tracing on");
        }
        else
        {
            internal_trace = 0;
            fprintf(crm_stderr, "Internal tracing off");
        }
        break;

    case 'e':
        {
            fprintf(crm_stderr, "expanding:\n");
            memmove(inbuf, &inbuf[1], strlen(inbuf) - 1);
            crm_nexpandvar(inbuf, strlen(inbuf) - 1, data_window_size);
            fprintf(crm_stderr, "%s", inbuf);
        }
        break;

    case 'i':
        {
            fprintf(crm_stderr, "Isolating %s", &inbuf[1]);
            fprintf(crm_stderr, "NOT YET IMPLEMENTED!  Sorry.\n");
        }
        break;

    case 'v':
        {
            long i, j;
            long stmtnum;
            i = sscanf(&inbuf[1], "%ld", &stmtnum);
            if (i > 0)
            {
                fprintf(crm_stderr, "statement %ld: ", stmtnum);
                if (stmtnum < 0 || stmtnum > csl->nstmts)
                {
                    fprintf(crm_stderr, "... does not exist!\n");
                }
                else
                {
                    for (j = csl->mct[stmtnum]->start;
                         j < csl->mct[stmtnum + 1]->start;
                         j++)
                    {
                        fprintf(crm_stderr, "%c", csl->filetext[j]);
                    }
                }
            }
            else
            {
                fprintf(crm_stderr, "What statement do you want to view?\n");
            }
        }
        break;

    case 'j':
        {
            long nextstmt;
            long i;
            long vindex;
            i = sscanf(&inbuf[1], "%ld", &nextstmt);
            if (i == 0)
            {
                //    maybe the user put in a label?
                long tstart;
                long tlen;
                crm_nextword(&inbuf[1], strlen(&inbuf[1]), 0,
                             &tstart, &tlen);
                memmove(inbuf, &inbuf[1 + tstart], tlen);
                inbuf[tlen] = '\000';
                vindex = crm_vht_lookup(vht, inbuf, tlen);
                if (vht[vindex] == NULL)
                {
                    fprintf(crm_stderr, "No label '%s' in this program.  ", inbuf);
                    fprintf(crm_stderr, "Staying at line %ld\n", csl->cstmt);
                    nextstmt = csl->cstmt;
                }
                else
                {
                    nextstmt = vht[vindex]->linenumber;
                }
            }
            if (nextstmt <= 0)
            {
                nextstmt = 0;
            }
            if (nextstmt >= csl->nstmts)
            {
                nextstmt = csl->nstmts;
                fprintf(crm_stderr, "last statement is %ld, assume you meant that.\n",
                        csl->nstmts);
            }
            if (csl->cstmt != nextstmt)
                fprintf(crm_stderr, "Next statement is statement %ld\n", nextstmt);
            csl->cstmt = nextstmt;
        }
        return 1;

        break;

    case 'b':
        {
            //  is there a breakpoint to make?
            long breakstmt;
            long i;
            long vindex;
            breakstmt = -1;
            i = sscanf(&inbuf[1], "%ld", &breakstmt);
            if (i == 0)
            {
                //    maybe the user put in a label?
                long tstart;
                long tlen;
                crm_nextword(&inbuf[1], strlen(&inbuf[1]), 0,
                             &tstart, &tlen);
                memmove(inbuf, &inbuf[1 + tstart], tlen);
                inbuf[tlen] = '\000';
                vindex = crm_vht_lookup(vht, inbuf, tlen);
                fprintf(crm_stderr, "vindex = %ld\n", vindex);
                if (vht[vindex] == NULL)
                {
                    fprintf(crm_stderr, "No label '%s' in this program.  ", inbuf);
                    fprintf(crm_stderr, "No breakpoint change made.\n");
                }
                else
                {
                    breakstmt = vht[vindex]->linenumber;
                }
            }
            if (breakstmt  <= -1)
            {
                breakstmt = 0;
            }
            if (breakstmt >= csl->nstmts)
            {
                breakstmt = csl->nstmts;
                fprintf(crm_stderr, "last statement is %ld, assume you meant that.\n",
                        csl->nstmts);
            }
            csl->mct[breakstmt]->stmt_break = 1 - csl->mct[breakstmt]->stmt_break;
            if (csl->mct[breakstmt]->stmt_break == 1)
            {
                fprintf(crm_stderr, "Setting breakpoint at statement %ld\n",
                        breakstmt);
            }
            else
            {
                fprintf(crm_stderr, "Clearing breakpoint at statement %ld\n",
                        breakstmt);
            }
        }
        return 1;

        break;

    case 'a':
        {
            //  do a debugger-level alteration
            //    maybe the user put in a label?
            long vstart, vlen;
            long vindex;
            long ostart, oend, olen;
            crm_nextword(&inbuf[1], strlen(&inbuf[1]), 0,
                         &vstart, &vlen);
            memmove(inbuf, &inbuf[1 + vstart], vlen);
            inbuf[vlen] = '\000';
            vindex = crm_vht_lookup(vht, inbuf, vlen);
            if (vht[vindex] == NULL)
            {
                fprintf(crm_stderr, "No variable '%s' in this program.  ", inbuf);
            }

            //     now grab what's left of the input as the value to set
            //
            ostart = vlen + 1;
            while (inbuf[ostart] != '/' && inbuf[ostart] != '\000') ostart++;
            ostart++;
            oend = ostart + 1;
            while (inbuf[oend] != '/' && inbuf[oend] != '\000') oend++;

            memmove(outbuf,
                    &inbuf[ostart],
                    oend - ostart);

            outbuf[oend - ostart] = '\000';
            olen = crm_nexpandvar(outbuf, oend - ostart, data_window_size);
            crm_destructive_alter_nvariable(inbuf, vlen, outbuf, olen);
        }
        break;

    case 'f':
        {
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            fprintf(crm_stderr, "Forward to }, next statement : %ld\n", csl->cstmt);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return 1;

        break;

    case 'l':
        {
            csl->cstmt = csl->mct[csl->cstmt]->liaf_index;
            fprintf(crm_stderr, "Backward to {, next statement : %ld\n", csl->cstmt);
        }
        return 1;

        break;

    case 'h':
        {
            fprintf(crm_stderr, "a :var: /value/ - alter :var: to /value/\n");
            fprintf(crm_stderr, "b <n> - toggle breakpoint on line <n>\n");
            fprintf(crm_stderr, "b <label> - toggle breakpoint on <label>\n");
            fprintf(crm_stderr, "c - continue execution till breakpoint or end\n");
            fprintf(crm_stderr, "c <n> - execute <number> more statements\n");
            fprintf(crm_stderr, "e - expand an expression\n");
            fprintf(crm_stderr, "f - fail forward to block-closing '}'\n");
            fprintf(crm_stderr, "j <n> - jump to statement <number>\n");
            fprintf(crm_stderr, "j <label> - jump to statement <label>\n");
            fprintf(crm_stderr, "l - liaf backward to block-opening '{'\n");
            fprintf(crm_stderr, "n - execute next statement (same as 'c 1')\n");
            fprintf(crm_stderr, "q - quit the program and exit\n");
            fprintf(crm_stderr, "t - toggle user-level tracing\n");
            fprintf(crm_stderr, "T - toggle system-level tracing\n");
            fprintf(crm_stderr, "v <n> - view source code statement <n>\n");
        }
        break;

    default:
        {
            fprintf(crm_stderr, "Command unrecognized - type \"h\" for help.\n");
        }
    }
    goto debug_top;
}

