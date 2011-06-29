//	crm_svm.c - Support Vector Machine

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

#include "crm_svm.h"

//static function declarations
static Vector *convert_document(char *text, long text_len,
				unsigned int *features,
				ARGPARSE_BLOCK *apb);
static int compare_features(const void *a, const void *b);

//depending on whether SVM_USE_MMAP is set these use mmap or fread
static void *map_svm_file(crm_svm_block *blck, char *filename);
static void svm_get_meta_data(char *filename, crm_svm_block *blck);
static int has_new_vectors(char *filename);
static Vector *get_theta_from_svm_file(char *filename, void **st_addr);

//these are around for the times when we want to read in the file
//without overwriting what we have in memory (ie in a learn during
// a classify)
static int read_svm_file(crm_svm_block *blck, char *filename);
static int read_svm_file_fp(crm_svm_block *blck, FILE *fp);

//these always use fwrite.  they have to be called sometimes even
//when SVM_USE_MMAP is set to grow the file size.
static size_t write_svm_file(crm_svm_block *blck, char *filename);
static size_t write_svm_file_fp(crm_svm_block *blck, FILE *fp);
static size_t svm_write_theta(Vector *theta, FILE *fp);

//this writes to the mmap'd file in memory if there's room or
//forces an unmap and calls append (then the next call to save_changes
//or write will grow the file)
static size_t append_vector_to_svm_file(Vector *v, char *filename);

//this writes everything back to disk using fwrite or unmap as
//appropriate.  if the file was read, it always uses fwrite.  if
//the file was mapped in, it tries to alter that memory to have the
//correct new values in it and, if it can't, fwrites it.
static size_t crm_svm_save_changes(crm_svm_block *blck, void *addr, 
				   char *filename);

static void crm_svm_block_init(crm_svm_block *blck);
static void crm_svm_block_free_data(crm_svm_block blck);
static void crm_svm_learn_new_examples(crm_svm_block *blck, int microgroom);

//set the debug value
//NOTE THAT: SVM_DEBUG_MODE, the variable used in the other svm files
//is min(0, svm_trace-1).
//see crm_svm_matrix_util.h for details, but a general scheme is
//0 will print out nothing
//1 will print out enough to update you on the progress
//2 is the first setting that prints anything from functions not in this file
//3-6 will print out enough info to tell you where the solver is getting
// stuck (not that that should happen!)
//7 can be used for big runs but only if you know what you're looking for
//8-9 should only be used on small runs because they print out big
// matrices
int svm_trace = 0;

//This is a "smart mode" where the SVM trains on examples in the way it
//thinks is best for it to learn.
//Mainly:
// It waits until it has SVM_BASE_EXAMPLES before doing a learn
// regardless of whether the user has actually put on an append.
// After that it does the incremental algorithm on up to SVM_INCR_FRAC
// appended examples.
// If more than SVM_INCR_FRAC get appended, it does a from start learn.
int svm_smart_mode = 0;

/**********************CONVERTING TEXT TO FEATURES***************************/

//function to be passed to qsort that compares two features
//the sort will be in INCREASING feature value order
static int compare_features(const void *a, const void *b) {
  unsigned int *c = (unsigned int *)a;
  unsigned int *d = (unsigned int *)b;
  if (*c < *d) {
    return -1;
  }
  if (*c > *d) {
    return 1;
  }
  return 0;
}
				     

/*******************************************************************
 *Helper function to convert text to features.
 *
 *INPUT: text: text to convert
 * text_len: number of characters in text
 * apb: the argparse block.
 *
 *OUTPUT: (features: as pass by reference contains the exact features.)
 * A vector of the features as a MATR_COMPACT, SPARSE_ARRAY.  This
 * feature vector multiplies the features by their label and adds
 * in a constant term - both things specific to the SVM.  In other
 * words, if apb contains the flag CRM_REFUTE, the vector will multiply
 * every feature by -1 (since CRM_REFUTE indicates a feature with
 * a -1 label).  In addition, if SVM_ADD_CONSTANT is set, EVERY vector 
 * returned from this function will have a +/-1 (according to its label) 
 * in the first column.  This is to introduce a "constant" value into the 
 * SVM classification, as discussed in the comment to 
 * svm_solve_no_init_sol in crm_svm_lib_fncts.c.  
 *
 *WARNINGS:
 *1) You need to free the returned vector (using vector_free)
 *   once you are done with it.
 *2) The returned vector is NOT just a vector of the features.  We do
 *   SVM-specific manipulations to it, specifically, multiplying the 
 *   features by their label and adding a column if SVM_ADD_CONSTANT
 *   is set.
 *******************************************************************/
static Vector *convert_document(char *text, long text_len,
				unsigned int *features,
				ARGPARSE_BLOCK *apb) {
  long next_offset;
  long n_features, i;
  int class;
  Vector *v;
  VectorIterator vit;
 
  crm_vector_tokenize_selector
    (apb,                   // the APB
     text,                   // input string buffer
     0,                      // start offset
     text_len,               // length
     NULL,                  // parser regex
     0,                     // parser regex len
     NULL,                   // tokenizer coeff array
     0,                      // tokenizer pipeline len
     0,                      // tokenizer pipeline iterations
     features,           // where to put the hashed results
     MAX_SVM_FEATURES - 1,        //  max number of hashes
     &n_features,             // how many hashes we actually got
     &next_offset);           // where to start again for more hashes

  if (apb->sflags & CRM_REFUTE) {
    //this is a negative example
    class = -1;
  } else {
    class = 1;
  }
  
  if (!n_features) {
    //blank document
    if (SVM_ADD_CONSTANT) {
      v = vector_make_size(1, SPARSE_ARRAY, MATR_COMPACT, 1);
      vectorit_set_at_beg(&vit, v);
      vectorit_insert(&vit, 0, class, v);
    } else {
      v = vector_make_size(1, SPARSE_ARRAY, MATR_COMPACT, 0);
    }
    return v;
  }

  //Put the features into a vector
  qsort(features, n_features, sizeof(unsigned int), compare_features);

  v = vector_make_size(features[n_features-1]+1, SPARSE_ARRAY, MATR_COMPACT, 
		       n_features);

  vectorit_set_at_beg(&vit, v);

  //the SVM solver does not incorporate a constant offset
  //putting this in all of our feature vectors gives that constant offset
  if (SVM_ADD_CONSTANT) {
    vectorit_insert(&vit, 0, class, v);
  }
  //put the features into the vector, making them unique
  //if necessary
  for (i = 0; i < n_features; i++) {
    if (features[i] == 0) {
      continue;
    }
    vectorit_find(&vit, features[i], v);
    if (vectorit_curr_col(vit, v) == features[i]) {
      if (!(apb->sflags & CRM_UNIQUE)) {
	//if we see something twice and we don't have UNIQUE set
	//it's entry is 2 (or -2) instead of 1
	vectorit_insert(&vit, features[i], vectorit_curr_val(vit, v) + class,
			 v);
      }
    } else {
      vectorit_insert(&vit, features[i], class, v);
    }
  }

  //make v only take up the amount of memory it should
  if (v && v->type == SPARSE_ARRAY) {
    expanding_array_trim(v->data.sparray);
  }
  return v;
}

/**************************SVM FILE FUNCTIONS*********************************/

/******************************************************************************
 *
 *There are two sets of functions here.  One set is used when SVM_USE_MMAP is
 *defined and, whenever possible, uses crm_mmap and crm_munmap to do file I/O.
 *The other, used when SVM_USE_MMAP is not defined, uses exclusively fread and
 *fwrite to do file I/O.  When caching is enabled, having SVM_USE_MMAP 
 *will SIGNIFICANTLY speed up file I/O.  In addition, using mmap allows shared
 *file I/O.  We there recommend you use SVM_USE_MMAP if possible.
 *
 *Note that SVM_USE_MMAP may call fread and fwrite.  It uses fwrite when it is
 *necessary to grow the file.  The file approximately doubles in size each
 *time fwrite is called, so calls to fwrite should, hopefully, amortize out
 *over the run.  It uses fread when it needs to do a learn in classify since
 *classify shouldn't make changes to the file.  This can be avoided by calling
 *learn without ERASE or APPEND before doing any classifies.
 *
 *Without SVM_USE_MMAP enabled, mmap is NEVER called.
 *
 *The SVM file is a binary file formatted as follows:
 *
 *SVM_FIRST_NBIT bytes: A string or whatever you want defined in 
 * SVM_FIRST_BITS.  This isn't a checksum since we don't want to have to read
 * in the whole file every time in order to verify it - it's simply a stored
 * value (or even a string) that all SVM stat files have as the first few
 * bytes to identify them.  While there is as much error checking as I can do
 * in this code, non-SVM binary files can create seg faults by mapping two
 * vector headers into the same space so that changing one changes part of
 * another.  There is almost nothing I can do about that, so, to eliminate
 * that problem as far as I can, we have a little "magic string" in front.
 *
 *N_OFFSETS_IN_SVM_FILE size_t's:
 *
 * size_t size: The offset until the end of the actual data stored in the file.
 *  We leave a large hole at the end of the file so we can append to it without
 *  having to uncache it from memory.  This is the offset to the beginning of
 *  the hole.  When reading the file in, we do not need to read past this
 *  offset since the rest is garbage.  This changes each time we append a
 *  vector.
 *
 * size_t OLD_OFFSET: The offset in bytes from the BEGINNING of the file to the
 *  matrix of old vectors if it exists.  This stays constant through maps and
 *  unmaps, but may change at fwrite.
 *
 * size_t NEW_OFFSET: The offset in bytes from the BEGINNING of the file to the
 *  begining of any vectors that have been appended since the last full map
 *  or read.  This is "size" at the time of the last read or map.
 *
 *N_CONSTANTS_NOT_IN_BLOCK ints:
 *
 * int n_new: number of examples that we have read in but HAVEN'T learned on.
 *  Clearly this number cannot include vectors that have been appended since
 *  the last read or map of the file.  If n_new > 0, we certainly have new
 *  vectors to learn on, but n_new = 0 DOES NOT indicate no new vectors to
 *  learn on.  It is also necessary to seek to NEW_OFFSET and check if there
 *  are vectors there.
 *
 *N_CONSTANTS_IN_SVM_BLOCK ints:
 *
 * int n_old: number of examples we have learned on that aren't support vectors
 *
 * int has_solution: 1 if there is a solution in the file, 0 else
 *
 * int n0: number of examples in class 0
 *
 * int n1: number of examples in class 1
 *
 * int n0f: total number of features in class 0
 *
 * int n1f: total number of features in class 1
 *
 * int map_size: the current size of the map that maps matrix rows to their
 *  actual location in the file.  This stays larger (usually) than the
 *  number of total vectors in the file because appending wouldn't allow
 *  us to grow at this point it the file.  Therefore, we leave a "hole" so
 *  that we don't always have to unmap the file if new vectors have been
 *  appended.
 *
 *VECTOR MAP:
 *
 * A map_size array of ints.  The ith entry in VECTOR_MAP is the offset from
 * the BEGINNING of the file to the ith vector where vectors are in SV, old,
 * new order.
 *
 *DECISION BOUNDARY:
 *
 * Decision vector: the decision vector written as a vector
 *
 * int fill: the amount of filler we leave to allow the decision boundary to
 *  to grow without having to grow the file.
 *
 * void fill: a "hole" allowing the decision vector to grow in size in new
 *  learns.
 *
 *RESTART CONSTANTS:
 *
 * int num_examples: the total number of examples we've learned on (since the
 *  beginning or the last FROMSTART)
 *
 * int max_train_val: sum_c alpha_c <= max_train_val (constant used in
 *  restarting - see crm_svm_lib_fncts.c for details)
 *
 *SV MATRIX:
 *
 * The support vector matrix header.  When the file is written using fwrite,
 * the support vectors are written after the header.  However, since SVs can
 * change on subsequent learns, the vectors written after SV matrix (if any)
 * aren't guaranteed to be SVs any more.  The VECTOR MAP must be used to
 * reconstruct the SV MATRIX.
 *
 *OLDXY MATRIX (at OLD_OFFSET):
 *
 * The oldXy matrix header.  The oldXy matrix consists of examples we have 
 * learned on, but that aren't SVs.  When the file is written using fwrite,
 * all rows of oldXy are written after oldXy.  However since these rows can
 * change on subsequent learns, the vectors written after oldXy (if any)
 * aren't guaranteed to actually be old, non-SV examples.  The VECTOR MAP
 * must be used to reconstruct the OLDXY MATRIX.
 *
 *NEW VECTORS YET TO BE LEARNED ON (at NEW_OFFSET or stored in VECTOR_MAP):
 *
 * Each new vector is formatted as a vector (ie we don't keep the matrix header
 * - this makes appending easy).  Some of them may not be in the VECTOR MAP if 
 * they have been appended since the last full read/map in.  These are all
 * listed after NEW_OFFSET.
 *
 *The file is formatted this way to make the following actions quick both using
 * fread/fwrite and mmap/munmap:
 *
 * Finding if the file has a solution: requires a seek to has_solution and a
 *  read of that value.
 * 
 * Finding the decision boundary if it exists: requires a sequential fread
 *  of N_CONSTANTS_IN_SVM_BLOCK, a seek to DECISION BOUNDARY, reading in the
 *  vector stored there.
 *
 * Querying if there are unlearned on vectors: requries a seek to the position
 *  of NEW_OFFSET in the file, a sequential read of NEW_OFFSET and of n_new.  
 *  If n_new = 0, requires a seek to NEW_OFFSET.
 *
 * Appending a vector:
 *  using fread/fwrite: requires opening the file for appending and writing
 *   out the vector
 *  using mmap/munmap: requires mapping in the file, reading in size and
 *   seeking to point size in the file.  if there is room, writes the vector
 *   there.  else forcibly munmaps the file and opens it for appending.
 *****************************************************************************/

