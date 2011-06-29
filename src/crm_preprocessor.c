//	crm_preprocessor.c  - crm file preprocesors

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
extern char *tempbuf;

//      crm preprocessor - pre-process a CRM file to make it
//      palatable to the sorry excuse we have for a compiler.
int crm_preprocessor (CSL_CELL *csl, int flags)
{
  int lflag;
  int status;
  long i, j;
  long done;
  regex_t preg;
  int numinserts = 0;
  int maxinserts = DEFAULT_MAX_INSERTS;
  regmatch_t matches[3];

  //  regex commentary:
  //    we want to match both
  //      "\n[ ]*insert[ ]*[file][ ]*\n"
  //    and
  //      "^[ ]*insert[ ]*[file][ ]*\n"
  //
  //    This is the file insertion regex.  Note that it does NOT allow
  //    spaces in filenames, nor does it deal with embedded #comments
  //    but then again, the "fixes" to deal with spaces in filenames
  //    also don't deal wth embedded #comments, because #comments
  //    themselves aren't dealt with till lower down in the code..
  //
  //    However, there's another problem with the above.  The trailing
  //    newline may not be there - consider:
  //
  //      #insert foo.crm ; output /hello, world!\n/
  //
  //    which fails because we aren't regex_conforming.
  //    So, what we really need is to grab the next nonblank token, then
  //    either get a newline or a semicolon.

  char *insert_regex =
    "\n[[:blank:]]*(insert)[[:blank:]]+([[:graph:]]+)[[:blank:]]*[\n;]";
  //    "\n[[:blank:]]*(insert)[[:blank:]]+([[:graph:]]+)[[:blank:]]*[\n;]";
  //
  //
  if (internal_trace )
    fprintf (stderr, " preprocessor - #insert processing...\n");

  lflag = 0;
  i = 0;
  done = 0;

  //
  //     Compile the insert regex
  //
  i = crm_regcomp (&preg,
		   insert_regex, strlen (insert_regex),
		   REG_EXTENDED | REG_ICASE | REG_NEWLINE);
  if ( i != 0)
    {
      crm_regerror ( i, &preg, tempbuf, data_window_size);
      untrappableerror5 (
	 "Regular Expression Compilation Problem during INSERT processing:",
	 tempbuf, CRM_ENGINE_HERE);
    };

  //
  //    Do the initial breaking pass
  //
  crm_break_statements (0, csl->nchars, csl);

  if (internal_trace)
    fprintf (stderr,
	     "After first pass, breaking statements we have -->>%s<<--\nlength %ld\n",
	     csl->filetext, csl->nchars);


  while (!done)
    {
      long filenamelen;
      j  = crm_regexec ( &preg, csl->filetext, csl->nchars,
			 		3, matches, lflag, NULL);
      if ( j != 0)
	{
	  if (internal_trace)
	    fprintf (stderr, "No insert files left to do.\n");
	  done = 1;
	}
      else
	{
	  char insertfilename [MAX_FILE_NAME_LEN];
	  struct stat statbuf;

	  filenamelen = matches[2].rm_eo - matches[2].rm_so ;

	  for (j = 0;
	       j < filenamelen
		 && j < MAX_FILE_NAME_LEN;
	       j++)
	    insertfilename[j] = csl->filetext[matches[2].rm_so + j];
	  insertfilename[j] = '\000';

	  //   Check to see if this was a "delimited" insertfile name
	  //   that is, wrapped in [filename] rather than plaintext -
	  //   if it is, then do a variable expansion on it.
	  if (insertfilename[0] == '['
	      && insertfilename[filenamelen-1] == ']')
	    {
	      if (user_trace)
		fprintf (stderr, "INSERT filename expand: '%s'",
			 insertfilename);
	      //  Get rid of the enclosing [ and ]
	      filenamelen = filenamelen - 2;
	      for (j = 0; j < filenamelen; j++)
		insertfilename[j] = insertfilename[j + 1];
	      insertfilename[filenamelen] = '\000';
	      filenamelen = crm_nexpandvar (insertfilename,
					      filenamelen,
					      MAX_FILE_NAME_LEN);
	      if (user_trace)
		fprintf (stderr, " to '%s' \n", insertfilename);
	    }
	  insertfilename[filenamelen] = '\000';

	  //   We have a filename; check to see if it will blow the
	  //   gaskets on the filesystem or not:
	  //
	  if (filenamelen > MAX_FILE_NAME_LEN-1)
	    untrappableerror5 ("INSERT Filename was too long!  Here's the"
			       "first part of it: ", insertfilename,
			       CRM_ENGINE_HERE);

	  //   To keep the matcher from looping, we change the string
	  //   'insert' to 'insert=' .  Cool, eh?
	  //
	  csl->filetext[matches[1].rm_eo] = '=';   // smash in an "="

	  //   stat the file - if 0, file exists
	  status = stat ( insertfilename, &statbuf );
	  if (! status )
	    {
	      //
	      //    OK, now we have to "insert" the file, but we have to
	      //    do it gracefully.  In particular, the file itself
	      //    must be loaded, then newline-fixupped, then
	      //    we know it's actual size and can actually -insert-
	      //    it.
	      //
	      //    We malloc a big hunk of memory, read the file in.
	      //    We expand it there (with impunity), then
	      //    we make a temporary copy in malloced memory,
	      //    and do the real insertion.
	      CSL_CELL *ecsl;
	      char *insert_buf;
	      if (user_trace)
		{
		  fprintf (stderr, "Inserting file '%s' .\n", insertfilename);
		};

	      ecsl = (CSL_CELL *) malloc (sizeof (CSL_CELL));
	      insert_buf = malloc (sizeof (char) * max_pgmsize);
	      if (!insert_buf || !ecsl)
		untrappableerror5 ("Couldn't malloc enough memory to do"
				   " the insert of file ", insertfilename,
				   CRM_ENGINE_HERE);

	      //   Loop prevention check - too many inserts?
	      //
	      numinserts++;
	      if (numinserts > maxinserts)
		untrappableerror5 ("Too many inserts!  Limit exceeded with"
				   "filename : ", insertfilename,
				   CRM_ENGINE_HERE);

	      ecsl->filetext = insert_buf;
	      ecsl->nchars = 0;
	      //   OK, we now have a buffer.  Read the file in...
	      {
		int fd;
		fd = open (insertfilename, O_RDONLY);
		dontcare = read (fd,
		      ecsl->filetext,
		      statbuf.st_size);
		close (fd);
		//
		//   file's read in, put in a trailing newline
		ecsl->nchars = statbuf.st_size;
		ecsl->filetext[ecsl->nchars] = '\n';
		ecsl->nchars ++;
		ecsl->filename = insertfilename;
		//
		//   now do the statement-break thing on this file
		crm_break_statements (0, ecsl->nchars, ecsl);
		//
		//   and we have the expanded text ready to insert.
		//
		//   will it fit?
		//
		if ( (csl->nchars + ecsl->nchars + 64)
		     > (sizeof (char) * max_pgmsize))
		  untrappableerror5 ( " Program file buffer overflow when "
				      " INSERTing file ", insertfilename,
				      CRM_ENGINE_HERE);

		//   Does the result end with a newline?  If not, fix it.
		if (ecsl->filetext[ecsl->nchars-1] != '\n')
		  {
		    ecsl->filetext [ecsl->nchars ] = '\n';
		    ecsl->nchars++;
		  };

		//   Does the result end with two newlines?  Fix
		//   that, too.
		//if (ecsl->filetext[ecsl->nchars-1] == '\n'
		//    && ecsl->filetext[ecsl->nchars-2] == '\n')
		//  {
		//    ecsl->nchars--;
		//  };

		//  Make a hole in the csl->filetext
		//
		//   (note- Fidelis' points out that we need to pace
		//    off from the end of matches[0] so as to not smash
		//    trailing stuff on the line.
		//
		memmove (&(csl->filetext[matches[0].rm_eo + ecsl->nchars]),
			 &(csl->filetext[matches[0].rm_eo]),
			 csl->nchars - matches[0].rm_eo + 1); // +1 for '\0'!
		//
		//   and put the new text into that hole
		//
		memmove (&(csl->filetext[matches[0].rm_eo]),
			 ecsl->filetext,
			 ecsl->nchars);

		//   Mark the new length of the csl text.
		if (internal_trace)
		  fprintf (stderr, "Old length: %ld, ", csl->nchars);
		csl->nchars += ecsl->nchars;
		if (internal_trace)
		  fprintf (stderr, "new length: %ld\n ", csl->nchars);

		//    Now we clean up (de-malloc all that memory)
		free (ecsl->filetext);
		free (ecsl);
	      }
	    }
	  else
	    {
	      //
	      //   Paolo's beautiful weasel hack to make missing
	      //   insert files a trappable error.  The hitch is that
	      //   the error "occurs" at preprocessor time, before the
	      //   compiler has a chance to set up the trap addresses.
	      //   So, Paolo presented the following waycool weasel hack.
	      //   "If the file is missing, 'insert' a FAULT that has the
	      //   fault message of "Missing insert file".  So, if we never
	      //   actually execute the missing lines, there's no problem,
	      //   and if we _do_, we can trap the error or not, as the
	      //   the programmer chooses.
	      //
	      //  untrappableerror
	      //    (" I'm having a problem inserting file ",
	      //		insertfilename);
	      //
	      char faulttext[MAX_VARNAME];
	      long textlen;

	      if (user_trace)
		{
		  fprintf (stderr, "Can't find '%s' to insert.\n"
			   "Inserting a FAULT instead\n",
                                  insertfilename);
		};

	      //
	      //    Build the fault string.
	      sprintf (faulttext,
		       "\n######  THE NEXT LINE WAS AUTO_INSERTED BECAUSE THE FILE COULDN'T BE FOUND \nfault /Couldn't insert the file named '%s' that you asked for.  This is probably a bad thing./\n",
		       insertfilename);
	      textlen = strlen (faulttext)  ; //  -1 gets rid of the \0
	      //
	      //       make a hole to put the fault string into.
	      //
	      memmove (&(csl->filetext[matches[0].rm_eo + textlen]),
		       &(csl->filetext[matches[0].rm_eo]),
		       csl->nchars - matches[0].rm_eo );
	      //
	      //   and put the new text into that hole
	      //
	      memmove (&(csl->filetext[matches[0].rm_eo]),
		       faulttext,
		       textlen);
	      //   Mark the new length of the csl text.
	      if (internal_trace)
		fprintf (stderr, "Added %ld chars to crmprogram\n",
			 textlen );
	      csl->nchars += textlen;
	    };
	  i = matches[1].rm_so + 1;
	};
      if (internal_trace)
	fprintf (stderr,
		 "----------Result after preprocessing-----\n"
		 "%s"
		 "\n-------------end preprocessing------\n",
		 csl->filetext);
    };

  //     define a hash of the expanded program for sanity checking on bugreps:
  //
  {
    char myhash[32];
    sprintf (myhash, "%08X", strnhash (csl->filetext, csl->nchars));
    myhash[8] = '\0';
    crm_set_temp_var (":_pgm_hash:", myhash);
  };

  ///   GROT GROT GROT  for some reason, Gnu Regex segfaults if it
  //    tries to free this register.
  //  crm_regfree (&preg);
  //fprintf (stderr, "returning\n");
  return (0);
};

