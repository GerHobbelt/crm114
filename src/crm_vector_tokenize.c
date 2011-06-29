//  crm_str_funcs.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2009  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
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
#include "crm114_osbf.h"



#if 0
#define ORIGINAL_VT_CODE  1
#endif



#if defined (ORIGINAL_VT_CODE)


///////////////////////////////////////////////////////////////////////////
//
//    This code section (from this comment block to the one declaring
//    "end of section dual-licensed to Bill Yerazunis and Joe
//    Langeway" is copyrighted and dual licensed by and to both Bill
//    Yerazunis and Joe Langeway; both have full rights to the code in
//    any way desired, including the right to relicense the code in
//    any way desired.
//
////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//    Vectorized tokenizing - get a bunch of features in a nice
//    predigested form (a counted array of chars plus control params
//    go in, and a nice array of 32-bit ints come out.  The idea is to
//    encapsulate tokenization/hashing into one function that all
//    CRM114 classifiers can use, and so improved tokenization raises
//    all boats equally, or something like that.
//
//    If you need two sets of hashes, call this routine twice, with
//    different pipeline coefficient arrays (the OSB and Markov
//    classifiers need this)
//
//    If the features_out area becomes close to overflowing, then
//    vector_stringhash will return with a value of next_offset <=
//    textlen.  If next_offset is > textlen, then there is nothing
//    more to hash.
//
//    The feature building is controlled via the pipeline coefficient
//    arrays as described in the paper "A Unified Approach To Spam
//    Filtration".  In short, each row of an array describes one
//    rendition of an arbitrarily long pipeline of hashed token
//    values; each row of the array supplies one output value.  Thus,
//    the 1x1 array {1} yields unigrams, the 5x6 array
//
//     {{ 1 3 0 0 0 0}
//      { 1 0 5 0 0 0}
//      { 1 0 0 11 0 0}
//      { 1 0 0 0 23 0}
//      { 1 0 0 0 0 47}}
//
//    yields "Classic CRM114" OSB features.  The unit vector
//
//     {{1}}
//
//    yields unigrams (that is, single units of whatever the
//    the tokenizing regex matched).  The 1x2array
//
//     {{1 1}}
//
//    yields bigrams that are not position nor order sensitive, while
//
//     {{1 2}}
//
//    yields bigrams that are order sensitive.
//
//    Because the array elements are used as dot-product multipliers
//    on the hashed token value pipeline, there is a small advantage to
//    having the elements of the array being odd (low bit set) and
//    relatively prime, as it decreases the chance of hash collisions.
//
//    NB: the reason that we have "output stride" is that for some formats,
//    we want more than 32 bits per feature (Markov, standard OSB, Winnow,
//    etc.) we need to interleave hashes, and "stride" makes that easy.
//
///////////////////////////////////////////////////////////////////////////

int crm_vector_tokenize
(
        const char                    *text,            // input string (null-safe!)
        int                            textlen,         //   how many bytes of input.
        int                            start_offset,    //     start tokenizing at this byte.
        VT_USERDEF_TOKENIZER          *tokenizer,       // the regex tokenizer (elements in struct MAY be changed)
        const VT_USERDEF_COEFF_MATRIX *our_coeff,       // the pipeline coefficient control array, etc.
        crmhash_t                     *features,        // where the output features go
        int                            featureslen,     //   how many output features (max)
        int                            features_stride, //   Spacing (in hashes) between features
        int                           *features_out     // how many longs did we actually use up
)
{
    int hashpipe[UNIFIED_WINDOW_LEN];  // the pipeline for hashes
    int keepgoing;                     // the loop controller
    int i, j, k;                       // some handy index vars
    int regcomp_status;
    int text_offset;
    int irow, icol;
    unsigned int ihash;
    char errortext[4096];

    //    now do the work.

    *features_out = 0;
    keepgoing = 1;
    j = 0;

    //    Compile the regex.
    if (tokenizer && tokenizer->regexlen)
    {
        regcomp_status = crm_regcomp(&tokenizer->regcb, tokenizer->regex, tokenizer->regexlen, REG_EXTENDED);
        if (regcomp_status > 0)
        {
            crm_regerror(regcomp_status, &tokenizer->regcb, errortext, 4096);
            nonfatalerror("Regular Expression Compilation Problem: ",
                    errortext);
            return -1;
        }
    }

    // fill the hashpipe with initialization
    for (i = 0; i < UNIFIED_WINDOW_LEN; i++)
        hashpipe[i] = 0xDEADBEEF;

    //   Run the hashpipe, either with regex, or without.
    //
    text_offset = start_offset;
    while (keepgoing)
    {
        //  If the pattern is empty, assume non-graph-delimited tokens
        //  (supposedly an 8% speed gain over regexec)
        if (tokenizer->regexlen == 0)
        {
            k = 0;
            //         skip non-graphical characthers
            tokenizer->match[0].rm_so = 0;
            while (!crm_isgraph(text[text_offset + tokenizer->match[0].rm_so])
                   && text_offset + tokenizer->match[0].rm_so < textlen)
                tokenizer->match[0].rm_so++;
            tokenizer->match[0].rm_eo = tokenizer->match[0].rm_so;
            while (crm_isgraph(text[text_offset + tokenizer->match[0].rm_eo])
                   && text_offset + tokenizer->match[0].rm_eo < textlen)
                tokenizer->match[0].rm_eo++;
            if (tokenizer->match[0].rm_so == tokenizer->match[0].rm_eo)
                k = 1;
        }
        else
        {
            k = crm_regexec(&tokenizer->regcb,
                    &text[text_offset],
                    textlen - text_offset,
                    WIDTHOF(tokenizer->match), tokenizer->match,
                    REG_EXTENDED, NULL);
        }


        //   Are we done?
        if (k == 0)
        {
            //   Not done,we have another token (the text in text[match[0].rm_so,
            //    of length match[0].rm_eo - match[0].rm_so size)

            //
            if (user_trace)
            {
                fprintf(stderr, "Token; k: %d T.O: %d len %d ( %d %d on >",
                        k,
                        text_offset,
                        tokenizer->match[0].rm_eo - tokenizer->match[0].rm_so,
                        tokenizer->match[0].rm_so,
                        tokenizer->match[0].rm_eo);
                for (k = tokenizer->match[0].rm_so + text_offset;
                     k < tokenizer->match[0].rm_eo + text_offset;
                     k++)
                    fprintf(stderr, "%c", text[k]);
                fprintf(stderr, "< )\n");
            }

            //   Now slide the hashpipe up one slot, and stuff this new token
            //   into the front of the pipeline
            //
            // for (i = UNIFIED_WINDOW_LEN; i > 0; i--)  // GerH points out that
            //  hashpipe [i] = hashpipe[i-1];            //  this smashes stack
            crm_memmove(&hashpipe[1], hashpipe,
                    sizeof(hashpipe) - sizeof(hashpipe[0]));

            hashpipe[0] = strnhash(&text[tokenizer->match[0].rm_so + text_offset],
                    tokenizer->match[0].rm_eo - tokenizer->match[0].rm_so);

            //    Now, for each row in the coefficient array, we create a
            //   feature.
            //
            for (irow = 0; irow < our_coeff->pipe_iters; irow++)
            {
                ihash = 0;
                for (icol = 0; icol < our_coeff->pipe_len; icol++)
                    ihash = ihash +
                            hashpipe[icol] * our_coeff->coeff_array[(our_coeff->pipe_len * irow) + icol];

                //    Stuff the final ihash value into reatures array
                features[*features_out] = ihash;
                if (internal_trace)
                    fprintf(stderr,
                            "New Feature: %lx at %d\n", (unsigned long int)ihash, *features_out);
                *features_out = *features_out + features_stride;
            }

            //   And finally move on to the next place in the input.
            //
            //  Move to end of current token.
            text_offset = text_offset + tokenizer->match[0].rm_eo;
        }
        else
        //     Failed to match.  This is the end...
        {
            keepgoing = 0;
        }

        //    Check to see if we have space left to add more
        //    features assuming there are any left to add.
        if (*features_out + our_coeff->pipe_iters + 1 + 2 > featureslen)
        {
            keepgoing = 0;
        }
    }
#if 0
    if (next_offset)
        *next_offset = text_offset + match[0].rm_eo;
#endif
    features[*features_out] = 0;
    features[*features_out + 1] = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   End of code section dual-licensed to Bill Yerazunis and Joe Langeway.
//
////////////////////////////////////////////////////////////////////////////




#else






///////////////////////////////////////////////////////////////////////////
//
//    This code section (from this comment block to the one declaring
//    "end of section dual-licensed to Bill Yerazunis and Joe
//    Langeway" is copyrighted and dual licensed by and to both Bill
//    Yerazunis and Joe Langeway; both have full rights to the code in
//    any way desired, including the right to relicense the code in
//    any way desired.
//
////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//    Vectorized tokenizing - get a bunch of features in a nice
//    predigested form (a counted array of chars plus control params
//    go in, and a nice array of 32-bit ints come out.  The idea is to
//    encapsulate tokenization/hashing into one function that all
//    CRM114 classifiers can use, and so improved tokenization raises
//    all boats equally, or something like that.
//
//    If you need two sets of hashes, call this routine twice, with
//    different pipeline coefficient arrays (the OSB and Markov
//    classifiers need this)
//
//    If the features_out area becomes close to overflowing, then
//    vector_stringhash will return with a value of next_offset <=
//    textlen.  If next_offset is > textlen, then there is nothing
//    more to hash.
//
//    The feature building is controlled via the pipeline coefficient
//    arrays as described in the paper "A Unified Approach To Spam
//    Filtration".  In short, each row of an array describes one
//    rendition of an arbitrarily long pipeline of hashed token
//    values; each row of the array supplies one output value.  Thus,
//    the 1x1 array {1} yields unigrams, the 5x6 array
//
//     {{ 1 3 0 0 0 0}
//      { 1 0 5 0 0 0}
//      { 1 0 0 11 0 0}
//      { 1 0 0 0 23 0}
//      { 1 0 0 0 0 47}}
//
//    yields "Classic CRM114" OSB features.  The unit vector
//
//     {{1}}
//
//    yields unigrams (that is, single units of whatever the
//    the tokenizing regex matched).  The 1x2 array
//
//     {{1 1}}
//
//    yields bigrams that are not position nor order sensitive, while
//
//     {{1 2}}
//
//    yields bigrams that are order sensitive.
//
//    Because the array elements are used as dot-product multipliers
//    on the hashed token value pipeline, there is a small advantage to
//    having the elements of the array being odd (low bit set) and
//    relatively prime, as it decreases the chance of hash collisions.
//
//    NB: the reason that we have "output stride" is that for some formats,
//    we want more than 32 bits per feature (Markov, standard OSB, Winnow,
//    etc.) we need to interleave hashes, and "stride" makes that easy.
//
///////////////////////////////////////////////////////////////////////////


#define CRM_VT_BURST_SIZE       256



typedef struct
{
    crmhash_t h1;
    crmhash_t h2;
} DUAL_CRMHASH_T;

typedef struct 
{
	crmhash_t feature;
	int weight;
	int order_no;
} CRM_SORT_ELEM;

typedef struct 
{
	crmhash_t feature[2];
	int weight[2];
	int order_no[2];
} CRM_SORT_ELEM2;

#if defined (CRM_WITHOUT_MJT_INLINED_QSORT)

static int hash_compare_1(void const *a, void const *b)
{
    crmhash_t *pa, *pb;

    pa = (crmhash_t *)a;
    pb = (crmhash_t *)b;
    if (*pa < *pb)
        return -1;

    if (*pa > *pb)
        return 1;

    return 0;
}

static int indirect_hash_compare_1(void const *a, void const *b)
{
    CRM_SORT_ELEM *pa, *pb;
	int rv;

    pa = (CRM_SORT_ELEM *)a;
    pb = (CRM_SORT_ELEM *)b;
    if (pa->feature < pb->feature)
        return -1;

    if (pa->feature > pb->feature)
        return 1;

    rv = (pb->order_no - pa->order_no);
	if (rv)
        return rv;

    rv = (pb->weight - pa->weight);
    return rv;
}

static int hash_compare_2(void const *a, void const *b)
{
    DUAL_CRMHASH_T *pa, *pb;

    pa = (DUAL_CRMHASH_T *)a;
    pb = (DUAL_CRMHASH_T *)b;
    if (pa->h1 < pb->h1)
        return -1;

    if (pa->h1 > pb->h1)
        return 1;

    if (pa->h2 < pb->h2)
        return -1;

    if (pa->h2 > pb->h2)
        return 1;

    return 0;
}

static int indirect_hash_compare_2(void const *a, void const *b)
{
    CRM_SORT_ELEM2 *pa, *pb;
	int rv;

    pa = (CRM_SORT_ELEM2 *)a;
    pb = (CRM_SORT_ELEM2 *)b;
    if (pa->feature[0] < pb->feature[0])
        return -1;

    if (pa->feature[0] > pb->feature[0])
        return 1;

    if (pa->feature[1] < pb->feature[1])
        return -1;

    if (pa->feature[1] > pb->feature[1])
        return 1;

    rv = (pb->order_no[0] - pa->order_no[0]);
	if (rv)
        return rv;

    rv = (pb->order_no[1] - pa->order_no[1]);
	if (rv)
        return rv;

    rv = (pb->weight[0] - pa->weight[0]);
	if (rv)
        return rv;

    rv = (pb->weight[1] - pa->weight[1]);
    return rv;
}

#else

#define hash_compare_1(a, b) \
    (*(a) < *(b))

#define indirect_hash_compare_1(a, b) \
    ((a)->feature < (b)->feature \
	|| ((a)->feature == (b)->feature && ((a)->order_no < (b)->order_no \
		|| ((a)->order_no == (b)->order_no && (a)->weight < (b)->weight))))

#define hash_compare_2(a, b) \
    ((a)->h1 < (b)->h1 || ((a)->h1 == (b)->h1 && (a)->h2 < (b)->h2))
/*  ((a)->h1 < (b)->h1 ? TRUE : (a)->h1 == (b)->h1 ? (a)->h2 < (b)->h2 : FALSE) */

#define indirect_hash_compare_2(a, b) \
    ((a)->feature[0] < (b)->feature[0] \
	|| ((a)->feature[0] == (b)->feature[0] && ((a)->feature[1] < (b)->feature[1] \
		|| ((a)->feature[1] == (b)->feature[1] && ((a)->order_no[0] < (b)->order_no[0] \
			|| ((a)->order_no[0] == (b)->order_no[0] && ((a)->order_no[1] < (b)->order_no[1] \
				|| ((a)->order_no[1] == (b)->order_no[1] && ((a)->weight[0] < (b)->weight[0] \
					|| ((a)->weight[0] == (b)->weight[0] && (a)->weight[1] < (b)->weight[1] \
					))))))))))

#endif





//
// The only thing this function does is perform the VT operation; that means
// you MUST have set up all parameters properly, no 'defaults' are assumed here.
//
// Return a negative number on error, zero on success.
//
//
// Features for the different 'strides' are stored in interleaved format, e.g.
// for stride=2 Markovian:
//
//   A
//   B
//   A
//   B
//   A
//   B
//   .
//   .
//   .
//
int crm_vector_tokenize
(
        const char                    *text,               // input string (null-safe!)
        int                            textlen,            //   how many bytes of input.
        int                            start_offset,       //     start tokenizing at this byte.
        VT_USERDEF_TOKENIZER          *tokenizer,          // the regex tokenizer (elements in struct MAY be changed)
        const VT_USERDEF_COEFF_MATRIX *our_coeff,          // the pipeline coefficient control array, etc.
        crmhash_t                     *features_buffer,    // where the output features go
        int                            features_bufferlen, //   how many output features (max)
        int                           *feature_weights,    // feature weight per feature
        int                           *order_no,           // order_no (starting at 0) per feature
        int                           *features_out        // how many longs did we actually use up
)
{
    crmhash_t hashpipe[CRM_VT_BURST_SIZE + UNIFIED_WINDOW_LEN - 1];  // the pipeline for hashes
	int hashpipe_offset = 0;
    int i;
    int feature_pos;
    crmhash_t *hp;
    int pipe_len;
    int pipe_iters;
    int pipe_matrices;
    int single_featureblock_size;

    if (internal_trace || user_trace)
    {
        fprintf(stderr, "VT: textlen = %d\n", textlen);
        fprintf(stderr, "VT: start_offset = %d\n", start_offset);
        fprintf(stderr, "VT: features_bufferlen = %d\n", features_bufferlen);
    }

    // sanity checks: everything must be 'valid'; no 'default' NULL pointers, nor zeroed/out-of-range values accepted.
    if (!text || !our_coeff || !features_buffer || !features_out)
    {
        CRM_ASSERT(!"This should never happen in a properly coded CRM114...");
        return -1;
    }

    // tag the output series with a zero asap:
    *features_out = 0;

    pipe_iters = our_coeff->cm.pipe_iters;
    pipe_len = our_coeff->cm.pipe_len;
    pipe_matrices = our_coeff->cm.output_stride;
    if (internal_trace || user_trace)
    {
        fprintf(stderr, "VT: pipe_iters = %d\n", pipe_iters);
        fprintf(stderr, "VT: pipe_len = %d\n", pipe_len);
        fprintf(stderr, "VT: pipe_matrices = %d\n", pipe_matrices);
    }
    if (pipe_iters <= 0 || pipe_iters > UNIFIED_VECTOR_LIMIT)
    {
        // index out of configurable range
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the VT iteration count (%d) is out of bounds [%d .. %d].",
                pipe_iters, 1, UNIFIED_VECTOR_LIMIT);
        return -1;
    }
    if (pipe_len <= 0 || pipe_len > UNIFIED_WINDOW_LEN)
    {
        // index out of configurable range
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the VT pipeline depth (%d) is out of bounds [%d .. %d].",
                pipe_len, 1, UNIFIED_WINDOW_LEN);
        return -1;
    }
    if (pipe_matrices <= 0 || pipe_matrices > UNIFIED_VECTOR_STRIDE)
    {
        // index out of configurable range
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the number of VT matrixes (%d) is out of bounds [%d .. %d].",
                pipe_matrices, 1, UNIFIED_VECTOR_STRIDE);
        return -1;
    }

    if (our_coeff->fw.column_count <= 0 || our_coeff->fw.column_count > UNIFIED_VECTOR_LIMIT)
    {
        // index out of configurable range
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the Feature Weight column (~ iteration) count (%d) is out of bounds [%d .. %d].",
                our_coeff->fw.column_count, 1, UNIFIED_VECTOR_LIMIT);
        return -1;
    }
    if (our_coeff->fw.row_count <= 0 || our_coeff->fw.row_count > UNIFIED_VECTOR_STRIDE)
    {
        // index out of configurable range
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the number of Feature Weight rows (~ VT matrixes) (%d) is out of bounds [%d .. %d].",
                our_coeff->fw.row_count, 1, UNIFIED_VECTOR_STRIDE);
        return -1;
    }

    single_featureblock_size = pipe_iters * pipe_matrices;
    if (internal_trace || user_trace)
    {
        fprintf(stderr, "VT: single_featureblock_size = %d\n", single_featureblock_size);
    }

    if (single_featureblock_size > features_bufferlen)
    {
        // feature storage space too small:
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the feature bucket is too small (%d) to contain the minimum "
                                    "required number of features (%d) at least.",
                features_bufferlen, single_featureblock_size);
        return -1;         // can't even store the features from one token, so why bother?
    }

    if (internal_trace || user_trace)
    {
        fprintf(stderr, "VT: element_step_size a.k.a. pipe_matrices = %d\n", pipe_matrices);
    }

    // is the tokenizer also configured properly?
    if (!tokenizer->tokenizer)
    {
        nonfatalerror("Sanity check failed while starting up the VT (Vector Tokenizer).",
                "The VT tokenizer has not been set up!");
        return -1;
    }
    if (!tokenizer->cleanup)
    {
        nonfatalerror("Sanity check failed while starting up the VT (Vector Tokenizer).",
                "Nobody's apparently cleaning up after the VT tokenizer!");
        return -1;
    }

    //    now do the work.

    // set the text to tokenize
    if (text)
    {
        tokenizer->input_text = text;
        tokenizer->input_textlen = textlen;
        tokenizer->input_next_offset = start_offset;
        tokenizer->eos_reached = 0;
        // do *NOT* *RESET* tokenizer->initial_setup_done as the user MAY already have called the tokenizer before!
        // tokenizer->initial_setup_done = 0;
    }


