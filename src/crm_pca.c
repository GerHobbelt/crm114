//	crm_pca.c - Principal Component Analysis

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


#include "crm_pca.h"

//static function declarations
static Vector *convert_document(char *text, long text_len,
				unsigned int *features,
				ARGPARSE_BLOCK *apb);
static int compare_features(const void *a, const void *b);
static void *pca_map_file(crm_pca_block *blck, char *filename);
static void pca_get_meta_data(char *filename, crm_pca_block *blck);
static int has_new_vectors(char *filename);
static PCA_Solution *get_solution_from_pca_file(char *filename, void **st_addr);

//these are around for the times when we want to read in the file
//without overwriting what we have in memory (ie in a learn during
// a classify)
static int pca_read_file(crm_pca_block *blck, char *filename);
static int pca_read_file_fp(crm_pca_block *blck, FILE *fp);

//these always use fwrite.  they have to be called sometimes even
//though we try to use mmap to grow the file size.
static size_t pca_write_file(crm_pca_block *blck, char *filename);
static size_t pca_write_file_fp(crm_pca_block *blck, FILE *fp);
static size_t pca_write_theta(Vector *theta, FILE *fp);

//this writes to the mmap'd file in memory if there's room or
//forces an unmap and calls append
static size_t append_vector_to_pca_file(Vector *v, char *filename);

//this writes everything back to disk using fwrite or unmap as
//appropriate.  if the file was read, it always uses fwrite.  if
//the file was mapped in, it tries to alter that memory to have the
//correct new values in it and, if it can't, fwrites it.
static size_t crm_pca_save_changes(crm_pca_block *blck, void *addr, 
				   char *filename);

static void crm_pca_block_init(crm_pca_block *blck);
static void crm_pca_block_free_data(crm_pca_block blck);
static void crm_pca_learn_new_examples(crm_pca_block *blck, int microgroom);

int pca_trace = 0;

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
 * feature vector adds in the class magnitude if it is non-zero using the
 * CRM_REFUTE flag to set the sign.
 *
 *WARNINGS:
 *1) You need to free the returned vector (using vector_free)
 *   once you are done with it.
 *2) The returned vector is NOT just a vector of the features.  We do
 *   PCA-specific manipulations to it, specifically, multiplying the 
 *   features by their label and adding a column if PCA_ADD_CONSTANT
 *   is set.
 *******************************************************************/
static Vector *convert_document(char *text, long text_len,
				unsigned int *features,
				ARGPARSE_BLOCK *apb) {
  long next_offset;
  long n_features, i;
  int class, entry = 1;
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
     MAX_PCA_FEATURES - 1,        //  max number of hashes
     &n_features,             // how many hashes we actually got
     &next_offset);           // where to start again for more hashes

  if (apb->sflags & CRM_REFUTE) {
    //this is a negative example
    if (PCA_CLASS_MAG) {
      class = -1*PCA_CLASS_MAG;
    }
  } else {
    class = PCA_CLASS_MAG;
  }



  if (!n_features) {
    if (class) {
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
  
  if (class) {
    //insert the class mag
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
	vectorit_insert(&vit, features[i], vectorit_curr_val(vit, v) + entry,
			 v);
      }
    } else {
      vectorit_insert(&vit, features[i], entry, v);
    }
  }

  //make v only take up the amount of memory it should
  if (v && v->type == SPARSE_ARRAY) {
    expanding_array_trim(v->data.sparray);
  }
  return v;
}


/**************************PCA FILE FUNCTIONS*********************************/

/******************************************************************************
 *
 *The PCA file is a binary file formatted as follows:
 *
 *PCA_FIRST_NBIT bytes: A string or whatever you want defined in 
 * PCA_FIRST_BITS.  This isn't a checksum since we don't want to have to read
 * in the whole file every time in order to verify it - it's simply a stored
 * value (or even a string) that all PCA stat files have as the first few
 * bytes to identify them.  While there is as much error checking as I can do
 * in this code, non-PCA binary files can create seg faults by mapping two
 * vector headers into the same space so that changing one changes part of
 * another.  There is almost nothing I can do about that, so, to eliminate
 * that problem as far as I can, we have a little "magic string" in front.
 *
 *N_OFFSETS_IN_PCA_FILE size_t's:
 *
 * size_t size: The offset until the end of the actual data stored in the file.
 *  We leave a large hole at the end of the file so we can append to it without
 *  having to uncache it from memory.  This is the offset to the beginning of
 *  the hole.  When reading the file in, we do not need to read past this
 *  offset since the rest is garbage.  This changes each time we append a
 *  vector.
 *
 *N_CONSTANTS_NOT_IN_BLOCK ints: don't actually have any :)
 *
 *N_CONSTANTS_IN_PCA_BLOCK ints:
 *
 * int has_new: 1 if there are new vectors, 0 else
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
 *
 *PRINCIPLE COMPONENT:
 *
 * theta: the principle component written as a vector
 *
 * int fill: the amount of filler we leave to allow the principle component to
 *  to grow without having to grow the file.
 *
 * void fill: a "hole" allowing the decision vector to grow in size in new
 *  learns.
 *
 * double mudottheta: the decision point
 *
 *EXAMPLE VECTORS:
 *
 * Each new vector is formatted as a vector (ie we don't keep the matrix header
 * - this makes appending easy).
 *
 *The file is formatted this way to make the following actions quick both using
 * fread/fwrite and mmap/munmap:
 *
 * Finding if the file has a solution: requires a seek to has_solution and a
 *  read of that value.
 * 
 * Finding the principle if it exists: requires a sequential fread
 *  of N_CONSTANTS_IN_PCA_BLOCK, a seek to DECISION BOUNDARY, reading in the
 *  vector stored there.
 *
 * Querying if there are unlearned on vectors: requries a seek has_new and a
 *  read of that value.
 *
 * Appending a vector: requires mapping in the file, reading in size and
 *   has_new, updating has_new,  and seeking to point size in the file.  
 *   if there is room, writes the vector there.  else forcibly munmaps the 
 *   file and opens it for appending.  creates a file if there isn't one.
 *****************************************************************************/


