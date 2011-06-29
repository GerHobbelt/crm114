//  crm_svm.c  - version v1.0
//
//  Copyright 2001-2006  William S. Yerazunis, all rights reserved.
//  
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.  
//
///////////////////////////////////////////////////////////////////////////
//
//    This code is originally copyright and owned by William
//    S. Yerazunis.  In return for addition of significant derivative
//    work, Yimin Wu is hereby granted a full unlimited license to use
//    this code, includng license to relicense under other licenses.
//
///////////////////////////////////////////////////////////////////////


//  Other licenses may be negotiated; contact the 
//  author for details.  
//
//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"



/* [i_a]
//    the command line argc, argv
extern int prog_argc;
extern char **prog_argv;

//    the auxilliary input buffer (for WINDOW input)
extern char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used 
//    wherever needed.  These are sized to the same size as the data window.
extern char *inbuf;
extern char *outbuf;
extern char *tempbuf;
*/


/* [i_a]
//    The following sqrtf mumbojumbo because ppc_osx doesn't define sqrtf
//    like it should.
#ifndef sqrtf
#define sqrtf(x) sqrt((x))
#endif
*/

#define LINEAR 0
#define RBF 1
#define POLY 2
#define C_SVC 0
#define ONE_CLASS 1
#define TAU 1e-12

typedef struct mythical_hyperspace_cell {
  unsigned long hash;
  // unsigned long key;
} HYPERSPACE_FEATUREBUCKET_STRUCT;
  


typedef struct mythical_svm_param {
  int svm_type;
  int kernel_type;
  double cache_size;     // in MB
  double eps;            //stop criteria
  double C;              // parameter in C_SVC
  double nu;             // parameter for One-class SVM
  double max_run_time;   // time control for microgroom (in seconds).  
                         // If computing time exceeds max_run_time, 
                         // then start microgrooming to delete the 
                         //  documents far away from the hyperplane. 
  int shrinking;         // use the shrinking heuristics, isn't available now
} SVM_PARAM;

typedef struct mythical_svm_problem {
  int l; //number of documents
  int *y; //label of documents -1/+1
  HYPERSPACE_FEATUREBUCKET_STRUCT **x; // x[i] is the ith document's 
                                       // feature vector
} SVM_PROBLEM;

typedef struct mythical_cache_node {
  struct mythical_cache_node *prev, *next;
  float *data;
  int len;
} CACHE_NODE;

typedef struct mythical_cache {
  int l;                   //the number of documents in the corpus
  long size;               //the cache size (bytes)
  CACHE_NODE  *head;
  CACHE_NODE lru_headnode; //least-recent-use node
} CACHE;

typedef struct mythical_solver{
  double *alpha;
  double *G;               //Gradient of objective function
  double *deci_array;      // decision values for all training data
} SOLVER;

/* [i_a] */
static SVM_PARAM param = {0};
static SVM_PROBLEM svm_prob = {0};
static CACHE svmcache = {0};
static float *DiagQ = NULL; //diagonal Qmatrix
static SOLVER solver = {0};

////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static const long hctable[] =
    { 1, 7,
      3, 13,
      5, 29,
      11, 51,
      23, 101,
      47, 203,
      97, 407,
      197, 817,
      397, 1637,
      797, 3277 };

static int hash_compare (void const *a, void const *b)
{
  HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;
  pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *) a;
  pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *) b;
  if (pa->hash < pb->hash)
    return (-1);
  if (pa->hash > pb->hash)
    return (1);
  return (0);
}

static int int_compare (void const *a, void const *b)
{
  int *pa, *pb;
  pa = (int *) a;
  pb = (int *) b;
  if (*pa < *pb)
    return (-1);
  if (*pa > *pb)
    return (1);
  return (0);
}

// Compare id of documents according to its decision value (decending 
//  absolute value), which is used for microgrooming
static int descend_int_decision_compare(void const *a, void const *b){
  int *pa, *pb;
  pa = (int *) a;
  pb = (int *) b;
  if(solver.deci_array[*pa] < solver.deci_array[*pb])
    return (1);
  if(solver.deci_array[*pa] > solver.deci_array[*pb])
    return (-1);
  return (0);
}

///////////////////////////////////////////////////////////////////////////
// 
//     Cache with least-recent-use strategy
//     This will be used to store part of Qmatrix
//

static void cache_init(int len, long size, CACHE *svmcache) {
  svmcache->l = len;
  svmcache->size = size;
  svmcache->head = (CACHE_NODE *) calloc(len, sizeof(svmcache->head[0])); /* [i_a] */
#if 0 /* [i_a] unused code... */
  size /= sizeof(float);
  size -= len * (sizeof(CACHE_NODE)/sizeof(float));
  if(size < (2 * len)) 
    size = 2 * len;      //  cache size must at least as large 
                         //   as two columns of Qmatrix
#endif
  (svmcache->lru_headnode).prev 
    = (svmcache->lru_headnode).next 
    = &(svmcache->lru_headnode);
}


static void cache_free(CACHE *svmcache){
  CACHE_NODE *temp;
  for(temp = (svmcache->lru_headnode).next; 
      temp != &(svmcache->lru_headnode); 
      temp = temp->next)
    free(temp->data);
  free(svmcache->head);
}

//  Delete one node
static void lru_delete(CACHE_NODE *h){
  h->prev->next = h->next;
  h->next->prev = h->prev;
}

//  Insert to the last position in the cache node list
static void lru_insert(CACHE_NODE *h, CACHE *svmcache){
  h->next = &(svmcache->lru_headnode);
  h->prev = (svmcache->lru_headnode).prev;
  h->prev->next = h;
  (svmcache->lru_headnode).prev = h;
}

//  Get data for certain document and return the length of the cached 
//   data.  If it is smaller than the request length, then we need to 
//    fill in the uncached data.
static int get_data(CACHE *svmcache, 
		    const int doc_index, 
		    float **data, 
		    int length)
{
  int result = length;
  CACHE_NODE *doc = svmcache->head + doc_index;
  assert(doc_index >= 0);
  assert(doc_index < svmcache->l);
  if(doc->len) lru_delete(doc); //least-recent-use strategy
  
  //  need to allocate more space
  if(length > (doc->len))
    {
      //  cache hasn't enough free space, we need to release some old space
      while((svmcache->size) < (length - doc->len)){
	CACHE_NODE *temp = (svmcache->lru_headnode).next;
	lru_delete(temp);
	free(temp->data);
	svmcache->size += temp->len;
	temp->data = 0;
	temp->len = 0;
      }
      
      //  allocate new space
      doc->data = (float *)realloc(doc->data, length * sizeof(doc->data[0])); /* [i_a] */
	  if (!doc->data)
	  {
		  fatalerror("Could not re-allocate enough space for document data ", "");
	  }
	  else
	  {
      svmcache->size -= (length - doc->len);
      result = doc->len;
      doc->len = length;
	  }
    }
  lru_insert(doc, svmcache);
  *data = doc->data;
  return result;
}


//  "Dot" operation of two feature vectors - this is basically how 
//  many slots of the two sparse vectors match with nonzero value.
static double dot(void const *a, void const *b)
{
  /* [i_a] it is C code, not C++ */
  int j = 0;
  int i = 0;
  double sum = 0;
  HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;

  pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *) a;
  pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *) b;
  while(pa[i].hash != 0 && pb[j].hash != 0)
  {
    if(pa[i].hash == pb[j].hash && pa[i].hash != 0){
      sum ++;
      i++;
      j++;
    }
    else{
      if(pa[i].hash > pb[j].hash)
	j++;
      else
	i++;
    }
  }
  return sum;
}

//   RBF (Radial Basis Function) is another basis function; it
//   measures distance based on a decaying exponential. 
//
static double rbf(void const *a, void const *b){
  /* [i_a] it is C code, not C++ */
  int j = 0;
  int i = 0;
  double sum = 0;
  HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;

  pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *) a;
  pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *) b;
  while(pa[i].hash != 0 && pb[j].hash != 0)
    {
      if(pa[i].hash > pb[j].hash)
	{
	  sum ++;
	  j++;
	}
      else if(pa[i].hash < pb[j].hash)
	{
	  sum ++;
	  i++;
	}
      else
	{
	  i++;
	  j++;
	}
    }
  while(pa[i].hash != 0)
    {
      sum ++;
      i++;
    }
  while(pb[j].hash != 0)
    {
      sum ++;
      j++;
    }
  return exp(-0.00001  * sum);
}


//dot operation of two feature vectors
static double poly(void const *a, void const *b)
{
  double gamma = 0.001;
  double sum = 0.0;
  sum = pow(gamma * dot(a,b) + 3, 2.0);
  return sum;
}


//kernel operation of two feature vectors. Now only support linear kernel
static double kernel(void const *a, void const *b)
{
  switch(param.kernel_type){
  case LINEAR:
    return dot(a,b);
  case RBF:
    return rbf(a,b);
  case POLY:
    return poly(a,b);  
  default:
    return 0;
  }
}



//request of the ith column in Qmatrix for C- Support Vector Classification(C-SVC)and one-class SVM
static float *get_columnQ(int i,int length){
  float *columnQ;
  int uncached;
  if((uncached = get_data(&svmcache, i, &columnQ, length)) < length){
    int temp;
    for(temp = uncached; temp < length; temp++){
      if(param.svm_type == C_SVC)
	columnQ[temp] = svm_prob.y[i] * svm_prob.y[temp] * kernel(svm_prob.x[i],svm_prob.x[temp]);
      else if(param.svm_type == ONE_CLASS)
	columnQ[temp] = kernel(svm_prob.x[i],svm_prob.x[temp]);
    }
  }
  return columnQ;
}

//request of the diagonal in Qmatrix for C- Support Vector Classification(C-SVC) and one-class SVM
static float *get_DiagQ(){
  float *DiagQ = malloc(svm_prob.l * sizeof(DiagQ[0])); /* [i_a] */
  int i;
  for(i = 0; i<svm_prob.l; i++){
    DiagQ[i] = kernel(svm_prob.x[i],svm_prob.x[i]);
  }
  return DiagQ;
}

//initialize the cache and diagonal Qmatrix
static void Q_init(){
  cache_init(svm_prob.l, (long)(param.cache_size*(1L<<20)), &svmcache);
  DiagQ = get_DiagQ();
}

// An SMO algorithm in Fan et al., JMLR 6(2005), p. 1889--1918
// Solves:
//
//	min 0.5(\alpha^T Q \alpha) + p^T \alpha
//
//		y^T \alpha = \delta
//		y_i = +1 or -1
//		0 <= alpha <= C
//	       
//
// Given:
//
//	Q, p, y, C, and an initial feasible point \alpha
//	l is the size of vectors and matrices
//	eps is the stopping tolerance
//
// solution will be put in \alpha, objective value will be put in obj
//

