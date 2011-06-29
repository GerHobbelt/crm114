//crm_fscm.c //sequence correlation monster

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
//     crm_fscm.c - //fast substring compression matcher
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
 *
 * Here's what's going on:
 *
 * We learn documents by copying them into a text space of stored documents and
 * caching every contiguous three characters with there offset so that we can find
 * common substrings relatively fast. The text space was set to 1MB for TREC.
 * Associated with every character position in the text space is an index
 * (s->indeces) to the correspond prefix node. Prefix nodes point to the postion
 * in the stored text of particular three digit prefixes. Every prefix node of the
 * same three charecter prefix belongs to a chain pointed to by a hash node. The
 * first prefix node of a chain points to the hash node's index with it's prev
 * field with a (-i - 1) to mark it as the first node. Hashnodes are chained, the
 * first in a chain is pointed to by the hash root (s->hash_root) so that we can
 * mod a key by n_bytes and look up the first node in the chain. The first hash
 * node in a chain points back to it's position in the hash root with a -i - 1. The
 * average length of chains should be close to one and never more than two. When we
 * finally classify we score documents as belonging to a class by awarding pionts
 * for every nonoverlapping substring match we can make, longest first. Currently
 * it is (length - 2)^1.5 points per match. That seems optimal.
 *
 */

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"


#if !defined (CRM_WITHOUT_FSCM)


#define NULL_INDEX 2147483647

typedef struct mythical_scm_header
{
  long n_bytes;             //this is the length of rememebred text, the number
                            //of hashbuckets, and the size of the hash root
  long n_trains;            //how many times have we had to train this guy
  long n_features;          //number of bytes we've eaten up to n_bytes
  long free_hash_nodes;     //index of first in free chain
  long free_prefix_nodes;   //index of first in free chain
  long hash_root_offset;
  long hash_offset;
  long prefix_offset;
  long text_offset;
  long text_pos;            //we wrap around when we fill the buffer
  long indeces_offset;
} SCM_HEADER_STRUCT;

//nodes for our hash table of three character prefixes
typedef struct mythical_hash
{
  char          prefix_text[4];   //we make it 4 bytes so thing align nicely
  unsigned long key;              //hash key of the three charactor prefix
  long          next;             //in hash chain
  long          prev;
  long          first;   //first prefix node
} HASH_STRUCT;

//one node for every contiguous three characters in stored text
typedef struct mythical_prefix
{
  long offset;
  long prev;
  long next;
} PREFIX_STRUCT;

//pointers to runtime structures to pass around so that we don't have a
// bizillion globals
typedef struct mythical_scm_state
{
  SCM_HEADER_STRUCT *header;
  //we dup some stuff from the header to shorten things up
  long *text_pos, n_bytes, *free_hash_nodes, *free_prefix_nodes;
  //s->hash_root[key % n_bytes] is the first hash_node in the chain that key
  //would be in
  long *hash_root;
  //s->hashee[i] is the ith hash node
  HASH_STRUCT *hashee;
  //and so forth...
  PREFIX_STRUCT *prefix;
  char          *text;
  char          *learnfilename;   //the classifier file we're working on
  long          *indeces;
} SCM_STATE_STRUCT;


//this is the one global that can be passed in from crm.
//it determines the amount of previous text we remember
//1 megabyte gives good accuracy and enough speed for TREC
//512K is twice as fast but ever so slightly less accurate
static long n_bytes = 1048576;


