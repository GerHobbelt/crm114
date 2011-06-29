//  crm_osb_hyperspace.c  - Controllable Regex Mutilator,  version v1.0
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



#if !defined (CRM_WITHOUT_OSB_HYPERSPACE)



//////////////////////////////////////////////////////////////////
//
//     Hyperspatial Classifiers
//
//     The current statistical classifiers like Markovian and OSB form
//     a single large cloud of points in hyperspace representing each
//     feature ever found in a sample document.  The known examples
//     are averaged together and so each class is a single large
//     cloud.  This is equivalent to having each class (possibly
//     containing hundreds of documents) averaging to a single point
//     in feature hyperspace, and the classifier code then picks which
//     of these averages is "closer" to the document.
//
//     A hyperspatial classifier is diffierent - it keeps the document
//     features separately rather than summing them all together.
//     Each known example document retains it's identity and so each
//     document acts independently as an example.
//
//     Evaluation:
//
//     1) Closeness: the single known example vector closest
//     to the unknown's vector is the winning class.  Closeness is
//     determined by Nth-root-of-sum-of-powers method
//
//     1A) Length Normalization: I.R. techniques suggest that
//     normalizing the known document vector lengths out to a unit
//     sphere of some relatively low dimensionality has a significant
//     accuracy advantage, especially for "closeness".  This is simply
//     the exponent used in the Pythagorean distance theorem.
//
//     2) Dominance: Look for how documents either dominate or are
//     dominated by (in the game-theory sense of a dominating
//     strategy) the unknown text.  Clearly, if a known document
//     dominates an unknown text, that text is of the document's
//     class.  If the unknown dominates a known text, then there is a
//     weaker implication of class membership.
//
//     2A) Dominance can be modulated by length normalization as well,
//     so a very long document that contains lots of features will not
//     dominate a short document very much at all.
//
//     3) Radiance:  use a non-linear match such as radiance to determine
//     distance to a class of N members.  Calculate by putting unit candles
//     on each known document vector endpoint; measure the power incident
//     at the unknown document's vector endpoint.  (that is, sum 1/R^2 of
//     document endpoints).  [[ this is different than closeness or dominance,
//     because all members of a class help a little, no matter how far
//     they are away.
//
//     3A) Raidiance with length normalization
//
//

////////////////////////////////////////////////////////////////////
//
//    Data storage format for hyperspace classifiers.
//
//     We sort the incoming features, so we can make linear rather than
//        random probes into the database, and can store individual
//        feature vectors sequentially (and save memory on vector IDs)
//
//     Because we don't merge the entire learning base together, we
//        drop down to 32-bit hashes, as the risk of a hash collision
//        within the much smaller single-document files is much lower
//        than it is in the big "all-together" hash files.
//
//
//    Option A1 - inline storage - store the (32-bit?  64-bit?) hash
//      codes as a sorted series.  Use hash code of 0x0 as a separator.
//      This doesn't allow merging of feature vectors.
//
//    Option A2 - inline valued - Store the 32-bit sorted hashes as
//      { hashcode, float_Weight } structs.  This allows merging of
//      multiple close features into a single feature vector when the
//      file gets too long. (foreach feature vec pair, measure dominant
//      overlaps or distances, and merge the two with either the closest
//      match or the smallest dominant overlap)
//
//    Option A3 - inline value with header count - Store the 32-bit
//      sorted hashes as { hashcode, float_Weight), and add total count
//      of merged vectors as 2nd value of header's 0x0 sentinel.  Merge
//      as before.
//
//    Option B1 - Keep the current "global" file, add a fourth slot
//      per bucket as the which-vector tag.   Does not support easy
//      merging.
//
//    Option B2 - Keep the current "global" file, add a fourth bitmapped
//      slot with 1 bit per vector (64? 128 bits?)  Merge vectors when you
//      run out of slots in the vector table.  (advantage- can have multi
//      classes in one table; put bitmap into the class. )  Advantage
//      of hashing- you only touch what you need.  Downside: Wastes most of the
//      storage in the bitmap.  Upside: can use bitmap to have multiple
//      text classes in the same file.
//
//    Option B3 - Use shared bit allocations, so classes use shared
//      bit patterns.  The bad news is that this generates phantom classes
//
//    Option C1 - use MySQL to do the storage.  Easy to implement
//
//    *****************************************************************
//
//      For now, we will KISS, till we see how well this actually
//      performs.
//
//    Option K1:   64-bit hashes, sorted, 0x0/0x0 sentinels between
//      class instances.  No class merging.  No weighting- if something
//      occurs twice, put in two entries.
//

typedef struct mythical_hyperspace_cell
{
  unsigned long hash;
} HYPERSPACE_FEATUREBUCKET_STRUCT;