#if 0
	// fill the start of the hashpipe with initialization
    for (i = 0; i < UNIFIED_WINDOW_LEN - 1; i++)
    {
        hashpipe[i] = 0xDEADBEEF;
    }
#endif

    //
    // Run the hashpipe, either with regex, or without - always use the provided tokenizer!
    //
    // loop: check to see if we have space left to add more
    //       features assuming there are any left to add.
    for (feature_pos = 0; ;)
    {
        int burst_len;

        // make sure the requested burstlen is small enough so the results derived from its
        // output will NOT overflow the target:
        burst_len = CRM_MIN(CRM_VT_BURST_SIZE, features_bufferlen / single_featureblock_size) - hashpipe_offset;

        // cough up a fresh token burst now and put them at the front of the pipe, i.e. at the END of the array:
        i = tokenizer->tokenizer(tokenizer, &hashpipe[hashpipe_offset], burst_len);
        // did we get any?
        if (internal_trace || user_trace)
        {
            fprintf(stderr, "VT: tokenizer delivered %d tokens\n", i);
        if (internal_trace && i > 0)
        {
			int h;
			for (h = 0; h < i; h++)
			{
				fprintf(stderr, "VT: burst: token hash[%3d] = 0x%08lX\n", h, (unsigned long int)hashpipe[h]);
			}
		}
		}
        if (i < 0)
        {
            return -1;
        }

        // EOS reached?
        // i.e. did we actually GET a new token or was the input already utterly empty to start with now?
        //
        // Fringe case which needs fixing? -- what if the tokenizer produced less tokens than the
        // maximum possible 'burst'? We now use a reversed pipeline: higher indexes are newer
        // instead of older; the 'hp' positional pointer can do it's regular job in this case.
		// BUT before we go ahead, we'd better make sure the VT matrix can be applied to the 
		// input data, i.e. make SURE there's ENOUGH input hashes now to apply each row of the
		// VT matrix without underrunning the hashpipe array.
        //
		i += hashpipe_offset;
		hashpipe_offset = i;

		i -= pipe_len;
        if (i < 0)
            break;

        // position the 'real' hashpipe at the proper spot in the 'burst': 
		// the oldest token is the first one in the burst, yet we must make sure the VT matrix
		// can be applied without underrunning the hashpipe array.
		hp = &hashpipe[pipe_len - 1];

        //
        // Since we loaded the tokens in burst, we can now speed up the second part of the process due to
        // loop tightening here too; since the 'reversed' pipeline stores newer tokens at higher indexes,
        // walking the sliding window (a.k.a. 'hp' pipeline) UP from old to new, means we can reposition the
        // 'hp' pipeline reference to point at the NEXT token produced during the burst tokenization before,
        // while keeping track how many tokens were produced by reducing the burst length 'i' --> i--, hp++
        //
        for ( ; i >= 0; i--, hp++)
        {
            int matrix;
            const int *coeff_array;
            const int *feature_weight_table;
            // remember how far we filled the features bucket already;
            // we're going to interleave another block:
            int basepos = feature_pos;

            feature_weight_table = our_coeff->fw.feature_weight;
            coeff_array = our_coeff->cm.coeff_array;

            // select the proper 2D matrix plane from the 3D matrix cube on every round:
            // one full round per available coefficient matrix
            for (matrix = 0;
                 matrix < pipe_matrices;
                 matrix++,
                 (coeff_array += UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT),
                 (feature_weight_table += UNIFIED_VECTOR_LIMIT))
            {
                int irow;

                CRM_ASSERT(matrix >= 0);
                CRM_ASSERT(matrix < UNIFIED_VECTOR_STRIDE);

                // reset the index for interleaved feature storage:
                feature_pos = basepos + matrix;

                //
                // Features for the different 'strides' are stored in interleaved format, e.g.
                // for stride=2 Markovian:
                //
                //   A
                //   B
                //   A
                //   B
                //   A
                //   B
                //   .
                //   .
                //   .
                //

                //    Now, for each row in the coefficient array, we create a
                //   feature.
                //
                CRM_ASSERT(pipe_iters <= UNIFIED_VECTOR_LIMIT);
                for (irow = 0; irow < pipe_iters; irow++)
                {
                    int icol;
                    register const int *ca = &coeff_array[UNIFIED_WINDOW_LEN * irow];

                    // [i_a] TO TEST:
                    //
                    // as we are creating a new hash from the combined tokens, it may be better to not use the
                    // old 'universal hash' method (new hash = old hash + position dependent multiplier * token hash)
                    // as there are hash methods available which have much better avalanche behaviour.
                    //
                    // To use such a thing, we'll need to store the picked token hashes as is, then feed the new
                    // collection to the hash routine as if this collection of token hashes is raw input.
                    //
                    register crmhash_t ihash = 0;

                    // WARNING: remember that we use a 'reversed' pipeline here, so higher indexes are newer;
                    // as we have positive offsets in ca[] pointing at presumably higher=OLDER tokens, we must
                    // negate them (index * -1) to point at the intended, OLDER, token in the hashpipe pointed at
                    // by 'hp' -- a valid case for 'negative indexing' in what seems to be an array but is a
                    // positional pointer ('hp') pointing at the storage END of the pipeline.
                    CRM_ASSERT(pipe_len <= UNIFIED_WINDOW_LEN);
                    for (icol = 0; icol < pipe_len; icol++)
                    {
                        register crmhash_t universal_multiplier = ca[icol];
                        // assert: make sure the multiplier values are odd for proper spread.
                        CRM_ASSERT(universal_multiplier == 0 ? TRUE : (universal_multiplier & 1));
                        // the - minus 'icol' is intentional: reversed pipeline: older is lower index.
                        ihash += hp[-icol] *universal_multiplier;
                    }

                    //    Stuff the final ihash value into features array -- iff we still got room for it
                    if (feature_pos >= features_bufferlen)
                    {
                        break;
                    }
                    features_buffer[feature_pos] = ihash;
                    if (feature_weights)
                    {
                        feature_weights[feature_pos] = feature_weight_table[irow];
                    }
                    if (order_no)
                    {
                        order_no[feature_pos] = irow;
                    }

                    crm_analysis_mark(&analysis_cfg,
                            MARK_VT_HASH_VALUE,
                            irow,
                            "LLi",
                            (unsigned long long int)ihash,
                            (long long int)feature_pos,
                            (int)matrix);

                    if (internal_trace || user_trace)
                    {
						fprintf(stderr, "New Feature: %08lX (weight: %d) at %d / row: %d / matrix: %d\n",
                                (unsigned long int)ihash, 
			(feature_weights ? feature_weights[feature_pos] : feature_weight_table[irow]),
                                feature_pos, irow, matrix);
                    }
                    feature_pos += pipe_matrices;
                }
            }
            // and correct for the interleaving we just did:
            feature_pos -= pipe_matrices - 1;     // feature_pos points 'pipe_matrices' elems PAST the last write; should be only +1(ONE)

            // also check if we should get another series of tokens ... or maybe not.
            // only allow another burst when we can at least write a single feature strip.
			CRM_ASSERT(feature_pos + single_featureblock_size <= features_bufferlen);
        }

        // check if we should get another series of tokens ... or maybe not.
		CRM_ASSERT(feature_pos + single_featureblock_size <= features_bufferlen);

		// move the last pipe_len-1 hashes to the front of the pipe: those will 'prefix' the
		// next burst, thus insuring the next round will continue where it left off and have
		// no trouble accessing 'historic' hashes while applying the VT matrix.
		crm_memmove(hashpipe, &hp[-pipe_len], (pipe_len - 1)* sizeof(hashpipe[0]));
		hashpipe_offset = pipe_len - 1;
    }

    // update the caller on what's left in the input after this:
    // *next_offset = tokenizer->input_next_offset;



//#if USE_FIXED_UNIQUE_MODE
    if (our_coeff->flags.sorted_output || our_coeff->flags.unique)
    {
        CRM_ASSERT(our_coeff->cm.output_stride > 0);
        CRM_ASSERT(our_coeff->cm.output_stride < UNIFIED_VECTOR_STRIDE);

        if (user_trace && our_coeff->flags.unique)
        {
            fprintf(stderr, " enabling uniqueifying features.\n");
        }

        if (internal_trace)
        {
            fprintf(stderr, "Total unsorted hashes generated: %d\n", feature_pos);
            switch (our_coeff->cm.output_stride)
            {
            case 0:
            case 1:
                for (i = 0; i < feature_pos; i++)
                {
                    fprintf(stderr, "hash[%6d] = %08lx", i, (unsigned long int)features_buffer[i]);
                    if (feature_weights)
                    {
                    fprintf(stderr, ", weight %d", feature_weights[i]);
                    }
                    if (order_no)
                    {
                    fprintf(stderr, ", order %d", order_no[i]);
                    }
                    fprintf(stderr, "\n");
                }
                break;

            default:
                for (i = 0; i < feature_pos;)
                {
                    int j;

                    fprintf(stderr, "hash[%6d] = (", i);

                    for (j = 0; j < our_coeff->cm.output_stride; j++)
                    {
                        fprintf(stderr, "%08lx ", (unsigned long int)features_buffer[i++]);
						if (feature_weights)
						{
						fprintf(stderr, "[weight %6d] ", feature_weights[i-1]);
						}
						if (order_no)
						{
						fprintf(stderr, "[order %3d] ", order_no[i-1]);
						}
                    }
                    fprintf(stderr, ")\n");
                }
                break;
            }
        }

        //   Now sort the hashes array.
		if (feature_pos > 0)
		{
        switch (our_coeff->cm.output_stride)
        {
        default:
            // failure case:
            nonfatalerror_ex(SRC_LOC(),
                    "VT can't perform feature hash ordering nor "
                    "<unique>-ifying for stride type %d "
                    "(supported types are: {1, 2})\n",
                    our_coeff->cm.output_stride);
            return -1;

        case 1:
			if (!feature_weights && !order_no)
			{
            QSORT(crmhash_t, features_buffer, feature_pos, hash_compare_1);

            if (our_coeff->flags.unique)
            {
                int j;

                for (i = 1, j = 0; i < feature_pos; i++)
                {
                    if (features_buffer[i] != features_buffer[j])
                    {
						if (++j != i)
						{
                        features_buffer[j] = features_buffer[i];
						}
                    }
                }
                feature_pos = j + 1;
            }
			}
			else
			{
				int j;
				CRM_SORT_ELEM *indirect = calloc(feature_pos, sizeof(indirect[0]));

				        if (!indirect)
        {
            untrappableerror("Cannot allocate VT tokenizer indirection array for sorting / unique-ing", "Stick a fork in us; we're _done_.");
        }
						for (j = 0; j < feature_pos; j++)
						{
							indirect[j].feature = features_buffer[j];
						}
						if (feature_weights)
						{
						for (j = 0; j < feature_pos; j++)
						{
							indirect[j].weight = feature_weights[j];
						}
						}
						if (order_no)
						{
						for (j = 0; j < feature_pos; j++)
						{
							indirect[j].order_no = order_no[j];
						}
						}
            QSORT(CRM_SORT_ELEM, indirect, feature_pos, indirect_hash_compare_1);

            if (our_coeff->flags.unique)
            {
                for (i = 1, j = 0; i < feature_pos; i++)
                {
					if (indirect[i].feature != indirect[j].feature)
                    {
						if (++j != i)
						{
                        indirect[j] = indirect[i];
						}
                    }
                }
                feature_pos = j + 1;
            }
			// now copy back the data to the target arrays
						for (j = 0; j < feature_pos; j++)
						{
							features_buffer[j] = indirect[j].feature;
						}
						if (feature_weights)
						{
						for (j = 0; j < feature_pos; j++)
						{
							feature_weights[j] = indirect[j].weight;
						}
						}
						if (order_no)
						{
						for (j = 0; j < feature_pos; j++)
						{
							order_no[j] = indirect[j].order_no;
						}
						}
			}
            break;

        case 2:
			if (!feature_weights && !order_no)
			{
				int len = feature_pos / 2;
				DUAL_CRMHASH_T *buf = (DUAL_CRMHASH_T *)features_buffer;

            QSORT(DUAL_CRMHASH_T, buf, len, hash_compare_2);

            if (our_coeff->flags.unique)
            {
                int j;

                for (i = 1, j = 0; i < len; i++)
                {
					if (buf[i].h1 != buf[j].h1 || buf[i].h2 != buf[j].h2)
                    {
						if (++j != i)
						{
                        buf[j] = buf[i];
						}
                    }
                }
                len = j + 1;
            }
			feature_pos = len * 2;
			}
			else
			{
				int j;
				int len = feature_pos / 2;
				CRM_SORT_ELEM2 *indirect = calloc(len, sizeof(indirect[0]));

				        if (!indirect)
        {
            untrappableerror("Cannot allocate VT tokenizer indirection array for sorting / unique-ing", "Stick a fork in us; we're _done_.");
        }
						for (j = 0; j < len; j++)
						{
							indirect[j].feature[0] = features_buffer[j * 2];
							indirect[j].feature[1] = features_buffer[j * 2 + 1];
						}
						if (feature_weights)
						{
						for (j = 0; j < len; j++)
						{
							indirect[j].weight[0] = feature_weights[j * 2];
							indirect[j].weight[1] = feature_weights[j * 2 + 1];
						}
						}
						if (order_no)
						{
						for (j = 0; j < len; j++)
						{
							indirect[j].order_no[0] = order_no[j * 2];
							indirect[j].order_no[1] = order_no[j * 2 + 1];
						}
						}
            QSORT(CRM_SORT_ELEM2, indirect, len, indirect_hash_compare_2);

            if (our_coeff->flags.unique)
            {
                for (i = 1, j = 0; i < len; i++)
                {
					if (indirect[i].feature[0] != indirect[j].feature[0]
					|| indirect[i].feature[1] != indirect[j].feature[1])
                    {
						if (++j != i)
						{
                        indirect[j] = indirect[i];
						}
                    }
                }
                len = j + 1;
            }
			feature_pos = len * 2;

			// now copy back the data to the target arrays
						for (j = 0; j < len; j++)
						{
							features_buffer[j * 2] = indirect[j].feature[0];
							features_buffer[j * 2 + 1] = indirect[j].feature[1];
						}
						if (feature_weights)
						{
						for (j = 0; j < len; j++)
						{
							feature_weights[j * 2] = indirect[j].weight[0];
							feature_weights[j * 2 + 1] = indirect[j].weight[1];
						}
						}
						if (order_no)
						{
						for (j = 0; j < len; j++)
						{
							order_no[j * 2] = indirect[j].order_no[0];
							order_no[j * 2 + 1] = indirect[j].order_no[1];
						}
						}
			}
            break;
        }
		}

        if (internal_trace)
        {
            fprintf(stderr, "Total hashes generated POST-order/unique: %d\n", feature_pos);
            switch (our_coeff->cm.output_stride)
            {
            case 0:
            case 1:
                for (i = 0; i < feature_pos; i++)
                {
                    fprintf(stderr, "hash[%6d] = %08lx", i, (unsigned long int)features_buffer[i]);
                    if (feature_weights)
                    {
                    fprintf(stderr, ", weight %d", feature_weights[i]);
                    }
                    if (order_no)
                    {
                    fprintf(stderr, ", order %d", order_no[i]);
                    }
                    fprintf(stderr, "\n");
                }
                break;

            default:
                for (i = 0; i < feature_pos;)
                {
                    int j;

                    fprintf(stderr, "hash[%6d] = (", i);

                    for (j = 0; j < our_coeff->cm.output_stride; j++)
                    {
                        fprintf(stderr, "%08lx ", (unsigned long int)features_buffer[i++]);
						if (feature_weights)
						{
						fprintf(stderr, "[weight %6d] ", feature_weights[i-1]);
						}
						if (order_no)
						{
						fprintf(stderr, "[order %3d] ", order_no[i-1]);
						}
                    }
                    fprintf(stderr, ")\n");
                }
                break;
            }
        }
    }


    *features_out = feature_pos;
    if (user_trace || internal_trace)
    {
        fprintf(stderr, "VT: feature count = %d\n", feature_pos);
    }

    // [i_a]
    // Fringe case: yes, we discard the last few feature(s) stored when the
    // feature_bufferlen % pipe_matrices != 0 where pipe_matrices > 1 as then an
    // earlier matrix=... loop WILL have generated one _more_ feature hash then
    // the very last round we are looking at right now by using 'feature_pos'.
    // We'll live with that. Nevertheless, now *features_out will always contain a
    // value which is zero (MOD pipe_matrices) which is kinda nice in a different way.

    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   End of code section dual-licensed to Bill Yerazunis and Joe Langeway.
//
////////////////////////////////////////////////////////////////////////////





#endif