static void selectB(int workset[], int *select_times){
  /* [i_a] it is C code, not C++ */
  int j = -1;
  double obj_min = HUGE_VAL;
  double a,b;
  float *Qi;
  //select i
  int i = -1;
  double G_max = -HUGE_VAL;
  double G_min = HUGE_VAL;
  int t;

  for (t = 0; t < svm_prob.l; t++){
    if((((svm_prob.y[t] == 1) && (solver.alpha[t] < param.C)) || ((svm_prob.y[t] == -1) && (solver.alpha[t] > 0))) && select_times[t] < 10){
      if(-svm_prob.y[t] * solver.G[t] >= G_max){
	i = t;
	G_max = -svm_prob.y[t] * solver.G[t];
      }
    }
  }
  //select j;
  /* [i_a] it is C code, not C++ */
  /*
  int j = -1;
  double obj_min = HUGE_VAL;
  double a,b;
  float *Qi;
  */
  for (t = 0; t< svm_prob.l; t++){
    if((((svm_prob.y[t] == -1) && (solver.alpha[t] < param.C)) || ((svm_prob.y[t] == 1) && (solver.alpha[t] > 0))) && select_times[t] < 10){
      b = G_max + svm_prob.y[t] * solver.G[t];
      if(-svm_prob.y[t] * solver.G[t] <= G_min)
	G_min = -svm_prob.y[t] * solver.G[t];
      if(b > 0){
	if(i != -1){
	  Qi = get_columnQ(i,svm_prob.l);
	  a = Qi[i] + DiagQ[t] - 2 * svm_prob.y[i] * svm_prob.y[t] * Qi[t];
	  if (a <= 0)
	    a = TAU;
	  if(-(b * b) / a <= obj_min){
	    j = t;
	    obj_min = -(b * b) / a;  
	  }
	}
	
      }
    }
  }
  if(G_max - G_min < param.eps){
    workset[0] = -1;
    workset[1] = -1;
  }else{
    workset[0] = i;
    workset[1] = j;
  }
}

#ifdef calc_obj
//calculate objective value given alpha
static double calc_obj(){
  double obj = 0;
  int i;
  for (i=0; i < svm_prob.l; i++){
    obj += solver.alpha[i] * (solver.G[i] - 1);
  }
  obj /= 2;
  return obj;
}
#endif 
static void solve(){
  int t,workset[2],i,j, n;
  double a,b, oldi, oldj, sum;// obj = 0;
  float *Qi, *Qj;

  //array for storing how many times it has been selected as working set
  int *select_times = malloc(svm_prob.l * sizeof(select_times[0]));   /* [i_a] it is C code, not C++ */
  for(i = 0; i < svm_prob.l; i++){
    select_times[i] = 0;
  }

  assert(solver.alpha == NULL);
  solver.alpha = malloc(svm_prob.l * sizeof(solver.alpha[0])); /* [i_a] */
  assert(solver.G == NULL);
  solver.G = malloc(svm_prob.l * sizeof(solver.G[0])); /* [i_a] */
  if(param.svm_type == C_SVC){
    //initialize alpha to all zero;
    //initialize G to all -1;
    for(t = 0; t < svm_prob.l; t++){
      solver.alpha[t] = 0;
      solver.G[t] = -1;
    }
  }else if(param.svm_type == ONE_CLASS){
    //initialize the first nu*l elements of alpha to have the value one;
    n = (int)(param.nu * svm_prob.l);  
	assert(n <= svm_prob.l);
    for(i = 0; i < n; i++)
      solver.alpha[i] = 1;
    if(n < svm_prob.l)
      solver.alpha[n] = param.nu * svm_prob.l - n;
    for(i = n + 1;i < svm_prob.l;i++)
      solver.alpha[i] = 0;
    //initialize G to all 0;
    for(i = 0; i < svm_prob.l; i++){
      solver.G[i] = 0;
    }
  }
  while(1){
    selectB(workset, select_times);
    i = workset[0];
    j = workset[1];
    if(i != -1)
      select_times[i] ++;
    if(j != -1)
      select_times[j] ++;
    if(j == -1)
      break;
    //fprintf(stderr, "i=%d\tj=%d\t",i,j);
    
    Qi = get_columnQ(i, svm_prob.l);
    Qj = get_columnQ(j, svm_prob.l);
    
    a = Qi[i] + DiagQ[j] - 2 * svm_prob.y[i] * svm_prob.y[j] * Qi[j];
    if(a <= 0)
      a = TAU;
    b = -svm_prob.y[i] * solver.G[i] + svm_prob.y[j] * solver.G[j];

    //update alpha
    oldi = solver.alpha[i];
    oldj = solver.alpha[j];
    solver.alpha[i] += svm_prob.y[i] * b/a;
    solver.alpha[j] -= svm_prob.y[j] * b/a;
    
    //project alpha back to the feasible region
    sum = svm_prob.y[i] * oldi + svm_prob.y[j] * oldj;
    if (solver.alpha[i] > param.C)
      solver.alpha[i] = param.C;
    if (solver.alpha[i] < 0 )
      solver.alpha[i] = 0;
    solver.alpha[j] = svm_prob.y[j] * (sum - svm_prob.y[i] * (solver.alpha[i]));
    if (solver.alpha[j] > param.C)
      solver.alpha[j] = param.C;
    if (solver.alpha[j] < 0 )
      solver.alpha[j] = 0;
    solver.alpha[i] = svm_prob.y[i] * (sum - svm_prob.y[j] * (solver.alpha[j]));
    
    
    //update gradient array
    for(t = 0; t < svm_prob.l; t++){  
      solver.G[t] += Qi[t] * (solver.alpha[i] - oldi) + Qj[t] * (solver.alpha[j] - oldj);
    }
    //obj = calc_obj();
  }

  free(select_times);   /* [i_a] it is C code, not C++ */
}

//calculate b after achieving alpha
static double calc_b(){
  int count = 0;
  double upper = HUGE_VAL;
  double lower = -HUGE_VAL;
  double sum = 0;
  int i;
  double b;
  for(i = 0; i < svm_prob.l; i++){
    if(svm_prob.y[i] == 1){
      if(solver.alpha[i] == param.C){
	if(solver.G[i] > lower){
	  lower = solver.G[i];
	}
      }else if(solver.alpha[i] == 0){
	if(solver.G[i] < upper){
	  upper = solver.G[i];
	}
      }else{
	count++;
	sum += solver.G[i];
      }
	
    }else{
      if(solver.alpha[i] == 0){
	if(-solver.G[i] > lower){
	  lower = -solver.G[i];
	}
      }else if(solver.alpha[i] == param.C){
	if(-solver.G[i] < upper){
	  upper = -solver.G[i];
	}
      }else{
	count++;
	sum -= solver.G[i];
      }
    }
  }
  if(count > 0)
    b = -sum/count;
  else
    b = -(upper + lower)/2;
  return b;
}

//calculate the decision function
static double calc_decision(HYPERSPACE_FEATUREBUCKET_STRUCT *x, double *alpha, double b){
  int i;
  double sum = 0;
  i=0;
  if(param.svm_type == C_SVC){
    for (i = 0; i < svm_prob.l; i++){
      sum += svm_prob.y[i] * alpha[i] * kernel(x,svm_prob.x[i]);
    }
    sum += b;
  }else if(param.svm_type == ONE_CLASS){
    for (i = 0; i < svm_prob.l; i++){
      sum += alpha[i] * kernel(x,svm_prob.x[i]);
    }
    sum -= b;
  }
  return sum;
}

//implementation of Lin's 2003 improved algorithm on Platt's probabilistic outputs for binary SVM
//input parameters: deci_array = array of svm decision values
//                  svm.prob
//                  posn = number of positive examples
//                  negn = number of negative examples

//outputs: parameters of sigmoid function-- A and B  (AB[0] = A, AB[1] = B)
static void calc_AB(double *AB, double *deci_array, int posn, int negn){
  int maxiter = 100, i, j;
  double minstep = 1e-10;
  double sigma = 1e-3;
  double fval = 0.0;
  double hiTarget = (posn + 1.0) / (posn + 2.0);
  double loTarget = 1 / (negn + 2.0);
  double *t = malloc(svm_prob.l * sizeof(t[0])); /* [i_a] */
  double fApB, h11, h22, h21, g1, g2, p, q, d1, d2, det, dA, dB, gd, stepsize, newA, newB, newf;
  
  for(i = 0; i< svm_prob.l; i++){
    if(svm_prob.y[i] > 0)
      t[i] = hiTarget;
    else
      t[i] = loTarget;
  }
  AB[0] = 0.0;
  AB[1] = log((negn + 1.0) / (posn + 1.0));
  for (i = 0; i < svm_prob.l; i++){
    fApB = deci_array[i] * AB[0] + AB[1];
    if(fApB >= 0)
      fval += t[i] * fApB + log(1 + exp(-fApB));
    else
      fval += (t[i] - 1) * fApB + log(1 + exp(fApB));
  }
  
  for(j = 0; j < maxiter; j++){
    h11 = h22 = sigma;
    h21 = g1 = g2 = 0.0;
    for(i = 0; i < svm_prob.l; i++){
      fApB = deci_array[i] * AB[0] + AB[1];
      if(fApB >= 0){
	p = exp(-fApB) / (1.0 + exp(-fApB));
	q = 1.0 / (1.0 + exp(-fApB));
      }else{
	p =  1.0 / (1.0 + exp(fApB));
	q = exp(fApB) / (1.0 + exp(fApB));
      }
      d2 = p * q;
      h11 += deci_array[i] * deci_array[i] * d2;
      h22 += d2;
      h21 += deci_array[i] * d2;
      d1 = t[i] - p;
      g1 += deci_array[i] * d1;
      g2 += d1; 
    }
    // Stopping Criteria
    if ((fabs(g1) < 1e-5) && (fabs(g2) < 1e-5)){
      break;
    }
    //compute modified Newton directions
    det = h11 * h22 - h21 * h21;
    dA = -(h22 * g1 - h21 * g2) / det;
    dB = -(-h21 * g1 +  h11 * g2) / det;
    gd = g1 * dA + g2 * dB;
    stepsize = 1;
    while (stepsize >= minstep){
      newA = AB[0] + stepsize * dA;
      newB = AB[1] + stepsize * dB;
      newf = 0.0;
      for (i = 0; i < svm_prob.l; i++){
	fApB = deci_array[i] * newA + newB;
	if (fApB >= 0)
	  newf += t[i] * fApB + log(1 + exp(-fApB));
	else
	  newf += (t[i] - 1) * fApB + log(1 + exp(fApB));
      }
      // Check whether sufficient decrease is satisfied
      if (newf < fval + 0.0001 * stepsize * gd){
	AB[0] = newA;
	AB[1] = newB;
	fval = newf;
	break;
      }
      else
	stepsize /= 2.0;
    }
    if (stepsize < minstep){
      if(user_trace)
	fprintf(stderr, "Line search fails in probability estimates\n");
      break;
    }
  }
  if (j >= maxiter)
    if(user_trace)
      fprintf(stderr, "Reaching maximal iterations in  probability estimates\n");
  free(t);
}