////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static const long hctable[] =
{
  1, 7,
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



int hash_compare(void const *a, void const *b)
{
  HYPERSPACE_FEATUREBUCKET_STRUCT *pa, *pb;

  pa = (HYPERSPACE_FEATUREBUCKET_STRUCT *)a;
  pb = (HYPERSPACE_FEATUREBUCKET_STRUCT *)b;
  if (pa->hash < pb->hash)
    return -1;

  if (pa->hash > pb->hash)
    return 1;

  return 0;
}
//
//    How to learn Osb_Hyperspacestyle - in this case, we'll include
//    the single word terms that may not strictly be necessary.
//

int crm_expr_osb_hyperspace_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                                  char *txtptr, long txtstart, long txtlen)
{
  //     learn the osb_hyperspace transform  of this input window as
  //     belonging to a particular type.
  //     learn <flags> (classname) /word/
  //
  long i, j, k;
  long h;                    //  h is our counter in the hashpipe;
  char ptext[MAX_PATTERN];   //  the regex pattern
  long plen;
  char htext[MAX_PATTERN];          //  the hash name
  char hashfilename[MAX_PATTERN];   // the hashfile name
  FILE *hashf;                      // stream of the hashfile
  long hlen;
  long cflags, eflags;
  struct stat statbuf;                       //  for statting the hash file
  HYPERSPACE_FEATUREBUCKET_STRUCT *hashes;   //  the hashes we'll sort
  long hashcounts;
  crmhash_t hashpipe[OSB_BAYES_WINDOW_LEN + 1];
  //
  regex_t regcb;
  regmatch_t match[5];      //  we only care about the outermost match
  long textoffset;
  long textmaxoffset;
  long sense;
  long microgroom;
  long unique;
  long use_unigram_features;
  long fev;

  //  long made_new_file;
  //
  //  unsigned long learns_index = 0;
  //  unsigned long features_index = 0;

  statbuf.st_size = 0;
  fev = 0;

  if (internal_trace)
    fprintf(stderr, "executing a Hyperspace LEARN\n");

  //   Keep the gcc compiler from complaining about unused variables
  //  i = hctable[0];

  //           extract the hash file name
  crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar(htext, hlen, MAX_PATTERN);

  //     get the "this is a word" regex
  crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);

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
      fprintf(stderr, "turning oncase-insensitive match\n");
  }
  if (apb->sflags & CRM_REFUTE)
  {
    sense = -sense;
    /////////////////////////////////////
    //    Take this out when we finally support refutation
    ////////////////////////////////////
    //      fprintf(stderr, "Hyperspace Refute is NOT SUPPORTED YET\n");
    //return (0);
    if (user_trace)
      fprintf(stderr, " refuting learning\n");
  }
  microgroom = 0;
  if (apb->sflags & CRM_MICROGROOM)
  {
    microgroom = 1;
    if (user_trace)
      fprintf(stderr, " enabling microgrooming.\n");
  }
  unique = 0;
  if (apb->sflags & CRM_UNIQUE)
  {
    unique = 1;
    if (user_trace)
      fprintf(stderr, " enabling uniqueifying features.\n");
  }

  use_unigram_features = 0;
  if (apb->sflags & CRM_UNIGRAM)
  {
    use_unigram_features = 1;
    if (user_trace)
      fprintf(stderr, " using only unigram features.\n");
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
  strcpy(hashfilename, &htext[i]);

  //   Note that during a LEARN in hyperspace, we do NOT use the mmap of
  //    pre-existing memory.  We just write to the end of the file instead.
  //    malloc up the unsorted hashbucket space
  hashes = calloc(HYPERSPACE_MAX_FEATURE_COUNT,
                  sizeof(hashes[0]));
  hashcounts = 0;
  //  put in a zero as the start marker.
  hashes[hashcounts].hash = 0;
  hashcounts++;


  //   compile the word regex
  //
  if (internal_trace)
    fprintf(stderr, "\nWordmatch pattern is %s", ptext);
  i = crm_regcomp(&regcb, ptext, plen, cflags);
  if (i > 0)
  {
    crm_regerror(i, &regcb, tempbuf, data_window_size);
    nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
    goto regcomp_failed;
  }


  //   Start by priming the pipe... we will shift to the left next.
  //     sliding, hashing, xoring, moduloing, and incrmenting the
  //     hashes till there are no more.
  k = 0;
  j = 0;
  i = 0;


  //   No need to do any parsing of a box restriction.
  //   We got txtptr, txtstart, and txtlen from the caller.
  //
  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;


  //   init the hashpipe with 0xDEADBEEF
  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
  {
    hashpipe[h] = 0xDEADBEEF;
  }

  //    and the big feature-generator loop... go through all of the text.
  i = 0;
  while (k == 0 && textoffset <= textmaxoffset
         && hashcounts < HYPERSPACE_MAX_FEATURE_COUNT)
  {
    long wlen;
    long slen;

    //  do the regex
    //  slen = endpoint (= start + len)
    //        - startpoint (= curr textoffset)
    //      slen = txtlen;
    slen = textmaxoffset - textoffset;

    // if pattern is empty, extract non graph delimited tokens
    // directly ([[graph]]+) instead of calling regexec  (8% faster)
    if (ptext[0] != 0)
    {
      k = crm_regexec(&regcb, &(txtptr[textoffset]),
                      slen, 5, match, 0, NULL);
    }
    else
    {
      k = 0;
      //         skip non-graphical characthers
      match[0].rm_so = 0;
      while (!crm_isgraph(txtptr[textoffset + match[0].rm_so])
             && textoffset + match[0].rm_so < textmaxoffset)
        match[0].rm_so++;
      match[0].rm_eo = match[0].rm_so;
      while (crm_isgraph(txtptr[textoffset + match[0].rm_eo])
             && textoffset + match[0].rm_eo < textmaxoffset)
        match[0].rm_eo++;
      if (match[0].rm_so == match[0].rm_eo)
        k = 1;
    }

    if (k != 0 || textoffset > textmaxoffset)
      goto learn_end_regex_loop;

    wlen = match[0].rm_eo - match[0].rm_so;
    memmove(tempbuf,
            &(txtptr[textoffset + match[0].rm_so]),
            wlen);
    tempbuf[wlen] = 0;

    if (internal_trace)
    {
      fprintf(stderr,
              "  Learn #%ld t.o. %ld strt %ld end %ld len %ld is -%s-\n",
              i,
              textoffset,
              (long)match[0].rm_so,
              (long)match[0].rm_eo,
              wlen,
              tempbuf);
    }
    if (match[0].rm_eo == 0)
    {
      nonfatalerror("The LEARN pattern matched zero length! ",
                    "\n Forcing an increment to avoid an infinite loop.");
      match[0].rm_eo = 1;
    }


    //      Shift the hash pipe down one
    //
    for (h = OSB_BAYES_WINDOW_LEN - 1; h > 0; h--)
    {
      hashpipe[h] = hashpipe[h - 1];
    }


    //  and put new hash into pipeline
    hashpipe[0] = strnhash(tempbuf, wlen);

    if (internal_trace)
    {
      fprintf(stderr, "  Hashpipe contents: ");
      for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
        fprintf(stderr, " 0x%08lX", (unsigned long)hashpipe[h]);
      fprintf(stderr, "\n");
    }


    //  and account for the text used up.
    textoffset = textoffset + match[0].rm_eo;
    i++;

    //        is the pipe full enough to do the hashing?
    if (1)         //  we always run the hashpipe now, even if it's
                   //  just full of 0xDEADBEEF.  (was i >=5)
    {
      crmhash_t h1;
      crmhash_t h2;
      long th = 0;               // a counter used for TSS tokenizing
      long j;
      //
      //     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
      //     (coefficients chosen by requiring superincreasing,
      //     as well as prime)
      //
      th = 0;
      //
      if (use_unigram_features == 1)
      {
        h1 = hashpipe[0];
        if (h1 == 0) h1 = 0xdeadbeef;
        h2 = 0xdeadbeef;
        if (internal_trace)
          fprintf(stderr, "Singleton feature : 0x%08lX\n", (unsigned long)h1);
        hashes[hashcounts].hash = h1;
        hashcounts++;
      }
      else
      {
        for (j = 1;
             j < OSB_BAYES_WINDOW_LEN;
             j++)
        {
          h1 = hashpipe[0] * hctable[0] + hashpipe[j] * hctable[j << 1];
          if (h1 == 0) h1 = 0xdeadbeef;
          // h2 = hashpipe[0]*hctable[1] + hashpipe[j] * hctable[(j<<1)-1];
          //if (h2 == 0) h2 = 0xdeadbeef;
          h2 = 0xdeadbeef;
          if (internal_trace)
            fprintf(stderr, "Polynomial %ld has h1:0x%08lX  h2:0x%08lX\n",
                    j, (unsigned long)h1, (unsigned long)h2);

          hashes[hashcounts].hash = h1;
          //          hashes[hashcounts].key = h2;
          hashcounts++;
        }
      }
    }
  }     //   end the while k==0

  learn_end_regex_loop:

  if (ptext[0] != 0) crm_regfree(&regcb);

  regcomp_failed:

#if defined (GER)
  CRM_ASSERT(hashcounts >= 0);
  CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
  //mark the end of a feature vector
  hashes[hashcounts].hash = 0;

  //   Now sort the hashes array.
  //
  qsort(hashes, hashcounts,
        sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
        &hash_compare);

  if (user_trace)
    fprintf(stderr, "Total hashes generated: %ld\n", hashcounts);

  //   And uniqueify the hashes array
  //

  if (unique)
  {
    i = 0;
    j = 0;

    CRM_ASSERT(hashcounts >= 0);
    CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
    CRM_ASSERT(hashes[hashcounts].hash == 0);

    while (i < hashcounts)
    {
      if (hashes[i].hash != hashes[i + 1].hash)
      {
        hashes[j] = hashes[i];
        j++;
      }
      i++;
    }
    hashcounts = j;

    //mark the end of a feature vector
    hashes[hashcounts].hash = 0;
  }

  CRM_ASSERT(hashcounts >= 0);
  CRM_ASSERT(hashcounts < HYPERSPACE_MAX_FEATURE_COUNT);
  CRM_ASSERT(hashes[hashcounts].hash == 0);
#else

  //   Now sort the hashes array.
  //
  qsort(hashes, hashcounts,
        sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
        &hash_compare);

  hashcounts--;
  if (user_trace)
    fprintf(stderr, "Total hashes generated: %ld\n", hashcounts);

  //   And uniqueify the hashes array
  //

  i = 1;
  j = 1;

  if (unique)
  {
    while (i <= hashcounts)
    {
      if (hashes[i].hash != hashes[i + 1].hash
          //      || hashes[i].key != hashes[i+1].key )
      )
      {
        hashes[j] = hashes[i];
        j++;
      }
      i++;
    }
    hashcounts = j;
  }
#endif
  if (user_trace)
    fprintf(stderr, "Unique hashes generated: %ld\n", hashcounts);
  //    store hash count of this document in the first bucket's .key slot
  //  hashes[hashcounts].key = hashcounts;


  if (sense > 0)
  {
    /////////////////
    //    THIS PATH TO LEARN A TEXT - just append the hashes.
    //     and open the output file
    /////////////////

    //  Now a nasty bit.  Because there are probably retained hashes of the
    //  file, we need to force an unmap-by-name which will allow a remap
    //  with the new file length later on.
    crm_force_munmap_filename(hashfilename);

    if (user_trace)
      fprintf(stderr, "Opening hyperspace file %s for append.\n",
              hashfilename);
    hashf = fopen(hashfilename, "ab+");
    if (hashf == 0)
    {
      fatalerror("For some reason, I was unable to append-open the file named ",
                 hashfilename);
    }
    else
    {
      int ret;

      if (user_trace)
        fprintf(stderr, "Writing to hash file %s\n", hashfilename);
      //    and write the sorted hashes out.
      ret = fwrite(hashes, sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
                   hashcounts,       /* [i_a] GROT GROT GROT shouldn't this be 'hashcounts+1', just like SVM/SKS? */
                   hashf);
      if (ret != hashcounts)
      {
        fatalerror("For some reason, I was unable to append a hash series to the file named ",
                   hashfilename);
      }
      fclose(hashf);
    }

    //  let go of the hashes.
    free(hashes);
  }
  else
  {
    /////////////////
    //     THIS IS THE UNLEARN PATH.  VERY, VERY MESSY
    //     What we have to do here is find the set of hashes that matches
    //     the input most closely - and then remove it.
    //
    //     For this, we want the single closest set of hashes.  That
    //     implies highest radiance, so we use the same bit of code
    //     we use down in classification.  We also keep start and
    //     end of the "best match" segment.
    /////////////////

    long beststart, bestend;
    long thisstart, thislen, thisend;
    double bestrad;
    long wrapup;
#if defined (GER)
    long kandu;
    long unotk, knotu;
#else
    double kandu;
    double unotk, knotu;
#endif
    double dist, radiance;
    long k, u;
    long file_hashlens;
    HYPERSPACE_FEATUREBUCKET_STRUCT *file_hashes;

    //   Get the file mmapped so we can find the closest match
    //
    {
      struct stat statbuf;          //  for statting the hash file

      //             stat the file to get it's length
      k = stat(hashfilename, &statbuf);

      //              does the file really exist?
      if (k != 0)
      {
        nonfatalerror("Refuting from nonexistent data cannot be done!"
                      " More specifically, this data file doesn't exist: ",
                      hashfilename);
        return 0;
      }
      else
      {
        file_hashlens = statbuf.st_size;
        file_hashes = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
                      crm_mmap_file(hashfilename,
                                    0, file_hashlens,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    NULL);
        file_hashlens = file_hashlens
                        / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
      }
    }

    wrapup = 0;

    k = u = 0;
    beststart = bestend = 0;
    bestrad = 0.0;
    while (k < file_hashlens)
    {
      long cmp;
      //   Except on the first iteration, we're looking one cell
      //   past the 0x0 start marker.
      kandu = 0;
      knotu = unotk = 10;
      u = 0;
      thisstart = k;
      if (internal_trace)
        fprintf(stderr,
                "At featstart, looking at %ld (next bucket value is %ld)\n",
                file_hashes[thisstart].hash,
                file_hashes[thisstart + 1].hash);
      while (wrapup == 0)
      {
        //    it's an in-class feature.
        cmp = hash_compare(&hashes[u], &file_hashes[k]);
        if (cmp < 0)
        {
          // unknown less, step u forward
          //   increment on u, because maybe k will match next time
          unotk++;
          u++;
        }
        if (cmp == 0)          // features matched.
        //   These aren't the features you're looking for.
        //   Move along, move along....
        {
          u++;
          k++;
          kandu++;
        }
        if (cmp > 0)          // unknown is greater, step k forward
        {
          //  increment on k, because maybe u will match next time.
          knotu++;
          k++;
        }
        //   End of the U's?  If so, skip k to the end marker
        //    and finish.
        if (u >= hashcounts - 1)
        {
          while (k < file_hashlens
                 && file_hashes[k].hash != 0)
          {
            k++;
            knotu++;
          }
        }
        //   End of the K's?  If so, skip U to the end marker
        if (k >= file_hashlens - 1
            || file_hashes[k].hash == 0)             //  end of doc features
        {
          unotk += hashcounts - u;
        }

        //    end of the U's or end of the K's?  If so, end document.
        if (u >= hashcounts - 1
            || k >= file_hashlens - 1
            || file_hashes[k].hash == 0)          // this sets end-of-document
        {
          wrapup = 1;
          k++;
        }
      }

      //  Now the per-document wrapup...
      wrapup = 0;                         // reset wrapup for next file

      //   drop our markers for this particular document.  We are now
      //   looking at the next 0 (or end of file).
      thisend = k - 2;
      thislen = thisend - thisstart;
      if (internal_trace)
        fprintf(stderr,
                "At featend, looking at %ld (next bucket value is %ld)\n",
                file_hashes[thisend].hash,
                file_hashes[thisend + 1].hash);

      //  end of a document- process accumulations

      //    Proper pythagorean (Euclidean) distance - best in
      //   SpamConf 2006 paper
#if defined (GER)
      dist = sqrt(unotk + knotu);
#else
      dist = sqrtf(unotk + knotu);
#endif

      // PREV RELEASE VER --> radiance = 1.0 / ((dist * dist )+ 1.0);
      //
      //  This formula was the best found in the MIT `SC 2006 paper.
      radiance = 1.0 / ((dist * dist) + .000001);
      radiance = radiance * kandu;
      radiance = radiance * kandu;
      //radiance = radiance * kandu;

      if (user_trace)
        fprintf(stderr, "Feature Radiance %f at %ld to %ld\n",
                radiance, thisstart, thisend);
      if (radiance >= bestrad)
      {
        beststart = thisstart;
        bestend = thisend;
        bestrad = radiance;
      }
    }
    //  end of the per-document stuff - now chop out the part of the
    //  file between beststart and bestend.

    //      if (user_trace)
    fprintf(stderr,
            "Deleting feature from %ld to %ld (rad %f) of file %s\n",
            beststart, bestend, bestrad, hashfilename);

    //   Deletion time - move the remaining stuff in the file
    //   up to fill the hole, then msync the file, munmap it, and
    //   then truncate it to the new, correct length.
    {
      long newhashlen, newhashlenbytes;
      newhashlen = file_hashlens - (bestend + 1 - beststart);
      newhashlenbytes = newhashlen * sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);

      memmove(&file_hashes[beststart],
              &file_hashes[bestend + 1],
              sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT)
              * (file_hashlens - bestend));
      memset(&file_hashes[file_hashlens - (bestend - beststart)],
             0,
             sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT));
      crm_force_munmap_filename(hashfilename);

      if (internal_trace)
        fprintf(stderr, "Truncating file to %ld cells ( %ld bytes)\n",
                newhashlen,
                newhashlenbytes);
      k = truncate(hashfilename,
                   newhashlenbytes);
      //      fprintf(stderr, "Return from truncate is %ld\n", k);
    }
  }
  // end of deletion path.
  return 0;
}