static void discover_matrices_max_dimensions(const VT_USERDEF_COEFF_MATRIX *matrix,
        int *pipe_len, int *pipe_iter, int *stride,
        int *weight_cols, int *weight_rows)
{
    int x;
    int y;
    int z;
    int x_max = 0;
    int y_max = 0;
    int z_max = 0;
    int i;
    const int *m;

    CRM_ASSERT(pipe_len);
    CRM_ASSERT(pipe_iter);
    CRM_ASSERT(stride);
    CRM_ASSERT(weight_rows);
    CRM_ASSERT(weight_cols);
    CRM_ASSERT(matrix);

    /*
     * Could've coded it like this:
     *
     * for (z = UNIFIED_VECTOR_STRIDE; z-- >= 0; )
     * {
     *      for (y = UNIFIED_VECTOR_LIMIT; y-- >= 0; )
     *      {
     *              for (x = UNIFIED_WINDOW_LEN; x-- >= 0; )
     *              {
     *                      ...
     *
     * but I wanted to do it different this time.
     * The excuse? Well, this one may be slightly faster than the above
     * because the number of multiplications, etc. in there are quite
     * many: one round for each and every element in the matrix,
     * but since we may expect tiny portions of the matrix space
     * filled, deriving x/y/z from the continguous index is probably
     * faster. Especially when the compiler converts those divisions
     * in here to bitshifts (which can be done as long as those constants
     * remain powers of 2...)
     *
     * Besides, as this code uses less vars in the main flow (which just
     * skips zeroes), this thing optimizes better on register-depraved
     * architectures, such as i86.
     *
     * But that's all rather anal excuses for checking out a crazy idea.
     *
     * ---
     *
     * And anyhow, this thing can run quite a bit faster still as we
     * 'know' valid rows always have their column[0] set, so the fastest
     * way to find max_y at least is to just scan the column[0] bottom up,
     * then once we non-zero values there, we can jump to the end of the
     * row to scan backwards looking for another possible max_x.
     *
     * The same for max_z: the max_z is the last one which has column[0]
     * for row[0] set. Once we've found that one that's it for max_z.
     *
     * But then everything after that '---' there is optimization
     * derived from knowledge about the data living in these matrices.
     *
     * Users _can_ feed this thing 'weird' row data, which will remain
     * undiscovered then.
     *
     * So we do it the 'we-don't-know-about-the-data-at-all' way here,
     * as this code will be used in diagnostics code, where such cuteness
     * doesn't really help:
     */
    m = matrix->cm.coeff_array;

    for (i = WIDTHOF(matrix->cm.coeff_array); --i >= 0;)
    {
        if (!m[i])
            continue;

        /* now we've hit a non-zero entry: derive x/y/z from i: */
        z = i / (UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT);
        CRM_ASSERT(z < UNIFIED_VECTOR_STRIDE);
        y = i / UNIFIED_WINDOW_LEN;
        x = i % UNIFIED_WINDOW_LEN;
        /* alt code: x = i - UNIFIED_WINDOW_LEN * y; -- y here still 'includes' z as well */
        y -= UNIFIED_VECTOR_LIMIT * z;

        if (x_max < x)
        {
            x_max = x;
        }
        if (y_max < y)
        {
            y_max = y;
        }
        CRM_ASSERT(z_max != 0 ? z_max >= z : TRUE);
        if (!z_max)
        {
            z_max = z;
        }

        /*
         * ignore the rest of this row, as it doesn't add
         * anything to what we found here. We sub X instead
         * of X-1 which would correct for that extra --i up
         * there in the loop, but we do NOT want to check
         * column [0] so doing it like this will get us
         * to the last column of the previous row, which
         * is where we want to start scanning again.
         */
        i -= x;
    }

    *pipe_len = x_max + 1;
    *pipe_iter = y_max + 1;
    *stride = z_max + 1;


    /* now do the same for the 2D weight matrix: */
    x_max = 0;
    y_max = 0;
    m = matrix->fw.feature_weight;

    for (i = WIDTHOF(matrix->fw.feature_weight); --i >= 0;)
    {
        if (!m[i])
            continue;

        /* now we've hit a non-zero entry: derive x/y from i: */
        y = i / UNIFIED_VECTOR_LIMIT;
        x = i % UNIFIED_VECTOR_LIMIT;

        if (x_max < x)
        {
            x_max = x;
        }
        CRM_ASSERT(y_max != 0 ? y_max >= y : TRUE);
        if (!y_max)
        {
            y_max = y;
        }

        /*
         * ignore the rest of this row, as it doesn't add
         * anything to what we found here. We sub X instead
         * of X-1 which would correct for that extra --i up
         * there in the loop, but we do NOT want to check
         * column[0] so doing it like this will get us
         * to the last column of the previous row, which
         * is where we want to start scanning again.
         */
        i -= x;
    }


    *weight_rows = y_max + 1;
    *weight_cols = x_max + 1;

	if (user_trace)
{
fprintf(stderr, "discover_matrices_max_dimensions() --> x: %d, y: %d, z: %d / weight: x: %d, y: %d\n",
        *pipe_len, *pipe_iter, *stride,        *weight_cols, *weight_rows);
}
}





#if 0

static const int markov1_coeff[] =
{
    1, 0, 0, 0, 0,
    1, 3, 0, 0, 0,
    1, 0, 5, 0, 0,
    1, 3, 5, 0, 0,
    1, 0, 0, 11, 0,
    1, 3, 0, 11, 0,
    1, 0, 5, 11, 0,
    1, 3, 5, 11, 0,
    1, 0, 0, 0, 23,
    1, 3, 0, 0, 23,
    1, 0, 5, 0, 23,
    1, 3, 5, 0, 23,
    1, 0, 0, 11, 23,
    1, 3, 0, 11, 23,
    1, 0, 5, 11, 23,
    1, 3, 5, 11, 23
};
static const int markov1_coeff_width = OSB_BAYES_WINDOW_LEN;                               // was 5
static const int markov1_coeff_height = WIDTHOF(markov1_coeff) / OSB_BAYES_WINDOW_LEN;     // should be 16

static const int markov2_coeff[] =
{
    7, 0, 0, 0, 0,
    7, 13, 0, 0, 0,
    7, 0, 29, 0, 0,
    7, 13, 29, 0, 0,
    7, 0, 0, 51, 0,
    7, 13, 0, 51, 0,
    7, 0, 29, 51, 0,
    7, 13, 29, 51, 0,
    7, 0, 0, 0, 101,
    7, 13, 0, 0, 101,
    7, 0, 29, 0, 101,
    7, 13, 29, 0, 101,
    7, 0, 0, 51, 101,
    7, 13, 0, 51, 101,
    7, 0, 29, 51, 101,
    7, 13, 29, 51, 101
};
static const int markov2_coeff_width = OSB_BAYES_WINDOW_LEN;                               // was 5
static const int markov2_coeff_height = WIDTHOF(markov2_coeff) / OSB_BAYES_WINDOW_LEN;     // should be 16
#endif

//#ifdef JUST_FOR_REFERENCE

//    hctable is where the OSB coeffs came from - this is now just a
//    historical artifact - DO NOT USE THIS!!! 
// --> [i_a] yes, we DO use this! Because those old VT tables can be readily generated from this in a very flexible manner!

/*
 * 'Unified hash' multipliers. Must be odd; being prime is good.
 */
static const int hctable[] =
{
    1, 7,
    3, 13,
    5, 29,
    11, 53 /*51*/,
    23, 101,
    47, 211 /*203*/,
    97, 409 /*407*/,
    197, 821 /*817*/,
    397, 1637,
    797, 3299 /*3277*/,
	1597, 6607,
2999,13259,
6007,26597,
12049,53077,
24083,106163,
48221,213467,
};

// #endif

#if 0
static const int osb1_coeff[] =
{
    1, 3, 0, 0, 0,
    1, 0, 5, 0, 0,
    1, 0, 0, 11, 0,
    1, 0, 0, 0, 23
};
static const int osb1_coeff_width = OSB_BAYES_WINDOW_LEN;                             // was 5
static const int osb1_coeff_height = WIDTHOF(osb1_coeff) / OSB_BAYES_WINDOW_LEN;      // should be 4

static const int osb2_coeff[] =
{
    7, 13, 0, 0, 0,
    7, 0, 29, 0, 0,
    7, 0, 0, 51, 0,
    7, 0, 0, 0, 101
};
static const int osb2_coeff_width = OSB_BAYES_WINDOW_LEN;                             // was 5
static const int osb2_coeff_height = WIDTHOF(osb2_coeff) / OSB_BAYES_WINDOW_LEN;      // should be 4
#endif

static const int string1_coeff[] =
{ 1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 49, 51 };

static const int string2_coeff[] =
{ 51, 49, 43, 41, 37, 31, 29, 23, 19, 17, 13, 11, 7, 5, 3, 1 };

static const int unigram_coeff[] =
{ 1, 17, 31, 49 };

#if 0
static const int winnow1_coeff[] =
{
    1, 3, 0, 0,  0,  0,  0,   0,   0,   0,
    1, 0, 5, 0,  0,  0,  0,   0,   0,   0,
    1, 0, 0, 11, 0,  0,  0,   0,   0,   0,
    1, 0, 0, 0, 23,  0,  0,   0,   0,   0,
    1, 0, 0, 0,  0, 47,  0,   0,   0,   0,
    1, 0, 0, 0,  0,  0, 97,   0,   0,   0,
    1, 0, 0, 0,  0,  0,  0, 197,   0,   0,
    1, 0, 0, 0,  0,  0,  0,   0, 397,   0,
    1, 0, 0, 0,  0,  0,  0,   0,   0, 797,
};
static const int winnow1_coeff_width = 10;
static const int winnow1_coeff_height = WIDTHOF(winnow1_coeff) / 10;

static const int winnow2_coeff[] =
{
    7, 13,  0,  0,  0,  0,  0,   0,   0,   0,
    7,  0, 29,  0,  0,  0,  0,   0,   0,   0,
    7,  0,  0, 51,  0,  0,  0,   0,   0,   0,
    7,  0,  0,  0, 101,  0,  0,   0,   0,   0,
    7,  0,  0,  0,  0, 203,  0,   0,   0,   0,
    7,  0,  0,  0,  0,  0, 407,   0,   0,   0,
    7,  0,  0,  0,  0,  0,  0, 817,   0,   0,
    7,  0,  0,  0,  0,  0,  0,   0, 1637,   0,
    7,  0,  0,  0,  0,  0,  0,   0,   0, 3277,
};
static const int winnow2_coeff_width = 10;
static const int winnow2_coeff_height = WIDTHOF(winnow2_coeff) / 10;
#endif







//
//    Note - a strict interpretation of Bayesian
//    chain probabilities should use 0 as the initial
//    state.  However, because we rapidly run out of
//    significant digits, we use a much less strong
//    initial state.   Note also that any nonzero
//    positive value prevents divide-by-zero
//

/*
 * All features of any 'order' (== VT matrix row)
 * all weigh in as much as the next one.
 */
static const int flat_model_weight[] =
{
    1,
};

static const int osb_bayes_feature_weight[] =
{
	24, 14, 7, 4, 2, 1
};
// cubic weights seems to work well for chi^2...- Fidelis
static const int chi2_feature_weight[] =
{
	125, 64, 27, 8, 1
};


//
//   Calculate entropic weight of this feature.
//   (because these are correllated features, this is
//   linear, not logarithmic)
//
//   These weights correspond to the number of elements
//   in the hashpipe that are used for this particular
//   calculation, == 1 + bitcount(Jval)

static const int entropic_weight[16] = // Jval
{
    1, // 0
    2, // 1
    2, // 2
    3, // 3
    2, // 4
    3, // 5
    3, // 6
    4, // 7
    2, // 8
    3, // 9
    3, // 10
    4, // 11
    3, // 12
    4, // 13
    4, // 14
    5
};      // 15


//
//   Calculate entropic weight of this feature.
//    However, this is based on a Hidden Markov Model
//    calculation.  Maybe it's right, maybe not.  It does
//    seem to work better than constant or entropic...

static const int hidden_markov_weight[16] = // Jval
{
    1, // 0
    2, // 1
    2, // 2
    4, // 3
    2, // 4
    4, // 5
    4, // 6
    8, // 7
    2, // 8
    4, // 9
    4, // 10
    8, // 11
    4, // 12
    8, // 13
    8, // 14
    16
};       // 15


//
//   Calculate entropic weight of this feature.
//   However, this is based on a more agressive Hidden
//   Markov Model calculation.  Maybe it's right, maybe
//   not.  However, testing shows that Super-Markov is
//   more accurate than constant, entropic, or straight Markov,
//   with errcounts of 69, 69, 63, and 56 per 5k,respectively.
//
//    hibits are  0, 1,  2.  3,   4,    5
//    weights are 1, 4, 16, 64, 256, 1024

static const int super_markov_weight[32] = // Jval
{
    1,   // 0
    4,   // 1
    4,   // 2
    16,  // 3
    4,   // 4
    16,  // 5
    16,  // 6
    64,  // 7
    4,   // 8
    16,  // 9
    16,  // 10  - A
    64,  // 11  - B
    16,  // 12  - C
    64,  // 13  - D
    64,  // 14  - E
    256, // 15 -  F
    4,   // 16 - 10
    16,  // 17 - 11
    16,  // 18 - 12
    64,  // 19 - 13
    16,  // 20 - 14
    64,  // 21 - 15
    64,  // 22 - 16
    256, // 23 - 17
    16,  // 24 - 18
    64,  // 25 - 19
    64,  // 26 - 1A
    256, // 27 - 1B
    64,  // 28 - 1C
    256, // 29 - 1D
    256, // 30 - 1E
    1024 // 31 - 1F
};


//
//   This uses the Breyer-Chhabra-Siefkes model that
//   uses coefficients of 1, 3, 13,, 75, and 541, which
//   assures complete override for any shorter string in
//   a single occurrence.
//

static const int breyer_chhabra_siefkes_weight[16] = // Jval
{
    1,  // 0
    3,  // 1
    3,  // 2
    13, // 3
    3,  // 4
    13, // 5
    13, // 6
    75, // 7
    3,  // 8
    13, // 9
    13, // 10
    75, // 11
    13, // 12
    75, // 13
    75, // 14
    541
};        // 15


//
//   This uses the Breyer-Chhabra-Siefkes model that
//   uses coefficients of 1, 3, 13,, 75, and 541, which
//   assures complete override for any shorter string in
//   a single occurrence.
//

static const int breyer_chhabra_siefkes_mws_weight[] = // Jval
{
    1,   // 0
    3,   // 1
    3,   // 2
    13,  // 3
    3,   // 4
    13,  // 5
    13,  // 6
    75,  // 7
    3,   // 8
    13,  // 9
    13,  // 10  - A
    75,  // 11  - B
    13,  // 12  - C
    75,  // 13  - D
    75,  // 14  - E
    541, // 15 -  F
    3,   // 16 - 10
    13,  // 17 - 11
    13,  // 18 - 12
    75,  // 19 - 13
    13,  // 20 - 14
    75,  // 21 - 15
    75,  // 22 - 13
    541, // 23 - 17
    13,  // 24 - 18
    75,  // 25 - 19
    75,  // 26 - 1A
    541, // 27 - 1B
    75,  // 28 - 1C
    541, // 29 - 1D
    541, // 30 - 1E
    4683 // 31 - 1F
};


//
//   This uses the Breyer-Chhabra-Siefkes model that
//   uses coefficients of 1, 3, 13,, 75, and 541, which
//   assures complete override for any shorter string in
//   a single occurrence.
//

static const int breyer_chhabra_siefkes_exp_weight[] = // Jval
{
    1,    // 0
    8,    // 1
    8,    // 2
    64,   // 3
    8,    // 4
    64,   // 5
    64,   // 6
    512,  // 7
    8,    // 8
    64,   // 9
    64,   // 10  - A
    512,  // 11  - B
    64,   // 12  - C
    512,  // 13  - D
    512,  // 14  - E
    4096, // 15 -  F
    8,    // 16 - 10
    64,   // 17 - 11
    64,   // 18 - 12
    512,  // 19 - 13
    64,   // 20 - 14
    512,  // 21 - 15
    512,  // 22 - 13
    4096, // 23 - 17
    64,   // 24 - 18
    512,  // 25 - 19
    512,  // 26 - 1A
    4096, // 27 - 1B
    512,  // 28 - 1C
    4096, // 29 - 1D
    4096, // 30 - 1E
    32768 // 31 - 1F
};


//
//   This uses the Breyer-Chhabra-Siefkes base 7 model that
//   uses coefficients of 1, 7, 49, 343, 2401 which
//   assures complete override for any shorter string in
//   a single occurrence.
//

static const int breyer_chhabra_siekfes_base7_weight[16] = // Jval
{
    1,   // 0
    7,   // 1
    7,   // 2
    49,  // 3
    7,   // 4
    49,  // 5
    49,  // 6
    343, // 7
    7,   // 8
    49,  // 9
    343, // 10
    343, // 11
    49,  // 12
    343, // 13
    343, // 14
    2401
};         // 15



static const int osbf_feature_weight[] =
{
    3125, 256, 27, 4, 1
};





/*
 * return matrix cell value for the given element.
 */
typedef int contributing_token_func(int x, int y, int z, const int *table, size_t table_size);



/*
 * Markovian: first node + N-deep all bit patterns
 */
static int markovian_contributing_token(int x, int y, int z, const int *table, size_t table_size)
{
    if (z >= 2)
    {
        return 0;
    }
if (table_size < 32 && y > 1 << (table_size - 1) /* ~ power(2, table_size/2)) */ )
    {
   // '>' means we can include the single token monogram as a vector in the matrix!
        return 0;
    }

    // 0 => 0
    // 1 => 0, 1
    // 2 => 0, 2
    if (x > 0)
    {
   // markov chain creation
        int bit = 1 << (x - 1);

        if (!(y & bit))
        {
            return 0;				 
        }
    }

	x <<= 1;     // 2 * x
	x += z;
    if (x >= table_size)
    {
        return 0;
    }
    return table[x];
}

/*
 * GerH markovian.alt et al: no single, just N-deep all bit patterns
 */
static int markov_alt_contributing_token(int x, int y, int z, const int *table, size_t table_size)
{
    if (z >= 2)
    {
        return 0;
    }
if (table_size < 32 && y > 1 << (table_size - 1) /* ~ power(2, table_size/2)) */ )
    {
   // '>' means we can include the single token monogram as a vector in the matrix!
        return 0;
    }

	if (x > 0)
    {
        int bit = 1 << (x - 1);

        if (!((y + 1) & bit))
        {
            return 0;
        }
    }

	x <<= 1;     // 2 * x
	x += z;
    if (x >= table_size)
    {
        return 0;
    }
    return table[x];
}

/*
 * OSB et al: vanilla CRM114 only uses 1 + 2^N patterns
 */
static int osb_contributing_token(int x, int y, int z, const int *table, size_t table_size)
{
    if (z >= 2)
    {
        return 0;
    }
if (y >= table_size / 2)
    {
   // '>=' means we can include the single token monogram as a last vector in the matrix!
        return 0;
    }

        if (y + 1 != x && x != 0)
        {
  // sparse bigram creation:
            return 0;
        }

	x <<= 1;     // 2 * x
	x += z;
    if (x >= table_size)
    {
        return 0;
    }
    return table[x];
}


