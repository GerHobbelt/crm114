//  crm_osbf_maintenance_.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2004-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//
//  OBS: CSS header structure and pruning method modified for OSBF classifier.
//       See functions crm_osbf_microgroom and crm_osbf_create_cssfile, below,
//       for details.  -- Fidelis Assis - 2004/10/20
//
//  include some standard files

#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

#include "crm114_osbf.h"

/* Version names */
char *CSS_version_name[] =
{
    "SBPH-Markovian",
    "OSB-Bayes",
    "Correlate",
    "Neural",
    "OSB-Winnow",
    "OSBF-Bayes",
    "Unknown"
};


//    microgroom flag for osbf
static int osbf_microgroom = 0;

// turn microgroom on (1) or off (0)
void crm_osbf_set_microgroom(int value)
{
    osbf_microgroom = value;
}


//
//     How to microgroom a .css file that's getting full
//
//     NOTA BENE NOTA BENE NOTA BENE NOTA BENE
//
//         This whole section of code is under intense develoment; right now
//         it "works" but not any better than nothing at all.  Be warned
//         that any patches issued on it may well never see the light of
//         day, as intense testing and comparison may show that the current
//         algorithms are, well, suckful.
//
//
//     There are two steps to microgrooming - first, since we know we're
//     already too full, we execute a 'zero unity bins'.
//
void crm_osbf_microgroom(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int                                hindex)
{
    int i, j, k;
    static int microgroom_count = 0;
    int packstart;
    int packlen;
    int zeroed_countdown, max_zeroed_buckets;
    int min_value, min_value_any, distance, max_distance;
    int groom_any = 0;
    OSBF_FEATUREBUCKET_STRUCT *h;

    // if not set by command line, use default
    if (microgroom_chain_length == 0)
        microgroom_chain_length = OSBF_MICROGROOM_CHAIN_LENGTH;
    if (microgroom_stop_after == 0)
        microgroom_stop_after = OSBF_MICROGROOM_STOP_AFTER;


    // make h point to the first feature bucket
    h = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;

    zeroed_countdown = microgroom_stop_after;
    i = j = k = 0;
    microgroom_count++;

    if (user_trace)
    {
        if (microgroom_count == 1)
            fprintf(stderr, "CSS file too full: microgrooming this css chain: ");
        fprintf(stderr, " %d ", microgroom_count);
    }

    //   micropack - start at initial chain start, move to back of
    //   chain that overflowed, then scale just that chain.

    i = j = hindex % header->buckets;
    min_value = OSBF_FEATUREBUCKET_VALUE_MAX;
    min_value_any = GET_BUCKET_VALUE(h[i]);
    while (BUCKET_IN_CHAIN(h[i]))
    {
        if (GET_BUCKET_VALUE(h[i]) < min_value && !BUCKET_IS_LOCKED(h[i]))
            min_value = GET_BUCKET_VALUE(h[i]);
        if (GET_BUCKET_VALUE(h[i]) < min_value_any)
            min_value_any = GET_BUCKET_VALUE(h[i]);
        if (i == 0)
            i = header->buckets - 1;
        else
            i--;
        if (i == j)
            break; // don't hang if we have a 100% full .css file
        // fprintf(stderr, "-");
    }

    if (min_value == OSBF_FEATUREBUCKET_VALUE_MAX)
    {
        /* no unlocked bucket avaiable so groom any */
        groom_any = 1;
        min_value = min_value_any;
    }

    //     now, move our index to the first bucket in this chain.
    i++;
    if (i >= header->buckets)
        i = 0;
    packstart = i;

    /* i = j = hindex % header->buckets; */
    while (BUCKET_IN_CHAIN(h[i]))
    {
        i++;
        if (i == header->buckets)
            i = 0;
        if (i == packstart)
            break; // don't hang if we have a 100% full .cfc file
    }
    //     now, our index is right after the last bucket in this chain.

    /* if there was a wraparound, full .cfc file,    */
    /* i == packstart and packlen == header->buckets */
    if (i > packstart)
        packlen = i - packstart;
    else
        packlen = header->buckets + i - packstart;

    //   This pruning method zeroes buckets with minimum count in the chain.
    //   It tries first buckets with minimum distance to their right position,
    //   to increase the chance of zeroing older buckets first. If none with
    //   distance 0 is found, the distance is increased until at least one
    //   bucket is zeroed.
    //
    //   We keep track of how many buckets we've zeroed and we stop
    //   zeroing additional buckets after that point.   NO!  BUG!  That
    //   messes up the tail length, and if we don't repack the tail, then
    //   features in the tail can become permanently inaccessible!   Therefore,
    //   we really can't stop in the middle of the tail (well, we could
    //   stop zeroing, but we need to pass the full length of the tail in.
    //
    //   Note that we can't do this "adaptively" in packcss, because zeroes
    //   there aren't necessarily overflow chain terminators (because -we-
    //   might have inserted them here.
    //
    //   GROT GROT GROT  Note that the following algorithm does multiple
    //   passes to find the lowest-valued features.  In fact, that's
    //   actually rather slow; a better algorithm would keep track of
    //   the N least-valued features in the chain in ONE pass and zero
    //   those.
    //
    //   --
    //   I'm not sure if it's worth working on a better algorithm for this:
    //
    //   This is a statistics report of microgroomings for 4147 messages
    //   of the SpamAssassin corpus. It shows that 77% is done in a single
    //   pass, 95.2% in 1 or 2 passes and 99% in at most 3 passes.
    //
    //   # microgrommings   passes   %    accum. %
    //        232584           1    76.6   76.6
    //         56396           2    18.6   95.2
    //         11172           3     3.7   98.9
    //          2502           4     0.8   99.7
    //           726           5     0.2   99.9
    //           ...
    //   -----------
    //        303773
    //
    //   If we consider only the last 100 microgroomings, when the css
    //   file is full, we'll have the following numbers showing that most
    //   microgroomings (61%) are still done in a single pass, almost 90%
    //   is done in 1 or 2 passes and 97% are done in at most 3 passes:
    //
    //   # microgrommings   passes   %    accum. %
    //          61             1    61      61
    //          27             2    27      88
    //           9             3     9      97
    //           3             4     3     100
    //         ---
    //         100
    //
    //   So, it's not so slow. Anyway, a better algorithm could be
    //   implemented using 2 additional arrays, with MICROGROOM_STOP_AFTER
    //   positions each, to store the indexes of the candidate buckets
    //   found with distance equal to 1 or 2 while we scan for distance 0.
    //   Those with distance 0 are zeroed immediatelly. If none with
    //   distance 0 is found, we'll zero the indexes stored in the first
    //   array. Again, if none is found in the first array, we'll try the
    //   second one. Finally, if none is found in both arrays, the loop
    //   will continue until one bucket is zeroed.
    //
    //   But now comes the question: do the numbers above justify the
    //   additional code/work? I'll try to find out the answer
    //   implementing it :), but this has low priority for now.
    //
    //   -- Fidelis Assis
    //

    // try features in their right place first
    max_distance = 1;

    /* zero up to 50% of packlen */
    /* max_zeroed_buckets = (int) (0.5 * packlen + 0.5); */
    max_zeroed_buckets =  microgroom_stop_after;
    zeroed_countdown = max_zeroed_buckets;

    /*fprintf(stderr, "packstart: %d,  packlen: %d, max_zeroed_buckets: %d\n",
     *  packstart, packlen, max_zeroed_buckets); */

    // while no bucket is zeroed...
    while (zeroed_countdown == max_zeroed_buckets)
    {
        /*
         * fprintf(stderr, "Start: %d, stop_after: %d, max_distance: %d\n", packstart, microgroom_stop_after, max_distance);
         */
        i = packstart;
        while (BUCKET_IN_CHAIN(h[i]) && zeroed_countdown > 0)
        {
            // check if it's a candidate
            if (GET_BUCKET_VALUE(h[i]) == min_value
                && (!BUCKET_IS_LOCKED(h[i]) || (groom_any != 0)))
            {
                // if it is, check the distance
                distance = i - BUCKET_HASH(h[i]) % header->buckets;
                if (distance < 0)
                    distance += header->buckets;
                if (distance < max_distance)
                {
                    BUCKET_RAW_VALUE(h[i]) = 0;
                    zeroed_countdown--;
                }
            }
            i++;
            if (i >= header->buckets)
                i = 0;
        }

        //  if none was zeroed, increase the allowed distance between the
        //  candidade's position and its right place.

        if (zeroed_countdown == max_zeroed_buckets)
            max_distance++;
    }

    /*
     * fprintf(stderr, "Leaving microgroom: %d buckets zeroed at distance %d\n", microgroom_stop_after - zeroed_countdown, max_distance - 1);
     */

    //   now we pack the buckets
    crm_osbf_packcss(header, packstart, packlen);
}

