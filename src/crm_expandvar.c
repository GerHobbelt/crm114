//  crm_expandvar.c  - Controllable Regex Mutilator,  version v1.0
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


/*
   [i_a] Note to self: this code is chocckfull of security risks, i.e. buffer overflow issues.
   And nonreentrant to boot.

   Overhaul when you've found the energy to do so...
*/

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

//
//
//     crm_nexpandvar - given a string and it's length, go through it
//     and if there's a variable expansion called for (by the :*:
//     operator) expand the variable.
//
//     the inputs are a buffer with the NULL-safe string in it, the
//     length of this string, and the maximum allocated length of the
//     buffer.  This function returns the new length of the buffer.
//     It will NOT increase the buffer length past maxlen, so
//     expansions beyond that will cause a nonfatal error and be
//     aborted.
//
//     Algorithm: 
//     1) efficiency check- do we need to do any expansions at all.
//     2) Start at buf[0], work up to buf[buflen]-3
//     2a) do \n, \r, \a, \xHH and \oOOO
//     3) are we looking at :*:?  
//     4) no: copy 1 character, increment from and to indexes, go to step 3
//     5) yes: skip from index ahead 3, from there to next : is the varname
//     6) copy var value to tbuf, incrementing tobuf index.
//     7) set from-index to third colon index + 1
//     8) go to 2 (modulo last two chars need copying)
//

long crm_nexpandvar (char *buf, long inlen, long maxlen)
{
  return (crm_zexpandvar (buf, 
			  inlen, 
			  maxlen,
			  NULL,
			  CRM_EVAL_ANSI 
			  | CRM_EVAL_STRINGVAR
			  | CRM_EVAL_REDIRECT
			  ));
}

//
// crm_qexpandvar is the "full evaluate one pass of everything" mode.

long crm_qexpandvar (char *buf, long inlen, long maxlen, long *qex_stat)
{
  return (crm_zexpandvar (buf,
			  inlen,
			  maxlen,
			  qex_stat,
			  CRM_EVAL_ANSI 
			  | CRM_EVAL_STRINGVAR 
			  | CRM_EVAL_REDIRECT
			  | CRM_EVAL_STRINGLEN
			  | CRM_EVAL_MATH ));
}
 