#define VECTOR_MODEL_REC_DEF(cb, x, y, z, id)		{ cb, x, y, z, hctable, WIDTHOF(hctable), id }
#define WEIGHT_MODEL_REC_DEF(table, id)		{ table, WIDTHOF(table), id }

static const struct vector_id_def
{
	contributing_token_func *cb;
	int x;
	int y;
	int z;
	const int *table;
	size_t table_size;
	char *identifier;
} vector_model_ids[] =
{
	VECTOR_MODEL_REC_DEF(osb_contributing_token, OSB_BAYES_WINDOW_LEN, 4, 2, "osb"),
	VECTOR_MODEL_REC_DEF(markovian_contributing_token, MARKOVIAN_WINDOW_LEN, 16, 2, "sbph"),
};

static const struct weight_id_def
{
	const int *weight_table;
	int weight_table_size;
	char *identifier;
} weight_model_ids[] =
{
	WEIGHT_MODEL_REC_DEF(flat_model_weight, "flat"),
	WEIGHT_MODEL_REC_DEF(osb_bayes_feature_weight, "osb"),
	WEIGHT_MODEL_REC_DEF(chi2_feature_weight, "chi2"),
	WEIGHT_MODEL_REC_DEF(entropic_weight, "entropic"),
	WEIGHT_MODEL_REC_DEF(hidden_markov_weight, "markov"),
	WEIGHT_MODEL_REC_DEF(super_markov_weight, "super_markov"),
	WEIGHT_MODEL_REC_DEF(breyer_chhabra_siefkes_weight, "bcs"),
	WEIGHT_MODEL_REC_DEF(breyer_chhabra_siefkes_mws_weight, "bcs_mws"),
	WEIGHT_MODEL_REC_DEF(breyer_chhabra_siefkes_exp_weight, "bcs_exp"),
	WEIGHT_MODEL_REC_DEF(breyer_chhabra_siekfes_base7_weight, "bcs_b7"),
	WEIGHT_MODEL_REC_DEF(osbf_feature_weight, "osbf"),
};


/*
 Return the number of tokens produced; return a negative number on error.


 Note regarding implementation:

 Be reminded that this callback CANNOT assume large tokenhash_dest
 buffers. As such, the code must mind those situations where the
 tokenhash_dest buffer is either so small to start with, or already
 filled to the brim while working in here, that some additional
 activity (more particularly: start and end padding) HAS to be
 spread across multiple invocations of this callback.

 In other words: the padding code CANNOT and MUST NOT assume that
 there's sufficient space left in tokenhash_dest[] to write the
 generated padding chunk all at at once.
*/
static int default_regex_VT_tokenizer_func(VT_USERDEF_TOKENIZER *obj,
        crmhash_t                                               *tokenhash_dest,
        int                                                      tokenhash_dest_size)
{
    const char *txt;
    int txt_len;
    int k = 0;
	int k4pad_store = 0;

    //   Run the hashpipe, either with regex, or without.
    //
    CRM_ASSERT(obj != NULL);
    CRM_ASSERT(tokenhash_dest != NULL);
    CRM_ASSERT(tokenhash_dest_size > 0);
    CRM_ASSERT(obj->input_text != NULL);

	// take care of start (leading) padding first!
	if (obj->pad_start && obj->padding_length > obj->padding_written_at_start)
	{
		int len = CRM_MIN(tokenhash_dest_size, obj->padding_length - obj->padding_written_at_start);

		for (k = 0; k < len; k++)
		{
			tokenhash_dest[k] = 0xDEADBEEF;
		}

		obj->padding_written_at_start += len;

		if (obj->padding_written_at_start == obj->padding_length)
		{
			// padding done: signal other code that we have padded so start store for end padding 
			// gets the 'right' stuff:
			k4pad_store = k;
		}
	}

	if (k < tokenhash_dest_size)
	{
		CRM_ASSERT(obj->pad_start ? obj->padding_written_at_start == obj->padding_length : TRUE);

    if (obj->input_next_offset >= obj->input_textlen)
    {
        if (internal_trace)
        {
            fprintf(stderr,
                    "WARNING: VT: exiting tokenizer @ %d -- obj->input_next_offset = %d,  obj->input_textlen = %d!\n",
                    __LINE__,
                    obj->input_next_offset,
                    obj->input_textlen);
        }
        /* return 0; */
    }
	else
	{
    txt = obj->input_text + obj->input_next_offset;
    txt_len = obj->input_textlen - obj->input_next_offset;
    if (txt_len <= 0)
    {
        obj->eos_reached = 1;
    }
    if (obj->eos_reached)
    {
        // you'll have no more tokens until you reset 'eos_reached'...
        if (internal_trace)
        {
            fprintf(stderr, "WARNING: VT: exiting tokenizer @ %d!\n", __LINE__);
        }
        /* return 0; */
    }
	else
	{
    //  If the pattern is empty, assume non-graph-delimited tokens
    //  (supposedly an 8% speed gain over regexec)
    if (obj->regexlen == 0)
    {
        int big_token_count = 0;

        obj->initial_setup_done = 1;

        for (/* k = 0 */; k < tokenhash_dest_size && txt_len > 0; k++)
        {
            int b;
            int e;
            int token_len;
            const char *token;

            //         skip non-graphical characters
            for (b = 0; b < txt_len && !crm_isgraph(txt[b]); b++)
                ;
            // [i_a] WARNING: fringe case where an input stream is terminated by whitespace:
            //       if we don't take care of that special occasion it will produce one extra
            //       'token' for a ZERO-length word. What's one token on several thousand, you
            //       may ask?
            //       It's the difference between an exact match for two documents, one terminated
            //       with extra whitespace, while the other isn't, or, if we don't fix this fringe
            //       case, only a _probable_ match - hopefully, but that depends on the number of
            //       matches so far (token count) and classifier.
            //       Since we look at documents as 'streams of tokens', non-token data should be
            //       discarded, not only at the start and middle of the documents, but also at the
            //       end!
            if (b == txt_len)
            {
                // Fringe case inside a fringe case: what if a document is ONLY whitespace, i.e.
                // ZERO tokens? DECISION: we DO NOT produce a 'faked' token for that one; zero stays
                // zero.
                obj->eos_reached = 1;
                break;
            }

            for (e = b; e < txt_len && crm_isgraph(txt[e]); e++)
                ;

            if (user_trace)
            {
                fprintf(stderr, "Token; k: %d re: --- T.O: %d len %d ( %d %d on >",
                        k,
                        (int)(txt - obj->input_text),
                        e - b,
                        b,
                        e);
                fwrite_ASCII_Cfied(stderr, txt + b, e - b);
                fprintf(stderr, "< )\n");
            }

            //
            //   Now stuff this new token into the front of the pipeline
            //   Since 'front' means 'higher index' we just drop it into slot [k].
            //

            token_len = e - b;
            token = &txt[b];

            if (obj->max_token_length <= 0)
            {
                // no 'big token globbing' enabled...
                tokenhash_dest[k] = strnhash(token, token_len);
            }
            else
            {
                // OSBF style big token clustering into a single feature for, for example, base64 encoded mail blocks.
                if (big_token_count == 0)
                {
                    tokenhash_dest[k] = strnhash(token, token_len);

                    if (token_len > obj->max_token_length)
                    {
                        big_token_count++;
                    }
                }
                else
                {
                    k--;                         // fix loop k++: stay at the previous feature!
                    CRM_ASSERT(k >= 0);

                    // [i_a] GROT GROT GROT
                    // original OSBF code (see crm_osbf_bayes.c) made tokens position INDEPENDENT inside the single feature; this should not be!
                    //tokenhash_dest[k] ^= strnhash(&txt[b], token_len);
                    //
                    // so instead we make the tokens position dependent by using a simple unified hash multiplier: 33
                    tokenhash_dest[k] *= 33;
                    tokenhash_dest[k] ^= strnhash(token, token_len);

                    if (token_len > obj->max_token_length && big_token_count < obj->max_big_token_count)
                    {
                        big_token_count++;

                        if (user_trace)
                        {
                            fprintf(stderr, "WARNING: continuation of 'big token' -- globbed %d tokens!\n", big_token_count);
                        }
                    }
                    else
                    {
                        if (user_trace)
                        {
                            fprintf(stderr, "WARNING: end of 'big token' detected @ %d globbed tokens!\n", big_token_count);
                        }

                        big_token_count = 0;
                        // okay, end of 'long token into single feature' run: allow loop to increment 'k' index now
                    }
                }
            }

            // and update our 'txt' pointer + length to scan to point at the next token in the input stream:
            txt += e;
            txt_len -= e;
        }
    }
    else
    {
        int big_token_count = 0;

        if (!obj->initial_setup_done)
        {
            int regcomp_status;

            if (!obj->regex_compiler_flags_are_set)
            {
                nonfatalerror("Sanity check failed while starting up the VT (Vector Tokenizer).",
                        "Why have the VT tokenizer regex flags not been set up? Developer asleep at the keyboard again?");
                if (internal_trace)
                {
                    fprintf(stderr, "WARNING: VT: exiting tokenizer @ %d!\n", __LINE__);
                }
                return -1;
            }

            regcomp_status = crm_regcomp(&obj->regcb, obj->regex, obj->regexlen, obj->regex_compiler_flags);
            if (regcomp_status != 0)
            {
                char errortext[4096];

                crm_regerror(regcomp_status, &obj->regcb, errortext, WIDTHOF(errortext));
                nonfatalerror("Regular Expression Compilation Problem for VT (Vector Tokenizer): ",
                        errortext);
                if (internal_trace)
                {
                    fprintf(stderr, "WARNING: VT: exiting tokenizer @ %d!\n", __LINE__);
                }
                return -1;
            }
            obj->initial_setup_done = 1;
        }

        for (/* k = 0 */; k < tokenhash_dest_size && txt_len > 0; k++)
        {
            int status;
            int token_len;
            const char *token;

            status = crm_regexec(&obj->regcb, txt, txt_len, WIDTHOF(obj->match), obj->match, obj->regex_compiler_flags, NULL);
            if (status != REG_OK)
            {
                if (status == REG_NOMATCH)
                {
                    // [i_a] WARNING: fringe case where an input stream is terminated by whitespace:
                    //       if we don't take care of that special occasion it will produce one extra
                    //       'token' for a ZERO-length word. What's one token on several thousand, you
                    //       may ask?
                    //       It's the difference between an exact match for two documents, one terminated
                    //       with extra whitespace, while the other isn't, or, if we don't fix this fringe
                    //       case, only a _probable_ match - hopefully, but that depends on the number of
                    //       matches so far (token count) and classifier.
                    //       Since we look at documents as 'streams of tokens', non-token data should be
                    //       discarded, not only at the start and middle of the documents, but also at the
                    //       end!
#if 0
                    if (k == 0)
                    {
                        // Fringe case inside a fringe case: what if a document is ONLY whitespace, i.e.
                        // ZERO tokens? DECISION: we DO NOT produce a 'faked' token for that one; zero stays
                        // zero.
                    }
#endif
                    obj->eos_reached = 1;
                }
                break;
            }

            //   Not done,we have another token (the text in text[match[0].rm_so,
            //    of length match[0].rm_eo - match[0].rm_so size)
            //
            if (user_trace)
            {
                fprintf(stderr, "Token; k: %d re: %d T.O: %d len %d ( %d %d on >",
                        k,
                        status,
                        (int)(txt - obj->input_text),
                        (int)(obj->match[0].rm_eo - obj->match[0].rm_so),
                        (int)obj->match[0].rm_so,
                        (int)obj->match[0].rm_eo);
                fwrite_ASCII_Cfied(stderr, &txt[obj->match[0].rm_so],
                        (obj->match[0].rm_eo - obj->match[0].rm_so));
                fprintf(stderr, "< )\n");
            }

            //
            //   Now stuff this new token into the front of the pipeline
            //   Since 'front' means 'higher index' we just drop it into slot [k].
            //

            token_len = obj->match[0].rm_eo - obj->match[0].rm_so;
            token = &txt[obj->match[0].rm_so];

            if (obj->max_token_length <= 0)
            {
                // no 'big token globbing' enabled...
                tokenhash_dest[k] = strnhash(token, token_len);
            }
            else
            {
                // OSBF style big token clustering into a single feature for, for example, base64 encoded mail blocks.
                if (big_token_count == 0)
                {
                    tokenhash_dest[k] = strnhash(token, token_len);

                    if (token_len > obj->max_token_length)
                    {
                        if (user_trace)
                        {
                            fprintf(stderr, "WARNING: start of 'big token' detected (min. length = %d) @ token len = %d -- globbing tokens!\n",
                                    obj->max_token_length,
                                    token_len);
                        }

                        big_token_count++;
                    }
                }
                else
                {
                    k--;                         // fix loop k++: stay at the previous feature!
                    CRM_ASSERT(k >= 0);

                    // [i_a] GROT GROT GROT
                    // original OSBF code (see crm_osbf_bayes.c) made tokens position INDEPENDENT inside the single feature; this should not be!
                    //tokenhash_dest[k] ^= strnhash(&txt[b], token_len);
                    //
                    // so instead we make the tokens position dependent by using a simple unified hash multiplier: 33
                    tokenhash_dest[k] *= 33;
                    tokenhash_dest[k] ^= strnhash(token, token_len);

                    if (token_len > obj->max_token_length && big_token_count < obj->max_big_token_count)
                    {
                        big_token_count++;
                    }
                    else
                    {
                        big_token_count = 0;
                        // okay, end of 'long token into single feature' run: allow loop to increment 'k' index now
                    }
                }
            }

            // and update our 'txt' pointer + length to scan to point at the next token in the input stream:
            txt += obj->match[0].rm_eo;
            txt_len -= obj->match[0].rm_eo;
        }
    }

    obj->input_next_offset = (int)(txt - obj->input_text);

#if 0
    // extra check if we tokenized the completed input yet:
    if (txt_len <= 0)
    {
        obj->eos_reached = 1;
    }
#endif
	}
	}
	}


	if (obj->pad_end_with_first_chunk)
	{
	// collect start tokens for end padding?
		if (obj->padding_in_store < obj->padding_length && k4pad_store < k)
	{
		int pos;

		for (pos = obj->padding_in_store; pos < obj->padding_length && k4pad_store < k; )
		{
			obj->padding_store[pos++] = tokenhash_dest[k4pad_store++];
		}

		obj->padding_in_store = pos;
	}

	// take care of end padding now:
	if (k < tokenhash_dest_size && obj->eos_reached && obj->padding_written_at_end < obj->padding_length)
	{
		int pos;

	/*
	Fringe case: you may encounter input which produces ZERO(0) hashes by itself: 
	such input should ALSO be end-padded but there's no input hashes to repeat!
	So we pad such a bugger with deadbeef: after all, empty inputs should be 
	recognizable as well, and the only way to recognize an input message is through
	the hashes it produces...
	*/
		if (obj->padding_in_store == 0)
		{
		for (pos = obj->padding_written_at_end; pos < obj->padding_length && k < tokenhash_dest_size; pos++)
		{
			tokenhash_dest[k++] = 0xDEADBEEF;
		}
		}
		else
		{
		CRM_ASSERT(obj->padding_store);
		CRM_ASSERT(obj->padding_in_store > 0);

		for (pos = obj->padding_written_at_end; pos < obj->padding_length && k < tokenhash_dest_size; pos++)
		{
			/*
			Recycle the padding store multiple times for very tiny input token series:
			when the input produced less than padding_length tokens itself, the store
			will contain those few only, so, in order to pad to length = padding_length,
			we must re-use the input tokens again and again:
			*/
			tokenhash_dest[k++] = obj->padding_store[pos % obj->padding_in_store];
		}
		}

		obj->padding_written_at_end = pos;
	}
	}

    if (internal_trace)
    {
        fprintf(stderr, "WARNING: VT: exiting tokenizer @ %d!\n", __LINE__);
    }
    return k;
}



static int default_regex_VT_tokenizer_cleanup_func(VT_USERDEF_TOKENIZER *obj)
{
    if (obj->regex_malloced)
    {
        free(obj->regex);
        obj->regex = NULL;
    }
    crm_regfree(&obj->regcb);
    memset(&obj->regcb, 0, sizeof(obj->regcb));

	if (obj->padding_store_malloced)
	{
		free(obj->padding_store);
		obj->padding_store = NULL;
	}

    obj->regex_malloced = 0;
    obj->eos_reached = 0;
    obj->initial_setup_done = 0;
    obj->input_next_offset = 0;
    obj->input_text = 0;
    obj->input_textlen = 0;
    memset(obj->match, 0, sizeof(obj->match));
    obj->max_big_token_count = 0;
    obj->max_token_length = 0;
    obj->regex_compiler_flags = 0;
    obj->regex_compiler_flags_are_set = 0;
    obj->regexlen = 0;
    obj->tokenizer = 0;
	obj->not_at_sos = 0;
	obj->padding_in_store = 0;
	obj->padding_settings_are_set = 0;
	obj->padding_store_malloced = 0;
	obj->padding_written_at_end = 0;
	obj->padding_written_at_start = 0;

    obj->cleanup = 0;

    return 0;
}


static int default_VT_coeff_matrix_cleanup_func(struct magical_VT_userdef_coeff_matrix *obj)
{
    memset(obj, 0, sizeof(*obj));

    obj->cleanup = 0;

    return 0;
}





/*
 *      Set up the data in the 'tokenizer' struct:
 *
 *      the 'default' tokenizer expects a regex (which may be NULL: then
 *      the default '[[:graph:]]+' regex will be used instead).
 *
 *      Alternatively, a custom 'tokenizer_function' can be specified, which
 *      may or may not use the specified 'regex'.
 *
 *      The benefit of this approach is that now we can use non-regex-based
 *      tokenizer functions, such as the one-bit tokenizer used in the
 *      Bit Entropy classifier, within the Vector Tokenizer.
 *
 *      This again enables us to mix the abilities of the Vector Tokenizer
 *      with any fathomable tokenizer, not just the ones which parse regexes!
 *
 *      The default 'tokenizer_function' is a regex-parsing tokenizer using
 *      the specified regex (or the default if that parameter is NULL).
 *
 *      Return 0 on success, negative return values on error.
 */
