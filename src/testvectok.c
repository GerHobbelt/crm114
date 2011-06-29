//  testvectok.c  - Controllable Regex Mutilator,  version v1.0
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

//  include the routine declarations file
#include "crm114.h"

//  and include OSBF declarations
#include "crm114_osbf.h"

//
//    Global variables


#define USE_DEFAULT_RE_AND_COEFF 1


/* [i_a] no variable instantiation in a common header file */
int vht_size = 0;

int cstk_limit = 0;

int max_pgmlines = 0;

int max_pgmsize = 0;

int user_trace = 0;

int internal_trace = 0;

int debug_countdown = 0;

int cmdline_break = 0;

int cycle_counter = 0;

int ignore_environment_vars = 0;

int data_window_size = 0;

int sparse_spectrum_file_length = 0;

int microgroom_chain_length = 0;

int microgroom_stop_after = 0;

double min_pmax_pmin_ratio = 0.0;

int profile_execution = 0;

int prettyprint_listing = 0;  //  0= none, 1 = basic, 2 = expanded, 3 = parsecode

int engine_exit_base = 0;  //  All internal errors will use this number or higher;
//  the user programs can use lower numbers freely.


//        how should math be handled?
//        = 0 no extended (non-EVAL) math, use algebraic notation
//        = 1 no extended (non-EVAL) math, use RPN
//        = 2 extended (everywhere) math, use algebraic notation
//        = 3 extended (everywhere) math, use RPN
int q_expansion_mode = 0;

int selected_hashfunction = 0;  //  0 = default




//   The VHT (Variable Hash Table)
VHT_CELL **vht = NULL;

//   The pointer to the global Current Stack Level (CSL) frame
CSL_CELL *csl = NULL;

//    the data window
CSL_CELL *cdw = NULL;

//    the temporarys data window (where argv, environ, newline etc. live)
CSL_CELL *tdw = NULL;

//    the pointer to a CSL that we use during matching.  This is flipped
//    to point to the right data window during matching.  It doesn't have
//    it's own data, unlike cdw and tdw.
CSL_CELL *mdw = NULL;

////    a pointer to the current statement argparse block.  This gets whacked
////    on every new statement.
//ARGPARSE_BLOCK *apb = NULL;




//    the app path/name
char *prog_argv0 = NULL;


//    the auxilliary input buffer (for WINDOW input)
char *newinputbuf = NULL;

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
char *inbuf = NULL;
char *outbuf = NULL;
char *tempbuf = NULL;


#if !defined (CRM_WITHOUT_BMP_ASSISTED_ANALYSIS)
CRM_ANALYSIS_PROFILE_CONFIG analysis_cfg = {0};
#endif /* CRM_WITHOUT_BMP_ASSISTED_ANALYSIS */







static int crm_vector_tokenize_selector_old
(
 ARGPARSE_BLOCK *apb,     // The args for this line of code
 char *text,             // input string (null-safe!)                         
 int textlen,           //   how many bytes of input.                        
 int start_offset,      //     start tokenizing at this byte.                
 const char *regex,            // the parsing regex (might be ignored)              
 int regexlen,          //   length of the parsing regex                     
 const int *coeff_array,      // the pipeline coefficient control array            
 int pipe_len,          //  how int a pipeline (== coeff_array row length)  
 int pipe_iters,        //  how many rows are there in coeff_array           
 crmhash_t *features,         // where the output features go             
 int featureslen,       //   how many output features (max)                  
 int *features_out,     // how many longs did we actually use up             
 int *next_offset       // next invocation should start at this offset   
 );


/*
 * memory cleanup routine which is called at the end of the crm114 run.
 *
 * Note: this routine *also* called when an error occurred (e.g. out of memory)
 *    so tread carefully here: do not assume all these pointers are filled.
 */
static void crm_final_cleanup(void)
{
    // GROT GROT GROT
    //
    // move every malloc/free to use xmalloc/xcalloc/xrealloc/xfree, so we can be sure
    // [x]free() will be able to cope with NULL pointers as it is.

    crm_munmap_all();

    free_hash_table(vht, vht_size);
    vht = NULL;
    //free_arg_parseblock(apb);
    //apb = NULL;

    free_regex_cache();
    cleanup_expandvar_allocations();

    free(newinputbuf);
    newinputbuf = NULL;
    free(inbuf);
    inbuf = NULL;
    free(outbuf);
    outbuf = NULL;
    free(tempbuf);
    tempbuf = NULL;

    free_debugger_data();

    crm_terminate_analysis(&analysis_cfg);

    cleanup_stdin_out_err_as_os_handles();
}