//
//     Set up statement breaks.
//
//     If we're not in a nesting (paren, angle, box, slash) then
//     we need to assure that there are newlines before and after
//     any { and }, and that there is a newline after every ; and
//     before every #.
//
//     If we ARE in a nesting, then all characters pass unchanged.
//
//     Note that this is an "in-place" mutilation, not a copying mutilation.
//

void crm_break_statements (long ini, long nchars, CSL_CELL *csl)
{
  int seennewline;
  int in_comment;
  int neednewline;
  int paren_nest, angle_nest, box_nest, slash_nest;
  long i;
  seennewline = 1;
  neednewline = 0;
  in_comment = 0;
  paren_nest = slash_nest = angle_nest = box_nest = 0;

  if ( internal_trace )
    fprintf (stderr, "  preprocessor - breaking statmeents... \n");
  for (i = ini; i < ini + nchars; i++)
    {
      //    now, no matter what, we're looking at a non-quoted character.
      //
      //   are we looking at a nonprinting character?
      if (csl->filetext[i] < 0x021 )
	{
	  if (csl->filetext[i] == '\n')
	    {
	      //    get rid of extraneous newlines.
	      //if (internal_trace)
	      //  fprintf (stderr, "  newline .");
	      seennewline = 1;
	      neednewline = 0;
	      in_comment = 0;
	      //    Userbug containment - a newline closes all nests
	      paren_nest = slash_nest = angle_nest = box_nest = 0;
	    };
	  //   other nonprinting characters do not change things.
	}
      else
	{
	  //   we don't do any processing inside a comment!
	  if ( in_comment )
	    {
	      //   inside a comment, we don't do squat to printing chars.
	      //    unless it's an escaped hash; in that case
	      //     it's end-of-comment
	      if (csl->filetext[i] == '#'
		  && (i - 1) >= 0
		  && csl->filetext[i-1] == '\\')
		{
		  neednewline = 1;
		  seennewline = 0;
		  in_comment = 0;
		};
	    }
	  else
	    {
	      //    we are looking at a printing character, so maybe we have
	      //    to add a newline.  Or maybe not...
	      if (neednewline)
		{
		  if ((csl->nchars+1) > (sizeof(char) * max_pgmsize))
		    untrappableerror5 ( "Program file buffer overflow - "
				       "post-inserting newline to: ",
					&(csl->filetext[i]),
					CRM_ENGINE_HERE);
		  //    we need a newline and are looking at a printingchar
		  //    so we need to insert a newline.
		  memmove ( &(csl->filetext[i+1]),
			    &(csl->filetext[i]),
			    strlen (&csl->filetext[i])+1);
		  csl->filetext[i] = '\n';
		  i++;
		  csl->nchars++;
		  nchars++;
		  neednewline = 0;
		  seennewline = 1;
		};
	      //
	      switch (csl->filetext[i])
		{
		case '\\':
		  {
		    //     if it's a backslash at the end of a line,
		    //     delete +both+ the backslash and newline, making
		    //     one big line out of it.
		    //
		    //     We do this whether or not we're in a nesting.
		    if (  csl->filetext[i+1] == '\n' )
		      {
			if (internal_trace)
			  fprintf (stderr, " backquoted EOL - splicing.\n");
			memmove ( &(csl->filetext[i]),
				  &(csl->filetext[i+2]),
				  strlen (&csl->filetext[i+2])+1);
			csl->nchars--;
			csl->nchars--;
			nchars--;
			nchars--;
			i--;
		      }
		    else
		      {
			//   Otherwise, we _always_ step over the next
			//   character- it can't change nesting, it can't
			//   close a string.  Thus, the preprocessor will
			//   do nothing to it.
			//
			//
			//    TRICKY BIT HERE !!!  Notice that we do
			//   this '\' step-over test _BEFORE_ we do
			//   any other character testing, so the '\'
			//   gets to do it's escape magic before
			//   anything else can operate - and it
			//   _preempts_ any other character's
			//   actions.
			//
			i++;
		      };
		  };
		  break;

		case '{':
		case '}':
		  {
		    //    put an unquoted '{' or '}' onto it's own line.
		    //   do we need to put in a prefix new line?
		    //		  if (internal_trace)
		    //
		    //   Are we inside a nesting?
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			if ( !seennewline )
			  {
			    if ((csl->nchars+1) > sizeof(char)*max_pgmsize)
			      untrappableerror5 ("Program buffer overflow when"
						 "post-inserting newline on:",
						 &csl->filetext[i],
						 CRM_ENGINE_HERE);
			    if (internal_trace)
			      fprintf (stderr, " preinserting a newline.\n");
			    memmove ( &(csl->filetext[i+1]),
				      &(csl->filetext[i]),
				      strlen (&csl->filetext[i])+1);
			    csl->filetext[i] = '\n';
			    csl->nchars++;
			    nchars++;
			    i++;
			  };
			seennewline = 0;
			//   and mark that we need a newline before any more
			//   printable characters come through.
			neednewline = 1;
		      }
		  };
		  break;
		case ';':
		  {
		    //       we can replace non-escaped semicolons with
		    //       newlines.
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			if ( seennewline ) //  we just saw a newline
			  {
			    //  was preceded by a newline so just get rid
			    //  of the ;
			    if (internal_trace)
			      fprintf (stderr,
				       "superfluous semicolon, *poof*.\n");
			    memmove ( &(csl->filetext[i]),
				      &(csl->filetext[i+1]),
				      strlen (&csl->filetext[i])+1);
			    csl->nchars--;
			    nchars--;
			    i--;
			    neednewline = 0;
			    seennewline = 1;
			  }
			else
			  {
			    //   this was not preceded by a newline,
			    //  so we just replace the semicolon with a
			    //  newline before any printed characters
			    if (internal_trace)
			      fprintf (stderr, " statement break semi.\n"
				       "--> \\n \n");
			    csl->filetext[i] = '\n';
			    neednewline = 0;
			    seennewline = 1;
			  };
		      };
		  };
		  break;
		case '#':
		  {
		    //      now, we're in a comment - everything should be
		    //      done only with the comment thing enabled.
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			in_comment = 1;
		      };
		  };
		  break;
		case '(':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			paren_nest = 1;
		      };
		  };
		  break;
		case ')':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 1 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			paren_nest = 0;
		      };
		  };
		  break;
		case '<':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			angle_nest = 1;
		      };
		  };
		  break;
		case '>':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 0 &&
			angle_nest == 1 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			angle_nest = 0;
		      };
		  };
		  break;
		case '[':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 &&
			slash_nest == 0)
		      {
			box_nest = 1;
		      };
		  };
		  break;
		case ']':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 1 &&
			slash_nest == 0)
		      {
			box_nest = 0;
		      };
		  };
		  break;
		case '/':
		  {
		    //      Update nesting if necessary
		    if (paren_nest == 0 &&
			angle_nest == 0 &&
			box_nest == 0 )
		      {
			if (slash_nest == 0)
			  {
			    slash_nest = 1;
			  }
			else
			  {
			    slash_nest = 0;
			  };
		      };
		  };
		  break;
		default:
		  {
		    //      none of the above - it's a normal printing
		    //  character - we can just do the
		    //  clearing of all the "seen/need" flags
		    seennewline = 0;
		    neednewline = 0;
		  };
		  break;
		};
	    };
	};
    };
};