int config_vt_tokenizer(VT_USERDEF_TOKENIZER *tokenizer,
        const ARGPARSE_BLOCK                 *apb,  // The args for this line of code
        VHT_CELL                            **vht,
        CSL_CELL                             *tdw,
        const VT_USERDEF_TOKENIZER           *default_tokenizer)
{
    if (!tokenizer)
    {
        nonfatalerror("Sanity check failed while starting up the VT (Vector Tokenizer).",
                "Where's the VT tokenizer gone? There's work to do here!");
        return -1;
    }
#if 10
    if (!apb)
    {
        nonfatalerror("Sanity check failed while starting up the VT (Vector Tokenizer).",
                "Where's the APB (Application Program Block) gone? Developer asleep at the keyboard again?");
        return -1;
    }
#endif

    // set up a default tokenizer if 'they' didn't specify one yet.
    if (!tokenizer->tokenizer)
    {
        tokenizer->tokenizer = &default_regex_VT_tokenizer_func;
        tokenizer->cleanup = &default_regex_VT_tokenizer_cleanup_func;
    }
    if (!tokenizer->cleanup)
    {
        // if 'they' specified a custom tokenizer but didn't spec a cleanupper...
        tokenizer->cleanup = &default_regex_VT_tokenizer_cleanup_func;
    }
    // apply the OSBF 'big token globbing into single feature hash' property to
    // ALL OF 'EM, regardless of classifier, as I expect other classifiers to benefit
    // from this little heuristic as well:
    if (tokenizer->max_token_length == -1)
    {
        // if user didn't want us to do this, s/he'd set max_token_length to 0...
        tokenizer->max_token_length = OSBF_MAX_TOKEN_SIZE;
    }
    if (tokenizer->max_big_token_count <= 0)
    {
        tokenizer->max_big_token_count = OSBF_MAX_LONG_TOKENS;
    }


    if (tokenizer->padding_length <= 0)
    {
        tokenizer->padding_length = default_tokenizer->padding_length;
    }
    CRM_ASSERT(default_tokenizer->padding_settings_are_set != 0);
    if (!tokenizer->padding_settings_are_set)
    {
        tokenizer->pad_start = default_tokenizer->pad_start;
        tokenizer->pad_end_with_first_chunk = default_tokenizer->pad_end_with_first_chunk;

        tokenizer->padding_settings_are_set = TRUE;
    }
    if (!tokenizer->padding_store && tokenizer->padding_length)
    {
        tokenizer->padding_store = malloc(tokenizer->padding_length * sizeof(tokenizer->padding_store[0]));
        if (!tokenizer->padding_store)
        {
            untrappableerror("Cannot allocate VT tokenizer padding buffer memory", "Stick a fork in us; we're _done_.");
        }
        tokenizer->padding_store_malloced = TRUE;
    }
    tokenizer->padding_written_at_end = 0;
    tokenizer->padding_written_at_start = 0;
    tokenizer->padding_in_store = 0;


    CRM_ASSERT(default_tokenizer->regex_compiler_flags_are_set != 0);
    if (!tokenizer->regex_compiler_flags_are_set)
    {
        if (!apb)
        {
            tokenizer->regex_compiler_flags = default_tokenizer->regex_compiler_flags;
            tokenizer->regex_compiler_flags_are_set = 1;
        }
        else
        {
            int cflags = REG_EXTENDED;

            if (apb->sflags & CRM_NOCASE)
            {
                cflags |= REG_ICASE;
                if (user_trace)
                {
                    fprintf(stderr, "turning on case-insensitive match\n");
                }
            }
            if (apb->sflags & CRM_BASIC)
            {
                cflags &= ~REG_EXTENDED;
                if (user_trace)
                {
                    fprintf(stderr, "  basic regex match turned on...\n");
                }
            }
            if (apb->sflags & CRM_NOMULTILINE)
            {
                cflags |= REG_NEWLINE;
                if (user_trace)
                {
                    fprintf(stderr, "  nomultiline turned on...\n");
                }
            }
            if (apb->sflags & CRM_LITERAL)
            {
                cflags |= REG_LITERAL;
                if (user_trace)
                {
                    fprintf(stderr, "  literal pattern search turned on...\n");
                }
            }
            tokenizer->regex_compiler_flags = cflags;
            tokenizer->regex_compiler_flags_are_set = 1;
        }
    }
    CRM_ASSERT(tokenizer->regex_compiler_flags_are_set);

    // Now all of the defaults have been filled in; we now see if the
    // caller has overridden any (or all!) of them.   We assume that the
    // user who overrides them has pre-sanity-checked them as well.

    // First check- did the user override the regex?

    // Did the user program specify a first slash parameter?  (only
    // override this if a regex was passed in)
    if (!tokenizer->regex)
    {
        if (apb)
        {
            int s1len;
            char s1text[MAX_PATTERN];
            char *dst;

            s1len = crm_get_pgm_arg(s1text, MAX_PATTERN, apb->s1start, apb->s1len);
            s1len = crm_nexpandvar(s1text, s1len, MAX_PATTERN, vht, tdw);

            if (s1len > 0)
            {
                tokenizer->regex = dst = calloc(s1len + 1, sizeof(tokenizer->regex[0]));
                if (!dst)
                {
                    untrappableerror("Cannot allocate VT memory", "Stick a fork in us; we're _done_.");
                }
                memcpy(dst, s1text, s1len * sizeof(tokenizer->regex[0]));
                dst[s1len] = 0;
                tokenizer->regexlen = s1len;
                tokenizer->regex_malloced = 1;
            }
        }

        // only take over default regex if there actually is one... (and we didn't find any in the 1st slash arg above)
        if (!tokenizer->regex && default_tokenizer->regex)
        {
            char *dst;

            tokenizer->regex = dst = calloc(default_tokenizer->regexlen + 1, sizeof(tokenizer->regex[0]));
            if (!dst)
            {
                untrappableerror("Cannot allocate VT memory", "Stick a fork in us; we're _done_.");
            }
            memcpy(dst, default_tokenizer->regex, default_tokenizer->regexlen * sizeof(tokenizer->regex[0]));
            dst[default_tokenizer->regexlen] = 0;
            tokenizer->regexlen = default_tokenizer->regexlen;
            tokenizer->regex_malloced = 1;
        }

        //    Compiling the regex is delayed: it will be compiled at first use.
        //  If you don't want that, you may compile the regex by calling the tokenizer
        //  with a ZERO-length output buffer here or anywhere.

#if 0
        if (s1len)
        {
            int regcomp_status = crm_regcomp(&tokenizer->regcb, regex, regexlen, tokenizer->regex_compiler_flags);
            if (regcomp_status != 0)
            {
                char errortext[4096];

                crm_regerror(regcomp_status, &regcb, errortext, WIDTHOF(errortext));
                nonfatalerror("Regular Expression Compilation Problem for VT (Vector Tokenizer): ",
                        errortext);
                return -1;
            }
        }
#endif
    }

    tokenizer->eos_reached = 0;
    tokenizer->not_at_sos = 0;

    return 0;
}







int transfer_matrix_to_VT(VT_USERDEF_COEFF_MATRIX *dst,
						const int *src, 
						size_t src_x, size_t src_y, size_t src_z)
{
	if (!dst || !src || !src_x || src_y || !src_z)
	{
		return -1;
	}
	if (src_x > UNIFIED_WINDOW_LEN
		|| src_y > UNIFIED_VECTOR_LIMIT
		|| src_z > UNIFIED_VECTOR_STRIDE)
	{
		// out of range: source matrix dimension too large!
		return -2;
	}

	dst->cm.output_stride = src_z;
	dst->cm.pipe_iters = src_y;
	dst->cm.pipe_len = src_x;

	while (src_z-- > 0)
	{
		int y;

		for (y = 0; y < src_y; y++)
		{
			int x;

			for (x = 0; x < src_x; x++)
			{
				int src_idx = src_z * src_y * src_x + y * src_x + x;
				int dst_idx = src_z * UNIFIED_VECTOR_LIMIT * UNIFIED_WINDOW_LEN + y * UNIFIED_WINDOW_LEN + x;

				dst->cm.coeff_array[dst_idx] = src[src_idx];
			}
		}
	}

	 return 0;
}



int generate_matrix_for_model(VT_USERDEF_COEFF_MATRIX *matrix, contributing_token_func *cb, const int *table, size_t table_size)
{
        int *ca = matrix->cm.coeff_array;
        int x;
        int y;
        int z;

	// construct the 3D VT matrix:
        for (z = 0; z < UNIFIED_VECTOR_STRIDE; z++)
        {
            for (y = 0; y < UNIFIED_VECTOR_LIMIT; y++)
            {
                int m = (*cb)(0, y, z, table, table_size);
                int *row = &ca[UNIFIED_WINDOW_LEN * (y + UNIFIED_VECTOR_LIMIT * z)];

                /* no useful row of the 2D matrix? == nothing more to generate for this section */
                if (m == 0)
                {
                    // make sure 'pipe_iters' isn't too large:
#if 0
					CRM_ASSERT(z == 0 ? pipe_iters <= y : TRUE);
#endif
					break;
                }

				row[0] = m;

                for (x = 1; x < UNIFIED_WINDOW_LEN; x++)
                {
                    m = (*cb)(x, y, z, table, table_size);
                    row[x] = m;
                }
            }
        }

	return 0;
}


/*
   Return TRUE when the given VT matrix would permit the use of Arne's optimization.

   Return FALSE otherwise.


   Note: Arne's optimization is based on the assumption that, when no hit is found for the
   feature hash proeduced through the first row of the VT matrix, the feature hashes of the
   subsequent rows of the same matrix won't produce a hit either.

   This means that all subsequent rows must AT LEAST include ALL tokens selected
   by the first row.


   Note #2: here we check for each 2D matrix (each 'stride') individually. Arne is only
   assumed possible when ALL 2D matrices (each stride's matrix) permits Arne's optimization.
*/

static int check_if_arne_optimization_is_possible(const VT_USERDEF_COEFF_MATRIX *matrix)
{
    int x;
    int y;
    int z;

    CRM_ASSERT(matrix);

	// only need to check the non-zero columns of the first row: all other rows must have these as well:
	for (z = matrix->cm.output_stride; z-- >= 0; )
     {
		 for (x = matrix->cm.pipe_len; x-- >= 0; )
          {
			  const int *col = &matrix->cm.coeff_array[z * UNIFIED_VECTOR_LIMIT * UNIFIED_WINDOW_LEN + x];
			  if (col[0] == 0)
				  continue;

			  // check if this column is active (non-zero) for all *other* rows as well:
			  for (y = matrix->cm.pipe_iters; y-- >= 1; )
          {
			  if (col[y * UNIFIED_WINDOW_LEN] == 0)
				  return FALSE;
			  }
		 }
	}

	return TRUE;
}





typedef int fprintf_equivalent_func (void *propagate, const char *msg, ...)
__attribute__((__format__(__printf__, 2, 3)));


static void print_matrices(const VT_USERDEF_COEFF_MATRIX *our_coeff,
        fprintf_equivalent_func *cb, void *propagate)
{
    int x;
    int y;
    int z;
    int x_max = 0;
    int y_max = 0;
    int z_max = 0;
    int fw_x_max = 0;
    int fw_y_max = 0;

    discover_matrices_max_dimensions(our_coeff, &x_max, &y_max, &z_max, &fw_x_max, &fw_y_max);

    CRM_ASSERT(cb != 0);
    for (z = 0; z < z_max; z++)
    {
        (*cb)(propagate, "\n========== coeff matrix[%d/%d] = %d x %d ==========",
              z + 1,
              z_max,
              x_max,
              y_max);
        for (y = 0; y < y_max;)
        {
            (*cb)(propagate, "\nrow[%3d]:", y);

            /* dump row: */
            for (x = 0; x < x_max;)
            {
                (*cb)(propagate, " %6d", our_coeff->cm.coeff_array[x + UNIFIED_WINDOW_LEN * (y + UNIFIED_VECTOR_LIMIT * z)]);
                x++;
                if (our_coeff->cm.pipe_len == x)
                {
					(*cb)(propagate, (our_coeff->cm.pipe_iters > y ? " :" : "  "));
                }
            }

            y++;
            if (our_coeff->cm.pipe_iters == y)
            {
                (*cb)(propagate, "\n         ");
                /* edge detect: */
                for (x = 0; ; x++)
                {
                    if (our_coeff->cm.pipe_len > x)
                    {
                        (*cb)(propagate, ".......");
                    }
                    else
                    {
                        (*cb)(propagate, ".'");
                        break;
                    }
                }
            }
        }
    }

    /* now dump the weight 2D matrix as well: */
    (*cb)(propagate, "\n========== feature weight matrix = %d x %d ==========",
          fw_x_max,
          fw_y_max);
    for (y = 0; y < fw_y_max;)
    {
        (*cb)(propagate, "\nrow[%3d]:", y);

        /* dump row: */
        for (x = 0; x < fw_x_max;)
        {
            (*cb)(propagate, " %6d", our_coeff->fw.feature_weight[x + UNIFIED_VECTOR_LIMIT * y]);
            x++;
            if (our_coeff->fw.column_count == x)
            {
				(*cb)(propagate, (our_coeff->fw.row_count > y ? " :" : "  "));
            }
        }

        y++;
        if (our_coeff->fw.row_count == y)
        {
            (*cb)(propagate, "\n         ");
            /* edge detect: */
            for (x = 0; ; x++)
            {
                if (our_coeff->fw.column_count > x)
                {
                    (*cb)(propagate, ".......");
                }
                else
                {
                    (*cb)(propagate, ".'");
                    break;
                }
            }
        }
    }

    (*cb)(propagate, "\n");
}





static int fetch_value(int *value_ref, const char **src_ref, int *srclen_ref, int lower_limit, int upper_limit, const char *description)
{
    int i;
    const char *src = *src_ref;
    char *conv_ptr;
    int srclen = *srclen_ref;
    char valbuf[80];
    long value;
    int toklen;

    // skip leading whitespace INCLUDING \n, \r, etc. by skipping any NONPRINTABLE character:
    for (i = 0; i < srclen && !crm_isgraph(src[i]); i++)
        ;
    src += i;
    srclen -= i;
    for (i = 0; i < srclen && crm_isgraph(src[i]); i++)
        ;
    toklen = CRM_MIN(i, WIDTHOF(valbuf) - 1);
    if (toklen > 0)
    {
        memcpy(valbuf, src, toklen);
        valbuf[toklen] = 0;

        conv_ptr = valbuf;
#ifndef HAVE__SET_ERRNO
        errno = 0;
#else
        _set_errno(0);                         // Win32/MSVC
#endif
        value = strtol(valbuf, &conv_ptr, 0);
        if (errno != 0 || *conv_ptr)
        {
            nonfatalerror_ex(SRC_LOC(), "You've specified a %s "
                                        "that cannot be completely decoded [break at offset %d]: "
                                        "'%s' is not within the limited range %d..%d.",
                    description,
                    (int)(conv_ptr - valbuf), valbuf,
                    lower_limit, upper_limit);
            return -1;
        }
        if (value < lower_limit || value > upper_limit)
        {
            nonfatalerror_ex(SRC_LOC(), "You've specified a %s "
                                        "that is out of range: '%s' --> %ld, for limited range %d..%d.",
                    description,
                    valbuf, value,
                    lower_limit, upper_limit);
            return -1;
        }
        if (internal_trace)
        {
            fprintf(stderr, "%s = %ld\n", description, value);
        }

        *value_ref = (int)value;
        *src_ref = src + i;
        *srclen_ref = srclen - i;

        return 0;
    }
    else
    {
        // only trailing whitespace discovered. Bugger!
        *value_ref = 0;
        *src_ref = src + i;
        *srclen_ref = srclen - i;

        nonfatalerror_ex(SRC_LOC(), "We'd expected to see another value specified for the %s "
                                    "but alas, the script apparently didn't deliver enough for that. "
                                    "Next time, make sure you've got a full custom vector!",
                description);
        return -1;
    }
}




