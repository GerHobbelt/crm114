//	crm_osb_bayes.c  - OSB Bayes utilities

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

#define STRIDE 2
// ugly hack for doing stride 2
struct double_feature {unsigned int word[STRIDE];};

// qsort compare function: compare 64-bit feature hashes
static int compare_double_features(const void *p0, const void *p1)
{
  const struct double_feature *d0 = (struct double_feature *)p0;
  const struct double_feature *d1 = (struct double_feature *)p1;
  int ret;

  if (d0->word[0] > d1->word[0])
    ret = 1;
  else if (d0->word[0] < d1->word[0])
    ret = -1;
#if (STRIDE >= 2)
  else if (d0->word[1] > d1->word[1])
    ret = 1;
  else if (d0->word[1] < d1->word[1])
    ret = -1;
#endif	// (STRIDE >= 2)
  else
    ret = 0;

  return ret;
}

//
//    How to learn OSB_Bayes style  - in this case, we'll include the single
//    word terms that may not strictly be necessary.
//

int crm_expr_osb_bayes_learn (CSL_CELL *csl, ARGPARSE_BLOCK *apb,
			      char *txtptr, long txtstart, long txtlen)
{
  //     learn the osb_bayes transform spectrum of this input window as
  //     belonging to a particular type.
  //     learn <flags> (classname) /word/
  //
  long i, j, k;
  char ptext[MAX_PATTERN];  //  the regex pattern
  long plen;
  long osb_bayes_file_length;
  char htext[MAX_PATTERN];  //  the hash file name
  long hlen;
  struct stat statbuf;      //  for statting the hash file
  long hfsize;              //  size of the hash file
  FEATUREBUCKET_STRUCT *hashes;  //  the text of the hash file
  //
  // malloc'ed large array of feature hashes
  unsigned int *features;
  long features_out;
  long textoffset;
  long sense;
  long microgroom;
  long fev;
  //
  unsigned long learns_index = 0;
  unsigned long features_index = 0;

  char *learnfilename;

  if (internal_trace)
    fprintf (stderr, "executing a LEARN\n");

  features = (unsigned int *)
    malloc(OSB_BAYES_MAX_FEATURE_COUNT * STRIDE * sizeof(*features));
  if (features == NULL)
    untrappableerror5("Couldn't allocate features array", "", CRM_ENGINE_HERE);

  //           extract the hash file name
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, MAX_PATTERN);

  //     get the "this is a word" regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);

  //            set our cflags, if needed.  The defaults are
  //            "case" and "affirm", (both zero valued).
  //            and "microgroom" disabled.
  sense = +1;
  if (apb->sflags & CRM_REFUTE)
    {
      sense = -sense;
      if (user_trace)
	fprintf (stderr, " refuting learning\n");
    };
  microgroom = 0;
  if (apb->sflags & CRM_MICROGROOM)
    {
      microgroom = 1;
      if (user_trace)
	fprintf (stderr, " enabling microgrooming.\n");
    };

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
  learnfilename = strdup ( &(htext[i] ));

  //             and stat it to get it's length
  k = stat (&htext[i], &statbuf);


  //             quick check- does the file even exist?
  if (k != 0)
    {
      //      file didn't exist... create it
      FILE *f;
      if (user_trace)
	fprintf (stderr, "\nHad to create new CSS file %s\n", &htext[i]);
      f = fopen (&htext[i], "wb");
      if (!f)
	{
	  fprintf (stderr,
		"\n Couldn't open your new CSS file %s for writing; errno=%d .\n",
		 &htext[i], errno);
	  if (engine_exit_base != 0)
	    {
	      exit (engine_exit_base + 20);
	    }
	  else
	    exit (EXIT_FAILURE);
        };
      //       did we get a value for sparse_spectrum_file_length?
      osb_bayes_file_length = sparse_spectrum_file_length;
      if (osb_bayes_file_length == 0 ) {
	osb_bayes_file_length =
	  DEFAULT_OSB_BAYES_SPARSE_SPECTRUM_FILE_LENGTH;
      }

      //       put in osb_bayes_file_length entries of NULL
      for (j = 0;
	   j < osb_bayes_file_length
	     * sizeof ( FEATUREBUCKET_STRUCT);
	   j++)
	fputc ('\000', f);
      //
      fclose (f);
      //    and reset the statbuf to be correct
      k = stat (&htext[i], &statbuf);
    };
  //
  hfsize = statbuf.st_size;
  if (user_trace)
    fprintf (stderr, "Sparse spectra file %s has length %ld bins\n",
	     &htext[i], hfsize / sizeof (FEATUREBUCKET_STRUCT));

  //
  //      map the .css file into memory
  //
  hashes = (FEATUREBUCKET_STRUCT *) crm_mmap_file (&(htext[i]),
						   0, hfsize,
						   PROT_READ | PROT_WRITE,
						   MAP_SHARED,
						   NULL);
  if (hashes == MAP_FAILED)
    {
      fev = fatalerror5 ("Couldn't get to the statistic file named: ",
			 &htext[i], CRM_ENGINE_HERE);
      return (fev);
    };


  //
  //   now set the hfsize to the number of entries, not the number
  //   of bytes total
  hfsize = hfsize / sizeof ( FEATUREBUCKET_STRUCT );


