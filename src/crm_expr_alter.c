//  crm_expr_alter.c  - Controllable Regex Mutilator,  version v1.0
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




int crm_expr_eval(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //      Here we evaluate the slash-string _repeatedly_, not just
    //      once as in ALTER.
    //
    //      To prevent infinite loops (or at least many of them) we:
    //      1) strictly limit the total number of loop iterations to
    //         the compile-time parameter MAX_EVAL_ITERATIONS
    //      2) we also keep an array of the hashes of the last 256 values,
    //         if we see a repeat, we assume that it's a loop and we stop
    //         right there.

    char varname[MAX_VARNAME];
    int varnamelen = 0;
    int newvallen;
    crmhash64_t ahash[MAX_EVAL_ITERATIONS];
    int ahindex;
    int itercount;
    int qex_stat;
    int has_output_var;

    // should use tempbuf for this instead.
    //   char newstr [MAX_PATTERN];
    if (user_trace)
        fprintf(stderr, "Executing an EVALuation\n");

    qex_stat = 0;
    has_output_var = 1;

    //     get the variable name
    CRM_ASSERT(apb != NULL);
    varnamelen = crm_get_pgm_arg(varname, MAX_VARNAME, apb->p1start, apb->p1len);
    if (varnamelen < 3)
    {
        has_output_var = 0;
        if (user_trace)
		{
            fprintf(stderr, "There's no output var for this EVAL, so we won't "
                            "be assigning the result anywhere.\n  It better have a "
                            "relational test, or you're just wasting CPU.\n");
		}
    }

    if (has_output_var)
    {
        //      do variable substitution on the variable name
        varnamelen = crm_nexpandvar(varname, varnamelen, MAX_VARNAME);
        if (varnamelen < 3)
        {
            nonfatalerror(
                    "The variable you're asking me to alter has an utterly bogus name\n",
                    "so I'll pretend it has no output variable, so we won't "
                            "be assigning the result anywhere.\n  It better have a "
                            "relational test, or you're just wasting CPU.\n");
            has_output_var = 0;
        }
    }
    //     get the new pattern, and expand it.
    newvallen = crm_get_pgm_arg(tempbuf, data_window_size, apb->s1start, apb->s1len);

    ahindex = 0;
 //
 //     Now, a loop - while it continues to change, keep looping.
 //     But to try and detect infinite loops, we keep track of the
 //     previous values (actually, their hashes) and if one of those
 //     values recur, we stop evaluating and throw an error.
 //
 // Note: also take into account the condition where the _calculated_
 // hash may be zero: since all possible values of the crmhash64_t
 // type can be produced (at least theoretically) by the hash function,
 // we must check against the actual, i.e. current hash value, no
 // matter what it's value is.
 //
    for (itercount = 0; itercount < MAX_EVAL_ITERATIONS; itercount++)
    {
        int i;
        crmhash64_t ihash = strnhash64(tempbuf, newvallen);

        ahash[itercount] = ihash;
        if (internal_trace)
            fprintf(stderr, "Eval ihash = %016llX\n", (unsigned long long int)ihash);

        // scan down to see if we see any change: faster than up.
        //
        // note that the old 'value may have returned to same' code is
        // still in here; when we can do without it, there's no need
        // for a hash array, only a 'previous hash' value to have tag along.
        for (i = itercount; --i >= 0;)
        {
            CRM_ASSERT(i < MAX_EVAL_ITERATIONS);
            if (ahash[i] == ihash)
            {
                if (i != itercount - 1)
                {
                    nonfatalerror("The variable you're attempting to EVAL seemes to return "
                                  "to the same value after a number of iterations, "
                                  "so it is probably an "
                                  "infinite loop.  I think I should give up.  I got this "
                                  "far: ", tempbuf);
                    return 0;
                }
                // identical hash, so no more expansions.
                break;
            }
        }
        if (i >= 0)
            break;

        newvallen = crm_qexpandvar(tempbuf, newvallen,
                data_window_size, &qex_stat);
    }

    if (itercount == MAX_EVAL_ITERATIONS)
    {
        nonfatalerror("The variable you're attempting to EVAL seems to eval "
                      "infinitely, and hence I cannot compute it.  I did try "
                      "a lot, though.  I got this far before I gave up: ",
                tempbuf);
        return 0;
    }

    //     and shove it out to wherever it needs to be shoved.
    //
    if (has_output_var)
        crm_destructive_alter_nvariable(varname, varnamelen,
                tempbuf, newvallen);

    if (internal_trace)
        fprintf(stderr, "Final qex_stat was %d\n", qex_stat);

    //    for now, use the qex_stat that came back from qexpandvar.
    if (qex_stat > 0)
    {
        if (user_trace)
            fprintf(stderr, "Mathematical expression at line was not satisfied, doing a FAIL at line %d\n", csl->cstmt);
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
    }
    return 0;
}



int crm_expr_alter(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //      here's where we surgically alter a variable.  We have to
    //      watch out in case a variable is not in the cdw (it might
    //      be in tdw; that's legal as well.
    //      syntax is to replace the contents of the variable in the
    //      varlist with the evaluated string.
    //      Syntax is "alter <flags> (var) /newvalue/

    char varname[MAX_VARNAME];
    int  varnamestart;
    int varnamelen;
    int newvallen;

    // should use tempbuf for this instead.
    //   char newstr [MAX_PATTERN];
    if (user_trace)
        fprintf(stderr, "Executing an ALTERation\n");

    //     get the variable name
    CRM_ASSERT(apb != NULL);
    varnamelen = crm_get_pgm_arg(varname, MAX_VARNAME, apb->p1start, apb->p1len);
    if (varnamelen < 3)
    {
        nonfatalerror(
                "This statement is missing the variable to alter,\n",
                "so I'll ignore the whole statement.");
        return 0;
    }

    //      do variable substitution on the variable name
    varnamelen = crm_nexpandvar(varname, varnamelen, MAX_VARNAME);
    if (!crm_nextword(varname, varnamelen, 0, &varnamestart, &varnamelen)
    || varnamelen < 3)
    {
        nonfatalerror(
                "The variable you're asking me to alter has an utterly bogus\n"
				"name or has not been specified at all\n",
                "so I'll ignore the whole statement.");
        return 0;
    }

    //     get the new pattern, and expand it.
    newvallen = crm_get_pgm_arg(tempbuf, data_window_size, apb->s1start, apb->s1len);
    newvallen = crm_nexpandvar(tempbuf, newvallen, data_window_size);

    crm_destructive_alter_nvariable(&varname[varnamestart], varnamelen,
            tempbuf, newvallen);
    return 0;
}