//      How to do a Osb_Bayes CLASSIFY some text.
//
int crm_expr_osb_hyperspace_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                                     char *txtptr, long txtstart, long txtlen)
{
  //      classify the sparse spectrum of this input window
  //      as belonging to a particular type.
  //
  //       This code should look very familiar- it's cribbed from
  //       the code for LEARN
  //
  long i, j, k;
  long h;                    //  we use h for our hashpipe counter, as needed.
  char ptext[MAX_PATTERN];   //  the regex pattern
  long plen;
  //  char ltext[MAX_PATTERN];  //  the variable to classify
  //long llen;
  //  the hash file names
  char htext[MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN];
  long htext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN;
  long hlen;
  //  the match statistics variable
  char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
  long stext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);

  long slen;
  char svrbl[MAX_PATTERN];   //  the match statistics text buffer
  long svlen;
  long fnameoffset;
  char fname[MAX_FILE_NAME_LEN];
  long eflags;
  long cflags;
  long use_unique;
  long not_microgroom = 1;
  long use_unigram_features;

  //  The hashes we'll generate from the unknown text - where and how many.
  HYPERSPACE_FEATUREBUCKET_STRUCT *unk_hashes;
  long unk_hashcount;

  struct stat statbuf;      //  for statting the hash file
  crmhash_t hashpipe[OSB_BAYES_WINDOW_LEN + 1];
  regex_t regcb;
  regmatch_t match[5];      //  we only care about the outermost match

#if defined (GER)
  hitcount_t totalhits[MAX_CLASSIFIERS];   // actual total hits per classifier
#else
  long totalhits[MAX_CLASSIFIERS];   // actual total hits per classifier