#ifdef OSB_LEARNCOUNTS
  //       If LEARNCOUNTS is enabled, we normalize with documents-learned.
  //
  //       We use the reserved h2 == 0 setup for the learncount.
  //
  {
    char* litf = "Learnings in this file";
    char* fitf = "Features in this file";
    unsigned int h1, hindex;
    //
    h1 = strnhash (litf, strlen ( litf ));
    hindex = h1 % hfsize;
    if (hashes[hindex].hash != h1)
      {
	// initialize the file?
	if (hashes[hindex].hash == 0 && hashes[hindex].key == 0)
	  {
	    hashes[hindex].hash = h1;
	    hashes[hindex].key = 0;
	    hashes[hindex].value = 1;
	    learns_index = hindex;
	  }
	else
	  {
	    fatalerror5 (" This file should have learncounts, but doesn't!",
			 " The slot is busy, too.  It's hosed.  Time to die.",
			 CRM_ENGINE_HERE);
	    goto done;
	  };
      }
    else
      {
	if (hashes[hindex].key == 0)
	  //   the learncount matched.
	  {
	    learns_index = hindex;
	    if (sense > 0)
	      hashes[hindex].value = hashes[hindex].value + sense;
	    else
	      {
	      if (hashes[hindex].value + sense > 0)
		hashes[hindex].value += sense;
	      else
		hashes[hindex].value = 0;
	      }
	    if (user_trace)
	      fprintf (stderr, "This file has had %u documents learned!\n",
		       hashes[hindex].value);
	  };
      };
    h1 = strnhash (fitf, strlen ( fitf ));
    hindex = h1 % hfsize;
    if (hashes[hindex].hash != h1)
      {
	// initialize the file?
	if (hashes[hindex].hash == 0 && hashes[hindex].key == 0)
	  {
	    hashes[hindex].hash = h1;
	    hashes[hindex].key = 0;
	    hashes[hindex].value = 1;
	    features_index = hindex;
	  }
	else
	  {
	    fatalerror5 (" This file should have learncounts, but doesn't!",
			 " The slot is busy, too.  It's hosed.  Time to die.",
			 CRM_ENGINE_HERE);
	    goto done ;
	  };
      }
    else
      {
	if (hashes[hindex].key == 0)
	  //   the learncount matched.
	  {
	    features_index = hindex;
	    if (user_trace)
	      fprintf (stderr, "This file has had %u features learned!\n",
		       hashes[hindex].value);
	  };
      };
  };

#endif	// OSB_LEARNCOUNTS


  textoffset = txtstart;


  (void)crm_vector_tokenize_selector(apb,
				     txtptr, txtstart, txtlen,
				     ptext, plen,
				     NULL, 0, 0,
				     features, (long)(OSB_BAYES_MAX_FEATURE_COUNT * STRIDE),
				     &features_out,
				     &textoffset);

// #if (0)
// // Can't count on this.  When learning QUICKREF.txt, tokenizer's
// // match.eo only goes a few characters into the whitespace at the
// // end, doesn't go to end of text.
// if (textoffset < txtlen)
//   (void)fatalerror5("Too many input features",
// 	      " (text being learned is too big).",
// 	      CRM_ENGINE_HERE);
// #ifdef

  if (apb->sflags & CRM_UNIQUE)
    {
      // hack: assume stride STRIDE
      struct double_feature *d = (struct double_feature *)&features[0];
      long n_double_features = features_out / STRIDE;

      qsort(d, n_double_features, sizeof(struct double_feature),
	    compare_double_features);
      i = 0;
      // remove successive duplicates
      for (j = 1; j < n_double_features; j++)
	if (d[j].word[0] != d[i].word[0] ||
	    d[j].word[1] != d[i].word[1])
	  d[++i] = d[j];
      // set new length, possibly shorter
      if (n_double_features > 0)
	n_double_features = i + 1;
      // convert new length to the other form
      features_out = n_double_features * STRIDE;
    };
  //    and the big loop... go through all of the text.
  // hack: assume crm_vector_tokenize_selector() picked stride STRIDE
  for (i = 0; i + (STRIDE - 1) < features_out; i += STRIDE)
    {
      unsigned int hindex;
      unsigned int h1, h2;
      unsigned long incrs;

      h1 = features[i];
      h2 = features[i + 1];
      if (h2 == 0) h2 = 0xdeadbeef;

      hindex = h1 % hfsize;

      //
      //   we now look at both the primary (h1) and
      //   crosscut (h2) indexes to see if we've got
      //   the right bucket or if we need to look further
      //
      incrs = 0;
      while ( //    part 1 - when to stop if sense is positive:
	     ! ( sense > 0
		 //          in positive mode, stop when we hit
		 //          the correct slot, OR when we hit an
		 //          zero-value (reusable) slot
		 && ( hashes[hindex].value == 0
		      || ( hashes[hindex].hash == h1
			   && hashes[hindex].key  == h2 )))
	     &&
	     ! ( sense <= 0
		 //          in negative/refute mode, stop when
		 //          we hit the correct slot, or a truly
		 //          unused (not just zero-valued reusable)
		 //          slot.
		 && ( ( hashes[hindex].hash == h1
			&& hashes[hindex].key == h2)
		      || ( hashes[hindex].value == 0
			   && hashes[hindex].hash == 0
			   && hashes[hindex].key == 0
			   ))))
	{
	  //
	  incrs++;
	  //
	  //       If microgrooming is enabled, and we've found a
	  //       chain that's too long, we groom it down.
	  //
	  if (microgroom && (incrs > MICROGROOM_CHAIN_LENGTH))
	    {
	      long zeroedfeatures;
	      //     set the random number generator up...
	      //     note that this is repeatable for a
	      //     particular test set, yet dynamic.  That
	      //     way, we don't always autogroom away the
	      //     same feature; we depend on the previous
	      //     feature's key.
	      srand (h2);
	      //
	      //   and do the groom.
	      zeroedfeatures = crm_microgroom (hashes,
					       NULL,
					       hfsize,
					       hindex);
	      hashes[features_index].value -= zeroedfeatures;

	      //  since things may have moved after a
	      //  microgroom, restart our search
	      hindex = h1 % hfsize;
	      incrs = 0;
	    };
	  //      check to see if we've incremented ourself all the
	  //      way around the .css file.  If so, we're full, and
	  //      can hold no more features (this is unrecoverable)
	  if (incrs > hfsize - 3)
	    {
	      nonfatalerror5 ("Your program is stuffing too many "
			      "features into this size .css file.  "
			      "Adding any more features is "
			      "impossible in this file.",
			      "You are advised to build a larger "
			      ".css file and merge your data into "
			      "it.", CRM_ENGINE_HERE);
	      goto done;
	    };
	  hindex++;
	  if (hindex >= hfsize) hindex = 0;
	};

      if (internal_trace)
	{
	  if (hashes[hindex].value == 0)
	    {
	      fprintf (stderr,"New feature at %u\n", hindex);
	    }
	  else
	    {
	      fprintf (stderr, "Old feature at %u\n", hindex);
	    };
	};
      //      always rewrite hash and key, as they may be incorrect
      //    (on a reused bucket) or zero (on a fresh one)
      //
      //      watch out - sense may be both + or -, so check before
      //      adding it...
      //
      //     let the embedded feature counter sorta keep up...
      hashes[features_index].value += sense;

      if (sense > 0 )
	{
	  //     Right slot, set it up
	  //
	  hashes[hindex].hash = h1;
	  hashes[hindex].key  = h2;
	  if ( hashes[hindex].value + sense
	       >= FEATUREBUCKET_VALUE_MAX-1)
	    {
	      hashes[hindex].value = FEATUREBUCKET_VALUE_MAX - 1;
	    }
	  else
	    {
	      hashes[hindex].value += sense;
	    };
	};
      if ( sense < 0 )
	{
	  if (hashes[hindex].value <= -sense )
	    {
	      hashes[hindex].value = 0;
	    }
	  else
	    {
	      hashes[hindex].value += sense;
	    };
	};
    };

 done:

  free(features);

  //  and remember to let go of the mmap
  //  (we force the munmap, because otherwise we still have a link
  //  to the file which stays around until program exit)
  crm_force_munmap_addr ((void *) hashes);