//
// Return coeff matrix described in 'src' in 'coeff_matrix'.
//
// coeff_matrix:pipe_len, :pipe_iters and :stride are the size parameters
// returned for the 3-dimensional matrix.
//
// When sanity/maximum sizes are surpassed, the function will produce an error message
// as a nonfatalerror() and return -2.
//
// When other errors occur, such as failure to decode a matrix value element, this function
// will write an error message using nonfatalerror() and return -1.
//
// When successful, it will return 0.
//
int decode_userdefd_vt_coeff_matrix(VT_USERDEF_COEFF_MATRIX *coeff_matrix,  // the pipeline coefficient control array, etc.
        const char *src, int srclen, int mode)
{
    CRM_ASSERT(mode == USERDEF_COEFF_DECODE_WEIGHT_MODE || mode == USERDEF_COEFF_DECODE_COEFF_MODE);
    if (src && *src)
    {
        int i;
        int j;
        int k;
        int *ca;

        for (i = 0; i < srclen && !crm_isgraph(src[i]); i++)
            ;

        // only decode it when there's something else next to only whitepace...
        if (i < srclen)
        {
            int pipe_len = coeff_matrix->cm.pipe_len;
            int pipe_iters = coeff_matrix->cm.pipe_iters;
            int output_stride = coeff_matrix->cm.output_stride;
            const char *type_msg;

            if (mode == USERDEF_COEFF_DECODE_COEFF_MODE)
            {
                //   The first parameter is the pipe length
                if (fetch_value(&pipe_len, &src, &srclen, 0, UNIFIED_WINDOW_LEN, "tokenizer pipe length"))
                {
                    return -1;
                }
            }
            if (coeff_matrix->cm.pipe_len != 0 && pipe_len != coeff_matrix->cm.pipe_len)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified inconsistent feature weight and VT coefficient matrix %s "
                                            "values: %d <> %d (where they should be identical!).",
                        "pipeline length",
                        pipe_len, coeff_matrix->cm.pipe_len);
            }
            coeff_matrix->cm.pipe_len = pipe_len;

            //   The second parameter is the number of repeats
            if (fetch_value(&pipe_iters, &src, &srclen, 0, UNIFIED_VECTOR_LIMIT, "tokenizer iteration count"))
            {
                return -1;
            }
            if (coeff_matrix->cm.pipe_iters != 0 && pipe_iters != coeff_matrix->cm.pipe_iters)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified inconsistent feature weight and VT coefficient matrix %s "
                                            "values: %d <> %d (where they should be identical!).",
                        "iteration counts",
                        pipe_iters, coeff_matrix->cm.pipe_iters);
            }
            coeff_matrix->cm.pipe_iters = pipe_iters;

            //   The third parameter is the number of coefficient matrices, i.e. one for each step of a full 'stride':
            if (fetch_value(&output_stride, &src, &srclen, 0, UNIFIED_VECTOR_STRIDE, "tokenizer matrix count"))
            {
                return -1;
            }
            if (coeff_matrix->cm.output_stride != 0 && output_stride != coeff_matrix->cm.output_stride)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified inconsistent feature weight and VT coefficient matrix %s "
                                            "values: %d <> %d (where they should be identical!).",
                        "stride counts",
                        output_stride, coeff_matrix->cm.output_stride);
            }
            coeff_matrix->cm.output_stride = output_stride;

            //    Now, get the coefficients.
            if (mode == USERDEF_COEFF_DECODE_COEFF_MODE)
            {
                ca = coeff_matrix->cm.coeff_array;
                type_msg = "VT coefficient matrix value";
            }
            else
            {
                ca = coeff_matrix->fw.feature_weight;
                type_msg = "VT feature weight matrix value";
                pipe_len = 1;
            }

            for (k = 0; k < output_stride; k++)
            {
                for (j = 0; j < pipe_iters; j++)
                {
                    for (i = 0; i < pipe_len; i++)
                    {
                        int idx = i + UNIFIED_WINDOW_LEN * (j + UNIFIED_VECTOR_LIMIT * k);

                        if (fetch_value(&ca[idx], &src, &srclen, INT_MIN, INT_MAX, type_msg))
                        {
                            return -1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}






int get_vt_vector_from_2nd_slasharg(VT_USERDEF_COEFF_MATRIX *coeff_matrix,
        const ARGPARSE_BLOCK                                *apb,  // The args for this line of code
        VHT_CELL                                           **vht,
        CSL_CELL                                            *tdw)
{
    char s2text[MAX_PATTERN];
    int s2len;
    int regex_status;
    regmatch_t match[2];     //  We'll only care about the second match

    //     get the second slash parameter (if used at all)
    s2len = crm_get_pgm_arg(s2text, MAX_PATTERN, apb->s2start, apb->s2len);
    s2len = crm_nexpandvar(s2text, s2len, MAX_PATTERN, vht, tdw);

    if (s2len > 0)
    {
        regex_t regcb;
        // static const char *vt_weight_regex = "vector: ([ 0-9]*)";
        static const char *vt_weight_regex = "vector:([^,;]+)";
        static const char *vt_feature_weight_regex = "weight:([^,;]+)";
		static const char *vt_matrix_clip_regex = "vector-clip:([^,;]+)"; 
		static const char *vt_weight_clip_regex = "weight-clip:([^,;]+)"; 
		static const char *vt_weight_model_regex = "weight-model:([^,;]+)"; 
		static const char *vt_vector_model_regex = "vector-model:([^,;]+)"; 

        //   Compile up the regex to find the desired VT model
        regex_status = crm_regcomp(&regcb, vt_vector_model_regex, (int)strlen(vt_vector_model_regex), REG_ICASE | REG_EXTENDED);
        if (regex_status != 0)
        {
            char errmsg[1024];

            crm_regerror(regex_status, &regcb, errmsg, WIDTHOF(errmsg));
            nonfatalerror_ex(SRC_LOC(), "Custom VT Coefficient Matrix: "
                                        "Regular Expression Compilation Problem in VT (Vector Tokenizer) pattern '%s': %s",
                    vt_vector_model_regex,
                    errmsg);
            return -1;
        }

        // Use the regex to find the feature weight model identifier
        regex_status = crm_regexec(&regcb, s2text, s2len, WIDTHOF(match), match, REG_ICASE | REG_EXTENDED, NULL);

        //   Did we actually get a match for the extended parameters?
        if (regex_status == 0 && match[1].rm_so >= 0)
        {
            // Yes, it matched. Take the string, remove leading and trailing whitespace and match
			// case-INsensitively to our list of known weight models:
			char *id = &s2text[match[1].rm_so];
			int len = match[1].rm_eo - match[1].rm_so;
			int start = 0;
			
			if (!crm_nextword(id, len, start, &start, &len))
			{
            nonfatalerror_ex(SRC_LOC(), "Custom VT Coefficient Model: "
				"You have specified an empty VT matrix model identifier with your 'vector-model:' attribute for the VT (Vector Tokenizer). Assuming the default model...");

			// don't change anything weight matrix related!
			}
			else
			{
				int i;
				int model = -1;
			
				id += start;

				for (i = 0; i < WIDTHOF(vector_model_ids); i++)
				{
					if (!strncasecmp(id, vector_model_ids[i].identifier, CRM_MAX(strlen(vector_model_ids[i].identifier), len)))
					{
						// match!
						model = i;
						break;
					}
				}

				if (model < 0)
				{
            fatalerror_ex(SRC_LOC(), "Custom VT Coefficient Model: "
				"You have specified an unidentified VT matrix model "
				"identifier '%*.*s' with your 'vector-model:' "
				"attribute for the VT (Vector Tokenizer). "
				"Assuming the default model...",
				len, len, id);
			return -1;
				}
				else
				{
					// copy model:
					if (generate_matrix_for_model(coeff_matrix, vector_model_ids[model].cb, vector_model_ids[model].table, vector_model_ids[model].table_size))
    {
        fatalerror("Failed to generate the VT matrix from the specified model.",
                "This kind of disaster shouldn't befall you.");
        return -1;
    }
	if (coeff_matrix->cm.pipe_len == 0)
	{
		coeff_matrix->cm.pipe_len = vector_model_ids[model].x;
	}
	if (coeff_matrix->cm.pipe_iters == 0)
	{
		coeff_matrix->cm.pipe_iters = vector_model_ids[model].y;
	}
	if (coeff_matrix->cm.output_stride == 0)
	{
		coeff_matrix->cm.output_stride = vector_model_ids[model].z;
	}
				}
			}
        }

        crm_regfree(&regcb);




        //   Compile up the regex to find the desired feature weight model
        regex_status = crm_regcomp(&regcb, vt_weight_model_regex, (int)strlen(vt_weight_model_regex), REG_ICASE | REG_EXTENDED);
        if (regex_status != 0)
        {
            char errmsg[1024];

            crm_regerror(regex_status, &regcb, errmsg, WIDTHOF(errmsg));
            nonfatalerror_ex(SRC_LOC(), "Custom VT Weight Model: "
                                        "Regular Expression Compilation Problem in VT (Vector Tokenizer) pattern '%s': %s",
                    vt_weight_model_regex,
                    errmsg);
            return -1;
        }

        // Use the regex to find the feature weight model identifier
        regex_status = crm_regexec(&regcb, s2text, s2len, WIDTHOF(match), match, REG_ICASE | REG_EXTENDED, NULL);

        //   Did we actually get a match for the extended parameters?
        if (regex_status == 0 && match[1].rm_so >= 0)
        {
            // Yes, it matched. Take the string, remove leading and trailing whitespace and match
			// case-INsensitively to our list of known weight models:
			char *id = &s2text[match[1].rm_so];
			int len = match[1].rm_eo - match[1].rm_so;
			int start = 0;
			
			if (!crm_nextword(id, len, start, &start, &len))
			{
            nonfatalerror_ex(SRC_LOC(), "Custom VT Weight Model: "
				"You have specified an empty weight model identifier with your 'weight-model:' attribute for the VT (Vector Tokenizer). Assuming the default model...");

			// don't change anything weight matrix related!
			}
			else
			{
				int i;
				int model = -1;
			
				id += start;

				for (i = 0; i < WIDTHOF(weight_model_ids); i++)
				{
					if (!strncasecmp(id, weight_model_ids[i].identifier, CRM_MAX(strlen(weight_model_ids[i].identifier), len)))
					{
						// match!
						model = i;
						break;
					}
				}

				if (model < 0)
				{
            fatalerror_ex(SRC_LOC(), "Custom VT Weight Model: "
				"You have specified an unidentified weight model identifier '%*.*s' with your 'weight-model:' attribute for the VT (Vector Tokenizer). Assuming the default model...",
				len, len, id);
			return -1;
				}
				else
				{
					// copy weight model:
					int x;
					int y;

					CRM_ASSERT(weight_model_ids[model].weight_table_size <= UNIFIED_VECTOR_LIMIT);

					for (x = coeff_matrix->fw.column_count = CRM_MAX(16, weight_model_ids[model].weight_table_size); x-- > 0; )
					{
						for (y = 0; y < UNIFIED_VECTOR_STRIDE; y++)
						{
							coeff_matrix->fw.feature_weight[x + UNIFIED_VECTOR_LIMIT * y] = weight_model_ids[model].weight_table[x];
						}
					}
					coeff_matrix->fw.row_count = UNIFIED_VECTOR_STRIDE;
				}
			}
        }

        crm_regfree(&regcb);




        //   Compile up the regex to find the vector tokenizer hash mix multipliers
        regex_status = crm_regcomp(&regcb, vt_weight_regex, (int)strlen(vt_weight_regex), REG_ICASE | REG_EXTENDED);
        if (regex_status != 0)
        {
            char errmsg[1024];

            crm_regerror(regex_status, &regcb, errmsg, WIDTHOF(errmsg));
            nonfatalerror_ex(SRC_LOC(), "Custom VT Coefficient Matrix: "
                                        "Regular Expression Compilation Problem in VT (Vector Tokenizer) pattern '%s': %s",
                    vt_weight_regex,
                    errmsg);
            return -1;
        }

        //   Use the regex to find the vector tokenizer weights
        regex_status = crm_regexec(&regcb, s2text, s2len, WIDTHOF(match), match, REG_ICASE | REG_EXTENDED, NULL);

        //   Did we actually get a match for the extended parameters?
        if (regex_status == 0 && match[1].rm_so >= 0)
        {
            //  Yes, it matched.  Set up the pipeline coeffs specially.
            //   The first parameter is the pipe length
            if (decode_userdefd_vt_coeff_matrix(coeff_matrix, &s2text[match[1].rm_so], match[1].rm_eo - match[1].rm_so,
                        USERDEF_COEFF_DECODE_COEFF_MODE))
            {
                return -1;
            }
        }

        crm_regfree(&regcb);




        //   Compile up the regex to find the vector tokenizer feature weights
        regex_status = crm_regcomp(&regcb, vt_feature_weight_regex, (int)strlen(vt_feature_weight_regex), REG_ICASE | REG_EXTENDED);
        if (regex_status != 0)
        {
            char errmsg[1024];

            crm_regerror(regex_status, &regcb, errmsg, WIDTHOF(errmsg));
            nonfatalerror_ex(SRC_LOC(), "Custom VT Coefficient Matrix: "
                                        "Regular Expression Compilation Problem in VT (Vector Tokenizer) pattern '%s': %s",
                    vt_feature_weight_regex,
                    errmsg);
            return -1;
        }

        //   Use the regex to find the vector tokenizer weights
        regex_status = crm_regexec(&regcb, s2text, s2len, WIDTHOF(match), match, REG_ICASE | REG_EXTENDED, NULL);

        //   Did we actually get a match for the extended parameters?
        if (regex_status == 0 && match[1].rm_so >= 0)
        {
            //  Yes, it matched.  Set up the pipeline coeffs specially.
            //   The first parameter is the pipe length
            if (decode_userdefd_vt_coeff_matrix(coeff_matrix, &s2text[match[1].rm_so], match[1].rm_eo - match[1].rm_so,
                        USERDEF_COEFF_DECODE_WEIGHT_MODE))
            {
                return -1;
            }
        }

        crm_regfree(&regcb);




        
		//   Compile up the regex to find the vector tokenizer mix matrix redim sizes
        regex_status = crm_regcomp(&regcb, vt_matrix_clip_regex, (int)strlen(vt_matrix_clip_regex), REG_ICASE | REG_EXTENDED);
        if (regex_status != 0)
        {
            char errmsg[1024];

            crm_regerror(regex_status, &regcb, errmsg, WIDTHOF(errmsg));
            nonfatalerror_ex(SRC_LOC(), "Custom VT Coefficient Matrix: "
                                        "Regular Expression Compilation Problem in VT (Vector Tokenizer) pattern '%s': %s",
                    vt_matrix_clip_regex,
                    errmsg);
            return -1;
        }

        //   Use the regex to find the vector tokenizer weights
        regex_status = crm_regexec(&regcb, s2text, s2len, WIDTHOF(match), match, REG_ICASE | REG_EXTENDED, NULL);

        //   Did we actually get a match for the extended parameters?
        if (regex_status == 0 && match[1].rm_so >= 0)
        {
            //  Yes, it matched. Fetch the 3D sizes and adjust the matrix accordingly.
			int x, y, z;
    int x_max = 0;
    int y_max = 0;
    int z_max = 0;
    int fw_x_max = 0;
    int fw_y_max = 0;
			char *src = &s2text[match[1].rm_so];
			int srclen = match[1].rm_eo - match[1].rm_so;
			int start = 0;

    discover_matrices_max_dimensions(coeff_matrix, &x_max, &y_max, &z_max, &fw_x_max, &fw_y_max);

                //   The first parameter is the pipe length
                if (fetch_value(&x, &src, &srclen, 0, UNIFIED_WINDOW_LEN, "tokenizer pipe length"))
                {
                    return -1;
                }
#if 0 /* cannot check yet! it may be that data is coming out of the default coeff matrix instead! */
            if (x > x_max)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified a VT coefficient matrix %s "
                                            "value which is too large: %d > %d",
                        "pipeline length",
                        x, x_max);
				return -1;
            }
#endif
            coeff_matrix->cm.pipe_len = x;

            //   The second parameter is the number of repeats
            if (fetch_value(&y, &src, &srclen, 0, UNIFIED_VECTOR_LIMIT, "tokenizer iteration count"))
            {
                return -1;
            }
#if 0 /* cannot check yet! it may be that data is coming out of the default coeff matrix instead! */
            if (y > y_max)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified a VT coefficient matrix %s "
                                            "value which is too large: %d > %d",
                        "iteration count",
                        y, y_max);
				return -1;
            }
#endif
            coeff_matrix->cm.pipe_iters = y;

            //   The third parameter is the number of coefficient matrices, i.e. one for each step of a full 'stride':
            if (fetch_value(&z, &src, &srclen, 0, UNIFIED_VECTOR_STRIDE, "tokenizer matrix count"))
            {
                return -1;
            }
#if 0 /* cannot check yet! it may be that data is coming out of the default coeff matrix instead! */
            if (z > z_max)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified a VT coefficient matrix %s "
                                            "value which is too large: %d > %d",
                        "matrix count",
                        z, z_max);
				return -1;
            }
#endif
            coeff_matrix->cm.output_stride = z;
        }

        crm_regfree(&regcb);




        
		//   Compile up the regex to find the vector tokenizer feature weight table redim sizes
        regex_status = crm_regcomp(&regcb, vt_weight_clip_regex, (int)strlen(vt_weight_clip_regex), REG_ICASE | REG_EXTENDED);
        if (regex_status != 0)
        {
            char errmsg[1024];

            crm_regerror(regex_status, &regcb, errmsg, WIDTHOF(errmsg));
            nonfatalerror_ex(SRC_LOC(), "Custom VT Weight Matrix: "
                                        "Regular Expression Compilation Problem in VT (Vector Tokenizer) pattern '%s': %s",
                    vt_weight_clip_regex,
                    errmsg);
            return -1;
        }

        //   Use the regex to find the vector tokenizer weights
        regex_status = crm_regexec(&regcb, s2text, s2len, WIDTHOF(match), match, REG_ICASE | REG_EXTENDED, NULL);

        //   Did we actually get a match for the extended parameters?
        if (regex_status == 0 && match[1].rm_so >= 0)
        {
            //  Yes, it matched. Fetch the 3D sizes and adjust the matrix accordingly.
			int x, y, z;
    int x_max = 0;
    int y_max = 0;
    int z_max = 0;
    int fw_x_max = 0;
    int fw_y_max = 0;
			char *src = &s2text[match[1].rm_so];
			int srclen = match[1].rm_eo - match[1].rm_so;
			int start = 0;

    discover_matrices_max_dimensions(coeff_matrix, &x_max, &y_max, &z_max, &fw_x_max, &fw_y_max);

            //   The first parameter is the number of repeats
            if (fetch_value(&x, &src, &srclen, 0, UNIFIED_VECTOR_LIMIT, "feature order (iteration count)"))
            {
                return -1;
            }
#if 0 /* cannot check yet! it may be that data is coming out of the default coeff matrix instead! */
            if (x > CRM_MAX(fw_x_max, y_max))
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified a VT weight table %s "
                                            "value which is too large: %d > %d",
                        "iteration count",
                        x, CRM_MAX(fw_x_max, y_max));
				return -1;
            }
#endif
			coeff_matrix->fw.column_count = x;

            //   The second parameter is the number of coefficient matrices, i.e. one for each step of a full 'stride':
            if (fetch_value(&y, &src, &srclen, 0, UNIFIED_VECTOR_STRIDE, "tokenizer matrix count (stride)"))
            {
                return -1;
            }
#if 0 /* cannot check yet! it may be that data is coming out of the default coeff matrix instead! */
            if (y > UNIFIED_VECTOR_STRIDE)
            {
                nonfatalerror_ex(SRC_LOC(), "You've specified a VT weight table %s "
                                            "value which is too large: %d > %d",
                        "matrix count (stride)",
                        y, UNIFIED_VECTOR_STRIDE);
				return -1;
            }
