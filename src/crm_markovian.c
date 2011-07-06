//	crm_markovian.c - Markovian tools

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



#if !CRM_WITHOUT_MARKOV




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



//    How to learn Markovian style.
//
int crm_expr_markov_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //     learn the sparse spectrum of this input window as
    //     belonging to a particular type.
    //     learn <flags> (classname) /word/
    //
    int j;
    int i;
    int k;
    int h;                   //  h is our counter in the hashpipe;
    char ptext[MAX_PATTERN]; //  the regex pattern
    int plen;
    long markov_file_length;
    char ltext[MAX_PATTERN]; //  the variable to learn
    int llen;
    char htext[MAX_PATTERN]; //  the hash name
    int hlen;
    int cflags;
    //int eflags;
    struct stat statbuf;        //  for statting the hash file
    int hfsize;                 //  size of the hash file
    FEATUREBUCKET_TYPE *hashes; //  the text of the hash file
    crmhash_t hashpipe[MARKOVIAN_WINDOW_LEN + 1];
    //
    regex_t regcb;
    regmatch_t match[5];    //  we only care about the outermost match
    int textoffset;
    int textmaxoffset;
    int sense;
    int microgroom;
    int max_feature_terms;   //  how many terms do we actually get per word?
    int fev;
    crmhash_t learns_index;
    crmhash_t features_index;

    //    seen_features - for UNIQUEness tests
    int unique_mode;
    unsigned char *seen_features = NULL;

    char *learnfilename;

    if (internal_trace)
        fprintf(stderr, "executing a LEARN\n");

    //   Keep the gcc compiler from complaining about unused variables
    //i = hctable[0];

    //           extract the hash file name
    hlen = crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, MAX_PATTERN, vht, tdw);
    //
    //           extract the variable name (if present)
    llen = crm_get_pgm_arg(ltext, MAX_PATTERN, apb->b1start, apb->b1len);
    llen = crm_nexpandvar(ltext, llen, MAX_PATTERN, vht, tdw);

    //     get the "this is a word" regex
    plen = crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN, vht, tdw);

    //            set our cflags, if needed.  The defaults are
    //            "case" and "affirm", (both zero valued).
    //            and "microgroom" disabled.
    cflags = REG_EXTENDED;
    //eflags = 0;
    sense = +1;
    unique_mode = 0;
    seen_features = NULL;
    if (apb->sflags & CRM_NOCASE)
    {
        cflags |= REG_ICASE;
        //eflags = 1;
        if (user_trace)
            fprintf(stderr, "turning oncase-insensitive match\n");
    }
    if (apb->sflags & CRM_REFUTE)
    {
        sense = -sense;
        if (user_trace)
            fprintf(stderr, " refuting learning\n");
    }
    if (apb->sflags & CRM_UNIQUE)
    {
        if (user_trace)
            fprintf(stderr, " turning on UNIQUE features only.\n");
        unique_mode = 1;
    }

    microgroom = 0;
    if (apb->sflags & CRM_MICROGROOM)
    {
        microgroom = 1;
        if (user_trace)
            fprintf(stderr, " enabling microgrooming.\n");
    }

    //     How many features in a Markovian?  Here's the answer:
    max_feature_terms = (1 << (MARKOVIAN_WINDOW_LEN - 1));
    if (apb->sflags & CRM_UNIGRAM)
    {
        max_feature_terms = 1;
        if (user_trace)
            fprintf(stderr, " enabling unigram-only (Bayesian) features.\n");
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

    //             quick check- does the file even exist?
    if (k != 0)
    {
        //      file didn't exist... create it
        FILE *f;
        CRM_PORTA_HEADER_INFO classifier_info = { 0 };

        if (user_trace)
        {
            fprintf(stderr, "\nHad to create new CSS file %s\n", learnfilename);
        }
        f = fopen(learnfilename, "wb");
        if (!f)
        {
            char dirbuf[DIRBUFSIZE_MAX];

            fev = fatalerror_ex(SRC_LOC(),
                                "\n Couldn't open your new CSS file '%s' for writing; (full path: '%s') errno=%d(%s)\n",
                                learnfilename,
                                mk_absolute_path(dirbuf, WIDTHOF(dirbuf), learnfilename),
                                errno,
                                errno_descr(errno));
            return fev;
        }
        //       do we have a user-specified file size?
        markov_file_length = sparse_spectrum_file_length;
        if (markov_file_length == 0)
        {
            markov_file_length =
                DEFAULT_MARKOVIAN_SPARSE_SPECTRUM_FILE_LENGTH;
        }

        classifier_info.classifier_bits = CRM_MARKOVIAN;
        classifier_info.hash_version_in_use = selected_hashfunction;

        if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
        {
            fev = fatalerror_ex(SRC_LOC(),
                                "\n Couldn't write header to file %s; errno=%d(%s)\n",
                                learnfilename, errno, errno_descr(errno));
            fclose(f);
            return fev;
        }

        //       put in markov_file_length entries of NULL
        if (file_memset(f, 0,
                        markov_file_length * sizeof(FEATUREBUCKET_STRUCT)))
        {
            fev = fatalerror_ex(SRC_LOC(),
                                "\n Couldn't write to file %s; errno=%d(%s)\n",
                                learnfilename, errno, errno_descr(errno));
            fclose(f);
            return fev;
        }

        fclose(f);

        //    and reset the statbuf to be correct
        k = stat(learnfilename, &statbuf);
        CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
    }
    //
    hfsize = statbuf.st_size;

    //
    //         mmap the hash file into memory so we can bitwhack it
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
        fev = fatalerror("Couldn't get access to the statistics file named: ",
                         learnfilename);
        return fev;
    }

    if (user_trace)
    {
        fprintf(stderr, "Sparse spectra file %s has length %d bins\n",
                learnfilename, (int)(hfsize / sizeof(FEATUREBUCKET_STRUCT)));
    }

    //
    //        check the version of the file
    //
    //  if (hashes[0].hash != 0 ||
    //    hashes[0].key  != 0 ||
    //    hashes[0].value!= 0)
    //  {
    //    fev =fatalerror ("The .css file is the wrong version!  Filename is: ",
    //                   learnfilename);
    //    return (fev);
    //  }

    //
    //   now set the hfsize to the number of entries, not the number
    //   of bytes total
    hfsize = hfsize / sizeof(FEATUREBUCKET_STRUCT);


