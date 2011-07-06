//	crm_pca_lib_fcnts.c - Principal Component Analysis

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

#include "crm_svm_matrix.h"
#include "crm_pca_lib_fncts.h"

/*******************************************************************************************
 *This is a variation of the VonMises power algorithm for sparse matrices.  Let us have
 *x_1, x_2,.., x_n examples with column means xbar.  Then the VonMises update is:
 *  p <- sum_{x_i} ((x_i - xbar)*p)*xbar
 *However, if the x_i were sparse, doing this exactly makes the calculations nonsparse.
 *Therefore we break it into the following algorithm:
 * 1) Precompute xbardotp = xbar*p
 * 2) In a loop over the x_i, using sparse dot products, compute
 *    a) p' = sum_{x_i}(x_i*p - xbar*p)*x_i
 *    b) S = sum_{x_i}(x_i*p - xbar*p)
 * 3) Update p <- p' - S*xbar
 *
 *INPUT: M: The example matrix.
 * init_pca: An initial guess at the PCA vector or NULL if you don't have one
 *
 *OUTPUT: A pca solution consisting of the first principal component p and xbar*p.
 *
 *TYPES: For fastest running, first principal component should be left as NON_SPARSE.
 ******************************************************************************************/

PCA_Solution *run_pca(Matrix *M, Vector *init_pca) {
  Vector *xbar = vector_make(M->cols, NON_SPARSE, MATR_PRECISE),
    *p = vector_make(M->cols, NON_SPARSE, MATR_PRECISE),
    *pold = vector_make(M->cols, NON_SPARSE, MATR_PRECISE),
    *row;
  VectorIterator vit;
  int i, loop_it = 0;
  double xbardotp, d, s, del, n;
  PCA_Solution *sol;
  
  MATR_DEBUG_MODE = PCA_DEBUG_MODE;


  //calculate the mean vector
  for (i = 0; i < M->rows; i++) {
    row = matr_get_row(M, i);
    vector_add(xbar, row, xbar); //will do fast addition for us
  }
  vector_multiply(xbar, 1.0/M->rows, xbar);

  if (!init_pca || vector_iszero(init_pca)) {
    //if we don't have an initial vector
    //make p a random vector
    vectorit_set_at_beg(&vit, p);
    for (i = 0; i < M->cols; i++) {
      vectorit_insert(&vit, i, rand()/(double)RAND_MAX, p);
    }
  } else {
    //otherwise, start at the initial input
    vector_copy(init_pca, p);
  }

  //normalize p
  vector_multiply(p, 1.0/norm(p), p);

  del = PCA_ACCURACY +1;

  //loop to calculate the principle component
  while (del > PCA_ACCURACY && loop_it < MAX_PCA_ITERATIONS) {
    //print out debug info if desired
    if (PCA_DEBUG_MODE >= PCA_DEBUG) {
      fprintf(stderr, "%d: delta = %.12lf\n", loop_it, del);
    }
    if (PCA_DEBUG_MODE >= PCA_LOOP) {
      fprintf(stderr, "p = ");
      vector_write_sp_fp(p, stderr);
    }

    
    xbardotp = dot(p, xbar); //mean vector dot p
    vector_copy(p, pold);    //pold = p on the previous iteration
    vector_zero(p);          //set p to be all zeros
    
    s = 0; 
    //when this loop finishes:
    //s = sum_{rows}(row*pold - xbar*pold)
    //p = p' = sum_{rows}(row*pold - xbar*pold)*row
    for (i = 0; i < M->rows; i++) {
      row = matr_get_row(M, i);          //ith row of M
      d = dot(row, pold) - xbardotp;     //row*pold - xbar*pold
      s += d;                            //add row*pold - xbar*pold to s
      vector_add_multiple(p, row, d, p); //add (row*pold - xbar*pold)*row to p
    }

    //compute p = p' - sum_{rows}(row*pold - xbar*pold)*xbar
    // => p -> p - s*xbar
    vector_add_multiple(p, xbar, -1.0*s, p);

    //normalize p
    n = norm(p);
    while (1.0/n < SVM_EPSILON) {
      //this will create a problem
      //since we treat SVM_EPSILON as zero
      vector_multiply(p, SVM_EPSILON*10, p);
      n *= SVM_EPSILON*10;
    }

    vector_multiply(p, 1.0/n, p);
    
    //this is the distance between the old and new vector
    del = vector_dist2(p, pold);
    loop_it++;
  }

  if (PCA_DEBUG_MODE >= PCA_DEBUG) {
    fprintf(stderr, "Number of iterations: %d\n", loop_it);
  }

  //create the solution
  sol = (PCA_Solution *)malloc(sizeof(PCA_Solution));
  sol->theta = p;
  sol->mudottheta = dot(p, xbar);

  //free everything
  vector_free(pold);
  vector_free(xbar);

  return sol;
}

