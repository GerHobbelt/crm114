//  crm_correlate.c  - Controllable Regex Mutilator,  version v1.0
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


//    How to learn correlation-style- just append the text to be
//    learned to the target file.  We don't care about the /regexes/
//

int crm_expr_correlate_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                              char *txtptr, long txtstart, long txtlen)
{
  //     learn the given text as correlative text
  //     belonging to a particular type.
  //     learn <flags> (classname) /regex/ (regex is ignored)
  //
  long i, j, k;
  char ptext[MAX_PATTERN];  //  the regex pattern
  long plen;
  char ltext[MAX_PATTERN];  //  the variable to learn
  long llen;
  char htext[MAX_PATTERN];  //  the hash name
  long hlen;
  long cflags, eflags;
  struct stat statbuf;      //  for statting the hash file
  int hfd;                  //  hashfile fd
  long hfsize;              //  size of the hash file
  //
  //regex_t regcb;
  long textoffset;
  long textlen;
  long sense;
  long vhtindex;
  long microgroom;
  long fev;
  long made_new_file;

    char *learnfilename;


  if (internal_trace)
    fprintf (stderr, "executing a LEARN (correlation format)\n");

  //   Keep the gcc compiler from complaining about unused variables
  //  i = hctable[0];

  //           extract the hash file name
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, MAX_PATTERN);
  //
  //           extract the variable name (if present)
  crm_get_pgm_arg (ltext, MAX_PATTERN, apb->b1start, apb->b1len);
  llen = apb->b1len;
  llen = crm_nexpandvar (ltext, llen, MAX_PATTERN);

  //     get the "this is a word" regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);

  //            set our cflags, if needed.  The defaults are
  //            "case" and "affirm", (both zero valued).
  //            and "microgroom" disabled.
  cflags = REG_EXTENDED;
  eflags = 0;
  sense = +1;
  if (apb->sflags & CRM_NOCASE)
    {
      cflags = cflags | REG_ICASE;
      eflags = 1;
      if (user_trace)
        fprintf (stderr, "turning oncase-insensitive match\n");
    }
  if (apb->sflags & CRM_REFUTE)
    {
      sense = -sense;
      if (user_trace)
        fprintf (stderr, " refuting learning\n");
    }
  microgroom = 0;
  if (apb->sflags & CRM_MICROGROOM)
    {
      microgroom = 1;
      if (user_trace)
        fprintf (stderr, " enabling microgrooming.\n");
    }

  //
  //             grab the filename, and stat the file
  //      note that neither "stat", "fopen", nor "open" are
  //      fully 8-bit or wchar clean...
  i = 0;
  while (htext[i] < 0x021) i++;
  CRM_ASSERT(i < hlen);
  j = i;
  while (htext[j] >= 0x021) j++;
  CRM_ASSERT(j <= hlen);

  //             filename starts at i,  ends at j. null terminate it.
  htext[j] = 0;
  learnfilename = strdup(&(htext[i]));


  //             and stat it to get it's length
  k = stat (learnfilename, &statbuf);

  made_new_file = 0;

  //             quick check- does the file even exist?
  if (k != 0)
    {
      //      file didn't exist... create it
      FILE *f;
      if (user_trace)
        {
          fprintf (stderr, "\nCreating new correlate file %s\n", learnfilename);
          fprintf (stderr, "Opening file %s for write\n", learnfilename);
        }
      f = fopen (learnfilename, "wb");
      if (!f)
        {
          fev = nonfatalerror_ex(SRC_LOC(),
                "\n Couldn't open your new correlate file %s for writing; errno=%d(%s)\n",
                 learnfilename,
                                 errno,
                                 errno_descr(errno)
                                 );

          if (engine_exit_base != 0)
          {
            exit (engine_exit_base + 1);
          }
          else
                  {
            exit (EXIT_FAILURE);
                  }
        }

      //      fputc ('\001', f); don't do any output at all.
      made_new_file = 1;
      //
            if (f)
                {
      fclose (f);
                }
      //    and reset the statbuf to be correct
      k = stat (learnfilename, &statbuf);
          CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
    }
  //
  hfsize = statbuf.st_size;
  if (user_trace)
  {
    fprintf (stderr, "Correlation text file %s has length %ld characters\n",
             learnfilename, hfsize / sizeof (FEATUREBUCKET_TYPE));
  }

  //
  //         open the text file into memory so we can bitwhack it
  //
  hfd = open (learnfilename, O_RDWR | O_BINARY);
  if (hfd < 0)
    {
      fev = fatalerror ("Couldn't open the correlation file named: ",
                        learnfilename);
      free(learnfilename);
      return (fev);
    }

  //
  //    get the text to "learn" (well, append to the correlation file)
  //
  //     This is the text that we'll append to the correlation file.

  /* removed i=0: re-init here: important! */

  if (llen > 0)
    {
      vhtindex = crm_vht_lookup (vht, ltext, llen);
    }
  else
    {
      vhtindex = crm_vht_lookup (vht, ":_dw:", 5);
    }

  if (vht[vhtindex] == NULL)
    {
      long q;

          CRM_ASSERT(hfd >= 0);
          close(hfd);
      q = fatalerror (" Attempt to LEARN from a nonexistent variable ",
                  ltext);
      free(learnfilename);
      return (q);
    }
  mdw = NULL;
  if (tdw->filetext == vht[vhtindex]->valtxt)
    mdw = tdw;
  if (cdw->filetext == vht[vhtindex]->valtxt)
    mdw = cdw;
  if (mdw == NULL)
    {
      long q;
          CRM_ASSERT(hfd >= 0);
          close(hfd);
      q = fatalerror (" Bogus text block containing variable ", ltext);
      free(learnfilename);
      return (q);
    }
  else
  {
  textoffset = vht[vhtindex]->vstart;
  textlen = vht[vhtindex]->vlen;

  if (user_trace)
    {
      fprintf (stderr, "learning the text (len %ld) :", textlen);
      fwrite (&(mdw->filetext[textoffset]),
              ((textlen < 128) ? textlen : 128),1,stderr);
      fprintf (stderr, "\n");
    }

  //      append the "learn" text to the end of the file.
  //
  lseek (hfd, 0, SEEK_END);
  if (textlen != write(hfd, &(mdw->filetext[textoffset]), textlen))
  {
          long q;
          CRM_ASSERT(hfd >= 0);
          close(hfd);
          q = fatalerror("Failed to append the 'learn' text to the correlation file '%s'\n",
                  learnfilename);
          free(learnfilename);
          return q;
  }
  }

  CRM_ASSERT(hfd >= 0);
  close (hfd);

  free(learnfilename);
  return (0);
}


