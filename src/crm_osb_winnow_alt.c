//  crm_osb_winnow.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
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



#if !CRM_WITHOUT_OSB_WINNOW



////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static const int hctable[] =
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



//          Where does the nominative data start?
static int spectra_start = 0;



//    How to learn Osb_Winnow style  - in this case, we'll include the single
//    word terms that may not strictly be necessary, but their weight will
//    be set to 0 in the evaluation.
//

int crm_expr_alt_osb_winnow_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //     learn the osb_winnow transform spectrum of this input window as
    //     belonging to a particular type.
    //     learn <flags> (classname) /word/
    //
    int i, j, k;
    int h;                   //  h is our counter in the hashpipe;
    char htext[MAX_PATTERN]; //  the hash name
    int hlen;
    int cflags, eflags;
    struct stat statbuf;    //  for statting the hash file
    int hfsize;             //  size of the hash file
    char *learnfilename = NULL;
    WINNOW_FEATUREBUCKET_STRUCT *hashes = MAP_FAILED; //  the text of the hash file
    unsigned char *xhashes = NULL;                    //  and the mask of what we've seen
    crmhash_t *hashpipe = NULL;
    int hashcounts;
    //
    double sense;
    int microgroom;
    int use_unigrams;
    int fev = 0;
    int made_new_file;



    if (internal_trace)
        fprintf(stderr, "executing an OSB-WINNOW LEARN\n");

    //   Keep the gcc compiler from complaining about unused variables
    //  i = hctable[0];

    //           extract the hash file name
    hlen = crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, MAX_PATTERN, vht, tdw);
    CRM_ASSERT(hlen < MAX_PATTERN);

    //     get the "this is a word" regex
    //plen = crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    //plen = crm_nexpandvar(ptext, plen, MAX_PATTERN, vht, tdw);

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
            fprintf(stderr, "turning oncase-insensitive match\n");
    }


    //
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
            fprintf(stderr, " refuting learning\n");
    }

    microgroom = 0;
    if (apb->sflags & CRM_MICROGROOM)
    {
        microgroom = 1;
        if (user_trace)
            fprintf(stderr, " enabling microgrooming.\n");
    }

    use_unigrams = 0;
    if (apb->sflags & CRM_UNIGRAM)
    {
        use_unigrams = 1;
        if (user_trace)
            fprintf(stderr, " enabling unigram-only operation.\n");
    }


    //
    //             grab the filename, and stat the file
    //      note that neither "stat", "fopen", nor "open" are
    //      fully 8-bit or wchar clean...
    if (!crm_nextword(htext, hlen, 0, &i, &j) || j == 0)
    {
        fev = nonfatalerror_ex(SRC_LOC(),
                "\nYou didn't specify a valid filename: '%.*s'\n",
                hlen,
                htext);
        return fev;
    }
    j += i;
    CRM_ASSERT(i < hlen);
    CRM_ASSERT(j <= hlen);

    //             filename starts at i,  ends at j. null terminate it.
    htext[j] = 0;
    learnfilename = &htext[i];
    if (!learnfilename)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }

    //             and stat it to get it's length
    k = stat(learnfilename, &statbuf);

    made_new_file = 0;

    //             quick check- does the file even exist?
    if (k != 0)
    {
        //      file didn't exist... create it
        FILE *f;
        CRM_PORTA_HEADER_INFO classifier_info = { 0 };

        if (user_trace)
        {
            fprintf(stderr, "\n Opening new COW file %s for write\n", learnfilename);
        }
        f = fopen(learnfilename, "wb");
        if (!f)
        {
            char dirbuf[DIRBUFSIZE_MAX];

            fev = fatalerror_ex(SRC_LOC(),
                    "\n Couldn't open your new COW file %s for writing; (full path: '%s') errno=%d(%s)\n",
                    learnfilename,
                    mk_absolute_path(dirbuf, WIDTHOF(dirbuf), learnfilename),
                    errno,
                    errno_descr(errno));
            goto fail_dramatically;
        }
        //       do we have a user-specified file size?
        if (sparse_spectrum_file_length == 0)
        {
            sparse_spectrum_file_length =
                DEFAULT_WINNOW_SPARSE_SPECTRUM_FILE_LENGTH;
        }

        classifier_info.classifier_bits = CRM_OSB_WINNOW;
        classifier_info.hash_version_in_use = selected_hashfunction;

        if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
        {
            fev = fatalerror_ex(SRC_LOC(),
                    "\n Couldn't write header to file %s; errno=%d(%s)\n",
                    learnfilename, errno, errno_descr(errno));
            fclose(f);
            goto fail_dramatically;
        }

        //       put in sparse_spectrum_file_length entries of NULL
        if (file_memset(f, 0,
                    sparse_spectrum_file_length * sizeof(WINNOW_FEATUREBUCKET_STRUCT)))
        {
            fev = fatalerror_ex(SRC_LOC(),
                    "\n Couldn't write to file %s; errno=%d(%s)\n",
                    learnfilename, errno, errno_descr(errno));
            fclose(f);
            goto fail_dramatically;
        }
        made_new_file = 1;
        //
        fclose(f);

        //    and reset the statbuf to be correct
        k = stat(learnfilename, &statbuf);
        CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
    }
    //
    hfsize = statbuf.st_size;

    //
    //         open the .cow hash file into memory so we can bitwhack it
    //
    hashes = crm_mmap_file(learnfilename,
            0,
            hfsize,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            CRM_MADV_RANDOM,
            &hfsize);
    if (hashes == MAP_FAILED)
    {
        fev = fatalerror("Couldn't memory-map the .cow file named: ",
                learnfilename);
        goto fail_dramatically;
    }

    if (user_trace)
    {
        fprintf(stderr, "Sparse spectra file %s has length %d bins\n",
                learnfilename, (int)(hfsize / sizeof(WINNOW_FEATUREBUCKET_STRUCT)));
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
    if (hashes[0].hash != 1
        || hashes[0].key != 0)
    {
        if (internal_trace)
            fprintf(stderr, "Hash was: %d, key was %d\n", hashes[0].hash, hashes[0].key);
        fev = fatalerror("The .cow file is the wrong type!  We're expecting "
                         "a Osb_Winnow-spectrum file.  The filename is: ",
                learnfilename);
        goto fail_dramatically;
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
    hfsize = hfsize / sizeof(WINNOW_FEATUREBUCKET_STRUCT);


    //    and allocate the mask-off flags for this file
    //    so we only use each feature at most once
    //
    xhashes = calloc(hfsize, sizeof(xhashes[0]));
    if (!xhashes)
    {
        untrappableerror("Couldn't alloc xhashes\n",
                "We need that part.  Sorry.\n");
    }



    //   Start by priming the pipe... we will shift to the left next.
    //     sliding, hashing, xoring, moduloing, and incrmenting the
    //     hashes till there are no more.
    k = 0;
    j = 0;
    i = 0;



    //   init the hashpipe with 0xDEADBEEF
    hashpipe = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(hashpipe[0]));
    if (!hashes)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    hashcounts = 0;

    //   Use the flagged vector tokenizer
    crm_vector_tokenize_selector(apb, // the APB
            vht,
            tdw,
            txtptr + txtstart,        // intput string
            txtlen,                   // how many bytes
            0,                        // starting offset
            NULL,                     // tokenizer
            NULL,                     // coeff array
            hashpipe,                 // where to put the hashed results
            BAYES_MAX_FEATURE_COUNT,  //  max number of hashes
            NULL,
            NULL,
            &hashcounts               // how many hashes we actually got
                                );
    CRM_ASSERT(hashcounts >= 0);
    CRM_ASSERT(hashcounts < BAYES_MAX_FEATURE_COUNT);
    CRM_ASSERT(hashcounts % 2 == 0);

    //    and the big loop...
    for (k = 0; k < hashcounts; k++)
    {
        {
            crmhash_t hindex;
            crmhash_t h1, h2;
            int th = 0;         // a counter used for TSS tokenizing
            unsigned int incrs;
            int j;
            //
            //
            th = 0;
            //
            //     Note that we start at j==1 here, so that we do NOT
            //     ever calculate (or save) the unigrams.
            //
// #ifdef TGB
// #ifdef TGB2
// #ifdef TSS
// #ifdef SBPH
            hindex = hashpipe[k++];
            h1 = hindex;

            //   and what's our primary hash index?  Note that
            //   hindex = 0 is reserved for our version and
            //   usage flags, so we autobump those to hindex=1
            hindex %= hfsize;
            if (hindex == 0)
                hindex = 1;

            //   this is the secondary (crosscut) hash, used for
            //   confirmation of the key value.  Note that it shares
            //   no common coefficients with the previous hash.
            h2 = hashpipe[k];
            if (h2 == 0)
                h2 = 0xdeadbeef;

// #ifdef ARBITRARY_WINDOW_LENGTH
            if (internal_trace)
            {
                fprintf(stderr, "Polynomial %d has hash: 0x%08lX  h1:0x%08lX  h2:0x%08lX\n",
                        k, (unsigned long int)hashpipe[k], (unsigned long int)h1, (unsigned long int)h2);
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
            while ((!((hashes[hindex].hash == h1) && (hashes[hindex].key == h2)))
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
#ifdef STOCHASTIC_AMNESIA
                    //     set the random number generator up...
                    //     note that this is repeatable for a
                    //     particular test set, yet dynamic.  That
                    //     way, we don't always autogroom away the
                    //     same feature; we depend on the previous
                    //     feature's key.
                    srand((unsigned int)h2);
#endif
                    //
                    //   and do the groom.

                    //   reset our hindex to where we started...
                    //
                    hindex = h1 % hfsize;
                    if (hindex < spectra_start)
                        hindex = spectra_start;

                    //    and microgroom.
                    //fprintf(stderr,  "\nCalling microgroom hindex %d hash: %d  key: %d  value: %f ",
                    //      hindex, hashes[hindex].hash, hashes[hindex].key, hashes[hindex].value );

                    crm_winnow_microgroom(hashes, xhashes, hfsize, hindex);
                    incrs = 0;
                }
                //      check to see if we've incremented ourself all the
                //      way around the .cow file.  If so, we're full, and
                //      can hold no more features (this is unrecoverable)
                if (incrs > hfsize - 3)
                {
                    nonfatalerror("Your program is stuffing too many "
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
                if (hindex >= hfsize)
                    hindex = spectra_start;
            }

            if (internal_trace)
            {
                if (hashes[hindex].value == 0)
                {
                    fprintf(stderr, "New feature at %d\n", (int)hindex);
                }
                else
                {
                    fprintf(stderr, "Old feature at %d\n", (int)hindex);
                }
            }

            //      With _winnow_, we just multiply by the sense factor.
            //
            if (xhashes[hindex] == 0)
            {
                hashes[hindex].hash = h1;
                hashes[hindex].key  = h2;
                xhashes[hindex] = 1;
                if (hashes[hindex].value > 0)
                {
                    hashes[hindex].value *= sense;
                }
                else
                {
                    hashes[hindex].value = sense;
                }
                    CRM_ASSERT(hashes[hindex].value > 0.0);
            }

            // fprintf(stderr, "Hash index: %d  value: %f \n", hindex, hashes[hindex].value);
        }
    }

learn_end_regex_loop:
fail_dramatically:

    //  and remember to let go of the mmap and the pattern bufffer
    // (and force a cache purge)
    // crm_munmap_all ();
    if (hashes != MAP_FAILED)
        crm_munmap_file((void *)hashes);

    free(xhashes);


    if (hashpipe)
        free(hashpipe);

#if 0  /* now touch-fixed inside the munmap call already! */
#if defined (HAVE_MMAP) || defined (HAVE_MUNMAP)
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

    return 0;
}



//      How to do a Osb_Winnow CLASSIFY some text.
//
int crm_expr_alt_osb_winnow_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //      classify the sparse spectrum of this input window
    //      as belonging to a particular type.
    //
    //       This code should look very familiar- it's cribbed from
    //       the code for LEARN
    //
    int i, j, k;
    int h;                      //  we use h for our hashpipe counter, as needed.
    char ostext[MAX_PATTERN];   //  optional pR offset
    int oslen;
    double pR_offset;
    //  the hash file names
    char htext[MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN];
    int htext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN;
    int hlen;
    //  the match statistics variable inbuf
    char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
    char *stext_ptr = stext;
    int stext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);
    int slen;
    char svrbl[MAX_PATTERN]; //  the match statistics text buffer
    int svlen;
    int fnameoffset;
    char fname[MAX_FILE_NAME_LEN];
    int eflags;
    int cflags;
    int not_microgroom = 1;
    int use_unigrams;

    struct stat statbuf;    //  for statting the hash file
                            //  longest association set in the hashing
    crmhash_t *hashpipe;

    double fcounts[MAX_CLASSIFIERS]; // total counts for feature normalize

    double cpcorr[MAX_CLASSIFIERS];        // corpus correction factors

#if defined (GER) || 01
    double totalcount = 0;
    hitcount_t hits[MAX_CLASSIFIERS];      // actual hits per feature per classifier
    hitcount_t totalhits[MAX_CLASSIFIERS]; // actual total hits per classifier
    double totalweights[MAX_CLASSIFIERS];  //  total of hits * weights
    hitcount_t unseens[MAX_CLASSIFIERS];       //  total unseen features.
    double classifierprs[MAX_CLASSIFIERS]; //  pR's of each class
    int totalfeatures;                     //  total features
    hitcount_t htf;                        // hits this feature got.
#else
    unsigned int totalcount = 0;
    double hits[MAX_CLASSIFIERS];          // actual hits per feature per classifier
    int totalhits[MAX_CLASSIFIERS];        // actual total hits per classifier
    double totalweights[MAX_CLASSIFIERS];  //  total of hits * weights
    double unseens[MAX_CLASSIFIERS];       //  total unseen features.
    double classifierprs[MAX_CLASSIFIERS]; //  pR's of each class
    int totalfeatures;                     //  total features
    double htf;                            // hits this feature got.
#endif
    double tprob = 0;                                   //  total probability in the "success" domain.
    double min_success = 0.5;                           // minimum probability to be considered success

    //double textlen;    //  text length  - rougly corresponds to
    //  information content of the text to classify

    WINNOW_FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
    unsigned char *xhashes[MAX_CLASSIFIERS];
    int hashlens[MAX_CLASSIFIERS];
    char *hashname[MAX_CLASSIFIERS];
    int succhash;
    int vbar_seen;     // did we see '|' in classify's args?
    int maxhash;
    int fnstart, fnlen;
    int fn_start_here;
    int bestseen;
    int thistotal;
    int *feature_weight;
    int *order_no;
    int hashcounts;

    double top10scores[10];
    int top10polys[10];
    char top10texts[10][MAX_PATTERN];


    if (internal_trace)
        fprintf(stderr, "executing an OSB-WINNOW CLASSIFY\n");

    //
    //      We get the to-be-classified text from the caller now.
    //
    // llen = crm_get_pgm_arg (ltext, MAX_PATTERN, apb->b1start, apb->b1len);
    // llen = crm_nexpandvar (ltext, llen, MAX_PATTERN, vht, tdw);

    //           extract the hash file names
    hlen = crm_get_pgm_arg(htext, htext_maxlen, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, htext_maxlen, vht, tdw);
    CRM_ASSERT(hlen < MAX_PATTERN);

    //           extract the "this is a word" regex
    //
    //plen = crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    //plen = crm_nexpandvar(ptext, plen, MAX_PATTERN, vht, tdw);

    //       extract the optional pR offset value
    //
    oslen = crm_get_pgm_arg(ostext, MAX_PATTERN, apb->s2start, apb->s2len);
    pR_offset = 0;
    min_success = 0.5;
    if (oslen > 0)
    {
        oslen = crm_nexpandvar(ostext, oslen, MAX_PATTERN, vht, tdw);
        CRM_ASSERT(oslen < MAX_PATTERN);
        ostext[oslen] = 0;
        pR_offset = strtod(ostext, NULL);
        min_success = 1.0 - 1.0 / (1 + pow(10, pR_offset));
    }

    //            extract the optional "match statistics" variable
    //
    svlen = crm_get_pgm_arg(svrbl, MAX_PATTERN, apb->p2start, apb->p2len);
    svlen = crm_nexpandvar(svrbl, svlen, MAX_PATTERN, vht, tdw);
    CRM_ASSERT(svlen < MAX_PATTERN);
    {
        int vstart, vlen;

        if (crm_nextword(svrbl, svlen, 0, &vstart, &vlen))
        {
            memmove(svrbl, &svrbl[vstart], vlen);
            svlen = vlen;
            svrbl[vlen] = 0;
        }
        else
        {
            svlen = 0;
            svrbl[0] = 0;
        }
    }

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
        cflags |= REG_ICASE;
        eflags = 1;
    }

    not_microgroom = 1;
    if (apb->sflags & CRM_MICROGROOM)
    {
        not_microgroom = 0;
        if (user_trace)
            fprintf(stderr, " disabling fast-skip optimization.\n");
    }

    use_unigrams = 0;
    if (apb->sflags & CRM_UNIGRAM)
    {
        use_unigrams = 1;
        if (user_trace)
            fprintf(stderr, " enabling unigram-only operation.\n");
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
        fcounts[i] = 0.0;        // check later to prevent a divide-by-zero
                                 // error on empty .css file
        cpcorr[i] = 0.0;         // corpus correction factors
        hits[i] = 0;             // absolute hit counts
        totalhits[i] = 0;        // absolute hit counts
        totalweights[i] = 0.0;   // hit_i * weight*i count
        unseens[i] = 0;        // text features not seen in statistics files
    }

    for (i = 0; i < 10; i++)
    {
        top10scores[i] = 0;
        top10polys[i] = 0;
        strcpy(top10texts[i], "");
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
        fprintf(stderr, "Classify list: -%s-\n", htext);
    fn_start_here = 0;
    fnlen = 1;
    while (fnlen > 0 && ((maxhash < MAX_CLASSIFIERS - 1)))
    {
        if (crm_nextword(htext, hlen, fn_start_here, &fnstart, &fnlen)
            && fnlen > 0)
        {
            strncpy(fname, &htext[fnstart], fnlen);
            fname[fnlen] = 0;
            //      fprintf(stderr, "fname is '%s' len %d\n", fname, fnlen);
            fn_start_here = fnstart + fnlen + 1;
            if (user_trace)
            {
                fprintf(stderr, "Classifying with file -%s- "
                                "succhash=%d, maxhash=%d\n",
                        fname, succhash, maxhash);
            }
            if (fname[0] == '|' && fname[1] == 0)
            {
                if (vbar_seen)
                {
                    nonfatalerror("Only one '|' allowed in a CLASSIFY.\n",
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
                int k;

                //  be sure the file exists
                //             stat the file to get it's length
                k = stat(fname, &statbuf);
                //             quick check- does the file even exist?
                if (k != 0)
                {
                    return nonfatalerror("Nonexistent Classify table named: ",
                            fname);
                }
                else
                {
                    // [i_a] check hashes[] range BEFORE adding another one!
                    if (maxhash >= MAX_CLASSIFIERS)
                    {
                        nonfatalerror("Too many classifier files.",
                                "Some may have been disregarded");
                    }
                    else
                    {
                        //  file exists - do the mmap
                        //
                        hashlens[maxhash] = statbuf.st_size;
                        //  mmap the hash file into memory so we can bitwhack it
                        hashes[maxhash] = crm_mmap_file(fname,
                                0,
                                hashlens[maxhash],
                                PROT_READ,
                                MAP_SHARED,
                                CRM_MADV_RANDOM,
                                &hashlens[maxhash]);

                        if (hashes[maxhash] == MAP_FAILED)
                        {
                            nonfatalerror("Couldn't memory-map the table file",
                                    fname);
                        }
                        else
                        {
#ifdef CSS_VERSION_CHECK
                            //
                            //     Check to see if this file is the right version
                            //
                            int fev;
                            if (hashes[maxhash][0].hash != 1
                                || hashes[maxhash][0].key  != 0)
                            {
                                fev = fatalerror("The .css file is the wrong type!  We're expecting "
                                                 "a Osb_Winnow-spectrum file.  The filename is: ",
                                        &htext[i]);
                                return fev;
                            }
#endif
                            //     grab the start of the actual spectrum data.
                            //
                            spectra_start = hashes[maxhash][0].value;


                            //  set this hashlens to the length in features instead
                            //  of the length in bytes.
                            hashlens[maxhash] = hashlens[maxhash] / sizeof(WINNOW_FEATUREBUCKET_STRUCT);
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

                            //    and allocate the mask-off flags for this file
                            //    so we only use each feature at most once
                            //
                            xhashes[maxhash] = calloc(hashlens[maxhash],
                                    sizeof(xhashes[maxhash][0]));
                            if (!xhashes[maxhash])
                            {
                                untrappableerror(
                                        "Couldn't alloc xhashes[maxhash]\n",
                                        "We need that part.  Sorry.\n");
                            }

                            maxhash++;
                        }
                    }
                }
            }
        }
    }

    //
    //    If there is no '|', then all files are "success" files.
    if (succhash == 0)
        succhash = maxhash;

    //    a CLASSIFY with no arguments is always a "success".
    if (maxhash == 0)
        return 0;

    if (user_trace)
    {
        fprintf(stderr, "Running with %d files for success out of %d files\n",
                succhash, maxhash);
    }

    // sanity checks...  Uncomment for super-strict CLASSIFY.
    //
    //    do we have at least 1 valid .css files?
    if (maxhash == 0)
    {
        return nonfatalerror("Couldn't open at least 1 .css file for classify().", "");
    }

#if 0
    //    do we have at least 1 valid .css file at both sides of '|'?
    if (!vbar_seen || succhash <= 0 || (maxhash <= succhash))
    {
        return nonfatalerror("Couldn't open at least 1 .css file per SUCC | FAIL category "
                             "for classify().\n", "Hope you know what are you doing.");
    }
#endif

    {
        int ifile;
        int k;

        //      count up the total first
        for (ifile = 0; ifile < maxhash; ifile++)
        {
            fcounts[ifile] = 0.0;
            {
                int k;

                for (k = 1; k < hashlens[ifile]; k++)
				{
                    CRM_ASSERT(hashes[ifile][k].value >= 0.0);
                    fcounts[ifile] += hashes[ifile][k].value;
				}
#if 0
                if (fcounts[ifile] <= 0.0)
				{
                    fcounts[ifile] = 1.0;
				}
#endif
                totalcount += fcounts[ifile];
            }
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


    //   init the hashpipe with 0xDEADBEEF
    hashpipe = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(hashpipe[0]));
    if (!hashpipe)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    feature_weight = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(feature_weight[0]));
    if (!feature_weight)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    order_no = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(order_no[0]));
    if (!order_no)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    hashcounts = 0;

    //   Use the flagged vector tokenizer
    crm_vector_tokenize_selector(apb, // the APB
            vht,
            tdw,
            txtptr + txtstart,        // intput string
            txtlen,                   // how many bytes
            0,                        // starting offset
            NULL,                     // tokenizer
            NULL,                     // coeff array
            hashpipe,                 // where to put the hashed results
            BAYES_MAX_FEATURE_COUNT,  //  max number of hashes
            feature_weight,
            order_no,
            &hashcounts               // how many hashes we actually got
                                );
    CRM_ASSERT(hashcounts >= 0);
    CRM_ASSERT(hashcounts < BAYES_MAX_FEATURE_COUNT);
    CRM_ASSERT(hashcounts % 2 == 0);

    totalfeatures = 0;

    //  stop when we no longer get any regex matches
    //   possible edge effect here- last character must be matchable, yet
    //    it's also the "end of buffer".
    {
        int i;

        for (i = 0; i < hashcounts; i++)
        {
            int l;
            int j, k;
            unsigned th = 0;      //  a counter used only in TSS hashing
            crmhash_t hindex;
            crmhash_t h1, h2;
            //unsigned int good, evil;
            //
            //
            th = 0;

            //
            //     Note that we start at j==1 here, so that we do NOT
            //     ever calculate (or save) the unigrams.
            //

// #ifdef FOO
// #ifdef TGB
// #ifdef TGB2
// #ifdef TSS
// #ifdef SBPH
            hindex = hashpipe[i++];
            h1 = hindex;

            //   this is the secondary (crosscut) hash, used for
            //   confirmation of the key value.  Note that it shares
            //   no common coefficients with the previous hash.
            h2 = hashpipe[i];
            if (h2 == 0)
                h2 = 0xdeadbeef;

// #ifdef ARBITRARY_WINDOW_LENGTH
            if (internal_trace)
            {
                fprintf(stderr, "Polynomial %d has hash: 0x%08lX  h1:0x%08lX  h2:0x%08lX\n",
                        i, (unsigned long int)hashpipe[i], (unsigned long int)h1, (unsigned long int)h2);
            }


            //    Now, for each of the feature files, what are
            //    the statistics (found, not found, whatever)
            //
            htf = 0;
            totalfeatures++;
            for (k = 0; k < maxhash; k++)
            {
                int lh, lh0;

                lh = hindex % hashlens[k];
                if (lh < spectra_start)
                    lh = spectra_start;
                lh0 = lh;
                hits[k] = 0;
                while (hashes[k][lh].key != 0
                       && (hashes[k][lh].hash != h1
                           || hashes[k][lh].key  != h2))
                {
                    lh++;
                    if (lh >= hashlens[k])
                        lh = spectra_start;
                    if (lh == lh0)
                        break;     // wraparound
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
#if defined (GER)
                        double z;
#else
                        float z;
#endif

                        // remember totalhits
                        htf++;                      // and hits-this-feature
                        hits[k]++;                  // increment hits.
                        z = hashes[k][lh].value;
                        //                  fprintf (stdout, "L: %f  ", z);
                        // and weight sum
                        totalweights[k] += z;
                        totalhits[k]++;
                        //
                        //  and mark the feature as seen.
                        xhashes[k][lh] = 1;
                    }
                }
                else
                {
                    // unseens score 1.0, which is totally ambivalent; seen
                    // and accepted score more, seen and refuted score less
                    //
                    unseens[k]++;
                    totalweights[k] += 1.0;
                }
            }

            if (internal_trace)
            {
                for (k = 0; k < maxhash; k++)
                {
                    // fprintf(stderr, "ZZZ\n");
                    fprintf(stderr,
                            " poly: %d  filenum: %d, HTF: %7ld, hits: %7ld, th: %10ld, tw: %6.4e\n",
                            i, k, (long int)htf, (long int)hits[k], (long int)totalhits[k], totalweights[k]);
                }
            }
        }
    }

    if (user_trace)
    {
        for (k = 0; k < maxhash; k++)
        {
            fprintf(stderr, "Match for file %d:  hits: %d  weight: %f\n",
                    k, (int)totalhits[k], totalweights[k]);
        }
    }
    //
    //      Do the calculations and format some output, which we may or may
    //      not use... but we need the calculated result anyway.
    //
    if (1 /* svlen > 0 */)
    {
        // char buf[1024];
        double accumulator;
        double remainder;
        double accumulator_hit;
        double remainder_hit;
        double overall_pR;
        int m;
	double totweight;
	double totweight_corr;
hitcount_t tothits;
hitcount_t totunseens;
double ch_prs[MAX_CLASSIFIERS];
double plcc[MAX_CLASSIFIERS];
	double tprob_hit;
double renorm;

        // buf[0] = 0;
        //accumulator = 10 * DBL_MIN;

	totweight = 0.0;
tothits = 0;
totunseens = 0;
        for (m = 0; m < maxhash; m++)
        {
		totweight += totalweights[m];
			tothits += totalhits[m];
			totunseens += unseens[m];
        }
				if (totweight < 10 * DBL_MIN)
				totweight = 10 * DBL_MIN;
totweight_corr = totweight - totunseens;
				if (totweight_corr < 10 * DBL_MIN)
				totweight_corr = 10 * DBL_MIN;

        for (m = 0; m < maxhash; m++)
        {
			double w = totalweights[m];
			hitcount_t h = totalhits[m];

#if 0
			classifierprs[m] = 10.0 * ((w >= 1.0 ? log10(w) : 0.0) - (h >= 1 ? log10(h) : 0.0));
#else
#if 0
			classifierprs[m] = 0.5 + (2 * w - totweight) / (LOCAL_PROB_DENOM * totweight);
#else
			w -= unseens[m];
			classifierprs[m] = (2 * w - totweight) / totweight;
			// classifierprs[m] = 0.5 + (2 * w - totweight_corr) / (LOCAL_PROB_DENOM * totweight_corr);
#endif
				if (tothits == 0)
ch_prs[m] = 0.0;
else
		ch_prs[m] = 0.5 + (2.0 * totalhits[m] - tothits) /  (LOCAL_PROB_DENOM * tothits);
#endif
        }
        accumulator_hit = 0.0;
        accumulator = 0.0;
        for (m = 0; m < succhash; m++)
        {
            accumulator += totalweights[m];
            accumulator_hit += totalhits[m];
        }

        remainder_hit = 0.0;
        remainder = 0.0;
        for (m = succhash; m < maxhash; m++)
        {
            remainder += totalweights[m];
            remainder_hit += totalhits[m];
        }
				if (accumulator < 10 * DBL_MIN)
				accumulator = 10 * DBL_MIN;
				if (remainder < 10 * DBL_MIN)
				remainder = 10 * DBL_MIN;
				if (accumulator_hit < 10 * DBL_MIN)
				accumulator_hit = 10 * DBL_MIN;
				if (remainder_hit < 10 * DBL_MIN)
				remainder_hit = 10 * DBL_MIN;

        tprob = accumulator / totweight;
        tprob_hit = accumulator_hit / tothits;

		//     *******************************************
        //
        //        Note - we use 10 as the normalization for pR here.
        //        it's because we don't have an actual probability
        //        but we want this to scale similarly with the other
        //        recognizers.
        //
        overall_pR = 10 * (log10(accumulator) - log10(remainder));

        //   note also that strcat _accumulates_ in stext.
        //  There would be a possible buffer overflow except that _we_ control
        //   what gets written here.  So it's no biggie.

        if (tprob > min_success)
        {
            // if a pR offset was given, print it together with the real pR
            if (oslen > 0)
            {
                snprintf(stext_ptr, stext_maxlen,
                        "CLASSIFY succeeds; (alt.winnow) success probability: "
                        "%6.4f  pR: %6.4f/%6.4f\n",
                        tprob, overall_pR, pR_offset);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                        "CLASSIFY succeeds; (alt.winnow) success probability: "
                        "%6.4f  pR: %6.4f\n", tprob, overall_pR);
            }
        }
        else
        {
            // if a pR offset was given, print it together with the real pR
            if (oslen > 0)
            {
                snprintf(stext_ptr, stext_maxlen,
                        "CLASSIFY fails; (alt.winnow) success probability: "
                        "%6.4f  pR: %6.4f/%6.4f\n",
                        tprob, overall_pR, pR_offset);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                        "CLASSIFY fails; (alt.winnow) success probability: "
                        "%6.4f  pR: %6.4f\n", tprob, overall_pR);
            }
        }
        stext_ptr[stext_maxlen - 1] = 0;
        stext_maxlen -= (int)strlen(stext_ptr);
        stext_ptr += strlen(stext_ptr);

        //   find best single matching file
        //
        bestseen = 0;
        for (m = 0; m < maxhash; m++)
        {
            if (classifierprs[m] > classifierprs[bestseen])
            {
                bestseen = m;
            }
        }

        remainder = 0.0;
        for (m = 0; m < maxhash; m++)
        {
            if (bestseen != m)
            {
                remainder += totalweights[m];
            }
        }

        for (m = 0; m < maxhash; m++)
        {
            int k;
double plc;
double w;
double fac;
double sig;
            
			remainder = 0.0;
            for (k = 0; k < maxhash; k++)
            {
                if (k != m)
                {
                    remainder += totalweights[k] - unseens[k];
                }
            }

		w = totalweights[m];
w -= unseens[m];

	fac = 1.0;
if (totalhits[m] != 0)
{
	fac = (totalweights[m] - unseens[m]) / totalhits[m];
}
sig = totalhits[m] / (double)(unseens[m] > 0 ? unseens[m] : 1.0);

plc = log10(w > 10 * DBL_MIN ? w : DBL_MIN) - log10(remainder > 10 * DBL_MIN ? remainder : 10 * DBL_MIN);
	
//plcc[m] = (1.0 + sig) * plc; // * (w > 1.0 ? w : 1.0); // * fac;
//plcc[m] *= 20;
if (tothits != 0)
{
plcc[m] = 20 * (2.0 * w - remainder) / tothits;
}
else
{
plcc[m] = -400.0;
}
        }

	// renormalize:
renorm = 0.0;
        for (m = 0; m < maxhash; m++)
        {
	renorm += plcc[m];
}
renorm /= 300.0 * maxhash;
if (renorm < 1.0)
renorm = 1.0;

        //   ... and format some output of best single matching file
        //
        snprintf(stext_ptr, stext_maxlen, "Best match to file #%d (%s) "
                                          "weight: %6.4f  pR: %6.4f\n",
                bestseen,
                hashname[bestseen],
                totalweights[bestseen],
                plcc[bestseen] / renorm);
        stext_ptr[stext_maxlen - 1] = 0;
        stext_maxlen -= (int)strlen(stext_ptr);
        stext_ptr += strlen(stext_ptr);

        snprintf(stext_ptr, stext_maxlen, "Total features in input file: %d\n", totalfeatures);
        stext_ptr[stext_maxlen - 1] = 0;
        stext_maxlen -= (int)strlen(stext_ptr);
        stext_ptr += strlen(stext_ptr);

        //     Now do the per-file breakdowns:
        //
        for (m = 0; m < maxhash; m++)
        {
            int k;
double plc;
double w;
double fac;
double sig;
double plcc1;
            
			remainder = 0.0;
            for (k = 0; k < maxhash; k++)
            {
                if (k != m)
                {
                    remainder += totalweights[k] - unseens[k];
                }
            }

		w = totalweights[m];
w -= unseens[m];

	fac = 1.0;
if (totalhits[m] != 0)
{
	fac = (totalweights[m] - unseens[m]) / totalhits[m];
}
sig = totalhits[m] / (double)(unseens[m] > 0 ? unseens[m] : 1.0);

plc = log10(w > 10 * DBL_MIN ? w : DBL_MIN) - log10(remainder > 10 * DBL_MIN ? remainder : 10 * DBL_MIN);
	
if (tothits != 0)
{
plcc1 = (2.0 * w - remainder) / tothits;
}
else
{
plcc1 = -400.0;
}
            snprintf(stext_ptr, stext_maxlen,
                    "#%d (%s):"
					" features: %.2f, unseen: %6ld, hits: %6ld, uhits: %6.2f, weight: %6.2f, wc: %6.2f, plc: %6.2f, fac: %6.2f, plcc: %6.2f, t: %6.2f, b: %6.2f, pR: %6.2f\n",
                    m,
                    hashname[m],
                    fcounts[m],
                    (long int)unseens[m],
					(long int)totalhits[m],
                    sig * 10000,
                    totalweights[m],
	w,
	plc,
fac,
plcc[m],
(2.0 * w - totalhits[m]),
plcc1,
plcc[m] / renorm /* classifierprs[m] */ );
            stext_ptr[stext_maxlen - 1] = 0;
            stext_maxlen -= (int)strlen(stext_ptr);
            stext_ptr += strlen(stext_ptr);
        }
    }
    // check here if we got enough room in stext to stuff everything
    // perhaps we'd better rise a nonfatalerror, instead of just
    // whining on stderr
    if (stext_maxlen <= 1)
    {
        nonfatalerror("WARNING: not enough room in the buffer to create "
                      "the statistics text.  Perhaps you could try bigger "
                      "values for MAX_CLASSIFIERS or MAX_FILE_NAME_LEN?",
                " ");
    }
    if (svlen > 0)
    {
        crm_destructive_alter_nvariable(svrbl, svlen, stext, (int)strlen(stext), csl->calldepth);
    }




    //  cleanup time!
    //  remember to let go of the fd's and mmaps
    {
        int k;

        for (k = 0; k < maxhash; k++)
        {
            //      close (hfds [k]);
            if (hashes[k])
            {
                crm_munmap_file(hashes[k]);
            }

            //
            //  Free the hashnames, to avoid a memory leak.
            //
            if (hashname[k])
            {
                free(hashname[k]);
            }

            //   and let go of the seen_features array
            if (xhashes[k])
                free(xhashes[k]);
            xhashes[k] = NULL;
        }
    }

    if (hashpipe)
    {
        free(hashpipe);
    }
    if (feature_weight)
{
    free(feature_weight);
    }
    if (order_no)
{
free(order_no);
}


    if (tprob <= min_success)
    {
        if (user_trace)
        {
            fprintf(stderr, "CLASSIFY was a FAIL, skipping forward.\n");
        }
        //    and do what we do for a FAIL here
#if defined (TOLERATE_FAIL_AND_OTHER_CASCADES)
        csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
        if (internal_trace)
        {
            fprintf(stderr, "CLASSIFY.WINNOW.ALT is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
        }
        CRM_ASSERT(csl->cstmt >= 0);
        CRM_ASSERT(csl->cstmt <= csl->nstmts);
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        return 0;
    }

    //
    //   all done... if we got here, we should just continue execution
    if (user_trace)
        fprintf(stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
    return 0;
}

#else /* CRM_WITHOUT_OSB_WINNOW */

int crm_expr_alt_osb_winnow_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}

#endif /* CRM_WITHOUT_OSB_WINNOW */




int crm_expr_alt_osb_winnow_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}


int crm_expr_alt_osb_winnow_css_migrate(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "OSB-Winnow");
}