#endif
  long totalfeatures;                      //  total features
  double tprob;                            //  total probability in the "success" domain.

  double ptc[MAX_CLASSIFIERS];   // current running probability of this class

  HYPERSPACE_FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
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

  long cls;
  long nfeats;    //  total features
  long ufeats;    // features in this unknown
  long kfeats;    // features in the known

  //     Basic match parameters
  //     These are computed intra-document, other stuff is only done
  //     at the end of the document.
#if defined (GER)
  long knotu;   // features in known doc, not in unknown
  long unotk;   // features in unknown doc, not in known
  long kandu;   // feature in both known and unknown

  //     Distance is the pythagorean distance (sqrt) between the
  //     unknown and a known-class text; we choose closest.  (this
  //     is (for each U and K feature, SQRT of count of U ~K + K ~ U)
  double dist;
  double closest_dist[MAX_CLASSIFIERS];
  double closest_normalized[MAX_CLASSIFIERS];
#else
  float knotu;   // features in known doc, not in unknown
  float unotk;   // features in unknown doc, not in known
  float kandu;   // feature in both known and unknown

  //     Distance is the pythagorean distance (sqrt) between the
  //     unknown and a known-class text; we choose closest.  (this
  //     is (for each U and K feature, SQRT of count of U ~K + K ~ U)
  float dist;
  float closest_dist[MAX_CLASSIFIERS];
  float closest_normalized[MAX_CLASSIFIERS];
#endif

  //#define KNN_ON
#ifdef KNN_ON
#define KNN_NEIGHBORHOOD_SIZE 21
  double top_n_val[KNN_NEIGHBORHOOD_SIZE];
  long top_n_class[KNN_NEIGHBORHOOD_SIZE];
#endif

  //      The collapse vector is a low-dimensioned hyperspace
  //      that uses the low-order N bits of the hash to
  //      collapse the 2^32 dimensions into a reasonable space..
  //
  long collapse_vec_same[256];
  long collapse_vec_diff[256];

  //  Dominance and Submission are related to Distance:
  //  -  Dominance is per-known - how many of the features of the
  //    unknown also exist in the known (for each U, count of K)
  //  -  Submission is how many of the features of the unknown do NOT
  //    exist in the known.  (for each U, count of ~K)
  //  -- Dominance minus Submission is a figure of merit of match.
#if defined (GER)
  double max_dominance[MAX_CLASSIFIERS];
  double dominance_normalized[MAX_CLASSIFIERS];
  double max_submission[MAX_CLASSIFIERS];
  double submission_normalized[MAX_CLASSIFIERS];
  double max_equivalence[MAX_CLASSIFIERS];
  double equivalence_normalized[MAX_CLASSIFIERS];
  double max_des[MAX_CLASSIFIERS];
  double des_normalized[MAX_CLASSIFIERS];


  //     Radiance - sum of the 1/r^2 radiances of each known text
  //     onto the unknown.  Unlike Distance and Dominance, Radiance
  //     is a function of an entire class, not of a single example
  //     in the class.   More radiance is a closer match.
  //     Flux is like Radiance, but the standard unit candle at each text
  //     is replaced by a flux source of intensity proportional to the
  //     number of features in the known text.
  double radiance;
  double class_radiance[MAX_CLASSIFIERS];
  double class_radiance_normalized[MAX_CLASSIFIERS];
  double class_flux[MAX_CLASSIFIERS];
  double class_flux_normalized[MAX_CLASSIFIERS];
#else
  float max_dominance[MAX_CLASSIFIERS];
  float dominance_normalized[MAX_CLASSIFIERS];
  float max_submission[MAX_CLASSIFIERS];
  float submission_normalized[MAX_CLASSIFIERS];
  float max_equivalence[MAX_CLASSIFIERS];
  float equivalence_normalized[MAX_CLASSIFIERS];
  float max_des[MAX_CLASSIFIERS];
  float des_normalized[MAX_CLASSIFIERS];


  //     Radiance - sum of the 1/r^2 radiances of each known text
  //     onto the unknown.  Unlike Distance and Dominance, Radiance
  //     is a function of an entire class, not of a single example
  //     in the class.   More radiance is a closer match.
  //     Flux is like Radiance, but the standard unit candle at each text
  //     is replaced by a flux source of intensity proportional to the
  //     number of features in the known text.
  float radiance;
  float class_radiance[MAX_CLASSIFIERS];
  float class_radiance_normalized[MAX_CLASSIFIERS];
  float class_flux[MAX_CLASSIFIERS];
  float class_flux_normalized[MAX_CLASSIFIERS];
#endif

  //     try using just the top n matches
  //  for thk=0.1
  //     N=1 --> 4/500, N=2 --> 8/500, N=3--> 8 (same exact!), N=4-->8 (same)
  //     N=8--> 8 (same), N=32--> 8 (same) N=128 -->8 (same) N=1024-->8(same)
  //  for thk=0.5
  //#define TOP_N 4
  //float topn[MAX_CLASSIFIERS][TOP_N];

  if (internal_trace)
    fprintf(stderr, "executing a CLASSIFY\n");

  //        make the space for the unknown text's hashes
  unk_hashes = calloc(HYPERSPACE_MAX_FEATURE_COUNT,
                      sizeof(unk_hashes[0]));
  unk_hashcount = 0;
#if defined (GER)
  // unk_hashcount++;
#else
  unk_hashcount++;