// from crm_vector_tokenize.c
int main(void)
{
    char input[1024];
    char arg[8192];
    char opts[1024];
    int i, j;
    int ret;
    int k;
    crmhash_t feavec[2048];
    ARGPARSE_BLOCK apb = { 0 };
    VT_USERDEF_TOKENIZER tokenizer = { 0 };
    VT_USERDEF_COEFF_MATRIX our_coeff = { 0 };
	int use_default_re_and_coeff = USE_DEFAULT_RE_AND_COEFF;

    char my_regex[256];

    static const int coeff[] =
    {
        1, 3, 0, 0, 0,
        1, 0, 5, 0, 0,
        1, 0, 0, 11, 0,
        1, 0, 0, 0, 23
    };




    init_stdin_out_err_as_os_handles();
#if 0
    setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));
    setvbuf(stderr, stderr_buf, _IOFBF, sizeof(stderr_buf));
#endif

#if (defined (WIN32) || defined (_WIN32) || defined (_WIN64) || defined (WIN64)) && defined (_DEBUG)
    /*
     * Hook in our client-defined reporting function.
     * Every time a _CrtDbgReport is called to generate
     * a debug report, our function will get called first.
     */
    _CrtSetReportHook(crm_dbg_report_function);

    /*
     * Define the report destination(s) for each type of report
     * we are going to generate.  In this case, we are going to
     * generate a report for every report type: _CRT_WARN,
     * _CRT_ERROR, and _CRT_ASSERT.
     * The destination(s) is defined by specifying the report mode(s)
     * and report file for each report type.
     */
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    // Store a memory checkpoint in the s1 memory-state structure
    _CrtMemCheckpoint(&crm_memdbg_state_snapshot1);

    atexit(crm_report_mem_analysis);

    // Get the current bits
    i = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    // Set the debug-heap flag so that freed blocks are kept on the
    // linked list, to catch any inadvertent use of freed memory
#if 0
    i |= _CRTDBG_DELAY_FREE_MEM_DF;
#endif

    // Set the debug-heap flag so that memory leaks are reported when
    // the process terminates. Then, exit.
    //i |= _CRTDBG_LEAK_CHECK_DF;

    // Clear the upper 16 bits and OR in the desired freqency
    //i = (i & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;

    i |= _CRTDBG_CHECK_ALWAYS_DF;

    // Set the new bits
    _CrtSetDbgFlag(i);

//    // set a malloc marker we can use it in the leak dump at the end of the program:
//    (void)_calloc_dbg(1, 1, _CLIENT_BLOCK, __FILE__, __LINE__);
#endif

    //  fprintf(stderr, " args: %d \n", argc);
    //  for (i = 0; i < argc; i++)
    //    fprintf(stderr, " argi: %d, argv: %s \n", i, argv[i]);

    atexit(crm_final_cleanup);

#if defined (HAVE__SET_OUTPUT_FORMAT)
    _set_output_format(_TWO_DIGIT_EXPONENT);     // force MSVC (& others?) to produce floating point %f with 2 digits for power component instead of 3 for easier comparison with 'knowngood'.
#endif

    // force MSwin/Win32 console I/O into binary mode: treat \r\n and \n as completely different - like it is on *NIX boxes!
#if defined (HAVE__SETMODE) && defined (HAVE__FILENO) && defined (O_BINARY)
    (void)_setmode(_fileno(crm_stdin), O_BINARY);
    (void)_setmode(_fileno(crm_stdout), O_BINARY);
    (void)_setmode(_fileno(crm_stderr), O_BINARY);
