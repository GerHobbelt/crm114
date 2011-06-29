//  crm_str_funcs.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
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
            memmove(&hashpipe[1], hashpipe,
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
  features[*features_out+1] = 0; 
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
        int                            features_stride,    //   Spacing (in hashes) between features
        int                           *features_out        // how many longs did we actually use up
)
{
    crmhash_t hashpipe[CRM_VT_BURST_SIZE + UNIFIED_WINDOW_LEN - 1];  // the pipeline for hashes
    int i;
    int feature_pos;
    crmhash_t *hp;
    int pipe_len;
    int pipe_iters;
    int pipe_matrices;
    int element_step_size;
    int single_featureblock_size;

    if (internal_trace)
    {
        fprintf(stderr, "VT: textlen = %d\n", textlen);
        fprintf(stderr, "VT: start_offset = %d\n", start_offset);
        fprintf(stderr, "VT: features_bufferlen = %d\n", features_bufferlen);
        fprintf(stderr, "VT: features_stride = %d\n", features_stride);
    }

    // sanity checks: everything must be 'valid'; no 'default' NULL pointers, nor zeroed/out-of-range values accepted.
    if (!text || !our_coeff || !features_buffer || !features_out)
    {
        CRM_ASSERT(!"This should never happen in a properly coded CRM114...");
        return -1;
    }

    // tag the output series with a zero asap:
    *features_out = 0;

    if (features_stride < 1)
    {
        CRM_ASSERT(!"This should never happen in a properly coded CRM114...");
        return -1;
    }

    // further sanity checks
    if (features_bufferlen < features_stride)
    {
        CRM_ASSERT(!"This should never happen in a properly coded CRM114...");
        return -1;
    }


    pipe_iters = our_coeff->pipe_iters;
    pipe_len = our_coeff->pipe_len;
    pipe_matrices = our_coeff->output_stride;
    if (internal_trace)
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
                                    "the number of VT matrixes (%d) is out of bounds [%d .. %d]. We don't support any stride that large.",
                pipe_matrices, 1, UNIFIED_VECTOR_STRIDE);
        return -1;
    }

    single_featureblock_size = pipe_iters * pipe_matrices;
    if (internal_trace)
    {
        fprintf(stderr, "VT: single_featureblock_size = %d\n", single_featureblock_size);
    }

    if (single_featureblock_size > features_bufferlen)
    {
        // feature storage space too small:
        nonfatalerror_ex(SRC_LOC(), "Sanity check failed while starting up the VT (Vector Tokenizer): "
                                    "the feature bucket is too small (%d) to contain the minimum required number of features (%d) at least.",
                features_bufferlen, single_featureblock_size);
        return -1;         // can't even store the features from one token, so why bother?
    }

    element_step_size = pipe_matrices * features_stride;
    if (internal_trace)
    {
        fprintf(stderr, "VT: element_step_size = %d\n", element_step_size);
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


    // fill the hashpipe with initialization
    for (i = 0; i < CRM_VT_BURST_SIZE - 1 + UNIFIED_WINDOW_LEN; i++)
    {
        hashpipe[i] = 0xDEADBEEF;
    }

    //
    //   Run the hashpipe, either with regex, or without - always use the provided tokenizer!
    //
    // loop: check to see if we have space left to add more
    //       features assuming there are any left to add.
    for (feature_pos = 0; ;)
    {
        // cough up a fresh token burst now and put them at the front of the pipe, i.e. at the END of the array:
        i = tokenizer->tokenizer(tokenizer, &hashpipe[UNIFIED_WINDOW_LEN - 1], CRM_VT_BURST_SIZE);
        // did we get any?
        if (internal_trace)
        {
            fprintf(stderr, "VT: tokenizer delivered %d tokens\n", i);
        }
        if (i < 0)
        {
            return -1;
        }

        // EOS reached?
        // i.e. did we actually GET a new token or was the input already utterly empty to start with now?
        if (i == 0)
            break;

        // position the 'real' hashpipe at the proper spot in the 'burst': the oldest token is the first one in the burst
        hp = &hashpipe[UNIFIED_WINDOW_LEN - 1];
        //
        // Fringe case which needs fixing? -- what if the tokenizer produced less tokens than the
        // maximum possible 'burst'? NO need to fix since we now use a reversed pipeline: higher indexes are newer
        // instead of older; the 'hp' positional pointer can do it's regular job in this case too
        //if (i != CRM_VT_BURST_SIZE) -- no need for such stuff
        //

        //
        // Since we loaded the tokens in burst, we can now speed up the second part of the process due to
        // loop tightening here too; since the 'reversed' pipeline stores newer tokens at higher indexes,
        // walking the sliding window (a.k.a. 'hp' pipeline) UP from old to new, means we can reposition the
        // 'hp' pipeline reference to point at the NEXT token produced during the burst tokenization before,
        // while keeping track how many tokens were produced by reducing the burst length 'i' --> i--, hp++
        //
        for ( ; i > 0; i--, hp++)
        {
            int matrix;
            const int *coeff_array;
            // remember how far we filled the features bucket already;
            // we're going to interleave another block:
            int basepos = feature_pos;


            coeff_array = our_coeff->coeff_array;

            // select the proper 2D matrix plane from the 3D matrix cube on every round:
            // one full round per available coefficient matrix
            for (matrix = 0;
                 matrix < pipe_matrices;
                 matrix++, coeff_array += pipe_iters * pipe_len)
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
                    register const int *ca = &coeff_array[pipe_len * irow];

                    // [i_a] TO TEST:
                    //
                    // as we are creating a new hash from the combined tokens, it may be better to not use the
                    // old 'universal hash' method (new hash = old hash + position dependent multiplier * token hash)
                    // as there are hash methods available which have much better avalanche behaviour.
                    //
                    // To use such thing, we'll need to store the picked token hashes as is, then feed the new
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
                        // the - minus 'icol' is intentional: reverse d pipeline: older is lower index.
                        ihash += hp[-icol] * universal_multiplier;
                    }

                    //    Stuff the final ihash value into features array -- iff we still got room for it
                    if (feature_pos >= features_bufferlen)
                    {
                        break;
                    }
                    features_buffer[feature_pos] = ihash;

					crm_analysis_mark(&analysis_cfg, MARK_VT_HASH_VALUE, irow, "LLi", (unsigned long long int)ihash, (long long int)feature_pos, (int)matrix);
                
					if (internal_trace)
                    {
                        fprintf(stderr, "New Feature: %08lX at %d\n",
                                (unsigned long int)ihash,
                                feature_pos);
                    }
                    feature_pos += element_step_size;
                }
            }
            // and correct for the interleaving we just did:
            feature_pos -= element_step_size - 1;     // feature_pos points 'element_step_size' elems PAST the last write; should be only +1(ONE)



			// TODO
			// TBD
			// check that feature_pos correction (maybe traverse matrices in reverse?) related to interleaving
			// PLUS the featurepos correction at the very end of this routine!





            // check if we should get another series of tokens ... or maybe not.
            // only allow another burst when we can at least write a single feature strip.
            if (feature_pos + single_featureblock_size > features_bufferlen)
            {
                break;
            }
        }

        // check if we should get another series of tokens ... or maybe not.
        if (feature_pos + single_featureblock_size > features_bufferlen)
        {
            break;
        }
    }

    // update the caller on what's left in the input after this:
    // *next_offset = tokenizer->input_next_offset;

    *features_out = feature_pos - element_step_size + 1; // feature_pos points 'element_step_size' elems PAST the last write; should be only +1(ONE)
    if (user_trace || internal_trace)
    {
        fprintf(stderr, "VT: feature count = %d\n", *features_out);
    }
    // [i_a]
    // Fringe case: yes, we discard the last few feature(s) stored when the
    // feature_bufferlen % pipe_matrices != 0 where pipe_matrices > 0 as then an
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

