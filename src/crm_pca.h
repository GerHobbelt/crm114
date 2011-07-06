//	crm_pca.h - Principal Component Analysis

////////////////////////////////////////////////////////////////////////
//    This code is originally copyright and owned by William
//    S. Yerazunis as file crm_neural_net.  In return for addition of 
//    significant derivative work, Jennifer Barry is hereby granted a full 
//    unlimited license to use this code, includng license to relicense under 
//    other licenses.
////////////////////////////////////////////////////////////////////////
//
// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.


#ifndef __CRM_PCA__H
#define __CRM_PCA__H

#include "crm_pca_lib_fncts.h"
#include "crm114_sysincludes.h"
#include "crm114_config.h"
#include "crm114_structs.h"
#include "crm114.h"
#include <string.h>

extern int PCA_DEBUG_MODE;                                 //Debug mode - 
                                                           //see crm_pca_lib_fncts.h 
                                                           //and crm_svm_matrix_util.h
                                                           //for details.
extern char *outbuf;

#define PCA_FIRST_BITS "CRM114 PCA FILE FOLLOWS:"          //PCA file magic string
#define PCA_FIRST_NBIT strlen(PCA_FIRST_BITS)*sizeof(char) //PCA magic string
                                                           //length (in bytes)
#define N_OFFSETS_IN_PCA_FILE 1                            //Number of size_t's before block
                                                           //starts in file
#define N_CONSTANTS_NOT_IN_BLOCK 0                         //Number of ints before block
                                                           //starts in file
#define N_CONSTANTS_IN_PCA_BLOCK 6                         //Number ints in block
#define HAS_NEW_INDEX 0                                    //Position of has_new in block
#define HAS_SOLUTION_INDEX 1                               //Position of has_solution in block

typedef struct {
  int has_new,
    has_solution,
    n0, n1,
    n0f, n1f;
  PCA_Solution *sol;
  Matrix *X;
} crm_pca_block;

#endif //crm_pca.h
