//  crm_css_maintenance_.c  - Controllable Regex Mutilator,  version v1.0
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

/* [i_a]
//    the command line argc, argv
extern int prog_argc;
extern char **prog_argv;

//    the auxilliary input buffer (for WINDOW input)
extern char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used 
//    wherever needed.  These are sized to the same size as the data window.
extern char *inbuf;
extern char *outbuf;
extern char *tempbuf;
*/

static long crm_zapcss ( FEATUREBUCKET_TYPE *h, 
		    unsigned long hs, 
		    unsigned long start, 
		    unsigned long end );

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
//     already too full, we execute a 'zero unity bins'.  Then, we see
//     how the file looks, and if necessary, we get rid of some data.
//     R is the "MICROGROOM_RESCALE_FACTOR"
//
long crm_microgroom (FEATUREBUCKET_TYPE *h, unsigned char *seen_features,
		     long hs, unsigned long hindex)
{
  long i, j, k;
  static long microgroom_count = 0;
  long steps;
  long packstart;     // first used bucket in the chain
  long packlen;       // # of used buckets in the chain
  long packend;       // last used bucket in the chain
  //  for stochastic grooming we need a place for the random...
  unsigned long randy;
  long zeroed_countdown;
  long actually_zeroed;
  long force_rescale;
  j = 0;
  k = 0;
  zeroed_countdown = MICROGROOM_STOP_AFTER;

  //

  i = j = k = 0;
  microgroom_count++;

  if (user_trace)
    {
      if (microgroom_count == 1)
	fprintf (stderr, "CSS file too full: microgrooming this css chain: ");
      fprintf (stderr, " %ld ",
	   microgroom_count);
    }


  //       We have two different algorithms for amnesia - stochastic
  //       (meaning random) and weight-distance based.  
  //     

  steps = 0;
  randy = 0;
  force_rescale = 0;

#ifdef STOCHASTIC_AMNESIA
  //   set our stochastic amnesia matcher - note that we add 
  //   our microgroom count so that we _eventually_ can knock out anything
  //   even if we get a whole string of buckets with hash keys that all alias
  //   to the same value.
  //
  //   We also keep track of how many buckets we've zeroed and we stop
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
  //   start at initial chain start, move to back of 
  //   chain that overflowed, then scale just that chain.
  //
  i = j = hindex % hs;
  if (i == 0) i = 1;
  while (h[i].hash != 0)
    {
      i--;
      if (i < 1) i = hs - 1;
      if (i == j)  break;        // don't hang if we have a 100% full .css file
      // fprintf (stderr, "-");
    }

  //     now, move our index to point to the first bucket in this chain.
  i++;
  if (i >= hs) i = 1;
  packstart = i;

  steps = 0;
  force_rescale = 0;
  while (h[i].value != 0 )
    {
      //      fprintf (stderr, "=");
      randy = rand() + microgroom_count;
      if ( 
	  ( h[i].key != 0 )   // hash keys == 0 are SPECIALS like #learns,
	                      // and must never be deleted.
	  && 
	  (force_rescale || 
	   (( h[i].key + randy ) & MICROGROOM_STOCHASTIC_MASK ) 
	   == MICROGROOM_STOCHASTIC_KEY ))
	{
	  h[i].value = h[i].value * MICROGROOM_RESCALE_FACTOR;
	}
      if (h[i].value == 0) zeroed_countdown--;
      i++;
      if (i >= hs ) i = 1;
      steps++;
    }
  packlen = steps;
#endif

#ifdef WEIGHT_DISTANCE_AMNESIA
  //    
  //    Weight-Distance Amnesia is an improvement by Fidelis Assis
  //    over Stochastic amnesia in that it doesn't delete information
  //    randomly; instead it uses the heuristic that low-count buckets
  //    at or near their original insert point are likely to be old and
  //    stale so expire those first.
  //
  //
  i = j = k = hindex % hs;
  if (i == 0) i = j = k = 1;
  while (h[i].hash != 0)
    {
      i--;
      if (i < 1) i = hs - 1;
      if (i == j)  break;        // don't hang if we have a 100% full .css file
      // fprintf (stderr, "-");
    }

  //     now, move our index to point to the first _used_ bucket in
  //     this chain.
  i++;
  if (i >= hs) i = 1;
  packstart = i;

  //     Now find the _end_ of the bucket chain.
  //

  while (h[j].hash != 0) 
    {
      j++;
      if (j >= hs) j = 1;
      if (j == k) break; //   don't hang on 100% full .css file
    }
  j--;
  if (j == 0) j = hs - 1;

  //  j is now the _last_ _used_ bucket.
  packend = j;

  //     Now we have the start and end of the bucket chain.  
  //  
  //     An advanced version of this algorithm would make just two passes;
  //     one to find the lowest-ranked buckets, and another to zero them.
  //     However, Fidelis' instrumentation indicates that an in-place, 
  //     multisweep algorithm may be as fast, or even faster, in the most
  //     common situations.  So for now, we'll do a multisweep.
  //
  //
  //     Normal Case:  hs=10, packstart = 4, packend = 7
  //     buck#  0 1 2 3 4 5 6 7 8 9
  //            R 0 0 0 X X X X 0 0
  //     so packlen = 4 ( == 7 - 4 + 1)
  //
  //     fixup for wraparound - note the 0th bucket is RESERVED:
  //     example hs = 10, packstart = 8, packend = 2
  //     buck#  0 1 2 3 4 5 6 7 8 9
  //            R X X 0 0 0 0 0 X X
  //    and so packlen = 4  (10 - 8 + 2) 

  if (packstart < packend )
    {
      packlen = packend - packstart + 1;
    }
  else
    {
      packlen = ( hs - packstart ) + packend;
    }

  //     And now zap some buckets - are we in wraparound?
  //
  if ( packstart < packend )
    {
      //      fprintf (stderr, "z");
      actually_zeroed = crm_zapcss ( h, hs, packstart, packend);
    }
  else
    {
      //fprintf (stderr, "Z");
      actually_zeroed = crm_zapcss (h, hs, packstart, hs -1 );
      actually_zeroed = actually_zeroed 
	+ crm_zapcss (h, hs, 1,   (packlen - (hs - packstart)));
    }
#endif


  
  //   now we pack the buckets
  crm_packcss (h, seen_features, hs, packstart, packlen);
  
  return (actually_zeroed);
}

