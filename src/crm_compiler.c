//  crm114_compiler.c  - Controllable Regex Mutilator,  version v1.0
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
//    (but not the stmt table )
#define BASE_COMPILER_TABLE_HERE
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"


//     Here's the real statement description table.
//
STMT_TABLE_TYPE stmt_table[39] =
  {
    /* *INDENT-OFF* */

    //  text      internal    nlen exec?   min max  min max  min max  flags
    //   rep        code                  slashargs  parens  boxes
    //
    {"\n",          CRM_NOOP,   0,  0,      0,   0,  0,  0,  0,  0,  0},
    { "#",          CRM_NOOP,   1,  0,      0,   0,  0,  0,  0,  0,  0},
    { "insert=",    CRM_NOOP,   7,  0,      0,   0,  0,  0,  0,  0,  0},
    { "noop",       CRM_NOOP,   0,  0,      0,   0,  0,  0,  0,  0,  0},
    { "exit",       CRM_EXIT,   0,  1,      0,   0,  0,  1,  0,  0,  0},
    { "{",   CRM_OPENBRACKET,   0,  0,      0,   0,  0,  0,  0,  0,  0},
    { "}",  CRM_CLOSEBRACKET,   0,  0,      0,   0,  0,  0,  0,  0,  0},
    { "goto",       CRM_GOTO,   0,  0,      1,   1,  0,  0,  0,  0,  0},
    { "match",     CRM_MATCH,   0,  1,      1,   1,  0,  1,  0,  1,
       CRM_ABSENT | CRM_NOCASE | CRM_LITERAL | CRM_FROMSTART
      | CRM_FROMCURRENT | CRM_FROMNEXT | CRM_FROMEND | CRM_NEWEND
      | CRM_BACKWARDS | CRM_NOMULTILINE                          },
    { "fail",       CRM_FAIL,   0,  1,      0,   0,  0,  0,  0,  0,  0},
    { "liaf",       CRM_LIAF,   0,  1,      0,   0,  0,  0,  0,  0,  0},
    { "accept",   CRM_ACCEPT,   0,  1,      0,   0,  0,  0,  0,  0,  0},
    { "trap",       CRM_TRAP,   0,  1,      1,   1,  0,  1,  0,  0,  0},
    { "fault",     CRM_FAULT,   0,  1,      0,   1,  0,  0,  0,  0,  0},
    { "output",   CRM_OUTPUT,   0,  1,      0,   1,  0,  0,  0,  1,
      CRM_APPEND                                                 },
    { "window",   CRM_WINDOW,   0,  1,       0,  2,  0,  2,  0,  0,
      CRM_NOCASE | CRM_BYCHAR | CRM_BYEOF | CRM_EOFACCEPTS
      | CRM_EOFRETRY                                             },
    { "alter",     CRM_ALTER,   0,  1,       1,  1,  1,  1,  0,  0,  0},
    { "learn",     CRM_LEARN,   0,  1,       1,  1,  1,  1,  0,  1,
      CRM_NOCASE | CRM_REFUTE | CRM_MICROGROOM                   },
    { "classify",CRM_CLASSIFY,  0,  1,       1,  1,  1,  2,  0,  1,
      CRM_NOCASE                                                 },
    { "isolate", CRM_ISOLATE,   0,  1,       0,  1,  1,  1,  0,  0,  0},
    { "input",     CRM_INPUT,   0,  1,       0,  0,  1,  1,  0,  1,
      CRM_BYLINE                                                 },
    { "syscall", CRM_SYSCALL,   0,  1,       1,  1,  0,  3,  0,  0,
      CRM_KEEP | CRM_ASYNC                                       },
    { "hash",       CRM_HASH,   0,  1,       1,  1,  1,  1,  0,  0,  0},
    { "translate",CRM_TRANSLATE,0,  1,       0,  2,  0,  1,  0,  1,
      CRM_UNIQUE | CRM_LITERAL                               },
    { "intersect",CRM_INTERSECT,0,  1,       0,  0,  1,  1,  1,  1,  0},
    { "union",    CRM_UNION,    0,  1,       0,  0,  1,  1,  1,  1,  0},
    { "eval",     CRM_EVAL,     0,  1,       1,  1,  1,  1,  0,  0,  0},
    { "alius",    CRM_ALIUS,    0,  1,       0,  0,  0,  0,  0,  0,  0},
    { "call",     CRM_CALL,     0,  1,       0,  0,  0,  0,  0,  0,  0},
    { "routine",  CRM_ROUTINE,  0,  1,       0,  0,  0,  0,  0,  0,  0},
    { "return",   CRM_RETURN,   0,  1,       0,  1,  0,  0,  0,  0,  0},
    { "debug",    CRM_DEBUG ,   0,  0,       0,  0,  0,  0,  0,  0,  0},
    { "clump",    CRM_CLUMP,    0,  1,       0,  1,  1,  1,  0,  1,  0},
    { "pmulc",    CRM_PMULC,    0,  1,       0,  1,  0,  0,  0,  1,  0},
    { "NoMoreStmts",CRM_UNIMPLEMENTED,0,0,   0,  0,  0,  0,  0,  0,  0}
    /* *INDENT-ON* */
  };
