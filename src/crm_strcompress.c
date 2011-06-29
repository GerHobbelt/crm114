//  crm_fscm.c //sequence correlation monster
//  by Joe Langeway derived from crm_bit_entropy.c and
//    produced for the crm114 so:
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
//     crm_scm.c - //sequence correlation monster, compares unknown document
//        to whole corpus of classes and counts co-occurring substrings
//
//     Original spec by Bill Yerazunis, original code by Joe Langeway,
//     recode for CRM114 use by Bill Yerazunis.
//
//     This code section (crm_scm and subsidiary routines) is
//     dual-licensed to both William S. Yerazunis and Joe Langeway,
//     including the right to reuse this code in any way desired,
//     including the right to relicense it under any other terms as
//     desired.
//
//////////////////////////////////////////////////////////////////////

/*
 * This file is part of on going research and should not be considered
 * a finished product, a reliable tool, an example of good software
 * engineering, or a reflection of any quality of Joe's besides his
 * tendancy towards long hours.
 */

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"



#if !defined (CRM_WITHOUT_SCM)



#define NULL_INDEX 2147483647

typedef struct mythical_scm_header
{
  long n_bytes;              //this is the length of rememebred text, the number of hash buckets, and the size of the hash root
  long n_bytes_used;
  long n_trains;             //how many times have we had to train this guy
  long n_features;           //number of bytes we've eaten up to n_bytes
  long free_hash_nodes;      //index of first in free chain
  long free_prefix_nodes;    //index of first in free chain
  long hash_root_offset;
  long hash_offset;
  long prefix_offset;
  long text_offset;
  long text_pos;             //we wrap around when we fill the buffer
} SCM_HEADER_STRUCT;

typedef struct mythical_hash
{
  char          prefix_text[4];   //we make it 4 bytes so thing align nicely
  unsigned long key;              //hash key of the three charactor prefix
  long          next;             //in hash chain
  long          first;            //first prefix node
} HASH_STRUCT;

typedef struct mythical_prefix
{
  long offset;
  long next;
} PREFIX_STRUCT;

typedef struct mythical_scm_state
{
  SCM_HEADER_STRUCT *header;
  long              *text_pos, n_bytes, *free_hash_nodes, *free_prefix_nodes;   //we dup some stuff from the header to shorten things up
  long              *hash_root;
  HASH_STRUCT       *hashee;
  PREFIX_STRUCT     *prefix;
  char              *text;
  char              *learnfilename;
} SCM_STATE_STRUCT;

//we set it here 'cause we're not done codin' yet
long n_bytes = 4000000;      //this is the one global that can be passed in from crm

static int joe_trace = 0;

static void make_scm_state(SCM_STATE_STRUCT *s, void *space)
{
  long n_hash = 2 * n_bytes, n_prefix = 2 * n_bytes;
  char   *o;
  long i;

  SCM_HEADER_STRUCT *h = space;

  h->n_bytes = n_bytes;
  h->n_bytes_used = 0;
  h->n_trains = 0;
  h->n_features = 0;
  h->free_hash_nodes = 0;
  h->hash_root_offset = sizeof(SCM_HEADER_STRUCT);
  h->hash_offset = sizeof(SCM_HEADER_STRUCT) + n_bytes * sizeof(long);
  h->prefix_offset =
    sizeof(SCM_HEADER_STRUCT) + n_bytes * sizeof(long) +
    n_hash * sizeof(HASH_STRUCT);
  h->text_offset =
    sizeof(SCM_HEADER_STRUCT) + n_bytes * sizeof(long) +
    n_hash * sizeof(HASH_STRUCT) + n_prefix * sizeof(PREFIX_STRUCT);
  h->text_pos = 0;

  o = space;
  s->header = h;
  s->text_pos = &h->text_pos;
  s->n_bytes = h->n_bytes;
  s->free_hash_nodes = &h->free_hash_nodes;
  s->free_prefix_nodes = &h->free_prefix_nodes;
  s->hash_root = (long *)&o[h->hash_root_offset];
  s->hashee = (HASH_STRUCT *)&o[h->hash_offset];
  s->prefix = (PREFIX_STRUCT *)&o[h->prefix_offset];
  s->text = (char *)&o[h->text_offset];

  for (i = 0; i < n_bytes; i++)
  {
    s->hash_root[i] = NULL_INDEX;
    s->text[i] = 0;
  }
  for (i = 0; i < n_hash; i++)
  {
    s->hashee[i].key = 0;
    s->hashee[i].next = i + 1;
    s->hashee[i].first = NULL_INDEX;
  }
  for (i = 0; i < n_prefix; i++)
  {
    s->prefix[i].offset = NULL_INDEX;
    s->prefix[i].next = i + 1;
  }
  s->hashee[n_hash - 1].next = NULL_INDEX;
  s->prefix[n_prefix - 1].next = NULL_INDEX;
}

