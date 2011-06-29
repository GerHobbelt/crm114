//	crm_svm_lib_fncts.c - Support Vector Machine

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

#ifndef __CRM__SVM_LIB_FNCTS__H
#define __CRM_SVM_LIB_FNCTS__H

#include "crm_svm_matrix.h"
#include "crm_svm_quad_prog.h"
#include "crm_svm_matrix_util.h"
#include "crm114_config.h" //some defines we will want

#define SVM_EXP_MAX_IT 50          //The expected maximum number of iterations
                                   //Used to set the default size for the QP 
                                   //problem sparse array matrices and vectors


#define SVM_MAX_X_VAL 0.0625*32562 //the "infinity" value for the alphas.  
                                   //I pulled this from Joachims' paper.  
                                   //The number of iterations of the solver 
                                   //scales with this number so it's a good 
                                   //one to get right.  playing with it will
                                   //affect both running time and accuracy.

#define SVM_MIN_X_VAL 100          //this is a lower bound on the bound on
                                   //sum_c alpha_c.  during the iterative
                                   //training, the bound can wind up nominally
                                   //depressed at zero so we lower bound it by
                                   //this value.  playing with it will affect
                                   //both running time and accuracy.

#define SVM_SOLVER_DEBUG 1      //basic information about the solver.  possible
                                //to use even on runs that need to be fast.
                                //gives a general progress overview

#define SVM_SOLVER_DEBUG_LOOP 8 //prints the matrices associated with the solver
                                //unless you're doing a small run, this isn't
                                //a feasible setting since the print operations
                                //put all the zeros in!

int SVM_DEBUG_MODE;         //There are a number of modes.
                                   //See crm_svm_matrix_util.h for them.
//the SVM solution struct
typedef struct {
  Vector *theta;    //decision boundary
  Matrix *SV;       //support vectors
  int num_examples, //number of examples that went into this training
    max_train_val;  //sum_c alpha_c <= max_train_val when restarting
} SVM_Solution;

//Actually involved in solving the SVM
SVM_Solution *svm_solve_no_init_sol(Matrix *Xy);
ExpandingArray *svm_preprocess(Matrix *X, Vector *old_theta);
void svm_solve(Matrix **Xy_ptr, SVM_Solution **sol_ptr);

//SVM_Solution methods
int svm_classify_example(Vector *ex, SVM_Solution *sol);
size_t svm_write_solution(SVM_Solution *sol, char *filename);
size_t svm_write_solution_fp(SVM_Solution *sol, FILE *fp);
SVM_Solution *svm_read_solution(char *filename);
SVM_Solution *svm_read_solution_fp(FILE *fp);
SVM_Solution *svm_map_solution(void **addr, void *last_addr);
void svm_free_solution(SVM_Solution *sol);

#endif //crm_svm_lib_fncts.h