static double sigmoid_predict(double decision_value, double A, double B)
{
	double fApB = decision_value * A + B;
	if (fApB >= 0)
		return exp(-fApB) / (1.0 + exp(-fApB));
	else
		return 1.0 / (1 + exp(fApB)) ;
}


int crm_expr_svm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
		       char *txtptr, long txtstart, long txtlen){
  int yimin_trace = 0;
  /* [i_a] it is C code, not C++ */
  long i = 0, j = 0, k = 0, h;
  char ftext[MAX_PATTERN];
  long flen;
  char file1[MAX_PATTERN];
  char file2[MAX_PATTERN];
  char file3[MAX_PATTERN];
  FILE *hashf; 
  long textoffset;
  long textmaxoffset;
  HYPERSPACE_FEATUREBUCKET_STRUCT *hashes;  //  the hashes we'll sort 
  long hashcounts; 
  long cflags, eflags;
  long sense;
  long microgroom;
  long unique;
  long use_unigram_features;
  char ptext[MAX_PATTERN]; //the regrex pattern
  long plen;
  regex_t regcb;
  regmatch_t match[5]; 
  struct stat statbuf1;      //  for statting the hash file1
  struct stat statbuf2;      //  for statting the hash file2
  unsigned long hashpipe[OSB_BAYES_WINDOW_LEN+1]; 
  time_t start_timer;
  time_t end_timer;
  double run_time;

  internal_trace = 0;
  user_trace = 0;
  
  start_timer = time(NULL);

  //            set our cflags, if needed.  The defaults are
  //            "case" and "affirm", (both zero valued).
  //            and "microgroom" disabled.
  cflags = REG_EXTENDED;
  eflags = 0;
  sense = +1;
  if (apb->sflags & CRM_NOCASE){
    cflags = cflags | REG_ICASE;
    eflags = 1;
    if (user_trace)
      fprintf (stderr, "turning oncase-insensitive match\n");
  }
  if (apb->sflags & CRM_REFUTE){
    sense = -sense;
    /////////////////////////////////////
    //    Take this out when we finally support refutation
    ////////////////////////////////////
    //      fprintf (stderr, "Hyperspace Refute is NOT SUPPORTED YET\n");
    //return (0);
    if (user_trace)
      fprintf (stderr, " refuting learning\n");
  }
  microgroom = 0;
  if (apb->sflags & CRM_MICROGROOM){
    microgroom = 1;
    if (user_trace)
      fprintf (stderr, " enabling microgrooming.\n");
  }
  unique = 0;
  if (apb->sflags & CRM_UNIQUE){
    unique = 1;
    if (user_trace)
      fprintf (stderr, " enabling uniqueifying features.\n");
  }

  use_unigram_features = 0;
  if (apb->sflags & CRM_UNIGRAM){
    use_unigram_features = 1;
    if (user_trace)
      fprintf (stderr, " using only unigram features.\n");
  }
   
  //   Note that during a LEARN in hyperspace, we do NOT use the mmap of
  //    pre-existing memory.  We just write to the end of the file instead.
  //    malloc up the unsorted hashbucket space 
  hashes = calloc (HYPERSPACE_MAX_FEATURE_COUNT, 
		   sizeof (hashes[0])); /* [i_a] */
  hashcounts = 0;
 
  // extract the file names for storing svm solver.( file1.svm | file2.svm | 1vs2_solver.svm )
  crm_get_pgm_arg (ftext, MAX_PATTERN, apb->p1start, apb->p1len);
  flen = apb->p1len;
  flen = crm_nexpandvar (ftext, flen, MAX_PATTERN);
 
  strcpy(ptext,"[[:space:]]*([[:graph:]]+)[[:space:]]+\\|[[:space:]]+([[:graph:]]+)[[:space:]]+\\|[[:space:]]+([[:graph:]]+)[[:space:]]*");
  plen = strlen(ptext);   
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
  i = crm_regcomp (&regcb, ptext, plen, cflags);
  if ( i > 0){
    crm_regerror ( i, &regcb, tempbuf, data_window_size);
    nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
    goto regcomp_failed;
  }
  k = crm_regexec (&regcb, ftext,
		   flen, 5, match, 0, NULL);
  if( k==0 ){
    //get three input files.
    memmove(file1,&ftext[match[1].rm_so],(match[1].rm_eo-match[1].rm_so));
    file1[match[1].rm_eo-match[1].rm_so]='\000';
    memmove(file2,&ftext[match[2].rm_so],(match[2].rm_eo-match[2].rm_so));
    file2[match[2].rm_eo-match[2].rm_so]='\000';
    memmove(file3,&ftext[match[3].rm_so],(match[3].rm_eo-match[3].rm_so));
    file3[match[3].rm_eo-match[3].rm_so]='\000';
    if(internal_trace)
      fprintf(stderr, "file1=%s\tfile2=%s\tfile3=%s\n", file1, file2, file3);
  }else{
    if (ptext[0] != '\0') crm_regfree (&regcb);
    i = 0;
    while(ftext[i] < 0x021) i++;
    j = i;
    while(ftext[j] >= 0x021) j++;
    ftext[j] = '\000';
    strcpy(file1, &ftext[i]); 
    file2[0] = '\000';
    file3[0] = '\000';
  }
  //if (|Text|>0) hide it 
  
  //     get the "this is a word" regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
  
  //   compile the word regex
  //
  if ( internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);

  i = crm_regcomp (&regcb, ptext, plen, cflags);
  
  if ( i > 0){
    crm_regerror ( i, &regcb, tempbuf, data_window_size);
    nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
    goto regcomp_failed;
  }
  
  //   Now tokenize the input text
  //   We got txtptr, txtstart, and txtlen from the caller.
  //
  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;
  i = 0;
  j = 0;
  k = 0;
  
   //   init the hashpipe with 0xDEADBEEF 
  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++){
    hashpipe[h] = 0xDEADBEEF;
  }
 
  // if [Text]>0 hide it and append to the file1
  if (txtlen > 0){
    while (k == 0 && textoffset <= textmaxoffset 
	   && hashcounts < HYPERSPACE_MAX_FEATURE_COUNT  ){
      long wlen;
      long slen = textmaxoffset - textoffset;
      // if pattern is empty, extract non graph delimited tokens
      // directly ([[graph]]+) instead of calling regexec  (8% faster)
      if (ptext[0] != '\0'){
	k = crm_regexec (&regcb, &(txtptr[textoffset]),
			 slen, 5, match, 0, NULL);
      }else{
	k = 0;
	//         skip non-graphical characthers
	match[0].rm_so = 0;
	while (!isgraph (txtptr[textoffset + match[0].rm_so])
	       && textoffset + match[0].rm_so < textmaxoffset)
	  match[0].rm_so ++;
	match[0].rm_eo = match[0].rm_so;
	while (isgraph (txtptr [textoffset + match[0].rm_eo])
	       && textoffset + match[0].rm_eo < textmaxoffset)
	  match[0].rm_eo ++;
	if ( match[0].rm_so == match[0].rm_eo)
	  k = 1;
      }
      if (!(k != 0 || textoffset > textmaxoffset)){
	wlen = match[0].rm_eo - match[0].rm_so;
	
	memmove (tempbuf, 
		 &(txtptr[textoffset + match[0].rm_so]),
		 wlen);
	tempbuf[wlen] = '\000';
	if (internal_trace){
	  fprintf (stderr, 
		   "  Learn #%ld t.o. %ld strt %ld end %ld len %ld is -%s-\n", 
		   i, 
		   textoffset,
		   (long) match[0].rm_so, 
		   (long) match[0].rm_eo,
		   wlen,
		   tempbuf);
	}
	if (match[0].rm_eo == 0){
	  nonfatalerror ( "The LEARN pattern matched zero length! ",
			  "\n Forcing an increment to avoid an infinite loop.");
	  match[0].rm_eo = 1;
	}
	
	//      Shift the hash pipe down one
	//
	for (h = OSB_BAYES_WINDOW_LEN-1; h > 0; h--){
	  hashpipe [h] = hashpipe [h-1];
	}
	//  and put new hash into pipeline
	hashpipe[0] = strnhash (tempbuf, wlen);
	if (internal_trace){
	  fprintf (stderr, "  Hashpipe contents: ");
	  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
	    fprintf (stderr, " %lud", hashpipe[h]);
	  fprintf (stderr, "\n");
	}
	
	//  and account for the text used up.
	textoffset = textoffset + match[0].rm_eo;
	i++;
	
	//        is the pipe full enough to do the hashing?
	//  we always run the hashpipe now, even if it's
	//  just full of 0xDEADBEEF.  (was i >=5)
	if(1){
	  unsigned long h1;
	  unsigned long h2;
	  long th = 0;         // a counter used for TSS tokenizing
	  long j2;
	  //
	  //     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
	  //     (coefficients chosen by requiring superincreasing,
	  //     as well as prime)
	  //
	  th = 0;
	  //
	  if (use_unigram_features == 1){
	    h1 = hashpipe[0];
	    if (h1 == 0) h1 = 0xdeadbeef;
	    h2 = 0xdeadbeef;
	    if (internal_trace)
	      fprintf (stderr, "Singleton feature : %lud\n", h1);
		assert(hashcounts >= 0);
		assert(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
	    hashes[hashcounts].hash = h1;
	    hashcounts++;
	  }
	  else{
	    for (j2 = 1; 
		 j2 < OSB_BAYES_WINDOW_LEN;      // OSB_BAYES_WINDOW_LEN;
		 j2++){
	      h1 = hashpipe[0]*hctable[0] + hashpipe[j2] * hctable[j2<<1];
	      if (h1 ==0 ) h1 = 0xdeadbeef;
	      // h2 = hashpipe[0]*hctable[1] + hashpipe[j2] * hctable[(j2<<1)-1];
	      //if (h2 == 0) h2 = 0xdeadbeef;
	      h2 = 0xdeadbeef;
	      if (internal_trace)
		fprintf (stderr, "Polynomial %ld has h1:%lud  h2: %lud\n",
			 j2, h1, h2);
	      
		assert(hashcounts >= 0);
		assert(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
	      hashes[hashcounts].hash = h1;
	      //		hashes[hashcounts].key = h2;
	      hashcounts++;
	    }
	  }
	}
      } else{
	if (ptext[0] != '\0') crm_regfree (&regcb);
	k = 1;
      }
    }   //   end the while k==0
       
    //   Now sort the hashes array.
    //
    qsort (hashes, hashcounts, 
	   sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT),
	   &hash_compare);
    
    if (user_trace){
      fprintf(stderr,"sorted hashes:\n");
      for(i=0;i<hashcounts;i++){
	fprintf(stderr, "hashes[%ld]=%lud\n",i,hashes[i].hash);
      }
      fprintf (stderr, "Total hashes generated: %ld\n", hashcounts);
    }

    //   And uniqueify the hashes array
    //
    i = 0;
    j = 0;
    
	assert(hashcounts >= 0);
	assert(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
    if (unique){
      while ( i < hashcounts ){
	if (hashes[i].hash != hashes[i+1].hash){
	  hashes[j]= hashes[i];
	  j++;
	}
	i++;
      }
      hashcounts = j;
    }
    
    //mark the end of a feature vector
    hashes[hashcounts].hash = 0;
 
    if (user_trace)
      fprintf (stderr, "Unique hashes generated: %ld\n", hashcounts);
    
    if(hashcounts > 0){
      //append the hashed text to file1
      
      //  Because there are probably retained hashes of the 
      //  file, we need to force an unmap-by-name which will allow a remap
      //  with the new file length later on.
      crm_force_munmap_filename (file1);
      if (user_trace)
	fprintf (stderr, "Opening a svm file %s for append.\n", file1);
      hashf = fopen ( file1 , "ab+");
      if (user_trace)
	fprintf (stderr, "Writing to a svm file %s\n", file1);
      //    and write the sorted hashes out.
      fwrite (hashes, 1, 
	      sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT) * (hashcounts + 1), 
	      hashf);
      fclose (hashf);
    }
    
    //fprintf(stderr, "Finish hiding text to %s!\n", file1);
  }
   
  //  let go of the hashes.
  free (hashes);
  
  //           extract parameters for svm 
  crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s2start, apb->s2len);
  plen = apb->s2len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
  if(plen){
    //set default parameters for SVM
    param.svm_type = C_SVC;
    param.kernel_type = LINEAR;
    param.cache_size = 1;//MB
    param.eps = 1e-3;
    param.C = 1;
	param.nu = 0.5;
    param.max_run_time = 1;
    param.shrinking = 1;//not available right now

    if (8 != sscanf(ptext, "%d %d %lf %lf %lf %lf %lf %d",&(param.svm_type), &(param.kernel_type), &(param.cache_size), &(param.eps), &(param.C), &(param.nu), &(param.max_run_time) , &(param.shrinking)))
		  {
			  nonfatalerror("Failed to decode the 8 SVM setup parameters [learn]: ", ptext);
		  }
  }else{
    //set default parameters for SVM
    param.svm_type = C_SVC;
    param.kernel_type = LINEAR;
    param.cache_size = 1;//MB
    param.eps = 1e-3;
    param.C = 1;
	param.nu = 0.5;
    param.max_run_time = 1;
    param.shrinking = 1;//not available right now
  }
  
  //if svm_type is ONE_CLASS, then do one class svm
  if(param.svm_type == ONE_CLASS){
    long file1_hashlens;
    HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
    int k1;
    k1 = stat (file1, &statbuf1);
    if (k1 != 0) { 
      nonfatalerror ("Refuting from nonexistent data cannot be done!"
		     " More specifically, this data file doesn't exist: ",
		     file1);
      return (0);
    }else{
		       int *y = NULL; /* [i_a] this is C, not C++ */
HYPERSPACE_FEATUREBUCKET_STRUCT **x = NULL;  /* [i_a] this is C, not C++ */

       k1 = 0;
       file1_hashlens = statbuf1.st_size;
       file1_hashes = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
	 crm_mmap_file (file1,
			0, file1_hashlens,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			NULL);
       file1_hashlens = file1_hashlens 
	 / sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT );
       //find out how many documents in file1
       for(i = 0;i< file1_hashlens;i++){
	 if(yimin_trace)
	   fprintf (stderr, "\nThe %ldth hash value in file1 is %lud", i, file1_hashes[i].hash);
	 if(file1_hashes[i].hash == 0){
	   k1 ++;
	 }
       }
       if(yimin_trace)
	 fprintf (stderr, "\nThe total number of documents in file1 is %d\n", k1);

       //initialize the svm_prob.x, svm_prob.y
       svm_prob.l = k1;
       /* int y[svm_prob.l];  ** [i_a] this is C, not C++ */
	   y = malloc(svm_prob.l * sizeof(y[0])); /* [i_a] */
       for(i = 0; i < k1; i++)
			y[i] = 1;
	svm_prob.y = y;
	/* HYPERSPACE_FEATUREBUCKET_STRUCT *x[svm_prob.l];  ** [i_a] this is C, not C++ */
	x = malloc(svm_prob.l * sizeof(x[0])); /* [i_a] */
	x[0] = &(file1_hashes[0]);
	k = 1;
	for(i = 1;i< file1_hashlens - 1;i++){
	  if(file1_hashes[i].hash == 0){
	    x[k++] = &(file1_hashes[i+1]);
	  }
	}
	svm_prob.x = x;
	if(yimin_trace){
	  for(i = 0;i< k;i++){
	    fprintf(stderr, "\nx[%ld]=%lud\n",i,x[i][1].hash);
	  }       
	}
	Q_init();
	solve(); //result is in solver
	
	//free cache
	cache_free(&svmcache);
    }
  }
  
  //if file2 is not empty, open file1 and file2, calculate hyperplane, 
  //and write the solution to file3
  if(file2[0] != '\000'){
 double b;  /* [i_a] this is C, not C++ */
 int *y = NULL;  /* [i_a] this is C, not C++ */
	double AB[2];  /* [i_a] this is C, not C++ */
HYPERSPACE_FEATUREBUCKET_STRUCT **x = NULL;  /* [i_a] this is C, not C++ */
	  double distance_fraction;  /* [i_a] this is C, not C++ */
	  double average1;   /* [i_a] this is C, not C++ */
	  double average2;  /* [i_a] this is C, not C++ */
	  double delete_fraction; // upbound of delete fraction   /* [i_a] this is C, not C++ */
	  int delete_num1;
	  int delete_num2;  /* [i_a] this is C, not C++ */
long file1_hashlens;
    HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
    long file2_hashlens;
    HYPERSPACE_FEATUREBUCKET_STRUCT *file2_hashes;
    int k1, k2;

    k1 = stat (file1, &statbuf1);
    k2 = stat (file2, &statbuf2);
    if (k1 != 0) { 
      nonfatalerror ("Refuting from nonexistent data cannot be done!"
		     " More specifically, this data file1 doesn't exist: ",
		     file1);
      return (0);
    }else if(k2 != 0){
      nonfatalerror ("Refuting from nonexistent data cannot be done!"
		     " More specifically, this data file2 doesn't exist: ",
		     file2);
      return (0);
    }else{
 double *deci_array = NULL;  /* [i_a] this is C, not C++ */
int *id_desc = NULL;  /* [i_a] this is C, not C++ */

      k1 = 0;
      k2 = 0;
      file1_hashlens = statbuf1.st_size;
      file1_hashes = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
	crm_mmap_file (file1,
		       0, file1_hashlens,
		       PROT_READ | PROT_WRITE,
		       MAP_SHARED,
		       NULL);
      file1_hashlens = file1_hashlens 
	/ sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT );
      file2_hashlens = statbuf2.st_size;
      file2_hashes = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
	crm_mmap_file (file2,
		       0, file2_hashlens,
		       PROT_READ | PROT_WRITE,
		       MAP_SHARED,
		       NULL);
      file2_hashlens = file2_hashlens 
	/ sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT );
      for(i = 0;i< file1_hashlens;i++){
	if(yimin_trace)
	  fprintf(stderr, "\nThe %ldth hash value in file1 is %lud", i, file1_hashes[i].hash);
	if(file1_hashes[i].hash == 0){
	  k1 ++;
	}
      }
      if(yimin_trace)
	fprintf (stderr, "\nThe total number of documents in file1 is %d\n", k1);
      
      for(i = 0;i< file2_hashlens;i++){
	if(yimin_trace)
	  fprintf (stderr, "\nThe %ldth hash value in file2 is %lud", i, file2_hashes[i].hash);
	if(file2_hashes[i].hash == 0){
	  k2 ++;
	}
      }
      if(yimin_trace)
	fprintf (stderr, "\nThe total number of documents in file2 is %d\n", k2);
      
      if((k1 > 0) && (k2 >0)){
	#ifdef yimin
	//           extract parameters for svm 
	crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s2start, apb->s2len);
	plen = apb->s2len;
	plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
	if(plen){
		//set default parameters for SVM
		param.svm_type = C_SVC;
		param.kernel_type = LINEAR;
		param.cache_size = 1;//MB
		param.eps = 1e-3;
		param.C = 1;
	param.nu = 0.5;
		param.max_run_time = 1;
		param.shrinking = 1;//not available right now

	   //sscanf(ptext, "%d %d %lf %lf %lf %d",&(param.svm_type), &(param.kernel_type), &(param.cache_size), &(param.eps), &(param.C), &(param.shrinking));
	   if (8 != sscanf(ptext, "%d %d %lf %lf %lf %lf %lf %d",&(param.svm_type), &(param.kernel_type), &(param.cache_size), &(param.eps), &(param.C), &(param.nu), &(param.max_run_time) , &(param.shrinking)))
		  {
			  nonfatalerror("Failed to decode the 8 SVM setup parameters [learn: hyperplane calc]: ", ptext);
		  }
	}else{
	  //set default parameters for SVM
	  param.svm_type = C_SVC;
	  param.kernel_type = LINEAR;
	  param.cache_size = 1;//MB
	  param.eps = 1e-3;
	  param.C = 1;
	param.nu = 0.5;
	  param.max_run_time = 1; //second
	  param.shrinking = 1;//not available right now
	}
	#endif
	//initialize the svm_prob.x, svm_prob.y
	svm_prob.l = k1 + k2;
	/* int y[svm_prob.l]; ** [i_a] this is C, not C++ */
	y = malloc(svm_prob.l * sizeof(y[0])); /* [i_a] this is C, not C++ */
	for(i = 0; i < k1; i++)
	  y[i] = 1;
	for(i = k1; i < svm_prob.l; i++)
	  y[i] = -1;
	svm_prob.y = y;
	x = malloc(svm_prob.l * sizeof(x[0])); /* [i_a] this is C, not C++ */
	x[0] = &(file1_hashes[0]);
	k = 1;
	for(i = 1;i< file1_hashlens - 1;i++){
	  if(file1_hashes[i].hash == 0){
	    x[k++] = &(file1_hashes[i+1]);
	  }
	}
	x[k++] = &(file2_hashes[0]);
	for(i = 1;i< file2_hashlens - 1;i++){
	  if(file2_hashes[i].hash == 0){
	    x[k++] = &(file2_hashes[i+1]);
	  }
	}
	svm_prob.x = x;
	if(yimin_trace){
	  for(i = 0;i< k;i++){
	    fprintf(stderr, "\nx[%ld]=%lud\n",i,x[i][1].hash);
	  }       
	}
	Q_init();
	solve(); //result is in solver
	b = calc_b(); /* [i_a] this is C, not C++ */
	
	//compute decision values for all training documents 
	deci_array = malloc(svm_prob.l * sizeof(deci_array[0])); /* [i_a] this is C, not C++ */
	for(i = 0; i < svm_prob.l; i++){
	  deci_array[i] = calc_decision(svm_prob.x[i], solver.alpha, b);
	}
	if (yimin_trace)
	  fprintf(stderr, "done********\n");
	/* double AB[2];  ** [i_a] this is C, not C++ */
	calc_AB(AB,deci_array, k1,k2);
	end_timer = time(NULL);
	run_time = difftime(end_timer, start_timer);
	if(user_trace)
	  fprintf(stderr, "run_time =  %f seconds\n", run_time);

	//IF MICROGROOMING IS ENABLED, WE'LL GET RID OF LESS THAN 10% DOCUMENTS THAT ARE FAR AWAY FROM THE HYPERPLANE (WITH LOW ABSOLUTE DECISION VALUE). HERE HAVE TWO PARAMETERS YOU CAN CONTROL, ONE IS DELETE_FRACTION, THE OTHER IS THE HOW FAR AWAY FROM THE HYPERPLANE. DEFAULT SET DELETE_FRACTION=5% AND IF |DISTANCE| > 1.2 * AVERAGE DISTANCE, THEN IT IS FAR AWAY FROM THE hyperplane. 
	//
       	if(microgroom && (run_time > param.max_run_time)){
	  distance_fraction = 1.0;  /* [i_a] this is C, not C++ */
	  if(user_trace)
	    fprintf(stderr, "\nStart microgrooming......\n");
	  assert(solver.deci_array == NULL);
	  solver.deci_array = deci_array;
	  
	  id_desc = malloc(svm_prob.l * sizeof(id_desc[0]));  /* [i_a] this is C, not C++ */
	  //int *id_asc = (int*) malloc(svm_prob.l * sizeof(int));
	  average1 = 0.0; average2 = 0.0;  /* [i_a] this is C, not C++ */
	  delete_fraction = pow(param.max_run_time/run_time, 1.0/3.0); // upbound of delete fraction   /* [i_a] this is C, not C++ */
	  for (i = 0; i < svm_prob.l; i++){
	    id_desc[i] = i;
	    //id_asc[i] = i;
	    if(i < k1)
	      average1 += solver.deci_array[i];
	    else 
	      average2 += solver.deci_array[i];
	  }
	  average1 /= k1;
	  average2 /= k2;
	  qsort (id_desc, svm_prob.l, sizeof (int), &descend_int_decision_compare );
	  if(user_trace)
	    fprintf(stderr, "average1=%f\taverage2=%f\n", average1, average2);
	  
	  //if decision[i] > 1.5 * average decision value, then get rid of it.
	  delete_num1 = 0; delete_num2 = 0;  /* [i_a] this is C, not C++ */
	  i = 0;
	  j = svm_prob.l - 1;
	  while(((delete_num1 + delete_num2) < floor(delete_fraction * svm_prob.l)) && (i < svm_prob.l) && (j >= 0)){
	    if((k1 - delete_num1) > (k2 - delete_num2)){
	      //delete the farest document from the first class
	      if(svm_prob.y[id_desc[i]] == 1 && fabs(solver.deci_array[id_desc[i]]) > distance_fraction * fabs(average1)){
		if(user_trace)
		  fprintf(stderr, "delete document %d!decision value=%f\n", id_desc[i],solver.deci_array[id_desc[i]]);
		id_desc[i] = -1;
		delete_num1 ++;
	      }
	      i++;
	    }else if((k1 - delete_num1) < (k2 - delete_num2)){
	      //delete the farest document from the second class
	      if(svm_prob.y[id_desc[j]] == -1 && fabs(solver.deci_array[id_desc[j]]) > distance_fraction * fabs(average2)){
		if(user_trace)
		  fprintf(stderr, "delete document %d!decision value=%f\n", id_desc[j],solver.deci_array[id_desc[j]]);
		id_desc[j] = -1;
		delete_num2 ++;
	      }
	      j--;
	    }else{
	      //delete the farest document from both classes
	      if(svm_prob.y[id_desc[i]] == 1  && fabs(solver.deci_array[id_desc[i]]) > distance_fraction * fabs(average1)){
		if(user_trace)
		  fprintf(stderr, "delete document %d!decision value=%f\n", id_desc[i],solver.deci_array[id_desc[i]]);
		id_desc[i] = -1;
		delete_num1 ++;
	      }
	      i++;
	      if(svm_prob.y[id_desc[j]] == -1 && fabs(solver.deci_array[id_desc[j]]) > distance_fraction * fabs(average2)){
		if(user_trace)
		  fprintf(stderr, "delete document %d!decision value=%f\n", id_desc[j],solver.deci_array[id_desc[j]]);
		id_desc[j] = -1;
		delete_num2 ++;
	      }
	      j--;
	    }	    
	  }
	  //if(user_trace)
	    fprintf(stderr, "delete_num1 = %d\t delete_num2 = %d\n", delete_num1, delete_num2);

	  #ifdef yimin  	  
	  for(i = 0; i < svm_prob.l; i++){
	    if((solver.alpha[id_desc[i]] == 0) && (solver.deci_array[id_desc[i]] >distance_fraction * average1) && (delete_num1 < k1 * delete_fraction) && (svm_prob.y[id_desc[i]] == 1)){
	      fprintf(stderr, "delete document %d!decision value=%lf\n", id_desc[i],solver.deci_array[id_desc[i]]);
	      id_desc[i] = -1;
	      delete_num1 ++;
	    }else if(delete_num1 >= k1 * delete_fraction)
	      break;
	  }
	  
	  for(i = svm_prob.l - 1; i >= 0; i--){
	    if((solver.alpha[id_desc[i]] == 0) && (solver.deci_array[id_desc[i]] < distance_fraction * average2) && (delete_num2 < k2 * delete_fraction) && (svm_prob.y[id_desc[i]] == -1) ){
	      fprintf(stderr, "delete document %d!decision value=%lf\n", id_desc[i],solver.deci_array[id_desc[i]]);
	      id_desc[i] = -1;
	      delete_num2 ++;
	    }else if(delete_num2 >= k2 * delete_fraction)
	      break;
	  }
	  #endif


          
	  if(delete_num1 != 0 || delete_num2 != 0){
		int temp_i; /* [i_a] this is C, not C++ */
		int temp_count; /* [i_a] this is C, not C++ */
	    HYPERSPACE_FEATUREBUCKET_STRUCT **new_x = malloc((k1 + k2 - delete_num1 - delete_num2) * sizeof(new_x[0])); /* [i_a] this is C, not C++ */
	    //double *newalpha = (double *)malloc((k1 + k2 - delete_num1 - delete_num2) * sizeof(double));
	    qsort (id_desc, svm_prob.l, sizeof (int), &int_compare );
	    //now start deleting documents and write the remain documents to file1, this is unrecoverable.
	    j = 0;
	    for(i = 0; i < svm_prob.l; i++){
	      if((id_desc[i] != -1) && (id_desc[i] < k1)){
		//newalpha[j] = solver.alpha[id_desc[i]];
		svm_prob.y[j] = svm_prob.y[id_desc[i]];
		temp_count = 0; /* [i_a] this is C, not C++ */
		while(svm_prob.x[id_desc[i]][temp_count].hash != 0)
		  temp_count ++;
		new_x[j] = (HYPERSPACE_FEATUREBUCKET_STRUCT *)malloc((temp_count + 1) * sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT));
		/* int temp_i; ** [i_a] this is C, not C++ */
		for(temp_i = 0; temp_i<temp_count; temp_i++)
		  new_x[j][temp_i] = svm_prob.x[id_desc[i]][temp_i];
		new_x[j][temp_count].hash = 0 ;
		j++;
	      }else if((id_desc[i] != -1) && (id_desc[i] >= k1)){
		//newalpha[j] = solver.alpha[id_desc[i]];
		svm_prob.y[j] = svm_prob.y[id_desc[i]];
		temp_count = 0; /* [i_a] this is C, not C++ */
		while(svm_prob.x[id_desc[i]][temp_count].hash != 0)
		  temp_count ++;
		new_x[j] = (HYPERSPACE_FEATUREBUCKET_STRUCT *)malloc((temp_count + 1) * sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT));
		/* int temp_i; ** [i_a] this is C, not C++ */
		for(temp_i = 0; temp_i<temp_count; temp_i++)
		  new_x[j][temp_i] = svm_prob.x[id_desc[i]][temp_i];
		new_x[j][temp_count].hash = 0 ;
		j++;
	      }
	    }
	    svm_prob.l = k1 + k2 - delete_num1 - delete_num2;
	    svm_prob.x = new_x;
	    //free(solver.alpha);
	    //solver.alpha = newalpha;
	    if(delete_num1 != 0){
	      crm_force_munmap_filename (file1);
	      if (user_trace)
		fprintf (stderr, "Opening a svm file %s for rewriting.\n", file1);
	      hashf = fopen ( file1 , "wb+");
	      if (user_trace)
		fprintf (stderr, "Writing to a svm file %s\n", file1);
	      for(i = 0; i < k1 - delete_num1; i++){
		temp_count = 0; /* [i_a] this is C, not C++ */
		while(svm_prob.x[i][temp_count].hash != 0){
		  temp_count ++;
		}
		fwrite (svm_prob.x[i], 1, 
			sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT) * (temp_count+1), 
			hashf);
	      }
	      fclose (hashf);
	    }
	    if(delete_num2 != 0){
	      crm_force_munmap_filename (file2);
	      if (user_trace)
		fprintf (stderr, "Opening a svm file %s for rewriting.\n", file2);
	      hashf = fopen ( file2 , "wb+");
	      if (user_trace)
		fprintf (stderr, "Writing to a svm file %s\n", file2);
	      for(i = k1 - delete_num1; i < svm_prob.l; i++){
		temp_count = 0; /* [i_a] this is C, not C++ */
		while(svm_prob.x[i][temp_count].hash != 0)
		  temp_count ++;
		fwrite (svm_prob.x[i], 1, 
			sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT) * (temp_count+1), 
			hashf);
	      }
	      fclose (hashf);
	    }
	   
	    k1 -= delete_num1;
	    k2 -= delete_num2;

	    //recalculate the hyperplane
            //
	    //free cache
	    cache_free(&svmcache);
	    if(user_trace)
	      fprintf(stderr, "recalculate the hyperplane!\n");
	    //recalculate the hyperplane
	    Q_init();
	    solve(); //result is in solver
	    b = calc_b();
	    if(yimin_trace)
	      fprintf(stderr, "b=%f\n",b);
	    
	    fprintf(stderr, "Finishing calculate the hyperplane\n");
	    
	    //compute A,B for sigmoid prediction
	    deci_array = realloc(deci_array, svm_prob.l * sizeof(deci_array[0])); /* [i_a] */
	    for(i = 0; i < svm_prob.l; i++){
	      deci_array[i] = calc_decision(svm_prob.x[i], solver.alpha, b);
	    }
	    calc_AB(AB, deci_array, k1, k2);
	    fprintf(stderr, "Finishing calculate AB\n");
	    for(i = 0; i < svm_prob.l; i++){
	      free(new_x[i]);
	    }
	    fprintf(stderr, "No bug untill here\n");
	  }
	  free(id_desc);
	}
	free(deci_array);
	solver.deci_array = NULL; /* [i_a] */
	
	//  write solver to file3
	if (user_trace)
	  fprintf (stderr, "Opening a solution file %s for writing alpha and b.\n", file3);
	hashf = fopen ( file3 , "wb+"); /* [i_a] on MSwin/DOS, fopen() opens in CRLF text mode by default; this will corrupt those binary values! */
	if (user_trace)
	  fprintf (stderr, "Writing to a svm solution file %s\n", file3);

	fwrite(&k1, sizeof(int), 1, hashf);
	fwrite(&k2, sizeof(int), 1, hashf);
	for(i = 0; i < svm_prob.l; i++)
	  fwrite(&(solver.alpha[i]), sizeof(double), 1, hashf);
	fwrite(&b, sizeof(double), 1, hashf);
	fwrite(&AB[0], sizeof(double), 1, hashf);
	fwrite(&AB[1], sizeof(double), 1, hashf);
	
	fclose (hashf);
        if(yimin_trace){
	  double decision = calc_decision(x[0],solver.alpha,b);
	  fprintf(stderr, "decision1 = %f\n",decision);
	  decision = calc_decision(x[301],solver.alpha,b);
	  fprintf(stderr, "decision2 = %f\n",decision);
	}
	//free cache
	cache_free(&svmcache);
	free(solver.G);
	solver.G = NULL; /* [i_a] */
	free(DiagQ);