////////////////////////////////////////////////
//
//      crm_zapcss - the distance-heuristic microgroomer core.

static long crm_zapcss ( FEATUREBUCKET_TYPE *h, 
		    unsigned long hs, 
		    unsigned long start, 
		    unsigned long end )
{    
  //     A question- what's the ratio deprecation ratio between
  //     "distance from original" vs. low point value?  The original
  //     Fidelis code did a 1:1 equivalence (being 1 place off is exactly as
  //     bad as having a count of just 1).
  //
  //     In reality, because of Zipf's law, most of the buckets
  //     stay at a value of 1 forever; they provide scant evidence
  //     no matter what.  Therefore, we will allow separate weights
  //     for V (value) and D (distance).  Note that a D of zero
  //     means "don't use distance, only value", and a V of zero
  //     means "don't use value, only distance.  Mixed values will
  //     give intermediate tradeoffs between distance( ~~ age) and
  //     value.
  //
  //     Similarly, VWEIGHT2 and DWEIGHT2 go with the _square_ of 
  //     the value and distance.

#define VWEIGHT 1.0 
#define VWEIGHT2 0.0 

#define DWEIGHT 1.0
#define DWEIGHT2 0.0
  
  long vcut;
  long zcountdown;
  unsigned long packlen;
  unsigned long k;
  long actually_zeroed;

  vcut = 1;
  packlen = end - start;
  //  fprintf (stderr, " S: %ld, E: %ld, L: %ld ", start, end, packlen );
  zcountdown = packlen / 32;  //   get rid of about 3% of the data  /* [i_a] */
  actually_zeroed = 0;
  while (zcountdown > 0)
    {
      //  fprintf (stderr, " %ld ", vcut);
      for (k = start; k <= end;  k++)
	{
	  if (h[k].key != 0 )       // key == 0 means "special- don't zero!"
	    {
	      //      fprintf (stderr, "a");
	      if (h[k].value > 0)      //  can't zero it if it's already zeroed
		{
		  //  fprintf (stderr, "b");
		  if ((VWEIGHT * h[k].value) +
		      (VWEIGHT2 * h[k].value * h[k].value ) +
		      (DWEIGHT * (k - h[k].hash % hs)) +
		      (DWEIGHT2 * (k - h[k].hash % hs) * (k - h[k].hash % hs))
		      <= vcut)
		    {
		      //  fprintf (stderr, "*");
		      h[k].value = 0;
		      zcountdown--;
		      actually_zeroed++;
		    }
		}
	    }
	}
      vcut++;
    }
  return (actually_zeroed);
}

void crm_packcss (FEATUREBUCKET_TYPE *h, unsigned char *seen_features,
		  long hs, long packstart, long packlen)
{
  //    How we pack...
  //   
  //    We look at each bucket, and attempt to reinsert it at the "best"
  //    place.  We know at worst it will end up where it already is, and
  //    at best it will end up lower (at a lower index) in the file, except
  //    if it's in wraparound mode, in which case we know it will not get
  //    back up past us (since the file must contain at least one empty)
  //    and so it's still below us in the file.

  //fprintf (stderr, "Packing %ld len %ld total %ld", 
  //	   packstart, packlen, packstart+packlen);
  //  if (packstart+packlen >= hs)
  //  fprintf (stderr, " BLORTTTTTT ");
  if (packstart+packlen <= hs)   //  no wraparound in this case
    {
      crm_packseg (h, seen_features, hs, packstart, packlen);
    }
  else    //  wraparound mode - do it as two separate repacks
    {
      crm_packseg (h, seen_features, hs, packstart, (hs - packstart));
      crm_packseg (h, seen_features, hs, 1, (packlen - (hs - packstart)));
    }
}
      