static void map_file(SCM_STATE_STRUCT *s, char *filename)
{
  struct stat statbuf;

  if (stat(filename, &statbuf))
  {
    void *space;
    long filesize;
    FILE *f;

    filesize = sizeof(SCM_HEADER_STRUCT)
               + n_bytes * (sizeof(long) + 2 * sizeof(HASH_STRUCT) +
                            2 * sizeof(PREFIX_STRUCT) + sizeof(char));
    f = fopen(filename, "wb");
    if (f == 0)
    {
      fatalerror("For some reason, I was unable to write-open the file named ",
                 filename);
    }
    else
    {
      CRM_PORTA_HEADER_INFO classifier_info = { 0 };

      classifier_info.classifier_bits = CRM_SCM;

      if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
      {
        fatalerror_ex(SRC_LOC(),
                      "\n Couldn't write header to file %s; errno=%d(%s)\n",
                      filename, errno, errno_descr(errno));
        fclose(f);
        return;
      }

      if (file_memset(f, 0, filesize + 1024))
      {
        fatalerror_ex(SRC_LOC(),
                      "\n Couldn't write to file %s; errno=%d(%s)\n",
                      filename, errno, errno_descr(errno));
        fclose(f);
        return;
      }

      fclose(f);
    }
    space = crm_mmap_file(filename,
                          0,
                          filesize,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          &filesize);
    make_scm_state(s, space);
  }
  else
  {
    char   *o;
    SCM_HEADER_STRUCT *h;

    s->header = crm_mmap_file(filename,
                              0,
                              statbuf.st_size,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              NULL);
    o = (char *)s->header;

    h = s->header;
    s->text_pos = &h->text_pos;
    s->n_bytes = h->n_bytes;
    s->free_hash_nodes = &h->free_hash_nodes;
    s->free_prefix_nodes = &h->free_prefix_nodes;
    s->hash_root = (long *)&o[h->hash_root_offset];
    s->hashee = (HASH_STRUCT *)&o[h->hash_offset];
    s->prefix = (PREFIX_STRUCT *)&o[h->prefix_offset];
    s->text = (char *)&o[h->text_offset];
  }
  s->learnfilename = filename;
}

static void unmap_file(SCM_STATE_STRUCT *s)
{
  crm_munmap_file((void *)s->header);

#if 0                           /* now touch-fixed inside the munmap call already! */
#if defined (HAVE_MMAP) || defined (HAVE_MUNMAP)
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .css file, to force
  //    the filesystem to repropagate it's caches.
  //
  crm_touch(s->learnfilename);
#endif
#endif
}

#if 0                           /* [i_a] unused */
static int match_prefix(SCM_STATE_STRUCT *s, long a, long b)
{
  return s->text[a] == s->text[b]
         && s->text[(a + 1) % s->n_bytes] == s->text[(b + 1) % s->n_bytes]
         && s->text[(a + 2) % s->n_bytes] == s->text[(b + 2) % s->n_bytes];
}
#endif

static int match_prefix_char_ptr(SCM_STATE_STRUCT *s, long a, char *b)
{
  return s->text[a] == b[0] && s->text[(a + 1) % s->n_bytes] == b[1]
         && s->text[(a + 2) % s->n_bytes] == b[2];
}

