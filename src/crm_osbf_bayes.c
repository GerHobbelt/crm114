//	crm_osbf_bayes.c - OSBF Bayes classifier

// Copyright 2004 Fidelis Assis
// Copyright 2004-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  This is the OSBF-Bayes classifier. It differs from SBPH-Markovian
//  and OSB-Bayes in the way P(F|C) is estimated.  See function
//  crm_expr_osbf_bayes_classify, below, for details.
//  -- Fidelis Assis - 2004/10/20

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  include the routine declarations file
#include "crm114.h"

//  include OSBF structures
#include "crm114_osbf.h"

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
extern char *tempbuf;

////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static long hctable[] = { 1, 7,
  3, 13,
  5, 29,
  11, 51,
  23, 101,
  47, 203,
  97, 407,
  197, 817,
  397, 1637,
  797, 3277
};

//          Where does the nominative data start?
static unsigned long spectra_start;

/* structure for token searching */
struct token_search
{
  unsigned char *ptok;
  unsigned long toklen;
  unsigned long hash;
  unsigned char *max_ptok;
  const char *pattern;
  regex_t *regcb;
  unsigned max_long_tokens;
};

/************************************************************/

static int
get_next_token (struct token_search *pts)
{
  unsigned char *p_end = NULL;	/* points to end of the token */
  int error = 0;		/* default: no error */

  if (pts->pattern[0] != '\0')
    {
      regmatch_t match[5];

      if (pts->ptok < pts->max_ptok)
	{
	  error = crm_regexec (pts->regcb, (char *) pts->ptok,
			       pts->max_ptok - pts->ptok, 5, match, 0, NULL);
	  if (error == REG_NOMATCH)
	    {
	      match[0].rm_so = 0;
	      match[0].rm_eo = 0;
	      error = 0;
	    }
           /* fprintf(stderr, "%s %ld %ld\n", pts->pattern, match[0].rm_so, match[0].rm_eo); */
	}
      else
	{
	  match[0].rm_so = 0;
	  match[0].rm_eo = 0;
	}

      if (error == 0)
	{
	  p_end = pts->ptok + match[0].rm_eo;
	  pts->ptok += match[0].rm_so;
	}
    }
  else
    {
      /* find nongraph delimited token */
      p_end = pts->ptok;
      while ((pts->ptok < pts->max_ptok) && !isgraph ((int) *pts->ptok))
	pts->ptok++;
      p_end = pts->ptok;
      while ((p_end < pts->max_ptok) && isgraph ((int) *p_end))
	p_end++;
    }

  if (error == 0)
    {
      /* update token length */
      pts->toklen = p_end - pts->ptok;
    }

  /* return error status */


  /*
  {
    unsigned long i = 0;
    while (error == 0 && i < pts->toklen)
      fputc (pts->ptok[i++], stderr);
    fprintf (stderr, " %lu", pts->toklen);
  }
  */

  return error;
}

/*****************************************************************/

static unsigned long
get_next_hash (struct token_search *pts)
{
  unsigned long hash_acc = 0;
  unsigned long count_long_tokens = 0;
  int error;

  /* get next token */
  error = get_next_token (pts);

  /* long tokens, probably base64 lines */
  while (error == 0 && pts->toklen > OSBF_MAX_TOKEN_SIZE &&
	 count_long_tokens < pts->max_long_tokens)
    {
      count_long_tokens++;
      /* XOR new hash with previous one */
      hash_acc ^= strnhash ((char *) pts->ptok, pts->toklen);
      /* fprintf (stderr, " %0lX +\n ", hash_acc);  */
      /* advance the pointer and get next token */
      pts->ptok += pts->toklen;
      error = get_next_token (pts);
    }

  if (error == 0)
    {
      if (pts->toklen > 0 || count_long_tokens > 0)
	{
	  hash_acc ^= strnhash ((char *) pts->ptok, pts->toklen);
	  /* fprintf (stderr, " %0lX %lu\n", hash_acc, pts->toklen); */
	  pts->hash = hash_acc;
	}
      else
	{
	  /* no more hashes */
	  /* fprintf (stderr, "End of text %0lX %lu\n", hash_acc, pts->toklen); */
	  error = 1;
	}
    }

  return error;
}

/*****************************************************************/

//    How to learn Osb_Bayes style  - in this case, we'll include the single
//    word terms that may not strictly be necessary.
//

