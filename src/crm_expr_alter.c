//	crm_expr_alter.c - expression alter or eval tools

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




int crm_expr_eval(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //      Here we evaluate the slash-string _repeatedly_, not just
    //      once as in ALTER.
    //
    //      To prevent infinite loops (or at least many of them) we:
    //      1) strictly limit the total number of loop iterations to
    //         the compile-time parameter MAX_EVAL_ITERATIONS
    //      2) we also keep an array of the hashes of the last MAX_EVAL_ITERATIONS values,
    //         if we see a repeat, we assume that it's a loop and we stop
    //         right there.

    char varname[MAX_VARNAME];
    int varnamelen = 0;
    int newvallen;
    crmhash64_t ahash[MAX_EVAL_ITERATIONS];
    int ahindex;
    int itercount;
    int qex_stat;
    // int has_output_var;
    int varnamestart;

    // should use tempbuf for this instead.
    //   char newstr [MAX_PATTERN];
    if (user_trace)
        fprintf(stderr, "Executing an EVALuation\n");

    qex_stat = 0;
    // has_output_var = 1;

    //     get the variable name
    CRM_ASSERT(apb != NULL);
    varnamelen = crm_get_pgm_arg(varname, MAX_VARNAME, apb->p1start, apb->p1len);
    //      do variable substitution on the variable name
    varnamelen = crm_nexpandvar(varname, varnamelen, MAX_VARNAME, vht, tdw);
    // [i_a] standardized code: get_pgm+nexpand+nextword to get a variable identifier from a parameter
    if (!crm_nextword(varname, varnamelen, 0, &varnamestart, &varnamelen)
       || varnamelen < 2)
    {
        // we do accept the special 'empty var' :: here as a valid var: it does exist after all :-)

        if (user_trace)
        {
            fprintf(stderr, "There's no output var for this EVAL, so we won't "
                            "be assigning the result anywhere.\n  It better have a "
                            "relational test, or you're just wasting CPU.\n");
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
        {
            fprintf(stderr, "Eval ihash = %016llX at round %d\n",
                    (unsigned long long int)ihash, itercount + 1);
        }
        if (user_trace)
        {
            fprintf(stderr, "EVAL round %d processes expression '%.*s'\n",
                    itercount + 1, newvallen, tempbuf);
        }

        // scan down to see if we see any change: faster than up.
        //
        // note that the old 'value may have returned to same' code is
        // still in here; when we can do without it, there's no need
        // for a hash array, only a 'previous hash' value to tag along.
        for (i = itercount; --i >= 0;)
        {
            CRM_ASSERT(i < MAX_EVAL_ITERATIONS);
            CRM_ASSERT(i < itercount);
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
                                   data_window_size, &qex_stat, vht, tdw);
        CRM_ASSERT(newvallen < data_window_size);
        tempbuf[newvallen] = 0;
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
    if (varnamelen >= 2)
    {
        // we do accept the special 'empty var' :: here as a valid var: it does exist after all :-)
        crm_destructive_alter_nvariable(&varname[varnamestart], varnamelen,                tempbuf, newvallen, csl->calldepth);
    }

    if (internal_trace)
        fprintf(stderr, "Final qex_stat was %d\n", qex_stat);

    //    for now, use the qex_stat that came back from qexpandvar.
    if (qex_stat != 0)
    {
        if (user_trace)
            fprintf(stderr, "Mathematical expression at line was not satisfied, doing a FAIL at line %d\n", csl->cstmt);
        CRM_ASSERT(csl->cstmt >= 0);
        CRM_ASSERT(csl->cstmt <= csl->nstmts);
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
        if (internal_trace)
        {
            fprintf(stderr, "EVAL is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
        }
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
    int varnamestart;
    int varnamelen;
    int newvallen;

    // should use tempbuf for this instead.
    //   char newstr [MAX_PATTERN];
    if (user_trace)
        fprintf(stderr, "Executing an ALTERation\n");

    //     get the variable name
    CRM_ASSERT(apb != NULL);
    varnamelen = crm_get_pgm_arg(varname, MAX_VARNAME, apb->p1start, apb->p1len);
    if (varnamelen < 2)
    {
        // we do accept the special 'empty var' :: here as a valid var: it does exist after all :-)

        nonfatalerror(
            "This statement is missing the variable to alter,\n",
            "so I'll ignore the whole statement.");
        return 0;
    }

    //      do variable substitution on the variable name
    varnamelen = crm_nexpandvar(varname, varnamelen, MAX_VARNAME, vht, tdw);
    if (!crm_nextword(varname, varnamelen, 0, &varnamestart, &varnamelen)
       || varnamelen < 2)
    {
        // we do accept the special 'empty var' :: here as a valid var: it does exist after all :-)

        nonfatalerror(
            "The variable you're asking me to alter has an utterly bogus\n"
            "name or has not been specified at all\n",
            "so I'll ignore the whole statement.");
        return 0;
    }

    //     get the new pattern, and expand it.
    newvallen = crm_get_pgm_arg(tempbuf, data_window_size, apb->s1start, apb->s1len);
    newvallen = crm_nexpandvar(tempbuf, newvallen, data_window_size, vht, tdw);

    crm_destructive_alter_nvariable(&varname[varnamestart], varnamelen,            tempbuf, newvallen, csl->calldepth);
    return 0;
}

