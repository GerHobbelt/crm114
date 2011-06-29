//	crm_svm_matrix.h - Support Vector Machine

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

#ifndef __CRM_SVM_MATRIX__H
#define __CRM_SVM_MATRIX__H

#include "crm_svm_matrix_util.h"

/*******************************************************************
 *A matrix/vector library that deals with different data structures
 *for representing vectors transparently.
 *
 *The functions in matrix.c are commented.  See them for
 *details on this library.
 ******************************************************************/

#define MATR_DEFAULT_VECTOR_SIZE 1200 //sparse arrays start at this size
                                      //unless otherwise specified
#define MATR_COMPACT 1
#define MATR_PRECISE 0
#define QSORT_COUNTING_CUTOFF 8e7 //with this many non-zero elts or above 
                                  //we use counting sort, not q-sort

extern int SVM_DEBUG_MODE;        //debug setting.  see crm_svm_matrix_util.h
                                  //for possible modes

//Possible vector types.
typedef enum {
  NON_SPARSE,
  SPARSE_ARRAY,
  SPARSE_LIST
} VectorType;

typedef enum {
  COUNTING,
  MERGE,
  QSORT
} SortingAlgorithms;


//yes, it would make more sense to have
//a sparse node as the iterator along a
//sparse list
//BUT for reasons i have yet to figure out
//that prevents some functions from being properly
//inline'd while the way the code is currently
//written does not
typedef struct {
  PreciseSparseNode *pcurr; //iterator along precise sparse lists
  CompactSparseNode *ccurr; //iterator along compact sparse lists
  int nscurr, //iterator along arrays
    pastend, //flag if the iterator is past the end of a vector
    pastbeg; //flag if the iterator is past the beginning of a vector
} VectorIterator;

//data for non-sparse
//can be either doubles or ints
typedef union {
  int *compact;
  double *precise;
} NSData;

//vectors can be either expanding arrays, lists,
//or arrays of NSData
typedef union {
  ExpandingArray *sparray;   //SPARSSE_ARRAY vector
  SparseElementList *splist; //SPARSE_LIST vector
  NSData nsarray;            //NON_SPARSE vector
} VectorData;

//vector struct
typedef struct {
  VectorData data;  //data stored in the vector
  unsigned int dim; //# columns (dimension) in the vector
  int nz,           //# non-zero elements (nz = dim if v is NON_SPARSE) 
    compact,        //flag for compactness
    size,           //starting size for expanding arrays
    was_mapped;     //1 if the vector was mapped into memory
  VectorType type;  //flag for type
} Vector;

//matrix struct
typedef struct {
  Vector **data;     //list of pointers to rows
  unsigned int rows, //# rows in the matrix
    cols;            //# columns in the matrix
  int nz,            //# non-zero elements (nz = rows*cols if M is NON_SPARSE)
    compact,         //flag for compactness
    size,            //starting size for expanding arrays
    was_mapped;      //1 if the matrix was mapped into memory
  VectorType type;   //flag for type
} Matrix;

//Matrix functions
Matrix *matr_make(unsigned int rows, unsigned int cols, VectorType type, 
		  int compact);
Matrix *matr_make_size(unsigned int rows, unsigned int cols, VectorType type, 
		       int compact, int init_size);
void matr_set(Matrix *M, unsigned int r, unsigned int c, double d);
double matr_get(Matrix *M, unsigned int r, unsigned int c);
Vector *matr_get_row(Matrix *A, unsigned int r);
extern inline Vector *matr_get_row(Matrix *A, unsigned int r) {
  return A->data[r];
}
void matr_set_row(Matrix *A, unsigned int r, Vector *v);
void matr_shallow_row_copy(Matrix *M, unsigned int r, Vector *v);
void matr_set_col(Matrix *A, unsigned int c, Vector *v);
void matr_add_row(Matrix *M);
void matr_add_nrows(Matrix *M, unsigned int n);
void matr_add_col(Matrix *M);
void matr_add_ncols(Matrix *M, unsigned int n);
void matr_remove_row(Matrix *M, unsigned int r);
void matr_erase_row(Matrix *M, unsigned int r);
void matr_remove_col(Matrix *M, unsigned int c);
ExpandingArray *matr_remove_zero_rows(Matrix *M);
ExpandingArray *matr_remove_zero_cols(Matrix *M);
void matr_append_matr(Matrix **to_ptr, Matrix *from);
void matr_vector(Matrix *M, Vector *v, Vector *ret);
void matr_vector_seq(Matrix **A, int nmatrices, unsigned int maxrows,
		     Vector *w, Vector *z);