//mmap functions
#ifdef SVM_USE_MMAP

//maps the full file into blck.  used before calling learn_new_examples.
static void *map_svm_file(crm_svm_block *blck, char *filename) {
  struct stat statbuf;
  long act_size;
  void *addr, *last_addr, *st_addr;
  Vector *v;
  size_t old_offset, new_offset, size;
  int *vmap, fill, curr_rows = 0, n_new, i;

  if (stat(filename, &statbuf)) {
    nonfatalerror("Attempt to read from nonexistent svm file", filename);
    return NULL;
  }

  if (!blck) {
    //this really shouldn't happen
    fatalerror5("map_svm_file: bad crm_svm_block pointer.", "",
		CRM_ENGINE_HERE);
    return NULL;
  }

  crm_svm_block_init(blck);

  addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, &act_size);

  if (addr == MAP_FAILED) {
    nonfatalerror("Attempt to map svm file failed.  The file was", filename);
    return NULL;
  }

  st_addr = addr;
  
  if (act_size < sizeof(size_t) + SVM_FIRST_NBIT) {
    nonfatalerror
      ("Attempt to read from corrupted svm file.  It is much too small.", "");
    crm_munmap_file(st_addr);
    return NULL;
  }

  if (strncmp(SVM_FIRST_BITS, (char *)st_addr, strlen(SVM_FIRST_BITS))) {
    nonfatalerror
      ("Attempt to map from corrupted SVM file.  The header is incorrect.", "");
    crm_munmap_file(st_addr);
    return NULL;
  }

  addr += SVM_FIRST_NBIT;

  //this is where the data actually ends
  size = *((size_t*)addr);
  if (size > act_size) {
    //corrupted file
    nonfatalerror("Attempt to read from corrupted svm file.  It thinks it has a larger length than it does.  The file is", filename);
    crm_munmap_file(st_addr);
    return NULL;
  }
  addr += sizeof(size_t);
  last_addr = st_addr + size; //last address that contains good data

  if (size < N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
      (N_CONSTANTS_IN_SVM_BLOCK + N_CONSTANTS_NOT_IN_BLOCK)*sizeof(int)) {
    //this is isn't a good file
    nonfatalerror("Attempt to read from corrupted svm file.  It is somewhat too small.", filename);
    crm_munmap_file(st_addr);
    return NULL;
  }

  old_offset = *((size_t*)addr);        //where oldXY header is
  addr += sizeof(size_t);
  new_offset = *((size_t*)addr);        //where new vectors not in vmap are
  addr += sizeof(size_t);
  n_new = *((int *)addr);               //# of read in, not learned on vectors
  addr += sizeof(int);
  blck->n_old = *((int*)(addr));        //# of learned-on, non-SV vectors
  addr += sizeof(int);
  blck->has_solution = *((int*)(addr)); //do we have a solution?
  addr += sizeof(int);
  blck->n0 = *((int *)(addr));          //# learned-on examples in class 0
  addr += sizeof(int);
  blck->n1 = *((int *)(addr));          //# learned-on examples in class 1
  addr += sizeof(int);
  blck->n0f = *((int *)(addr));         //# features in class 0
  addr += sizeof(int);
  blck->n1f = *((int *)(addr));         //# features in class 1
  addr += sizeof(int);
  blck->map_size = *((int *)(addr));    //space allocated for vmap
  addr += sizeof(int);

  if (addr + sizeof(int)*blck->map_size > last_addr) {
    nonfatalerror
      ("Attempt to map from bad svm file.  It can't fit its own map.", "");
    crm_svm_block_init(blck);
    crm_munmap_file(st_addr);
    return NULL;
  }

  
  vmap = (int *)addr; //map that tells us where each vector is stored
  addr += sizeof(int)*blck->map_size;

  if (blck->has_solution) {
    //read in the solution
    blck->sol = (SVM_Solution *)malloc(sizeof(SVM_Solution));
    blck->sol->theta = vector_map(&addr, last_addr); //decision boundary
    blck->sol->SV = NULL;
    if (addr + sizeof(int) > last_addr) {
      nonfatalerror
	("Attempt to map from bad svm file.  It can't fit its solution.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    fill = *((int *)addr); //hole to grow decision boundary
    addr += sizeof(int);
    if (!blck->sol->theta || addr + fill + 2*sizeof(int) + 
	sizeof(Matrix) > last_addr) {
      nonfatalerror
	("Attempt to map from bad svm file.  It can't fit in the SV matrix.", 
	 "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    addr += fill;
    //restart constants
    blck->sol->num_examples = *((int *)addr);
    addr += sizeof(int);
    blck->sol->max_train_val = *((int *)addr);
    addr += sizeof(int);
    blck->sol->SV = (Matrix *)addr; //SV matrix header
    addr += sizeof(Matrix);
    blck->sol->SV->was_mapped = 1;
    blck->sol->SV->data = 
      (Vector **)malloc(sizeof(Vector *)*blck->sol->SV->rows);
    if (!blck->sol->SV->data) {
      nonfatalerror("Unable to allocate enough memory for support vector matrix.  This is likely a corrupted SVM file, but we aren't going to be able to recover from it.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    //read in the SV vectors using vmap
    for (i = 0; i < blck->sol->SV->rows; i++) {
      addr = st_addr + vmap[i + curr_rows];
      blck->sol->SV->data[i] = vector_map(&addr, last_addr);
      if (!blck->sol->SV->data[i]) {
	break;
      }
    }
    if (i != blck->sol->SV->rows) {
      blck->sol->SV->rows = i;
      nonfatalerror("Attempt to map from bad svm file.  An SV was wrong somehow.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    curr_rows += blck->sol->SV->rows;
  }


  //oldXy matrix
  if (blck->n_old) {
    addr = st_addr + old_offset;
    if (addr + sizeof(Matrix) > last_addr) {
      nonfatalerror("Attempt to map from bad svm file.  There's no room for the old example matrix.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    blck->oldXy = (Matrix *)addr; //oldXy header
    addr += sizeof(Matrix);
    blck->oldXy->was_mapped = 1;
    blck->oldXy->data = (Vector **)malloc(sizeof(Vector *)*blck->oldXy->rows);
    if (!blck->oldXy->data) {
      nonfatalerror("Unable to allocate enough memory for support vector matrix.  This is likely a corrupted SVM file.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    //read in oldXy vectors using vmap
    for (i = 0; i < blck->oldXy->rows; i++) {
      addr = st_addr + vmap[i + curr_rows];
      blck->oldXy->data[i] = vector_map(&addr, last_addr);
      if (!blck->oldXy->data[i]) {
	break;
      }
    }
    if (i != blck->oldXy->rows) {
      blck->oldXy->rows = i;
      nonfatalerror("Attempt to map from bad svm file.  An old example was wrong somehow.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    curr_rows += blck->oldXy->rows;
  }

  //newXy vectors
  if (n_new) {
    //read in ones we've already read in and put in vmap
    addr = st_addr + vmap[curr_rows];
    v = vector_map(&addr, last_addr);
    i = 0;
    if (v) {
      blck->newXy = matr_make_size(n_new, v->dim, v->type, v->compact,
				   v->size);
      if (!blck->newXy) {
	nonfatalerror("Attempt to map from bad svm file.  An unrecognized new vector type in our new matrix..", "");
	crm_svm_block_free_data(*blck);
	crm_svm_block_init(blck);
	crm_munmap_file(st_addr);
	return NULL;
      }
      matr_shallow_row_copy(blck->newXy, 0, v);
      for (i = 1; i < n_new; i++) {
	addr = st_addr + vmap[i + curr_rows];
	v = vector_map(&addr, last_addr);
	if (!v) {
	  break;
	}
	matr_shallow_row_copy(blck->newXy, i, v);
      }
    }
    if (i != n_new) {
      nonfatalerror("Attempt to map from bad svm file.  A new vector was wrong somewhere.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
  }

  addr = st_addr + new_offset;

  //read in any vectors that have been appended since the last map
  if (addr < last_addr) {
    v = vector_map(&addr, last_addr);
    if (v) {
      if (!blck->newXy) {
	blck->newXy = matr_make_size(0, v->dim, v->type, v->compact, v->size);
      }
      if (!blck->newXy) {
	nonfatalerror("Attempt to map from bad svm file.  A very new vector had an unrecognized type.", "");
	crm_svm_block_free_data(*blck);
	crm_svm_block_init(blck);
	crm_munmap_file(st_addr);
	return NULL;
      }
      matr_shallow_row_copy(blck->newXy, blck->newXy->rows, v);
      while (addr < last_addr) {
	v = vector_map(&addr, last_addr);
	if (v && v->dim) {
	  matr_shallow_row_copy(blck->newXy, blck->newXy->rows, v);
	} else {
	  if (v && !v->dim) {
	    vector_free(v);
	  }
	  break;
	}
      }
    }
  }

  return st_addr;
}

//gets the integers (like n0, n1, etc) stored in the first few bytes
//of the file without reading in the whole file.
//puts them in blck
static void svm_get_meta_data(char *filename, crm_svm_block *blck) {
  void *addr, *last_addr, *st_addr;
  struct stat statbuf;
  size_t size;
  long act_size;


  if (stat(filename, &statbuf)) {
    //heck, we don't even have a file!
    nonfatalerror
      ("You are trying to use an SVM to classify from the nonexistant file",
       filename);
    if (blck) {
      blck->n_old = 0;
      blck->has_solution = 0;
      blck->n0 = 0;
      blck->n1 = 0;
      blck->n0f = 0;
      blck->n1f = 0;
      blck->map_size = SVM_DEFAULT_MAP_SIZE;
    } else {
      fatalerror5("svm_get_meta_data: bad crm_svm_block pointer.", "",
		  CRM_ENGINE_HERE);
    }
    return;
  }

  if (!blck) {
    fatalerror5("svm_get_meta_data: bad crm_svm_block pointer.", "",
		CRM_ENGINE_HERE);
    return;
  }

  //just always do PROT_READ | PROT_WRITE so that if it's cached we get it
  addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE, 
		       MAP_SHARED,
		       &act_size);
  if (addr == MAP_FAILED || act_size < sizeof(size_t) + SVM_FIRST_NBIT) {
    fatalerror5("Could not map SVM file to get meta data.  Something is very wrong and I doubt we can recover.  The file is", filename, CRM_ENGINE_HERE);
    if (addr != MAP_FAILED) {
      crm_munmap_file(addr);
    }
    return;
  }

  st_addr = addr;

  if (strncmp(SVM_FIRST_BITS, (char *)addr, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("This svm file is corrupted.  The file is", filename);
    blck->n_old = 0;
    blck->has_solution = 0;
    blck->n0 = 0;
    blck->n1 = 0;
    blck->n0f = 0;
    blck->n1f = 0;
    blck->map_size = SVM_DEFAULT_MAP_SIZE;
    crm_munmap_file(st_addr);
    return;
  }

  addr += SVM_FIRST_NBIT;
  size = *((size_t *)addr); //actual size (rest is garbage hole)
  last_addr = st_addr + size;

  if (size > act_size || addr + N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
      (N_CONSTANTS_IN_SVM_BLOCK + N_CONSTANTS_NOT_IN_BLOCK)*sizeof(int) 
      > last_addr) {
    nonfatalerror("This svm file is corrupted.  The file is", filename);
    blck->n_old = 0;
    blck->has_solution = 0;
    blck->n0 = 0;
    blck->n1 = 0;
    blck->n0f = 0;
    blck->n1f = 0;
    blck->map_size = SVM_DEFAULT_MAP_SIZE;
    crm_munmap_file(st_addr);
    return;
  }

  addr += N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
    N_CONSTANTS_NOT_IN_BLOCK*sizeof(int);
  blck->n_old = *((int *)addr);        //# learned-on, non-SV examples
  addr += sizeof(int);
  blck->has_solution = *((int *)addr); //1 if there is a solution
  addr += sizeof(int);
  blck->n0 = *((int *)addr);           //# examples in class 0
  addr += sizeof(int);
  blck->n1 = *((int *)addr);           //# examples in class 1
  addr += sizeof(int);
  blck->n0f = *((int *)addr);          //# features in class 0
  addr += sizeof(int);
  blck->n1f = *((int *)addr);          //# features in class 1
  addr += sizeof(int);
  blck->map_size = *((int *)addr);     //size of vector map

  crm_munmap_file(st_addr);

}

//returns 1 if the file has vectors that have been appended but not yet
//learned on
//returns 0 else
static int has_new_vectors(char *filename) {
  Vector *v;
  void *addr, *last_addr, *st_addr;
  size_t offset, size;
  int n_new;
  struct stat statbuf;
  long act_size;

  if (stat(filename, &statbuf)) {
    //heck, we don't even have a file!
    return 0;
  }

  //this is PROT_WRITE because, if we read in a vector, we may flip
  //a bit telling us that the vector was mapped in - which tells us what parts
  //of the vector should be freed

  addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE, 
		       MAP_SHARED, &act_size);

  if (addr == MAP_FAILED || act_size < sizeof(size_t) + SVM_FIRST_NBIT) {
    nonfatalerror("There was a problem mapping the svm file in while checking for new vectors.  I am going to assume there are no new vectors.  The file was",
		  filename);
    if (addr != MAP_FAILED) {
      crm_munmap_file(addr);
    }
    return 0;
  }

  st_addr = addr;

  if (strncmp(SVM_FIRST_BITS, (char *)addr, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("The SVM file is corrupted.  I am going to assume it contains no new examples.  The file is", filename);
    crm_munmap_file(st_addr);
    return 0;
  }

  addr += SVM_FIRST_NBIT;

  size = *((size_t *)addr); //actual amount of good data
  last_addr = st_addr + size;

  if (size > act_size || addr + 3*sizeof(size_t) + sizeof(int) > last_addr) {
    nonfatalerror("There was a problem mapping the svm file in while checking for new vectors.  I am going to assume there are no new vectors.  The file was",
		  filename);
    crm_munmap_file(st_addr);
    return 0;
  }

  addr += 2*sizeof(size_t);
  offset = *((size_t *)addr); //offset to new, unread vectors
  addr += sizeof(size_t);
  n_new = *((int *)addr);    //number of new, read vectors
  addr += sizeof(int);
  if (n_new) {
    //yep, definitely have new vectors
    crm_munmap_file(st_addr);
    return 1;
  }

  //do we have vectors we haven't read in before but have appended?
  addr = st_addr + offset;

  if (addr >= last_addr) {
    crm_munmap_file(st_addr);
    return 0;
  }

  //do we really have a vector?  let's try reading one in
  v = vector_map(&addr, last_addr);
  crm_munmap_file(st_addr);
  if (v) {
    vector_free(v);
    return 1;
  }

  return 0;
}


//returns the decision boundary from an svm file
//we map the decision boundary from the file so you must
// FREE THE DECISION BOUNDARY returned by the function
// MUNMAP THE FILE returned pass-by-reference in *addr
static Vector *get_theta_from_svm_file(char *filename, void **st_addr) {
  Vector *v;
  void *last_addr, *addr;
  size_t size;
  int *hs;
  struct stat statbuf;
  long act_size;

  if (stat(filename, &statbuf)) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't exist.  The file is", filename);
    return NULL;
  }

  //this is PROT_WRITE because, if we read in theta, we may need to flip
  //a bit telling us that theta was mapped in - which tells us what parts
  //of theta should be freed

  addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, &act_size);

  if (addr == MAP_FAILED || act_size < sizeof(size_t) + SVM_FIRST_NBIT) {
    nonfatalerror("Attempt to map svm file while getting decision boundary failed.  The file was", filename);
    if (addr != MAP_FAILED) {
      crm_munmap_file(addr);
    }
    *st_addr = NULL;
    return NULL;
  }

  *st_addr = addr;
  if (strncmp(SVM_FIRST_BITS, (char *)addr, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("Attempt to read decision boundary from a corrupt SVM file.  The file was", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    return NULL;
  }

  addr += SVM_FIRST_NBIT;
  size = *((size_t *)addr);
  last_addr = *st_addr + size;

  if (size > act_size || addr + N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
      (N_CONSTANTS_NOT_IN_BLOCK+N_CONSTANTS_IN_SVM_BLOCK)*sizeof(int) 
      > last_addr) {
    nonfatalerror("Attempt to map svm file while getting decision boundary failed.  The file was", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    return NULL;
  }

  addr += N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
    N_CONSTANTS_NOT_IN_BLOCK*sizeof(int);
  hs = (int *)addr;
  addr += sizeof(int)*N_CONSTANTS_IN_SVM_BLOCK;

  if (addr > last_addr || !hs[HAS_SOLUTION_INDEX] ||
      addr + sizeof(int)*hs[MAP_SIZE_INDEX] > last_addr) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain an SVM solution or is corrupted.  The file is", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    return NULL;
  }

  addr += sizeof(int)*hs[MAP_SIZE_INDEX];

  v = vector_map(&addr, last_addr);
  
  return v;
}

//fread functions
#else

//reads a binary svm block from a file
//to make the ifdefs work this has the same prototype as the
//function that maps the file in, but it always returns NULL.
static void *map_svm_file(crm_svm_block *blck, char *filename) {
  FILE *fp = fopen(filename, "rb");
  int ret;
  
  if (!fp) {
    //this file doesn't exist
    nonfatalerror("Attempt to read from nonexistent svm file", filename);
    return NULL;
  }

  ret = read_svm_file_fp(blck, fp);
  
  fclose(fp);
  
  if (ret) {
    return (void *)filename;
  } else {
    return NULL;
  }
}



//gets the integers (like n0, n1, etc) stored in the first few bytes
//of the file without reading in the whole file.
//puts them in blck
static void svm_get_meta_data(char *filename, crm_svm_block *blck) {
  FILE *fp = fopen(filename, "rb");
  size_t amount_read;
  char firstbits[strlen(SVM_FIRST_BITS)];

  if (!fp) {
    //heck, we don't even have a file!
    nonfatalerror
      ("You are trying to use an SVM to classify from the nonexistant file",
       filename);
    if (blck) {
      blck->n_old = 0;
      blck->has_solution = 0;
      blck->n0 = 0;
      blck->n1 = 0;
      blck->n0f = 0;
      blck->n1f = 0;
      blck->map_size = SVM_DEFAULT_MAP_SIZE;
    } else {
      fatalerror5("svm_get_meta_data: bad crm_svm_block pointer.", "",
		  CRM_ENGINE_HERE);
    }
    return;
  }

  if (!blck) {
    fatalerror5("svm_get_meta_data: bad crm_svm_block pointer.", "",
		CRM_ENGINE_HERE);
    fclose(fp);
    return;
  }

  amount_read = fread(firstbits, 1, SVM_FIRST_NBIT, fp);
  if (strncmp(SVM_FIRST_BITS, firstbits, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("This svm file is corrupted.  The file is", filename);
    blck->n_old = 0;
    blck->has_solution = 0;
    blck->n0 = 0;
    blck->n1 = 0;
    blck->n0f = 0;
    blck->n1f = 0;
    blck->map_size = SVM_DEFAULT_MAP_SIZE;
    fclose(fp);
    return;
  }

  fseek(fp, SVM_FIRST_NBIT + N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
	N_CONSTANTS_NOT_IN_BLOCK*sizeof(int), SEEK_SET);
  amount_read = fread(&(blck->n_old), sizeof(int), 1, fp);
  amount_read += fread(&(blck->has_solution), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n0), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n1), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n0f), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n1f), sizeof(int), 1, fp);
  amount_read += fread(&(blck->map_size), sizeof(int), 1, fp);

  if (amount_read < N_CONSTANTS_IN_SVM_BLOCK) {
    nonfatalerror("This svm file is corrupted.  The file is", filename);
    blck->n_old = 0;
    blck->has_solution = 0;
    blck->n0 = 0;
    blck->n1 = 0;
    blck->n0f = 0;
    blck->n1f = 0;
    blck->map_size = SVM_DEFAULT_MAP_SIZE;
  }

  fclose(fp);

}

//returns 1 if the file has vectors that have been appended but not yet
//learned on
//returns 0 else
static int has_new_vectors(char *filename) {
  FILE *fp = fopen(filename, "rb");
  size_t offset, unused, size;
  Vector *v;
  int n_new;
  char firstbits[strlen(SVM_FIRST_BITS)];

  if (!fp) {
    //heck, we don't even have a file!
    return 0;
  }

  unused = fread(firstbits, 1, SVM_FIRST_NBIT, fp);
  if (strncmp(SVM_FIRST_BITS, firstbits, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("This svm file is corrupted.  I am assuming it has no new vectors since I can't read it.  The file is", filename);
    fclose(fp);
    return 0;
  }

  unused = fread(&size, sizeof(size_t), 1, fp);
  fseek(fp, sizeof(size_t), SEEK_CUR);
  unused = fread(&offset, sizeof(size_t), 1, fp);
  unused = fread(&n_new, sizeof(int), 1, fp);
  if (n_new) {
    //we have new vectors
    fclose(fp);
    return 1;
  }

  fseek(fp, offset, SEEK_SET);
  if (feof(fp) || ftell(fp) >= size) {
    //no new vectors
    fclose(fp);
    return 0;
  }

  //maybe new vectors?  sometimes the end of a file is a funny place
  v = vector_read_bin_fp(fp);
  fclose (fp);
  if (v) {
    vector_free(v);
    if (ftell(fp) <= size) {
      return 1;
    }
  }
  return 0;
}

//returns the decision boundary from an svm file
//don't forget to free the boundary when you are done with it!
static Vector *get_theta_from_svm_file(char *filename, void **st_addr) {
  FILE *fp = fopen(filename, "rb");
  int hs[N_CONSTANTS_IN_SVM_BLOCK];
  size_t amount_read, size;
  Vector *v;
  char firstbits[strlen(SVM_FIRST_BITS)];

  *st_addr = NULL;

  if (!fp) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't exist.  The file is", filename);
    return NULL;
  }

  amount_read = fread(firstbits, 1, SVM_FIRST_NBIT, fp);
  if (strncmp(SVM_FIRST_BITS, firstbits, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("This svm file is corrupted.  I cannot read a decision boundary from it.  The file is", filename);
    fclose(fp);
    return NULL;
  }


  amount_read = fread(&size, sizeof(size_t), 1, fp);
  fseek(fp, SVM_FIRST_NBIT+ N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
	N_CONSTANTS_NOT_IN_BLOCK*sizeof(int), SEEK_SET);
  amount_read = fread(hs, sizeof(int), N_CONSTANTS_IN_SVM_BLOCK, fp);
  if (feof(fp) || ftell(fp) >= size || amount_read < N_CONSTANTS_IN_SVM_BLOCK 
      || !hs[HAS_SOLUTION_INDEX]) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain an SVM solution or is corrupted.  The file is", filename);
    return NULL;
  }

  fseek(fp, hs[MAP_SIZE_INDEX]*sizeof(int), SEEK_CUR);

  if (feof(fp) || ftell(fp) >= size) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain an SVM solution or is corrupted.  The file is", filename);
    return NULL;
  }

  v = vector_read_bin_fp(fp);

  if (feof(fp) || ftell(fp) >= size) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain an SVM solution or is corrupted.  The file is", filename);
    vector_free(v);
    fclose(fp);
    return NULL;
  }

  fclose(fp);
 
  return v;
}

#endif

//functions used to read in the file. 
//these are used both under map and read since we read in a file using fread
//always when we need to do a learn in classify.

static int read_svm_file(crm_svm_block *blck, char *filename) {
 FILE *fp = fopen(filename, "rb");
 int ret;

 if (!fp) {
   nonfatalerror("Attempt to read from nonexistent svm file", filename);
   return 0;
 }

 ret = read_svm_file_fp(blck, fp);

 fclose(fp);

 return ret;
}

//reads a binary svm block from a file
//returns 0 on failure
static int read_svm_file_fp(crm_svm_block *blck, FILE *fp) {
  size_t amount_read, old_offset, new_offset, size;
  Vector *v;
  int *vmap, i, curr_rows = 0, fill, n_new;
  char firstbits[strlen(SVM_FIRST_BITS)];

  if (!blck) {
    //this really shouldn't happen
    fatalerror5("read_svm_file_fp: bad crm_svm_block pointer.", "",
		CRM_ENGINE_HERE);
    return 0;
  }


  if (!fp) {
    nonfatalerror("Attempt to read svm from bad file pointer.", "");
    return 0;
  }

  crm_svm_block_free_data(*blck);
  crm_svm_block_init(blck);
  
  amount_read = fread(firstbits, 1, SVM_FIRST_NBIT, fp);
  if (strncmp(SVM_FIRST_BITS, firstbits, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("This svm file is corrupted.  I cannot read it.", "");
    return 0;
  }

  amount_read = fread(&size, sizeof(size_t), 1, fp);
  amount_read = fread(&old_offset, sizeof(size_t), 1, fp);
  amount_read = fread(&new_offset, sizeof(size_t), 1, fp);
  amount_read = fread(&n_new, sizeof(int), 1, fp);
  amount_read = fread(&(blck->n_old), sizeof(int), 1, fp);
  amount_read += fread(&(blck->has_solution), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n0), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n1), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n0f), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n1f), sizeof(int), 1, fp);
  amount_read += fread(&(blck->map_size), sizeof(int), 1, fp);

  if ((amount_read < N_CONSTANTS_IN_SVM_BLOCK) ||
      ftell(fp) > size) {
    nonfatalerror("Attempt to read from bad svm file", "");
    crm_svm_block_init(blck);
    return 0;
  }

  vmap = (int *)malloc(sizeof(int)*blck->map_size);
  amount_read = fread(vmap, sizeof(int), blck->map_size, fp);

  //read in solution
  if (blck->has_solution) {
    blck->sol = (SVM_Solution *)malloc(sizeof(SVM_Solution));
    blck->sol->theta = vector_read_bin_fp(fp);
    blck->sol->SV = NULL;
    amount_read = fread(&fill, sizeof(int), 1, fp);
    fseek(fp, fill, SEEK_CUR);
    if (!blck->sol->theta || !amount_read || feof(fp) || 
	ftell(fp) > size) {
      //die!
      nonfatalerror("Attempt to read from bad svm file.",  "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      free(vmap);
      return 0;
    }
    amount_read = fread(&(blck->sol->num_examples), sizeof(int), 1, fp);
    amount_read += fread(&(blck->sol->max_train_val), sizeof(int), 1, fp);
    if (amount_read < 2) {
      //die!
      nonfatalerror("Attempt to read from bad svm file.",  "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      free(vmap);
      return 0;
    }
    blck->sol->SV = (Matrix *)malloc(sizeof(Matrix));
    amount_read = fread(blck->sol->SV, sizeof(Matrix), 1, fp);
    blck->sol->SV->was_mapped = 0;
    if (!amount_read) {
      //die!
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      free(vmap);
      return 0;
    }
    blck->sol->SV->data = 
      (Vector **)malloc(sizeof(Vector *)*blck->sol->SV->rows);
    for (i = 0; i < blck->sol->SV->rows; i++) { 
     fseek(fp, vmap[i + curr_rows], SEEK_SET);
      blck->sol->SV->data[i] = vector_read_bin_fp(fp);
      if (!blck->sol->SV->data[i]) {
	//oh boy, bad file
	break;
      }
    }
    if (i != blck->sol->SV->rows) {
      blck->sol->SV->rows = i;
      nonfatalerror("Attempt to read from bad SVM file.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      free(vmap);
      return 0;
    }
    curr_rows += blck->sol->SV->rows;
  }

  //read in oldXy
  if (blck->n_old) {
    fseek(fp, old_offset, SEEK_SET);  
    blck->oldXy = (Matrix *)malloc(sizeof(Matrix));
    amount_read = fread(blck->oldXy, sizeof(Matrix), 1, fp);
    blck->oldXy->was_mapped = 0;
    blck->oldXy->data = 
      (Vector **)malloc(sizeof(Vector *)*blck->oldXy->rows);
    for (i = 0; i < blck->oldXy->rows; i++) {
      fseek(fp, vmap[i + curr_rows], SEEK_SET);
      blck->oldXy->data[i] = vector_read_bin_fp(fp);
      if (!blck->oldXy->data[i]) {
	//oh boy, bad file
	break;
      }
    }
    if (i != blck->oldXy->rows) {
      blck->oldXy->rows = i;
      nonfatalerror("Attempt to read from bad SVM file.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      free(vmap);
      return 0;
    }
    curr_rows += blck->oldXy->rows;
  }

  //read in parts of newXy we've seen before
  if (n_new) {
    fseek(fp, vmap[curr_rows], SEEK_SET);
    v = vector_read_bin_fp(fp);
    i = 0;
    if (v) {
      blck->newXy = matr_make_size(n_new, v->dim, v->type, v->compact,
				   v->size);
      if (!blck->newXy) {
	nonfatalerror("Attempt to map from bad svm file.", "");
	crm_svm_block_free_data(*blck);
	crm_svm_block_init(blck);
	free(vmap);
	return 0;
      }
      matr_shallow_row_copy(blck->newXy, 0, v);
      for (i = 1; i < n_new; i++) {
	fseek(fp, vmap[curr_rows+i], SEEK_SET);
	v = vector_read_bin_fp(fp);
	if (!v) {
	  break;
	}
	matr_shallow_row_copy(blck->newXy, i, v);
      }
    }
    if (i != n_new) {
      nonfatalerror("Attempt to read from bad SVM file.", "");
      crm_svm_block_free_data(*blck);
      crm_svm_block_init(blck);
      free(vmap);
      return 0;
    }
  }

  //read in new vectors
  fseek(fp, new_offset, SEEK_SET);
  if (!feof(fp) && ftell(fp) < size) {
    v = vector_read_bin_fp(fp);
    if (v && v->dim) {
      if (!(blck->newXy)) {
	blck->newXy = matr_make_size(0, v->dim, v->type, v->compact, v->size);
      }
      if (!blck->newXy) {
	nonfatalerror("Attempt to map from bad svm file.", "");
	crm_svm_block_free_data(*blck);
	crm_svm_block_init(blck);
	free(vmap);
	return 0;
      }
      matr_shallow_row_copy(blck->newXy, blck->newXy->rows, v);
      while (!feof(fp) && ftell(fp) < size) {
	v = vector_read_bin_fp(fp);
	if (v && v->dim) {
	  matr_shallow_row_copy(blck->newXy, blck->newXy->rows, v);
	} else {
	  if (v && !v->dim) {
	    vector_free(v);
	  }
	  break;
	}
      }
    } else if (v) {
      vector_free(v);
    }
  }
  free(vmap);
  return 1;
}

//fwrite functions.  used by both read and mmap modes since under mmap it
//is sometimes necessary to grow the file

//writes an svm block to a file in binary format
//returns the number of bytes written
//WARNING: this function creates (and removes) a temporary file to avoid
//map/fwrite issues.
static size_t write_svm_file(crm_svm_block *blck, char *filename) {
  //this is tricky because the file may be mmapped in
  //and we may want to be writing some of that back out
  //so we write it to a temporary file
  //then unmap the file
  //then rename the temporary file
  char tmpfilename[MAX_PATTERN];
  FILE *fp;
  size_t size;
  int i, lim;
  
  //figure out what directory filename is in
  for (i = strlen(filename); i > 0; i--) {
    if (filename[i-1] == '/') {
      break;
    }
  }

  if (!i) {
    tmpfilename[0] = '.';
    tmpfilename[1] = '/';
    i = 2;
  } else {
    strncpy(tmpfilename, filename, i);
  }
  lim = i+6;
  for ( ; i < lim; i++) {
    tmpfilename[i] = 'X';
  }
  tmpfilename[lim] = '\0';

  //create a temporary file in that directory
  lim = mkstemp(tmpfilename);
  if (lim < 0) {
    if (svm_trace) {
      perror("Error opening temporary file");
    }
    fatalerror5("Error opening a temporary file.  Your directory may be too full or some other problem, but this will really mess things up.\n", 
		"", CRM_ENGINE_HERE);
    return 0;
  } else {
    close(lim);
  }

  fp = fopen(tmpfilename, "wb");

  if (!fp) {
    fatalerror5("Error opening a temporary file.  Your directory may be too full or some other problem, but this will really mess things up.\n", 
		"", CRM_ENGINE_HERE);
    return 0;
  }

  size = write_svm_file_fp(blck, fp);

  fclose(fp);

#ifdef SVM_USE_MMAP
  //do the unmap AFTER since blck probably has memory somewhere in that mmap
  crm_force_munmap_filename(filename);
#endif
  
  //delete the old file
  if (unlink(filename)) {
    if (svm_trace) {
      perror("Error deleting out-of-date svm file");
    }
    unlink(tmpfilename);
    return 0;
  }

  //now rename our temporary file to be the old file
  if (rename(tmpfilename, filename)) {
    if (svm_trace) {
      perror("Error renaming temporary file");
    }
    unlink(tmpfilename);
    fatalerror5("Could not copy from the temporary file to the new svm file.  Perhaps you don't have write permissions?  Whatever is going on, we are unlikely to be able to recover from it.", "", CRM_ENGINE_HERE);
    return 0;
  }

  return size;
}


//writes an svm block to a file in binary format
//returns the number of bytes written
//doesn't munmap the file since it doesn't have a file name!!
//frees blck
static size_t write_svm_file_fp(crm_svm_block *blck, FILE *fp) {
  size_t size = MAX_INT_VAL, unused;
  int i, curr_rows = 0, nv = 0, n_new;
  int *vmap;
  Matrix *M = matr_make(0, 0, SPARSE_ARRAY, MATR_COMPACT); 
#ifndef SVM_USE_MMAP
  size_t tmp;
#endif

  if (!blck) {
    fatalerror5("write_svm_file: attempt to write NULL block.", "",
		CRM_ENGINE_HERE);
    return 0;
  }

  if (!fp) {
    nonfatalerror("Trying to write an svm file to a null file pointer.", "");
    return 0;
  }

  if (blck->sol) {
    blck->has_solution = 1;
  } else {
    blck->has_solution = 0;
  }

  if (blck->sol && blck->sol->SV) {
    nv += blck->sol->SV->rows;
  }

  if (blck->oldXy) {
    nv += blck->oldXy->rows;
  }

  if (blck->newXy) {
    nv += blck->newXy->rows;
    n_new = blck->newXy->rows;
  } else {
    n_new = 0;
  }

  while (nv > blck->map_size) {
    //grow the map if we need to
    if (!(blck->map_size)) {
      blck->map_size = 1;
    }
    blck->map_size *= 2;
  }

  vmap = (int *)malloc(sizeof(int)*blck->map_size);  
  
  size = sizeof(char)*fwrite(SVM_FIRST_BITS, 1, SVM_FIRST_NBIT, fp);
  size += sizeof(size_t)*fwrite(&size, sizeof(size_t), 1, fp);
  size += sizeof(size_t)*fwrite(&size, sizeof(size_t), 1, fp);
  size += sizeof(size_t)*fwrite(&size, sizeof(size_t), 1, fp);
  size += sizeof(int)*fwrite(&n_new, sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n_old), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->has_solution), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n0), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n1), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n0f), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n1f), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->map_size), sizeof(int), 1, fp);

  //vector map
  size += sizeof(int)*fwrite(vmap, sizeof(int), blck->map_size, fp);

  if (blck->sol) {
    //write theta
    size += svm_write_theta(blck->sol->theta, fp);
    //write the constants
    size += sizeof(int)*fwrite(&(blck->sol->num_examples), sizeof(int), 1, fp);
    size += sizeof(int)*fwrite(&(blck->sol->max_train_val), sizeof(int), 1, fp);
    //write out the matrix
    size += sizeof(Matrix)*fwrite(blck->sol->SV, sizeof(Matrix), 1, fp);
    for (i = 0; i < blck->sol->SV->rows; i++) {
      vmap[i + curr_rows] = size;
      size += vector_write_bin_fp(blck->sol->SV->data[i], fp);
    }
    curr_rows += blck->sol->SV->rows;
  } else {
    //leave room for the solution
    size += svm_write_theta(NULL, fp);
    size += sizeof(int)*fwrite(&curr_rows, sizeof(int), 1, fp);
    i = SVM_MAX_X_VAL;
    size += sizeof(int)*fwrite(&i, sizeof(int), 1, fp);
    size += sizeof(Matrix)*fwrite(M, sizeof(Matrix), 1, fp);
  }

  //this is where the oldXy matrix is stored
  fseek(fp, SVM_FIRST_NBIT+sizeof(size_t), SEEK_SET);
  unused = fwrite(&size, sizeof(size_t), 1, fp);
  fseek(fp, size, SEEK_SET);
  
  if (blck->oldXy) {
    size += sizeof(Matrix)*fwrite(blck->oldXy, sizeof(Matrix), 1, fp);
    for (i = 0; i < blck->oldXy->rows; i++) {
      vmap[i+curr_rows] = size;
      size += vector_write_bin_fp(blck->oldXy->data[i], fp);
    }
    curr_rows += blck->oldXy->rows;
  } else {
    size += sizeof(Matrix)*fwrite(M, sizeof(Matrix), 1, fp);
  }

  if (blck->newXy && blck->newXy->data) {
    for (i = 0; i < blck->newXy->rows; i++) {
      if (blck->newXy->data[i]) {
	vmap[i+curr_rows] = size;
	size += vector_write_bin_fp(blck->newXy->data[i], fp);
      }
    }
    curr_rows += blck->newXy->rows;
  } 


  //this tells you where the data in the file ends
  fseek(fp, SVM_FIRST_NBIT, SEEK_SET);
#ifdef SVM_USE_MMAP
  unused = fwrite(&size, sizeof(size_t), 1, fp);
#else
  tmp = MAX_INT_VAL;
  unused = fwrite(&tmp, sizeof(size_t), 1, fp);
#endif
  fseek(fp, sizeof(size_t), SEEK_CUR);
  //this tells you the offset to appended vectors
  //so you can check if there *are* new vectors quickly
  unused = fwrite(&size, sizeof(size_t), 1, fp);

  //now we actually have vmap
  //so write it out
  fseek(fp, SVM_FIRST_NBIT + N_OFFSETS_IN_SVM_FILE*sizeof(size_t) + 
	N_CONSTANTS_NOT_IN_BLOCK*sizeof(int) + 
	N_CONSTANTS_IN_SVM_BLOCK*sizeof(int), SEEK_SET);
  unused = fwrite(vmap, sizeof(int), curr_rows, fp);
  free(vmap);

#ifdef SVM_USE_MMAP
  //now leave a nice big hole
  //so we can add lots of nice vectors
  //without changing the file size
  if (SVM_HOLE_FRAC > 0) {
    fseek(fp, 0, SEEK_END);
    vmap = malloc((int)(SVM_HOLE_FRAC*size));
    size += fwrite(vmap, 1, (int)(SVM_HOLE_FRAC*size), fp);
    free(vmap);
  }
#endif

  matr_free(M);
  crm_svm_block_free_data(*blck);
  crm_svm_block_init(blck);
  return size;
}

//writes theta to a file, leaving it room to grow
static size_t svm_write_theta(Vector *theta, FILE *fp) {
  int dec_size = MATR_DEFAULT_VECTOR_SIZE*sizeof(double);
  size_t size = 0, theta_written, theta_size;
  void *filler = NULL;

  if (!fp) {
    if (svm_trace) {
      fprintf(stderr, "svm_write_theta: null file pointer.\n");
    }
    return 0;
  }

  if (theta) {
    theta_size = vector_size(theta);
    while (theta_size >= dec_size) {
      if (!(dec_size)) {
	dec_size = 1;
      }
      dec_size *= 2;
    }
    
    theta_written = vector_write_bin_fp(theta, fp);
  } else {
    theta_written = 0;
  }

  size += theta_written;
  dec_size -= theta_written;
  if (dec_size > 0) {
    filler = malloc(dec_size);
  } else {
    dec_size = 0;
  }
  size += sizeof(int)*fwrite(&dec_size, sizeof(int), 1, fp);
  if (filler) {
    size += fwrite(filler, 1, dec_size, fp);
    free(filler);
  }
  return size;
}

//appends a vector to the svm file to be learned on later without
//reading in the whole file
//frees the vector
static size_t append_vector_to_svm_file(Vector *v, char *filename) {
  FILE *fp;
  crm_svm_block blck;
  int exists = 0;
  long size;

#ifdef SVM_USE_MMAP
  size_t data_ends, vsize;
  int ret;
  void *addr, *last_addr, *new_addr, *st_addr;
  struct stat statbuf;
  
  if (!v) {
    nonfatalerror("Something is wrong with the new input.  I think it is NULL.  I am not trying to append it.", "");
    return 0;
  }

  //do we have space to write this vector without forcing an unmap?
  if (!stat(filename, &statbuf)) {
    if (statbuf.st_size > 0) {
      exists = 1;
      addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE,
			   MAP_SHARED, &size);
      if (addr == MAP_FAILED || size < sizeof(size_t) + SVM_FIRST_NBIT) {
	vector_free(v);
	fatalerror5("Unable to map SVM file in order to append a vector.  Something is very wrong and we are unlikely to be able to recover.  The file is", filename, CRM_ENGINE_HERE);
	return 0;
      }
      st_addr = addr;
      last_addr = st_addr+size;
      if (strncmp(SVM_FIRST_BITS, (char *)addr, strlen(SVM_FIRST_BITS))) {
	nonfatalerror("I think this SVM file is corrupted.  You may want to stop now and rerun this test with an uncorrupted file.  For now, I'm not going to touch it.  The file is", filename);
	crm_munmap_file(st_addr);
	vector_free(v);
	return 0;
      }
      addr += SVM_FIRST_NBIT;
      data_ends = *((size_t *)addr);
      vsize = vector_size(v);
      //no matter what, the data now ends here
      //it's important to mark that
      if (data_ends <= size) {
	*((size_t *)addr) = data_ends + vsize;
      } else {
	*((size_t *)addr) = size + vsize;
      }
      if (data_ends < size && st_addr + data_ends + vsize <= last_addr) {
	//we have room to write the vector
	//so add it
	new_addr = vector_memmove(st_addr + data_ends, v);
	vector_free(v);
	crm_munmap_file(st_addr);
	return vsize;
      }
      //we don't have room to write the vector
      //get rid of the hole
      crm_munmap_file(st_addr);
      if (data_ends < size) {
	ret = truncate(filename, data_ends);
      } else if (data_ends > size) {
	nonfatalerror("I think this SVM file is corrupted.  You may want to stop now and rerun this test with an uncorrupted file.  For now, I'm not going to touch it.  The file is", filename);
	vector_free(v);
	return 0;
      }
    }
  }

#else
  fp = fopen(filename, "rb");
  if (fp) {
    exists = 1;
    fclose(fp);
  }
#endif

  if (!exists) {
    if (svm_trace) {
      fprintf(stderr, "Creating new stat file.\n");
    }
    //the file doesn't exist yet
    //we'll create it!
    //note that leaving this as open for appending instead
    //of writing creates problems.  i'm not sure why.
    fp = fopen(filename, "wb");
    crm_svm_block_init(&blck);
    blck.newXy = matr_make_size(1, v->dim, v->type, v->compact, v->size);
    if (!blck.newXy) {
      nonfatalerror("Attempt to append bad vector to SVM file.", "");
      fclose(fp);
      return 0;
    }
    matr_shallow_row_copy(blck.newXy, 0, v);
    size = write_svm_file_fp(&blck, fp);
    fclose(fp);
    return size;
  }

#ifdef SVM_USE_MMAP
  //force an unmap if it is mapped
  //append this vector to the file
  crm_force_munmap_filename(filename);
#endif
  fp = fopen(filename, "ab");  
  size = vector_write_bin_fp(v, fp);
  vector_free(v);
#ifdef SVM_USE_MMAP
  if (SVM_HOLE_FRAC > 0) {
    if (svm_trace) {
      fprintf(stderr, "Appending hole of size %d to file.\n", 
	      (int)(SVM_HOLE_FRAC*statbuf.st_size));
    }
    new_addr = malloc((int)(SVM_HOLE_FRAC*statbuf.st_size));
    size += fwrite(new_addr, 1, (int)(SVM_HOLE_FRAC*statbuf.st_size), fp);
    free(new_addr);
  }
#endif
  fclose(fp);
  return size;
}



//this function writes the changes that have been made to blck
//to disk
//if addr is NULL, it will fwrite blck to filename 
//if blck was mapped in, it will attempt to write things back into
//memory and 
//if this isn't possible it will force a fwrite the file
//this frees all data associated with blck
static size_t crm_svm_save_changes(crm_svm_block *blck, void *addr,
				   char *filename) {
  size_t old_offset, theta_room, theta_req, size;
  void *curr = addr, *prev, *last_addr;
  crm_svm_block old_block;
  struct stat statbuf;
  int nv = 0, i, *vmap, curr_rows = 0;

#ifndef SVM_USE_MMAP
  return write_svm_file(blck, filename);
#endif

  if (!addr) {
    nonfatalerror("Attempting to save a file to a NULL address.  Probably the original file was corrupted and couldn't be read.  The file is", filename);
    return 0;
  }

  if (stat(filename, &statbuf)) {
    //ok this is really wrong
    fatalerror5("svm save changes: the file you are trying to save to doesn't exist.  This is unrecoverable.  The file is", filename, CRM_ENGINE_HERE);
    return 0;
  }

  if (statbuf.st_size < sizeof(size_t) + SVM_FIRST_NBIT) {
    if (svm_trace) {
      fprintf(stderr, "Writing file because it is waaaay too small.\n");
    }
    return write_svm_file(blck, filename);
  }

  if (strncmp(SVM_FIRST_BITS, (char *)addr, strlen(SVM_FIRST_BITS))) {
    nonfatalerror("The magic string of the file I am trying to save isn't what I think it should be.  This probably indicates that the file is corrupted and I shouldn't touch it so I won't.  The file is", filename);
    return 0;
  }
  
  curr += SVM_FIRST_NBIT;

  size = *((size_t *)curr);
  curr += sizeof(size_t);

  if (size + sizeof(double)*MATR_DEFAULT_VECTOR_SIZE >= statbuf.st_size) {
    //we have no more room to append vectors to this file
    //so write it out now
    //otherwise size won't change
    if (svm_trace) {
      fprintf(stderr, "Writing file to leave a hole at the end.\n");
    }
    return write_svm_file(blck, filename);
  }

  last_addr = addr + size;

  //if we are going to unmap the file, old_offset won't change
  //since oldXy will be in the same place
  old_offset = *((size_t *)curr);
  curr += sizeof(size_t);
  //new_offset, however, will go away because we now have locations
  //for all of the "new vectors".  that we need to do a learn is
  //marked with a non_zero n_new
  *((size_t *)curr) = size;
  curr += sizeof(size_t);

  //make all of the constants correct
  if (blck->sol) {
    blck->has_solution = 1;
  } else {
    blck->has_solution = 0;
  }

  if (blck->sol && blck->sol->SV) {
    nv += blck->sol->SV->rows;
  }

  if (blck->oldXy) {
    nv += blck->oldXy->rows;
  }

  if (blck->newXy) {
    nv += blck->newXy->rows;
  }

  while (nv > blck->map_size) {
    if (!(blck->map_size)) {
      blck->map_size = 1;
    }
    blck->map_size *= 2;
  }


  if (blck->newXy) {
    *((int *)curr) = blck->newXy->rows;
  } else {
    *((int *)curr) = 0;
  }
  curr += sizeof(int);

  old_block.n_old = *((int *)curr);
  *((int *)curr) = blck->n_old;
  curr += sizeof(int);
  old_block.has_solution = *((int *)curr);
  *((int *)curr) = blck->has_solution;
  curr += sizeof(int);
  old_block.n0 = *((int *)curr);
  *((int *)curr) = blck->n0;
  curr += sizeof(int);
  old_block.n1 = *((int *)curr);
  *((int *)curr) = blck->n1;
  curr += sizeof(int);
  old_block.n0f = *((int *)curr);
  *((int *)curr) = blck->n0f;
  curr += sizeof(int);
  old_block.n1f = *((int *)curr);
  *((int *)curr) = blck->n1f;
  curr += sizeof(int);
  old_block.map_size = *((int *)curr);
  *((int *)curr) = blck->map_size;
  curr += sizeof(int);

  if (blck->map_size > old_block.map_size) {
    //we don't have enough room to do a vector map
    //we need to write out the file
    if (svm_trace) {
      fprintf(stderr, "Writing svm  file to grow map size from %d to %d.\n",
	      old_block.map_size, blck->map_size);
    }
    return write_svm_file(blck, filename);
  }

  //this is the map we will fill in
  vmap = curr;

  //do we have room to write theta?
  curr += sizeof(int)*blck->map_size;

  //keep where theta starts
  prev = curr;

  //this is how much room for theta
  if (old_block.has_solution) {
    theta_room = vector_size((Vector *)curr);
  } else {
    theta_room = 0;
  }
  curr += theta_room;
  theta_room += *((int *)curr);
  curr = prev;

  //how much room will theta actually take?
  if (blck->has_solution && blck->sol && blck->sol->theta) {
    theta_req = vector_size(blck->sol->theta);
  } else {
    theta_req = 0;
  }

  if (curr + theta_room > last_addr || theta_room < theta_req) {
    //we don't have enough room in the file to write
    //the decision boundary
    //so we need to use fwrite
    if (svm_trace) {
      fprintf
	(stderr, 
	 "Writing file to grow decision boundary size from %lu to %lu.\n",
	 theta_room, theta_req);
    }
    return write_svm_file(blck, filename);
  }

  //we have enough room to unmap the solution to this file
  //let's do it!

  //write the new solution boundary
  if (blck->has_solution && blck->sol) {
    if (blck->sol->theta) {
      //copy over the decision boundary
      //it is possible that curr and blck->sol->theta
      //overlap if we didn't actually do a learn 
      //so use memmove NOT memcpy
      prev = vector_memmove(curr, blck->sol->theta);
    }
    //leave a marker to let us know how much filler space we have
    *((int *)prev) = theta_room-theta_req;
    //keep the filler!
    curr += theta_room + sizeof(int);
    //write in the solution constants
    if (blck->has_solution && blck->sol) {
      *((int *)curr) = blck->sol->num_examples;
    }
    curr += sizeof(int);
    if (blck->has_solution && blck->sol) {
      *((int *)curr) = blck->sol->max_train_val;
    }
    curr += sizeof(int);

    if (blck->sol->SV) {
      //copy the matrix header
      *((Matrix *)curr) = *(blck->sol->SV);
      //now use the map (remember back where we stored it in vmap?)
      //to record which of the vectors (already somewhere in this chunk
      //of memory) belong to this matrix
      for (i = 0; i < blck->sol->SV->rows; i++) {
	//vmap stores offsets from the beginning of the file
	if (((void *)blck->sol->SV->data[i]) < addr || 
	    ((void *)blck->sol->SV->data[i]) > last_addr) {
	  //oh oh, something is very wrong
	  //give up and write the file
	  if (svm_trace) {
	    fprintf(stderr, "save_changes: somehow a vector is outside the mapped memory.\n");
	  }
	  return write_svm_file(blck, filename);
	}
	vmap[i + curr_rows] = ((void *)blck->sol->SV->data[i]) - addr;
      }
      curr_rows += blck->sol->SV->rows;
    }
  }
      
  if (blck->n_old && blck->oldXy && blck->oldXy->data) {
    curr = addr + old_offset; //note that this shouldn't change!
    *((Matrix *)curr) = *(blck->oldXy);
    for (i = 0; i < blck->oldXy->rows; i++) {
      if (((void *)blck->oldXy->data[i]) < addr || 
	  ((void *)blck->oldXy->data[i]) > last_addr) {
	//whoops
	if (svm_trace) {
	  fprintf(stderr, "save_changes: somehow a vector is outside the mapped memory.\n");
	}
	return write_svm_file(blck, filename);
      }
      vmap[i + curr_rows] = ((void *)blck->oldXy->data[i]) - addr;
    }
    curr_rows += blck->oldXy->rows;
  }

  if (blck->newXy) {
    //newXy isn't saved as a matrix
    //since new vectors come and go all the time
    for (i = 0; i < blck->newXy->rows; i++) {
      if (((void *)blck->newXy->data[i]) < addr || 
	  ((void *)blck->newXy->data[i]) > last_addr) {
	if (svm_trace) {
	  fprintf(stderr, "save_changes: somehow a vector is outside the mapped memory.\n");
	}
	return write_svm_file(blck, filename);
      }
      vmap[i + curr_rows] = ((void *)blck->newXy->data[i]) - addr;
    }
  }

  //whew!  we made it
  crm_svm_block_free_data(*blck);
  crm_svm_block_init(blck);
  crm_munmap_file(addr);
  return size;

}


/***************************SVM BLOCK FUNCTIONS*******************************/

//initializes an svm block
static void crm_svm_block_init(crm_svm_block *blck) {
  blck->sol = NULL;
  blck->newXy = NULL;
  blck->oldXy = NULL;
  blck->n_old = 0;
  blck->has_solution = 0;
  blck->n0 = 0;
  blck->n1 = 0;
  blck->n0f = 0;
  blck->n1f = 0;
  blck->map_size = SVM_DEFAULT_MAP_SIZE;
}

//frees all data associated with a block
static void crm_svm_block_free_data(crm_svm_block blck) {

  if (blck.sol) {
    svm_free_solution(blck.sol);
  }

  if (blck.oldXy) {
    matr_free(blck.oldXy);
  }

  if (blck.newXy) {
    matr_free(blck.newXy);
  }
}

/***************************LEARNING FUNCTIONS********************************/

//does the actual work of learning new examples
static void crm_svm_learn_new_examples(crm_svm_block *blck, int microgroom) {
  int i;
  int inc = 0, offset = 0, n_ex = 0, lim;
  double d;
  PreciseSparseElement *thetaval = NULL;
  VectorIterator vit;
  Vector *row;

  if (!blck->newXy && !blck->sol) {
    nonfatalerror
      ("There are no examples for an SVM to learn on in the file you have supplied.  Note that supplying a non-empty but incorrectly formatted file can cause this warning.", "");
    //reset the block
    crm_svm_block_free_data(*blck);
    crm_svm_block_init(blck);
    return;
  }

  //update n0, n1, n0f, n1f
  if (blck->newXy) {
    for (i = 0; i < blck->newXy->rows; i++) {
      row = matr_get_row(blck->newXy, i);
      if (!row) {
	//this would be weird
	continue;
      }
      vectorit_set_at_beg(&vit, row);
      if (!vectorit_past_end(vit, row)) {
	if (vectorit_curr_val(vit, row) < 0) {
	  //a new example for class 1
	  blck->n1++;
	  blck->n1f += row->nz;
	  if (SVM_ADD_CONSTANT) {
	    blck->n1f--;
	  }
	} else {
	  blck->n0++;
	  blck->n0f += row->nz;
	  if (SVM_ADD_CONSTANT) {
	    blck->n0f--;
	  }
	}
      }
    }
  }

  //actually learn something!
  if (svm_trace) {
    fprintf(stderr, "Calling SVM solve.\n");
  }
  svm_solve(&(blck->newXy), &(blck->sol));

  if (!blck->sol || !blck->sol->theta) {
    nonfatalerror("Unable to solve SVM.  This is likely due to a corrupted SVM statistics file.", "");
    crm_svm_block_free_data(*blck);
    crm_svm_block_init(blck);
    return;
  }

  if (svm_trace) {
    fprintf(stderr, 
	    "Reclassifying all old examples to find extra support vectors.\n");
  }

  if (blck->oldXy) {
    n_ex += blck->oldXy->rows;
    if (microgroom && blck->oldXy->rows >= SVM_GROOM_OLD) {
      thetaval = (PreciseSparseElement *)
	malloc(sizeof(PreciseSparseElement)*blck->oldXy->rows);
    }
    //check the classification of everything in oldXy
    //put anything not classified with high enough margin into sol->SV
    lim = blck->oldXy->rows;
    for (i = 0; i < lim; i++) {
      row = matr_get_row(blck->oldXy, i - offset);
      if (!row) {
	continue;
      }
      d = dot(blck->sol->theta, row);
      if (d <= 0) {
	inc++;
      }
      if (d <= 1+SV_TOLERANCE) {
	matr_shallow_row_copy(blck->sol->SV, blck->sol->SV->rows, row);
	matr_erase_row(blck->oldXy, i - offset);
	offset++;
      } else if (thetaval) {
	thetaval[i-offset].col = i - offset;
	thetaval[i-offset].data = d;
      }
    }

    if (thetaval && blck->oldXy->rows >= SVM_GROOM_OLD) {
      //microgroom
      if (svm_trace) {
	fprintf(stderr, "Microgrooming...\n");
      }
      qsort(thetaval, blck->oldXy->rows, sizeof(PreciseSparseElement), 
	    precise_sparse_element_val_compare);
      //take the top SVM_GROOM_FRAC of this
      qsort(&(thetaval[(int)(blck->oldXy->rows*SVM_GROOM_FRAC)]), 
	    blck->oldXy->rows - (int)(blck->oldXy->rows*SVM_GROOM_FRAC),
	    sizeof(PreciseSparseElement), precise_sparse_element_col_compare);
      lim = blck->oldXy->rows;
      for (i = (int)(blck->oldXy->rows*SVM_GROOM_FRAC); i < lim; i++) {
	matr_remove_row(blck->oldXy, thetaval[i].col);
      }
    }
    if (thetaval) {
      free(thetaval);
    }
    if (!blck->oldXy->rows) {
      matr_free(blck->oldXy);
      blck->oldXy = NULL;
    } 
  }

  if (svm_trace) {
    fprintf(stderr, "Of %d old training examples, we got %d incorrect.  There are now %d support vectors (we added %d).\n", 
	    n_ex, inc, blck->sol->SV->rows, offset);
  }

  //if we have any vectors that weren't support vectors
  //they are now stored in newXy.
  //so copy newXy into oldXy

  if (blck->newXy) {
    matr_append_matr(&(blck->oldXy), blck->newXy);
    matr_free(blck->newXy);
    blck->newXy = NULL;
  }

  //update the counts we keep of the number of rows
  //of oldXy (mostly so we know whether it exists)
  if (blck->oldXy) {
    blck->n_old = blck->oldXy->rows;
  } else {
    blck->n_old = 0;
  }

  //we've solved it!  so we have a solution
  blck->has_solution = 1;
}

/******************************************************************************
 *Use an SVM to learn a classification task.
 *This expects two classes: a class with a +1 label and a class with
 *a -1 label.  These are denoted by the presence or absense of the 
 *CRM_REFUTE label (see the FLAGS section of the comment).
 *For an overview of how the algorithm works, look at the comments in 
 *crm_svm_lib_fncts.c.
 *
 *INPUT: This function is for use with CRM 114 so it takes the
 * canonical arguments:
 * csl: The control block.  Never actually used.
 * apb: The argparse block.  This is passed to vector_tokenize_selector
 *  and I use the flags (see the FLAG section).
 * txtptr: A pointer to the text to classify.
 * txtstart: The text to classify starts at txtptr+txtstart
 * txtlen: number of characters to classify
 *
 *OUTPUT: 0 on success
 *
 *FLAGS: The SVM calls crm_vector_tokenize_selector so uses any flags
 * that that function uses.  For learning, it interprets flags as
 * follows:
 *
 * CRM_REFUTE: If present, this indicates that this text has a -1
 *  label and should be classified as such.  If absent, indicates
 *  that this text has a +1 label.
 *
 * CRM_UNIQUE: If present, CRM_UNIQUE indicates that we should ignore
 *  the number of times we see a feature.  With CRM_UNIQUE, feature
 *  vectors are binary - a 1 in a column indicates that a feature
 *  with that column number was seen once or more.  Without it, features
 *  are integer valued - a number in a column indicates the number of
 *  times that feature was seen in the document.
 *
 * CRM_MICROGROOM: If there are more than SVM_GROOM_OLD (defined in
 *  (crm114_config.h) examples that we have learned on but are
 *  not support vectors, CRM_MICROGROOM will remove the SVM_GROOM_FRAC
 *  (defined in crm11_config.h) of them furthest from the decision
 *  boundary.  CRM_MICROGROOM ONLY runs AFTER an actual learn - ie
 *  we will never microgroom during an APPEND.  In fact, PASSING IN
 *  MICROGROOM WITH APPEND DOES NOTHING.  Also note that the effects
 *  of microgrooming are not obvious until the next time the file is
 *  written using fwrite.  This will actually happen the next time enough
 *  vectors are added 
 *
 * CRM_APPEND: The example will be added to the set of examples but
 *  not yet learned on.  We will learn on this example the next time
 *  a learn without APPEND or ERASE is called or if classify is called.
 *  If you call learn with CRM_APPEND and actual learn will NEVER happen.
 *  All calls to learn with CRM_APPEND will execute very quickly.
 *
 * CRM_FROMSTART: Relearn on every seen (and not microgroomed away) example
 *  instead of using an incremental method.  If CRM_FROMSTART and
 *  CRM_APPEND are both flagged, the FROMSTART learn will be done the
 *  next time there is a learn without APPEND or ERASE or a classify.  If
 *  examples are passed in using CRM_APPEND after CRM_FROMSTART, we will 
 *  also learn those examples whenever we do the FROMSTART learn.
 *
 * CRM_ERASE: Erases the example from the example set.  If this
 *  example was just appended and never learned on or if it is not
 *  in the support vector set of the last solution, this simply erases 
 *  the example from the set of examples.  If the example is a support
 *  vector, we relearn everything from the start including any new examples 
 *  that were passed in using CRM_APPEND and haven't been learned on.  If 
 *  CRM_ERASE and CRM_APPEND are passed in together and a relearn is required, 
 *  the relearn is done the next time learn is called without APPEND or ERASE 
 *  or a classify is called.
 *
 * ALL FLAGS NOT LISTED HERE OR USED IN THE VECTOR_TOKENIZER ARE IGNORED.
 *
 *WHEN WE LEARN:
 *
 * The various flags can seem to interact bizarrely to govern whether a
 * learn actually happens, but, in fact, everything follows three basic rules:
 *
 * 1) WE NEVER LEARN ON CRM_APPEND.
 * 2) IF WE LEARN, WE LEARN ON ALL EXAMPLES PRESENT.
 * 3) WHEN ERASING, WE DO EXACTLY AS MUCH WORK IS REQUIRED TO ERASE THE
 *    EXAMPLE AND NO MORE EXCEPT WHERE THIS CONFLICTS WITH THE FIRST 2 RULES.
 *
 * Therefore, rule 2 says that a FROMSTART, for example, will learn on both 
 * old and new examples.  Likewise rule 2 states that an ERASE that requires 
 * a relearn, will learn on both old and new examples.  An ERASE that DOESN'T 
 * require a relearn, however, is governed by rule 3 and therefore
 * will NOT run a learn on new examples because that is NOT necessary to
 * erase the example.  Rule 1 ensures that passing in CRM_MICROGROOM with 
 * CRM_APPEND does nothing because we only MICROGROOM after a learn and we 
 * NEVER learn on CRM_APPEND.  Etc.
 *
 *FORCING A LEARN:
 *
 * You can force a learn by passing in a NULL txtptr or a txtlen of 0.
 * This will call the svm learn functions EVEN IF there are no new
 * examples.  If the SVM is incorrectly classifying examples it has
 * already seen, forcing a relearn will fix that problem.
 *****************************************************************************/

int crm_svm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
		  long txtstart, long txtlen) {

  char htext[MAX_PATTERN], filename[MAX_PATTERN];
  long i, j;
  unsigned int features[MAX_SVM_FEATURES];
  crm_svm_block blck;
  size_t unused;
  Vector *nex, *row;
  int read_file = 0, do_learn = 1, lim = 0;
  void *addr = NULL;

  if (user_trace) {
    svm_trace = 1;
  }

  if (internal_trace) {
    //this is a "mediumly verbose" setting
    svm_trace = SVM_INTERNAL_TRACE_LEVEL + 1;
  }

  SVM_DEBUG_MODE = svm_trace - 1;
  if (SVM_DEBUG_MODE < 0) {
    SVM_DEBUG_MODE = 0;
  }

  if (svm_trace) {
    fprintf(stderr, "Doing an SVM learn.\n");
  }

  //Get the filename
  //crm_stmt_parser.c
  crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
  crm_nexpandvar(htext, apb->p1len, MAX_PATTERN);

  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;
  htext[j] = '\000';
  strcpy (filename, &htext[i]);

  //set things to NULL that should be null
  crm_svm_block_init(&blck);


  if (txtptr && txtlen > 0) {
    //get the new example
    nex = convert_document(txtptr+txtstart, txtlen, features, apb);

    if (apb->sflags & CRM_ERASE) {
      //find this example and remove all instances of it
      //then do a FROMSTART unless we haven't learned on this
      //example yet
      //requires reading in the whole file
      //load our stat file in
      if (!(addr = map_svm_file(&blck, filename))) {
	nonfatalerror("An error occurred trying to map in the file.  Likely it is corrupted.  Your vector will not be erased.  The file is", filename);
      } else {
	read_file = 1;
      }
      do_learn = 0; //we are erasing, not learning
      if (blck.sol && blck.sol->SV) {
	j = 0;
	lim = blck.sol->SV->rows;
	for (i = 0; i < lim; i++) {
	  row = matr_get_row(blck.sol->SV, i-j);
	  if (!row) {
	    continue;
	  }
	  if (vector_equals(nex, row)) {
	    //support vector
	    //have to start over
	    do_learn = 1;
	    if (!(apb->sflags & CRM_FROMSTART)) {
	      apb->sflags = apb->sflags | CRM_FROMSTART;
	    }
	    matr_remove_row(blck.sol->SV, i-j);
	    if (vector_get(nex, 0) < 0) {
	      blck.n1--;
	      blck.n1f -= nex->nz;
	      if (SVM_ADD_CONSTANT) {
		blck.n1f++;
	      }
	    } else {
	      blck.n0--;
	      blck.n0f -= nex->nz;
	      if (SVM_ADD_CONSTANT) {
		blck.n0f++;
	      }
	    }
	    j++;
	  }
	}
      }
      if (blck.oldXy) {
	j = 0;
	lim = blck.oldXy->rows;
	for (i = 0; i < lim; i++) {
	  row = matr_get_row(blck.oldXy, i-j);
	  if (!row) {
	    continue;
	  }
	  if (vector_equals(nex, row)) {
	    matr_remove_row(blck.oldXy, i-j);
	    j++;
	    if (vector_get(nex, 0) < 0) {
	      blck.n1--;
	      blck.n1f -= nex->nz;
	      if (SVM_ADD_CONSTANT) {
		blck.n1f++;
	      }
	    } else {
	      blck.n0--;
	      blck.n0f -= nex->nz;
	      if (SVM_ADD_CONSTANT) {
		blck.n0f++;
	      }
	    }
	  }
	}

      }
      if (blck.newXy) {
	j = 0;
	lim = blck.newXy->rows;
	for (i = 0; i < lim; i++) {
	  row = matr_get_row(blck.newXy, i-j);
	  if (!row) {
	    continue;
	  }
	  if (vector_equals(nex, row)) {
	    matr_remove_row(blck.newXy, i-j);
	    j++;
	  }
	}
      }
      vector_free(nex);
    } else {
      //add the vector to the new matrix
      append_vector_to_svm_file(nex, filename);
    }
  }

  if (apb->sflags & CRM_FROMSTART) {
    do_learn = 1;
    if (!read_file) {
      if (!(addr = map_svm_file(&blck, filename))) {
	nonfatalerror("An error occurred trying to map in the file.  Likely it is corrupted.  The fromstart learn will have no effect.  The file is", filename);
      } else {
	read_file = 1;
      }
    }
    //copy oldXy into newXy
    if (blck.oldXy) {
      matr_append_matr(&(blck.newXy), blck.oldXy);
      matr_free(blck.oldXy);
      blck.oldXy = NULL;
      blck.n_old = 0;
    }
    //copy the support vectors into newXy
    if (blck.sol) {
      matr_append_matr(&(blck.newXy), blck.sol->SV);
      svm_free_solution(blck.sol);
      blck.sol = NULL;
    }
    blck.n0 = 0;
    blck.n1 = 0;
    blck.n0f = 0;
    blck.n1f = 0;
  }

  if (!(apb->sflags & CRM_APPEND) && do_learn) {
    if (!read_file) {
      if (!(addr = map_svm_file(&blck, filename))) {
	nonfatalerror("An error occurred trying to map in the file.  Either it is corrupted or the only string you have learned on so far is the empty string.  Note that the SVM needs at least one non-empty example to initialize its file.  Whatever is going on, your learn will have no effect.  The file is", filename);
	do_learn = 0;
      } else {
	read_file = 1;
      }
    }
    //do we actually want to do this learn?
    //let's consult smart mode
    if (read_file && svm_smart_mode) {
      //wait until we have a good base of examples to learn
      if (!blck.has_solution && (!blck.newXy || 
				 blck.newXy->rows < SVM_BASE_EXAMPLES)) {
	if (svm_trace) {
	  fprintf(stderr, "Running under smart_mode: postponing learn until we have enough examples.\n");
	}
	do_learn = 0;
      }

      //if we have more than SVM_INCR_FRAC examples we haven't yet
      //learned on, do a fromstart
      if (blck.sol && blck.sol->SV && blck.oldXy && blck.newXy && 
	  blck.newXy->rows >=
	  SVM_INCR_FRAC*(blck.oldXy->rows + blck.sol->SV->rows)) {
	if (svm_trace) {
	  fprintf(stderr, "Running under smart_mode: Doing a fromstart to incorporate new examples.\n");
	}
	matr_append_matr(&(blck.newXy), blck.oldXy);
	matr_free(blck.oldXy);
	blck.oldXy = NULL;
	blck.n_old = 0;
      }
    }
    if (do_learn) {
      crm_svm_learn_new_examples(&blck, apb->sflags & CRM_MICROGROOM);
    }
  }

  if (read_file) {
    //we did something to it!
    //save it
    unused = crm_svm_save_changes(&blck, addr, filename);
  }

  //free everything
  crm_svm_block_free_data(blck);
  return 0;
}

/****************************CLASSIFICATION FUNCTIONS*************************/

/******************************************************************************
 *Use an SVM for a classification task.
 *This expects two classes: a class with a +1 label and a class with
 *a -1 label.  The class with the +1 label is class 0 and the class
 *with the -1 label is class 1.  When learning, class 1 is denoted by
 *passing in the CRM_REFUTE flag.  The classify is considered to FAIL
 *if the example classifies as class 1 (-1 label).  The SVM requires
 *at least one example to do any classification, although really you should
 *give it at least one from each class.  If classify is called without any
 *examples to learn on at all, it will classify the example as class 0, but
 *it will also print out an error.
 *
 *If classify is called and there are new examples that haven't been learned
 *on or a FROMSTART learn that hasn't been done, this function will do that
 *BEFORE classifying.  in other words:
 *
 *CLASSIFY WILL DO A LEARN BEFORE CLASSIFYING IF NECESSARY.  IT WILL NOT STORE
 *THAT LEARN BECAUSE IT HAS NO WRITE PRIVILEGES TO THE FILE.
 *
 *INPUT: This function is for use with CRM 114 so it takes the
 * canonical arguments:
 * csl: The control block.  Used to skip if classify fails.
 * apb: The argparse block.  This is passed to vector_tokenize_selector
 *  and I use the flags (see the FLAG section).
 * txtptr: A pointer to the text to classify.
 * txtstart: The text to classify starts at txtptr+txtstart
 * txtlen: number of characters to classify
 *
 *OUTPUT: return is 0 on success
 * The text output (stored in out_var) is formatted as follows:
 *
 * LINE 1: CLASSIFY succeeds/fails success probability: # pR: #
 *  (note that success probability is high for success and low for failure)
 * LINE 2: Best match to class #0/1 probability: # pR: #
 *  (probability >= 0.5 since this is the best matching class.)
 * LINE 3: Total features in input file: #
 * LINE 4: #0 (label +1): documents: #, features: #, prob: #, pR #
 * LINE 5: #1 (label -1): documents: #, features: #, prob: #, pR #
 *  (prob is high for match class, low else.  pR is positive for match class.)
 *
 * I've picked a random method for calculating probability and pR.  Thinking
 * about it, there may be literature for figuring out the probability at
 * least.  Anyone who wants to do that, be my guest.  For now, I've found
 * a function that stays between 0 and 1 and called it good.  Specifically,
 * if theta is the decision boundary and x is the example to classify:
 *
 *       prob(class = 0) = 0.5 + 0.5*tanh(theta dot x)
 *       pR = sgn(theta dot x)*(pow(11, fabs(theta dot x)) - 1)
 *
 *FLAGS: The SVM calls crm_vector_tokenize_selector so uses any flags
 * that that function uses.  For classifying, it interprets flags as
 * follows:
 *
 * CRM_REFUTE: Returns the OPPOSITE CLASS.  In other words, if this should
 *  classify as class 1, it now classifies as class 0.  I don't know why
 *  you would want to do this, but you should be aware it happens.
 *
 * CRM_UNIQUE: If present, CRM_UNIQUE indicates that we should ignore
 *  the number of times we see a feature.  With CRM_UNIQUE, feature
 *  vectors are binary - a 1 in a column indicates that a feature
 *  with that column number was seen once or more.  Without it, features
 *  are integer valued - a number in a column indicates the number of
 *  times that feature was seen in the document.  If you used CRM_UNIQUE
 *  to learn, use CRM_UNIQUE to classify! (duh)
 *
 * CRM_MICROGROOM: If classify does a learn, it will MICROGROOM.  See the
 *  comment to learn for how microgroom works.
 *
 * ALL FLAGS NOT LISTED HERE OR USED IN THE VECTOR_TOKENIZER ARE IGNORED.
 * INCLUDING FLAGS USED FOR LEARN!
 *****************************************************************************/

int crm_svm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
		  long txtstart, long txtlen) {

  char htext[MAX_PATTERN], filename[MAX_PATTERN], out_var[MAX_PATTERN];
  long i, j, out_var_len = 0;
  unsigned int features[MAX_SVM_FEATURES], out_pos = 0;
  Vector *nex, *theta;
  double dottheta = 0;
  int class, sgn, nz;
  crm_svm_block blck;
  void *addr = NULL;

  if (user_trace) {
    svm_trace = 1;
  }

  if (internal_trace) {
    //this is a "mediumly verbose" setting
    svm_trace = SVM_INTERNAL_TRACE_LEVEL + 1;
  }
  
  SVM_DEBUG_MODE = svm_trace - 1;

  if (SVM_DEBUG_MODE < 0) {
    SVM_DEBUG_MODE = 0;
  }

  if (svm_trace) {
    fprintf(stderr, "Doing an SVM classify.\n");
  }

  crm_svm_block_init(&blck);

  //Get the filename (we only have one)
  //crm_stmt_parser.c
  crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
  crm_nexpandvar(htext, apb->p1len, MAX_PATTERN);
  
  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;
  htext[j] = '\000';
  strcpy (filename, &htext[i]);
  
  //Get the output variable name
  if (apb->p2start) {
    crm_get_pgm_arg(out_var, MAX_PATTERN, apb->p2start, apb->p2len);
    out_var_len = crm_nexpandvar(out_var, apb->p2len, MAX_PATTERN);
  }

  //do we have new vectors to learn on?
  if (has_new_vectors(filename)) {
    //we use read so that we don't make changes to the file
    //also doing a learn when you can't benefit from it is stupid
    //so we don't do that in smart mode
    if (!svm_smart_mode && read_svm_file(&blck, filename)) {
      crm_svm_learn_new_examples(&blck, 0);
    }
    if (blck.sol) {
      theta = blck.sol->theta;
    } else {
      crm_svm_block_free_data(blck);
      crm_svm_block_init(&blck);
      theta = NULL;
    }
  } else {
    svm_get_meta_data(filename, &blck);
    theta = get_theta_from_svm_file(filename, &addr);
  }

  //get the new example
  nex = convert_document(txtptr+txtstart, txtlen, features, apb);

  //classify it
  if (theta) {
    dottheta = dot(nex, theta);
    if (blck.sol) {
      crm_svm_block_free_data(blck);
    } else {
      vector_free(theta);
    }
  } else {
    if (!svm_smart_mode) {
      nonfatalerror
	("Nothing was learned before asking SVM for a classification. I am trying to classify from the file", filename);
    }
    dottheta = 0;
  }

  if (addr) {
    crm_munmap_file(addr);
  }
  
  if (svm_trace) {
    fprintf(stderr, 
	    "The dot product of the example and decision boundary is %lf\n", 
	    dottheta);
  }
  if (dottheta < 0) {
    class = 1;
    sgn = -1;
  } else {
    class = 0;
    sgn = 1;
  }

  if (fabs(dottheta) > 6/log10(11)) {
    nonfatalerror("The pR values here are HUGE.  One fix for this is to redo things with the unique flag set.  This is especially true if you are also using the string flag.", "");
    dottheta = sgn*6/log10(11);
  }

  if (apb->p2start) {
    //annnnnnd... write it all back out
    if (!class) {
      //these are very arbitrary units of measurement
      //i picked tanh because... it's a function with a middle at 0
      //and nice asymptotic properties near 1
      //yay!
      out_pos += sprintf
	(outbuf + out_pos, "CLASSIFY succeeds");
    } else {
      out_pos += sprintf(outbuf + out_pos, "CLASSIFY fails");
    }
    out_pos += sprintf(outbuf + out_pos, " success probability: %f pR: %6.4f\n",
		       0.5 + 0.5*tanh(dottheta), 
		       sgn*(pow(11, fabs(dottheta))-1));
    out_pos += sprintf(outbuf + out_pos,
		       "Best match to class #%d prob: %6.4f pR: %6.4f   \n",
		       class, 0.5 + 0.5*tanh(fabs(dottheta)), 
		       pow(11, fabs(dottheta))-1);
    nz = nex->nz;
    if (SVM_ADD_CONSTANT) {
      nz--;
    }
    out_pos += sprintf(outbuf + out_pos,
		       "Total features in input file: %d\n", nz);
    out_pos += sprintf
      (outbuf + out_pos,
       "#0 (label +1): documents: %d, features: %d, prob: %3.2e, pR: %6.2f\n",
       blck.n0, blck.n0f, 0.5 + 0.5*tanh(dottheta), 
       sgn*(pow(11, fabs(dottheta)) - 1));
    out_pos += sprintf
      (outbuf + out_pos,
       "#1 (label -1): documents: %d, features: %d, prob: %3.2e, pR: %6.2f\n",
       blck.n1, blck.n1f, 0.5 - 0.5*tanh(dottheta), 
       -1*sgn*(pow(11, fabs(dottheta))-1));
       
    if (out_var_len) {
      crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_pos);
    }
  }

  vector_free(nex);

  if (class) {
    //classifies out-of-class
    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
    csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
    return 0;
  }

  return 0;
}