//removes all prefixes which have had their text written over and all hash nodes which are left empty afterwards
static void macro_groom(SCM_STATE_STRUCT *s)
{
  // fprintf(stdout, "macrogrooming!\n");
  long i, j, *k, l, *m;

  for (i = 0; i < s->n_bytes; i++)
    if (s->hash_root[i] != NULL_INDEX)
    {
      k = &s->hash_root[i];
      j = s->hash_root[i];
      while (j != NULL_INDEX)
      {
        m = &s->hashee[j].first;
        l = s->hashee[j].first;
        while (l != NULL_INDEX
               && !match_prefix_char_ptr(s, s->prefix[l].offset,
                                         s->hashee[j].prefix_text))
        {
          *m = s->prefix[l].next;
          s->prefix[l].offset = NULL_INDEX;
          s->prefix[l].next = *s->free_prefix_nodes;
          *s->free_prefix_nodes = l;
          l = s->hashee[j].first;
          s->header->n_features--;
        }
        if (l != NULL_INDEX)
        {
          m = &s->prefix[l].next;
          l = s->prefix[l].next;
          while (l != NULL_INDEX)
          {
            if (!match_prefix_char_ptr
                (s, s->prefix[l].offset, s->hashee[j].prefix_text))
            {
              *m = s->prefix[l].next;
              s->prefix[l].offset = NULL_INDEX;
              s->prefix[l].next = *s->free_prefix_nodes;
              *s->free_prefix_nodes = l;
              l = *m;
              s->header->n_features--;
            }
            else
            {
              m = &s->prefix[l].next;
              l = s->prefix[l].next;
            }
          }
          k = &s->hashee[j].next;
          j = s->hashee[j].next;
        }
        else
        {
          *k = s->hashee[j].next;
          s->hashee[j].first = NULL_INDEX;
          s->hashee[j].next = *s->free_hash_nodes;
          *s->free_hash_nodes = j;
          j = *k;
        }
      }
    }
}

static PREFIX_STRUCT *add_new_string(SCM_STATE_STRUCT *s, long t)
{
  unsigned long key;
  long i;
  long   *m;
  long j;
  char tt[3];

  tt[0] = s->text[t];
  tt[1] = s->text[(t + 1) % s->n_bytes];
  tt[2] = s->text[(t + 2) % s->n_bytes];

  key = strnhash(tt, 3);
  i = s->hash_root[key % s->n_bytes];
  m = &s->hash_root[key % s->n_bytes];

  while (i != NULL_INDEX && s->hashee[i].key != key
         && !match_prefix_char_ptr(s, t, s->hashee[i].prefix_text))
  {
    m = &s->hashee[i].next;
    i = s->hashee[i].next;
  }
  if (i == NULL_INDEX)
  {
    *m = i = *s->free_hash_nodes;
    *s->free_hash_nodes = s->hashee[i].next;
    s->hashee[i].next = NULL_INDEX;
    s->hashee[i].key = key;
    s->hashee[i].prefix_text[0] = s->text[t];
    s->hashee[i].prefix_text[1] = s->text[(t + 1) % s->n_bytes];
    s->hashee[i].prefix_text[2] = s->text[(t + 2) % s->n_bytes];
    j = s->hashee[i].first = *s->free_prefix_nodes;
    *s->free_prefix_nodes = s->prefix[j].next;
    if (*s->free_prefix_nodes == NULL_INDEX)
      macro_groom(s);
    s->prefix[j].next = NULL_INDEX;
    s->prefix[j].offset = t;
    s->header->n_features++;
    return &s->prefix[j];
  }
  else
  {
    j = *s->free_prefix_nodes;
    *s->free_prefix_nodes = s->prefix[j].next;
    if (*s->free_prefix_nodes == NULL_INDEX)
      macro_groom(s);
    s->prefix[j].next = s->hashee[i].first;
    s->hashee[i].first = j;
    s->prefix[j].offset = t;
    s->header->n_features++;
    return &s->prefix[j];
  }
}

