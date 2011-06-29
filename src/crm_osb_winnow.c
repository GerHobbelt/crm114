//  crm_osb_winnow.c  - Controllable Regex Mutilator,  version v1.0
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



////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static const long hctable[] =
    { 1, 7,
      3, 13,
      5, 29,
      11, 51,
      23, 101,
      47, 203,
      97, 407,
      197, 817,
      397, 1637,
      797, 3277 };



//          Where does the nominative data start?
static long spectra_start = 0;



//    How to learn Osb_Winnow style  - in this case, we'll include the single
//    word terms that may not strictly be necessary, but their weight will
//    be set to 0 in the evaluation.
//

int crm_expr_osb_winnow_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                               char *txtptr, long txtstart, long txtlen)
{
  //     learn the osb_winnow transform spectrum of this input window as
  //     belonging to a particular type.
  //     learn <flags> (classname) /word/
  //
  long i, j, k;
  long h;                   //  h is our counter in the hashpipe;
  char ptext[MAX_PATTERN];  //  the regex pattern
  long plen;
  char htext[MAX_PATTERN];  //  the hash name
  long hlen;
  long cflags, eflags;
  struct stat statbuf;      //  for statting the hash file
  long hfsize;              //  size of the hash file
  char *learnfilename;
  WINNOW_FEATUREBUCKET_STRUCT *hashes;  //  the text of the hash file
  unsigned char *xhashes;               //  and the mask of what we've seen
  unsigned long hashpipe[OSB_WINNOW_WINDOW_LEN+1];
  //
  regex_t regcb;
  regmatch_t match[5];      //  we only care about the outermost match
  long textoffset;
  long textmaxoffset;
  float sense;
  long microgroom;
  long use_unigrams;
  long fev;
  long made_new_file;



  if (internal_trace)
    fprintf (stderr, "executing an OSB-WINNOW LEARN\n");

  //   Keep the gcc compiler from complaining about unused variables
  //  i = hctable[0];

  //           extract the hash file name
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, MAX_PATTERN);
  //
  //        We get the varname and var-restriction from the caller now
  //  crm_get_pgm_arg (ltext, MAX_PATTERN, apb->b1start, apb->b1len);
  // llen = apb->b1len;
  // llen = crm_nexpandvar (ltext, llen, MAX_PATTERN);

  //     get the "this is a word" regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);

  //            set our cflags, if needed.  The defaults are
  //            "case" and "affirm", (both zero valued).
  //            and "microgroom" disabled.
  cflags = REG_EXTENDED;
  eflags = 0;

  if (apb->sflags & CRM_NOCASE)
    {
      cflags = cflags | REG_ICASE;
      eflags = 1;
      if (user_trace)
        fprintf (stderr, "turning oncase-insensitive match\n");
    }


  //
  sense = OSB_WINNOW_PROMOTION;
  if (apb->sflags & CRM_REFUTE)
    {
      sense = OSB_WINNOW_DEMOTION;
                    //  GROT GROT GROT Learning would be symmetrical
                    //  if this were
                    //       sense = 1.0 / sense;
                    //  but that's inferior, because thenn the weights are
                    // limited to the values of sense^n.
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

  use_unigrams = 0;
  if (apb->sflags & CRM_UNIGRAM)
    {
      use_unigrams = 1;
      if (user_trace)
        fprintf (stderr, " enabling unigram-only operation.\n");
    }


  //
  //             grab the filename, and stat the file
  //      note that neither "stat", "fopen", nor "open" are
  //      fully 8-bit or wchar clean...
  i = 0;
  while (htext[i] < 0x021) i++;
  j = i;
  while (htext[j] >= 0x021) j++;

  //             filename starts at i,  ends at j. null terminate it.
  htext[j] = '\000';

  learnfilename = strdup(&htext[i]);
  //             and stat it to get it's length
  k = stat (learnfilename, &statbuf);

  made_new_file = 0;

  //             quick check- does the file even exist?
  if (k != 0)
    {
      //      file didn't exist... create it
      FILE *f;
      if (user_trace)
        fprintf (stderr, "\n Opening new COW file %s for write\n", learnfilename);
      f = fopen (learnfilename, "wb");
      if (!f)
        {
          nonfatalerror_ex(SRC_LOC(),
                "\n Couldn't open your new COW file %s for writing; errno=%d(%s)\n",
                 learnfilename,
                                 errno,
                                 errno_descr(errno)
                                 );
          if (engine_exit_base != 0)
            {
              exit (engine_exit_base + 21);
            }
          else
          {
            exit (EXIT_FAILURE);
          }
          }
      //       do we have a user-specified file size?
      if (sparse_spectrum_file_length == 0 ) {
        sparse_spectrum_file_length =
          DEFAULT_WINNOW_SPARSE_SPECTRUM_FILE_LENGTH;
      }

          if (f)
          {
      //       put in sparse_spectrum_file_length entries of NULL
      for (j = 0;
           j < sparse_spectrum_file_length
             * sizeof ( WINNOW_FEATUREBUCKET_STRUCT);
           j++)
        fputc ('\000', f);
      made_new_file = 1;
      //
      fclose (f);
          }

      //    and reset the statbuf to be correct
      k = stat (learnfilename, &statbuf);
          CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
    }
  //
  hfsize = statbuf.st_size;
  if (user_trace)
    fprintf (stderr, "Sparse spectra file %s has length %ld bins\n",
             learnfilename, hfsize / sizeof (WINNOW_FEATUREBUCKET_STRUCT));

  //
  //         open the .cow hash file into memory so we can bitwhack it
  //
  hashes = (WINNOW_FEATUREBUCKET_STRUCT *)
    crm_mmap_file (
                   learnfilename,
                   0, hfsize,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   NULL);
  if (hashes == MAP_FAILED)
    {
      fev = fatalerror ("Couldn't memory-map the .cow file named: ",
                        learnfilename);
      free(learnfilename);
      return (fev);
    }

  //          if this is a new file, set the proper version number.
  if (made_new_file)
    {
      hashes[0].hash  = 1;
      hashes[0].key   = 0;
      hashes[0].value = 1;
    }

  //        check the version of the file
  //
#ifdef CSS_VERSION_CHECK
  if (hashes[0].hash != 1 ||
      hashes[0].key  != 0 )
    {
      fprintf (stderr, "Hash was: %ld, key was %ld\n", hashes[0].hash, hashes[0].key);
      fev =fatalerror ("The .cow file is the wrong type!  We're expecting "
                       "a Osb_Winnow-spectrum file.  The filename is: ",
                       learnfilename);
      free(learnfilename);
      return (fev);
    }
#endif

  //
  //         In this format, bucket 0.value contains the start of the spectra.
  //
  spectra_start = 1;
  hashes[0].value = 1;

  //
  //   now set the hfsize to the number of entries, not the number
  //   of bytes total
  hfsize = hfsize / sizeof ( WINNOW_FEATUREBUCKET_STRUCT );


  //    and allocate the mask-off flags for this file
  //    so we only use each feature at most once
  //
  xhashes = calloc ( hfsize, sizeof(xhashes[0]));
  if ( !xhashes )
  {
    untrappableerror(
                     "Couldn't malloc xhashes\n",
                     "We need that part.  Sorry.\n");
  }

  //   compile the word regex
  //
  if ( internal_trace)
  {
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);
  }

  i = crm_regcomp (&regcb, ptext, plen, cflags);
  if ( i > 0)
    {
      crm_regerror ( i, &regcb, tempbuf, data_window_size);
      nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
      goto regcomp_failed;
    }


  //   Start by priming the pipe... we will shift to the left next.
  //     sliding, hashing, xoring, moduloing, and incrmenting the
  //     hashes till there are no more.
  k = 0;
  j = 0;
  i = 0;

