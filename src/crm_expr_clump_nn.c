//	crm_expr_nn_clump.c

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
//     crm_expr_nn_clump.c - translate characters of a string.
//    
//     Original spec by Bill Yerazunis, original code by Joe Langeway,
//     recode for CRM114 use by Bill Yerazunis. 
//
//     This code section (crm_expr_nn_clump and subsidiary routines) is
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

Documents get tokenized. We remember the MAX_TOKENS most recent
tokens and compute statistics for the MAX_COR_TOKENS most frequent
tokens. We store all the info we need to quickly
compute statistical correlation between tokens. We make a graph where
each token is connected to the token it is most statistically
correlated to. The connected regions of the graph are clusters.
Non-connected tokens are in different clusters. All this is done
incrementally so that we can learn a document at a time. Actually
labeling tokens with associated clusters is super fast.

We have a few data structures for this: A hash table to remember
tokens, tokenization first involves splitting the document by a regex
and then hashing the tokens. Same hash code means same token as far as
we care. The first MAX_TOKENS hash slots are the root and the rest of
the slots are for chains. There are currently 2 * MAX_TOKENS hash
slots, which is one more than we need in the worst case.
		
The tokens are represented by TOKEN_STRUCT's which are initially in a
linked list of free nodes, but are placed in a linked list ordered by
recency of the token so that we can dump the least recent token when a
new slot is needed and we're all out. They are further sorted by their
frequency. That order is updated as occurences are accumulated by the
document scorer. Tokens don't remember their actual text but will
probably be augmented to do that soon for debugging.
	
TOKEN_STRUCTS contain a field called cor_index which is set to point
into the array cor_tokens if that token is one of the MAX_COR_TOKEN's
most frequent. This way we can remember many more tokens than we make
statistics for and not get clobbered by hapaxes.
	
There is a large array of COOCCURRENCE_SCORE_TYPE to remember all the
counts of cooccurences. It is only the lower triangle of a symetric
matrix.  actual cooccurence scores are computed by score_cooccurance
so that occerance information can be turned into actual statistical
correlation or something like it. Documents are actually scored by
pluggable functions as well, this file contains one which considers
the events of there or not-there occurrence in documents and another
which slides a window of width 100 over a stream of tokens and scores
an occurrence for every appearence of a token and a fractional
cooccurence of 1 / d every time two tokens appear d tokens apart.
	 
There is a graph which is represented by a linked list attachted to
each token.  Tokens know which cluster they're in but clusters do not
know which tokens they have.
	
Currently it takes 20 minutes and 3 seconds seconds to learn the first
10000 emails of the ham50 subset of the trec 2005 corpus with the flat
scorer.  I'm still in the process of interpreting the resulting
statistics.
	
*/


//  include some standard files
#include "crm114_sysincludes.h"
//  include any local crm114 configuration file
#include "crm114_config.h"
//  include the crm114 data structures file
#include "crm114_structs.h"
//  and include the routine declarations file
#include "crm114.h"
// finally include stuff specific to this beast
#include "crm_expr_clump_nn.h"

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

//end crap that appears in every file
*/

//GLOBALS parameters passed from crm

/* [i_a] */
static long max_tokens = 0;
static long max_cor_tokens = 0;
static double flat_weight = 0.0;
static double sharp_weight = 0.0;
static double gauss_weight = 0.0;
static double gauss_sigma = 0.0;

//GLOBALS for debugging only, Joe hates globals


//  THESE HAVE ALL BEEN REPLACED BY FLAGS SO SETTINGS HERE MEAN NOTHING,
//  THOUGH THESE ARE STILL THE GLOBALS WE USE
static int joe_trace = 0;

/* [i_a] */
static int go_flat = 0; //select scoring method


// GROT GROT GROT we F'ed up! - should be NONFATALERROR

static void break_point()
{
	fprintf(stderr, "\n\npoop!\n");
	if (engine_exit_base != 0)
		exit (engine_exit_base + 23);
	else
		exit (EXIT_FAILURE);
}

//PROTOTYPES

     // this function examines the clusters file and prints useful statistics
static void poke_around(CLUSTEROR_STATE_STRUCT *s); 


//  This function turns accumulated data into a normalized statistic,
//  high values mean put these tokens in same cluster, i and j are
//  indeces of the array tokens, not cor_tokens
static COOCCURRENCE_SCORE_TYPE score_cooccurance 
          (CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j);

//  function for bit sets
static int get_bit(long *b, long i);
static void set_bit(long *b, long i, int a);

//  This is the only acceptable way to accumlate occurences of tokens
//  as incremental calculations are called from these, again i and j
//  are indeces of the array tokens, not cor_tokens
static COOCCURRENCE_SCORE_TYPE get_occ(CLUSTEROR_STATE_STRUCT *s, index_t i);
static void add_occ(CLUSTEROR_STATE_STRUCT *s, index_t i, COOCCURRENCE_SCORE_TYPE a);

//  Likewise for co-occurences
static COOCCURRENCE_SCORE_TYPE get_cooc(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j);
static void add_cooc(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j, COOCCURRENCE_SCORE_TYPE a);

//  these work on the graph, they do not call the split and join
//  methods, see update_graph_and_clusters() for that action
static void set_edge(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j, int e);
static int get_edge(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j);

//  token handling goodness
static void delete_from_hash(CLUSTEROR_STATE_STRUCT *s, index_t token, long key);
static void dump_least_recent_token(CLUSTEROR_STATE_STRUCT *s);
static index_t grab_fresh_token(CLUSTEROR_STATE_STRUCT *s);
static index_t get_token_from_hash(CLUSTEROR_STATE_STRUCT *s, long hash_key);

//  this guy takes text and makes tokens, calling get_token_from_hash
//  which will allocate a new token for a new hash code and even drop
//  an old one to make room
static void learning_tokenize
	(	CLUSTEROR_STATE_STRUCT *s,
		index_t *t, int *n, int max,
		char *text, long len,
		regex_t *regee	);
		
//  This guy puts an initial state into s and a huge block on
//  contiguous memory starting at s->header
static void make_new_clusteror_state(CLUSTEROR_STATE_STRUCT *s);

//  obvious
static void map_file_for_learn(CLUSTEROR_STATE_STRUCT *s, char *filename);
static void unmap_file_for_learn(CLUSTEROR_STATE_STRUCT *s);

//  if we refuted we couldn't figure out the nearest neighbor
//  incrementally, so instead we marked the bit set token_nn_changed
//  with all the tokens whose cooccurence values had changed, now we
//  go through and compute nearest neighbors and record changes back
//  in token_nn_changed
static void find_NNs_for_unlearning(CLUSTEROR_STATE_STRUCT *s);

//  add_occ marks the closed list for each token which acumulates
//  occurences over the current document, this guy goes through and
//  brings all those tokens to the front of the recency list
static void update_seen_tokens(CLUSTEROR_STATE_STRUCT *s);

//  document scorers, they consume documents and update statistics by
//  calling add_N, add_occ, and add_cooc in that order, go take a
//  look!
static void score_document_mix(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense);
static void score_document_sharp(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense);
static void score_document_gauss(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense);
static void score_document_flat(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense);


//  clusters remember how often they occur so we always use this guy to
//  change cluster asignments and update cluster info
static void change_cluster(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t c);

//  we've joined all the tokens of this cluster to another and wish to
//  free up the cluster slot
static void give_back_cluster(CLUSTEROR_STATE_STRUCT *s, index_t c);

//  get a hot and delicious cluster slot fresh of the free list
static index_t get_fresh_cluster(CLUSTEROR_STATE_STRUCT *s);

//  i and j are indices to cor_tokens and their clusters will be
//  joined, i and j must be connected first and have different cluster
//  asignments, the more frequent cluster label is used for the final
//  larger group and the less frequent given back, we also deal with
//  the possibility that cluster asignments have not yet been made
static void join_clusters(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j);

//  this is the magical graph monster the observes changes in nearest
//  neighbor relationships and updates the graph and clusters
static void update_graph_and_clusters(CLUSTEROR_STATE_STRUCT *s);

//  for plumc operations this function tokenizes but marks new tokens
//  with HAPAX_INDEX instead of remembering them
static void nolearning_tokenize
	(	CLUSTEROR_STATE_STRUCT *s,
		index_t *t, int *n, int max,
		char *text, long len,
		regex_t *regee	);

//  this is the total of accumulated occurences
static COOCCURRENCE_SCORE_TYPE get_N(CLUSTEROR_STATE_STRUCT *s);
static COOCCURRENCE_SCORE_TYPE add_N(CLUSTEROR_STATE_STRUCT *s, COOCCURRENCE_SCORE_TYPE a);

//  just look it up or return NULL_INDEX
static index_t simple_hash_lookup(CLUSTEROR_STATE_STRUCT *s, long hash_key);

//  called by add_occ to update frequency list
static void add_token_count(CLUSTEROR_STATE_STRUCT *s, index_t t, COOCCURRENCE_SCORE_TYPE c);

//  for debugging
static void print_header(CLUSTEROR_STATE_STRUCT *s);
static void diagnose_hash_table(CLUSTEROR_STATE_STRUCT *s);
static void audit_frequency_list(CLUSTEROR_STATE_STRUCT *s);
static void audit_nns(CLUSTEROR_STATE_STRUCT *s);

#ifdef NOTUSED
static COOCCURRENCE_SCORE_TYPE score_cooccurance_quick_and_dirty(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
	return (get_cooc(s,i,j) - get_occ(s,i) * get_occ(s,j) / get_N(s)) / get_N(s);
}
#endif

#ifdef NOTUSED
static COOCCURRENCE_SCORE_TYPE score_cooccurance_slow_and_clean(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
	double	N = get_N(s),
			Pi = get_occ(s, i) / N,
			Pj = get_occ(s, j) / N,
			PijHyp = Pi * Pj,
			PijHat = get_cooc(s, i, j) / N,
			Z = (PijHat - PijHyp) / sqrt(PijHyp * (1 - PijHyp) / N),
			correlation = crm_norm_cdf(Z);
	return correlation;
}
#endif

static COOCCURRENCE_SCORE_TYPE score_cooccurance_slow_and_clean2(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
	double	N = get_N(s),
			Pi = get_occ(s, i) / N,
			Pj = get_occ(s, j) / N,
			PijHyp = Pi * Pj,
			PijHat = get_cooc(s, i, j) / N,
			L = PijHat / PijHyp,
			correlation = crm_log(L);
	return (COOCCURRENCE_SCORE_TYPE)correlation;
}

static COOCCURRENCE_SCORE_TYPE score_cooccurance(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
//	return score_cooccurance_quick_and_dirty(s, i, j);
	return score_cooccurance_slow_and_clean2(s, i, j);
}

//  THIS FUNCTION ASSUMES 4 BYTE LONGS!
static int get_bit(long *b, long i)
{
	return b[i >> 5] & (1 << (i & 31));
}

//  THIS FUNCTION ASSUMES 4 BYTE LONGS!
static void set_bit(long *b, long i, int a)
{
	if(a)
		b[i >> 5] |= (1L << (i & 31));
	else
		b[i >> 5] &= ~(1L << (i & 31));
}

static COOCCURRENCE_SCORE_TYPE get_N(CLUSTEROR_STATE_STRUCT *s)
{
	return s->header->tot_occ;
}

static COOCCURRENCE_SCORE_TYPE add_N(CLUSTEROR_STATE_STRUCT *s, 
				     COOCCURRENCE_SCORE_TYPE a)
{
	return s->header->tot_occ += a;
}