#endif






    user_trace = 1;

    internal_trace = 1;

	do
	{
		strcpy(my_regex, "[[:alpha:]]+");
		memset(&tokenizer, 0, sizeof(tokenizer));
		memset(&our_coeff, 0, sizeof(our_coeff));

		fprintf(stdout, "Enter a test string: ");
		fgets(input, sizeof(input), stdin);
		input[sizeof(input) - 1] = 0;
		fprintf(stdout, "Input = '%s'\n", input);
		// fscanf(stdin, "%1023s", input);
		// fprintf(stdout, "Input = '%s'\n", input);

		fprintf(stdout, "Enter optional 'vector: ...' arg (don't forget the 'vector: prefix in there!): ");
		fgets(arg, sizeof(arg), stdin);
		arg[sizeof(arg) - 1] = 0;
		fprintf(stdout, "Args = '%s'\n", arg);

		apb.s1start = my_regex;
		apb.s1len = (int)strlen(my_regex);

		apb.s2start = arg;
		apb.s2len = (int)strlen(arg);

		fprintf(stdout, "Optional OSBF style token globbing: type integer values for max_token_size and count (must specify both!): ");
		fgets(opts, sizeof(opts), stdin);
		opts[sizeof(opts) - 1] = 0;
		k = sscanf(opts, "%d %d", &i, &j);
		if (k == 2)
		{
			fprintf(stdout, "using max_token_size %d and count %d.\n", i, j);

			tokenizer.max_token_length = i;
			tokenizer.max_big_token_count = j;
		}

		tokenizer.regex = my_regex;
		tokenizer.regexlen = (int)strlen(my_regex);

		if (strlen(arg) < 3)
		{
			memcpy(our_coeff.coeff_array, coeff, sizeof(coeff));
			our_coeff.output_stride = 1;
			our_coeff.pipe_iters = 4;
			our_coeff.pipe_len = 5;
		}

		memset(feavec, 0, sizeof(feavec));
		tokenizer.input_next_offset = 0;
		ret = crm_vector_tokenize_selector(&apb,
			vht,
			tdw,
				input,
				(int)strlen(input),
				0,
				(use_default_re_and_coeff ? NULL : &tokenizer),
				(use_default_re_and_coeff ? NULL : &our_coeff),
				feavec,
				WIDTHOF(feavec),
				&j);

		for (k = 0; k < j; k++)
		{
			fprintf(stdout, "feature[%4d] = %12lu (%08lX)\n", k, (unsigned long int)feavec[k], (unsigned long int)feavec[k]);
		}

		fprintf(stdout, "... and next_offset is %d\n", tokenizer.input_next_offset);

		tokenizer.input_next_offset = 0;
		memset(feavec, 0, sizeof(feavec));
		ret = crm_vector_tokenize_selector_old(&apb,
				input,
				(int)strlen(input),
				0,
				(use_default_re_and_coeff ? NULL : my_regex),
				(use_default_re_and_coeff ? 0 : (int)strlen(my_regex)),
				(use_default_re_and_coeff ? NULL : coeff),
				(use_default_re_and_coeff ? 0 : 5),
				(use_default_re_and_coeff ? 0 : 4),
				feavec,
				WIDTHOF(feavec),
				&j, 
				&tokenizer.input_next_offset);

		for (k = 0; k < j; k++)
		{
			fprintf(stdout, "feature[%4d] = %12lu (%08lX)\n", k, (unsigned long int)feavec[k], (unsigned long int)feavec[k]);
		}

		fprintf(stdout, "... and next_offset is %d\n", tokenizer.input_next_offset);
	
		fprintf(stdout, "Another round? (enter 'y' for yes): ");
		fgets(input, sizeof(input), stdin);
		input[sizeof(input) - 1] = 0;
	} while (input[0] == 'y');
	
	return ret >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}







// just copied here to make this bugger compile ASAP:


void free_stack_item(CSL_CELL *csl)
{
    if (!csl)
        return;

    if (csl->mct && csl->mct_allocated)
    {
        int i;

        for (i = 0; i < csl->mct_size; i++)
        {
            MCT_CELL *cp = csl->mct[i];

            if (cp != NULL)
            {
#if !FULL_PARSE_AT_COMPILE_TIME
                free(cp->apb);
                cp->apb = NULL;
#endif
                // free(cp->hosttxt);
                free(cp);
                csl->mct[i] = NULL;
            }
        }
        free(csl->mct);
        csl->mct = NULL;
    }

    if (csl->filename_allocated)
    {
        free(csl->filename);
    }
    csl->filename = NULL;
    if (csl->filetext_allocated)
    {
        free(csl->filetext);
    }
    csl->filetext = NULL;
    free(csl);
}

















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

