//	crm_expr_classify.c - learn and classify functions for different schema

// Copyright 2001-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  include the routine declarations file
#include "crm114.h"

//  OSBF declarations
#include "crm114_osbf.h"

//     Dispatch a LEARN statement
//
int crm_expr_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  char *txt;
  long start;
  long len;
  int retval;
  long long classifier_flags = 0;

  if (crm_exec_box_restriction(csl, apb, &txt, &start, &len) != 0)
    return 0;

  //            get our flags... the only ones we're interested in here
  //            are the ones that specify _which_ algorithm to use.
  classifier_flags = apb->sflags;

  //     Joe thinks that this should be a table or a loop.
  classifier_flags = classifier_flags &
    ( CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET);

  if (classifier_flags & CRM_OSB_BAYES)
    {
      retval = crm_expr_osb_bayes_learn (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_CORRELATE)
    {
      retval = crm_expr_correlate_learn (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_OSB_WINNOW)
    {
      retval = crm_expr_osb_winnow_learn (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_OSBF )
    {
      retval = crm_expr_osbf_bayes_learn (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_HYPERSPACE)
    {
      retval = crm_expr_osb_hyperspace_learn(csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_ENTROPY)
    {
      retval = crm_expr_bit_entropy_learn(csl, apb, txt, start, len);
    }
  else
#ifndef PRODUCTION_CLASSIFIERS_ONLY
  if (classifier_flags & CRM_SVM)
    {
      retval = crm_expr_svm_learn(csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_SKS)
    {
      retval = crm_expr_sks_learn(csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_FSCM)
    {
      retval = crm_fast_substring_learn(csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_NEURAL_NET)
    {
      retval = crm_neural_net_learn (csl, apb, txt, start, len);
    }
  else
#endif	// !PRODUCTION_CLASSIFIERS_ONLY
    {
      //    Default with no classifier specified is Markov
      apb->sflags = apb->sflags | CRM_MARKOVIAN;
      retval = crm_expr_markov_learn (csl, apb, txt, start, len);
    };

  return (retval);
}

//      Dispatch a CLASSIFY statement
//
int crm_expr_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  char *txt;
  long start;
  long len;
  long retval;
  long long classifier_flags = 0;

  if (crm_exec_box_restriction(csl, apb, &txt, &start, &len) != 0)
    return 0;

  //            get our flags... the only ones we're interested in here
  //            are the ones that specify _which_ algorithm to use.
  classifier_flags = apb->sflags;

  classifier_flags = classifier_flags &
    ( CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET );

  if (classifier_flags & CRM_OSB_BAYES)
    {
      retval = crm_expr_osb_bayes_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_CORRELATE)
    {
      retval = crm_expr_correlate_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_OSB_WINNOW)
    {
      retval = crm_expr_osb_winnow_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_OSBF )
    {
      retval = crm_expr_osbf_bayes_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_HYPERSPACE)
    {
      retval = crm_expr_osb_hyperspace_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_ENTROPY)
    {
      retval = crm_expr_bit_entropy_classify (csl, apb, txt, start, len);
    }
  else
#ifndef PRODUCTION_CLASSIFIERS_ONLY
  if (classifier_flags & CRM_SVM)
    {
      retval = crm_expr_svm_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_SKS)
    {
      retval = crm_expr_sks_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_FSCM)
    {
      retval = crm_fast_substring_classify (csl, apb, txt, start, len);
    }
  else
  if (classifier_flags & CRM_NEURAL_NET)
    {
      retval = crm_neural_net_classify (csl, apb, txt, start, len);
    }
  else
#endif	// !PRODUCTION_CLASSIFIERS_ONLY
    {
      //    Default with no classifier specified is Markov
      apb->sflags = apb->sflags | CRM_MARKOVIAN;
      retval = crm_expr_markov_classify (csl, apb, txt, start, len);
    };
  return (0);
}