/****************************SAMPLE MAINS*************************************/

//#define MAKE_PREC_RECALL_GRAPHS
#ifdef MAKE_PREC_RECALL_GRAPHS

//    the command line argv
char **prog_argv;

//    the auxilliary input buffer (for WINDOW input)
char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
char *inbuf;
char *outbuf;
char *tempbuf;

//classify spam
//this generates svm_time.txt and svm_err.txt
int main(int argc, char **argv) {

  //CSL_CELL csl;
  ARGPARSE_BLOCK apb;
  char *txtptr = NULL;
  long txtstart, txtlen;
  FILE *fp, *slfp, *hlfp;
  size_t size;
  Matrix *X;
  char *gdir, *glist, *bdir, buf[256];
  crm_svm_block blck;
  int i = 0, start, read_good = 0, ng = 0, nb = 0, label, curr_pt = 0;
  unsigned int features[MAX_SVM_FEATURES];
  Vector *v;
  double d;
  int errpts[20], deth = 0, deta = 0, ah = 0;
  FILE *err_file = fopen("svm_err.txt", "w");

  csl = (CSL_CELL *)malloc(sizeof(CSL_CELL));
  apb.p1start = (char *)malloc(sizeof(char)*MAX_PATTERN);
  strcpy(apb.p1start, "");
  apb.p1len = strlen(apb.p1start);
  apb.a1start = buf;
  apb.a1len = 0;
  apb.p2start = NULL;
  apb.p2len = 0;
  apb.p3start = buf;
  apb.p3len = 0;
  apb.b1start = buf;
  apb.b1len = 0;
  apb.s1start = buf;
  apb.s1len = 0;
  apb.s2start = buf;
  apb.s2len = 0;
  
  gdir = argv[1];
  bdir = argv[2];

  unlink(apb.p1start);

  data_window_size = DEFAULT_DATA_WINDOW;
  printf("data_window_size = %d\n", data_window_size);
  outbuf = (char *)malloc(sizeof(char)*data_window_size);
  prog_argv = (char **)malloc(sizeof(char *));
  prog_argv[0] = (char *)malloc(sizeof(char)*MAX_PATTERN);
  //list of files in ham folder
  strcpy(buf, gdir);
  start = strlen(buf); 
  strcpy(&(buf[start]), "/list.txt\0");
  start = strlen(buf);
  printf("start = %d\n", start);
  hlfp = fopen(buf, "r");

  //list of files in spam folder
  strcpy(buf, bdir);
  start = strlen(buf);
  strcpy(&(buf[start]), "/list.txt\0");
  start = strlen(buf);
  slfp = fopen(buf, "r");

  crm_svm_block_init(&blck);
  i = 0;
  while (fscanf(hlfp, "%s", buf) != EOF) {
    ng++;
  }
  while (fscanf(slfp, "%s", buf) != EOF) {
    nb++;
  }
  printf("ng = %d, nb = %d\n", ng, nb);
  
  errpts[0] = 125;
  curr_pt = 0;
  while (errpts[curr_pt] < nb + ng) {
    errpts[curr_pt+1] = 2*errpts[curr_pt];
    curr_pt++;
  }
  errpts[curr_pt-1] = nb + ng;
  curr_pt = 0;

  rewind(hlfp);
  rewind(slfp);
  while (!feof(hlfp) || !feof(slfp)) {
    v = NULL;
    if ((read_good && !feof(hlfp)) || feof(slfp)) {
      ah++;
      strcpy(buf, gdir);
      start = strlen(buf);
      strcpy(&buf[start], "/");
      start = strlen(buf);
      if (fscanf(hlfp, "%s", &(buf[start])) == EOF) {
	continue;
      }
      read_good++;
      if (read_good >= ng/nb + 1) {
	read_good = 0;
      }
      apb.sflags = CRM_UNIQUE;
      label = 1;
    } else if (!feof(slfp)) {
      strcpy(buf, bdir);
      start = strlen(buf);
      strcpy(&buf[start], "/");
      start = strlen(buf);
      if (fscanf(slfp, "%s", &(buf[start])) == EOF) {
	continue;
      }
      start = strlen(buf);
      apb.sflags = CRM_REFUTE | CRM_UNIQUE;
      read_good = 1;
      label = -1;
    }
    printf("Reading %s i = %d\n", buf, i);
    fp = fopen(buf, "r");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    txtptr = (char *)realloc(txtptr, size);
    size = fread(txtptr, 1, size, fp);
    fclose(fp);

    //do a classify
    //d = crm_svm_classify(csl, &apb, txtptr, 0, size);
    
    v = convert_document(txtptr, size, features, &apb);
    if (blck.sol) {
      d = dot(blck.sol->theta, v);
    } else {
      d = 0;
    }
    printf("d = %f\n", d);
    if (d < 1) {
      //do a learn
      //crm_svm_learn(csl, &apb, txtptr, 0, size);
      if (!blck.newXy) {
      blck.newXy = matr_make_size(1, v->dim, v->type, v->compact, v->size);
      }
      matr_shallow_row_copy(blck.newXy, blck.newXy->rows-1, v);
      crm_svm_learn_new_examples(&blck, 0);
    }
    
    if (d > 0 && label > 0) {
      //a correct ham detection!
      deth++;
      deta++;
    }
    //could be less than or equal if d is actual dot
    //right now it is return from classify
    if (d < 0 && label < 0) {
      //an incorrect ham detection
      deta++;
    }

    i++;
    if (i == errpts[curr_pt]) {
      //record this
      fprintf(err_file, "%d %d %d %d\n", i, deth, deta, ah);
      deth = 0;
      deta = 0;
      ah = 0;
      curr_pt++;
    }


  }
  fclose(hlfp);
  fclose(slfp);
  fclose(err_file);

  free(outbuf);
  free(csl);
  free(apb.p1start);
  free(txtptr);
  crm_svm_block_free_data(blck);

  return 0;
}