#endif

  //           extract the variable name (if present)
  //    (we now get those fromt he caller)

  //           extract the hash file names
  crm_get_pgm_arg(htext, htext_maxlen, apb->p1start, apb->p1len);
  hlen = apb->p1len;
  hlen = crm_nexpandvar(htext, hlen, htext_maxlen);

  //           extract the "this is a word" regex
  //
  crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
  plen = apb->s1len;
  plen = crm_nexpandvar(ptext, plen, MAX_PATTERN);

  //            extract the optional "match statistics" variable
  //
  crm_get_pgm_arg(svrbl, MAX_PATTERN, apb->p2start, apb->p2len);
  svlen = apb->p2len;
  svlen = crm_nexpandvar(svrbl, svlen, MAX_PATTERN);
  {
    long vstart, vlen;
    crm_nextword(svrbl, svlen, 0, &vstart, &vlen);
    memmove(svrbl, &svrbl[vstart], vlen);
    svlen = vlen;
    svrbl[vlen] = 0;
  }
  if (user_trace)
    fprintf(stderr, "Status out var %s (len %ld)\n",
            svrbl, svlen);

  //     status variable's text (used for output stats)
  //
  stext[0] = 0;
  slen = 0;

  //            set our flags, if needed.  The defaults are
  //            "case"
  cflags = REG_EXTENDED;
  eflags = 0;

  if (apb->sflags & CRM_NOCASE)
  {
    if (user_trace)
      fprintf(stderr, " setting NOCASE for tokenization\n");
    cflags += REG_ICASE;
    eflags = 1;
  }

  not_microgroom = 1;
  if (apb->sflags & CRM_MICROGROOM)
  {
    not_microgroom = 0;
    if (user_trace)
      fprintf(stderr, " disabling fast-skip optimization.\n");
  }

  use_unique = 0;
  if (apb->sflags & CRM_UNIQUE)
  {
    use_unique = 1;
    if (user_trace)
      fprintf(stderr, " unique engaged - repeated features are ignored \n");
  }

  use_unigram_features = 0;
  if (apb->sflags & CRM_UNIGRAM)
  {
    use_unigram_features = 1;
    if (user_trace)
      fprintf(stderr, " using only unigram features. \n");
  }

  //   compile the word regex
  if (internal_trace)
    fprintf(stderr, "\nWordmatch pattern is %s", ptext);
  i = crm_regcomp(&regcb, ptext, plen, cflags);
  if (i > 0)
  {
    crm_regerror(i, &regcb, tempbuf, data_window_size);
    nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
    goto regcomp_failed;
  }



  //       Now, the loop to open the files.
  bestseen = 0;
  thistotal = 0;

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
    fprintf(stderr, "Classify list: -%s-\n", htext);
  fn_start_here = 0;
  fnlen = 1;

  while (fnlen > 0 && ((maxhash < MAX_CLASSIFIERS - 1)))
  {
    crm_nextword(htext,
                 hlen, fn_start_here,
                 &fnstart, &fnlen);
    if (fnlen > 0)
    {
      strncpy(fname, &htext[fnstart], fnlen);
      fname[fnlen] = 0;
      //      fprintf(stderr, "fname is '%s' len %ld\n", fname, fnlen);
      fn_start_here = fnstart + fnlen + 1;
      if (user_trace)
        fprintf(stderr, "Classifying with file -%s- "
                        "succhash=%ld, maxhash=%ld\n",
                fname, succhash, maxhash);
      if (fname[0] == '|' && fname[1] == 0)
      {
        if (vbar_seen)
        {
          nonfatalerror("Only one ' | ' allowed in a CLASSIFY. \n",
                        "We'll ignore it for now.");
        }
        else
        {
          succhash = maxhash;
        }
        vbar_seen++;
      }
      else
      {
        //  be sure the file exists
        //             stat the file to get it's length
        k = stat(fname, &statbuf);
        //             quick check- does the file even exist?
        if (k != 0)
        {
          nonfatalerror("Nonexistent Classify table named: ",
                        fname);
        }
        else
        {
          //  file exists - do the mmap
          //
          hashlens[maxhash] = statbuf.st_size;

          hashes[maxhash] = (HYPERSPACE_FEATUREBUCKET_STRUCT *)
                            crm_mmap_file(fname,
                                          0, hashlens[maxhash],
                                          PROT_READ,
                                          MAP_SHARED,
                                          NULL);

          if (hashes[maxhash] == MAP_FAILED)
          {
            nonfatalerror("Couldn't memory-map the table file :",
                          fname);
          }
          else
          {
            //
            //     Check to see if this file is the right version
            //
            //     FIXME : for now, there's no version number
            //     associated with a .correllation file
            // long fev;
            // if (0)
            //              if (hashes[maxhash][0].hash != 0 ||
            //          hashes[maxhash][0].key  != 0)
            //        {
            //          fev =fatalerror ("The .css file is the wrong version!  Filename is: ",
            //                           fname);
            //          return (fev);
            //        }

            //    read it in (this does not work either);
            if (0)
            {
              FILE *hashf;
              if (user_trace)
                fprintf(stderr, "Read-opening file %s\n", fname);
              hashf = fopen(fname, "rb");
              if (hashf == 0)
              {
                fatalerror("For some reason, I was unable to read-open the file named ",
                           fname);
              }
              else
              {
                fread(hashes[maxhash], 1, hashlens[maxhash], hashf);
                fclose(hashf);
              }
            }

            //  set this hashlens to the length in features instead
            //  of the length in bytes.
            hashlens[maxhash] = hashlens[maxhash]
                                / sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT);
            //
            //     save the name for later...
            //
            hashname[maxhash] = (char *)calloc((fnlen + 10), sizeof(hashname[maxhash][0]));
            if (!hashname[maxhash])
            {
              untrappableerror(
                "Couldn't alloc hashname[maxhash]\n", "We need that part later, so we're stuck.  Sorry.");
            }
            else
            {
              strncpy(hashname[maxhash], fname, fnlen);
              hashname[maxhash][fnlen] = 0;
            }
            maxhash++;
          }
        }
      }

      if (maxhash > MAX_CLASSIFIERS - 1)
        nonfatalerror("Too many classifier files.",
                      "Some may have been disregarded");
    }
  }



  //
  //    If there is no '|', then all files are "success" files.
  if (succhash == 0)
    succhash = maxhash;

  //    now, set up the normalization factor fcount[]
  if (user_trace)
    fprintf(stderr, "Running with %ld files for success out of %ld files\n",
            succhash, maxhash);

  // sanity checks...  Uncomment for super-strict CLASSIFY.
  //
  //    do we have at least 1 valid .css files?
  if (maxhash == 0)
  {
    nonfatalerror("Couldn't open at least 1 .css files for classify().", "");
  }

  //    a CLASSIFY with no arguments is always a "success".
  if (maxhash == 0)
    return 0;

  //    do we have at least 1 valid .css file at both sides of '|'?
  if (!vbar_seen || succhash < 0 || (maxhash <= succhash))
  {
    nonfatalerror(
      "Couldn't open at least 1 .css file per SUCC | FAIL category "
      "for classify().\n", "Hope you know what are you doing.");
  }

  //
  //   now all of the files are mmapped into memory,
  //   and we can do the polynomials and add up points.
  i = 0;
  j = 0;
  k = 0;
  thistotal = 0;

  //   we get txtstart and txtlen from the caller.

  textoffset = txtstart;
  textmaxoffset = txtstart + txtlen;

  //   init the hashpipe with 0xDEADBEEF
  for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
  {
    hashpipe[h] = 0xDEADBEEF;
  }

  totalfeatures = 0;

  //  stop when we no longer get any regex matches
  //   possible edge effect here- last character must be matchable, yet
  //    it's also the "end of buffer".
  while (k == 0 && textoffset <= textmaxoffset
         && unk_hashcount < HYPERSPACE_MAX_FEATURE_COUNT)
  {
    long wlen;
    long slen;

    //  do the regex
    //
    slen = textmaxoffset - textoffset;

    // if pattern is empty, extract non graph delimited tokens
    // directly ([[graph]]+) instead of calling regexec  (8% faster)
    if (ptext[0] != 0)
    {
      k = crm_regexec(&regcb, &(txtptr[textoffset]),
                      slen, 5, match, 0, NULL);
    }
    else
    {
      k = 0;
      //         skip non-graphical characters
      match[0].rm_so = 0;
      while (!crm_isgraph(txtptr[textoffset + match[0].rm_so])
             && textoffset + match[0].rm_so < textmaxoffset)
        match[0].rm_so++;
      match[0].rm_eo = match[0].rm_so;
      while (crm_isgraph(txtptr[textoffset + match[0].rm_eo])
             && textoffset + match[0].rm_eo < textmaxoffset)
        match[0].rm_eo++;
      if (match[0].rm_so == match[0].rm_eo)
        k = 1;
    }

    if (k != 0 || textoffset > textmaxoffset)
      goto classify_end_regex_loop;

    wlen = match[0].rm_eo - match[0].rm_so;
    memmove(tempbuf,
            &(txtptr[textoffset + match[0].rm_so]),
            wlen);
    tempbuf[wlen] = 0;

    if (internal_trace)
    {
      fprintf(stderr,
              "  Classify #%ld t.o. %ld strt %ld end %ld len %ld is -%s-\n",
              i,
              textoffset,
              (long)match[0].rm_so,
              (long)match[0].rm_eo,
              wlen,
              tempbuf);
    }
    if (match[0].rm_eo == 0)
    {
      nonfatalerror("The CLASSIFY pattern matched zero length! ",
                    "\n Forcing an increment to avoid an infinite loop.");
      match[0].rm_eo = 1;
    }
    //  slide previous hashes up 1
    for (h = OSB_BAYES_WINDOW_LEN - 1; h > 0; h--)
    {
      hashpipe[h] = hashpipe[h - 1];
    }


    //  and put new hash into pipeline
    hashpipe[0] = strnhash(tempbuf, wlen);

    if (0)
    {
      fprintf(stderr, "  Hashpipe contents: ");
      for (h = 0; h < OSB_BAYES_WINDOW_LEN; h++)
        fprintf(stderr, " 0x%08lX", (unsigned long)hashpipe[h]);
      fprintf(stderr, "\n");
    }

    //   account for the text we used up...
    textoffset = textoffset + match[0].rm_eo;
    i++;

    //        is the pipe full enough to do the hashing?
    if (1)     //  we init with 0xDEADBEEF, so the pipe is always full (i >=5)
    {
      int j;
      unsigned th = 0;            //  a counter used only in TSS hashing
      crmhash_t hindex;
      crmhash_t h1;
      //
      th = 0;
      //
      if (use_unigram_features == 1)
      {
        h1 = hashpipe[0];
        if (h1 == 0) h1 = 0xdeadbeef;
        if  (internal_trace)
          fprintf(stderr, "Singleton feature : 0x%08lX\n", (unsigned long)h1);
        unk_hashes[unk_hashcount].hash = h1;
        unk_hashcount++;
      }
      else
      {
        for (j = 1;
             j < OSB_BAYES_WINDOW_LEN;
             j++)
        {
          h1 = hashpipe[0] * hctable[0] + hashpipe[j] * hctable[j << 1];
          if (h1 == 0) h1 = 0xdeadbeef;
          //              h2 = hashpipe[0]*hctable[1] + hashpipe[j] * hctable[(j<<1)-1];
          //if (h2 == 0) h2 = 0xdeadbeef;
          hindex = h1;

          if (internal_trace)
            fprintf(stderr, "Polynomial %d has h1:0x%08lX\n",
                    j, (unsigned long)h1);

          unk_hashes[unk_hashcount].hash = h1;
          unk_hashcount++;
        }
      }
    }
  }        //  end of repeat-the-regex loop
  classify_end_regex_loop:

  ////////////////////////////////////////////////////////////
  //
  //     We now have the features.  sort the unknown feature array so
  //     we can do fast comparisons against each document's hashes in
  //     the hyperspace vector files.