int
crm_expr_osbf_bayes_learn (CSL_CELL * csl, ARGPARSE_BLOCK * apb,
			   char *txtptr, long txtstart, long txtlen)
{
  //     learn the osb_bayes transform spectrum of this input window as
  //     belonging to a particular type.
  //     learn <flags> (classname) /word/
  //
  long i, j, k;
  long h;			//  h is our counter in the hashpipe;
  char ptext[MAX_PATTERN];	//  the regex pattern
  long plen;
  char htext[MAX_PATTERN];	//  the hash name
  long hlen;
  long cflags, eflags;
  struct stat statbuf;		//  for statting the hash file
  OSBF_FEATUREBUCKET_STRUCT *hashes;	//  the text of the hash file
  OSBF_FEATURE_HEADER_STRUCT *header;	//  header of the hash file
  //char *seen_features;
  unsigned int hashpipe[OSB_BAYES_WINDOW_LEN + 1];

  regex_t regcb;
  long textoffset;
  long textmaxoffset;
  long sense;
  long fev;
  char *fname;
  struct token_search ts;

  /* fprintf(stderr, "Starting learning...\n"); */

  if (user_trace)
    fprintf (stderr, "OSBF Learn\n");
  if (internal_trace)
    fprintf (stderr, "executing a LEARN\n");

  //   Keep the gcc compiler from complaining about unused variables
  //  i = hctable[0];

  //           extract the hash file name
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, MAX_PATTERN);

  //     get the "this is a word" regex
  ptext[0] = '\0';		// start with empty regex
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
    };
  if (apb->sflags & CRM_REFUTE)
    {
      sense = -sense;
      if (user_trace)
	fprintf (stderr, " refuting learning\n");
    };
  if (apb->sflags & CRM_MICROGROOM)
    {
      // enable microgroom
      crm_osbf_set_microgroom(1);;
      // if not set by command line, use default
      if (microgroom_chain_length == 0)
	microgroom_chain_length = OSBF_MICROGROOM_CHAIN_LENGTH;
      // if not set by command line, use default
      if (microgroom_stop_after == 0)
	microgroom_stop_after = OSBF_MICROGROOM_STOP_AFTER;

      if (user_trace)
	fprintf (stderr, " enabling microgrooming.\n");
    }
  else
    {
      // disable microgroom
      crm_osbf_set_microgroom(0);
    }

  //
  //             grab the filename, and stat the file
  //      note that neither "stat", "fopen", nor "open" are
  //      fully 8-bit or wchar clean...
  i = 0;
  while (htext[i] < 0x021)
    i++;
  j = i;
  while (htext[j] >= 0x021)
    j++;

  //             filename starts at i,  ends at j. null terminate it.
  htext[j] = '\000';

  //             and stat it to get it's length
  k = stat (&htext[i], &statbuf);

  //             quick check- does the file even exist?
  if (k != 0)
    {
      if (crm_osbf_create_cssfile (&htext[i],
				   (sparse_spectrum_file_length
				    !=
				    0) ?
				   sparse_spectrum_file_length :
				   OSBF_DEFAULT_SPARSE_SPECTRUM_FILE_LENGTH,
				   OSBF_VERSION, 0,
				   OSBF_CSS_SPECTRA_START) != EXIT_SUCCESS)
	{
	  fprintf (stderr,
		   "\n Couldn't create file %s; errno=%d .\n",
		   &htext[i], errno);
	  exit (EXIT_FAILURE);
	}

      //    and reset the statbuf to be correct
      k = stat (&htext[i], &statbuf);
    };

  //
  //         open the hash file into memory so we can bitwhack it
  //
  fname = strdup (&htext[i]);
  header = crm_mmap_file (fname,
			   0, statbuf.st_size,
			  PROT_READ | PROT_WRITE, MAP_SHARED, NULL);

  if (header == MAP_FAILED)
    {
      fev =
	fatalerror ("Couldn't memory-map the .cfc file named: ", &htext[i]);
      return (fev);
    };

  //
  if (user_trace)
    fprintf (stderr,
	     "Sparse spectra file %s has length %ld bins\n",
	     &htext[i], header->buckets);

  hashes = (OSBF_FEATUREBUCKET_STRUCT *) header + header->buckets_start;

  //        check the version of the file
  //
  if (*((unsigned long *) header->version) != OSBF_VERSION
      || header->flags != 0)
    {
      fprintf (stderr, "Version was: %ld, flags was %ld\n",
	       *((unsigned long *) header->version), header->flags);
      fev =
	fatalerror
	("The .cfc file is the wrong type!  We're expecting "
	 "a OSBF_Bayes-spectrum file.  The filename is: ", &htext[i]);

      return (fev);
    };

  //
  //
  spectra_start = header->buckets_start;

  //   compile the word regex
  //
  if (internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);

  // compile regex if not empty - empty regex means "plain regex"
  if (ptext[0] != '\0')
    {
      i = crm_regcomp (&regcb, ptext, plen, cflags);
      if (i > 0)
	{
	  crm_regerror (i, &regcb, tempbuf, data_window_size);
	  nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
	  goto regcomp_failed;
	};
    }


  //   Start by priming the pipe... we will shift to the left next.
  //     sliding, hashing, xoring, moduloing, and incrmenting the
  //     hashes till there are no more.
  k = 0;
  j = 0;
  i = 0;

  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;

  //   init the hashpipe with 0xDEADBEEF
  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
    {
      hashpipe[h] = 0xDEADBEEF;
    };

  //    and the big loop...
  i = 0;

  // initialize the token search structure
  ts.ptok = (unsigned char *) &(txtptr[textoffset]);
  ts.max_ptok = (unsigned char *) &(txtptr[textmaxoffset]);
  ts.toklen = 0;
  ts.pattern = ptext;
  ts.regcb = &regcb;
  ts.max_long_tokens = OSBF_MAX_LONG_TOKENS;

  while (get_next_hash (&ts) == 0)
    {
      if (internal_trace)
	{
	  memmove (tempbuf, ts.ptok, ts.toklen);
	  tempbuf[ts.toklen] = '\000';
	  fprintf (stderr,
		   "  Learn #%ld t.o. %ld strt %ld end %ld len %lu is -%s-\n",
		   i,
		   textoffset,
		   ts.ptok - (unsigned char *) &(txtptr[textoffset]),
		   (ts.ptok + ts.toklen) -
		   (unsigned char *) &(txtptr[textoffset]),
		   ts.toklen, tempbuf);
	};

      //      Shift the hash pipe down one
      for (h = OSB_BAYES_WINDOW_LEN - 1; h > 0; h--)
	{
	  hashpipe[h] = hashpipe[h - 1];
	};

      //  and put new hash into pipeline
      hashpipe[0] = ts.hash;

      if (internal_trace)
	{
	  fprintf (stderr, "  Hashpipe contents: ");
	  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
	    fprintf (stderr, " %u", hashpipe[h]);
	  fprintf (stderr, "\n");
	};

      /* prepare for next token */
      ts.ptok += ts.toklen;
      textoffset += ts.ptok - (unsigned char *) &(txtptr[textoffset]);
      i++;

      {
	unsigned long hindex, bindex;
	unsigned long h1, h2;
	long th = 0;		// a counter used for TSS tokenizing
	long j;
	//
	//     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
	//     (coefficients chosen by requiring superincreasing,
	//     as well as prime)
	//
	th = 0;
	//
	for (j = 1; j < OSB_BAYES_WINDOW_LEN; j++)
	  {
	    h1 = hashpipe[0] * hctable[0] + hashpipe[j] * hctable[j << 1];
	    h2 =
	      hashpipe[0] * hctable[1] + hashpipe[j] * hctable[(j << 1) - 1];

	    hindex = h1 % header->buckets;

	    if (internal_trace)
	      fprintf (stderr,
		       "Polynomial %ld has h1:%ld  h2: %ld\n", j, h1, h2);

	    //
	    //   we now look at both the primary (h1) and
	    //   crosscut (h2) indexes to see if we've got
	    //   the right bucket or if we need to look further
	    //

	    bindex = crm_osbf_find_bucket (header, h1, h2);
	    if (VALID_BUCKET (header, bindex))
	      {
		if (!EMPTY_BUCKET (hashes[bindex]))
		  {
		    if (!BUCKET_IS_LOCKED (hashes[bindex]))
		      {
			crm_osbf_update_bucket (header, bindex, sense);
			if (internal_trace)
			  fprintf (stderr, "Updated feature at %ld\n",
				   hindex);
		      }
		  }
		else if (sense > 0)
		  {
		    crm_osbf_insert_bucket (header, bindex, h1, h2, sense);
		    if (internal_trace)
		      fprintf (stderr, "New feature at %ld\n", hindex);
		  }
	      }
	    else
		  {
		    nonfatalerror
		      ("Your program is stuffing too many "
		       "features into this size .cfc file.  "
		       "Adding any more features is "
		       "impossible in this file.",
		       "You are advised to build a larger "
		       ".cfc file and merge your data into " "it.");
		    goto learn_end_regex_loop;
		  }
	      }
      }
    }				//   end the while k==0

