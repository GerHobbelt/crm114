//      crm_expr_nn_clump.h

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
//     crm_expr_nn_clump.h - translate characters of a string.
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
//if we had 2^16 tokens we'd have 2^31 cooccurences which is bigger than a long can hold, so it is safe and prudent to use an index_t for token id's

//this uses 206470625 + sizeof(header) bytes of file or ~192.3Megs
#define MAX_TOKENS 100000
#define MAX_COR_TOKENS 10000

//while we can't have this many when we're done, we can intermediately
#define MAX_CLUSTERS    10000

//align segments of files to this many bytes, to make sure we don't point to wierd boundrys and gum things up
#define BYTE_ALIGN 4

//TYPEDEFS
//we use index_t whenever we're talking about indices because we might want to shrink things done later
typedef long index_t;

//we use NULL_INDEX just like we'd use NULL with pointers
#define NULL_INDEX 2147483647
//HAPAX_INDEX is what the non-learning tokenizer labels unfamiliar tokens
#define HAPAX_INDEX 2147483646

//we need some kind of floating point type to generate correlation scores
typedef double COOCCURRENCE_SCORE_TYPE;

#define REALLY_SMALL_SCORE -1000000.0

typedef struct mythical_cluster
{
    long                    next_free;
    COOCCURRENCE_SCORE_TYPE occurrences;
} CLUSTER_STRUCT;

typedef struct mythical_edge
{
    index_t edge_to;
    index_t next;
} EDGE_STRUCT;

typedef struct mythical_token
{
    index_t                 cor_index;
    index_t                 more_common; //we maintain a sorted list of token seen counts so that we only cluster the MAX_COR_TOKENS most common to kill hapaxes
    index_t                 less_common;
    COOCCURRENCE_SCORE_TYPE count;
    index_t                 more_recent; //we need to know the second least recent all the time, so we need to look up
    index_t                 less_recent; //next unused slot if this slot unused
    long                    hash_code;
} TOKEN_STRUCT;

typedef struct mythical_cor_token
{
    index_t token;
    index_t cluster;            //which cluster is this token in?
    index_t nearest_neihbor;
    index_t edges;
} COR_TOKEN_STRUCT;

typedef struct mythical_token_hash_node
{
    long key;
    //next free slot if not in use, NULL_INDEX if end of chain
    index_t next_in_hash_chain;
    //token = NULL_INDEX if unused
    index_t token;
} HASH_NODE_STRUCT;

typedef struct mythical_NNclusteror_header
{
    index_t                 n_tokens;
    index_t                 n_cor_tokens;
    index_t                 max_tokens;
    index_t                 max_cor_tokens;
    index_t                 n_clusters;
    long                    hash_slots_offset;
    long                    tokens_offset;
    long                    cor_tokens_offset;
    index_t                 next_free_cor_token;
    index_t                 first_unused_token_slot;
    index_t                 first_unused_hash_slot;
    index_t                 first_unused_cluster_slot;
    index_t                 last_unused_cluster_slot;
    index_t                 most_recent_token;
    index_t                 least_recent_token;
    index_t                 least_frequent_token;
    index_t                 least_frequent_cor_token; //the token number, not cor token number of the least frequent token in the corelation tables
    index_t                 first_unused_edge;
    long                    cooccurences_offset;
    long                    graph_offset;
    long                    clusters_offset;
    COOCCURRENCE_SCORE_TYPE tot_occ,       //total occurrances recorded, to normalize
                            normal_factor; //when tot_occ is huge divide everything down
    //then when we add we nult by normal_factor to make everything work out
} NNCLUSTEROR_HEADER_STRUCT;

typedef struct mythical_clusteror_state
{
    NNCLUSTEROR_HEADER_STRUCT *header;
    HASH_NODE_STRUCT          *hash_table;
    TOKEN_STRUCT              *tokens;
    COR_TOKEN_STRUCT          *cor_tokens;
    COOCCURRENCE_SCORE_TYPE   *cooccurences;

    EDGE_STRUCT *graph;
    //bit vector to flag when tokens have changed nearest neihbor or when tokens have changed at all for unlearning
    long           *token_nn_changed;
    long           *closed_list; //bit vector to flag when tokens are visited to propagate cluster member ship or when tokens are seen before we worry about clusters
    CLUSTER_STRUCT *clusters;
    index_t        *old_nearest_neihbors;
} CLUSTEROR_STATE_STRUCT;

