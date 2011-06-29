//  crm_expr_clump.c

//  by Joe Langeway derived from crm_bit_entropy.c and produced for the crm114 so:
//  
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.  
//
//  Other licenses may be negotiated; contact Bill for details.  
//
/////////////////////////////////////////////////////////////////////
//
//     crm_expr_clump.c - automatically cluster unlabelled documents.
//    
//     Original spec by Bill Yerazunis, original code by Joe Langeway,
//     recode for CRM114 use by Bill Yerazunis. 
//
//     This code section (crm_expr_clump and subsidiary routines) is
//     dual-licensed to both William S. Yerazunis and Joe Langeway,
//     including the right to reuse this code in any way desired,
//     including the right to relicense it under any other terms as
//     desired.
//
//////////////////////////////////////////////////////////////////////

/*
  This file is part of on going research and should not be considered
  a finished product, a reliable tool, an example of good software
  engineering, or a reflection of any quality of Joe's besides his
  tendancy towards long hours. 
  
  
  Here's what's going on:

  Documents are fed in with calls to "clump" and the distance between each
  document is recorded in a matrix. We then find clusters for automatic 
  classification without the need for a gold standard judgement ahead of time.

  Cluster assignments start at index 1 and negative numbers indicate perma
assignments made by crm.

*/

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

//    the command line argc, argv
extern int prog_argc;
extern char **prog_argv;

//    the auxilliary input buffer (for WINDOW input)
extern char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used 
//    wherever needed.  These are sized to the same size as the data window.
// do not mut these or random binary shall be shat upon thee
extern char *inbuf;
extern char *outbuf;
extern char *tempbuf;

#define MAX_CLUSTERS 4096
#define CLUSTER_LABEL_LEN 32
#define DOCUMENT_TAG_LEN 32

typedef struct mythical_clumper_header
{
  long max_documents, n_documents;
  long document_offsets_offset;//list of offsets to documents
  long clusters_offset; //cluster assignments of documents
  long distance_matrix_offset;
  long cluster_labels_offset;
  long document_tags_offset;
  long n_perma_clusters;
  long file_length; //is the offset of new files when learning
  long last_action; //0 = made clumps, non-zero means make clumps if you've got
                        // a chance and we're told not to
  long n_clusters;
} CLUMPER_HEADER_STRUCT;

typedef struct mythical_clumper_state
{
  char *file_origin;
  CLUMPER_HEADER_STRUCT *header;
  long *document_offsets;
  long *cluster_assignments;
  float *distance_matrix;
  char (*cluster_labels)[CLUSTER_LABEL_LEN];
  char (*document_tags)[DOCUMENT_TAG_LEN];
} CLUMPER_STATE_STRUCT;

// tracing for this module
int joe_trace = 0;

long max_documents = 1000;

static void make_new_clumper_backing_file(char *filename)
{
  CLUMPER_HEADER_STRUCT H, *h = &H;
  FILE *f;
  long i;
  h->max_documents = max_documents;
  h->n_documents = 0;
  h->document_offsets_offset = sizeof(CLUMPER_HEADER_STRUCT);
  h->clusters_offset = h->document_offsets_offset +
                          sizeof(long) * max_documents;
  h->distance_matrix_offset = h->clusters_offset +
                          sizeof(long) * max_documents;
  h->cluster_labels_offset = h->distance_matrix_offset + ( sizeof(float) *
               max_documents * (max_documents + 1) / 2);
  h->document_tags_offset = h->cluster_labels_offset + 
                           ( sizeof(char) * max_documents * CLUSTER_LABEL_LEN);
  h->file_length = h->document_tags_offset + 
                           ( sizeof(char) * max_documents * DOCUMENT_TAG_LEN);
  h->n_perma_clusters = 0;
  h->n_clusters = 0;
  crm_force_munmap_filename(filename);
  f = fopen(filename, "wb");
  fwrite(h, 1, sizeof(CLUMPER_HEADER_STRUCT), f);
  i = h->file_length - sizeof(CLUMPER_HEADER_STRUCT);
  if(joe_trace)
    fprintf(stderr, "about to write %ld zeros to backing file\n", i);
  while(i--)
    fputc('\0', f);
  fflush(f);
  fclose(f);
  if(joe_trace)
    fprintf(stderr, "Just wrote backing file for clumper size %ld\n",
        h->file_length);
}

static int map_file(CLUMPER_STATE_STRUCT *s, char *filename)
{
  struct stat statee; 
  if(stat(filename, &statee))
  {
    nonfatalerror("Couldn't stat file!", filename);
    return -1;
  }
 
  s->file_origin = crm_mmap_file
            (filename,
             0, statee.st_size,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             NULL);
  if(s->file_origin == MAP_FAILED)
  {
    nonfatalerror("Couldn't mmap file!", filename);
    return -1;
  }
  if(joe_trace)
    fprintf(stderr,"Definately think I've mapped a file.\n");
  
  s->header = (CLUMPER_HEADER_STRUCT *)(s->file_origin);
  s->document_offsets =
      (void *)( s->file_origin + s->header->document_offsets_offset );
  s->cluster_assignments =
      (void *)( s->file_origin + s->header->clusters_offset );
  s->distance_matrix =
      (void *)( s->file_origin + s->header->distance_matrix_offset );
  s->cluster_labels = 
      (void *)( s->file_origin + s->header->cluster_labels_offset );
  s->document_tags =
      (void *)( s->file_origin + s->header->document_tags_offset );
  return 0;
}

static void unmap_file(CLUMPER_STATE_STRUCT *s)
{
  crm_munmap_file ((void *) s->file_origin);
}

