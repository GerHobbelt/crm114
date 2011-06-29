//  crm_expr_alter.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2006  William S. Yerazunis, all rights reserved.
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
*/



int crm_expr_eval (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  //      Here we evaluate the slash-string _repeatedly_, not just
  //      once as in ALTER.  
  //
  //      To prevent infinite loops (or at least many of them) we:
  //      1) strictly limit the total number of loop iterations to
  //         the compile-time parameter MAX_EVAL_ITERATIONS
  //      2) we also keep an array of the hashes of the last 256 values,
  //         if we see a repeat, we assume that it's a loop and we stop
  //         right there.
  
  char varname[MAX_VARNAME];
  long varnamelen = 0;
  long newvallen;
  unsigned long long ihash;
  unsigned long long ahash[MAX_EVAL_ITERATIONS];
  long ahindex;
  long itercount;
  long loop_abort;
  long qex_stat;
  long has_output_var;
  // should use tempbuf for this instead.
  //   char newstr [MAX_PATTERN];
  if (user_trace)
    fprintf (stderr, "Executing an EVALuation\n");
  
  qex_stat = 0;
  has_output_var = 1;

  //     get the variable name
  crm_get_pgm_arg (varname, MAX_VARNAME, apb->p1start, apb->p1len);
  if (apb->p1len < 3) 
    {
      has_output_var = 0;
      if (user_trace)
	fprintf (stderr, "There's no output var for this EVAL, so we won't "
		 "be assigning the result anywhere.\n  It better have a "
		 "relational test, or you're just wasting CPU.\n");
    }
  
  if (has_output_var)
    {
      //      do variable substitution on the variable name
      varnamelen = crm_nexpandvar (varname, apb->p1len, MAX_VARNAME);
      if (varnamelen < 3) 
	{
	  nonfatalerror (
			 "The variable you're asking me to alter has an utterly bogus name\n",
			 "so I'll pretend it has no output variable.");
	  has_output_var = 0;
	}
    }
  //     get the new pattern, and expand it.
  crm_get_pgm_arg (tempbuf, data_window_size, apb->s1start, apb->s1len);
  
  ihash = 0;
  itercount = 0;
  for (ahindex = 0; ahindex < MAX_EVAL_ITERATIONS; ahindex++)
    ahash[ahindex] = 0;
  ahindex = 0;
  loop_abort = 0;
  //
  //     Now, a loop - while it continues to change, keep looping.
  //     But to try and detect infinite loops, we keep track of the 
  //     previous values (actually, their hashes) and if one of those 
  //     values recur, we stop evaluating and throw an error.
  //
  newvallen = apb->s1len;
  while (itercount < MAX_EVAL_ITERATIONS 
	 && ! (loop_abort))
    {
      int i;
      itercount++;
      ihash = strnhash (tempbuf, newvallen);
      //   
      //     build a 64-bit hash by changing the initial conditions and
      //     by using all but two of the characters and by overlapping
      //     the results by two bits.  This is intentionally evil and 
      //     tangled.  Hopefully it will work.
      //
      if (newvallen > 3) 
	ihash = (ihash << 30) + strnhash (&tempbuf[1], newvallen - 2); 
      if (internal_trace)
	fprintf (stderr, "Eval ihash = %lld\n", ihash);
    for (i = 0;  i < itercount; i++)
	  {
	  assert(i < MAX_EVAL_ITERATIONS);
  	if (ahash[i] == ihash)
	  {
	    loop_abort = 1;
	    if ( i != itercount - 1)
	      loop_abort = 2;
	  }
	  }
	  /* assert(i < MAX_EVAL_ITERATIONS); ** [i_a] this one was triggered during the infiniteloop test */
	  if (i < MAX_EVAL_ITERATIONS)
	{
      ahash[i] = ihash;
	}
      newvallen = crm_qexpandvar (tempbuf, newvallen, 
				  data_window_size, &qex_stat );
    }
   
  if (itercount == MAX_EVAL_ITERATIONS)
    {
      nonfatalerror ("The variable you're attempting to EVAL seems to eval "
		     "infinitely, and hence I cannot compute it.  I did try "
		     "a lot, though.  I got this far before I gave up: ", 
		     tempbuf);
      return (0);
    }
  if (loop_abort == 2)
    {
      nonfatalerror ("The variable you're attempting to EVAL seemes to return "
		     "to the same value after a number of iterations, "
		     "so it is probably an "
		     "infinite loop.  I think I should give up.  I got this "
		     "far: ", tempbuf);
      return (0);
    }
  
  //     and shove it out to wherever it needs to be shoved.
  //
  if (has_output_var)
    crm_destructive_alter_nvariable (varname, varnamelen, 
				   tempbuf, newvallen);
  
  if (internal_trace)
    fprintf (stderr, "Final qex_stat was %ld\n", qex_stat);
  
  //    for now, use the qex_stat that came back from qexpandvar.
  if (qex_stat > 0)
    {
      if (user_trace)
	fprintf (stderr, "Mathematical expression at line was not satisfied, doing a FAIL at line %ld\n", csl->cstmt);
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
    }
  return (0);  
}

int crm_expr_alter (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  //      here's where we surgiclly alter a variable.  We have to
  //      watch out in case a variable is not in the cdw (it might
  //      be in tdw; that's legal as well.
  //      syntax is to replace the contents of the variable in the
  //      varlist with the evaluated string.
  //      Syntax is "alter <flags> (var) /newvalue/
  
	char varname[MAX_VARNAME];
	long varnamestart;
	long varnamelen;
	long newvallen;
	// should use tempbuf for this instead.
	//   char newstr [MAX_PATTERN];
	if (user_trace)
	  fprintf (stderr, "Executing an ALTERation\n");
	
	//     get the variable name
       	crm_get_pgm_arg (varname, MAX_VARNAME, apb->p1start, apb->p1len);
	if (apb->p1len < 3) 
	  {
	    nonfatalerror (
		     "This statement is missing the variable to alter,\n",
			   "so I'll ignore the whole statement.");
	    return (0);
	  }
	
	//      do variable substitution on the variable name
	varnamelen = crm_nexpandvar (varname, apb->p1len, MAX_VARNAME);
	crm_nextword (varname, varnamelen, 0, &varnamestart, &varnamelen);
	if (varnamelen - varnamestart < 3) 
	  {
	    nonfatalerror (
	  "The variable you're asking me to alter has an utterly bogus name\n",
		"so I'll ignore the whole statement.");
	    return (0);
	  }

	//     get the new pattern, and expand it.
	crm_get_pgm_arg (tempbuf, data_window_size, apb->s1start, apb->s1len);
	newvallen = crm_nexpandvar (tempbuf, apb->s1len, data_window_size);
	
	crm_destructive_alter_nvariable (&varname[varnamestart], varnamelen, 
					tempbuf, newvallen);
	return (0);
}