/********************************************************************************************
 *Removes zero columns from the example matrix and initial PCA guess.
 *
 *INPUT: X: example matrix
 * init_pca: initial PC guess if you have one or NULL.  if init_pca is NON-NULL this will
 * remove only columns that are all zeros in the X matrix AND the init_pca vector so that
 * the new columns of X and init_pca correspond.
 *
 *OUTPUT: An expanding array mapping columns back so that A[i] = c indicates that what is
 * now the ith column of X used to be column c.
 *******************************************************************************************/

ExpandingArray *pca_preprocess(Matrix *X, Vector *init_pca) {
  ExpandingArray *colMap;
  
  if (init_pca) {
    matr_shallow_row_copy(X, X->rows, init_pca);
  }
  colMap = matr_remove_zero_cols(X);
  if (init_pca) {
    matr_erase_row(X, X->rows-1);
  }
  return colMap;
}

/*******************************************************************************************
 *Solves a PCA and returns its solution in *sol.
 *
 *INPUT: X: example matrix
 * sol: A pointer to a previous solution OR a pointer to NULL if there is no previous
 *  solution.  THIS POINTER SHOULD BE NON-NULL.  If you have no initial solution, pass in
 *  a pointer TO NULL.
 *
 *OUTPUT: A pca solution as pass-by-reference in *sol.
 ******************************************************************************************/

void pca_solve(Matrix *X, PCA_Solution **sol) {
  PCA_Solution *new_sol;
  int i;
  Vector *row, *theta;
  VectorIterator vit;
  ExpandingArray *colMap;

  MATR_DEBUG_MODE = PCA_DEBUG_MODE;
  
  if (!X) {
    if (PCA_DEBUG_MODE) {
      fprintf(stderr, "Null example matrix.\n");
    }
    if (sol && *sol) {
      pca_free_solution(*sol);
      *sol = NULL;
    }
    return;
  }

  if (!sol) {
    if (PCA_DEBUG_MODE) {
      fprintf(stderr, "Null pointer to old pca solution.  If you have no pca solution pass in a POINTER to a null vector.  Do not pass in a NULL pointer.  I am returning.\n");
    }
    return;
  }

  if (*sol) {
    theta = (*sol)->theta;
  } else {
    theta = NULL;
  }

  if (PCA_DEBUG_MODE >= PCA_DEBUG) {
    fprintf(stderr, "X is %d by %u with %d non-zero elements\n", 
	   X->rows, X->cols, X->nz);
  }

  colMap = pca_preprocess(X, theta);

  if (PCA_DEBUG_MODE >= PCA_DEBUG) {
    fprintf(stderr, "After preprocess X is %d by %u\n", X->rows, X->cols);
  }

  if (PCA_DEBUG_MODE >= PCA_LOOP) {
    fprintf(stderr, "X = \n");
    matr_print(X);
  }

  //run pca
  new_sol = run_pca(X, theta);

  if (*sol) {
    pca_free_solution(*sol);
  }
  *sol = new_sol;

  if (!(*sol)) {
    //uh oh
    if (colMap) {
      expanding_array_free(colMap);
    }
    if (PCA_DEBUG_MODE) {
      fprintf(stderr, "PCA Solver Error.\n");
    }
    return;
  }

  if (colMap) {
    //redensify theta and X
    vector_convert_nonsparse_to_sparray((*sol)->theta, colMap);
    matr_add_ncols(X, 
		   expanding_array_get(X->cols-1, colMap).compact->i+1 - 
		   X->cols);
    for (i = 0; i < X->rows; i++) {
      row = matr_get_row(X, i);
      if (!row) {
	continue;
      }
      vectorit_set_at_end(&vit, row);
      while (!vectorit_past_beg(vit, row)) {
	vectorit_set_col(vit, 
			 expanding_array_get
			 (vectorit_curr_col(vit, row), 
			  colMap).compact->i,
			 row);
	vectorit_prev(&vit, row);
      }
    }
    expanding_array_free(colMap);
  }

  if (PCA_DEBUG_MODE >= PCA_DEBUG) {
    fprintf(stderr, "Solver complete.\n");
  }

}

int pca_classify(Vector *new_ex, PCA_Solution *sol) {
  double d = dot(new_ex, sol->theta) - sol->mudottheta;

  if (d > 0) {
    return 1;
  }

  if (d < 0) {
    return -1;
  }

  return 0;
}

/******************************************************************************************
 *Frees a PCA solution.
 *
 *INPUT: solution to free.
 *****************************************************************************************/
void pca_free_solution(PCA_Solution *sol) {
  if (!sol) {
    return;
  }

  vector_free(sol->theta);
  free(sol);
}