#ifdef OSB_LEARNCOUNTS
    //       Reserve the OSB_LEARNCOUNT buckets, even though Markov doesn't
    //       use them.
    //
    //       We use the reserved h2 == 0 setup for the learncount.
    //
    {
        const char *litf = "Learnings in this file";
        const char *fitf = "Features in this file";
        crmhash_t hcode, h1, h2;
        //
        hcode = strnhash(litf, strlen(litf));
        h1 = hcode % hfsize;
        h2 = 0;
        if (hashes[h1].hash != hcode)
        {
            // initialize the file?
            if (hashes[h1].hash == 0 && hashes[h1].key == 0)
            {
                hashes[h1].hash = hcode;
                hashes[h1].key = 0;
                // [i_a] make sure init is correct, even when we start by learning
                //       in 'refute' mode without learning that sample before!
                hashes[h1].value = ((apb->sflags & CRM_REFUTE) ? /* -1 */ 0 : sense);
                learns_index = h1;
            }
            else
            {
                //fatalerror (" This file should have learncounts, but doesn't!",
                //  " The slot is busy, too.  It's hosed.  Time to die.");
                // goto regcomp_failed;
            }
        }
        else
        {
            if (hashes[h1].key == 0)
            //   the learncount matched.
            {
                learns_index = h1;
                if (sense > 0)
                {
                    hashes[h1].value += sense;
                }
                else
                {
                    if (hashes[h1].value + sense > 0)
                        hashes[h1].value += sense;
                    else
                        hashes[h1].value = 0;
                }
                if (user_trace)
                {
                    fprintf(stderr, "This file has had %u documents learned!\n",
                            (unsigned int)hashes[h1].value);
                }
            }
        }
        hcode = strnhash(fitf, strlen(fitf));
        h1 = hcode % hfsize;
        h2 = 0;
        if (hashes[h1].hash != hcode)
        {
            // initialize the file?
            if (hashes[h1].hash == 0 && hashes[h1].key == 0)
            {
                hashes[h1].hash = hcode;
                hashes[h1].key = 0;
                // [i_a] make sure init is correct, even when we start by learning
                //       in 'refute' mode without learning that sample before!
                hashes[h1].value = ((apb->sflags & CRM_REFUTE) ? /* -1 */ 0 : sense);
                features_index = h1;
            }
            else
            {
                //fatalerror (" This file should have learncounts, but doesn't!",
                //          " The slot is busy, too.  It's hosed.  Time to die.");
                //goto regcomp_failed ;
            }
        }
        else
        {
            if (hashes[h1].key == 0)
            //   the learncount matched.
            {
                features_index = h1;
                /* [i_a] added this code below: update learncount here too! */
                if (sense > 0)
                {
                    hashes[h1].value += sense;
                }
                else
                {
                    if (hashes[h1].value + sense > 0)
                        hashes[h1].value += sense;
                    else
                        hashes[h1].value = 0;
                }
                if (user_trace)
                {
                    fprintf(stderr, "This file has had %u features learned!\n",
                            hashes[h1].value);
                }
            }
        }
    }