#if defined (GER)
  CRM_ASSERT(unk_hashcount >= 0);
  CRM_ASSERT(unk_hashcount < HYPERSPACE_MAX_FEATURE_COUNT);
  //mark the end of a feature vector
  unk_hashes[unk_hashcount].hash = 0;


  qsort(unk_hashes, unk_hashcount, sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
        &hash_compare);

  if (user_trace)
    fprintf(stderr, "Total hashes in the unknown text: %ld\n", unk_hashcount);


  //       uniqueify the hashes array.
  if (use_unique)
  {
    i = 1;
    j = 1;

    CRM_ASSERT(unk_hashcount >= 0);
    CRM_ASSERT(unk_hashcount < HYPERSPACE_MAX_FEATURE_COUNT);
    CRM_ASSERT(unk_hashes[unk_hashcount].hash == 0);
    while (i < unk_hashcount)
    {
      if (unk_hashes[i].hash != unk_hashes[i + 1].hash)
      {
        unk_hashes[j] = unk_hashes[i];
        j++;
      }
      i++;
    }
    unk_hashcount = j;

    //mark the end of a feature vector
    unk_hashes[unk_hashcount].hash = 0;
  }

  CRM_ASSERT(unk_hashcount >= 0);
  CRM_ASSERT(unk_hashcount < HYPERSPACE_MAX_FEATURE_COUNT);
  CRM_ASSERT(unk_hashes[unk_hashcount].hash == 0);
#else
  qsort(unk_hashes, unk_hashcount, sizeof(HYPERSPACE_FEATUREBUCKET_STRUCT),
        &hash_compare);

  unk_hashcount--;
  if (user_trace)
    fprintf(stderr, "Total hashes in the unknown text: %ld\n", unk_hashcount);



  //       uniqueify the hashes array.
  i = 1;
  j = 1;
  if (use_unique)
  {
    while (i <= unk_hashcount)
    {
      if (unk_hashes[i].hash != unk_hashes[i + 1].hash
          //              || unk_hashes[i].key != unk_hashes[i+1].key)
      )
      {
        unk_hashes[j] = unk_hashes[i];
        j++;
      }
      i++;
    }
    j--;
    unk_hashcount = j;
  }
