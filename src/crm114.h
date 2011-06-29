//  crm114.h  - Controllable Regex Mutilator base declarations, version X0.1
//  Copyright William S. Yerazunis, all rights reserved.
//  
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 1.0.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org .  Other licenses may be negotiated; contact the 
//  author for details.  

//
//    Global variables

//   The VHT (Variable Hash Table)     
VHT_CELL **vht;      

//   The pointer to the global Current Stack Level (CSL) frame
CSL_CELL *csl;

//    the data window
CSL_CELL *cdw;

//    the temporarys data window (where argv, environ, newline etc. live)
CSL_CELL *tdw;

//    the pointer to a CSL that we use during matching.  This is flipped
//    to point to the right data window during matching.  It doesn't have
//    it's own data, unlike cdw and tdw.
CSL_CELL *mdw;

//    a pointer to the current statement argparse block.  This gets whacked
//    on every new statement.
ARGPARSE_BLOCK *apb;

//    the microcompiler
int crm_microcompiler (CSL_CELL *csl,
			       VHT_CELL **vht);

#define CRM_ENGINE_HERE (char*)__FILE__, (char *)__FUNCTION__, (unsigned)__LINE__ 
//  helper routine for untrappable errors
void untrappableerror (char *msg1, char *msg2); 
void untrappableerror5 (char *msg1, char *msg2, 
			char *filename, 
			char *function,
			unsigned lineno); 

//  helper routine for fatal errors
long fatalerror (char *msg1, char *msg2); 
long fatalerror5 (char *msg1, char *msg2, 
		  char *filename, 
		  char *function,
		  unsigned lineno); 

//  helper routine for nonfatal errors
long nonfatalerror (char *msg1, char *msg2); 
long nonfatalerror5 (char *msg1, char *msg2, 
		     char *filename, 
		     char *function, 
		     unsigned lineno ); 


//  hash function for variable tables
unsigned long strnhash (char *str, long len); 

//  string translate function - for the TRANSLATE function
long strntrn (
	      unsigned char *datastr,
	      long *datastrlen,
	      long maxdatastrlen,
	      unsigned char *fromstr,
	      long fromstrlen,
	      unsigned char *tostr,
	      long tostrlen,
	      long flags);


//   basic math evaluator top function
long strmath (char *buf, long inlen, long maxlen, long *retstat); 

//   basic math evaluator in RPN
long strpnmath (char *buf, long inlen, long maxlen, long *retstat); 

//   basic math evaluator in RPN
long stralmath (char *buf, long inlen, long maxlen, long *retstat); 
long stralmath_reduce (double *valstack, long *opstack, 
		       long *sp, char *outformat);

//   load a file with info in a partially filled out csl cell.
int crm_load_csl (CSL_CELL *csl);

//    alter a variable to another value (this is destructive!)
void crm_destructive_alter_variable (char *varname, char *newstr);
void crm_destructive_alter_nvariable (char *varname, long varlen,
				      char *newstr, long newlen);

//  setting a program label in the VHT
void crm_setvar (   
		 char *filename,        // file where first defined (or NULL)
		 int filedesc,        // filedesc of defining file (or NULL)
		 char *nametxt,         // block of text hosting variable name
		 long nstart,           // index into nametxt to start varname
		 long nlen,             // length of name 
		 char *valtxt,         // text block hosts the captured value
		 long vstart,          // index of start of cap. value
		 long vlen,            // length of captured value
		 long linenumber      // linenumber (if pgm, else -1)
		 );

//   put a variable and a value into the temporary area
void crm_set_temp_nvar (char *varname, char *value, long vallen);
void crm_set_temp_var (char *varname, char *value);

//   put a variable and a window-based value into the temp area
void crm_set_windowed_var (char *varname, 
			   char *text, 
			   long start, 
			   long len,
			   long stmtnum);

