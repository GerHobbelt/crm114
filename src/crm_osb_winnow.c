//	crm_osb_winnow.c - OSB winnowing utilities

// Copyright 2001-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.
//
//  crm_osb_winnow.c  - Controllable Regex Mutilator,  version v1.0

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

// crm_vector_tokenize_selector() selects this.  We just know.
#define STRIDE 2

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

  // large thing too big for stack: feature hashes from vector tokenizer
  unsigned int *features;
  long features_out;
  long next_offset;
  long i, j, k;
  char ptext[MAX_PATTERN];  //  the regex pattern
  long plen;
  long winnow_file_length;
  char htext[MAX_PATTERN];  //  the hash name
  long hlen;
  struct stat statbuf;      //  for statting the hash file
  long hfsize;              //  size of the hash file
  char *fname;
  WINNOW_FEATUREBUCKET_STRUCT *hashes;  //  the text of the hash file
  //
  float sense;
  long microgroom;
  long fev;
  long made_new_file;



  if (internal_trace)
    fprintf (stderr, "executing an OSB-WINNOW LEARN\n");

  features = (unsigned int *)
    malloc(WINNOW_MAX_FEATURE_COUNT * STRIDE * sizeof(*features));
  if (features == NULL)
    untrappableerror5("Couldn't allocate feature hashes", "", CRM_ENGINE_HERE);

  //           extract the hash file name
  crm_get_pgm_arg (htext, MAX_PATTERN, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, MAX_PATTERN);

  //     get the "this is a word" regex
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);

  if (apb->sflags & CRM_NOCASE)
    {
      if (user_trace)
	fprintf (stderr, "turning on case-insensitive match\n");
    };

  sense = OSB_WINNOW_PROMOTION;
  if (apb->sflags & CRM_REFUTE)
    {
      sense = OSB_WINNOW_DEMOTION;
                    //  GROT GROT GROT Learning would be symmetrical
                    //  if this were
                    //       sense = 1.0 / sense;
                    //  but that's inferior, because then the weights are
                    // limited to the values of sense^n.
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

  if (apb->sflags & CRM_UNIGRAM)
    {
      if (user_trace)
	fprintf (stderr, " enabling unigram-only operation.\n");
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

  fname = strdup (&htext[i]);
  //             and stat it to get it's length
  k = stat (fname, &statbuf);

  made_new_file = 0;

  //             quick check- does the file even exist?
  if (k != 0)
    {
      //      file didn't exist... create it
      FILE *f;
      if (user_trace)
	fprintf (stderr, "\n Opening new COW file %s for write\n", fname);
      f = fopen (fname, "wb");
      if (!f)
	{
	  fprintf (stderr,
		"\n Couldn't open your new COW file %s for writing; errno=%d .\n",
		 fname, errno);
	  if (engine_exit_base != 0)
	    {
	      exit (engine_exit_base + 21);
	    }
	  else
	    exit (EXIT_FAILURE);
        };
      //       do we have a user-specified file size?
      winnow_file_length = sparse_spectrum_file_length;
      if (winnow_file_length == 0 ) {
        winnow_file_length =
	  DEFAULT_WINNOW_SPARSE_SPECTRUM_FILE_LENGTH;
      };

      //       put in winnow_file_length entries of NULL
      for (j = 0;
	   j < winnow_file_length
	     * sizeof ( WINNOW_FEATUREBUCKET_STRUCT);
	   j++)
	fputc ('\000', f);
      made_new_file = 1;
      //
      fclose (f);
      //    and reset the statbuf to be correct
      k = stat (fname, &statbuf);
    };
  //
  hfsize = statbuf.st_size;
  if (user_trace)
    fprintf (stderr, "Sparse spectra file %s has length %ld bins\n",
	     fname, hfsize / sizeof (WINNOW_FEATUREBUCKET_STRUCT));

  //
  //         open the .cow hash file into memory so we can bitwhack it
  //
  hashes = (WINNOW_FEATUREBUCKET_STRUCT *)
    crm_mmap_file (
		   fname,
		   0, hfsize,
		   PROT_READ | PROT_WRITE,
		   MAP_SHARED,
		   NULL);
  if (hashes == MAP_FAILED)
    {
      fev = fatalerror5 ("Couldn't memory-map the .cow file named: ",
			 fname, CRM_ENGINE_HERE);
      return (fev);
    };

  //
  //   now set the hfsize to the number of entries, not the number
  //   of bytes total
  hfsize = hfsize / sizeof ( WINNOW_FEATUREBUCKET_STRUCT );


  (void)crm_vector_tokenize_selector(apb,
				     txtptr, txtstart, txtlen,
				     ptext, plen,
				     NULL, 0, 0,
				     features, (long)(WINNOW_MAX_FEATURE_COUNT * STRIDE),
				     &features_out, &next_offset);

  // ??? error if features not big enough

  // Winnow always uniquifies
  {
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
  }


  //    and the big loop...

  for (i = 0; i + (STRIDE - 1) < features_out; i += STRIDE)
    {
      unsigned long h1, h2;
      unsigned long hindex;
      unsigned long incrs;


      h1 = features[i];
      h2 = features[i + 1];
      if (h2 == 0)
	h2 = 0xdeadbeef;
      hindex = h1 % hfsize;

      if (internal_trace)
	fprintf (stderr, "Polynomial %ld has h1:%ld  h2: %ld\n",
		 i, h1, h2);

      //
      //   we now look at both the primary (h1) and
      //   crosscut (h2) indexes to see if we've got
      //   the right bucket or if we need to look further
      //
      incrs = 0;
      //   while ( hashes[hindex].key != 0
      //	&&  ( hashes[hindex].hash != h1
      //	      || hashes[hindex].key  != h2 ))
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

	      //    and microgroom.
	      //fprintf (stderr,  "\nCalling microgroom hindex %ld hash: %ld  key: %ld  value: %f ",
	      //	hindex, hashes[hindex].hash, hashes[hindex].key, hashes[hindex].value );

	      crm_winnow_microgroom(hashes, NULL, hfsize, hindex);
	      incrs = 0;
	    };
	  //      check to see if we've incremented ourself all the
	  //      way around the .cow file.  If so, we're full, and
	  //      can hold no more features (this is unrecoverable)
	  if (incrs > hfsize - 3)
	    {
	      nonfatalerror5 ("Your program is stuffing too many "
			      "features into this size .cow file.  "
			      "Adding any more features is "
			      "impossible in this file.",
			      "You are advised to build a larger "
			      ".cow file and merge your data into "
			      "it.", CRM_ENGINE_HERE);
	      goto learn_end_feature_loop;
	    };
	  //
	  //     FINALLY!!!
	  //
	  //    This isn't the hash bucket we're looking for.  Move
	  //    along, move along....
	  incrs++;
	  hindex++;
	  if (hindex >= hfsize) hindex = 0;
	};

      if (internal_trace)
	{
	  if (hashes[hindex].value == 0)
	    {
	      fprintf (stderr,"New feature at %ld\n", hindex);
	    }
	  else
	    {
	      fprintf (stderr, "Old feature at %ld\n", hindex);
	    };
	};

      //      With _winnow_, we just multiply by the sense factor.
      //
      hashes[hindex].hash = h1;
      hashes[hindex].key  = h2;
      if (hashes[hindex].value > 0.0)
	{
	  hashes[hindex].value = hashes[hindex].value * sense;
	}
      else
	{
	  hashes[hindex].value = sense;
	};

      //		fprintf (stderr, "Hash index: %ld  value: %f \n", hindex, hashes[hindex].value);

    };

 learn_end_feature_loop:

  free(features);

  //  and remember to let go of the mmap
  // (and force a cache purge)
  // crm_munmap_all ();
  crm_munmap_file ((void *) hashes);