//fill in a fresh classifier state assuming that the space is already allocated
// and mapped
static void make_scm_state(SCM_STATE_STRUCT *s, void *space)
{
  SCM_HEADER_STRUCT *h = space;
  char *o = space;
  long i;

  h->n_bytes = n_bytes;
  h->n_trains = 0;
  h->n_features = 0;
  h->free_prefix_nodes = 0;
  h->free_hash_nodes = 0;
  h->hash_root_offset = sizeof(SCM_HEADER_STRUCT);
  h->hash_offset = sizeof(SCM_HEADER_STRUCT) + n_bytes * sizeof(long);
  h->prefix_offset = sizeof(SCM_HEADER_STRUCT) + n_bytes *
                     (sizeof(long) + sizeof(HASH_STRUCT));
  h->text_offset = sizeof(SCM_HEADER_STRUCT) +
                   n_bytes * (sizeof(long) + sizeof(HASH_STRUCT)
                              + sizeof(PREFIX_STRUCT));
  h->text_pos = 0;
  h->indeces_offset = sizeof(SCM_HEADER_STRUCT) + n_bytes *
                      (sizeof(long) + sizeof(HASH_STRUCT)
                       + sizeof(PREFIX_STRUCT) + sizeof(char));
  s->header = h;
  s->text_pos = &h->text_pos;
  s->n_bytes = h->n_bytes;
  s->free_hash_nodes = &h->free_hash_nodes;
  s->free_prefix_nodes = &h->free_prefix_nodes;
  s->hash_root = (long *)&o[h->hash_root_offset];
  s->hashee = (HASH_STRUCT *)&o[h->hash_offset];
  s->prefix = (PREFIX_STRUCT *)&o[h->prefix_offset];
  s->text =   (char *)&o[h->text_offset];
  s->indeces = (long *)&o[h->indeces_offset];

  for (i = 0; i < n_bytes; i++)
  {
    s->hash_root[i] = NULL_INDEX;
    s->text[i] = 0;
    s->hashee[i].key = 0;
    s->hashee[i].next = i + 1;
    s->hashee[i].first = NULL_INDEX;
    s->prefix[i].offset = NULL_INDEX;
    s->prefix[i].next = i + 1;
    s->indeces[i] = NULL_INDEX;
  }
  s->hashee[n_bytes - 1].next = NULL_INDEX;
  s->prefix[n_bytes - 1].next = NULL_INDEX;
}

//map a classifier state, make a new one if need be
static void map_file(SCM_STATE_STRUCT *s, char *filename)
{
  struct stat statbuf;

  s->header = NULL;

  if (stat(filename, &statbuf))
  {
    long filesize;
    FILE *f;
    void *space;

    filesize = sizeof(SCM_HEADER_STRUCT) +
               n_bytes *
               (sizeof(long) +
                sizeof(HASH_STRUCT) +
                sizeof(PREFIX_STRUCT) +
                sizeof(char) +
                sizeof(long)
               );
    f = fopen(filename, "wb");
    if (f == NULL)
    {
      fatalerror("For some reason, I was unable to write-open the file named ",
                 filename);
      return;
    }
    else
    {
      CRM_PORTA_HEADER_INFO classifier_info = { 0 };

      classifier_info.classifier_bits = CRM_FSCM;

      if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
      {
        fatalerror_ex(SRC_LOC(),
                      "\n Couldn't write header to file %s; errno=%d(%s)\n",
                      filename, errno, errno_descr(errno));
        fclose(f);
        return;
      }

      if (file_memset(f, 0, filesize))
      {
        fatalerror_ex(SRC_LOC(),
                      "\n Couldn't write to file %s; errno=%d(%s)\n",
                      filename, errno, errno_descr(errno));
        fclose(f);
        return;
      }
      fclose(f);
    }
    if (stat(filename, &statbuf))
    {
      fatalerror("For some reason, I was unable to determine the file properties after creating the file! ",
                 filename);
      return;
    }

    space = crm_mmap_file(filename,
                          0,
                          statbuf.st_size,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
					CRM_MADV_RANDOM,
                          &filesize);
    if (space == MAP_FAILED)
    {
      nonfatalerror("failed to do mmap of freshly created file", filename);
      return;
    }
    make_scm_state(s, space);
  }
  else
  {
    char *o;
    SCM_HEADER_STRUCT *h;

    s->header = crm_mmap_file(filename,
                              0,
                              statbuf.st_size,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
					CRM_MADV_RANDOM,
                              NULL);
    if (s->header == MAP_FAILED)
    {
      nonfatalerror("failed to do mmap of existing file", filename);
      return;
    }
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
    s->indeces = (long *)&o[h->indeces_offset];
  }
  s->learnfilename = filename;
}

