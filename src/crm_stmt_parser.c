//	crm_preprocessor.c  - statement preprocessor utilities

// Copyright 2001-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

//
//       the actual textual representations of the flags, with their values
//     DON'T FORGET TO ALSO MODIFY THIS IN crm114_structs.h !!

FLAG_DEF crm_flags[46] =
  {
    {"fromstart", CRM_FROMSTART},
    {"fromnext", CRM_FROMNEXT},
    {"fromend", CRM_FROMEND},
    {"newend", CRM_NEWEND},
    {"fromcurrent", CRM_FROMCURRENT},
    {"nocase", CRM_NOCASE},
    {"absent", CRM_ABSENT},
    {"basic", CRM_BASIC},
    {"backwards", CRM_BACKWARDS},
    {"literal", CRM_LITERAL},
    {"nomultiline", CRM_BYLINE},
    {"byline", CRM_BYLINE},
    {"bychar", CRM_BYCHAR},
    {"string", CRM_BYCHAR},
    {"bychunk", CRM_BYCHUNK},
    {"byeof", CRM_BYEOF},
    {"eofaccepts", CRM_EOFACCEPTS},
    {"eofretry", CRM_EOFRETRY},
    {"append", CRM_APPEND},
    {"keep", CRM_KEEP},
    {"async", CRM_ASYNC},
    {"refute", CRM_REFUTE},
    {"microgroom", CRM_MICROGROOM},
    {"markovian", CRM_MARKOVIAN},
    {"markov", CRM_MARKOVIAN},
    {"osb", CRM_OSB_BAYES},
    {"correlate", CRM_CORRELATE},
    {"winnow", CRM_OSB_WINNOW},
    {"unique", CRM_UNIQUE},
    {"chi2", CRM_CHI2},
    {"entropy", CRM_ENTROPY},
    {"entropic", CRM_ENTROPY},
    {"osbf", CRM_OSBF },
    {"hyperspace", CRM_HYPERSPACE},
    {"unigram", CRM_UNIGRAM},
    {"crosslink", CRM_CROSSLINK},
    {"default", CRM_DEFAULT},
    {"lineedit", CRM_READLINE},
    {"sks", CRM_SKS},
    {"svm", CRM_SVM},
    {"fscm", CRM_FSCM},
    {"neural", CRM_NEURAL_NET},
    {"erase", CRM_ERASE},
    {"pca", CRM_PCA},
    {"", 0},
    {"", 0}
  };

#define CRM_MAXFLAGS 43