//   put a counted-length var and a data-window-based value into the temp area.
void crm_set_windowed_nvar (char *varname, 
			   long varlen,
			   char *valtext, 
			   long start, 
			   long len,
			   long stmtnum);

//    set a program label.
void crm_setpgmlabel ( long start, long end, long stmtnum );


//     preprocess the program... including fixing up semicolons.
int crm_preprocessor (CSL_CELL *csl, int flags);

void crm_break_statements (long ini, long nchars, CSL_CELL *csl);


//     suck on a stream and put it into a buffer.

char * crm_mapstream ( FILE *instream); // read from instream till
                              // it goes dry, putting result into a buffer.

//     actually execute a compiled CRM file
int crm_invoke ();

//     look up a variable line number (for GOTOs among other things)
long crm_lookupvarline (VHT_CELL **vht, char *text, long start, long len);


//      grab_delim_string looks thru char *in for the first occurrence
//      of delim[0].  It then looks for the next occurrence of delim[1],
//      (with an escape character of delim[2]), 
//      and copies the resulting string (without the delims) into res,
//      null-terminating the result.  At most reslen-1 charscters 
//      are copied, and at most inlen characters are checked.  The return
//      value of this function is the address of the closing delimiter in *in.
//
//      flags:  CRM_FIRST_CLOSE - first close delimiter found ends string.
//              CRM_LAST_CLOSE  - last close delimiter found ends string
//              CRM_COUNT_CLOSE - keep a count of open and close delims  NYI
//

char *grab_delimited_string (char *res, char *in, char *delim, 
			    long inlen, long reslen, long flags);


//   expand the variable in the input buffer (according to the :*: operator
long crm_expandvar (char *buf, long maxlen);


//   look up a vht cell, from a variable name.  Returns the VHT cell
//   it's either stored in, or ought to be stored in (i.e. check for a NULL
//   VHT cell before use).

long crm_vht_lookup (VHT_CELL **vht, char *vname, long vlen);


//     crm_extractflag - given an arbitrary string cmd (start/len)
//     with words delimited by spaces, and a second string "flag"
//     (start/len).
//
//      1) does "flag" exist in "cmd"?
//      2) if so, where?
//      3) what is the start/len of flag in cmd?
//      4) what is the arg _after_ flag (start/len)
//   
//     Return value - pointer to start of flag in cmd.  It's
//     unnecessary to return the length of flag, as we already know
//     what it is.  also modifies nextarg start and length.
 
long crm_extractflag (const char *cmd, long cmdl, const char *flag, long flagl,
		      long *next, long *nextl);

//      initialize the vht, insert some some useful variables
void crm_vht_init (int argc, char **argv);


//      Surgically lengthen or shorten a window.   The window pointed to by
//      mdw gets delta extra characters added or cut at "where".  If the
//      allocated length is not enough, additional space can be malloced.
//      Finally, the vht is fixed up so everything still points "correctly".
void crm_slice_and_splice_window ( CSL_CELL *mdw, long where, long delta);

//      Update the start and length of all captured variables whenever
//      the input buffer gets mangled.  Mangles are all expressed in
//      the form of a start point, and a delta.
void crm_updatecaptures (char *text, long loc, long delta);

//      A helper function to calculate what the proper changes are for
//      any marked point, given a dot and a delta on that dot.  (sl is
//      0 for a start, and 1 for an end mark).
long crm_mangle_offset ( long mark, long dot, long delta, long sl);

//      Possibly reclaim storage in the given zone.   
long crm_compress_tdw_section (char *oldtext, long oldstart, long oldend);

//      create a new .css file
int crm_create_cssfile(char *cssfile, long buckets, 
		       long major, long minor, long spectrum_start);


//      argslice - given a string, destructively slice it up on whitespace 
//      boundaries into an argv-like array, and return argc and argv[].
//      WARNING WARNING WARNING this is a destructive operation on the
//      input string!  argc and argv are now suitable for parsing in 
//      the usual ways.  Argc should be initialized with the length
//      of argv, it is clobbered with the number of args actually used.