DiagQ = NULL; /* [i_a] */
	free(solver.alpha);
	solver.alpha = NULL; /* [i_a] */
	if(user_trace)
	  fprintf(stderr, "Finish calculating SVM hyperplane, store the solution to %s!\n", file3);
      }else{
	if (user_trace)
	  fprintf(stderr, "There hasn't enough documents to calculate a svm hyperplane!\n");
      }
      crm_force_munmap_filename (file1);
      crm_force_munmap_filename (file2);
      crm_force_munmap_filename (file3);
    }
  }//end if(file2[0])
 regcomp_failed:
  return 0;
}

int crm_expr_svm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
			  char *txtptr, long txtstart, long txtlen){
  int yimin_trace = 0;
  long i,j, k, h;
  char ftext[MAX_PATTERN];
  long flen;
  char ptext[MAX_PATTERN]; //the regrex pattern
  long plen;
  char file1[MAX_PATTERN];
  char file2[MAX_PATTERN];
  char file3[MAX_PATTERN];
  regex_t regcb;
  regmatch_t match[5];
  FILE *hashf;
  long textoffset;
  long textmaxoffset;
  unsigned long hashpipe[OSB_BAYES_WINDOW_LEN+1]; 
  HYPERSPACE_FEATUREBUCKET_STRUCT *hashes;  //  the hashes we'll sort 
  long hashcounts; 
  long cflags, eflags;
  long sense;
  long microgroom;
  long unique;
  long use_unigram_features;
  struct stat statbuf1;      //  for statting the hash file1
  struct stat statbuf2;      //  for statting the hash file2
  struct stat statbuf3;      //  for statting the hash file3
  double *alpha = NULL;  /* [i_a] */
  double b;
  double AB[2];
  long slen;
  char svrbl[MAX_PATTERN];  //  the match statistics text buffer
  long svlen;
  //  the match statistics variable
  char stext [MAX_PATTERN+MAX_CLASSIFIERS*(MAX_FILE_NAME_LEN+100)]; 
  long stext_maxlen = MAX_PATTERN+MAX_CLASSIFIERS*(MAX_FILE_NAME_LEN+100);
  double decision = 0;
  
  long totalfeatures = 0;   //  total features
  long bestseen;
  double ptc[MAX_CLASSIFIERS]; // current running probability of this class
  long hashlens[MAX_CLASSIFIERS];
  char *hashname[MAX_CLASSIFIERS];
  long doc_num[MAX_CLASSIFIERS];
  

  internal_trace = 0;
  user_trace = 0;

  //            extract the optional "match statistics" variable
  //
  crm_get_pgm_arg (svrbl, MAX_PATTERN, apb->p2start, apb->p2len);
  svlen = apb->p2len;
  svlen = crm_nexpandvar (svrbl, svlen, MAX_PATTERN);
  { 
    long vstart, vlen;
    crm_nextword (svrbl, svlen, 0, &vstart, &vlen);
    memmove (svrbl, &svrbl[vstart], vlen);
    svlen = vlen;
    svrbl[vlen] = '\000';
  }
  
  //     status variable's text (used for output stats)
  //    
  stext[0] = '\000';
  slen = 0;
  
  //            set our cflags, if needed.  The defaults are
  //            "case" and "affirm", (both zero valued).
  //            and "microgroom" disabled.
  cflags = REG_EXTENDED;
  eflags = 0;
  sense = +1;
  if (apb->sflags & CRM_NOCASE){
    cflags = cflags | REG_ICASE;
    eflags = 1;
    if (user_trace)
      fprintf (stderr, "turning oncase-insensitive match\n");
  }
  if (apb->sflags & CRM_REFUTE){
    sense = -sense;
    /////////////////////////////////////
    //    Take this out when we finally support refutation
    ////////////////////////////////////
    //      fprintf (stderr, "Hyperspace Refute is NOT SUPPORTED YET\n");
    //return (0);
    if (user_trace)
      fprintf (stderr, " refuting learning\n");
  }
  microgroom = 0;
  if (apb->sflags & CRM_MICROGROOM){
    microgroom = 1;
    if (user_trace)
      fprintf (stderr, " enabling microgrooming.\n");
  }
  unique = 0;
  if (apb->sflags & CRM_UNIQUE){
    unique = 1;
    if (user_trace)
      fprintf (stderr, " enabling uniqueifying features.\n");
  }

  use_unigram_features = 0;
  if (apb->sflags & CRM_UNIGRAM){
    use_unigram_features = 1;
    if (user_trace)
      fprintf (stderr, " using only unigram features.\n");
  }
  
  //   Note that during a LEARN in hyperspace, we do NOT use the mmap of
  //    pre-existing memory.  We just write to the end of the file instead.
  //    malloc up the unsorted hashbucket space 
  hashes = calloc (HYPERSPACE_MAX_FEATURE_COUNT, 
		   sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT));
  hashcounts = 0;
  
  //     get the "this is a word" regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
  
  //   compile the word regex
  //
  if ( internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);
  
  i = crm_regcomp (&regcb, ptext, plen, cflags);
  
  if ( i > 0){
    crm_regerror ( i, &regcb, tempbuf, data_window_size);
    nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
    goto regcomp_failed;
  }
  
  //   Now tokenize the input text
  //   We got txtptr, txtstart, and txtlen from the caller.
  //
  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;
  
   //   init the hashpipe with 0xDEADBEEF 
  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++){
    hashpipe[h] = 0xDEADBEEF;
  }

  i = 0;
  j = 0;
  k = 0;
  //generate the sorted hashes of input text
  if(txtlen > 0){
    while (k == 0 && textoffset <= textmaxoffset 
	   && hashcounts < HYPERSPACE_MAX_FEATURE_COUNT  ){
      long wlen;
      long slen = textmaxoffset - textoffset;
      // if pattern is empty, extract non graph delimited tokens
      // directly ([[graph]]+) instead of calling regexec  (8% faster)
      if (ptext[0] != '\0'){
	k = crm_regexec (&regcb, &(txtptr[textoffset]),
			 slen, 5, match, 0, NULL);
      }else{
	k = 0;
	//         skip non-graphical characthers
	match[0].rm_so = 0;
	while (!isgraph (txtptr[textoffset + match[0].rm_so])
	       && textoffset + match[0].rm_so < textmaxoffset)
	  match[0].rm_so ++;
	match[0].rm_eo = match[0].rm_so;
	while (isgraph (txtptr [textoffset + match[0].rm_eo])
	       && textoffset + match[0].rm_eo < textmaxoffset)
	  match[0].rm_eo ++;
	if ( match[0].rm_so == match[0].rm_eo)
	  k = 1;
      }
      if (!(k != 0 || textoffset > textmaxoffset)){
	wlen = match[0].rm_eo - match[0].rm_so;
	
	memmove (tempbuf, 
		 &(txtptr[textoffset + match[0].rm_so]),
		 wlen);
	tempbuf[wlen] = '\000';
	if (internal_trace){
	  fprintf (stderr, 
		   "  Learn #%ld t.o. %ld strt %ld end %ld len %ld is -%s-\n", 
		   i, 
		   textoffset,
		   (long) match[0].rm_so, 
		   (long) match[0].rm_eo,
		   wlen,
		   tempbuf);
	}
	if (match[0].rm_eo == 0){
	  nonfatalerror ( "The LEARN pattern matched zero length! ",
			  "\n Forcing an increment to avoid an infinite loop.");
	  match[0].rm_eo = 1;
	}
	//  slide previous hashes up 1
	for (h = OSB_BAYES_WINDOW_LEN-1; h > 0; h--){
	  hashpipe [h] = hashpipe [h-1];
	}

	
	//  and put new hash into pipeline
	hashpipe[0] = strnhash ( tempbuf, wlen);
	
	//for now, only consider unigram
	//unsigned long h1= strnhash (tempbuf, wlen);	
	//fprintf(stderr, "h1 = %lud\nhashcounts = %ld\n", h1,hashcounts);
	//if (h1 == 0) h1 = 0xdeadbeef;
	//hashes[hashcounts].hash = h1;
	//hashcounts++;
	
	//  and account for the text used up.
	textoffset = textoffset + match[0].rm_eo;
	i++;
	//        is the pipe full enough to do the hashing?
      if (1)   //  we init with 0xDEADBEEF, so the pipe is always full (i >=5)
	{
	  int j;
	  unsigned th=0;          //  a counter used only in TSS hashing
	  unsigned long hindex;
	  unsigned long h1; //, h2;
	  //
	  th = 0;
	  //
	  if (use_unigram_features == 1){
	    h1 = hashpipe[0];
	    if (h1 == 0) h1 = 0xdeadbeef;
	    if  (internal_trace)
	      fprintf (stderr, "Singleton feature : %lud\n", h1);
	    hashes[hashcounts].hash = h1;
	    hashcounts++;
	  }
	  else{
	    for (j = 1; 
		 j < OSB_BAYES_WINDOW_LEN; //OSB_BAYES_WINDOW_LEN;
		 j++)
	      {
		h1 = hashpipe[0]*hctable[0] + hashpipe[j] * hctable[j<<1];
		if (h1 == 0) h1 = 0xdeadbeef;
		//		  h2 = hashpipe[0]*hctable[1] + hashpipe[j] * hctable[(j<<1)-1];
		//if (h2 == 0) h2 = 0xdeadbeef;
		hindex = h1;
		
		if (internal_trace)
		  fprintf (stderr, "Polynomial %d has h1:%lud \n",
			   j, h1);
		
		hashes[hashcounts].hash = h1;
		//	      unk_hashes[unk_hashcount].key = h2;
		hashcounts++;
	      }
	  }
	}  
      } else{
	if (ptext[0] != '\0') crm_regfree (&regcb);
	k = 1;
      }
    }   //   end the while k==0
    //   Now sort the hashes array.
    //
    //hashcounts--;
   
    qsort (hashes, hashcounts, 
	   sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT),
	   &hash_compare);
    //mark the end of a feature vector
    hashes[hashcounts].hash = 0;
    if (user_trace)
      fprintf (stderr, "Total hashes generated: %ld\n", hashcounts);
    
    //   And uniqueify the hashes array
    //
    i = 0;
    j = 0;
    
    if (unique){
      while ( i <= hashcounts ){
	if (hashes[i].hash != hashes[i+1].hash){
	  hashes[j]= hashes[i];
	  j++;
	}
	i++;
      }
      hashcounts = j;
    }
    if (user_trace)
      fprintf (stderr, "Unique hashes generated: %ld\n", hashcounts);
    totalfeatures = hashcounts;
  }else{
    fprintf(stderr, "please input the text you want to classify! Try it again!\n");
    return 0;
  }
  
  if (user_trace){
    fprintf(stderr,"sorted hashes:\n");
    for(i=0;i<hashcounts;i++){
      fprintf(stderr, "hashes[%ld]=%lud\n",i,hashes[i].hash);
    }
    fprintf (stderr, "Total hashes generated: %ld\n", hashcounts);
  }

  // extract the file names.( file1.svm | file2.svm | 1vs2_solver.svm )
  crm_get_pgm_arg (ftext, MAX_PATTERN, apb->p1start, apb->p1len);
  flen = apb->p1len;
  flen = crm_nexpandvar (ftext, flen, MAX_PATTERN);
  
  strcpy(ptext,"[[:space:]]*([[:graph:]]+)[[:space:]]+\\|[[:space:]]+([[:graph:]]+)[[:space:]]+\\|[[:space:]]+([[:graph:]]+)[[:space:]]*");
  plen = strlen(ptext);   
  i = crm_regcomp (&regcb, ptext, plen, cflags);
  if ( i > 0){
    crm_regerror ( i, &regcb, tempbuf, data_window_size);
    nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
    goto regcomp_failed;
  }  
  k = crm_regexec (&regcb, ftext,
		   flen, 5, match, 0, NULL);
  if( k==0 ){
	long file1_hashlens; /* [i_a] this is C, not C++ */
    HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
    long file2_hashlens;
    HYPERSPACE_FEATUREBUCKET_STRUCT *file2_hashes;
    int k1, k2, k3;

    //get three input files.
    memmove(file1,&ftext[match[1].rm_so],(match[1].rm_eo-match[1].rm_so));
    file1[match[1].rm_eo-match[1].rm_so]='\000';
    memmove(file2,&ftext[match[2].rm_so],(match[2].rm_eo-match[2].rm_so));
    file2[match[2].rm_eo-match[2].rm_so]='\000';
    memmove(file3,&ftext[match[3].rm_so],(match[3].rm_eo-match[3].rm_so));
    file3[match[3].rm_eo-match[3].rm_so]='\000';
    if(internal_trace)
      fprintf(stderr, "file1=%s\tfile2=%s\tfile3=%s\n", file1, file2, file3);
    
    //open all files,first check whether file3 is the current version solution.
    /*
	long file1_hashlens; ** [i_a] this is C, not C++ **
    HYPERSPACE_FEATUREBUCKET_STRUCT *file1_hashes;
    long file2_hashlens;
    HYPERSPACE_FEATUREBUCKET_STRUCT *file2_hashes;
    int k1, k2, k3;
	*/
    k1 = stat (file1, &statbuf1);
    k2 = stat (file2, &statbuf2);
    k3 = stat (file3, &statbuf3);
     
    if (k1 != 0) { 
      nonfatalerror ("Refuting from nonexistent data cannot be done!"
		     " More specifically, this data file doesn't exist: ",
		     file1);
      return (0);
    }else if(k2 != 0){
      nonfatalerror ("Refuting from nonexistent data cannot be done!"
		     " More specifically, this data file doesn't exist: ",
		     file2);
      return (0);
    }else{
      int temp_k1; /* [i_a] this is C, not C++ */
	  int temp_k2; /* [i_a] this is C, not C++ */
      int *y = NULL; /* [i_a] this is C, not C++ */
      HYPERSPACE_FEATUREBUCKET_STRUCT **x = NULL; /* [i_a] this is C, not C++ */
	  double *deci_array = NULL;  /* [i_a] this is C, not C++ */
 
	  k1 = 0;
      k2 = 0;
      
      file1_hashlens = statbuf1.st_size;
      crm_force_munmap_filename (file1);
      crm_force_munmap_filename (file2);
    
      file1_hashes = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
	crm_mmap_file (file1,
		       0, file1_hashlens,
		       PROT_READ | PROT_WRITE,
		       MAP_SHARED,
		       NULL);
      file1_hashlens = file1_hashlens 
	/ sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT );
      
      hashlens[0] = file1_hashlens;
      hashname[0] = (char *) malloc ((strlen(file1)+10) * sizeof(hashname[0][0])); /* [i_a] */
      if (!hashname[0])
	untrappableerror(
			 "Couldn't malloc hashname[0]\n","We need that part later, so we're stuck.  Sorry.");
      strcpy(hashname[0],file1);


      file2_hashlens = statbuf2.st_size;
      file2_hashes = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
	crm_mmap_file (file2,
		       0, file2_hashlens,
		       PROT_READ | PROT_WRITE,
		       MAP_SHARED,
		       NULL);
      file2_hashlens = file2_hashlens 
	/ sizeof (HYPERSPACE_FEATUREBUCKET_STRUCT );
      hashlens[1] = file2_hashlens;
      hashname[1] = (char *) malloc (strlen(file2)+10);
      if (!hashname[1])
	untrappableerror(
			 "Couldn't malloc hashname[1]\n","We need that part later, so we're stuck.  Sorry.");
      strcpy(hashname[1],file2);

      //find out how many documents in file1 and file2 separately
      for(i = 0;i< file1_hashlens;i++){
	if(yimin_trace)
	  fprintf (stderr, "\nThe %ldth hash value in file1 is %lud", i, file1_hashes[i].hash);
	if(file1_hashes[i].hash == 0){
	  k1 ++;
	}
      }
      if(yimin_trace)
	fprintf (stderr, "\nThe total number of documents in file1 is %d\n", k1);
      
      for(i = 0;i< file2_hashlens;i++){
	if(yimin_trace)
	  fprintf (stderr, "\nThe %ldth hash value in file2 is %lud", i, file2_hashes[i].hash);
	if(file2_hashes[i].hash == 0){
	  k2 ++;
	}
      }
      
      if(yimin_trace)
	fprintf (stderr, "\nThe total number of documents in file2 is %d\n", k2);
      hashf = fopen ( file3 , "rb+"); /* [i_a] on MSwin/DOS, fopen() opens in CRLF text mode by default; this will corrupt those binary values! */
      temp_k1 = 0; temp_k2 = 0; /* [i_a] this is C, not C++ */
      if(k3 == 0){ 
	//hashf = fopen ( file3 , "r+");
	fread(&temp_k1, sizeof(int), 1, hashf);
	fread(&temp_k2, sizeof(int), 1, hashf);
	//fscanf(hashf,"%d\t%d", &temp_k1, &temp_k2);
	if (yimin_trace)
	  fprintf(stderr, "temp_k1=%d\ttemp_k2=%d\n",temp_k1,temp_k2);
      }
      doc_num[0] = k1;
      doc_num[1] = k2;
      //assign svm_prob.x, svm_prob.y
      svm_prob.l = k1 + k2;
      y = malloc(svm_prob.l * sizeof(y[0])); /* [i_a] this is C, not C++ */
      for(i = 0; i < k1; i++)
	y[i] = 1;
      for(i = k1; i < svm_prob.l; i++)
	y[i] = -1;
      svm_prob.y = y;
      x = malloc(svm_prob.l * sizeof(x[0])); /* [i_a] this is C, not C++ */
      x[0] = &(file1_hashes[0]);
      k = 1;
      for(i = 1;i< file1_hashlens - 1;i++){
	if(file1_hashes[i].hash == 0){
	  x[k++] = &(file1_hashes[i+1]);
	}
      }
      x[k++] = &(file2_hashes[0]);
      for(i = 1;i< file2_hashlens - 1;i++){
	if(file2_hashes[i].hash == 0){
	  x[k++] = &(file2_hashes[i+1]);
	}
      }
      svm_prob.x = x;

      alpha = malloc((k1 + k2) * sizeof(alpha[0])); /* [i_a] */

      if((k3 != 0) || (temp_k1 != k1) || (temp_k2 != k2)){
	fprintf(stderr, "temp_k1=%d\ttemp_k2=%d\tSVM solution file is not up-to-date! we'll recalculate it!\n", temp_k1, temp_k2);
	//recalculate the svm solution
	if((k1 > 0) && (k2 >0)){
	  //           extract parameters for svm 
	  crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s2start, apb->s2len);
	  plen = apb->s2len;
	  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
	  if(plen){
	    //set default parameters for SVM
	    param.svm_type = C_SVC;
	    param.kernel_type = LINEAR;
	    param.cache_size = 1;//MB
	    param.eps = 1e-3;
	    param.C = 1;
	param.nu = 0.5;
	    param.max_run_time = 1; //second
	    param.shrinking = 1;//not available right now

	    //sscanf(ptext, "%d %d %lf %lf %lf %d",&(param.svm_type), &(param.kernel_type), &(param.cache_size), &(param.eps), &(param.C), &(param.shrinking));
	    if (8 != sscanf(ptext, "%d %d %lf %lf %lf %lf %lf %d",&(param.svm_type), &(param.kernel_type), &(param.cache_size), &(param.eps), &(param.C), &(param.nu), &(param.max_run_time) , &(param.shrinking)))
		  {
			  nonfatalerror("Failed to decode the 8 SVM setup parameters [classify]: ", ptext);
		  }
	  }else{
	    //set default parameters for SVM
	    param.svm_type = C_SVC;
	    param.kernel_type = LINEAR;
	    param.cache_size = 1;//MB
	    param.eps = 1e-3;
	    param.C = 1;
	param.nu = 0.5;
	    param.max_run_time = 1; //second
	    param.shrinking = 1;//not available right now
	  }
	  if(yimin_trace){
	    for(i = 0;i< k;i++){
	      fprintf(stderr, "\nx[%ld]=%lud\n",i,x[i][1].hash);
	    }       
	  }
	  Q_init();
	  solve(); //result is in solver
	  b = calc_b();
	  if(yimin_trace){
	    fprintf(stderr, "b=%f\n",b);
	  }
	  //alpha = (double *)malloc((k1 + k2) * sizeof(double));
	assert(alpha != NULL);
	assert(svm_prob.l <= (k1 + k2));
	  for(i = 0; i < svm_prob.l; i++)
	    alpha[i] = solver.alpha[i];
	  //free cache
	  cache_free(&svmcache);
	  //compute A,B for sigmoid prediction
	  deci_array = (double*) malloc(svm_prob.l * sizeof(deci_array[0])); /* [i_a] this is C, not C++ */
	  for(i = 0; i < svm_prob.l; i++){
	    deci_array[i] = calc_decision(svm_prob.x[i], alpha, b);
	  }
	  calc_AB(AB, deci_array, k1, k2);
	  //decision = sigmoid_predict(decision, AB[0], AB[1]);
	  //free(hashes);
	  free(deci_array);
	  deci_array = NULL;
	  // free(alpha);
	  if(user_trace)
	    fprintf(stderr, "Recalculation of svm hyperplane is finished!\n");
	}else{
	  if(user_trace)
	    fprintf(stderr, "There hasn't enough documents to recalculate a svm hyperplane!\n");
	  return (0);
	}	
      }else{

	if(yimin_trace){
	  for(i=0;i<svm_prob.l;i++){
	    j = 0;
	    do{fprintf(stderr, "x[%ld][%ld]=%lud\n",i,j,svm_prob.x[i][j].hash);}while(svm_prob.x[i][j++].hash!=0);
	  }
	}
	//alpha = (double *)malloc((k1 + k2) * sizeof(double));
	assert(alpha != NULL);
	for(i = 0; i<(k1 + k2); i++){
	  fread(&alpha[i], sizeof(double), 1, hashf);     
	}
	fread(&b, sizeof(double), 1, hashf);
	fread(&AB[0], sizeof(double), 1, hashf);
	fread(&AB[1], sizeof(double), 1, hashf);
	fclose(hashf);
      }
      decision = calc_decision(hashes,alpha,b);
      decision = sigmoid_predict(decision, AB[0], AB[1]);
      //free(hashes);
      free(alpha);
      alpha = NULL; /* [i_a] */
    }//end (k1==0 && k2 ==0)
  }//end (k==0)
  else{
    fprintf(stderr, "You need to input (file1.svm | file2.svm| f1vsf2.svmhyp)\n");
  }
  free(hashes);
  hashes = NULL;
  
  
  if(svlen > 0){
    double pr; /* [i_a] this is C, not C++ */
    char fname[MAX_FILE_NAME_LEN]; /* [i_a] this is C, not C++ */
    char buf [4096];
    buf[0] = 0;
    
    //   put in standard CRM114 result standard header:
    /* double pr; ** [i_a] this is C, not C++ */
    ptc[0] = decision;
    ptc[1] = 1 - decision;
    if(decision >= 0.5)
      {
	pr = 10 * (log10 (decision + 1e-300) - log10 (1.0 - decision +1e-300 ));
	sprintf(buf, 
		"CLASSIFY succeeds; success probability: %6.4f  pR: %6.4f\n", 
		decision, pr);
	bestseen = 0;
      }
    else 
      {      
	pr = 10 * (log10 (decision + 1e-300) - log10 (1.0 - decision +1e-300 ));
	sprintf(buf, 
		"CLASSIFY fails; success probability: %6.4f  pR: %6.4f\n", 
		decision, pr);
	bestseen = 1;
      }
    if (strlen (stext) + strlen(buf) <= stext_maxlen)
      strcat (stext, buf);
    
    //   Second line of the status report is the "best match" line:
    //
    //    Here's a template to use
    /*    snprintf (buf, NUMBEROF(buf), "Best match to file #%ld (%s) prob: %6.4f  pR: %6.4f  \n",
	     bestseen,
	     hashname[bestseen],
	     ptc[bestseen],
	     10 * (log10(ptc[bestseen]) - log10(remainder)));
		 buf[NUMBEROF(buf) - 1] = 0;
    */
    /* char fname[MAX_FILE_NAME_LEN]; ** [i_a] this is C, not C++ */
    if(bestseen)
      strncpy(fname, file2, MAX_FILE_NAME_LEN);
    else
      strncpy(fname, file1, MAX_FILE_NAME_LEN);
	fname[MAX_FILE_NAME_LEN - 1] = 0;
    snprintf (buf, NUMBEROF(buf), "Best match to file #%ld (%s) "	\
	     "prob: %6.4f  pR: %6.4f  \n",
	     bestseen,
	     fname,
	     ptc[bestseen],
	     10 * (log10 (ptc[bestseen] + 1e-300) - log10 (1.0 - ptc[bestseen] +1e-300 )));
	buf[NUMBEROF(buf) - 1] = 0;
    if (strlen (stext) + strlen(buf) <= stext_maxlen)
      strcat (stext, buf);
    
    sprintf (buf, "Total features in input file: %ld\n", totalfeatures); 
    if (strlen (stext) + strlen(buf) <= stext_maxlen)
      strcat (stext, buf);
    for(k = 0; k < 2; k++){
      snprintf (buf, NUMBEROF(buf),
	       "#%ld (%s):"						\
	       " documents: %ld, features: %ld,  prob: %3.2e, pR: %6.2f \n", 
	       k,
	       hashname[k],
	       doc_num[k],
	       hashlens[k],
	       ptc[k], 
	       10 * (log10 (ptc[k] + 1e-300) - log10 (1.0 - ptc[k] + 1e-300) )  );
	  buf[NUMBEROF(buf) - 1] = 0;
      
      if (strlen(stext)+strlen(buf) <= stext_maxlen)
	strcat (stext, buf);
    }
    
    for(k = 0; k < 2; k++){
      free(hashname[k]);
    }

    //   finally, save the status output
    //
    crm_destructive_alter_nvariable (svrbl, svlen, 
				     stext, strlen (stext));
  }
  
  //    Return with the correct status, so an actual FAIL or not can occur.
  if (decision >= 0.5 )
    {
      //   all done... if we got here, we should just continue execution
      if (user_trace)
        fprintf (stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
    }
  else
    {
      //  Classify was a fail.  Do the skip.
      if (user_trace)
        fprintf (stderr, "CLASSIFY was a FAIL, skipping forward.\n");
      //    and do what we do for a FAIL here
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk [csl->mct[csl->cstmt]->nest_level] = -1;
      return (0);
    }
 regcomp_failed:
  return (0); 
  
}

