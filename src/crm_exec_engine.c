//	crm_exec_engine.c - core of CRM114, execution engine toplevel

// Copyright 2001-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"



//
//        Here it is, the core of CRM114 - the execution engine toplevel,
//      which, given a CSL and a CDW, executes the CSL against the CDW
//

int crm_invoke(CSL_CELL **csl_ref)
{
    int i, j, k;
    int status;
    // int done;
    int slen;
    //    a pointer to the current statement argparse block.  This gets whacked
    //    on every new statement.
    ARGPARSE_BLOCK *apb = NULL;
    CSL_CELL *csl;

    //     timer1 and timer2 are for time profiling.
    //
#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
    int64_t timer1 = 0;
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
    int64_t timer1 = 0;
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
    int64_t timer1 = 0;
#elif defined(HAVE_CLOCK)
    int64_t timer1 = 0;
#else
    // nada
#endif
#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
    struct tms timer1a = { 0 };
#endif

    int tstmt;

    CRM_ASSERT(csl_ref);
    CRM_ASSERT(*csl_ref);
    csl = *csl_ref;

    tstmt = 0;
    i = j = k = 0;

    status = 0;

    //    Sanity check - don't try to execute a file before compilation
    if (csl->mct == NULL)
    {
        untrappableerror("Can't execute a file without compiling first.\n",
                         "This means that CRM114 is somehow broken.");
    }

    //    empty out the alius stack (nothing FAILed yet.)
    //
    for (i = 0; i < MAX_BRACKETDEPTH; i++)
        csl->aliusstk[i] = 1;

    //    if there was a command-line-specified BREAK, set it.
    //
    if (cmdline_break > 0)
    {
        if (cmdline_break <= csl->nstmts)
        {
            csl->mct[cmdline_break]->stmt_break = 1;
        }
    }

    if (user_trace)
    {
        fprintf(stderr, "Starting to execute %s at line %d\n",
                csl->filename, csl->cstmt);
    }

    //   initialize timers ?
    //
    // [i_a] for better timing results (timers have a resolution which is definitely
    //       (far) less than required to accurately time instructions by measuring
    //       start to end as was done by Bill: this will result in significant time
    //       losses as start and end times both contain something akin to
    //       'rounding errors' (inaccuricies due to low timer frequency).
    //       To prevent 'rounding losses' in the timing measurements here, we measure
    //       end to end; only at the start do we init the timer once for a
    //       start to end measurement. This way, no time is 'lost' and when executing
    //       lines repetively, the inaccuracies due to 'time overflowing into the next
    //       statement' will average out.
    if (profile_execution)
    {
#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
        LARGE_INTEGER t1;
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
        struct timespec t1;
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
        struct timeval t1;
#elif defined(HAVE_CLOCK)
        clock_t t1;
#else
        // nada
#endif
#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
        struct tms t1a;
        clock_t t2a;
#endif

#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
        if (!QueryPerformanceCounter(&t1))
        {
            t1.QuadPart = 0;
        }
        timer1 = t1.QuadPart;
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
        if (!clock_gettime(CLOCK_REALTIME, &t1))
        {
            timer1 = ((int64_t)t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        }
        else
        {
            timer1 = 0; // unknown; due to error
        }
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
        if (!gettimeofday(&t1))
        {
            timer1 = ((int64_t)t1.tv_sec) * 1000000LL + t1.tv_usec;
        }
        else
        {
            timer1 = 0; // unknown; due to error
        }
#elif defined(HAVE_CLOCK)
        t1 = clock();
        if (t1 == (clock) - 1)
        {
            timer1 = 0; // unknown; due to error
        }
        else
        {
            timer1 = t1;
        }
#else
#endif

#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
        t2a = times(&t1a);
        if (t2a == (clock_t)(-1))
        {
            memset(&timer1a, 0, sizeof(timer1a)); // unknown; due to error
        }
        else
        {
            timer1a = t1a;
        }
#endif
    }

    // signal the error handlers, etc. that we are running the script code now:
    csl->running = 1;

invoke_top:

    tstmt = csl->cstmt;   // [i_a] also used by the marker analysis profiling calls
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    csl->cstmt_recall = tstmt;
    csl->next_stmt_due_to_fail = -1;
    csl->next_stmt_due_to_trap = -1;
    csl->next_stmt_due_to_jump = -1;
    csl->next_stmt_due_to_debugger = -1;
#endif

    if (csl->cstmt >= csl->nstmts)
    {
        //  OK, we're at the end of the program.  When this happens,
        //  we know we can exit this invocation of the invoker
        if (user_trace)
            fprintf(stderr, "Finished the program %s.\n", csl->filename);
        // done = 1;
        // status = 0;
        goto invoke_done;
    }

    slen = csl->mct[csl->cstmt + 1]->fchar - csl->mct[csl->cstmt]->fchar;

    if (user_trace)
    {
        fprintf(stderr, "\nParsing line %d :\n", csl->cstmt);
        fprintf(stderr, " -->  ");
#if 0
        for (i = 0; i < slen; i++)
            fprintf(stderr, "%c",
                    csl->filetext[csl->mct[csl->cstmt]->fchar + i]);
#else
        fwrite_ASCII_Cfied(stderr,
                           csl->filetext + csl->mct[csl->cstmt]->fchar,
                           slen);
#endif
        fprintf(stderr, "\n");
    }


    //    THIS IS THE ULTIMATE SCREAMING TEST - CHECK VHT EVERY LOOP
    //     TURN THIS ON ONLY IN EXTREMIS!
    // do a GC on the whole tdw:
    //       crm_compress_tdw_section (tdw->filetext, 0, tdw->nchars);



    //   Invoke the common declensional parser on the statement only if it's
    //   an executable statement.
    //
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif
    switch (csl->mct[csl->cstmt]->stmt_type)
    {
    //
    //   Do the processing that all statements need (well, _almost_ all.)
    //
    case CRM_NOOP:
    case CRM_BOGUS:
        break;

    default:
        //         if we've already generated the argparse block (apb) for this
        //         statement, we use it, otherwise, we create one.
        //
#if !FULL_PARSE_AT_COMPILE_TIME
        if (!csl->mct[csl->cstmt]->apb)
        {
            csl->mct[csl->cstmt]->apb = calloc(1, sizeof(csl->mct[csl->cstmt]->apb[0]));
            if (!csl->mct[csl->cstmt]->apb)
            {
                untrappableerror("Couldn't alloc the space to incrementally "
                                 "compile a statement.  ",
                                 "Stick a fork in us; we're _done_.\n");
            }
            //  we now have the statement's apb allocated; we point the generic
            //  apb at the same place and run with it.
            apb = csl->mct[csl->cstmt]->apb;
            (void)crm_statement_parse(
                &(csl->filetext[csl->mct[csl->cstmt]->fchar]),
                slen,
                csl->mct[csl->cstmt],
                apb);
        }
        else
        {
            //    The CSL->MCT->APB was valid, we can just reuse the old apb.
            if (internal_trace)
                fprintf(stderr, "JIT parse reusing line %d\n", csl->cstmt);
            apb = csl->mct[csl->cstmt]->apb;
        }
#else
        apb = &csl->mct[csl->cstmt]->apb;
#endif
        //    Either way, the flags might have changed, so we run the
        //    standard flag parser against the flags found (if any)
        {
            char flagz[MAX_PATTERN];
            int fl;

            CRM_ASSERT(apb != NULL);
            fl = crm_get_pgm_arg(flagz, MAX_PATTERN, apb->a1start, apb->a1len);
            fl = crm_nexpandvar(flagz, fl, MAX_PATTERN, vht, tdw);
            //    fprintf(stderr,
            //           "flagz --%s-- len %d\n", flagz, (int)strlen(flagz));
            apb->sflags = crm_flagparse(flagz, fl, csl->mct[csl->cstmt]->stmt_def);
        }
        break;
    }
    //    and maybe drop into the debugger?
    //
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif
    cycle_counter++;
    if (debug_countdown > 0)
        debug_countdown--;
    if (debug_countdown == 0
       || csl->mct[csl->cstmt]->stmt_break)
    {
        i = crm_debugger(csl, (csl->mct[csl->cstmt]->stmt_break ? CRM_DBG_REASON_BREAKPOINT : CRM_DBG_REASON_UNDEFINED), NULL);
        if (i == -1)
        {
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 6);
            }
            else
            {
                exit(EXIT_SUCCESS);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        if (csl->next_stmt_due_to_debugger >= 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, "Engine: debugger jump from %d to %d/%d\n", csl->cstmt, csl->next_stmt_due_to_debugger, csl->nstmts);
            }
            csl->cstmt = csl->next_stmt_due_to_debugger;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            tstmt = csl->cstmt;
        }
#endif
        if (i == 1)
            goto invoke_top;
    }