static void unmap_file(SCM_STATE_STRUCT *s)
{
  crm_munmap_file((void *)s->header);

#if 0  /* now touch-fixed inside the munmap call already! */
#if defined (HAVE_MMAP) || defined (HAVE_MUNMAP)
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .css ile, to force
  //    the filesystem to repropagate it's caches.
  //
  crm_touch(s->learnfilename);
#endif
#endif
}

//are the three charecters at b the same as the three characters in the stored
// text? wrapping around the stored text buffer if need be
static int match_prefix(SCM_STATE_STRUCT *s, long a, char *b)
{
  if (s->text[a++] != b[0])
    return 0;

  if (a == s->n_bytes)
    a = 0;
  if (s->text[a++] != b[1])
    return 0;

  if (a == s->n_bytes)
    a = 0;
  if (s->text[a] != b[2])
    return 0;

  return 1;
}

//whats the hashcode of the three characters at spot a in the stored text
static crmhash_t get_text_hash(SCM_STATE_STRUCT *s, long a)
{
  char b[3];

  b[0] = s->text[a++];
  if (a == s->n_bytes)
    a = 0;
  b[1] = s->text[a++];
  if (a == s->n_bytes)
    a = 0;
  b[2] = s->text[a];
  return strnhash(b, 3);
}

//get the three characters from the stored text
static void copy_prefix(SCM_STATE_STRUCT *s, long a, char *b)
{
  b[0] = s->text[a++];
  if (a == s->n_bytes)
    a = 0;
  b[1] = s->text[a++];
  if (a == s->n_bytes)
    a = 0;
  b[2] = s->text[a];
}

//check every global structure for consistency, if there's a bug, this will
// find it! We just go into a loop on error here because the only apropriate
// action to be taken is to attach the debugger
static int audit_structs(SCM_STATE_STRUCT *s)
{
  long i, j, k;
  long n_p = 0, n_h = 0;

  for (i = 0; i < s->n_bytes; i++)
  {
    if (s->hash_root[i] != NULL_INDEX)
    {
      if (s->hashee[s->hash_root[i]].prev != -i - 1)
      {
        fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                         " report");
        return -1;
      }
      for (j = s->hash_root[i]; j != NULL_INDEX; j = s->hashee[j].next)
      {
        n_h++;
        if (s->hashee[j].next != NULL_INDEX
            && s->hashee[s->hashee[j].next].prev != j)
        {
          fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                           " report");
          return -1;
        }
        if (s->prefix[s->hashee[j].first].prev != -j - 1)
        {
          fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                           " report");
          return -1;
        }
        for (k = s->hashee[j].first; k != NULL_INDEX; k = s->prefix[k].next)
        {
          n_p++;
          if (s->prefix[k].next != NULL_INDEX
              && s->prefix[s->prefix[k].next].prev != k)
          {
            fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                             " report");
            return -1;
          }
          if (!match_prefix(s, s->prefix[k].offset, s->hashee[j].prefix_text))
          {
            fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                             " report");
            return -1;
          }
        }
      }
    }
  }
  for (i = *s->free_hash_nodes;
       i != NULL_INDEX && n_h < s->n_bytes + 1;
       i = s->hashee[i].next)
  {
    n_h++;
  }
  if (n_h != s->n_bytes)
  {
    fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                     " report");
    return -1;
  }
  for (i = *s->free_prefix_nodes;
       i != NULL_INDEX && n_p < s->n_bytes + 1;
       i = s->prefix[i].next)
  {
    n_p++;
  }
  if (n_p != s->n_bytes)
  {
    fatalerror("FSCM: INCONSISTENT INTERNAL STATE!", "Please submit bug"
                                                     " report");
    return -1;
  }
  return 0;
}