#ifdef yimin
//swap two documents in CACHE
static void cache_swap(CACHE *svmcache, int i, int j){
  double *temp;   /* [i_a] this is C, not C++ */
  int templen;   /* [i_a] this is C, not C++ */
  CACHE_NODE *headi , *headj;   /* [i_a] this is C, not C++ */
  CACHE_NODE *tempnode = NULL;   /* [i_a] this is C, not C++ */
  
  if(i == j)
    return;
  
  assert(i >= 0);
  assert(j >= 0);
  assert(i < svmcache->l);
  assert(j < svmcache->l);
  headi = svmcache->head + i;
  headj = svmcache->head + j;
  if(headi->len) lru_delete(headi);
  if(headj->len) lru_delete(headj);
  temp = headi->data;   /* [i_a] this is C, not C++ */
  headi->data = headj->data;
  headj->data = temp;
  templen = headi->len;   /* [i_a] this is C, not C++ */
  headi->len = headj->len;
  headj->len = templen;
  if(headi->len) lru_insert(headi, svmcache);
  if(headj->len) lru_insert(headj, svmcache);
  if(i > j){
    templen = i;
    i = j;
    j = templen;
  }
  /* CACHE_NODE *tempnode;   ** [i_a] this is C, not C++ */
  for(tempnode = (svmcache->lru_headnode).next; tempnode != &(svmcache->lru_headnode); tempnode = tempnode->next ){
	float tempfloat;   /* [i_a] this is C, not C++ */
    
	if(tempnode->len > i){
      if(tempnode->len > j){
	tempfloat = tempnode->data[i];   /* [i_a] this is C, not C++ */
	tempnode->data[i] = tempnode->data[j];
	tempnode->data[j] = tempfloat;
      }else{
	lru_delete(tempnode);
	free(tempnode->data);
	svmcache->size += tempnode->len;
	tempnode->data = 0;
	tempnode->len = 0;
      }
    }      
  }
}



//swap two documents. This includes swap CACHE, svm_prob.x, svm_prob.y and DiagQ
static void swap(int i, int j){
  if(i != j){
    if(1){
      HYPERSPACE_FEATUREBUCKET_STRUCT *temp = svm_prob.x[i];
      svm_prob.x[i] = svm_prob.x[j];
      svm_prob.x[j] = temp;
    }
    if(1){
      float temp = svm_prob.y[i];
      svm_prob.y[i] = svm_prob.y[j];
      svm_prob.y[j] = temp;
    }
    if(1){
      double temp = DiagQ[i];
      DiagQ[i] = DiagQ[j];
      DiagQ[j] = temp;
    }
    cache_swap(&svmcache, i, j);
  }
}

#endif