//      How to do a correlate-style CLASSIFY on some text.
//
int crm_expr_correlate_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                                 char *txtptr, long txtstart, long txtlen)
{

  //      classify the sparse spectrum of this input window
  //      as belonging to a particular type.
  //
  //       This code should look very familiar- it's cribbed from
  //       the code for LEARN
  //
  long i, j, k;
  char ptext[MAX_PATTERN];  //  the regex pattern
  long plen;
  //  the hash file names
  char htext[MAX_PATTERN+MAX_CLASSIFIERS*MAX_FILE_NAME_LEN];
  long htext_maxlen = MAX_PATTERN+MAX_CLASSIFIERS*MAX_FILE_NAME_LEN;
  long hlen;
  //  the match statistics variable
  char stext[MAX_PATTERN+MAX_CLASSIFIERS*(MAX_FILE_NAME_LEN+100)];
  long stext_maxlen = MAX_PATTERN+MAX_CLASSIFIERS*(MAX_FILE_NAME_LEN+100);
  long slen;
  char svrbl[MAX_PATTERN];  //  the match statistics text buffer
  long svlen;
  long fnameoffset;
  char fname[MAX_FILE_NAME_LEN];
  long eflags;
  long cflags;

  struct stat statbuf;      //  for statting the hash file
  //regex_t regcb;

  unsigned long fcounts[MAX_CLASSIFIERS]; // total counts for feature normalize

  double cpcorr[MAX_CLASSIFIERS];  // corpus correction factors
  long long linear_hits[MAX_CLASSIFIERS];  // actual hits per classifier
  long long square_hits[MAX_CLASSIFIERS];  // square of runlenths of match
  long long cube_hits[MAX_CLASSIFIERS];  // cube of runlength matches
  long long quad_hits[MAX_CLASSIFIERS];  // quad of runlength matches
  long incr_hits[MAX_CLASSIFIERS];  // 1+2+3... hits per classifier

  long long total_linear_hits;  // actual total linear hits for all classifiers
  long long total_square_hits;  // actual total square hits for all classifiers
  long long total_cube_hits;    // actual total cube hits for all classifiers
  long long total_quad_hits;    // actual total cube hits for all classifiers
  long long total_features;     // total number of characters in the system

  hitcount_t totalhits [MAX_CLASSIFIERS];
  double tprob;         //  total probability in the "success" domain.

  long textlen;    //  text length  - rougly corresponds to
                        //  information content of the text to classify

  double ptc[MAX_CLASSIFIERS]; // current running probability of this class
  double renorm = 0.0;

  char *hashes[MAX_CLASSIFIERS];
  long hashlens[MAX_CLASSIFIERS];
  char *hashname[MAX_CLASSIFIERS];
  long succhash;
  long vbar_seen;       // did we see '|' in classify's args?
  long maxhash;
  long fnstart, fnlen;
  long fn_start_here;
  long textoffset;
  long bestseen;
  long thistotal;

  if (internal_trace)
    fprintf (stderr, "executing a CLASSIFY\n");

  //          we use the main line txtptr, txtstart, and txtlen now,
  //          so we don't need to extract anything from the b1start stuff.

  //           extract the hash file names
  crm_get_pgm_arg (htext, htext_maxlen, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, htext_maxlen);

  //           extract the "this is a word" regex
  //
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);

  //            extract the optional "match statistics" variable
  //
  crm_get_pgm_arg (svrbl, MAX_PATTERN, apb->p2start, apb->p2len);
  svlen = apb->p2len;
  svlen = crm_nexpandvar (svrbl, svlen, MAX_PATTERN);
  {
    long vstart, vlen;
    crm_nextword (svrbl, svlen, 0, &vstart, &vlen);
    memmove (svrbl, &svrbl[vstart], vlen);
    svlen = vlen;
    svrbl[vlen] = '\000';
  }

  //     status variable's text (used for output stats)
  //
  stext[0] = '\000';
  slen = 0;

  //            set our flags, if needed.  The defaults are
  //            "case"
  cflags = REG_EXTENDED;
  eflags = 0;

  if (apb->sflags & CRM_NOCASE)
    {
      cflags += REG_ICASE;
      eflags = 1;
    }

  //   compile the word regex