void crm_osbf_packcss(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int packstart, unsigned int packlen)
{
	//    How we pack...
	//
	//    We look at each bucket, and attempt to reinsert it at the "best"
	//    place.  We know at worst it will end up where it already is, and
	//    at best it will end up lower (at a lower index) in the file, except
	//    if it's in wraparound mode, in which case we know it will not get
	//    back up past us (since the file must contain at least one empty)
	//    and so it's still below us in the file.


    OSBF_FEATUREBUCKET_STRUCT *h;

    // make h point to the first feature bucket
    h = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;

    if (packstart + packlen <= header->buckets) //  no wraparound in this case
    {
        crm_osbf_packseg(header, packstart, packlen);
    }
    else        //  wraparound mode - do it as two separate repacks
    {
        crm_osbf_packseg(header, packstart, (header->buckets - packstart));
        crm_osbf_packseg(header, 0, (packlen - (header->buckets - packstart)));
    }
}

void crm_osbf_packseg(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int packstart, unsigned int packlen)
{
    unsigned int ifrom, ito;
    crmhash_t thash, tkey;
    OSBF_FEATUREBUCKET_STRUCT *h;

    // make h point to the first feature bucket
    h = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;

    if (internal_trace)
        fprintf(stderr, " < %d %d >", packstart, packlen);

    // Our slot values are now somewhat in disorder because empty
    // buckets may now have been inserted into a chain where there used
    // to be placeholder buckets.  We need to re-insert slot data in a
    // bucket where it will be found.

    for (ifrom = packstart; ifrom < packstart + packlen; ifrom++)
    {
        //    Now find the next bucket to place somewhere
        thash = BUCKET_HASH(h[ifrom]);
        tkey = BUCKET_KEY(h[ifrom]);

        if (GET_BUCKET_VALUE(h[ifrom]) == 0)
        {
            if (internal_trace)
                fprintf(stderr, "X");
        }
        else
        {
            ito = thash % header->buckets;
            // fprintf(stderr, "a %d", ito);

            while (BUCKET_IN_CHAIN(h[ito])
                   && !BUCKET_HASH_COMPARE(h[ito], thash, tkey))
            {
                ito++;
                if (ito >= header->buckets)
                    ito = 0;
            }

            //   found an empty slot, put this value there, and zero the
            //   original one.  Sometimes this is a noop.  We don't care.

            if (ito != ifrom)
            {
                BUCKET_HASH(h[ito]) = thash;
                BUCKET_KEY(h[ito]) = tkey;
                // move value and lock together
                BUCKET_RAW_VALUE(h[ito]) = BUCKET_RAW_VALUE(h[ifrom]);

                // clean "from" bucket
                BUCKET_HASH(h[ifrom]) = 0;
                BUCKET_KEY(h[ifrom]) = 0;
                BUCKET_RAW_VALUE(h[ifrom]) = 0;
            }

            if (internal_trace)
            {
                if (ifrom == ito)
                    fprintf(stderr, "=");
                if (ito < ifrom)
                    fprintf(stderr, "<");
                if (ito > ifrom)
                    fprintf(stderr, ">");
            }
        }
    }
}