#ifndef CRM_WINDOWS
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .css ile, to force
  //    the filesystem to repropagate it's caches.
  //
  {
    int hfd;                  //  hashfile fd
    FEATURE_HEADER_STRUCT foo;
    hfd = open (learnfilename, O_RDWR);
    dontcare = read (hfd, &foo, sizeof(foo));
    lseek (hfd, 0, SEEK_SET);
    dontcare = write (hfd, &foo, sizeof(foo));
    close (hfd);
  }
#endif	// !CRM_WINDOWS

  return (0);
}


//      How to do a OSB_Bayes CLASSIFY some text.
//
int crm_expr_osb_bayes_classify (CSL_CELL *csl, ARGPARSE_BLOCK *apb,
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
  //  long vhtindex;
  long nrows;
  long use_chisquared;
  //
  //            use embedded feature index counters, rather than full scans
  unsigned long learns_index [MAX_CLASSIFIERS];
  unsigned long total_learns;
  unsigned long features_index [MAX_CLASSIFIERS];
  unsigned long total_features;

  // malloc'ed large array of feature hashes
  unsigned int *features;
  long features_out;		// number returned by vector tokenizer
  long next_offset;		// where in text to look for next token

  //       map of features already seen (used for uniqueness tests)
  int use_unique;
  unsigned char *seen_features;

  struct stat statbuf;      //  for statting the hash file

  // unsigned long fcounts[MAX_CLASSIFIERS]; // total counts for feature normalize
  // unsigned long totalcount = 0;

  double cpcorr[MAX_CLASSIFIERS];	// corpus correction factors
  double hits[MAX_CLASSIFIERS];	// actual hits per feature per classifier
  long totalhits[MAX_CLASSIFIERS];	// actual total hits per classifier
  double chi2[MAX_CLASSIFIERS];	// chi-squared values (such as they are)
  long expected;		// expected hits for chi2.
  long unk_features;		//  total unknown features in the document
  double htf;			// hits this feature got.
  double tprob = 0.0;		//  total probability in the "success" domain.
				//  set to 0.0 for compiler warnings
  double ptc[MAX_CLASSIFIERS];	// current running probability of this class
  double renorm = 0.0;
  double pltc[MAX_CLASSIFIERS];	// current local probability of this class

  //  int hfds[MAX_CLASSIFIERS];
  FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
  long hashlens[MAX_CLASSIFIERS];
  char *hashname[MAX_CLASSIFIERS];
  long succhash;
  long vbar_seen;	// did we see '|' in classify's args?
  long maxhash;
  long fnstart, fnlen;
  long fn_start_here;
  long textoffset;
  long textmaxoffset;
  long bestseen;
  long thistotal;
  int ifile;

  double top10scores[10];
  long top10polys[10];
  char top10texts[10][MAX_PATTERN];


  if (internal_trace)
    fprintf (stderr, "executing a CLASSIFY\n");

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
  };

  //     status variable's text (used for output stats)
  //
  stext[0] = '\000';
  slen = 0;

  //            set our flags, if needed.

  use_unique = 0;
  if (apb->sflags & CRM_UNIQUE)
    {
      use_unique = 1;
      if (user_trace)
	fprintf (stderr, " unique engaged -repeated features are ignored \n");
    };

  // crm_vector_tokenize_selector() picks these numbers.  We just know.
  nrows = 4;
  if (apb->sflags & CRM_UNIGRAM)
    {
      nrows = 1;
      if (user_trace)
	fprintf (stderr, " using unigram features only \n");
    };

  use_chisquared = 0;
  if (apb->sflags & CRM_CHI2)
    {
      use_chisquared = 1;
      if (user_trace)
	fprintf (stderr, " using chi^2 chaining rule \n");
    };

  if ( internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);

  features = (unsigned int *)
    malloc(OSB_BAYES_MAX_FEATURE_COUNT * STRIDE * sizeof(*features));
  if (features == NULL)
    untrappableerror5("Couldn't allocate features array", "", CRM_ENGINE_HERE);
  if (use_unique)
    {
      if ((seen_features = calloc(OSB_BAYES_MAX_FEATURE_COUNT, 1)) == NULL)
	untrappableerror5
	  (" Couldn't allocate enough memory to keep track",
	   "of nonunique features.  This is deadly", CRM_ENGINE_HERE);
    }
  else
    seen_features = NULL;


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
      // fcounts[i] = 0;    // check later to prevent a divide-by-zero
    			 // error on empty .css file
      cpcorr[i] = 0.0;   // corpus correction factors
      hits[i] = 0.0;     // absolute hit counts
      totalhits[i] = 0;     // absolute hit counts
      ptc[i] = 0.5;      // priori probability
      pltc[i] = 0.5;     // local probability

    };

  for (i = 0; i < 10; i++)
    {
      top10scores[i] = 0;
      top10polys[i] = 0;
      strcpy (top10texts[i], "");
    };
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
  while ( fnlen > 0 && maxhash < MAX_CLASSIFIERS)
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
		  nonfatalerror5 ("Only one ' | ' allowed in a CLASSIFY. \n" ,
				  "We'll ignore it for now.", CRM_ENGINE_HERE);
		}
	      else
		{
		  succhash = maxhash;
		};
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
		  nonfatalerror5 ("Nonexistent Classify table named: ",
				  fname, CRM_ENGINE_HERE);
		}
	      else
		{
		  //  file exists - do the open/process/close
		  //
		  hashlens[maxhash] = statbuf.st_size;
		  //  mmap the hash file into memory so we can bitwhack it

		  hashes[maxhash] = (FEATUREBUCKET_STRUCT *)
		    crm_mmap_file ( fname,
				    0, hashlens[maxhash],
				    PROT_READ | PROT_WRITE,
				    MAP_SHARED,
				    NULL);
		  if (hashes[maxhash] == MAP_FAILED )
		    {
		      nonfatalerror5 ("Couldn't memory-map the table file",
				      fname, CRM_ENGINE_HERE);
		    }
		  else
		    {
		      //  set this hashlens to the length in features instead
		      //  of the length in bytes.
		      hashlens[maxhash] = hashlens[maxhash] / sizeof (FEATUREBUCKET_STRUCT);
		      hashname[maxhash] = (char *) malloc (fnlen+10);
		      if (!hashname[maxhash])
			untrappableerror5
			  ("Couldn't malloc hashname[maxhash]\n","We need that part later, so we're stuck.  Sorry.", CRM_ENGINE_HERE);
		      strncpy(hashname[maxhash],fname,fnlen);
		      hashname[maxhash][fnlen]='\000';
		      maxhash++;
		    };
		};
	    };
	  if (maxhash > MAX_CLASSIFIERS-1)
	    nonfatalerror5 ("Too many classifier files.",
			    "Some may have been disregarded", CRM_ENGINE_HERE);
	};
    };

  //
  //    If there is no '|', then all files are "success" files.
  if (succhash == 0)
    succhash = maxhash;

  if (user_trace)
    fprintf (stderr, "Running with %ld files for success out of %ld files\n",
	     succhash, maxhash );

  // sanity checks...  Uncomment for super-strict CLASSIFY.
  //
  //	do we have at least 1 valid .css files?
  if (maxhash == 0)
    {
      fatalerror5 ("Couldn't open at least 1 .css file for classify().",
		   "", CRM_ENGINE_HERE);
    };
  //	do we have at least 1 valid .css file at both sides of '|'?
  //if (!vbar_seen || succhash < 0 || (maxhash < succhash + 2))
  //  {
  //    nonfatalerror (
  //      "Couldn't open at least 1 .css file per SUCC | FAIL classes "
  //	" for classify().\n","Hope you know what are you doing.");
  //  };

  // CLASSIFY with no arguments is a "success", if not found insane above
  if (maxhash == 0)
    return (0);

  for (ifile = 0; ifile < maxhash; ifile++)
    {
      //    now, set up the normalization factor fcount[]
      //      count up the total first
      // fcounts[ifile] = 0;
      // {
      //   long k;
      //
      //   for (k = 1; k < hashlens[ifile]; k++)
      //     fcounts [ifile] = fcounts[ifile] + hashes[ifile][k].value;
      // }
      // if (fcounts[ifile] == 0) fcounts[ifile] = 1;
      // totalcount = totalcount + fcounts[ifile];

#ifdef OSB_LEARNCOUNTS
      //       If LEARNCOUNTS is enabled, we normalize with
      //       documents-learned.
      //
      //       We use the reserved h2 == 0 setup for the learncount.
      //
      {
	char* litf = "Learnings in this file";
	char* fitf = "Features in this file";
	unsigned int h1;
	unsigned int hindex;
	//
	h1 = strnhash (litf, strlen ( litf ));
	hindex = h1 % hashlens[ifile];
	if (hashes[ifile][hindex].hash != h1 || hashes[ifile][hindex].key != 0)
	  {
	    if (hashes[ifile][hindex].hash == 0 && hashes[ifile][hindex].key == 0)
	      {
		//   the slot is vacant - we use it.
		hashes[ifile][hindex].hash = h1;
		hashes[ifile][hindex].key = 0;
		hashes[ifile][hindex].value = 1;
		learns_index [ifile] = hindex;
	      }
	    else
	      {
		fatalerror5
		  (" This file should have learncounts, but doesn't,"
		   " and the learncount slot is busy.  It's hosed. ",
		   " Time to die.", CRM_ENGINE_HERE);
		goto done;
	      }
	  }
	else
	  {
	    //   the learncount slot was found matched.
	    learns_index [ifile] = hindex;
	    if (user_trace)
	      fprintf (stderr, "File # %d has had %u documents learned.\n",
		       ifile,
		       hashes[ifile][hindex].value);
	  };
	h1 = strnhash (fitf, strlen ( fitf ));
	hindex = h1 % hashlens[ifile];
	if (hindex == learns_index[ifile]) hindex++;
	if (hashes[ifile][hindex].hash != h1 || hashes[ifile][hindex].key != 0)
	  {
	    if (hashes[ifile][hindex].hash == 0 && hashes[ifile][hindex].key == 0)
	      {
		//   the slot is vacant - we use it.
		hashes[ifile][hindex].hash = h1;
		hashes[ifile][hindex].key = 0;
		hashes[ifile][hindex].value = 1;
		features_index[ifile] = hindex;
	      }
	    else
	      {
		fatalerror5
		  ("This file should have featurecounts, but doesn't,"
		   "and the featurecount slot is busy.  It's hosed. ",
		   " Time to die.", CRM_ENGINE_HERE);
		goto done;
	      }
	  }
	else
	  {
	    //   the learncount matched.
	    features_index[ifile] = hindex;
	    if (user_trace)
	      fprintf (stderr, "File %d has had %u features learned\n",
		       ifile,
		       hashes[ifile][hindex].value);
	  };
      };
#endif	// OSB_LEARNCOUNTS
    };
  //
  //     calculate cpcorr (count compensation correction)
  //
  total_learns = 0;
  total_features = 0;
  for (ifile = 0; ifile < maxhash; ifile++)
    {
      total_learns +=  hashes[ifile][learns_index[ifile]].value;
      total_features += hashes[ifile][features_index[ifile]].value;
    };

  for (ifile = 0; ifile < maxhash; ifile++)
    {
      //   disable cpcorr for now... unclear that it's useful.
      // cpcorr[ifile] = 1.0;
      //
      //  new cpcorr - from Fidelis' work on evaluators.  Note that
      //   we renormalize _all_ terms, not just the min term.
      cpcorr [ifile] =  (total_learns / (float) maxhash) /
	((float) hashes[ifile][learns_index[ifile]].value);

      if (use_chisquared)
	cpcorr[ifile] = 1.00;
    };

  if (internal_trace)
    fprintf (stderr, " files  %ld  learns #0 %u  #1 %u  total %lu cp0 %f cp1 %f \n",
	     maxhash,
	     hashes[0][learns_index[0]].value,
	     hashes[1][learns_index[1]].value,
	     total_learns,
	     cpcorr [0],
	     cpcorr [1] );


  //
  //   now all of the files are mmapped into memory,
  //   and we can do the polynomials and add up points.
  thistotal = 0;

  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;

  (void)crm_vector_tokenize_selector(apb,
				     txtptr, txtstart, txtlen,
				     ptext, plen,
				     NULL, 0, 0,
				     features, (long)(OSB_BAYES_MAX_FEATURE_COUNT * STRIDE),
				     &features_out, &next_offset);