//     crm_zexpandvar - "expanded" expandvar.  Does all the expansions, 
//     but does not repeat the evaluations.  If you want repeats, you
//     must do that yourself (that way, this function will always
//     move down the string at least one character, and thus this
//     function will always terminate,  Nice, that.  :) 
//
//     the inputs are a buffer with the NULL-safe string in it, the
//     length of this string, and the maximum allocated length of the
//     buffer.  This function returns the new length of the buffer.
//     It will NOT increase the buffer length past maxlen, so
//     expansions beyond that will cause a nonfatal error and be
//     aborted.
//
//     Algorithm: 
//     1) efficiency check- do we need to do any expansions at all.
//     2) Start at buf[0], work up to buf[buflen]-3
//     2a) do \n, \r, \a, \xHH and \oOOO
//     3) are we looking at :<some-operator>:?  
//     4) no: copy 1 character, increment from and to indexes, go to step 3
//     5) yes: skip from index ahead 3, from there to next : is the varname
//     6) copy var value to tbuf, incrementing tobuf index.
//     7) set from-index to third colon index + 1
//     8) go to 2 (modulo last two chars need copying)
//
long crm_zexpandvar (char *buf, 
		     long inlen, 
		     long maxlen, 
		     long *retstat, 
		     long exec_bitmask)
{
  long is, id;
  long vht_index;
  long q;

  //  the maximum length allocated so far for these random buffers...
  static long current_maxlen = 0;
  //  a temporary work buffer...
  static char *tbuf = NULL;
  //  and another for variable names...
  static char *vname = NULL; 
  
  char *cp;
  long vlen;
  
  char opchar;
  
  //    efficiency check - do we even _have_ a :*: in the buffer?
  //
  
  if (inlen == 0)
    return (0);
  
  is= 0; id = 0; 

  if (internal_trace) 
    fprintf (stderr, "qexpandvar on =%s= len: %ld bitmask: %ld\n", 
	     buf, inlen, exec_bitmask);
  
  //  GROT GROT GROT must fix this for 8-bit safe error messages
  if (inlen > maxlen)
    {
      q = fatalerror (
		      "You have blown the gaskets while building a string.  Orig string was: ",
		      buf);
      if (q == 0 )
	  {
		/* return (inlen); -- this is a serious buffer overflow risk as any 
		   using routines will assume the maxlen will never be surpassed! */			
		return maxlen;
	  }
      goto bailout;
    }  
  
  //   First thing- do the ANSI \-Expansions
  //
  if (exec_bitmask & CRM_EVAL_ANSI)
    {
      is = 0;
      id = 0;
      if (internal_trace)
	fprintf (stderr, " Doing backslash expansion\n");
      for (is = 0; is <= inlen ; is++)  
	{
	  if (buf[is] != '\\' )
	    {
	      buf [id] = buf [is];
	      id++;
	    }
	  else
	    {
	      //  we're looking at a '\\'.
	      //  
	      //   Check for a few common things: \n, \a, \xNN, \oNNN
	      is++;
	      //
	      switch (buf[is])
		{
		case '0':
		  {
		    //   it's a NULL.
		    buf[id] = '\0';
		    id++;
		  }
		  break;
		case 'b':
		  {
		    //   it's a backspace
		    buf[id] = '\b';
		    id++;
		  }
		  break;
		case 't':
		  {
		    //   it's a tab
		    buf[id] = '\t';
		    id++;
		  }
		  break;
		case 'n':
		  {
		    //   it's a newline.  stuff in a newline.
		    buf[id] = '\n';
		    id++;
		  }
		  break;
		case 'v':
		  {
		    //   it's a vtab
		    buf[id] = '\v';
		    id++;
		  }
		  break;
		case 'f':
		  {
		    //   it's a form feed.
		    buf[id] = '\f';
		    id++;
		  }
		  break;
		case 'r':
		  {
		    //   it's a carriage return
		    buf[id] = '\r';
		    id++;
		  }
		  break;
		case 'a':
		  {
		    //   it's a BELL.  put that in.
		    buf[id] = '\a';
		    id++;
		  }
		  break;
		case 'x':
		case 'X':
		  {
		    //   it might be a hex char constant.  read it 
		    //    and stuff it.
		    unsigned int value;
		    int conv_count;
		    conv_count = 0;
		    value = '\000';
		    if (is+2 < inlen)      // watch out for end-of-string
		      conv_count = sscanf (&buf[is+1], "%2X", &value);
		    if (conv_count == 1)
		      {
			buf[id] = value;
			id++;
			is++; is++;  // move over the hex digits
		      }
		    else	//   and otherwise, just copy the x
		      {
			buf[id] = buf[is];
			id++;
		      }
		  }
		  break;
		case 'o':
		case 'O':
		  {
		    //   it might be an octal char constant.  read it 
		    //   and stuff it.
		    unsigned int value;
		    int conv_count ;
		    conv_count = 0;
		    value = '\000';
		    if (is+3 < inlen)     // watch out for end-of-string
		      conv_count = sscanf (&buf[is+1], "%3o", &value);
		    if (conv_count == 1)
		      {
			buf[id] = value;
			id++;
			is++; is++; is++;  // move over the octal digits
		      }
		    else	//   and otherwise, just copy the conv. char.
		      {
			buf[id] = buf[is];
			id++;
		      }
		  }
		  break;
		case '>':
		case ')':
		case ']':
		case '/':
		case ';':
		case '{':
		case '}':
		case '#':
		case '\\':
		  {
		    //      >, ), ], ;, {, }, #, and / are themselves after a '\',
		    //    but need the \ escape to pass thru the parser
		    //    without terminating their enclosed args
		    buf[id] = buf[is];
		    id++;
		  }
		  break;
		default:
		  {
		    //       if it's "none of the above" characters, then
		    //       the '\' character _stays_ as a literal
		    buf[id] = '\\';
		    id++;
		    buf[id] = buf[is];
		    id++;
		  }
		  break;
		}
	    }
	}
      //     and update the new inlen
      buf[id] = '\000';    // needed because slimy old GNU REGEX needs it.
      //   and take one off for inlen, because it always gets incremented one 
      //   extra time
      inlen = id - 1;

      if (internal_trace)
	fprintf (stderr, "backslash expansion yields: =%s= len %ld \n", buf, inlen);
    }
  //    END OF ANSI \-EXPANSIONS
      


  //    Do a quick check for :'s - this is just a speedup, as all further 
  //    operators use the : notation.

  //    if no :, then no ":" operators possible.
  cp = memchr (buf, ':', inlen);
  if (cp == NULL)
    {
      if (internal_trace)
	fprintf (stderr, "No further expansions possible");
      return (inlen);
    }
  

  //    allocate some memory for tbuf and vname; (this funky allocation
  //    is a workaround for malloc memory fragmentation that caused
  //    out-of-memory problems in some kernels.  Eventually we'll have
  //    a much grander system for all mallocs, but not yet.)

  //   if the currently allocated buffers are too small, drop them
  //   (and force a reallocation), else we will reuse them.
  if (current_maxlen < maxlen + 1)     // do we need to drop and reallocate? 
    {
      if (tbuf != NULL) 
	{
	  free (tbuf);
	  tbuf = NULL;
	}
      if (vname != NULL) 
	{
	  free (vname);
	  vname = NULL;
	}
      current_maxlen = maxlen + 2;
    }
  
  if (tbuf == NULL )
    {
      tbuf = (char *) malloc (current_maxlen * sizeof(tbuf[0]));  /* [i_a] */
    }
  
  if (vname == NULL)
    vname = (char *) malloc (current_maxlen * sizeof(vname[0])); /* [i_a] */
 

  if (tbuf == NULL || vname == NULL)
    {
      q = fatalerror ("Couldn't allocate memory for Q-variable expansion!",
		      "Try making the window set smaller with the -w option");
      if (q == 0)
	return (inlen);
    }
  
  //    OK, we might have a :*: substitution operator, so we actually have
  //    to do some work.

  //
  //    Now, do we have a :*: (singlevar) possible?

  if ( exec_bitmask & CRM_EVAL_STRINGVAR )
    {
      
      is = 0;    //   is is the input position index
      id = 0;    //   id is the destination position index
      if (internal_trace)
	fprintf (stderr, "Doing singlevar eval expansion\n");

      //
      //   First time through the loop, for :*: (variable expansion)
      //
      for (is = 0; is <= inlen && id < maxlen; is++)
	{
	  if (is <= inlen - 5  //  check only if :*:c:" possible
	      && buf[is] == ':' 
	      && buf[is+1] == '*' 
	      && ( buf[is+2] ==':' ))
	    {
	      //   yes, it's probably an expansion of some sort.
	      opchar = buf[is+1];
	      //    copy everything from the colon to the second colon
	      //    ( or the end of the string) into the vname buffer.
	      is = is + 2;
	      vname [0] = buf[is];
	      vlen = 1;
	      is++;
	      while (is < maxlen
		     && is <= inlen
		     && buf [is] != ':')
		{
		  vname[vlen] = buf[is];
		  is++;
		  vlen++;
		}
	      //
	      //    check for the second colon as well...
	      if (buf[is] == ':')
		{
		  vname[vlen] = ':';
		  vlen++;
		}
	      vname [vlen] = '\000';
	      
	      //
	      //      Now we've got the variable name in vname, we can 
	      //      go get it's value and copy _that_ into tbuf as well.
	      if (internal_trace)
		fprintf (stderr, "looking up variable >%s<\n", vname);
	      vht_index = crm_vht_lookup (vht, vname, vlen);
	      
	      if (vht[vht_index] == NULL)
		{
		  //      there was no variable by that name, use the text itself
		  switch (opchar)
		    {
		    case '*':
		      {
			//
			//    simply copy text till the close colon
			//
			for (q = 0; q < vlen && id < maxlen; q++)
			  {
			    tbuf[id] = vname[q];
			    id++;
			  }
		      }
		      break;
		    }
		}
	      else
		{
		  //     There really was a variable value by that name.
		  //     suck it out, and splice it's text value
		  
		  //   check to see if it's one of the self-mutating 
		  //    internal variables, like :_iso: or :_cd:
		  
		  if (strncmp(
			      (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
			      ":_", 2) == 0)
		    {
		      if (strncmp(
				  (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
				  ":_iso:", 6) == 0)
			{
			  //   if this was :_iso:, update iso's length
			  vht[vht_index]->vlen = tdw->nchars;
			}
		      if (strncmp(
				  (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
				  ":_cs:", 5) == 0)
			{
			  //   if this was :_cs:, update the current line num
			  char lcstring [32];
			  long lclen;
			  lcstring[0] = '\0';
			  lclen = sprintf (lcstring, "%ld", csl->cstmt);
			  crm_set_temp_nvar (":_cs:", lcstring, lclen); 
			}
		    }
		  
		  switch (opchar)
		    {
		    case '*':
		      {
			for (q = 0; q < vht[vht_index]->vlen && id < maxlen; q++)
			  {
			    tbuf[id] = vht[vht_index]->valtxt
			      [(vht[vht_index]->vstart)+q];
			    id++;
			  }
		      }
		      break;
		    }
		}
	    }
	  //         Now, handle the case where we were NOT looking at
	  //         :*:c: in buf
	  else
	    {
	      tbuf[id] = buf[is];
	      id++;
	    }
	}

      //    and put our results back into buf       
      memcpy (buf, tbuf, id);
      buf[id] = '\000';
      inlen = id - 1;

      if (internal_trace)
	fprintf (stderr, " :*: var-expansion yields: =%s= len %ld \n", buf, inlen);

    }  

  //     END OF :*: EXPANSIONS


  //
  //    Now, do we have a :+: (REDIRECT) operators

  if ( exec_bitmask & CRM_EVAL_REDIRECT )
    {
      
      is = 0;    //   is is the input position index
      id = 0;    //   id is the destination position index
      if (internal_trace)
	fprintf (stderr, "Doing singlevar redirect expansion\n");

      //
      //   First time through the loop, for :+: (variable expansion)
      //
      for (is = 0; is <= inlen && id < maxlen; is++)
	{
	  if (is <= inlen - 5  //  check only if :*:c:" possible
	      && buf[is] == ':' 
	      && buf[is+1] == '+' 
	      && ( buf[is+2] ==':' ))
	    {
	      //   yes, it's probably an expansion of some sort.
	      //    copy everything from the colon to the second colon
	      //    ( or the end of the string) into the vname buffer.
	      is = is + 2;
	      vname [0] = buf[is];
	      vlen = 1;
	      is++;
	      while (is < maxlen
		     && is <= inlen
		     && buf [is] != ':')
		{
		  vname[vlen] = buf[is];
		  is++;
		  vlen++;
		}
	      //
	      //    check for the second colon as well...
	      if (buf[is] == ':')
		{
		  vname[vlen] = ':';
		  vlen++;
		}
	      vname [vlen] = '\000';
	      
	      //
	      //      Now we've got the variable name in vname, we can 
	      //      go get it's value and copy _that_ into the vname buffer
	      if (internal_trace)
		fprintf (stderr, "looking up variable >%s<\n", vname);
	      vht_index = crm_vht_lookup (vht, vname, vlen);
	      
	      if (vht[vht_index] == NULL)
		{
		  //   no op - if no such variable, no change...
		}
	      else
		{
		  //     There really was a variable value by that name.
		  //     suck it out, and make that the new vname text
		  
		  //   if this was :_iso:, update iso's length
		  if (strncmp(
			      (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
			      ":_iso:", 6) == 0)
		    {
		      vht[vht_index]->vlen = tdw->nchars;
		    }
		  
		  for (q = 0; q < vht[vht_index]->vlen && id < maxlen; q++)
		    {
		      vname[q] = vht[vht_index]->valtxt
			[(vht[vht_index]->vstart)+q];
		    }
		  //   note that vlen is varname len, but vht[]->vlen is
		  //    the length of the text.  Bad choice of names, eh?
		  vlen = vht[vht_index]->vlen;
		}
	      //      Second time around:
	      //      We have something in vname (either the indirected
	      //      varname, or the original varname), we can 
	      //      go get it's value and copy _that_ into tbuf as well.
	      if (internal_trace)
		fprintf (stderr, "Second lookup variable >%s<\n", vname);
	      vht_index = crm_vht_lookup (vht, vname, vlen);
	      
	      if (vht[vht_index] == NULL)
		{
		  //
		  //    simply copy text including the close colon
		  //
		  for (q = 0; q < vlen && id < maxlen; q++)
		    {
		      tbuf[id] = vname[q];
		      id++;
		    }
		}
	      else
		{
		  //     There really was a variable value by that name.
		  //     suck it out, and splice it's text value
		  
		  //   if this was :_iso:, update iso's length
		  if (strncmp(
			      (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
			      ":_iso:", 6) == 0)
		    {
		      vht[vht_index]->vlen = tdw->nchars;
		    }
		  {
		    for (q = 0; q < vht[vht_index]->vlen && id < maxlen; q++)
		      {
			tbuf[id] = vht[vht_index]->valtxt
			  [(vht[vht_index]->vstart)+q];
			id++;
		      }
		  }
		}
	    }
	  //         Now, handle the case where we were NOT looking at
	  //         :+:c: in buf
	  else
	    {
	      tbuf[id] = buf[is];
	      id++;
	    }
	}
      //    and put our results back into buf       
      memcpy (buf, tbuf, id);
      buf[id] = '\000';
      inlen = id - 1;
      if (internal_trace)
	fprintf (stderr, "indirection :+: expansion yields: =%s= len %ld \n", buf, inlen);
    }  
  
  //     END OF :+: EXPANSIONS
  
  
  if (exec_bitmask & CRM_EVAL_STRINGLEN)
    {
      //
      //
      //   Expand :#: (string lengths)
      //
      if (internal_trace)
	fprintf (stderr, "Doing stringglength expansion\n");

      buf[id] = '\000';
      if (internal_trace)
	fprintf (stderr, " var-expand yields: =%s= len %ld\n", buf, inlen);
      id = 0;
      for (is = 0; is <= inlen && id < maxlen; is++)
	{
	  if (is <= inlen - 5  //  check only if :#:c:" possible
	      && buf[is] == ':' 
	      && ( buf[is+1] == '#' )
	      && buf[is+2] ==':')
	    {
	      //   yes, it's probably an expansion of some sort.
	      opchar = buf[is+1];
	      //    copy everything from the colon to the second colon
	      //    into the vname buffer.
	      is = is + 2;
	      vname [0] = buf[is];
	      vlen = 1;
	      is++;
	      while (is < maxlen
		     && is <= inlen
		     && buf [is] != ':')
		{
		  vname[vlen] = buf[is];
		  is++;
		  vlen++;
		}
	      //
	      //    check for the second colon as well...
	      if (buf[is] == ':')
		{
		  vname[vlen] = ':';
		  vlen++;
		}
	      vname [vlen] = '\000';
	      
	      //
	      //      Now we've got the variable name in vname, we can 
	      //      go get it's value and copy _that_ into tbuf as well.
	      if (internal_trace)
		fprintf (stderr, "looking up variable >%s<\n", vname);
	      vht_index = crm_vht_lookup (vht, vname, vlen);
	      
	      if (vht[vht_index] == NULL)
		{
		  //      there was no variable by that name, use the 
		  //      text itself
		  switch (opchar)
		    {
		    case '#':
		      {
			char lentext[MAX_VARNAME];
			int m, mm;
			//   the vlen-2 is because we need to get
			//    rid of the ':' 
			sprintf (lentext, "%ld", vlen-2);
			mm = strlen (lentext);
			for (m = 0; m < mm && id < maxlen; m++)
			  {
			    tbuf[id] = lentext[m];
			    id++;
			  }
		      }
		      break;
		    }
		}
	      else
		{
		  //     There really was a variable value by that name.
		  //     suck it out, and splice it's text value
		  
		  //   if this was :_iso:, update iso's length
		  if (strncmp(
			      (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
			      ":_iso:", 6) == 0)
		    {
		      vht[vht_index]->vlen = tdw->nchars;
		    }
		  
		  switch (opchar)
		    {
		    case '#':
		      {
			//
			//   Actually, we want the _length_ of the variable
			//
			char lentext[MAX_VARNAME];
			int m, mm;
			sprintf (lentext, "%ld", vht[vht_index]->vlen);
			mm = strlen (lentext);
			for (m = 0; m < mm && id < maxlen; m++)
			  {
			    tbuf[id] = lentext[m];
			    id++;
			  }
		      }
		      break;
		    }
		}
	    }
	  //         Now, handle the case where we were NOT looking at
	  //         :*:c: in buf
	  else
	    {
	      tbuf[id] = buf[is];
	      id++;
	    }
	}
      //    and put our results back into buf       
      memcpy (buf, tbuf, id);
      buf[id] = '\000';
      //    and because id always gets an extra increment...
      inlen = id - 1;
      if (internal_trace)
	fprintf (stderr, " strlen :#: expansion yields: =%s= len %ld \n", buf, inlen);

    }
  //       END OF :#: STRING LENGTH EXPANSIONS


  //       Do we have any math expansions?
  if (exec_bitmask & CRM_EVAL_MATH)
    {
  
      //
      //       handle :@:  (math evaluations)
      //
      //
      if (internal_trace)
	fprintf (stderr, "Doing math expansion\n");

      buf[id] = '\000';
      if (internal_trace)
	fprintf (stderr, " length-expand yields: =%s= len %ld\n", buf, inlen);
      id = 0;
      for (is = 0; is <= inlen && id < maxlen; is++)
	{
	  if (is <= inlen - 5  //  check only if :*:c:" possible
	      && buf[is] == ':' 
	      && ( buf[is+1] == '@' )
	      && buf[is+2] ==':')
	    {
	      //   yes, it's probably an expansion of some sort.
	      opchar = buf[is+1];
	      //    copy everything from the colon to the second colon
	      //    into the vname buffer.
	      is = is + 2;
	      vname [0] = buf[is];
	      vlen = 1;
	      is++;
	      while (is < maxlen
		     && is <= inlen
		     && buf [is] != ':')
		{
		  vname[vlen] = buf[is];
		  is++;
		  vlen++;
		}
	      //
	      //    check for the second colon as well...
	      if (buf[is] == ':')
		{
		  vname[vlen] = ':';
		  vlen++;
		}
	      else
		{
		  nonfatalerror ("This math eval didn't end with a ':' which is",
				 " often an error...  check it sometime? ");
		}
	      vname [vlen] = '\000';
	      
	      //
	      //      Now we've got the variable name in vname, we can 
	      //      go get it's value and copy _that_ into tbuf as well.
	      if (internal_trace)
		fprintf (stderr, "looking up variable >%s<\n", vname);
	      vht_index = crm_vht_lookup (vht, vname, vlen);
	      
	      if (vht[vht_index] == NULL)
		{
		  //      there was no variable by that name, use the text itself
		  switch (opchar)
		    {
		    case '@':
		      {
			char mathtext[MAX_VARNAME];
			int m, mm;
			memcpy (mathtext, &vname[1], vlen-2);
			mathtext[vlen-2] = '\000';
			if (internal_trace)
			  fprintf (stderr, "In-Mathtext is -'%s'-\n", mathtext); 
			m = strmath (mathtext, vlen-2, MAX_VARNAME, retstat);
			if (internal_trace)
			  fprintf (stderr, "Out-Mathtext is -'%s'-\n", mathtext);
			if (retstat && *retstat < 0)
			  {
			    q = fatalerror ("Problem during math evaluation of ",
					    mathtext);
			    if (q == 0)
			      return (inlen);
			    goto bailout;
			  }
			mm = strlen (mathtext);
			for (m = 0; m < mm && id < maxlen; m++)
			  {
			    tbuf[id] = mathtext[m];
			    id++;
			  }
		      }
		      break;
		    }
		}
	      else
		{
		  //     There really was a variable value by that name.
		  //     suck it out, and splice it's text value
		  
		  //   if this was :_iso:, update iso's length
		  if (strncmp(
			      (char *) &vht[vht_index]->nametxt[vht[vht_index]->nstart],
			      ":_iso:", 6) == 0)
		    {
		      vht[vht_index]->vlen = tdw->nchars;
		    }
		  
		  switch (opchar)
		    {
		    case '@':
		      {
			char mathtext[MAX_VARNAME];
			int m, mm;
			m = 0;
			for (q = 0; q < vht[vht_index]->vlen && m < maxlen; q++)
			  {
			    mathtext[m] = vht[vht_index]->valtxt
			      [(vht[vht_index]->vstart)+q];
			    m++;
			  }
			mathtext[vlen-1] = '\000';
			m = strmath (mathtext, vlen-2, MAX_VARNAME, retstat );
			if (retstat && *retstat < 0)
			  {
			    q = fatalerror ("Problem during math evaluation of ",
					    mathtext);
			    if (q == 0)
			      return (inlen);
			    goto bailout;
			  }
			mm = strlen (mathtext);
			for (m = 0; m < mm && id < maxlen; m++)
			  {
			    tbuf[id] = mathtext[m];
			    id++;
			  }
		      }
		      break;
		    }
		}
	    }
	  //         Now, handle the case where we were NOT looking at
	  //         :*:c: in buf
	  else
	    {
	      tbuf[id] = buf[is];
	      id++;
	    }
	}
      //    and put our results back into buf       
      memcpy (buf, tbuf, id);
      buf [id] = '\000';
      inlen = id - 1;

      if (internal_trace)
	fprintf (stderr, " math-expand yields: =%s= len %ld\n", buf, inlen);

    }

  //    END OF :@: MATH EXPANSIONS

  //    That's all, folks!  Clean up the temporary buffer.  We null-terminate
  //    it in case we need to do stupid non-8-bit-clean IO on it.
  tbuf[inlen+1] = '\000';
  
  //     We reuse tbuf and vname from now on.
  // free (tbuf);
  //free (vname);
  if (internal_trace)
    {
      fprintf (stderr, " Returned length from qexpandvar is %ld\n", inlen);
      if (retstat) 
	fprintf (stderr, "retstat was: %ld\n", *retstat);
    }
  return (inlen);
 bailout:
  return (inlen);
}

////////////////////////////////////////////////////////////////////
//
//     crm_restrictvar - hand this routine a []-string, and it hands
//     you back the VHT index, the applicable char* for the start, and
//     the length.  Or maybe an error.  
//
//     Error codes: 0 == none, 1 = nonfatal, 2 = fatal
//
//     Algorithm: first "nextword" thing is always the var.  Grab it,
//     and get the pertinent info from the VHT.  Successive nextwords
//     get either a /regex/ or a numeric (or possibly a numeric pair).
//     On each one, do successive regexing/indexranging.  When no more
//     nextwords, you're done.
//     

long crm_restrictvar ( char *boxstring,
                       long boxstrlen,
		       long *vht_idx,
                       char **outblock,
                       long *outoffset,
		       long *outlen,
                       char *errstr)
{
  char datastring[MAX_PATTERN + 1];
  long datastringlen;

  char varname[MAX_PATTERN + 1];
  long varnamelen;
  long vmidx;

  regex_t preg;
  regmatch_t matches [MAX_SUBREGEX];

  char scanbuf[MAX_PATTERN + 1];
  long scanbuflen;
  
  long nw_start, nw_len;

  char *mdw;               //  the data window that this var is stored in.
  char *start_ptr;
  long actual_offset;
  long actual_len;
  long in_subscript;

  long i, j;

  nw_start = 0;
  nw_len = 0;

  if (user_trace)
    fprintf (stderr, "Performing variable restriction.\n");
  
  //     Expand the string we were handed.
  assert(boxstrlen < MAX_PATTERN);
  memcpy (datastring, boxstring, boxstrlen);
  datastring[boxstrlen] = '\0';

  if (user_trace)
    fprintf (stderr, "Variable before expansion '%s' len %ld\n",
	     datastring, boxstrlen);
  datastringlen = crm_qexpandvar(datastring, boxstrlen, MAX_PATTERN, NULL); 
  if (user_trace)
    fprintf (stderr, "Variable after expansion: '%s' len %ld\n", 
	     datastring, datastringlen);
 
  //     Get the variable name.
  crm_nextword (datastring, datastringlen, nw_start, &nw_start, &nw_len);
  if (internal_trace)
    fprintf (stderr, "box-parsing varname got start: %ld, len: %ld .\n",
	     nw_start, nw_len);

  if (nw_len > 0)
    {
      memcpy (varname, &datastring[nw_start], nw_len);
      varname[nw_len] = '\0';
      varnamelen = nw_len;
    }
  else
    {
      //    if no variable, use :_dw:
      memcpy (varname, ":_dw:", 6);
      varnamelen = 5;
    }
  if (user_trace)
    fprintf (stderr, "Using variable '%s' for source.\n", varname);

  //      Got the varname.  Do a lookup.
  vmidx = crm_vht_lookup (vht, varname, varnamelen);
  //  fprintf (stderr, "vmidx = %ld, vht[vmidx] = %lx\n", vmidx, vht[vmidx]);

  //       Is it a real variable?
  if ( ((void *) vht[vmidx]) == NULL )  
    {
      strcpy ( errstr, 
	       "This program wants to use a nonexistent variable named: '");
      strncat ( errstr, varname, MAX_PATTERN - 128);
      strcat (errstr, "'");
      return (-2);
    }
  
  //     Get the data window - cdw, or tdw.
  mdw = cdw->filetext;    //  assume cdw unless otherwise proven...
  if (vht[vmidx]->valtxt == tdw->filetext)
    mdw=tdw->filetext;
  //  sanity check - must be tdw or cdw for searching!
  if (vht[vmidx]->valtxt != tdw->filetext
      && vht[vmidx]->valtxt != cdw->filetext)
    {
      errstr[0] = '\0';
      strcat (errstr, "Bogus text block (neither cdw nor tdw) on var ");
      strcat (errstr, varname);
      strcat (errstr, "\n");
      return (-2);
    }

  if (user_trace)
    fprintf (stderr, "Found that variable\n");
  actual_offset = vht[vmidx]->vstart;
  actual_len = vht[vmidx]->vlen;

  //     Now, we can go through the remaining terms in the var restriction 
  //     and chop the maximal region down (if desired)
  
  in_subscript = 0;
  while ( nw_start <= datastringlen )
    {
      if (user_trace) 
	fprintf (stderr, 
		 "Checking restriction at start %ld len %ld (subscr=%ld)\n",
		 nw_start+nw_len,
		 (datastringlen - (nw_start + nw_len)),
		 in_subscript);

      //      get the next word
      crm_nextword (datastring, datastringlen, nw_start+nw_len, 
		    &nw_start, &nw_len);

      if (internal_trace)
	fprintf (stderr, "box-parsing left returned start: %ld, len: %ld .\n",
		 nw_start, nw_len);
      
      //     Are we done?
      if (nw_len <= 0) 
	{
	  if (user_trace)
	    fprintf (stderr, "Nothing more to do in the var-restrict.\n");
	  break;
	}

      //      we need to shred the word (put a NULL at the end so we can 
      //      use sscanf on it )
      memcpy (scanbuf, &datastring[nw_start], nw_len);
      scanbuflen = nw_len;
      scanbuf[scanbuflen] = '\0';
            
      if (internal_trace)
	fprintf (stderr, "  var restrict clause was '%s' len %ld \n", 
		 scanbuf, scanbuflen);

      //      Is it int-able?
      i = sscanf ( scanbuf, "%ld", &j);
      if (i > 0) 
	{
	  //   Check for a negative value of j; negative j would allow
	  //   out-of-bounds accessing.
	  if (j < 0)
	    {  
	      j = 0;
	      fprintf (stderr, "Var-restriction has negative start or length."
		     "  Sorry, but negative start/lengths are not "
		       "allowed, as it's a possible security exploit.");
	    }
	  //      Do the offset/length alternation thing.
	  if (in_subscript == 0)
	    {
	      if (actual_len <= j) 
		{
		  if (user_trace) fprintf (stderr, "Clipping start to %ld",
					   actual_len);
		  j = actual_len;
		}
	      if (user_trace)
		fprintf (stderr, "Variable starting offset: %ld\n", j);
	      actual_offset = actual_offset + j;
	      actual_len = actual_len - j;
	      in_subscript = 1;
	    }
	  else
	    {
	      if (actual_len < j) 
		{
		  if (user_trace)
		    fprintf (stderr, "Clipping length to %ld\n", 
			     actual_len);
		  j = actual_len;
		}
	      if (user_trace)
		fprintf (stderr, "Variable starting offset: %ld\n", j);
	      actual_len = j;
	      in_subscript = 0;
	    }
	}
      else
	//      it's not an int; see if it's a /regex/
	{
	  long regex_start;
	  in_subscript = 0;   //  no longer in subscript-length mode.
	  if (datastring[nw_start] == '/')
	    //      yes, it's a start of regex.  copy it into the scan buf,
	    //      while looking for the closing '/', and keeping 
	    //      any \/ as / (everything eles is verbatim).
	    {
	      regex_start = nw_start + 1;  // regex starts +1 past start of str
	      nw_len = 0;                     // nw_len is next open char idx.
	      while ( (regex_start < datastringlen
		       && datastring [regex_start] != '/' )
		      || ( regex_start < datastringlen
			   && datastring [regex_start] == '/'
			   && datastring [regex_start-1] == '\\'))
		{
		  //   overwrite escaped slashes?
		  if (datastring[regex_start] == '/')
		    nw_len--;       
		  scanbuf[nw_len] = datastring[regex_start];
		  nw_len++;
		  regex_start++;
		}
	      scanbuf[nw_len] = '\0';
	      if (user_trace)
		fprintf (stderr, "Var restriction with regex '%s' len %ld\n", 
			 scanbuf, nw_len);

	      //
	      //    Compile up that regex
	      j = crm_regcomp (&preg, scanbuf, nw_len, REG_EXTENDED);
	      if (j > 0) 
		{
		  long curstmt;
		  curstmt = csl->cstmt;
		  crm_regerror ( i, &preg, tempbuf, data_window_size);
		  strcpy (errstr, 
			  "Regular Expression Compilation Problem on:");
		  strncat (errstr, tempbuf, MAX_PATTERN - 128);
		  return (-2);
		}
	      
	      if (internal_trace)
		fprintf (stderr, " Running regexec, start at %ld\n",
			 actual_offset);
	      //    Time to run the match
	      start_ptr= &(mdw[actual_offset]);
	      j = crm_regexec ( &preg, start_ptr, actual_len,
				MAX_SUBREGEX, matches, 0, NULL);
	      crm_regfree (&preg);
	      if (j == 0)
		{
		  //    Yes, the regex matched.  Find the innermost 
		  //    participating submatch, and use that.
		  long i;
		  i = 0;
		  while (matches[i].rm_so >= 0)
		    i++;
		  i--;
		  //     Now use the stuff in matches[i] as 
		  //    data to seet the new limits to our var
		  actual_offset = actual_offset + matches[i].rm_so;
		  actual_len = matches[i].rm_eo - matches[i].rm_so;
		  if (user_trace)
		    fprintf (stderr, " Var restrict regex matched, "
			     "new start offset %ld, new length %ld\n",
			     (long) matches[i].rm_so, (long) matches[i].rm_eo);
		}
	      else
		{
		  //    The regex didn't match.  We're done.  Length
		  //    is now zero.
		  actual_len = 0;
		  if (user_trace)
		    fprintf (stderr, "Var restrict regex didn't match, "
			     "string is now zero length.\n");
		  goto all_done;
		}
	    }
	}
    }
 all_done:
  /////////////////////////////
  //     All calculations done.  Push actual start and actual length
  //     back onto the output vars

  if ( outblock )
    *outblock = vht[vmidx]->valtxt;
  if ( outoffset)
    *outoffset = actual_offset;
  if ( outlen )
    *outlen = actual_len;
  if (vht_idx)
    *vht_idx = vmidx;
  //
  if (internal_trace)
    {
      fprintf (stderr, "Final non-nulls: ");
      if (vht_idx)
	fprintf (stderr, " VHTidx %ld", (unsigned long) *vht_idx);
      if (outblock)
	fprintf (stderr, " blockaddr %p", *outblock);  /* [i_a] */
      if (outoffset)
	fprintf (stderr, " startoffset %ld", (unsigned long) *outoffset);
      if (outlen)
	fprintf (stderr, " length %ld", (unsigned long) *outlen);
      fprintf (stderr, "\n");
    }
  return (0);
}

    
    