#ifdef JUST_FOR_REFERENCE

//    hctable is where the OSB coeffs came from - this is now just a
//    historical artifact - DO NOT USE THIS!!!
static const int hctable[] =
{
    1, 7,
    3, 13,
    5, 29,
    11, 51,
    23, 101,
    47, 203,
    97, 407,
    197, 817,
    397, 1637,
    797, 3277
};

#endif

static const int osb1_coeff[] =
{
    1, 3, 0, 0, 0,
    1, 0, 5, 0, 0,
    1, 0, 0, 11, 0,
    1, 0, 0, 0, 23
};

static const int osb2_coeff[] =
{
    7, 13, 0, 0, 0,
    7, 0, 29, 0, 0,
    7, 0, 0, 51, 0,
    7, 0, 0, 0, 101
};

static const int string1_coeff[] =
{ 1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 49, 51 };

static const int string2_coeff[] =
{ 51, 49, 43, 41, 37, 31, 29, 23, 19, 17, 13, 11, 7, 5, 3, 1 };

static const int unigram_coeff[] =
{ 1, 17, 31, 49 };





//
// Return the number of tokens produced; return a negative number on error.
//
static int default_regex_VT_tokenizer_func(VT_USERDEF_TOKENIZER *obj,
        crmhash_t                                               *tokenhash_dest,
        int                                                      tokenhash_dest_size)
{
    const char *txt;
    int txt_len;
    int k = 0;

    //   Run the hashpipe, either with regex, or without.
    //
    CRM_ASSERT(obj != NULL);
    CRM_ASSERT(tokenhash_dest != NULL);
    CRM_ASSERT(tokenhash_dest_size > 0);
    CRM_ASSERT(obj->input_text != NULL);

    if (obj->input_next_offset >= obj->input_textlen)
    {
        return 0;
    }
    txt = obj->input_text + obj->input_next_offset;
    txt_len = obj->input_textlen - obj->input_next_offset;
    if (txt_len <= 0)
    {
        obj->eos_reached = 1;
    }
    if (obj->eos_reached)
    {
        // you'll have no more tokens until you reset 'eos_reached'...
        return 0;
    }

    //  If the pattern is empty, assume non-graph-delimited tokens
    //  (supposedly an 8% speed gain over regexec)
    if (obj->regexlen == 0)
    {
        int big_token_count = 0;

        obj->initial_setup_done = 1;

        for (k = 0; k < tokenhash_dest_size && txt_len > 0; k++)
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
                return -1;
            }

            regcomp_status = crm_regcomp(&obj->regcb, obj->regex, obj->regexlen, obj->regex_compiler_flags);
            if (regcomp_status != 0)
            {
                char errortext[4096];

                crm_regerror(regcomp_status, &obj->regcb, errortext, WIDTHOF(errortext));
                nonfatalerror("Regular Expression Compilation Problem for VT (Vector Tokenizer): ",
                        errortext);
                return -1;
            }
            obj->initial_setup_done = 1;
        }

        for (k = 0; k < tokenhash_dest_size && txt_len > 0; k++)
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

    obj->cleanup = 0;

    return 0;
}