void matr_transpose(Matrix *A, Matrix *T);
void matr_multiply(Matrix *M1, Matrix *M2, Matrix *ret);
int matr_iszero(Matrix *M);
void matr_convert_nonsparse_to_sparray(Matrix *M, ExpandingArray *colMap);
void matr_print(Matrix *M);
void matr_write(Matrix *M, char *filename);
void matr_write_fp(Matrix *M, FILE *out);
size_t matr_write_bin(Matrix *M, char *filename);
size_t matr_write_bin_fp(Matrix *M, FILE *fp);
Matrix *matr_read_bin(char *filename);
Matrix *matr_read_bin_fp(FILE *fp);
Matrix *matr_map(void **addr, void *last_addr);
void matr_free(Matrix *M);

//Vector functions
Vector *vector_make(unsigned int dim, VectorType type, int compact);
Vector *vector_make_size(unsigned int dim, VectorType type, int compact, 
			 int init_size);
void vector_copy(Vector *from, Vector *to);
inline void vector_set(Vector *v, unsigned int i, double d);
inline double vector_get(Vector *v, unsigned int i);
unsigned int vector_dim(Vector *v);
int vector_num_elts(Vector *v);
void vector_zero(Vector *v);
void vector_add(Vector *v1, Vector *v2, Vector *ret);
void vector_multiply(Vector *v, double s, Vector *ret);
double dot(Vector *v1, Vector *v2);
double norm2(Vector *v);
double norm(Vector *v);
double vector_dist2(Vector *v1, Vector *v2);
double vector_dist(Vector *v1, Vector *v2);
void vector_add_col(Vector *v);
void vector_add_ncols(Vector *v, unsigned int n);
void vector_remove_col(Vector *v, unsigned int c);
int vector_iszero(Vector *V);
int vector_equals(Vector *v1, Vector *v2);
void vector_convert_nonsparse_to_sparray(Vector *v, ExpandingArray *colMap);
void vector_print(Vector *v);
void vector_write(Vector *v, char *filename);
void vector_write_fp(Vector *v, FILE *out);
void vector_write_sp(Vector *v, char *filename);
void vector_write_sp_fp(Vector *v, FILE *out);
size_t vector_write_bin(Vector *v, char *filename);
size_t vector_write_bin_fp(Vector *v, FILE *fp);
Vector *vector_read_bin(char *filename);
Vector *vector_read_bin_fp(FILE *fp);
Vector *vector_map(void **addr, void *last_addr);
void *vector_memmove(void *to, Vector *from);
size_t vector_size(Vector *v);
void vector_free(Vector *v);

//Vector iterator functions
inline void vectorit_set_at_beg(VectorIterator *vit, Vector *v);
inline void vectorit_set_at_end(VectorIterator *vit, Vector *v);
inline double vectorit_curr_val(VectorIterator vit, Vector *v);
inline unsigned int vectorit_curr_col(VectorIterator vit, Vector *v);
inline void vectorit_next(VectorIterator *vit, Vector *v);
inline void vectorit_prev(VectorIterator *vit, Vector *v);
inline void vectorit_set_col(VectorIterator vit, unsigned int c, Vector *v);
void vectorit_zero_elt(VectorIterator *vit, Vector *v);
int vectorit_past_end(VectorIterator vit, Vector *v);
extern inline int vectorit_past_end(VectorIterator vit, Vector *v) {
  return vit.pastend;
}

int vectorit_past_beg(VectorIterator vit, Vector *v);
extern inline int vectorit_past_beg(VectorIterator vit, Vector *v) {
  return vit.pastbeg;
}
void vectorit_insert(VectorIterator *vit, unsigned int c, double d, Vector *v);
void vectorit_copy(VectorIterator from, VectorIterator *to);
inline void vectorit_find(VectorIterator *vit, unsigned int c, Vector *v);

#endif //crm_svm_matrix.h