long crm_argslice (char *input, int *argc, char **argv);

//    The magic flag parser.  Given a string of input, and the builtin 
//    crm_flags array, returns the flags that are set.
//
//      for each input[i], is it equal to some member of flag_string[j]?
//         if YES, then 
//                 out_code[i] gets the value of flag_code[j]
//                 count_code[j] gets incremented.
//         if NONE match, then out_code[j] is zero
//
//      This makes it easy to parse a flag set for presence
//      
//      Note that this is a long long- which limits us to no more than
//      64 discrete flags.
unsigned long long crm_flagparse (char *input, long inlen); //  the user input
		

//     get the next word in the input.  (note- the regex stops only when
//     one hits a NULL, which may yield a slightly bogus result.
long crm_nextword ( char *input,
		    long inlen,
		    long starthere,
		    long *start,
		    long *len);
		    
//   The big one - matching...
int crm_expr_match (CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//   the learner... in variant forms...
int crm_expr_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
			   char *txt, long start, long len);
int crm_expr_osb_bayes_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
			      char *txt, long start, long len);
int crm_expr_osb_neural_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
			       char *txt, long start, long len);
int crm_expr_correlate_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
			      char *txt, long start, long len);
int crm_expr_osb_winnow_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
			       char *txt, long start, long len);
int crm_expr_osb_hyperspace_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				   char *txt, long start, long len);
int crm_expr_bit_entropy_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				   char *txt, long start, long len);
int crm_expr_alt_bit_entropy_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				    char *txt, long start, long len);
int crm_expr_svm_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				    char *txt, long start, long len);
int crm_expr_sks_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				    char *txt, long start, long len);
int crm_expr_fscm_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				    char *txt, long start, long len);



//   The bigger one - classifying...
int crm_expr_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
			      char *txt, long start, long len);
int crm_expr_osb_bayes_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				 char *txt, long start, long len);
int crm_expr_osb_neural_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				  char *txt, long start, long len);
int crm_expr_correlate_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				 char *txt, long start, long len);
int crm_expr_osb_winnow_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				  char *txt, long start, long len);
int crm_expr_osb_hyperspace_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				      char *txt, long start, long len);
int crm_expr_bit_entropy_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				      char *txt, long start, long len);
int crm_expr_alt_bit_entropy_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				       char *txt, long start, long len);
int crm_expr_svm_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				       char *txt, long start, long len);
int crm_expr_sks_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				       char *txt, long start, long len);
int crm_expr_fscm_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb, 
				       char *txt, long start, long len);