#endif

  if (user_trace)
    fprintf(stderr, "unique hashes generated: %ld\n", unk_hashcount);

  totalfeatures = unk_hashcount;

  //     Now we have the uniqueified feature hashes of the unknown text
  //     ready for matching.  For now, we will match with simple closeness,
  //     dominance, and radiosity but eventually we'll figure out what works
  //     best.
  //
  {
    //   Initialize this mess.
    for (i = 0; i < MAX_CLASSIFIERS; i++)
    {
      closest_dist[i] = 1000000000.0;
      closest_normalized[i] = 1000000000.0;
      max_dominance[i] = 0.0;
      dominance_normalized[i] = 0.0;
      max_submission[i] = 0.0;
      submission_normalized[i] = 0.0;
      max_equivalence[i] = 0.0;
      equivalence_normalized[i] = 0.0;
      max_des[i] = 0.0;
      des_normalized[i] = 0.0;
      class_radiance[i] = 0.0;
      class_radiance_normalized[i] = 0.0;
      class_flux[i] = 0.0;
      class_flux_normalized[i] = 0.0;
      totalhits[i] = 0;
      //      for (j = 0; j < TOP_N; j++)
      //  topn[i][j] = 0.0;
    }

#ifdef KNN_ON
    //      Initialize the KNN neighborhood.
    for (i = 0; i < KNN_NEIGHBORHOOD_SIZE; i++) ;
    {
      top_n_val[i] = 0.0;
      top_n_class[i] = -1;
    }
#endif

    if (internal_trace)
      fprintf(stderr, "About to run classify loop with %ld files\n",
              maxhash);

    //    Now run through each of the classifier maps
    for (cls = 0; cls < maxhash; cls++)
    {
      unsigned long u, k, wrapup;
      int cmp;

      //       Prepare for a new file
      k = 0;
      u = 0;
      wrapup = 0;
      //      fprintf(stderr, "Header: %ld %lx %lx %lx %lx %lx %lx\n",
      //       cls,
      //       hashes[cls][0].hash,
      //       hashes[cls][0].key,
      //       hashes[cls][1].hash,
      //       hashes[cls][1].key,
      //       hashes[cls][2].hash,
      //       hashes[cls][2].key);

      if (user_trace)
      {
        fprintf(stderr, "now processing file %ld\n", cls);
        fprintf(stderr, "Hashlens = %ld\n", hashlens[cls]);
      }

      while (k < hashlens[cls] && hashes[cls][k].hash == 0)
        k++;

      while (k < hashlens[cls])
      {
        //      This is the per-document level of the loop.
        u = 0;
        nfeats = 0;
        ufeats = 0;
        kfeats = 0;
        unotk = 0;
        knotu = 0;
        kandu = 0;
        wrapup = 0;
        {
          long j;
          for (j = 0; j < 256; j++)
          {
            collapse_vec_same[j] = 0;
            collapse_vec_diff[j] = 0;
          }
        }
        while (!wrapup)
        {
          nfeats++;
          //    it's an in-class feature.
          cmp = hash_compare(&unk_hashes[u], &hashes[cls][k]);
          if (cmp < 0)
          {
            // unknown less, step u forward
            //   increment on u, because maybe k will match next time
            unotk++;
            u++;
            ufeats++;
            collapse_vec_diff[(unk_hashes[u].hash & 0xFF)]++;
          }
          if (cmp == 0)           // features matched.
          //   These aren't the features you're looking for.
          //   Move along, move along....
          {
            u++;
            k++;
            kandu++;
            ufeats++;
            kfeats++;
            collapse_vec_same[(unk_hashes[u].hash & 0xFF)]++;
          }
          if (cmp > 0)           // unknown is greater, step k forward
          {
            //  increment on k, because maybe u will match next time.
            knotu++;
            k++;
            kfeats++;
            collapse_vec_diff[(hashes[cls][k].hash & 0xFF)]++;
          }
          //   End of the U's?  If so, skip k to the end marker
          //    and finish.
          if (u >= unk_hashcount - 1)
          {
            while (k < hashlens[cls]
                   && hashes[cls][k].hash != 0)
            {
              k++;
              kfeats++;
              knotu++;
            }
          }
          //   End of the K's?  If so, skip U to the end marker
          if (k >= hashlens[cls] - 1
              || hashes[cls][k].hash == 0)           //  end of doc features
          {
            unotk += unk_hashcount - u;
            ufeats += unk_hashcount - u;
          }

          //    end of the U's or end of the K's?  If so, end document.
          if (u >= unk_hashcount - 1
              || k >= hashlens[cls] - 1
              || hashes[cls][k].hash == 0)           // this sets end-of-document
          {
            wrapup = 1;
            k++;
          }
        }

        //  Now the wrapup...
        wrapup = 0;
        if (nfeats > 10)
        {
          //  end of a document- process accumulations
          //           distance first;
          //   Pythagorean distance with unit dimensions
          //  DO NOT USE.  Works like crap.
          //    dist = sqrt (knotu  + unotk);

          //    The following distance function is "weakly founded"
          //    (it's the matrix determinant) but it seems to work
          //    MUCH better than Pythagorean distance for text
          //    classification.
          //    It can probably be extended to a larger matrix
          //    for more dimensions
          //    This formula (and then using radiance = 1/dist
          //    because this formula is already really distance^2;
          //    look at the denomenator if you don't believe me)
          //    trained with a thickness of 1, yelds 27 errors on
          //    the 10x SA corpus.  Not bad.  :)
          //     (5 / 500 on pass 1, unique, 0 thk)
          // dist = (unotk * knotu + 1.0) / ( kandu * kandu + 1.0);

          //    actual (pythag) distance,-- note the sqrt
          // PREVIOUS RELEASE VER -->>>
          // dist = sqrt((unotk * knotu) / ( kandu * kandu + 1.0));

          //    Proper pythagorean (Euclidean) distance - best in
          //   SpamConf 2006 paper
          dist = sqrt(unotk + knotu);

          //    treat kandu better... count matches like mismatches
          //     5/500 pass 1 (0 thk)
          // dist = (unotk * knotu + 1.0) / ( 4 * (kandu * kandu) + 1.0);

          //    Something really simple - similarity.  -Totally hopeless.
          //              dist = 1/(kandu + 1);

          //    unotk?  10 in last 500 of pass 1.(unigram)
          // dist = unotk;

          //    knotu?   Also 10 in last 500 of pass 1 (unigram) (0 thk)
          //    And also 10 in 500 @ pass1 (OSB)  (0 thk)
          //dist = knotu;

          //     knotu + unotk?   8 /500 @ pass1 (unigram, 0 thk)
          //                      6 / 600 @ pass1 (unique, 0 thk)
          // dist = knotu + unotk;

          //     knotu * unotk  20 / 500 unique. 20/500 unigram 0 thk)
          //dist = knotu * unotk;

          //     How important is the kandu term?
          //     result - 29 errors in first pass alone!  Sucks!
          //dist = (unotk * knotu + 1.0) / ( (kandu * kandu * kandu) + 1.0)

          //       Not as good as the above
          //   dist =  (unotk + knotu) / (kandu + 1);

          //       This one's awful....
          //   dist = sqrt (unotk + knotu);

          //       this one is not much better.... slow to learn
          // dist=(unotk * knotu + 1.0) / (kandu * kandu * kandu + 1.0);
          //   Actual determinant-based distance.  works like crap
          //              dist = fmax ( 0.00000000001,
          //            (unotk * knotu) - (kandu * kandu));

          //     Collapse-vector-based distance:
          //{
          //  long i;
          //  float num, denom;
          //  num = 0;
          //  denom = 0;
          //  dist = 0.0 ;
          //  for (i = 0; i < 255; i++)
          //    {
          //      num   = (collapse_vec_diff[i]*collapse_vec_diff[i]);
          //      denom = (collapse_vec_same[i]*collapse_vec_same[i]);
          //      dist += num / ( denom + 1.0);
          //    }
          //  //  dist = (sqrt(num) / sqrt(denom));
          //}

          // BOGUS ERR
          //if ( dist < closest_dist [cls])
          //  closest_dist[cls] = dist;

          //  normalized distance - make each dimension of
          //   distance of a text be sqrt(1/doc_feat_count), so the
          //    text is somewhere on a unit hypersphere.  Then
          //     calculate the distance between the texts based on
          //      that unit length in hyperspace.  The problem is
          //       that this puts far too much emphasis on the text
          //        lengths, so we bastardize it.
          //dist_normalized = dist / nfeats;
          //if ( dist_normalized < closest_normalized[cls])
          //  closest_normalized[cls] = dist_normalized;

          //       dominance and submission
          //dominance = knotu;
          //equivalence = kandu;
          //submission = unotk;

          //if (dominance > max_dominance[cls])
          //  max_dominance[cls] = dominance;
          //if (equivalence > max_equivalence[cls])
          //  max_equivalence[cls] = equivalence;
          //if (submission > max_submission[cls])
          //  max_submission[cls] = submission;

          //if (dominance/nfeats > dominance_normalized[cls])
          //  dominance_normalized[cls] = dominance/ nfeats;
          //if (equivalence/nfeats > equivalence_normalized[cls])
          //  equivalence_normalized [cls] = equivalence/nfeats;
          //if (submission/nfeats > submission_normalized[cls])
          //  submission_normalized[cls] = submission/sqrt(nfeats);

          //des = equivalence * equivalence / (dominance * submission);
          //if (des > max_des[cls])
          //  max_des[cls] = des;
          //des_nrm = des / nfeats;
          //if (des_nrm > des_normalized[cls])
          //  des_normalized[cls] = des_nrm;

          //       radiance and flux;  - note that these are
          //      _cumulative_ over a class, not for te best member,
          //      so it's a +=, not if-closer-then-update.

          //    Radiance = inverse (distance) works pretty well...)
          //              radiance = 1.0 / (dist * dist + 1);
          //              radiance = des;
          //  radiance = 1.0 / (sqrt (dist) + .0000000000000000000001);
          //   this gives 34 errors and about 1 Mbyte
          // radiance = 1.0 / (dist + .000000000000000000000001);
          //   this gives 27 errors and 1.3 mbytes
          // radiance = 1.0 / (dist + 0.01);

          //   version for use with square-rooted distance
          // PREV RELEASE VER --> radiance = 1.0 / ((dist * dist )+ 1.0);
          //
          //  This formula was the best found in the MIT `SC 2006 paper.
          radiance = 1.0 / ((dist * dist) + .000001);
          radiance = radiance * kandu;
          radiance = radiance * kandu;
          //radiance = radiance * kandu;

          //  bad radiance design - based on similarity only.
          //              radiance = kandu;

          //  HACK HACK HACK  - this is based on the empirical
          //  ratio of an average 4.69:1 between features in
          //  the correct class vs. features in the incorrect class.
          //radiance = ( ( kandu ) - (knotu + unotk));
          //if (radiance < 0.0) radiance = 0;


          //    this gives 25 errors in 1st 3 passes... skipping
          // radiance = 1.0 / ( dist + 10.0);

          //              fprintf(stderr, "%1ld %10ld %10ld %10ld  ",
          // cls, kandu, unotk, knotu);
          //fprintf(stderr, "%15.5f %15.5lf\n", dist, radiance);
          class_radiance[cls] += radiance;
          class_radiance_normalized[cls] += radiance / nfeats;


          //       flux is a normalized radiance.
          //flux = 1 / (dist + 1) ;
          //flux = 100000.0 / ( sqrt (dist) + .000000000000001) ;
          //class_flux[cls] += flux;
          //class_flux_normalized[cls] += flux / nfeats;

          //    And for fun, we also keep totalhits
          //BOGUS FAULT
          totalhits[cls] += kandu;

#ifdef KNN_ON
          //     Do the TopN updates; this is a swap-sort.
          //
          {
            float local_val;
            float local_class;
            float temp_val;
            float temp_class;
            long i;
            local_val = 1 / (dist + 0.000000001);
            local_class = cls;
            for (i = 0; i < KNN_NEIGHBORHOOD_SIZE - 1; i++)
            {
              if (local_val > top_n_val[i])
              {
                temp_val = top_n_val[i];
                temp_class = top_n_class[i];
                top_n_val[i] = local_val;
                top_n_class[i] = local_class;
                local_val = temp_val;
                local_class = temp_class;
              }
            }
          }
#endif
        }
      }       //  end per-document stuff
              // fprintf(stderr, "exit K = %ld\n", k);
    }

    //    TURN THIS ON IF YOU WANT TO SEE ALL OF THE HUMILIATING DEAD
    //    ENDS OF EVALUATIONS THAT DIDN'T WORK OUT WELL....
    if (internal_trace)
    {
      for (i = 0; i < maxhash; i++)
      {
        fprintf(stderr,
                "f: %ld  dist %f %f\n"
                "dom: %f %f  equ: %f %f sub: %f %f\n"
                "DES: %f %f\nrad: %f %f  flux: %f %f\n\n",
                i,
                closest_dist[i], closest_normalized[i],
                max_dominance[i], dominance_normalized[i],
                max_equivalence[i], equivalence_normalized[i],
                max_submission[i], submission_normalized[i],
                max_des[i], des_normalized[i],
                class_radiance[i], class_radiance_normalized[i],
                class_flux[i], class_flux_normalized[i]);
      }
    }
  }

  //////////////////////////////////////////////
  //
  //          Class radiance via top-N documents?
  //