learn_end_regex_loop:

  // unlock features locked during learning
  for (i = 0; i < header->buckets; i++)
    UNLOCK_BUCKET (hashes[i]);

  // update the number of learnings
  if (sense > 0)
    {
      header->learnings += sense;
      if (header->learnings >= (OSBF_FEATUREBUCKET_VALUE_MAX - 1))
	{
	  header->learnings >>= 1;
	  for (i = 0; i < header->buckets; i++)
	    BUCKET_RAW_VALUE (hashes[i]) = BUCKET_RAW_VALUE (hashes[i]) >> 1;
	  nonfatalerror
	    ("You have managed to LEARN so many documents that"
	     " you have forced rescaling of the entire database.",
	     " If you are the first person to do this, Fidelis "
	     " owes you a bottle of good singlemalt scotch");
	}
    }
  else if (header->learnings >= (unsigned long) (-sense))
    {
      header->learnings += sense;
    }

regcomp_failed:

  //  and remember to let go of the mmaps and the pattern bufffer
  //     (because we may have written it, force a cache flush)
  //  crm_munmap_all ();
  crm_munmap_file ((void *) header);

#ifndef CRM_WINDOWS
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .cfc ile, to force
  //    the filesystem to repropagate it's caches.
  //
  {
    int hfd;			//  hashfile fd
    OSBF_FEATURE_HEADER_STRUCT foo;
    hfd = open (fname, O_RDWR);
    dontcare = read (hfd, &foo, sizeof (foo));
    lseek (hfd, 0, SEEK_SET);
    dontcare = write (hfd, &foo, sizeof (foo));
    close (hfd);
  }
#endif	// !CRM_WINDOWS

  if (ptext[0] != '\0')
    crm_regfree (&regcb);
  return (0);
}