#ifndef CRM_WINDOWS
  //    Because mmap/alter/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is to do a trivial read/write on the .cow file, to force
  //    the filesystem to repropagate it's caches.
  //

  {
    int hfd;                  //  hashfile fd
    FEATURE_HEADER_STRUCT foo;

    hfd = open (fname, O_RDWR);
    dontcare = read (hfd, &foo, sizeof(foo));
    lseek (hfd, 0, SEEK_SET);
    dontcare = write (hfd, &foo, sizeof(foo));
    close (hfd);
  }
#endif	// !CRM_WINDOWS

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
  char fname[MAX_FILE_NAME_LEN];

  struct stat statbuf;      //  for statting the hash file

  // large thing too big for stack: feature hashes from vector tokenizer
  unsigned int *features;
  long features_out;
  long next_offset;

  double fcounts [MAX_CLASSIFIERS]; // total counts for feature normalize
  unsigned long totalcount = 0;

  double cpcorr[MAX_CLASSIFIERS];  // corpus correction factors
  double hits[MAX_CLASSIFIERS];  // actual hits per feature per classifier
  long totalhits[MAX_CLASSIFIERS];  // actual total hits per classifier
  double totalweights[MAX_CLASSIFIERS];  //  total of hits * weights
  double unseens[MAX_CLASSIFIERS]; //  total unseen features.
  double classifierprs[MAX_CLASSIFIERS]; //  pR's of each class
  long totalfeatures;   //  total features
  double htf;             // hits this feature got.
  double tprob = 0;         //  total probability in the "success" domain.

  WINNOW_FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
  long hashlens[MAX_CLASSIFIERS];
  char *hashname[MAX_CLASSIFIERS];
  long succhash;
  long vbar_seen;	// did we see '|' in classify's args?
  long maxhash;
  long fnstart, fnlen;
  long fn_start_here;
  long bestseen;

  double top10scores[10];
  long top10polys[10];
  char top10texts[10][MAX_PATTERN];


  if (internal_trace)
    fprintf (stderr, "executing an OSB-WINNOW CLASSIFY\n");

  features = (unsigned int *)
    malloc(WINNOW_MAX_FEATURE_COUNT * STRIDE * sizeof(*features));
  if (features == NULL)
    untrappableerror5("Couldn't allocate feature hashes", "", CRM_ENGINE_HERE);

  //           extract the hash file names
  crm_get_pgm_arg (htext, htext_maxlen, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar (htext, hlen, htext_maxlen);

  //           extract the "this is a word" regex
  //
  crm_get_pgm_arg (ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar (ptext, plen, MAX_PATTERN);
  if ( internal_trace)
    fprintf (stderr, "\nWordmatch pattern is %s", ptext);

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

  if (apb->sflags & CRM_UNIGRAM)
    {
      if (user_trace)
	fprintf (stderr, " enabling unigram-only operation.\n");
    };


  //       Now, the loop to open the files.
  bestseen = 0;

  //      initialize our arrays for N .css files
  for (i = 0; i < MAX_CLASSIFIERS; i++)
    {
      fcounts[i] = 0.0;    // check later to prevent a divide-by-zero
    			 // error on empty .css file
      cpcorr[i] = 0.0;   // corpus correction factors
      hits[i] = 0.0;     // absolute hit counts
      totalhits[i] = 0.0;        // absolute hit counts
      totalweights[i] = 0.0;     // hit_i * weight*i count
      unseens[i] = 0.0;       // text features not seen in statistics files
    };

  for (i = 0; i < 10; i++)
    {
      top10scores[i] = 0;
      top10polys[i] = 0;
      strcpy (top10texts[i], "");
    };
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

  //    now, get the file names and mmap each file
  //     get the file name (grody and non-8-bit-safe, but doesn't matter
  //     because the result is used for open() and nothing else.
  //   GROT GROT GROT  this isn't NULL-clean on filenames.  But then
  //    again, stdio.h itself isn't NULL-clean on filenames.
  if (user_trace)
    fprintf (stderr, "Classify list: -%s- \n", htext);
  fn_start_here = 0;
  fnlen = 1;
  while (fnlen > 0 && maxhash < MAX_CLASSIFIERS)
    {
      crm_nextword (htext, hlen, fn_start_here, &fnstart, &fnlen);
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
		  hashes[maxhash] = (WINNOW_FEATUREBUCKET_STRUCT *)
		    crm_mmap_file ( fname,
				    0, hashlens[maxhash],
				    PROT_READ,
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
		      hashlens[maxhash] = hashlens[maxhash] / sizeof (WINNOW_FEATUREBUCKET_STRUCT);
		      hashname[maxhash] = (char *) malloc (fnlen+10);
		      if (!hashname[maxhash])
			untrappableerror5 (
					   "Couldn't malloc hashname[maxhash]\n","We need that part later, so we're stuck.  Sorry.", CRM_ENGINE_HERE);
		      strncpy(hashname[maxhash],fname,fnlen);
		      hashname[maxhash][fnlen]='\000';
		      maxhash++;
		    };
		};
	    };
	  if (maxhash > MAX_CLASSIFIERS-1)
	    nonfatalerror5 ("Too many classifier files.",
			    "Some may have been disregarded",
			    CRM_ENGINE_HERE);
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
  //if (maxhash == 0)
  //  {
  //    fatalerror5 ("Couldn't open at least 1 .cow files for classify().",
  //		   "", CRM_ENGINE_HERE);
  //  };
  //	do we have at least 1 valid .cow file at both sides of '|'?
  //if (!vbar_seen || succhash < 0 || (maxhash < succhash + 2))
  //  {
  //    nonfatalerror (
  //      "Couldn't open at least 1 .css file per SUCC | FAIL classes "
  //	" for classify().\n","Hope you know what are you doing.");
  //  };

  // CLASSIFY with no arguments is "success" unless found insane above
  if (maxhash == 0)
    return (0);

  //      count up the total first
  for (i = 0; i < maxhash; i++)
    {
      fcounts[i] = 0.0 ;
      for (k = 1; k < hashlens[i]; k++)
	fcounts [i] = fcounts[i] + hashes[i][k].value;
      if (fcounts[i] == 0.0) fcounts[i] = 1.0 ;
      totalcount = totalcount + fcounts[i];
    };
  //
  //     calculate cpcorr (count compensation correction)
  //

  for (i = 0; i < maxhash; i++)
    {
      //  cpcorr [i] = ( totalcount / (fcounts[i] * (maxhash-1)));
      //
      //   disable cpcorr for now... unclear that it's useful.
      cpcorr[i] = 1.0;
    };

  //
  //   now all of the files are mmapped into memory,
  //   and we can do the polynomials and add up points.


  (void)crm_vector_tokenize_selector(apb,
				     txtptr, txtstart, txtlen,
				     ptext, plen,
				     NULL, 0, 0,
				     features, (long)(WINNOW_MAX_FEATURE_COUNT * STRIDE),
				     &features_out, &next_offset);

  // ??? error if features not big enough

  // Winnow always uniquifies
  {
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
  }

  //    and the big loop...

  totalfeatures = 0;

  for (i = 0; i + (STRIDE - 1) < features_out; i += STRIDE)
    {
      int k;
      unsigned int h1, h2;
      unsigned int hindex;

      h1 = features[i];
      h2 = features[i + 1];
      if (internal_trace)
	fprintf (stderr, "Polynomial %ld has h1:%d  h2: %d\n",
		 i, h1, h2);

      //    Now, for each of the feature files, what are
      //    the statistics (found, not found, whatever)
      //
      htf = 0;
      totalfeatures++;
      for (k = 0; k < maxhash; k++)
	{
	  long lh, lh0;
	  float z;

	  hindex = h1 % hashlens[k];

	  lh0 = lh = hindex;
	  hits[k] = 0;
	  while ( hashes[k][lh].key != 0
		  && ( hashes[k][lh].hash != h1
		       || hashes[k][lh].key  != h2 ))
	    {
	      lh++;
	      if (lh >= hashlens[k])	// wraparound
		lh = 0;
	      if (lh == lh0)		// tried whole file
		break;
	    };

	  //   Did we find the feature?  Or did we hit end-of-chain?
	  //
	  if (hashes[k][lh].hash == h1 && hashes[k][lh].key == h2)
	    {
	      //    found the feature
	      //
	      // remember totalhits
	      htf = htf + 1;            // and hits-this-feature
	      hits[k] ++;               // increment hits.
	      z = hashes[k][lh].value;
	      //		      fprintf (stdout, "L: %f  ", z);
	      // and weight sum
	      totalweights[k] = totalweights[k] + z;
	      totalhits[k] = totalhits[k] + 1;
	    }
	  else
	    {
	      // unseens score 1.0, which is totally ambivalent; seen
	      //  and accepted score more, seen and refuted score less
	      //
	      unseens[k] = unseens[k] + 1.0 ;
	      totalweights[k] = totalweights[k] + 1.0 ;
	    };
	};

      if (internal_trace)
	{
	  for (k = 0; k < maxhash; k++)
	    {
	      // fprintf (stderr, "ZZZ\n");
	      fprintf (stderr,
		       " poly: %ld  filenum: %d, HTF: %7.0f, hits: %7.0f, th: %10ld, tw: %6.4e\n",
		       i, k, htf, hits[k], totalhits[k], totalweights[k]);
	    };
	};
    };


  //  cleanup time!

  free(features);

  //  remember to let go of the fd's and mmaps
  for (k = 0; k < maxhash; k++)
    {
      crm_munmap_file ( (void *) hashes[k]);
    };

  if (user_trace)
    {
      for (k = 0; k < maxhash; k++)
	fprintf (stderr, "Match for file %ld:  hits: %ld  weight: %f\n",
		 k, totalhits[k], totalweights[k]);
    };

  // we're not through with hashnames yet

  //
  //      Do the calculations and format some output, which we may or may
  //      not use... but we need the calculated result anyway.
  //
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
	classifierprs[m] = 10*(log10 (totalweights[m])-log10(totalhits[m]));
      };
    for (m = 0; m < succhash; m++)
      {
	accumulator = accumulator + totalweights[m];
      };
    remainder = 10 * DBL_MIN;
    for (m = succhash; m < maxhash; m++)
      {
	remainder = remainder + totalweights[m];
      };

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
      };
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
	};

    //   ... and format some output of best single matching file
    //
    sprintf (buf, "Best match to file #%ld (%s) "\
	     "weight: %6.4f  pR: %6.4f  \n",
	     bestseen,
	     hashname[bestseen],
	     totalweights[bestseen],
	     classifierprs[bestseen]);
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
	    };
	sprintf (buf,
		 "#%ld (%s):"\
		 " features: %.2f, unseen: %3.2e, weight: %3.2e, pR: %6.2f \n",
		 k,
		 hashname[k],
		 fcounts[k],
		 unseens[k],
		 totalweights[k],
		 classifierprs[k]);
	// strcat (stext, buf);
	if (strlen(stext)+strlen(buf) <= stext_maxlen)
	  strcat (stext, buf);
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
    if (svlen > 0)
      crm_destructive_alter_nvariable (svrbl, svlen,
				       stext, strlen (stext));
  };

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
    };


  //
  //   all done... if we got here, we should just continue execution
  if (user_trace)
    fprintf (stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");

  return (0);
};