#ifdef KNN_ON
  {
    long i;
    long j;
    for (i = 0; i < maxhash; i++)
    {
      class_radiance[i] = 0.0;
    }
    for (j = 0; j < KNN_NEIGHBORHOOD_SIZE; j++)
    {
      if (top_n_class[j] >= 0)
      {
        // class_radiance[top_n_class[j]] ++;
        //  class_radiance[top_n_class[j]] += KNN_NEIGHBORHOOD_SIZE - j;
        class_radiance[top_n_class[j]] += top_n_val[j];
      }
    }
  }
#endif
  ///////////////////////////////////////////////////////
  //
  //     Now we have the relative match values in closest_dist,
  //     class_radiance, and class_radiance_normalized.  We
  //     must choose one.  For now, use class_radiance.
  //
  //     To translate from radiance to probability, we just renormalize...

  {
    tprob = 0.0;
    for (i = 0; i < maxhash; i++)
    {
      //   the following works OK.  But we gotta try something else.
      ptc[i] = class_radiance[i];
      //      ptc[i] = 1/closest_dist[i];
      //      ptc[i] = max_des[i];
      //      ptc[i] = class_flux[i];
      if (ptc[i] < 0.000000000000000000000001)
        ptc[i] =   0.000000000000000000000001;
      tprob = tprob + ptc[i];
    }
    for (i = 0; i < maxhash; i++)
      ptc[i] = ptc[i] / tprob;

    if (user_trace)
    {
      for (k = 0; k < maxhash; k++)
      {
        fprintf(stderr, "Match for file %ld:  radiance: %f  prob: %f\n",
                k, class_radiance[k], ptc[k]);
      }
    }
  }
  //
  tprob = 0.0;
  for (k = 0; k < succhash; k++)
    tprob = tprob + ptc[k];
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
    buf[0] = 0;
    accumulator = 10 * DBL_MIN;
    for (m = 0; m < succhash; m++)
    {
      accumulator = accumulator + ptc[m];
    }
    remainder = 10 * DBL_MIN;
    for (m = succhash; m < maxhash; m++)
    {
      if (bestseen != m)
      {
        remainder = remainder + ptc[m];
      }
    }

    //  overall_pR = 10 * (log10 (accumulator) - log10 (remainder));
    //overall_pR = 10 * (accumulator - remainder);
    overall_pR = 10 * (log10(accumulator) - log10(remainder));
    //   Rescaled for +/-10 pR units of optimal thick training threshold.
    //overall_pR = 250 * (log10 (accumulator) - log10 (remainder));
    //   going to 1500+  as 250x will do.  A little much, so we will
    //   rescale yet again, for +/- 1.0 units.
    //overall_pR = 25 * (log10 (accumulator) - log10(remainder));

    //   note also that strcat _accumulates_ in stext.
    //  There would be a possible buffer overflow except that _we_ control
    //   what gets written here.  So it's no biggie.

    if (tprob > 0.5)
    {
      sprintf(buf, "CLASSIFY succeeds; success probability: %6.4f  pR: %6.4f\n", tprob, overall_pR);
    }
    else
    {
      sprintf(buf, "CLASSIFY fails; success probability: %6.4f  pR: %6.4f\n", tprob, overall_pR);
    }
    if (strlen(stext) + strlen(buf) <= stext_maxlen)
      strcat(stext, buf);

    //   find best single matching file
    //
    bestseen = 0;
    for (k = 0; k < maxhash; k++)
    {
      if (ptc[k] > ptc[bestseen]) bestseen = k;
    }
    remainder = 10 * DBL_MIN;
    for (m = 0; m < maxhash; m++)
    {
      if (bestseen != m)
      {
        remainder = remainder + ptc[m];
      }
    }
    CRM_ASSERT(bestseen >= 0);
    /* CRM_ASSERT(bestseen < maxhash); ** [i_a] this one was triggered in a zero file error condition
     *                                    due to the nonfatalness of the maxhash==0 error report. */

    //   ... and format some output of best single matching file
    //
    buf[0] = 0;
    if (bestseen < maxhash)
    {
      snprintf(buf, WIDTHOF(buf), "Best match to file #%ld (%s) "
                                  "prob: %6.4f  pR: %6.4f  \n",
               bestseen,
               hashname[bestseen],
               ptc[bestseen],
               10 * (log10(ptc[bestseen]) - log10(remainder))
               //   Rescaled for +/- 10.0 thick training threshold optimal
               //250 * (log10(ptc[bestseen]) - log10(remainder))
               // 10 * (ptc[bestseen] - remainder)
               //    rescaled yet again for pR from 1500 to 150
               // 25 * (log10(ptc[bestseen]) - log10(remainder))
      );
      buf[WIDTHOF(buf) - 1] = 0;
    }
    if (strlen(stext) + strlen(buf) <= stext_maxlen)
      strcat(stext, buf);
    sprintf(buf, "Total features in input file: %ld\n", totalfeatures);
    if (strlen(stext) + strlen(buf) <= stext_maxlen)
      strcat(stext, buf);

    //     Now do the per-file breakdowns:
    //
    for (k = 0; k < maxhash; k++)
    {
      long m;
      remainder = 10 * DBL_MIN;
      for (m = 0; m < maxhash; m++)
      {
        if (k != m)
        {
          remainder = remainder + ptc[m];
        }
      }
      CRM_ASSERT(k >= 0);
      CRM_ASSERT(k < maxhash);
      snprintf(buf, WIDTHOF(buf),
               "#%ld (%s):"
               " features: %ld, hits: %ld, radiance: %3.2e, prob: %3.2e, pR: %6.2f\n",
               k,
               hashname[k],
               hashlens[k],
               (long)totalhits[k],
               class_radiance[k],
               ptc[k],
               10.0 * (log10(ptc[k]) - log10(remainder)));
      buf[WIDTHOF(buf) - 1] = 0;
      //  Rescaled for +/- 10 pR units optimal thick threshold
      //250 * (log10 (ptc[k]) - log10 (remainder) )  );
      //    rescaled yet again for pR from 1500 to 150
      //25 * (log10 (ptc[k]) - log10 (remainder) )  );

      // strcat (stext, buf);
      if (strlen(stext) + strlen(buf) <= stext_maxlen)
        strcat(stext, buf);
    }
    // check here if we got enough room in stext to stuff everything
    // perhaps we'd better rise a nonfatalerror, instead of just
    // whining on stderr
    if (strcmp(&(stext[strlen(stext) - strlen(buf)]), buf) != 0)
    {
      nonfatalerror("WARNING: not enough room in the buffer to create "
                    "the statistics text.  Perhaps you could try bigger "
                    "values for MAX_CLASSIFIERS or MAX_FILE_NAME_LEN?",
                    " ");
    }
    if (svlen > 0)
    {
      crm_destructive_alter_nvariable(svrbl, svlen,
                                      stext, strlen(stext));
    }
  }


  //  cleanup time!
  //  remember to let go of the fd's and mmaps
  for (k = 0; k < maxhash; k++)
  {
    //      close (hfds [k]);
    crm_munmap_file((void *)hashes[k]);
  }
  //  and let go of the regex buffery
  if (ptext[0] != 0) crm_regfree(&regcb);

  //   and drop the list of unknown hashes
  free(unk_hashes);

  //
  //  Free the hashnames, to avoid a memory leak.
  //


  for (i = 0; i < maxhash; i++) {
    ///////////////////////////////////////
    //    ! XXX SPAMNIX HACK!
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
    free(hashname[i]);
  }


  if (tprob <= 0.5)
  {
    if (user_trace)
      fprintf(stderr, "CLASSIFY was a FAIL, skipping forward.\n");
    //    and do what we do for a FAIL here
    csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
    csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
    return 0;
  }


  //
  //   all done... if we got here, we should just continue execution
  if (user_trace)
    fprintf(stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
  regcomp_failed:
  return 0;
}

#else /* CRM_WITHOUT_OSB_HYPERSPACE */

int crm_expr_osb_hyperspace_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                                  char *txtptr, long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "OSB-Bayes");
}


int crm_expr_osb_hyperspace_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
                                     char *txtptr, long txtstart, long txtlen)
{
  fatalerror_ex(SRC_LOC(),
                "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                "You may want to run 'crm -v' to see which classifiers are available.\n",
                "OSB-Bayes");
}

#endif /* CRM_WITHOUT_OSB_HYPERSPACE */