static int crm_vector_tokenize_old
(
   char *text,             // input string (null-safe!)
   int textlen,           //   how many bytes of input.
   int start_offset,      //     start tokenizing at this byte.
   const char *regex,            // the parsing regex (might be ignored)
   int regexlen,          //   length of the parsing regex
   const int *coeff_array,      // the pipeline coefficient control array
   int pipe_len,          //  how long a pipeline (== coeff_array row length)
   int pipe_iters,        //  how many rows are there in coeff_array
   crmhash_t *features, // where the output features go
   int featureslen,       //   how many output features (max)
   int features_stride,   //   Spacing (in words) between features
   int *features_out,     // how many longs did we actually use up
   int *next_offset       // next invocation should start at this offset
   )
{
  int hashpipe[UNIFIED_WINDOW_LEN];    // the pipeline for hashes
  int keepgoing;                       // the loop controller
  regex_t regcb;                    // the compiled regex
  regmatch_t match[5];              // we only care about the outermost match
  int i, j, k;             // some handy index vars
  int regcomp_status;
  int text_offset;
  int irow, icol;
  crmhash_t ihash;
  char errortext[4096];

  //    now do the work.

  *features_out = 0;
  keepgoing = 1;
  j = 0;

  //    Compile the regex.   
  if (regexlen)
    {
      regcomp_status = crm_regcomp (&regcb, regex, regexlen, REG_EXTENDED);
      if (regcomp_status > 0)
	{
	  crm_regerror (regcomp_status, &regcb, errortext, 4096);
	  nonfatalerror("Regular Expression Compilation Problem: ",
			  errortext);
	  return (-1);
	};
    };

  // fill the hashpipe with initialization
  for (i = 0; i < UNIFIED_WINDOW_LEN; i++)
    hashpipe[i] = 0xDEADBEEF ;
  
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
          //         skip non-graphical characthers 
	  match[0].rm_so = 0;
          while (!crm_isgraph (text [text_offset + match[0].rm_so])
                 && text_offset + match[0].rm_so < textlen)
            match[0].rm_so ++;
          match[0].rm_eo = match[0].rm_so;
          while (crm_isgraph (text [text_offset + match[0].rm_eo])
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
      if ( k == 0 )
	{
	  //   Not done,we have another token (the text in text[match[0].rm_so,
	  //    of length match[0].rm_eo - match[0].rm_so size)
	  
	  //
	  if (user_trace)
	  {
	    fprintf (stderr, "Token; k: %d T.O: %d len %d ( %d %d on >",
		     k,
		     text_offset,
		     match[0].rm_eo - match[0].rm_so,
		     match[0].rm_so,
		     match[0].rm_eo);
	    for (k = match[0].rm_so+text_offset; 
		 k < match[0].rm_eo+text_offset; 
		 k++)
	      fprintf (stderr, "%c", text[k]);
	    fprintf (stderr, "< )\n");
	  };
	  
	  //   Now slide the hashpipe up one slot, and stuff this new token
	  //   into the front of the pipeline
	  //
	  // for (i = UNIFIED_WINDOW_LEN; i > 0; i--)  // GerH points out that
	  //  hashpipe [i] = hashpipe[i-1];            //  this smashes stack
	  memmove (& hashpipe [1], hashpipe, 
		   sizeof (hashpipe) - sizeof (hashpipe[0]) );
 
	  hashpipe[0] = strnhash( &text[match[0].rm_so+text_offset], 
				  match[0].rm_eo - match[0].rm_so);
	  
	  //    Now, for each row in the coefficient array, we create a
	  //   feature.
	  //    
	  for (irow = 0; irow < pipe_iters; irow++)
	    {
	      ihash = 0;
	      for (icol = 0; icol < pipe_len; icol++)
		ihash = ihash + 
		  hashpipe[icol] * coeff_array[ (pipe_len * irow) + icol];
	      
	      //    Stuff the final ihash value into reatures array
	      features[*features_out] = ihash;
	      if (internal_trace)
		fprintf (stderr, 
			 "New Feature: %lx at %d\n",(unsigned long int)ihash, *features_out);
	      *features_out = *features_out + features_stride ;
	    };
	  
	  //   And finally move on to the next place in the input.
	  //   
	  //  Move to end of current token.
	  text_offset = text_offset + match[0].rm_eo;
	}
      else
	//     Failed to match.  This is the end...
	{
	  keepgoing = 0;
	};
      
      //    Check to see if we have space left to add more 
      //    features assuming there are any left to add.
      if ( *features_out + pipe_iters + 3 > featureslen)
	{
	  keepgoing = 0;
	}

    };
  if (next_offset)
    *next_offset = text_offset + match[0].rm_eo;
  features[*features_out] = 0;
  features[*features_out+1] = 0; 
  return (0);
}

