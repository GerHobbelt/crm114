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



int inside_debugger = 0;

static FILE *mytty = NULL;
static char **show_expr_list = NULL;
static char *last_e_expression = NULL;
static char *show_expr_buffer = NULL;
static char *dbg_inbuf = NULL;
static char *dbg_outbuf = NULL;
static int dbg_iobuf_size = MAX_PATTERN + MAX_VARNAME + 256;
static char *dbg_last_command = NULL;

/*
 * dbg_step_mode mode flags:
 */
// group 1:
#define SM_REGULAR                                                   0
#define SM_NEXT_EXECUTABLE_STMT                                 0x0001
#define SM_BREAK_ON_EXCEPTION                                   0x0002
// group 2:
#define SM_STEP_INTO                                                 0
#define SM_STEP_OVER_CALL                                       0x0010
#define SM_STEP_OUT_RETURN                                      0x0020
#define SM_STEP_OUT_BRACED_SCOPE                                0x0040
// group 3:
// INTERNAL USE: signal that a step command was given and is now
//               executing; only stop and enter debugger when a line
//               is hit which matches the criteria above.
#define SM_PENDING                                              0x8000

static int dbg_step_mode = SM_REGULAR;
static int dbg_exec_calldepth = -1;
static int dbg_exec_nestlevel = -1;