static COOCCURRENCE_SCORE_TYPE get_occ(CLUSTEROR_STATE_STRUCT *s, index_t i)
{
        //  this should be identical to what's in
        //  s->cooccurences[cor_index * (cor_index + 3) / 2]
	return s->tokens[i].count; 
}

static void add_occ(CLUSTEROR_STATE_STRUCT *s, 
		    index_t i, 
		    COOCCURRENCE_SCORE_TYPE a)
{
  set_bit(s->closed_list, i, 1);
  a *= s->header->normal_factor;
  add_token_count(s, i, a);
  if( (i = s->tokens[i].cor_index) != NULL_INDEX)
    {
      s->cooccurences[i * (i + 3) / 2] += a;
      if( s->cor_tokens[i].cluster != NULL_INDEX )
	s->clusters[ s->cor_tokens[i].cluster].occurrences += a;
    }
}

static COOCCURRENCE_SCORE_TYPE get_cooc(CLUSTEROR_STATE_STRUCT *s, 
					index_t i, 
					index_t j)
{
  if(i == j 
     || (i = s->tokens[i].cor_index) == NULL_INDEX 
     || (j = s->tokens[j].cor_index) == NULL_INDEX)
    return 0;
  if(i < j) {index_t t = i; i = j; j = t;}
  return s->cooccurences[i * (i + 1) / 2 + j];
}

static void audit_nns(CLUSTEROR_STATE_STRUCT *s)
{
  int f = 0;
  index_t i;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    if(s->cor_tokens[i].nearest_neihbor == i)
      f = 1;
  if(f)
    {	
      for(i = 0; i < s->header->n_cor_tokens; i++)
	fprintf(stderr, "nn(%ld) = %ld\n", 
		i, s->cor_tokens[i].nearest_neihbor);
      break_point();
    }
}

static void add_cooc(CLUSTEROR_STATE_STRUCT *s, 
		     index_t i, 
		     index_t j, 
		     COOCCURRENCE_SCORE_TYPE a)
{
  //  i and j are regular token numbers, ci and cj are thier indeces
  //  in correlation tables
  index_t ci, cj; 
  if(i == j 
     || (ci = s->tokens[i].cor_index) == NULL_INDEX 
     || (cj = s->tokens[j].cor_index) == NULL_INDEX)
    return;
  if(ci == cj)
    {
      fprintf(stderr, "Two tokens have same correlation index!\n");
      break_point();
    }
  if(ci < cj) {index_t t = i; i = j; j = t; t = ci; ci = cj; cj = t;}
  a *= s->header->normal_factor;
  if(a > 0)
    {
      s->cooccurences[ci * (ci + 1) / 2 + cj] += a;
      if ( s->cor_tokens[ci].nearest_neihbor == NULL_INDEX
	   || score_cooccurance(s,i,j) > score_cooccurance(s, i, s->cor_tokens[ s->cor_tokens[ci].nearest_neihbor ].token)	)
	{
	  if(s->old_nearest_neihbors[ci] == NULL_INDEX)
	    s->old_nearest_neihbors[ci] = s->cor_tokens[ci].nearest_neihbor;
	  s->cor_tokens[ci].nearest_neihbor = cj;
	  set_bit(s->token_nn_changed, ci, 1);
	}
      if( s->cor_tokens[cj].nearest_neihbor == NULL_INDEX
	  || score_cooccurance(s,i,j) 
	  > score_cooccurance(s, j, s->cor_tokens[ s->cor_tokens[cj].nearest_neihbor ].token)	)
	{
	  //  if(joe_trace) fprintf(stderr, "%ld is %ld's new nearest neighbor!\n", ci, cj);
	  if(s->old_nearest_neihbors[cj] == NULL_INDEX)
	    s->old_nearest_neihbors[cj] = s->cor_tokens[cj].nearest_neihbor;
	  s->cor_tokens[cj].nearest_neihbor = ci;
	  set_bit(s->token_nn_changed, cj, 1);
	}
    } else if (a < 0)
    {
      s->cooccurences[ci * (ci + 1) / 2 + cj] += a;
      set_bit(s->token_nn_changed, ci, 1);
      set_bit(s->token_nn_changed, cj, 1);
    }
  if(joe_trace) audit_nns(s);
}

static index_t grab_edge(CLUSTEROR_STATE_STRUCT *s)
{
  index_t a = s->header->first_unused_edge;
  if(a != NULL_INDEX)
    s->header->first_unused_edge = s->graph[a].next;
  else
    {
      fprintf(stderr, "We've run out of graph edges. This should never happen! PANIC! The pigeon hole principle no longer holds!\n");
      break_point();
    }
  return a;
}

static void give_back_edge(CLUSTEROR_STATE_STRUCT *s, index_t a)
{
  s->graph[a].next = s->header->first_unused_edge;
  s->header->first_unused_edge = a;
}


//  graph stuff is only touched by back end, so work with correlation
//  table indeces
static int get_edge(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
  index_t a;
  for(a = s->cor_tokens[i].edges; a != NULL_INDEX; a = s->graph[a].next)
    if(s->graph[a].edge_to == j)
      return 1;
  return 0; 
}

static void set_edge(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j, int e)
{
  //   everyone likes graphs with loops, everyone except the Nearest
  //   Neighbor Clustering Monster!
  if(i == j)
    {
      fprintf(stderr, "someone tried to make a loop in the graph!!!\n");
      break_point();
    }
  
  if(e)
    {
      if(get_edge(s, i, j))
	return;
	  { 
		  /* [i_a] it is C code, not C++ */
      index_t a = grab_edge(s), b = grab_edge(s);
      s->graph[a].edge_to = j;
      s->graph[a].next = s->cor_tokens[i].edges;
      s->cor_tokens[i].edges = a;
      s->graph[b].edge_to = i;
      s->graph[b].next = s->cor_tokens[j].edges;
      s->cor_tokens[j].edges = b;
	  }
    } else
    {
      index_t a;
      if(s->graph[ s->cor_tokens[i].edges ].edge_to == j)
	{
	  a = s->cor_tokens[i].edges;
	  s->cor_tokens[i].edges = s->graph[ a ].next;
	  give_back_edge(s, a);
	} else
	{
	  index_t b;
	  a = s->cor_tokens[i].edges;
	  for(b = s->graph[ a ].next; b != NULL_INDEX; b = s->graph[ b ].next)
	    {
	      if(s->graph[ b ].edge_to == j)
		{
		  s->graph[ a ].next = s->graph[ b ].next;
		  give_back_edge(s, b);
		  break;
		}
	      a = b;
	    }
	}
      if(s->graph[ s->cor_tokens[j].edges ].edge_to == i)
	{
	  a = s->cor_tokens[j].edges;
	  s->cor_tokens[j].edges = s->graph[ a ].next;
	  give_back_edge(s, a);
	} else
	{
	  index_t b;
	  a = s->cor_tokens[j].edges;
	  for(b = s->graph[ a ].next; b != NULL_INDEX; b = s->graph[ b ].next)
	    {
	      if(s->graph[ b ].edge_to == i)
		{
		  s->graph[ a ].next = s->graph[ b ].next;
		  give_back_edge(s, b);
		  break;
		}
	      a = b;
	    }
	}
    }
}

static void delete_from_hash(CLUSTEROR_STATE_STRUCT *s, index_t token, long key)
{
  int first_hash_level_size = s->header->max_tokens;
  index_t i = ((unsigned long)key) % first_hash_level_size, j = NULL_INDEX, k;
  for(;;)
    {
      if(i == NULL_INDEX)
	{
	  fprintf(stderr, "NNCluster: tried to delete non-existent hash code. Token: %ld, Hash: %lu\n", s->header->least_recent_token, key);
	  fprintf(stderr, "Pursued this chain:\n");
	  for(i = ((unsigned long)key) % first_hash_level_size; i != NULL_INDEX; i = s->hash_table[i].next_in_hash_chain)
	    fprintf(stderr, "	i = %ld, hash = %lu, token = %ld, next = %ld\n", i, s->hash_table[i].key, s->hash_table[i].token, s->hash_table[i].next_in_hash_chain);
	  
	  break_point();
	}
      if(s->hash_table[i].key == key || s->hash_table[i].token == token)
	{
	  if(0 && joe_trace && s->hash_table[i].key != key)
	    {
	      //  so for some reason hash slots in the first level
	      //  will often change their key by some amount divisible
	      //  by the size of the first level, the behavior has no
	      //  explanation
	      fprintf(stderr, "Couldn't find proper hash key to delete, but we found the write token number, gonna try and press on, Token: %ld, Hash: %lu\n", s->header->least_recent_token, key);
	      fprintf(stderr, "Pursued this chain:\n");
	      for(k = ((unsigned long)key) % first_hash_level_size; k != NULL_INDEX; k = s->hash_table[k].next_in_hash_chain)
		fprintf(stderr, "	i = %ld, hash = %lu, token = %ld, next = %ld\n", k, s->hash_table[k].key, s->hash_table[k].token, s->hash_table[k].next_in_hash_chain);
	    }
	  
	  if(j != NULL_INDEX)
	    {	//  remember root nodes, ones less than MAX_TOKENS don't
		//  go on the free list
	      s->hash_table[j].next_in_hash_chain = s->hash_table[i].next_in_hash_chain;
	      //  we're not in the root or else j wouldv'e been null
	      s->hash_table[i].next_in_hash_chain = s->header->first_unused_hash_slot;
	      s->header->first_unused_hash_slot = i;
	      
	      s->hash_table[i].token = NULL_INDEX;
	    }
	  else
	    {	//root! we stop searching when we hit a null token so copy list upwards
	      j = s->hash_table[i].next_in_hash_chain;
	      if( j != NULL_INDEX)
		{
		  s->hash_table[i].next_in_hash_chain = s->hash_table[j].next_in_hash_chain;
		  s->hash_table[i].token = s->hash_table[j].token;
		  
		  s->hash_table[j].next_in_hash_chain = s->header->first_unused_hash_slot;
		  s->header->first_unused_hash_slot = j;
		  
		} 
	      else
		s->hash_table[i].token = NULL_INDEX;
	    }
	  return;
	}
      j = i;
      i = s->hash_table[i].next_in_hash_chain;
    }
}

static void dump_least_recent_token(CLUSTEROR_STATE_STRUCT *s)
{
  index_t v = s->header->least_recent_token;
  while(s->tokens[v].cor_index != NULL_INDEX)
    v = s->tokens[v].more_recent;
  delete_from_hash(s, v, s->tokens[v].hash_code);
  s->tokens[v].hash_code = 22;
  if(v == s->header->least_recent_token)
    s->header->least_recent_token = s->tokens[v].more_recent;
  if(s->tokens[v].less_recent != NULL_INDEX)
    s->tokens[ s->tokens[v].less_recent ].more_recent = s->tokens[v].more_recent;
  if(s->tokens[v].more_recent != NULL_INDEX)
    s->tokens[ s->tokens[v].more_recent ].less_recent = s->tokens[v].less_recent;
  if(s->tokens[v].less_common != NULL_INDEX)
    s->tokens[ s->tokens[v].less_common ].more_common = s->tokens[v].more_common;
  if(s->tokens[v].more_common != NULL_INDEX)
    s->tokens[ s->tokens[v].more_common ].less_common = s->tokens[v].less_common;
  s->tokens[v].less_recent = s->header->first_unused_token_slot;
  s->header->first_unused_token_slot = v;
  s->header->n_tokens--;
}