///////////////////////////////////////////////////////////////////////////
//
//   End of code section dual-licensed to Bill Yerazunis and Joe Langeway.
//
////////////////////////////////////////////////////////////////////////////

static int markov1_coeff [] =
  { 1, 0, 0, 0, 0,
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
    1, 3, 5, 11, 23 };

static int markov2_coeff [] =
  { 7, 0, 0, 0, 0,
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
    7, 13, 29, 51, 101 };

#ifdef JUST_FOR_REFERENCE
//    hctable is where the OSB coeffs came from- this is now just a
//    historical artifact - DO NOT USE THIS!!!
static int hctable[] =
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
#endif

static int osb1_coeff [] =
  { 1, 3, 0, 0, 0, 
    1, 0, 5, 0, 0, 
    1, 0, 0, 11, 0,
    1, 0, 0, 0, 23};

static int osb2_coeff [] =
  { 7, 13, 0, 0, 0,
    7, 0, 29, 0, 0,
    7, 0, 0, 51, 0,
    7, 0, 0, 0, 101};

static int string1_coeff [] =
  { 1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 49, 51 };

static int string2_coeff [] =
  { 51, 49, 43, 41, 37, 31, 29, 23, 19, 17, 13, 11, 7, 5, 3, 1 };

static int unigram_coeff [] =
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

