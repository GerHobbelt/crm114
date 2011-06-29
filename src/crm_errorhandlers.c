//  crm_errorhandlers.c  - Controllable Regex Mutilator,  version v1.0
// Copyright 2001-2006  William S. Yerazunis, all rights reserved.
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


//     Helper routines 
//    

//     apocalyptic eerror - an error that can't be serviced on a TRAP - forces
//     exit, not a prayer of survival.
void untrappableerror (const char *text1, const char *text2)
{
  int iline, ichar;
  int maxchar;
  char reason[MAX_PATTERN];
  //
  //   Create our reason string.
  snprintf (reason, MAX_PATTERN, 
	    "\n%s: *UNTRAPPABLE ERROR*\n %s %s\nSorry, but I can't recover from that.\nThis happened at line %ld of file %s\n", 
	   prog_argv[0], text1, text2, csl->cstmt, csl->filename);
  fprintf (stderr, "%s", reason);

  //     Check to see - is there a trap available or is this a non-trap 
  //     program?
  //
  if (csl && csl->mct)
    {
      fprintf (stderr, "The line was:\n--> ");
      iline = csl->cstmt;
      maxchar = csl->mct[iline+1]->fchar;
      if (maxchar > csl->mct[iline]->fchar + 255) 
	maxchar = csl->mct[iline]->fchar + 255;
      if (iline > 0)
	for (ichar = csl->mct[iline]->fchar;
	     ichar < maxchar;
	     ichar++)
	  fprintf (stderr, "%c", csl->filetext[ichar]);
      fprintf (stderr, "\n");
    }
  if (engine_exit_base != 0)
    {
      exit (engine_exit_base + 2);
    }
  else
    exit (CRM_EXIT_APOCALYPSE);

}


//     fatalerror - print a fatal error on stdout, trap if we can, else exit  
long fatalerror ( const char *text1, const char *text2 )
{
  int iline, ichar;
  int maxchar;
  char *rbuf;
  char reason[MAX_PATTERN];
  //
  //   Create our reason string.  Note that some reason text2's can be VERY 
  //   long, so we put out only the first 1024 characters
  //
  if (strlen (text2) < 1023)
  {
    snprintf (reason, NUMBEROF(reason), "\n%s: *ERROR*\n %.1024s %.1024s\n Sorry, but this program is very sick and probably should be killed off.\nThis happened at line %ld of file %s\n", 
	   prog_argv[0], text1, text2, csl->cstmt, csl->filename);
  }
  else
  {
	  snprintf (reason, NUMBEROF(reason), "\n%s: *ERROR*\n %.1024s %.1024s(...truncated)\n Sorry, but this program is very sick and probably should be killed off.\nThis happened at line %ld of file %s\n", 
	   prog_argv[0], text1, text2, csl->cstmt, csl->filename);
  }
  reason[NUMBEROF(reason) - 1] = 0;

  //     Check to see - is there a trap available or is this a non-trap 
  //     program?
  //
  //   if (csl->mct[csl->cstmt]->trap_index <  csl->nstmts)
  if ((csl->nstmts > 0) && (csl->mct[csl->cstmt]->trap_index < csl->nstmts))
    {
      long fresult;
      rbuf = malloc (MAX_PATTERN * sizeof(rbuf[0])); /* [i_a] */
      if (!rbuf)
	{
          fprintf(stderr,
	    "Couldn't malloc rbuf in 'fatalerror()'!\nIt's really bad when the error fixup routine gets an error!.\n");
          if (engine_exit_base != 0)
            {
              exit (engine_exit_base + 3);
            }
          else
	    exit( CRM_EXIT_FATAL);
	}

      strcpy (rbuf, reason);
      fresult = crm_trigger_fault (rbuf);
      if (fresult == 0)
	return (0);
    }
  fprintf (stderr, "%s", reason);
  if (csl->nstmts > 0)
    {
      fprintf (stderr, "The line was:\n--> ");
      iline = csl->cstmt;
      maxchar = csl->mct[iline+1]->fchar;
      if (maxchar > csl->mct[iline]->fchar + 255) 
	maxchar = csl->mct[iline]->fchar + 255;
      for (ichar = csl->mct[iline]->fchar;
	   ichar < maxchar;
	   ichar++)
	fprintf (stderr, "%c", csl->filetext[ichar]);
    }
  else
    fprintf (stderr, "\n <<< no compiled statements yet >>>");

  fprintf (stderr, "\n");
  if (engine_exit_base != 0)
    {
      exit (engine_exit_base + 4);
    }
  else
  exit ( CRM_EXIT_FATAL );
}