//  dump least recent if no more token slots, do not fill in hash_code
static index_t grab_fresh_token(CLUSTEROR_STATE_STRUCT *s)
{
  index_t t = s->header->first_unused_token_slot;
  if(t == NULL_INDEX)
    {
      dump_least_recent_token(s);
      t = s->header->first_unused_token_slot;
    } else
    {
      s->tokens[t].cor_index = NULL_INDEX;
    }
  s->header->first_unused_token_slot = s->tokens[t].less_recent;
  
  s->tokens[t].less_recent = s->header->most_recent_token;
  if(s->header->most_recent_token != NULL_INDEX)
    s->tokens[ s->header->most_recent_token ].more_recent = t;
  s->header->most_recent_token = t;
  
  if(s->header->n_tokens == 0)
    {
      s->header->least_recent_token = t;
      s->header->least_frequent_token = t;
      s->tokens[t].more_common = NULL_INDEX;
    } else
    {
      s->tokens[t].more_common = s->header->least_frequent_token;
      s->tokens[ s->header->least_frequent_token ].less_common = t;
      s->header->least_frequent_token = t;
    }
  
  s->tokens[t].more_recent = NULL_INDEX;
  s->tokens[t].less_common = NULL_INDEX;
  s->tokens[t].count = 0;
  
  s->header->n_tokens++;
  return t;
}

static void wipe_cooccurences(CLUSTEROR_STATE_STRUCT *s, index_t t) //t is the cor_index
{
  index_t i, j; /* [i_a] it is C code, not C++ */
  if(t >= s->header->n_cor_tokens)
    {
      fprintf(stderr, "Tried to qipe a stupid token number!\n");
      break_point();
    }
  
  i = t * (t + 1) / 2;
  for(j = 0; j < t; j++)
    s->cooccurences[i + j] = 0;
  for(j = t; j < s->header->n_cor_tokens; j++)
    s->cooccurences[j * (j + 1) / 2 + t] = 0;
}

static void audit_frequency_list(CLUSTEROR_STATE_STRUCT *s)
{
  long i, j, a, b;
  //fprintf(stderr, "auditing frequency list...\n");
  for(a = s->header->least_frequent_token, i = 0; a != NULL_INDEX; a = s->tokens[a].more_common, i++)
    {
      if(s->tokens[a].more_common != NULL_INDEX && s->tokens[ s->tokens[a].more_common ].count < s->tokens[a].count)
	{
	  fprintf(stderr, "The frequency list is out of order!\n");
	  print_header(s);
	  break_point();
	}
      for(b = s->header->least_frequent_token, j = 0; b != a && j < i; b = s->tokens[b].more_common, j++);
      if(j < i)
	{
	  fprintf(stderr, "There's a token in the frequency list twice, which means a loop!\n");
	  print_header(s);
	  break_point();
	}
      for(b = a, j = 0; b != NULL_INDEX && j <= i; b = s->tokens[b].less_common, j++);
      if(j <= i)
	{
	  fprintf(stderr, "There's a break going backwards in the frequency list!\n");
	  print_header(s);
	  break_point();
	}
      if(b != NULL_INDEX)
	{
	  fprintf(stderr, "There's a backwards inconsistancy in the frequency list!\n");
	  print_header(s);
	  break_point();
	}
    }
  if(i < s->header->n_tokens)
    {
      fprintf(stderr, "Not all tokens are in the frequency list!\n");
      print_header(s);
      break_point();
    }
}

//  maintain tables to mark MAX_COR_TOKENS most frequent tokens
static void add_token_count(CLUSTEROR_STATE_STRUCT *s, index_t t, COOCCURRENCE_SCORE_TYPE c)
{
  long a, b;
  COOCCURRENCE_SCORE_TYPE count = (s->tokens[t].count += c);
  //  if(joe_trace) fprintf(stderr, "entered add_token_count\n");
  //  one line for loops are fun!
  //  fprintf(stderr,"1\n");
  //  audit_frequency_list(s);
  
  for(b = t; 
      s->tokens[b].more_common != NULL_INDEX 
	&& s->tokens[ s->tokens[b].more_common ].count < count; 
      b = s->tokens[b].more_common);
  //  b is now the index of the token we go right above

  if(b != t) //  nothing to do, we didn't change enough to move
    {
      //  take our selves out of list
      if(t == s->header->least_frequent_token)
	{	if(s->tokens[t].less_common != NULL_INDEX)
	    s->header->least_frequent_token = s->tokens[t].less_common;
	  else
	    s->header->least_frequent_token = s->tokens[t].more_common;
	}
      if(t == s->header->least_frequent_cor_token)
	do
	  s->header->least_frequent_cor_token = s->tokens[ s->header->least_frequent_cor_token ].more_common;
	while(s->tokens[ s->header->least_frequent_cor_token ].cor_index == NULL_INDEX);
      if (s->tokens[t].less_common != NULL_INDEX )
	s->tokens[ s->tokens[t].less_common ].more_common = s->tokens[t].more_common;
      if (s->tokens[t].more_common != NULL_INDEX )
	s->tokens[ s->tokens[t].more_common ].less_common = s->tokens[t].less_common;
      //put our selves where we want to be
      s->tokens[t].less_common = b;
      s->tokens[t].more_common = s->tokens[b].more_common;
      s->tokens[b].more_common = t;
      if( s->tokens[t].more_common != NULL_INDEX)
	s->tokens[ s->tokens[t].more_common ].less_common = t;
    }
  
  if(0 && joe_trace) audit_frequency_list(s);
  
  //  maintain s->header->least_frequent_cor_token and correlated tokens

  //  see if maybe we need to promote this to a correlated token
  if(s->tokens[t].cor_index == NULL_INDEX) 
    {
      // we've got more then MAX_COR_TOKENS tokens
      if( s->header->next_free_cor_token == MAX_COR_TOKENS ) 
	{
	  if(count > s->tokens[ s->header->least_frequent_cor_token ].count)
	    {
	      s->tokens[t].cor_index = s->tokens[ s->header->least_frequent_cor_token ].cor_index;
	      s->tokens[ s->header->least_frequent_cor_token ].cor_index = NULL_INDEX;
	      
	      while(s->tokens[ s->header->least_frequent_cor_token ].cor_index == NULL_INDEX)
		s->header->least_frequent_cor_token = s->tokens[ s->header->least_frequent_cor_token ].more_common;
	      
	      s->cor_tokens[ s->tokens[t].cor_index ].token = t;
	      wipe_cooccurences(s, s->tokens[t].cor_index );
	      //			if(joe_trace) fprintf(stderr, "gave token %ld cor_index %ld\n", t, s->tokens[t].cor_index);
	    }
	} else //we're still filling cor token slots
	{
	  a = s->tokens[t].cor_index = s->header->next_free_cor_token++;
	  s->header->n_cor_tokens++;
	  if(s->header->least_frequent_cor_token == NULL_INDEX || count <= s->tokens[ s->header->least_frequent_cor_token ].count )
	    s->header->least_frequent_cor_token = t;
	  s->cor_tokens[a].token = t;
	  s->cor_tokens[a].nearest_neihbor = NULL_INDEX;
	  s->cor_tokens[a].cluster = NULL_INDEX;
	  s->cor_tokens[a].edges = NULL_INDEX;
	  //		if(joe_trace) fprintf(stderr, "gave token %ld cor_index %ld\n", t, a);
	}
    }
}

static index_t simple_hash_lookup(CLUSTEROR_STATE_STRUCT *s, long hash_key)
{
  long first_hash_level_size = s->header->max_tokens;
  index_t i = (index_t)(((unsigned long)hash_key) % first_hash_level_size);
  if(s->hash_table[i].token == NULL_INDEX)
    return NULL_INDEX;
  for(	; i != NULL_INDEX	;	i = s->hash_table[i].next_in_hash_chain)
    if(s->hash_table[i].key == hash_key)
      return s->hash_table[i].token;
  return NULL_INDEX;
}

static void hash_insert(CLUSTEROR_STATE_STRUCT *s, long hash_key, index_t token)
{
  long first_hash_level_size = s->header->max_tokens;
  index_t i = (index_t)(((unsigned long)hash_key) % first_hash_level_size), j = NULL_INDEX;
  
  if(s->hash_table[i].token == NULL_INDEX) //there's a free slot in the root
    {
      s->hash_table[i].token = token;
      s->hash_table[i].key = hash_key;
      s->hash_table[i].next_in_hash_chain = NULL_INDEX;
      return;
    }
  
  for(	;	i != NULL_INDEX;	j = i, i = s->hash_table[i].next_in_hash_chain	) //go to end of hash chain
    if(s->hash_table[i].key == hash_key)
      {
	fprintf(stderr, "Attempting to insert (%ld : %lu) ontop of (%ld : %lu)\n", token, hash_key, s->hash_table[i].key, hash_key);
	break_point();
      }
  
  i = s->header->first_unused_hash_slot;
  if( i == NULL_INDEX )
    {
      fprintf(stderr, "Ran out of hash slots!\n");
      diagnose_hash_table(s);
      break_point();
    }
  s->header->first_unused_hash_slot = s->hash_table[i].next_in_hash_chain;
  
  //  i is now a free hash slot and j is the slot to precede it
  s->hash_table[j].next_in_hash_chain = i;
  
  s->hash_table[i].token = token;
  s->hash_table[i].key = hash_key;
  s->hash_table[i].next_in_hash_chain = NULL_INDEX;
  
}

//  lookup token by hash key, if hash key not seen before, make a new
//  token for it
static index_t get_token_from_hash(CLUSTEROR_STATE_STRUCT *s, long hash_key)
{
  index_t t = simple_hash_lookup(s, hash_key);
  if(t == NULL_INDEX)
    {
      t = grab_fresh_token(s);
      hash_insert(s, hash_key, t);
      s->tokens[t].hash_code = hash_key;
    }
  return t;
}

static void diagnose_hash_table(CLUSTEROR_STATE_STRUCT *s)
{
  long c = 0, i, j, f = 0, r = 0, longest_chain = 0, l;
  for(i = 0; i < s->header->max_tokens; i++)
    if(s->hash_table[i].token != NULL_INDEX)
      {
	r++;
	l = 0;
	for(j = i; j != NULL_INDEX; j = s->hash_table[j].next_in_hash_chain)
	  {
	    c++;
	    l++;
	  }
	if(l > longest_chain)
	  longest_chain = l;
      }	else
      c++; //to account for usused root node
  for(i = s->header->first_unused_hash_slot; i != NULL_INDEX; i = s->hash_table[i].next_in_hash_chain)
    if(i < s->header->max_tokens / 2)
      fprintf(stderr, "poop!\n");
    else
      {
	c++;
	f++;
      }
  
  fprintf(stderr, 
	  "%ld hash slot accounted for, %ld in free list, %ld root nodes in use, s->header->first_unused_hash_slot = %ld, longest_chain = %ld\n", 
	  c, f, r, s->header->first_unused_hash_slot, longest_chain);
}