static void *pca_map_file(crm_pca_block *blck, char *filename) {
  struct stat statbuf;
  long act_size;
  void *addr, *last_addr, *st_addr;
  Vector *v;
  size_t size;
  int fill;

  if (stat(filename, &statbuf)) {
    nonfatalerror("Attempt to read from nonexistent pca file", filename);
    return NULL;
  }

  if (!blck) {
    //this really shouldn't happen
    fatalerror5("pca_map_file: bad crm_pca_block pointer.", "",
		CRM_ENGINE_HERE);
    return NULL;
  }

  crm_pca_block_init(blck);

  addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, &act_size);

  if (addr == MAP_FAILED) {
    nonfatalerror("Attempt to map pca file failed.  The file was", filename);
    return NULL;
  }

  st_addr = addr;
  
  if (act_size < sizeof(size_t) + PCA_FIRST_NBIT) {
    nonfatalerror
      ("Attempt to read from corrupted pca file.  It is much too small.", "");
    crm_munmap_file(st_addr);
    return NULL;
  }

  if (strncmp(PCA_FIRST_BITS, (char *)st_addr, strlen(PCA_FIRST_BITS))) {
    nonfatalerror
      ("Attempt to map from corrupted PCA file.  The header is incorrect.", "");
    crm_munmap_file(st_addr);
    return NULL;
  }

  addr += PCA_FIRST_NBIT;

  //this is where the data actually ends
  size = *((size_t*)addr);
  if (size > act_size) {
    //corrupted file
    nonfatalerror("Attempt to read from corrupted pca file.  It thinks it has a larger length than it does.  The file is", filename);
    crm_munmap_file(st_addr);
    return NULL;
  }
  addr += sizeof(size_t);
  last_addr = st_addr + size; //last address that contains good data

  if (size < N_CONSTANTS_IN_PCA_BLOCK*sizeof(int)) {
    //this is isn't a good file
    nonfatalerror("Attempt to read from corrupted pca file.  It is somewhat too small.", filename);
    crm_munmap_file(st_addr);
    return NULL;
  }

  blck->has_new = *((int*)(addr));      //do we have unlearned-on examples?
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

  if (blck->has_solution) {
    blck->sol = (PCA_Solution *)malloc(sizeof(PCA_Solution));
    if (!blck->sol) {
      nonfatalerror("Unable to malloc space for solution struct.  Could this be a corrupted file?", "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      crm_munmap_file(st_addr);
    }
    //read in the solution
    blck->sol->theta = vector_map(&addr, last_addr); //decision boundary
    if (addr + sizeof(int) > last_addr) {
      nonfatalerror
	("Attempt to map from bad pca file.  It can't fit its solution.", "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    fill = *((int *)addr); //hole to grow pca
    addr += sizeof(int);
    if (!blck->sol->theta || addr + fill + sizeof(double) > last_addr) {
      nonfatalerror
	("Attempt to map from bad pca file.  It can't fit in the solution.", 
	 "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      crm_munmap_file(st_addr);
      return NULL;
    }
    addr += fill;
    blck->sol->mudottheta = *((double *)addr);
    addr += sizeof(double);
  } else {
    fill = *((int *)addr);
    addr += sizeof(int);
    addr += fill + sizeof(double);
  }

  //example vectors!
  
  if (addr < last_addr) {
    v = vector_map(&addr, last_addr);
    if (v) {
      if (!blck->X) {
	blck->X = matr_make_size(0, v->dim, v->type, v->compact, v->size);
      }
      if (!blck->X) {
	nonfatalerror("Attempt to map from bad pca file.  A very new vector had an unrecognized type.", "");
	crm_pca_block_free_data(*blck);
	crm_pca_block_init(blck);
	crm_munmap_file(st_addr);
	return NULL;
      }
      matr_shallow_row_copy(blck->X, blck->X->rows, v);
      while (addr < last_addr) {
	v = vector_map(&addr, last_addr);
	if (v && v->dim) {
	  matr_shallow_row_copy(blck->X, blck->X->rows, v);
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
static void pca_get_meta_data(char *filename, crm_pca_block *blck) {
  void *addr, *last_addr, *st_addr;
  struct stat statbuf;
  size_t size;
  long act_size;


  if (stat(filename, &statbuf)) {
    //heck, we don't even have a file!
    nonfatalerror
      ("You are trying to use a PCA to classify from the nonexistant file",
       filename);
    if (blck) {
      blck->has_new = 0;
      blck->has_solution = 0;
      blck->n0 = 0;
      blck->n1 = 0;
      blck->n0f = 0;
      blck->n1f = 0;
    } else {
      fatalerror5("pca_get_meta_data: bad crm_pca_block pointer.", "",
		  CRM_ENGINE_HERE);
    }
    return;
  }

  if (!blck) {
    fatalerror5("pca_get_meta_data: bad crm_pca_block pointer.", "",
		CRM_ENGINE_HERE);
    return;
  }

  //just always do PROT_READ | PROT_WRITE so that if it's cached we get it
  addr = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE, 
		       MAP_SHARED,
		       &act_size);
  if (addr == MAP_FAILED || act_size < sizeof(size_t) + PCA_FIRST_NBIT) {
    fatalerror5("Could not map PCA file to get meta data.  Something is very wrong and I doubt we can recover.  The file is", filename, CRM_ENGINE_HERE);
    if (addr != MAP_FAILED) {
      crm_munmap_file(addr);
    }
    return;
  }

  st_addr = addr;

  if (strncmp(PCA_FIRST_BITS, (char *)addr, strlen(PCA_FIRST_BITS))) {
    nonfatalerror("This pca file is corrupted.  The file is", filename);
    blck->has_new = 0;
    blck->has_solution = 0;
    blck->n0 = 0;
    blck->n1 = 0;
    blck->n0f = 0;
    blck->n1f = 0;
    crm_munmap_file(st_addr);
    return;
  }

  addr += PCA_FIRST_NBIT;
  size = *((size_t *)addr); //actual size (rest is garbage hole)
  last_addr = st_addr + size;

  if (size > act_size || addr + N_OFFSETS_IN_PCA_FILE*sizeof(size_t) + 
      (N_CONSTANTS_IN_PCA_BLOCK + N_CONSTANTS_NOT_IN_BLOCK)*sizeof(int) 
      > last_addr) {
    nonfatalerror("This pca file is corrupted.  The file is", filename);
    blck->has_new = 0;
    blck->has_solution = 0;
    blck->n0 = 0;
    blck->n1 = 0;
    blck->n0f = 0;
    blck->n1f = 0;
    crm_munmap_file(st_addr);
    return;
  }

  addr += N_OFFSETS_IN_PCA_FILE*sizeof(size_t) + 
    N_CONSTANTS_NOT_IN_BLOCK*sizeof(int);
  blck->has_new = *((int *)addr);      //Are there un-learned on examples?
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

  crm_munmap_file(st_addr);

}

//returns 1 if the file has vectors that have been appended but not yet
//learned on
//returns 0 else
static int has_new_vectors(char *filename) {
  void *addr, *last_addr, *st_addr;
  size_t size;
  int *data, ret;
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

  if (addr == MAP_FAILED || act_size < sizeof(size_t) + PCA_FIRST_NBIT) {
    nonfatalerror("There was a problem mapping the pca file in while checking for new vectors.  I am going to assume there are no new vectors.  The file was",
		  filename);
    if (addr != MAP_FAILED) {
      crm_munmap_file(addr);
    }
    return 0;
  }

  st_addr = addr;

  if (strncmp(PCA_FIRST_BITS, (char *)addr, strlen(PCA_FIRST_BITS))) {
    nonfatalerror("The PCA file is corrupted.  I am going to assume it contains no new examples.  The file is", filename);
    crm_munmap_file(st_addr);
    return 0;
  }

  addr += PCA_FIRST_NBIT;

  size = *((size_t *)addr); //actual amount of good data
  last_addr = st_addr + size;

  if (size > act_size || addr + N_OFFSETS_IN_PCA_FILE*sizeof(size_t) +
      (N_CONSTANTS_IN_PCA_BLOCK + N_CONSTANTS_NOT_IN_BLOCK)*sizeof(int) > 
      last_addr) {
    nonfatalerror("There was a problem mapping the pca file in while checking for new vectors.  I am going to assume there are no new vectors.  The file was",
		  filename);
    crm_munmap_file(st_addr);
    return 0;
  }

  addr += N_OFFSETS_IN_PCA_FILE*sizeof(size_t) + 
    N_CONSTANTS_NOT_IN_BLOCK*sizeof(int);
  
  data = (int *)addr;
  ret = data[HAS_NEW_INDEX];

  crm_munmap_file(st_addr);

  return ret;
}


//returns the decision boundary from an pca file
//we map the decision boundary from the file so you must
// FREE THE DECISION BOUNDARY returned by the function
// MUNMAP THE FILE returned pass-by-reference in *addr
static PCA_Solution *get_solution_from_pca_file(char *filename, 
						void **st_addr) {
  PCA_Solution *sol;
  void *last_addr, *addr;
  size_t size;
  int *hs, fill;
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

  if (addr == MAP_FAILED || act_size < sizeof(size_t) + PCA_FIRST_NBIT) {
    nonfatalerror("Attempt to map pca file while getting decision boundary failed.  The file was", filename);
    if (addr != MAP_FAILED) {
      crm_munmap_file(addr);
    }
    *st_addr = NULL;
    return NULL;
  }

  *st_addr = addr;
  if (strncmp(PCA_FIRST_BITS, (char *)addr, strlen(PCA_FIRST_BITS))) {
    nonfatalerror("Attempt to read decision boundary from a corrupt PCA file.  The file was", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    return NULL;
  }

  addr += PCA_FIRST_NBIT;
  size = *((size_t *)addr);
  last_addr = *st_addr + size;

  if (size > act_size || addr + N_OFFSETS_IN_PCA_FILE*sizeof(size_t) + 
      (N_CONSTANTS_NOT_IN_BLOCK+N_CONSTANTS_IN_PCA_BLOCK)*sizeof(int) 
      > last_addr) {
    nonfatalerror("Attempt to map pca file while getting decision boundary failed.  The file was", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    return NULL;
  }

  addr += N_OFFSETS_IN_PCA_FILE*sizeof(size_t) + 
    N_CONSTANTS_NOT_IN_BLOCK*sizeof(int);
  hs = (int *)addr;
  addr += sizeof(int)*N_CONSTANTS_IN_PCA_BLOCK;

  if (addr > last_addr || !hs[HAS_SOLUTION_INDEX]) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain a PCA solution or is corrupted.  The file is", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    return NULL;
  }

  sol = (PCA_Solution *)malloc(sizeof(PCA_Solution));
  sol->theta = vector_map(&addr, last_addr);
  if (addr +sizeof(int) > last_addr) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain a PCA solution or is corrupted.  The file is", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    pca_free_solution(sol);
  }
  fill = *((int *)addr);
  addr += sizeof(int);
  if (addr + fill +sizeof(double) > last_addr) {
    nonfatalerror("You are trying to read a decision boundary from a file that doesn't contain a PCA solution or is corrupted.  The file is", filename);
    crm_munmap_file(*st_addr);
    *st_addr = NULL;
    pca_free_solution(sol);
  }
  
  addr += fill;
  sol->mudottheta = *((double *)addr);
  
  return sol;
}


//functions used to read in the file
//when we need to do a learn in classify.
static int pca_read_file(crm_pca_block *blck, char *filename) {
 FILE *fp = fopen(filename, "rb");
 int ret;

 if (!fp) {
   nonfatalerror("Attempt to read from nonexistent pca file", filename);
   return 0;
 }

 ret = pca_read_file_fp(blck, fp);

 fclose(fp);

 return ret;
}

//reads a binary pca block from a file
//returns 0 on failure
static int pca_read_file_fp(crm_pca_block *blck, FILE *fp) {
  size_t amount_read, size;
  Vector *v;
  int fill;
  char firstbits[strlen(PCA_FIRST_BITS)];

  if (!blck) {
    //this really shouldn't happen
    fatalerror5("read_pca_file_fp: bad crm_pca_block pointer.", "",
		CRM_ENGINE_HERE);
    return 0;
  }


  if (!fp) {
    nonfatalerror("Attempt to read pca from bad file pointer.", "");
    return 0;
  }

  crm_pca_block_free_data(*blck);
  crm_pca_block_init(blck);
  
  amount_read = fread(firstbits, 1, PCA_FIRST_NBIT, fp);
  if (strncmp(PCA_FIRST_BITS, firstbits, strlen(PCA_FIRST_BITS))) {
    nonfatalerror("This pca file is corrupted.  I cannot read it.", "");
    return 0;
  }

  amount_read = fread(&size, sizeof(size_t), 1, fp);
  amount_read = fread(&(blck->has_new), sizeof(int), 1, fp);
  amount_read += fread(&(blck->has_solution), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n0), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n1), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n0f), sizeof(int), 1, fp);
  amount_read += fread(&(blck->n1f), sizeof(int), 1, fp);

  if ((amount_read < N_CONSTANTS_IN_PCA_BLOCK) ||
      ftell(fp) > size) {
    nonfatalerror("Attempt to read from bad pca file", "");
    crm_pca_block_init(blck);
    return 0;
  }

  //read in solution
  if (blck->has_solution) {
    blck->sol = (PCA_Solution *)malloc(sizeof(PCA_Solution));
    blck->sol->theta = vector_read_bin_fp(fp);
    amount_read = fread(&fill, sizeof(int), 1, fp);
    fseek(fp, fill, SEEK_CUR);
    if (!blck->sol->theta || !amount_read || feof(fp) || 
	ftell(fp) > size) {
      //die!
      nonfatalerror("Attempt to read from bad pca file.",  "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      return 0;
    }
    amount_read = fread(&(blck->sol->mudottheta), sizeof(double), 1, fp);
    if (!amount_read) {
      //die!
      nonfatalerror("Attempt to read from bad pca file.",  "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      return 0;
    }
  } else {
    amount_read = fread(&fill, sizeof(int), 1, fp);
    fseek(fp, fill + sizeof(double), SEEK_CUR);
    if (!amount_read || feof(fp) || ftell(fp) >= size) {
      nonfatalerror("Attempt to read from bad SVM file.", "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      return 0;
    }
  }
   
  //read in new vectors
  if (!feof(fp) && ftell(fp) < size) {
    v = vector_read_bin_fp(fp);
    if (v && v->dim) {
      if (!(blck->X)) {
	blck->X = matr_make_size(0, v->dim, v->type, v->compact, v->size);
      }
      if (!blck->X) {
	nonfatalerror("Attempt to map from bad pca file.", "");
	crm_pca_block_free_data(*blck);
	crm_pca_block_init(blck);
	return 0;
      }
      matr_shallow_row_copy(blck->X, blck->X->rows, v);
      while (!feof(fp) && ftell(fp) < size) {
	v = vector_read_bin_fp(fp);
	if (v && v->dim) {
	  matr_shallow_row_copy(blck->X, blck->X->rows, v);
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

  return 1;
}


static size_t pca_write_file(crm_pca_block *blck, char *filename) {
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
    if (pca_trace) {
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

  size = pca_write_file_fp(blck, fp);

  fclose(fp);

  //do the unmap AFTER since blck probably has memory somewhere in that mmap
  crm_force_munmap_filename(filename);
  
  //delete the old file
  if (unlink(filename)) {
    if (pca_trace) {
      perror("Error deleting out-of-date pca file");
    }
    unlink(tmpfilename);
    return 0;
  }

  //now rename our temporary file to be the old file
  if (rename(tmpfilename, filename)) {
    if (pca_trace) {
      perror("Error renaming temporary file");
    }
    unlink(tmpfilename);
    fatalerror5("Could not copy from the temporary file to the new pca file.  Perhaps you don't have write permissions?  Whatever is going on, we are unlikely to be able to recover from it.", "", CRM_ENGINE_HERE);
    return 0;
  }

  return size;
}


//writes an pca block to a file in binary format
//returns the number of bytes written
//doesn't munmap the file since it doesn't have a file name!!
//frees blck
static size_t pca_write_file_fp(crm_pca_block *blck, FILE *fp) {
  size_t size = MAX_INT_VAL, unused;
  int i;
  Matrix *M = matr_make(0, 0, SPARSE_ARRAY, MATR_COMPACT); 
  void *hole;
  double d;

  if (!blck) {
    fatalerror5("pca_write_file: attempt to write NULL block.", "",
		CRM_ENGINE_HERE);
    return 0;
  }

  if (!fp) {
    nonfatalerror("Trying to write a pca file to a null file pointer.", "");
    return 0;
  }

  if (blck->sol && blck->sol->theta) {
    blck->has_solution = 1;
  } else {
    blck->has_solution = 0;
  }

  size = sizeof(char)*fwrite(PCA_FIRST_BITS, 1, PCA_FIRST_NBIT, fp);
  size += sizeof(size_t)*fwrite(&size, sizeof(size_t), 1, fp);
  size += sizeof(int)*fwrite(&(blck->has_new), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->has_solution), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n0), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n1), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n0f), sizeof(int), 1, fp);
  size += sizeof(int)*fwrite(&(blck->n1f), sizeof(int), 1, fp);

  //write the principle component and the fill
  //write the principle component dot the mean vector
  if (blck->sol) {
    size += pca_write_theta(blck->sol->theta, fp);
    size += sizeof(double)*fwrite(&(blck->sol->mudottheta), sizeof(double), 1, 
				  fp);
  } else {
    //leave room
    size += pca_write_theta(NULL, fp);
    d = 0.0;
    size += sizeof(double)*fwrite(&d, sizeof(double), 1, fp);
  }

  //now write out the example vectors
  if (blck->X) {
    for (i = 0; i < blck->X->rows; i++) {
      if (blck->X->data[i]) {
	size += vector_write_bin_fp(blck->X->data[i], fp);
      }
    }
  }

  //this tells you where the data in the file ends
  fseek(fp, PCA_FIRST_NBIT, SEEK_SET);

  //this tells you the offset to appended vectors
  //so you can check if there *are* new vectors quickly
  unused = fwrite(&size, sizeof(size_t), 1, fp);

  //now leave a nice big hole
  //so we can add lots of nice vectors
  //without changing the file size
  if (PCA_HOLE_FRAC > 0) {
    fseek(fp, 0, SEEK_END);
    hole = malloc((int)(PCA_HOLE_FRAC*size));
    size += fwrite(hole, 1, (int)(PCA_HOLE_FRAC*size), fp);
    free(hole);
  }

  matr_free(M);
  crm_pca_block_free_data(*blck);
  crm_pca_block_init(blck);
  return size;
}


//writes theta to a file, leaving it room to grow
static size_t pca_write_theta(Vector *theta, FILE *fp) {
  int dec_size = MATR_DEFAULT_VECTOR_SIZE*sizeof(double);
  size_t size = 0, theta_written, theta_size;
  void *filler = NULL;

  if (!fp) {
    if (pca_trace) {
      fprintf(stderr, "pca_write_theta: null file pointer.\n");
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
      
//appends a vector to the pca file to be learned on later without
//reading in the whole file
//frees the vector
static size_t append_vector_to_pca_file(Vector *v, char *filename) {
  FILE *fp;
  crm_pca_block blck;
  int exists = 0;
  long size;
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
      if (addr == MAP_FAILED || size < sizeof(size_t) + sizeof(int) + 
	  PCA_FIRST_NBIT) {
	vector_free(v);
	fatalerror5("Unable to map PCA file in order to append a vector.  Something is very wrong and we are unlikely to be able to recover.  The file is", filename, CRM_ENGINE_HERE);
	return 0;
      }
      st_addr = addr;
      last_addr = st_addr+size;
      if (strncmp(PCA_FIRST_BITS, (char *)addr, strlen(PCA_FIRST_BITS))) {
	nonfatalerror("I think this PCA file is corrupted.  You may want to stop now and rerun this test with an uncorrupted file.  For now, I'm not going to touch it.  The file is", filename);
	crm_munmap_file(st_addr);
	vector_free(v);
	return 0;
      }
      addr += PCA_FIRST_NBIT;
      data_ends = *((size_t *)addr);
      vsize = vector_size(v);
      //no matter what, the data now ends here
      //it's important to mark that
      if (data_ends <= size) {
	*((size_t *)addr) = data_ends + vsize;
      } else {
	*((size_t *)addr) = size + vsize;
      }
      addr += sizeof(size_t);
      //now note that we have new vectors that haven't been learned on
      *((int *)addr) = 1;
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
	nonfatalerror("I think this PCA file is corrupted.  You may want to stop now and rerun this test with an uncorrupted file.  For now, I'm not going to touch it.  The file is", filename);
	vector_free(v);
	return 0;
      }
    }
  }

  if (!exists) {
    if (pca_trace) {
      fprintf(stderr, "Creating new stat file.\n");
    }
    //the file doesn't exist yet
    //we'll create it!
    //note that leaving this as open for appending instead
    //of writing creates problems.  i'm not sure why.
    fp = fopen(filename, "wb");
    crm_pca_block_init(&blck);
    blck.has_new = 1;
    blck.X = matr_make_size(1, v->dim, v->type, v->compact, v->size);
    if (!blck.X) {
      nonfatalerror("Attempt to append bad vector to PCA file.", "");
      fclose(fp);
      return 0;
    }
    matr_shallow_row_copy(blck.X, 0, v);
    size = pca_write_file_fp(&blck, fp);
    fclose(fp);
    return size;
  }

  //force an unmap if it is mapped
  //append this vector to the file
  crm_force_munmap_filename(filename);
  fp = fopen(filename, "ab");  
  size = vector_write_bin_fp(v, fp);
  vector_free(v);

  if (PCA_HOLE_FRAC > 0) {
    if (pca_trace) {
      fprintf(stderr, "Appending hole of size %d to file.\n", 
	      (int)(PCA_HOLE_FRAC*statbuf.st_size));
    }
    new_addr = malloc((int)(PCA_HOLE_FRAC*statbuf.st_size));
    size += fwrite(new_addr, 1, (int)(PCA_HOLE_FRAC*statbuf.st_size), fp);
    free(new_addr);
  }
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
static size_t crm_pca_save_changes(crm_pca_block *blck, void *addr,
				   char *filename) {
  size_t theta_room, theta_req, size;
  void *curr = addr, *prev, *last_addr;
  crm_pca_block old_block;
  struct stat statbuf;

  if (!addr) {
    nonfatalerror("Attempting to save a file to a NULL address.  Probably the original file was corrupted and couldn't be read.  The file is", filename);
    return 0;
  }

  if (stat(filename, &statbuf)) {
    //ok this is really wrong
    fatalerror5("pca save changes: the file you are trying to save to doesn't exist.  This is unrecoverable.  The file is", filename, CRM_ENGINE_HERE);
    return 0;
  }

  if (statbuf.st_size < sizeof(size_t) + PCA_FIRST_NBIT) {
    if (pca_trace) {
      fprintf(stderr, "Writing file because it is waaaay too small.\n");
    }
    return pca_write_file(blck, filename);
  }

  if (strncmp(PCA_FIRST_BITS, (char *)addr, strlen(PCA_FIRST_BITS))) {
    nonfatalerror("The magic string of the file I am trying to save isn't what I think it should be.  This probably indicates that the file is corrupted and I shouldn't touch it so I won't.  The file is", filename);
    return 0;
  }
  
  curr += PCA_FIRST_NBIT;

  size = *((size_t *)curr);
  curr += sizeof(size_t);

  if (size + sizeof(double)*MATR_DEFAULT_VECTOR_SIZE >= statbuf.st_size) {
    //we have no more room to append vectors to this file
    //so write it out now
    //otherwise size won't change
    if (pca_trace) {
      fprintf(stderr, "Writing file to leave a hole at the end.\n");
    }
    return pca_write_file(blck, filename);
  }

  last_addr = addr + size;

  //make all of the constants correct
  if (blck->sol && blck->sol->theta) {
    blck->has_solution = 1;
  } else {
    blck->has_solution = 0;
  }

  old_block.has_new = *((int *)curr);
  *((int *)curr) = blck->has_new;
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
    if (pca_trace) {
      fprintf
	(stderr, 
	 "Writing file to grow PC size from %lu to %lu.\n",
	 theta_room, theta_req);
    }
    return pca_write_file(blck, filename);
  }

  //we have enough room to unmap the solution to this file
  //let's do it!

  //write the new solution boundary
  if (blck->has_solution && blck->sol) {
    //copy over the decision boundary
    //it is possible that curr and blck->sol->theta
    //overlap if we didn't actually do a learn 
    //so use memmove NOT memcpy
    prev = vector_memmove(curr, blck->sol->theta);
  }
  //leave a marker to let us know how much filler space we have
  *((int *)prev) = theta_room-theta_req;
  curr += theta_room + sizeof(int);
  if (blck->has_solution && blck->sol) {
    *((double *)curr) = blck->sol->mudottheta;
  }
  curr += sizeof(double);

  //and that's all folks!
  crm_pca_block_free_data(*blck);
  crm_pca_block_init(blck);
  crm_munmap_file(addr);
  return size;

}


/***************************PCA BLOCK FUNCTIONS*******************************/

//initializes an pca block
static void crm_pca_block_init(crm_pca_block *blck) {
  blck->sol = NULL;
  blck->X = NULL;
  blck->has_new = 0;
  blck->has_solution = 0;
  blck->n0 = 0;
  blck->n1 = 0;
  blck->n0f = 0;
  blck->n1f = 0;
}

//frees all data associated with a block
static void crm_pca_block_free_data(crm_pca_block blck) {

  if (blck.sol) {
    pca_free_solution(blck.sol);
  }
  if (blck.X) {
    matr_free(blck.X);
  }
}

/***************************LEARNING FUNCTIONS********************************/

//does the actual work of learning new examples
static void crm_pca_learn_new_examples(crm_pca_block *blck, int microgroom) {
  int i, inc, loop_it, pinc, ninc, sgn, lim;
  VectorIterator vit;
  Vector *row;
  double frac_inc, d, val, offset, back, front, ratio, last_offset, *dt;
  PreciseSparseElement *thetaval = NULL;

  if (!blck->X) {
    nonfatalerror
      ("There are no examples for a PCA to learn on in the file you have supplied.  Note that supplying a non-empty but incorrectly formatted file can cause this warning.", "");
    //reset the block
    crm_pca_block_free_data(*blck);
    crm_pca_block_init(blck);
    return;
  }

  //update n0, n1, n0f, n1f
  blck->n0 = 0;
  blck->n1 = 0;
  blck->n0f = 0;
  blck->n1f = 0;
  for (i = 0; i < blck->X->rows; i++) {
    row = matr_get_row(blck->X, i);
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
	if (PCA_CLASS_MAG) {
	    blck->n1f--;
	}
      } else {
	blck->n0++;
	blck->n0f += row->nz;
	if (PCA_CLASS_MAG) {
	  blck->n0f--;
	}
      }
    }
  }
  dt = (double *)malloc(sizeof(double)*blck->X->rows);


  //actually learn something!
  if (pca_trace) {
    fprintf(stderr, "Calling PCA solve.\n");
  }
  
  frac_inc = PCA_REDO_FRAC + 1;
  
  loop_it = 0;
  inc = 0;
  pinc = 0;
  ninc = 0;
  while (frac_inc >= PCA_REDO_FRAC && loop_it < PCA_MAX_REDOS) {
    pca_solve((blck->X), &(blck->sol));
    
    if (!blck->sol) {
      nonfatalerror("Unable to solve PCA.  This is likely due to a corrupted PCA statistics file.", "");
      crm_pca_block_free_data(*blck);
      crm_pca_block_init(blck);
      free(dt);
      return;
    }
    
    inc = 0;
    pinc = 0;
    ninc = 0;
    if (PCA_CLASS_MAG && blck->n0 > 0 && blck->n1 > 0) {
      //check to see if we have class mag set high enough
      //it's set high enough if we correctly classify everything
      //(around a 0 decision point) USING the first element
      d = vector_get(blck->sol->theta, 0);
      if (d < 0) {
	vector_multiply(blck->sol->theta, -1, blck->sol->theta);
	blck->sol->mudottheta *= -1;
      }
      for (i = 0; i < blck->X->rows; i++) {
	dt[i] = dot(matr_get_row(blck->X, i), blck->sol->theta);
	val = matr_get(blck->X, i, 0);
	if (dt[i]*val <= 0) {
	  inc++;
	}

	//get rid of the influence of the first element
	//now we can use this dt[i] to find the correct
	//decision point later
	dt[i] -= vector_get(blck->sol->theta, 0)*val;
	if (dt[i] <= 0 && val > 0) {
	  pinc++;
	} else if (dt[i] >= 0 && val < 0) {
	  ninc++;
	}
      }
    }
    frac_inc = inc/(double)blck->X->rows;
    if (frac_inc >= PCA_REDO_FRAC) {
      for (i = 0; i < blck->X->rows; i++) {
	matr_set(blck->X, i, 0, 2*matr_get(blck->X, i, 0));
      }
      pca_free_solution(blck->sol);
      blck->sol = NULL;
      if (pca_trace) {
	fprintf(stderr, "The fraction of wrong classifications was %lf.  Repeating with class mag = %lf\n", frac_inc, matr_get(blck->X, 0, 0));
      }
      loop_it++;
    }
  }

  if (!blck->sol) {
    nonfatalerror("Unable to solve PCA.  This is likely due to a corrupted PCA statistics file.", "");
    crm_pca_block_free_data(*blck);
    crm_pca_block_init(blck);
    free(dt);
    return;
  }

  offset = 0;

  if (PCA_CLASS_MAG) {
    if (loop_it) {
      //we increased the class mags - set them back to the initial value
      for (i = 0; i < blck->X->rows; i++) {
	d = matr_get(blck->X, i, 0);
	if (d > 0) {
	  matr_set(blck->X, i, 0, PCA_CLASS_MAG);
	} else {
	  matr_set(blck->X, i, 0, -PCA_CLASS_MAG);
	}
      }
    }

    //calculate decision point
    //if number of negative examples = number of positive examples,
    //this point is xbardotp.  however, it turns out that if there
    //is a skewed distribution, the point moves.  i feel like there
    //should be a theoretical way of knowing where this point is since
    //we know it for a non-skewed distribution, but i can't seem to find
    //it.  so... we do this as a binary search - we are trying to make the
    //number of positive and negative mistakes the same

    //figure out initial direction (n <? p)
    if (blck->n1 > 0) {
      ratio = blck->n0/(double)blck->n1;//ratio of positive examples to negative
    } else {
      ratio = 0;
    }
    inc = ninc+pinc;

    if ((int)(ratio*ninc + 0.5) < pinc) {
      //we are getting more positive examples wrong than negative
      //ones - we should decrease the offset
      sgn = -1;
    } else {
      sgn = 1;
    }

    offset = 0;
    //one point of the binary search is zero - we need the other
    //far point.  just go out in big jumps until we find it
    while ((sgn < 0 && (int)(ratio*ninc + 0.5) < pinc) ||
	   (sgn > 0 && (int)(ratio*ninc + 0.5) > pinc)) {
      offset += sgn;
      ninc = 0;
      pinc = 0;
      for (i = 0; i < blck->X->rows; i++) {
	val = matr_get(blck->X, i, 0);
	if ((dt[i] - offset)*val <= 0) {
	  if (val < 0) {
	    ninc++;
	  } else {
	    pinc++;
	  }
	}
      }
    }

    //now do the search
    //our boundaries on the binary search are 0 and offset
    if (offset > 0) {
      front = 0;
      back = offset;
    } else {
      front = offset;
      back = 0;
    }
    last_offset = offset + 1;
    while ((int)(ratio*ninc + 0.5) != pinc && front < back &&
	   last_offset != offset) {
      last_offset = offset;
      offset = (front + back)/2.0;
      ninc = 0;
      pinc = 0;
      for (i = 0; i < blck->X->rows; i++) {
	val = matr_get(blck->X, i, 0);
	if ((dt[i] - offset)*val <= 0) {
	  if (val < 0) {
	    ninc++;
	  } else {
	    pinc++;
	  }
	}
      }
      if ((int)(ratio*ninc + 0.5) < pinc) {
	//offset should get smaller
	//ie back should move closer to front
	back = offset;
      } else if ((int)(ratio*ninc + 0.5) > pinc) {
	front = offset;
      }
      if (pca_trace) {
	fprintf(stderr, "searching for decision point: current point = %lf pinc = %d ninc = %d ratio = %lf\n", offset, pinc, ninc, ratio);
      }
    }
    inc = pinc+ninc;
    //offset is now the decision point
    blck->sol->mudottheta = offset;

    if (pca_trace) {
      fprintf(stderr, "found decision point: %lf pinc = %d ninc = %d ratio = %lf\n", offset, pinc, ninc, ratio);
    }
  }  

  if (pca_trace) {
    fprintf(stderr, "Of %d examples, we classified %d incorrectly.\n",
	    blck->X->rows, inc);
  }

  //microgroom
  if (microgroom && blck->X->rows >= PCA_GROOM_OLD) {
    if (pca_trace) {
      fprintf(stderr, "Microgrooming...\n");
    }
    thetaval = 
      (PreciseSparseElement *)
      malloc(sizeof(PreciseSparseElement)*blck->X->rows);
    for (i = 0; i < blck->X->rows; i++) {
      thetaval[i].data = dt[i] - offset;
      if (matr_get(blck->X, i, 0) < 0) {
	thetaval[i].data = -thetaval[i].data;
      }
      thetaval[i].col = i;
    }

    //sort based on the value
    qsort(thetaval, blck->X->rows, sizeof(PreciseSparseElement),
	  precise_sparse_element_val_compare);
    //get rid of the top PCA_GROOM_FRAC
    qsort(&(thetaval[(int)(blck->X->rows*PCA_GROOM_FRAC)]), 
	  blck->X->rows - (int)(blck->X->rows*PCA_GROOM_FRAC),
	  sizeof(PreciseSparseElement), precise_sparse_element_col_compare);
    lim = blck->X->rows;
    for (i = (int)(blck->X->rows*PCA_GROOM_FRAC); i < lim; i++) {
      matr_remove_row(blck->X, thetaval[i].col);
    }
    free(thetaval);
  }

  free(dt);

  //we've learned all new examples
  blck->has_new = 0;

  //we've solved it!  so we have a solution
  blck->has_solution = 1;
}

/******************************************************************************
 *Use a PCA to learn a classification task.
 *This expects two classes: a class with a +1 label and a class with
 *a -1 label.  These are denoted by the presence or absense of the 
 *CRM_REFUTE label (see the FLAGS section of the comment).
 *For an overview of how the algorithm works, look at the comments in 
 *crm_pca_lib_fncts.c.
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
 *FLAGS: The PCA calls crm_vector_tokenize_selector so uses any flags
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
 * CRM_MICROGROOM: If there are more than PCA_GROOM_OLD (defined in
 *  (crm114_config.h) examples, CRM_MICROGROOM will remove the PCA_GROOM_FRAC
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
 *****************************************************************************/

int crm_pca_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
		  long txtstart, long txtlen) {

  char htext[MAX_PATTERN], filename[MAX_PATTERN];
  long i, j;
  unsigned int features[MAX_PCA_FEATURES];
  crm_pca_block blck;
  size_t unused;
  Vector *nex, *row;
  int read_file = 0, do_learn = 1, lim = 0;
  void *addr = NULL;

  if (user_trace) {
    pca_trace = 1;
  }

  if (internal_trace) {
    //this is a "mediumly verbose" setting
    pca_trace = PCA_INTERNAL_TRACE_LEVEL + 1;
  }

  PCA_DEBUG_MODE = pca_trace - 1;
  if (PCA_DEBUG_MODE < 0) {
    PCA_DEBUG_MODE = 0;
  }

  if (pca_trace) {
    fprintf(stderr, "Doing a PCA learn.\n");
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
  crm_pca_block_init(&blck);


  if (txtptr && txtlen > 0) {
    //get the new example
    nex = convert_document(txtptr+txtstart, txtlen, features, apb);

    if (apb->sflags & CRM_ERASE) {
      //find this example and remove all instances of it
      //then do a FROMSTART unless we haven't learned on this
      //example yet
      //requires reading in the whole file
      //load our stat file in
      if (!(addr = pca_map_file(&blck, filename))) {
	nonfatalerror("An error occurred trying to map in the file.  Likely it is corrupted.  Your vector will not be erased.  The file is", filename);
      } else {
	read_file = 1;
      }
      do_learn = 0; //we are erasing, not learning
      j = 0;
      lim = blck.X->rows;
      for (i = 0; i < lim; i++) {
	row = matr_get_row(blck.X, i-j);
	if (!row) {
	  continue;
	}
	if (vector_equals(nex, row)) {
	  //have to start over
	  do_learn = 1;
	  if (!(apb->sflags & CRM_FROMSTART)) {
	    apb->sflags = apb->sflags | CRM_FROMSTART;
	  }
	  matr_remove_row(blck.X, i-j);
	  j++;
	  if (vector_get(nex, 0) < 0) {
	    blck.n1--;
	    blck.n1f -= nex->nz;
	    if (PCA_CLASS_MAG) {
	      blck.n1f++;
	    }
	  } else {
	    blck.n0--;
	    blck.n0f -= nex->nz;
	    if (PCA_CLASS_MAG) {
	      blck.n0f++;
	    }
	  }
	}
      }
      vector_free(nex);
    } else {
      //add the vector to the new matrix
      append_vector_to_pca_file(nex, filename);
    }
  }

  if (apb->sflags & CRM_FROMSTART) {
    do_learn = 1;
    if (!read_file) {
      if (!(addr = pca_map_file(&blck, filename))) {
	nonfatalerror("An error occurred trying to map in the file.  Likely it is corrupted.  The fromstart learn will have no effect.  The file is", filename);
      } else {
	read_file = 1;
      }
    }
    //get rid of the old solution
    pca_free_solution(blck.sol);
    blck.sol = NULL;

    //reset the constants
    blck.n0 = 0;
    blck.n1 = 0;
    blck.n0f = 0;
    blck.n1f = 0;
  }

  if (!(apb->sflags & CRM_APPEND) && do_learn) {
    if (!read_file) {
      if (!(addr = pca_map_file(&blck, filename))) {
	nonfatalerror("An error occurred trying to map in the file.  Likely it is corrupted.  Your learn will have no effect.  The file is", filename);
	do_learn = 0;
      } else {
	read_file = 1;
      }
    }

    if (do_learn) {
      crm_pca_learn_new_examples(&blck, apb->sflags & CRM_MICROGROOM);
    }
  }

  if (read_file) {
    //we did something to it!
    //save it
    unused = crm_pca_save_changes(&blck, addr, filename);
  }

  //free everything
  crm_pca_block_free_data(blck);
  return 0;
}

/****************************CLASSIFICATION FUNCTIONS*************************/

/******************************************************************************
 *Use a PCA for a classification task.
 *This expects two classes: a class with a +1 label and a class with
 *a -1 label.  The class with the +1 label is class 0 and the class
 *with the -1 label is class 1.  When learning, class 1 is denoted by
 *passing in the CRM_REFUTE flag.  The classify is considered to FAIL
 *if the example classifies as class 1 (-1 label).  The PCA requires
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
 *       prob(class = 0) = 0.5 + 0.5*tanh(theta dot x - mudottheta)
 *       pR = (theta dot x  - mudottheta)*10
 *
 *FLAGS: The PCA calls crm_vector_tokenize_selector so uses any flags
 * that that function uses.  For classifying, it interprets flags as
 * follows:
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

int crm_pca_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
		  long txtstart, long txtlen) {

  char htext[MAX_PATTERN], filename[MAX_PATTERN], out_var[MAX_PATTERN];
  long i, j, out_var_len = 0;
  unsigned int features[MAX_PCA_FEATURES], out_pos = 0;
  Vector *nex;
  double dottheta = 0;
  int class, sgn, nz;
  crm_pca_block blck;
  void *addr = NULL;

  if (user_trace) {
    pca_trace = 1;
  }

  if (internal_trace) {
    //this is a "mediumly verbose" setting
    pca_trace = PCA_INTERNAL_TRACE_LEVEL + 1;
  }
  
  PCA_DEBUG_MODE = pca_trace - 1;

  if (PCA_DEBUG_MODE < 0) {
    PCA_DEBUG_MODE = 0;
  }

  if (pca_trace) {
    fprintf(stderr, "Doing a PCA classify.\n");
  }

  crm_pca_block_init(&blck);

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
    if (pca_read_file(&blck, filename)) {
      crm_pca_learn_new_examples(&blck, 0);
    }
  } else {
    pca_get_meta_data(filename, &blck);
    blck.sol = get_solution_from_pca_file(filename, &addr);
  }

  //get the new example
  nex = convert_document(txtptr+txtstart, txtlen, features, apb);

  if (PCA_CLASS_MAG) {
    //we're classifying.  we don't want a class mag running around.
    vector_set(nex, 0, 0);
  }

  //classify it
  if (blck.sol && blck.sol->theta) {
    //this is biased towards the negative not sure why
    dottheta = dot(nex, blck.sol->theta) - blck.sol->mudottheta;
    crm_pca_block_free_data(blck);
  } else {
    nonfatalerror
      ("Nothing was learned before asking PCA for a classification. I am trying to classify from the file", filename);
    dottheta = 0;
  }

  if (addr) {
    crm_munmap_file(addr);
  }
  
  if (dottheta < 0) {
    class = 1;
    sgn = -1;
  } else {
    class = 0;
    sgn = 1;
  }

  if (fabs(dottheta) > 100000) {
    nonfatalerror("The pR values here are HUGE.  One fix for this is to redo things with the unique flag set.  This is especially true if you are also using the string flag.", "");
    dottheta = sgn*9999.9;
  }
  
  if (pca_trace) {
    fprintf
      (stderr, 
       "The dot product of the decision boundary and the example is %lf\n",
       dottheta);
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
		       dottheta*1000.0);
    //note: this next pR is always positive (or zero)
    out_pos += sprintf(outbuf + out_pos,
		       "Best match to class #%d prob: %6.4f pR: %6.4f   \n",
		       class, 0.5 + 0.5*tanh(fabs(dottheta)), 
		       sgn*dottheta*1000.0);
    nz = nex->nz;
    if (PCA_CLASS_MAG) {
      nz--;
    }
    out_pos += sprintf(outbuf + out_pos,
		       "Total features in input file: %d\n", nz);
    //these following pR's always have opposite signs from each other
    out_pos += sprintf
      (outbuf + out_pos,
       "#0 (label +1): documents: %d, features: %d, prob: %3.2e, pR: %6.2f\n",
       blck.n0, blck.n0f, 0.5 + 0.5*tanh(dottheta), 
       dottheta*1000.0);
    out_pos += sprintf
      (outbuf + out_pos,
       "#1 (label -1): documents: %d, features: %d, prob: %3.2e, pR: %6.2f\n",
       blck.n1, blck.n1f, 0.5 - 0.5*tanh(dottheta), 
       -1*dottheta*1000.0);
       
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
