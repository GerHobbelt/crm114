//  crm_expr_translate.c  - Controllable Regex Mutilator,  version v1.0
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




//        And the translate routine.  We use strntrn to do the hard work;
//        this code here is just glue code.
//
int crm_expr_translate(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    uint64_t strntrn_flags;
    char destination[MAX_VARNAME];
    int destination_len, dst_nstart;
    //   for source, we use tempbuf
    int vmidx;
    char *mdwptr;
    int offset;
    int len, retlen;
    char errstr[MAX_PATTERN];
    int i;
	int fev = 0;

    //   the "from" charset
    char fromset[MAX_PATTERN];
    int fromset_len;
    //   the "to" charset
    char toset[MAX_PATTERN];
    int toset_len;

    //
    strntrn_flags = 0;
    //       Go through the flags
    //
    //       UNIQUE flag set?
    //
    CRM_ASSERT(apb != NULL);
    if (apb->sflags & CRM_UNIQUE)
    {
        if (user_trace)
            fprintf(stderr, "  uniquing flag turned on...\n");
        strntrn_flags |= CRM_UNIQUE;
    }
    //
    //                                How about the LITERAL flag
    if (apb->sflags & CRM_LITERAL)
    {
        if (user_trace)
            fprintf(stderr, "  literal (no invert or ranges) turned on...\n");
        strntrn_flags |= CRM_LITERAL;
    }

    //      Get the destination for the translation
    //
    crm_get_pgm_arg(destination, MAX_VARNAME, apb->p1start, apb->p1len);
    destination_len = crm_nexpandvar(destination, apb->p1len, MAX_VARNAME);
    //if (destination_len == 0)
    //  {
    //    strcpy (destination, ":_dw:");
    //    destination_len = 5;
    //  }

    if (internal_trace)
        fprintf(stderr, " destination: ***%s*** len=%d\n",
                destination, destination_len);
    crm_nextword(destination, destination_len, 0, &dst_nstart,
            &destination_len);
    if (destination_len < 3)
    {
        strcpy(destination, ":_dw:");
        destination_len = 5;
		dst_nstart = 0;
    }


    //     here's where we look for a [] var-restriction source
    //
    //     Experimentally, we're adding [ :foo: 123 456 ] to
    //     allow an externally specified start and length.
    crm_get_pgm_arg(tempbuf, data_window_size, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(tempbuf, apb->b1len,
            &vmidx,
            &mdwptr,
            &offset,
            &len,
            errstr,
			WIDTHOF(errstr));

    if (internal_trace)
        fprintf(stderr,
                "restriction out: vmidx: %d  mdw: %p   start: %d  len: %d\n",
                vmidx, mdwptr, offset, len);
    if (i < 0)
    {
        int curstmt;
        curstmt = csl->cstmt;
        if (i == -1)
            fev = nonfatalerror(errstr, "");
        if (i == -2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        goto nonfatal_route_outwards;
    }

    //    No problems then.  We can just memmove the result into tempbuf
    memmove(tempbuf, &mdwptr[offset], len);

    //    get the FROM charset out of the first // slashes
    crm_get_pgm_arg(fromset, MAX_PATTERN, apb->s1start, apb->s1len);
    if (internal_trace)
        fprintf(stderr, " FROM-charset: =%s=\n", fromset);

    //     if not LITERAL, then expand them as well
    fromset_len = apb->s1len;
    if (!(strntrn_flags & CRM_LITERAL))
        fromset_len = crm_nexpandvar(fromset, apb->s1len, MAX_PATTERN);

    if (user_trace)
        fprintf(stderr, " from-charset expands to =%s= len %d\n",
                fromset, fromset_len);


    //    get the TO charset out of the second // slashes
    crm_get_pgm_arg(toset, MAX_PATTERN, apb->s2start, apb->s2len);
    if (internal_trace)
        fprintf(stderr, " TO-charset: =%s=\n", toset);

    //     if not LITERAL, then expand them as well
    toset_len = apb->s2len;
    if (!(strntrn_flags & CRM_LITERAL))
        toset_len = crm_nexpandvar(toset, apb->s2len, MAX_PATTERN);

    if (user_trace)
        fprintf(stderr, " to-charset expands to =%s= len %d\n",
                toset, toset_len);

    //    We have it all now - the [expanded] input in tempbuf, the
    //     from-charset, the to-charset, and the flags.  We can now
    //      make the big call to strntrn and get the new (in-place) string.

    retlen = strntrn((unsigned char *)tempbuf, &len, data_window_size,
            (unsigned char *)fromset, fromset_len,
            (unsigned char *)toset, toset_len,
            strntrn_flags);

    if (retlen < 0)
    {
        fev = nonfatalerror("Messy problem in TRANSLATE.",
                "Try again with -t tracing maybe?");
        goto nonfatal_route_outwards;
    }

    //
    //    OK, we have final result and a valid length.  Now push that
    //    back into the destination.
    //tempbuf[retlen] = 0;
    //if (user_trace)
    //  fprintf(stderr, "Result of TRANSLATE: %s len %d\n",
    //         tempbuf, retlen);

    if (user_trace)
    {
        int i2;
        fprintf(stderr, "Result of TRANSLATE: -");
#if 0
        for (i2 = 0; i2 < retlen; i2++)
            fputc(tempbuf[i2], stderr);
#else
		memnCdump(stderr, tempbuf, retlen);
#endif
        fprintf(stderr, "- len %d\n", retlen);
    }
    crm_destructive_alter_nvariable(destination + dst_nstart, destination_len, /* [i_a] */
            tempbuf, retlen);

    //  All done - return to caller.
    //
    if (0)
    {
nonfatal_route_outwards:
        if (user_trace)
            fprintf(stderr, "The TRANSLATE FAULTed and we're taking the TRAP out");
    }
    return fev;
}