/* get next bucket index */
unsigned int crm_osbf_next_bindex(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int                                          hindex)
{
    hindex++;
    if (hindex >= header->buckets)
        hindex = 0;
    return hindex;
}

/* get index of the last bucket in a chain */
unsigned int crm_osbf_last_in_chain(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int                                            hindex)
{
    unsigned int wraparound;
    OSBF_FEATUREBUCKET_STRUCT *hashes;

    hashes = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;

    /* if the bucket is not in a chain, return an index */
    /* out of the buckets space, equal to the number of */
    /* buckets in the file to indicate an empty chain */
    if (!BUCKET_IN_CHAIN(hashes[hindex]))
        return header->buckets;

    wraparound = hindex;
    while (BUCKET_IN_CHAIN(hashes[hindex]))
    {
        hindex++;
        if (hindex >= header->buckets)
            hindex = 0;

        /* if .cfc file is full return an index out of */
        /* the buckets space, equal to number of buckets */
        /* in the file, plus one */
        if (hindex == wraparound)
            return header->buckets + 1;
    }
    hindex = crm_osbf_prev_bindex(header, hindex);
    return hindex;
}


/* get previous bucket index */
unsigned int crm_osbf_prev_bindex(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int                                          hindex)
{
    if (hindex == 0)
        hindex = header->buckets - 1;
    else
        hindex--;
    return hindex;
}