#endif
			coeff_matrix->fw.row_count = y;
        }

        crm_regfree(&regcb);
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
//
//  config_vt_coeff_matrix_and_tokenizer can be used to configure your (not-so-)default
//  values for the VT 3D coefficient matrix and tokenizer. This routine will fill in the
//  appropriate classifier algorithm defaults, using the 'apb' code line info,
//  resulting in a coeff vector with pipelen and pipe_iters in 'our_coeff', unless
//  you passed in already preconfigured values for these.
//
//  Algorithm: - non-NIL entries for our_coeff/tokenizer elements on entry are highest
//               priority.
//             - A specfication in the apb:FLAGS is next highest priority; if
//               the FLAGS specifies a particular tokenization, use that.
//             - Finally, use the default for the particular classifier.
//
//  Nota Bene: you'll have to add new defaults here as new classifier
//  algorithms get added.
//
//  Nota Bene Quoque: This routine ASSUMES you already have your tokenizer
//  set up when you want to use a custom tokenizer; if it is NIL (i.e. hoping
//  for us to fill in the blanks), several classifiers/classifier-flags WILL
//  configure (part of) your tokenizer too with default values particular
//  to that flag/classifier.
//  <by_char> is such a flag, which will configure the tokenizer to assume
//  a 'token' is a single character (instead of a series of characters).
//
//  Return a negative number on error, 0 on success.
//
int config_vt_coeff_matrix_and_tokenizer
(
        ARGPARSE_BLOCK          *apb,           // The args for this line of code
        VHT_CELL               **vht,
        CSL_CELL                *tdw,
        VT_USERDEF_TOKENIZER    *tokenizer,     // the parsing regex (might be ignored)
        VT_USERDEF_COEFF_MATRIX *our_coeff      // the pipeline coefficient control array, etc.
)
{
    //    To do the defaulting, we work from the "bottom up", filling
    //    in defaults as we go.
    //
    //    First, we pick the length by what the classifier expects/needs.
    //    Some classifiers (Markov, OSB, and Winnow) use the OSB feature
    //    set, which is 64-bit features (referred to as "hash and key",
    //    where hash and key are each 32-bit).  Others (Hyperspace, SVM)
    //    use only 32-bit features.  And finally, Correlate, FSCM and
    //    Bit Entropy don't use tokenization at all; getting here with those
    //    is an error of the first water.  :-)
    //
    //    Second, the actual hashing vector is chosen.  Because of a
    //    historical accident (well, actually stupidity on Bill's part)
    //    Markov and OSB use slightly different hashing control vectors; they
    //    should have been the same.
    //
    uint64_t classifier_flags;

    VT_USERDEF_COEFF_MATRIX default_coeff = { 0 };
    VT_USERDEF_TOKENIZER default_tokenizer = { 0 };
    const int *ew;
    int ew_len;

    // sanity checks
    if (!apb || !tokenizer || !our_coeff)
    {
        nonfatalerror("Sanity check failed while configuring VT (Vector Tokenizer) default coefficient matrix and tokenizer setup.",
                "That developer overthere should've passed some nice, clean, zeroed structures in there, you know. Or was it you?");
        return -1;
    }

    //    now do the work.

    classifier_flags = apb->sflags;

    default_tokenizer.regex_compiler_flags = REG_EXTENDED;
    default_tokenizer.regex_compiler_flags_are_set = 1;

	//default_tokenizer.padding_length = OSB_BAYES_WINDOW_LEN - 1;
	default_tokenizer.pad_start = TRUE;
	default_tokenizer.pad_end_with_first_chunk = FALSE;

    ew = flat_model_weight;
    ew_len = WIDTHOF(flat_model_weight);

    default_coeff.cm.output_stride = 1;   // but may be adjusted to 2 further on ...

    //     If it's one of the dual-hash (= 64-bit-key) classifiers, then the featurebits
    //     need to be 64 --> stride = 2 (x 32 bits).
    if (classifier_flags & CRM_MARKOVIAN
        || classifier_flags & CRM_OSB
        || classifier_flags & CRM_WINNOW
        || classifier_flags & CRM_OSBF
       )
    {
        //     We're a 64-bit hash, so build a 64-bit interleaved feature set.
        default_coeff.cm.output_stride = 2;
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN
             || classifier_flags & CRM_ALT_OSB_BAYES
             || classifier_flags & CRM_ALT_OSB_WINNOW
             || classifier_flags & CRM_ALT_OSBF
            )
    {
        //     We're a 64-bit hash, so build a 64-bit interleaved feature set.
        default_coeff.cm.output_stride = 2;

		//default_tokenizer.padding_length = OSB_BAYES_WINDOW_LEN - 1;
	default_tokenizer.pad_start = FALSE;
	default_tokenizer.pad_end_with_first_chunk = TRUE;
	}

    //    Set up some clean initial values for the important parameters.
    //    Default is always the OSB featureset, 32-bit features.


    {
        int x;
        int y;
        int z;
        contributing_token_func *cb = osb_contributing_token;
        int pipe_len = OSB_BAYES_WINDOW_LEN;
        int pipe_iters = 4;

        if (classifier_flags & CRM_MARKOVIAN)
        {
            cb = markovian_contributing_token;

            pipe_len = MARKOVIAN_WINDOW_LEN;
            pipe_iters = 16;

            ew = hidden_markov_weight;
            ew_len = WIDTHOF(hidden_markov_weight);

		//default_tokenizer.padding_length = MARKOVIAN_WINDOW_LEN - 1;
		}
        else if (classifier_flags & CRM_ALT_MARKOVIAN)
        {
            cb = markov_alt_contributing_token;

            pipe_len = MARKOVIAN_WINDOW_LEN;
            pipe_iters = 16;

            ew = hidden_markov_weight;
            ew_len = WIDTHOF(hidden_markov_weight);

		//default_tokenizer.padding_length = MARKOVIAN_WINDOW_LEN - 1;
        }
        else if (classifier_flags & CRM_OSB_WINNOW)
        {
            // cb = markov_alt_contributing_token;

            pipe_len = OSB_WINNOW_WINDOW_LEN;
            pipe_iters = 4;

            //ew = hidden_markov_weight;
            //ew_len = WIDTHOF(hidden_markov_weight);

		//default_tokenizer.padding_length = OSB_WINNOW_WINDOW_LEN - 1;
        }
        else if (classifier_flags & CRM_ALT_OSB_WINNOW)
        {
            // cb = markov_alt_contributing_token;

            pipe_len = OSB_WINNOW_WINDOW_LEN;
            pipe_iters = 4;

            //ew = hidden_markov_weight;
            //ew_len = WIDTHOF(hidden_markov_weight);

		//default_tokenizer.padding_length = OSB_WINNOW_WINDOW_LEN - 1;
        }
        else if (classifier_flags & CRM_OSBF)
        {
            ew = osbf_feature_weight;
            ew_len = WIDTHOF(osbf_feature_weight);
        }
        else if (classifier_flags & CRM_ALT_OSBF)
        {
            ew = osbf_feature_weight;
            ew_len = WIDTHOF(osbf_feature_weight);
        }
        else if (classifier_flags & CRM_OSB_BAYES)
        {
            ew = osb_bayes_feature_weight;
            ew_len = WIDTHOF(osb_bayes_feature_weight);
        }
        else if (classifier_flags & CRM_ALT_OSB_BAYES)
        {
            ew = osb_bayes_feature_weight;
            ew_len = WIDTHOF(osb_bayes_feature_weight);
        }
        else if (classifier_flags & (CRM_ALT_HYPERSPACE | CRM_HYPERSPACE))
        {
            // requires sorted feature hash series as output
            our_coeff->flags.sorted_output = TRUE;
        }

    if (classifier_flags & CRM_CHI2)
    {
        if (user_trace)
{
            fprintf(stderr, " using chi^2 feature weights\n");
}
            ew = chi2_feature_weight;
            ew_len = WIDTHOF(chi2_feature_weight);
    }

	
	if (generate_matrix_for_model(&default_coeff, cb, hctable, WIDTHOF(hctable)))
    {
        fatalerror("Failed to generate the VT matrix from the specified model.",
                "This kind of disaster shouldn't befall you.");
        return -1;
    }

            default_coeff.cm.pipe_len = pipe_len;
            default_coeff.cm.pipe_iters = pipe_iters;

        //
        //     Do we want a string kernel a.k.a. <by_char> flag?  If so, then we have to override
        //     a few things.
        //
        // The new FSCM does in fact do tokenization and hashing over
        // a string kernel, but only for the indexing.
        if (classifier_flags & (CRM_STRING | CRM_FSCM))
        {
            crm_memmove(default_coeff.cm.coeff_array, string1_coeff, sizeof(string1_coeff));
            crm_memmove(default_coeff.cm.coeff_array + UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT, string2_coeff, sizeof(string2_coeff));

            default_coeff.cm.pipe_len = (classifier_flags & CRM_FSCM ? FSCM_DEFAULT_CODE_PREFIX_LEN : 5);                // was 5
            default_coeff.cm.pipe_iters = 1;
            CRM_ASSERT(default_coeff.cm.pipe_len <= WIDTHOF(string1_coeff));         // was 5
            CRM_ASSERT(default_coeff.cm.pipe_len <= WIDTHOF(string2_coeff));         // was 5

            // set specific tokenizer default too: single char regex;
            //
            // WARNING: this is a little wicked, because this way the <by_char>
            // flag will configure your tokenizer anyway if you didn't do so
            // yourself already, even when you DID specify a custom 3D coefficient
            // matrix!
            //
            // Hence we only use this default once we've made absolutely sure
            // there's no custom tokenizer regex specified
            // by the user (we ignore custom matrices):
            // action is delayed... inject into config_vt_tokenizer() further below.
            if (!tokenizer->regex)
            {
                default_tokenizer.regex = ".";
                default_tokenizer.regexlen = 1;
                default_tokenizer.regex_malloced = 0;
            }
        }

        //     Do we want a unigram system?  If so, then we change a few more
        //     things.
        if (classifier_flags & CRM_UNIGRAM)
        {
            int i;

            // [i_a] GROT GROT GROT: shouldn't we make sure that for stride > 1 even UNIGRAM delivers
            // different hashes for the 1-token features, say, by using different multipliers for the different
            // stride offsets? say, 1, 3, 7, 13 for 1..UNIFIED_VECTOR_STRIDE(=4) ?
            //
            // This new code does just that; the OLD code set ALL multipliers to '1', which can impact
            // hashtable performance for classifiers which use all features (hashes) for (stride > 1) to
            // index the features in such a hashtable.
            CRM_ASSERT(WIDTHOF(unigram_coeff) >= UNIFIED_VECTOR_STRIDE);
            for (i = 0; i < UNIFIED_VECTOR_STRIDE; i++)
            {
                default_coeff.cm.coeff_array[0 + i * UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT] = unigram_coeff[i % WIDTHOF(unigram_coeff)];
                // old code:
                //*coeff_array++ = unigram_coeff[0];
            }
            default_coeff.cm.pipe_len = 1;
            default_coeff.cm.pipe_iters = 1;
        }

//#if USE_FIXED_UNIQUE_MODE
        if (classifier_flags & CRM_UNIQUE)
        {
            if (user_trace)
            {
                fprintf(stderr, " enabling uniqueifying features.\n");
            }
            our_coeff->flags.unique = TRUE;
            our_coeff->flags.sorted_output = TRUE;     // implied!
        }



        /* generate the default feature weights matrix... */
                        default_coeff.fw.column_count = CRM_MAX(16, CRM_MAX(ew_len, pipe_iters));
                        default_coeff.fw.row_count = UNIFIED_VECTOR_STRIDE;

						for (y = UNIFIED_VECTOR_STRIDE; y-- > 0;)
        {
            int *fw = &default_coeff.fw.feature_weight[y * UNIFIED_VECTOR_LIMIT];

            for (x = default_coeff.fw.column_count; x-- > 0;)
            {
                // zero weights are simply not acceptable ;-)

				fw[x] = ew[(x < ew_len ? x : ew_len - 1)];                                         
				// expand fw array beyond own range: this is so we can tolerate larger iteration counts.
            }
        }
    }


    if (internal_trace || user_trace)
    {
	fprintf(stderr, "default matrices -->\n");
        print_matrices(&default_coeff, (fprintf_equivalent_func *)fprintf, stderr);
	fprintf(stderr, "\n-------------------------------------------------\n");
    }

    {
        VT_USERDEF_COEFF_MATRIX coeff_matrix = { 0 };

        if (get_vt_vector_from_2nd_slasharg(&coeff_matrix, apb, vht, tdw))
        {
            nonfatalerror(
                    "There's a boo-boo in the 2nd slash argument where we try to look for the 'vector:...' "
                    "coefficient matrix collection while configuring VT (Vector Tokenizer) default coefficient "
                    "matrix and tokenizer setup.",
                    "");
            return -1;
        }
        else
        {
    if (internal_trace || user_trace)
    {
	fprintf(stderr, "2nd slash arg: matrices -->\n");
        print_matrices(&coeff_matrix, (fprintf_equivalent_func *)fprintf, stderr);
	fprintf(stderr, "\n-------------------------------------------------\n");
    }

            if (our_coeff->cm.pipe_len == 0 || our_coeff->cm.pipe_iters == 0 || our_coeff->cm.output_stride == 0)
            {
                if (coeff_matrix.cm.pipe_len != 0 && coeff_matrix.cm.pipe_iters != 0 && coeff_matrix.cm.output_stride != 0
                    && coeff_matrix.cm.coeff_array[0] != 0 /* do we have a valid matrix fill? */)
                {
                    crm_memmove(&our_coeff->cm, &coeff_matrix.cm, sizeof(coeff_matrix.cm));
                }
                else
                {
                    // no custom spec in second slash arg? too bad, use our defaults instead
                    crm_memmove(&our_coeff->cm, &default_coeff.cm, sizeof(default_coeff.cm));

                    if (coeff_matrix.cm.pipe_len != 0 && coeff_matrix.cm.pipe_iters != 0 && coeff_matrix.cm.output_stride != 0
                        && coeff_matrix.cm.coeff_array[0] == 0 /* is this 'clipping info only' in there? */)
                    {
                        our_coeff->cm.output_stride = coeff_matrix.cm.output_stride;
                        our_coeff->cm.pipe_iters = coeff_matrix.cm.pipe_iters;
                        our_coeff->cm.pipe_len = coeff_matrix.cm.pipe_len;
                    }
                }
            }

            if (our_coeff->fw.column_count == 0 || our_coeff->fw.row_count == 0)
            {
                if (coeff_matrix.fw.column_count != 0 && coeff_matrix.fw.row_count != 0
                    && coeff_matrix.fw.feature_weight[0] != 0 /* do we have a valid matrix fill? */)
                {
                    crm_memmove(&our_coeff->fw, &coeff_matrix.fw, sizeof(coeff_matrix.fw));
                }
                else
                {
                    // no custom spec in second slash arg? too bad, use our defaults instead
                    crm_memmove(&our_coeff->fw, &default_coeff.fw, sizeof(default_coeff.fw));

                    if (coeff_matrix.fw.column_count != 0 && coeff_matrix.fw.row_count != 0
                        && coeff_matrix.fw.feature_weight[0] == 0 /* is this 'clipping info only' in there? */)
                    {
                        our_coeff->fw.column_count = coeff_matrix.fw.column_count;
                        our_coeff->fw.row_count = coeff_matrix.fw.row_count;
                    }
                }
            }
        }
    }

    if (internal_trace || user_trace)
    {
	fprintf(stderr, "VT: constructed matrices -->\n");
        print_matrices(our_coeff, (fprintf_equivalent_func *)fprintf, stderr);
	fprintf(stderr, "\n-------------------------------------------------\n");
    }

	{
        int x_max = 0;
        int y_max = 0;
        int z_max = 0;
        int fw_x_max = 0;
        int fw_y_max = 0;

        discover_matrices_max_dimensions(our_coeff, &x_max, &y_max, &z_max, &fw_x_max, &fw_y_max);

						/* now validate those dimensions: */
        if (our_coeff->cm.output_stride > z_max
            || our_coeff->cm.pipe_iters > y_max
            || our_coeff->cm.pipe_len > x_max)
        {
            nonfatalerror_ex(SRC_LOC(),
                    "ERROR: we're faced with a 3D VT matrix with "
                    "out-of-bounds dimensions: actual = %d x %d x %d, "
                    "maximum fill = %d x %d x %d -- and "
                    "those actual dimensions should all be less or "
                    "equal to the maximum fill!",
                    our_coeff->cm.pipe_len,
                    our_coeff->cm.pipe_iters,
                    our_coeff->cm.output_stride,
                    x_max,
                    y_max,
                    z_max);
            return -1;
        }
        if (our_coeff->cm.output_stride <= 0
            || our_coeff->cm.pipe_iters <= 0
            || our_coeff->cm.pipe_len <= 0)
        {
            nonfatalerror_ex(SRC_LOC(),
                    "ERROR: we're faced with a 3D VT matrix with "
                    "out-of-bounds dimensions: actual = %d x %d x %d, "
                    "maximum fill = %d x %d x %d -- and "
                    "those actual dimensions should all be larger "
                    "than zero!",
                    our_coeff->cm.pipe_len,
                    our_coeff->cm.pipe_iters,
                    our_coeff->cm.output_stride,
                    x_max,
                    y_max,
                    z_max);
            return -1;
        }

        if (our_coeff->fw.column_count > fw_x_max
            || our_coeff->fw.row_count > fw_y_max)
        {
            nonfatalerror_ex(SRC_LOC(),
                    "ERROR: we're faced with a 2D feature weight matrix with "
                    "out-of-bounds dimensions: actual = %d x %d, "
                    "maximum fill = %d x %d -- and "
                    "those actual dimensions should all be less or "
                    "equal to the maximum fill!",
                    our_coeff->fw.column_count,
                    our_coeff->fw.row_count,
                    fw_x_max,
                    fw_y_max);
            return -1;
        }
        if (our_coeff->fw.column_count <= 0
            || our_coeff->fw.row_count <= 0)
        {
            nonfatalerror_ex(SRC_LOC(),
                    "ERROR: we're faced with a 2D feature weight matrix with "
                    "out-of-bounds dimensions: actual = %d x %d, "
                    "maximum fill = %d x %d -- and "
                    "those actual dimensions should all be larger "
                    "than zero!",
                    our_coeff->fw.column_count,
                    our_coeff->fw.row_count,
                    fw_x_max,
                    fw_y_max);
            return -1;
        }

		our_coeff->flags.arne_optimization_allowed = check_if_arne_optimization_is_possible(our_coeff);
    }

	CRM_ASSERT(default_tokenizer.padding_length == 0);
		default_tokenizer.padding_length = our_coeff->cm.pipe_len - 1;
	default_tokenizer.padding_settings_are_set = TRUE;

    //     Now all of the classifier defaults have been filled in; we now see if the
    //     caller has overridden any (or all!) of them.   We assume that the
    //     user who overrides them has pre-sanity-checked them as well.

    if (!tokenizer->initial_setup_done)
    {
        if (config_vt_tokenizer(tokenizer, apb, vht, tdw, &default_tokenizer))
        {
            nonfatalerror("Error while configuring VT (Vector Tokenizer) default tokenizer setup",
                    "");
            return -1;
        }
    }


    if (!our_coeff->cleanup)
    {
        our_coeff->cleanup = &default_VT_coeff_matrix_cleanup_func;
    }

    return 0;
}







//////////////////////////////////////////////////////////////////////////
//
//     Now, some nice, easy-to-use code wrappers for commonly used
//     versions of the vector tokenizer
//
//////////////////////////////////////////////////////////////////////////
//
//  crm_vector_tokenize_selector is the "single interface" to get
//  the right vector tokenizer result given an classifier algorithm default,
//  an int64 "flags", and a coeff vector with pipelen and pipe_iters
//
//  Algorithm:  coeff / pipelen / pipe_iters are highest priority; if
//              coeff is non-NULL, use those.
//              A specfication in the FLAGS is next highest priority; if
//              the FLAGS specifies a particular tokenization, use that.
//              Finally, use the default for the particular classifier
//
//  Nota Bene: you'll have to add new defaults here as new classifier
//  algorithms get added.
//