void free_debugger_data(void)
{
    int j;

    if (show_expr_buffer)
    {
        free(show_expr_buffer);
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


int dbg_decode_step_mode(char **arg)
{
    char *dbg_arg = *arg;
    int cmd_done_len = 0;

    for ( ; dbg_arg; dbg_arg++)
    {
        switch (*dbg_arg)
        {
        case 'n':                       // normal / default
        case 'd':                       // normal / default
            dbg_step_mode = SM_REGULAR;
            continue;

        case 'e':
            // only 'executable' statements!
            dbg_step_mode |= SM_NEXT_EXECUTABLE_STMT;
            continue;

        case 'a':
            // break at any statement - this includes a comment
            dbg_step_mode &= ~SM_NEXT_EXECUTABLE_STMT;
            continue;

        case 'x':
            // break on exceptions: trappable fault while *going* *to* *execute* the trap
            dbg_step_mode ^= SM_BREAK_ON_EXCEPTION;
            continue;

        case 'i':                       // step into
            dbg_step_mode &= ~(SM_STEP_INTO | SM_STEP_OVER_CALL | SM_STEP_OUT_RETURN | SM_STEP_OUT_BRACED_SCOPE);
            dbg_step_mode |= SM_STEP_INTO;
            continue;

        case 's':                       // 'skip': step over calls
            dbg_step_mode &= ~(SM_STEP_INTO | SM_STEP_OVER_CALL | SM_STEP_OUT_RETURN | SM_STEP_OUT_BRACED_SCOPE);
            dbg_step_mode |= SM_STEP_OVER_CALL;
            continue;

        case 'o':                       // step out till next return
            dbg_step_mode &= ~(SM_STEP_INTO | SM_STEP_OVER_CALL | SM_STEP_OUT_RETURN | SM_STEP_OUT_BRACED_SCOPE);
            dbg_step_mode |= SM_STEP_OUT_RETURN;
            continue;

        case 'f':                       // step out till we're outside this scope block
            dbg_step_mode &= ~(SM_STEP_INTO | SM_STEP_OVER_CALL | SM_STEP_OUT_RETURN | SM_STEP_OUT_BRACED_SCOPE);
            dbg_step_mode |= SM_STEP_OUT_BRACED_SCOPE;
            continue;

        default:
            // assume the command has another argument (or more): prepare to point at it
            dbg_arg += strspn(dbg_arg, " \t\r\n");

            cmd_done_len = (int)(dbg_arg - *arg);

            if (*dbg_arg == ';' || *dbg_arg == 0)
            {
                dbg_arg = NULL;
            }
            break;
        }
        break;
    }

    *arg = dbg_arg;

    return cmd_done_len;
}


/*
 * Decode N,M and N.M line number specs; also supported: ^,M and N,$ for start/end.
 *
 * start and *end are set to the decoded number range (0 for ^, INT_MAX for $)
 * and number of decoded characters is returned.
 *
 * Also support other formats:
 *
 *   N<M  from M before N upto N including. N may be removed to signal 'current'. No M means 'everything'.
 *
 *       N>M  from N upto M after N. N may be removed to signal 'current'. No M means 'everything'.
 *
 *       N~M  from M before N upto M after N. default M = 3, default N = current.
 *
 *
 * Negative return code means decode failure:
 *
 * -2: leading cruft before ./,
 *
 * -3: non-numeric cruft immediately after ./, which do not match any of 'accepted_follow_chars'
 */
int decode_line_number_range(const char *buf,
        int                              buflen,
        int                             *start,
        int                             *end,
        int                              current,
        const char                      *accepted_follow_chars)
{
    char s[256];
    int i;
    int val1;
    int val2;
    char *s2;
    int ret = 0;
    int mode;

    CRM_ASSERT(start != NULL);
    CRM_ASSERT(end != NULL);
    CRM_ASSERT(buf != NULL);
    CRM_ASSERT(buflen >= 0);

    if (buflen == 0)
    {
        buflen = (int)strlen(buf);
    }
    buflen = CRM_MIN(WIDTHOF(s) - 2, buflen);
    strncpy(s, buf, buflen);
    s[buflen] = 0;
    buflen = (int)strlen(s);
    s[buflen + 1] = 0;      // special extra NUL for use by the 's2' section below.

    i = (int)strcspn(s, ".,<>~");
    mode = s[i];

    CRM_ASSERT(i + 1 < WIDTHOF(s));

    // set first section plus optional ./, as 'decoded' for later.
    ret = (s[i] == 0 ? i : i + 1);

    // split buf in two parts:
    s2 = s + i + 1;
    s[i] = 0;

    // decode first part:
    i = sscanf(s, " %d", &val1);
    if (i != 1)
    {
        // if it's all whitespace up front, set start as 'current'
        i = (int)strspn(s, " \t\r\n");
        if (s[i] == 0)
        {
            *start = current;
        }
        else
        {
            // failed to decode: cruft preceeding the initial . or ,
            return -2;
        }
    }
    else
    {
        *start = val1;
    }

    // decode second part
    CRM_ASSERT(strcspn(s2, "") < WIDTHOF(s));    // that's why we NULled both [l-1] and [l-2] at
    // the start up there: two strings for the price of one.

    i = sscanf(s2, " %d", &val2);
    if (i != 1)
    {
        // if it's all whitespace at the end, set end to 'current'
        i = (int)strspn(s2, " \t\r\n");
        if (s2[i] == 0)
        {
            *end = current;

            // set section as 'decoded' for later.
            ret += (int)strlen(s2);
        }
        else if (s2[i] == '$')
        {
            // end is end:
            *end = INT_MAX;

            // set section as 'decoded' for later.
            ret += i + 1;                     // one past the '$'
        }
        else if (!accepted_follow_chars
                 || !strchr(accepted_follow_chars, s2[i]))
        {
            // failed to decode: cruft following the initial . or ,
            return -3;
        }
        else
        {
            // special case: trailing cruft is acceptable cruft.
            *end = current;

            // set section as 'decoded' for later.
            ret += i;
        }
    }
    else
    {
        *end = val2;

        // skip whitespace and number and more whitespace:
        i = (int)strspn(s2, " \t\r\n");
        i += (int)strspn(s2 + i, "01234567890x");
        i += (int)strspn(s2 + i, " \t\r\n");

        ret += i;
    }

    // process 'mode' now:
    switch (mode)
    {
    default:
        // format: N,M
        break;

    case '<':
        // everything before N:
        if (*end != 0)
        {
            // range: N<M: M lines before current
            int e = *start;

            *start = e - *end;
            *end = e;
        }
        else
        {
            *end = *start;
            *start = 0;
        }
        break;

    case '>':
        // everything AFTER N:
        if (*end != 0)
        {
            // range: N>M: M lines after current
            *end += *start;
        }
        else
        {
            *end = INT_MAX;
        }
        break;

    case '~':
        // *end around *start
        if (*start != 0)
        {
            current = *start;
        }
        if (*end == 0 || *end == INT_MAX)
        {
            *end = 3;                     // default
        }
        // don't care about integer wrap; if that happens, it was intentional from the caller anyway.
        *start = current - *end;
        *end = current + *end;
        break;
    }
    // also don't care if *end < *start; again, if that happens, it was probably intentional from the caller anyway.

    return ret;
}



/*
 * Return 0 on success, non-zero on failure.
 *
 * dst will point to a strdup()ped string when succesful; *arg will have been updated too.
 */
int dbg_fetch_expression(char **dst, char **arg, CSL_CELL *csl, MCT_CELL *current_crm_command)
{
    char *dbg_arg = *arg;

    char buf[MAX_PATTERN];
    int len;

    buf[0] = 0;

    if (*dbg_arg == '<')
    {
        // indirection: fetch expression from the active source code line itself. Or somewhere else...
        //
        // format: <NBI
        //
        // where
        //
        //   N is a number signifying a line number (statement number in the source, corresponding to the line
        //     number as reported in a 'v' program listing.
        //   N can also be simply '.', which is the current statement.
        //
        //   B is the type of argument delimiter we're looking for: /, <, [ or (
        //
        //   I is the number of this particular braced element: 1 by default.
        //
        // Thus
        //
        //   <./2
        //
        // indicates that we like a copy of the 2nd // argument for the current statement.
        //
        // Other indirections:
        //
        //   N can also be '<', which indicates that the remainder of the line is specially formatted line,
        //     which must be processed once to produce the desired expression.
        //
        //     The 'formatted line' may include '%n%' (where n is a number) arguments, where these 'n' numbers
        //     point at that particular watched expression.
        //
        // Example:
        //
        //   <<:*%5%
        //
        // will take the 5th existing watch expression and replace the '%5%' string with that. Assuming the
        // 5th watched expression is ':filename:' (for instance grabbed from a match expression using '<.(3'),
        // then the resulting expression string will become:
        //
        //   :*:filename:
        //
        // The '<<' formatted indirection is handy to use for a two-step copy&paste process where output
        // variables are grabbed from the script, then merged together with other text to watch the value written
        // to such variables.
        //
        // Of course, you may specify multiple %n% items in a format string, e.g.
        //
        //   << full path = ':*%3%/%7%'
        //
        // where, for example, the 3rd and 7 th watched expression read:
        //
        // 3rd:  ':spamdir:'
        // 7th:  ':*:stripped_filename:'
        //
        int max_expr;
        int i;
        char *end;
        int idx;
        char *start_idx;


        switch (*++dbg_arg)
        {
        case '<':
            // formatted string with %n% items:
            for (i = 0; show_expr_list[i]; i++)
                ;
            max_expr = i;

            dbg_arg++;
            for (i = 0; *dbg_arg && i < WIDTHOF(buf); dbg_arg++)
            {
                switch (*dbg_arg)
                {
                case '\r':
                case '\n':
                    // end of format string
                    break;

                case '\\':
                    dbg_arg++;

                    // fall through

                default:
                    buf[i++] = *dbg_arg;
                    continue;

                case '%':
                    idx = strtol(dbg_arg + 1, &end, 0);

                    if (idx <= 0 || idx > max_expr)
                    {
                        // out of range: echo as-is; do NOT expand
                    }
                    else if (*end != '%')
                    {
                        // not terminated by '%': format error; do NOT expand!
                    }
                    else
                    {
                        // okay, expand...
                        size_t expr_len = strlen(show_expr_list[idx - 1]);

                        if (expr_len > WIDTHOF(buf) - 1 - i)
                        {
                            // expansion will not fit: do NOT expand
                        }
                        else
                        {
                            strcpy(buf + i, show_expr_list[idx - 1]);
                            i += (int)expr_len;

                            dbg_arg = end;
                            // ^^^^ this points now to the terminating '%', which
                            // will be skipped thanks to the increment in the for(;;) above
                            continue;
                        }
                    }
                    // when we get here, we couldn't/wouldn't expand for some reason:
                    // copy parameter verbatim to 'end':
                    for ( ; dbg_arg <= end && i < WIDTHOF(buf);)
                    {
                        buf[i++] = *dbg_arg++;
                    }
                    continue;
                }
            }

            if (i < WIDTHOF(buf))
            {
                buf[i] = 0;
            }
            else
            {
                buf[WIDTHOF(buf) - 1] = 0;
            }
            break;

        default:
            // number must follow:
            idx = strtol(dbg_arg, &end, 0);
            if (end == dbg_arg)
            {
                // format error: must be a number!
                fprintf(stderr, "Indirection format error: '<' must be followed by another '<', a '.' dot or a statement number.\n"
                                "I cannot decode '%s'\n",
                        dbg_arg);
                return -1;
            }
            if (idx < 0 || idx >= csl->nstmts)
            {
                // format error: must be a number IN RANGE!
                fprintf(stderr, "Indirection format error: '<' must be followed by a VALID statement number.\n"
                                "The specified line number '%d' is NOT within the range '%d' .. '%d' inclusive.\n",
                        idx,
                        0,
                        csl->nstmts - 1);
                return -1;
            }
            // now get the command/statement of interest:
            dbg_arg = end - 1;              // -1 to compensate to the adjustment after the 'fall-through'.
            current_crm_command = csl->mct[idx];

            // fall through
        case '.':
            // 'current_crm_command' points at the statement line we're going to grab stuff from. Now determine WHAT EXACTLY we want to grab.

            // prepare the statement so we can easily grab those args:
#if !FULL_PARSE_AT_COMPILE_TIME
            if (!current_crm_command->apb)
            {
                fprintf(stderr, "Indirection error:\n"
                                "I am very sorry, but we cannot grab the indicated argument,\n"
                                "as this line has not been JITted far enough yet. You will\n"
                                "have to wait for the new compiler to arrive, as that one WILL\n"
                                "be able to provide you with the option to grab expression\n"
                                "content from forward referenced statements. So sorry. (GerH)\n"
                                "\n");
                return -1;
            }
#endif

            dbg_arg++;
            dbg_arg += strspn(dbg_arg, " \t");           // skip optional whitespace

            // optional number must follow after type indicator:
            end = dbg_arg + 1;
            end += strspn(end, " \t");
            if (crm_isdigit(*end))
            {
                idx = strtol(end, &end, 0);
            }
            else
            {
                idx = 1;

                if (*end && !strchr("\r\n", *end))
                {
                    fprintf(stderr, "Indirection format error: only an index number may follow the argument type indicator in the '<' format.\n"
                                    "I do not understand why this is here: '%s'\n",
                            end);
                    return -1;
                }
            }

            start_idx = NULL;
            len = -1;

            {
                ARGPARSE_BLOCK *apb;

#if !FULL_PARSE_AT_COMPILE_TIME
                apb = current_crm_command->apb;
#else
                apb = &current_crm_command->apb;
#endif
                switch (*dbg_arg)
                {
                case '(':
                    switch (idx)
                    {
                    case 1:
                        start_idx = apb->p1start;
                        len = apb->p1len;
                        break;

                    case 2:
                        start_idx = apb->p2start;
                        len = apb->p2len;
                        break;

                    case 3:
                        start_idx = apb->p3start;
                        len = apb->p3len;
                        break;
                    }
                    break;

                case '/':
                    switch (idx)
                    {
                    case 1:
                        start_idx = apb->s1start;
                        len = apb->s1len;
                        break;

                    case 2:
                        start_idx = apb->s2start;
                        len = apb->s2len;
                        break;
                    }
                    break;

                case '[':
                    switch (idx)
                    {
                    case 1:
                        start_idx = apb->b1start;
                        len = apb->b1len;
                        break;
                    }
                    break;

                case '<':
                    switch (idx)
                    {
                    case 1:
                        start_idx = apb->a1start;
                        len = apb->a1len;
                        break;
                    }
                    break;

                default:
                    fprintf(stderr,
                            "Indirection format error: only a VALID type indicator ('<', '[', '(' or '/') may be specified in the '<' format.\n"
                            "I do not understand why this is here: '%s'\n",
                            dbg_arg);
                    return -1;
                }
            }

            if (start_idx == NULL || len < 0)
            {
                fprintf(stderr,
                        "Indirection format error: only a VALID index number may follow the argument type indicator '%c' in the '<' format.\n"
                        "I do not understand why the index %d was specified.\n",
                        *dbg_arg,
                        idx);
                return -1;
            }

            len = CRM_MIN(len, WIDTHOF(buf) - 1);
            memmove(buf, start_idx, len);
            buf[len] = 0;

            dbg_arg = end;

            break;
        }
    }
    else
    {
        // literal arg until EOL:
        len = (int)strcspn(dbg_arg, "\r\n");

        len = CRM_MIN(len, WIDTHOF(buf) - 1);
        memmove(buf, dbg_arg, len);
        buf[len] = 0;

        dbg_arg += len;
    }


    // now feed it back to caller in malloc()ed space:
    *dst = strdup(buf);
    if (! * dst)
    {
        untrappableerror("Cannot allocate debugger memory", "Stick a fork in us; we're _done_.");
    }

    // and feed back corrected dbg_arg:
    *arg = dbg_arg;

    return 0;
}




#if defined (CRM_WITHOUT_MJT_INLINED_QSORT)

static int dbg_vht_compare(const void *a, const void *b)
{
    return strncmp((*(VHT_CELL **)a)->nametxt + (*(VHT_CELL **)a)->nstart,
            (*(VHT_CELL **)b)->nametxt + (*(VHT_CELL **)b)->nstart,
            CRM_MAX((*(VHT_CELL **)a)->nlen,
                    (*(VHT_CELL **)b)->nlen));
}

#else

#define dbg_vht_compare(a, b)              \
    (strncmp(a[0]->nametxt + a[0]->nstart, \
             b[0]->nametxt + b[0]->nstart, \
             CRM_MAX(a[0]->nlen,           \
                     b[0]->nlen)) < 0)

#endif





//         If we got to here, we need to run some user-interfacing
//         (i.e. debugging).
//
//         possible return values:
//         1: reparse and continue execution
//         -1: exit immediately
//         0: continue

int crm_debugger(CSL_CELL *csl, crm_debug_reason_t reason_for_the_call, const char *message)
{
    int ichar;
    int ret_code = 0;
    int parsing_done = 0;
    int show_watched_expr = 1;
    MCT_CELL *current_crm_command = NULL;
    const STMT_TABLE_TYPE *current_crm_commanddef = NULL;

    inside_debugger = 1;     // signal the other code section we're now inside the debugger!

    CRM_ASSERT(csl != NULL);

    if (csl != NULL
        && csl->cstmt >= 0 && csl->cstmt <= /* !!! */ csl->nstmts
        && csl->mct[csl->cstmt] != NULL)
    {
        current_crm_command = csl->mct[csl->cstmt];

        if (current_crm_command->stmt_def)
        {
            current_crm_commanddef = current_crm_command->stmt_def;
        }
    }
    if (!current_crm_command || !current_crm_commanddef)
    {
        untrappableerror("The debugger has found that the CRM114 compiler screwed up somewhere. ",
                "The inspected statement doesn't seem valid. You may scream now...");

        inside_debugger = 0;         // signal the other code section we're now LEAVING the debugger!

        return -1;
    }


    if (dbg_exec_calldepth < 0)
    {
        dbg_exec_calldepth = csl->calldepth;
    }
    if (dbg_exec_nestlevel < 0)
    {
        dbg_exec_nestlevel = current_crm_command->nest_level;
    }

    if (reason_for_the_call == CRM_DBG_REASON_DEBUG_STATEMENT)
    {
        // programmed breakpoint detected ('debug' statement): allow the debugger to pop up on the NEXT statement!
        dbg_step_mode &= ~SM_PENDING;

        inside_debugger = 0;         // signal the other code section we're now LEAVING the debugger!

        return 0;
    }
    else if (reason_for_the_call == CRM_DBG_REASON_BREAKPOINT)
    {
        // configured/set breakpoint detected: allow the debugger to pop up NOW!
        dbg_step_mode &= ~SM_PENDING;
    }
    else if (reason_for_the_call == CRM_DBG_REASON_DEBUG_END_OF_PROGRAM)
    {
        // end of program detected: allow the debugger to pop up NOW, because this is very last time we will get a chance!
        dbg_step_mode &= ~SM_PENDING;
    }
    else if (reason_for_the_call == CRM_DBG_REASON_EXCEPTION_HANDLING)
    {
        if (!(dbg_step_mode & SM_BREAK_ON_EXCEPTION))
        {
            inside_debugger = 0;
        }
        dbg_step_mode &= ~SM_PENDING;
    }


    // when we're at the end of the program, always break, even if we have, for instance,
    // a pending 'step out' operation, because this the Mother Of All Step Outs really. ;-)
    //
    // It's the end, so 'step over' has also done it's job, don't you think?
    if (dbg_step_mode & SM_PENDING)
    {
        if (dbg_step_mode & SM_STEP_OUT_RETURN)
        {
            if (csl->calldepth >= dbg_exec_calldepth)
            {
                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                return 0;
            }
        }
        // auto-reset: even while other conditions might cause us to _not_ break _yet_!
        dbg_step_mode &= ~SM_STEP_OUT_RETURN;
        if (dbg_step_mode & SM_STEP_OUT_BRACED_SCOPE)
        {
            if (csl->calldepth >= dbg_exec_calldepth)
            {
                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                return 0;
            }
            if (current_crm_command->nest_level >= dbg_exec_nestlevel)
            {
                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                return 0;
            }
        }
        // auto-reset: even while other conditions might cause us to _not_ break _yet_!
        dbg_step_mode &= ~SM_STEP_OUT_BRACED_SCOPE;
        if (dbg_step_mode & SM_STEP_OVER_CALL)
        {
            if (csl->calldepth > dbg_exec_calldepth)
            {
                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                return 0;
            }
        }
        if (dbg_step_mode & SM_NEXT_EXECUTABLE_STMT)
        {
            if (!current_crm_commanddef->is_executable)
            {
                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                return 0;
            }
        }
        // we've hit a matching line: break
        dbg_step_mode &= ~SM_PENDING;
    }

    // auto-reset no matter what:
    dbg_step_mode &= ~(SM_STEP_OUT_RETURN | SM_STEP_OUT_BRACED_SCOPE);
    dbg_exec_calldepth = csl->calldepth;
    dbg_exec_nestlevel = current_crm_command->nest_level;

    crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 0, "");


    if (!mytty)
    {
        fprintf(stderr, "CRM114 Debugger - type \"h\" for help. Type 'hh' for help with examples.\n");
        fprintf(stderr, "User trace turned on.\n");
        user_trace = 1;
        if (user_trace)
            fprintf(stderr, "Opening the user terminal for debugging I/O\n");
#if (defined (WIN32) || defined (_WIN32) || defined (_WIN64) || defined (WIN64))
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
            untrappableerror("Cannot allocate debugger expression show list",
                    "Stick a fork in us; we're _done_.");
        }
        show_expr_list[0] = NULL;
    }
    if (!dbg_inbuf)
    {
        dbg_inbuf = calloc(dbg_iobuf_size, sizeof(dbg_inbuf[0]));
        if (!dbg_inbuf)
        {
            untrappableerror("Cannot allocate debugger input buffer",
                    "Stick a fork in us; we're _done_.");
        }
        dbg_inbuf[0] = 0;

        dbg_outbuf = calloc(dbg_iobuf_size, sizeof(dbg_outbuf[0]));
        if (!dbg_outbuf)
        {
            untrappableerror("Cannot allocate debugger output buffer",
                    "Stick a fork in us; we're _done_.");
        }
        dbg_outbuf[0] = 0;

        dbg_last_command = calloc(dbg_iobuf_size, sizeof(dbg_last_command[0]));
        if (!dbg_last_command)
        {
            untrappableerror("Cannot allocate debugger input recall buffer",
                    "Stick a fork in us; we're _done_.");
        }
        dbg_last_command[0] = 0;
    }
    CRM_ASSERT(dbg_outbuf != NULL);
    CRM_ASSERT(dbg_last_command != NULL);

    if (current_crm_command->stmt_break)
    {
        fprintf(stderr, "Breakpoint tripped at statement %d\n", csl->cstmt);

        // when this is a breakpoint, stop and repetitive '.' debug macro run in its tracks!
        dbg_inbuf[0] = 0;
    }
    else if (reason_for_the_call == CRM_DBG_REASON_EXCEPTION_HANDLING)
    {
        // dbg_step_mode & SM_BREAK_ON_EXCEPTION
        fprintf(stderr, "Trappable Exception occurred in CRM114 script at statement %d\n"
                        "Exception report:\n"
                        "%s\n\n"
                        "*** Going to find a matching trap r.e. after this! ***\n",
                csl->cstmt,
                (message ? message : "???"));

        // when this is a breakpoint, stop and repetitive '.' debug macro run in its tracks!
        dbg_inbuf[0] = 0;
    }


    for ( ; !parsing_done;)
    {
        // show watched expressions:
        int watch;
        int cmd_done_len = 1;
        int remember_cmd = 0;
        char *dbg_arg;

        //    find out what they want to do
        //
        //    if there's still a command waiting in the buffer, handle that one first.
        if (!dbg_inbuf[0])
        {
            if (show_watched_expr && show_expr_list && show_expr_list[0])
            {
                // store the user+internal trace values and reset them for the duration we're
                // printing watched variables as the routines we use for that will contain
                // code which produces extra output when those traces are enabled, and
                // we do NOT want that screen clutter while we're trying to dump the watched
                // vars here using :@: math expansion!
                int old_user_trace = user_trace;
                int old_internal_trace = internal_trace;

                user_trace = 0;
                internal_trace = 0;

                show_watched_expr = 0;

                if (!show_expr_buffer)
                {
                    show_expr_buffer = calloc(data_window_size, sizeof(show_expr_buffer[0]));
                    if (!show_expr_buffer)
                    {
                        // reset traces to stored setting
                        user_trace = old_user_trace;
                        internal_trace = old_internal_trace;
                        untrappableerror("cannot allocate buffer to display watched expressions.",
                                "Stick a fork in us; we're _done_.");
                    }
                }

                fprintf(stderr, "\n--- watched expressions: ------------------------------\n");
                for (watch = 0; show_expr_list[watch]; watch++)
                {
                    int retval = 0;
                    int op_pos;
                    int expand_flags = 0;
                    int len;

                    strncpy(show_expr_buffer, show_expr_list[watch], data_window_size);
                    show_expr_buffer[data_window_size - 1] = 0;
                    len = (int)strlen(show_expr_buffer);

                    // what's the first operator that we see? Try to keep expansion to the bare minimum so watched
                    // EXPRESSIONS are not processed any more than needed.
                    for (op_pos = (int)strcspn(show_expr_buffer, ":");
                         op_pos < len;
                         op_pos = (int)strcspn(show_expr_buffer + op_pos + 1, ":"))
                    {
                        CRM_ASSERT(show_expr_buffer[op_pos] == ':');
                        CRM_ASSERT(op_pos + 1 <= len);
                        switch (show_expr_buffer[op_pos + 1])
                        {
                        default:
                            continue;

                        case '@':
                            expand_flags |= (CRM_EVAL_ANSI
                                             | CRM_EVAL_MATH
                                             | CRM_EVAL_STRINGLEN
                                             | CRM_EVAL_REDIRECT
                                             | CRM_EVAL_STRINGVAR);

                        case '#':
                            expand_flags |= (CRM_EVAL_ANSI
                                             | CRM_EVAL_MATH
                                             | CRM_EVAL_STRINGLEN
                                             | CRM_EVAL_REDIRECT
                                             | CRM_EVAL_STRINGVAR);
                            break;

                        case '+':
                            expand_flags |= (CRM_EVAL_ANSI
                                             | CRM_EVAL_MATH
                                             | CRM_EVAL_STRINGLEN
                                             | CRM_EVAL_REDIRECT
                                             | CRM_EVAL_STRINGVAR);
                            break;

                        case '*':
                            expand_flags |= (CRM_EVAL_ANSI
                                             | CRM_EVAL_STRINGVAR);
                            break;
                        }
                        break;
                    }

                    retval = crm_zexpandvar(show_expr_buffer, (int)strlen(
                                    show_expr_buffer), data_window_size,
                            &retval,
                            expand_flags, vht, tdw);

                    fprintf(stderr,
                            "[#%2d]: '%s' (%d) /%s/\n",
                            watch + 1,
                            show_expr_list[watch],
                            retval,
                            show_expr_buffer);
                }

                // reset traces to stored setting
                user_trace = old_user_trace;
                internal_trace = old_internal_trace;
            }

            //    let the user know they're in the debugger
            //
            {
                char step_mode_str[8];

                step_mode_str[0] = 'A';
                if (dbg_step_mode & SM_NEXT_EXECUTABLE_STMT)
                {
                    step_mode_str[0] = 'E';
                }
                step_mode_str[1] = 'i';
                switch (dbg_step_mode &
                        (SM_STEP_INTO | SM_STEP_OVER_CALL | SM_STEP_OUT_RETURN |
                         SM_STEP_OUT_BRACED_SCOPE))
                {
                case 0:
                    break;

                default:
                    step_mode_str[1] = '?';
                    break;

#if 0
                case SM_STEP_INTO:
                    step_mode_str[1] = 'I';
                    break;
#endif

                case SM_STEP_OVER_CALL:
                    step_mode_str[1] = 'S';
                    break;

                case SM_STEP_OUT_RETURN:
                    step_mode_str[1] = 'O';
                    break;

                case SM_STEP_OUT_BRACED_SCOPE:
                    step_mode_str[1] = 'F';
                    break;
                }
                step_mode_str[2] = '-';
                if (reason_for_the_call == CRM_DBG_REASON_DEBUG_STATEMENT)
                {
                    // programmed breakpoint detected ('debug' statement): allow the debugger to pop up on the NEXT statement!
                    step_mode_str[2] = 'B';
                }
                else if (reason_for_the_call == CRM_DBG_REASON_BREAKPOINT)
                {
                    // configured/set breakpoint detected: allow the debugger to pop up NOW!
                    step_mode_str[2] = 'b';
                }
                else if (reason_for_the_call == CRM_DBG_REASON_DEBUG_END_OF_PROGRAM)
                {
                    // end of program detected: allow the debugger to pop up NOW, because this is very last time we will get a chance!
                    step_mode_str[2] = '$';
                }
                else if (reason_for_the_call == CRM_DBG_REASON_EXCEPTION_HANDLING)
                {
                    // pop up debugger while right before locating and jumping to a suitable trap handler.
                    step_mode_str[2] = 'X';
                }
                step_mode_str[3] = 0;

#ifdef HAVE_LIBREADLINE
                {
                    char *chartemp;
                    char prompt[128];

                    fprintf(stderr, "\n");
                    snprintf(prompt, 128, "crm-dbg[%d|%s]> ", csl->calldepth, step_mode_str);
                    prompt[128 - 1] = 0;

                    chartemp = readline(prompt);
                    if (!chartemp)
                    {
                        chartemp = strdup("");                        // see the UNIX man page: readline() MAY return NULL!
                    }
                    if (strlen(chartemp) > dbg_iobuf_size - 1)
                    {
                        fprintf(stderr, "Dang, this line of text is way too long: truncated!\n");
                    }
                    strncpy(dbg_inbuf, chartemp, dbg_iobuf_size);
                    dbg_inbuf[dbg_iobuf_size - 1] = 0;                    /* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
                    free(chartemp);
                }
#else
                fprintf(stderr, "\ncrm-dbg[%d|%s]> ", csl->calldepth, step_mode_str);

                ichar = 0;

                while (!feof(mytty)
                       && ichar < dbg_iobuf_size - 1
                       && (dbg_inbuf[ichar - 1] != '\n'))
                {
                    dbg_inbuf[ichar] = fgetc(mytty);
                    ichar++;
                }
                dbg_inbuf[ichar] = 0;
#endif
            }

            remember_cmd = 1;             // signal we MAY be allowed to store this commandline

            if (feof(mytty))
            {
                fprintf(stderr, "Quitting\n");
                ret_code = -1;
            }
        }

        // assume the command has a argument (or more): prepare to point at it
        dbg_arg = dbg_inbuf + 1 + strspn(dbg_inbuf + 1, " \t\r\n");
        if (*dbg_arg == ';' || *dbg_arg == 0)
        {
            dbg_arg = NULL;
        }


        //   now a big switch statement on the first character of the command
        //
        switch (dbg_inbuf[0])
        {
        case 'q':
        case 'Q':
            if (user_trace)
                fprintf(stderr, "Quitting.\n");
            ret_code = -1;
            parsing_done = 1;
            break;

        case ';':
            // act as optional command seperator for commands which use arguments, when these are used in a command sequence

            //show_watched_expr = 1;
            break;

        case '.':
            // repeat last run command
            //
            // Note that if the previous command was a line containing mutiple commands,
            // EACH of those is executed again!
            {
                char *dst = dbg_inbuf + 1;
                int len = (int)strlen(dst);

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

        case 'm':
            // set step mode

            if (dbg_arg)
            {
                cmd_done_len = (int)(dbg_arg - dbg_inbuf);

                cmd_done_len += dbg_decode_step_mode(&dbg_arg);
            }

            // ret_code = 0;
            break;

        case 'n':
        case 'N':
            debug_countdown = 0;

            if (dbg_arg)
            {
                cmd_done_len = (int)(dbg_arg - dbg_inbuf);

                cmd_done_len += dbg_decode_step_mode(&dbg_arg);
            }

            dbg_step_mode |= SM_PENDING;

            // ret_code = 0;
            parsing_done = 1;
            show_watched_expr = 1;

            if (remember_cmd == 1)
                remember_cmd = 2; // signal we ARE allowed to store this commandline
            break;

        case 'c':
        case 'C':
            debug_countdown = 0;

            if (dbg_arg)
            {
                char *end = NULL;

                cmd_done_len = (int)(dbg_arg - dbg_inbuf);

                cmd_done_len += dbg_decode_step_mode(&dbg_arg);

                debug_countdown = strtol(dbg_arg, &end, 0);
                if (end == dbg_arg)
                {
                    fprintf(stderr, "Failed to decode the debug 'C' "
                                    "command countdown number '%s'. "
                                    "Assume 0: 'continue'.\n",
                            dbg_arg);
                    debug_countdown = 0;
                }
                cmd_done_len = (int)(end - dbg_inbuf);
            }

            dbg_step_mode |= SM_PENDING;

            if (debug_countdown <= 0)
            {
                debug_countdown = -1;
                fprintf(stderr, "continuing execution...\n");
            }
            else
            {
                fprintf(stderr, "continuing %d cycles...\n", debug_countdown);
            }
            CRM_ASSERT(cmd_done_len > 0);
            // ret_code = 0;
            parsing_done = 1;
            show_watched_expr = 1;

            if (remember_cmd == 1)
                remember_cmd = 2; // signal we ARE allowed to store this commandline
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
            if (dbg_arg)
            {
                if (last_e_expression)
                    free(last_e_expression);
                last_e_expression = NULL;
                if (dbg_fetch_expression(&last_e_expression, &dbg_arg, csl, current_crm_command))
                {
                    fprintf(stderr, "'e' statement line is rejected.\n");
                    cmd_done_len = (int)strlen(dbg_inbuf);
                    break;
                }
                CRM_ASSERT(last_e_expression != NULL);

                cmd_done_len = (int)(dbg_arg - dbg_inbuf);
            }

            if (last_e_expression)
            {
                int elen;

                strcpy(dbg_outbuf, last_e_expression);

                elen = crm_nexpandvar(dbg_outbuf, (int)strlen(dbg_outbuf), dbg_iobuf_size, vht, tdw);
                CRM_ASSERT(elen < dbg_iobuf_size);
                dbg_outbuf[elen] = 0;

                fprintf(stderr,
                        "expression '%s' ==> (len: %d)\n%s",
                        last_e_expression,
                        (int)strlen(dbg_outbuf),
                        dbg_outbuf);
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
            if (dbg_arg)
            {
                if (last_e_expression)
                    free(last_e_expression);
                last_e_expression = NULL;
                if (dbg_fetch_expression(&last_e_expression, &dbg_arg, csl, current_crm_command))
                {
                    fprintf(stderr, "'+' statement line is rejected.\n");
                    cmd_done_len = (int)strlen(dbg_inbuf);
                    break;
                }
                CRM_ASSERT(last_e_expression != NULL);

                cmd_done_len = (int)(dbg_arg - dbg_inbuf);
            }

            if (last_e_expression)
            {
                // handle special watch requests here: any watched expression that doesn't contain
                // any :@:/:+:/:+:/:@: is considered a regex into the variables collection.
                //
                // With a 'thank you' to 'C' boolean evauation preventing crashes in the conditional :-)
                if (!strchr(last_e_expression, ':')
                    || !strchr("*+#@", strchr(last_e_expression, ':')[1])
                    || ':' != strchr(last_e_expression, ':')[2])
                {
                    // add every variable matching the specified regex
                    int i;
                    int len;
                    int maxlen;
                    int status;
                    regex_t preg;
                    int add_system_vars_too = 0;

                    char *arg = last_e_expression;
                    int arglen = (int)strcspn(arg, "\r\n");

                    for (arglen--; arglen >= 0 && !crm_isgraph(arg[arglen]); arglen--)
                        ;
                    arglen++;

                    if (!strncmp(arg, "*", arglen))
                    {
                        // kill 'em all!
                        arg = ".*";
                        arglen = (int)strlen(arg);
                    }

                    // ONLY add system variables into the mix when they SEEM explicitly addressed:
                    // we don't check the precise regex, but the mention of an underscore in there
                    // is good enough for us. Debuggers are smart people, right? ;-)
                    add_system_vars_too = !!strchr(arg, '_');

                    //       compile the regex
                    status = crm_regcomp(&preg, arg, arglen, REG_EXTENDED);
                    if (status != 0)
                    {
                        CRM_ASSERT(dbg_outbuf);
                        CRM_ASSERT(dbg_iobuf_size > 0);

                        crm_regerror(status, &preg, dbg_outbuf, dbg_iobuf_size);
                        fprintf(stderr,
                                "\n\n"
                                "ERROR: Regular Expression Compilation Problem in variable search pattern '%.*s' "
                                "while trying to add those matching variables to the watch list: %s\n\n",
                                arglen,
                                arg,
                                dbg_outbuf);
                    }
                    else
                    {
                        regmatch_t match[2];
                        VHT_CELL **vht_ref;

                        maxlen = 0;
                        vht_ref = calloc(vht_size, sizeof(vht_ref[0]));
                        if (!vht_ref)
                        {
                            untrappableerror("Cannot allocate debugger memory",
                                    "Stick a fork in us; we're _done_.");
                        }

                        len = 0;
                        for (i = 0; i < vht_size; i++)
                        {
                            if (vht[i] == NULL)
                                continue;
                            /*
                             * Note that, when :label:s have been isolate'd, the code in there
                             * MAY have modified the VHT record to look like any other regular variable;
                             * in that case, the only way to detect if we're talking about a :label:
                             * instead of a :variable: is by its usage in the code. :-(
                             */
                            if (vht[i]->valtxt == NULL)
                                continue; // don't add labels to the watch list, that's useless!
                            if (vht[i]->nlen > 0)
                            {
                                const char *varname = vht[i]->nametxt + vht[i]->nstart;
                                int varlen = vht[i]->nlen;

                                // see if this variable matches our regex:
                                status = crm_regexec(&preg,
                                        varname, varlen,
                                        WIDTHOF(match), match,
                                        0, NULL);
                                if (status != REG_OK && status != REG_NOMATCH)
                                {
                                    CRM_ASSERT(dbg_outbuf);
                                    CRM_ASSERT(dbg_iobuf_size > 0);

                                    crm_regerror(status, &preg, dbg_outbuf, dbg_iobuf_size);
                                    fprintf(stderr,
                                            "\n\n"
                                            "ERROR: Regular Expression Execution Problem in variable search pattern '%.*s' "
                                            "while trying to check variable '%.*s'(len: %d) for adding to the watch list: %s\n\n",
                                            arglen,
                                            arg,
                                            varlen,
                                            varname,
                                            varlen,
                                            dbg_outbuf);

                                    break;
                                }
                                else if (status == REG_OK)
                                {
                                    // regex matches the variable - at least part of the variable and that's good enough for us.
                                    CRM_ASSERT(match[0].rm_eo >= 0);
                                    CRM_ASSERT(match[0].rm_so >= 0);

                                    if (!add_system_vars_too && varlen > 0 && varname[1] == '_')
                                        continue; // do not include system vars this time around

                                    vht_ref[len++] = vht[i];
                                    maxlen = CRM_MAX(maxlen, varlen);
                                }
                            }
                        }

                        // done with the regex
                        crm_regfree(&preg);

                        QSORT(VHT_CELL *, vht_ref, len, dbg_vht_compare);

                        for (i = 0; i < len; i++)
                        {
                            int j;

                            CRM_ASSERT(vht_ref[i]->valtxt != NULL);                             // no labels in this list
                            CRM_ASSERT(dbg_outbuf);
                            CRM_ASSERT(dbg_iobuf_size > 0);

                            // note that the variable comes with its own ':' delimiters already!
                            snprintf(dbg_outbuf, dbg_iobuf_size, ":*%.*s",
                                    vht_ref[i]->nlen, vht_ref[i]->nametxt + vht_ref[i]->nstart);
                            dbg_outbuf[dbg_iobuf_size - 1] = 0;

                            // make sure the expression isn't in the list already:
                            for (j = 0; show_expr_list[j]; j++)
                            {
                                if (strcmp(show_expr_list[j], dbg_outbuf) == 0)
                                    break;
                            }
                            // add to list if not already in it:
                            if (show_expr_list[j] == NULL)
                            {
                                show_expr_list =
                                    realloc(show_expr_list, (j + 2) * sizeof(show_expr_list[0]));
                                if (!show_expr_list)
                                {
                                    untrappableerror("Cannot allocate debugger memory",
                                            "Stick a fork in us; we're _done_.");
                                }
                                show_expr_list[j] = strdup(dbg_outbuf);
                                if (!show_expr_list[j])
                                {
                                    untrappableerror("Cannot allocate debugger memory",
                                            "Stick a fork in us; we're _done_.");
                                }
                                show_expr_list[j + 1] = NULL;

                                fprintf(stderr, "'e' expression added to the watch list:\n"
                                                "    %s\n",
                                        dbg_outbuf);

                                show_watched_expr = 1;
                            }
                            // else: expression not added to watch list: expression already exists in watch list!
                        }
                        free(vht_ref);
                    }
                }
                else
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
                        show_expr_list =
                            realloc(show_expr_list, (j + 2) * sizeof(show_expr_list[0]));
                        if (!show_expr_list)
                        {
                            untrappableerror("Cannot allocate debugger memory",
                                    "Stick a fork in us; we're _done_.");
                        }
                        show_expr_list[j] = strdup(last_e_expression);
                        if (!show_expr_list[j])
                        {
                            untrappableerror("Cannot allocate debugger memory",
                                    "Stick a fork in us; we're _done_.");
                        }
                        show_expr_list[j + 1] = NULL;

                        fprintf(stderr, "'e' expression added to the watch list:\n"
                                        "    %s\n",
                                last_e_expression);

                        show_watched_expr = 1;
                    }
                    else
                    {
                        fprintf(stderr,
                                "expression not added to watch list: expression already exists in watch list!\n");
                    }
                }
            }
            else
            {
                fprintf(stderr,
                        "no 'e' expression specified: no expression added to the watch list\n");
            }
            break;

        case '-':
            // remove expression #n from watch list
            {
                int j;
                int expr_number = 0;

                if (!dbg_arg)
                {
                    fprintf(stderr, "You should specify either a number or a regex to "
                                    "identify the watched expressions you wish to see removed.\n");
                }
                else
                {
                    cmd_done_len = (int)(dbg_arg - dbg_inbuf) + (int)strcspn(dbg_arg, "; \t\r\n");

                    j = sscanf(dbg_arg, "%d", &expr_number);
                    if (j == 1)
                    {
                        // remove expression 'expr_number'
                        expr_number--;                                     // remember the number starts at 1!
                        if (expr_number >= 0 && show_expr_list[0] != NULL)
                        {
                            for (j = 0; show_expr_list[j]; j++)
                            {
                                if (j == expr_number)
                                {
                                    fprintf(stderr, "removed watched expression #%d: %s\n",
                                            j + 1, show_expr_list[j]);
                                    free(show_expr_list[j]);
                                    show_expr_list[j] = NULL;

                                    show_watched_expr = 1;
                                }
                                // shift other expressions one down in the list to close the gap: including NULL sentinel
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
                            fprintf(stderr,
                                    "You specified an illegal watched expression #%d; command ignored.\n",
                                    expr_number + 1);
                        }
                    }
                    else
                    {
                        // else, it's a regex to ditch expressions: any expression matching the regex is TOAST!
                        regex_t preg;
                        int status;
                        int arglen = (int)strcspn(dbg_arg, "\r\n");

                        cmd_done_len = (int)(dbg_arg - dbg_inbuf) + arglen;
                        for (arglen--; arglen >= 0 && !crm_isgraph(dbg_arg[arglen]); arglen--)
                            ;
                        arglen++;

                        if (!strncmp(dbg_arg, "*", arglen))
                        {
                            // kill 'em all!
                            dbg_arg = ".*";
                            arglen = (int)strlen(dbg_arg);
                        }

                        //       compile the regex
                        status = crm_regcomp(&preg, dbg_arg, arglen, REG_EXTENDED);
                        if (status != 0)
                        {
                            CRM_ASSERT(dbg_outbuf);
                            CRM_ASSERT(dbg_iobuf_size > 0);

                            crm_regerror(status, &preg, dbg_outbuf, dbg_iobuf_size);
                            fprintf(stderr,
                                    "\n\n"
                                    "ERROR: Regular Expression Compilation Problem in variable search pattern '%.*s' "
                                    "while trying to remove those matching variables from the watch list: %s\n\n",
                                    arglen,
                                    dbg_arg,
                                    dbg_outbuf);
                        }
                        else
                        {
                            regmatch_t match[2];
                            int count = 0;
                            int k;

                            for (k = j = 0; show_expr_list[j]; j++)
                            {
                                const char *varname = show_expr_list[j];
                                int varlen = (int)strlen(varname);

                                // see if this variable matches our regex:
                                status = crm_regexec(&preg,
                                        varname, varlen,
                                        WIDTHOF(match), match,
                                        0, NULL);
                                if (status != REG_OK && status != REG_NOMATCH)
                                {
                                    CRM_ASSERT(dbg_outbuf);
                                    CRM_ASSERT(dbg_iobuf_size > 0);

                                    crm_regerror(status, &preg, dbg_outbuf, dbg_iobuf_size);
                                    fprintf(stderr,
                                            "\n\n"
                                            "ERROR: Regular Expression Execution Problem in variable search pattern '%.*s' "
                                            "while trying to check variable '%.*s'(len: %d) for removal from the watch list: %s\n\n",
                                            arglen,
                                            dbg_arg,
                                            varlen,
                                            varname,
                                            varlen,
                                            dbg_outbuf);

                                    break;
                                }
                                else if (status == REG_OK)
                                {
                                    // regex matches the variable - at least part of the variable and that's good enough for us.
                                    CRM_ASSERT(match[0].rm_eo >= 0);
                                    CRM_ASSERT(match[0].rm_so >= 0);

                                    fprintf(stderr, "removed watched expression #%d: %s\n",
                                            j + 1, show_expr_list[j]);
                                    free(show_expr_list[j]);
                                    show_expr_list[j] = NULL;

                                    count++;

                                    show_watched_expr = 1;
                                }
                                else
                                {
                                    // shift other expressions one down in the list to close the gap: including NULL sentinel

                                    show_expr_list[k++] = show_expr_list[j];
                                }
                            }
                            // when we get here, all valid remaining expressions are there, yet the NULL sentinel is missing:
                            show_expr_list[k] = NULL;


                            // done with the regex
                            crm_regfree(&preg);


                            if (count == 0)
                            {
                                fprintf(stderr,
                                        "You specified a watched regex expression '%.*s' which "
                                        "did not match any of the watched expressions. None were removed.\n",
                                        arglen,
                                        dbg_arg);
                            }
                        }
                    }
                }
            }
            break;

        case 'i':
            // skip until ';' seperator or newline:
            cmd_done_len = (int)strcspn(dbg_inbuf, ";\r\n");

            fprintf(stderr, "Isolating %s", &dbg_inbuf[1]);
            fprintf(stderr, "NOT YET IMPLEMENTED!  Sorry.\n");

            show_watched_expr = 1;
            break;

        case 'v':
            {
                int i, j;
                int stmtnum;
                int endstmtnum;

                show_watched_expr = 1;

                // skip until ';' seperator or newline:
                cmd_done_len = (int)strcspn(dbg_inbuf, ";\r\n");

                i = decode_line_number_range(&dbg_inbuf[1],
                        0,
                        &stmtnum,
                        &endstmtnum,
                        csl->cstmt,
                        ";");

                if (i < 0)
                {
                    fprintf(stderr,
                            "Debugger: Invalid number range specified: %s\n",
                            &dbg_inbuf[1]);
                    fprintf(stderr,
                            "          Accepted formats (where N and M are numbers):\n"
                            "            N.M\n"
                            "            N,M\n"
                            "            <M\n"
                            "            >M\n"
                            "            N~M\n"
                            "          N and/or M may be removed to use default value there.\n");
                }
                else
                {
                    cmd_done_len = i + 1;

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

                    fprintf(stderr,
                            "\n--- listing: --------------------------------------------\n");
                    for ( ; stmtnum <= endstmtnum; stmtnum++)
                    {
                        int stmt_len;

                        CRM_ASSERT(stmtnum >= 0 && stmtnum <= csl->nstmts);
                        j = csl->mct[stmtnum]->start;
                        stmt_len = csl->mct[stmtnum + 1]->start - j;
                        fprintf(stderr, /* "statement" */ "%c%c%4d: %.*s",
                                (csl->cstmt == stmtnum ? '*' : ' '),
                                (csl->mct[stmtnum]->stmt_break ? 'B' : ' '),
                                stmtnum,
                                stmt_len, csl->filetext + j);
                    }
                }
            }
            break;

        case 'j':
            {
                int nextstmt = 0;
                int i;
                int vindex;

                // skip until ';' seperator or newline:
                if (dbg_arg)
                {
                    cmd_done_len = (int)(dbg_arg - dbg_inbuf);
                    cmd_done_len += (int)strcspn(dbg_arg, "; \t\r\n");

                    i = sscanf(dbg_arg, "%d", &nextstmt);
                    if (i == 0)
                    {
                        //    maybe the user put in a label?
                        int tstart;
                        int tlen;
                        if (crm_nextword(dbg_arg, (int)strlen(dbg_arg), 0,
                                    &tstart, &tlen))
                        {
                            memmove(dbg_inbuf, &dbg_arg[tstart], tlen);
                            dbg_inbuf[tlen] = 0;
                            vindex = crm_vht_lookup(vht, dbg_inbuf, tlen);
                            if (vht[vindex] == NULL)
                            {
                                fprintf(stderr, "No label '%s' in this debug command.  ", dbg_inbuf);
                                fprintf(stderr, "Staying at line %d\n", csl->cstmt);
                                nextstmt = csl->cstmt;
                            }
                            else
                            {
                                nextstmt = vht[vindex]->linenumber;
                            }
                        }
                        else
                        {
                            fprintf(stderr, "No label '%s' in this debug command.  ", dbg_inbuf);
                            fprintf(stderr, "Staying at line %d\n", csl->cstmt);
                            nextstmt = csl->cstmt;
                        }
                    }

                    if (nextstmt < 0)
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

#if defined (TOLERATE_FAIL_AND_OTHER_CASCADES)
                        csl->cstmt = nextstmt;
#else
                        csl->cstmt = nextstmt;
#endif

                        CRM_ASSERT(csl->cstmt >= 0);
                        CRM_ASSERT(csl->cstmt <= csl->nstmts);
                        current_crm_command = csl->mct[csl->cstmt];
                        current_crm_commanddef = current_crm_command->stmt_def;
                        if (!current_crm_command || !current_crm_commanddef)
                        {
                            untrappableerror("The debugger has found that the CRM114 compiler screwed up somewhere. ",
                                    "The inspected statement doesn't seem valid. You may scream now...");

                            inside_debugger = 0;                     // signal the other code section we're now LEAVING the debugger!

                            crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 1, "");
                            return -1;
                        }
                        dbg_exec_nestlevel = current_crm_command->nest_level;

                        if (!ret_code)
                            ret_code = 1;
                    }
                }
                else
                {
                    fprintf(stderr,
                            "debugger: 'j'ump command is missing a parameter: line number or label accepted.\n");
                }
            }
            //ret_code = 1;
            parsing_done = 1;
            show_watched_expr = 1;

            if (remember_cmd == 1)
                remember_cmd = 2; // signal we ARE allowed to store this commandline
            break;

        case 'b':
            {
                //  is there a breakpoint to make?
                int breakstmt;
                int i;
                int vindex;
                breakstmt = -1;

                if (dbg_arg)
                {
                    // skip until ';' seperator or whitespace/newline:
                    cmd_done_len = (int)(dbg_arg - dbg_inbuf);
                    cmd_done_len += (int)strcspn(dbg_arg, "; \t\r\n");

                    i = sscanf(dbg_arg, "%d", &breakstmt);
                    if (i == 0)
                    {
                        //    maybe the user put in a label?
                        int tstart;
                        int tlen;
                        if (crm_nextword(dbg_arg, (int)strlen(dbg_arg), 0,
                                    &tstart, &tlen))
                        {
                            tlen = CRM_MIN(tlen, WIDTHOF(dbg_inbuf) - 1);
                            memmove(dbg_inbuf, &dbg_arg[tstart], tlen);
                            dbg_inbuf[tlen] = 0;
                            vindex = crm_vht_lookup(vht, dbg_inbuf, tlen);
                            fprintf(stderr, "vindex = %d\n", vindex);
                            if (vht[vindex] == NULL)
                            {
                                fprintf(stderr, "No label '%s' in this debug command.  ", dbg_inbuf);
                                fprintf(stderr, "No breakpoint change made.\n");
                                breakstmt = -1;
                            }
                            else
                            {
                                breakstmt = vht[vindex]->linenumber;
                            }
                        }
                        else
                        {
                            fprintf(stderr, "No label '%s' in this debug command.  ", dbg_inbuf);
                            fprintf(stderr, "No breakpoint change made.\n");
                            breakstmt = -1;
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
                    // toggle breakpoint:
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
                else
                {
                    // list existing breakpoints.
                    int stmtnum;

                    show_watched_expr = 1;

                    fprintf(stderr,
                            "\n--- breakpoints: --------------------------------------------\n");
                    for (stmtnum = 0; stmtnum <= csl->nstmts; stmtnum++)
                    {
                        int stmt_len;
                        int j;

                        if (!csl->mct[stmtnum]->stmt_break)
                            continue;

                        j = csl->mct[stmtnum]->start;
                        stmt_len = csl->mct[stmtnum + 1]->start - j;

                        fprintf(stderr, /* "statement" */ "%c%c%4d: %.*s",
                                (csl->cstmt == stmtnum ? '*' : ' '),
                                'B',
                                stmtnum,
                                stmt_len, csl->filetext + j);
                    }
                }
            }
            //ret_code = 0;
            parsing_done = 1;
            break;

        case 'a':
            {
                //  do a debugger-level alteration
                //    maybe the user put in a label?
                int vstart, vlen;
                int vindex;
                int ostart, oend, olen;

                if (dbg_arg)
                {
                    // skip until newline (';' seperator is not accepted as end of this command):
                    cmd_done_len = (int)strcspn(dbg_arg, "\r\n");

                    if (crm_nextword(dbg_arg, (int)strlen(dbg_arg), 0,
                                &vstart, &vlen))
                    {
                        vlen = CRM_MIN(vlen, WIDTHOF(dbg_inbuf) - 1);
                        memmove(dbg_inbuf, &dbg_arg[vstart], vlen);
                        dbg_inbuf[vlen] = 0;
                        vindex = crm_vht_lookup(vht, dbg_inbuf, vlen);
                        if (vht[vindex] == NULL)
                        {
                            fprintf(stderr, "No variable '%s' in this program.  ", dbg_inbuf);
                        }

                        //     now grab what's left of the input as the value to set
                        //
                        ostart = (int)(dbg_arg - dbg_inbuf) + vlen;
                        while (dbg_inbuf[ostart] != '/' && dbg_inbuf[ostart] != 0)
                            ostart++;
                        ostart++;
                        oend = ostart + 1;
                        while (dbg_inbuf[oend] != '/' && dbg_inbuf[oend] != 0)
                        {
                            if (dbg_inbuf[oend] == '\\' && dbg_inbuf[oend] != 0)
                                oend++;
                            oend++;
                        }

                        cmd_done_len = oend + 1;                 // skip terminating '/'

                        CRM_ASSERT(oend - ostart < dbg_iobuf_size - 1);
                        memmove(dbg_outbuf,
                                &dbg_inbuf[ostart],
                                oend - ostart);
                        dbg_outbuf[oend - ostart] = 0;
                        olen = crm_nexpandvar(dbg_outbuf, oend - ostart, dbg_iobuf_size, vht, tdw);
                        crm_destructive_alter_nvariable(dbg_inbuf, vlen, dbg_outbuf, olen);

                        show_watched_expr = 1;
                    }
                    else
                    {
                        fprintf(stderr, "debugger: 'a'lter command is missing parameters.\n");
                    }
                }
                else
                {
                    fprintf(stderr, "debugger: 'a'lter command is missing parameters.\n");
                }
            }
            break;

        case 'f':
            debug_countdown = 0;
            dbg_step_mode |= SM_PENDING;

#if defined (TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = current_crm_command->fail_index;
#else
            csl->cstmt = current_crm_command->fail_index - 1;
#endif
            fprintf(stderr, "Forward to }, next statement : %d\n", csl->cstmt);
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            current_crm_command = csl->mct[csl->cstmt];
            current_crm_commanddef = current_crm_command->stmt_def;
            if (!current_crm_command || !current_crm_commanddef)
            {
                untrappableerror("The debugger has found that the CRM114 compiler screwed up somewhere. ",
                        "The inspected statement doesn't seem valid. You may scream now...");

                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 2, "");
                return -1;
            }
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
            dbg_exec_nestlevel = current_crm_command->nest_level;

            if (!ret_code)
                ret_code = 1;
            parsing_done = 1;
            show_watched_expr = 1;

            if (remember_cmd == 1)
                remember_cmd = 2; // signal we ARE allowed to store this commandline
            break;

        case 'l':
            debug_countdown = 0;
            dbg_step_mode |= SM_PENDING;

#if defined (TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_jump = csl->mct[csl->cstmt]->liaf_index;
#else
            csl->cstmt = current_crm_command->liaf_index;
#endif
            fprintf(stderr, "Backward to {, next statement : %d\n", csl->cstmt);
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            current_crm_command = csl->mct[csl->cstmt];
            current_crm_commanddef = current_crm_command->stmt_def;
            if (!current_crm_command || !current_crm_commanddef)
            {
                untrappableerror("The debugger has found that the CRM114 compiler screwed up somewhere. ",
                        "The inspected statement doesn't seem valid. You may scream now...");

                inside_debugger = 0;                 // signal the other code section we're now LEAVING the debugger!

                crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 3, "");
                return -1;
            }
            dbg_exec_nestlevel = current_crm_command->nest_level;

            if (!ret_code)
                ret_code = 1;
            parsing_done = 1;
            show_watched_expr = 1;

            if (remember_cmd == 1)
                remember_cmd = 2; // signal we ARE allowed to store this commandline
            break;

        case 'd':
            // dump a list of all variables
            {
                int i;
                int len;
                int maxlen = 0;
                int len_decades;
                int width_decades;
                VHT_CELL **vht_ref = calloc(vht_size, sizeof(vht_ref[0]));
                if (!vht_ref)
                {
                    untrappableerror("Cannot allocate debugger memory",
                            "Stick a fork in us; we're _done_.");
                }

                len = 0;
                for (i = 0; i < vht_size; i++)
                {
                    if (vht[i] == NULL)
                        continue;
                    if (vht[i]->nlen > 0)
                    {
                        vht_ref[len++] = vht[i];
                        maxlen = CRM_MAX(maxlen, vht[i]->nlen);
                    }
                }

                QSORT(VHT_CELL *, vht_ref, len, dbg_vht_compare);

                len_decades = (int)ceil(log10(len));
                width_decades = (int)ceil(log10(maxlen));

                for (i = 0; i < len; i++)
                {
                    /*
                     * Note that, when :label:s have been isolate'd, the code in there
                     * MAY have modified the VHT record to look like any other regular variable;
                     * in that case, the only way to detect if we're talking about a :label:
                     * instead of a :variable: is by its usage in the code. :-(
                     */
                    fprintf(stderr, "var[%*d]: (len=%*d; type=%s) name='",
                            len_decades, i,
                            width_decades, vht_ref[i]->nlen,
                            (vht_ref[i]->valtxt == NULL ? "LABEL" : "?VAR?"));
                    fwrite_ASCII_Cfied(stderr,
                            vht_ref[i]->nametxt + vht_ref[i]->nstart,
                            vht_ref[i]->nlen);
                    fprintf(stderr, "'\n");
                }

                free(vht_ref);
            }
            break;

        case '?':
        case 'h':
            fprintf(stderr, "a :var: /value/ - alter :var: to /value/\n");
            fprintf(stderr, "b <n> - toggle breakpoint on line <n>\n");
            fprintf(stderr, "b <label> - toggle breakpoint on <label>\n");
            fprintf(stderr, "c     - continue execution till breakpoint or end\n");
            fprintf(stderr, "c <n> - execute <number> more statements\n");
            fprintf(stderr, "c <m> - execute next statement while setting executing <mode> to\n");
            fprintf(stderr, "        n / d    - reset to default: single step [into], break at any line\n");
            fprintf(stderr, "        i        - single step [into]\n");
            fprintf(stderr, "        s        - skip calls, i.e. skip to next stmt on same call depth\n");
            fprintf(stderr, "        o        - step out, i.e. skip to stmt after 'return'ing outa here\n");
            fprintf(stderr, "        f        - 'fail': skip until } terminating this scope block\n");
            fprintf(stderr, "      Note: 'o' and 'f' are auto-reset: they're active only once.\n");
            fprintf(stderr, "        a        - break at any statement\n");
            fprintf(stderr, "        e        - break only at executable statements\n");
            fprintf(stderr, "      Note: 'a'/'e' can be combined with any one of 'i'/'s'/'o'/'f'.\n");
            fprintf(stderr, "c <m> <n> - the above combined: set mode + step <n> stmt\n");
            fprintf(stderr, "d     - dump a list of all variables known to the program.\n");
            fprintf(stderr, "e <e> - expand expression <e>; if none specified, show the previous one again.\n");
            fprintf(stderr, "+ <e> - watch the expanded expression <e>.\n"
                            "        No arg? -> add last 'e' expr to show list\n"
                            "        Is <e> not a :@:, :#:, :+: or :@: expression? Assume it to be a regex\n"
                            "        which will select any (partially) matching existing variable to the watch\n"
                            "        list. Make sure an '_' is in there when you want to add system vars too.\n");
            fprintf(stderr, "- <n> - remove the watched expression #<n> from the show list.\n"
                            "        <n> not a number? Treat as a regex and remove any (partially) matching\n"
                            "        watched expression.\n");
            fprintf(stderr, "f     - fail forward to block-closing '}'\n");
            fprintf(stderr, "h     - show this on-line help. Add extra 'h' or '?' for list of examples\n");
            fprintf(stderr, "?     - same as 'h'.\n");
            fprintf(stderr, "j <n> - jump to statement <number>\n");
            fprintf(stderr, "j <label> - jump to statement <label>\n");
            fprintf(stderr, "l     - liaf backward to block-opening '{'\n");
            fprintf(stderr, "m <m> - set executing <mode>. See 'c' above for <m> mode flags list\n");
            fprintf(stderr, "n     - execute next statement (same as 'c 1')\n");
            fprintf(stderr, "n <m> - same as 'c <m> 1'\n");
            fprintf(stderr, "q     - quit the program and exit\n");
            fprintf(stderr, "t     - toggle user-level tracing\n");
            fprintf(stderr, "T     - toggle system-level tracing\n");
            fprintf(stderr, "v <n>.<m> - view source code statement <n> till <m>.\n"
                            "        No <n> given: show current statement\n"
                            "        Alternatives: 'v >5' (type '>' char!) = show\n"
                            "        current and 5 extra; 'v ~3' = show context of\n"
                            "        3 lines before till 3 lines after\n");
            fprintf(stderr, ";     - no-op: separate multiple commands\n");
            fprintf(stderr, ".     - repeat previous commandline which did not\n"
                            "        start with the '.' command\n");

            if (dbg_arg)
            {
                fprintf(stderr, "\n\n");
                fprintf(stderr, "Some examples\n");
                fprintf(stderr, "=============\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "To start off:\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "m ie\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Set Step Mode to default 'step into' but only show the prompt again,\n");
                fprintf(stderr, "        when we've hit an executable statement. This also means '{' and '}'\n");
                fprintf(stderr, "        braces will be skipped, together with any commant lines.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "n ; v >5\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Very handy command sequence, which will execute one more statement,\n");
                fprintf(stderr, "        then show the currently active and 5 more source lines. Once you've\n");
                fprintf(stderr, "        executed this debugger 'macro' once, follow up with:\n");
                fprintf(stderr, "\n");
                fprintf(stderr, ".\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Execute previous 'debugger macro' again. A 'debugger macro' is just the\n");
                fprintf(stderr, "        last line you typed in the debugger, which did NOT start with a '.'\n");
                fprintf(stderr, "        dot. By hitting ENTER on the debugger prompt, you 'erase' any 'macro'\n");
                fprintf(stderr, "        in memory and '.' will do nothing.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "...\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Execute the last recorded 'debugger macro' 3 times in a row.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "+ :*:filename:\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Add the expression ':*:filename:' to the watched expressions, which are\n");
                fprintf(stderr, "        printed by the debugger each time when a prompt is to be shown. Of\n");
                fprintf(stderr, "        course, the expressions support all CRM114 :?: expression types,\n");
                fprintf(stderr, "        include :@: math expressions.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Note: invalid expressions do not result in error messages; these are\n");
                fprintf(stderr, "        simply processed as best as possible and the result printed on screen.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Note 2: when you use '+', semicolons which can usually be used to\n");
                fprintf(stderr, "        separate multiple commands on a single debugger prompt line, are\n");
                fprintf(stderr, "        ignored: anything following the '+' is considered part of the\n");
                fprintf(stderr, "        expression to be watched.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "+ :_.*HOME\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Add any system variable (i.e. variable which starts with an '_'\n");
                fprintf(stderr, "        underscore) matching the ':_.*HOME' regex. This means a variable\n");
                fprintf(stderr, "        called ':_HOME_VAR:' will be added to the watch list too as partial\n");
                fprintf(stderr, "        matches are accepted!\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "- 4\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        remove the 4th watched expression\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "- *\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Special case, identical to '- .*' which remove all watched expressions\n");
                fprintf(stderr, "        as each will match the '.*' regex.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "Extended examples\n");
                fprintf(stderr, "=================\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "c do 5\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Reset Step mode to default: break at ANY statement (including comments\n");
                fprintf(stderr, "        and braces) and then run until EITHER you've 'executed' 5 lines, OR\n");
                fprintf(stderr, "        until you've finally exited the function by having executed a 'RETRUN'\n");
                fprintf(stderr, "        statement. Note that when you execute 'RETURN' when the count is not\n");
                fprintf(stderr, "        '5 lines executed', CRM114 will CONTINUE RUNNING until the 5 statements\n");
                fprintf(stderr, "        have been executed.\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "b 511\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "        Put a breakpoint to program line 511. Note that the debugger will\n");
                fprintf(stderr, "        ALWAYS pop up and show a breakpoint on the given line, even when step\n");
                fprintf(stderr, "        mode is 'E' (executable statements only) and the breakpoint is located\n");
                fprintf(stderr, "        at a brace or comment line.\n");

                cmd_done_len = (int)strcspn(dbg_inbuf, "\r\n");
            }
            break;

        default:
            {
                // skip whitespace:
                int len = (int)strspn(dbg_inbuf, " \t\r\n");

                cmd_done_len = len;
                if (cmd_done_len == 0)
                {
                    fprintf(stderr,
                            "Command '%c' unrecognized - type \"h\" for help.\n",
                            dbg_inbuf[0]);

                    // skip until ';' seperator or newline:
                    cmd_done_len = (int)strcspn(dbg_inbuf, ";\r\n");
                }
            }
            break;
        }

        if (remember_cmd == 2)         // only do so if we ARE allowed to store this commandline
        {
            strcpy(dbg_last_command, dbg_inbuf);
        }

        if (cmd_done_len > 0)
        {
            // skip additional whitespace and separator semicolons:
            int len = (int)strspn(dbg_inbuf + cmd_done_len, "; \t\r\n");

            cmd_done_len += len;
            len = CRM_MIN(strlen(dbg_inbuf + cmd_done_len), WIDTHOF(dbg_inbuf) - 1);
            memmove(dbg_inbuf, dbg_inbuf + cmd_done_len, strlen(dbg_inbuf + cmd_done_len));
            dbg_inbuf[len] = 0;
        }
    }

    inside_debugger = 0;             // signal the other code section we're now LEAVING the debugger!

    crm_analysis_mark(&analysis_cfg, MARK_DEBUG_INTERACTION, 4, "");

    return ret_code;
}