#ifdef OLD_STUPID_VAR_RESTRICTION
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
      q = fatalerror (" Attempt to LEARN from a nonexistent variable ",
                  ltext);
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
      q = fatalerror (" Bogus text block containing variable ", ltext);
      free(learnfilename);
      return (q);
    }
  textoffset = vht[vhtindex]->vstart;
  textmaxoffset = textoffset + vht[vhtindex]->vlen;
#endif

  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;


  //   init the hashpipe with 0xDEADBEEF
  for (h = 0; h < OSB_WINNOW_WINDOW_LEN; h++)
    {
      hashpipe[h] = 0xDEADBEEF;
    }

  //    and the big loop...
  i = 0;
  while (k == 0 && textoffset <= textmaxoffset)
    {
      long wlen;
      long slen;
      //      unsigned char *ptok = &(mdw->filetext[textoffset]);
      //unsigned char *ptok_max = &(mdw->filetext[textmaxoffset]);

      //  do the regex
      //  slen = endpoint (= start + len)
      //        - startpoint (= curr textoffset)
      //      slen = txtlen ;
      slen = textmaxoffset - textoffset;

      // if pattern is empty, extract non graph delimited tokens
      // directly ([[graph]]+) instead of calling regexec
      if (ptext[0] != '\0')
        {
          k = crm_regexec (&regcb, &(txtptr[textoffset]),
                           slen, 5, match, 0, NULL);
        }
      else
        {
          k = 0;
          //         skip non-graphical characthers
          match[0].rm_so = 0;
          while (!crm_isgraph (txtptr[textoffset + match[0].rm_so])
                 && textoffset + match[0].rm_so < textmaxoffset)
            match[0].rm_so ++;
          match[0].rm_eo = match[0].rm_so;
          while (crm_isgraph (txtptr [textoffset + match[0].rm_eo])
                 && textoffset + match[0].rm_eo < textmaxoffset)
            match[0].rm_eo ++;
          if ( match[0].rm_so == match[0].rm_eo)
            k = 1;
        }

      if (k != 0 || textoffset > textmaxoffset)
        goto learn_end_regex_loop;


        wlen = match[0].rm_eo - match[0].rm_so;
        memmove (tempbuf,
                 &(txtptr[textoffset + match[0].rm_so]),
                 wlen);
        tempbuf[wlen] = '\000';

        if (internal_trace)
          {
            fprintf (stderr,
                     "  Learn #%ld t.o. %ld strt %ld end %ld len %ld is -%s-\n",
                     i,
                     textoffset,
                     (long) match[0].rm_so,
                     (long) match[0].rm_eo,
                     wlen,
                     tempbuf);
          }
        if (match[0].rm_eo == 0)
          {
            nonfatalerror ( "The LEARN pattern matched zero length! ",
                            "\n Forcing an increment to avoid an infinite loop.");
            match[0].rm_eo = 1;
          }


        //      Shift the hash pipe down one
        //
        for (h = OSB_WINNOW_WINDOW_LEN-1; h > 0; h--)
          {
            hashpipe [h] = hashpipe [h-1];
          }


        //  and put new hash into pipeline
        hashpipe[0] = strnhash (tempbuf, wlen);

        if (internal_trace)
          {
            fprintf (stderr, "  Hashpipe contents: ");
            for (h = 0; h < OSB_WINNOW_WINDOW_LEN; h++)
              fprintf (stderr, " %ld", hashpipe[h]);
            fprintf (stderr, "\n");
          }


        //  and account for the text used up.
        textoffset = textoffset + match[0].rm_eo;
        i++;

        //        is the pipe full enough to do the hashing?
        if (1)   //  we always run the hashpipe now, even if it's
                 //  just full of 0xDEADBEEF.  (was i >=5)
          {
            unsigned long hindex;
            unsigned long h1, h2;
            long th = 0;         // a counter used for TSS tokenizing
            unsigned long incrs;
            long j;
            //
            //
            th = 0;
            //
            //     Note that we start at j==1 here, so that we do NOT
            //     ever calculate (or save) the unigrams.
            //
            for (j = 1;
                 j < OSB_WINNOW_WINDOW_LEN;
                 j++)
              {
                if (use_unigrams)
                  {
                    h1 = hashpipe[0]*hctable[0];
                    if (h1 < spectra_start)
                      h1 = spectra_start;
                    h2 = hashpipe[0]*hctable[1];
                    if (h2 == 0) h2 = 0xdeadbeef;
                    j = OSB_WINNOW_WINDOW_LEN;
                  }
                else
                  {
                    h1 = hashpipe[0]*hctable[0] + hashpipe[j] * hctable[j<<1];
                    if (h1 < spectra_start)
                      h1 = spectra_start;
                    h2 = hashpipe[0]*hctable[1] + hashpipe[j] * hctable[(j<<1)-1];
                    if (h2 == 0) h2 = 0xdeadbeef;
                  }
                hindex = h1 % hfsize;
                if (hindex < spectra_start ) hindex = spectra_start;

                if (internal_trace)
                {
                  fprintf (stderr, "Polynomial %ld has h1:%ld  h2: %ld\n",
                           j, h1, h2);
                }

                //
                //   we now look at both the primary (h1) and
                //   crosscut (h2) indexes to see if we've got
                //   the right bucket or if we need to look further
                //
                incrs = 0;
                //   while ( hashes[hindex].key != 0
                //      &&  ( hashes[hindex].hash != h1
                //            || hashes[hindex].key  != h2 ))
                while((!((hashes[hindex].hash==h1)&&(hashes[hindex].key==h2)))
                      //   Unnecessary - if it doesn't match, and value != 0...
                      //  && (hashes[hindex].key != 0)
                      && (hashes[hindex].value != 0))
                  {
                    //
                    //
                    //       If microgrooming is enabled, and we've found a
                    //       chain that's too long, we groom it down.
                    //
                    if (microgroom && (incrs > MICROGROOM_CHAIN_LENGTH))
                      {
                        //     set the random number generator up...
                        //     note that this is repeatable for a
                        //     particular test set, yet dynamic.  That
                        //     way, we don't always autogroom away the
                        //     same feature; we depend on the previous
                        //     feature's key.
                        srand ( (unsigned int) h2);
                        //
                        //   and do the groom.

                        //   reset our hindex to where we started...
                        //
                        hindex = h1 % hfsize;
                        if (hindex < spectra_start ) hindex = spectra_start;

                        //    and microgroom.
                        //fprintf (stderr,  "\nCalling microgroom hindex %ld hash: %ld  key: %ld  value: %f ",
                        //      hindex, hashes[hindex].hash, hashes[hindex].key, hashes[hindex].value );

                        crm_winnow_microgroom(hashes, xhashes, hfsize, hindex);
                        incrs = 0;
                      }
                    //      check to see if we've incremented ourself all the
                    //      way around the .cow file.  If so, we're full, and
                    //      can hold no more features (this is unrecoverable)
                    if (incrs > hfsize - 3)
                      {
                        nonfatalerror ("Your program is stuffing too many "
                                       "features into this size .cow file.  "
                                       "Adding any more features is "
                                       "impossible in this file.",
                                       "You are advised to build a larger "
                                       ".cow file and merge your data into "
                                       "it.");
                        goto learn_end_regex_loop;
                      }
                    //
                    //     FINALLY!!!
                    //
                    //    This isn't the hash bucket we're looking for.  Move
                    //    along, move along....
                    incrs++;
                    hindex++;
                    if (hindex >= hfsize) hindex = spectra_start;
                  }

                if (internal_trace)
                  {
                    if (hashes[hindex].value == 0)
                      {
                        fprintf (stderr,"New feature at %ld\n", hindex);
                      }
                    else
                      {
                        fprintf (stderr, "Old feature at %ld\n", hindex);
                      }
                  }

                //      With _winnow_, we just multiply by the sense factor.
                //
                if (xhashes[hindex] == 0)
                  {
                    hashes[hindex].hash = h1;
                    hashes[hindex].key  = h2;
                    xhashes[hindex] = 1;
                    if (hashes[hindex].value > 0.0)
                      {
                          hashes[hindex].value = hashes[hindex].value * sense;
                      }
                    else
                      {
                        hashes[hindex].value = sense;
                      }
                  }

                //              fprintf (stderr, "Hash index: %ld  value: %f \n", hindex, hashes[hindex].value);
              }
          }
    }
  //   end the while k==0

 learn_end_regex_loop:
 regcomp_failed:

  //  and remember to let go of the mmap and the pattern bufffer
  // (and force a cache purge)
  // crm_munmap_all ();
  crm_munmap_file ((void *) hashes);

  free (xhashes);