//  this learns tokens, but doesn't touch the correlated token info,
//  we need to update the counts of tokens somewhere ... here? no,
//  let's do it in add_occ
static void learning_tokenize (	CLUSTEROR_STATE_STRUCT *s,
				index_t *t, int *n, int max,
				char *text, long len,
				regex_t *regee	)
{
  int l, i;
  regmatch_t match;
  for(*n = 0; *n < max; (*n)++)
    {
      //remember to inc/dec text and len
      if(crm_regexec ( regee, text, len, 1, &match, 0, NULL))
	return;
      l = match.rm_eo - match.rm_so;
      if(0 && joe_trace)
	{
	  fprintf(stderr, "about to look up:");
	  for(i = match.rm_so; i < match.rm_eo; i++)
	    fprintf(stderr, "%c", text[i]);
	  fprintf(stderr, ", the hash of which is %lu", strnhash(&text[match.rm_so], l));
	  fprintf(stderr, ", the mod %ld of which is %lu\n", s->header->max_tokens, strnhash(&text[match.rm_so], l) % s->header->max_tokens);
	}
      
      *t++ = get_token_from_hash(s, strnhash(&text[match.rm_so], l));
      
      if(0 && joe_trace && *(t - 1) != simple_hash_lookup(s, strnhash(&text[match.rm_so], l)))
	{
	  fprintf(stderr, 
 	   "Failed to relookup token! hash: %lu token: %ld gotback: %ld\n", 
		  strnhash(&text[match.rm_so], l), 
		  *(t - 1), 
		  simple_hash_lookup(s, strnhash(&text[match.rm_so], l)));
	  break_point();
	} 
      if(0 && joe_trace)
	{
	  fprintf(stderr, "tokenized:");
	  for(i = match.rm_so; i < match.rm_eo; i++)
	    fprintf(stderr, "%c", text[i]);
	  fprintf(stderr, 
		  ", hash: %ld, token:%ld, stored_hash: %ld, second hashlookup: %ld\n", 
		  strnhash(&text[match.rm_so], l), 
		  *(t - 1), 
		  s->tokens[ *(t - 1)].hash_code , 
		  get_token_from_hash(s, s->tokens[ *(t - 1)].hash_code));
	  print_header(s);
	}
      
      text += match.rm_eo;
      len -= match.rm_eo;
      
    }
  fprintf(stderr, "NNCluster: Not enough space given to tokenizer! Truncated text.\n");
}

static void print_header(CLUSTEROR_STATE_STRUCT *s)
{
  fprintf(stderr, "	s->header->hash_slots_offset = %ld\n", s->header->hash_slots_offset);
  fprintf(stderr, "	s->header->tokens_offset = %ld\n", s->header->tokens_offset);
  fprintf(stderr, "	s->header->cor_tokens_offset = %ld\n", s->header->cor_tokens_offset);
  fprintf(stderr, "	s->header->cooccurences_offset = %ld\n", s->header->cooccurences_offset);
  fprintf(stderr, "	s->header->graph_offset = %ld\n", s->header->graph_offset);
  fprintf(stderr, "	s->header->clusters_offset = %ld\n", s->header->clusters_offset);
  fprintf(stderr, "	s->clusters[0].next_free = %ld\n", s->clusters[0].next_free); 
  fprintf(stderr, "	s->hash_table[199999].next_in_hash_chain = %ld\n", s->hash_table[199999].next_in_hash_chain);
  fprintf(stderr, "	s->header->n_tokens = %ld\n", s->header->n_tokens);
  fprintf(stderr, "	s->header->n_cor_tokens = %ld\n", s->header->n_cor_tokens);
  fprintf(stderr, "	s->header->n_clusters = %ld\n", s->header->n_clusters);
  fprintf(stderr, "	next_free_cor_token = %ld\n", 	s->header->next_free_cor_token);
  fprintf(stderr, "	first_unused_token_slot = %ld\n", 	s->header->first_unused_token_slot);
  fprintf(stderr, "	first_unused_hash_slot = %ld\n", 	s->header->first_unused_hash_slot);
  fprintf(stderr, "	first_unused_cluster_slot = %ld\n", 	s->header->first_unused_cluster_slot);
  fprintf(stderr, "	last_unused_cluster_slot = %ld\n", 	s->header->last_unused_cluster_slot);
  fprintf(stderr, "	most_recent_token = %ld\n", 	s->header->most_recent_token);
  fprintf(stderr, "	least_recent_token = %ld\n", 	s->header->least_recent_token);
  fprintf(stderr, "	least_frequent_token = %ld\n", 	s->header->least_frequent_token);
  fprintf(stderr, "	least_frequent_cor_token = %ld\n", 	s->header->least_frequent_cor_token);
  fprintf(stderr, "	first_unused_edge = %ld\n", 	s->header->first_unused_edge);
  
  if(s->hash_table[199999].next_in_hash_chain != NULL_INDEX)
    break_point();
}

//  expect s->header and s->header->max_tokens to be filled in and
//  nothing else also s->header better point to mmap or else we'll
//  segfault like mofo
static void make_new_clusteror_state(CLUSTEROR_STATE_STRUCT *s)
{
  long i, max_tokens = s->header->max_tokens, max_cor_tokens = s->header->max_cor_tokens;
  
  s->header->n_tokens = 0;
  s->header->n_clusters = 0;
  s->header->first_unused_token_slot = 0;
  s->header->first_unused_hash_slot = max_tokens;
  s->header->most_recent_token = NULL_INDEX;
  s->header->least_recent_token = NULL_INDEX;
  s->header->first_unused_cluster_slot = 0;
  s->header->last_unused_cluster_slot = MAX_CLUSTERS - 1;
  s->header->normal_factor = 1.0;
  s->header->tot_occ = 0.0;
  s->header->first_unused_edge = 0;
  s->header->n_cor_tokens = 0;
  s->header->next_free_cor_token = 0;
  s->header->least_frequent_token = NULL_INDEX;
  
  s->header->hash_slots_offset = sizeof(NNCLUSTEROR_HEADER_STRUCT);
  s->header->hash_slots_offset += BYTE_ALIGN - s->header->hash_slots_offset % BYTE_ALIGN;
  
  s->header->tokens_offset = s->header->hash_slots_offset + 2 * max_tokens * sizeof(HASH_NODE_STRUCT);
  s->header->tokens_offset += BYTE_ALIGN - s->header->tokens_offset % BYTE_ALIGN;
  
  s->header->cor_tokens_offset = s->header->tokens_offset + max_tokens * sizeof(TOKEN_STRUCT);
  s->header->cor_tokens_offset += BYTE_ALIGN - s->header->cor_tokens_offset % BYTE_ALIGN;
  
  s->header->cooccurences_offset = s->header->cor_tokens_offset + max_cor_tokens * sizeof(COR_TOKEN_STRUCT);
  s->header->cooccurences_offset += BYTE_ALIGN - s->header->cooccurences_offset % BYTE_ALIGN;
  
  s->header->graph_offset = 4 + s->header->cooccurences_offset + max_cor_tokens * (max_cor_tokens + 3) / 2 * sizeof(COOCCURRENCE_SCORE_TYPE);
  s->header->graph_offset += BYTE_ALIGN - s->header->graph_offset % BYTE_ALIGN;
  
  s->header->clusters_offset = s->header->graph_offset + (3 * max_cor_tokens + 2) * sizeof(EDGE_STRUCT);
  s->header->clusters_offset += BYTE_ALIGN - s->header->clusters_offset % BYTE_ALIGN;
  
  s->hash_table = (HASH_NODE_STRUCT *)(((char *)(s->header)) + s->header->hash_slots_offset);
  s->tokens = (TOKEN_STRUCT *)(((char *)(s->header)) + s->header->tokens_offset);
  s->cor_tokens = (COR_TOKEN_STRUCT *)(((char *)(s->header)) + s->header->cor_tokens_offset);
  s->cooccurences = (COOCCURRENCE_SCORE_TYPE *)(((char *)(s->header)) + s->header->cooccurences_offset);
  s->graph = (EDGE_STRUCT *)(((char *)(s->header)) + s->header->graph_offset);
  s->clusters = (CLUSTER_STRUCT *)(((char *)(s->header)) + s->header->clusters_offset);
  
  s->hash_table[2 * max_tokens - 1].next_in_hash_chain = NULL_INDEX;
  i = 2 * max_tokens;
  while(--i > s->header->first_unused_hash_slot)
    {	s->hash_table[i - 1].next_in_hash_chain = i;
      s->hash_table[i].key = i - 1;
      s->hash_table[i].token = NULL_INDEX;
    }
  for(i = 0; i < s->header->first_unused_hash_slot; i++)
    s->hash_table[i].token = NULL_INDEX;
  
  s->tokens[max_tokens - 1].less_recent = NULL_INDEX;
  i = max_tokens;
  while(--i)
    s->tokens[i - 1].less_recent = i;
  
  
  i = (3 * MAX_COR_TOKENS + 2);
  s->graph[i-1].next = NULL_INDEX;
  s->graph[i-1].edge_to = NULL_INDEX;
  while(--i)
    {
      s->graph[i-1].next = i;
      s->graph[i-1].edge_to = NULL_INDEX;
    }
  for(i = 0; i < MAX_CLUSTERS; i++)
    s->clusters[i].next_free = i + 1;
}

static int test_memory_consistancy(unsigned char *m, int n)
{
  int i, j = n / 2;
  for(i = 0; i < n / 2; i++)
    m[i] = m[i + j] = (unsigned char)(rand() % 256);
  for(i = 0; i < n / 2; i++)
    if(m[i] != m[i + j])
      return 0;
  for(i = 0; i < n; i++)
    m[i] = 0;
  return 1;
}

