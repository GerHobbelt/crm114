//	crm_pca_lib_fncts.h - Principal Component Analysis

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


#ifndef __PCA__H
#define __PCA__H

#include "crm_svm_matrix_util.h"
#include "crm_svm_matrix.h"
#include "crm114_config.h"
#include <string.h>

#define PCA_DEBUG 1         //Debug mode defines - prints out minimal information
#define PCA_LOOP 8          //Prints out matrices and vectors - only use for small problems
                            //The intermediate DEBUG modes may enable debug printing for the
                            //matrix operations.  See crm_svm_matrix_util.h for details.

int PCA_DEBUG_MODE;         //The debug mode for the PCA
extern int MATR_DEBUG_MODE; //Debug mode for matrices.  MATR_DEBUG_MODE = PCA_DEBUG_MODE
                            //Defined in crm_svm_matrix_util.h

typedef struct {
  Vector *theta;     //first principal component
  double mudottheta; //decision point (unlike SVM this isn't necessarily 0)
} PCA_Solution;

PCA_Solution *run_pca(Matrix *M, Vector *init_pca);
ExpandingArray *pca_preprocess(Matrix *X, Vector *init_pca);
void pca_solve(Matrix *X, PCA_Solution **init_pca);
void pca_free_solution(PCA_Solution *sol);

#endif //crm_pca_lib_fncts.h