void crm_packseg (FEATUREBUCKET_TYPE *h, unsigned char *seen_features,
		  long hs, long packstart, long packlen)
{
  unsigned long ifrom, ito;
  unsigned long thash, tkey, tvalue;
  unsigned char tseen; /* [i_a] */

  //  keep the compiler quiet - tseen is used only if seen_features 
  //  is non-null, but the compiler isn't smart enough to know that.
  tseen = 0;

  if (internal_trace) fprintf (stderr, " < %ld %ld >", packstart, packlen);

  for (ifrom = packstart; ifrom < packstart + packlen; ifrom++)
    {
      //  Is it an empty bucket?  (remember, we're compressing out 
      //  all placeholder buckets, so any bucket that's zero-valued
      //  is a valid target.)
      if ( h[ifrom].value == 0)
	{
	  //    Empty bucket - turn it from marker to empty
	 if (internal_trace) fprintf (stderr, "x");
	  h[ifrom].key = 0;
	  h[ifrom].hash = 0;
	  if (seen_features)
	   seen_features[ifrom] = 0;
	}
      else 
	{ if (internal_trace) fprintf (stderr, "-");}
    }

  //  Our slot values are now somewhat in disorder because empty
  //  buckets may now have been inserted into a chain where there used
  //  to be placeholder buckets.  We need to re-insert slot data in a
  //  bucket where it will be found.
  //
  ito = 0;
  for (ifrom = packstart; ifrom < packstart+packlen; ifrom++)
    {
      //    Now find the next bucket to place somewhere
      //
      thash  = h[ifrom].hash;
      tkey   = h[ifrom].key;
      tvalue = h[ifrom].value;
      if (seen_features)
	tseen  = seen_features[ifrom];

      if (tvalue == 0)
	{
	  if (internal_trace) fprintf (stderr, "X");
	}
      else
	{
	  ito = thash % hs;
	  if (ito == 0) ito = 1;
	  // fprintf (stderr, "a %ld", ito);

	  while ( ! ( (h[ito].value == 0)
		      || (  h[ito].hash == thash 
			    && h[ito].key == tkey )))
	    {
	      ito++;
	      if (ito >= hs) ito = 1;
	      // fprintf (stderr, "a %ld", ito);
	    }
	  
	  //
	  //    found an empty slot, put this value there, and zero the
	  //    original one.  Sometimes this is a noop.  We don't care.

	  if (internal_trace)
	    {
	      if ( ifrom == ito ) fprintf (stderr, "=");
	      if ( ito < ifrom) fprintf (stderr, "<");
	      if ( ito > ifrom ) fprintf (stderr, ">");
	    }

	  h[ifrom].hash  = 0;
	  h[ifrom].key   = 0;
	  h[ifrom].value = 0;
	  if (seen_features)
	    seen_features[ifrom] = 0;
	  
	  h[ito].hash  = thash;
	  h[ito].key   = tkey;
	  h[ito].value = tvalue;
	  if (seen_features)
	    seen_features[ito] = tseen;
	}
    }
}

int crm_create_cssfile(char *cssfile, long buckets, 
		       long major, long minor, long spectrum_start)
{
  FILE *f;
  long i;
  FEATUREBUCKET_STRUCT feature = {0, 0, 0};
  
  if (user_trace)
    fprintf (stderr, "Opening file %s for writing.\n", cssfile);
  f = fopen (cssfile, "wb");
  if (!f)
    {
      fprintf (stderr, "\n Couldn't open file %s for writing; errno=%d .\n",
	       cssfile, errno);
      return (EXIT_FAILURE);
    }
  //  Initialize CSS file - zero all buckets
  feature.hash =  major;
  feature.key = minor;
  feature.value = spectrum_start;
  for (i=0; i < buckets; i++)
    {
      if (fwrite(&feature, sizeof(feature), 1, f) != 1)
        {
          fprintf (stderr, "\n Couldn't initialize .CSS file %s, "
		   "errno=%d.\n",
		   cssfile,
                   errno);
          return (EXIT_FAILURE);
        }
      //
      //   HACK ALERT HACK ALERT HACK ALERT
      //
      //  yeah,there's more efficient ways to do this, but this will
      //  stay in cache; an IF-statement will need at least three ops as
      //  well.   Probably six of one...
      feature.hash = 0;
      feature.key = 0;
      feature.value = 0;
    }
  fclose (f);
  return (EXIT_SUCCESS);
}