static void map_file_for_learn(CLUSTEROR_STATE_STRUCT *s, char *filename)
{
  long i, file_size;
  struct stat statbuf;
  /* [i_a] it is C code, not C++ */
  FILE *f;
  
  if(stat (filename, &statbuf)) //if the file didn't exist already
    {
      long n_tokens = max_tokens, n_cor_tokens = max_cor_tokens; 
      file_size = sizeof(NNCLUSTEROR_HEADER_STRUCT);
      file_size += BYTE_ALIGN - file_size % BYTE_ALIGN;

      //  account for token hash
      if(joe_trace) fprintf(stderr, "hash offset ought to be %ld bytes\n", file_size);
      file_size += 2 * n_tokens * sizeof(HASH_NODE_STRUCT);
      file_size += BYTE_ALIGN - file_size % BYTE_ALIGN;

      //  accoutn for tokens
      if(joe_trace) fprintf(stderr, "token offset ought to be %ld bytes\n", file_size);
      file_size += n_tokens * sizeof(TOKEN_STRUCT);
      file_size += BYTE_ALIGN - file_size % BYTE_ALIGN;

      //  accoutn for cor_tokens
      if(joe_trace) fprintf(stderr, "corellated token offset ought to be %ld bytes\n", file_size);
      file_size += n_cor_tokens * sizeof(COR_TOKEN_STRUCT);
      file_size += BYTE_ALIGN - file_size % BYTE_ALIGN;

      //  account for cooccurance scores
      if(joe_trace) fprintf(stderr, "cooccurence offset ought to be %ld bytes\n", file_size);
      file_size += n_cor_tokens * (n_cor_tokens + 3) / 2 * sizeof(COOCCURRENCE_SCORE_TYPE);
      file_size += BYTE_ALIGN - file_size % BYTE_ALIGN;

      //  account for graph
      if(joe_trace) fprintf(stderr, "graph offset ought to be %ld bytes\n", file_size);
      file_size += 4 + (3 * n_cor_tokens + 2) * sizeof(EDGE_STRUCT); 
      file_size +=  BYTE_ALIGN - file_size % BYTE_ALIGN;
      //account for clusters
      if(joe_trace) fprintf(stderr, "cluster offset ought to be %ld bytes\n", file_size);
      file_size += MAX_CLUSTERS * sizeof(CLUSTER_STRUCT);
      file_size += BYTE_ALIGN - file_size % BYTE_ALIGN;
      
      if(joe_trace) fprintf(stderr, "new file size is %ld bytes\n", file_size);
      
	  /* [i_a] it is C code, not C++ */
      f = fopen(filename, "wb");
	  if (f != NULL)
	  {
      i = file_size + 1024;
      while(i--)
	fputc('\0', f);
      fclose(f);
	  }
	  else
	  {
		  fatalerror("Could not create map file: ", filename);
	  }
      
      if(joe_trace) fprintf(stderr, "\ndone writing file, about to mmap\n");
      s->header = crm_mmap_file(filename, 0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED,  NULL /*&actual_file_size*/);
      
      if(joe_trace)
	{
	  if(!test_memory_consistancy((unsigned char *)(s->header), file_size))
	    {
	      fprintf(stderr, "Memory map found to be inconsistent!\n");
	      break_point();
	    } else
	    fprintf(stderr, "Memory map found to be consistent\n");
	}
      
      if(s->header == MAP_FAILED)
	{
	  fprintf(stderr, "Couldn't map new file %s! errno = %d\n", filename, errno);
	  break_point();
	}
	  assert(s->header != NULL);
      s->header->max_tokens = n_tokens;
      s->header->max_cor_tokens = n_cor_tokens;
      if(joe_trace) fprintf(stderr,"about to make a new cluster state allowing for %ld tokens\n", s->header->max_tokens);
      make_new_clusteror_state(s);
    } else
    {
      file_size = statbuf.st_size;
      s->header = crm_mmap_file(filename, 0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, NULL /*&actual_file_size*/);
      if(s->header == MAP_FAILED)
	{
	  fprintf(stderr, "Couldn't map existing file %s! errno = %d\n", filename, errno);
	  break_point();
	}
      s->hash_table = (HASH_NODE_STRUCT *)(((char *)(s->header)) + s->header->hash_slots_offset);
      s->tokens = (TOKEN_STRUCT *)(((char *)(s->header)) + s->header->tokens_offset);
      s->cor_tokens = (COR_TOKEN_STRUCT *)(((char *)(s->header)) + s->header->cor_tokens_offset);
      s->cooccurences = (COOCCURRENCE_SCORE_TYPE *)(((char *)(s->header)) + s->header->cooccurences_offset);
      s->graph = (EDGE_STRUCT *)(((char *)(s->header)) + s->header->graph_offset);
      s->clusters = (CLUSTER_STRUCT *)(((char *)(s->header)) + s->header->clusters_offset);
    }
  //THESE LINES ASSUMES 4 BYTE LONGS
  //s->token_nn_changed records when a nearest neihbor changes or for unlearning if we need to check the fact later
  s->token_nn_changed = (long *)tempbuf;
  tempbuf += 4 * ((s->header->n_cor_tokens + 31) / 32);
  //closed list is first used to record when tokens have been seen in a document and is later used as a closed list for graph traversals
  s->closed_list = (long *)inbuf;
  inbuf += 4 * ((s->header->max_tokens + 31) / 32);
  
  i = (s->header->max_cor_tokens + 31) / 32;
  while(--i >= 0)
    s->token_nn_changed[i] = 0;
  i = (s->header->max_tokens + 31) / 32;
  while(--i >= 0)
    s->closed_list[i] = 0;
  
  if(joe_trace)
    print_header(s);
}

static void unmap_file_for_learn(CLUSTEROR_STATE_STRUCT *s)
{
  if(joe_trace)
    {
      fprintf(stderr, "unmapping\n");
      //print_header(s);
    }
  tempbuf -= 4 * ((s->header->n_cor_tokens + 31) / 32);
  inbuf -= 4 * ((s->header->max_tokens + 31) / 32);
  crm_munmap_file ((void *) s->header);
}

//  finds NN for all tokens in token_nn_changed, then leaves
//  token_nn_changed in appropriate state there had better be at least
//  one correlator token by the time we get called, this may get fudged
//  if we unlearn first
static void find_NNs_for_unlearning(CLUSTEROR_STATE_STRUCT *s)
{
  index_t i, j, n;
  COOCCURRENCE_SCORE_TYPE m, p;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    {
      if(get_bit(s->token_nn_changed, i))
	{	
	  n = (i == 0 ? 1 : 0);
	  m = score_cooccurance(s, s->cor_tokens[i].token, 1); 
	  for(j = 1; j < s->header->n_cor_tokens; j++)
	    {
	      p = score_cooccurance(s, s->cor_tokens[i].token, s->cor_tokens[j].token); 
	      if(i != j && p > m)
		{
		  m = p;
		  n = j;
		}
	    }
	  if(s->cor_tokens[i].nearest_neihbor != n)
	    {
	      set_bit(s->token_nn_changed, i, 1);
	      s->old_nearest_neihbors[i] = s->cor_tokens[i].nearest_neihbor;
	      s->cor_tokens[i].nearest_neihbor = n;
	    } else
	    set_bit(s->token_nn_changed, i, 0);
	}
    }
}

//  brings all the tokens we've seen to the top of most recent list
//  and zeros out the closed list
static void update_seen_tokens(CLUSTEROR_STATE_STRUCT *s)
{
  index_t i;
  for(i = 0; i < s->header->n_tokens; i++)
    if(get_bit(s->closed_list, i))
      {
	set_bit(s->closed_list, i, 0);
	if(i == s->header->most_recent_token)
	  continue;
	if(s->tokens[i].more_recent < NULL_INDEX)
	  s->tokens[ s->tokens[i].more_recent ].less_recent = s->tokens[i].less_recent;
	if(s->tokens[i].less_recent < NULL_INDEX)
	  s->tokens[ s->tokens[i].less_recent ].more_recent = s->tokens[i].more_recent;
	if(i == s->header->least_recent_token)
	  s->header->least_recent_token = s->tokens[i].more_recent;
	s->tokens[i].more_recent = NULL_INDEX;
	s->tokens[i].less_recent = s->header->most_recent_token;
	s->header->most_recent_token = i;
	s->tokens[ s->tokens[i].less_recent ].more_recent = i;
      }
}


//  It is a document scorers responsibility to update occurrences
//  first and then cooccurences by calling add_N, add_occ and add_cooc
//  in that order!  We've gone to great pains to make sure that scorers
//  only need to use token indeces and not cor_indeces
#define WINDOW_SIZE 100
static void score_document_mix(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense)
{
  if(fabs(sharp_weight) > 0.001)
    score_document_sharp(s, doc, len, sense * sharp_weight);
  if(fabs(gauss_weight) > 0.001)
    score_document_gauss(s, doc, len, sense * gauss_weight);
  if(fabs(flat_weight) > 0.001)
    score_document_flat(s, doc, len, sense * flat_weight);
}

static void score_document_sharp(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense)
{
  index_t i, j;
  add_N(s, (COOCCURRENCE_SCORE_TYPE)len * sense);
  for(i = 0; i < len; i++)
    add_occ(s, doc[i], sense);
  for(i = 0; i < len; i++)
    for(j = i + 1; j < i + WINDOW_SIZE && j < len; j++)
      add_cooc(s, doc[i], doc[j], sense / (COOCCURRENCE_SCORE_TYPE)(j - i));
} 

static void score_document_gauss(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense)
{
  index_t i, j;
  add_N(s, (COOCCURRENCE_SCORE_TYPE)len * sense);
  for(i = 0; i < len; i++)
    add_occ(s, doc[i], sense);
  for(i = 0; i < len; i++)
    for(j = i + 1; j < i + WINDOW_SIZE && j < len; j++)
      add_cooc(s, doc[i], doc[j], sense * normalized_gauss((COOCCURRENCE_SCORE_TYPE)(j - i), gauss_sigma));
} 

//This one only worries about presense in whole document
static void score_document_flat(CLUSTEROR_STATE_STRUCT *s, index_t *doc, long len, COOCCURRENCE_SCORE_TYPE sense)
{
  index_t i, j;
  long seen_len = 1 + s->header->n_tokens / (sizeof(long) * 8);
  long *seen = (long *)tempbuf;
  index_t *seen_set = (index_t *)inbuf;
  long seen_set_len = 0;
  for(i = 0; i < seen_len; i++)
    seen[i] = 0;
  for(i = 0; i < len; i++)
    if(!get_bit(seen, doc[i]))
      {
	set_bit(seen, doc[i], 1);
	seen_set[seen_set_len++] = doc[i];
      }
  if(joe_trace) fprintf(stderr, "saw %ld unique tokens\n", seen_set_len);
  add_N(s, sense);
  for(i = 0; i < seen_set_len; i++)
    add_occ(s, seen_set[i], sense);
  for(i = 0; i < seen_set_len; i++)
    for(j = i + 1; j < seen_set_len; j++)
      add_cooc(s, seen_set[i], seen_set[j], sense);
}

#ifdef NOT_USED
static int check_if_connected(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
  /* [i_a] it is C code, not C++ */
  index_t *stack = (index_t *)outbuf, *stack_ptr = stack;
  index_t a, b;

  if(joe_trace) fputc('c',stderr);
  
  *stack_ptr++ = i;
  while(stack_ptr > stack)
    {
      a = *--stack_ptr;
      if(!get_bit(s->closed_list, a))
	{
	  set_bit(s->closed_list, a, 1);
	  if(a == j)
	    break;
	  for(b = s->cor_tokens[a].edges; b != NULL_INDEX; b = s->graph[b].next)
	    if(!get_bit(s->closed_list, s->graph[b].edge_to))
	      *stack_ptr++ = s->graph[b].edge_to;
	}
    }
  b = (s->header->max_cor_tokens + 31) / 32;
  for(a = 0; a < b; a++)
    s->closed_list[a] = 0;
  return stack_ptr > stack;
}
#endif


static void change_cluster(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t c)
{
  if(c >= MAX_CLUSTERS)
    fprintf(stderr, "asked to make a wonkee cluster assignment! i = %ld, c = %ld\n", i, c);
  
  if(s->cor_tokens[i].cluster != NULL_INDEX)
    s->clusters[ s->cor_tokens[i].cluster ].occurrences -= get_occ(s, s->cor_tokens[i].token);
  s->clusters[ c ].occurrences += get_occ(s, s->cor_tokens[i].token); 
  s->cor_tokens[i].cluster = c;
}


//  GROT GROT GROT GROT
//   These are Globals... why?
int given = 0, goten = 0;

static long cluster_audit(CLUSTEROR_STATE_STRUCT *s)
{
  /* [i_a] it is C code, not C++ */
  long *cluster_closed = (long *)inbuf;
  index_t a, b;

  fprintf(stderr, "Auditing clusters ...\n");

  b = (MAX_CLUSTERS + 31) / 32;
  for(a = 0; a < b; a++)
    cluster_closed[a] = 0;
  b = 0;
  for(a = 0; a < s->header->n_cor_tokens; a++)
    if( s->cor_tokens[a].cluster != s->cor_tokens[ s->cor_tokens[a].nearest_neihbor ].cluster	)
      b++;
  fprintf(stderr, "Found %ld tokens which were not in the same cluser as their nearest neighbors.\n", b);
  
  b = 0;
  for(a = 0; a < s->header->n_cor_tokens; a++)
    if(	s->cor_tokens[a].cluster == NULL_INDEX )
      b++;
  fprintf(stderr, "Found %ld tokens without cluster assignments.\n", b);
  
  for(a = 0; a < s->header->n_cor_tokens; a++)
    if(	s->cor_tokens[a].cluster != NULL_INDEX )
      set_bit(cluster_closed, s->cor_tokens[a].cluster, 1);
  b = 0;
  for(a = 0; a < MAX_CLUSTERS; a++)
    if(get_bit(cluster_closed, a))
      b++;
  fprintf(stderr, "Found %ld unique cluster assignments.\n", b);
  return b;
}

