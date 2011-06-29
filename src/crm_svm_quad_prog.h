//	crm_svm_quad_prog.h - Support Vector Machine

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

#ifndef __CRM_SVM_QUAD_PROG__H
#define __CRM_SVM_QUAD_PROG__H

#include "crm_svm_matrix_util.h"
#include "crm_svm_matrix.h"

extern int SVM_DEBUG_MODE;     //debugging mode. see crm_svm_matrix_util.h for
                               //possible values.

//the accuracy to which we run conjugate_gradient
//this should be a pretty small number!!!
#define QP_LINEAR_ACCURACY 1e-10
//we should never exceed this but here just in case
#define QP_MAX_ITERATIONS 1000

void run_qp(Matrix *G, Matrix *A, Vector *c, Vector *b, Vector *x);
void add_constraint(int toadd, Matrix *A, Matrix *Q, Matrix *R, 
		    Matrix **Z_ptr);
void delete_constraint(int toad, Matrix *A, Matrix *Q, Matrix *R, 
		       Matrix **Z_ptr);
int compute_lambda(Matrix *R, Matrix *Q, Vector *g);
void back_sub(Matrix *U, Vector *b, Vector *ret);
double find_direction(Matrix *Z, Matrix *G, Vector *g, Vector *p);
void conjugate_gradient(Matrix **A, int nmatrices, int maxrows,
			Vector *b, Vector *x);
void gradient_descent(Matrix **A, int nmatrices, int maxrows, Vector *v,
		      Vector *x);
void run_linear(Matrix *A, Vector *b, Vector *x);

//int main(int argc, char **argv);

#endif //crm_svm_quad_prog.h
