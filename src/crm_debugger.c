//  crm114_.c  - Controllable Regex Mutilator,  version v1.0
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



static FILE *mytty = NULL;
static char **show_expr_list = NULL;
static char *last_e_expression = NULL;
static char *show_expr_buffer = NULL;


void free_debugger_data(void)
{
    int j;

    if (show_expr_buffer)
    {
        free(show_expr_buffer);
        show_expr_buffer = NULL;
    }
    if (show_expr_list)
    {
        for (j = 0; show_expr_list[j]; j++)
        {
            free(show_expr_list[j]);
        }
        free(show_expr_list);
        show_expr_list = NULL;
    }
    if (last_e_expression)
    {
        free(last_e_expression);
        last_e_expression = NULL;
    }
    if (mytty)
    {
        fclose(mytty);
        mytty = NULL;
    }
}


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

    if (!mytty)
    {
        fprintf(stderr, "CRM114 Debugger - type \"h\" for help.  ");
        fprintf(stderr, "User trace turned on.\n");
        user_trace = 1;
        if (user_trace)
            fprintf(stderr, "Opening the user terminal for debugging I/O\n");
#if defined (WIN32)
        mytty = fopen("CON", "rb");
#else
        mytty = fopen("/dev/tty", "rb");
#endif
        clearerr(mytty);
    }
    if (!show_expr_list)
    {
        show_expr_list = calloc(1, sizeof(show_expr_list[0]));
        if (!show_expr_list)
        {
            untrappableerror("Cannot allocate debugger expression show list", "");
        }
        show_expr_list[0] = NULL;
    }

    if (csl->mct[csl->cstmt]->stmt_break > 0)
        fprintf(stderr, "Breakpoint tripped at statement %ld\n", csl->cstmt);

    for ( ; ;)
    {
        // show watched expressions:
        int watch;

        if (show_expr_list && show_expr_list[0])
        {
            if (!show_expr_buffer)
            {
                show_expr_buffer = calloc(data_window_size, sizeof(show_expr_buffer[0]));
                if (!show_expr_buffer)
                {
                    untrappableerror("cannot allocate buffer to display watched expressions.", "");
                }
            }

            for (watch = 0; show_expr_list[watch]; watch++)
            {
                strcpy(show_expr_buffer, show_expr_list[watch]);
                crm_nexpandvar(show_expr_buffer, strlen(show_expr_buffer) - 1, data_window_size);

                fprintf(stderr, "[#%2d]: '%s' /%s/\n", watch + 1, show_expr_list[watch], show_expr_buffer);
            }
        }

        //    let the user know they're in the debugger
        //
        fprintf(stderr, "\ncrm-dbg> ");

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
        inbuf[ichar] = 0;

        if (feof(mytty))
        {
            fprintf(stderr, "Quitting\n");
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 8);
            }
            else
            {
                exit(EXIT_SUCCESS);
            }
        }


        //   now a big switch statement on the first character of the command
        //
        switch (inbuf[0])
        {
        case 'q':
        case 'Q':
            if (user_trace)
                fprintf(stderr, "Quitting.\n");
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 9);
            }
            else
            {
                exit(EXIT_SUCCESS);
            }
            break;

        case 'n':
        case 'N':
            debug_countdown = 0;
            return 0;

        case 'c':
        case 'C':
            debug_countdown = 0;
            if (1 != sscanf(&inbuf[1], "%d", &debug_countdown))
            {
                fprintf(stderr, "Failed to decode the debug 'C' "
                                "command countdown number '%s'. "
                                "Assume 0: 'continue'.\n", &inbuf[1]);
            }
            if (debug_countdown <= 0)
            {
                debug_countdown = -1;
                fprintf(stderr, "continuing execution...\n");
            }
            else
            {
                fprintf(stderr, "continuing %d cycles...\n", debug_countdown);
            }
            return 0;

        case 't':
            if (user_trace == 0)
            {
                user_trace = 1;
                fprintf(stderr, "User tracing on");
            }
            else
            {
                user_trace = 0;
                fprintf(stderr, "User tracing off");
            }
            break;

        case 'T':
            if (internal_trace == 0)
            {
                internal_trace = 1;
                fprintf(stderr, "Internal tracing on");
            }
            else
            {
                internal_trace = 0;
                fprintf(stderr, "Internal tracing off");
            }
            break;

        case 'e':
            fprintf(stderr, "expanding:\n");
            memmove(inbuf, &inbuf[1], strlen(inbuf) - 1);
            if (last_e_expression)
                free(last_e_expression);
            last_e_expression = strdup(inbuf);
            crm_nexpandvar(inbuf, strlen(inbuf) - 1, data_window_size);
            fprintf(stderr, "%s", inbuf);
            break;

        case '+':
            // add expression to watch list

            // check if an expression has been specified; if none, re-use the last e expression if _that_ one exists.
            memmove(inbuf, &inbuf[1], strlen(inbuf) - 1);
            if (inbuf[strspn(inbuf, " \t\r\n")])
            {
                if (last_e_expression)
                    free(last_e_expression);
                last_e_expression = strdup(inbuf);
            }

            if (last_e_expression)
            {
                int j;

                // make sure the expression isn't in the list already:
                for (j = 0; show_expr_list[j]; j++)
                {
                    if (strcmp(show_expr_list[j], last_e_expression) == 0)
                        break;
                }
                // add to list if not already in it:
                if (show_expr_list[j] == NULL)
                {
                    show_expr_list = realloc(show_expr_list, (j + 2) * sizeof(show_expr_list[0]));
                    show_expr_list[j] = strdup(last_e_expression);
                    show_expr_list[j + 1] = NULL;

                    fprintf(stderr, "'e' expression added to the watch list:\n"
                                    "    %s\n",
                            last_e_expression);
                }
                else
                {
                    fprintf(stderr,
                            "expression not added to watch list: expression already exists in watch list!\n");
                }
            }
            else
            {
                fprintf(stderr, "no 'e' expression specified: no expression added to the watch list\n");
            }
            break;

        case '-':
            // remove expression #n from watch list
            {
                int j;
                int expr_number = 0;

                j = sscanf(&inbuf[1], "%d", &expr_number);
                if (j == 0)
                {
                    // assume 'remove all':
                    for (j = 0; show_expr_list[j]; j++)
                    {
                        free(show_expr_list[j]);
                    }
                    show_expr_list[0] = NULL;

                    fprintf(stderr, "removed all watched expressions\n");
                }
                else
                {
                    // remove expression 'expr_number'
                    expr_number--;             // remember the number starts at 1!
                    if (expr_number >= 0 && show_expr_list[0] != NULL)
                    {
                        for (j = 0; show_expr_list[j]; j++)
                        {
                            if (j == expr_number)
                            {
                                fprintf(stderr, "removed watched expression #%d: %s\n",
                                        j + 1, show_expr_list[j]);
                                free(show_expr_list[j]);
                            }
                            // shift other expressions one down in the list to close the gap:
                            if (j >= expr_number)
                            {
                                show_expr_list[j] = show_expr_list[j + 1];
                            }
                        }
                        // barf if user supplied a bogus expression #
                        if (j <= expr_number)
                        {
                            fprintf(stderr,
                                    "cannot remove watched expression #%d as there are only %d expressions.\n",
                                    expr_number + 1,
                                    j);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "You specified an illegal watched expression #%d; command ignored.\n",
                                expr_number + 1);
                    }
                }
            }
            break;

        case 'i':
            fprintf(stderr, "Isolating %s", &inbuf[1]);
            fprintf(stderr, "NOT YET IMPLEMENTED!  Sorry.\n");
            break;

        case 'v':
            {
                int i, j;
                int stmtnum;
                int endstmtnum;

                i = sscanf(&inbuf[1], " %d.%d", &stmtnum, &endstmtnum);
                if (i <= 0)
                {
                    stmtnum = csl->cstmt;

                    endstmtnum = stmtnum;

                    // +N = show current + N subsequent statements
                    i = sscanf(&inbuf[1], " >%d", &j);
                    if (i == 1)
                    {
                        endstmtnum = stmtnum + j;

                        // sanity check: do not print beyond end of statement range; no need
                        // to report an error message in the loop below...
                        if (endstmtnum > csl->nstmts)
                        {
                            endstmtnum = csl->nstmts;
                        }
                    }
                    else
                    {
                        // ~N: show a 'context' of +/- N statements around the current statement
                        i = sscanf(&inbuf[1], " ~%d", &j);
                        if (i == 1)
                        {
                            endstmtnum = stmtnum + j;
                            stmtnum -= j;

                            // sanity check: do not print beyond end of statement range; no need
                            // to report an error message in the loop below...
                            if (endstmtnum > csl->nstmts)
                            {
                                endstmtnum = csl->nstmts;
                            }
                            if (stmtnum < 0)
                            {
                                stmtnum = 0;
                            }
                        }
                    }
                }
                else if (i == 1)
                {
                    endstmtnum = stmtnum;
                }
                else if (i == 2)
                {
                    // sanity check: do not print beyond end of statement range; no need
                    // to report an error message in the loop below...
                    if (endstmtnum > csl->nstmts)
                    {
                        endstmtnum = csl->nstmts;
                    }
                }

                for ( ; stmtnum <= endstmtnum; stmtnum++)
                {
                    fprintf(stderr, "statement %d: ", stmtnum);
                    if (stmtnum < 0 || stmtnum > csl->nstmts)
                    {
                        fprintf(stderr, "... does not exist!\n");
                        break;
                    }
                    else
                    {
#if 0
                        for (j = csl->mct[stmtnum]->start;
                             j < csl->mct[stmtnum + 1]->start;
                             j++)
                        {
                            fprintf(stderr, "%c", csl->filetext[j]);
                        }
#else
                        int stmt_len;

                        j = csl->mct[stmtnum]->start;
                        stmt_len = csl->mct[stmtnum + 1]->start - j;
                        if (stmt_len > 0)
                        {
                            fprintf(stderr, "%.*s", stmt_len, &csl->filetext[j]);
                        }
#endif
                    }
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
                    int tstart;
                    int tlen;
                    crm_nextword(&inbuf[1], strlen(&inbuf[1]), 0,
                            &tstart, &tlen);
                    memmove(inbuf, &inbuf[1 + tstart], tlen);
                    inbuf[tlen] = 0;
                    vindex = crm_vht_lookup(vht, inbuf, tlen);
                    if (vht[vindex] == NULL)
                    {
                        fprintf(stderr, "No label '%s' in this program.  ", inbuf);
                        fprintf(stderr, "Staying at line %ld\n", csl->cstmt);
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
                    fprintf(stderr, "last statement is %ld, assume you meant that.\n",
                            csl->nstmts);
                }
                if (csl->cstmt != nextstmt)
                {
                    fprintf(stderr, "Next statement is statement %ld\n", nextstmt);
                }
                csl->cstmt = nextstmt;
            }
            return 1;

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
                    int tstart;
                    int tlen;
                    crm_nextword(&inbuf[1], strlen(&inbuf[1]), 0,
                            &tstart, &tlen);
                    memmove(inbuf, &inbuf[1 + tstart], tlen);
                    inbuf[tlen] = 0;
                    vindex = crm_vht_lookup(vht, inbuf, tlen);
                    fprintf(stderr, "vindex = %ld\n", vindex);
                    if (vht[vindex] == NULL)
                    {
                        fprintf(stderr, "No label '%s' in this program.  ", inbuf);
                        fprintf(stderr, "No breakpoint change made.\n");
                    }
                    else
                    {
                        breakstmt = vht[vindex]->linenumber;
                    }
                }
                if (breakstmt <= -1)
                {
                    breakstmt = 0;
                }
                if (breakstmt >= csl->nstmts)
                {
                    breakstmt = csl->nstmts;
                    fprintf(stderr, "last statement is %ld, assume you meant that.\n",
                            csl->nstmts);
                }
                csl->mct[breakstmt]->stmt_break = 1 - csl->mct[breakstmt]->stmt_break;
                if (csl->mct[breakstmt]->stmt_break == 1)
                {
                    fprintf(stderr, "Setting breakpoint at statement %ld\n",
                            breakstmt);
                }
                else
                {
                    fprintf(stderr, "Clearing breakpoint at statement %ld\n",
                            breakstmt);
                }
            }
            return 1;

        case 'a':
            {
                //  do a debugger-level alteration
                //    maybe the user put in a label?
                int vstart, vlen;
                int vindex;
                int ostart, oend, olen;
                crm_nextword(&inbuf[1], strlen(&inbuf[1]), 0,
                        &vstart, &vlen);
                memmove(inbuf, &inbuf[1 + vstart], vlen);
                inbuf[vlen] = 0;
                vindex = crm_vht_lookup(vht, inbuf, vlen);
                if (vht[vindex] == NULL)
                {
                    fprintf(stderr, "No variable '%s' in this program.  ", inbuf);
                }

                //     now grab what's left of the input as the value to set
                //
                ostart = vlen + 1;
                while (inbuf[ostart] != '/' && inbuf[ostart] != 0)
                    ostart++;
                ostart++;
                oend = ostart + 1;
                while (inbuf[oend] != '/' && inbuf[oend] != 0)
                    oend++;

                memmove(outbuf,
                        &inbuf[ostart],
                        oend - ostart);

                outbuf[oend - ostart] = 0;
                olen = crm_nexpandvar(outbuf, oend - ostart, data_window_size);
                crm_destructive_alter_nvariable(inbuf, vlen, outbuf, olen);
            }
            break;

        case 'f':
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            fprintf(stderr, "Forward to }, next statement : %ld\n", csl->cstmt);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;

            return 1;

        case 'l':
            csl->cstmt = csl->mct[csl->cstmt]->liaf_index;
            fprintf(stderr, "Backward to {, next statement : %ld\n", csl->cstmt);

            return 1;

        case 'h':
        case '?':
            fprintf(stderr, "a :var: /value/ - alter :var: to /value/\n");
            fprintf(stderr, "b <n> - toggle breakpoint on line <n>\n");
            fprintf(stderr, "b <label> - toggle breakpoint on <label>\n");
            fprintf(stderr, "c     - continue execution till breakpoint or end\n");
            fprintf(stderr, "c <n> - execute <number> more statements\n");
            fprintf(stderr, "e     - expand an expression\n");
            fprintf(stderr, "+     - watch the expanded expression\n"
                            "        (no arg: add last 'e' expr to show list)\n");
            fprintf(stderr, "- <n> - hide the watched expression #<n> from the show list.\n"
                            "        No <n> given: hide all\n");
            fprintf(stderr, "f     - fail forward to block-closing '}'\n");
            fprintf(stderr, "j <n> - jump to statement <number>\n");
            fprintf(stderr, "j <label> - jump to statement <label>\n");
            fprintf(stderr, "l     - liaf backward to block-opening '{'\n");
            fprintf(stderr, "n     - execute next statement (same as 'c 1')\n");
            fprintf(stderr, "q     - quit the program and exit\n");
            fprintf(stderr, "t     - toggle user-level tracing\n");
            fprintf(stderr, "T     - toggle system-level tracing\n");
            fprintf(stderr, "v <n>.<m> - view source code statement <n> till <m>.\n"
                            "        No <n> given: show current statement\n"
                            "        Alternatives: 'v >5' (type '>' char!) = show\n"
                            "        current and 5 extra; 'v ~3' = show context of\n"
                            "        3 lines before till 3 lines after\n");
            break;

        default:
            fprintf(stderr, "Command unrecognized - type \"h\" for help.\n");
            break;
        }
    }
}