static void give_back_cluster(CLUSTEROR_STATE_STRUCT *s, index_t c)
{
  if(joe_trace) fputc('b',stderr);
  //if(joe_trace) fprintf(stderr, "\ngiving back cluster %ld\n", c);
  if(c >= MAX_CLUSTERS)
    {
      fprintf(stderr, "gave back a wonkee cluster! %ld\n", c);
      break_point();
    }
  if(c != NULL_INDEX) //in case we give back the null cluster for fum
    {
      if(joe_trace)
	{
	  long i, j;
	  for(i = s->header->first_unused_cluster_slot, j = 0; 
	      i != MAX_CLUSTERS 
		&& i != NULL_INDEX 
		&& j < 2 * MAX_CLUSTERS; 
	      i = s->clusters[i].next_free, j++)
	    if(i == c)
	      {
		fprintf(stderr, "giving back cluster %ld twice, arg!!!\n", c);
		break_point();
	      }
	  if(i == NULL_INDEX)
	    {
	      fprintf(stderr, "There's a cluster in use AND in the free cluster list, arg!!!\n");
	      break_point();
	    }
	  if(j == MAX_CLUSTERS)
	    {
	      fprintf(stderr, "There's a loop in the free cluster list, arg!!!\n");
				break_point();
	    }
	}
      s->clusters[c].next_free = MAX_CLUSTERS;
      
      if( s->header->last_unused_cluster_slot != MAX_CLUSTERS )
	s->clusters[ s->header->last_unused_cluster_slot ].next_free = c;
      if( s->header->first_unused_cluster_slot == MAX_CLUSTERS )
	s->header->first_unused_cluster_slot  = c;
      s->header->last_unused_cluster_slot = c;
      s->header->n_clusters--;
      goten++;
    }
}

static index_t get_fresh_cluster(CLUSTEROR_STATE_STRUCT *s)
{
  /* [i_a] it is C code, not C++ */
  index_t c = s->header->first_unused_cluster_slot;

  if(joe_trace) fputc('g',stderr);

  if(c == MAX_CLUSTERS)
    {
      fprintf(stderr, "\nWe've run out of cluster slots. This is thoroughly impossible.\n We've given out a total of %d and goten back %d\n", given, goten);
      fprintf(stderr, "s->header->n_clusters = %ld\n", s->header->n_clusters);
      cluster_audit(s);
      break_point();
    }
  
  if(c == s->header->last_unused_cluster_slot)
    s->header->last_unused_cluster_slot = MAX_CLUSTERS;
  
  s->header->first_unused_cluster_slot = s->clusters[c].next_free;
  s->header->n_clusters++;
  s->clusters[c].next_free = NULL_INDEX; //to mark it as in use
  s->clusters[c].occurrences = 0.0;
  given++;
  return c;
}

//  i and j are recently connected tokens in two seperate clusters to
//  be joined because we're changing cluster numbers we already have
//  an implicate closed list yay!
static void join_clusters(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
  if(joe_trace) fputc('j',stderr);
  
  if(i < 0 || i >= s->header->n_cor_tokens || j < 0 || j > s->header->n_cor_tokens)
    fprintf(stderr, "\nmade to join wonkee token numbers!\n i = %ld, j = %ld\n", i, j); 
  if(s->cor_tokens[i].cluster < 0 
     || (s->cor_tokens[i].cluster >= MAX_CLUSTERS 
	 && s->cor_tokens[i].cluster != NULL_INDEX) 
     || s->cor_tokens[j].cluster < 0 
     || (s->cor_tokens[j].cluster >= MAX_CLUSTERS 
	 && s->cor_tokens[j].cluster != NULL_INDEX))
    fprintf(stderr, "\nmade to join wonkee cluster numbers!\n cluster(i) = %ld, cluster(j) = %ld\n", s->cor_tokens[i].cluster, s->cor_tokens[j].cluster); 
  if( 
     (s->cor_tokens[i].cluster != NULL_INDEX 
      && s->clusters[ s->cor_tokens[i].cluster ].next_free != NULL_INDEX) 
     ||	(s->cor_tokens[j].cluster != NULL_INDEX 
	 && s->clusters[ s->cor_tokens[j].cluster ].next_free != NULL_INDEX))
    {
      index_t q; /* [i_a] */
      fprintf(stderr, "\nmade to join a cluster in the free list!\n cluster(%ld) = %ld, cluster(%ld) = %ld, next(%ld) = %ld, next(%ld) = %ld\n", i, s->cor_tokens[i].cluster, j, s->cor_tokens[j].cluster, s->cor_tokens[i].cluster, s->clusters[ s->cor_tokens[i].cluster ].next_free, s->cor_tokens[j].cluster, s->clusters[ s->cor_tokens[j].cluster ].next_free);
      fprintf(stderr, "adjacent to %ld:\n", i);
      for(q = s->cor_tokens[i].edges; q != NULL_INDEX; q = s->graph[q].next)
	fprintf(stderr, "\tcluster(%ld) = %ld\n", s->graph[q].edge_to, s->cor_tokens[ s->graph[q].edge_to ].cluster);
      fprintf(stderr, "adjacent to %ld:\n", j);
      for(q = s->cor_tokens[j].edges; 
	  q != NULL_INDEX; 
	  q = s->graph[q].next)
	fprintf(stderr, "\tcluster(%ld) = %ld\n", s->graph[q].edge_to, s->cor_tokens[ s->graph[q].edge_to ].cluster);
      break_point();
    }
  if(i == j)
    {
      fprintf(stderr, "Made to join identical tokens!\n nn(%ld) = %ld\n", i, s->cor_tokens[i].nearest_neihbor);
      break_point();
    }
  
  if(0 && joe_trace)
    {	fprintf(stderr, "\nWe're going to join nodes/tokens %ld and %ld, they have respective cluster numbers %ld and %ld", i, j, s->cor_tokens[i].cluster, s->cor_tokens[j].cluster);
      if(s->cor_tokens[i].cluster < NULL_INDEX)
	fprintf(stderr, ", cluster %ld's occurence is %f", s->cor_tokens[i].cluster, s->clusters[ s->cor_tokens[i].cluster ].occurrences);
      if(s->cor_tokens[j].cluster < NULL_INDEX)
	fprintf(stderr, ", cluster %ld's occurence is %f", s->cor_tokens[j].cluster, s->clusters[ s->cor_tokens[j].cluster ].occurrences);
      fprintf(stderr, "\n");
    }
  
  
  if(s->cor_tokens[i].cluster == s->cor_tokens[j].cluster && s->cor_tokens[i].cluster != NULL_INDEX)
    {
      if( joe_trace ) fputc('I', stderr);
      return;  //this can happen if i and j were already joined transitively
    }

  {
	    /* [i_a] it is C code, not C++ */
  index_t a, b, c, old_cluster;
  index_t *stack = (index_t *)tempbuf, *stack_ptr = stack;
  
  //   if both are without clusters, as will happen with two new tokens  
  if(s->cor_tokens[i].cluster == NULL_INDEX 
     && s->cor_tokens[j].cluster == NULL_INDEX) 
    {
      c = get_fresh_cluster(s);
      old_cluster = NULL_INDEX;
      *stack_ptr++ = i;
      while(stack_ptr > stack)
	{
	  a = *--stack_ptr;
	  if(s->cor_tokens[a].cluster == c)
	    continue;
	  change_cluster(s, a, c);
	  for(b = s->cor_tokens[a].edges; 
	      b != NULL_INDEX; 
	      b = s->graph[b].next)
	    if(s->cor_tokens[ s->graph[b].edge_to ].cluster == old_cluster)
	      *stack_ptr++ = s->graph[b].edge_to;
	}
      if( joe_trace ) fputc('N', stderr);
      //  because and i and j are connected that previous traversal
      //  should have painted both groups
      return;
    } else if( s->cor_tokens[i].cluster == NULL_INDEX )
    {
      index_t t = i; i = j; j = t;
    } else if(	s->cor_tokens[j].cluster != NULL_INDEX
		&&	s->clusters[ s->cor_tokens[i].cluster ].occurrences 
		< s->clusters[ s->cor_tokens[j].cluster ].occurrences	)
    {index_t t = i; i = j; j = t;}
  c = s->cor_tokens[i].cluster;
  old_cluster = s->cor_tokens[j].cluster;
  *stack_ptr++ = j;
  while(stack_ptr > stack)
    {
      a = *--stack_ptr;
      if(s->cor_tokens[a].cluster == c)
	continue;
      change_cluster(s, a, c);
      for(b = s->cor_tokens[a].edges; 
	  b != NULL_INDEX; 
	  b = s->graph[b].next)
	if(s->cor_tokens[ s->graph[b].edge_to ].cluster == old_cluster)
	  *stack_ptr++ = s->graph[b].edge_to;
    }
  if(joe_trace && old_cluster == NULL_INDEX)
    fputc('Q', stderr);
  
  if(old_cluster != NULL_INDEX)
    give_back_cluster(s, old_cluster);
  } /* [i_a] */
}



//  i and j are recently disconnected tokens of the same cluster to be split
//  this guy tests for connectivity while begining cut
static void speculative_split_clusters(CLUSTEROR_STATE_STRUCT *s, index_t i, index_t j)
{
  /* [i_a] it is C code, not C++ */
  index_t c;
  index_t a, b;
  index_t *stack = (index_t *)tempbuf, *stack_ptr = stack;
  index_t *groupI = (index_t *)inbuf, n_groupI = 0;
  index_t *groupJ = (index_t *)outbuf, n_groupJ = 0;
  COOCCURRENCE_SCORE_TYPE occI = 0.0, occJ = 0.0;

  if(joe_trace) fputc('S',stderr);

  *stack_ptr++ = i;
  while(stack_ptr > stack)
    {
      a = *--stack_ptr;
      if(a == j) //they were connected!
	{	//zero out closed list and scam!
	  b = (s->header->max_tokens + 31) / 32;
	  for(a = 0; a < b; a++)
	    s->closed_list[a] = 0;
	  return;
	}
      if(get_bit(s->closed_list, a))
	continue;
      set_bit(s->closed_list, a, 1);
      groupI[n_groupI++] = a;
      occI += get_occ(s, a);
      for(b = s->cor_tokens[a].edges; b != NULL_INDEX; b = s->graph[b].next)
	if(!get_bit(s->closed_list, s->graph[b].edge_to))
	  *stack_ptr++ = s->graph[b].edge_to;
    }
  /*a = (s->max_tokens + 31) / 32;
    while(--a >= 0)
    s->closed_list[a] = 0;
  */ //we don't need to zero it out as these regions SHOULD BE unconnected
  *stack_ptr++ = j;
  while(stack_ptr > stack)
    {
      a = *--stack_ptr;
      if(get_bit(s->closed_list, a))
	continue;
      set_bit(s->closed_list, a, 1);
      groupJ[n_groupJ++] = a;
      occJ += get_occ(s, a);
      for(b = s->cor_tokens[a].edges; b != NULL_INDEX; b = s->graph[b].next)
	if(!get_bit(s->closed_list, s->graph[b].edge_to))
	  *stack_ptr++ = s->graph[b].edge_to;
    }
  b = (s->header->max_tokens + 31) / 32;
  for(a = 0; a < b; a++)
    s->closed_list[a] = 0;
  c = get_fresh_cluster(s);
  if(occI < occJ)
    for(a = 0; a < n_groupI; a++)
      change_cluster(s, groupI[a], c);
  else
    for(a = 0; a < n_groupJ; a++)
      change_cluster(s, groupJ[a], c);
}