/* get index of the first bucket in a chain */
unsigned int crm_osbf_first_in_chain(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int                                             hindex)
{
    unsigned int wraparound;
    OSBF_FEATUREBUCKET_STRUCT *hashes;

    hashes = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;

    /* if the bucket is not in a chain, return an index */
    /* out of the buckets space, equal to the number of */
    /* buckets in the file to indicate an empty chain */
    if (!BUCKET_IN_CHAIN(hashes[hindex]))
        return header->buckets;

    wraparound = hindex;
    while (BUCKET_IN_CHAIN(hashes[hindex]))
    {
        if (hindex == 0)
            hindex = header->buckets - 1;
        else
            hindex--;

        /* if .cfc file is full return an index out of */
        /* the buckets space, equal to number of buckets */
        /* in the file, plus one */
        if (hindex == wraparound)
            return header->buckets + 1;
    }
    return crm_osbf_next_bindex(header, hindex);
}

unsigned int crm_osbf_find_bucket(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int hash, unsigned int key)
{
    OSBF_FEATUREBUCKET_STRUCT *hashes;
    unsigned int hindex, start;

    hashes = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;
    hindex = start = hash % header->buckets;
    while (!BUCKET_HASH_COMPARE(hashes[hindex], hash, key)
           && !EMPTY_BUCKET(hashes[hindex]))
    {
        hindex = crm_osbf_next_bindex(header, hindex);
        /* if .cfc file is completely full return an index */
        /* out of the buckets space, equal to number of buckets */
        /* in the file, plus one */
        if (hindex == start)
            return header->buckets + 1;
    }

    /* return the index of the found bucket or, if not found,
     * the index of a free bucket where it could be put */
    return hindex;
}

void crm_osbf_update_bucket(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int bindex, int delta)
{
    OSBF_FEATUREBUCKET_STRUCT *hashes;

    hashes = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;
    /*
     * fprintf(stderr, "Bucket updated at %lu, hash: %lu, key: %lu, value: %d\n",
     * bindex, hashes[bindex].hash, hashes[bindex].key, delta);
     */
    if (delta > 0 && GET_BUCKET_VALUE(hashes[bindex]) +
        delta >= OSBF_FEATUREBUCKET_VALUE_MAX - 1)
    {
        SETL_BUCKET_VALUE(hashes[bindex], OSBF_FEATUREBUCKET_VALUE_MAX - 1);
    }
    else if (delta < 0 && GET_BUCKET_VALUE(hashes[bindex]) <= -delta)
    {
        int i, j, packlen;

        BUCKET_RAW_VALUE(hashes[bindex]) = 0;
        BUCKET_HASH(hashes[bindex]) = 0;
        BUCKET_KEY(hashes[bindex]) = 0;
        /* pack chain */

        i = crm_osbf_next_bindex(header, bindex);
        j = crm_osbf_last_in_chain(header, i);

        /* if there's a valid chain tail starting at i, pack it */
        if (j < header->buckets)
        {
            if (j >= i)
                packlen = j - i + 1;
            else
                packlen = header->buckets + 1 - (i - j);
            crm_osbf_packcss(header, i, packlen);
        }
    }
    else
    {
        SETL_BUCKET_VALUE(hashes[bindex],
                GET_BUCKET_VALUE(hashes[bindex]) + delta);
    }
}