//remove the longest string associated with this guy from the prefix hash
static void refute_string(SCM_STATE_STRUCT *s, char *t, long max_len)
{
  //fprintf(stdout, "refuting!\n");
  unsigned long key = strnhash(t, 3);
  long k, *l, m, n, longest_prefix = NULL_INDEX, longest_len = 0;
  long i = s->hash_root[key % s->n_bytes];
  long *j = &s->hash_root[key % s->n_bytes];

  while (i != NULL_INDEX && s->hashee[i].key != key
         && (t[0] != s->hashee[i].prefix_text[0]
             || t[1] != s->hashee[i].prefix_text[1]
             || t[2] != s->hashee[i].prefix_text[2]))
  {
    j = &s->hashee[i].next;
    i = s->hashee[i].next;
  }
  if (i == NULL_INDEX)
    return;

  l = &s->hashee[i].first;
  k = s->hashee[i].first;
  while (k != NULL_INDEX)
    if (match_prefix_char_ptr(s, s->prefix[k].offset, t))
    {
      for (m = 1, n = (s->prefix[k].offset + 3) % s->n_bytes;
           m + 2 < max_len && s->text[n] == t[m + 2]; m++)
      {
        n++;
        if (n == s->n_bytes)
          n = 0;
      }
      if (m > longest_len)
      {
        longest_prefix = k;
        longest_len = m;
      }
      l = &s->prefix[k].next;
      k = s->prefix[k].next;
    }
    else
    {
      *l = s->prefix[k].next;
      s->prefix[k].next = *s->free_prefix_nodes;
      *s->free_prefix_nodes = k;
      k = *l;
      s->header->n_features--;
    }
  l = &s->hashee[i].first;
  k = s->hashee[i].first;
  while (k != NULL_INDEX)
    if (k == longest_prefix)
    {
      //fprintf(stdout, "refute works!\n");
      *l = s->prefix[k].next;
      s->prefix[k].next = *s->free_prefix_nodes;
      *s->free_prefix_nodes = k;
      s->header->n_features--;
      break;
    }
    else
    {
      l = &s->prefix[k].next;
      k = s->prefix[k].next;
    }
  if (s->hashee[i].first == NULL_INDEX)
  {
    *j = s->hashee[i].next;
    s->hashee[i].first = NULL_INDEX;
    s->hashee[i].next = *s->free_hash_nodes;
    *s->free_hash_nodes = i;
    i = *j;
  }
}

static int pow_table_init = 1;

static double pow_table[256] = { 0.0 };

static double pow15m(long i)
{
  if (i >= 256)
    return pow((double)i, 1.5);

  if (pow_table_init)
  {
    long j;

    pow_table_init = 0;
    for (j = 0; j < 256; j++)
      pow_table[j] = pow((double)j, 1.5);
  }
  return pow_table[i];
}

static double score_string(SCM_STATE_STRUCT *s, char *t, long max_len)
{
  long k, *l, m, n;
  unsigned long key = strnhash(t, 3);
  long i = s->hash_root[key % s->n_bytes], *j =
    &s->hash_root[key % s->n_bytes];
  double score = 0.0;

  while (i != NULL_INDEX && s->hashee[i].key != key
         && (t[0] != s->hashee[i].prefix_text[0]
             || t[1] != s->hashee[i].prefix_text[1]
             || t[2] != s->hashee[i].prefix_text[2]))
  {
    j = &s->hashee[i].next;
    i = s->hashee[i].next;
  }
  if (i == NULL_INDEX)
    return 0.0;

  l = &s->hashee[i].first;
  k = s->hashee[i].first;
  while (k != NULL_INDEX)
    if (match_prefix_char_ptr(s, s->prefix[k].offset, t))
    {
      for (m = 1, n = (s->prefix[k].offset + 3) % s->n_bytes;
           m + 2 < max_len && s->text[n] == t[m + 2]; m++)
      {
        n++;
        if (n == s->n_bytes)
          n = 0;
      }
      score += pow15m(m);
      l = &s->prefix[k].next;
      k = s->prefix[k].next;
    }
    else
    {
      //fprintf(stdout, "%c%c%c!=%c%c%c\n", s->text[s->prefix[k].offset], s->text[s->prefix[k].offset+1], s->text[s->prefix[k].offset+2], t[0], t[1], t[2]);
      // *(int *)0 = 0; //sgfault to get to debugger
      *l = s->prefix[k].next;
      s->prefix[k].next = *s->free_prefix_nodes;
      *s->free_prefix_nodes = k;
      k = *l;
      s->header->n_features--;
    }
  if (s->hashee[i].first == NULL_INDEX)
  {
    *j = s->hashee[i].next;
    s->hashee[i].first = NULL_INDEX;
    s->hashee[i].next = *s->free_hash_nodes;
    *s->free_hash_nodes = i;
    i = *j;
  }
  return score;
}

