//  crm_exec_engine.c  - Controllable Regex Mutilator,  version v1.0
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

//
//        Here it is, the core of CRM114 - the execution engine toplevel,
//      which, given a CSL and a CDW, executes the CSL against the CDW      
//

int crm_invoke ()
{
  long i, j, k;
  long status;
  long done;
  long slen;

  //     timer1, timer2, and tstmt are for time profiling.
  //
  TMS_STRUCT timer1, timer2;
  long tstmt;

  tstmt = 0;
  i = j = k = 0;
  
  status = 0;

  //    Sanity check - don't try to execute a file before compilation
  if (csl->mct == NULL)
    {
      untrappableerror ( "Can't execute a file without compiling first.\n",
		   "This means that CRM114 is somehow broken.");
    };

  //    empty out the alius stack (nothing FAILed yet.)
  //
  for (i = 0; i < MAX_BRACKETDEPTH; i++)
    csl->aliusstk[i] = 1;
    
  //    if there was a command-line-specified BREAK, set it.
  //    
  if (cmdline_break > 0)
    {
      if (cmdline_break <= csl->nstmts)
	{
	  csl->mct[cmdline_break]->stmt_break = 1;
	};
    };

  if (user_trace > 0)
    fprintf (stderr, "Starting to execute %s at line %ld\n",
	     csl->filename, csl->cstmt);

 invoke_top:

  //   initialize timers ?
  if (profile_execution)
    {
      tstmt = csl->cstmt;
      times ( (void *) &timer1);
    };

  if (csl->cstmt >= csl->nstmts)
    {
      //  OK, we're at the end of the program.  When this happens,
      //  we know we can exit this invocation of the invoker
      if (user_trace > 0 )
	fprintf (stderr, "Finished the program %s.\n", csl->filename);
      done = 1;
      status = 0;
      goto invoke_done;
    };

  slen = (csl->mct[csl->cstmt+1]->fchar) - (csl->mct[csl->cstmt ]->fchar);

  if (user_trace > 0)
    {
      fprintf (stderr, "\nParsing line %ld :\n", csl->cstmt);
      fprintf (stderr, " -->  ");
      for (i = 0; i < slen; i++)
	fprintf (stderr, "%c", 
		 csl->filetext[csl->mct[csl->cstmt]->fchar+i]);
      fprintf (stderr, "\n");
    };


  //    THIS IS THE ULTIMATE SCREAMING TEST - CHECK VHT EVERY LOOP
  //     TURN THIS ON ONLY IN EXTREMIS!
  // do a GC on the whole tdw:
  //       crm_compress_tdw_section (tdw->filetext, 0, tdw->nchars);
  


  //   Invoke the common declensional parser on the statement only if it's
  //   an executable statement.
  //
  switch ( csl->mct[csl->cstmt]->stmt_type )
    { 
      //
      //   Do the processing that all statements need (well, _almost_ all.)
      //
    case CRM_NOOP:
    case CRM_BOGUS:
	break;
    default:
      //         if we've already generated the argparse block (apb) for this
      //         statement, we use it, otherwise, we create one.
      //
      if ( ! csl->mct[csl->cstmt]->apb )
	{
	  csl->mct[csl->cstmt]->apb = malloc (sizeof (ARGPARSE_BLOCK));
	  if ( ! csl->mct[csl->cstmt]->apb ) 
	    untrappableerror ( "Couldn't malloc the space to incrementally "
			       "compile a statement.  ", 
			       "Stick a fork in us; we're _done_.\n");
	  //  we now have the statement's apb allocated; we point the generic
	  //  apb at the same place and run with it.
	  apb = csl->mct[csl->cstmt]->apb;
	  i = crm_statement_parse (  
			    &(csl->filetext[csl->mct[csl->cstmt]->fchar]),
			    slen,
			    apb);
	}
      else
	{
	  //    The CSL->MCT->APB was valid, we can just reuse the old apb.
	  if (internal_trace)
	    fprintf (stderr, "JIT parse reusing line %ld \n", csl->cstmt); 
	  apb =  csl->mct[csl->cstmt]->apb;
	};
      //    Either way, the flags might have changed, so we run the
      //    standard flag parser against the flags found (if any)
      {
	char flagz[MAX_PATTERN];
	long fl;
	fl = MAX_PATTERN;
	crm_get_pgm_arg (flagz, fl, apb->a1start, apb->a1len);
	fl = crm_nexpandvar (flagz, apb->a1len, MAX_PATTERN);
	//    fprintf (stderr, 
	//           "flagz --%s-- len %d\n", flagz, strlen(flagz));
	apb->sflags = crm_flagparse (flagz, fl);
      };
      break;
    };
  //    and maybe drop into the debugger?
  //
  cycle_counter++;
  if (debug_countdown > 0) debug_countdown--;
  if (debug_countdown == 0
      || csl->mct[csl->cstmt]->stmt_break == 1 )
    {
      i = crm_debugger ();
      if (i == -1)
	{
	  if (engine_exit_base != 0)
	    {
	      exit (engine_exit_base + 6);
	    }
	  else
	    exit ( EXIT_SUCCESS );
	};
      if (i == 1)
	goto invoke_top;
    };

  if (user_trace > 0)
    {
      fprintf (stderr, "\nExecuting line %ld :\n", csl->cstmt);
    };


  //  so, we're not off the end of the program (yet), which means look
  //  at the statement type and see if it's somethign we know how to
  //  do, otherwise we make a nasty little noise and continue onward.
  //  Dispatch is done on a big SWITCH statement
  
  switch ( csl->mct[csl->cstmt]->stmt_type )
    {
    case CRM_NOOP:
    case CRM_LABEL:
      {
	if (user_trace)
	  fprintf (stderr, "Statement %ld is non-executable, continuing.\n",
		   csl->cstmt);
      }
      break;

    case CRM_OPENBRACKET:
      {
	//  the nest_level+1 is because the statements in front are at +1 depth
	csl->aliusstk [ csl->mct [ csl->cstmt] -> nest_level+1 ] = 1;
	if (user_trace)
	  fprintf (stderr, "Statement %ld is an openbracket. depth now %d.\n",
		   csl->cstmt, 1 + csl->mct [ csl->cstmt]->nest_level);
      }
      break;

    case CRM_CLOSEBRACKET:
      {
	if (user_trace)
	  fprintf (stderr, "Statement %ld is a closebracket. depth now %d.\n",
		   csl->cstmt, csl->mct[ csl->cstmt]->nest_level);
      }
      break;
      
    case CRM_BOGUS:
      {
        char bogusbuffer[1024];
	char bogusstmt [1024];
        sprintf (bogusbuffer, "Statement %ld is bogus!!!  Here's the text: \n",
 		 csl->cstmt);
	memmove (bogusstmt, 
		&csl->filetext[csl->mct[csl->cstmt]->start],
		csl->mct[csl->cstmt+1]->start - csl->mct[csl->cstmt]->start);
	bogusstmt [csl->mct[csl->cstmt+1]->start - csl->mct[csl->cstmt]->start]
	  = '\000';
        fatalerror (bogusbuffer, bogusstmt);
	goto invoke_bailout;
      }
      break;

    case CRM_EXIT:
      {
	int retval; long retlen;
	char retstr [MAX_PATTERN];
	crm_get_pgm_arg (retstr, MAX_VARNAME, apb->s1start, apb->s1len);
	retlen = apb->s1len;
	retlen = crm_nexpandvar (retstr, retlen, MAX_VARNAME);
	retval = 0;
	if (retlen > 0)
	  sscanf (retstr, "%d", &retval);
	if (user_trace)
	  fprintf (stderr, "Exiting at statement %ld with value %d\n", 
		   csl->cstmt, retval);
	//if (profile_execution)
	//  crm_output_profile (csl);
	//	exit (retval);
	status = retval;
	done = 1;
	goto invoke_exit;
      }
      break;

    case CRM_RETURN:
      {
	CSL_CELL *old_csl;
	unsigned long retlen;
	char *namestart;
	unsigned long namelen;
	if (user_trace)
	  fprintf (stderr, "Returning to caller at statement %ld\n", 
		   csl->cstmt);
	//  
	//    Is this the toplevel csl call frame?  If so, return!
	if (csl->caller == NULL) 
	  return (0);

	//    Nope!  We can now pop back up to the previous context
	//    just by popping the most recent csl off.
	if (internal_trace)
	  fprintf (stderr, "Return - popping CSL back to %lx\n", 
		   (long) csl->caller );
	
	//     Do the argument transfer here.
	//     Evaluate the "subject", and return that.  
	if (apb->s1len > 0)
	  {
	    long idx;
	    unsigned long noffset;
	    crm_get_pgm_arg (outbuf, MAX_VARNAME, apb->s1start, apb->s1len);
	    retlen = apb->s1len;
	    retlen = crm_nexpandvar (outbuf, retlen, data_window_size);
	    //
	    //      Now we have the return value in outbuf, and the return
	    //      length in retlen.  Get it's name
	    idx = csl->return_vht_cell;
	    if (idx < 0)
	      {
		if (user_trace)
		  fprintf (stderr, "Returning, no return argument given\n");
	      }
	    else
	      {
		long i;
		//fprintf (stderr, "idx: %lu  vht[idx]: %lu", idx, vht[idx] );
		noffset = vht[idx]->nstart;
		//fprintf (stderr, " idx: %lu noffset: %lu \n/", idx, noffset);
		namestart 
		  = &(vht[idx]->nametxt[noffset]);
		namelen = vht[idx]->nlen;
		
		if (user_trace)
		  {
		    fprintf (stderr, " setting return value of >");
		    for (i = 0; i < namelen; i++ )
		      fprintf (stderr, "%c", namestart[i]);
		    fprintf (stderr, "<\n");
		  }
		//     stuff the return value into that variable.
		//
		crm_destructive_alter_nvariable (namestart, namelen,
					   outbuf, retlen);
	      }
	  }
	//
	//     release the current csl cell back to the free memory pool.
	old_csl = csl;
	csl = csl->caller;
	free (old_csl);
	{
	  //   properly set :_cd: as well - note that this can be delayed
	  //   since the vht index of the return location was actually 
	  //   set during the CALL statement, not calculated during RETURN.
	  char depthstr[33];
	  sprintf (depthstr, "%ld", csl->calldepth);
	  crm_set_temp_var (":_cd:", depthstr);
	};
      }
      break;
      
    case CRM_GOTO:
      {
	static char target[MAX_VARNAME];
	long tarlen;
	//  look up the variable name in the vht.  If it's not there, or 
	//  not in our file, call a fatal error.

	crm_get_pgm_arg ( target, MAX_VARNAME, apb->s1start, apb->s1len);
	if (apb->s1len < 2)
	  nonfatalerror 
	    ("This program has a GOTO without a place to 'go' to.",
	     " By any chance, did you leave off the '/' delimiters? ");
	tarlen = apb->s1len;
	if (internal_trace) 
	  fprintf (stderr, "\n    untranslated label %s , ", 
		   target);

	//   do indirection if needed.
	tarlen = crm_qexpandvar (target, tarlen, MAX_VARNAME, NULL);
	if (internal_trace)
	  fprintf (stderr, " translates to %s .", target);

	k = crm_lookupvarline (vht, target, 0, tarlen);

	if (k > 0)
	  {
	    if (user_trace)
	      fprintf (stderr, "GOTO from line %ld to line %ld\n", 
		       csl->cstmt,  k);
	    csl->cstmt = k;  // this gets autoincremented
	    //  and going here didn't fail... 
	    csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = 1;
	  }
	else
	  {
	    int conv_count;
	    conv_count = sscanf (target, "%ld", &k);
	    if (conv_count == 1)
	      {
		if (user_trace)
		  fprintf (stderr, "GOTO from line %ld to line %ld\n",
			   csl->cstmt, k);
		csl->cstmt = k -1 ; // this gets autoincremented, so we must --
		//  and going here didn't fail...
		csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = 1;
	      }
	    else
	      {
		//  this is recoverable if we have a trap... so we continue
		//   execution right to the BREAK.
		fatalerror (" Can't GOTO the nonexistent label/line: ", 
			    target);
		goto invoke_bailout;
	      };
	  };
      }
      break;

    case CRM_FAIL:
      {   
	//       If we get a FAIL, then we should branch to the statement
	//         pointed to by the fail_index entry for that line.
	//
	//              note we cheat - we branch to "fail_index - 1"
	//                and let the increment happen.
	if (user_trace)
	  fprintf (stderr, "Executing hard-FAIL at line %ld\n", csl->cstmt);
	csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;

	//   and mark that we "failed", so an ALIUS will take this as a 
	//   failing statement block
	csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
      };
      break;
	
    case CRM_LIAF:
      {   
	//       If we get a LIAF, then we should branch to the statement
	//         pointed to by the liaf_index entry for that line.
	//
	//               (note the "liaf-index - 1" cheat - we branch to 
	//               liaf_index -1 and let the incrment happen)
	if (user_trace)
	  fprintf (stderr, "Executing hard-LIAF at line %ld\n", csl->cstmt);
	csl->cstmt = csl->mct[csl->cstmt]->liaf_index - 1 ;
      };
      break;

    case CRM_ALIUS:
      {
	//   ALIUS looks at the finish state of the last bracket - if it
	//   was a FAIL-to, then ALIUS is a no-op.  If it was NOT a fail-to,
	//   then ALIUS itself is a FAIL
	if (user_trace)
	  fprintf (stderr, "Executing ALIUS at line %ld\n", csl->cstmt);
	if (csl->aliusstk [csl->mct[csl->cstmt]->nest_level + 1] == 1)
	  {
	    if (user_trace)
	      fprintf (stderr, "prior group exit OK, ALIUS fails forward. \n");
	    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;  
	  };
      }
      break;

    case CRM_TRAP:
      {
	//  TRAP is a placeholder statement that holds the regex that
	//  the faulting statement must match.  The background support
	//  code is in crm_trigger_fault that will look at the error string
	//  and see if it matches the regex.  
	//
	//  If we get to a TRAP statement itself, we should treat it as
	//  a skip to end of block (that's a SKIP, not a FAIL)

	if (user_trace) 
	  {
	    fprintf (stderr, "Executing a TRAP statement...");
	    fprintf (stderr, " this is a NOOP unless you have a live FAULT\n");
	  }
	csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      }
      break;

    case CRM_FAULT:
      {
	char *reason;
	char rbuf [MAX_PATTERN];
	long rlen;
	long fresult;
	
	//  FAULT forces the triggering of the TRAP; it's a super-FAIL
	//   statement that can skip downward a large number of blocks.
	//
	if (user_trace)
	  fprintf (stderr, "Forcing a FAULT at line %ld\n", csl->cstmt);
	crm_get_pgm_arg ( rbuf, MAX_PATTERN, apb->s1start, apb->s1len );
	rlen = crm_nexpandvar (rbuf, apb->s1len, MAX_PATTERN);

	//   We malloc the reason - better free() it when we take the trap.
	//   in crm_trigger_fault
	//
	reason = malloc (rlen + 5);
        if (!reason)
	  untrappableerror(
	    "Couldn't malloc 'reason' in CRM_FAULT - out of memory.\n", 
	    "Don't you just HATE it when the error fixup routine gets"
	    "an error?!?!");
	strncpy (reason, rbuf, rlen+1);
	fresult = crm_trigger_fault (reason);
	if (fresult != 0)
	  {
	    fatalerror("Your program has no TRAP for the user defined fault:",
			reason);
	    goto invoke_bailout;
	  };
      }
      break;

    case CRM_ACCEPT:
      {
	char varname [MAX_VARNAME];
	long varidx;
	//   Accept:  take the current window, and output it to
	//   standard output.  
	//
	//
	if (user_trace)
	  fprintf (stderr, "Executing an ACCEPT \n");
	//
	//
	varname[0] = '\0';
	strcpy (varname, ":_dw:");
	varidx = crm_vht_lookup (vht, varname, strlen (varname));
	if (varidx == 0
	    || vht[varidx] == NULL)
	  {
	    fatalerror ("This is very strange... there is no data window!",
			"As such, death is our only option.");
	    goto invoke_bailout;
	  }
	else
	  {
	    fwrite (&(vht[varidx]->valtxt[vht[varidx]->vstart]),
		    vht[varidx]->vlen,
		    1,
		    stdout);
	    fflush (stdout);
	  }
	//    WE USED TO DO CHARACTER I/O.  OUCH!!!
	//	for (i = 0; i < cdw->nchars ; i++)
	//        fprintf (stdout, "%c", cdw->filetext[i]);
      }
      break;

    case CRM_MATCH:
      {
	crm_expr_match (csl, apb);
      }
      break;

    case CRM_OUTPUT:
      {
	crm_expr_output (csl, apb);
      }
      break;

    case CRM_WINDOW:
      {
	long i;
	i = crm_expr_window (csl, apb);
	if (i == 1) goto invoke_bailout;
      }
      break;
      
    case CRM_ALTER:
      {
	crm_expr_alter (csl, apb);
      }
      break;
      
    case CRM_EVAL:
      {
	crm_expr_eval (csl, apb);
      }
      break;

    case CRM_HASH:
      {
	//      here's where we surgiclly alter a variable to a hash.
	//      We have to watch out in case a variable is not in the
	//      cdw (it might be in tdw; that's legal as well.  syntax
	//      is to replace the contents of the variable in the
	//      varlist with hash of the evaluated string.  

	char varname[MAX_VARNAME];
	long varlen;
	long vns, vnl;
	char newstr [MAX_VARNAME];
	long newstrlen;
	unsigned long hval;         //   hash value

	if (user_trace)
	  fprintf (stderr, "Executing a HASHing\n");

	//     get the variable name
	crm_get_pgm_arg (varname, MAX_VARNAME, apb->p1start, apb->p1len);
	varlen = apb->p1len;
	varlen = crm_nexpandvar (varname, varlen, MAX_VARNAME);
	crm_nextword (varname, varlen, 0, &vns, &vnl);

	//   If we didn't get a variable name, we replace the data window!
	if (vnl == 0) 
	  {
	    strcpy (varname, ":_dw:");
	    vnl = strlen (varname);
	  }

	//     get the to-be-hashed pattern, and expand it.
	crm_get_pgm_arg (tempbuf, data_window_size, apb->s1start, apb->s1len);
	newstrlen = apb->s1len;
	//
	//                   if no var given, hash the full data window.
	if (newstrlen == 0)
	  {
	    strcpy (tempbuf, ":*:_dw:");
	    newstrlen = strlen (tempbuf);
	  }
	newstrlen = crm_nexpandvar (tempbuf, newstrlen, data_window_size);

	//    The pattern is now expanded, we can hash it to obscure meaning.
	hval = strnhash (tempbuf, newstrlen );
	sprintf (newstr, "%08lX", hval);

	if (internal_trace)
	  {
	    fprintf (stderr, "String: '%s' \n hashed to: %08lX\n", 
		     tempbuf,
		     hval);
	  };

	//    and stuff the new value in.
	crm_destructive_alter_nvariable (&varname[vns], vnl,
					 newstr, strlen (newstr));

      };
      break;

    case CRM_TRANSLATE:
      {
	crm_expr_translate (csl, apb);
      };
      break;

    case CRM_LEARN:
      {
	crm_expr_learn (csl, apb);
      };
      break;
      
      //   we had to split out classify- it was just too big.
    case CRM_CLASSIFY:
      crm_expr_classify ( csl, apb);
      break;

    case CRM_ISOLATE:
      //    nonzero return means "bad things happened"...
      if ( crm_expr_isolate (csl, apb) )
	goto invoke_bailout;
      break;
      
    case CRM_INPUT:
      {
	crm_expr_input (csl, apb);
      };
      break;

    case CRM_SYSCALL:
      {
	if (crm_expr_syscall (csl, apb))
	  goto invoke_bailout;
      }
      break;

    case CRM_CALL:
      {
	//    a user-mode call - create a new CSL frame and move in!
	//
	char target[MAX_VARNAME];
	long tarlen;
	CSL_CELL * newcsl;
	
	if (user_trace)
	  fprintf (stderr, "Executing a user CALL statement\n");
	//  look up the variable name in the vht.  If it's not there, or 
	//  not in our file, call a fatal error.
	
	crm_get_pgm_arg ( target, MAX_VARNAME, apb->s1start, apb->s1len);
	tarlen = apb->s1len;
	if (internal_trace) 
	  fprintf (stderr, "\n    untranslated label %s , ", 
		   target);
	
	//   do indirection if needed.
	tarlen = crm_nexpandvar (target, tarlen, MAX_VARNAME);
	if (internal_trace)
	  fprintf (stderr, " translates to %s .", target);
	
	k = crm_lookupvarline (vht, target, 0, tarlen);
	
	//      Is this a call to a known label?
	//    
	if (k <= 0) 
	  {
	    //    aw, crud.  No such label known.  But it _is_ continuable
	    //    if there is a trap for it.
	    fatalerror (" Can't CALL the nonexistent label: ", target);
	    goto invoke_bailout;
	  }
	newcsl = (CSL_CELL *) malloc (sizeof (CSL_CELL));
	newcsl->filename = csl->filename;
	newcsl->filetext = csl->filetext;
	newcsl->filedes = csl->filedes;
	newcsl->rdwr = csl->rdwr;
	newcsl->nchars = csl->nchars;
	newcsl->hash = csl->hash;
	newcsl->mct = csl->mct;
	newcsl->nstmts = csl->nstmts;
	newcsl->preload_window = csl->preload_window;
	newcsl->caller = csl;
	newcsl->calldepth = csl->calldepth + 1;
	//     put in the target statement number - this is a label!
	newcsl->cstmt = k;
	newcsl->return_vht_cell = -1;
	//     whack the alius stack so we are not in a "fail" situation
	newcsl->aliusstk [csl->mct[csl->cstmt]->nest_level + 1 ] = 0;
	
	//
	//    Argument transfer - is totally freeform.  The arguments
	//    (the box-enclosed string) are var-expanded and the result
	//    handed off as the value of the first paren string of the
	//    routine (which should be a :var:, if it doesn't exist or
	//    isn't isolated, it is created and isolated).  The
	//    routine can use this string in any way desired.  It can
	//    be parsed, searched, MATCHEd etc.  This allows
	//    call-by-anything to be implemented.
	//
	//    On return, a similar process is performed - the return value
	//    is formed, and the result put in an isolated variable in
	//    the caller's memory.
	//    
	//    Note that there is _no_ local (hidden, etc) vars - everything
	//    is still shared.  
	//
	{
	  long argvallen, argnamelen;
	  CSL_CELL * oldcsl;
	  long vns, vnl;
	  long vmidx, oldvstart, oldvlen;
	  //
	  //    First, get the argument string into full expansion
	  crm_get_pgm_arg(tempbuf, data_window_size, apb->b1start, apb->b1len);
	  argvallen = apb->b1len;
	  argvallen = crm_nexpandvar (tempbuf, argvallen, data_window_size);
	  
	  tempbuf[argvallen] = '\0';

	  //   Stuff the new csl with the return-value-locations'
	  //   vht index - if it's -1, then we don't have a return
	  //   value location.  We do this now rather than on return
	  //   so that we already have the vht entry rather than a 
	  //   name, and so the returnname isn't dependent on the 
	  //   function being executed (is this a bug?  What if the
	  //   returnname is :+:something: ?)
	  //
	  newcsl->return_vht_cell = -1;
	  if (apb->p1len > 0)
	    {
	      unsigned long ret_idx;
	      long retname_start, retnamelen;
	      crm_get_pgm_arg (outbuf, data_window_size,
			       apb->p1start, apb->p1len);
	      retnamelen = apb->p1len;
	      retnamelen = crm_nexpandvar (outbuf, 
					   retnamelen, data_window_size);
	      crm_nextword (outbuf, retnamelen,0, &retname_start, &retnamelen);
	      ret_idx = crm_vht_lookup (vht, 
					&outbuf[retname_start], 
					retnamelen);
	      if (vht[ret_idx] == NULL)
		{
		  // nonfatalerror 
		  // ("Your call statement wants to return a value "
		  // "to a nonexistent variable; I'll created an "
		  //"isolated one.  Hope that's OK. Varname was",
		  //		 outbuf);
		  if (user_trace)
		    fprintf (stderr, 
			     "No such return value var, creating var %s\n",
			     outbuf);
		  crm_set_temp_var( outbuf, "");
		};
	      ret_idx = crm_vht_lookup (vht, outbuf, retnamelen);
	      if (user_trace)
		fprintf (stderr, " Setting return value to VHT cell %lu", 
			 ret_idx);
	      newcsl->return_vht_cell = ret_idx;
	    }


	  //    Now, get the place to put the incoming args - here's
	  //    where "control" sort of transfers to the new
	  //    statement.  From here on, everything we do is in the
	  //    context of the callee... so be _careful_.
	  //
	  oldcsl = csl;
	  csl = newcsl;
	  slen = (csl->mct[csl->cstmt+1]->fchar) 
	    - (csl->mct[csl->cstmt ]->fchar);	

	  //
	  //    At this point, the context is now "the callee".  Everything
	  //    done from now must remember that.
	  {
	    //   properly set :_cd: since we're now in the 'callee' code
	    char depthstr[33];
	    sprintf (depthstr, "%ld", csl->calldepth);
	    crm_set_temp_var (":_cd:", depthstr);
	  };

	  //  maybe run some JIT parsing on the called statement?
	  //
	  if ( ! csl->mct[csl->cstmt]->apb )
	    {
	      csl->mct[csl->cstmt]->apb = malloc (sizeof (ARGPARSE_BLOCK));
	      if ( ! csl->mct[csl->cstmt]->apb ) 
		untrappableerror ( "Couldn't malloc the space to incrementally "
				   "compile a statement.  ", 
				   "Stick a fork in us; we're _done_.\n");
	      //  we now have the statement's apb allocated; we point
	      //  the generic apb at the same place and run with it.
	      i = crm_statement_parse (  
	      	       &(csl->filetext[csl->mct[csl->cstmt]->fchar]),
		       slen,
		       csl->mct[csl->cstmt]->apb);
	    };
	  //    and start using the JITted apb
	  apb = csl->mct[csl->cstmt]->apb;
	  //
	  //     We don't have flags, so we don't bother fixing the 
	  //     flag variables.
	  //
	  //            get the paren arg of this routine
	  crm_get_pgm_arg (outbuf, data_window_size, 
			   apb->p1start, apb->p1len);
	  argnamelen = apb->p1len;
	  argnamelen = crm_nexpandvar (outbuf, argnamelen, data_window_size);
	  //
	  //      get the generalized argument name (first varname)
	  crm_nextword (outbuf, argnamelen, 0, &vns, &vnl);
	  memmove (outbuf, &outbuf[vns], vnl);
	  outbuf[vnl] = '\0';
	  if (vnl > 0)
	    {
	      //
	      //      and now create the isolated arg transfer variable.
	      //
	      //  GROT GROT GROT
	      //  GROT GROT GROT possible tdw memory leak here...
	      //  GROT GROT GROT the right fix is to refactor crm_expr_isolate.
	      //  GROT GROT GROT but for now, we'll reuse the same code as 
	      //  GROT GROT GROT it does, releasing the old memory.
	      //
	      vmidx = crm_vht_lookup (vht, outbuf, vnl);
	      if (vht[vmidx] && vht[vmidx]->valtxt == tdw->filetext)
		{
		  oldvstart = vht[vmidx]->vstart;
		  oldvlen = vht[vmidx]->vlen;
		  vht[vmidx]->vstart = tdw->nchars++;
		  vht[vmidx]->vlen   = 0;
		  crm_compress_tdw_section (vht[vmidx]->valtxt,
					    oldvstart,
					    oldvstart+oldvlen);
		};
	      //  
	      //      finally, we can put the variable in.  (this is
	      //      an ALTER to a zero-length variable, which is why
	      //      we moved it to the end of the TDW.
	      crm_set_temp_nvar (outbuf, tempbuf, argvallen);
	    }
	  //
	  //   That's it... we're done.

	}
      }
      break;
      
    case CRM_INTERSECT:
      //           Calculate the intersection of N variables; the result
      //           replaces the captured value of the first variable.
      //           Captured values not in the data window are ignored.
      {
	char temp_vars [MAX_VARNAME];
	long tvlen;
	char out_var [MAX_VARNAME];
	long ovstart;
	long ovlen;
	long vstart;
	long vend;
	long vlen;
	long istart, iend, ilen, i_index;
	long mc;
	long done;
	
	if (user_trace)
	  fprintf (stderr, "executing an INTERSECT statement");

	//    get the output variable (the one we're gonna whack)
	//
	crm_get_pgm_arg (out_var, MAX_VARNAME, apb->p1start, apb->p1len);
	ovstart = 0;
	ovlen = crm_nexpandvar (out_var, apb->p1len, MAX_VARNAME);
	

	//    get the list of variable names
	//
	//     note- since vars never contain wchars, we're OK here.
	crm_get_pgm_arg (temp_vars, MAX_VARNAME, apb->b1start, apb->b1len);
	tvlen = crm_nexpandvar (temp_vars, apb->b1len, MAX_VARNAME);
	if (internal_trace)
	  {
	    fprintf (stderr, "  Intersecting vars: ***%s***\n", temp_vars);
	    fprintf (stderr, "   with result in ***%s***\n", out_var);
	  };
	done = 0;
	mc = 0;
	vstart = 0;
	vend = 0;
	istart = 0;
	iend = cdw->nchars;
	ilen = 0;
	i_index = -1;
	while (!done)
	  {
	    while (temp_vars[vstart] < 0x021
		   && vstart < tvlen )    //  was temp_vars[vstart] != '\000')
	      vstart++;
	    vlen = 0;
	    while (temp_vars[vstart+vlen] >= 0x021
		   && vstart+vlen < tvlen )
	      vlen++;
	    if (vlen == 0) 
	      {
		done = 1;
	      }
	    else
	      {
		long vht_index;
		//
		//        look up the variable
		vht_index = crm_vht_lookup (vht, &temp_vars[vstart], vlen);
		if (vht[vht_index] == NULL )
		  {
		    char varname[MAX_VARNAME];
		    strncpy (varname, &temp_vars[vstart], vlen);
		    varname[vlen] = '\000';
		    nonfatalerror ( "can't intersection a nonexistent variable.",
				    varname);
		    goto invoke_bailout;
		  }
		else
		  { 
		    //  it was a good var, make sure it's in the data window
		    //
		    if (vht[vht_index] -> valtxt != cdw->filetext)
		      {
			char varname[MAX_VARNAME];
			strncpy (varname, &temp_vars[vstart], vlen);
			varname[vlen] = '\000';
			nonfatalerror ( "can't intersect isolated variable.",
					varname);
			goto invoke_bailout;

		      }
		    else
		      {      //  it's a cdw variable; go for it.
			if (vht[vht_index] -> vstart > istart)
			  istart = vht[vht_index] -> vstart;
			if ((vht[vht_index]->vstart + vht[vht_index]->vlen) 
			    < iend)
			  iend = vht[vht_index]->vstart + vht[vht_index]->vlen;
		      };
		  };
	      };
	    vstart = vstart + vlen;
	    if (temp_vars[vstart] == '\000')
	      done = 1;
	  };
	//
	//      all done with the looping, set the start and length of the 
	//      first var.
	vlen = iend - istart;
	if (vlen < 0 ) vlen = 0;
	crm_nextword (out_var, ovlen, 0, &ovstart, &ovlen);
	crm_set_windowed_nvar (&out_var[ovstart], ovlen, cdw->filetext, 
			       istart, vlen,
			       csl->cstmt);
      }
      break;
    case CRM_UNION:
      //           Calculate the union of N variables; the result
      //           replaces the captured value of the first variable.
      //           Captured values not in the data window are ignored.
      {
	char temp_vars [MAX_VARNAME];
	long tvlen;
	char out_var[MAX_VARNAME];
	long ovstart;
	long ovlen;
	long vstart;
	long vend;
	long vlen;
	long istart, iend, ilen, i_index;
	long mc;
	long done;
	
	if (user_trace)
	  fprintf (stderr, "executing a UNION statement");

	//    get the output variable (the one we're gonna whack)
	//
	crm_get_pgm_arg (out_var, MAX_VARNAME, apb->p1start, apb->p1len);
	ovstart = 0;
	ovlen = crm_nexpandvar (out_var, apb->p1len, MAX_VARNAME);
	

	//    get the list of variable names
	//
	//    since vars never contain wchars, we don't have to be 8-bit-safe
	crm_get_pgm_arg (temp_vars, MAX_VARNAME, apb->b1start, apb->b1len);
	tvlen = crm_nexpandvar (temp_vars, apb->b1len, MAX_VARNAME);
	if (internal_trace)
	  fprintf (stderr, "  Uniting vars: ***%s***\n", temp_vars);
	
	done = 0;
	mc = 0;
	vstart = 0;
	vend = 0;
	istart = cdw->nchars;
	iend = 0;
	ilen = 0;
	i_index = -1;
	while (!done)
	  {
	    while (temp_vars[vstart] < 0x021
		   && vstart < tvlen)    //  was temp_vars[vstart] != '\000')
	      vstart++;
	    vlen = 0;
	    while (temp_vars[vstart+vlen] >= 0x021
		   && vstart+vlen < tvlen)
	      vlen++;
	    if (vlen == 0) 
	      {
		done = 1;
	      }
	    else
	      {
		long vht_index;
		//
		//        look up the variable
		vht_index = crm_vht_lookup (vht, &temp_vars[vstart], vlen);
		if (vht[vht_index] == NULL )
		  {
		    char varname[MAX_VARNAME];
		    strncpy (varname, &temp_vars[vstart], vlen);
		    varname[vlen] = '\000';
		    nonfatalerror ( "can't intersect a nonexistent variable.",
				    varname);
		    goto invoke_bailout;

		  }
		else
		  { 
		    //  it was a good var, make sure it's in the data window
		    //
		    if (vht[vht_index] -> valtxt != cdw->filetext)
		      {
			char varname[MAX_VARNAME];
			strncpy (varname, &temp_vars[vstart], vlen);
			varname[vlen] = '\000';
			nonfatalerror ( "can't intersect isolated variable.",
					varname);
			goto invoke_bailout;
		      }
		    else
		      {      //  it's a cdw variable; go for it.
			if (vht[vht_index] -> vstart < istart)
			  istart = vht[vht_index] -> vstart;
			if ((vht[vht_index]->vstart + vht[vht_index]->vlen) 
			    > iend)
			  iend = vht[vht_index]->vstart + vht[vht_index]->vlen;
		      };
		  };
	      };
	    vstart = vstart + vlen;
	    if (temp_vars[vstart] == '\000')
	      done = 1;
	  };
	//
	//      all done with the looping, set the start and length of the 
	//      output var.
	vlen = iend - istart;
	if (vlen < 0 ) vlen = 0;
	crm_nextword (out_var, ovlen, 0, &ovstart, &ovlen);
	crm_set_windowed_nvar (&out_var[ovstart], ovlen, cdw->filetext, 
			       istart, vlen,
			       csl->cstmt);
	
      }
      break;
      
     
    case CRM_DEBUG:
      {               // turn on the debugger - NOW!
	if (user_trace)
	  fprintf (stderr, "executing a DEBUG statement - drop to debug\n");
	debug_countdown = 0;
      }
      break;

    case CRM_UNIMPLEMENTED:
      {
        char bogusbuffer[1024];
	char bogusstmt [1024];
        sprintf (bogusbuffer, "Statement %ld NOT YET IMPLEMENTED !!!"
		 "Here's the text: \n",
		 csl->cstmt);
	memmove(bogusstmt, 
		&csl->filetext[csl->mct[csl->cstmt]->start],
		csl->mct[csl->cstmt+1]->start - csl->mct[csl->cstmt]->start);
	bogusstmt [csl->mct[csl->cstmt+1]->start - csl->mct[csl->cstmt]->start]
	  = '\000';
        fatalerror (bogusbuffer, bogusstmt);
	goto invoke_bailout;

      };
      break;

    default:
      {
        char bogusbuffer[1024];
	char bogusstmt [1024];
        sprintf (bogusbuffer, 
		 "Statement %ld way, way bizarre !!!  Here's the text: \n",
		 csl->cstmt);
	memmove (bogusstmt, 
		&csl->filetext[csl->mct[csl->cstmt]->start],
		csl->mct[csl->cstmt+1]->start - csl->mct[csl->cstmt]->start);
	bogusstmt [csl->mct[csl->cstmt+1]->start - csl->mct[csl->cstmt]->start]
	  = '\000';
        fatalerror (bogusbuffer, bogusstmt);
	goto invoke_bailout;
      };
    }

  //    If we're in some sort of strange abort mode, and we just need to move
  //    on to the next statement, we branch here.
 invoke_bailout:

  //  grab end-of-statement timers ?
  if (profile_execution)
    {
      times ( (void *) &timer2);
      csl->mct[tstmt]->stmt_utime += (timer2.tms_utime - timer1.tms_utime);
      csl->mct[tstmt]->stmt_stime += (timer2.tms_stime - timer1.tms_stime);
    };


  //    go on to next statement (unless we're failing, laifing, etc,
  //    in which case we have no business getting to here.
  csl->cstmt ++;

  goto invoke_top;
  
 invoke_done:

  //     give the debugger one last chance to do things.
  if (debug_countdown == 0)
    i=crm_debugger ();
  if (csl->cstmt < csl->nstmts)
    goto invoke_top;

 invoke_exit:

  //     if we asked for an output profile, give it to us.
  if (profile_execution)
    crm_output_profile (csl);

  return (status);
}