//    The magic flag parser.  Given a string of input, return the
//    codes that were found as the (long int) return value.  If an
//    unrecognized code is found, squalk an error (whether it is fatal
//    or not is another issue)
//
//    Note that since flags (like variables) are always ASCII, we don't
//    need to worry about 8-bit-safety.
//
unsigned long long crm_flagparse (char *input, long inlen)  //  the user input
{
  char flagtext [MAX_PATTERN];
  char *remtext;
  long remlen;
  char *wtext;
  long flagsearch_start_here;
  long wstart;
  long wlen;
  unsigned long long outcode;

  int done;
  int i;
  int j;
  int k;
  int recog_flag;

  outcode = 0;

  memmove (flagtext, input, inlen);
  flagtext[inlen] = '\000';

  if (internal_trace)
    fprintf (stderr, "Flag string: %s\n", flagtext);

  //  now loop on thru the nextwords,
  remtext = flagtext;
  done = 0;
  remlen = inlen;
  wstart = 0;
  wlen = 0;
  flagsearch_start_here = 0;
  while (!done && remlen > 0)
    {
      i=crm_nextword (remtext, remlen, flagsearch_start_here, &wstart, &wlen);
      flagsearch_start_here = wstart + wlen + 1;
      if (wlen > 0)
	{
	  //    We got a word, so aim wtext at it
	  wtext = &(remtext[wstart]);
	  if (internal_trace)
	    {
	      fprintf (stderr, "found flag, len %ld: ", wlen) ;
	      for (j = 0; j < wlen; j++) fprintf (stderr, "%c", wtext[j]);
	      fprintf (stderr, "\n");
	    };

	  //    find sch in our table, squalk a nonfatal/fatal if necessary.
	  recog_flag = 0;
	  for (j = 0; j <= CRM_MAXFLAGS; j++)
	    {
	      // fprintf (stderr, " Trying %s (%ld) \n", crm_flags[j].string, crm_flags[j].value );
	      k = strlen (crm_flags[j].string);
	      if (k == wlen
		  && 0 == strncasecmp (wtext, crm_flags[j].string, k))
		{
		  //    mark this flag as valid so we don't squalk an error
		  recog_flag = 1;
		  //     and OR this into our outcode
		  outcode = outcode | crm_flags[j].value;
		  if (user_trace)
		    {
		      fprintf (stderr, "Mode #%d, '%s' turned on. \n",
			       j,
			       crm_flags[j].string);
		    };
		};
	    };

	  //   check to see if we need to squalk an error condition
	  if (recog_flag == 0)
	    {
	      long q;
	      char foo[1024];
	      strncpy (foo, wtext, 128);
	      foo[wlen] = '\000';
	      q = nonfatalerror5 ("Darn...  unrecognized flag :",
				  foo, CRM_ENGINE_HERE);
	    };


	  //  and finally,  move sch up to point at the remaining string
	  if (remlen <= 0) done = 1;
	}
      else
	done = 1;
    };

  if (internal_trace )
    fprintf (stderr, "Flag code is : %llx\n", outcode);

  return (outcode);
}

//     Get the next word in a string.  "word" is defined by the
//     continuous span of characters that are above ascii ! (> hex 0x20
//
//     The search starts at the "start" position given; the start position
//     is updated on each call and so is mutilated.  To step through a
//     arglist, you must add the returned value of "len" to the returned
//     value of start!
//
//     The returned value is 0/1 as to whether we found
//     a valid word, and *start and *length, which give it's position.
//
long crm_nextword ( char *input,
		    long inlen,
		    long starthere,
		    long *start,
		    long *len)
{
  *start = starthere;
  *len = 0;
  //   find start of string (if it exists)
  while (*start < inlen && input [*start] <= 0x20 ) *start = *start + 1;

  //  check - did we hit the end and still be invalid?  If so, return 0
  if (*start == inlen) return (0);

  //    if we get to here, then we have a valid string.
  *len = 0;
  while ((*start+*len) < inlen
	 && input [*start+*len] > 0x20 ) *len = *len + 1;

  return ( (*len) > 0);
}



//
//    experimental code for a statement-type-sensitive parser.
//   Not in use yet... but someday... goal is to provide better error
//   detection.

int crm_profiled_statement_parse ( char *in,
				   long slen,
				   ARGPARSE_BLOCK *apb,
				   long amin, long amax,
				   long pmin, long pmax,
				   long bmin, long bmax,
				   long smin, long smax)
{
  return (0);
}

//      parse a CRM114 statement; this is mostly a setup routine for
//     the generic parser.

