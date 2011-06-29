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
static char *dbg_inbuf = NULL;
static char *dbg_outbuf = NULL;
static int dbg_iobuf_size = MAX_PATTERN + MAX_VARNAME + 256;
static char *dbg_last_command = NULL;


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
    if (dbg_last_command)
    {
        free(dbg_last_command);
        dbg_last_command = NULL;
    }
    if (mytty)
    {
        fclose(mytty);
        mytty = NULL;
    }
    if (dbg_inbuf)
    {
        free(dbg_inbuf);
        dbg_inbuf = NULL;
    }
    if (dbg_outbuf)
    {
        free(dbg_outbuf);
        dbg_outbuf = NULL;
    }
}



//         If we got to here, we need to run some user-interfacing
//         (i.e. debugging).
//
//         possible return values:
//         1: reparse and continue execution
//         -1: exit immediately
//         0: continue

int crm_debugger(void)
{
    int ichar;
    int ret_code = 0;
    int parsing_done = 0;

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
    if (!dbg_inbuf)
    {
        dbg_inbuf = calloc(dbg_iobuf_size, sizeof(dbg_inbuf[0]));
        if (!dbg_inbuf)
        {
            untrappableerror("Cannot allocate debugger input buffer", "");
        }
        dbg_inbuf[0] = 0;

        dbg_outbuf = calloc(dbg_iobuf_size, sizeof(dbg_outbuf[0]));
        if (!dbg_outbuf)
        {
            untrappableerror("Cannot allocate debugger output buffer", "");
        }
        dbg_outbuf[0] = 0;

        dbg_last_command = calloc(dbg_iobuf_size, sizeof(dbg_last_command[0]));
        if (!dbg_last_command)
        {
            untrappableerror("Cannot allocate debugger input recall buffer", "");
        }
        dbg_last_command[0] = 0;
    }
    CRM_ASSERT(dbg_outbuf != NULL);
    CRM_ASSERT(dbg_last_command != NULL);

    if (csl->mct[csl->cstmt]->stmt_break)
        fprintf(stderr, "Breakpoint tripped at statement %d\n", csl->cstmt);

    for ( ; !parsing_done;)
    {
        // show watched expressions:
        int watch;
        int cmd_done_len = 1;
        int remember_cmd = 0;

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
        //    if there's still a command waiting in the buffer, handle that one first.
        if (!dbg_inbuf[0])
        {
            ichar = 0;

            while (!feof(mytty)
                   && ichar < dbg_iobuf_size - 1
                   && (dbg_inbuf[ichar - 1] != '\n'))
            {
                dbg_inbuf[ichar] = fgetc(mytty);
                ichar++;
            }
            dbg_inbuf[ichar] = 0;

            remember_cmd = 1;

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
        }


        //   now a big switch statement on the first character of the command
        //
        switch (dbg_inbuf[0])
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

        case ';':
            // act as optional command seperator for commands which use arguments, when these are used in a command sequence
            break;

        case '.':
            // repeat last run command
            //
            // Note that if the previous command was a line containing mutiple commands,
            // EACH of those is executed again!
            {
                char *dst = dbg_inbuf + 1;
                int len = strlen(dst);

                ichar = dbg_iobuf_size - 1;
                ichar -= len;
                dst += len;
                snprintf(dst, ichar, "\n%s", dbg_last_command);
                dbg_inbuf[dbg_iobuf_size - 1] = 0;
            }
            //
            // disable storing this new command buffer to prevent infinite loops
            //
            // CAVEAT: note however, that a commandline like this:
            //
            //   n . c
            //
            // WILL cause the complete line to be duplicated once the '.' is hit, to look
            // like this:
            //
            //   . c n . c
            //
            // thus introducing another '.' automagically, leading to an infinitely
            // repeating pattern (or until the complete CRM script has been executed).
            //
            // Though this is at least 'sneaky', it IS allowed, as I _like_ sneaky like this ;-)
            //
            // -- Ger '[i_a]' Hobbelt
            //
            remember_cmd = 0;
            break;

        case 'n':
        case 'N':
            debug_countdown = 0;

            ret_code = 0;
            parsing_done = 1;
            break;

        case 'c':
        case 'C':
            debug_countdown = 1;
            {
                char *end = NULL;

                debug_countdown = strtol(dbg_inbuf + 1, &end, 0);
                if (end == dbg_inbuf + 1)
                {
                    fprintf(stderr, "Failed to decode the debug 'C' "
                                    "command countdown number '%s'. "
                                    "Assume 0: 'continue'.\n", &dbg_inbuf[1]);
                    debug_countdown = 0;
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

                cmd_done_len = end - dbg_inbuf;
            }
            CRM_ASSERT(cmd_done_len > 0);
            ret_code = 0;
            parsing_done = 1;
            break;

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
                    //
                    // GROT GROT GROT
                    //
                    // when an arg is given, it MAY contain semicolons
                    // That means this command can NOT be used in a command sequence!
			//
            if (dbg_inbuf[strspn(dbg_inbuf, " \t\r\n")])
            {
                if (last_e_expression)
                    free(last_e_expression);
                last_e_expression = strdup(dbg_inbuf + 1);
				last_e_expression[strcspn(last_e_expression, "\r\n")] = 0;

                cmd_done_len = strcspn(dbg_inbuf, "\r\n");
            }

			if (last_e_expression)
			{
                    strcpy(dbg_outbuf, last_e_expression);

                    crm_nexpandvar(dbg_outbuf, strlen(dbg_outbuf), dbg_iobuf_size);

                fprintf(stderr, "expression '%s' ==>\n%s", last_e_expression, dbg_outbuf);
}
			else
            {
				fprintf(stderr, "no 'e' expression specified: not now and never before\n");
            }
            break;

        case '+':
            // add expression to watch list

            // check if an expression has been specified; if none, re-use the last e expression if _that_ one exists.
                    //
                    // GROT GROT GROT
                    //
                    // when an arg is given, it MAY contain semicolons
                    // That means this command can NOT be used in a command sequence!
            if (dbg_inbuf[strspn(dbg_inbuf, " \t\r\n")])
            {
                if (last_e_expression)
                    free(last_e_expression);
                last_e_expression = strdup(dbg_inbuf + 1);
				last_e_expression[strcspn(last_e_expression, "\r\n")] = 0;

                cmd_done_len = strcspn(dbg_inbuf, "\r\n");
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

                j = sscanf(&dbg_inbuf[1], "%d", &expr_number);
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
            fprintf(stderr, "Isolating %s", &dbg_inbuf[1]);
            fprintf(stderr, "NOT YET IMPLEMENTED!  Sorry.\n");
            break;

        case 'v':
            {
                int i, j;
                int stmtnum;
                int endstmtnum;

                i = sscanf(&dbg_inbuf[1], " %d.%d", &stmtnum, &endstmtnum);
                if (i <= 0)
                {
                    stmtnum = csl->cstmt;

                    endstmtnum = stmtnum;

                    // +N = show current + N subsequent statements
                    i = sscanf(&dbg_inbuf[1], " >%d", &j);
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
                        i = sscanf(&dbg_inbuf[1], " ~%d", &j);
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
                        memnCdump(stderr,
                            csl->filetext + j,
                            stmt_len);
#endif
                    }
                }
            }
            break;

        case 'j':
            {
                int nextstmt;
                int i;
                int vindex;
                i = sscanf(&dbg_inbuf[1], "%d", &nextstmt);
                if (i == 0)
                {
                    //    maybe the user put in a label?
                    int tstart;
                    int tlen;
                    crm_nextword(&dbg_inbuf[1], strlen(&dbg_inbuf[1]), 0,
                        &tstart, &tlen);
                    memmove(dbg_inbuf, &dbg_inbuf[1 + tstart], tlen);
                    dbg_inbuf[tlen] = 0;
                    vindex = crm_vht_lookup(vht, dbg_inbuf, tlen);
                    if (vht[vindex] == NULL)
                    {
                        fprintf(stderr, "No label '%s' in this program.  ", dbg_inbuf);
                        fprintf(stderr, "Staying at line %d\n", csl->cstmt);
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
                    fprintf(stderr, "last statement is %d, assume you meant that.\n",
                        csl->nstmts);
                }
                if (csl->cstmt != nextstmt)
                {
                    fprintf(stderr, "Next statement is statement %d\n", nextstmt);
                }
                csl->cstmt = nextstmt;
            }
            ret_code = 1;
            parsing_done = 1;
            break;

        case 'b':
            {
                //  is there a breakpoint to make?
                int breakstmt;
                int i;
                int vindex;
                breakstmt = -1;
                i = sscanf(&dbg_inbuf[1], "%d", &breakstmt);
                if (i == 0)
                {
                    //    maybe the user put in a label?
                    int tstart;
                    int tlen;
                    crm_nextword(&dbg_inbuf[1], strlen(&dbg_inbuf[1]), 0,
                        &tstart, &tlen);
                    memmove(dbg_inbuf, &dbg_inbuf[1 + tstart], tlen);
                    dbg_inbuf[tlen] = 0;
                    vindex = crm_vht_lookup(vht, dbg_inbuf, tlen);
                    fprintf(stderr, "vindex = %d\n", vindex);
                    if (vht[vindex] == NULL)
                    {
                        fprintf(stderr, "No label '%s' in this program.  ", dbg_inbuf);
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
                    fprintf(stderr, "last statement is %d, assume you meant that.\n",
                        csl->nstmts);
                }
                csl->mct[breakstmt]->stmt_break = !csl->mct[breakstmt]->stmt_break;
                if (csl->mct[breakstmt]->stmt_break)
                {
                    fprintf(stderr, "Setting breakpoint at statement %d\n",
                        breakstmt);
                }
                else
                {
                    fprintf(stderr, "Clearing breakpoint at statement %d\n",
                        breakstmt);
                }
            }
            ret_code = 1;
            parsing_done = 1;
            break;

        case 'a':
            {
                //  do a debugger-level alteration
                //    maybe the user put in a label?
                int vstart, vlen;
                int vindex;
                int ostart, oend, olen;
                crm_nextword(&dbg_inbuf[1], strlen(&dbg_inbuf[1]), 0,
                    &vstart, &vlen);
                memmove(dbg_inbuf, &dbg_inbuf[1 + vstart], vlen);
                dbg_inbuf[vlen] = 0;
                vindex = crm_vht_lookup(vht, dbg_inbuf, vlen);
                if (vht[vindex] == NULL)
                {
                    fprintf(stderr, "No variable '%s' in this program.  ", dbg_inbuf);
                }

                //     now grab what's left of the input as the value to set
                //
                ostart = vlen + 1;
                while (dbg_inbuf[ostart] != '/' && dbg_inbuf[ostart] != 0)
                    ostart++;
                ostart++;
                oend = ostart + 1;
                while (dbg_inbuf[oend] != '/' && dbg_inbuf[oend] != 0)
                    oend++;

                CRM_ASSERT(oend - ostart < dbg_iobuf_size - 1);
                memmove(dbg_outbuf,
                    &dbg_inbuf[ostart],
                    oend - ostart);

                dbg_outbuf[oend - ostart] = 0;
                olen = crm_nexpandvar(dbg_outbuf, oend - ostart, dbg_iobuf_size);
                crm_destructive_alter_nvariable(dbg_inbuf, vlen, dbg_outbuf, olen);
            }
            break;

        case 'f':
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            fprintf(stderr, "Forward to }, next statement : %d\n", csl->cstmt);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;

            ret_code = 1;
            parsing_done = 1;
            break;

        case 'l':
            csl->cstmt = csl->mct[csl->cstmt]->liaf_index;
            fprintf(stderr, "Backward to {, next statement : %d\n", csl->cstmt);

            ret_code = 1;
            parsing_done = 1;
            break;

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
            {
                // skip whitespace:
                int len = strspn(dbg_inbuf, " \t\r\n");

                cmd_done_len = len;
                if (cmd_done_len == 0)
                {
                    fprintf(stderr, "Command '%c' unrecognized - type \"h\" for help.\n", dbg_inbuf[0]);

                    // skip until ';' seperator or newline:
                    cmd_done_len = strcspn(dbg_inbuf, ";\n");
                }
            }
            break;
        }

        if (remember_cmd)
        {
            strcpy(dbg_last_command, dbg_inbuf);
        }

        if (cmd_done_len > 0)
        {
            memmove(dbg_inbuf, dbg_inbuf + cmd_done_len, strlen(dbg_inbuf + cmd_done_len) + 1);
        }
    }

    return ret_code;
}