#if 0  /* now touch-fixed inside the munmap call already! */
#if defined(HAVE_MMAP) || defined(HAVE_MUNMAP)
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .css ile, to force
  //    the filesystem to repropagate it's caches.
  //
  crm_touch(learnfilename);
#endif
#endif

  if (ptext[0] != '\0') crm_regfree (&regcb);

  free(learnfilename);
  return (0);
}



//      How to do a Osb_Winnow CLASSIFY some text.
//
int crm_expr_osb_winnow_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                                  char *txtptr, long txtstart, long txtlen)
{
  //      classify the sparse spectrum of this input window
  //      as belonging to a particular type.
  //
  //       This code should look very familiar- it's cribbed from
  //       the code for LEARN
  //
  long i, j, k;
  long h;          //  we use h for our hashpipe counter, as needed.
  char ptext[MAX_PATTERN];  //  the regex pattern
  long plen;
  //  the hash file names
  char htext[MAX_PATTERN+MAX_CLASSIFIERS*MAX_FILE_NAME_LEN];
  long htext_maxlen = MAX_PATTERN+MAX_CLASSIFIERS*MAX_FILE_NAME_LEN;
  long hlen;
  //  the match statistics variable inbuf
  char stext[MAX_PATTERN+MAX_CLASSIFIERS*(MAX_FILE_NAME_LEN+100)];
  long stext_maxlen = MAX_PATTERN+MAX_CLASSIFIERS*(MAX_FILE_NAME_LEN+100);
  long slen;
  char svrbl[MAX_PATTERN];  //  the match statistics text buffer
  long svlen;
  long fnameoffset;
  char fname[MAX_FILE_NAME_LEN];
  long eflags;
  long cflags;
  long not_microgroom = 1;
  long use_unigrams;

  struct stat statbuf;      //  for statting the hash file
  unsigned long hashpipe[OSB_WINNOW_WINDOW_LEN+1];
  regex_t regcb;
  regmatch_t match[5];      //  we only care about the outermost match

  double fcounts [MAX_CLASSIFIERS]; // total counts for feature normalize
  hitcount_t totalcount = 0;

  double cpcorr[MAX_CLASSIFIERS];  // corpus correction factors
  hitcount_t hits[MAX_CLASSIFIERS];  // actual hits per feature per classifier
  hitcount_t totalhits[MAX_CLASSIFIERS];  // actual total hits per classifier
  double totalweights[MAX_CLASSIFIERS];  //  total of hits * weights
  double unseens[MAX_CLASSIFIERS]; //  total unseen features.
  double classifierprs[MAX_CLASSIFIERS]; //  pR's of each class
  long totalfeatures;   //  total features
  hitcount_t htf;             // hits this feature got.
  double tprob = 0;         //  total probability in the "success" domain.

  //double textlen;    //  text length  - rougly corresponds to
                        //  information content of the text to classify

  WINNOW_FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
  unsigned char *xhashes[MAX_CLASSIFIERS];
  long hashlens[MAX_CLASSIFIERS];
  char *hashname[MAX_CLASSIFIERS];
  long succhash;
  long vbar_seen;       // did we see '|' in classify's args?
  long maxhash;
  long fnstart, fnlen;
  long fn_start_here;
  long textoffset;
  long textmaxoffset;
  long bestseen;
  long thistotal;

  double top10scores[10];
  long top10polys[10];
  char top10texts[10][MAX_PATTERN];


  if (internal_trace)
    fprintf (stderr, "executing an OSB-WINNOW CLASSIFY\n");

  //
  //      We get the to-be-classified text from the caller now.
  //
  //  crm_get_pgm_arg (ltext, MAX_PATTERN, apb->b1start, apb->b1len);
  // llen = apb->b1len;
  // llen = crm_nexpandvar (ltext, llen, MAX_PATTERN);

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

  not_microgroom = 1;
  if (apb->sflags & CRM_MICROGROOM)
    {
      not_microgroom = 0;
      if (user_trace)
        fprintf (stderr, " disabling fast-skip optimization.\n");
    }

  use_unigrams = 0;
  if (apb->sflags & CRM_UNIGRAM)
    {
      use_unigrams = 1;
      if (user_trace)
        fprintf (stderr, " enabling unigram-only operation.\n");
    }

  //   compile the word regex
  if ( internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);
  i = crm_regcomp (&regcb, ptext, plen, cflags);
  if ( i > 0)
    {
      crm_regerror ( i, &regcb, tempbuf, data_window_size);
      nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
      goto regcomp_failed;
    }



  //       Now, the loop to open the files.
  bestseen = 0;
  thistotal = 0;
  //  goodcount = evilcount = 1;   // prevents a divide-by-zero error.
  //cpgood = cpevil = 0.0;
  //ghits = ehits = 0.0 ;
  //psucc = 0.5;
  //pfail = (1.0 - psucc);
  //pic = 0.5;
  //pnic = 0.5;


  //      initialize our arrays for N .css files
  for (i = 0; i < MAX_CLASSIFIERS; i++)
    {
      fcounts[i] = 0.0;    // check later to prevent a divide-by-zero
                         // error on empty .css file
      cpcorr[i] = 0.0;   // corpus correction factors
      hits[i] = 0;     // absolute hit counts
      totalhits[i] = 0;        // absolute hit counts
      totalweights[i] = 0.0;     // hit_i * weight*i count
      unseens[i] = 0.0;       // text features not seen in statistics files
    }

  for (i = 0; i < 10; i++)
    {
      top10scores[i] = 0;
      top10polys[i] = 0;
      strcpy (top10texts[i], "");
    }
  //
  //     --  The Winnow evaluator --
  //
  //    Winnow is NOT a bayesian evaluator.  Instead, it generates
  //    a set of positive-only weights for each feature.  If a
  //    feature is present, it's weight is added to the total.
  //    The feature file with the greater total wins.  Simple, eh?
  //
  //    Initial weights (set when a feature is first seen in learning)
  //    is 1.0.  Whenever a feature is "learned" as true, it's weight
  //    is multiplied by the OSB_PROMOTION factor.  When it's learned
  //    as incorrect, it's multiplied by the OSB_DEMOTION factor.
  //
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
    fprintf (stderr, "Classify list: -%s- \n", htext);
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
            fprintf (stderr, "Classifying with file -%s- "\
                             "succhash=%ld, maxhash=%ld\n",
                             fname, succhash, maxhash);
          if ( fname[0] == '|' && fname[1] == '\000')
            {
              if (vbar_seen)
                {
                  nonfatalerror ("Only one ' | ' allowed in a CLASSIFY. \n" ,
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
                  //  file exists - do the open/process/close
                  //
                  hashlens[maxhash] = statbuf.st_size;
                  //  mmap the hash file into memory so we can bitwhack it
                  hashes[maxhash] = (WINNOW_FEATUREBUCKET_STRUCT *)
                    crm_mmap_file ( fname,
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
#ifdef CSS_VERSION_CHECK
                      //
                      //     Check to see if this file is the right version
                      //
                      long fev;
                      if (hashes[maxhash][0].hash != 1 ||
                          hashes[maxhash][0].key  != 0)
                        {
                          fev = fatalerror ("The .css file is the wrong type!  We're expecting "
                                           "a Osb_Winnow-spectrum file.  The filename is: ",
                                           &htext[i]);
                          return (fev);
                        }
#endif
                      //     grab the start of the actual spectrum data.
                      //
                      spectra_start = hashes[maxhash][0].value;


                      //  set this hashlens to the length in features instead
                      //  of the length in bytes.
                      hashlens[maxhash] = hashlens[maxhash] / sizeof (WINNOW_FEATUREBUCKET_STRUCT);
                      hashname[maxhash] = (char *) calloc((fnlen+10) , sizeof(hashname[maxhash][0]));
                      if (!hashname[maxhash])
                        untrappableerror(
                                         "Couldn't malloc hashname[maxhash]\n","We need that part later, so we're stuck.  Sorry.");
                      strncpy(hashname[maxhash],fname,fnlen);
                      hashname[maxhash][fnlen]='\000';

                      //    and allocate the mask-off flags for this file
                      //    so we only use each feature at most once
                      //
                      xhashes[maxhash] = calloc (hashlens[maxhash],
                                                 sizeof (xhashes[maxhash][0]));
                      if (!xhashes[maxhash])
                        untrappableerror(
                                         "Couldn't malloc xhashes[maxhash]\n",
                                         "We need that part.  Sorry.\n");

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

  if (user_trace)
    fprintf (stderr, "Running with %ld files for success out of %ld files\n",
             succhash, maxhash );

  // sanity checks...  Uncomment for super-strict CLASSIFY.
  //
  //    do we have at least 1 valid .css files?
  if (maxhash == 0)
    {
      fatalerror ("Couldn't open at least 1 .cow files for classify().", "");
    }
  //    do we have at least 1 valid .cow file at both sides of '|'?
  //if (!vbar_seen || succhash < 0 || (maxhash < succhash + 2))
  //  {
  //    nonfatalerror (
  //      "Couldn't open at least 1 .css file per SUCC | FAIL classes "
  //    " for classify().\n","Hope you know what are you doing.");
  //  }

  {
    long ifile;
    long k;
    //      count up the total first
    for (ifile = 0; ifile < maxhash; ifile++)
      {
        fcounts[ifile] = 0.0 ;
        for (k = 1; k < hashlens[ifile]; k++)
          fcounts [ifile] = fcounts[ifile] + hashes[ifile][k].value;
        if (fcounts[ifile] <= 0.0) fcounts[ifile] = 1.0 ;
        totalcount = totalcount + fcounts[ifile];
      }
    //
    //     calculate cpcorr (count compensation correction)
    //

    for (ifile = 0; ifile < maxhash; ifile++)
      {
        //  cpcorr [ifile] = ( totalcount / (fcounts[ifile] * (maxhash-1)));
        //
        //   disable cpcorr for now... unclear that it's useful.
        cpcorr[ifile] = 1.0;
      }
  }

  //
  //   now all of the files are mmapped into memory,
  //   and we can do the polynomials and add up points.
  i = 0;
  j = 0;
  k = 0;
  thistotal = 0;

#ifdef OLD_STUPID_VAR_RESTRICTION
  if (llen > 0)
    {
      vhtindex = crm_vht_lookup (vht, ltext, llen );
    }
  else
    {
      vhtindex = crm_vht_lookup (vht, ":_dw:", 5);
    }
  if (vht[vhtindex] == NULL)
    {
      return (fatalerror (" Attempt to CLASSIFY from a nonexistent variable ",
                          ltext));
    }
  mdw = NULL;
  if (tdw->filetext == vht[vhtindex]->valtxt)
    mdw = tdw;
  if (cdw->filetext == vht[vhtindex]->valtxt)
    mdw = cdw;
  if (mdw == NULL)
    return ( fatalerror (" Bogus text block containing variable ", ltext));
  textoffset = vht[vhtindex]->vstart;
  textmaxoffset = textoffset + vht[vhtindex]->vlen;

  textlen = (vht[vhtindex]->vlen);
  if (textlen < 1.0) textlen = 1.0;
#endif
  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;


  //   init the hashpipe with 0xDEADBEEF
  for (h = 0; h < OSB_WINNOW_WINDOW_LEN; h++)
    {
      hashpipe[h] = 0xDEADBEEF;
    }

  totalfeatures = 0;

  //  stop when we no longer get any regex matches
  //   possible edge effect here- last character must be matchable, yet
  //    it's also the "end of buffer".
  while (k == 0 && textoffset <= textmaxoffset)
    {
      long wlen;
      long slen;
      //      unsigned char *ptok = &(mdw->filetext[textoffset]);
      //  unsigned char *ptok_max = &(mdw->filetext[textmaxoffset]);

      //  do the regex
      //      slen = txtlen - textoffset;
      slen = textmaxoffset - textoffset;

      // if pattern is empty, extract non graph delimited tokens
      // directly ([[graph]]+) instead of calling regexec
      if (ptext[0] != '\0')
        {
          k = crm_regexec (&regcb, &(txtptr[textoffset]),
                           slen, 5, match, 0, NULL);
        }
      else
        {
          k = 0;
          //         skip non-graphical characthers
          match[0].rm_so = 0;
          while (!crm_isgraph (txtptr[textoffset + match[0].rm_so])
                 && textoffset + match[0].rm_so < textmaxoffset)
            match[0].rm_so ++;
          match[0].rm_eo = match[0].rm_so;
          while (crm_isgraph (txtptr [textoffset + match[0].rm_eo])
                 && textoffset + match[0].rm_eo < textmaxoffset)
            match[0].rm_eo ++;
          if ( match[0].rm_so == match[0].rm_eo)
            k = 1;
        }

      if (k != 0 || textoffset > textmaxoffset)
        goto classify_end_regex_loop;

      wlen = match[0].rm_eo - match[0].rm_so;
      memmove (tempbuf,
               &(txtptr[textoffset + match[0].rm_so]),
               wlen);
      tempbuf[wlen] = '\000';

      if (internal_trace)
        {
          fprintf (stderr,
                   "  Classify #%ld t.o. %ld strt %ld end %ld len %ld is -%s-\n",
                   i,
                   textoffset,
                   (long) match[0].rm_so,
                   (long) match[0].rm_eo,
                   wlen,
                   tempbuf);

        }
      if (match[0].rm_eo == 0)
        {
          nonfatalerror ( "The CLASSIFY pattern matched zero length! ",
                          "\n Forcing an increment to avoid an infinite loop.");
          match[0].rm_eo = 1;
        }
      //  slide previous hashes up 1
      for (h = OSB_WINNOW_WINDOW_LEN-1; h >= 1; h--)
        {
          hashpipe [h] = hashpipe [h-1];
        }


      //  and put new hash into pipeline
      hashpipe[0] = strnhash ( tempbuf, wlen);

      if (0)
          {
            fprintf (stderr, "  Hashpipe contents: ");
            for (h = 0; h < OSB_WINNOW_WINDOW_LEN; h++)
              fprintf (stderr, " %ld", hashpipe[h]);
            fprintf (stderr, "\n");
          }

      //   account for the text we used up...
      textoffset = textoffset + match[0].rm_eo;
      i++;

      //        is the pipe full enough to do the hashing?
      if (1)   //  we init with 0xDEADBEEF, so the pipe is always full (i >=5)
        {
          int j, k;
          unsigned th=0;          //  a counter used only in TSS hashing
          unsigned long hindex;
          unsigned long h1, h2;
          //unsigned long good, evil;
          //
          //
          th = 0;

          //
          //     Note that we start at j==1 here, so that we do NOT
          //     ever calculate (or save) the unigrams.
          //
          for (j = 1;
               j < OSB_WINNOW_WINDOW_LEN;
               j++)
            {
              if (use_unigrams)
                {
                  h1 = hashpipe[0]*hctable[0];
                  if (h1 < spectra_start)
                    h1 = spectra_start;
                  h2 = hashpipe[0]*hctable[1];
                  if (h2 == 0) h2 = 0xdeadbeef;
                  j = OSB_WINNOW_WINDOW_LEN;
                }
              else
                {
                  h1 = hashpipe[0]*hctable[0] + hashpipe[j] * hctable[j<<1];
                  if (h1 < spectra_start)
                    h1 = spectra_start;
                  h2 = hashpipe[0]*hctable[1] + hashpipe[j] * hctable[(j<<1)-1];
                  if (h2 == 0) h2 = 0xdeadbeef;
                }

              hindex = h1;
              if (internal_trace)
                fprintf (stderr, "Polynomial %d has h1:%ld  h2: %ld\n",
                         j, h1, h2);

              //    Now, for each of the feature files, what are
              //    the statistics (found, not found, whatever)
              //
              htf = 0;
              totalfeatures++;
              for (k = 0; k < maxhash; k++)
                {
                  long lh, lh0;
                  float z;
                  lh = hindex % (hashlens[k]);
                  if (lh < spectra_start ) lh = spectra_start;
                  lh0 = lh;
                  hits[k] = 0;
                  while ( hashes[k][lh].key != 0
                          && ( hashes[k][lh].hash != h1
                               || hashes[k][lh].key  != h2 ))
                    {
                      lh++;
                      if (lh >= hashlens[k]) lh = spectra_start;
                      if (lh == lh0) break; // wraparound
                    }

                  //   Did we find the feature?  Or did we hit end-of-chain?
                  //
                  if (hashes[k][lh].hash == h1 && hashes[k][lh].key == h2)
                    {
                      //    found the feature
                      //
                      //    Have we seen it before?
                      if (xhashes[k][lh] == 0)
                        {
                          // remember totalhits
                          htf = htf + 1;            // and hits-this-feature
                          hits[k] ++;               // increment hits.
                          z = hashes[k][lh].value;
                          //                  fprintf (stdout, "L: %f  ", z);
                          // and weight sum
                          totalweights[k] = totalweights[k] + z;
                          totalhits[k] = totalhits[k] + 1;
                          //
                          //  and mark the feature as seen.
                          xhashes[k][lh] = 1;
                        }
                    }
                  else
                    {
                      // unseens score 1.0, which is totally ambivalent; seen
                      //  and accepted score more, seen and refuted score less
                      //
                      unseens[k] = unseens[k] + 1.0 ;
                      totalweights[k] = totalweights[k] + 1.0 ;
                    }
                }

              if (internal_trace)
                {
                  for (k = 0; k < maxhash; k++)
                    {
                      // fprintf (stderr, "ZZZ\n");
                      fprintf (stderr,
                       " poly: %d  filenum: %d, HTF: %7ld, hits: %7ld, th: %10ld, tw: %6.4e\n",
                              j, k, (long)htf, (long)hits[k], (long)totalhits[k], totalweights[k]);
                    }
                }
              //
              //    avoid the fencepost error for window=1
              if ( OSB_WINNOW_WINDOW_LEN == 1)
                {
                  j = 99999;
                }
            }
        }
    }      //  end of repeat-the-regex loop
 classify_end_regex_loop:

  //  cleanup time!
  //  remember to let go of the fd's and mmaps and mallocs
  for (k = 0; k < maxhash; k++)
    {
      crm_munmap_file ( (void *) hashes[k]);
      free (xhashes[k]);
    }
  //  and let go of the regex buffery
  crm_regfree (&regcb);

  if (user_trace)
    {
      for (k = 0; k < maxhash; k++)
        fprintf (stderr, "Match for file %ld:  hits: %ld  weight: %f\n",
                 k, (long)totalhits[k], totalweights[k]);
    }
  //
  //      Do the calculations and format some output, which we may or may
  //      not use... but we need the calculated result anyway.
  //
  if (1)
    {
      char buf[1024];
      double accumulator;
      double remainder;
      double overall_pR;
      long m;
      buf [0] = '\000';
      accumulator = 10 * DBL_MIN;

      for (m = 0; m < maxhash; m++)
        {
          if (totalweights[m] < 1)
            totalweights[m] = 1;
          if (totalhits[m] < 1)
            totalhits[m] = 1;
          classifierprs[m] = 10.0*(log10 (totalweights[m])-log10(totalhits[m]));
        }
      for (m = 0; m < succhash; m++)
        {
          accumulator = accumulator + totalweights[m];
        }
      remainder = 10 * DBL_MIN;
      for (m = succhash; m < maxhash; m++)
        {
          remainder = remainder + totalweights[m];
        }

      tprob = (accumulator) / (accumulator + remainder);

      //     *******************************************
      //
      //        Note - we use 10 as the normalization for pR here.
      //        it's because we don't have an actual probability
      //        but we want this to scale similarly with the other
      //        recognizers.
      //
      overall_pR = 10 * (log10 (accumulator) - log10 (remainder));

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

      //   find best single matching file
      //
      bestseen = 0;
      for (k = 0; k < maxhash; k++)
        if (classifierprs[k] > classifierprs[bestseen] ) bestseen = k;

      remainder = 10 * DBL_MIN;
      for (m = 0; m < maxhash; m++)
        if (bestseen != m)
          {
            remainder = remainder + totalweights[m];
          }

      //   ... and format some output of best single matching file
      //
      snprintf (buf, WIDTHOF(buf), "Best match to file #%ld (%s) "
                    "weight: %6.4f  pR: %6.4f\n",
               bestseen,
               hashname[bestseen],
               totalweights[bestseen],
               classifierprs[bestseen]);
          buf[WIDTHOF(buf) - 1] = 0;
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
        strcat (stext, buf);
      sprintf (buf, "Total features in input file: %ld\n", totalfeatures);
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
        strcat (stext, buf);

      //     Now do the per-file breakdowns:
      //
      for (k = 0; k < maxhash; k++)
        {
          long m;
          remainder = 10 * DBL_MIN;
          for (m = 0; m < maxhash; m++)
              if (k != m)
                {
                  remainder = remainder + totalweights[m];
                }
          snprintf (buf, WIDTHOF(buf),
                   "#%ld (%s):"\
                   " features: %.2f, unseen: %3.2e, weight: %3.2e, pR: %6.2f \n",
                   k,
                   hashname[k],
                   fcounts[k],
                   unseens[k],
                   totalweights[k],
                   classifierprs[k]);
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
      if (svlen > 0)
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
 regcomp_failed:
  return (0);
}