static float *aref_dist_mat(float *m, int j, int i)
{
  if(i < j) {int t = i; i = j; j = t;}
  return m + i * (i - 1) / 2 + j;
}

static float get_document_affinity(long *doc1, long *doc2)
{
  int u = 0, l1 = 0, l2 = 0;
  for(;;)
    if(doc1[l1] == 0)
    {
        while(doc2[l2] != 0) l2++;
        break;
    }
    else if(doc2[l2] == 0)
    {
        while(doc1[l1] != 0) l1++;
        break;
    }
    else if(doc1[l1] == doc2[l2])
    {
      u++; l1++; l2++;
    }
    else if(doc1[l1] < doc2[l2])
        l1++;
    else if(doc1[l1] > doc2[l2])
        l2++;
    else
    {
      fprintf(stderr, "panic in the disco!  ");
      break;
    }
  if(joe_trace)
        fprintf(stderr, "Just compared two documents u=%d l1=%d l2=%d\n",
                 u, l1, l2);
  return pow((double)(1.0 + u * u) / (double)(1.0 + l1 * l2), 0.2);
}

static int compare_longs(const void *a, const void *b)
{
  if(*(long *)a < *(long *)b)
    return -1;
  if(*(long *)a > *(long *)b)
    return 1;
  return 0;
}

static int eat_document
        (       char *text, long text_len, long *ate,
                regex_t *regee,
                long *feature_space, long max_features,
                long long flags)
{
  long n_features = 0, i, j;
  long hash_pipe[OSB_BAYES_WINDOW_LEN];
  long hash_coefs[] = { 1, 3, 5, 11, 23, 47};
  regmatch_t match[1];
  char *t_start;
  long t_len;
  long f;
  int unigram, unique, string;
   
  unique = apb->sflags & CRM_UNIQUE;
  unigram = apb->sflags & CRM_UNIGRAM;
  string = apb->sflags & CRM_STRING;
  
  if(string)
    unique = 1;
  
  *ate = 0;
  
  for(i = 0; i < OSB_BAYES_WINDOW_LEN; i++)
    hash_pipe[i] = 0xdeadbeef;
  while(text_len > 0 && n_features < max_features - 1)
  {
    if(crm_regexec (regee, text, text_len, 1, match, 0, NULL))
      //no match or regex error, we're done
      break;
    else
    {
      t_start = text + match->rm_so;
      t_len = match->rm_eo - match->rm_so;
      if(string)
      {
        text += match->rm_so + 1;
        text_len -= match->rm_so + 1;
        *ate += match->rm_so + 1;
      }else
      {
        text += match->rm_eo;
        text_len -= match->rm_eo;
        *ate += match->rm_eo;
      }

      for(i = OSB_BAYES_WINDOW_LEN - 1; i > 0; i--)
        hash_pipe[i] = hash_pipe[i - 1];
      hash_pipe[0] = strnhash(t_start, t_len);
    }  
    f = 0;
    if(unigram)
      feature_space[n_features++] = hash_pipe[0];
    else
      for(i = 1; i < OSB_BAYES_WINDOW_LEN && hash_pipe[i] != 0xdeadbeef; i++)
        feature_space[n_features++] = 
                hash_pipe[0] + hash_pipe[i] * hash_coefs[i];

  }
  qsort(feature_space, n_features, sizeof(long), compare_longs);
  
  if(unique)
  {
    i = 0; j = 0;
    for(j = 0; j < n_features; j++)
      if(feature_space[i] != feature_space[j])
        feature_space[++i] = feature_space[j];
  feature_space[++i] = 0;
  n_features = i + 1; //the zero counts
  } else
    feature_space[n_features++] = 0;
  return n_features;
}


static long find_closest_document
          (CLUMPER_STATE_STRUCT *s,
          char *text, long text_len,
          regex_t *regee,
         long long flags)
{
  long feature_space[32768], n, i, b = -1;
  float b_s = 0.0, n_s;
  n = eat_document(text, text_len, &i,
                    regee, feature_space, 32768,
                    flags);
  for(i = 0; i < s->header->n_documents; i++)
  {
    n_s = get_document_affinity
           (feature_space, (long *)(s->file_origin + s->document_offsets[i]));
    if(n_s > b_s)
    {
      b = i;
      b_s = n_s;
    }
  }
  return b;
}

typedef struct mythical_cluster_head
{
  struct mythical_cluster_head *head, *next_head, *prev_head, *next;
  long count;
} CLUSTER_HEAD_STRUCT;

static void join_clusters(CLUSTER_HEAD_STRUCT *a, CLUSTER_HEAD_STRUCT *b)
{
  if(joe_trace)
    fprintf(stderr, "Joining clusters of sizes %ld and %ld\n,",
        a->head->count, b->head->count);
  
  while(a->next) a = a->next;
  b = b->head;
  a->next = b;
  a->head->count += b->count;
  b->count = 0; //though we wont actually touch this value anymore
  if(b->prev_head)
    b->prev_head->next_head = b->next_head;
  if(b->next_head)
    b->next_head->prev_head = b->prev_head;
  b->next_head = NULL;
  b->prev_head = NULL;
  do
    b->head = a->head;
  while( (b = b->next) );
}

