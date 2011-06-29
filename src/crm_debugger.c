//  crm114_.c  - Controllable Regex Mutilator,  version v1.0
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




//         If we got to here, we need to run some user-interfacing
//         (i.e. debugging).
//
//         possible return values:
//         1: reparse and continue execution
//         -1: exit immediately
//         0: continue

long crm_debugger ()
{
  long ichar;
  static int firsttime = 1;
  static FILE *mytty;

  if (firsttime)
    {
      fprintf (stderr, "CRM114 Debugger - type \"h\" for help.  ");
      fprintf (stderr, "User trace turned on.\n");
      user_trace = 1;
      firsttime = 0;
      if (user_trace)
	fprintf (stderr, "Opening the user terminal for debugging I/O\n");
#ifdef POSIX
      mytty = fopen ("/dev/tty", "rb");
#endif
#ifdef WIN32
      mytty = fopen ("CON", "rb");
#endif
      clearerr (mytty);
    };

  if (csl->mct[csl->cstmt]->stmt_break > 0)
    fprintf (stderr, "Breakpoint tripped at statement %ld\n", csl->cstmt);
 debug_top:
  //    let the user know they're in the debugger
  //
  fprintf (stderr, "\ncrm-dbg> ");

  //    find out what they want to do
  //
  ichar = 0;

  while (!feof (mytty) 
	 && ichar < data_window_size - 1 
	 && (inbuf [ichar-1] != '\n' ) )	    
    {
      inbuf[ichar] = fgetc (mytty);
      ichar++;
    };  
  inbuf [ichar] = '\000';

  if (feof (mytty) )  
    {
      fprintf (stderr, "Quitting\n");
      if (engine_exit_base != 0)
	{
	  exit (engine_exit_base + 8);
	}
      else
	exit ( EXIT_SUCCESS );
    };


  //   now a big siwtch statement on the first character of the command
  //
  switch (inbuf[0])
    {
    case 'q':
    case 'Q':
      {
	if (user_trace)
	  fprintf (stderr, "Quitting.\n");
	if (engine_exit_base != 0)
	  {
	    exit (engine_exit_base + 9);
	  }
	else
	  exit ( EXIT_SUCCESS );
      };
      break;
    case 'n':
    case 'N':
      {
	debug_countdown = 0;
	return (0);
      }
      break;
    case 'c':
    case 'C':
     {
       sscanf (&inbuf[1], "%ld", &debug_countdown);
       if (debug_countdown <= 0) 
	 {
	   debug_countdown = -1;
	   fprintf (stderr, "continuing execution...\n");
	 }
       else
	 {
	   fprintf (stderr, "continuing %ld cycles...\n", debug_countdown);
	 };
       return (0);
     };
     break;
    case 't':
      if (user_trace == 0 ) 
	{
	  user_trace = 1 ;
	  fprintf (stderr, "User tracing on");
	}
      else
	{
	  user_trace = 0;
	  fprintf (stderr, "User tracing off");
	};
      break;
    case 'T':
      if (internal_trace == 0 ) 
	{
	  internal_trace = 1 ;
	  fprintf (stderr, "Internal tracing on");
	}
      else
	{
	  internal_trace = 0;
	  fprintf (stderr, "Internal tracing off");
	};
      break;
    case 'e':
      {
	fprintf (stderr, "expanding: \n");
	memmove (inbuf, &inbuf[1], strlen (inbuf) -1);
	crm_nexpandvar (inbuf, strlen(inbuf) -1, data_window_size);
	fprintf (stderr, "%s", inbuf);
      };
      break;
    case 'i':
      {
	fprintf (stderr, "Isolating %s", &inbuf[1]);
	fprintf (stderr, "NOT YET IMPLEMENTED!  Sorry.  \n");
      }
      break;
    case 'v':
      {
	long i, j;
	long stmtnum;
	i = sscanf (&inbuf[1], "%ld", &stmtnum);
	if (i > 0)
	  {
	    fprintf (stderr, "statement %ld: ", stmtnum);
	    if ( stmtnum < 0 || stmtnum > csl->nstmts)
	      {
		fprintf (stderr, "... does not exist!\n");
	      }
	    else
	      {
		for ( j = csl->mct[stmtnum]->start;
		      j < csl->mct[stmtnum+1]->start;
		      j++)
		  {
		    fprintf (stderr, "%c", csl->filetext[j]);
		  };
		
	      };
	  }
	else
	  {
	    fprintf (stderr, "What statement do you want to view?\n");
	  }
      };
      break;
    case 'j':
      {
	long nextstmt;
	long i;
	long vindex;
	i = sscanf (&inbuf[1], "%ld", &nextstmt);
	if (i == 0)
	  {
	    //    maybe the user put in a label?
	    long tstart;
	    long tlen;
	    crm_nextword (&inbuf[1], strlen (&inbuf[1]), 0,
			  &tstart, &tlen);
	    memmove (inbuf, &inbuf[1+tstart], tlen);
	    inbuf[tlen] = '\000';
	    vindex= crm_vht_lookup (vht, inbuf, tlen);
	    if (vht[vindex] == NULL)
	      {
		fprintf (stderr, "No label '%s' in this program.  ", inbuf);
		fprintf (stderr, "Staying at line %ld\n", csl->cstmt);
		nextstmt = csl->cstmt;
	      }
	    else
	      {
		nextstmt = vht[vindex]->linenumber;
	      };
	  };
	if (nextstmt <= 0) 
	  {
	    nextstmt = 0;
	  }
	if (nextstmt >= csl->nstmts)
	  {
	    nextstmt = csl-> nstmts;
	    fprintf (stderr, "last statement is %ld, assume you meant that.\n",
		     csl->nstmts);
	  };
	if (csl->cstmt != nextstmt)
	  fprintf (stderr, "Next statement is statement %ld\n", nextstmt);
	csl->cstmt = nextstmt;
      }
      return (1);
      break;
    case 'b':
      {                  //  is there a breakpoint to make?
	long breakstmt;
	long i;
	long vindex;
	breakstmt = -1;
	i = sscanf (&inbuf[1], "%ld", &breakstmt);
	if (i == 0)
	  {
	    //    maybe the user put in a label?
	    long tstart;
	    long tlen;
	    crm_nextword (&inbuf[1], strlen (&inbuf[1]), 0,
			  &tstart, &tlen);
	    memmove (inbuf, &inbuf[1+tstart], tlen);
	    inbuf[tlen] = '\000';
	    vindex= crm_vht_lookup (vht, inbuf, tlen);
	    fprintf (stderr, "vindex = %ld\n", vindex);
	    if (vht[vindex] == NULL)
	      {
		fprintf (stderr, "No label '%s' in this program.  ", inbuf);
		fprintf (stderr, "No breakpoint change made. \n");
	      }
	    else
	      {
		breakstmt = vht[vindex]->linenumber;
	      };
	  };
	if (breakstmt  <= -1) 
	  {
	    breakstmt = 0;
	  }
	if (breakstmt >= csl->nstmts)
	  {
	    breakstmt = csl-> nstmts;
	    fprintf (stderr, "last statement is %ld, assume you meant that.\n",
		     csl->nstmts);
	  };
	csl->mct[breakstmt]->stmt_break = 1 - csl->mct[breakstmt]->stmt_break;
	if (csl->mct[breakstmt]->stmt_break == 1)
	  {
	    fprintf (stderr, "Setting breakpoint at statement %ld\n", 
		     breakstmt);
	  }
	else
	  {
	    fprintf (stderr, "Clearing breakpoint at statement %ld\n", 
		     breakstmt);
	  };
      }
      return (1);
      break;
    case 'a':
      {                  //  do a debugger-level alteration
	//    maybe the user put in a label?
	long vstart, vlen;
	long vindex;
	long ostart, oend, olen;
	crm_nextword (&inbuf[1], strlen (&inbuf[1]), 0, 
		      &vstart, &vlen);
	memmove (inbuf, &inbuf[1+vstart], vlen);
	inbuf[vlen] = '\000';
	vindex= crm_vht_lookup (vht, inbuf, vlen);
	if (vht[vindex] == NULL)
	  {
	    fprintf (stderr, "No variable '%s' in this program.  ", inbuf);
	  }
	
	//     now grab what's left of the input as the value to set
	//
	ostart = vlen + 1;
	while (inbuf[ostart] != '/' && inbuf[ostart] != '\000') ostart++;
	ostart++;
	oend = ostart + 1;
	while (inbuf[oend] != '/' && inbuf[oend] != '\000') oend++;

	memmove (outbuf, 
		 &inbuf[ostart],
		 oend - ostart);

	outbuf [oend - ostart] = '\000';
	olen = crm_nexpandvar (outbuf, oend - ostart, data_window_size);
	crm_destructive_alter_nvariable (inbuf, vlen, outbuf, olen);
      };
      break;
    case 'f':
      {
	csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
	fprintf (stderr, "Forward to }, next statement : %ld\n", csl->cstmt);
	csl->aliusstk [csl->mct[csl->cstmt]->nest_level] = -1;
      };
      return (1);
      break;
    case 'l':
      {
	csl->cstmt = csl->mct[csl->cstmt]->liaf_index;
	fprintf (stderr, "Backward to {, next statement : %ld\n", csl->cstmt);
      };
      return (1);
      break;
    case 'h':
      {
	fprintf (stderr, "a :var: /value/ - alter :var: to /value/ \n");
	fprintf (stderr, "b <n> - toggle breakpoint on line <n> \n");
	fprintf (stderr, "b <label> - toggle breakpoint on <label> \n");
	fprintf (stderr, "c - continue execution till breakpoint or end\n");
	fprintf (stderr, "c <n> - execute <number> more statements\n");
	fprintf (stderr, "e - expand an expression\n");
	fprintf (stderr, "f - fail forward to block-closing '}'\n");
	fprintf (stderr, "j <n> - jump to statement <number>\n");
	fprintf (stderr, "j <label> - jump to statement <label>\n");
	fprintf (stderr, "l - liaf backward to block-opening '{'\n");
	fprintf (stderr, "n - execute next statement (same as 'c 1')\n");
	fprintf (stderr, "q - quit the program and exit\n");
	fprintf (stderr, "t - toggle user-level tracing\n");
	fprintf (stderr, "T - toggle system-level tracing\n");
	fprintf (stderr, "v <n> - view source code statement <n>\n");
      };
      break;
    default:
      {
	fprintf (stderr, "Command unrecognized - type \"h\" for help. \n");
      };
    };
  goto debug_top;
}