static void update_graph_and_clusters(CLUSTEROR_STATE_STRUCT *s)
{
  index_t  n_cuts = 0;
  index_t n_joins = 0;
  index_t i, j;
  tempbuf += MAX_TOKENS * sizeof(index_t);
  inbuf += MAX_TOKENS * sizeof(index_t);
  
  if(joe_trace)
    {
      fprintf(stderr, "s->header->n_clusters = %ld\n", s->header->n_clusters);
      if(cluster_audit(s) != s->header->n_clusters)
	{
	  fprintf(stderr, "cluster_audit does not agree!\n");
	  fprintf(stderr, "We've given out a total of %d and goten back %d\n", given, goten);
	  break_point();
	}
    }
  // we shouldn't really need to do this but how long could it really take?
  j = (s->header->max_tokens + 31) / 32;
  for(i = 0; i < j; i++)
    s->closed_list[i] = 0;
  
  for(i = 0; i < s->header->n_cor_tokens; i++)
    if(get_bit(s->token_nn_changed, i))
      {
	j = s->old_nearest_neihbors[i];
	//  we test to see if the new NN was chenged back to the old
	//  NN, we don't cut in this case.
	//  get_edge(s, i, j) == 0 if we already cut it for j
	if( j != NULL_INDEX 
	    && j != s->cor_tokens[i].nearest_neihbor 
	    && get_edge(s, i, j) 
	    && s->cor_tokens[j].nearest_neihbor != i	) 
	  {
	    set_edge(s, i, j, 0);
	    //if(!check_if_connected(s, i, j))
	    //	split_clusters(s, i, j);
	    speculative_split_clusters(s, i, j);
	    n_cuts++;
	  }
      }
  for(i = 0; i < s->header->n_cor_tokens; i++)
    if(get_bit(s->token_nn_changed, i))
      {
	j = s->cor_tokens[i].nearest_neihbor;
	if( j != NULL_INDEX && !get_edge(s, i, j)) //j should never be = NULL_INDEX
	  {
	    set_edge(s, i, j, 1);
	    join_clusters(s, i, j);
	    n_joins++;
	  }
      }
  
  if(joe_trace)
    {
      fprintf(stderr, "s->header->n_clusters = %ld\ndid %ld cuts, and %ld joins\n", s->header->n_clusters, n_cuts, n_joins);
      if(cluster_audit(s) != s->header->n_clusters)
	{
	  fprintf(stderr, "cluster_audit does not agree!\n");
	  fprintf(stderr, "We've given out a total of %d and goten back %d\n", given, goten);
	  break_point();
	}
    }
  
  tempbuf -= MAX_TOKENS * sizeof(index_t);
  inbuf -= MAX_TOKENS * sizeof(index_t);
}

int verify_graph(CLUSTEROR_STATE_STRUCT *s)
{
  long i, j, c = 0, d = 0;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    for(j = i + 1; j < s->header->n_cor_tokens; j++, d++)
      if(get_edge(s, i, j))
	c++;
  if(joe_trace)
    fprintf(stderr, "the graph has %ld edges, there are %ld possible. There are %ld correlated tokens\n", c, d, s->header->n_cor_tokens);
  return c <= 2 * s->header->n_tokens;
}

int test_bit_vectors()
{
  long b, i;
  for(i = 0; i < 32; i++)
    set_bit(&b, i, 1);
  for(i = 0; i < 30; i++)
    set_bit(&b, i, 0);
  for(i = 30; i < 32; i++)
    if(!get_bit(&b, i))
      return 0;
  for(i = 0; i < 30; i++)
    if(get_bit(&b, i))
      return 0;
  set_bit(&b, 3, 1);
  set_bit(&b, 3, 0);
  if(get_bit(&b, 3))
    return 0;
  
  return 1;
}		

//  This guy escapes every charector in null terminated string *in
//  which occurs in the null terminated string *escape_these and
//  returns how many charectors it copied to out

static long copy_and_escape(char *out, const char *in, const char *escape_these)
{
  const char *i = in;
  char *o = out;
  if(joe_trace)
    fprintf(stderr, "escaping string: %s\n", in);
  
  while(*i)
    {
      const char *m;
      for(m = escape_these; *m != '\0'; m++)
	if(*i == *m)
	  break;
      if(*m) //is not the null charecter, zero
	*o++ = '\\';
      *o++ = *i++;
    }
  *o++ = '\0';
  
  if(joe_trace)
    fprintf(stderr, "produced string: %s\n", out);
  
  return o - out;
}

//  This guy takes a string of the form "(key1=value1,key2,value2
//  ...)" and grabs the values which had better be sensical numbers
//  keys are given as a null terminated array of pointers to null
//  terminated strings, no key may be a suffix of another or behavior
//  is undefined anything besides digits and key names can be used to
//  delimit values are placed in an array of doubles called values,
//  default values should already be in it

static void parse_monster(char *text, long len, const char **keys, double *values)
{
  regex_t regee;
  char regee_text[MAX_PATTERN];
  long regee_text_len;
  char bufee[MAX_PATTERN], *b;
  int i;
  regmatch_t match[2];
  while(*keys)
    {
      if(joe_trace)
	fprintf(stderr, "parsing for key: %s\n", *keys);
      regee_text_len = copy_and_escape(regee_text, *keys, ".()[]{}^*+-?");
      regee_text_len--; //to eat null charactor
      regee_text_len = strmov(regee_text + regee_text_len, "[[:space:]]*=[[:space:]]*(-?[0-9.]+)") - regee_text;
      if(joe_trace)
	{
	  fprintf(stderr, "compiling regex: %.*s\n", (int)regee_text_len, regee_text);
	}
      if(crm_regcomp (&regee, regee_text, regee_text_len, REG_EXTENDED))
	{	
	  if(joe_trace)	fprintf(stderr, "some jerk gave us a wonkey key to parse in parse_monster! : %s\n", *keys);
	} else
	{
	  if(!crm_regexec ( &regee, text, len, 2, match, 0, NULL))
	    {
	      for(b = bufee, i = match[1].rm_so; i < match[1].rm_eo; b++, i++)
		  {
			assert(i < MAX_PATTERN);
			*b = text[i];
		  }
	      assert(b - bufee < MAX_PATTERN);
	      *b = '\0';
	      *values = atof(bufee);
	    }
	}
      keys++;
      values++;
    }
}

//  write now the only clusterer is this one, connect every token to
//  it's nearest neihbor by the score
int crm_expr_clump_nn (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  char filename[MAX_PATTERN];
  long filename_len;
  
  char regex_text[MAX_PATTERN];
  long regex_text_len;
  regex_t regee;
  COOCCURRENCE_SCORE_TYPE sense = 1.0;
  
  CLUSTEROR_STATE_STRUCT s;
  
  char box_text[MAX_PATTERN];
  char errstr [MAX_PATTERN];
  char *text;
  long text_start, text_len;
  
  //  these happenin second slashed string in /key=value key=value ... / style 
  char parameters_text[MAX_PATTERN];
  long parameters_text_len;
  
  index_t *tokenized_text;
  int tokenized_text_len;
  
  index_t i;
  
  /* [i_a] it is C code, not C++ */
  const char *keys[] = {"max_tokens", "max_cor_tokens", "flat_weight", "sharp_weight", "gauss_weight", "gauss_sigma", (char *)0};
  /* values[] will be edited by parse_monster(): */
  double values[] = {MAX_TOKENS, MAX_COR_TOKENS, 0.0, 1.0, 0.0, 1.0};

  //joe_trace = internal_trace
  
  joe_trace = 0;
  if(internal_trace)
    joe_trace = 1;
  
  if(joe_trace)
    fprintf(stderr, "entered crm_expr_clump (learn)\n");
  if(joe_trace && !test_bit_vectors())
    fprintf(stderr, "bit vectors don't work!");
  
  
  //  parse out .css file name and flags and token regex
  crm_get_pgm_arg (filename, MAX_PATTERN, apb->p1start, apb->p1len);
  filename_len = apb->p1len;
  filename_len = crm_nexpandvar (filename, filename_len, MAX_PATTERN);

  go_flat = 0;
  if(apb->sflags & CRM_FLAT)
    go_flat = 1;
  
  if (apb->sflags & CRM_REFUTE)
    sense = -sense;
  
  /* [i_a] it is C code, not C++ */
  /*
  char *keys[] = {"max_tokens", "max_cor_tokens", "flat_weight", "sharp_weight", "gauss_weight", "gauss_sigma", (char *)0};
  double values[] = {MAX_TOKENS, MAX_COR_TOKENS, 0.0, 1.0, 0.0, 1.0};
  */

  crm_get_pgm_arg (parameters_text, MAX_PATTERN, apb->s2start, apb->s2len);
  parameters_text_len = apb->s2len;
  parameters_text_len = crm_nexpandvar (parameters_text, parameters_text_len, MAX_PATTERN);
  parse_monster(parameters_text, parameters_text_len, keys, values);
  
  max_tokens = (long)values[0];
  max_cor_tokens = (long)values[1]; 
  flat_weight = values[2];
  sharp_weight = values[3];
  gauss_weight = values[4];
  gauss_sigma = values[5];
  
  if(joe_trace)
    {
      const char **ki = keys;
      const double *vi = values;
      fprintf(stderr, "parameters:\n"); 
      while(*ki)
	fprintf(stderr, "\t%s = %0.3f\n", *ki++, *vi++);
    }
  
  crm_get_pgm_arg (regex_text, MAX_PATTERN, apb->s1start, apb->s1len);
  regex_text_len = apb->s1len;
  if(regex_text_len == 0)
    {
      strcpy(regex_text, "[[:graph:]]+"); 
      regex_text_len = strlen( regex_text );
    }
  regex_text[regex_text_len] = '\0'; //only nessicary for debug print later
  regex_text_len = crm_nexpandvar (regex_text, regex_text_len, MAX_PATTERN);
  //  THIS IS WHERE REGEX FLAGS GO FOR THINGS LIKE CASE INSENSITIVITY
  if(joe_trace) fprintf(stderr, "about to compile token regex: \"%s\"\n", regex_text);
  if( crm_regcomp (&regee, regex_text, regex_text_len, REG_EXTENDED) )
    {
      fprintf(stderr, "Problem compiling this regex %s\n", regex_text);
      break_point();
    }
  
  crm_get_pgm_arg (box_text, MAX_PATTERN, apb->b1start, apb->b1len);
  if(0 > crm_restrictvar(box_text, apb->b1len, NULL, &text, &text_start, &text_len, errstr) )
    {
      fprintf(stderr, "Error grabbing text! %s\n", errstr);
      break_point();
    }
  
  if(joe_trace) fprintf(stderr, "about to call map_file_for_learn on %s\n", filename);
  
  map_file_for_learn(&s, filename);
  
  if(joe_trace)
    diagnose_hash_table(&s);
  
  if(0)//apb->sflags & CRM_PEEK) //just peeking
    poke_around( &s);
  else
    {
      tokenized_text = (index_t *)outbuf;
      outbuf += text_len + 2;
      
      if(joe_trace) fprintf(stderr, "about to tokenize %ld characters with regex %s, there are %ld unique tokens in hash already\n", text_len, regex_text, s.header->n_tokens);
      
      learning_tokenize(&s, tokenized_text, &tokenized_text_len, text_len / 2 + 1, text + text_start, text_len, &regee);
      
      if(joe_trace) fprintf(stderr, "the document was %d tokens long, there are now %ld tokens in hash\n", tokenized_text_len, s.header->n_tokens);
      
      s.old_nearest_neihbors = (index_t *)outbuf; 
      outbuf += (s.header->n_tokens * sizeof(index_t));
      for(i = 0; i < s.header->n_tokens; i++)
	s.old_nearest_neihbors[i] = NULL_INDEX; 
      
      if(joe_trace) fprintf(stderr, "about to score with %s\n", go_flat ? "score_document_flat" : "score_document_fuzzy");
      if(go_flat)
	score_document_flat(&s, tokenized_text, tokenized_text_len, sense);
      else
	score_document_mix(&s, tokenized_text, tokenized_text_len, sense);
      
      if(sense < 0)
	find_NNs_for_unlearning(&s);
      
      update_seen_tokens(&s);
      
      if(joe_trace) fprintf(stderr, "activating graph monster!\n");
      update_graph_and_clusters(&s);
      
      if(joe_trace && !verify_graph(&s))
	fprintf(stderr, "wonkee graph!\n");
      
      outbuf -= text_len;
      outbuf -= (s.header->n_tokens * sizeof(index_t));
      
    }	
  unmap_file_for_learn(&s);
  
#ifdef POSIX
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .css ile, to force
  //    the filesystem to repropagate it's caches.
  //
  {
    int hfd;                  //  hashfile fd
    char foo;

    if(joe_trace) fprintf(stderr, "posix blah!\n");

    hfd = open (filename, O_RDWR | O_BINARY); /* [i_a] on MSwin/DOS, open() opens in CRLF text mode by default; this will corrupt those binary values! */
    if(hfd == -1) fprintf(stderr, "Couldn't reopen %s to touch\n", filename);
    read (hfd, &foo, sizeof(foo));
    lseek (hfd, 0, SEEK_SET);
    write (hfd, &foo, sizeof(foo));
    close (hfd);
  }
#endif
  
  
  return 0;
}

