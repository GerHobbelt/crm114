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
//    Vectorized stringhashing - get a bunch of features in a nice
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
//    yields "Classic CRM114" OSB features, and the 2x3 array 
//
//     {{1 1 0}
//      {1 0 1}}
//
//    yields bigrams that are not position nor order sensitive, while
//
//     {{1 2 0}
//      {1 0 2}}
//
//    yields bigrams that are order sensitive, but not position sensitive.
// 
//    Because the array elements are used as dot-product multipliers
//    on the hashed token value pipeline, there is a small advantage to
//    having the elements of the array being odd (low bit set) and
//    relatively prime, as it decreases the chance of hash collisions.
//
///////////////////////////////////////////////////////////////////////////

long crm_vector_stringhash 
(
   char *text,             // input string (null-safe!)
   long textlen,           //   how many bytes of input.
   long start_offset,      //     start tokenizing at this byte.
   char *regex,            // the parsing regex (might be ignored)
   long regexlen,          //   length of the parsing regex
   long *coeff_array,     // the pipeline coefficient control array
   long pipe_len,          //  how long a pipeline (== coeff_array row length)
   long pipe_iters,        //  how many rows are there in coeff_array
   unsigned long *features,         // where the output features go
   long featureslen,       //   how many output features (max)
   long *features_out,     // how many longs did we actually use up
   long *next_offset       // next invocation should start at this offset
   )
{
  long hashpipe[UNIFIED_WINDOW_LEN];    // the pipeline for hashes
  long keepgoing;                       // the loop controller
  regex_t regcb;                    // the compiled regex
  regmatch_t match[5];              // we only care about the outermost match
  long i, j, k;             // some handy index vars
  int regcomp_status;
  long text_offset;
  long irow, icol;
  unsigned long ihash;
  //    now do the work.

  *features_out = 0;
  keepgoing = 1;
  

  regcomp_status = crm_regcomp (&regcb, regex, regexlen, REG_EXTENDED);
 
  // fill the hashpipe with initialization
  for (i = 0; i < UNIFIED_WINDOW_LEN; i++)
    hashpipe[i] = 0xDEADBEEF ;



  //   Run the hashpipe.  
  text_offset = start_offset;
  while (keepgoing)
    {
      //  If the pattern is empty, assume non-graph-delimited tokens
      //  (supposedly an 8% speed gain over regexec)
      if (regexlen == 0)
	{
	  k = 0;
          //         skip non-graphical characthers 
	  match[0].rm_so = 0;
          while (!crm_isgraph(text [text_offset + match[0].rm_so])
                 && text_offset + match[0].rm_so < textlen)
            match[0].rm_so ++;
          match[0].rm_eo = match[0].rm_so;
          while (crm_isgraph(text [text_offset + match[0].rm_eo])
                 && text_offset + match[0].rm_eo < textlen)
            match[0].rm_eo ++;
          if ( match[0].rm_so == match[0].rm_eo)
            k = 1;
        }
      else
	{
	  k = crm_regexec (&regcb, 
			   &text[text_offset], 
			   textlen - text_offset,
			   5, match,
			   REG_EXTENDED, NULL);
	};


      //   Are we done?
      if ( k != 0 
	   || text_offset >= textlen 
	   || *features_out + pipe_iters + 1 > featureslen)
	{
	  keepgoing = 0;
	  if (next_offset)
	    *next_offset = match[0].rm_eo;
	}
      
      //   OK, now we have another token (the text in text[match[0].rm_so,
      //    of length match[0].rm_eo - match[0].rm_so size)

      //
      //if (user_trace)
	{
	  fprintf (stderr, "Match T.O: %d len %d (%d %d on >",
		   (int)text_offset,
		   match[0].rm_eo - match[0].rm_so,
		   match[0].rm_so,
		   match[0].rm_eo);
	  for (k = match[0].rm_so+text_offset; 
	       k < match[0].rm_eo+text_offset; 
	       k++)
	    fprintf (stderr, "%c", text[k]);
	  fprintf (stderr, "<\n");
	};

      //   Now slide the hashpipe up one slit, and stuff this new token
      //   into the front of the pipeline
      for (i = UNIFIED_WINDOW_LEN; i > 0; i--)
	hashpipe [i] = hashpipe[i-1];
      hashpipe[0] = strnhash( &text[match[0].rm_so+text_offset], 
				     match[0].rm_eo - match[0].rm_so);
      
      //    Now, for each row in the coefficient array, we create a feature.
      //    
      for (irow = 0; irow < pipe_iters; irow++)
	{
	  ihash = 0;
	  for (icol = 0; icol < pipe_len; icol++)
	    ihash = ihash + 
	      hashpipe[icol] * coeff_array[ (pipe_len * irow) + icol];
	  
	  //    Stuff the final ihash value into reatures array
	  features[*features_out] = ihash;
	  fprintf (stderr, "New Feature: %lx at %ld\n",ihash, *features_out);
	  *features_out = *features_out + 1 ;
	};
      
      //   And finally move on to the next place in the input.
      //   
      //  Move to end of current token.
      text_offset = text_offset + match[0].rm_eo;
    }
  return (0);
}

///////////////////////////////////////////////////////////////////////////
//
//   End of code section dual-licensed to Bill Yerazunis and Joe Langeway.
//
////////////////////////////////////////////////////////////////////////////

#define DUMMY_MAIN_TEST
#ifdef DUMMY_MAIN_TEST
//    
int main2()
{
  char input [1024];
  long i, j;
  unsigned long feavec [2048];

  char my_regex [256];

  long coeff[]= { 1, 3, 0, 0, 0,
                   1, 0, 5, 0, 0,
                   1, 0, 0, 11, 0,
                   1, 0, 0, 0, 23 } ;

  strcpy (my_regex, "[[:alpha:]]+");
  printf ("Enter a test string: ");
  scanf ("%128c", &input[0]);
  crm_vector_stringhash (
			 input,
			 strlen(input),
			 0,
			 my_regex,
			 strlen (my_regex),
			 coeff,
			 5,
			 4,
			 feavec,
			 2048,
			 & j,
			 & i);

  printf ("... and i is %ld\n", i);
  exit(0);
}

			 
#endif
    


