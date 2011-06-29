//  crm_expr_classify.c  - Controllable Regex Mutilator,  version v1.0
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

//  OSBF declarations
#include "crm114_osbf.h"



//     Dispatch a LEARN statement
//
int crm_expr_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    long i;
    char *txt;
    long start;
    long len;
    int retval;
    long saved_ssfl;
    uint64_t classifier_flags = 0;

    //            get start/length of the text we're going to learn:
    //
    CRM_ASSERT(apb != NULL);
    crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, apb->b1len,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr);

    if (i < 0)
    {
        long curstmt;
        long fev;
        CRM_ASSERT(i == -1 || i == -2);
        fev = 0;
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
        return fev;
    }

    //  keep the original value of the ssfl, because many learners
    //  mangle it and then it won't work right for other classifiers
    saved_ssfl = sparse_spectrum_file_length;

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.

    classifier_flags = apb->sflags;

    //     Joe thinks that this should be a table or a loop.
    classifier_flags = classifier_flags &
                       (CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
                        | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
						 | CRM_SCM);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_learn(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_learn(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_learn(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_learn(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_learn(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_learn(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_learn(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_learn(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_learn(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SCM)
    {
        retval = crm_expr_scm_learn(csl, apb, txt, start, len);
    }
    else
    {
        retval = crm_expr_markov_learn(csl, apb, txt, start, len);
    }

    sparse_spectrum_file_length = saved_ssfl;

    return retval;
}

//      Dispatch a CLASSIFY statement
//
int crm_expr_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    long i;
    char *txt;
    long start;
    long len;
    long retval;
    long long classifier_flags = 0;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, apb->b1len,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr);

    if (i > 0)
    {
        long curstmt;
        long fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    classifier_flags = classifier_flags &
                       (CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
                        | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
						 | CRM_SCM);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_classify(csl, apb, txt, start, len);
    }
    else    if (classifier_flags & CRM_SCM)
    {
        retval = crm_expr_scm_classify(csl, apb, txt, start, len);
    }
    else
	{
        retval = crm_expr_markov_classify(csl, apb, txt, start, len);
    }
    return 0;
}