//   Get a file into a memory buffer.  We can either prep to execute
//    it, or use it as read-only data, or as read-write data.
//
int crm_load_csl (CSL_CELL *csl)
{

  struct stat statbuf;          //  status buffer - for statting files
  int i;

  //   open it first
  csl->filedes = -1;
  if (csl->rdwr)
    {
      csl->filedes = open (csl->filename, O_RDWR);
    }
  else
    {
      csl->filedes = open (csl->filename, O_RDONLY);
    }

  if (csl->filedes < 0)
    {
      if (errno == ENAMETOOLONG)
          {
                  untrappableerror ("Couldn't open the file (filename too long): ",
                          csl->filename);
          }
      else
          {
        untrappableerror ("Couldn't open the file: ",
                          csl->filename);
          }
    }
  else
  {
  if (internal_trace)
    fprintf (stderr, "file open on file descriptor %ld\n", csl->filedes);

  // and stat the file descriptor
  fstat (csl->filedes, &statbuf);
  csl->nchars = statbuf.st_size;
  if (internal_trace)
    fprintf (stderr, "file is %ld bytes\n", csl->nchars);
  if (csl->nchars + 2048 > max_pgmsize)
  {
          assert(csl->filedes >= 0);
          close(csl->filedes);
    untrappableerror ("Your program is too big.  ",
                      " You need to use smaller programs or the -P flag, ");
  }

  //   and read in the source file
  csl->filetext = (char *) calloc ( max_pgmsize , sizeof (csl->filetext[0]));
  if (csl->filetext == NULL)
  {
          assert(csl->filedes >= 0);
          close(csl->filedes);
          untrappableerror ("malloc of the file text failed", csl->filename);
  }
  if (internal_trace)
    fprintf (stderr, "File text malloc'd at %p\n", csl->filetext);

  //    put in a newline at the beginning
  csl->filetext[0] = '\n';

  /* [i_a] make sure we never overflow the buffer: */
  if (csl->nchars + 1 + 3 > max_pgmsize)
  {
          csl->nchars = max_pgmsize - 1 - 3;
  }
  //     read the file in...
  assert(csl->nchars + 1 + 3 <= max_pgmsize);
  i = read (csl->filedes, &(csl->filetext[1]), csl->nchars);
  /*
     From the MS docs: If fd is invalid, the file is not open for reading,
         or the file is locked, the invalid parameter handler is invoked, as
         described in Parameter Validation. If execution is allowed to continue,
         the function returns �1 and sets errno to EBADF.
  */
  if (i < 0)
  {
          assert(csl->filedes >= 0);
          close(csl->filedes);
          untrappableerror("Cannot read from file (it may be locked?): ", csl->filename);
  }

  /*
     Close the file handle now we're done. (This is important when we have nested files:
     there's only so many file handles to go around.)
   */
  assert(csl->filedes >= 0);
  close(csl->filedes);

  //     and put a cr and then a null at the end.
  i++;
  csl->filetext[i] = '\n';
  i++;
  csl->filetext[i] = '\n';
  i++;
  csl->filetext[i] = '\000';
  assert(i < max_pgmsize);
  csl->nchars = i;

  csl->hash = strnhash (csl->filetext, csl->nchars);
  if (user_trace)
    fprintf (stderr, "Hash of program: %lX, length %ld bytes\n",
             csl->hash, csl->nchars );
  }

  return 0;
}


//      The CRM114 microcompiler.  It takes in a paritally completed
//      csl struct which is the program, the program length, and
//      returns a completed microcompile table for that particular
//      program.  Side effect: it also sets some variables in the
//      variable hash table (a.k.a. the VHT)