int crm_vector_tokenize_selector
(
        ARGPARSE_BLOCK          *apb,                // The args for this line of code
        VHT_CELL               **vht,
        CSL_CELL                *tdw,
        const char              *text,               // input string (null-safe!)
        int                      textlen,            // how many bytes of input.
        int                      start_offset,       // start tokenizing at this byte.
        VT_USERDEF_TOKENIZER    *user_tokenizer,     // the parsing regex (might be ignored)
        VT_USERDEF_COEFF_MATRIX *user_coeff,         // the pipeline coefficient control array, etc.
        crmhash_t               *features,           // where the output features go
        int                      featureslen,        // how many output features (max)
        int                     *feature_weights,    // feature weight per feature
        int                     *order_no,           // order_no (starting at 0) per feature
        int                     *features_out        // how many feature-slots did we actually use up
// int *next_offset --> is in VT_USERDEF_TOKENIZER object already
)
{
    int status;
    VT_USERDEF_TOKENIZER default_toker = { 0 };
    VT_USERDEF_COEFF_MATRIX default_coeff = { 0 };
    VT_USERDEF_TOKENIZER *tokenizer;
    VT_USERDEF_COEFF_MATRIX *coeff;


    // sanity checks
    if (!text || !features || !features_out)
    {
        nonfatalerror("Sanity check failed while starting up the VT (Vector Tokenizer) selector.",
                "Either we were'nt passed a text or no feature bucket to put the data in.");
        return -1;
    }
    *features_out = 0;
    if (featureslen < 1)
    {
        nonfatalerror("Sanity check coughed up a hairball while starting up the VT (Vector Tokenizer) selector.",
                "That was one rediculously small feature bucket size right there.");
        return -1;
    }
    tokenizer = user_tokenizer;
    if (!tokenizer)
    {
        tokenizer = &default_toker;
    }
    coeff = user_coeff;
    if (!coeff)
    {
        coeff = &default_coeff;
    }

    //    now do the work.

    // set up a default tokenizer and/or coefficient matrix if 'they' didn't specify one yet.
    if (config_vt_coeff_matrix_and_tokenizer(apb, vht, tdw, tokenizer, coeff))
    {
        return -1;
    }


    if (user_trace)
    {
        fprintf(stderr, "Vector tokenization summary: start %d len %d\n",
                start_offset, textlen);
    }


    //    We now have our parameters all set, and we can run the vector hashing.
    status = crm_vector_tokenize(text,
            textlen,
            start_offset,
            tokenizer,
            coeff,
            features,
            featureslen,
            feature_weights,
            order_no,
            features_out);



    // cleanup our own 'default' structures...
    if (!user_tokenizer)
    {
        tokenizer->cleanup(tokenizer);
    }
    if (!user_coeff)
    {
        coeff->cleanup(coeff);
    }

    if (status < 0)
        return status;

    return *features_out;
}







#if 0


//////////////////////////////////////////////////////////////////////////
//
//     Now, some nice, easy-to-use code wrappers for commonly used
//     versions of the vector tokenizer
//////////////////////////////////////////////////////////////////////////

//  crm_vector_tokenize_selector is the "single interface" to get
//  the right vector tokenizer result given an classifier algorithm default,
//  an int64 "flags", and a coeff vector with pipelen and pipe_iters
//
//  Algorithm:  coeff / pipelen / pipe_iters are highest priority; if
//                coeff is non-NULL, use those.
//              A specfication in the FLAGS is next highest priority; if
//                the FLAGS specifies a particular tokenization, use that.
//              Finally, use the default for the particular classifier
//
//  Nota Bene: you'll have to add new defaults here as new classifier
//  algorithms get added.
//

int crm_vector_tokenize_selector
(
        ARGPARSE_BLOCK          *apb,            // The args for this line of code
        VHT_CELL               **vht,
        CSL_CELL                *tdw,
        const char              *text,            // input string (null-safe!)
        int                      textlen,         //   how many bytes of input.
        int                      start_offset,    //     start tokenizing at this byte.
        VT_USERDEF_TOKENIZER    *tokenizer,       // the parsing regex (might be ignored)
        VT_USERDEF_COEFF_MATRIX *userdef_coeff,   // the pipeline coefficient control array, etc.
        crmhash_t               *features,        // where the output features go
        int                      featureslen,     //   how many output features (max)
        int                     *features_out,    // how many feature-slots did we actually use up
        int                     *next_offset      // next invocation should start at this offset
)
{
    //    To do the defaulting, we work from the "bottom up", filling
    //    in defaults as we go.
    //
    //    First, we pick the length by what the classifier expects/needs.
    //    Some classifiers (Markov, OSB, and Winnow) use the OSB feature
    //    set, which is 64-bit features (referred to as "hash and key",
    //    where hash and key are each 32-bit).  Others (Hyperspace, SVM)
    //    use only 32-bit features; FSCM uses them as an ersatz entry
    //    to do index speedup.  And finally, Correlate and
    //    Bit Entropy don't use tokenization at all; getting here with those
    //    is an error of the first water.  :-)
    //
    //    Second, the actual hashing vector is chosen.  Because of a
    //    historical accident (well, actually stupidity on Bill's part)
    //    Markov and OSB use slightly different hashing control vectors; they
    //    should have been the same.
    //
    uint64_t classifier_flags;
    int featurebits;

    int *hash_vec0;
    int hash_len0;
    int hash_iters0;
    int *hash_vec1;
    int hash_len1;
    int hash_iters1;
    int output_stride;
    char *my_regex;
    int my_regex_len;

    char s1text[MAX_PATTERN];
    int s1len;


    // For slash-embedded pipeline definitions.
    int ca[UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT];

    char *string_kern_regex = ".";
    int string_kern_regex_len = 1;
    char *fscm_kern_regex = ".";
    int fscm_kern_regex_len = 1;

    if (user_trace)
        fprintf(stderr, "Vector tokenization summary: start %ld len %ld\n",
                txtstart, txtlen);

    //    Set up some clean initial values for the important parameters.
    //    Default is always the OSB featureset, 32-bit features.
    //
    classifier_flags = apb->sflags;
    featurebits = 32;
    hash_vec0 = osb1_coeff;
    hash_len0 = OSB_BAYES_WINDOW_LEN;  // was 5
    hash_iters0 = 4;                   // should be 4
    hash_vec1 = osb2_coeff;
    hash_len1 = OSB_BAYES_WINDOW_LEN;   // was 5
    hash_iters1 = 4;                    // should be 4
    output_stride = 1;

    //    put in the passed-in regex values, if any.
    my_regex = regex;
    my_regex_len = regexlen;


    //    Now we can proceed to set up the work in a fairly linear way.

    //    If it's the Markov classifier, then different coeffs and a longer len
    if (classifier_flags & CRM_MARKOVIAN)
    {
        hash_vec0 = markov1_coeff;
        hash_vec1 = markov2_coeff;
        hash_iters0 = hash_iters1 = 16;
    }

    //     If it's one of the 64-bit-key classifiers, then the featurebits
    //     need to be 64.
    if (classifier_flags & CRM_MARKOVIAN
        || classifier_flags & CRM_OSB
        || classifier_flags & CRM_WINNOW
        || classifier_flags & CRM_OSBF
       )
    {
        //     We're a 64-bit hash, so build a 64-bit interleaved feature set.
        featurebits = 64;
        output_stride = 2;
    }

    //       The new FSCM does in fact do tokeniation and hashing over
    //       a string kernel, but only for the indexing.
    if (classifier_flags & CRM_FSCM)
    {
        // fprintf (stderr, "FSCM selector activated.\n");
        hash_vec0 = string1_coeff;
        hash_len0 = FSCM_DEFAULT_CODE_PREFIX_LEN;
        hash_iters0 = 1;
        hash_vec1 = string2_coeff;
        hash_len1 = 1;
        hash_iters1 = 0;
        if (regexlen > 0)
        {
            my_regex = regex;
            my_regex_len = regexlen;
        }
        else
        {
            my_regex = fscm_kern_regex;
            my_regex_len = fscm_kern_regex_len;
        }
    }

    //     Do we want a string kernel?  If so, then we have to override
    //     a few things.
    if (classifier_flags & CRM_STRING)
    {
        //      fprintf (stderr, "String Kernel");
        hash_vec0 = string1_coeff;
        hash_len0 = 5;
        hash_iters0 = 1;
        hash_vec1 = string2_coeff;
        hash_len1 = 5;
        hash_iters1 = 1;
        if (regexlen == 0)
        {
            my_regex = string_kern_regex;
            my_regex_len = string_kern_regex_len;
        }
    }

    //     Do we want a unigram system?  If so, then we change a few more
    //     things.
    if (classifier_flags & CRM_UNIGRAM)
    {
        hash_vec0 = unigram_coeff;
        hash_len0 = 1;
        hash_iters0 = 1;
        hash_vec1 = unigram_coeff;
        hash_len1 = 1;
        hash_iters1 = 1;
    }


    //     Now all of the defaults have been filled in; we now see if the
    //     caller has overridden any (or all!) of them.   We assume that the
    //     user who overrides them has pre-sanity-checked them as well.

    //     First check- did the user override the regex?

    //    Did the user program specify a first slash paramter?  (only
    //    override this if a regex was passed in)
    if (regexlen > 0)
    {
        crm_get_pgm_arg(s1text, MAX_PATTERN, apb->s1start, apb->s1len);
        s1len = apb->s1len;
        s1len = crm_nexpandvar(s1text, s1len, MAX_PATTERN, vht, tdw);
        my_regex = s1text;
        my_regex_len = s1len;
    }


    //      Did the user specify a pipeline vector set ?   If so, it's
    //      in the second set of slashes.
    {
        char s2text[MAX_PATTERN];
        int s2len;
        int local_pipe_len;
        int local_pipe_iters;
        char *vt_weight_regex = "vector: ([ 0-9]*)";
        regex_t regcb;
        int regex_status;
        regmatch_t match[5]; //  We'll only care about the second match
        local_pipe_len = 0;
        local_pipe_iters = 0;

        //     get the second slash parameter (if used at all)
        crm_get_pgm_arg(s2text, MAX_PATTERN, apb->s2start, apb->s2len);
        s2len = apb->s2len;
        s2len = crm_nexpandvar(s2text, s2len, MAX_PATTERN, vht, tdw);

        if (s2len > 0)
        {
            //   Compile up the regex to find the vector tokenizer weights
            crm_regcomp
                        (&regcb, vt_weight_regex, strlen(vt_weight_regex),
                        REG_ICASE | REG_EXTENDED);

            //   Use the regex to find the vector tokenizer weights
            regex_status =  crm_regexec(&regcb,
                    s2text,
                    s2len,
                    5,
                    match,
                    REG_EXTENDED,
                    NULL);

            //   Did we actually get a match for the extended parameters?
            if (regex_status == 0)
            {
                char *conv_ptr;
                int i;

                //  Yes, it matched.  Set up the pipeline coeffs specially.
                //   The first parameter is the pipe length
                conv_ptr = &s2text[match[1].rm_so];
                local_pipe_len = strtol(conv_ptr, &conv_ptr, 0);
                if (local_pipe_len > UNIFIED_WINDOW_LEN)
                {
                    nonfatalerror("You've specified a tokenizer pipe length "
                                  "that is too long.", "  I'll trim it.");
                    local_pipe_len = UNIFIED_WINDOW_LEN;
                }
                //fprintf (stderr, "local_pipe_len = %ld\n", local_pipe_len);
                //   The second parameter is the number of repeats
                local_pipe_iters = strtol(conv_ptr, &conv_ptr, 0);
                if (local_pipe_iters > UNIFIED_VECTOR_LIMIT)
                {
                    nonfatalerror("You've specified too high a tokenizer "
                                  "iteration count.", "  I'll trim it.");
                    local_pipe_iters = UNIFIED_VECTOR_LIMIT;
                }
                //fprintf (stderr, "pipe_iters = %ld\n", local_pipe_iters);

                //    Now, get the coefficients.
                for (i = 0; i < local_pipe_len * local_pipe_iters; i++)
                {
                    ca[i] = strtol(conv_ptr, &conv_ptr, 0);
                    //  fprintf (stderr, "coeff: %ld\n", ca[i]);
                }

                //   If there was a numeric coeff array, use that, else
                //   use our slash coeff array.
                if (!coeff_array)
                {
                    coeff_array = ca;
                    pipe_len = local_pipe_len;
                    pipe_iters = local_pipe_iters;
                }
            }
            //  free the compiled regex.
            crm_regfree(&regcb);
        }
    };

    //      if any non-default coeff array was given, use that instead.
    if (coeff_array)
    {
        hash_vec0 = coeff_array;
        //                    GROT GROT GROT --2nd array should be different from
        //                    first array- how can we do that nonlinearly?
        //                    This will work for now, but birthday clashes will
        //                    happen more often in 64-bit featuresets
        hash_vec1 = coeff_array;
    }

    if (pipe_len > 0)
    {
        hash_len0 = pipe_len;
        hash_len1 = pipe_len;
    }

    if (pipe_iters > 0)
    {
        hash_iters0 = pipe_iters;
        hash_iters1 = pipe_iters;
    }

    //    We now have our parameters all set, and we can run the vector hashing.
    //
    if (internal_trace)
        fprintf(stderr, "Sext offset: %ld, length: %ld\n", start_offset, txtlen);

    if (output_stride == 1)
    {
        crm_vector_tokenize(
                text,
                textlen,
                start_offset,
                my_regex,
                my_regex_len,
                hash_vec0,
                hash_len0,
                hash_iters0,
                features,
                featureslen,
                1,                      //  stride 1 for 32-bit
                features_out,
                next_offset);
    }
    else
    {
        //        We're doing the 64-bit-long features for Markov/OSB
        crm_vector_tokenize(
                text,
                textlen,
                start_offset,
                my_regex,
                my_regex_len,
                hash_vec0,
                hash_len0,
                hash_iters0,
                features,
                featureslen,
                2,                      //  stride 1 for 32-bit
                features_out,
                next_offset);
        crm_vector_tokenize(
                text,
                textlen,
                start_offset,
                regex,
                regexlen,
                hash_vec1,
                hash_len1,
                hash_iters1,
                &(features[1]),
                featureslen,
                2,                      //  stride 1 for 32-bit
                features_out,
                next_offset);
    }
    return *features_out;
}


//  crm_vector_markov_1 gets the features of the markov H1 field

long crm_vector_markov_1
(
        char          *txtptr,       // input string (null-safe!)
        long           txtstart,     //     start tokenizing at this byte.
        long           txtlen,       //   how many bytes of input.
        char          *regex,        // the parsing regex (might be ignored)
        long           regexlen,     //   length of the parsing regex
        unsigned long *features,     // where the output features go
        long           featureslen,  //   how many output features (max)
        long          *features_out, // how many longs did we actually use up
        long          *next_offset   // next invocation should start at this offset
)
{
    return crm_vector_tokenize
                (txtptr,
                txtstart,
                txtlen,
                regex,
                regexlen,
                markov1_coeff,
                5,
                16,
                features,
                featureslen,
                2,       //  stride 2 for 64-bit features
                features_out,
                next_offset);
}



//  crm_vector_markov_2 is the H2 field in the Markov classifier.
long crm_vector_markov_2
(
        char          *txtptr,       // input string (null-safe!)
        long           txtstart,     //     start tokenizing at this byte.
        long           txtlen,       //   how many bytes of input.
        char          *regex,        // the parsing regex (might be ignored)
        long           regexlen,     //   length of the parsing regex
        unsigned long *features,     // where the output features go
        long           featureslen,  //   how many output features (max)
        long          *features_out, // how many longs did we actually use up
        long          *next_offset   // next invocation should start at this offset
)
{
    return crm_vector_tokenize
                (txtptr,
                txtstart,
                txtlen,
                regex,
                regexlen,
                markov2_coeff,
                5,
                16,
                features,
                featureslen,
                2,          // Stride 2 for 64-bit features
                features_out,
                next_offset);
}

//            vectorized OSB featureset generator.
//
long crm_vector_osb1
(
        char          *txtptr,       // input string (null-safe!)
        long           txtstart,     //     start tokenizing at this byte.
        long           txtlen,       //   how many bytes of input.
        char          *regex,        // the parsing regex (might be ignored)
        long           regexlen,     //   length of the parsing regex
        unsigned long *features,     // where the output features go
        long           featureslen,  //   how many output features (max)
        long          *features_out, // how many longs did we actually use up
        long          *next_offset   // next invocation should start at this offset
)
{
    return crm_vector_tokenize
                (txtptr,
                txtstart,
                txtlen,
                regex,
                regexlen,
                osb1_coeff,
                OSB_BAYES_WINDOW_LEN,
                4, // should be 4
                features,
                featureslen,
                2,
                features_out,
                next_offset);
}

long crm_vector_osb2
(
        char          *txtptr,       // input string (null-safe!)
        long           txtstart,     //     start tokenizing at this byte.
        long           txtlen,       //   how many bytes of input.
        char          *regex,        // the parsing regex (might be ignored)
        long           regexlen,     //   length of the parsing regex
        unsigned long *features,     // where the output features go
        long           featureslen,  //   how many output features (max)
        long          *features_out, // how many longs did we actually use up
        long          *next_offset   // next invocation should start at this offset
)
{
    return crm_vector_tokenize
                (txtptr,
                txtstart,
                txtlen,
                regex,
                regexlen,
                osb2_coeff,
                OSB_BAYES_WINDOW_LEN,
                4, // should be 4
                features,
                featureslen,
                2,
                features_out,
                next_offset);
}


//            vectorized string kernel featureset generator.
//
long crm_vector_string_kernel1
(
        char          *txtptr,          // input string (null-safe!)
        long           txtstart,        //     start tokenizing at this byte.
        long           txtlen,          //   how many bytes of input.
        long           string_kern_len, //   length of the kernel (must be < 16)
        unsigned long *features,        // where the output features go
        long           featureslen,     //   how many output features (max)
        long          *features_out,    // how many longs did we actually use up
        long          *next_offset      // next invocation should start at this offset
)
{
    //    The coeffs should be relatively prime.  Relatively...

    if (string_kern_len > 15)
        string_kern_len = 15;

    return crm_vector_tokenize
                (txtptr,
                txtstart,
                txtlen,
                ".", // regex
                1,   // regexlen
                string1_coeff,
                string_kern_len, //  how many coeffs to use
                1,               //  how many variations (just one)
                features,
                featureslen,
                1,
                features_out,
                next_offset);
}

long crm_vector_string_kernel2
(
        char          *txtptr,          // input string (null-safe!)
        long           txtstart,        //     start tokenizing at this byte.
        long           txtlen,          //   how many bytes of input.
        long           string_kern_len, //   length of the kernel (must be < 16)
        unsigned long *features,        // where the output features go
        long           featureslen,     //   how many output features (max)
        long          *features_out,    // how many longs did we actually use up
        long          *next_offset      // next invocation should start at this offset
)
{
    //    The coeffs should be relatively prime.  Relatively...

    if (string_kern_len > 15)
        string_kern_len = 15;

    return crm_vector_tokenize
                (txtptr,
                txtstart,
                txtlen,
                ".", // regex
                1,   // regexlen
                string2_coeff,
                string_kern_len, //  how many coeffs to use
                1,               //  how many variations (just one)
                features,
                featureslen,
                1,
                features_out,
                next_offset);
}



#endif