static int crm_vector_tokenize_selector_old
(
 ARGPARSE_BLOCK *apb,     // The args for this line of code
 char *text,             // input string (null-safe!)                         
 int textlen,           //   how many bytes of input.                        
 int start_offset,      //     start tokenizing at this byte.                
 const char *regex,            // the parsing regex (might be ignored)              
 int regexlen,          //   length of the parsing regex                     
 const int *coeff_array,      // the pipeline coefficient control array            
 int pipe_len,          //  how int a pipeline (== coeff_array row length)  
 int pipe_iters,        //  how many rows are there in coeff_array           
 crmhash_t *features,         // where the output features go             
 int featureslen,       //   how many output features (max)                  
 int *features_out,     // how many longs did we actually use up             
 int *next_offset       // next invocation should start at this offset   
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

  const int *hash_vec0;
  int hash_len0;
  int hash_iters0;
  const int *hash_vec1;
  int hash_len1;
  int hash_iters1;
  int output_stride;
  const char *my_regex;
  int my_regex_len;

  char s1text[MAX_PATTERN];
  int s1len;


 // For slash-embedded pipeline definitions.
  int ca[UNIFIED_WINDOW_LEN * UNIFIED_VECTOR_LIMIT]; 

  const char *string_kern_regex = ".";
  int string_kern_regex_len = 1;
  const char *fscm_kern_regex = ".";
  int fscm_kern_regex_len = 1;

  //    Set up some clean initial values for the important parameters.
  //    Default is always the OSB featureset, 32-bit features.
  //
  classifier_flags = apb->sflags;
  featurebits = 32;
  hash_vec0 = osb1_coeff;
  hash_len0 = OSB_BAYES_WINDOW_LEN;    // was 5
  hash_iters0 = 4; // should be 4
  hash_vec1 = osb2_coeff;
  hash_len1 = OSB_BAYES_WINDOW_LEN;     // was 5
  hash_iters1 = 4; // should be 4
  output_stride = 1;

  //    put in the passed-in regex values, if any.
  my_regex = regex;
  my_regex_len = regexlen;


  //    Now we can proceed to set up the work in a fairly linear way.

  //    If it's the Markov classifier, then different coeffs and a longer len
  if ( classifier_flags & CRM_MARKOVIAN)
    {
      hash_vec0 = markov1_coeff;
      hash_vec1 = markov2_coeff;
      hash_iters0 = hash_iters1 = 16;
    };

  //     If it's one of the 64-bit-key classifiers, then the featurebits
  //     need to be 64.
  if ( classifier_flags & CRM_MARKOVIAN 
       || classifier_flags & CRM_OSB
       || classifier_flags & CRM_WINNOW
       || classifier_flags & CRM_OSBF
       )
    {
      //     We're a 64-bit hash, so build a 64-bit interleaved feature set.
      featurebits = 64;
      output_stride = 2;
    };

  //       The new FSCM does in fact do tokeniation and hashing over
  //       a string kernel, but only for the indexing.  
  if (classifier_flags & CRM_FSCM)
    {
      hash_vec0 = string1_coeff;
      hash_len0 = 4;
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
	};
    };
  
  //     Do we want a string kernel?  If so, then we have to override
  //     a few things.
  if ( classifier_flags & CRM_STRING)
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
	};
    };

  //     Do we want a unigram system?  If so, then we change a few more
  //     things.
  if ( classifier_flags & CRM_UNIGRAM)
    {
      hash_vec0 = unigram_coeff;
      hash_len0 = 1;
      hash_iters0 = 1;
      hash_vec1 = unigram_coeff;
      hash_len1 = 1;
      hash_iters1 = 1;
    };
  

  //     Now all of the defaults have been filled in; we now see if the 
  //     caller has overridden any (or all!) of them.   We assume that the
  //     user who overrides them has pre-sanity-checked them as well.
  
  //     First check- did the user override the regex?

  //    Did the user program specify a first slash paramter?  (only
  //    override this if a regex was passed in)
  if (regexlen > 0)
    {
      crm_get_pgm_arg (s1text, MAX_PATTERN, apb->s1start, apb->s1len);
      s1len = apb->s1len;
      s1len = crm_nexpandvar (s1text, s1len, MAX_PATTERN, vht, tdw);
      my_regex = s1text;
      my_regex_len = s1len;
    };


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
    regmatch_t match[5];   //  We'll only care about the second match 
    local_pipe_len = 0;
    local_pipe_iters = 0;

    //     get the second slash parameter (if used at all)
    crm_get_pgm_arg (s2text, MAX_PATTERN, apb->s2start, apb->s2len);
    s2len = apb->s2len;
    s2len = crm_nexpandvar (s2text, s2len, MAX_PATTERN, vht, tdw);

    if (s2len > 0)
      {
	//   Compile up the regex to find the vector tokenizer weights
	crm_regcomp
	  (&regcb, vt_weight_regex, (int)strlen(vt_weight_regex),
	   REG_ICASE | REG_EXTENDED);
	
	//   Use the regex to find the vector tokenizer weights       
	regex_status =  crm_regexec (&regcb,
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
	    conv_ptr = & s2text[match[1].rm_so];
	    local_pipe_len = strtol (conv_ptr, &conv_ptr, 0);
	    if (local_pipe_len > UNIFIED_WINDOW_LEN)
	      {
		nonfatalerror ("You've specified a tokenizer pipe length "
			      "that is too long.", "  I'll trim it.");
		local_pipe_len = UNIFIED_WINDOW_LEN;
	      };
	    //fprintf (stderr, "local_pipe_len = %ld\n", local_pipe_len);
	    //   The second parameter is the number of repeats
	    local_pipe_iters = strtol (conv_ptr, &conv_ptr, 0);
	    if (local_pipe_iters > UNIFIED_VECTOR_LIMIT)
	      {
		nonfatalerror ("You've specified too high a tokenizer "
			      "iteration count.", "  I'll trim it.");
		local_pipe_iters = UNIFIED_VECTOR_LIMIT;
	      };
	    //fprintf (stderr, "pipe_iters = %ld\n", local_pipe_iters);

	    //    Now, get the coefficients.
	    for (i = 0; i < local_pipe_len * local_pipe_iters; i++)
	      {
		ca[i] = strtol (conv_ptr, &conv_ptr, 0);
		//  fprintf (stderr, "coeff: %ld\n", ca[i]);
	      };

	    //   If there was a numeric coeff array, use that, else
	    //   use our slash coeff array.
	    if (! coeff_array)
	      {
		coeff_array = ca;
		pipe_len = local_pipe_len;
		pipe_iters = local_pipe_iters;
	      };
	  };
	//  free the compiled regex.
	crm_regfree (&regcb);
      };
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
    };

  if (pipe_len > 0)
    {
      hash_len0 = pipe_len;
      hash_len1 = pipe_len;
    };

  if (pipe_iters > 0)
    {
      hash_iters0 = pipe_iters;
      hash_iters1 = pipe_iters;
    };

  //    We now have our parameters all set, and we can run the vector hashing.
  //
  if (output_stride == 1)
    {
      crm_vector_tokenize_old (
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
			   1,           //  stride 1 for 32-bit
			   features_out,
			   next_offset);
    }
  else
    {
      //        We're doing the 64-bit-long features for Markov/OSB
      crm_vector_tokenize_old (
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
			   1,           //  stride 1 for 32-bit
			   features_out,
			   next_offset);
      crm_vector_tokenize_old (
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
			   1,           //  stride 1 for 32-bit
			   features_out,
			   next_offset);
    };
    return (*features_out);
}