//  surgically alter a variable
int crm_expr_alter (CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  EVAL - double-evaluate for indirectiion's sake.  Otherwise, it's just 
//   like ALTER    
int crm_expr_eval (CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  WINDOW - do a windowing operation on a variable
int crm_expr_window ( CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  ISOLATE - do an isolation
int crm_expr_isolate ( CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_isolate_this (long *vptr, 
		  char *nametext, long namestart, long namelen, 
		      char *valuetext, long valuestart, long valuelen);

//  INPUT - do input
int crm_expr_input  ( CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  OUTPUT - do an output 
int crm_expr_output  ( CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  SYSCALL - fork another process
int crm_expr_syscall ( CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  TRANSLATE - translate character sets
int crm_expr_translate ( CSL_CELL *csl, ARGPARSE_BLOCK *apb);


//      parse a CRM114 statement; this is mostly a setup routine for 
//     the generic parser.

int crm_statement_parse ( char *in, 
			  long slen,
			  ARGPARSE_BLOCK *apb);


//    and a genric parser routine for parsing a line according
//    to the type of qoting done.
int crm_generic_parse_line ( 
		    char *txt,       //   the start of the program line
		    long len,        //   how long is the line
		    char *schars,    //   characters that can "start" an arg
		    char *fchars,    //   characters that "finish" an arg
		    char *echars,    //   characters that escape in an arg
		    long maxargs,    //   howm many things to search for (max)
		    long *ftype,     //   type of thing found (index by schars)
		    long *fstart,    //   starting location of found arg
		    long *flen       //   length of found arg
		    );

//    and to avoid all the mumbo-jumbo, an easy way to get a copy of
//    an arg found by the declensional parser.
void crm_get_pgm_arg (char *to, long tolen, char *from, long fromlen) ;


//     crm execution-time debugging environment - an interpreter unto itself
//
long crm_debugger ();

//     expand a variable or string with known length (8-bit and null-safe)

long crm_nexpandvar (char *buf, long inlen, long maxlen);

//     execute a FAULT triggering.
long crm_trigger_fault (char *reason);

//     do an microgroom of a hashed file.
long crm_microgroom (FEATUREBUCKET_TYPE *h,
		     unsigned char *seen_features,
		     long hs, 
		     unsigned long hindex );
void crm_packcss (FEATUREBUCKET_TYPE *h,
		  unsigned char *seen_features,
		  long hs, long packstart, long packlen);
void crm_packseg (FEATUREBUCKET_TYPE *h, 
		  unsigned char *seen_features,
		  long hs, long packstart, long packlen);
//
//     and microgrooming for winnow files
long crm_winnow_microgroom (WINNOW_FEATUREBUCKET_STRUCT *h, 
			    unsigned char *seen_features , 
			    unsigned long hfsize, 
			    unsigned long hindex);

void crm_pack_winnow_css (WINNOW_FEATUREBUCKET_STRUCT *h, 
		  unsigned char* xhashes,
		  long hs, long packstart, long packlen);
void crm_pack_winnow_seg (WINNOW_FEATUREBUCKET_STRUCT *h, 
			 unsigned char* xhashes,
			 long hs, long packstart, long packlen);



//     print out timings of each statement
void crm_output_profile ( CSL_CELL *csl);

//     do basic math expressions
long crm_expr_math (char *instr, unsigned long inlen,
		    char *outstr, unsigned long max_outlen,
		    long *status_p);

//      var-expansion operators
//             simple (escapes and vars) expansion
long crm_nexpandvar (char *buf, long inlen, long maxlen);

//             complex (escapes, vars, strlens, and maths) expansion
long crm_qexpandvar (char *buf, long inlen, long maxlen, long *retstat);

//              generic (everything, as you want it, bitmasked) expansion
long crm_zexpandvar (char *buf, 
		     long inlen, 
		     long maxlen, 
		     long *retstat, 
		     long exec_bitmask);

//       Var-restriction operators  (do []-vars, like subscript and regex )
long crm_restrictvar ( char *boxstring,
		       long boxstrlen,
		       long *vht_idx,
		       char **outblock,
		       long *outoffset,
		       long *outlen,
		       char *errstr);


//      crm114-specific regex compilation

int crm_regcomp (regex_t *preg, char *regex, long regex1_len, int cflags);

int crm_regexec ( regex_t *preg, char *string, long string_len,
		 size_t nmatch, regmatch_t pmatch[], int eflags, 
		  char *aux_string);

size_t crm_regerror (int errocode, regex_t *preg, char *errbuf,
		     size_t errbuf_size);

void crm_regfree (regex_t *preg);

char * crm_regversion ();


//        Portable mmap/munmap
//


void *crm_mmap_file (char *filename,
		     long start,
		     long len,
		     long prot,
		     long mode,
		     long *actual_len);
		    
void crm_munmap_file (void *where);
void crm_munmap_file_internal ( void *map);
void crm_munmap_all ();
void crm_force_munmap_filename (char *filename);
void crm_force_munmap_addr (void *addr);

//    Some statistics functions
//
double crm_norm_cdf(double x);
double crm_log(double x);
double norm_pdf(double x);
double normalized_gauss(double x, double s);