int crm_statement_parse ( char *in,
			  long slen,
			  ARGPARSE_BLOCK *apb)
{
#define CRM_STATEMENT_PARSE_MAXARG 10
  int i,  k;

  long ftype[CRM_STATEMENT_PARSE_MAXARG];
  long fstart[CRM_STATEMENT_PARSE_MAXARG];
  long flen [CRM_STATEMENT_PARSE_MAXARG];

  //     we call the generic parser with the right args to slice and
  //     dice the incoming statement into declension-delimited parts
  k = crm_generic_parse_line ( in,
			       slen,
			       "<([/",
			       ">)]/",
			       "\\\\\\\\",      // this is four backslashes
			       CRM_STATEMENT_PARSE_MAXARG,
			       ftype,
			       fstart,
			       flen);

  //   now we have all these nice chunks... we split them up into the
  //   various allowed categories.


  //   start out with empties on each possible chunk
  apb->a1start = NULL; apb->a1len = 0;
  apb->p1start = NULL; apb->p1len = 0;
  apb->p2start = NULL; apb->p2len = 0;
  apb->p3start = NULL; apb->p3len = 0;
  apb->b1start = NULL; apb->b1len = 0;
  apb->s1start = NULL; apb->s1len = 0;
  apb->s2start = NULL; apb->s2len = 0;

  //   Scan through the incoming chunks
  for (i = 0; i < k; i++)
    {
      switch (ftype[i])
       	{
	case CRM_ANGLES:
	  {
	    //  Grab the angles, if we don't have one already
	    if (apb->a1start == NULL)
	      {
		apb->a1start = &in[fstart[i]];
		apb->a1len = flen [i];
	      }
	    else nonfatalerror5
		   ("There are multiple flag sets on this line.",
		    " ignoring all but the first", CRM_ENGINE_HERE);
	  }
	  break;
	case CRM_PARENS:
	  {
	    //  grab a set of parens, cascading till we find an one
	    if (apb->p1start == NULL)
	      {
		apb->p1start = &in[fstart[i]];
		apb->p1len = flen [i];
	      }
	    else
	      if (apb->p2start == NULL)
		{
		  apb->p2start = &in[fstart[i]];
		  apb->p2len = flen [i];
		}
	      else
		if (apb->p3start == NULL)
		  {
		    apb->p3start = &in[fstart[i]];
		    apb->p3len = flen [i];
		  }
		else
		  nonfatalerror5
		    ("Too many parenthesized varlists.",
		     "ignoring the excess varlists.", CRM_ENGINE_HERE);
	  }
	  break;
	case CRM_BOXES:
	  {
	    //  Grab the angles, if we don't have one already
	    if (apb->b1start == NULL)
	      {
		apb->b1start = &in[fstart[i]];
		apb->b1len = flen [i];
	      }
	    else nonfatalerror5
		   ("There are multiple domain limits on this line.",
		    " ignoring all but the first", CRM_ENGINE_HERE);
	  }
	  break;
	case CRM_SLASHES:
	  {
	    //  grab a set of parens, cascading till we find an one
	    if (apb->s1start == NULL)
	      {
		apb->s1start = &in[fstart[i]];
		apb->s1len = flen [i];
	      }
	    else
	      if (apb->s2start == NULL)
		{
		  apb->s2start = &in[fstart[i]];
		  apb->s2len = flen [i];
		}
	      else
		nonfatalerror5 (
		       "There are too many regex sets in this statement,",
		       " ignoring all but the first.", CRM_ENGINE_HERE);
	  }
	  break;
	default:
	  fatalerror5( "Declensional parser returned an undefined typecode!",
		       "What the HECK did you do to cause this?",
		       CRM_ENGINE_HERE);
	};
    }
  return (k);    // return value is how many declensional arguments we found.
};