static int default_VT_coeff_matrix_cleanup_func(struct magical_VT_userdef_coeff_matrix *obj)
{
    memset(obj->coeff_array, 0, sizeof(obj->coeff_array));
    obj->output_stride = 0;
    obj->pipe_iters = 0;
    obj->pipe_len = 0;

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
		VHT_CELL **vht,
		CSL_CELL *tdw,
        const char                           *regex,
        int                                   regex_len,
        int                                   regex_compiler_flags_override)
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
        // is user didn't want us to do this, s/he'd set max_token_length to 0...
        tokenizer->max_token_length = OSBF_MAX_TOKEN_SIZE;
    }
    if (tokenizer->max_big_token_count == 0)
    {
        tokenizer->max_big_token_count = OSBF_MAX_LONG_TOKENS;
    }

    if (!tokenizer->regex_compiler_flags_are_set)
    {
        if (!apb)
        {
            tokenizer->regex_compiler_flags = regex_compiler_flags_override;
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
        if (!regex && apb)
        {
            int s1len;
            char s1text[MAX_PATTERN];
            char *dst;

            s1len = crm_get_pgm_arg(s1text, MAX_PATTERN, apb->s1start, apb->s1len);
            s1len = crm_nexpandvar(s1text, s1len, MAX_PATTERN, vht, tdw);

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
        else
        {
            char *dst;

            tokenizer->regex = dst = calloc(regex_len + 1, sizeof(tokenizer->regex[0]));
            if (!dst)
            {
                untrappableerror("Cannot allocate VT memory", "Stick a fork in us; we're _done_.");
            }
            memcpy(dst, regex, regex_len * sizeof(tokenizer->regex[0]));
            dst[regex_len] = 0;
            tokenizer->regexlen = regex_len;
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

    return 0;
}




static int fetch_value(int *value_ref,
        const char        **src_ref,
        int                *srclen_ref,
        int                 lower_limit,
        int                 upper_limit,
        const char         *description)
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
        const char *src, int srclen)
{
    if (src && *src)
    {
        int i;
        int *ca;

        for (i = 0; i < srclen && !crm_isgraph(src[i]); i++)
            ;

        // only decode it when there's something else next to only whitepace...
        if (i < srclen)
        {
            //   The first parameter is the pipe length
            if (fetch_value(&coeff_matrix->pipe_len, &src, &srclen, 0, UNIFIED_WINDOW_LEN, "tokenizer pipe length"))
            {
                return -1;
            }

            //   The second parameter is the number of repeats
            if (fetch_value(&coeff_matrix->pipe_iters, &src, &srclen, 0, UNIFIED_WINDOW_LEN, "tokenizer iteration count"))
            {
                return -1;
            }

            //   The third parameter is the number of coefficient matrices, i.e. one for each step of a full 'stride':
            if (fetch_value(&coeff_matrix->output_stride, &src, &srclen, 0, UNIFIED_WINDOW_LEN, "tokenizer matrix count"))
            {
                return -1;
            }

            //    Now, get the coefficients.
            ca = coeff_matrix->coeff_array;
            for (i = 0;
                 i < coeff_matrix->pipe_len * coeff_matrix->pipe_iters * coeff_matrix->output_stride;
                 i++)
            {
                if (fetch_value(&coeff_matrix->coeff_array[i], &src, &srclen, 0, UNIFIED_WINDOW_LEN, "VT coefficient matrix value"))
                {
                    return -1;
                }
            }
        }
    }
    return 0;
}






int get_vt_vector_from_2nd_slasharg(VT_USERDEF_COEFF_MATRIX *coeff_matrix,
        const ARGPARSE_BLOCK                                *apb,  // The args for this line of code
		VHT_CELL **vht,
		CSL_CELL *tdw                                 )
{
    char s2text[MAX_PATTERN];
    int s2len;
    regex_t regcb;
    int regex_status;
    regmatch_t match[2];     //  We'll only care about the second match

    //     get the second slash parameter (if used at all)
    s2len = crm_get_pgm_arg(s2text, MAX_PATTERN, apb->s2start, apb->s2len);
    s2len = crm_nexpandvar(s2text, s2len, MAX_PATTERN, vht, tdw);

    if (s2len > 0)
    {
        // static const char *vt_weight_regex = "vector: ([ 0-9]*)";
        static const char *vt_weight_regex = "vector:([^,;]+)";

        //   Compile up the regex to find the vector tokenizer weights
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
            if (decode_userdefd_vt_coeff_matrix(coeff_matrix, &s2text[match[1].rm_so], match[1].rm_eo - match[1].rm_so))
            {
                return -1;
            }
        }
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
		VHT_CELL **vht,
		CSL_CELL *tdw,
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
    int *coeff_array;


    // sanity checks
    if (!apb || !tokenizer || !our_coeff)
    {
        nonfatalerror("Sanity check failed while configuring VT (Vector Tokenizer) default coefficient matrix and tokenizer setup.",
                "That developer overthere should've passed some nice, clean, zeroed structures in there, you know. Or was it you?");
        return -1;
    }

    //    now do the work.



    //    Set up some clean initial values for the important parameters.
    //    Default is always the OSB featureset, 32-bit features.
    //
    classifier_flags = apb->sflags;
    coeff_array = default_coeff.coeff_array;
    memmove(coeff_array, osb1_coeff, sizeof(osb1_coeff));
    coeff_array += WIDTHOF(osb1_coeff);
    default_coeff.pipe_len = OSB_BAYES_WINDOW_LEN;                             // was 5
    default_coeff.pipe_iters = WIDTHOF(osb1_coeff) / OSB_BAYES_WINDOW_LEN;     // should be 4
    memmove(coeff_array, osb1_coeff, sizeof(osb2_coeff));
    coeff_array += WIDTHOF(osb2_coeff);
    CRM_ASSERT(default_coeff.pipe_len == OSB_BAYES_WINDOW_LEN);                             // was 5
    CRM_ASSERT(default_coeff.pipe_iters == WIDTHOF(osb2_coeff) / OSB_BAYES_WINDOW_LEN);     // should be 4
    default_coeff.output_stride = 1;                                                        // but may be adjusted to 2 further on ...


    //    Now we can proceed to set up the work in a fairly linear way.

    //    If it's the Markov classifier, then different coeffs and a longer len
    if (classifier_flags & CRM_MARKOVIAN)
    {
        coeff_array = default_coeff.coeff_array;
        memmove(coeff_array, markov1_coeff, sizeof(markov1_coeff));
        coeff_array += WIDTHOF(markov1_coeff);
        default_coeff.pipe_len = OSB_BAYES_WINDOW_LEN;                                    // was 5
        default_coeff.pipe_iters = WIDTHOF(markov1_coeff) / OSB_BAYES_WINDOW_LEN;         // should be 16
        memmove(coeff_array, markov2_coeff, sizeof(markov2_coeff));
        coeff_array += WIDTHOF(markov2_coeff);
        CRM_ASSERT(default_coeff.pipe_len == OSB_BAYES_WINDOW_LEN);                                    // was 5
        CRM_ASSERT(default_coeff.pipe_iters == WIDTHOF(markov2_coeff) / OSB_BAYES_WINDOW_LEN);         // should be 16
    }

    //     If it's one of the dual-hash (= 64-bit-key) classifiers, then the featurebits
    //     need to be 64 --> stride = 2 (x 32 bits).
    if (classifier_flags & CRM_MARKOVIAN
        || classifier_flags & CRM_OSB
        || classifier_flags & CRM_WINNOW
        || classifier_flags & CRM_OSBF
       )
    {
        //     We're a 64-bit hash, so build a 64-bit interleaved feature set.
        default_coeff.output_stride = 2;
    }

    //
    //     Do we want a string kernel a.k.a. <by_char> flag?  If so, then we have to override
    //     a few things.
    //
    if (classifier_flags & CRM_STRING)
    {
        //      fprintf (stderr, "String Kernel");
        coeff_array = default_coeff.coeff_array;
        memmove(coeff_array, string1_coeff, sizeof(string1_coeff));
        coeff_array += WIDTHOF(string1_coeff);
        memmove(coeff_array, string2_coeff, sizeof(string2_coeff));
        coeff_array += WIDTHOF(string2_coeff);
#if !defined (ORIGINAL_VT_CODE)
        default_coeff.pipe_len = WIDTHOF(string1_coeff);            // was 5
        default_coeff.pipe_iters = 1;
        CRM_ASSERT(default_coeff.pipe_len == WIDTHOF(string1_coeff));             // was 5
        CRM_ASSERT(default_coeff.pipe_len == WIDTHOF(string2_coeff));             // was 5
#else
        default_coeff.pipe_len = 5;            // was 5
        default_coeff.pipe_iters = 1;
        CRM_ASSERT(default_coeff.pipe_len <= OSB_BAYES_WINDOW_LEN);               // was 5
        CRM_ASSERT(default_coeff.pipe_len <= WIDTHOF(string1_coeff));             // was 5
        CRM_ASSERT(default_coeff.pipe_len <= WIDTHOF(string2_coeff));             // was 5
#endif

        // set specific tokenizer default too: single char regex;
        //
        // WARNING: this is a little wicked, because this way the <by_char>
        // flag will configure your tokenizer anyway if you didn't do so
        // yourself already, even when you DID specify a custom 3D coefficient
        // matrix!
        if (0 && !tokenizer->regex)
        {
            tokenizer->regex = ".";
            tokenizer->regexlen = 1;
            tokenizer->regex_malloced = 0;

            // override the regex flags too, no matter if those have been set before:
            tokenizer->regex_compiler_flags = REG_EXTENDED;
            tokenizer->regex_compiler_flags_are_set = 1;
        }
    }

    //     Do we want a unigram system?  If so, then we change a few more
    //     things.
    if (classifier_flags & CRM_UNIGRAM)
    {
        int i;

        coeff_array = default_coeff.coeff_array;
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
            *coeff_array++ = unigram_coeff[i];
            // old code:
            //*coeff_array++ = unigram_coeff[0];
        }
        default_coeff.pipe_len = 1;
        default_coeff.pipe_iters = 1;
    }



    //     Now all of the classifier defaults have been filled in; we now see if the
    //     caller has overridden any (or all!) of them.   We assume that the
    //     user who overrides them has pre-sanity-checked them as well.

    if (!tokenizer->initial_setup_done)
    {
        if (config_vt_tokenizer(tokenizer, apb, vht, tdw, NULL, 0, REG_EXTENDED))
        {
            nonfatalerror("Error while configuring VT (Vector Tokenizer) default tokenizer setup",
                    "");
            return -1;
        }
    }


    if (our_coeff->pipe_len == 0 || our_coeff->pipe_iters == 0 || our_coeff->output_stride == 0)
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
        else if (coeff_matrix.pipe_len != 0 && coeff_matrix.pipe_iters != 0 && coeff_matrix.output_stride != 0)
        {
            memmove(our_coeff, &coeff_matrix, sizeof(coeff_matrix));
        }
        else
        {
            // no custom spec in second slash arg? too bad, use our defaults instead
            memmove(our_coeff, &default_coeff, sizeof(default_coeff));
        }
    }


    if (internal_trace)
    {
        int irow;
        int icol;
        int matrix;
        int *ca = our_coeff->coeff_array;

        for (matrix = 0; matrix < our_coeff->output_stride; matrix++)
        {
            fprintf(stderr, "========== matrix[%d/%d] = %d x %d ==========\n",
                    matrix,
                    our_coeff->output_stride,
                    our_coeff->pipe_iters,
                    our_coeff->pipe_len);

            for (irow = 0; irow < our_coeff->pipe_iters; irow++)
            {
                fprintf(stderr, "row[%2d] coeff: ", irow);

                ca += our_coeff->pipe_len;

                for (icol = 0; icol < our_coeff->pipe_len; icol++)
                {
                    fprintf(stderr, " %6u", our_coeff->coeff_array[icol]);
                }
                fprintf(stderr, "\n");
            }
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
        ARGPARSE_BLOCK          *apb,                // The args for this line of code
		VHT_CELL **vht,
		CSL_CELL *tdw,
        const char              *text,               // input string (null-safe!)
        int                      textlen,            //   how many bytes of input.
        int                      start_offset,       //     start tokenizing at this byte.
        VT_USERDEF_TOKENIZER    *user_tokenizer,     // the parsing regex (might be ignored)
        VT_USERDEF_COEFF_MATRIX *user_coeff,         // the pipeline coefficient control array, etc.
        crmhash_t               *features,           // where the output features go
        int                      featureslen,        //   how many output features (max)
        int                     *features_out        // how many feature-slots did we actually use up
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
    fprintf (stderr, "Vector tokenization summary: start %d len %d\n",
	     start_offset, textlen);


    //    We now have our parameters all set, and we can run the vector hashing.
    status = crm_vector_tokenize(text,
            textlen,
            start_offset,
            tokenizer,
            coeff,
            features,
            featureslen,
            1,
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
		VHT_CELL **vht,
		CSL_CELL *tdw,
        const char              *text,           // input string (null-safe!)
        int                      textlen,        //   how many bytes of input.
        int                      start_offset,   //     start tokenizing at this byte.
        VT_USERDEF_TOKENIZER    *tokenizer,      // the parsing regex (might be ignored)
        VT_USERDEF_COEFF_MATRIX *userdef_coeff,  // the pipeline coefficient control array, etc.
        crmhash_t               *features,       // where the output features go
        int                      featureslen,    //   how many output features (max)
        int                     *features_out    // how many feature-slots did we actually use up
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
    int output_stride = 1;
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
    fprintf (stderr, "Vector tokenization summary: start %ld len %ld\n",
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
      hash_vec0 = string1_coeff;
      hash_len0 = 8;
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
    fprintf (stderr, "Sext offset: %ld, length: %ld\n", start_offset, txtlen);

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
			   2,           //  stride 1 for 32-bit
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
			   2,           //  stride 1 for 32-bit
                features_out,
                next_offset);
    }
    return *features_out;
}


//  crm_vector_markov_1 gets the features of the markov H1 field

long crm_vector_markov_1
(
  char *txtptr,             // input string (null-safe!)  
  long txtstart,          //     start tokenizing at this byte.                
  long txtlen,           //   how many bytes of input.                        
  char *regex,            // the parsing regex (might be ignored)              
  long regexlen,          //   length of the parsing regex                     
  unsigned long *features,         // where the output features go             
  long featureslen,       //   how many output features (max)                  
  long *features_out,     // how many longs did we actually use up             
  long *next_offset       // next invocation should start at this offset       
 )
{


  return crm_vector_tokenize
    ( txtptr,
      txtstart,
      txtlen,
      regex,
      regexlen,
      markov1_coeff,
      5, 
      16,
      features,
      featureslen,
      2,                 //  stride 2 for 64-bit features
      features_out,
      next_offset );
}



//  crm_vector_markov_2 is the H2 field in the Markov classifier.
long crm_vector_markov_2
(
  char *txtptr,             // input string (null-safe!)  
  long txtstart,      //     start tokenizing at this byte.                
  long txtlen,           //   how many bytes of input.                        
  char *regex,            // the parsing regex (might be ignored)              
  long regexlen,          //   length of the parsing regex                     
  unsigned long *features,         // where the output features go             
  long featureslen,       //   how many output features (max)                  
  long *features_out,     // how many longs did we actually use up             
  long *next_offset       // next invocation should start at this offset       
 )
{

  return crm_vector_tokenize
    ( txtptr,
      txtstart,
      txtlen,
      regex,
      regexlen,
      markov2_coeff,
      5, 
      16,
      features,
      featureslen,
      2,                    // Stride 2 for 64-bit features
      features_out,
      next_offset );
}

//            vectorized OSB featureset generator.
//
long crm_vector_osb1
(
  char *txtptr,             // input string (null-safe!)
  long txtstart,      //     start tokenizing at this byte.                
  long txtlen,           //   how many bytes of input.                        
  char *regex,            // the parsing regex (might be ignored)              
  long regexlen,          //   length of the parsing regex                     
  unsigned long *features,         // where the output features go             
  long featureslen,       //   how many output features (max)                  
  long *features_out,     // how many longs did we actually use up             
  long *next_offset       // next invocation should start at this offset       
 )
{

  return crm_vector_tokenize
    ( txtptr,
      txtstart,
      txtlen,
      regex,
      regexlen,
      osb1_coeff,
      OSB_BAYES_WINDOW_LEN, 
      4,  // should be 4
      features,
      featureslen,
      2,
      features_out,
      next_offset );
}

long crm_vector_osb2
(
  char *txtptr,             // input string (null-safe!)       
  long txtstart,      //     start tokenizing at this byte.                
  long txtlen,           //   how many bytes of input.                        
  char *regex,            // the parsing regex (might be ignored)              
  long regexlen,          //   length of the parsing regex                     
  unsigned long *features,         // where the output features go             
  long featureslen,       //   how many output features (max)                  
  long *features_out,     // how many longs did we actually use up             
  long *next_offset       // next invocation should start at this offset       
 )
{

  return crm_vector_tokenize
    ( txtptr,
      txtstart,
      txtlen,
      regex,
      regexlen,
      osb2_coeff,
      OSB_BAYES_WINDOW_LEN, 
      4,  // should be 4
      features,
      featureslen,
      2,
      features_out,
      next_offset );
}


//            vectorized string kernel featureset generator.
//
long crm_vector_string_kernel1
(
  char *txtptr,             // input string (null-safe!) 
  long txtstart,      //     start tokenizing at this byte.                
  long txtlen,           //   how many bytes of input.                        
  long string_kern_len,   //   length of the kernel (must be < 16)
  unsigned long *features, // where the output features go             
  long featureslen,       //   how many output features (max)                  
  long *features_out,     // how many longs did we actually use up             
  long *next_offset       // next invocation should start at this offset       
 )
{

  //    The coeffs should be relatively prime.  Relatively...

  if (string_kern_len > 15) string_kern_len = 15;

  return crm_vector_tokenize
    ( txtptr,
      txtstart,
      txtlen,
      ".",     // regex
      1,       // regexlen
      string1_coeff,
      string_kern_len, //  how many coeffs to use
      1,               //  how many variations (just one)
      features,
      featureslen,
      1,
      features_out,
      next_offset );
}

long crm_vector_string_kernel2
(
  char *txtptr,             // input string (null-safe!)  
  long txtstart,      //     start tokenizing at this byte.                
  long txtlen,           //   how many bytes of input.                        
  long string_kern_len,   //   length of the kernel (must be < 16)
  unsigned long *features, // where the output features go             
  long featureslen,       //   how many output features (max)                  
  long *features_out,     // how many longs did we actually use up             
  long *next_offset       // next invocation should start at this offset       
 )
{

  //    The coeffs should be relatively prime.  Relatively...

  if (string_kern_len > 15) string_kern_len = 15;

  return crm_vector_tokenize
    ( txtptr,
      txtstart,
      txtlen,
      ".",     // regex
      1,       // regexlen
      string2_coeff,
      string_kern_len, //  how many coeffs to use
      1,               //  how many variations (just one)
      features,
      featureslen,
      1,
      features_out,
      next_offset );
}



#endif