static void nolearning_tokenize
(	CLUSTEROR_STATE_STRUCT *s,
	index_t *t, int *n, int max,
	char *text, long len,
	regex_t *regee	)
{
  long l, i, m;
  regmatch_t match;
  for(*n = 0; *n < max; (*n)++)
    {
      //remember to inc/dec text and len
      if(0 && joe_trace) fprintf(stderr, "calling crm_regexec...");
      m = crm_regexec ( regee, text, len, 1, &match, 0, NULL);
      if(m == REG_NOMATCH)
	return;
      if(m != REG_OK)
	{
	  fprintf(stderr, "problem number %ld with regex match in non-learning tokenizer.\n", m);
	  return;
	}
      l = match.rm_eo - match.rm_so;
      if(0 && joe_trace)
	{
	  fprintf(stderr, "matched token: ");
	  for(i = match.rm_so; i < match.rm_eo; i++)
	    fprintf(stderr, "%c", text[i]);
	  fprintf(stderr, ". The hashcode of which is %ld\n", strnhash(&text[match.rm_so], l));
	}
      
      if( 0 && joe_trace) fprintf(stderr, "calling no_learning_get_token_from_hash...\n");
      
      *t++ = simple_hash_lookup(s, strnhash(&text[match.rm_so], l));
      
      if(0 && joe_trace)
	if(joe_trace) fprintf(stderr, "no_learning_get_token_from_hash returned %ld\n", *(t-1));
      
      
      text += match.rm_eo;
      len -= match.rm_eo;
    }
  fprintf(stderr, "NNCluster: Not enough space given to tokenizer! Truncated text.\n");
}

int crm_expr_pmulc_nn (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  char filename[MAX_PATTERN];
  long filename_len;
  
  char regex_text[MAX_PATTERN];
  long regex_text_len;
  regex_t regee;
  
  char box_text[MAX_PATTERN];
  char errstr [MAX_PATTERN];
  char *text;
  long text_start, text_len;
  
  index_t *tokenized_text;
  int tokenized_text_len;
  
  int i;
  long out_pos;
  
  char out_var[MAX_PATTERN];
  long out_var_len;
  
  struct stat statbuf;
  
  index_t t;  /* [i_a] it is C code, not C++ */

  /* [i_a] it is C code, not C++ */
  CLUSTEROR_STATE_STRUCT s;
  
  joe_trace = 0;
  if(internal_trace)
    joe_trace = 1;
  
  //  parse out cluster file name and flags and token regex
  crm_get_pgm_arg (filename, MAX_PATTERN, apb->p1start, apb->p1len);
  filename_len = apb->p1len;
  filename_len = crm_nexpandvar (filename, filename_len, MAX_PATTERN);
  
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
  regex_text_len = crm_nexpandvar (regex_text, regex_text_len, MAX_PATTERN);
  //  THIS IS WHERE REGEX FLAGS GO FOR THINGS LIKE CASE INSENSITIVITY
  if( crm_regcomp (&regee, regex_text, regex_text_len, REG_EXTENDED) )
    {
      fprintf(stderr, "Problem compiling this regex %s\n", regex_text);
      break_point();
    }
  
  crm_get_pgm_arg (box_text, MAX_PATTERN, apb->b1start, apb->b1len);
  if(0 > crm_restrictvar(box_text, apb->b1len, NULL, &text, &text_start, &text_len, errstr) )
    {
      fprintf(stderr, "Error grabbing text! %s\n", errstr);
      break_point();
    }
  
  /* [i_a] it is C code, not C++ */
  /* CLUSTEROR_STATE_STRUCT s; */
  
  if(stat (filename, &statbuf))
    {
      fprintf(stderr, "Unable to open cluster file %s\n", filename);
      return 0;
    }
  
  s.header = crm_mmap_file(filename, 0, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, NULL);
  
  s.hash_table = (HASH_NODE_STRUCT *)((char *)(s.header) + s.header->hash_slots_offset);
  s.tokens = (TOKEN_STRUCT *)((char *)(s.header) + s.header->tokens_offset);
  s.cor_tokens = (COR_TOKEN_STRUCT *)((char *)(s.header) + s.header->cor_tokens_offset);
  if (joe_trace)
    {
      fprintf(stderr, "In theory we've mapped %s, and it has %ld tokens and %ld clusters", filename, s.header->n_tokens, s.header->n_clusters);
    
      /*		for(i = s.header->most_recent_token; i < NULL_INDEX; i = s.tokens[i].less_recent)
	fprintf(stderr, "token:%ld, recorded hash %ld, lookup returns: %ld, incluster: %ld\n", i, s.tokens[i].hash_code, no_learning_get_token_from_hash(&s, s.tokens[i].hash_code), s.tokens[i].cluster );
      */
      
      
    }
  
  
  
  tokenized_text = (index_t *)inbuf;
  
  if(joe_trace) fprintf(stderr, "about to tokenize with regex_text: %s\n", regex_text);
  
  nolearning_tokenize(&s, tokenized_text, &tokenized_text_len, text_len / 2 + 1, text + text_start, text_len, &regee);
  
  if(joe_trace) fprintf(stderr, "tokenized\n");
  
  out_pos = 0;
  outbuf[out_pos] = 0;
  
  /* index_t t; ** [i_a] it is C code, not C++ */
  for(i = 0; i < tokenized_text_len; i++)
    {
      t = tokenized_text[i];
      if(t == NULL_INDEX)
	out_pos = strmov(outbuf + out_pos, "new ") - outbuf;
      else if(s.tokens[t].cor_index == NULL_INDEX)
	out_pos = strmov(outbuf + out_pos, "hapax ") - outbuf;
      else
	out_pos += sprintf(outbuf + out_pos, "%ld ", s.cor_tokens[ s.tokens[t].cor_index ].cluster);
    }
  assert(outbuf[out_pos] == 0);
  
  crm_munmap_file ((void *) s.header);
  
  crm_destructive_alter_nvariable(out_var, out_var_len, outbuf, out_pos);
  
  return 0;
}

//for cooccurence histogram:
#define N_BINS 100
static void poke_around(CLUSTEROR_STATE_STRUCT *s)
{
  double min = 1000000.0, max = -1000000.0, a, b, bb;
  long i, j, k, bins[N_BINS];
  printf("s->header->n_tokens = %ld\n", s->header->n_tokens);
  printf("s->header->n_clusters = %ld\n", s->header->n_clusters);
  
  for(i = 0; i < s->header->n_cor_tokens; i++)
    for(j = i + 1; j < s->header->n_cor_tokens; j++)
      {
	a = score_cooccurance(s, s->cor_tokens[i].token, s->cor_tokens[j].token);
	if(a < min) min = a;
	if(a > max) max = a;
      }
  for(i = 0; i < N_BINS; i++)
    bins[i] = 0;
  
  bb = (max - min) / N_BINS;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    for(j = i + 1; j < s->header->n_cor_tokens; j++)
      {
	a = score_cooccurance(s, s->cor_tokens[i].token, s->cor_tokens[j].token);
	for(k = 0, b = bb + min; k < N_BINS; k++, b += bb)
	  if(a <= b)
	    {
	      bins[k]++;
	      break;
	    }
	if(k == N_BINS)
	  bins[N_BINS - 1]++;
      }
  printf("Cooccurences:\n\tmin: %0.3f\tmax:%0.3f\n", min, max);
  for(k = 0, b = min; k < N_BINS; k++, b += bb)
    printf("\t(%0.4f, %0.4f):\t%ld\n", b, b + bb, bins[k]);
  for(i = 0; i < N_BINS; i++)
    bins[i] = 0;
  bb = (max - min) / N_BINS;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    {
      a = score_cooccurance(s, s->cor_tokens[i].token, s->cor_tokens[ s->cor_tokens[i].nearest_neihbor ].token);
      for(k = 0, b = bb + min; k < N_BINS; k++, b += bb)
	if(a <= b)
	  {
	    bins[k]++;
	    break;
	  }
      if(k == N_BINS)
	bins[N_BINS - 1]++;
    }
  printf("Nearest neighbor cooccurences:\n");
  for(k = 0, b = min; k < N_BINS; k++, b += bb)
    printf("\t(%0.4f, %0.4f):\t%ld\n", b, b + bb, bins[k]);
  
  min = 1000000.0; max = -1000000.0;
  for(i = 0; i < s->header->n_tokens; i++)
    {
      a = get_occ(s, i);
      if(a < min) min = a;
      if(a > max) max = a;
    }
  for(i = 0; i < N_BINS; i++)
    bins[i] = 0;
  bb = (max - min) / N_BINS;
  for(i = 0; i < s->header->n_tokens; i++)
    {
      a = get_occ(s, i);
      for(k = 0, b = bb + min; k < N_BINS; k++, b += bb)
	if(a <= b)
	  {
	    bins[k]++;
	    break;
	  }
      if(k == N_BINS)
	bins[N_BINS - 1]++;
    }
  printf("Occurences:\n\tmin: %0.3f\tmax:%0.3f\n", min, max);
  for(k = 0, b = min; k < N_BINS; k++, b += bb)
    printf("\t(%0.3f, %0.3f):\t%ld\n", b, b + bb, bins[k]);
  
  min = 1000000.0; max = -1000000.0;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    {
      a = get_occ(s, s->cor_tokens[i].token);
      if(a < min) min = a;
      if(a > max) max = a;
    }
  for(i = 0; i < N_BINS; i++)
    bins[i] = 0;
  bb = (max - min) / N_BINS;
  for(i = 0; i < s->header->n_cor_tokens; i++)
    {
      a = get_occ(s, s->cor_tokens[i].token);
      for(k = 0, b = bb + min; k < N_BINS; k++, b += bb)
	if(a <= b)
	  {
	    bins[k]++;
	    break;
	  }
      if(k == N_BINS)
	bins[N_BINS - 1]++;
    }
  printf("cor_token Occurences:\n\tmin: %0.3f\tmax:%0.3f\n", min, max);
  for(k = 0, b = min; k < N_BINS; k++, b += bb)
    printf("\t(%0.3f, %0.3f):\t%ld\n", b, b + bb, bins[k]);
}