#endif

//#define SVM_NON_TEXT
#ifdef SVM_NON_TEXT
//and yet another main to test taking non-text data

int main(int argc, char **argv) {
  Vector *theta;
  int currarg = 1, i;
  char *opt;
  FILE *thout = NULL;
  SVM_Solution *sol = NULL;
  Matrix *Xy;

  if (argc < 2) {
    fprintf(stderr,
	    "Usage: linear_svm [options] example_file [solution_file].\n");
    exit(1);
  }

  opt = argv[currarg];
  DEBUG_MODE = 0;
  while (currarg < argc && opt[0] == '-') {
    switch(opt[1]) {
    case 'v':
      DEBUG_MODE = atoi(&(opt[2]));
      break;
    case 't':
      currarg++;
      thout = fopen(argv[currarg], "w");
      if (!thout) {
	fprintf(stderr, "Bad theta output file name: %s.  Writing to stdout.\n",
		argv[currarg]);
	thout = stdout;
      }
      break;
    case 'p':
      thout = stdout;
      break;
    case 's':
      currarg++;
      sol = read_solution(argv[currarg]);
      break;
    default:
      fprintf(stderr, "Options are:\n");
      fprintf(stderr, "\t-v#: Verbosity level.\n");
      fprintf(stderr, "\t-t filename: Theta ascii output file.\n");
      fprintf(stderr, "\t-p filename: Print theta to screen.\n");
      fprintf(stderr, "\t-s filename: Starting solution file.\n");
      break;
    }
    currarg++;
    opt = argv[currarg];
  }

  printf("DEBUG_MODE = %d\n", DEBUG_MODE);

  if (currarg >= argc) {
    fprintf(stderr, "Error: No input file or no output file.\n");
    fprintf(stderr,
	    "Usage: linear_svm [options] example_file [solution_file].\n");
    if (thout != stdout) {
      fclose(thout);
    }
    exit(1);
  }

  Xy = matr_read_bin(argv[currarg]);
  currarg++;

  //if (sol) {
  //solve(NULL, &sol);
  //} else {
    solve(&Xy, &sol);
    //}
  matr_free(Xy);

  theta = sol->theta;

  if (thout == stdout) {
    //otherwise this just gets in the way
    fprintf(thout, "There are %d SVs\n", sol->SV->rows);
    fprintf(thout, "Solution using Cutting Planes is\n");
  }

  if (thout) {
    vector_write_sp_fp(theta,thout);
  }

  if (currarg < argc) {
    //write out the solution
    write_solution(sol, argv[currarg]);
  }

  free_solution(sol);

  return 0;
}

#endif