static double score_document(SCM_STATE_STRUCT *s, char *doc, long len)
{
  long i;
  double score = 1.0;          //scores start at 1 ok!?

  for (i = 0; i < len; i++)
    score += score_string(s, doc + i, len - i);
  //len -= 1; //to prevent probabilities greater than one
  //len = len * (len + 1) / 2;
  if (joe_trace)
    fprintf(stderr, "assigning score %f to document of length %ld\n", score,
            len);
  return score;                 // / (double)len;
}

int crm_expr_scm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
                       long txtstart, long txtlen)
{
  char filename[MAX_PATTERN];
  long filename_len;

  SCM_STATE_STRUCT S, *s = &S;

  long doc_start;
  long i;

  if (internal_trace)
    joe_trace = 1;

  if (joe_trace)
    fprintf(stderr, "entered crm_expr_scm_learn (learn)\n");

  //parse out .scm file name
  crm_get_pgm_arg(filename, MAX_PATTERN, apb->p1start, apb->p1len);
  filename_len = apb->p1len;
  filename_len = crm_nexpandvar(filename, filename_len, MAX_PATTERN);

  map_file(s, filename);

  txtptr += txtstart;

  doc_start = *s->text_pos;

  if (apb->sflags & CRM_REFUTE)
    for (i = 0; i < txtlen; i++)
      refute_string(s, txtptr, txtlen - i);
  else
  {
    s->header->n_trains++;
    //cat it to our other text
    for (i = 0; i < txtlen; i++)
    {
      s->text[(*s->text_pos)++] = txtptr[i];
      if (*s->text_pos >= s->n_bytes)
        *s->text_pos -= s->n_bytes;
    }
    if (s->header->n_bytes_used > s->n_bytes)
      s->header->n_bytes_used = s->n_bytes;


    //cache all the three character prefixes
    for (i = doc_start; i != *s->text_pos; i++)
    {
      if (i >= s->n_bytes)
        i = 0;
      add_new_string(s, i);
      s->header->n_bytes_used++;
    }
  }
  if (joe_trace)
    fprintf(stderr, "leaving crm_expr_scm_learn (learn)\n");
  return 0;
}