long nonfatalerror ( const char *text1, const char *text2 )
{
  int iline, ichar;
  int maxchar;
  static int nonfatalerrorcount = 0;
  char *rbuf;
  char reason[MAX_PATTERN];

  //
  //   Create our reason string.  Note that some reason text2's can be VERY 
  //   long, so we put out only the first 1024 characters
  //
  if (strlen (text2) < 1023)
    snprintf (reason, MAX_PATTERN, "\n%s: *WARNING*\n %.1024s %.1024s\nI'll try to keep working.\nThis happened at line %ld of file %s\n", 
	   prog_argv[0], text1, text2, csl->cstmt, csl->filename);
  else
    snprintf (reason, MAX_PATTERN, "\n%s: *WARNING*\n %.1024s %.1024s(...truncated)\nI'll try to keep working.\nThis happened at line %ld of file %s\n", 
	   prog_argv[0], text1, text2, csl->cstmt, csl->filename);
  reason[MAX_PATTERN-1] = 0;

  //     Check to see - is there a trap available or is this a non-trap 
  //     program?
  //

  //  if (csl->mct[csl->cstmt]->trap_index < csl->nstmts)
  if ((csl->nstmts > 0) && (csl->mct[csl->cstmt]->trap_index<csl->nstmts))
    {
      long fresult;
      rbuf = malloc (MAX_PATTERN * sizeof(rbuf[0]));  /* [i_a] */
      if (!rbuf)
	{
          fprintf(stderr,
	    "Couldn't malloc rbuf in 'nonfatalerror()'!\nIt's really bad when the error fixup routine gets an error ! \n");
          
	  if (engine_exit_base != 0)
            {
              exit (engine_exit_base + 5);
            }
          else
	    exit(CRM_EXIT_FATAL);
	}

      strcpy (rbuf, reason);
      fresult = crm_trigger_fault (rbuf);
      if (fresult == 0) return (0);
    }

  fprintf (stderr, "%s", reason);
  if (csl->nstmts > 0) 
    {
      fprintf (stderr, "The line was:\n--> "); 
      iline = csl->cstmt;
      maxchar = csl->mct[iline+1]->fchar;
      if (maxchar > csl->mct[iline]->fchar + 255) 
	maxchar = csl->mct[iline]->fchar + 255;
      for (ichar = csl->mct[iline]->fchar;
	   ichar < maxchar;
	   ichar++)
	fprintf (stderr, "%c", csl->filetext[ichar]);
    }
  else 
    fprintf (stderr, "<<< no compiled statements yet >>>");
  fprintf (stderr, "\n");
  nonfatalerrorcount++;
  if (nonfatalerrorcount > MAX_NONFATAL_ERRORS) 
    fatalerror 
      ("Too many untrapped warnings; your program is very likely unrecoverably broken.\n",
       "\n\n  'Better shut her down, Scotty.  She's sucking mud again.'\n");
    return (0);
}

//     print out timings of each statement
void crm_output_profile ( CSL_CELL *csl)
{
  long i;
  fprintf (stderr,
	   "\n         Execution Profile Results\n");
  fprintf (stderr, 
	   "\n  Memory usage at completion: %10ld window, %10ld isolated\n",
	   cdw->nchars, tdw->nchars);
  fprintf (stderr, 
	   "\n  Statement Execution Time Profiling (0 times suppressed)");
  fprintf (stderr, 
	   "\n  line:      usertime   systemtime    totaltime\n");
  for (i = 0; i < csl->nstmts; i++)
    {
      if (csl->mct[i]->stmt_utime +csl->mct[i]->stmt_stime > 0)
	fprintf (stderr, " %5ld:   %10ld   %10ld   %10ld\n",
		 i,
		 csl->mct[i]->stmt_utime,
		 csl->mct[i]->stmt_stime,
		 csl->mct[i]->stmt_utime + csl->mct[i]->stmt_stime);
    }
}



// crm_trigger_fault - whenever there's a fault, this routine Does The
//  Right Thing, in terms of looking up the next TRAP statement and
//  turning control over to it.
//  
//  Watch out, this routine must return cleanly with the CSL top set up
//  appropriately so execution can continue at the chosen statement.
//  That next statement executed will be a TRAP statement whose regex
//  matches the fault string.  All other intervening statements will
//  be ignored.
//
//  This routine returns 0 if execution should continue, or 1 if there
//  was no applicable trap to catch the fault and we should take the
//  default fault action (which might be to exit).
//
//  Routines that get here must be careful to NOT trash the constructed
//  csl frame that causes the next statement to be the TRAP statement.
//  In particular, we act like the MATCH and CLASSIFY and FAIL statments
//  and branch to the chosen location -1 (due to the increment in the 
//  main invocation loop)