#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif
    if (user_trace)
    {
        fprintf(stderr, "\nExecuting line %d:\n", csl->cstmt);
        fprintf(stderr, " -->  ");
#if 0
        for (i = 0; i < slen; i++)
            fprintf(stderr, "%c",
                    csl->filetext[csl->mct[csl->cstmt]->fchar + i]);
#else
        fwrite_ASCII_Cfied(stderr, csl->filetext + csl->mct[csl->cstmt]->fchar, slen);
#endif
        fprintf(stderr, "\n");
    }


    //  so, we're not off the end of the program (yet), which means look
    //  at the statement type and see if it's somethign we know how to
    //  do, otherwise we make a nasty little noise and continue onward.
    //  Dispatch is done on a big SWITCH statement

    CRM_ASSERT(csl->mct[csl->cstmt] != NULL);
    CRM_ASSERT(csl->mct[csl->cstmt + 1] != NULL);

    crm_analysis_mark(&analysis_cfg,
                      MARK_OPERATION,
                      csl->cstmt,
                      "iL",
                      csl->mct[csl->cstmt]->stmt_type,
                      (unsigned long long int)csl->mct[csl->cstmt]->apb.sflags);

#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif
    switch (csl->mct[csl->cstmt]->stmt_type)
    {
    case CRM_NOOP:
    case CRM_LABEL:
        {
            if (user_trace)
            {
                fprintf(stderr, "Statement %d is non-executable, continuing.\n",
                        csl->cstmt);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_OPENBRACKET:
        {
            //  the nest_level+1 is because the statements in front are at +1 depth
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level + 1] = 1;
            if (user_trace)
            {
                fprintf(stderr, "Statement %d is an openbracket. depth now %d.\n",
                        csl->cstmt, 1 + csl->mct[csl->cstmt]->nest_level);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CLOSEBRACKET:
        {
            if (user_trace)
            {
                fprintf(stderr, "Statement %d is a closebracket. depth now %d.\n",
                        csl->cstmt, csl->mct[csl->cstmt]->nest_level);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_BOGUS:
        {
            int width = CRM_MIN(1024, csl->mct[csl->cstmt + 1]->start - csl->mct[csl->cstmt]->start);

            fatalerror_ex(SRC_LOC(),
                          "Statement %d is bogus!!!  Here's the text:\n%.*s%s",
                          csl->cstmt,
                          width,
                          &csl->filetext[csl->mct[csl->cstmt]->start],
                          (csl->mct[csl->cstmt + 1]->start - csl->mct[csl->cstmt]->start > width
                           ? "(...truncated)"
                           : "")
                         );
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_EXIT:
        {
            int retval;
            int retlen;
            char retstr[MAX_PATTERN];
            CRM_ASSERT(apb != NULL);
            retlen = crm_get_pgm_arg(retstr, MAX_PATTERN, apb->s1start, apb->s1len);
            retlen = crm_nexpandvar(retstr, retlen, MAX_PATTERN, vht, tdw);
            CRM_ASSERT(retlen < MAX_PATTERN);
            retstr[retlen] = 0;
            retval = 0;
            if (retlen > 0)
            {
                if (1 != sscanf(retstr, "%d", &retval))
                {
                    nonfatalerror("Failed to decode the CRM114 exit code: ", retstr);
                }
            }
            if (user_trace)
            {
                fprintf(stderr, "Exiting at statement %d with value %d\n",
                        csl->cstmt, retval);
            }
            //if (profile_execution)
            //  crm_output_profile (csl);
            //      exit (retval);
            status = retval;
            // done = 1;
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            CRM_ASSERT(tstmt == csl->cstmt);
#endif
            goto invoke_done;
        }
        break;

    case CRM_RETURN:
        {
            CSL_CELL *old_csl;
            unsigned int retlen;
            char *namestart;
            unsigned int namelen;
            if (user_trace)
            {
                fprintf(stderr, "Returning to caller at statement %d\n",
                        csl->cstmt);
            }

            //    We can now pop back up to the previous context
            //    just by popping the most recent csl off.
            if (internal_trace)
            {
                fprintf(stderr, "Return - popping CSL back to %p (we're at call depth %d)\n",
                        csl->caller, csl->calldepth);
            }

            //     Do the argument transfer here.
            //     Evaluate the "subject", and return that.
            CRM_ASSERT(apb != NULL);
            retlen = 0;
            if (apb->s1len > 0)
            {
                int idx;
                unsigned int noffset;

                retlen = crm_get_pgm_arg(outbuf, MAX_VARNAME, apb->s1start, apb->s1len);
                retlen = crm_nexpandvar(outbuf, retlen, data_window_size, vht, tdw);
                //
                //      Now we have the return value in outbuf, and the return
                //      length in retlen.  Get it's name
                idx = csl->return_vht_cell;
                if (idx < 0)
                {
                    if (user_trace)
                        fprintf(stderr, "Returning, no return argument given\n");
                }
                else if (csl->calldepth == 0)
                {
                    CRM_ASSERT(idx == 0);

                    if (user_trace)
                        fprintf(stderr, "Returning equals EXITing when doing that at main level\n");
                }
                else
                {
                    int i;
                    //fprintf(stderr, "idx: %lu  vht[idx]: %lu", idx, vht[idx] );
                    noffset = vht[idx]->nstart;
                    //fprintf(stderr, " idx: %lu noffset: %lu\n/", idx, noffset);
                    namestart = &(vht[idx]->nametxt[noffset]);
                    namelen = vht[idx]->nlen;

                    if (user_trace)
                    {
                        fprintf(stderr, " setting return value of >");
#if 0
                        for (i = 0; i < namelen; i++)
                            fprintf(stderr, "%c", namestart[i]);
#else
                        fwrite_ASCII_Cfied(stderr, namestart, namelen);
#endif
                        fprintf(stderr, "<\n");
                    }
                    //     stuff the return value into that variable.
                    //
                    crm_destructive_alter_nvariable(namestart, namelen, outbuf, retlen, csl->calldepth - 1); /* scope = PARENT level! */
                }
            }

            //
            //    Is this the toplevel csl call frame?  If so, return!
            if (csl->caller == NULL)
            {
                status = EXIT_FAILURE;
                if (retlen > 0)
                {
                    // value in 'outbuf': decode to status value --> make 'return /x/' behave like 'exit /x/'
                    outbuf[retlen] = 0;

                    if (1 != sscanf(outbuf, "%d", &status))
                    {
                        status = EXIT_FAILURE;
                    }
                }
                goto invoke_done;
            }

            //
            //     release the current csl cell back to the free memory pool.
            old_csl = csl;
            csl = csl->caller;
            // ensure the caller / global ref gets updated as well!
            *csl_ref = csl;
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            tstmt = csl->cstmt;
            //csl->next_stmt_due_to_fail = -1;
            //csl->next_stmt_due_to_trap = -1;
            //csl->next_stmt_due_to_jump = -1;
#endif

#if 0
            if (csl->filename == old_csl->filename)
                csl->filename_allocated = old_csl->filename_allocated;
#endif
            free_stack_item(old_csl);
            /* free (old_csl); */
            {
                //   properly set :_cd: as well - note that this can be delayed
                //   since the vht index of the return location was actually
                //   set during the CALL statement, not calculated during RETURN.
                char depthstr[33];
                sprintf(depthstr, "%d", csl->calldepth);
                crm_set_temp_var(":_cd:", depthstr, -1, 0);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_GOTO:
        {
            char target[MAX_VARNAME];
            int tarlen;
            //  look up the variable name in the vht.  If it's not there, or
            //  not in our file, call a fatal error.

            CRM_ASSERT(apb != NULL);
            tarlen = crm_get_pgm_arg(target, MAX_VARNAME, apb->s1start, apb->s1len);
            if (tarlen < 2)
            {
                nonfatalerror
                                      ("This program has a GOTO without a place to 'go' to.",
                                      " By any chance, did you leave off the '/' delimiters? ");
            }
            else
            {
                if (internal_trace)
                {
                    fprintf(stderr, "\n    untranslated label (len = %d) '%s',",
                            tarlen, target);
                }

                //   do indirection if needed.
                tarlen = crm_qexpandvar(target, tarlen, MAX_VARNAME, NULL, vht, tdw);
                if (internal_trace)
                {
                    fprintf(stderr, " translates to '%s'.\n", target);
                }

                k = crm_lookupvarline(vht, target, 0, tarlen, -1);

                if (k > 0)
                {
                    if (user_trace)
                    {
                        fprintf(stderr, "GOTO from line %d to line %d\n",
                                csl->cstmt,  k);
                    }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                    csl->next_stmt_due_to_jump = k;
#else
                    csl->cstmt = k; // this gets autoincremented
#endif
                    if (internal_trace)
                    {
                        fprintf(stderr, "GOTO is jumping to statement line: %d/%d\n", k, csl->nstmts);
                    }
                    CRM_ASSERT(csl->cstmt >= 0);
                    CRM_ASSERT(csl->cstmt <= csl->nstmts);
                    //  and going here didn't fail...
                    csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = 1;
                }
                else
                {
                    int conv_count;
                    conv_count = sscanf(target, "%d", &k);
                    if (conv_count == 1)
                    {
                        if (user_trace)
                        {
                            fprintf(stderr, "GOTO from line %d to line %d\n",
                                    csl->cstmt, k);
                        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        csl->next_stmt_due_to_jump = k;
#else
                        csl->cstmt = k - 1; // this gets autoincremented, so we must --
#endif
                        if (internal_trace)
                        {
                            fprintf(stderr, "GOTO is jumping to statement line: %d/%d\n", k, csl->nstmts);
                        }
                        CRM_ASSERT(csl->cstmt >= 0);
                        CRM_ASSERT(csl->cstmt <= csl->nstmts);
                        //  and going here didn't fail...
                        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = 1;
                    }
                    else
                    {
                        //  this is recoverable if we have a trap... so we continue
                        //   execution right to the BREAK.
                        fatalerror(" Can't GOTO the nonexistent label/line: ",
                                   target);
                    }
                }
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_FAIL:
        {
            //       If we get a FAIL, then we should branch to the statement
            //         pointed to by the fail_index entry for that line.
            //
            //              note we cheat - we branch to "fail_index - 1"
            //                and let the increment happen.
            if (user_trace)
                fprintf(stderr, "Executing hard-FAIL at line %d\n", csl->cstmt);
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "FAIL is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);

            //   and mark that we "failed", so an ALIUS will take this as a
            //   failing statement block
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_LIAF:
        {
            //       If we get a LIAF, then we should branch to the statement
            //         pointed to by the liaf_index entry for that line.
            //
            //               (note the "liaf-index - 1" cheat - we branch to
            //               liaf_index -1 and let the increment happen)
            if (user_trace)
                fprintf(stderr, "Executing hard-LIAF at line %d\n", csl->cstmt);
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_jump = csl->mct[csl->cstmt]->liaf_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->liaf_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "LIAF is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->liaf_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_ALIUS:
        {
            //   ALIUS looks at the finish state of the last bracket - if it
            //   was a FAIL-to, then ALIUS is a no-op.  If it was NOT a fail-to,
            //   then ALIUS itself is a FAIL
            if (user_trace)
                fprintf(stderr, "Executing ALIUS at line %d\n", csl->cstmt);
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            if (csl->aliusstk[csl->mct[csl->cstmt]->nest_level + 1] == 1)
            {
                if (user_trace)
                    fprintf(stderr, "prior group exit OK, ALIUS fails forward to statement #%d.\n", csl->mct[csl->cstmt]->fail_index - 1);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
                csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
                if (internal_trace)
                {
                    fprintf(stderr, "ALIUS is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
                }
                CRM_ASSERT(csl->cstmt >= 0);
                CRM_ASSERT(csl->cstmt <= csl->nstmts);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_TRAP:
        {
            //  TRAP is a placeholder statement that holds the regex that
            //  the faulting statement must match.  The background support
            //  code is in crm_trigger_fault that will look at the error string
            //  and see if it matches the regex.
            //
            //  If we get to a TRAP statement itself, we should treat it as
            //  a skip to end of block (that's a SKIP, not a FAIL)

            if (user_trace)
            {
                fprintf(stderr, "Executing a TRAP statement...");
                fprintf(stderr, " this is a NOOP unless you have a live FAULT\n");
            }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "TRAP is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_FAULT:
        {
            char *reason;
            char rbuf[MAX_PATTERN];
            int rlen;
            int fresult;

            //  FAULT forces the triggering of the TRAP; it's a super-FAIL
            //   statement that can skip downward a large number of blocks.
            //
            if (user_trace)
                fprintf(stderr, "Forcing a FAULT at line %d\n", csl->cstmt);
            CRM_ASSERT(apb != NULL);
            rlen = crm_get_pgm_arg(rbuf, MAX_PATTERN, apb->s1start, apb->s1len);
            rlen = crm_nexpandvar(rbuf, rlen, MAX_PATTERN, vht, tdw);

            //   We alloc the reason - better free() it when we take the trap.
            //   in crm_trigger_fault
            //
            reason = calloc(rlen + 1, sizeof(reason[0]));
            if (!reason)
            {
                untrappableerror(
                    "Couldn't alloc 'reason' in CRM_FAULT - out of memory.\n",
                    "Don't you just HATE it when the error fixup routine gets"
                    "an error?!?!");
            }
            memcpy(reason, rbuf, rlen);
            reason[rlen] = 0;

            // [i_a] extension: HIDDEN_DEBUG_FAULT_REASON_VARNAME keeps track of the last error/nonfatal/whatever error report:
            if (debug_countdown > DEBUGGER_DISABLED_FOREVER)
            {
                crm_set_temp_var(HIDDEN_DEBUG_FAULT_REASON_VARNAME, reason, -1, 0);
            }
            crm_set_temp_var(":_fault:", reason, -1, 0);

            fresult = crm_trigger_fault(reason);
            if (fresult != 0)
            {
                fatalerror("Your program has no TRAP for the user defined fault:",
                           reason);
            }
            free(reason);
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_ACCEPT:
        {
            //char varname[MAX_VARNAME];
            int varidx;
            //   Accept:  take the current window, and output it to
            //   standard output.
            //
            //
            if (user_trace)
                fprintf(stderr, "Executing an ACCEPT\n");
            //
            //
            //varname[0] = 0;
            //strcpy(varname, ":_dw:");
            varidx = crm_vht_lookup(vht, ":_dw:", 5, csl->calldepth);
            if (varidx == 0
               || vht[varidx] == NULL)
            {
                fatalerror("This is very strange... there is no data window!",
                           "As such, death is our only option.");
            }
            else
            {
                int len_written = fwrite4stdio(&(vht[varidx]->valtxt[vht[varidx]->vstart]),
                                               vht[varidx]->vlen,
                                               stdout);
                if (len_written != vht[varidx]->vlen)
                {
                    nonfatalerror_ex(SRC_LOC(), "ACCEPT: all data (len = %d) could not be written to the output: "
                                                "errno = %d(%s)\n",
                                     vht[varidx]->vlen,
                                     errno,
                                     errno_descr(errno));
                }
                // [i_a] not needed to flush each time if the output is not stdout/stderr: this is faster
                if (isatty(fileno(stdout)))
                {
                    fflush(stdout);
                }
            }
            //    WE USED TO DO CHARACTER I/O.  OUCH!!!
            //      for (i = 0; i < cdw->nchars ; i++)
            //        fprintf (stdout, "%c", cdw->filetext[i]);
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_MATCH:
        crm_expr_match(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_OUTPUT:
        crm_expr_output(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_WINDOW:
        crm_expr_window(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_ALTER:
        crm_expr_alter(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_EVAL:
        crm_expr_eval(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_HASH:
        {
            //      here's where we surgically alter a variable to a hash.
            //      We have to watch out in case a variable is not in the
            //      cdw (it might be in tdw; that's legal as well.  syntax
            //      is to replace the contents of the variable in the
            //      varlist with hash of the evaluated string.

            char varname[MAX_VARNAME];
            int varlen;
            int vns, vnl;
            char newstr[17]; // place for a hexdumped 64-bit future hash + NUL sentinel
            int newstrlen;
            crmhash_t hval;     //   hash value

            if (user_trace)
                fprintf(stderr, "Executing a HASHing\n");

            //     get the variable name
            CRM_ASSERT(apb != NULL);
            varlen = crm_get_pgm_arg(varname, MAX_VARNAME, apb->p1start, apb->p1len);
            varlen = crm_nexpandvar(varname, varlen, MAX_VARNAME, vht, tdw);
            //   If we didn't get a variable name, we replace the data window!
            if (!crm_nextword(varname, varlen, 0, &vns, &vnl)
               || vnl == 0)
            {
                strcpy(varname, ":_dw:");
                vnl = (int)strlen(varname);
                vns = 0;
            }

            //     get the to-be-hashed pattern, and expand it.
            newstrlen = crm_get_pgm_arg(tempbuf, data_window_size, apb->s1start, apb->s1len);
            //
            //                   if no var given, hash the full data window.
            if (newstrlen == 0)
            {
                strcpy(tempbuf, ":*:_dw:");
                newstrlen = (int)strlen(tempbuf);
            }
            newstrlen = crm_nexpandvar(tempbuf, newstrlen, data_window_size, vht, tdw);

            //    The pattern is now expanded, we can hash it to obscure meaning.
            hval = strnhash(tempbuf, newstrlen);
            snprintf(newstr, WIDTHOF(newstr), "%08lX", (unsigned long int)hval);
            newstr[WIDTHOF(newstr) - 1] = 0;

            if (internal_trace)
            {
                fprintf(stderr, "String: (len: %d) '%s'\n hashed to: %08lX\n",
                        newstrlen, tempbuf,
                        (unsigned long int)hval);
            }

            //    and stuff the new value in.
            crm_destructive_alter_nvariable(&varname[vns], vnl,                    newstr, (int)strlen(newstr), csl->calldepth);
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_TRANSLATE:
        crm_expr_translate(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_MUTATE:
#if 0
        crm_expr_mutate(csl, apb);
#else
        fatalerror("Shucks, this version, like, does not (yet) support the MUTATE command, like, you know? ", "This is _so_ unfair!");
#endif
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_SORT:
#if 0
        crm_expr_sort(csl, apb);
#else
        fatalerror("Shucks, this version, like, does not (yet) support the SORT command, like, you know? ", "This is _so_ unfair!");
#endif
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_LEARN:
        crm_expr_learn(csl, apb, vht, tdw);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    //   we had to split out classify- it was just too big.
    case CRM_CLASSIFY:
        crm_expr_classify(csl, apb, vht, tdw);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_MERGE:
        crm_expr_css_merge(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_DIFF:
        crm_expr_css_diff(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_BACKUP:
        crm_expr_css_backup(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_RESTORE:
        crm_expr_css_restore(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_INFO:
        crm_expr_css_info(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_CREATE:
        crm_expr_css_create(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_ANALYZE:
        crm_expr_css_analyze(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CSS_MIGRATE:
        crm_expr_css_migrate(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_ISOLATE:
        //    nonzero return means "bad things happened"...
        crm_expr_isolate(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_INPUT:
        crm_expr_input(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_SYSCALL:
        crm_expr_syscall(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_CALL:
        {
            //    a user-mode call - create a new CSL frame and move in!
            //
            char target[MAX_VARNAME];
            int tarlen;
            CSL_CELL *newcsl;

            if (user_trace)
                fprintf(stderr, "Executing a user CALL statement\n");

            //  look up the variable name in the vht.  If it's not there, or
            //  not in our file, call a fatal error.

            CRM_ASSERT(apb != NULL);
            tarlen = crm_get_pgm_arg(target, MAX_VARNAME, apb->s1start, apb->s1len);
            if (internal_trace)
            {
                fprintf(stderr, "\n    untranslated label (len = %d) '%s',",
                        tarlen, target);
            }

            //   do indirection if needed.
            tarlen = crm_nexpandvar(target, tarlen, MAX_VARNAME, vht, tdw);
            if (internal_trace)
            {
                fprintf(stderr, " translates to '%s'.\n", target);
            }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            CRM_ASSERT(tstmt == csl->cstmt);
#endif

            k = crm_lookupvarline(vht, target, 0, tarlen, -1);

            //      Is this a call to a known label?
            //
            if (k <= 0)
            {
                //    aw, crud.  No such label known.  But it _is_ continuable
                //    if there is a trap for it.
                fatalerror(" Can't CALL the nonexistent label: ", target);
                break;
            }
            newcsl = (CSL_CELL *)calloc(1, sizeof(newcsl[0]));
            if (!newcsl)
            {
                untrappableerror("Cannot allocate compiler memory", "Stick a fork in us; we're _done_.");
            }
            newcsl->filename = csl->filename;
            newcsl->filename_allocated = 0;
            newcsl->filetext = csl->filetext;
            newcsl->filetext_allocated = 0;
            newcsl->filedes = csl->filedes;
            newcsl->rdwr = csl->rdwr;
            newcsl->nchars = csl->nchars;
            newcsl->hash = csl->hash;
            newcsl->mct = csl->mct;
            newcsl->nstmts = csl->nstmts;
            newcsl->mct_size = csl->mct_size;
            newcsl->mct_allocated = 0;
            newcsl->preload_window = csl->preload_window;
            newcsl->caller = csl;
            newcsl->calldepth = csl->calldepth + 1;
            newcsl->running = csl->running;
            //     put in the target statement number - this is a label!
            CRM_ASSERT(k >= 0);
            CRM_ASSERT(k <= csl->nstmts);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            CRM_ASSERT(tstmt == csl->cstmt);
#endif
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            CRM_ASSERT(tstmt == csl->cstmt);
            newcsl->cstmt = k;
            newcsl->next_stmt_due_to_fail = -1;
            newcsl->next_stmt_due_to_trap = -1;
            newcsl->next_stmt_due_to_jump = -1;
            newcsl->next_stmt_due_to_debugger = -1;
#else
            newcsl->cstmt = k;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CALL is jumping to statement line: %d/%d\n", k, csl->nstmts);
            }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            CRM_ASSERT(tstmt == csl->cstmt);
#endif
            CRM_ASSERT(k >= 0);
            CRM_ASSERT(k <= csl->nstmts);
            newcsl->return_vht_cell = -1;
            //     whack the alius stack so we are not in a "fail" situation
            newcsl->aliusstk[csl->mct[k]->nest_level + 1] = 0;

            //
            //    Argument transfer - is totally freeform.  The arguments
            //    (the box-enclosed string) are var-expanded and the result
            //    handed off as the value of the first paren string of the
            //    routine (which should be a :var:, if it doesn't exist or
            //    isn't isolated, it is created and isolated).  The
            //    routine can use this string in any way desired.  It can
            //    be parsed, searched, MATCHed etc.  This allows
            //    call-by-anything to be implemented.
            //
            //    On return, a similar process is performed - the return value
            //    is formed, and the result put in an isolated variable in
            //    the caller's memory.
            //
            //    Note that there is _no_ local (hidden, etc) vars - everything
            //    is still shared.
            //
            {
                int argvallen, argnamelen;
                CSL_CELL *oldcsl;
                int vns, vnl;
                int vmidx, oldvstart, oldvlen;
                //
                //    First, get the argument string into full expansion
                CRM_ASSERT(apb != NULL);
                argvallen = crm_get_pgm_arg(tempbuf, data_window_size, apb->b1start, apb->b1len);
                argvallen = crm_nexpandvar(tempbuf, argvallen, data_window_size, vht, tdw);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                CRM_ASSERT(tstmt == csl->cstmt);
#endif

                //   Stuff the new csl with the return-value-locations'
                //   vht index - if it's -1, then we don't have a return
                //   value location.  We do this now rather than on return
                //   so that we already have the vht entry rather than a
                //   name, and so the returnname isn't dependent on the
                //   function being executed (is this a bug?  What if the
                //   returnname is :+:something: ?)
                //
                newcsl->return_vht_cell = -1;
                if (apb->p1len > 0)
                {
                    int ret_idx;
                    int retname_start, retnamelen;
                    retnamelen = crm_get_pgm_arg(outbuf, data_window_size, apb->p1start, apb->p1len);
                    retnamelen = crm_nexpandvar(outbuf, retnamelen, data_window_size, vht, tdw);
                    CRM_ASSERT(retnamelen < data_window_size);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                    CRM_ASSERT(tstmt == csl->cstmt);
#endif
                    if (crm_nextword(outbuf, retnamelen, 0, &retname_start, &retnamelen))
                    {
                        if (!crm_is_legal_variable(&outbuf[retname_start], retnamelen))
                        {
                            fatalerror_ex(
                                SRC_LOC(), "Attempt to store a CALL return value into an illegal variable '%.*s'. How very bizarre.", retnamelen,
                                &outbuf[retname_start]);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                            CRM_ASSERT(tstmt == csl->cstmt);
#endif
                            break;
                        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        ret_idx = crm_vht_lookup(vht, &outbuf[retname_start], retnamelen, csl->calldepth); /* scope = PARENT level! */
                        if (vht[ret_idx] == NULL)
                        {
                            CRM_ASSERT(retname_start + retnamelen <= data_window_size - 1);
                            outbuf[retname_start + retnamelen] = 0;
                            nonfatalerror_ex(SRC_LOC(), "Your call statement wants to return a value "
                                                        "to a nonexistent variable; I'll create an "
                                                        "isolated one.  Hope that's OK. Varname was '%.*s'",
                                             retnamelen, &outbuf[retname_start]);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                            CRM_ASSERT(tstmt == csl->cstmt);
#endif
                            if (user_trace)
                            {
                                fprintf(stderr,
                                        "No such return value var, creating var %s\n",
                                        &outbuf[retname_start]);
                            }
                            crm_set_temp_var(&outbuf[retname_start], "", csl->calldepth, !!(apb->sflags & CRM_KEEP)); /* scope = PARENT level! or... global! */
                            ret_idx = crm_vht_lookup(vht, &outbuf[retname_start], retnamelen, csl->calldepth);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                            CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        if (user_trace)
                        {
                            fprintf(stderr, " Setting return value to VHT cell %d",
                                    ret_idx);
                        }
                        newcsl->return_vht_cell = ret_idx;
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                    }
                    else
                    {
                        if (user_trace)
                        {
                            fprintf(stderr, "No return value var specified. Ignoring.\n");
                        }
                    }
                }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                CRM_ASSERT(tstmt == csl->cstmt);
#endif


                //    Now, get the place to put the incoming args - here's
                //    where "control" sort of transfers to the new
                //    statement.  From here on, everything we do is in the
                //    context of the callee... so be _careful_.
                //
                oldcsl = csl;
                csl = newcsl;
                // ensure the caller / global ref gets updated as well!
                *csl_ref = csl;
                tstmt = csl->cstmt;
                slen = csl->mct[csl->cstmt + 1]->fchar - csl->mct[csl->cstmt]->fchar;

                //
                //    At this point, the context is now "the callee".  Everything
                //    done from now must remember that.
                {
                    //   properly set :_cd: since we're now in the 'callee' code
                    char depthstr[33];
                    sprintf(depthstr, "%d", csl->calldepth);
                    crm_set_temp_var(":_cd:", depthstr, -1, 0);
                }

                //  maybe run some JIT parsing on the called statement?
                //
#if !FULL_PARSE_AT_COMPILE_TIME
                if (!csl->mct[csl->cstmt]->apb)
                {
                    csl->mct[csl->cstmt]->apb = calloc(1, sizeof(csl->mct[csl->cstmt]->apb[0]));
                    if (!csl->mct[csl->cstmt]->apb)
                    {
                        untrappableerror("Couldn't alloc the space to incrementally "
                                         "compile a statement.  ",
                                         "Stick a fork in us; we're _done_.\n");
                    }
                    else
                    {
                        //  we now have the statement's apb allocated; we point
                        //  the generic apb at the same place and run with it.
                        (void)crm_statement_parse(
                            &csl->filetext[csl->mct[csl->cstmt]->fchar],
                            slen,
                            csl->mct[csl->cstmt],
                            csl->mct[csl->cstmt]->apb);
                    }
                }
                //    and start using the JITted apb
                apb = csl->mct[csl->cstmt]->apb;
#else
                apb = &csl->mct[csl->cstmt]->apb;
#endif
                //
                //     We don't have flags, so we don't bother fixing the
                //     flag variables.
                //
                // get the paren arg of this routine
                CRM_ASSERT(apb != NULL);
                argnamelen = crm_get_pgm_arg(outbuf, data_window_size, apb->p1start, apb->p1len);
                argnamelen = crm_nexpandvar(outbuf, argnamelen, data_window_size, vht, tdw);
                //
                //      get the generalized argument name (first varname)
                if (crm_nextword(outbuf, argnamelen, 0, &vns, &vnl))
                {
                    if (vnl >= data_window_size)
                    {
                        nonfatalerror_ex(SRC_LOC(), "CALL statement comes with a label reference which is too long (len = %d) "
                                                    "while the maximum allowed size is %d.",
                                         vnl,
                                         data_window_size - 1);
                        vnl = data_window_size - 1;
                    }
                    crm_memmove(outbuf, &outbuf[vns], vnl);
                    outbuf[vnl] = 0;
                    if (vnl > 0)
                    {
                        //
                        //      and now create the isolated arg transfer variable.
                        //
                        //  GROT GROT GROT
                        //  GROT GROT GROT possible tdw memory leak here...
                        //  GROT GROT GROT the right fix is to refactor crm_expr_isolate.
                        //  GROT GROT GROT but for now, we'll reuse the same code as
                        //  GROT GROT GROT it does, releasing the old memory.
                        //
                        if (!crm_is_legal_variable(outbuf, vnl))
                        {
                            fatalerror_ex(SRC_LOC(), "Attempt to CALL with an illegal variable '%.*s' argument. How very bizarre.", vnl, outbuf);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                            CRM_ASSERT(tstmt == csl->cstmt);
#endif
                            break;
                        }
                        vmidx = crm_vht_lookup(vht, outbuf, vnl, csl->calldepth);
                        if (vht[vmidx] && vht[vmidx]->valtxt == tdw->filetext)
                        {
                            oldvstart = vht[vmidx]->vstart;
                            oldvlen = vht[vmidx]->vlen;
                            vht[vmidx]->vstart = tdw->nchars++;
                            vht[vmidx]->vlen   = 0;
                            crm_compress_tdw_section(vht[vmidx]->valtxt,
                                                     oldvstart,
                                                     oldvstart + oldvlen);
                        }
                        //
                        //      finally, we can put the variable in.  (this is
                        //      an ALTER to a zero-length variable, which is why
                        //      we moved it to the end of the TDW.
                        //
                        // NEVER allow this var to be allocated in outer scope if it doesn't reside
                        // there yet:
                        //
                        crm_set_temp_nvar(outbuf, tempbuf, argvallen, csl->calldepth, 0);
                    }
                    //
                    //   That's it... we're done.
                }
                // else: ignore.
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_INTERSECT:
        //           Calculate the intersection of N variables; the result
        //           replaces the captured value of the first variable.
        //           Captured values not in the data window are ignored.
        {
            char temp_vars[MAX_VARNAME];
            int tvlen;
            char out_var[MAX_VARNAME];
            int ovstart;
            int ovlen;
            int vstart;
            int vend;
            int vlen;
            int istart, iend, ilen, i_index;
            int mc;
            int done;

            if (user_trace)
                fprintf(stderr, "executing an INTERSECT statement");

            //    get the output variable (the one we're gonna whack)
            //
            CRM_ASSERT(apb != NULL);
            ovlen = crm_get_pgm_arg(out_var, MAX_VARNAME, apb->p1start, apb->p1len);
            ovlen = crm_nexpandvar(out_var, ovlen, MAX_VARNAME, vht, tdw);
            ovstart = 0;

            //    get the list of variable names
            //
            //     note- since vars never contain wchars, we're OK here.
            tvlen = crm_get_pgm_arg(temp_vars, MAX_VARNAME, apb->b1start, apb->b1len);
            tvlen = crm_nexpandvar(temp_vars, tvlen, MAX_VARNAME, vht, tdw);
            CRM_ASSERT(tvlen < MAX_VARNAME);
            if (internal_trace)
            {
                fprintf(stderr, "  Intersecting vars: (len: %d) ***%s***\n", tvlen, temp_vars);
                fprintf(stderr, "   with result in (len: %d) ***%s***\n", ovlen, out_var);
            }
            done = 0;
            mc = 0;
            vstart = 0;
            vend = 0;
            istart = 0;
            iend = cdw->nchars;
            ilen = 0;
            i_index = -1;
            while (!done)
            {
                if (!crm_nextword(temp_vars, tvlen, vstart, &vstart, &vlen)
                   || vlen == 0)
                {
                    done = 1;
                }
                else
                {
                    int vht_index;
                    //
                    //        look up the variable
                    if (!crm_is_legal_variable(&temp_vars[vstart], vlen))
                    {
                        fatalerror_ex(SRC_LOC(), "Attempt to INTERSECT with an illegal variable '%.*s'. How very bizarre.", vlen, &temp_vars[vstart]);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        goto invoke_bailout;
                    }
                    vht_index = crm_vht_lookup(vht, &temp_vars[vstart], vlen, csl->calldepth);
                    if (vht[vht_index] == NULL)
                    {
                        char varname[MAX_VARNAME];
                        if (vlen >= WIDTHOF(varname))
                        {
                            nonfatalerror_ex(SRC_LOC(), "INTERSECT statement comes with a variable name which is too long (len = %d) "
                                                        "while the maximum allowed size is %d.",
                                             vlen,
                                             (int)(WIDTHOF(varname) - 1));
                            vlen = WIDTHOF(varname) - 1;
                        }
                        strncpy(varname, &temp_vars[vstart], vlen);
                        varname[vlen] = 0;
                        nonfatalerror("can't intersection a nonexistent variable.",
                                      varname);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        goto invoke_bailout;
                    }
                    else
                    {
                        //  it was a good var, make sure it's in the data window
                        //
                        if (vht[vht_index]->valtxt != cdw->filetext)
                        {
                            char varname[MAX_VARNAME];
                            if (vlen >= WIDTHOF(varname))
                            {
                                nonfatalerror_ex(SRC_LOC(), "INTERSECT statement comes with a variable name which is too long (len = %d) "
                                                            "while the maximum allowed size is %d.",
                                                 vlen,
                                                 (int)(WIDTHOF(varname) - 1));
                                vlen = WIDTHOF(varname) - 1;
                            }
                            strncpy(varname, &temp_vars[vstart], vlen);
                            varname[vlen] = 0;
                            nonfatalerror("can't intersect isolated variable.",
                                          varname);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                            CRM_ASSERT(tstmt == csl->cstmt);
#endif
                            goto invoke_bailout;
                        }
                        else
                        {
                            //  it's a cdw variable; go for it.
                            if (vht[vht_index]->vstart > istart)
                                istart = vht[vht_index]->vstart;
                            if ((vht[vht_index]->vstart + vht[vht_index]->vlen)
                                < iend)
                                iend = vht[vht_index]->vstart + vht[vht_index]->vlen;
                        }
                    }
                }
                vstart = vstart + vlen;
                if (temp_vars[vstart] == 0)
                    done = 1;
            }
            //
            //      all done with the looping, set the start and length of the
            //      first var.
            vlen = iend - istart;
            if (vlen < 0)
                vlen = 0;
            if (crm_nextword(out_var, ovlen, 0, &ovstart, &ovlen))
            {
                crm_set_windowed_nvar(NULL, &out_var[ovstart], ovlen, cdw->filetext,
                                      istart, vlen,
                                      csl->cstmt, csl->calldepth, !!(apb->sflags & CRM_KEEP));
            }
            else
            {
                nonfatalerror("INTERSECT didn't come with a target variable to store the results.",
                              "'We focus on results around here,' the businessman said, and he wept silently.");
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_UNION:
        //           Calculate the union of N variables; the result
        //           replaces the captured value of the first variable.
        //           Captured values not in the data window are ignored.
        {
            char temp_vars[MAX_VARNAME];
            int tvlen;
            char out_var[MAX_VARNAME];
            int ovstart;
            int ovlen;
            int vstart;
            int vend;
            int vlen;
            int istart, iend, ilen, i_index;
            int mc;
            int done;

            if (user_trace)
                fprintf(stderr, "executing a UNION statement");

            //    get the output variable (the one we're gonna whack)
            //
            CRM_ASSERT(apb != NULL);
            ovlen = crm_get_pgm_arg(out_var, MAX_VARNAME, apb->p1start, apb->p1len);
            ovlen = crm_nexpandvar(out_var, ovlen, MAX_VARNAME, vht, tdw);
            ovstart = 0;


            //    get the list of variable names
            //
            //    since vars never contain wchars, we don't have to be 8-bit-safe
            tvlen = crm_get_pgm_arg(temp_vars, MAX_VARNAME, apb->b1start, apb->b1len);
            tvlen = crm_nexpandvar(temp_vars, tvlen, MAX_VARNAME, vht, tdw);
            if (internal_trace)
                fprintf(stderr, "  Uniting vars: (len: %d) ***%s***\n", tvlen, temp_vars);

            done = 0;
            mc = 0;
            vstart = 0;
            vend = 0;
            istart = cdw->nchars;
            iend = 0;
            ilen = 0;
            i_index = -1;
            while (!done)
            {
                if (!crm_nextword(temp_vars, tvlen, vstart, &vstart, &vlen)
                   || vlen == 0)
                {
                    done = 1;
                }
                else
                {
                    int vht_index;
                    //
                    //        look up the variable
                    if (!crm_is_legal_variable(&temp_vars[vstart], vlen))
                    {
                        fatalerror_ex(SRC_LOC(), "Attempt to UNION with an illegal variable '%.*s'. How very bizarre.", vlen, &temp_vars[vstart]);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        goto invoke_bailout;
                    }
                    vht_index = crm_vht_lookup(vht, &temp_vars[vstart], vlen, csl->calldepth);
                    if (vht[vht_index] == NULL)
                    {
                        char varname[MAX_VARNAME];
                        strncpy(varname, &temp_vars[vstart], vlen);
                        varname[vlen] = 0;
                        nonfatalerror("can't union a nonexistent variable.",
                                      varname);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                        CRM_ASSERT(tstmt == csl->cstmt);
#endif
                        goto invoke_bailout;
                    }
                    else
                    {
                        //  it was a good var, make sure it's in the data window
                        //
                        if (vht[vht_index]->valtxt != cdw->filetext)
                        {
                            char varname[MAX_VARNAME];
                            strncpy(varname, &temp_vars[vstart], vlen);
                            varname[vlen] = 0;
                            nonfatalerror("can't union an isolated variable.",
                                          varname);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
                            CRM_ASSERT(tstmt == csl->cstmt);
#endif
                            goto invoke_bailout;
                        }
                        else
                        {
                            //  it's a cdw variable; go for it.
                            if (vht[vht_index]->vstart < istart)
                                istart = vht[vht_index]->vstart;
                            if ((vht[vht_index]->vstart + vht[vht_index]->vlen)
                                > iend)
                                iend = vht[vht_index]->vstart + vht[vht_index]->vlen;
                        }
                    }
                }
                vstart = vstart + vlen;
                if (temp_vars[vstart] == 0)
                    done = 1;
            }
            //
            //      all done with the looping, set the start and length of the
            //      output var.
            vlen = iend - istart;
            if (vlen < 0)
                vlen = 0;
            if (crm_nextword(out_var, ovlen, 0, &ovstart, &ovlen))
            {
                crm_set_windowed_nvar(NULL, &out_var[ovstart], ovlen, cdw->filetext,
                                      istart, vlen,
                                      csl->cstmt, csl->calldepth, !!(apb->sflags & CRM_KEEP));
            }
            else
            {
                nonfatalerror("UNION didn't come with a target variable to store the results.",
                              "'We focus on results around here,' the businessman said, and he wept silently.");
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;


    case CRM_DEBUG:
        // turn on the debugger - NOW!
        if (user_trace)
            fprintf(stderr, "executing a DEBUG statement - drop to debug\n");
        debug_countdown = 0;
        // we need to call the debugger here, because otherwise we get 'fringe' errors because
        // we would not be able to determine _inside_ the debugger, when the current_statement-1 points
        // at a CRM_DEBUG opcode such as this one, if the debugger was actually triggered from an
        // embedded debug statement or from a previous debugger instruction, which just ended us
        // at debug-line+1, such as some 'j' command.
        //
        // Hence we're going to tell the debugger explicitly it was this 'debug' command that
        // caused it to be called; the debugger then MUST pop up and any run-N-statements or
        // step-out/until-return/whatever runs will be forcibly aborted.
        i = crm_debugger(csl, CRM_DBG_REASON_DEBUG_STATEMENT, NULL);
        if (i == -1)
        {
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 6);
            }
            else
            {
                exit(EXIT_SUCCESS);
            }
        }
        if (i == 1)
            goto invoke_top;
        break;

    case CRM_CLUMP:
        crm_expr_clump(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_PMULC:
        crm_expr_pmulc(csl, apb);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    case CRM_UNIMPLEMENTED:
        {
            int width = CRM_MIN(1024, csl->mct[csl->cstmt + 1]->start - csl->mct[csl->cstmt]->start);

            fatalerror_ex(SRC_LOC(),
                          "Statement %d NOT YET IMPLEMENTED !!!"
                          "Here's the text:\n%.*s%s",
                          csl->cstmt,
                          width,
                          &csl->filetext[csl->mct[csl->cstmt]->start],
                          (csl->mct[csl->cstmt + 1]->start - csl->mct[csl->cstmt]->start > width
                           ? "(...truncated)"
                           : "")
                         );
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;

    default:
        {
            int width = CRM_MIN(1024, csl->mct[csl->cstmt + 1]->start - csl->mct[csl->cstmt]->start);

            fatalerror_ex(SRC_LOC(),
                          "Statement %d way, way bizarre!!!  Here's the text:\n%.*s%s",
                          csl->cstmt,
                          width,
                          &csl->filetext[csl->mct[csl->cstmt]->start],
                          ((csl->mct[csl->cstmt + 1]->start - csl->mct[csl->cstmt]->start) > width
                           ? "(...truncated)"
                           : "")
                         );
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        CRM_ASSERT(tstmt == csl->cstmt);
#endif
        break;
    }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif

    //    If we're in some sort of strange abort mode, and we just need to move
    //    on to the next statement, we branch here.
invoke_bailout:

#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif
    crm_analysis_mark(&analysis_cfg, MARK_OPERATION, tstmt, "");

    //  grab end-of-statement timers ?
    if (profile_execution)
    {
#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
        LARGE_INTEGER t1;
        int64_t timer2;
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
        struct timespec t1;
        int64_t timer2;
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
        struct timeval t1;
        int64_t timer2;
#elif defined(HAVE_CLOCK)
        clock_t t1;
#else
        // nada
#endif
#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
        struct tms t1a;
        clock_t t2a;
#endif
        int64_t w = 0;

        csl->mct[tstmt]->stmt_exec_count++;

#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
        if (!QueryPerformanceCounter(&t1))
        {
            // unknown; due to error
        }
        else
        {
            timer2 = t1.QuadPart;

            w = (timer2 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = timer2; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
        if (!clock_gettime(CLOCK_REALTIME, &t1))
        {
            timer2 = ((int64_t)t1.tv_sec) * 1000000000LL + t1.tv_nsec;

            w = (timer2 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = timer2; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
        else
        {
            // unknown; due to error
        }
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
        if (!gettimeofday(&t1))
        {
            timer2 = ((int64_t)t1.tv_sec) * 1000000LL + t1.tv_usec;

            w = (timer2 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = timer2; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
        else
        {
            // unknown; due to error
        }
#elif defined(HAVE_CLOCK)
        t1 = clock();
        if (t1 == (clock) - 1)
        {
            // unknown; due to error
        }
        else
        {
            w = (t1 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = t1; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
#else
#endif

#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
        t2a = times(&t1a);
        if (t2a == (clock_t)(-1))
        {
            // unknown; due to error
        }
        else
        {
            int64_t d = ((t1a.tms_utime - timer1a.tms_utime) + (t1a.tms_stime - timer1a.tms_stime));
            double ratio;

            if (d > 0)
            {
                ratio = d;
                ratio = (t1a.tms_stime - timer1a.tms_stime) / ratio;
                d = ratio * w;
                csl->mct[tstmt]->stmt_stime += d;
                csl->mct[tstmt]->stmt_utime -= d;
            }
            timer1a = t1a;
        }
#endif
    }

    //    go on to next statement (unless we're failing, laifing, etc,
    //    in which case we have no business getting to here.
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
    if (csl->next_stmt_due_to_fail >= 0
       || csl->next_stmt_due_to_trap >= 0
       || csl->next_stmt_due_to_debugger >= 0
       || csl->next_stmt_due_to_jump >= 0)
    {
        if (csl->next_stmt_due_to_jump >= 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, "Engine: goto jump from %d to %d/%d\n", csl->cstmt, csl->next_stmt_due_to_jump, csl->nstmts);
            }
            csl->cstmt = csl->next_stmt_due_to_jump;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
        if (csl->next_stmt_due_to_fail >= 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, "Engine: fail jump from %d to %d/%d\n", csl->cstmt, csl->next_stmt_due_to_fail, csl->nstmts);
            }
            csl->cstmt = csl->next_stmt_due_to_fail;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
        if (csl->next_stmt_due_to_trap >= 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, "Engine: trap jump from %d to %d/%d\n", csl->cstmt, csl->next_stmt_due_to_trap, csl->nstmts);
            }
            csl->cstmt = csl->next_stmt_due_to_trap;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
        if (csl->next_stmt_due_to_debugger >= 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, "Engine: debugger jump from %d to %d/%d\n", csl->cstmt, csl->next_stmt_due_to_debugger, csl->nstmts);
            }
            csl->cstmt = csl->next_stmt_due_to_debugger;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
    }
    else
    {
        csl->cstmt++;
        CRM_ASSERT(csl->cstmt >= 0);
        CRM_ASSERT(csl->cstmt <= csl->nstmts);
    }
#else
    csl->cstmt++;
#endif
    if (internal_trace)
    {
        fprintf(stderr, "Going to executing statement line: %d/%d\n", csl->cstmt, csl->nstmts);
    }

    goto invoke_top;

invoke_done:
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif

    crm_analysis_mark(&analysis_cfg, MARK_OPERATION, tstmt, "");

    //  grab end-of-statement timers ?
    if (profile_execution)
    {
#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
        LARGE_INTEGER t1;
        int64_t timer2;
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
        struct timespec t1;
        int64_t timer2;
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
        struct timeval t1;
        int64_t timer2;
#elif defined(HAVE_CLOCK)
        clock_t t1;
#else
        // nada
#endif
#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
        struct tms t1a;
        clock_t t2a;
#endif
        int64_t w = 0;

        csl->mct[tstmt]->stmt_exec_count++;

#if defined(HAVE_QUERYPERFORMANCECOUNTER) && defined(HAVE_QUERYPERFORMANCEFREQUENCY)
        if (!QueryPerformanceCounter(&t1))
        {
            // unknown; due to error
        }
        else
        {
            timer2 = t1.QuadPart;

            w = (timer2 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = timer2; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
#elif defined(HAVE_CLOCK_GETTIME) && defined(HAVE_STRUCT_TIMESPEC)
        if (!clock_gettime(CLOCK_REALTIME, &t1))
        {
            timer2 = ((int64_t)t1.tv_sec) * 1000000000LL + t1.tv_nsec;

            w = (timer2 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = timer2; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
        else
        {
            // unknown; due to error
        }
#elif defined(HAVE_GETTIMEOFDAY) && defined(HAVE_STRUCT_TIMEVAL)
        if (!gettimeofday(&t1))
        {
            timer2 = ((int64_t)t1.tv_sec) * 1000000LL + t1.tv_usec;

            w = (timer2 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = timer2; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
        else
        {
            // unknown; due to error
        }
#elif defined(HAVE_CLOCK)
        t1 = clock();
        if (t1 == (clock) - 1)
        {
            // unknown; due to error
        }
        else
        {
            w = (t1 - timer1);
            csl->mct[tstmt]->stmt_utime += w;
            // csl->mct[tstmt]->stmt_stime += 0;
            timer1 = t1; // [i_a] set as new start time for next opcode: prevent time loss by measuring end-to-end.
        }
#else
#endif

#if defined(HAVE_TIMES) && defined(HAVE_STRUCT_TMS)
        t2a = times(&t1a);
        if (t2a == (clock_t)(-1))
        {
            // unknown; due to error
        }
        else
        {
            int64_t d = ((t1a.tms_utime - timer1a.tms_utime) + (t1a.tms_stime - timer1a.tms_stime));
            double ratio;

            if (d > 0)
            {
                ratio = d;
                ratio = (t1a.tms_stime - timer1a.tms_stime) / ratio;
                d = ratio * w;
                csl->mct[tstmt]->stmt_stime += d;
                csl->mct[tstmt]->stmt_utime -= d;
            }
            timer1a = t1a;
        }
#endif
    }

#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
    CRM_ASSERT(tstmt == csl->cstmt);
#endif
    //     give the debugger one last chance to do things.
    if (debug_countdown > DEBUGGER_DISABLED_FOREVER) // also pop up the debugger when in 'continue' or 'counted' run
    {
        int end_stmt_nr = csl->cstmt;

        i = crm_debugger(csl, CRM_DBG_REASON_DEBUG_END_OF_PROGRAM, NULL);
        if (i == -1)
        {
            if (engine_exit_base != 0)
            {
                exit(engine_exit_base + 6);
            }
            else
            {
                exit(EXIT_SUCCESS);
            }
        }
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        if (csl->next_stmt_due_to_debugger >= 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, "Engine: debugger jump from %d to %d/%d\n", csl->cstmt, csl->next_stmt_due_to_debugger, csl->nstmts);
            }
            csl->cstmt = csl->next_stmt_due_to_debugger;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        }
#endif
        // prevent looping when 'fail'ing to end from inside the debugger, as we're already there anyhow.
        if (i == 1)
        {
            if (csl->cstmt < csl->nstmts && end_stmt_nr != csl->cstmt)
            {
                // only loop when debugger actually _changed_ the execution position!
                goto invoke_top;
            }
        }
    }

    //     if we asked for an output profile, give it to us.
    if (profile_execution)
    {
        crm_output_profile(csl);
    }

    return status;
}