void crm_osbf_insert_bucket(OSBF_FEATURE_HEADER_STRUCT *header,
        unsigned int bindex, unsigned int hash,
        unsigned int key, int value)
{
    unsigned int hindex, distance;
    OSBF_FEATUREBUCKET_STRUCT *hashes;

    if (microgroom_chain_length == 0)
        microgroom_chain_length = OSBF_MICROGROOM_CHAIN_LENGTH;


    hashes = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;
    /* "right" bucket position */
    hindex = hash % header->buckets;
    /* distance from right position to free position */
    distance = (bindex >= hindex) ? bindex - hindex :
               header->buckets - (hindex - bindex);
    if ((osbf_microgroom != 0) && (value > 0))
    {
        while (distance > microgroom_chain_length)
        {
            /*
             * fprintf(stderr, "hindex: %lu, bindex: %lu, distance: %lu\n",
             * hindex, bindex, distance);
             */
            crm_osbf_microgroom(header, crm_osbf_prev_bindex(header, bindex));
            /* get new free bucket index */
            bindex = crm_osbf_find_bucket(header, hash, key);
            distance = (bindex >= hindex) ? bindex - hindex :
                       header->buckets - (hindex - bindex);
        }
    }

    /*
     * fprintf(stderr, "new bucket at %lu, hash: %lu, key: %lu, distance: %lu\n",
     * bindex, hash, key, distance);
     */

    SETL_BUCKET_VALUE(hashes[bindex], value);
    BUCKET_HASH(hashes[bindex]) = hash;
    BUCKET_KEY(hashes[bindex]) = key;
}

static OSBF_HEADER_UNION hu = { { { 0 } } };

int crm_osbf_create_cssfile(char *cssfile, unsigned int buckets,
        unsigned int major, unsigned int minor /* [i_a] unused anyway ,
        unsigned int spectrum_start */ )
{
    FILE *f;
    int i;
    OSBF_FEATUREBUCKET_STRUCT feature = { 0, 0, 0 };

    if (user_trace)
        fprintf(stderr, "Opening file %s for read/write\n", cssfile);
    f = fopen(cssfile, "wb");
    if (!f)
    {
        int q;
        q = fatalerror("Couldn't open the new .cfc file for writing; file = ",
                cssfile);
        return q;
    }
    else
    {
        CRM_PORTA_HEADER_INFO classifier_info = { 0 };

        classifier_info.classifier_bits = CRM_OSBF;
		classifier_info.hash_version_in_use = selected_hashfunction;

        if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
        {
            fatalerror("For some reason, I was unable to write the header to the .cfc file named ",
                    cssfile);
        }

        // Set the header.
        *((unsigned int *)hu.header.version) = major;  // quick hack for now...
        hu.header.flags = minor;
        hu.header.learnings = 0;
        hu.header.buckets = buckets;
        hu.header.buckets_start = OSBF_CSS_SPECTRA_START;
        // Write header
        if (fwrite(&hu, sizeof(hu), 1, f) != 1)
        {
            fatalerror(" Couldn't initialize the .cfc file header; file = ",
                    cssfile);
        }

        //  Initialize CSS hashes - zero all buckets
        for (i = 0; i < buckets; i++)
        {
            // Write buckets
            if (fwrite(&feature, sizeof(feature), 1, f) != 1)
            {
                fatalerror(" Couldn't initialize the .cfc buckets; file = ",
                        cssfile);
            }
            memset(&feature, 0, sizeof(feature));
        }
        fclose(f);
    }
    return EXIT_SUCCESS;
}