static void agglomerative_averaging_cluster(CLUMPER_STATE_STRUCT *s, long goal)
{
  long i, j, k, l, n = s->header->n_documents;
  CLUSTER_HEAD_STRUCT *clusters = malloc(n * sizeof(CLUSTER_HEAD_STRUCT)),
                        *a, *b, *c, first_head_ptr;
  float *M = malloc( (n * (n + 1) / 2 - 1) * sizeof(float)), d, e;
  long ck, cl, ckl;

  if(joe_trace)
    fprintf(stderr, "agglomerative averaging clustering...\n");
  
  for(i = 1; i < s->header->n_documents - 1; i++)
  {
    clusters[i].head = &clusters[i];
    clusters[i].prev_head = &clusters[i - 1];
    clusters[i].next_head = &clusters[i + 1];
    clusters[i].next = NULL;
    clusters[i].count = 1;
  }
  clusters[0].head = &clusters[0];
  clusters[0].prev_head = &first_head_ptr;
  clusters[0].next_head = &clusters[1];
  clusters[0].next = NULL;
  clusters[0].count = 1;
  if(s->header->n_documents > 1) //don't muck the first one!
  {
    clusters[s->header->n_documents-1].head = 
                                &clusters[s->header->n_documents - 1];
    clusters[s->header->n_documents - 1].prev_head =
                                &clusters[s->header->n_documents - 2];
    clusters[s->header->n_documents - 1].next = NULL;
    clusters[s->header->n_documents - 1].count = 1;
  }
  //always make sure the chain ends
  clusters[s->header->n_documents - 1].next_head = NULL;
  
  first_head_ptr.next_head = &clusters[0];

  j = (n * (n + 1) / 2 - 1);
  for(i = 0; i < j; i++)
    M[i] = s->distance_matrix[i];

  for(a = first_head_ptr.next_head; a; a = a->next_head)
    if(s->cluster_assignments[a - clusters] < 0)
      for(b = a->next_head; b; b = b->next_head)
        if(s->cluster_assignments[a - clusters]
            == s->cluster_assignments[b - clusters])
        {
          k = a - clusters;
          l = b - clusters;
          ck = clusters[k].count;
          cl = clusters[l].count;
          ckl = ck + cl;
      
          for(c = &clusters[0]; c; c = c->next_head)
          {
            i = c - clusters;
            if(i == k || i == l)
              continue;
            *aref_dist_mat(M, k, i) = (ck * *aref_dist_mat(M, k, i) +
                                      cl * *aref_dist_mat(M, l, i) ) / ckl;
            *aref_dist_mat(M, l, i) = 0.0;
          }
          join_clusters(&clusters[k], &clusters[l]);
          n--;
        }
    
  while(n > goal)
  {
    l = 0; k = 0;
    d = -1.0;
    for(a = first_head_ptr.next_head; a; a = a->next_head)
      for(b = a->next_head; b; b = b->next_head)
      {
        i = a - clusters;
        j = b - clusters;
        if(s->cluster_assignments[i] < 0 && s->cluster_assignments[j] < 0)
          e = *aref_dist_mat(M, i, j) = -1000000000.0;
        else
          e = *aref_dist_mat(M, i, j);
        if( e > d )
        {
          if(s->cluster_assignments[j] < 0)
          {
            k = j;
            l = i;
          } else
          {
            k = i;
            l = j;
          }
          d = e;
        }
      }
    if(l == 0 && k == 0)
    {
      fprintf(stderr, "CLUMP FAILED TO JOIN ENOUGH CLUMPS!\n");
      break;
    }
    ck = clusters[k].count;
    cl = clusters[l].count;
    ckl = ck + cl;

    for(a = &clusters[0]; a; a = a->next_head)
    {
      i = a - clusters;
      if(i == k || i == l)
        continue;
      *aref_dist_mat(M, k, i) = (ck * *aref_dist_mat(M, k, i) +
                                 cl * *aref_dist_mat(M, l, i) ) / ckl;
      *aref_dist_mat(M, l, i) = 0.0;
    }
    join_clusters(&clusters[k], &clusters[l]);
    n--;
  }

  i = s->header->n_perma_clusters + 1;
  for(a = &clusters[0]; a; a = a->next_head)
  {
    if(s->cluster_assignments[a - clusters] < 0)
      j = -s->cluster_assignments[a - clusters];
    else
      j = i++;
    for(b = a; b; b = b->next)
      s->cluster_assignments[b - clusters] = j;
  }
  
  s->header->n_clusters = n;
  free(M);
}

static void index_to_pair(long t, long *i, long *j)
{
  long p = 2 * t;
  *i = (long)sqrt(p);
  if(*i * *i + *i > p)
    (*i)--;
  *j = t - (*i * (*i + 1) / 2);
}

static int compare_float_ptrs(const void *a, const void *b)
{
  if(**(float **)a > **(float **)b)
    return 1;
  if(**(float **)a < **(float **)b)
    return -1;
  return 0;
}

