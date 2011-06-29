//	crm_svm.h - Support Vector Machine

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

#ifndef __CRM_SVM__H
#define __CRM_SVM__H

#include "crm_svm_lib_fncts.h"
#include "crm114_sysincludes.h"
#include "crm114_config.h"
#include "crm114_structs.h"
#include "crm114.h"
#include <string.h>

#define N_CONSTANTS_IN_SVM_BLOCK 7 //the number of integers in crm_svm_block
                                   //(n_old, has_solution, n0, n1, n0f, n1f, 
                                   //map_size = 7)
#define N_CONSTANTS_NOT_IN_BLOCK 1 //the number of integers not in the
                                   //crm_svm_block (n_new = 1)
#define N_OFFSETS_IN_SVM_FILE 3    //the number of size_t's giving offsets
                                   //to various locations in the file
#define SVM_DEFAULT_MAP_SIZE 1000  //size the VECTOR MAP starts at
#define HAS_SOLUTION_INDEX 1       //index of has_solution in the crm_svm_block
#define MAP_SIZE_INDEX 6           //index of map_size in the crm_svm_block
#define SVM_USE_MMAP               //define if you want file I/O to be done
                                   //using crm_mmap and crm_munmap rather than
                                   //fread and fwrite (default is to define it)
#define SVM_FIRST_BITS "CRM114 SVM FILE FOLLOWS:" //SVM file magic string
#define SVM_FIRST_NBIT strlen(SVM_FIRST_BITS)*sizeof(char) //SVM magic string
                                                           //length (in bytes)

extern int SVM_DEBUG_MODE;         //defined in crm_svm_matrix_util.h

extern char *outbuf;               //defined in crm_main.c

//this is the full SVM block we use while learning
//for classifying, appending, etc, we usually only
//read or write part of the block.
typedef struct {
  int n_old,         //the number of examples in oldXy
                     //ie, the number of examples we have seen but aren't
                     //training on currently
    has_solution,    //1 if the block contains a solution, 0 else
    n0, n1,          //number of examples in classes 0 (n0) and 1 (n1) 
    n0f, n1f,        //number of total features in classes 0 (n0f) and 1 (n1f)
    map_size;        //the size of the mapping from rows to vectors
  SVM_Solution *sol; //previous solution if it exists or NULL
  Matrix *oldXy;     //examples we've seen but aren't training on or NULL
  Matrix *newXy;     //examples we haven't trained on yet or NULL
  int was_mapped;    //1 if the block was mapped in
} crm_svm_block;

typedef struct {
  Vector *vptr;
  int place;
} vector_id;

#endif //crm_svm.h