//insert the three character prefix starting at postion t into the tables for
// fast lookup in the future. This happens to every contiguous three characters
// in a document during learning, right after concatenating it into the stored
// text
//
static long add_prefix(SCM_STATE_STRUCT *s, long t)
{
  unsigned long key = get_text_hash(s, t);
  long i = s->hash_root[key % s->n_bytes], j;

  //find the proper hashnode or set i to NULL_INDEX
  while (!(i == NULL_INDEX
           || (s->hashee[i].key == key
               && match_prefix(s, t, s->hashee[i].prefix_text)
           )
         ))
  {
    i = s->hashee[i].next;
  }

  if (i == NULL_INDEX)
  {
    //we need to insert a new hashnode

    //grab a fesh hash struct
    i = *s->free_hash_nodes;
    *s->free_hash_nodes = s->hashee[i].next;

    //insert it at front of chain
    j = key % s->n_bytes;
    s->hashee[i].prev = -j - 1;
    s->hashee[i].next = s->hash_root[j];
    if (s->hash_root[j] != NULL_INDEX)
      s->hashee[s->hash_root[j]].prev = i;
    s->hash_root[j] = i;

    //fill  in key and prefix
    s->hashee[i].key = key;
    copy_prefix(s, t, s->hashee[i].prefix_text);
    //grab fresh prefix node and make it start of chain from this hash
    j = s->hashee[i].first = *s->free_prefix_nodes;
    *s->free_prefix_nodes = s->prefix[j].next;
    s->prefix[j].prev = -i - 1;
    s->prefix[j].next = NULL_INDEX;
    s->prefix[j].offset = t;
    //increment feature count for this classifier
    s->header->n_features++;
  }
  else
  {
    //i is the proper hashnode to stick a new prefix to so grab a fresh prefix
    j = *s->free_prefix_nodes;
    *s->free_prefix_nodes = s->prefix[j].next;
    //insert it at beginning of chain
    s->prefix[j].next = s->hashee[i].first;
    s->prefix[j].prev = -i - 1;
    if (s->prefix[j].next != NULL_INDEX)
      s->prefix[s->prefix[j].next].prev = j;
    s->hashee[i].first = j;
    s->prefix[j].offset = t;
    //increment feature count for this classifier
    s->header->n_features++;
  }
  return j;
}

//we're writing over the position in the text corresponding to this prefix, so
// remove it from tables or we'll have big trouble!
static void delete_prefix(SCM_STATE_STRUCT *s, long p)
{
  long i;

  if (s->prefix[p].prev < 0)
  {
    //this prefix is the start of a chain, we need to update a hashnode
    i = -(s->prefix[p].prev) - 1;
    if (s->prefix[p].next == NULL_INDEX)
    {
      //this prefix was the only one attached to the hash so delete the hash too
      if (s->hashee[i].prev < 0)  //was this hash the first in its chain?
        s->hash_root[-(s->hashee[i].prev) - 1] = s->hashee[i].next;
      else
        s->hashee[s->hashee[i].prev].next = s->hashee[i].next;
      if (s->hashee[i].next != NULL_INDEX)
        s->hashee[s->hashee[i].next].prev = s->hashee[i].prev;
      //free hash node
      s->hashee[i].next = *s->free_hash_nodes;
      *s->free_hash_nodes = i;
      //free prefix node
      s->prefix[p].offset = NULL_INDEX;
      s->prefix[p].prev = NULL_INDEX;
      s->prefix[p].next = *s->free_prefix_nodes;
      *s->free_prefix_nodes = p;
      //decrement feature count for this classifier
      s->header->n_features--;
      return;
    }
    else
    {
      //this hash node has other prefixes so just update hash node
      s->hashee[i].first = s->prefix[p].next;
      //and free prefix node
      s->prefix[s->prefix[p].next].prev = s->prefix[p].prev;
      s->prefix[p].offset = NULL_INDEX;
      s->prefix[p].prev = NULL_INDEX;
      s->prefix[p].next = *s->free_prefix_nodes;
      *s->free_prefix_nodes = p;
      //decrement feature count
      s->header->n_features--;
      return;
    }
  }
  else
  {
    //this prefix was not the start of a chain
    s->prefix[s->prefix[p].prev].next = s->prefix[p].next;
  }

  if (s->prefix[p].next != NULL_INDEX)
  {
    s->prefix[s->prefix[p].next].prev = s->prefix[p].prev;
  }
  s->prefix[p].offset = NULL_INDEX;
  s->prefix[p].prev = NULL_INDEX;
  s->prefix[p].next = *s->free_prefix_nodes;
  *s->free_prefix_nodes = p;
  s->header->n_features--;
}

