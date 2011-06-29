//  crm_expr_window.c  - Controllable Regex Mutilator,  version v1.0
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



//    a helper function that should be in the C runtime lib but isn't.
//
char *my_strnchr(const char *str, long len, int c)
{
    long i;

    i = 0;
    for (i = 0; i < len; i++)
    {
        if (str[i] == (char)c)
            return (char *)&(str[i]);
    }
    return NULL;
}


int crm_expr_window(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //       a window operation - two steps...first is to discard
    //       everything up till the first regex match, and second
    //       is to add more data until the second regex is
    //       satisfied by the incoming data.  We just add the
    //       incoming data onto the back of the window buffer, and
    //       when we get a read completes.
    //
    //       Yes, there are more efficient, less memory-intensive ways
    //       to do this, but this is simple and unlikely to be broken in
    //       subtle ways.

    static long newbuflen = 0;
    char pch[MAX_PATTERN];

    long i;
    long srcidx;
    int inputsrc;
    char inputsrcname[MAX_VARNAME];
    long inputsrclen;
    char *savedinputtxt;
    long savedinputtxtlen;
    char wvname[MAX_VARNAME];
    long wvnamelen;
    CSL_CELL *mdw;
    long flen;
    int regexflags;
    regex_t preg;
    //  int inputmode;
    int inputsize;       //  can be by char or by EOF  < bychar byeof >
    int inputretryEOF;   //  do we retry an EOF?       < eoffails eofretry >
    int inputEOFaccept;  //  accept an EOF as pat-end  < acceptEOF >
    int saweof;
    int failout;
    long vmidx;
    regmatch_t matches[2]; //  we're only interested in the first match.
    int done;
    int firsttime;


    inputsrcname[0] = '\000';
    inputsrclen = 0;

    //    wvname[0] = '\000';
    //wvnamelen = 0;

    srcidx = 0;
    savedinputtxt = NULL;
    savedinputtxtlen = 0;
    failout = 0;

    if (user_trace)
        fprintf(stderr, "Executing a 'window' operation\n");

    //       there's the choice of input from a
    //       variable, or input from stdin.  This is controlled strictly
    //       by whether there's a [] in the statement (someday it may
    //       allow other files than stdin, but not yet.)  So right now, it's-
    //          1) read from the variable [:foo:] if supplied, else
    //          2) read from STDIN (default)
    //                these are inputsrc=FROM_VAR vs FROM_STDIN
#define FROM_STDIN 0
#define FROM_VAR 1
#define FROM_VAR_DONE 2
    //
    //       Second, there's how much to read "preemptively", that is,
    //       to read ahead, but with the possibility of reading ahead too
    //       much (and thereby messing up a script or other typeahead that
    //       another program sharing stdin was meant to actually read.
    //       The three choices we support are:
    //          1) read everything available (BYEOF), else
    //          3) read one character at a time (BYCHAR) (default)
    //                 these are inputsize = bychar, byeof
#define BYCHAR 0
#define BYEOF 1
#define BYCHUNK 2
#define BYLINE 999   //  DANGER - BYLINE IS NOT SUPPORTED ANY MORE!!!
    //
    //       Third, there's the question of what to do if the read doesn't
    //       have enough material to satisfy the second regex (i.e. we hit
    //       end of variable or EOF first).
    //
    //       Our options are
    //
    //       1) just fail.  (the default)
    //       2) just accept what we got, even though it doesn't fulfill
    //          the paste regex (accepteof).
    //      these are expressed as inputEOFaccept= ...
#define EOFFAILS 0
#define EOFACCEPTS 1
    //

    //       As to other behavior, we can also clear the eof, wait a
    //       bit, and try again, so we have:
    //
    //          1) leave EOF's alone.
    //          2) try to reset the EOF before reading
    //              these are denoted by inputretryEOF = ...
#define EOFSTAYS 0
#define EOFRETRY 1

    //      check for the flags
    //
    //   default is BYCHAR
    CRM_ASSERT(apb != NULL);
    inputsrc = 0;
    inputEOFaccept = EOFFAILS;
    inputsize = BYCHAR;
    inputretryEOF = EOFSTAYS;
    if (apb->sflags & CRM_BYCHAR)
    {
        if (user_trace)
            fprintf(stderr, "  window input by character\n");
        inputsize = BYCHAR;
    }
    if (apb->sflags & CRM_BYCHUNK)
    {
        if (user_trace)
            fprintf(stderr, "  window input by chunks\n");
        inputsize = BYCHUNK;
    }
    if (apb->sflags & CRM_BYEOF)
    {
        if (user_trace)
            fprintf(stderr, "  window input by EOFs\n");
        inputsize = BYEOF;
    }
    inputEOFaccept = EOFFAILS;
    if (apb->sflags & CRM_EOFACCEPTS)
    {
        if (user_trace)
            fprintf(stderr, "  window input EOF is always accepted\n");
        inputEOFaccept = EOFACCEPTS;
    }
    inputretryEOF = EOFSTAYS;
    if (apb->sflags & CRM_EOFRETRY)
    {
        if (user_trace)
            fprintf(stderr, "  window input EOF is retried\n");
        inputretryEOF = EOFRETRY;
    }
    regexflags = REG_EXTENDED;
    if (apb->sflags & CRM_NOCASE)
    {
        if (user_trace)
            fprintf(stderr, "  no case matching turned on\n ");
        regexflags = regexflags | REG_ICASE;
    }
    if (apb->sflags & CRM_NOCASE)
    {
        if (user_trace)
            fprintf(stderr, "  no case matching turned on\n ");
        regexflags = regexflags | REG_ICASE;
    }
    if (apb->sflags & CRM_LITERAL)
    {
        if (user_trace)
            fprintf(stderr, "  no case matching turned on\n ");
        regexflags = regexflags | REG_LITERAL;
    }

    //    part 1: dispose of old window worth of data.  If no match,
    //    dispose of all of the old window.
    //
    //     get the disposal pattern
    //
    crm_get_pgm_arg(pch, MAX_PATTERN, apb->s1start, apb->s1len);

    //     null window check - if no cut or paste patterns, then we
    //     just skip to the end of the WINDOW statement code
    //     which is how a WINDOW statement can be used to have a
    //     program "come out running" before reading stdin.
    if (apb->s1len == 0 && apb->s2len == 0)
        goto crm_window_no_changes_made;

    //     We have the first pattern in pch.  We ought to look for the
    //     appropriate flags here (common code, anyone?) but for now,
    //     we'll just do a brutally straightforward expansion and then
    //     matching.

    if (internal_trace)
        fprintf(stderr, " window cut pattern ---%s---\n", pch);
    flen = apb->s1len;

    //       expand the match pattern
    flen = crm_nexpandvar(pch, apb->s1len, MAX_PATTERN);
    //
    //       compile the regex
    i = crm_regcomp(&preg, pch, flen, regexflags);
    if (i > 0)
    {
        crm_regerror(i, &preg, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto invoke_bailout;
    }

    //    Get the variable we're windowing.  If there's no such
    //    variable, we default to :_dw:

    crm_get_pgm_arg(wvname, MAX_PATTERN, apb->p1start, apb->p1len);
    wvnamelen = crm_nexpandvar(wvname, apb->p1len, MAX_PATTERN);
    //    if no svname, then we're defaulted to :_dw:
    if (strlen(wvname) == 0)
    {
        strcat(wvname, ":_dw:");
        wvnamelen = strlen(":_dw:");
    }

    vmidx = crm_vht_lookup(vht, wvname,
                           strlen(wvname));
    if (vht[vmidx] == NULL)
    {
        nonfatalerror("We seem to be windowing a nonexistent variable.",
                      "How very bizarre.");
        goto invoke_bailout;
    }

    mdw = NULL;
    if (vht[vmidx]->valtxt == cdw->filetext) mdw = cdw;
    if (vht[vmidx]->valtxt == tdw->filetext) mdw = tdw;
    if (mdw == NULL)
    {
        nonfatalerror("We seem to have lost the windowed var buffer",
                      "This is just plain sick.");
        goto invoke_bailout;
    }

    //
    //
    //       OK, we've got the arguments for part 1 - the cutting out
    //       of the old data.  So, let's do the cut.
    //
    //       execute the regex.
    i = crm_regexec(&preg,
                    &(vht[vmidx]->valtxt[vht[vmidx]->vstart]),
                    vht[vmidx]->vlen,
                    1, matches, 0, NULL);
    crm_regfree(&preg);

    //       starting offset of the "keep section" is at matches[0].rm.eo
    //       so we use crm_slice_and_splice_window to get rid of it.
    //
    if (i == 0)
    {
        //     delete everything up to and including the delimiter
        crm_slice_and_splice_window(mdw,
                                    vht[vmidx]->vstart,
                                    -matches[0].rm_eo);
    }
    else
    {
        //  didn't find the terminator pattern at all, which means we
        //  flush the input window completely.

        crm_slice_and_splice_window(mdw,
                                    vht[vmidx]->vstart,
                                    -vht[vmidx]->vlen);
    }

    if (user_trace)
        fprintf(stderr, "  cut completed, variable length after cut is %ld\n",
                vht[vmidx]->vlen);

    //**************************************************************
    //       OK, part one is done- we've windowed off the first
    //       part of the input.
    //
    //       Now we put the new
    //
    //        Now we get the "put" half of the regex.

    if (user_trace)
        fprintf(stderr, " now finding new section to add to end.\n");

    crm_get_pgm_arg(pch, MAX_PATTERN, apb->s2start, apb->s2len);
    flen = apb->s2len;
    if (user_trace)
        fprintf(stderr, "adding input with terminator of --%s--,", pch);

    //       expand the match pattern
    flen = crm_nexpandvar(pch, flen, MAX_PATTERN);

    if (user_trace)
        fprintf(stderr, " which expands to --%s--", pch);

    //
    //       compile the paste match regex
    i = crm_regcomp(&preg, pch, flen, regexflags);
    if (i > 0)
    {
        crm_regerror(i, &preg, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto invoke_bailout;
    }

    //    decide - do we suck input from stdin, or from
    //    a variable that's already here?
    //
    //     Get the input source, if one is supplied (2nd set of parens is
    //     the var to use as input source, if it exists)
    crm_get_pgm_arg(inputsrcname, MAX_PATTERN, apb->p2start, apb->p2len);
    inputsrclen = apb->p2len;

    if (apb->p2start)
    {
        //     NonZero input source variable, so we're gonna take our input
        //     from this input variable.
        inputsrc = FROM_VAR;
        if (user_trace)
            fprintf(stderr, "  getting input from var %s\n", inputsrcname);
    }

    //
    //    Now, depending on inputmode, we set up the final pasting
    //    to do the right thing (the final pasting params are in
    //     matches[0] ).
    //
    //     we'll set up dummy limits for now though...
    //
    matches[0].rm_so = 0;
    matches[0].rm_eo = 0;

    //   Now, the WHILE loop to find satisfaction for the second
    //   regex, within the boundaries of from_var vs from_stdin, and
    //   byline vs bychar vs byeof.  So it's really a read/test/maybe_loop
    //   loop.

    done = 0;
    saweof = 0;
    firsttime = 1;

    while (!done)
    {
        //
        //     Switch on whether we're reading from a var or from
        //     standard input.  (either way, we use the newinputbuf)
        //
        switch (inputsrc)
        {
        case FROM_VAR:
            {
                //    we're supposed to grab our input from an input variable.
                //     so we fake it as though it came from a file.
                //
                //    Later on, we have to undo the faking, and also modify
                //    the length of the input variable (cutting out the stuff
                //    that went into the WINDOW).

                //   diagnostic - what was in the newinputbuf before this stmt?
                if (user_trace)
                {
                    fprintf(stderr, " Using input source from variable %s\n",
                            inputsrcname);
                    fprintf(stderr, "   prior newinput buf --%s--\n",
                            newinputbuf);
                }

                //  Get the source input stuff
                //
                srcidx = crm_vht_lookup(vht, inputsrcname, inputsrclen);
                if (vht[srcidx] == NULL)
                {
                    nonfatalerror("Trying to take WINDOW input from"
                                  "nonexistent variables doesn't work,"
                                  "in this case, from :", inputsrcname);
                    goto invoke_bailout;
                }
                //
                //
                //    malloc up some temporary space to keep the static input
                //   buffer's stored text
                savedinputtxt = (char *)
                                calloc((32 + newbuflen), sizeof(savedinputtxt[0]));
                if (savedinputtxt == NULL)
                {
                    fatalerror("Malloc in WINDOW failed.  Aw, crud.",
                               "Can't WINDOW this way");
                    goto invoke_bailout;
                }

                //
                //    save our newinputbuf txt
                strncpy(savedinputtxt,
                        newinputbuf,
                        newbuflen);
                savedinputtxt[newbuflen] = 0;/* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
                savedinputtxtlen = newbuflen;
                //
                //     and push the contents of the variable into newinputbuf
                //     (we know it's no bigger than data_window_len)
                strncpy(newinputbuf,
                        &vht[srcidx]->valtxt[vht[srcidx]->vstart],
                        vht[srcidx]->vlen);
                newinputbuf[vht[srcidx]->vlen] = '\000';
                newbuflen = vht[srcidx]->vlen;
                //
                //    and there we have it - newintputbuf has all we will
                //    get from this variable.
                //
                //    Mark the fact that we're done with this variable by
                //    setting inputsrc to FROM_VAR_DONE;
                inputsrc = FROM_VAR_DONE;
                saweof = 1;
            }
            break;

        case FROM_VAR_DONE:
            {
                if (user_trace)
                    fprintf(stderr, "  got to FROM_VAR_DONE - this should"
                                    " NEVER happen.  You've found a bug.");
                saweof = 1;
            }
            break;

        case FROM_STDIN:
            {
                int icount;
                icount = 0;
                //
                //         the reason we _don't_ do this on te first interation
                //       is that we may already have data in the temp
                //      buffer, and we should use that data up first.
                if (!firsttime)
                {
                    //  If we're reading from stdin, then we have three options:
                    //   read a character, read up to (and including) the newline,
                    //   or read till EOF.  After each one, we set
                    if (feof(stdin))
                        saweof = 1;
                    if (inputretryEOF == EOFRETRY
                        && (feof(stdin) || ferror(stdin)))
                    {
                        if (user_trace)
                            fprintf(stderr, "  resetting the stdin stream\n");
                        clearerr(stdin);
                    }
                    if (user_trace)
                        fprintf(stderr, "  getting window input from STDIN\n");
                    switch (inputsize)
                    {
                    case BYLINE:
                        {
                            fatalerror(" Sorry, but BYLINE input is not supported;",
                                       " we recommend using '\\n' in your match "
                                       "pattern");
                        }
                        break;

                    case BYEOF:
                        {
                            //    if BYEOF, we read as big a hunk as will fit.
                            //    If that's less than the full buffer, we declare
                            //    that we got an EOF as well.
                            if (user_trace)
                                fprintf(stderr, "  bigchunk BYEOF read starting \n");
                            //
                            //        fread doesn't stop on pipe empty, while
                            icount = fread(&(newinputbuf[newbuflen]), 1,
                                           data_window_size - (newbuflen + 256),
                                           stdin);
                            if (feof(stdin)) saweof = 1;
                        }
                        break;

                    case BYCHUNK:
                        {
                            //    if BYCHUNK, we read all we can, and then we're
                            //    off and running.
                            //    Since we read everything available, we always
                            //    declare we saw EOF.  Use EOFRETRY to run again.
                            if (user_trace)
                                fprintf(stderr, "  bigchunk BYEOF read starting \n");
                            //
                            //        fread (stdin) doesn't return on pipe
                            //        empty, while read on STDIN_FILENO does.
                            //        So, for reading by chunks, we use read (STDIN
                            icount = read(fileno(stdin),
                                          &(newinputbuf[newbuflen]),
                                          data_window_size / 4);
                            saweof = 1;
                        }
                        break;

                    case BYCHAR:
                    default:
                        {
                            //   if BYCHAR, read one character and we're done
                            //          icount = read (0, &(newinputbuf[newbuflen]), 1);
                            //
                            if (user_trace)
                                fprintf(stderr, "   single character BYCHAR read\n");
                            icount = fread(&(newinputbuf[newbuflen]), 1, 1, stdin);
                        }
                        break;
                    }
                }
                //
                //     end of major part of BYCHAR / BYEOF specialized code.
                //
                if (icount > 0)
                {
                    newbuflen = newbuflen + icount;
                    newinputbuf[newbuflen] = '\000'; // put on the terminator
                }
                //              icount < 0 means an error occurred
                if (icount < 0)
                {
                    nonfatalerror(" Something went wrong in WINDOW "
                                  "while trying to read",
                                  "I will keep trying. ");
                }
                if (feof(stdin))
                    saweof = 1;
            }
        }      // END OF SWITCH ON INPUTSRC

        //     mark that this is not the first time through the loop
        //
        firsttime = 0;

        //      now have an newinputbuf with something worth examining
        //     in it, of length newbuflen (i.e. using chars [0...newbuflen-1])
        //
        //     So, we run the paste regex on it, and depending on the outcome,
        //     set "done" or not.

        i = crm_regexec(&preg,
                        newinputbuf,
                        newbuflen,
                        1, matches, 0, NULL);

        //
        //        Now we deal with the result of the regex matching (or not
        //        matching.  i== 0 for success, i > 0 for failure.
        //
        if (i == 0)
        {
            //   we found the regex; do the cut/paste
            //
            done = 1;
            if (user_trace)
                fprintf(stderr, "  Found the paste pattern\n");
            //   (and the cut/paste is already set up correctly in
            //   matches[0]; we don't have to do anything.
        }
        else
        {
            //    Nope, the regex was not found.  But if we had inputEOFaccept=
            //    EOFACCEPTS, then we accept it anyway.
            if (saweof)
            {
                done = 1;
                failout = 1;
                if (user_trace)
                    fprintf(stderr, " saw EOF, EOFAccept= %d\n", inputEOFaccept);
                switch (inputEOFaccept)
                {
                case EOFACCEPTS:
                    {
                        //         In EOFENDS and EOFAIL, we take the available
                        //     input, shove it in, and go onward.  We do this
                        //     by "faking" the matches[0] variable.
                        matches[0].rm_so = 0;
                        matches[0].rm_eo = newbuflen;
                        if (matches[0].rm_eo < 0) matches[0].rm_eo = 0;
                        failout = 0;
                    }
                    break;

                case EOFFAILS:
                default:
                    {
                        //      Nope - got an EOF, and we aren't supposed to
                        //      accept it.  So we MIGHT be done.  Or maybe not...
                        //      if we have EOFRETRY set then we clear it and
                        //      try again.
                        if (inputretryEOF == EOFRETRY)
                        {
                            clearerr(stdin);
                            done = 0;
                            failout = 0;
                        }
                        //     But, if we are reading from a var, there will never
                        //     be any more, so we are -always- done.
                        if (inputsrc == FROM_VAR
                            || inputsrc == FROM_VAR_DONE)
                        {
                            done = 1;
                        }
                    }
                    break;
                }
            }
        }
    }  // end of the (!done) loop...
    //
    //    It's just use the computed values from here on.

    crm_regfree(&preg);

    if (internal_trace)
        fprintf(stderr, "   now newinput buf --%s--\n", newinputbuf);

    //     Once we get to here, we have the new input in newinputbuf, and
    //     matches[0].rm_eo is the length.  So, we copy the new data onto
    //     the end of the cdw window, and slide the new input up.
    //
    //     start by making some space at the end of the input buffer

    crm_slice_and_splice_window(mdw,
                                vht[vmidx]->vstart + vht[vmidx]->vlen,
                                matches[0].rm_eo);

    //     copy the pertinent part of newinputbuf into the space
    //     we just made.
    memmove(&(vht[vmidx]->valtxt[vht[vmidx]->vstart
                                 + vht[vmidx]->vlen
                                 - matches[0].rm_eo]),
            newinputbuf,
            matches[0].rm_eo);

    //     and get rid of the same characters out of newinputbuf
    if (newbuflen > 0)
        memmove(newinputbuf,
                &(newinputbuf[matches[0].rm_eo]),
                newbuflen - matches[0].rm_eo + 1);
    newbuflen = newbuflen - matches[0].rm_eo;
    newinputbuf[newbuflen] = '\000';


    //       Now, if we had EOFFAILS, and we hit the fail condition,
    //       we have to set up the CSL so that it will continue execution
    //       in the "right" place.

    if (failout == 1)
    {
        if (user_trace)
            fprintf(stderr, "  CUT match failed so we're going to fail.\n");
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
    }

    //       and, if we got a nonfatal error, we skip all the stuff above;
    //       this is cleanup that we have to do eiher way.  Failure here
    //       is Really Bad.
invoke_bailout:
    //

    //
    //    Last little bit of cleanup is that IF we fetched from a
    //    variable (not a file) we have to undo our fakery of stuffing
    //    the var's contents into newinputbuf.
    //
    //    This cleanup is two parts - stuffing the remains of inputsrcname
    //    back into inputsrcname, and then restoring the old stdin buffer
    //    contents from savedinputtxt and freeing the temporary
    //    space,
    if (inputsrc == FROM_VAR || inputsrc == FROM_VAR_DONE)
    {
        //     stuff the remaining characters back into the src var

        if (user_trace)
            fprintf(stderr, " restoring remains of input src variable.\n");

        crm_destructive_alter_nvariable(inputsrcname, inputsrclen,
                                        newinputbuf,
                                        newbuflen);

        //      and restore the old stdin buffer
        strncpy(newinputbuf,
                savedinputtxt,
                savedinputtxtlen);           /* [i_a] we'll add the NUL below, son */
        newinputbuf[savedinputtxtlen] = 0;   /* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
        newbuflen = savedinputtxtlen;
    }

    //      and free the temporary space
    if (savedinputtxt) free(savedinputtxt);

crm_window_no_changes_made:

    return 0;
}