int crm_microcompiler ( CSL_CELL *csl, VHT_CELL ** vht )
{
  //    ***   This is the CRM114 microcompiler.  It's a 5-pass
  //   compiler, but each pass is really simple.
  //   pass 1) count lines and allocate microcompile table
  //   pass 2) run thru file, matching on first word in statement, setting
  //        statement type code.  If it's a label statement, also add
  //        an entry to the variables hash table.  If the statement
  //        assigns a value, we _don't_ put the value itself into the
  //        variable hash table until we actually execute the statement
  //   pass 3) run thru file, setting bracket nesting level.  If
  //        bracket level ever goes below 0 or is nonzero at the end
  //        of the file, issue a warning.
  //   pass 4) run thru file, setting FAIL and LIAF targets

  //
  //    HACK ALERT HACK ALERT HACK ALERT
  //
  //    THIS WHOLE PARSER IS A PIECE OF JUNK.  IT REALLY NEEDS TO BE
  //    REDONE IN BISON.  MAYBE FOR V 2.0?
  //
  //    NOTE: this is redone to be table-driven; it's still a piece of
  //    junk but it's _good_ junk.  And it will allow us to do JITting
  //    of programs (1 pass to get labels, then JIT each statement as we
  //    encounter it the first time.  This should make programs run
  //    significantly faster as we never parse twice, and we only parse
  //    the full statement if we are about to execute it.
  //
  //    HACK ALERT HACK ALERT HACK ALERT


  //  i, j, and k are random beat-upon-me longs
  long i, j, k;

  //  a counter to use when iterating thru statements
  long stmtnum;

  //  number of statements actually used
  long numstmts;

  //   how many chars in this program
  long pgmlength;

  //   pointer to the chars
  char *pgmtext;

  // how deep a nesting of brackets does this file have?
  long bracketlevel;

  // index of first char of this statement
  long sindex;

  // index of first nonblank character in the line?
  long nbindex;

  // length of the first nonblank string on the line?
  long nblength;

  // index of first character in the arguments to any particular statement
  long aindex;

  // length of this statement
  long slength;

  //  have we seen an action statement yet?
  long seenaction ;

  //   counters for looking through the statemt archetype table.
  long stab_idx;
  long stab_max;

  if (internal_trace)
    fprintf (stderr, "Starting phase 1 of microcompile.\n");

  seenaction = 0;
  pgmlength = csl->nchars;
  pgmtext = csl->filetext;

  //  ***  Microcompile phase 1 ****
  //  *** Allocation of the microcompile table

  //  get a line count
  j = 0;   //  j will be our line counter here.

  //    preprocessor has already run; all statements have been
  //   properly line-broken and we just count the '\n's.
  for (i = 0; i < pgmlength; i++)
    {
      if (  pgmtext[i] == '\n' )
        j++;
    }

  csl->nstmts = j;

  //  now, allocate the microcompile table
  if (user_trace)
    fprintf (stderr, "Program statements: %ld, program length %ld\n",
             j, pgmlength);

  csl->mct = (MCT_CELL **) calloc ((csl->nstmts + 10), sizeof(csl->mct[0]) );
  if (!csl->mct)
    untrappableerror("Couldn't malloc MCT table.\n"
                     "This is where your compilation results go, "
                     "so if we can't compile, we can't run.  Sorry.","");

  if (internal_trace)
    fprintf (stderr, "microcompile table at %p\n", (void *)csl->mct);

  //  malloc all of the statement cells.
  for (i = 0; i < csl->nstmts + 10; i++)
    {
      csl->mct[i] = (MCT_CELL *) calloc (1, sizeof(csl->mct[i][0]));
      if (!csl->mct[i])
        untrappableerror(
                "Couldn't malloc MCT cell. This is very bad.\n","");
    }

  // ***  Microcompile phase 2 - set statement types

  //   iterate through the statements, setting types.
  //   i is our character counter
  //
  //    HACK ALERT HACK ALERT HACK ALERT
  //
  //    THIS WHOLE PARSER IS A PIECE OF JUNK.  IT REALLY NEEDS TO BE
  //    REDONE IN BISON.  MAYBE FOR V 2.0?
  //
  //    HACK ALERT HACK ALERT HACK ALERT

  if (internal_trace)
    fprintf (stderr, "Starting phase 2 of microcompile.\n");

  stmtnum = 0;
  sindex = 0;
  bracketlevel = 0;


  // #define STAB_TEST
  // #ifdef STAB_TEST
  //    Since we don't know how big the stmt_table actually is,
  //    we go through it once, looking for the "NoMoreStmts" statement,
  //    with operation code of CRM_BOGUS.  This tells us how many
  //    entries there are; we also set up the namelens for the
  //    statement types.
  //
  stab_idx = 0;
  while ( strncmp (stmt_table[stab_idx].stmt_name,
                   "NoMoreStmts",
                   strlen ("NoMoreStmts")) != 0)
    {
      if (stmt_table[stab_idx].namelen == 0)
        stmt_table[stab_idx].namelen = strlen (stmt_table[stab_idx].stmt_name);
      stab_idx++;
    }
  stab_max = stab_idx;
  //
  //    now the statement table should be set up.

  //  #endif

  while (stmtnum <= csl->nstmts && sindex < pgmlength)
    {
      long stab_stmtcode;
      long stab_done;

      //      the strcspan below will fail if there's an unescaped
      //      semicolon embedded in a string (or, for that matter, an
      //      explicit newline).  Fortunately, the preprocessor fixes the
      //      former and the latter is explicitly prohibited by the language.
      //
      slength = strcspn (&pgmtext[sindex], "\n");

      // allocate and fill in the mct table entry for this line
      csl->mct[stmtnum]->hosttxt = pgmtext;
      csl->mct[stmtnum]->apb = NULL;
      csl->mct[stmtnum]->start = sindex;
      csl->mct[stmtnum+1]->start = sindex + slength + 1;
      csl->mct[stmtnum]->stmt_utime = 0;
      csl->mct[stmtnum]->stmt_stime = 0;
      csl->mct[stmtnum]->stmt_type = CRM_BOGUS;
      csl->mct[stmtnum]->nest_level = bracketlevel;
      csl->mct[stmtnum]->fail_index = 0;
      csl->mct[stmtnum]->liaf_index = 0;
      csl->mct[stmtnum]->stmt_break = 0;
      csl->cstmt = stmtnum;

      //  skip nbindex to the first nonblank
      //  GROT GROT GROT  here we define nonblank as values > 0x21
      //  GROT GROT GROT  which absolutely _sucks_ in terms of coding
      //  GROT GROT GROT  portability, but it's what we have.
      nbindex = sindex;
      while (pgmtext[nbindex] < 0x021 && nbindex < slength + sindex)
        nbindex++;

      //  save up the first nonblank char:
      csl->mct[stmtnum]->fchar = nbindex;

      // and set up the start of arguments as well, they start at the first
      // nonblank after the first blank after the command...

      aindex = nbindex;
      while (pgmtext[aindex] > 0x021 && aindex < slength + sindex )
        aindex++;
      nblength = aindex - nbindex ;

      while (pgmtext[aindex] < 0x021 && aindex < slength + sindex )
        aindex++;

      csl->mct[stmtnum]->achar = aindex;

      //    We can now sweep thru the statement archetype table from 0
      //    to stab_max and compare the strlens and strings themselves.
      //
      stab_done = 0;
      stab_stmtcode = 0;

      //                    Empty lines are noops.
      if (nblength == 0)
        {
          stab_done = 1;
          stab_stmtcode = CRM_NOOP;
        }
      //                            Comment lines are also NOOPS
      if ( pgmtext[nbindex] == '#')
        {
          stab_done = 1;
          stab_stmtcode = CRM_NOOP;
        }
      //                             :LABEL: lines get special treatment
      if ( pgmtext[nbindex] == ':'
           && pgmtext[nbindex + nblength - 1] == ':')
        {
          stab_done = 1;
          stab_stmtcode = CRM_LABEL;
          k = strcspn (&pgmtext[nbindex+1], ":");
          crm_setvar ( NULL, -1, pgmtext, nbindex, k+2,
                       NULL, 0, 0,  stmtnum);
        }

      //                 INSERTs get special handling (NOOPed..)
      if ( strncasecmp ( &pgmtext[nbindex], "insert=", 7) == 0)
        {
          stab_done = 1;
          stab_stmtcode = CRM_NOOP;
        }
      i = -1;

      //                      Now a last big loop for the rest of the stmts.
      while (! stab_done)
        {
          i++;
          if ( nblength == stmt_table[i].namelen
               &&  strncasecmp (&pgmtext[nbindex],
                                stmt_table[i].stmt_name,
                                nblength) == 0)
            {
              stab_done = 1;
              stab_stmtcode = stmt_table[i].stmt_code;
              //   Deal with executable statements and WINDOW
              if (stab_stmtcode == CRM_WINDOW && !seenaction)
                csl->preload_window = 0;
              //   and mark off the executable statements
              if (stmt_table[i].is_executable) seenaction = 1;
            }
          if (i >= stab_max)
            stab_done = 1;
        }

      //            Fill in the MCT entry with what we've learned.
      //
      csl->mct [stmtnum] -> stmt_type = stab_stmtcode;
      if (stab_stmtcode == CRM_OPENBRACKET)
        bracketlevel++;
      if (stab_stmtcode == CRM_CLOSEBRACKET)
        {
          bracketlevel--;
          // hack - reset the bracketlevel here, as a bracket is "outside"
          //  the nestlevel, not inside.
          csl->mct[stmtnum]->nest_level = bracketlevel;
        }

      if (0) // (internal_trace)
        {
          fprintf (stderr, "\nStmt %3ld type %2d ",
                 stmtnum, csl->mct[stmtnum]->stmt_type);
          {
            long ic;
            for (ic = csl->mct[stmtnum    ]->start;
                 ic < csl->mct[stmtnum + 1]->start-1 ;
                 ic++)
              fprintf (stderr, "%c", pgmtext[ic]);
          }
        }

#ifdef STAB_TEST
      if (stab_stmtcode != csl->mct[stmtnum]-> stmt_type)
        {
          fprintf (stderr,"Darn!  Microcompiler stab error (not your fault!)\n"
                   "Please file a bug report if you can.  The data is:\n");
          fprintf (stderr,
   "Stab got %ld, Ifstats got %d, on line %ld with len %ld\n",
                   stab_stmtcode,
                   csl->mct[stmtnum]->stmt_type,
                   stmtnum,
                   nblength );
          fprintf (stderr, "String was >>>");
          fwrite ( &pgmtext[nbindex], 1, nblength, stderr);
          fprintf (stderr, "<<<\n\n");
        }
#endif


      //    check for bracket level underflow....
      if (bracketlevel < 0)
        fatalerror (" Your program seems to achieve a negative nesting",
                       "level, which is quite likely bogus.");

      // move on to the next statement - +1 to get past the\n
      sindex = sindex + slength + 1;

      stmtnum++;
    }

  numstmts = stmtnum - 1;

  //  check to be sure that the brackets close!

  if (bracketlevel != 0)
    nonfatalerror ("\nDang!  The brackets don't match up!\n",
      "Check your source code. ");


  //  Phase 3 of microcompiler- set FAIL and LIAF targets for each line
  //  in the MCT.

  {
    long stack[MAX_BRACKETDEPTH];
    long sdx ;

    if (internal_trace)
      fprintf (stderr, "Starting phase 3 of microcompile.\n");

    //  set initial stack values
    sdx = 0;
    stack[sdx] = 0;

    //   Work downwards first, assigning LIAF targets
    for (stmtnum = 0; stmtnum < numstmts; stmtnum++)
      {
        switch (csl->mct[stmtnum]-> stmt_type)
          {
          case  CRM_OPENBRACKET:
            {
              //  if we're open bracket, we're the new LIAF target,
              //  but we ourselves LIAF to the previous open bracket.
              csl->mct[stmtnum]->liaf_index = stack[sdx];
              sdx++;
              stack[sdx] = stmtnum;
            }
            break;
          case CRM_CLOSEBRACKET:
            {
              //  if we're a close bracket, we LIAF not to the current
              //  open bracket, but to the one before it, so pop the
              //  stack and LIAF there.
              sdx--;
              csl->mct[stmtnum]->liaf_index = stack [sdx];
            }
            break;
          default:
            {
              //   Most statements use the current liaf
              csl->mct[stmtnum] -> liaf_index = stack[sdx];
            }
            break;
          }
      }

    //   Work upwards next, assigning the fail targets
    sdx = 0;
    stack[sdx] = numstmts+1;
    for (stmtnum = numstmts; stmtnum >= 0; stmtnum--)
      {
        switch (csl->mct[stmtnum]-> stmt_type)
          {
          case  CRM_CLOSEBRACKET:
            {
              //  if we're close bracket, we're the new FAIL target,
              //  but we ourselves FAIL to the next close bracket
              csl->mct[stmtnum]->fail_index = stack[sdx];
              sdx++;
              stack[sdx] = stmtnum;
            }
            break;
          case CRM_OPENBRACKET:
            {
              //  if we're an open bracket, we FAIL not to the current
              //  CLOSE bracket, but to the one before it, so pop the
              //  stack and FAIL there.
              sdx--;
              csl->mct[stmtnum]->fail_index = stack [sdx];
            }
            break;
          default:
            {
              //   Most statements use the current liaf
              csl->mct[stmtnum] -> fail_index = stack[sdx];
            }
            break;
          }
      }


    //   Work upwards again, assigning the TRAP targets
    sdx = 0;
    stack[sdx] = numstmts+1;
    for (stmtnum = numstmts; stmtnum >= 0; stmtnum--)
      {
        switch (csl->mct[stmtnum]-> stmt_type)
          {
          case  CRM_TRAP:
            {
              //  if we're the TRAP statement, we change the TRAP target,
              //  but we ourselves TRAP to the next TRAP statement
              csl->mct[stmtnum]->trap_index = stack[sdx];
              stack[sdx] = stmtnum;
            }
            break;
          case CRM_OPENBRACKET:
            {
              //  if we're an open bracket, we trap not to the current
              //  level's TRAP statement, but to the one before it, so pop the
              //  stack and aim TRAP there.
              sdx--;
              csl->mct[stmtnum]->trap_index = stack [sdx];
            }
            break;
          case CRM_CLOSEBRACKET:
            {
              //  if we're a close bracket, we keep our current trap target
              //  but move down one level in the stack
              stack[sdx + 1] = stack [sdx];
              sdx++;
              csl->mct[stmtnum]->trap_index = stack [sdx];
            }
            break;
          default:
            {
              //   Most statements use the current TRAP level
              csl->mct[stmtnum] -> trap_index = stack[sdx];
            }
            break;
          }
      }

    //  print out statement info if desired
    if ( prettyprint_listing > 0 )
      {
        for (stmtnum = 0; stmtnum <= numstmts; stmtnum++)
          {
            fprintf (stderr, "\n");
            if (prettyprint_listing > 1)
                fprintf (stderr, "%4.4ld ", stmtnum);
            if (prettyprint_listing > 2)
              fprintf (stderr, "{%2.2d}", csl->mct[stmtnum]->nest_level);

            if (prettyprint_listing > 3)
              {
                fprintf (stderr, " <<%2.2d>>",
                         csl->mct[stmtnum]->stmt_type);

                fprintf (stderr, " L%4.4d F%4.4d T%4.4d",
                         csl->mct[stmtnum]->liaf_index,
                         csl->mct[stmtnum]->fail_index,
                         csl->mct[stmtnum]->trap_index);
              }
            if (prettyprint_listing > 1)
              fprintf (stderr, " :  ");

            //  space over two spaces per indent
            for (k = 0; k < csl->mct[stmtnum]->nest_level; k++)
              fprintf (stderr, "  ");

            //   print out text of the first statement:
            if (prettyprint_listing > 4)
              fprintf (stderr,"-");
            k = csl->mct[stmtnum]->fchar;
            while (pgmtext[k] > 0x021 && k < csl->mct[stmtnum]->achar)
              {
                fprintf (stderr, "%c", pgmtext[k]);
                k++;
              }
            if (prettyprint_listing > 4)
              fprintf (stderr,"-");

            fprintf (stderr, " ");
            //  and if there are args, print them out as well.
            if ( csl->mct[stmtnum]->achar < csl->mct[stmtnum+1]->start-1)
              {
                if (prettyprint_listing > 4)
                  fprintf (stderr, "=");
                for (k = csl->mct[stmtnum]->achar;
                     k< csl->mct[stmtnum+1]->start-1; k++)
                  fprintf (stderr, "%c", pgmtext[k]);
                if (prettyprint_listing > 4)
                  fprintf (stderr, "=");
              }
          }
        fprintf (stderr, "\n");
      }

    //    Finally got to the end.  Fill in the last bits of the CSL
    //  with the new information, and return.

    csl->nstmts = numstmts;

    if (internal_trace)
      fprintf (stderr, "microcompile completed\n");
  }

  return (0);
}