//     The new and improved line core parser routine.  Instead of
//     being totally ad hoc, this new parser actually retains context
//     durng the parse.
//
//     this hopefully will keep the parser from getting confused by [] in
//     the slash matching and other such abominations.
//
//     (one way to view this style of parsing is that each arg in a
//     CRM114 statement is "declined" by it's delimiters to determine
//     what role this variable is to play in the statement.  Kinda like
//     Latin - to a major extent, you can mix the parts around and it
//     won't make any difference.

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
		    )
{
  //    the general algorithm here is to move along the input line,
  //    looking for one of the characters in schars.  When we find it,
  //    we lock onto that and commit to finding an arg of that type.
  //    We then start scanning ahead keeping count of schars minus echars.
  //    when the count hits zero, it's end for that arg and we move onward
  //    to the next arg, with the same procedure.
  //
  //    note that when we are scanning for a new arg, we are open to args
  //    of any type (as defined by the members of schars, while in an arg
  //    we are looking only for the unescaped outstanding echar and are blind
  //    to everything else.
  //
  //    when not in an arg, we do not have any escape character active.
  //
  //     We return the number of args found

  long chidx;
  char curchar;
  long argc;
  long i;
  long itype;
  long depth;

  //    zeroize the outputs to start...
  for (i = 0; i < maxargs; i++)
    {
      ftype[i] = -1;
      fstart[i] = 0;
      flen[i] = 0;
    };


  //    scan forward, looking for any member of schars

  depth = 0;
  chidx = -1;
  argc = 0;
  itype = -1;

  if (internal_trace)
    {
      fprintf (stderr, " declensional parsing for %ld chars on: ", len);
      for (i = 0; i < len; i++)
	fprintf (stderr, "%c", txt[i]);
      fprintf (stderr, "\n");
    }

  while (chidx < len  &&  argc <= maxargs)
    {
      chidx++;
      curchar = txt[chidx];
      if (itype == -1)     // are we looking for an argstart char?
	{
	  //    is curchar one of the start chars?  (this is 8-bit-safe,
	  //     because schars is always normal ASCII)
	  for (i = 0; i < strlen (schars); i++)
	    if (curchar == schars[i])
	      {
		if (internal_trace)
		  fprintf (stderr, "   found opener %c at %ld,",curchar,chidx);
		itype = i;
		fstart[argc] = chidx + 1;
		ftype [argc] = itype;
		depth = 1;
	      };
	  //  if it wasn't a start-character for an arg, we are done.
	}
      else    // nope, we're in an arg, so we check for unescaped schar
	     // and fchar characers
	{
	  //  if (curchar == fchars [itype] && txt[chidx-1] != echars[itype])
          if (curchar == fchars [itype]
	      && (txt[chidx-1] != echars[itype]
		  || txt[chidx-1] == txt[chidx-2]))
	    {
	      depth--;
	      if (depth == 0)
		{
		  //   we've found the end of the text arg.  Close it off and
		  //   note it into the output vectors
		  flen [argc] = chidx - fstart[argc] ;
		  if (internal_trace)
		    {
		      int q;
		      fprintf (stderr, " close %c at %ld --", curchar, chidx);
		      for (q = fstart[argc]; q < fstart[argc]+flen[argc]; q++)
			fprintf (stderr, "%c", txt[q]);
		      fprintf (stderr, "-- len %ld\n", flen[argc]);
		    };
		  itype = -1;
		  argc++;
		};
	    }
	  else
	    //if (curchar == schars [itype] && txt[chidx-1] != echars[itype])
	    if (curchar == schars [itype]
		&& (txt[chidx-1] != echars[itype]
		    || txt[chidx-1] == txt[chidx-2]))
	      {
		depth++;
	      };
	};
      //    if we weren't a schar or an unexcaped echar, we're done!
    };
  if (depth != 0)
    {
      char errstmt[MAX_PATTERN];
      flen[argc] = chidx - fstart[argc];
      //
      //   GROT GROT GROT Somehow, sometimes we get flen[argc] < 0.   It's
      //   always with buggy userprograms, but we shouldn't need this anyway.
      //   So, until we find out what _we_ are doing wrong, leave the check
      //   for flen[argc] < 0 in here.
      //
      if (flen[argc] < 0) flen[argc] = 0;
      strncpy ( errstmt, &txt[fstart[argc]],
		flen[argc] );
      nonfatalerror5 (" This operand doesn't seem to end.  Bug?  \n -->  ",
		      errstmt, CRM_ENGINE_HERE);
      argc++;
    };
  return (argc);
}

//    and to avoid all the mumbo-jumbo, an easy way to get a copy of
//    an arg found by the declensional parser.
void crm_get_pgm_arg (char *to, long tolen, char *from, long fromlen)
{
  long len;

  if (to == NULL)
    return;

  if (from == NULL)
    {
      to[0] = '\000';
    }
  else
    {
      len = tolen - 1;
      if (len > fromlen ) len = fromlen ;
      memmove (to, from, len);
      to[len] = '\000';
    }
}