int crm_expr_scm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
                          long txtstart, long txtlen)
{
  SCM_STATE_STRUCT S, *s = &S;

  char filenames_field[MAX_PATTERN];
  long filenames_field_len;
  char filenames[MAX_CLASSIFIERS][MAX_FILE_NAME_LEN];

  char out_var[MAX_PATTERN];
  long out_var_len;

  long i, j, k, n_classifiers;

  long fail_on = MAX_CLASSIFIERS;    //depending on where the vbar is

  double scores[MAX_CLASSIFIERS], probs[MAX_CLASSIFIERS],
         norms[MAX_CLASSIFIERS], bn, pR[MAX_CLASSIFIERS];
  long n_features[MAX_CLASSIFIERS], n_bytes_used[MAX_CLASSIFIERS];
  long out_pos;

  double tot_score = 0.0, suc_prob = 0.0, suc_pR;
  long max_scorer, min_scorer;

  crm_get_pgm_arg(filenames_field, MAX_PATTERN, apb->p1start, apb->p1len);
  filenames_field_len = apb->p1len;
  filenames_field_len =
    crm_nexpandvar(filenames_field, filenames_field_len, MAX_PATTERN);

  crm_get_pgm_arg(out_var, MAX_PATTERN, apb->p2start, apb->p2len);
  out_var_len = apb->p2len;
  out_var_len = crm_nexpandvar(out_var, out_var_len, MAX_PATTERN);

  for (i = 0, j = 0, k = 0; i < filenames_field_len && j < MAX_CLASSIFIERS;
       i++)
    if (filenames_field[i] == '\\')
      filenames[j][k++] = filenames_field[++i];
    else if (crm_isspace(filenames_field[i]) && k > 0)
    {
      filenames[j][k] = 0;
      k = 0;
      j++;
    }
    else if (filenames_field[i] == '|')
    {
      fail_on = j;
      if (k > 0)
      {
        k = 0;
        j++;
      }
    }
    else if (crm_isgraph(filenames_field[i]))
      filenames[j][k++] = filenames_field[i];

  filenames[j][k] = 0;
  n_classifiers = j + 1;

  if (joe_trace)
  {
    fprintf(stderr, "fail_on = %ld\n", fail_on);
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "filenames[%ld] = %s\n", i, filenames[i]);
  }

  for (i = 0; i < n_classifiers; i++)
  {
    map_file(s, filenames[i]);
    n_features[i] = s->header->n_features;
    n_bytes_used[i] = s->header->n_bytes_used;
    norms[i] = (double)s->header->n_trains;
    scores[i] = score_document(s, txtptr + txtstart, txtlen);
    unmap_file(s);
  }

  if (joe_trace)
  {
    fprintf(stderr, "suc_prob = %f\n", suc_prob);
    fprintf(stderr, "tot_score = %f\n", tot_score);
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "scores[%ld] = %f\n", i, scores[i]);
  }


  max_scorer = 0;
  for (j = 1; j < n_classifiers; j++)
    if (scores[j] > scores[max_scorer])
      max_scorer = j;
  min_scorer = 0;
  for (j = 1; j < n_classifiers; j++)
    if (scores[j] < scores[min_scorer])
      min_scorer = j;
  bn = scores[min_scorer] * 0.8;

  out_pos = 0;

  tot_score = 0.0;
  for (j = 0; j < n_classifiers; j++)
    probs[j] = scores[j] - bn;

  for (j = 0; j < n_classifiers; j++)
    tot_score += probs[j];
  for (j = 0; j < n_classifiers; j++)
    probs[j] /= tot_score;
  for (j = 0; j < fail_on; j++)
    suc_prob += probs[j];
  //suc_pR = suc_prob > .5 ? pow(10.0, 10.0 * suc_prob - 5.0) : -1.0 * pow(10.0, 5.0 - 10.0 * suc_prob);
  suc_pR = log10(suc_prob) - log10(1.0 - suc_prob);
  for (j = 0; j < fail_on; j++)
    pR[j] = log10(probs[j]) - log10(1.0 - probs[j]);
  //pR[j] = probs[j] > .5 ? pow(10.0, 10.0 * probs[j] - 5.0) : -1.0 * pow(10.0, 5.0 - 10.0 * probs[j]);

  if (joe_trace)
  {
    fprintf(stderr, "suc_prob = %f\n", suc_prob);
    fprintf(stderr, "tot_score = %f\n", tot_score);
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "scores[%ld] = %f\n", i, scores[i]);
  }

  if (suc_prob > 0.5 && suc_prob <= 1.0)  //test for nan as well
    out_pos +=
      sprintf(outbuf + out_pos,
              "CLASSIFY succeeds; success probability: %f  pR: %6.4f\n",
              suc_prob, suc_pR);
  else
    out_pos +=
      sprintf(outbuf + out_pos,
              "CLASSIFY fails; success probability: %6.4f  pR: %6.4f\n",
              suc_prob, log10(suc_prob) - log10(1.0 - suc_prob));
  /* [i_a] GROT GROT GROT: %s in sprintf may cause buffer overflow. not fixed in this review/scan */
  out_pos +=
    sprintf(outbuf + out_pos,
            "Best match to file #%ld (%s) prob: %6.4f  pR: %6.4f\n",
            max_scorer, filenames[max_scorer], probs[max_scorer],
            pR[max_scorer]);

  out_pos +=
    sprintf(outbuf + out_pos, "Total features in input file: %ld\n",
            txtlen);

  for (i = 0; i < n_classifiers; i++)
  {
    /* [i_a] GROT GROT GROT: %s in sprintf may cause buffer overflow. not fixed in this review/scan */
    out_pos +=
      sprintf(
      outbuf + out_pos,
      "#%ld (%s): bytes used: %ld, features: %ld, score: %3.2e, prob: %3.2e, pR: %6.2f\n",
      i,
      filenames[i],
      n_bytes_used[i],
      n_features[i],
      scores[i],
      probs[i],
      pR[i]);
  }

  if (out_var_len)
    crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_pos);

  if (suc_prob <= 0.5 || suc_prob > 1.0)
  {
    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
    csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
  }
  return 0;
}

#else /* CRM_WITHOUT_SCM */

int crm_expr_scm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
                       long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "SCM");
}


int crm_expr_scm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb, char *txtptr,
                          long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "SCM");
}

#endif /* CRM_WITHOUT_SCM */