//find the longest match of contiguous characters to *text in stored text
static void find_longest_match(SCM_STATE_STRUCT *s,
                               char             *text,
                               long              max_len,
                               long             *prefix,
                               long             *len)
{
  crmhash_t key;
  long i, j, k;

  if (max_len < 3)
  {
    //then we don't have enough text to lookup a three character prefix
    *prefix = NULL_INDEX;
    *len = 0;
    return;
  }

  //get a hashcode and find the hashchain for the three characters that start
  // the string we're matching
  key = strnhash(text, 3);
  i = s->hash_root[key % s->n_bytes];

  //find the right hashnode or set i to NULL_INDEX
  while (!(i == NULL_INDEX
           ||  (s->hashee[i].key == key
                && text[0] == s->hashee[i].prefix_text[0]
                && text[1] == s->hashee[i].prefix_text[1]
                && text[2] == s->hashee[i].prefix_text[2]
           )))
  {
    i = s->hashee[i].next;
  }

  if (i == NULL_INDEX)
  {
    //then there was no hashnode for these three characters
    *prefix = NULL_INDEX;
    *len = 0;
    return;
  }
  //else go through the prefix chain and find the longest match
  *len = 0;
  *prefix = NULL_INDEX;
  //loop over all the spots in the stored text where the first three characters
  // in *text happened contiguously
  for (i = s->hashee[i].first; i != NULL_INDEX; i = s->prefix[i].next)
  {
    k = (s->prefix[i].offset + 3) % s->n_bytes;
    for (j = 3; j < max_len && s->text[k] == text[j]; j++)
    {
      if (++k == s->n_bytes)
        k = 0;
    }
    if (j > *len)
    {
      *prefix = i;
      *len = j;
    }
  }
}

//find all substring matches of three characters or more between *t and the
// stored text which do not overlap, giving preference to longest first, put
// the corresponding matches indeces from the stored text in *starts, from *t
// into *locals and the lengths of those matches into *lens, return the number
// of matches
//
static int deflate(SCM_STATE_STRUCT *s, char *t, long len, long *starts, long
                   *locals, long *lens, long max_n)
{
  //at each place in *t remember the best match found so far, bmi[...] is a
  // prefix node index, bml[...] is a length, open[..] is whether or not we can
  // still make a match at this spot
  long *bmi = (long *)inbuf;
  long *bml = (long *)outbuf;
  int *open = (int *)tempbuf;
  long i, j, k, n;

  //fill arrays
  for (i = 0; i < len; i++)
  {
    find_longest_match(s, t + i, len - i, &bmi[i], &bml[i]);
    open[i] = 1;
  }
  //and make matches until we reach the maximum number or can not make anymore
  // nonoverlapping matches
  n = 0;
  for ( ; ;)
  {
    //find longest potential match not yet made
    j =  NULL_INDEX;
    k = 0;
    for (i = 0; i < len; i++)
    {
      if (open[i] && bml[i] > k)
      {
        j = i;
        k = bml[j];
      }
    }
    //if we couldn't find one then we're done
    if (j == NULL_INDEX)
      break;
    //push this match to return it
    *starts++ = s->prefix[bmi[j]].offset;
    *locals++ = j;
    *lens++ = k;
    //increment count and bail if we're full
    if (++n >= max_n)
      break;
    //close all positions we just matched over
    k += j;
    for (i = j; i < k; i++)
      open[i] = 0;
    //readjust potential matches so not to overlap with the one we just made
    for (i = 0; i < j; i++)
    {
      if (open[i] && i + bml[i] >= j)
      {
        find_longest_match(s, t + i, j - i, &bmi[i], &bml[i]);
      }
    }
  }
  return n;
}

//we need to raise substring match lengths to a power quickly
static int pow_table2_init = 1;
static double pow_table2[256];