//      How to do a Osb_Bayes CLASSIFY some text.
//
int
crm_expr_osbf_bayes_classify (CSL_CELL * csl, ARGPARSE_BLOCK * apb,
			      char *txtptr, long txtstart, long txtlen)
{
  //      classify the sparse spectrum of this input window
  //      as belonging to a particular type.
  //
  //       This code should look very familiar- it's cribbed from
  //       the code for LEARN
  //
  long i, j, k;
  long h;			//  we use h for our hashpipe counter, as needed.
  char ptext[MAX_PATTERN];	//  the regex pattern
  long plen;
  char ostext[MAX_PATTERN];	//  optional pR offset
  long oslen;
  double pR_offset;
  //  the hash file names
  char htext[MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN];
  long htext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN;
  long hlen;
  //  the match statistics variable
  char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
  long stext_maxlen =
    MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);
  long slen;
  char svrbl[MAX_PATTERN];	//  the match statistics text buffer
  long svlen;
  long fnameoffset;
  char fname[MAX_FILE_NAME_LEN];
  long eflags;
  long cflags;
  //  long vhtindex;
  long not_microgroom = 1;

  struct stat statbuf;		//  for statting the hash file
  unsigned int hashpipe[OSB_BAYES_WINDOW_LEN + 1];
  regex_t regcb;

  double hits[MAX_CLASSIFIERS];	// actual hits per feature per classifier
  unsigned long totalhits[MAX_CLASSIFIERS];	// actual total hits per classifier
  unsigned long learnings[MAX_CLASSIFIERS];	// total learnings per classifier
  unsigned long total_learnings = 0;
  unsigned long totalfeatures;	//  total features
  unsigned long uniquefeatures[MAX_CLASSIFIERS];	//  found features per class
  unsigned long missedfeatures[MAX_CLASSIFIERS];	//  missed features per class
  double htf;			// hits this feature got.
  double tprob;			//  total probability in the "success" domain.
  double min_success = 0.5;	// minimum probability to be considered success

  // double textlen;		//  text length  - rougly corresponds to
  //  information content of the text to classify

  double ptc[MAX_CLASSIFIERS];	// current running probability of this class
  double renorm = 0.0;

  OSBF_FEATURE_HEADER_STRUCT *header[MAX_CLASSIFIERS];
  OSBF_FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
  char *seen_features[MAX_CLASSIFIERS];
  long hashlens[MAX_CLASSIFIERS];
  char *hashname[MAX_CLASSIFIERS];
  long succhash;
  long vbar_seen;		// did we see '|' in classify's args?
  long maxhash;
  long fnstart, fnlen;
  long fn_start_here;
  long textoffset;
  long textmaxoffset;
  long bestseen;
  long thistotal;
  struct token_search ts;

  // cubic weights seem to work well with this new code... - Fidelis
  //float feature_weight[] = { 0,   125,   64,  27,  8, 1, 0 }; // cubic
  // these empirical weights give better accuracy with
  // the CF * unique/totalfeatures used in this code - Fidelis
  float feature_weight[] = { 0, 3125, 256, 27, 4, 1 };
  float confidence_factor;
  int asymmetric = 0;		/* for testings */
  int voodoo = 1;		/* default */

  //double top10scores[10];
  //long top10polys[10];
  //char top10texts[10][MAX_PATTERN];

  /* fprintf(stderr, "Starting classification...\n"); */

  if (user_trace)
    fprintf (stderr, "OSBF classify\n");
  if (internal_trace)
    fprintf (stderr, "executing a CLASSIFY\n");

  //           extract the hash file names
  crm_get_pgm_arg (htext, htext_maxlen, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, htext_maxlen);

  //           extract the "this is a word" regex
  //
  ptext[0] = '\0';		// assume empty regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);

  //       extract the optional pR offset value
  //
  crm_get_pgm_arg (ostext, MAX_PATTERN, apb->s2start, apb->s2len);
  oslen = apb->s2len;
  pR_offset = 0;
  min_success = 0.5;
  if (oslen > 0)
    {
      oslen = crm_nexpandvar (ostext, oslen, MAX_PATTERN);
      pR_offset = strtod (ostext, NULL);
      min_success = 1.0 - 1.0 / (1 + pow (10, pR_offset));
    }

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
  };

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
    };

  not_microgroom = 1;
  if (apb->sflags & CRM_MICROGROOM)
    {
      not_microgroom = 0;
      if (user_trace)
	fprintf (stderr, " disabling fast-skip optimization.\n");
    };

  //   compile the word regex if not empty
  if (ptext[0] != '\0')
    {
      if (internal_trace)
	fprintf (stderr, "\nWordmatch pattern is |%s|", ptext);
      i = crm_regcomp (&regcb, ptext, plen, cflags);
      if (i > 0)
	{
	  crm_regerror (i, &regcb, tempbuf, data_window_size);
	  nonfatalerror ("Regular Expression Compilation Problem:", tempbuf);
	  goto regcomp_failed;
	};
    }


  //       Now, the loop to open the files.
  bestseen = 0;
  thistotal = 0;

  //for (i = 0; i < 10; i++)
  //  {
  //    top10scores[i] = 0;
  //    top10polys[i] = 0;
  //    strcpy (top10texts[i], "");
  //  };
  //        --  probabilistic evaluator ---
  //     S = success; A = a testable attribute of success
  //     ns = not success, na = not attribute
  //     the chain rule we use is:
  //
  //                   P(A|S) P(S)
  //  P (S|A) =   -------------------------
  //             P(A|S) P(S) + P(A|NS) P(NS)
  //
  //     and we apply it repeatedly to evaluate the final prob.  For
  //     the initial a-priori probability, we use 0.5.  The output
  //     value (here, P(S|A) ) becomes the new a-priori for the next
  //     iteration.
  //
  //     Extension - we generalize the above to I classes as and feature
  //      F as follows:
  //
  //                         P(F|Ci) P(Ci)
  //    P(Ci|F) = ----------------------------------------
  //              Sum over all classes Ci of P(F|Ci) P(Ci)
  //
  //     We also correct for the unequal corpus sizes by multiplying
  //     the probabilities by a renormalization factor.  if Tg is the
  //     total number of good features, and Te is the total number of
  //     evil features, and Rg and Re are the raw relative scores,
  //     then the corrected relative scores Cg aqnd Ce are
  //
  //     Cg = (Rg / Tg)
  //     Ce = (Re / Te)
  //
  //     or  Ci = (Ri / Ti)
  //
  //     Cg and Ce can now be used as "corrected" relative counts
  //     to calculate the naive Bayesian probabilities.
  //
  //     Lastly, the issue of "over-certainty" rears it's ugly head.
  //     This is what happens when there's a zero raw count of either
  //     good or evil features at a particular place in the file; the
  //     strict but naive mathematical interpretation of this is that
  //     "feature A never/always occurs when in good/evil, hence this
  //     is conclusive evidence of good/evil and the probabilities go
  //     to 1.0 or 0.0, and are stuck there forevermore.  We use the
  //     somewhat ad-hoc interpretation that it is unreasonable to
  //     assume that any finite number of samples can appropriately
  //     represent an infinite continuum of spewage, so we can bound
  //     the certainty of any meausre to be in the range:
  //
  //        limit: [ 1/featurecount+2 , 1 - 1/featurecount+2].
  //
  //     The prior bound is strictly made-up-on-the-spot and has NO
  //     strong theoretical basis.  It does have the nice behavior
  //     that for feature counts of 0 the probability is clipped to
  //     [0.5, 0.5], for feature counts of 1 to [0.333, 0.666]
  //     for feature counts of 2 to [0.25, 0.75], for 3 to
  //     [0.2, 0.8], for 4 to [0.166, 0.833] and so on.
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
  while (fnlen > 0 && ((maxhash < MAX_CLASSIFIERS - 1)))
    {
      crm_nextword (htext, hlen, fn_start_here, &fnstart, &fnlen);
      if (fnlen > 0)
	{
	  strncpy (fname, &htext[fnstart], fnlen);
	  fn_start_here = fnstart + fnlen + 1;
	  fname[fnlen] = '\000';
	  if (user_trace)
	    fprintf (stderr, "Classifying with file -%s- "
		     "succhash=%ld, maxhash=%ld\n", fname, succhash, maxhash);
	  if (fname[0] == '|' && fname[1] == '\000')
	    {
	      if (vbar_seen)
		{
		  nonfatalerror
		    ("Only one ' | ' allowed in a CLASSIFY. \n",
		     "We'll ignore it for now.");
		}
	      else
		{
		  succhash = maxhash;
		};
	      vbar_seen++;
	    }
	  else
	    {
	      //  be sure the file exists
	      //             stat the file to get it's length
	      k = stat (fname, &statbuf);
	      //             quick check- does the file even exist?
	      if (k != 0)
		{
		  nonfatalerror ("Nonexistent Classify table named: ", fname);
		}
	      else
		{
		  //  file exists - do the open/process/close
		  //
		  hashlens[maxhash] = statbuf.st_size;
		  //  mmap the hash file into memory so we can bitwhack it
		  header[maxhash] = (OSBF_FEATURE_HEADER_STRUCT *)
		    crm_mmap_file ( fname,
				    0, hashlens[maxhash],
				    PROT_READ | PROT_WRITE,
				    MAP_SHARED,
				    NULL);
		  if (header[maxhash] == MAP_FAILED)
		    {
		      nonfatalerror
			("Couldn't memory-map the table file", fname);
		    }
		  else
		    {
		      //
		      //     Check to see if this file is the right version
		      //
		      long fev;
		      if (*
			  ((unsigned long *)
			   header[maxhash]->version) !=
			  OSBF_VERSION || header[maxhash]->flags != 0)
			{
			  fev =
			    fatalerror
			    ("The .cfc file is the wrong version!  Filename is: ",
			     fname);
			  return (fev);
			};

		      //     grab the start of the actual spectrum data.
		      //
		      hashes[maxhash] =
			(OSBF_FEATUREBUCKET_STRUCT *)
			header[maxhash] + header[maxhash]->buckets_start;
		      spectra_start = header[maxhash]->buckets_start;
		      learnings[maxhash] = header[maxhash]->learnings;
		      //
		      //   increment learnings to avoid division by 0
		      if (learnings[maxhash] == 0)
			learnings[maxhash]++;

		      // update total learnings
		      total_learnings += learnings[maxhash];

		      // set this hashlens to the length in features instead
		      // of the length in bytes.
		      hashlens[maxhash] = header[maxhash]->buckets;
		      hashname[maxhash] = (char *) malloc (fnlen + 10);
		      if (!hashname[maxhash])
			untrappableerror
			  ("Couldn't malloc hashname[maxhash]\n",
			   "We need that part later, so we're stuck. Sorry.");
		      strncpy (hashname[maxhash], fname, fnlen);
		      hashname[maxhash][fnlen] = '\000';
		      maxhash++;
		    };
		};
	    };
	  if (maxhash > MAX_CLASSIFIERS - 1)
	    nonfatalerror ("Too many classifier files.",
			   "Some may have been disregarded");
	};
    };

  for (i = 0; i < maxhash; i++)
    {
      seen_features[i] = malloc (header[i]->buckets);
      if (!seen_features[i])
	untrappableerror
	  ("Couldn't malloc seen features array\n",
	   "We need that part later, so we're stuck.  Sorry.");
      memset (seen_features[i], 0, header[i]->buckets);

      //  initialize our arrays for N .cfc files
      hits[i] = 0.0;		// absolute hit counts
      totalhits[i] = 0;		// absolute hit counts
      uniquefeatures[i] = 0;	// features counted per class
      missedfeatures[i] = 0;	// missed features per class
      // a priori probability
      ptc[i] = (double) learnings[i] / total_learnings;
      // ptc[i] = 0.5;
    }

  //
  //    If there is no '|', then all files are "success" files.
  if (succhash == 0)
    succhash = maxhash;

  //    a CLASSIFY with no arguments is always a "success".
  if (maxhash == 0)
    return (0);

  if (user_trace)
    fprintf (stderr,
	     "Running with %ld files for success out of %ld files\n",
	     succhash, maxhash);
  // sanity checks...  Uncomment for super-strict CLASSIFY.
  //
  //    do we have at least 1 valid .cfc files?
  if (maxhash == 0)
    {
      fatalerror ("Couldn't open at least 2 .cfc files for classify().", "");
    };
  //    do we have at least 1 valid .cfc file at both sides of '|'?
  //if (!vbar_seen || succhash < 0 || (maxhash < succhash + 2))
  //  {
  //    nonfatalerror (
  //      "Couldn't open at least 1 .cfc file per SUCC | FAIL classes "
  //    " for classify().\n","Hope you know what are you doing.");
  //  };

  //
  //   now all of the files are mmapped into memory,
  //   and we can do the polynomials and add up points.
  i = 0;
  j = 0;
  k = 0;
  thistotal = 0;

  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;


  //   init the hashpipe with 0xDEADBEEF
  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
    {
      hashpipe[h] = 0xDEADBEEF;
    };

  totalfeatures = 0;

  //  stop when we no longer get any regex matches
  //   possible edge effect here- last character must be matchable, yet
  //    it's also the "end of buffer".

  // initialize the token search structure
  ts.ptok = (unsigned char *) &(txtptr[textoffset]);
  ts.max_ptok = (unsigned char *) &(txtptr[textmaxoffset]);
  ts.toklen = 0;
  ts.pattern = ptext;
  ts.regcb = &regcb;
  ts.max_long_tokens = OSBF_MAX_LONG_TOKENS;

  while (get_next_hash (&ts) == 0)
    {

      if (internal_trace)
	{
	  memmove (tempbuf, ts.ptok, ts.toklen);
	  tempbuf[ts.toklen] = '\000';
	  fprintf (stderr,
		   "  Classify #%ld t.o. %ld strt %ld end %ld len %lu is -%s-\n",
		   i,
		   textoffset,
		   ts.ptok -
		   (unsigned char *) &(txtptr[textoffset]),
		   (ts.ptok + ts.toklen) -
		   (unsigned char *) &(txtptr[textoffset]),
		   ts.toklen, tempbuf);
	};

      //  slide previous hashes up 1
      for (h = OSB_BAYES_WINDOW_LEN - 1; h > 0; h--)
	{
	  hashpipe[h] = hashpipe[h - 1];
	};

      //  and put new hash into pipeline
      hashpipe[0] = ts.hash;

      if (0)
	{
	  fprintf (stderr, "  Hashpipe contents: ");
	  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
	    fprintf (stderr, " %u", hashpipe[h]);
	  fprintf (stderr, "\n");
	};

      /* prepare for next token */
      ts.ptok += ts.toklen;
      textoffset += ts.ptok - (unsigned char *) &(txtptr[textoffset]);
      i++;

      {
	int j, k;
	unsigned th = 0;	//  a counter used only in TSS hashing
	unsigned long hindex;
	unsigned long h1, h2;
	// remember indexes of classes with min and max local probabilities
	int i_min_p, i_max_p;
	// remember min and max local probabilities of a feature
	double min_local_p, max_local_p;
	int already_seen;

	//
	th = 0;
	//
	for (j = 1; j < OSB_BAYES_WINDOW_LEN; j++)
	  {
	    h1 = hashpipe[0] * hctable[0] + hashpipe[j] * hctable[j << 1];
	    h2 =
	      hashpipe[0] * hctable[1] + hashpipe[j] * hctable[(j << 1) - 1];
	    hindex = h1;

	    if (internal_trace)
	      fprintf (stderr,
		       "Polynomial %d has h1:%ld  h2: %ld\n", j, h1, h2);
	    //
	    //    Note - a strict interpretation of Bayesian
	    //    chain probabilities should use 0 as the initial
	    //    state.  However, because we rapidly run out of
	    //    significant digits, we use a much less strong
	    //    initial state.   Note also that any nonzero
	    //    positive value prevents divide-by-zero
	    //
	    //       Zero out "Hits This Feature"
	    htf = 0;
	    totalfeatures++;
	    //
	    //    calculate the precursors to the local probabilities;
	    //    these are the hits[k] array, and the htf total.
	    //
	    min_local_p = 1.0;
	    max_local_p = 0;
	    i_min_p = i_max_p = 0;
	    already_seen = 0;
	    for (k = 0; k < maxhash; k++)
	      {
		long lh, lh0;
		float p_feat = 0;

		lh = hindex % (hashlens[k]);
		lh0 = lh;
		hits[k] = 0;

		lh = crm_osbf_find_bucket (header[k], h1, h2);

		// if the feature isn't found in the class, the index lh
		// will point to the first empty bucket after the chain
		// and its value will be 0.
		//
		// the bucket is valid if its index is valid. if the
		// index "lh" is >= the number of buckets, it means that
		// the .cfc file is full and the bucket wasn't found

		if (VALID_BUCKET (header[k], lh) && seen_features[k][lh] == 0)
		  {
		    // only not previously seen features are considered
		    if (GET_BUCKET_VALUE (hashes[k][lh]) != 0)
		      {
			uniquefeatures[k] += 1;	// count unique features used
			hits[k] = GET_BUCKET_VALUE (hashes[k][lh]);
			totalhits[k] += hits[k];	// remember totalhits
			htf += hits[k];	// and hits-this-feature
			p_feat = hits[k] / learnings[k];
			// find class with minimum P(F)
			if (p_feat <= min_local_p)
			  {
			    i_min_p = k;
			    min_local_p = p_feat;
			  }
			// find class with maximum P(F)
			if (p_feat >= max_local_p)
			  {
			    i_max_p = k;
			    max_local_p = p_feat;
			  }
			// mark the feature as seen
			seen_features[k][lh] = 1;
		      }
		    else
		      {
			// a feature that wasn't found can't be marked as
			// already seen in the doc because the index lh
			// doesn't refer to it, but to the first empty bucket
			// after the chain, which is common to all not-found
			// features in the same chain. This is not a problem
			// though, because if the feature is found in another
			// class, it'll be marked as seen on that class,
			// which is enough to mark it as seen. If it's not
			// found in any class, it will have zero count on all
			// classes and will be ignored as well. So, only
			// found features are marked as seen.
			i_min_p = k;
			min_local_p = p_feat = 0;
			// for statistics only (for now...)
			missedfeatures[k] += 1;
		      }
		  }
		else
		  {
		    // ignore already seen features
		    if (VALID_BUCKET (header[k], lh))
		      {
		    min_local_p = max_local_p = 0;
		    already_seen = 1;
		    if (asymmetric != 0)
		      break;
		  }
		    else
		      {
			/* bucket not valid. treat like feature not found */
			i_min_p = k;
			min_local_p = p_feat = 0;
			// for statistics only (for now...)
			missedfeatures[k] += 1;
		      }
		  }
	      }

	    //=======================================================
	    // Update the probabilities using Bayes:
	    //
	    //                      P(F|S) P(S)
	    //     P(S|F) = -------------------------------
	    //               P(F|S) P(S) +  P(F|NS) P(NS)
	    //
	    // S = class spam; NS = class nonspam; F = feature
	    //
	    // Here we adopt a different method for estimating
	    // P(F|S). Instead of estimating P(F|S) as (hits[S][F] /
	    // (hits[S][F] + hits[NS][F])), like in the original
	    // code, we use (hits[S][F] / learnings[S]) which is the
	    // ratio between the number of messages of the class S
	    // where the feature F was observed during learnings and
	    // the total number of learnings of that class. Both
	    // values are kept in the respective .cfc file, the
	    // number of learnings in the header and the number of
	    // occurrences of the feature F as the value of its
	    // feature bucket.
	    //
	    // It's worth noting another important difference here:
	    // as we want to estimate the *number of messages* of a
	    // given class where a certain feature F occurs, we
	    // count only the first ocurrence of each feature in a
	    // message (repetitions are ignored), both when learning
	    // and when classifying.
	    //
	    // Advantages of this method, compared to the original:
	    //
	    // - First of all, and the most important: accuracy is
	    // really much better, at about the same speed! With
	    // this higher accuracy, it's also possible to increase
	    // the speed, at the cost of a low decrease in accuracy,
	    // using smaller .cfc files;
	    //
	    // - It is not affected by different sized classes
	    // because the numerator and the denominator belong to
	    // the same class;
	    //
	    // - It allows a simple and fast pruning method that
	    // seems to introduce little noise: just zero features
	    // with lower count in a overflowed chain, zeroing first
	    // those in their right places, to increase the chances
	    // of deleting older ones.
	    //
	    // Disadvantages:
	    //
	    // - It breaks compatibility with previous css file
	    // format because of different header structure and
	    // meaning of the counts.
	    //
	    // Confidence factors
	    //
	    // The motivation for confidence factors is to reduce
	    // the noise introduced by features with small counts
	    // and/or low significance. This is an attempt to mimic
	    // what we do when inspecting a message to tell if it is
	    // spam or not. We intuitively consider only a few
	    // tokens, those which carry strong indications,
	    // according to what we've learned and remember, and
	    // discard the ones that may occur (approximately)
	    // equally in both classes.
	    //
	    // Once P(Feature|Class) is estimated as above, the
	    // calculated value is adjusted using the following
	    // formula:
	    //
	    //  CP(Feature|Class) = 0.5 +
	    //             CF(Feature) * (P(Feature|Class) - 0.5)
	    //
	    // Where CF(Feature) is the confidence factor and
	    // CP(Feature|Class) is the adjusted estimate for the
	    // probability.
	    //
	    // CF(Feature) is calculated taking into account the
	    // weight, the max and the min frequency of the feature
	    // over the classes, using the empirical formula:
	    //
	    //     (((Hmax - Hmin)^2 + Hmax*Hmin - K1/SH) / SH^2) ^ K2
	    // CF(Feature) = ------------------------------------------
	    //                    1 +  K3 / (SH * Weight)
	    //
	    // Hmax  - Number of documents with the feature "F" on
	    // the class with max local probability;
	    // Hmin  - Number of documents with the feature "F" on
	    // the class with min local probability;
	    // SH - Sum of Hmax and Hmin
	    // K1, K2, K3 - Empirical constants
	    //
	    // OBS: - Hmax and Hmin are normalized to the max number
	    //  of learnings of the 2 classes involved.
	    //  - Besides modulating the estimated P(Feature|Class),
	    //  reducing the noise, 0 <= CF < 1 is also used to
	    //  restrict the probability range, avoiding the
	    //  certainty falsely implied by a 0 count for a given
	    //  class.
	    //
	    // -- Fidelis Assis
	    //=========================================================

	    // ignore less significant features (confidence factor = 0)
	    if (already_seen != 0 || (max_local_p - min_local_p) < 1.0E-6)
	      continue;
	    // testing speed-up...
	    if (min_local_p > 0
		&& (max_local_p / min_local_p) < min_pmax_pmin_ratio)
	      continue;

	    // code under testing....
	    // calculate confidence_factor
	    {
	      // hmmm, unsigned long gives better precision than float...
	      //float hits_max_p, hits_min_p, sum_hits, diff_hits;
	      //unsigned long hits_max_p, hits_min_p, sum_hits, diff_hits;
	      unsigned long hits_max_p, hits_min_p, sum_hits;
	      long diff_hits;
	      float K1, K2, K3;

	      hits_min_p = hits[i_min_p];
	      hits_max_p = hits[i_max_p];

	      // normalize hits to max learnings
	      if (learnings[i_min_p] < learnings[i_max_p])
		hits_min_p *=
		  (float) learnings[i_max_p] / (float) learnings[i_min_p];
	      else
		hits_max_p *=
		  (float) learnings[i_min_p] / (float) learnings[i_max_p];

	      sum_hits = hits_max_p + hits_min_p;
	      diff_hits = hits_max_p - hits_min_p;
	      if (diff_hits < 0)
		diff_hits = -diff_hits;

	      // constants used in the CF formula above
	      // K1 = 0.25; K2 = 10; K3 = 8;
	      K1 = 0.25;
	      K2 = 10;
	      K3 = 8;

	      // calculate confidence factor (CF)
	      if (voodoo == 0)	/* || min_local_p > 0) */
		confidence_factor = 1 - DBL_MIN;
	      else
		confidence_factor =
		  pow ((diff_hits * diff_hits +
			hits_max_p * hits_min_p -
			K1 / sum_hits) / (sum_hits * sum_hits),
		       K2) / (1.0 + K3 / (sum_hits * feature_weight[j]));

	      if (internal_trace)
		printf ("CF: %.4f, max_hits = %3ld, min_hits = %3ld, "
			"weight: %5.1f\n", confidence_factor,
			hits_max_p, hits_min_p, feature_weight[j]);
	    }

	    // calculate the numerators P(F|C) * P(C)
	    renorm = 0.0;
	    for (k = 0; k < maxhash; k++)
	      {
		// P(F|C) = hits[k]/learnings[k], adjusted with a
		// confidence factor, to reduce the influence
		// of features common to all classes
		ptc[k] = ptc[k] * (0.5 + confidence_factor *
				   (hits[k] / learnings[k] - 0.5));

		//   if we have underflow (any probability == 0.0 ) then
		//   bump the probability back up to 10^-308, or
		//   whatever a small multiple of the minimum double
		//   precision value is on the current platform.
		if (ptc[k] < 10 * DBL_MIN)
		  ptc[k] = 10 * DBL_MIN;
		renorm += ptc[k];

		if (internal_trace)
		  printf
		    ("CF: %.4f, totalhits[k]: %lu, missedfeatures[k]: %lu, "
		     "uniquefeatures[k]: %lu, totalfeatures: %lu, "
		     "weight: %5.1f\n", confidence_factor,
		     totalhits[k], missedfeatures[k],
		     uniquefeatures[k], totalfeatures, feature_weight[j]);
	      }

	    // renormalize probabilities
	    for (k = 0; k < maxhash; k++)
	      ptc[k] = ptc[k] / renorm;

	    if (internal_trace)
	      {
		for (k = 0; k < maxhash; k++)
		  {
		    fprintf (stderr,
			     " poly: %d  filenum: %d, HTF: %7.0f, "
			     "learnings: %7lu, hits: %7.0f, "
			     "Pc: %6.4e\n", j, k, htf,
			     header[k]->learnings, hits[k], ptc[k]);
		  };
	      };
	    //
	    //    avoid the fencepost error for window=1
	    if (OSB_BAYES_WINDOW_LEN == 1)
	      {
		j = 99999;
	      };
	  };
      };
    };				//  end of repeat-the-regex loop

  //  cleanup time!
  //  remember to let go of the fd's and mmaps
  for (k = 0; k < maxhash; k++)
    {
      //  let go of the file, but allow caches to be retained
      if (header[k]) crm_munmap_file ((void *) header[k]);
      free (seen_features[k]);
    };

  //  and let go of the regex buffery
  if (ptext[0] != '\0')
    crm_regfree (&regcb);

  //   and one last chance to force probabilities into the non-stuck zone
  //
  //  if (pic == 0.0 ) pic = DBL_MIN;
  //if (pnic == 0.0 ) pnic = DBL_MIN;
  /*
  for (k = 0; k < maxhash; k++)
    if (ptc[k] < 10 * DBL_MIN)
      ptc[k] = 10 * DBL_MIN;
  */


  if (user_trace)
    {
      for (k = 0; k < maxhash; k++)
	fprintf (stderr,
		 "Probability of match for file %ld: %f\n", k, ptc[k]);
    };
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
      buf[0] = '\000';
      accumulator = 10 * DBL_MIN;
      for (m = 0; m < succhash; m++)
	{
	  accumulator = accumulator + ptc[m];
	};
      remainder = 10 * DBL_MIN;
      for (m = succhash; m < maxhash; m++)
	  {
	    remainder = remainder + ptc[m];
	  };
      overall_pR = log10 (accumulator) - log10 (remainder);

      //  note also that strcat _accumulates_ in stext.
      //  There would be a possible buffer overflow except that _we_ control
      //  what gets written here.  So it's no biggie.

      if (tprob > min_success)
	{
	  // if a pR offset was given, print it together with the real pR
	  if (oslen > 0)
	    {
	      sprintf (buf,
		       "CLASSIFY succeeds; success probability: "
		       "%6.4f  pR: %6.4f/%6.4f\n",
		       tprob, overall_pR, pR_offset);
	    }
	  else
	    {
	      sprintf (buf,
		       "CLASSIFY succeeds; success probability: "
		       "%6.4f  pR: %6.4f\n", tprob, overall_pR);
	    }
	}
      else
	{
	  // if a pR offset was given, print it together with the real pR
	  if (oslen > 0)
	    {
	      sprintf (buf,
		       "CLASSIFY fails; success probability: "
		       "%6.4f  pR: %6.4f/%6.4f\n",
		       tprob, overall_pR, pR_offset);
	    }
	  else
	    {
	      sprintf (buf,
		       "CLASSIFY fails; success probability: "
		       "%6.4f  pR: %6.4f\n", tprob, overall_pR);
	    }
	};
      if (strlen (stext) + strlen (buf) <= stext_maxlen)
	strcat (stext, buf);
      bestseen = 0;
      for (k = 0; k < maxhash; k++)
	if (ptc[k] > ptc[bestseen])
	  bestseen = k;
      remainder = 10 * DBL_MIN;
      for (m = 0; m < maxhash; m++)
	if (bestseen != m)
	  {
	    remainder = remainder + ptc[m];
	  };
      sprintf (buf, "Best match to file #%ld (%s) "
	       "prob: %6.4f  pR: %6.4f  \n",
	       bestseen,
	       hashname[bestseen],
	       ptc[bestseen], (log10 (ptc[bestseen]) - log10 (remainder)));
      if (strlen (stext) + strlen (buf) <= stext_maxlen)
	strcat (stext, buf);
      sprintf (buf, "Total features in input file: %ld\n", totalfeatures);
      if (strlen (stext) + strlen (buf) <= stext_maxlen)
	strcat (stext, buf);
      for (k = 0; k < maxhash; k++)
	{
	  long m;
	  remainder = 10 * DBL_MIN;
	  for (m = 0; m < maxhash; m++)
	    if (k != m)
	      {
		remainder = remainder + ptc[m];
	      };
	  sprintf (buf,
		   "#%ld (%s):"
		   " hits: %ld, ufeats: %ld, prob: %3.2e, pR: %6.2f \n",
		   k,
		   hashname[k],
		   totalhits[k],
		   uniquefeatures[k],
		   ptc[k], (log10 (ptc[k]) - log10 (remainder)));
	  // strcat (stext, buf);
	  if (strlen (stext) + strlen (buf) <= stext_maxlen)
	    strcat (stext, buf);
	}
      // check here if we got enough room in stext to stuff everything
      // perhaps we'd better rise a nonfatalerror, instead of just
      // whining on stderr
      if (strcmp (&(stext[strlen (stext) - strlen (buf)]), buf) != 0)
	{
	  nonfatalerror
	    ("WARNING: not enough room in the buffer to create "
	     "the statistics text.  Perhaps you could try bigger "
	     "values for MAX_CLASSIFIERS or MAX_FILE_NAME_LEN?", " ");
	}
      crm_destructive_alter_nvariable (svrbl, svlen, stext, strlen (stext));
    }

  //
  //  Free the hashnames, to avoid a memory leak.
  //
  for (i = 0; i < maxhash; i++)
    free (hashname[i]);
  if (tprob <= min_success)
    {
      if (user_trace)
	fprintf (stderr, "CLASSIFY was a FAIL, skipping forward.\n");
      //    and do what we do for a FAIL here
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
      return (0);
    }


  //
  //   all done... if we got here, we should just continue execution
  if (user_trace)
    fprintf (stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
regcomp_failed:
  return (0);
}