#endif


    if (unique_mode)
    {
        seen_features = calloc(hfsize, sizeof(seen_features[0]));
        if (seen_features == NULL)
        {
            untrappableerror(" Couldn't allocate enough memory to keep track",
                             "of nonunique features.  This is deadly");
        }
    }


    //   compile the word regex
    //
    if (internal_trace)
    {
        fprintf(stderr, "\nWordmatch pattern is %s", ptext);
    }

    i = crm_regcomp(&regcb, ptext, plen, cflags);
    if (i != 0)
    {
        crm_regerror(i, &regcb, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
        goto regcomp_failed;
    }


    //   Start by priming the pipe... we will shift to the left next.
    //     sliding, hashing, xoring, moduloing, and incrmenting the
    //     hashes till there are no more.
    k = 0;
    i = 0;

#ifdef OLD_STUPID_VAR_RESTRICTION
    if (llen > 0)
    {
        vhtindex = crm_vht_lookup(vht, ltext, llen, csl->calldepth);
    }
    else
    {
        vhtindex = crm_vht_lookup(vht, ":_dw:", 5, csl->calldepth);
    }

    if (vht[vhtindex] == NULL)
    {
        int q;
        q = fatalerror(" Attempt to LEARN from a nonexistent variable ",
                       ltext);
        return q;
    }
    mdw = NULL;
    if (tdw->filetext == vht[vhtindex]->valtxt)
        mdw = tdw;
    if (cdw->filetext == vht[vhtindex]->valtxt)
        mdw = cdw;
    if (mdw == NULL)
    {
        int q;
        q = fatalerror(" Bogus text block containing variable ", ltext);
        return q;
    }
    textoffset = vht[vhtindex]->vstart;
    textmaxoffset = textoffset + vht[vhtindex]->vlen;
#endif

    //  No need to do any parsing of a box restriction, as those
    //   are passed in to us from the caller.
    textoffset = txtstart;
    textmaxoffset = txtstart + txtlen;

    //   init the hashpipe with 0xDEADBEEF
    for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
    {
        hashpipe[h] = 0xDEADBEEF;
    }

    //    and the big loop...
    i = 0;
    while (k == 0 && textoffset <= textmaxoffset)
    {
        int wlen;
        int slen;

        //  do the regex
        //  slen = endpoint (= start + len)
        //        - startpoint (= curr textoffset)
        //      slen = txtlen;
        slen = textmaxoffset - textoffset;

        // if pattern is empty, extract non graph delimited tokens
        // directly ([[graph]]+) instead of calling regexec (8% faster!)
        if (ptext[0] != 0)
        {
            k = crm_regexec(&regcb, &txtptr[textoffset],
                            slen, WIDTHOF(match), match, 0, NULL);
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
        crm_memmove(tempbuf,
                    &(txtptr[textoffset + match[0].rm_so]),
                    wlen);
        tempbuf[wlen] = 0;

        if (internal_trace)
        {
            fprintf(stderr,
                    "  Learn #%d t.o. %d strt %d end %d len %d is -%s-\n",
                    i,
                    textoffset,
                    (int)match[0].rm_so,
                    (int)match[0].rm_eo,
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
        for (h = MARKOVIAN_WINDOW_LEN - 1; h > 0; h--)
        {
            hashpipe[h] = hashpipe[h - 1];
        }


        //  and put new hash into pipeline
        hashpipe[0] = strnhash(tempbuf, wlen);

        if (internal_trace)
        {
            fprintf(stderr, "  Hashpipe contents: ");
            for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
                fprintf(stderr, " 0x%08lX", (unsigned long int)hashpipe[h]);
            fprintf(stderr, "\n");
        }


        //  and account for the text used up.
        textoffset += match[0].rm_eo;
        i++;

        //        is the pipe full enough to do the hashing?
        if (1)     //  we always run the hashpipe now, even if it's
                   //  just full of 0xDEADBEEF.  (was i >=5)
        {
            crmhash_t hindex;
            crmhash_t h1, h2;
            int th = 0;         // a counter used for TSS tokenizing
            unsigned int incrs;
            int j;
            //
            //     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
            //     (coefficients chosen by requiring superincreasing,
            //     as well as prime)
            //
            th = 0;
            //      for ( j = 0; j <= 15 ; j++)
            for (j = 0; j < max_feature_terms; j++)
            {
#ifdef TGB
                //
                //   Token Grab Bag - ignore sequence, distance. alias
                //
                hindex = hashpipe[0]
                         + (hashpipe[1] *((j >> 0) & 0x0001))
                         + (hashpipe[2] *((j >> 1) & 0x0001))
                         + (hashpipe[3] *((j >> 2) & 0x0001))
                         + (hashpipe[4] *((j >> 3) & 0x0001));
                if (hindex == 0)
                    hindex = 1;
                h1 = hindex;
                hindex = hindex % hfsize;
                if (hindex == 0)
                    hindex = 1;

                //   this is the secondary (crosscut) hash, used for
                //   confirmation of the key value.
                h2 = hashpipe[0]
                     + (hashpipe[1] *((j >> 0) & 0x0001))
                     + (hashpipe[2] *((j >> 1) & 0x0001))
                     + (hashpipe[3] *((j >> 2) & 0x0001))
                     + (hashpipe[4] *((j >> 3) & 0x0001));
                if (h2 == 0)
                    h2 = 0xdeadbeef;
#endif
#ifdef TGB2
                //
                //   Token Grab Bag - ignore sequence, distance.
                //
                hindex = hashpipe[0]
                         + (hashpipe[1] *((j >> 0) & 0x0001))
                         + (hashpipe[2] *((j >> 1) & 0x0001))
                         + (hashpipe[3] *((j >> 2) & 0x0001))
                         + (hashpipe[4] *((j >> 3) & 0x0001));
                if (hindex == 0)
                    hindex = 1;
                h1 = hindex;
                hindex = hindex % hfsize;
                if (hindex == 0)
                    hindex = 1;

                //   this is the secondary (crosscut) hash, used for
                //   confirmation of the key value.
                h2 = hashpipe[0] *hashpipe[0]
                     + (hashpipe[1] *hashpipe[1] *((j >> 0) & 0x0001))
                     + (hashpipe[2] *hashpipe[2] *((j >> 1) & 0x0001))
                     + (hashpipe[3] *hashpipe[3] *((j >> 2) & 0x0001))
                     + (hashpipe[4] *hashpipe[4] *((j >> 3) & 0x0001));
                if (h2 == 0)
                    h2 = 0xdeadbeef;
#endif
#ifdef TSS
                //
                //   Token Grab Bag - ignore sequence, distance, prevent
                //   aliasing (quadratic H2)
                //
                hindex = hashpipe[0]
                         + (hashpipe[1] *((j >> 0) & 0x0001))
                         + (hashpipe[2] *((j >> 1) & 0x0001))
                         + (hashpipe[3] *((j >> 2) & 0x0001))
                         + (hashpipe[4] *((j >> 3) & 0x0001));
                if (hindex == 0)
                    hindex = 1;
                h1 = hindex;
                hindex = hindex % hfsize;
                if (hindex == 0)
                    hindex = 1;

                //   this is the secondary (crosscut) hash, used for
                //   confirmation of the key value.
                th = 2;
                h2 = hashpipe[0];
                if ((j >> 0) & 0x0001)
                {
                    h2 = h2 + hashpipe[1] *th;
                    th++;
                }
                if ((j >> 1) & 0x0001)
                {
                    h2 = h2 + hashpipe[2] *th;
                    th++;
                }
                if ((j >> 2) & 0x0001)
                {
                    h2 = h2 + hashpipe[3] *th;
                    th++;
                }
                if ((j >> 3) & 0x0001)
                {
                    h2 = h2 + hashpipe[4] *th;
                    th++;
                }
                if (h2 == 0)
                    h2 = 0xdeadbeef;
#endif
#ifdef SBPH
                hindex = hashpipe[0]
                         + (3 * hashpipe[1] *((j >> 0) & 0x0001))
                         + (5 * hashpipe[2] *((j >> 1) & 0x0001))
                         + (11 * hashpipe[3] *((j >> 2) & 0x0001))
                         + (23 * hashpipe[4] *((j >> 3) & 0x0001));
                h1 = hindex;

                //   and what's our primary hash index?  Note that
                //   hindex = 0 is reserved for our version and
                //   usage flags, so we autobump those to hindex=1
                hindex = hindex % hfsize;
                if (hindex == 0)
                    hindex = 1;

                //   this is the secondary (crosscut) hash, used for
                //   confirmation of the key value.  Note that it shares
                //   no common coefficients with the previous hash.
                h2 = 7 * hashpipe[0]
                     + (13 * hashpipe[1] *((j >> 0) & 0x0001))
                     + (29 * hashpipe[2] *((j >> 1) & 0x0001))
                     + (51 * hashpipe[3] *((j >> 2) & 0x0001))
                     + (101 * hashpipe[4] *((j >> 3) & 0x0001));
                if (h2 == 0)
                    h2 = 0xdeadbeef;
#endif

#ifdef ARBITRARY_WINDOW_LENGTH
                //////////////////////////////////////////////////
                //
                //     Generic N-length hashing.
                //
                //     first term (0th) is always on
                h1 = hashpipe[0] *hctable[0];
                //     2nd and onward terms are variable.
                ASSERT(2 * MARKOVIAN_WINDOW_LEN <= WIDTHOF(hctable));
                for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
                {
                    h1 += hashpipe[h] *hctable[h * 2] *((j >> (h - 1)) & 0x0001);
                }
                hindex = h1;
                hindex = hindex % hfsize;
                if (hindex == 0)
                    hindex = 1;

                //     0th term is always turned on.
                h2 = hashpipe[0] *hctable[1];
                //     terms 1 through N are variable
                for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
                {
                    h2 += hashpipe[h] *hctable[h * 2 + 1] *((j >> (h - 1)) & 0x0001);
                }
                if (h2 == 0)
                    h2 = 0xDEADBEEF;

#endif
                if (internal_trace)
                {
                    fprintf(stderr, "Polynomial %d has h1:0x%08lX  h2:0x%08lX\n",
                            j, (unsigned long int)h1, (unsigned long int)h2);
                }

                //
                //   we now look at both the primary (h1) and
                //   crosscut (h2) indexes to see if we've got
                //   the right bucket or if we need to look further
                //
                incrs = 0;
                while (hashes[hindex].key != 0
                      &&  (hashes[hindex].hash != h1
                          || hashes[hindex].key  != h2))
                {
                    //
                    incrs++;
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
                        crm_microgroom(hashes, seen_features, hfsize, hindex);
                        //  since things may have moved after a
                        //  microgroom, restart our search
                        hindex = h1 % hfsize;
                        if (hindex == 0)
                            hindex = 1;
                        incrs = 0;
                    }
                    //      check to see if we've incremented ourself all the
                    //      way around the .css file.  If so, we're full, and
                    //      can hold no more features (this is unrecoverable)
                    if (incrs > hfsize - 3)
                    {
                        nonfatalerror("Your program is stuffing too many "
                                      "features into this size .css file.  "
                                      "Adding any more features is "
                                      "impossible in this file.",
                                      "You are advised to build a larger "
                                      ".css file and merge your data into "
                                      "it.");
                        goto learn_end_regex_loop;
                    }
                    hindex++;
                    if (hindex >= hfsize)
                        hindex = 1;
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
                //      always rewrite hash and key, as they may be incorrect
                //    (on a reused bucket) or zero (on a fresh one)
                //
                hashes[hindex].hash = h1;
                hashes[hindex].key  = h2;

                //      watch out - sense may be both + or -, so check before
                //      adding it...
                //
                if (!seen_features || (seen_features[hindex] == 0))
                {
                    if (seen_features)
                    {
                        seen_features[hindex]++;
                    }
                    if (sense > 0
                       && hashes[hindex].value + sense >= FEATUREBUCKET_VALUE_MAX - 1)
                    {
                        hashes[hindex].value = FEATUREBUCKET_VALUE_MAX - 1;
                    }
                    else if (sense < 0 && hashes[hindex].value <= -sense)
                    {
                        hashes[hindex].value = 0;
                    }
                    else
                    {
                        hashes[hindex].value += sense;
                    }
                }
            }
        }
    }
    //   end the while k==0

learn_end_regex_loop:

    //     free up the pattern buffer:
    if (ptext[0] != 0)
        crm_regfree(&regcb);

regcomp_failed:

    //  and remember to let go of all the mmaps (full flush)
    //  crm_munmap_all ();
    crm_force_munmap_addr(hashes);

    //   and let go of the seen_features array
    if (seen_features)
        free(seen_features);
    seen_features = NULL;

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

    return 0;
}



//      How to Markovian CLASSIFY some text.
//
int crm_expr_markov_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
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
    int i, j;
    int h;                   //  we use h for our hashpipe counter, as needed.
    char ptext[MAX_PATTERN]; //  the regex pattern
    int plen;
    //  char ltext[MAX_PATTERN];  //  the variable to classify
    //int llen;
    char ostext[MAX_PATTERN];   //  optional pR offset
    int oslen;
    double pR_offset;
    //  the hash file names
    char htext[MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN];
    int htext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN;
    int hlen;
    //  the match statistics variable
    char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
    char *stext_ptr = stext;
    int stext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);
    char svrbl[MAX_PATTERN]; //  the match statistics text buffer
    int svlen;
    int fnameoffset;
    char fname[MAX_FILE_NAME_LEN];
    // int eflags;
    int cflags;
    int not_microgroom;    //  is microgrooming disabled (fast-quit
                           //   optimization if 0th feature gone)
    int max_feature_terms; //  how many features do we get at each
                           //    pipe position?

    struct stat statbuf;    //  for statting the hash file
                            //  longest association set in the hashing
    crmhash_t hashpipe[MARKOVIAN_WINDOW_LEN + 1];
    regex_t regcb;
    regmatch_t match[5];    //  we only care about the outermost match

    unsigned int fcounts[MAX_CLASSIFIERS]; // total counts for feature normalize
    unsigned int totalcount = 0;

    double cpcorr[MAX_CLASSIFIERS];        // corpus correction factors

#if defined(GER)
    hitcount_t hits[MAX_CLASSIFIERS];      // actual hits per feature per classifier
    hitcount_t totalhits[MAX_CLASSIFIERS]; // actual total hits per classifier
    int totalfeatures;                     //  total features
    hitcount_t htf;                        // hits this feature got.
#else
    double hits[MAX_CLASSIFIERS];    // actual hits per feature per classifier
    int totalhits[MAX_CLASSIFIERS];  // actual total hits per classifier
    int totalfeatures;               //  total features
    double htf;                      // hits this feature got.
#endif
    double tprob;                                       //  total probability in the "success" domain.
    double min_success = 0.5;                           // minimum probability to be considered success

    //  double textlen;    //  text length  - rougly corresponds to
    //  information content of the text to classify

    double ptc[MAX_CLASSIFIERS]; // current running probability of this class
    double renorm = 0.0;
    double pltc[MAX_CLASSIFIERS];  // current local probability of this class
    double plltc[MAX_CLASSIFIERS]; // current local probability of this class

    //   int hfds[MAX_CLASSIFIERS];
    FEATUREBUCKET_STRUCT *hashes[MAX_CLASSIFIERS];
    int hashlens[MAX_CLASSIFIERS];
    char *hashname[MAX_CLASSIFIERS];
    int succhash;
    int vbar_seen;     // did we see '|' in classify's args?
    int maxhash;
    int fnstart, fnlen;
    int fn_start_here;
    int textoffset;
    int textmaxoffset;
    int bestseen;
    int thistotal;
    int feature_weight = 1;

#if 0 /* [i_a] unused */
    double top10scores[10];
    int top10polys[10];
    char top10texts[10][MAX_PATTERN];
#endif

    //    for UNIQUE runs
    int unique_mode = 0;
    unsigned char *seen_features[MAX_CLASSIFIERS];

    if (internal_trace)
        fprintf(stderr, "executing a CLASSIFY\n");

    //          We get the variable block, start, and len from caller
    //
    // llen = crm_get_pgm_arg (ltext, MAX_PATTERN, apb->b1start, apb->b1len);
    // llen = crm_nexpandvar (ltext, llen, MAX_PATTERN, vht, tdw);

    //           extract the hash file names
    hlen = crm_get_pgm_arg(htext, htext_maxlen, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, htext_maxlen, vht, tdw);

    //           extract the "this is a word" regex
    //
    plen = crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN, vht, tdw);

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
            crm_memmove(svrbl, &svrbl[vstart], vlen);
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

    //            set our flags, if needed.  The defaults are
    //            "case"
    cflags = REG_EXTENDED;
    //eflags = 0;

    if (apb->sflags & CRM_NOCASE)
    {
        if (user_trace)
            fprintf(stderr, " setting NOCASE for tokenization\n");
        cflags |= REG_ICASE;
        //eflags = 1;
    }

    not_microgroom = 1;
    if (apb->sflags & CRM_MICROGROOM)
    {
        not_microgroom = 0;
        if (user_trace)
            fprintf(stderr, " disabling fast-skip optimization.\n");
    }

    max_feature_terms = (1 << (MARKOVIAN_WINDOW_LEN - 1));
    if (apb->sflags & CRM_UNIGRAM)
    {
        max_feature_terms = 1;
        if (user_trace)
            fprintf(stderr, " enabling unigram-only (Bayesian) features.\n");
    }

    unique_mode = 0;
    if (apb->sflags & CRM_UNIQUE)
    {
        if (user_trace)
            fprintf(stderr, " setting UNIQUE feature filtering.\n");
        unique_mode = 1;
    }


    //   compile the word regex
    if (internal_trace)
        fprintf(stderr, "\nWordmatch pattern is %s", ptext);
    i = crm_regcomp(&regcb, ptext, plen, cflags);
    if (i != 0)
    {
        crm_regerror(i, &regcb, tempbuf, data_window_size);
        nonfatalerror("Regular Expression Compilation Problem:", tempbuf);
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
        fcounts[i] = 0;     // check later to prevent a divide-by-zero
                            // error on empty .css file
        cpcorr[i] = 0.0;    // corpus correction factors
        hits[i] = 0;        // absolute hit counts
        totalhits[i] = 0;   // absolute hit counts
        ptc[i] = 0.5;       // priori probability
        pltc[i] = 0.5;      // local probability
    }

#if 0 /* [i_a] unused */
    for (i = 0; i < 10; i++)
    {
        top10scores[i] = 0;
        top10polys[i] = 0;
        strcpy(top10texts[i], "");
    }
#endif

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
    //     the certainty of any measure to be in the range:
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
                            nonfatalerror("Couldn't get access to the "
                                          "statistics file named: ",
                                          fname);
                        }
                        else
                        {
                            //  fprintf(stderr, "MMap got %lx\n", hashes);
                            //
                            //     Check to see if this file is the right version
                            //
                            //     FIXME : for now, there's no version number
                            //     associated with a .correllation file
                            //int fev;
                            // if (0)
                            //if (hashes[maxhash][0].hash != 0 ||
                            //          hashes[maxhash][0].key  != 0 ||
                            //  hashes[maxhash][0].value!= 0)
                            //{
                            //  fev = fatalerror ("The .css file is the wrong version!  Filename is: ",
                            //                   fname);
                            //  return (fev);
                            //}


                            //  set this hashlens to the length in
                            //  features instead of the length in bytes.

                            hashlens[maxhash] = hashlens[maxhash] / sizeof(FEATUREBUCKET_STRUCT);
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

        for (ifile = 0; ifile < maxhash; ifile++)
        {
            if (unique_mode != 0)
            {
                //     Note that we _calloc_, not malloc, to zero the memory first.
                seen_features[ifile] = calloc(hashlens[ifile] + 1, sizeof(seen_features[ifile][0]));
                if (seen_features[ifile] == NULL)
                {
                    untrappableerror(" Couldn't allocate enough memory to keep "
                                     " track of nonunique features.  ",
                                     "This is deadly. ");
                }
            }
            else
            {
                seen_features[ifile] = NULL;
            }
        }



        //      count up the total first
        for (ifile = 0; ifile < maxhash; ifile++)
        {
            fcounts[ifile] = 0;
            //
            //   GROT GROT GROT
            //
            //   Comment out the next two lines to double speed!!!
            //   This will break your "totalcounts" diag output, but
            //   that doesn't really get used.
            //
            {
                int k;

                for (k = 1; k < hashlens[ifile]; k++)
                    fcounts[ifile] = fcounts[ifile] + hashes[ifile][k].value;
                if (fcounts[ifile] == 0)
                    fcounts[ifile] = 1;
                totalcount = totalcount + fcounts[ifile];
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
    thistotal = 0;

#ifdef OLD_STUPID_VAR_RESTRICTION
    if (llen > 0)
    {
        vhtindex = crm_vht_lookup(vht, ltext, llen, csl->calldepth);
    }
    else
    {
        vhtindex = crm_vht_lookup(vht, ":_dw:", 5, csl->calldepth);
    }
    if (vht[vhtindex] == NULL)
    {
        return fatalerror(" Attempt to CLASSIFY from a nonexistent variable ",
                          ltext);
    }
    mdw = NULL;
    if (tdw->filetext == vht[vhtindex]->valtxt)
        mdw = tdw;
    if (cdw->filetext == vht[vhtindex]->valtxt)
        mdw = cdw;
    if (mdw == NULL)
        return fatalerror(" Bogus text block containing variable ", ltext);

    textoffset = vht[vhtindex]->vstart;
    textmaxoffset = textoffset + vht[vhtindex]->vlen;

    textlen = (vht[vhtindex]->vlen);
    if (textlen < 1.0)
        textlen = 1.0;
#endif

    textoffset = txtstart;
    textmaxoffset = txtstart + txtlen;

    //   init the hashpipe with 0xDEADBEEF
    for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
    {
        hashpipe[h] = 0xDEADBEEF;
    }

    totalfeatures = 0;

    //  stop when we no longer get any regex matches
    //   possible edge effect here- last character must be matchable, yet
    //    it's also the "end of buffer".
    {
        int k = 0;

        while (k == 0 && textoffset <= textmaxoffset)
        {
            int wlen;
            int slen;
            //unsigned char *ptok = &(mdw->filetext[textoffset]);
            //unsigned char *ptok_max = &(mdw->filetext[textmaxoffset]);

            //  do the regex
            //      slen = vht[vhtindex]->vstart + vht[vhtindex]->vlen - textoffset ;
            //slen = txtlen;
            slen = textmaxoffset - textoffset;

            // if pattern is empty, extract non graph delimited tokens
            // directly ([[graph]]+) instead of calling regexec  (8% faster!)
            if (ptext[0] != 0)
            {
                k = crm_regexec(&regcb, &txtptr[textoffset],
                                slen, WIDTHOF(match), match, 0, NULL);
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
            crm_memmove(tempbuf,
                        &(txtptr[textoffset + match[0].rm_so]),
                        wlen);
            tempbuf[wlen] = 0;

            if (internal_trace)
            {
                fprintf(stderr,
                        "  Classify #%d t.o. %d strt %d end %d len %d is -%s-\n",
                        i,
                        textoffset,
                        (int)match[0].rm_so,
                        (int)match[0].rm_eo,
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
            for (h = MARKOVIAN_WINDOW_LEN - 1; h > 0; h--)
            {
                hashpipe[h] = hashpipe[h - 1];
            }


            //  and put new hash into pipeline
            hashpipe[0] = strnhash(tempbuf, wlen);

            if (0)
            {
                fprintf(stderr, "  Hashpipe contents: ");
                for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
                    fprintf(stderr, " 0x%08lX", (unsigned long int)hashpipe[h]);
                fprintf(stderr, "\n");
            }

            //   account for the text we used up...
            textoffset += match[0].rm_eo;
            i++;

            //        is the pipe full enough to do the hashing?
            if (1) //  we init with 0xDEADBEEF, so the pipe is always full (i >=5)
            {
                int j;
                int l;
                unsigned int th = 0;  //  a counter used only in TSS hashing
                crmhash_t hindex;
                crmhash_t h1, h2;
                int skip_this_feature = 0;
                //unsigned int good, evil;
                //
                //     Hash polynomial: h0 + 3h1 + 5h2 +9h3 +17h4
                //     (coefficients chosen by formula of 1 + 2^n)
                //
                //    GROT GROT GROT  make the order of the SBPH
                //    a compile-time parameter
                th = 0;
                //      for ( j = 0; j < 16 ; j++)
                for (j = 0;
                     j < max_feature_terms;
                     j++)
                {
#ifdef FOO
                    //
                    //   First Order Only - only use 1 token
                    //
                    hindex = hashpipe[0];
                    if (hindex == 0)
                        hindex = 1;
                    h1 = hindex;

                    //   this is the secondary (crosscut) hash, used for
                    //   confirmation of the key value.
                    h2 =   hashpipe[0];
                    if (h2 == 0)
                        h2 = 0xdeadbeef;
                    j = 99999;
#endif
#ifdef TGB
                    //
                    //   Token Grab Bag - ignore sequence, distance, allow
                    //   aliasing ( note that H2 is linear ).
                    //
                    hindex = hashpipe[0]
                             + (hashpipe[1] *((j >> 0) & 0x0001))
                             + (hashpipe[2] *((j >> 1) & 0x0001))
                             + (hashpipe[3] *((j >> 2) & 0x0001))
                             + (hashpipe[4] *((j >> 3) & 0x0001));
                    if (hindex == 0)
                        hindex = 1;
                    h1 = hindex;

                    //   this is the secondary (crosscut) hash, used for
                    //   confirmation of the key value.
                    h2 =   hashpipe[0]
                         + (hashpipe[1] *((j >> 0) & 0x0001))
                         + (hashpipe[2] *((j >> 1) & 0x0001))
                         + (hashpipe[3] *((j >> 2) & 0x0001))
                         + (hashpipe[4] *((j >> 3) & 0x0001));
                    if (h2 == 0)
                        h2 = 0xdeadbeef;
#endif
#ifdef TGB2
                    //
                    //   Token Grab Bag - ignore sequence, distance, prevent
                    //   aliasing (note that H2 is quadratic ).
                    //
                    hindex = hashpipe[0]
                             + (hashpipe[1] *((j >> 0) & 0x0001))
                             + (hashpipe[2] *((j >> 1) & 0x0001))
                             + (hashpipe[3] *((j >> 2) & 0x0001))
                             + (hashpipe[4] *((j >> 3) & 0x0001));
                    if (hindex == 0)
                        hindex = 1;
                    h1 = hindex;

                    //   this is the secondary (crosscut) hash, used for
                    //   confirmation of the key value.
                    h2 =   hashpipe[0] *hashpipe[0]
                         + (hashpipe[1] *hashpipe[1] *((j >> 0) & 0x0001))
                         + (hashpipe[2] *hashpipe[2] *((j >> 1) & 0x0001))
                         + (hashpipe[3] *hashpipe[3] *((j >> 2) & 0x0001))
                         + (hashpipe[4] *hashpipe[4] *((j >> 3) & 0x0001));
                    if (h2 == 0)
                        h2 = 0xdeadbeef;
#endif
#ifdef TSS
                    //
                    //   Token Seqence Sensitive - ignore distance, prevent
                    //   aliasing (quadratic H2)
                    //
                    hindex = hashpipe[0]
                             + (hashpipe[1] *((j >> 0) & 0x0001))
                             + (hashpipe[2] *((j >> 1) & 0x0001))
                             + (hashpipe[3] *((j >> 2) & 0x0001))
                             + (hashpipe[4] *((j >> 3) & 0x0001));
                    if (hindex == 0)
                        hindex = 1;
                    h1 = hindex;

                    //   this is the secondary (crosscut) hash, used for
                    //   confirmation of the key value.
                    th = 2;
                    h2 =   hashpipe[0];
                    if ((j >> 0) & 0x0001)
                    {
                        h2 = h2 + hashpipe[1] *th;
                        th++;
                    }
                    if ((j >> 1) & 0x0001)
                    {
                        h2 = h2 + hashpipe[2] *th;
                        th++;
                    }
                    if ((j >> 2) & 0x0001)
                    {
                        h2 = h2 + hashpipe[3] *th;
                        th++;
                    }
                    if ((j >> 3) & 0x0001)
                    {
                        h2 = h2 + hashpipe[4] *th;
                        th++;
                    }
#endif
#ifdef SBPH
                    hindex = hashpipe[0]
                             + (3 * hashpipe[1] *((j >> 0) & 0x0001))
                             + (5 * hashpipe[2] *((j >> 1) & 0x0001))
                             + (11 * hashpipe[3] *((j >> 2) & 0x0001))
                             + (23 * hashpipe[4] *((j >> 3) & 0x0001));
                    if (hindex == 0)
                        hindex = 1;
                    h1 = hindex;

                    //   this is the secondary (crosscut) hash, used for
                    //   confirmation of the key value.
                    h2 =    7 * hashpipe[0]
                         + (13 * hashpipe[1] *((j >> 0) & 0x0001))
                         + (29 * hashpipe[2] *((j >> 1) & 0x0001))
                         + (51 * hashpipe[3] *((j >> 2) & 0x0001))
                         + (101 * hashpipe[4] *((j >> 3) & 0x0001));
                    if (h2 == 0)
                        h2 = 0xdeadbeef;
#endif


#ifdef ARBITRARY_WINDOW_LENGTH
                    //////////////////////////////////////////////////
                    //
                    //     Generic N-length hashing.
                    //
                    //     first term (0th) is always on
                    h1 = hashpipe[0] *hctable[0];
                    //     2nd and onward terms are variable.
                    for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
                    {
                        h1 = h1 + hashpipe[h] *hctable[h * 2] *((j >> (h - 1)) & 0x0001);
                    }
                    hindex = h1;
                    if (hindex == 0)
                        hindex = 1;

                    //     0th term is always turned on.
                    h2 = hashpipe[0] *hctable[1];
                    //     terms 1 through N are variable
                    for (h = 0; h < MARKOVIAN_WINDOW_LEN; h++)
                    {
                        h2 = h2 + hashpipe[h] *hctable[h * 2 + 1] *((j >> (h - 1)) & 0x0001);
                    }
                    if (h2 == 0)
                        h2 = 0xDEADBEEF;
#endif
                    //
                    //    Note - a strict interpretation of Bayesian
                    //    chain probabilities should use 0 as the initial
                    //    state.  However, because we rapidly run out of
                    //    significant digits, we use a much less strong
                    //    initial state.   Note also that any nonzero
                    //    positive value prevents divide-by-zero
                    //
#ifdef ENTROPIC_WEIGHTS
                    //
                    //   Calculate entropic weight of this feature.
                    //   (because these are correllated features, this is
                    //   linear, not logarithmic)
                    //
                    //   These weights correspond to the number of elements
                    //   in the hashpipe that are used for this particular
                    //   calculation, == 1 + bitcount(Jval)

                    {
                        const int ew[16] = // Jval
                        {
                            1,  // 0
                            2,  // 1
                            2,  // 2
                            3,  // 3
                            2,  // 4
                            3,  // 5
                            3,  // 6
                            4,  // 7
                            2,  // 8
                            3,  // 9
                            3,  // 10
                            4,  // 11
                            3,  // 12
                            4,  // 13
                            4,  // 14
                            5
                        };      // 15

                        feature_weight = ew[j];
                    }
#endif
#ifdef MARKOV_WEIGHTS
                    //
                    //   Calculate entropic weight of this feature.
                    //    However, this is based on a Hidden Markov Model
                    //    calculation.  Maybe it's right, maybe not.  It does
                    //    seem to work better than constant or entropic...
                    {
                        const int ew[16] = // Jval
                        {
                            1,  // 0
                            2,  // 1
                            2,  // 2
                            4,  // 3
                            2,  // 4
                            4,  // 5
                            4,  // 6
                            8,  // 7
                            2,  // 8
                            4,  // 9
                            4,  // 10
                            8,  // 11
                            4,  // 12
                            8,  // 13
                            8,  // 14
                            16
                        };       // 15

                        feature_weight = ew[j];
                    }
#endif
#ifdef SUPER_MARKOV_WEIGHTS
                    //
                    //   Calculate entropic weight of this feature.
                    //   However, this is based on a more agressive Hidden
                    //   Markov Model calculation.  Maybe it's right, maybe
                    //   not.  However, testing shows that Super-Markov is
                    //   more accurate than constant, entropic, or straight Markov,
                    //   with errcounts of 69, 69, 63, and 56 per 5k,respectively.
                    //
                    //    hibits are  0, 1,  2.  3,   4,    5
                    //    weights are 1, 4, 16, 64, 256, 1024
                    {
                        const int ew[32] = // Jval
                        {
                            1,     // 0
                            4,     // 1
                            4,     // 2
                            16,    // 3
                            4,     // 4
                            16,    // 5
                            16,    // 6
                            64,    // 7
                            4,     // 8
                            16,    // 9
                            16,    // 10  - A
                            64,    // 11  - B
                            16,    // 12  - C
                            64,    // 13  - D
                            64,    // 14  - E
                            256,   // 15 -  F
                            4,     // 16 - 10
                            16,    // 17 - 11
                            16,    // 18 - 12
                            64,    // 19 - 13
                            16,    // 20 - 14
                            64,    // 21 - 15
                            64,    // 22 - 16
                            256,   // 23 - 17
                            16,    // 24 - 18
                            64,    // 25 - 19
                            64,    // 26 - 1A
                            256,   // 27 - 1B
                            64,    // 28 - 1C
                            256,   // 29 - 1D
                            256,   // 30 - 1E
                            1024   // 31 - 1F
                        };
                        feature_weight = ew[j];
                    }
#endif
#ifdef BREYER_CHHABRA_SIEFKES_WEIGHTS
                    //
                    //   This uses the Breyer-Chhabra-Siefkes model that
                    //   uses coefficients of 1, 3, 13,, 75, and 541, which
                    //   assures complete override for any shorter string in
                    //   a single occurrence.
                    //
                    {
                        const int ew[16] = // Jval
                        {
                            1,   // 0
                            3,   // 1
                            3,   // 2
                            13,  // 3
                            3,   // 4
                            13,  // 5
                            13,  // 6
                            75,  // 7
                            3,   // 8
                            13,  // 9
                            13,  // 10
                            75,  // 11
                            13,  // 12
                            75,  // 13
                            75,  // 14
                            541
                        };        // 15

                        feature_weight = ew[j];
                    }
#endif
#ifdef BCS_MWS_WEIGHTS
                    //
                    //   This uses the Breyer-Chhabra-Siefkes model that
                    //   uses coefficients of 1, 3, 13,, 75, and 541, which
                    //   assures complete override for any shorter string in
                    //   a single occurrence.
                    //
                    {
                        const int ew[] = // Jval
                        {
                            1,     // 0
                            3,     // 1
                            3,     // 2
                            13,    // 3
                            3,     // 4
                            13,    // 5
                            13,    // 6
                            75,    // 7
                            3,     // 8
                            13,    // 9
                            13,    // 10  - A
                            75,    // 11  - B
                            13,    // 12  - C
                            75,    // 13  - D
                            75,    // 14  - E
                            541,   // 15 -  F
                            3,     // 16 - 10
                            13,    // 17 - 11
                            13,    // 18 - 12
                            75,    // 19 - 13
                            13,    // 20 - 14
                            75,    // 21 - 15
                            75,    // 22 - 13
                            541,   // 23 - 17
                            13,    // 24 - 18
                            75,    // 25 - 19
                            75,    // 26 - 1A
                            541,   // 27 - 1B
                            75,    // 28 - 1C
                            541,   // 29 - 1D
                            541,   // 30 - 1E
                            4683   // 31 - 1F
                        };
                        feature_weight = ew[j];
                    }
#endif
#ifdef BCS_EXP_WEIGHTS
                    //
                    //   This uses the Breyer-Chhabra-Siefkes model that
                    //   uses coefficients of 1, 3, 13,, 75, and 541, which
                    //   assures complete override for any shorter string in
                    //   a single occurrence.
                    //
                    {
                        const int ew[] = // Jval
                        {
                            1,      // 0
                            8,      // 1
                            8,      // 2
                            64,     // 3
                            8,      // 4
                            64,     // 5
                            64,     // 6
                            512,    // 7
                            8,      // 8
                            64,     // 9
                            64,     // 10  - A
                            512,    // 11  - B
                            64,     // 12  - C
                            512,    // 13  - D
                            512,    // 14  - E
                            4096,   // 15 -  F
                            8,      // 16 - 10
                            64,     // 17 - 11
                            64,     // 18 - 12
                            512,    // 19 - 13
                            64,     // 20 - 14
                            512,    // 21 - 15
                            512,    // 22 - 13
                            4096,   // 23 - 17
                            64,     // 24 - 18
                            512,    // 25 - 19
                            512,    // 26 - 1A
                            4096,   // 27 - 1B
                            512,    // 28 - 1C
                            4096,   // 29 - 1D
                            4096,   // 30 - 1E
                            32768   // 31 - 1F
                        };
                        feature_weight = ew[j];
                    }
#endif
#ifdef BREYER_CHHABRA_SIEFKES_BASE7_WEIGHTS
                    //
                    //   This uses the Breyer-Chhabra-Siefkes base 7 model that
                    //   uses coefficients of 1, 7, 49, 343, 2401 which
                    //   assures complete override for any shorter string in
                    //   a single occurrence.
                    //
                    {
                        const int ew[16] = // Jval
                        {
                            1,    // 0
                            7,    // 1
                            7,    // 2
                            49,   // 3
                            7,    // 4
                            49,   // 5
                            49,   // 6
                            343,  // 7
                            7,    // 8
                            49,   // 9
                            343,  // 10
                            343,  // 11
                            49,   // 12
                            343,  // 13
                            343,  // 14
                            2401
                        };         // 15

                        feature_weight = ew[j];
                    }
#endif

                    //       Zero out "Hits This Feature"
                    htf = 0;
                    totalfeatures++;
                    //
                    //    calculate the precursors to the local probabilities;
                    //    these are the hits[k] array, and the htf total.
                    //
                    skip_this_feature = 0;
                    {
                        int k;

                        for (k = 0; k < maxhash; k++)
                        {
                            int lh, lh0;
                            lh = hindex % (hashlens[k]);
                            if (lh == 0)
                                lh = 1;
                            lh0 = lh;
                            hits[k] = 0;
                            while (hashes[k][lh].key != 0
                                  && (hashes[k][lh].hash != h1
                                     || hashes[k][lh].key  != h2))
                            {
                                lh++;
                                if (lh >= hashlens[k])
                                    lh = 1;
                                if (lh == lh0)
                                    break; // wraparound
                            }
                            if (hashes[k][lh].hash == h1 && hashes[k][lh].key == h2)
                            {
                                //
                                l = hashes[k][lh].value * feature_weight;
                                totalhits[k] += l;                         // remember totalhits  /* [i_a] compare this code with elsewhere; here totalhits is counted different; should it be double type??? */
                                hits[k] = l * cpcorr[k];                   // remember corr. hits
                                htf += hits[k];                            // and hits-this-feature
                                if (unique_mode)
                                {
                                    if (seen_features[k][lh] > 0)
                                        skip_this_feature = 1;
                                    if (seen_features[k][lh] < 250)
                                        seen_features[k][lh]++;
                                }
                            }
                        }
                    }


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
                    //      I'm guessing this- the validity is the differential
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

                    if (!skip_this_feature)
                    {
                        int k;

#ifdef STATIC_LOCAL_PROBABILITIES
                        //fgood = good;
                        //fevil = evil;
                        //pmag = (fgood - fevil) / ( 2 * (fgood + fevil + 1 )  );
                        //plnic = 0.5 - pmag;
                        //plic = 1.0 - plnic;

                        for (k = 0; k < maxhash; k++)
                        {
                            pltc[k] = 0.5
                                      + ((hits[k] - (htf - hits[k]))
                                         / (LOCAL_PROB_DENOM * (htf + 1.0)));
                        }
#endif
#ifdef LENGTHBASED_LOCAL_PROBABILIIES
                        // fgood = good;
                        //fevil = evil;
                        //pmag = (fgood - fevil) / ( (fgood + fevil + 1) * textlen);
                        //plnic = 0.5 - pmag;
                        //plic = 1.0 - plnic;

                        for (k = 0; k < maxhash; k++)
                        {
                            pltc[k] = 0.5
                                      + ((hits[k] - (htf - hits[k]))
                                         / ((htf + 1) * textlen));
                        }
#endif


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
                        //    is where we actually care.
                        //
                        //      update the global probabilities

                        //  ptemp = ( pic*plic)  / ((pic * plic) + (pnic * plnic));
                        //  pnic  = (pnic*plnic) / ((pic * plic) + (pnic * plnic));
                        //  pic   = ptemp;

                        //   calculate renormalizer (the Bayesian formula's denomenator)
                        renorm = 0.0;
                        //   now calculate the per-ptc numerators
                        plltc[0] = 0; // keep gcc from complaining about unused vars.
#ifdef USE_PEAK
                        for (k = 0; k < maxhash; k++)
                            renorm = renorm + (ptc[k] *plltc[k]);

                        if (j == 0)
                            for (k = 0; k < maxhash; k++)
                                plltc[k] = pltc[k];
                        if (j < 15)
                            for (k = 0; k < maxhash; k++)
                                if (pltc[k] > plltc[k])
                                    plltc[k] = pltc[k];

                        if (j == 15)
                            for (k = 0; k < maxhash; k++)
                                ptc[k] = (ptc[k] *plltc[k]) / renorm;
#else
                        for (k = 0; k < maxhash; k++)
                            renorm = renorm + (ptc[k] *pltc[k]);

                        for (k = 0; k < maxhash; k++)
                            ptc[k] = (ptc[k] *pltc[k]) / renorm;
#endif

                        //
                        //   Arne's Optimization:  If the low-order feature
                        //   (that is, hashpipe[0]) isn't there, then there's no
                        //   possibility of other features being there either.
                        //   So, we can abort the rest of this classification
                        //   for this particular file at least.
                        //
                        //   Of course this is only legal to do if the file has
                        //   not been microgroomed- otherwise the low-order
                        //   (single-word token) feature might already
                        //   have been deleted or microgroomed away, causing
                        //   the higher-order feature to be missed.

#define ARNE_OPTIMIZATION
#ifdef ARNE_OPTIMIZATION
                        if (not_microgroom && (/* dangerous, should have (int) cast */ htf) == 0 && j == 0)
                            j = 999999;
#endif

                        //   if we have underflow (any probability == 0.0 ) then
                        //   bump the probability back up to 10^-308, or
                        //   whatever a small multiple of the minimum double
                        //   precision value is on the current platform.
                        //
                        for (k = 0; k < maxhash; k++)
                            if (ptc[k] < 10 * DBL_MIN)
                                ptc[k] = 10 * DBL_MIN;

                        //
                        //      part 2) renormalize to sum probabilities to 1.0
                        //
                        renorm = 0.0;
                        for (k = 0; k < maxhash; k++)
                            renorm = renorm + ptc[k];
                        for (k = 0; k < maxhash; k++)
                            ptc[k] = ptc[k] / renorm;

                        for (k = 0; k < maxhash; k++)
                            if (ptc[k] < 10 * DBL_MIN)
                                ptc[k] = 10 * DBL_MIN;

                        //if (pnic < pic)
                        //    { pic = 1.0 - pnic;} else { pnic = 1.0 - pic; }

                        if (internal_trace)
                        {
                            for (k = 0; k < maxhash; k++)
                            {
                                // fprintf(stderr, "ZZZ\n");
                                fprintf(stderr,
                                        " poly: %d  filenum: %d, HTF: %7ld, hits: %7ld, Pl: %6.4e, Pc: %6.4e\n",
                                        j, k, (long int)htf, (long int)hits[k], pltc[k], ptc[k]);
                            }
                        }
                        //
                        //    avoid the fencepost error for window=1
                        if (MARKOVIAN_WINDOW_LEN == 1)
                        {
                            j = 99999;
                        }
                    }
                }
            }
        }  //  end of repeat-the-regex loop
    }
classify_end_regex_loop:
    {
        int k;

        //   and one last chance to force probabilities into the non-stuck zone
        //
        //  if (pic == 0.0 ) pic = DBL_MIN;
        //if (pnic == 0.0 ) pnic = DBL_MIN;
        for (k = 0; k < maxhash; k++)
        {
            if (ptc[k] < 10 * DBL_MIN)
                ptc[k] = 10 * DBL_MIN;
        }


        if (user_trace)
        {
            for (k = 0; k < maxhash; k++)
                fprintf(stderr, "Probability of match for file %d: %f\n", k, ptc[k]);
        }
        //
        tprob = 0.0;
        for (k = 0; k < succhash; k++)
        {
            tprob += ptc[k];
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
        double overall_pR;
        int m;

        // buf[0] = 0;
        accumulator = 10 * DBL_MIN;
        for (m = 0; m < succhash; m++)
        {
            accumulator += ptc[m];
        }
        remainder = 10 * DBL_MIN;
        for (m = succhash; m < maxhash; m++)
        {
            remainder += ptc[m];
        }
        overall_pR = log10(accumulator) - log10(remainder);

        //   note also that strcat _accumulates_ in stext.
        //  There would be a possible buffer overflow except that _we_ control
        //   what gets written here.  So it's no biggie.

        if (tprob > min_success)
        {
            // if a pR offset was given, print it together with the real pR
            if (oslen > 0)
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY succeeds; (markovian) success probability: "
                         "%6.4f  pR: %6.4f / %6.4f\n",
                         tprob, overall_pR, pR_offset);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY succeeds; (markovian) success probability: "
                         "%6.4f  pR: %6.4f\n", tprob, overall_pR);
            }
        }
        else
        {
            // if a pR offset was given, print it together with the real pR
            if (oslen > 0)
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY fails; (markovian) success probability: "
                         "%6.4f  pR: %6.4f / %6.4f\n",
                         tprob, overall_pR, pR_offset);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY fails; (markovian) success probability: "
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
            if (ptc[m] > ptc[bestseen])
            {
                bestseen = m;
            }
        }

        remainder = 10 * DBL_MIN;
        for (m = 0; m < maxhash; m++)
        {
            if (bestseen != m)
            {
                remainder += ptc[m];
            }
        }

        //   ... and format some output of best single matching file
        //
        // if (bestseen < maxhash)
        snprintf(stext_ptr, stext_maxlen, "Best match to file #%d (%s) "
                                          "prob: %6.4f  pR: %6.4f\n",
                 bestseen,
                 hashname[bestseen],
                 ptc[bestseen],
                 (log10(ptc[bestseen]) - log10(remainder)));
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
            remainder = 10 * DBL_MIN;
            for (k = 0; k < maxhash; k++)
            {
                if (k != m)
                {
                    remainder += ptc[k];
                }
            }
            CRM_ASSERT(m >= 0);
            CRM_ASSERT(m < maxhash);
            snprintf(stext_ptr, stext_maxlen,
                     "#%d (%s):"
                     " features: %d, hits: %d, prob: %3.2e, pR: %6.2f\n",
                     m,
                     hashname[m],
                     fcounts[m],
                     (int)totalhits[m],
                     ptc[m],
                     (log10(ptc[m]) - log10(remainder)));
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
        crm_destructive_alter_nvariable(svrbl, svlen,                stext, (int)strlen(stext), csl->calldepth);
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
            //   and let go of the seen_features array
            if (seen_features[k])
                free(seen_features[k]);
            seen_features[k] = NULL;

            //
            //  Free the hashnames, to avoid a memory leak.
            //
            if (hashname[k])
            {
                free(hashname[k]);
            }
        }
    }

    //  and let go of the regex buffery
    if (ptext[0] != 0)
    {
        crm_regfree(&regcb);
    }


    if (tprob <= min_success)
    {
        if (user_trace)
        {
            fprintf(stderr, "CLASSIFY was a FAIL, skipping forward.\n");
        }
        //    and do what we do for a FAIL here
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
        if (internal_trace)
        {
            fprintf(stderr, "CLASSIFY.MARKOVIAN is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
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
regcomp_failed:
    return 0;
}


#else /* CRM_WITHOUT_MARKOV */

int crm_expr_markov_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    fatalerror_ex(SRC_LOC(),
                  "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                  "You may want to run 'crm -v' to see which classifiers are available.\n",
                  "Markov");
}


int crm_expr_markov_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}

#endif /* CRM_WITHOUT_MARKOV */



int crm_expr_markov_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}


int crm_expr_markov_css_migrate(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "Markov");
}