static double power2(long i)
{
  double pow2 = 1.5;

  //who would have thought that this would give best results?
  //it was thought that 2.0 would be most justifiable.
  //1.0 would be almost identical to seeing how much we could compress a text
  //with LZ77, lacking only huffman coding.
  //    (verified against TREC 05 corpus, at least)
  if (i >= 256)
  {
    return pow(i, pow2);
  }

  if (pow_table2_init)
  {
    long j;

    pow_table2_init = 0;
    for (j = 0; j < 256; j++)
    {
      pow_table2[j] = pow(j, pow2);
    }
  }

  return pow_table2[i];
}

//this is the number of substring matches we can make,
// > one third of max length of input is useless since each match must be of at
// least three characters, less just means less accurate
#define MAX_N 1065

//deflate document and give points for each substring match
static double score_document(SCM_STATE_STRUCT *s, char *doc, long len)
{
  double score = 0.0;
  long i, n;
  long starts[MAX_N], locals[MAX_N], lens[MAX_N];

  n = deflate(s, doc, len, starts, locals, lens, MAX_N);
  for (i = 0; i < n; i++)
    score += power2(lens[i] - 2);
  return score;
}

static void refute_document(SCM_STATE_STRUCT *s, char *doc, long len)
{
  long starts[MAX_N], locals[MAX_N], lens[MAX_N], i, j, k, n;

  n = deflate(s, doc, len, starts, locals, lens, MAX_N);
  for (i = 0; i < n; i++)
  {
    for (j = 0, k = starts[i]; j < lens[i]; j++, k++)
    {
      if (k == s->n_bytes)
        k = 0;
      if (s->indeces[k] != NULL_INDEX)
      {
        delete_prefix(s, s->indeces[k]);
        s->indeces[k] = NULL_INDEX;
      }
    }
  }
}

//entry point for learning
int crm_expr_fscm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                        char *txtptr, long txtstart, long txtlen)
{
  char filename[MAX_PATTERN];
  char htext[MAX_PATTERN];
  long htext_len;

  SCM_STATE_STRUCT S, *s = &S;

  long i, j;
  long doc_start;

  if (internal_trace)
    fprintf(stderr, "entered crm_expr_fscm_learn (learn)\n");

  //parse out .fscm file name
  crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
  htext_len = apb->p1len;
  htext_len = crm_nexpandvar(htext, htext_len, MAX_PATTERN);

  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;
  htext[j] = 0;
  strcpy(filename, &htext[i]);

  //   Check to see if user specified the file length
  //    This is the one global that can be passed in from crm.
  //     It determines the amount of previous text we remember
  //      1 megabyte gives good accuracy and enough speed for TREC
  //       512K is twice as fast but ever so slightly less accurate

  if (sparse_spectrum_file_length == 0)
  {
    n_bytes = 1048576;
  }
  else
  {
    n_bytes = sparse_spectrum_file_length;
  };

  //map it
  map_file(s, filename);
  if (!s->header)
  {
    //then we couldn't map the file and already whined about it
    return 0;
  }

  /* Shouldn't this go _without_ the 'only when internal_trace is ON' as we want to validate at _all_ times? */
  if (internal_trace && audit_structs(s))
  {
    return 0;
  }

  txtptr += txtstart;

  doc_start = *s->text_pos;

  if (apb->sflags & CRM_REFUTE)
  {
    refute_document(s, txtptr, txtlen);
    s->header->n_trains--;
  }
  else
  {
    //remember how many documents we've eaten
    s->header->n_trains++;

    //cat it to our other text
    for (i = 0; i < txtlen; i++)
    {
      //delete previous prefixes here so that we get all of them
      if (s->indeces[*s->text_pos] != NULL_INDEX)
        delete_prefix(s, s->indeces[*s->text_pos]);
      s->indeces[*s->text_pos] = NULL_INDEX;

      s->text[(*s->text_pos)++] = txtptr[i];
      if (*s->text_pos >= s->n_bytes)
        *s->text_pos -= s->n_bytes;
    }

    if (internal_trace && audit_structs(s))
    {
      return 0;
    }

    //cache all the three character prefixes
    for (i = doc_start, j = txtlen; j > 2; i++, j--)
    {
      if (i >= s->n_bytes)
        i = 0;
      s->indeces[i] = add_prefix(s, i);
    }
  }
  if (internal_trace)
    fprintf(stderr, "leaving crm_expr_fscm_learn (learn)\n");
  if (internal_trace && audit_structs(s))
  {
    return 0;
  }
  unmap_file(s);
  crm_force_munmap_filename(s->learnfilename);
  return 0;
}