// #if (0)
//  // can't count on this
//  if (next_offset < txtlen)
//    (void)fatalerror5("Too many input features",
//		      " (text being classified is too big).",
//		      CRM_ENGINE_HERE);
// #endif	// !0

  unk_features = features_out / STRIDE;

  // GROT GROT GROT
  // For each token found in the text, the vector tokenizer returns
  // nrows feature hashes, where nrows is the number of rows in the
  // coefficients matrix.  Feature weights and chi-squared feature
  // weights, below, are chosen according to which row the feature
  // hash came from. crm_vector_tokenize_selector() doesn't tell us
  // how many rows are in the matrix it selected, so we just have to
  // know -- see setting variable nrows, above -- and step through the
  // returned array of features in lockstep with how we think the
  // tokenizer generated it.  And we're doing stride STRIDE, which we
  // also just have to know.  Assuming that all works, the matrix row
  // subscript for a feature hash is (j / STRIDE) % nrows, where j is
  // the subscript in the array features.
  //
  // That lockstep requirement is why we uniquify with seen_features,
  // instead of just sort-uniquing what came back from vector
  // tokenize.  Sort-uniquing would throw away the implicit row
  // numbers in the array of features.

  for (j = 0; j + (STRIDE - 1) < features_out; j += STRIDE)
    {
      long irow = (j / STRIDE) % nrows;
      unsigned int h1, h2;
      int do_this_feature;

      //       Zero out "Hits This Feature"
      htf = 0.0;

      h1 = features[j];
      h2 = features[j + 1];
      if (h2 == 0) h2 = 0xdeadbeef;

      if (internal_trace)
	fprintf (stderr, "Polynomial %ld has h1:%u  h2: %u\n",
		 irow, h1, h2);

      if (use_unique)
	{
	  if (seen_features[h1 % OSB_BAYES_MAX_FEATURE_COUNT])
	    do_this_feature = 0;
	  else
	    {
	      do_this_feature = 1;
	      seen_features[h1 % OSB_BAYES_MAX_FEATURE_COUNT] = 1;
	    }
	}
      else
	do_this_feature = 1;

      if (do_this_feature)
	for (ifile = 0; ifile < maxhash; ifile++)
	  {
	    unsigned int hindex;
	    unsigned int lh;

	    hindex = h1 % hashlens[ifile];
	    lh = hindex;
	    hits[ifile] = 0;
	    while ( hashes[ifile][lh].key != 0
		    && ( hashes[ifile][lh].hash != h1
			 || hashes[ifile][lh].key  != h2 ))
	      {
		lh++;
		if (lh >= hashlens[ifile]) lh = 0; // wraparound
		if (lh == hindex) break;	// tried them all
	      };
	    if (hashes[ifile][lh].hash == h1 && hashes[ifile][lh].key == h2)
	      {
		//    Note - a strict interpretation of Bayesian
		//    chain probabilities should use 0 as the initial
		//    state.  However, because we rapidly run out of
		//    significant digits, we use a much less strong
		//    initial state.   Note also that any nonzero
		//    positive value prevents divide-by-zero.
		static int fw[] = {24, 14, 7, 4, 2, 1, 0};
		// cubic weights seems to work well for chi^2...- Fidelis
		static int chi_feature_weight[] = {125, 64, 27, 8, 1, 0};
		int feature_weight;
		long wh;	// occurrences this feature this file, weighted
				// ..."weighted hits"
		//
		//    calculate the precursors to the local probabilities;
		//    these are the hits[ifile] array, and the htf total.

		feature_weight = fw[irow];
		if ( use_chisquared )
		  {
		    feature_weight = chi_feature_weight[irow];
		    //  turn off weighting?
		    feature_weight = 1;
		  };
		wh = hashes[ifile][lh].value * feature_weight;
		wh = wh * cpcorr [ifile];    	// Correct with cpcorr
		// remember totalhits
		if (use_chisquared)
		  {
		    totalhits[ifile]++;
		  }
		else
		  {
		    totalhits[ifile] = totalhits[ifile] + wh;
		  }
		hits[ifile] = wh;
		htf = htf + hits[ifile];            // and hits-this-feature
	      };
	  };


      //      now update the probabilities.
      //
      //     NOTA BENE: there are a bunch of different ways to
      //      calculate local probabilities.  The text below
      //      refers to an experiment that may or may not make it
      //      as the "best way".
      //
      //      The hard part is this - what is the local in-class
      //      versus local out-of-class probability given the finding
      //      of a particular feature?
      //
      //      I'm guessing this- the validity is the differntial
      //      seen on the feature (that is, fgood - fevil )
      //      times the entropy of that feature as seen in the
      //      corpus (that is,
      //
      //              Pfeature*log2(Pfeature)
      //
      //      =
      //        totalcount_this_feature
      //            ---------------    * log2 (totalcount_this_feature)
      //         totalcount_all_features
      //
      //    (note, yes, the last term seems like it should be
      //    relative to totalcount_all_features, but a bit of algebra
      //    will show that if you view fgood and fevil as two different
      //    signals, then you end up with + and - totalcount inside
      //    the logarithm parenthesis, and they cancel out.
      //    (the 0.30102 converts "log10" to "log2" - it's not
      //    a magic number, it's just that log2 isn't in glibc)
      //

      //  HACK ALERT- this code here is still under development
      //  and should be viewed with the greatest possible
      //  suspicion.  :=)

      //    Now, some funky magic.  Our formula above is
      //    mathematically correct (if features were
      //    independent- something we conveniently ignore.),
      //    but because of the limited word length in a real
      //    computer, we can quickly run out of dynamic range
      //    early in a CLASSIFY (P(S) indistinguishable from
      //    1.00) and then there is no way to recover.  To
      //    remedy this, we use two alternate forms of the
      //    formula (in Psucc and Pfail) and use whichever
      //    form that yields the smaller probability to
      //    recalculate the value of the larger.
      //
      //    The net result of this subterfuge is a nonuniform
      //    representation of the probability, with a huge dynamic
      //    range in two places - near 0.0, and near 1.0 , which
      //    are the two places where we actually care.
      //
      //    Note upon note - we don't do this any more - instead we
      //    do a full renormalize and unstick at each local prob.
      //
      //   calculate renormalizer (the Bayesian formula's denomenator)

      if (do_this_feature)
	{
	  if (use_chisquared)
	    {

	      //  Actually, for chisquared with ONE feature
	      //  category (that being the number of weighted
	      //  hits) we end up with not having to do
	      //  anything here at all.  Instead, we look at
	      //  total hits expected in a document of this
	      //  length.
	      //
	      //  This actually makes sense, since the reality
	      //  is that most texts have an expected value of
	      //  far less than 1.0 for almost all featuess.
	      //  and thus common chi-squared assumptions
	      //  break down (like "need at least 5 in each
	      //  category"!)

	      // float renorm;
	      //double expected;
	      //for ( ifile = 0; ifile < maxhash; ifile++)
	      //	{
	      //   This is the first half of a BROKEN
	      //   chi-squared formula -
	      //
	      //  MeritProd =
	      //       Product (expected-observed)^2 / expected
	      //
	      //   Second half- when done with features, take the
	      //     featurecounth root of MeritProd.
	      //
	      //   Note that here the _lowest_ Merit is best fit.
	      //if (htf > 0 )
	      //  ptc[ifile] = ptc[ifile] *
	      //    (1.0 + ((htf/maxhash) - hits[ifile])
	      //     * (1.0 +(htf/maxhash) - hits[ifile]))
	      //    / (2.0 + htf/maxhash);
	      //
	      //  Renormalize to avoid really small
	      //  underflow...  this is unnecessary with
	      //  above better additive form
	      //
	      //renorm = 1.0;
	      //for (ifile = 0; ifile < maxhash; ifile++)
	      //renorm = renorm * ptc[ifile];
	      //for (ifile = 0; ifile < maxhash; ifile++)
	      //{
	      //  ptc[ifile] = ptc[ifile] / renorm;
	      //  fprintf (stderr, "IFILE= %d, rn=%f, ptc[ifile] = %f\n",
	      //  //   ifile, renorm,  ptc[ifile]);
	      //};

	      //    Nota BENE: the above is not standard chi2
	      //    here's a better one.
	      //    Again, lowest Merit is best fit.
	      //if (htf > 0 )
	      //  {
	      //    expected = (htf + 0.000001) / (maxhash + 1.0);
	      //  ptc[ifile] = ptc[ifile] +
	      //((expected - hits[ifile])
	      // * (expected - hits[ifile]))
	      // / expected;
	      //};
	      //};
	    }
	  else     //  if not chi-squared, use Bayesian
	    {
	      //   calculate local probabilities from hits
	      //

	      for (ifile = 0; ifile < maxhash; ifile++)
		{
		  pltc[ifile] = 0.5 +
		    (( hits[ifile] - (htf - hits[ifile]))
		     / (LOCAL_PROB_DENOM * (htf + 1.0)));
		};

	      //   Calculate the per-ptc renormalization numerators
	      renorm = 0.0;
	      for (ifile = 0; ifile < maxhash; ifile++)
		renorm = renorm + (ptc[ifile]*pltc[ifile]);

	      for (ifile = 0; ifile < maxhash; ifile++)
		ptc[ifile] = (ptc[ifile] * pltc[ifile]) / renorm;

	      //   if we have underflow (any probability == 0.0 ) then
	      //   bump the probability back up to 10^-308, or
	      //   whatever a small multiple of the minimum double
	      //   precision value is on the current platform.
	      //
	      for (ifile = 0; ifile < maxhash; ifile++)
		if (ptc[ifile] < 1000*DBL_MIN) ptc[ifile] = 1000 * DBL_MIN;

	      //
	      //      part 2) renormalize to sum probabilities to 1.0
	      //
	      renorm = 0.0;
	      for (ifile = 0; ifile < maxhash; ifile++)
		renorm = renorm + ptc[ifile];
	      for (ifile = 0; ifile < maxhash; ifile++)
		ptc[ifile] = ptc[ifile] / renorm;

	      for (ifile = 0; ifile < maxhash; ifile++)
		if (ptc[ifile] < 10*DBL_MIN) ptc[ifile] = 1000 * DBL_MIN;
	    };
	};
      if (internal_trace)
	{
	  for (ifile = 0; ifile < maxhash; ifile++)
	    {
	      fprintf (stderr,
		       " poly: %ld  filenum: %d, HTF: %7.0f, hits: %7.0f, Pl: %6.4e, Pc: %6.4e\n",
		       irow, ifile, htf, hits[ifile], pltc[ifile], ptc[ifile]);
	    };
	};
    };

  expected = 1;
  //     Do the chi-squared computation.  This is just
  //           (expected-observed)^2  / expected.
  //     Less means a closer match.
  //
  if (use_chisquared)
    {
      double features_here, learns_here;
      double avg_features_per_doc, this_doc_relative_len;
      double actual;

      //    The next statement appears stupid, but we don't have a
      //    good way to estimate the fraction of features that
      //    will be "out of corpus".  A very *rough* guess is that
      //    about 2/3 of the learned document features will be
      //    hapaxes - that is, features not seen before, so we'll
      //    start with the 1/3 that we expect to see in the corpus
      //    as not-hapaxes.
      expected = unk_features / 1.5 ;
      for (k = 0; k < maxhash; k++)
	{
	  if (totalhits[k] > expected)
	    expected = totalhits[k] + 1;
	}

      for (k = 0; k < maxhash; k++)
	{
	  features_here = hashes[k][features_index[k]].value;
	  learns_here = hashes[k][learns_index[k]].value ;
	  avg_features_per_doc = 1.0 + features_here / ( learns_here + 1.0);
	  this_doc_relative_len = unk_features / avg_features_per_doc;
	  // expected = 1 + this_doc_relative_len * avg_features_per_doc / 3.0;
	  // expected = 1 + this_doc_relative_len * avg_features_per_doc;
	  actual = totalhits[k];
	  chi2[k] = (expected - actual) * (expected - actual) / expected;
	  //     There's a real (not closed form) expression to
	  //     convert from chi2 values to probability, but it's
	  //     lame.  We'll approximate it as 2^-chi2.  Close enough
	  //     for government work.
	  ptc[k] = 1 / (pow (chi2[k], 2));
	  if (user_trace)
	    fprintf (stderr,
		     "CHI2: k: %ld, feats: %lf, learns: %lf, avg fea/doc: %lf, rel_len: %lf, exp: %ld, act: %lf, chi2: %lf, p: %lf\n",
		     k, features_here, learns_here,
		     avg_features_per_doc, this_doc_relative_len,
		     expected, actual, chi2[k], ptc[k] );
	};
    }

  //  One last chance to force probabilities into the non-stuck zone
  for (k = 0; k < maxhash; k++)
    if (ptc[k] < 1000 * DBL_MIN) ptc[k] = 1000 * DBL_MIN;

  //   and one last renormalize for both bayes and chisquared
  renorm = 0.0;
  for (k = 0; k < maxhash; k++)
    renorm = renorm + ptc[k];
  for (k = 0; k < maxhash; k++)
    ptc[k] = ptc[k] / renorm;

  if (user_trace)
    {
      for (k = 0; k < maxhash; k++)
	fprintf (stderr, "Probability of match for file %ld: %f\n", k, ptc[k]);
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
      buf [0] = '\000';
      accumulator = 1000 * DBL_MIN;
      for (m = 0; m < succhash; m++)
	{
	  accumulator = accumulator + ptc[m];
	};
      remainder = 1000 * DBL_MIN;
      for (m = succhash; m < maxhash; m++)
	{
	  remainder = remainder + ptc[m];
	};
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
	};
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
	strcat (stext, buf);
      bestseen = 0;
      for (k = 0; k < maxhash; k++)
	if (ptc[k] > ptc[bestseen] ) bestseen = k;
      remainder = 1000 * DBL_MIN;
      for (m = 0; m < maxhash; m++)
	if (bestseen != m)
	  {
	    remainder = remainder + ptc[m];
	  };
      sprintf (buf, "Best match to file #%ld (%s) "\
	       "prob: %6.4f  pR: %6.4f  \n",
	       bestseen,
	       hashname[bestseen],
	       ptc[bestseen],
	       (log10(ptc[bestseen]) - log10(remainder)));
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
	strcat (stext, buf);
      sprintf (buf, "Total features in input file: %ld\n", unk_features);
      if (strlen (stext) + strlen(buf) <= stext_maxlen)
	strcat (stext, buf);
      if (use_chisquared)
	{
	  for (k = 0; k < maxhash; k++)
	    {
	      long m;
	      remainder = 1000 * DBL_MIN;
	      for (m = 0; m < maxhash; m++)
		if (k != m)
		  {
		    remainder = remainder + ptc[m];
		  };
	      sprintf (buf,
		       "#%ld (%s):" \
		       " features: %u, hits: %ld,"   // exp: %ld,"
		       " chi2: %3.2e, pR: %6.2f \n",
		       k,
		       hashname[k],
		       hashes[k][features_index[k]].value,
		       totalhits[k],
		       //       expected,
		       chi2[k],
		       (log10 (ptc[k]) - log10 (remainder) )  );
	      // strcat (stext, buf);
	      if (strlen(stext)+strlen(buf) <= stext_maxlen)
		strcat (stext, buf);
	    };
	}
      else
	{
	  for (k = 0; k < maxhash; k++)
	    {
	      long m;
	      remainder = 1000 * DBL_MIN;
	      for (m = 0; m < maxhash; m++)
		if (k != m)
		  {
		    remainder = remainder + ptc[m];
		  };
	      sprintf (buf,
		       "#%ld (%s):"					\
		       " features: %u, hits: %ld, prob: %3.2e, pR: %6.2f \n",
		       k,
		       hashname[k],
		       hashes[k][features_index[k]].value,
		       totalhits[k],
		       ptc[k],
		       (log10 (ptc[k]) - log10 (remainder) )  );
	      // strcat (stext, buf);
	      if (strlen(stext)+strlen(buf) <= stext_maxlen)
		strcat (stext, buf);
	    };
	};

      // check here if we got enough room in stext to stuff everything
      // perhaps we'd better rise a nonfatalerror, instead of just
      // whining on stderr
      if (strcmp(&(stext[strlen(stext)-strlen(buf)]), buf) != 0)
        {
          nonfatalerror5
	    ( "WARNING: not enough room in the buffer to create "
	      "the statistics text.  Perhaps you could try bigger "
	      "values for MAX_CLASSIFIERS or MAX_FILE_NAME_LEN?",
	      " ", CRM_ENGINE_HERE);
	};
      crm_destructive_alter_nvariable (svrbl, svlen,
				       stext, strlen (stext));
    };


 done:
  //  cleanup time!

  free(features);
  if (use_unique)
    free(seen_features);

  //  remember to let go of the fd's and mmaps
  for (k = 0; k < maxhash; k++)
    {
      //      close (hfds [k]);
      crm_munmap_file ((void *) hashes[k]);
    };

  //
  //  Free the hashnames, to avoid a memory leak.
  //

  for (i = 0; i < maxhash; i++) {
    ///////////////////////////////////////
    //	  ! XXX SPAMNIX HACK!
    //!                         -- by Barry Jaspan
    //
    //! Without the statement "k = i" (which should have no effect),
    //! the for statement crashes on MacOS X when compiled with gcc
    //! -O3.  I've examined the pointers being freed, and they appear
    //! valid.  I've run this under Purify on Windows, valgrind on
    //! Linux, and efence on MacOS X; none report a problem here
    //! (though valgrind reports umrs in the VHT code; see my post to
    //! crm114-developers).  I've also examined the assembler produced
    //! with various changes here and, though I don't speak PPC, w/o
    //! the k = i it is qualitatively different.
    //!
    //! For now, I'm concluding it is an optimizer bug, and fixing it
    //! with the "k = i" statement.  This occurs on MacOS X 10.2 with
    //! Apple Computer, Inc. GCC version 1175, based on gcc version
    //! 3.1 20020420 (prerelease).
    //
    k = i;
    free (hashname[i]);
  }

  if (tprob <= 0.5000)
    {
      if (user_trace)
	fprintf (stderr, "CLASSIFY was a FAIL, skipping forward.\n");
      //    and do what we do for a FAIL here
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk [csl->mct[csl->cstmt]->nest_level] = -1;
      return (0);
    };


  //
  //   all done... if we got here, we should just continue execution
  if (user_trace)
    fprintf (stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");

  return (0);
};