static void agglomerative_nearest_cluster(CLUMPER_STATE_STRUCT *s, long goal)
{
  long i, j, k, l, m, n = s->header->n_documents;
  CLUSTER_HEAD_STRUCT *clusters = malloc(n * sizeof(CLUSTER_HEAD_STRUCT)),
                        *a, *b, first_head_ptr;
  float **M = malloc( (n * (n + 1) / 2 - 1) * sizeof(float *));

  if(joe_trace)
    fprintf(stderr, "agglomerative nearest clustering...\n");
  
  for(i = 1; i < s->header->n_documents - 1; i++)
  {
    clusters[i].head = &clusters[i];
    clusters[i].prev_head = &clusters[i - 1];
    clusters[i].next_head = &clusters[i + 1];
    clusters[i].next = NULL;
    clusters[i].count = 1;
  }
  clusters[0].head = &clusters[0];
  clusters[0].prev_head = &first_head_ptr;
  clusters[0].next_head = &clusters[1];
  clusters[0].next = NULL;
  clusters[0].count = 1;
  if(s->header->n_documents > 1) //don't muck the first one!
  {
    clusters[s->header->n_documents - 1].head = 
                                &clusters[s->header->n_documents - 1];
    clusters[s->header->n_documents - 1].prev_head =
                                &clusters[s->header->n_documents - 2];
    clusters[s->header->n_documents - 1].next = NULL;
    clusters[s->header->n_documents - 1].count = 1;
  }
  //always make sure the chain ends
  clusters[s->header->n_documents - 1].next_head = NULL;
  
  first_head_ptr.next_head = &clusters[0];

  j = (n * (n + 1) / 2 - 1);
  for(i = 0; i < j; i++)
    M[i] = &s->distance_matrix[i];
  qsort(M, j, sizeof(float *), compare_float_ptrs);
  
  for(a = first_head_ptr.next_head; a; a = a->next_head)
    if(s->cluster_assignments[a - clusters] < 0)
      for(b = a->next_head; b; b = b->next_head)
        if(s->cluster_assignments[a - clusters]
            == s->cluster_assignments[b - clusters])
        {
          k = a - clusters;
          l = b - clusters;
          join_clusters(&clusters[k], &clusters[l]);
          n--;
        }
  i = j;  
  while(n > goal)
  {
    do
    {
      k = M[--i] - s->distance_matrix;
      index_to_pair(k, &l, &m);
    } while(clusters[m].head == clusters[l].head);
    
    join_clusters(&clusters[m], &clusters[l]);
    n--;
  }

  i = s->header->n_perma_clusters + 1;
  for(a = &clusters[0]; a; a = a->next_head)
  {
    if(s->cluster_assignments[a - clusters] < 0)
      j = -s->cluster_assignments[a - clusters];
    else
      j = i++;
    for(b = a; b; b = b->next)
      s->cluster_assignments[b - clusters] = j;
  }
  free(M);
  s->header->n_clusters = n;
}

double square(double a) {return a * a;}
double minf(double a, double b) {return a < b ? a : b;}

#define H_BUCKETS 50
static void thresholding_average_cluster(CLUMPER_STATE_STRUCT *s)
{
  long i, j, k, l, ck, cl, ckl, n = s->header->n_documents;
  CLUSTER_HEAD_STRUCT *clusters = malloc(n * sizeof(CLUSTER_HEAD_STRUCT)),
                        *a, *b, *c, first_head_ptr;
  long H[H_BUCKETS], C[H_BUCKETS];
  float A[H_BUCKETS], t_A, t, t_score, scoro,gM;
  float min, max, scale;
  float *M = malloc( (n * (n + 1) / 2 - 1) * sizeof(float)), d, e;

  if(joe_trace)
    fprintf(stderr, "threshold average clustering...\n");

  for(i = 1; i < s->header->n_documents - 1; i++)
  {
    clusters[i].head = &clusters[i];
    clusters[i].prev_head = &clusters[i - 1];
    clusters[i].next_head = &clusters[i + 1];
    clusters[i].next = NULL;
    clusters[i].count = 1;
  }
  clusters[0].head = &clusters[0];
  clusters[0].prev_head = &first_head_ptr;
  clusters[0].next_head = &clusters[1];
  clusters[0].next = NULL;
  clusters[0].count = 1;
  if(s->header->n_documents > 1) //don't muck the first one!
  {
    clusters[s->header->n_documents-1].head = 
                                &clusters[s->header->n_documents - 1];
    clusters[s->header->n_documents - 1].prev_head =
                                &clusters[s->header->n_documents - 2];
    clusters[s->header->n_documents - 1].next = NULL;
    clusters[s->header->n_documents - 1].count = 1;
  }
  //always make sure the chain ends
  clusters[s->header->n_documents - 1].next_head = NULL;
  
  first_head_ptr.next_head = &clusters[0];
  
  j = (n * (n + 1) / 2 - 1);
  for(i = 0; i < j; i++)
    M[i] = s->distance_matrix[i];

  
  for(i = 0; i < H_BUCKETS; i++)
    H[i] = 0.0;
  j = n * (n - 1) / 2;
  min = 1000000000.0;
  max = -1000000000.0;
  
  for(i = 0; i < j; i++)
  {
    if(M[i] < min)
      min = s->distance_matrix[i];
    if(M[i] > max)
      max = s->distance_matrix[i];
  }
  scale = (max - min) / ((float)H_BUCKETS - 0.1);
  t = -1.0;
  for(i = 0; i < j; i++)
    H[ (int)( (M[i] - min) / scale ) ]++;
  if(joe_trace)
  {
    fprintf(stderr, "Historgram of document distances:\n");
    for(i = 0; i < H_BUCKETS; i++)
    {
      for(k = 0; k < H[i]; k += 100)
        fputc('*', stderr);
      fputc('\n', stderr);
    }
  }    
  
  k = 0;
  t_A = 0.0;
  for(i = 0; i < H_BUCKETS; i++)
  {
    k = C[i] = H[i] + k;
    t_A = A[i] = H[i] * (min + (i + 0.5) * scale) + t_A;
  }
  gM = t_A / (float)j;
  t_score = 0.0;
  for(i = 2; i < H_BUCKETS - 2; i++)
  {
    scoro = square(gM - (t_A - A[i]) / (k - C[i])) * (k - C[i])
          + square(gM - A[i] / C[i]) * C[i];
    if(scoro > t_score)
    {
      t_score = scoro;
      t = min + scale * (float)(i );
    }
  }
  if(joe_trace)
    fprintf(stderr, "min = %f, max = %f, t = %f\n", min, max, t);
  for(a = first_head_ptr.next_head; a; a = a->next_head)
    if(s->cluster_assignments[a - clusters] < 0)
      for(b = a->next_head; b; b = b->next_head)
        if(s->cluster_assignments[a - clusters]
            == s->cluster_assignments[b - clusters])
        {
          k = a - clusters;
          l = b - clusters;
          ck = clusters[k].count;
          cl = clusters[l].count;
          ckl = ck + cl;
      
          for(c = &clusters[0]; c; c = c->next_head)
          {
            i = c - clusters;
            if(i == k || i == l)
              continue;
            //*aref_dist_mat(M, k, i) = (ck * *aref_dist_mat(M, k, i) +
            //                         cl * *aref_dist_mat(M, l, i) ) / ckl;
            *aref_dist_mat(M, k, i) =
                minf(*aref_dist_mat(M, k, i), *aref_dist_mat(M, l, i));
            *aref_dist_mat(M, l, i) = 0.0;
          }
          join_clusters(&clusters[k], &clusters[l]);
          n--;
        }
    
  for(;;)
  {
    l = 0; k = 0;
    d = -1.0;
    for(a = first_head_ptr.next_head; a; a = a->next_head)
      for(b = a->next_head; b; b = b->next_head)
      {
        i = a - clusters;
        j = b - clusters;
        if(s->cluster_assignments[i] < 0 && s->cluster_assignments[j] < 0)
        {
          e = *aref_dist_mat(M, i, j) = -1000000000.0;
          if(joe_trace)
            fprintf(stderr, rand() & 0x1 ? "wonk!\n" : " wonk!\n");
        }
        else
          e = *aref_dist_mat(M, i, j);
        if( e > d )
        {
          if(s->cluster_assignments[j] < 0)
          {
            k = j;
            l = i;
          } else
          {
            k = i;
            l = j;
          }
          d = e;
        }
      }
    if(joe_trace)
      fprintf(stderr, "l = %ld, k = %ld, d = %f\n", l, k, d);
    if( (l == 0 && k == 0) || d < t) //we're done
      break;
    
    ck = clusters[k].count;
    cl = clusters[l].count;
    ckl = ck + cl;

    for(a = &clusters[0]; a; a = a->next_head)
    {
      i = a - clusters;
      if(i == k || i == l)
        continue;
      *aref_dist_mat(M, k, i) = (ck * *aref_dist_mat(M, k, i) +
                                 cl * *aref_dist_mat(M, l, i) ) / ckl;
      *aref_dist_mat(M, l, i) = 0.0;
    }
    join_clusters(&clusters[k], &clusters[l]);
    n--;
  }

  i = s->header->n_perma_clusters + 1;
  for(a = &clusters[0]; a; a = a->next_head)
  {
    if(s->cluster_assignments[a - clusters] < 0)
      j = -s->cluster_assignments[a - clusters];
    else
      j = i++;
    for(b = a; b; b = b->next)
      s->cluster_assignments[b - clusters] = j;
  }
  
  s->header->n_clusters = n;
  free(M);

}

