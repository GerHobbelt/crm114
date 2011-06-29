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

int crm_vector_tokenize
(
        const char          *text         // input string (null-safe!)
, int textlen                             //   how many bytes of input.
, int start_offset                        //     start tokenizing at this byte.
, const char          *regex              // the parsing regex (might be ignored)
, int regexlen                            //   length of the parsing regex
, const crmhash_t     *coeff_array        // the pipeline coefficient control array
, int pipe_len                            //  how long a pipeline (== coeff_array col height)
, int pipe_iters                          //  how many rows are there in coeff_array
, crmhash_t           *features           // where the output features go
, int featureslen                         //   how many output features (max)
, int features_stride                     //   Spacing (in words) between features
, int                 *features_out       // how many longs did we actually use up
, int                 *next_offset        // next invocation should start at this offset
)
{
    crmhash_t hashpipe[UNIFIED_WINDOW_LEN];  // the pipeline for hashes
    int keepgoing;                           // the loop controller
    regex_t regcb;                           // the compiled regex
    regmatch_t match[5];                     // we only care about the outermost match
    int i, j, k;                             // some handy index vars
    int regcomp_status;
    int text_offset;
    int irow, icol;
    char errortext[4096];

    //    now do the work.

    *features_out = 0;
    keepgoing = 1;
    j = 0;

    //    Compile the regex.
    if (regexlen)
    {
        regcomp_status = crm_regcomp(&regcb, regex, regexlen, REG_EXTENDED);
        if (regcomp_status > 0)
        {
            crm_regerror(regcomp_status, &regcb, errortext, 4096);
            nonfatalerror("Regular Expression Compilation Problem: "
                         , errortext);
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
        if (regexlen == 0)
        {
            k = 0;
            //         skip non-graphical characters
            match[0].rm_so = 0;
            while (!crm_isgraph(text[text_offset + match[0].rm_so])
                   && text_offset + match[0].rm_so < textlen)
                match[0].rm_so++;
            match[0].rm_eo = match[0].rm_so;
            while (crm_isgraph(text[text_offset + match[0].rm_eo])
                   && text_offset + match[0].rm_eo < textlen)
                match[0].rm_eo++;
            if (match[0].rm_so == match[0].rm_eo)
                k = 1;
        }
        else
        {
            k = crm_regexec(&regcb
                           , &text[text_offset]
                           , textlen - text_offset
                           , 5, match
                           , REG_EXTENDED, NULL);
        }


        //   Are we done?
        if (k == 0)
        {
            //   Not done,we have another token (the text in text[match[0].rm_so,
            //    of length match[0].rm_eo - match[0].rm_so size)

            //
            if (user_trace)
            {
                fprintf(stderr, "Token; k: %d T.O: %d len %d ( %d %d on >"
                       , k
                       , text_offset
                       , match[0].rm_eo - match[0].rm_so
                       , match[0].rm_so
                       , match[0].rm_eo);
                for (k = match[0].rm_so + text_offset;
                     k < match[0].rm_eo + text_offset;
                     k++)
                    fprintf(stderr, "%c", text[k]);
                fprintf(stderr, "< )\n");
            }

            //   Now slide the hashpipe up one slot, and stuff this new token
            //   into the front of the pipeline
            //
            // for (i = UNIFIED_WINDOW_LEN; i > 0; i--)  // GerH points out that
            //  hashpipe [i] = hashpipe[i-1];            //  this smashes stack
#if 0
            for (i = UNIFIED_WINDOW_LEN - 1; i > 0; i--)
                hashpipe[i] = hashpipe[i - 1];
#else
            memmove(&hashpipe[1], hashpipe
                   , sizeof(hashpipe) - sizeof(hashpipe[0]));
#endif

            hashpipe[0] = strnhash(&text[match[0].rm_so + text_offset]
                                  , match[0].rm_eo - match[0].rm_so);

            //    Now, for each row in the coefficient array, we create a
            //   feature.
            //
            for (irow = 0; irow < pipe_iters; irow++)
            {
                crmhash_t ihash = 0;

                for (icol = 0; icol < pipe_len; icol++)
                    ihash += hashpipe[icol] * coeff_array[(pipe_len * irow) + icol];

                //    Stuff the final ihash value into features array
                features[*features_out] = ihash;
                if (internal_trace)
                    fprintf(stderr
                           , "New Feature: %08lX at %d\n", (unsigned long)ihash, *features_out);
                *features_out += features_stride;
            }

            //   And finally move on to the next place in the input.
            //
            //  Move to end of current token.
            text_offset = text_offset + match[0].rm_eo;
            *next_offset = text_offset + 1;
        }
        else
        {
            //     Failed to match.  This is the end...
            keepgoing = 0;
            if (next_offset)
                *next_offset = match[0].rm_eo;
        }

        //    Check to see if we have space left to add more
        //    features assuming there are any left to add.
        if (*features_out + pipe_iters + 1 > featureslen)
        {
            keepgoing = 0;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   End of code section dual-licensed to Bill Yerazunis and Joe Langeway.
//
////////////////////////////////////////////////////////////////////////////

static const crmhash_t markov1_coeff[] =
{
    1, 0, 0, 0, 0
    , 1, 3, 0, 0, 0
    , 1, 0, 5, 0, 0
    , 1, 3, 5, 0, 0
    , 1, 0, 0, 11, 0
    , 1, 3, 0, 11, 0
    , 1, 0, 5, 11, 0
    , 1, 3, 5, 11, 0
    , 1, 0, 0, 0, 23
    , 1, 3, 0, 0, 23
    , 1, 0, 5, 0, 23
    , 1, 3, 5, 0, 23
    , 1, 0, 0, 11, 23
    , 1, 3, 0, 11, 23
    , 1, 0, 5, 11, 23
    , 1, 3, 5, 11, 23
};

static const crmhash_t markov2_coeff[] =
{
    7, 0, 0, 0, 0
    , 7, 13, 0, 0, 0
    , 7, 0, 29, 0, 0
    , 7, 13, 29, 0, 0
    , 7, 0, 0, 51, 0
    , 7, 13, 0, 51, 0
    , 7, 0, 29, 51, 0
    , 7, 13, 29, 51, 0
    , 7, 0, 0, 0, 101
    , 7, 13, 0, 0, 101
    , 7, 0, 29, 0, 101
    , 7, 13, 29, 0, 101
    , 7, 0, 0, 51, 101
    , 7, 13, 0, 51, 101
    , 7, 0, 29, 51, 101
    , 7, 13, 29, 51, 101
};

#ifdef JUST_FOR_REFERENCE

//    hctable is where the OSB coeffs came from - this is now just a
//    historical artifact - DO NOT USE THIS!!!
static const crmhash_t hctable[] =
{
    1, 7
    , 3, 13
    , 5, 29
    , 11, 51
    , 23, 101
    , 47, 203
    , 97, 407
    , 197, 817
    , 397, 1637
    , 797, 3277
};

#endif

static const crmhash_t osb1_coeff[] =
{
    1, 3, 0, 0, 0
    , 1, 0, 5, 0, 0
    , 1, 0, 0, 11, 0
    , 1, 0, 0, 0, 23
};

static const crmhash_t osb2_coeff[] =
{
    7, 13, 0, 0, 0
    , 7, 0, 29, 0, 0
    , 7, 0, 0, 51, 0
    , 7, 0, 0, 0, 101
};

static const crmhash_t string1_coeff[] =
{ 1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 49, 51 };

static const crmhash_t string2_coeff[] =
{ 51, 49, 43, 41, 37, 31, 29, 23, 19, 17, 13, 11, 7, 5, 3, 1 };

static const crmhash_t unigram_coeff[] =
{ 1 };


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
        ARGPARSE_BLOCK       *apb       // The args for this line of code
, const char           *text            // input string (null-safe!)
, int textlen                           //   how many bytes of input.
, int start_offset                      //     start tokenizing at this byte.
, const char           *regex           // the parsing regex (might be ignored)
, int regexlen                          //   length of the parsing regex
, const crmhash_t      *coeff_array     // the pipeline coefficient control array
, int pipe_len                          //  how long a pipeline (== coeff_array col length)
, int pipe_iters                        //  how many rows are there in coeff_array
, crmhash_t            *features        // where the output features go
, int featureslen                       //   how many output features (max)
, int                  *features_out    // how many longs did we actually use up
, int                  *next_offset     // next invocation should start at this offset
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
    int64_t classifier_flags;
    int featurebits;

    const crmhash_t *hash_vec0;
    int hash_len0;
    int hash_iters0;
    const crmhash_t *hash_vec1;
    int hash_len1;
    int hash_iters1;
    int output_stride;
    const char *my_regex;
    int my_regex_len;

    // For slash-embedded pipeline definitions.
    crmhash_t ca[UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT];

    const char *string_kern_regex = ".";
    int string_kern_regex_len = 1;

    //    Set up some clean initial values for the important parameters.
    //    Default is always the OSB featureset, 32-bit features.
    //
    classifier_flags = apb->sflags;
    featurebits = 32;
    hash_vec0 = osb1_coeff;
    hash_len0 = 5;
    hash_iters0 = 5;
    hash_vec1 = osb2_coeff;
    hash_len1 = 5;
    hash_iters1 = 5;
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
        //     Build a 64-bit interleaved feature set.
        featurebits = 64;
        output_stride = 2;
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
        if (!my_regex)
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
    //
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
        s2len = crm_nexpandvar(s2text, s2len, MAX_PATTERN);

        if (s2len > 0)
        {
            //   Compile up the regex to find the vector tokenizer weights
            crm_regcomp
            (&regcb, vt_weight_regex, strlen(vt_weight_regex)
            , REG_ICASE | REG_EXTENDED);

            //   Use the regex to find the vector tokenizer weights
            regex_status =  crm_regexec(&regcb
                                       , s2text
                                       , s2len
                                       , 5
                                       , match
                                       , REG_EXTENDED
                                       , NULL);

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
    }

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
    if (output_stride == 1)
    {
        crm_vector_tokenize(
                text
                           , textlen
                           , start_offset
                           , my_regex
                           , my_regex_len
                           , hash_vec0
                           , hash_len0
                           , hash_iters0
                           , features
                           , featureslen
                           , output_stride          //  stride 1 for 32-bit
                           , features_out
                           , next_offset);
    }
    else
    {
        //        We're doing the 64-bit-long features for Markov/OSB
        crm_vector_tokenize(
                text
                           , textlen
                           , start_offset
                           , my_regex
                           , my_regex_len
                           , hash_vec0
                           , hash_len0
                           , hash_iters0
                           , features
                           , featureslen
                           , output_stride          //  stride 2 for 64-bit, offset 0
                           , features_out
                           , next_offset);
        crm_vector_tokenize(
                text
                           , textlen
                           , start_offset
                           , my_regex
                           , my_regex_len
                           , hash_vec1
                           , hash_len1
                           , hash_iters1
                           , &features[1]
                           , featureslen
                           , output_stride          //  stride 2 for 64-bit, offset 0
                           , features_out
                           , next_offset);
    }
    return *features_out;
}


//  crm_vector_markov_1 gets the features of the markov H1 field

int crm_vector_markov_1
(
        const char          *text        // input string (null-safe!)
, int textlen                            //   how many bytes of input.
, int start_offset                       //     start tokenizing at this byte.
, const char          *regex             // the parsing regex (might be ignored)
, int regexlen                           //   length of the parsing regex
, crmhash_t           *features          // where the output features go
, int featureslen                        //   how many output features (max)
, int                 *features_out      // how many longs did we actually use up
, int                 *next_offset       // next invocation should start at this offset
)
{
    return crm_vector_tokenize
           (text
           , textlen
           , start_offset
           , regex
           , regexlen
           , markov1_coeff
           , 5
           , 16
           , features
           , featureslen
           , 2           //  stride 2 for 64-bit features
           , features_out
           , next_offset);
}



//  crm_vector_markov_2 is the H2 field in the Markov classifier.
int crm_vector_markov_2
(
        const char          *text        // input string (null-safe!)
, int textlen                            //   how many bytes of input.
, int start_offset                       //     start tokenizing at this byte.
, const char          *regex             // the parsing regex (might be ignored)
, int regexlen                           //   length of the parsing regex
, crmhash_t           *features          // where the output features go
, int featureslen                        //   how many output features (max)
, int                 *features_out      // how many longs did we actually use up
, int                 *next_offset       // next invocation should start at this offset
)
{
    return crm_vector_tokenize
           (text
           , textlen
           , start_offset
           , regex
           , regexlen
           , markov2_coeff
           , 5
           , 16
           , features
           , featureslen
           , 2              // Stride 2 for 64-bit features
           , features_out
           , next_offset);
}

//            vectorized OSB featureset generator.
//
int crm_vector_osb1
(
        const char          *text        // input string (null-safe!)
, int textlen                            //   how many bytes of input.
, int start_offset                       //     start tokenizing at this byte.
, const char          *regex             // the parsing regex (might be ignored)
, int regexlen                           //   length of the parsing regex
, crmhash_t           *features          // where the output features go
, int featureslen                        //   how many output features (max)
, int                 *features_out      // how many longs did we actually use up
, int                 *next_offset       // next invocation should start at this offset
)
{
    return crm_vector_tokenize
           (text
           , textlen
           , start_offset
           , regex
           , regexlen
           , osb1_coeff
           , OSB_BAYES_WINDOW_LEN
           , 5
           , features
           , featureslen
           , 2
           , features_out
           , next_offset);
}

int crm_vector_osb2
(
        const char          *text        // input string (null-safe!)
, int textlen                            //   how many bytes of input.
, int start_offset                       //     start tokenizing at this byte.
, const char          *regex             // the parsing regex (might be ignored)
, int regexlen                           //   length of the parsing regex
, crmhash_t           *features          // where the output features go
, int featureslen                        //   how many output features (max)
, int                 *features_out      // how many longs did we actually use up
, int                 *next_offset       // next invocation should start at this offset
)
{
    return crm_vector_tokenize
           (text
           , textlen
           , start_offset
           , regex
           , regexlen
           , osb2_coeff
           , OSB_BAYES_WINDOW_LEN
           , 5
           , features
           , featureslen
           , 2
           , features_out
           , next_offset);
}


//            vectorized string kernel featureset generator.
//
int crm_vector_string_kernel1
(
        const char          *text         // input string (null-safe!)
, int textlen                             //   how many bytes of input.
, int start_offset                        //     start tokenizing at this byte.
, int string_kern_len                     //   length of the kernel (must be < 16)
, crmhash_t           *features           // where the output features go
, int featureslen                         //   how many output features (max)
, int                 *features_out       // how many longs did we actually use up
, int                 *next_offset        // next invocation should start at this offset
)
{
    //    The coeffs should be relatively prime.  Relatively...

    if (string_kern_len > 15)
        string_kern_len = 15;

    return crm_vector_tokenize
           (text
           , textlen
           , start_offset
           , "." // regex
           , 1   // regexlen
           , string1_coeff
           , string_kern_len //  how many coeffs to use
           , 1               //  how many variations (just one)
           , features
           , featureslen
           , 1
           , features_out
           , next_offset);
}

int crm_vector_string_kernel2
(
        const char          *text         // input string (null-safe!)
, int textlen                             //   how many bytes of input.
, int start_offset                        //     start tokenizing at this byte.
, int string_kern_len                     //   length of the kernel (must be < 16)
, crmhash_t           *features           // where the output features go
, int featureslen                         //   how many output features (max)
, int                 *features_out       // how many longs did we actually use up
, int                 *next_offset        // next invocation should start at this offset
)
{
    //    The coeffs should be relatively prime.  Relatively...

    if (string_kern_len > 15)
        string_kern_len = 15;

    return crm_vector_tokenize
           (text
           , textlen
           , start_offset
           , "." // regex
           , 1   // regexlen
           , string2_coeff
           , string_kern_len //  how many coeffs to use
           , 1               //  how many variations (just one)
           , features
           , featureslen
           , 1
           , features_out
           , next_offset);
}