#if 0 /* unused code */
  if ( internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);
  i = crm_regcomp (&regcb, ptext, plen, cflags);
  if ( i > 0)
    {
      crm_regerror ( i, &regcb, tempbuf, data_window_size);
      nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
      goto regcomp_failed;
    }
#endif



  //       Now, the loop to open the files.
  bestseen = 0;
  thistotal = 0;

  //      initialize our arrays for N .css files
  for (i = 0; i < MAX_CLASSIFIERS; i++)
    {
      fcounts[i] = 0;    // check later to prevent a divide-by-zero
                         // error on empty .css file
      cpcorr[i] = 0.0;   // corpus correction factors
      linear_hits[i] = 0;     // linear hits
      square_hits[i] = 0;     // square of the runlength
      cube_hits[i] = 0;       // cube of the runlength
      quad_hits[i] = 0;       // quad of the runlength
      incr_hits[i] = 0;      // 1+2+3... hits hits
      totalhits[i] = 0;     // absolute hit counts
      ptc[i] = 0.5;      // priori probability
    }

  //

  vbar_seen = 0;
  maxhash = 0;
  succhash = 0;
  fnameoffset = 0;

  //    now, get the file names and mmap each file
  //     get the file name (grody and non-8-bit-safe, but doesn't matter
  //     because the result is used for open() and nothing else.
  //   GROT GROT GROT  this isn't NULL-clean on filenames.  But then
  //    again, stdio.h itself isn't NULL-clean on filenames.
  if (user_trace)
    fprintf (stderr, "Classify list: -%s-\n", htext);
  fn_start_here = 0;
  fnlen = 1;
  while ( fnlen > 0 && ((maxhash < MAX_CLASSIFIERS-1)))
    {
      crm_nextword (htext,
                    hlen, fn_start_here,
                    &fnstart, &fnlen);
      if (fnlen > 0)
        {
          strncpy (fname, &htext[fnstart], fnlen);
          fn_start_here = fnstart + fnlen + 1;
          fname[fnlen] = '\000';
          if (user_trace)
            fprintf (stderr, "Classifying with file -%s- "
                             "succhash=%ld, maxhash=%ld\n",
                             fname, succhash, maxhash);
          if ( fname[0] == '|' && fname[1] == '\000')
            {
              if (vbar_seen)
                {
                  nonfatalerror ("Only one ' | ' allowed in a CLASSIFY.\n" ,
                                 "We'll ignore it for now.");
                }
              else
                {
                  succhash = maxhash;
                }
              vbar_seen ++;
            }
          else
            {
              //  be sure the file exists
              //             stat the file to get it's length
              k = stat (fname, &statbuf);
              //             quick check- does the file even exist?
              if (k != 0)
                {
                  nonfatalerror ("Nonexistent Classify table named: ",
                                 fname);
                }
              else
                {
                  //  file exists - do the mmap
                  //
                  hashlens[maxhash] = statbuf.st_size;
                  hashes[maxhash] = (char *)
                      crm_mmap_file( fname,
                                     0, hashlens[maxhash],
                                     PROT_READ,
                                     MAP_SHARED,
                                     NULL);
                  if (hashes[maxhash] == MAP_FAILED )
                        {
                          nonfatalerror ("Couldn't memory-map the table file",
                                         fname);
                        }
                      else
                        {
                          //
                          //     Check to see if this file is the right version
                          //
                          //     FIXME : for now, there's no version number
                          //     associated with a .correllation file
                          long fev;
                          if (0)
                            //(hashes[maxhash][0].hash != 1 ||
                            //  hashes[maxhash][0].key  != 0)
                            {
                              fev =fatalerror ("The .css file is the wrong version!  Filename is: ",
                                               &htext[i]);
                              return (fev);
                            }

                          //
                          //     save the name for later...
                          //
                          hashname[maxhash] = (char *) calloc((fnlen+10) , sizeof(hashname[maxhash][0]));
                          if (!hashname[maxhash])
                          {
                            untrappableerror(
                              "Couldn't malloc hashname[maxhash]\n","We need that part later, so we're stuck.  Sorry.");
                          }
                          strncpy(hashname[maxhash],fname,fnlen);
                          hashname[maxhash][fnlen]='\000';
                          maxhash++;
                        }
                }
            }
          if (maxhash > MAX_CLASSIFIERS-1)
            nonfatalerror ("Too many classifier files.",
                           "Some may have been disregarded");
        }
    }

  //
  //    If there is no '|', then all files are "success" files.
  if (succhash == 0)
    succhash = maxhash;

  //    a CLASSIFY with no arguments is always a "success".
  if (maxhash == 0)
    return (0);

  //    now, set up the normalization factor fcount[]
  if (user_trace)
    fprintf (stderr, "Running with %ld files for success out of %ld files\n",
             succhash, maxhash );
  // sanity checks...  Uncomment for super-strict CLASSIFY.
  //
  //    do we have at least 1 valid .css files?
  if (maxhash == 0)
    {
      fatalerror ("Couldn't open at least 1 .css files for classify().", "");
    }
  //    do we have at least 1 valid .css file at both sides of '|'?
  //if (!vbar_seen || succhash < 0 || (maxhash < succhash + 2))
  //  {
  //    nonfatalerror (
  //      "Couldn't open at least 1 .css file per SUCC | FAIL classes "
  //    " for classify().\n","Hope you know what are you doing.");
  //  }

  //
  //   now all of the files are mmapped into memory,
  //   and we can do the correlations and add up matches.
  i = 0;
  j = 0;
  k = 0;
  thistotal = 0;

  //     put in the ptr/start/len values we got from the outside caller
  textoffset = txtstart;
  textlen = txtlen;

  //
  //    We keep track of the hits in these categories
  //  linear_hits[MAX_CLASSIFIERS];  // actual hits per classifier
  //  square_hits[MAX_CLASSIFIERS];  // square of runlenths of match
  //  incr_hits[MAX_CLASSIFIERS];  // 1+2+3... hits per classifier
  //

  //   Now we do the actual correllation.
  //   for each file...
  //    slide the incoming text (mdw->filetext[textofset])
  //     across the corpus text (hashes[] from 0 to hashlens[])
  //      and count the bytes that are the same, the runlengths,
  //       etc.

  for (k = 0; k < maxhash; k++)
    {
      long it;    // it is the start index into the tested text
      long ik;    // ik is the start index into the known corpus text
      long ilm;   // ilm is the "local" matches (N in a row)

      //    for each possible displacement of the known  (ik) text...
      for (ik = 0;
           ik < hashlens[k];
           ik++)
        {
          long itmax;
          ilm = 0;
          itmax = textlen;
          if (ik + itmax > hashlens[k])
              itmax = hashlens[k] - ik;
          // for each position in the test (it) text...
          for (it = 0;
               it < itmax;
               it++)
            {
              //   do the characters in this position match?
              if ( hashes[k][ik+it] == txtptr[textoffset+it])
                {
                  // yes they matched
                  linear_hits[k]++;
                  ilm++;
                  square_hits[k] = square_hits[k] + (ilm * ilm);
                  cube_hits[k] = cube_hits[k] + (ilm * ilm * ilm);
                  quad_hits[k] = quad_hits[k] + (ilm * ilm * ilm * ilm);
                }
              else
                {
                  //   nope, they didn't match.
                  //   So, we do the end-of-runlength stuff:
                  ilm = 0;
                }
              if (0)
                fprintf (stderr, "ik: %ld  it: %ld  chars %c %c lin: %lld  sqr: %lld cube: %lld quad: %lld\n",
                         ik, it,
                         hashes[k][ik+it],
                         txtptr[textoffset+it],
                         linear_hits[k],
                         square_hits[k],
                         cube_hits[k],
                         quad_hits[k]);
            }
        }
    }


  //   Now we have the total hits for each text corpus.  We can then
  //  turn that into a vague probability measure, and then renormalize
  //  that to get probabilities.
  //
  //   But first, let's reflect on what we've got here.  We our test
  //   text, and we have a corpus which is "nominally correllated",
  //   and another corpus that is nominally uncorrellated.
  //
  //   The uncorrellated text will have an average match rate of 1/256'th
  //   in the linear domain (well, for random bytes; english text will match
  //   a lot more often, due to the fact that ASCII only uses the low 7
  //   bits, most text is written in lower case, Zipf's law, etc.
  //
  //   We can calculate a predicted total on a per-character basis for all
  //   of the corpi, then use that as an average expectation.

  //    Calculate total hits
  total_linear_hits = 0;
  total_square_hits = 0;
  total_cube_hits = 0;
  total_quad_hits = 0;
  total_features = 0;
  for (k = 0; k < maxhash; k++)
    {
      total_linear_hits += linear_hits[k];
      total_square_hits += square_hits[k];
      total_cube_hits += cube_hits[k];
      total_quad_hits += quad_hits[k];
      total_features += hashlens[k];
    }


  for (k = 0; k < maxhash; k++)
    {
      if (hashlens[k] > 0
          && total_features > 0 )
        {
          //     Note that we don't normalize the probabilities yet- we do
          //     that down below.
          //
          //     .00397 is not a magic number - it's the random coincidence
          //     rate for 1 chance in 256, with run-length-squared boost.
          //     .00806 is the random coincidence rate for 7-bit characters.
          //
          //ptc[k] = ((0.0+square_hits[k] - (.00397 * hashlens[k] )));
          //      ptc[k] = ((0.0+square_hits[k] - (.00806 * hashlens[k] )))
          //        / hashlens[k];

          //      ptc[k] = (0.0+square_hits[k] ) / hashlens[k];
          //      ptc[k] = (0.0+ quad_hits[k] ) / hashlens[k];
          ptc[k] = (0.0+ quad_hits[k] ) / linear_hits[k];

          if (ptc[k] < 0) ptc[k] = 10*DBL_MIN;
        }
      else
        {
          ptc [k] = 0.5;
        }
    }


  //    ptc[k] = (sqrt (0.0 + square_hits[k])-linear_hits[k] ) / hashlens[k] ;
  //    ptc[k] =  (0.0 + square_hits[k] - linear_hits[k] ) ;
  //    ptc[k] =  ((0.0 + square_hits[k]) / hashlens[k]) ;
  //    ptc[k] = sqrt ((0.0 + square_hits[k]) / hashlens[k]) ;
  //    ptc[k] = ((0.0 + linear_hits[k]) / hashlens[k]) ;


  //   calculate renormalizer (the Bayesian formula's denomenator)
  renorm = 0.0;

  //   now calculate the per-ptc numerators
  for (k = 0; k < maxhash; k++)
    renorm = renorm + (ptc[k]);

  //   check for a zero normalizer
  if (renorm == 0) renorm = 1.0;

  //  and renormalize
  for (k = 0; k < maxhash; k++)
    ptc[k] = ptc[k] / renorm;

  //   if we have underflow (any probability == 0.0 ) then
  //   bump the probability back up to 10^-308, or
  //   whatever a small multiple of the minimum double
  //   precision value is on the current platform.
  //
  for (k = 0; k < maxhash; k++)
    if (ptc[k] < 10*DBL_MIN) ptc[k] = 10 * DBL_MIN;

    if (internal_trace)
    {
      for (k = 0; k < maxhash; k++)
        {
          fprintf (stderr,
                   " file: %ld  linear: %lld  square: %lld  RMS: %6.4e  ptc[%ld] = %6.4e\n",
                   k, linear_hits[k], square_hits[k],
                   sqrt(0.0+square_hits[k]), k, ptc[k]);
        }
    }
  //

  ;      //  end of repeat-the-regex loop


  //  cleanup time!
  //  remember to let go of the fd's and mmaps
  for (k = 0; k < maxhash; k++)
    {
        crm_munmap_file(hashes[k]);
    }

  if (user_trace)
    {
      for (k = 0; k < maxhash; k++)
        fprintf (stderr, "Probability of match for file %ld: %f\n", k, ptc[k]);
    }
  //
  tprob = 0.0;
  for (k = 0; k < succhash; k++)
    tprob = tprob + ptc[k];
  if (svlen > 0)
    {
      char buf[1024];
      double accumulator;
      double remainder;
      double overall_pR;
      long m;
      buf [0] = '\000';
      accumulator = 10 * DBL_MIN;
      for (m = 0; m < succhash; m++)
        {
          accumulator = accumulator + ptc[m];
        }
      remainder = 10 * DBL_MIN;
      for (m = succhash; m < maxhash; m++)
        if (bestseen != m)
          {
            remainder = remainder + ptc[m];
          }
      overall_pR = log10 (accumulator) - log10 (remainder);

      //   note also that strcat _accumulates_ in stext.
      //  There would be a possible buffer overflow except that _we_ control
      //   what gets written here.  So it's no biggie.

      if (tprob > 0.5000)
        {
          sprintf (buf, "CLASSIFY succeeds; success probability: %6.4f  pR: %6.4f\n", tprob, overall_pR );
        }
      else
        {
          sprintf (buf, "CLASSIFY fails; success probability: %6.4f  pR: %6.4f\n", tprob, overall_pR );
        }
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
        strcat (stext, buf);
      bestseen = 0;
      for (k = 0; k < maxhash; k++)
        if (ptc[k] > ptc[bestseen] ) bestseen = k;
      remainder = 10 * DBL_MIN;
      for (m = 0; m < maxhash; m++)
        if (bestseen != m)
          {
            remainder = remainder + ptc[m];
          }
      snprintf (buf, WIDTHOF(buf), "Best match to file #%ld (%s) "\
               "prob: %6.4f  pR: %6.4f\n",
               bestseen,
               hashname[bestseen],
               ptc[bestseen],
               (log10(ptc[bestseen]) - log10(remainder)));
          buf[WIDTHOF(buf) - 1] = 0;
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
        strcat (stext, buf);
      sprintf (buf, "Total features in input file: %ld\n", hashlens[bestseen]);
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
        strcat (stext, buf);
      for (k = 0; k < maxhash; k++)
        {
          long m;
          remainder = 10 * DBL_MIN;
          for (m = 0; m < maxhash; m++)
            if (k != m)
              {
                remainder = remainder + ptc[m];
              }
          snprintf (buf, WIDTHOF(buf),
                   "#%ld (%s):"
                   " features: %ld, L1: %lld L2: %lld L3: %lld, L4: %lld prob: %3.2e, pR: %6.2f\n",
                   k,
                   hashname[k],
                   hashlens[k],
                   linear_hits[k],
                   square_hits[k],
                   cube_hits[k],
                   quad_hits[k],
                   ptc[k],
                   (log10 (ptc[k]) - log10 (remainder) )  );
          buf[WIDTHOF(buf) - 1] = 0;
          // strcat (stext, buf);
          if (strlen(stext)+strlen(buf) <= stext_maxlen)
            strcat (stext, buf);
        }
      // check here if we got enough room in stext to stuff everything
      // perhaps we'd better rise a nonfatalerror, instead of just
      // whining on stderr
      if (strcmp(&(stext[strlen(stext)-strlen(buf)]), buf) != 0)
        {
          nonfatalerror( "WARNING: not enough room in the buffer to create "
                         "the statistics text.  Perhaps you could try bigger "
                         "values for MAX_CLASSIFIERS or MAX_FILE_NAME_LEN?",
                         " ");
        }
      crm_destructive_alter_nvariable (svrbl, svlen,
                                       stext, strlen (stext));
    }

  //
  //  Free the hashnames, to avoid a memory leak.
  //
  for (i = 0; i < maxhash; i++)
    free (hashname[i]);
  if (tprob <= 0.5000)
    {
      if (user_trace)
        fprintf (stderr, "CLASSIFY was a FAIL, skipping forward.\n");
      //    and do what we do for a FAIL here
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk [csl->mct[csl->cstmt]->nest_level] = -1;
      return (0);
    }


  //
  //   all done... if we got here, we should just continue execution
  if (user_trace)
    fprintf (stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
// regcomp_failed:
  return (0);
}