static void assign_perma_cluster(CLUMPER_STATE_STRUCT *s,
                                long doc,
                                char *lab)
{
  long i;
  for(i = 1; i <= s->header->n_perma_clusters; i++)
    if(0 == strcmp(s->cluster_labels[i], lab))
      break;
  if(i > s->header->n_perma_clusters)
  {
    i = ++(s->header->n_perma_clusters);
    strcpy(s->cluster_labels[i], lab);
  }
  s->cluster_assignments[doc] = -i;
}

long max(long a, long b) {return a > b ? a : b;}

int crm_expr_clump(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{

  char htext[MAX_PATTERN];
  char filename[MAX_PATTERN];
  long htext_len;
  
  char regex_text[MAX_PATTERN];  //  the regex pattern
  long regex_text_len;
  char param_text[MAX_PATTERN];
  long param_text_len;
  int unique, unigram, bychunk, n_clusters = 0;
  
  char tag[DOCUMENT_TAG_LEN];
  char class[CLUSTER_LABEL_LEN];
  
  struct stat statbuf;
  
  CLUMPER_STATE_STRUCT S, *s = &S;
  
  regex_t regee;
  regmatch_t matchee[2];
  
  long i, j, k, l;
  
  char *txtptr;
  long txtstart;
  long txtlen;
  char box_text[MAX_PATTERN];
  char errstr [MAX_PATTERN];

  crm_get_pgm_arg (box_text, MAX_PATTERN, apb->b1start, apb->b1len);

  //  Use crm_restrictvar to get start & length to look at.
  i = crm_restrictvar(box_text, apb->b1len, 
          NULL,
          &txtptr,
          &txtstart,
          &txtlen,
          errstr);
  if ( i < 0)
    {
      long curstmt;
      long fev;
      fev = 0;
      curstmt = csl->cstmt;
      if (i == -1)
  fev = nonfatalerror (errstr, "");
      if (i == -2)
  fev = fatalerror (errstr, "");
      //
      //     did the FAULT handler change the next statement to execute?
      //     If so, continue from there, otherwise, we FAIL.
      if (curstmt == csl->cstmt)
  {
    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
    csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
  };
      return (fev);
    };

  
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  htext_len = apb->p1len;
  htext_len = crm_nexpandvar (htext, htext_len, MAX_PATTERN);
  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;
  htext[j] = '\000';
  strcpy (filename, &htext[i]);

  //use regex_text and regee to grab parameters
  crm_get_pgm_arg (param_text, MAX_PATTERN, apb->s2start, apb->s2len);
  param_text_len = apb->s2len;
  param_text_len = crm_nexpandvar (param_text, param_text_len, MAX_PATTERN);
  
  param_text[ param_text_len ] = '\0';
  if(joe_trace)
    fprintf( stderr, "param_text = %s\n", param_text );
  
  strcpy(regex_text, "n_clusters[[:space:]]*=[[:space:]]*([0-9]+)");
  if( crm_regcomp (&regee, regex_text, strlen(regex_text), REG_EXTENDED) )
  {
    nonfatalerror("Problem compiling regex to grab params:", regex_text);
    return 0;
  }
  if(!crm_regexec (&regee, param_text, param_text_len, 2, matchee, 0, NULL))
  {
    param_text[matchee[1].rm_eo + 1] = '\0';
    if(joe_trace)
      fprintf(stderr, "&param_text[matchee[1].rm_so] = %s\n",
                &param_text[matchee[1].rm_so]);
    n_clusters = atol(&param_text[matchee[1].rm_so]);
    if(joe_trace)
      fprintf(stderr, "n_clusters = %d\n", n_clusters);
  }
  strcpy(regex_text, "tag[[:space:]]*=[[:space:]]*([[:graph:]]+)");
  if( crm_regcomp (&regee, regex_text, strlen(regex_text), REG_EXTENDED) )
  {
    nonfatalerror("Problem compiling regex to grab params:", regex_text);
    return 0;
  }
  if(!crm_regexec (&regee, param_text, param_text_len, 2, matchee, 0, NULL))
  {
    param_text[matchee[1].rm_eo] = '\0';
    strcpy(tag, &param_text[matchee[1].rm_so]);
  } else
    tag[0] = '\0';
  strcpy(regex_text, "clump[[:space:]]*=[[:space:]]*([[:graph:]]+)");
  if( crm_regcomp (&regee, regex_text, strlen(regex_text), REG_EXTENDED) )
  {
    nonfatalerror("Problem compiling regex to grab params:", regex_text);
    return 0;
  }
  if(!crm_regexec (&regee, param_text, param_text_len, 2, matchee, 0, NULL))
  {
    param_text[matchee[1].rm_eo] = '\0';
    strcpy(class, &param_text[matchee[1].rm_so]);
  } else
    class[0] = '\0';

  strcpy(regex_text, "max_documents[[:space:]]*=[[:space:]]*([[:graph:]]+)");
  if( crm_regcomp (&regee, regex_text, strlen(regex_text), REG_EXTENDED) )
  {
    nonfatalerror("Problem compiling regex to grab params:", regex_text);
    return 0;
  }
  if(!crm_regexec (&regee, param_text, param_text_len, 2, matchee, 0, NULL))
  {
    param_text[matchee[1].rm_eo] = '\0';
    max_documents = atol(&param_text[matchee[1].rm_so]);
  }
  //we've already got a default max_documents
  
  crm_get_pgm_arg (regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
  regex_text_len = apb->s1len;
  if(regex_text_len == 0)
  {
    strcpy(regex_text, "[[:graph:]]+"); 
    regex_text_len = strlen( regex_text );
  }
  regex_text[regex_text_len] = '\0';
  regex_text_len = crm_nexpandvar (regex_text, regex_text_len, MAX_PATTERN);
  if( crm_regcomp (&regee, regex_text, regex_text_len, REG_EXTENDED) )
  {
    nonfatalerror("Problem compiling this regex:", regex_text);
    return 0;
  }
  
  unique = apb->sflags & CRM_UNIQUE;
  unigram = apb->sflags & CRM_UNIGRAM;
  bychunk = apb->sflags & CRM_BYCHUNK;

  if (apb->sflags & CRM_REFUTE)
  {
    if(map_file(s, filename))
      //we already nonfatalerrored
      return 0;
    if(tag[0])
      for(i = s->header->n_documents; i >= 0; i--)
        if(0 == strcmp(tag, s->document_tags[i]))
          break;
    else
      i = find_closest_document(s, txtptr + txtstart, txtlen,
                                      &regee, apb->sflags);
    if(i < 0)
    {
      unmap_file(s);
      return 0;
    }
    memmove(s->file_origin + s->document_offsets[i],
            s->file_origin + s->document_offsets[i + 1],
            (s->header->file_length - s->document_offsets[i + 1]) );
    memmove(&s->document_tags[i],
            &s->document_tags[i + 1],
            sizeof(char) * DOCUMENT_TAG_LEN *(s->header->n_documents - i - 1 ));
    memmove(&s->cluster_labels[i],
            &s->cluster_labels[i + 1],
            sizeof(char) * CLUSTER_LABEL_LEN*(s->header->n_documents - i - 1 ));
    s->header->n_documents--;
    j = s->document_offsets[i + 1] - s->document_offsets[i];
    for(k = i; k < s->header->n_documents; k++)
    {
      s->document_offsets[k] = s->document_offsets[k + 1] - j;
      s->cluster_assignments[k] = s->cluster_assignments[k + 1];
    }
    s->header->file_length -= j;
    for(k = 0; k < s->header->n_documents; k++)
      for(l = max(k + 1, i); l < s->header->n_documents; l++)
        *aref_dist_mat(s->distance_matrix, k, l) =
            *aref_dist_mat(s->distance_matrix, k, l + 1);
    if(n_clusters > 0)
    {
      if(bychunk)
        agglomerative_averaging_cluster(s, n_clusters);
      else
        agglomerative_nearest_cluster(s, n_clusters);
    }
    else if(n_clusters == 0)
    {
      if(bychunk)
        thresholding_average_cluster(s);
      else
        thresholding_average_cluster(s);
    }
    l = s->header->file_length;
    
    unmap_file(s);
    crm_force_munmap_filename(filename);
    truncate(filename, l);
    return 0;
  } else
  { //LEARNIN'!
    long n, feature_space[32768];
    FILE *f;
    if(stat(filename, &statbuf))
      make_new_clumper_backing_file(filename);
    if(txtlen == 0)
    {
      if(tag[0] && class[0]) //is not null
      {
        if(map_file(s, filename))
        //we already nonfatalerrored
          return 0;
        for(i = s->header->n_documents - 1; i >= 0; i++)
          if(0 == strcmp(tag, s->document_tags[i]))
            break;
        if(i >= 0)
          assign_perma_cluster(s, i, class);
        unmap_file(s);
      }
      return 0;
    }
      
    n = eat_document
        (       txtptr + txtstart, txtlen, &i,
                &regee,
                feature_space, 32768,
                apb->sflags );
    crm_force_munmap_filename(filename);
    f = fopen(filename, "ab+");
    fwrite(feature_space, n, sizeof(long), f);
    fclose(f);
    if(map_file(s, filename))
      //we already nonfatalerrored
      return 0;
    if(s->header->n_documents >= s->header->max_documents)
    {
      nonfatalerror("This clump backing file is full and cannot"
                    " assimelate new documents!", filename);
      unmap_file(s);
      return 0;
    }
    i = s->header->n_documents++;
    
    s->document_offsets[i] = s->header->file_length;
    s->header->file_length += sizeof(long) * n;
    for(j = 0; j < i; j++)
      *aref_dist_mat(s->distance_matrix, j, i) = get_document_affinity(
            (void *)( s->file_origin + s->document_offsets[i]),
            (void *)( s->file_origin + s->document_offsets[j]) );
    strcpy(s->document_tags[i], tag);
    if(class[0])
      assign_perma_cluster(s, i, class);
    else
      s->cluster_assignments[i] = 0;

    if(n_clusters > 0)
    {
      if(bychunk)
        agglomerative_averaging_cluster(s, n_clusters);
      else
        agglomerative_nearest_cluster(s, n_clusters);
    }
    else if(n_clusters == 0)
    {
      if(bychunk)
        thresholding_average_cluster(s);
      else
        thresholding_average_cluster(s);
    }
    unmap_file(s);
    return 0;
  }
}

int sprint_lab(CLUMPER_STATE_STRUCT *s, char *b, int l)
{
  if(l == 0)
  {
    strcpy(b, "unassigned");
    return strlen("unassigned");
  }
  if(s->cluster_labels[l][0] != '\0')
    return sprintf(b, "%s", s->cluster_labels[l]);
  else
    return sprintf(b, "clump_#%d", l);
}

int sprint_tag(CLUMPER_STATE_STRUCT *s, char *b, int d)
{
  if(s->document_tags[d][0] != '\0')
    return sprintf(b, "%s", s->document_tags[d]);
  else
    return sprintf(b, "document_#%d", d);
}

int crm_expr_pmulc(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  char htext[MAX_PATTERN];
  char filename[MAX_PATTERN];
  long htext_len;
  
  char regex_text[MAX_PATTERN];  //  the regex pattern
  long regex_text_len;
  int unique, unigram, bychunk;
  
  char out_var[MAX_PATTERN];
  long out_var_len;

  float A[MAX_CLUSTERS], T;
  long N[MAX_CLUSTERS];
  double p[MAX_CLUSTERS], pR[MAX_CLUSTERS];
  
  long feature_space[32768];
  long n;
  
  long closest_doc = -1;
  float closest_doc_affinity;
  
  long out_len = 0;
  
  CLUMPER_STATE_STRUCT S, *s = &S;
  
  regex_t regee;
    
  long i, j;
  
  char *txtptr;
  long txtstart;
  long txtlen;
  char box_text[MAX_PATTERN];
  char errstr [MAX_PATTERN];

  crm_get_pgm_arg (box_text, MAX_PATTERN, apb->b1start, apb->b1len);

  //  Use crm_restrictvar to get start & length to look at.
  i = crm_restrictvar(box_text, apb->b1len, 
          NULL,
          &txtptr,
          &txtstart,
          &txtlen,
          errstr);
  if ( i < 0)
    {
      long curstmt;
      long fev;
      fev = 0;
      curstmt = csl->cstmt;
      if (i == -1)
  fev = nonfatalerror (errstr, "");
      if (i == -2)
  fev = fatalerror (errstr, "");
      //
      //     did the FAULT handler change the next statement to execute?
      //     If so, continue from there, otherwise, we FAIL.
      if (curstmt == csl->cstmt)
  {
    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
    csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
  };
      return (fev);
    };

  
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  htext_len = apb->p1len;
  htext_len = crm_nexpandvar (htext, htext_len, MAX_PATTERN);
  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;
  htext[j] = '\000';
  strcpy (filename, &htext[i]);

  //grab output variable name
  crm_get_pgm_arg (out_var, MAX_PATTERN, apb->p2start, apb->p2len);
  out_var_len = apb->p2len;
  out_var_len = crm_nexpandvar (out_var, out_var_len, MAX_PATTERN);

  crm_get_pgm_arg (regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
  regex_text_len = apb->s1len;
  if(regex_text_len == 0)
  {
    strcpy(regex_text, "[[:graph:]]+"); 
    regex_text_len = strlen( regex_text );
  }
  regex_text[regex_text_len] = '\0';
  regex_text_len = crm_nexpandvar (regex_text, regex_text_len, MAX_PATTERN);
  if( crm_regcomp (&regee, regex_text, regex_text_len, REG_EXTENDED) )
  {
    nonfatalerror("Problem compiling this regex:", regex_text);
    return 0;
  }

  unique = apb->sflags & CRM_UNIQUE;
  unigram = apb->sflags & CRM_UNIGRAM;
  bychunk = apb->sflags & CRM_BYCHUNK;

  if(map_file(s, filename))
  //we already nonfatalerrored
    return 0;
  
  if(txtlen == 0)
  {
    for(i = 0; i < s->header->n_documents; i++)
    {
      A[0] = 0.0;
      N[0] = 1;
      for(j = 0; j < s->header->n_documents; j++)
        if(i != j && s->cluster_assignments[i] == s->cluster_assignments[j])
        {
          N[0]++;
          if(bychunk)
            A[0] += *aref_dist_mat(s->distance_matrix, i, j);
          else
            if(*aref_dist_mat(s->distance_matrix, i, j) > A[0])
              A[0] = *aref_dist_mat(s->distance_matrix, i, j);
        }
      if(bychunk)
        A[0] /= (float)N[0];
      out_len += sprintf(outbuf + out_len, "%ld (", i);
      out_len += sprint_tag(s, outbuf + out_len, i);
      out_len += sprintf(outbuf + out_len,
        ") clump: %ld (", s->cluster_assignments[i]);
      out_len += sprint_lab(s, outbuf + out_len, s->cluster_assignments[i]);
      out_len += sprintf(outbuf + out_len, ") affinity: %0.4f\n", A[0]);

    }
    outbuf[out_len] = '\0';
    if(out_var_len)
      crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_len);
    unmap_file(s);
    return 0;
  } else
  {
    if(joe_trace)
      fprintf(stderr, "pmulcing!\n");
    n = eat_document
        (       txtptr + txtstart, txtlen, &i,
                &regee,
                feature_space, 32768,
                apb->sflags );
    closest_doc_affinity = -1.0;
    for(i = 0; i <= s->header->n_clusters; i++)
    {
      A[i] = 0.0;
      N[i] = 0;
    }
    if(bychunk)
    {
      for(i = 0; i < s->header->n_documents; i++)
      {
        j = s->cluster_assignments[i];
        if(j == 0)
          continue;
        T = get_document_affinity(feature_space, (void *)(s->file_origin +
                    s->document_offsets[i]));
        A[j] += T;
        if(T > closest_doc_affinity)
        {
          closest_doc = i;
          closest_doc_affinity = T;
        }
        N[j]++;
      }
      for(i = 1; i <= s->header->n_clusters; i++)
        T += A[i] /= N[i];
    } else
    {
      for(i = 0; i < s->header->n_documents; i++)
      {
        j = s->cluster_assignments[i];
        T = get_document_affinity(feature_space, (void *)(s->file_origin +
                    s->document_offsets[i]));
        if(T > A[j])
          A[j] = T;
        if(T > closest_doc_affinity)
        {
          closest_doc = i;
          closest_doc_affinity = T;
        }
        N[j]++;
      }
    }
    T = 0.0000001;  
    j = 1;
    for(i = 1; i <= s->header->n_clusters; i++)
    {
      if(A[i] > A[j])
        j = i;
      if(A[i] == 0.0)
        p[i] = 0.0;
      else
        p[i] = normalized_gauss(1.0 / A[i] - 1.0, 0.5);
      //p[i] = A[i];
      T += p[i];
    }
    if(s->header->n_clusters < 2)
      p[j = 1] = 0.0;
    for(i = 1; i <= s->header->n_clusters; i++)
    {
      p[i] /= T;
      pR[i] = 10 * ( log10(0.0000001 + p[i])
                        - log10(1.0000001 - p[i]) );
    }
    
    if(joe_trace)
      fprintf(stderr, "generating output...\n");
      
    if(p[j] > 0.5)
      out_len += sprintf(outbuf + out_len,
          "PMULC succeeds; success probabilty: %0.4f pR: %0.4f\n", p[j], pR[j]);
    else
      out_len += sprintf(outbuf + out_len,
          "PMULC fails; success probabilty: %0.4f pR: %0.4f\n", p[j], pR[j]);
    out_len += sprintf(outbuf + out_len,
          "Best match to clump #%ld (", j);
    out_len += sprint_lab(s, outbuf + out_len, j);
    out_len += sprintf(outbuf + out_len,
          ") prob: %0.4f  pR: %0.4f\n", p[j], pR[j]);
    out_len += sprintf(outbuf + out_len,
          "Closest document: #%ld (", closest_doc);
    out_len += sprint_tag(s, outbuf + out_len, closest_doc);
    out_len += sprintf(outbuf + out_len,
          ") affinity: %0.4f\n", closest_doc_affinity);
    out_len += sprintf(outbuf + out_len,
          "Total features in input file: %ld\n", n);
    for(i = 1; i <= s->header->n_clusters; i++)
    {
      out_len += sprintf(outbuf + out_len,
          "%ld: (", i);
      out_len += sprint_lab(s, outbuf + out_len, i);
      out_len += sprintf(outbuf + out_len,
          "): documents: %ld  affinity: %0.4f  prob: %0.4f  pR: %0.4f\n",
          N[i], A[i], p[i], pR[i]);
    }
      
    if (p[j] > 0.5000)
    {
      if (user_trace)
        fprintf (stderr, "CLUMP was a SUCCESS, continuing execution.\n");
    }
    else
    {
      if (user_trace)
        fprintf (stderr, "CLUMP was a FAIL, skipping forward.\n");
      //    and do what we do for a FAIL here
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk [csl->mct[csl->cstmt]->nest_level] = -1;
    };
    outbuf[out_len] = '\0';
    if(joe_trace)
      fprintf(stderr, "JOE_TRACE:\n%s", outbuf);
    if(out_var_len)
      crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_len);
    unmap_file(s);
    return 0;
  }

}