long crm_trigger_fault ( char *reason) 
{
  //
  //    and if the fault is correctly trapped, this is the fixup.
  //
  char trap_pat[MAX_PATTERN];
  long pat_len;
  regex_t preg;
  long i, j;
  ARGPARSE_BLOCK apb;
  long slen;
  long done;
  long trapline;

  //  Non null fault_text rstring -we'll take the trap 
  if (user_trace)
    {
      fprintf (stderr, "Catching FAULT generated on line %ld\n", 
	       csl->cstmt);
      fprintf (stderr, "FAULT reason:\n%s\n",
	       reason );
    }

  trapline = csl->cstmt;

  done = 0;
  while ( ! done)
    {
      if (csl->mct[trapline]->trap_index >= csl->nstmts
	  || (trapline = csl->mct[trapline]->trap_index) == -1)

	{
	  if (user_trace)
	    {
	      fprintf (stderr, "     ... no applicable TRAP.\n");
	    }
	  return (1);
	}
	      
      //      trapline = csl->mct[trapline]->trap_index;
      //      
      //        make sure we're really on a trap statement.
      //
      if (csl->mct[trapline]->stmt_type != CRM_TRAP)
	return (1);

      //       OK, we're here, with a live trap.  
      slen = (csl->mct[trapline+1]->fchar) 
	- (csl->mct[trapline ]->fchar);
      
      i = crm_statement_parse(
		      &(csl->filetext[csl->mct[trapline]->fchar]),
		      slen,
		      &apb);
      if (user_trace)
	{
	  fprintf (stderr, "Trying trap at line %ld:\n", trapline);
	  for (j =  (csl->mct[trapline ]->fchar);
	       j < (csl->mct[trapline+1]->fchar);
	       j++)
	    fprintf (stderr, "%c", csl->filetext[j]);
	  fprintf (stderr, "\n");
	}
      
      //  Get the trap pattern and  see if we match.
      crm_get_pgm_arg (trap_pat, MAX_PATTERN,
		       apb.s1start, apb.s1len);
      //
      //      Do variable substitution on the pattern
      pat_len = crm_nexpandvar (trap_pat, apb.s1len, MAX_PATTERN);
      
      //
      if (user_trace)
	{
	  fprintf (stderr, "This TRAP will trap anything matching =%s= .\n", 
		   trap_pat);
	}
      //       compile the regex
      i = crm_regcomp (&preg, trap_pat, pat_len, REG_EXTENDED);
      if ( i == 0)
	{
	  i = crm_regexec (&preg, 
			   reason,
			   strlen (reason),
			   0, NULL, 0, NULL);
	  crm_regfree (&preg);
	}
      else
	{
	  crm_regerror ( i, &preg, tempbuf, data_window_size);
	  csl->cstmt = trapline;
	  fatalerror ("Regular Expression Compilation Problem in TRAP pattern:", 
		      tempbuf);
	}
      
            
      //    trap_regcomp_error:


      //    if i != 0, we didn't match - kick the error further
      if (i == 0)
	{
	  if (user_trace)
	    {
	      fprintf (stderr, "TRAP matched.\n");
	      fprintf (stderr, "Next statement will be %ld\n",
		       trapline);
	    }
	  //
	  //   set the next statement to execute to be
	  //   the TRAP statement itself.

	  
	  csl->cstmt = trapline;
	  csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = 1;
	  //
	  //     If there's a trap variable, modify it.
	  {
	    char reasonname [MAX_VARNAME];
	    long rnlen;
	    crm_get_pgm_arg(reasonname, MAX_VARNAME, apb.p1start, apb.p1len);
	    if (apb.p1len > 2)
	      {
		rnlen = crm_nexpandvar (reasonname, apb.p1len, MAX_VARNAME);
		//   crm_nexpandvar null-terminates for us so we can be
		//   8-bit-unclean here
		crm_set_temp_nvar (reasonname, 
				   reason,
				   strlen (reason));
	      }
	    done = 1;
	  }
	}
      else
	{
	  //   The trap pattern didn't match - move outward to 
	  //   the surrounding trap (if it exists)
	  if (user_trace)
	    {
	      fprintf (stderr, 
		       "TRAP didn't match- trying next trap in line.\n");
	    }
	}
      //      and note that we haven't set "done" == 1 yet, so
      //      we will go through the loop again.
    }
  return (0);
}