//pR is hard to figure for this guy but this one seems to do ok
static double calc_pR(double p)
{
  double m = 10.0 *fabs(p - 0.5);

  m = pow(m, 3.32);
  return p < 0.5 ? -m : m;
}


//entry point for classifying
int crm_expr_fscm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                           char *txtptr, long txtstart, long txtlen)
{
  SCM_STATE_STRUCT S, *s = &S;

  char filenames_field[MAX_PATTERN];
  long filenames_field_len;
  char filenames[MAX_CLASSIFIERS][MAX_FILE_NAME_LEN];

  char out_var[MAX_PATTERN];
  long out_var_len;

  char params[MAX_PATTERN];
  long params_len;

  regex_t regee;   //for extracting params
  regmatch_t pp[2];

  long i, j, k, n_classifiers;

  long fail_on = MAX_CLASSIFIERS;   //depending on where the vbar is

  double scores[MAX_CLASSIFIERS],
         probs[MAX_CLASSIFIERS],
         norms[MAX_CLASSIFIERS],
         bn, pR[MAX_CLASSIFIERS];
  long n_features[MAX_CLASSIFIERS];
  long out_pos;

  double tot_score = 0.0, suc_prob = 0.0, suc_pR;
  long max_scorer, min_scorer;

  //grab filenames field
  crm_get_pgm_arg(filenames_field, MAX_PATTERN, apb->p1start, apb->p1len);
  filenames_field_len = apb->p1len;
  filenames_field_len =
    crm_nexpandvar(filenames_field, filenames_field_len, MAX_PATTERN);

  //grab output variable name
  crm_get_pgm_arg(out_var, MAX_PATTERN, apb->p2start, apb->p2len);
  out_var_len = apb->p2len;
  out_var_len = crm_nexpandvar(out_var, out_var_len, MAX_PATTERN);

  //check second slashed group for parameters
  crm_get_pgm_arg(params, MAX_PATTERN, apb->s2start, apb->s2len);
  params_len = apb->s2len;
  params_len = crm_nexpandvar(params, params_len, MAX_PATTERN);
  params[params_len] = 0;
  if (crm_regcomp(&regee, "n_bytes[[:space:]]*=[[:space:]]*([0-9]+)",
                  40, REG_EXTENDED))
  {
    //This should never ever happen
    fprintf(stderr, "regex compilation problem! I'm about to segfault!\n");
  }
  else if (!crm_regexec(&regee, params, params_len, 2, pp, 0, NULL))
  {
    params[pp[1].rm_eo] = 0;
    n_bytes = atol(params + pp[1].rm_so);
  }

  //a tiny automata for your troubles to grab the names of our classifier files
  // and figure out what side of the "|" they're on
  for (i = 0, j = 0, k = 0; i < filenames_field_len && j < MAX_CLASSIFIERS; i++)
  {
    if (filenames_field[i] == '\\')    //allow escaped in case filename is wierd
    {
      filenames[j][k++] = filenames_field[++i];
    }
    else if (crm_isspace(filenames_field[i]) && k > 0)
    {
      //white space terminates filenames
      filenames[j][k] = 0;
      k = 0;
      j++;
    }
    else if (filenames_field[i] == '|')
    {
      //found the bar, terminate filename if we're in one
      if (k > 0)
      {
        k = 0;
        j++;
      }
      fail_on = j;
    }
    else if (crm_isgraph(filenames_field[i]))      //just copy char otherwise
    {
      filenames[j][k++] = filenames_field[i];
    }
  }

  if (j < MAX_CLASSIFIERS)
    filenames[j][k] = 0;
  if (k > 0)
    n_classifiers = j + 1;
  else
    n_classifiers = j;

  if (internal_trace)
  {
    fprintf(stderr, "fail_on = %ld\n", fail_on);
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "filenames[%ld] = %s\n", i, filenames[i]);
  }
  ;

  //loop over classifiers and calc scores
  for (i = 0; i < n_classifiers; i++)
  {
    map_file(s, filenames[i]);
    if (!s->header)
    {
      //then we couldn't map this guy for some reason and already wined to
      // strderr
      n_features[i] = 0;
      scores[i] = 0;
      continue;
    }
    ;
    n_features[i] = s->header->n_features;
    norms[i] = (double)s->header->n_trains;
    scores[i] = score_document(s, txtptr + txtstart, txtlen);
    if (internal_trace && audit_structs(s))
    {
      return 0;
    }
    unmap_file(s);
  }

  if (internal_trace)
  {
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "scores[%ld] = %f\n", i, scores[i]);
  }


  max_scorer = 0;
  for (j = 1; j < n_classifiers; j++)
  {
    if (scores[j] > scores[max_scorer])
      max_scorer = j;
  }
  min_scorer = 0;
  for (j = 1; j < n_classifiers; j++)
  {
    if (scores[j] < scores[min_scorer])
      min_scorer = j;
  }
  //subtract 80% of lowest score from everybody to remove features having to
  //do with medium
  bn = scores[min_scorer] * 0.8;

  out_pos = 0;

  tot_score = 0.0;
  for (j = 0; j < n_classifiers; j++)
    probs[j] = scores[j] - bn;

  for (j = 0; j < n_classifiers; j++)
    tot_score += probs[j];
  if (tot_score > 0.0)
  {
    for (j = 0; j < n_classifiers; j++)
    {
      probs[j] /= tot_score;
    }
  }
  else
  {
    for (j = 0; j < n_classifiers; j++)
    {
      probs[j] = 1.0 / n_classifiers;
    }
  }

  for (j = 0; j < fail_on; j++)
  {
    suc_prob += probs[j];
  }
  suc_pR = calc_pR(suc_prob);
  for (j = 0; j < n_classifiers; j++)
  {
    pR[j] = calc_pR(probs[j]);
  }

  if (internal_trace)
  {
    fprintf(stderr, "suc_prob = %f\n", suc_prob);
    fprintf(stderr, "tot_score = %f\n", tot_score);
    for (i = 0; i < n_classifiers; i++)
      fprintf(stderr, "scores[%ld] = %f\n", i, scores[i]);
  }

  if (suc_prob > 0.5)    //test for nan as well
  {
    out_pos += sprintf(outbuf + out_pos,
                       "CLASSIFY succeeds; success probability: %f  pR: %6.4f\n",
                       suc_prob, suc_pR);
  }
  else
  {
    out_pos += sprintf(outbuf + out_pos,
                       "CLASSIFY fails; success probability: %f  pR: %6.4f\n",
                       suc_prob, suc_pR);
  }

  /* [i_a] GROT GROT GROT: %s in sprintf may cause buffer overflow. not fixed in this review/scan */
  out_pos += sprintf(outbuf + out_pos,
                     "Best match to file #%ld (%s) prob: %6.4f  pR: %6.4f\n",
                     max_scorer,
                     filenames[max_scorer],
                     probs[max_scorer], pR[max_scorer]);

  out_pos += sprintf(outbuf + out_pos,
                     "Total features in input file: %ld\n",
                     txtlen);

  for (i = 0; i < n_classifiers; i++)
  {
    /* [i_a] GROT GROT GROT: %s in sprintf may cause buffer overflow. not fixed in this review/scan */
    out_pos += sprintf(outbuf + out_pos,
                       "#%ld (%s): features: %ld, score:%3.2e, prob: %3.2e,"
                       "pR: %6.2f\n",
                       i, filenames[i],
                       n_features[i], scores[i],
                       probs[i], pR[i]);
  }

  if (out_var_len)
    crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_pos);

  if (suc_prob <= 0.5)
  {
    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
    csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
  }
  return 0;
}


#else /* CRM_WITHOUT_FSCM */

int crm_expr_fscm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                        char *txtptr, long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "FSCM");
}


int crm_expr_fscm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                           char *txtptr, long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "FSCM");
}

#endif

