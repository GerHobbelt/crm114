//  crm_str_funcs.c  - Controllable Regex Mutilator,  version v1.0
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






/*
 * -------------------------------------------------------------------------------
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 *
 * These are functions for producing 32-bit hashes for hash table lookup.
 * hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final()
 * are externally useful functions.  Routines to test the hash are included
 * if SELF_TEST is defined.  You can use this free for any purpose.  It's in
 * the public domain.  It has no warranty.
 *
 * You probably want to use hashlittle().  hashlittle() and hashbig()
 * hash byte arrays.  hashlittle() is is faster than hashbig() on
 * little-endian machines.  Intel and AMD are little-endian machines.
 * On second thought, you probably want hashlittle2(), which is identical to
 * hashlittle() except it returns two 32-bit hashes for the price of one.
 * You could implement hashbig2() if you wanted but I haven't bothered here.
 *
 * If you want to find a hash of, say, exactly 7 integers, do
 * a = i1;  b = i2;  c = i3;
 * mix(a,b,c);
 * a += i4; b += i5; c += i6;
 * mix(a,b,c);
 * a += i7;
 * final(a,b,c);
 * then use c as the hash value.  If you have a variable length array of
 * 4-byte integers to hash, use hashword().  If you have a byte array (like
 * a character string), use hashlittle().  If you have several byte arrays, or
 * a mix of things, see the comments above hashlittle().
 *
 * Why is this so big?  I read 12 bytes at a time into 3 4-byte integers,
 * then mix those integers.  This is fast (you can do a lot more thorough
 * mixing with 12*3 instructions on 3 integers than you can with 3 instructions
 * on 1 byte), but shoehorning those bytes into integers efficiently is messy.
 * -------------------------------------------------------------------------------
 */
#if defined (MACHINE_IS_LITTLE_ENDIAN)
#define HASH_LITTLE_ENDIAN 1
#define HASH_BIG_ENDIAN 0
#elif defined (MACHINE_IS_BIG_ENDIAN)
#define HASH_LITTLE_ENDIAN 0
#define HASH_BIG_ENDIAN 1
#else
#define HASH_LITTLE_ENDIAN 0
#define HASH_BIG_ENDIAN 0
#endif



#define hashsize(n) ((uint32_t)1 << (n))
#define hashmask(n) (hashsize(n) - 1)
#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))


/*
 * -------------------------------------------------------------------------------
 * mix -- mix 3 32-bit values reversibly.
 *
 * This is reversible, so any information in (a,b,c) before mix() is
 * still in (a,b,c) after mix().
 *
 * If four pairs of (a,b,c) inputs are run through mix(), or through
 * mix() in reverse, there are at least 32 bits of the output that
 * are sometimes the same for one pair and different for another pair.
 * This was tested for:
 * pairs that differed by one bit, by two bits, in any combination
 * of top bits of (a,b,c), or in any combination of bottom bits of
 * (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 * the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 * is commonly produced by subtraction) look like a single 1-bit
 * difference.
 * the base values were pseudorandom, all zero but one bit set, or
 * all zero plus a counter that starts at zero.
 *
 * Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
 * satisfy this are
 *  4  6  8 16 19  4
 *  9 15  3 18 27 15
 * 14  9  3  7 17  3
 * Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
 * for "differ" defined as + with a one-bit base and a two-bit delta.  I
 * used http://burtleburtle.net/bob/hash/avalanche.html to choose
 * the operations, constants, and arrangements of the variables.
 *
 * This does not achieve avalanche.  There are input bits of (a,b,c)
 * that fail to affect some output bits of (a,b,c), especially of a.  The
 * most thoroughly mixed value is c, but it doesn't really even achieve
 * avalanche in c.
 *
 * This allows some parallelism.  Read-after-writes are good at doubling
 * the number of bits affected, so the goal of mixing pulls in the opposite
 * direction as the goal of parallelism.  I did what I could.  Rotates
 * seem to cost as much as shifts on every machine I could lay my hands
 * on, and rotates are much kinder to the top and bottom bits, so I used
 * rotates.
 * -------------------------------------------------------------------------------
 */
#define mix(a, b, c) \
  { \
    a -= c;  a ^= rot(c, 4);  c += b; \
    b -= a;  b ^= rot(a, 6);  a += c; \
    c -= b;  c ^= rot(b, 8);  b += a; \
    a -= c;  a ^= rot(c, 16);  c += b; \
    b -= a;  b ^= rot(a, 19);  a += c; \
    c -= b;  c ^= rot(b, 4);  b += a; \
  }

/*
 * -------------------------------------------------------------------------------
 * final -- final mixing of 3 32-bit values (a,b,c) into c
 *
 * Pairs of (a,b,c) values differing in only a few bits will usually
 * produce values of c that look totally different.  This was tested for
 * pairs that differed by one bit, by two bits, in any combination
 * of top bits of (a,b,c), or in any combination of bottom bits of
 * (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 * the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 * is commonly produced by subtraction) look like a single 1-bit
 * difference.
 * the base values were pseudorandom, all zero but one bit set, or
 * all zero plus a counter that starts at zero.
 *
 * These constants passed:
 * 14 11 25 16 4 14 24
 * 12 14 25 16 4 14 24
 * and these came close:
 * 4  8 15 26 3 22 24
 * 10  8 15 26 3 22 24
 * 11  8 15 26 3 22 24
 * -------------------------------------------------------------------------------
 */
#define final(a, b, c) \
  { \
    c ^= b; c -= rot(b, 14); \
    a ^= c; a -= rot(c, 11); \
    b ^= a; b -= rot(a, 25); \
    c ^= b; c -= rot(b, 16); \
    a ^= c; a -= rot(c, 4);  \
    b ^= a; b -= rot(a, 14); \
    c ^= b; c -= rot(b, 24); \
  }

/*
 * --------------------------------------------------------------------
 * This works on all machines.  To be useful, it requires
 * -- that the key be an array of uint32_t's, and
 * -- that the length be the number of uint32_t's in the key
 *
 * The function hashword() is identical to hashlittle() on little-endian
 * machines, and identical to hashbig() on big-endian machines,
 * except that the length has to be measured in uint32_ts rather than in
 * bytes.  hashlittle() is more complicated than hashword() only because
 * hashlittle() has to dance around fitting the key bytes into registers.
 * --------------------------------------------------------------------
 */
uint32_t hashword(
  const uint32_t *k,                  /* the key, an array of uint32_t values */
  size_t          length,             /* the length of the key, in uint32_ts */
  uint32_t        initval)            /* the previous hash, or an arbitrary value */
{
  uint32_t a, b, c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((uint32_t)length) << 2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a, b, c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch (length)                    /* all the case statements fall through */
  {
  case 3:
    c += k[2];

  case 2:
    b += k[1];

  case 1:
    a += k[0];
    final(a, b, c);

  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}


/*
 * --------------------------------------------------------------------
 * hashword2() -- same as hashword(), but take two seeds and return two
 * 32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
 * both be initialized with seeds.  If you pass in (*pb)==0, the output
 * (*pc) will be the same as the return value from hashword().
 * --------------------------------------------------------------------
 */
void hashword2(
  const uint32_t *k,                     /* the key, an array of uint32_t values */
  size_t          length,                /* the length of the key, in uint32_ts */
  uint32_t       *pc,                    /* IN: seed OUT: primary hash value */
  uint32_t       *pb)                    /* IN: more seed OUT: secondary hash value */
{
  uint32_t a, b, c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)(length << 2)) + *pc;
  c += *pb;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a, b, c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch (length)                    /* all the case statements fall through */
  {
  case 3:
    c += k[2];

  case 2:
    b += k[1];

  case 1:
    a += k[0];
    final(a, b, c);

  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  *pc = c;
  *pb = b;
}


/*
 * -------------------------------------------------------------------------------
 * hashlittle() -- hash a variable-length key into a 32-bit value
 * k       : the key (the unaligned variable-length array of bytes)
 * length  : the length of the key, counting by bytes
 * initval : can be any 4-byte value
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Two keys differing by one or two bits will have
 * totally different hash values.
 *
 * The best hash table sizes are powers of 2.  There is no need to do
 * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 * use a bitmask.  For example, if you need only 10 bits, do
 * h = (h & hashmask(10));
 * In which case, the hash table should have hashsize(10) elements.
 *
 * If you are hashing n strings (uint8_t **)k, do it like this:
 * for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);
 *
 * By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
 * code any way you wish, private, educational, or commercial.  It's free.
 *
 * Use for hash table lookup, or anything where one collision in 2^^32 is
 * acceptable.  Do NOT use for cryptographic purposes.
 * -------------------------------------------------------------------------------
 */

uint32_t hashlittle(const void *key, size_t length, uint32_t initval)
{
  uint32_t a, b, c;                                        /* internal state */

  union
  {
    const void *ptr;
    size_t      i;
  } u;                                        /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0))
  {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
#ifdef VALGRIND
    const uint8_t  *k8;
#endif

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a, b, c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch (length)
    {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;

    case 11:
      c += k[2] & 0xffffff;
      b += k[1];
      a += k[0];
      break;

    case 10:
      c += k[2] & 0xffff;
      b += k[1];
      a += k[0];
      break;

    case 9:
      c += k[2] & 0xff;
      b += k[1];
      a += k[0];
      break;

    case 8:
      b += k[1];
      a += k[0];
      break;

    case 7:
      b += k[1] & 0xffffff;
      a += k[0];
      break;

    case 6:
      b += k[1] & 0xffff;
      a += k[0];
      break;

    case 5:
      b += k[1] & 0xff;
      a += k[0];
      break;

    case 4:
      a += k[0];
      break;

    case 3:
      a += k[0] & 0xffffff;
      break;

    case 2:
      a += k[0] & 0xffff;
      break;

    case 1:
      a += k[0] & 0xff;
      break;

    case 0:
      return c;                     /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch (length)
    {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;

    case 11:
      c += ((uint32_t)k8[10]) << 16;      /* fall through */

    case 10:
      c += ((uint32_t)k8[9]) << 8;       /* fall through */

    case 9:
      c += k8[8];                        /* fall through */

    case 8:
      b += k[1];
      a += k[0];
      break;

    case 7:
      b += ((uint32_t)k8[6]) << 16;      /* fall through */

    case 6:
      b += ((uint32_t)k8[5]) << 8;       /* fall through */

    case 5:
      b += k8[4];                        /* fall through */

    case 4:
      a += k[0];
      break;

    case 3:
      a += ((uint32_t)k8[2]) << 16;      /* fall through */

    case 2:
      a += ((uint32_t)k8[1]) << 8;       /* fall through */

    case 1:
      a += k8[0];
      break;

    case 0:
      return c;
    }

#endif /* !valgrind */
  }
  else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0))
  {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1]) << 16);
      b += k[2] + (((uint32_t)k[3]) << 16);
      c += k[4] + (((uint32_t)k[5]) << 16);
      mix(a, b, c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch (length)
    {
    case 12:
      c += k[4] + (((uint32_t)k[5]) << 16);
      b += k[2] + (((uint32_t)k[3]) << 16);
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 11:
      c += ((uint32_t)k8[10]) << 16;        /* fall through */

    case 10:
      c += k[4];
      b += k[2] + (((uint32_t)k[3]) << 16);
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 9:
      c += k8[8];                           /* fall through */

    case 8:
      b += k[2] + (((uint32_t)k[3]) << 16);
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 7:
      b += ((uint32_t)k8[6]) << 16;         /* fall through */

    case 6:
      b += k[2];
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 5:
      b += k8[4];                           /* fall through */

    case 4:
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 3:
      a += ((uint32_t)k8[2]) << 16;         /* fall through */

    case 2:
      a += k[0];
      break;

    case 1:
      a += k8[0];
      break;

    case 0:
      return c;                            /* zero length requires no mixing */
    }
  }
  else
  {
    /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1]) << 8;
      a += ((uint32_t)k[2]) << 16;
      a += ((uint32_t)k[3]) << 24;
      b += k[4];
      b += ((uint32_t)k[5]) << 8;
      b += ((uint32_t)k[6]) << 16;
      b += ((uint32_t)k[7]) << 24;
      c += k[8];
      c += ((uint32_t)k[9]) << 8;
      c += ((uint32_t)k[10]) << 16;
      c += ((uint32_t)k[11]) << 24;
      mix(a, b, c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch (length)                  /* all the case statements fall through */
    {
    case 12:
      c += ((uint32_t)k[11]) << 24;

    case 11:
      c += ((uint32_t)k[10]) << 16;

    case 10:
      c += ((uint32_t)k[9]) << 8;

    case 9:
      c += k[8];

    case 8:
      b += ((uint32_t)k[7]) << 24;

    case 7:
      b += ((uint32_t)k[6]) << 16;

    case 6:
      b += ((uint32_t)k[5]) << 8;

    case 5:
      b += k[4];

    case 4:
      a += ((uint32_t)k[3]) << 24;

    case 3:
      a += ((uint32_t)k[2]) << 16;

    case 2:
      a += ((uint32_t)k[1]) << 8;

    case 1:
      a += k[0];
      break;

    case 0:
      return c;
    }
  }

  final(a, b, c);
  return c;
}


/*
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
void hashlittle2(
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
  uint32_t a, b, c;                                        /* internal state */

  union
  {
    const void *ptr;
    size_t      i;
  } u;                                        /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0))
  {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
#ifdef VALGRIND
    const uint8_t  *k8;
#endif

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a, b, c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch (length)
    {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;

    case 11:
      c += k[2] & 0xffffff;
      b += k[1];
      a += k[0];
      break;

    case 10:
      c += k[2] & 0xffff;
      b += k[1];
      a += k[0];
      break;

    case 9:
      c += k[2] & 0xff;
      b += k[1];
      a += k[0];
      break;

    case 8:
      b += k[1];
      a += k[0];
      break;

    case 7:
      b += k[1] & 0xffffff;
      a += k[0];
      break;

    case 6:
      b += k[1] & 0xffff;
      a += k[0];
      break;

    case 5:
      b += k[1] & 0xff;
      a += k[0];
      break;

    case 4:
      a += k[0];
      break;

    case 3:
      a += k[0] & 0xffffff;
      break;

    case 2:
      a += k[0] & 0xffff;
      break;

    case 1:
      a += k[0] & 0xff;
      break;

    case 0:
      *pc = c;
      *pb = b;
      return;                        /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch (length)
    {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;

    case 11:
      c += ((uint32_t)k8[10]) << 16;      /* fall through */

    case 10:
      c += ((uint32_t)k8[9]) << 8;       /* fall through */

    case 9:
      c += k8[8];                        /* fall through */

    case 8:
      b += k[1];
      a += k[0];
      break;

    case 7:
      b += ((uint32_t)k8[6]) << 16;      /* fall through */

    case 6:
      b += ((uint32_t)k8[5]) << 8;       /* fall through */

    case 5:
      b += k8[4];                        /* fall through */

    case 4:
      a += k[0];
      break;

    case 3:
      a += ((uint32_t)k8[2]) << 16;      /* fall through */

    case 2:
      a += ((uint32_t)k8[1]) << 8;       /* fall through */

    case 1:
      a += k8[0];
      break;

    case 0:
      *pc = c;
      *pb = b;
      return;                        /* zero length strings require no mixing */
    }

#endif /* !valgrind */
  }
  else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0))
  {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1]) << 16);
      b += k[2] + (((uint32_t)k[3]) << 16);
      c += k[4] + (((uint32_t)k[5]) << 16);
      mix(a, b, c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch (length)
    {
    case 12:
      c += k[4] + (((uint32_t)k[5]) << 16);
      b += k[2] + (((uint32_t)k[3]) << 16);
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 11:
      c += ((uint32_t)k8[10]) << 16;        /* fall through */

    case 10:
      c += k[4];
      b += k[2] + (((uint32_t)k[3]) << 16);
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 9:
      c += k8[8];                           /* fall through */

    case 8:
      b += k[2] + (((uint32_t)k[3]) << 16);
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 7:
      b += ((uint32_t)k8[6]) << 16;         /* fall through */

    case 6:
      b += k[2];
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 5:
      b += k8[4];                           /* fall through */

    case 4:
      a += k[0] + (((uint32_t)k[1]) << 16);
      break;

    case 3:
      a += ((uint32_t)k8[2]) << 16;         /* fall through */

    case 2:
      a += k[0];
      break;

    case 1:
      a += k8[0];
      break;

    case 0:
      *pc = c;
      *pb = b;
      return;                        /* zero length strings require no mixing */
    }
  }
  else
  {
    /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1]) << 8;
      a += ((uint32_t)k[2]) << 16;
      a += ((uint32_t)k[3]) << 24;
      b += k[4];
      b += ((uint32_t)k[5]) << 8;
      b += ((uint32_t)k[6]) << 16;
      b += ((uint32_t)k[7]) << 24;
      c += k[8];
      c += ((uint32_t)k[9]) << 8;
      c += ((uint32_t)k[10]) << 16;
      c += ((uint32_t)k[11]) << 24;
      mix(a, b, c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch (length)                  /* all the case statements fall through */
    {
    case 12:
      c += ((uint32_t)k[11]) << 24;

    case 11:
      c += ((uint32_t)k[10]) << 16;

    case 10:
      c += ((uint32_t)k[9]) << 8;

    case 9:
      c += k[8];

    case 8:
      b += ((uint32_t)k[7]) << 24;

    case 7:
      b += ((uint32_t)k[6]) << 16;

    case 6:
      b += ((uint32_t)k[5]) << 8;

    case 5:
      b += k[4];

    case 4:
      a += ((uint32_t)k[3]) << 24;

    case 3:
      a += ((uint32_t)k[2]) << 16;

    case 2:
      a += ((uint32_t)k[1]) << 8;

    case 1:
      a += k[0];
      break;

    case 0:
      *pc = c;
      *pb = b;
      return;                        /* zero length strings require no mixing */
    }
  }

  final(a, b, c);
  *pc = c;
  *pb = b;
}



/*
 * hashbig():
 * This is the same as hashword() on big-endian machines.  It is different
 * from hashlittle() on all machines.  hashbig() takes advantage of
 * big-endian byte ordering.
 */
uint32_t hashbig(const void *key, size_t length, uint32_t initval)
{
  uint32_t a, b, c;

  union
  {
    const void *ptr;
    size_t      i;
  } u;                                    /* to cast key to (size_t) happily */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_BIG_ENDIAN && ((u.i & 0x3) == 0))
  {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
#ifdef VALGRIND
    const uint8_t  *k8;
#endif

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a, b, c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]<<8" actually reads beyond the end of the string, but
     * then shifts out the part it's not allowed to read.  Because the
     * string is aligned, the illegal read is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch (length)
    {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;

    case 11:
      c += k[2] & 0xffffff00;
      b += k[1];
      a += k[0];
      break;

    case 10:
      c += k[2] & 0xffff0000;
      b += k[1];
      a += k[0];
      break;

    case 9:
      c += k[2] & 0xff000000;
      b += k[1];
      a += k[0];
      break;

    case 8:
      b += k[1];
      a += k[0];
      break;

    case 7:
      b += k[1] & 0xffffff00;
      a += k[0];
      break;

    case 6:
      b += k[1] & 0xffff0000;
      a += k[0];
      break;

    case 5:
      b += k[1] & 0xff000000;
      a += k[0];
      break;

    case 4:
      a += k[0];
      break;

    case 3:
      a += k[0] & 0xffffff00;
      break;

    case 2:
      a += k[0] & 0xffff0000;
      break;

    case 1:
      a += k[0] & 0xff000000;
      break;

    case 0:
      return c;                     /* zero length strings require no mixing */
    }

#else  /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch (length)                  /* all the case statements fall through */
    {
    case 12:
      c += k[2];
      b += k[1];
      a += k[0];
      break;

    case 11:
      c += ((uint32_t)k8[10]) << 8;      /* fall through */

    case 10:
      c += ((uint32_t)k8[9]) << 16;      /* fall through */

    case 9:
      c += ((uint32_t)k8[8]) << 24;      /* fall through */

    case 8:
      b += k[1];
      a += k[0];
      break;

    case 7:
      b += ((uint32_t)k8[6]) << 8;      /* fall through */

    case 6:
      b += ((uint32_t)k8[5]) << 16;      /* fall through */

    case 5:
      b += ((uint32_t)k8[4]) << 24;      /* fall through */

    case 4:
      a += k[0];
      break;

    case 3:
      a += ((uint32_t)k8[2]) << 8;      /* fall through */

    case 2:
      a += ((uint32_t)k8[1]) << 16;      /* fall through */

    case 1:
      a += ((uint32_t)k8[0]) << 24;
      break;

    case 0:
      return c;
    }

#endif /* !VALGRIND */
  }
  else
  {
    /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += ((uint32_t)k[0]) << 24;
      a += ((uint32_t)k[1]) << 16;
      a += ((uint32_t)k[2]) << 8;
      a += ((uint32_t)k[3]);
      b += ((uint32_t)k[4]) << 24;
      b += ((uint32_t)k[5]) << 16;
      b += ((uint32_t)k[6]) << 8;
      b += ((uint32_t)k[7]);
      c += ((uint32_t)k[8]) << 24;
      c += ((uint32_t)k[9]) << 16;
      c += ((uint32_t)k[10]) << 8;
      c += ((uint32_t)k[11]);
      mix(a, b, c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch (length)                  /* all the case statements fall through */
    {
    case 12:
      c += k[11];

    case 11:
      c += ((uint32_t)k[10]) << 8;

    case 10:
      c += ((uint32_t)k[9]) << 16;

    case 9:
      c += ((uint32_t)k[8]) << 24;

    case 8:
      b += k[7];

    case 7:
      b += ((uint32_t)k[6]) << 8;

    case 6:
      b += ((uint32_t)k[5]) << 16;

    case 5:
      b += ((uint32_t)k[4]) << 24;

    case 4:
      a += k[3];

    case 3:
      a += ((uint32_t)k[2]) << 8;

    case 2:
      a += ((uint32_t)k[1]) << 16;

    case 1:
      a += ((uint32_t)k[0]) << 24;
      break;

    case 0:
      return c;
    }
  }

  final(a, b, c);
  return c;
}


#ifdef SELF_TEST

/* used for timings */
void driver1()
{
  uint8_t buf[256];
  uint32_t i;
  uint32_t h = 0;
  time_t a, z;

  time(&a);
  for (i = 0; i < 256; ++i) buf[i] = 'x';
  for (i = 0; i < 1; ++i)
  {
    h = hashlittle(&buf[0], 1, h);
  }
  time(&z);
  if (z - a > 0) fprintf(stdout, "time %d %.8x\n", z - a, h);
}

/* check that every input bit changes every output bit half the time */
#define HASHSTATE 1
#define HASHLEN   1
#define MAXPAIR 60
#define MAXLEN  70
void driver2()
{
  uint8_t qa[MAXLEN + 1], qb[MAXLEN + 2], *a = &qa[0], *b = &qb[1];
  uint32_t c[HASHSTATE], d[HASHSTATE], i = 0, j = 0, k, l, m = 0, z;
  uint32_t e[HASHSTATE], f[HASHSTATE], g[HASHSTATE], h[HASHSTATE];
  uint32_t x[HASHSTATE], y[HASHSTATE];
  uint32_t hlen;

  fprintf(stdout, "No more than %d trials should ever be needed \n", MAXPAIR / 2);
  for (hlen = 0; hlen < MAXLEN; ++hlen)
  {
    z = 0;
    for (i = 0; i < hlen; ++i)    /*----------------------- for each input byte, */
    {
      for (j = 0; j < 8; ++j)      /*------------------------ for each input bit, */
      {
        for (m = 1; m < 8; ++m)        /*------------ for serveral possible initvals, */
        {
          for (l = 0; l < HASHSTATE; ++l)
            e[l] = f[l] = g[l] = h[l] = x[l] = y[l] = ~((uint32_t)0);

          /*---- check that every output bit is affected by that input bit */
          for (k = 0; k < MAXPAIR; k += 2)
          {
            uint32_t finished = 1;
            /* keys have one bit different */
            for (l = 0; l < hlen + 1; ++l) {
              a[l] = b[l] = (uint8_t)0;
            }
            /* have a and b be two keys differing in only one bit */
            a[i] ^= (k << j);
            a[i] ^= (k >> (8 - j));
            c[0] = hashlittle(a, hlen, m);
            b[i] ^= ((k + 1) << j);
            b[i] ^= ((k + 1) >> (8 - j));
            d[0] = hashlittle(b, hlen, m);
            /* check every bit is 1, 0, set, and not set at least once */
            for (l = 0; l < HASHSTATE; ++l)
            {
              e[l] &= (c[l] ^ d[l]);
              f[l] &= ~(c[l] ^ d[l]);
              g[l] &= c[l];
              h[l] &= ~c[l];
              x[l] &= d[l];
              y[l] &= ~d[l];
              if (e[l] | f[l] | g[l] | h[l] | x[l] | y[l]) finished = 0;
            }
            if (finished) break;
          }
          if (k > z) z = k;
          if (k == MAXPAIR)
          {
            fprintf(stdout, "Some bit didn't change: ");
            fprintf(stdout, "%.8x %.8x %.8x %.8x %.8x %.8x  ",
                    e[0], f[0], g[0], h[0], x[0], y[0]);
            fprintf(stdout, "i %d j %d m %d len %d\n", i, j, m, hlen);
          }
          if (z == MAXPAIR) goto done;
        }
      }
    }
    done:
    if (z < MAXPAIR)
    {
      fprintf(stdout, "Mix success  %2d bytes  %2d initvals  ", i, m);
      fprintf(stdout, "required  %d  trials\n", z / 2);
    }
  }
  fprintf(stdout, "\n");
}

/* Check for reading beyond the end of the buffer and alignment problems */
void driver3()
{
  uint8_t buf[MAXLEN + 20], *b;
  uint32_t len;
  uint8_t q[] = "This is the time for all good men to come to the aid of their country...";
  uint32_t h;
  uint8_t qq[] = "xThis is the time for all good men to come to the aid of their country...";
  uint32_t i;
  uint8_t qqq[] = "xxThis is the time for all good men to come to the aid of their country...";
  uint32_t j;
  uint8_t qqqq[] = "xxxThis is the time for all good men to come to the aid of their country...";
  uint32_t ref, x, y;
  uint8_t *p;

  fprintf(stdout, "Endianness.  These lines should all be the same (for values filled in):\n");
  fprintf(stdout, "%.8x                            %.8x                            %.8x\n",
          hashword((const uint32_t *)q, (sizeof(q) - 1) / 4, 13),
          hashword((const uint32_t *)q, (sizeof(q) - 5) / 4, 13),
          hashword((const uint32_t *)q, (sizeof(q) - 9) / 4, 13));
  p = q;
  fprintf(stdout, "%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
          hashlittle(p, sizeof(q) - 1, 13), hashlittle(p, sizeof(q) - 2, 13),
          hashlittle(p, sizeof(q) - 3, 13), hashlittle(p, sizeof(q) - 4, 13),
          hashlittle(p, sizeof(q) - 5, 13), hashlittle(p, sizeof(q) - 6, 13),
          hashlittle(p, sizeof(q) - 7, 13), hashlittle(p, sizeof(q) - 8, 13),
          hashlittle(p, sizeof(q) - 9, 13), hashlittle(p, sizeof(q) - 10, 13),
          hashlittle(p, sizeof(q) - 11, 13), hashlittle(p, sizeof(q) - 12, 13));
  p = &qq[1];
  fprintf(stdout, "%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
          hashlittle(p, sizeof(q) - 1, 13), hashlittle(p, sizeof(q) - 2, 13),
          hashlittle(p, sizeof(q) - 3, 13), hashlittle(p, sizeof(q) - 4, 13),
          hashlittle(p, sizeof(q) - 5, 13), hashlittle(p, sizeof(q) - 6, 13),
          hashlittle(p, sizeof(q) - 7, 13), hashlittle(p, sizeof(q) - 8, 13),
          hashlittle(p, sizeof(q) - 9, 13), hashlittle(p, sizeof(q) - 10, 13),
          hashlittle(p, sizeof(q) - 11, 13), hashlittle(p, sizeof(q) - 12, 13));
  p = &qqq[2];
  fprintf(stdout, "%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
          hashlittle(p, sizeof(q) - 1, 13), hashlittle(p, sizeof(q) - 2, 13),
          hashlittle(p, sizeof(q) - 3, 13), hashlittle(p, sizeof(q) - 4, 13),
          hashlittle(p, sizeof(q) - 5, 13), hashlittle(p, sizeof(q) - 6, 13),
          hashlittle(p, sizeof(q) - 7, 13), hashlittle(p, sizeof(q) - 8, 13),
          hashlittle(p, sizeof(q) - 9, 13), hashlittle(p, sizeof(q) - 10, 13),
          hashlittle(p, sizeof(q) - 11, 13), hashlittle(p, sizeof(q) - 12, 13));
  p = &qqqq[3];
  fprintf(stdout, "%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
          hashlittle(p, sizeof(q) - 1, 13), hashlittle(p, sizeof(q) - 2, 13),
          hashlittle(p, sizeof(q) - 3, 13), hashlittle(p, sizeof(q) - 4, 13),
          hashlittle(p, sizeof(q) - 5, 13), hashlittle(p, sizeof(q) - 6, 13),
          hashlittle(p, sizeof(q) - 7, 13), hashlittle(p, sizeof(q) - 8, 13),
          hashlittle(p, sizeof(q) - 9, 13), hashlittle(p, sizeof(q) - 10, 13),
          hashlittle(p, sizeof(q) - 11, 13), hashlittle(p, sizeof(q) - 12, 13));
  fprintf(stdout, "\n");

  /* check that hashlittle2 and hashlittle produce the same results */
  i = 47;
  j = 0;
  hashlittle2(q, sizeof(q), &i, &j);
  if (hashlittle(q, sizeof(q), 47) != i)
    fprintf(stdout, "hashlittle2 and hashlittle mismatch\n");

  /* check that hashword2 and hashword produce the same results */
  len = 0xdeadbeef;
  i = 47, j = 0;
  hashword2(&len, 1, &i, &j);
  if (hashword(&len, 1, 47) != i)
    fprintf(stdout, "hashword2 and hashword mismatch %x %x\n",
            i, hashword(&len, 1, 47));

  /* check hashlittle doesn't read before or after the ends of the string */
  for (h = 0, b = buf + 1; h < 8; ++h, ++b)
  {
    for (i = 0; i < MAXLEN; ++i)
    {
      len = i;
      for (j = 0; j < i; ++j) *(b + j) = 0;

      /* these should all be equal */
      ref = hashlittle(b, len, (uint32_t)1);
      *(b + i) = (uint8_t) ~0;
      *(b - 1) = (uint8_t) ~0;
      x = hashlittle(b, len, (uint32_t)1);
      y = hashlittle(b, len, (uint32_t)1);
      if ((ref != x) || (ref != y))
      {
        fprintf(stdout, "alignment error: %.8x %.8x %.8x %d %d\n", ref, x, y,
                h, i);
      }
    }
  }
}

/* check for problems with nulls */
void driver4()
{
  uint8_t buf[1];
  uint32_t h, i, state[HASHSTATE];


  buf[0] = ~0;
  for (i = 0; i < HASHSTATE; ++i) state[i] = 1;
  fprintf(stdout, "These should all be different\n");
  for (i = 0, h = 0; i < 8; ++i)
  {
    h = hashlittle(buf, 0, h);
    fprintf(stdout, "%2ld  0-byte strings, hash is  %.8x\n", i, h);
  }
}


int hash_selftest(void)
{
  driver1();   /* test that the key is hashed: used for timings */
  driver2();   /* test that whole key is hashed thoroughly */
  driver3();   /* test that nothing but the key is hashed */
  driver4();   /* test hashing multiple buffers (all buffers are null) */
  return 1;
}

#endif  /* SELF_TEST */





//     strnhash - generate the hash of a string of length N
//     goals - fast, works well with short vars includng
//     letter pairs and palindromes, not crypto strong, generates
//     hashes that tend toward relative primality against common
//     hash table lengths (so taking the output of this function
//     modulo the hash table length gives a relatively uniform distribution
//
//     In timing tests, this hash function can hash over 10 megabytes
//     per second (using as text the full 2.4.9 linux kernel source)
//     hashing individual whitespace-delimited tokens, on a Transmeta
//     666 MHz.

/*****    OLD VERSION NOT 64-BIT PORTABLE DON'T USE ME *********
 * long strnhash (char *str, long len)
 * {
 * long i;
 * long hval;
 * char *hstr;
 * char chtmp;
 *
 * // initialize hval
 * hval= len;
 *
 * hstr = (char *) &hval;
 *
 * //  for each character in the incoming text:
 *
 * for ( i = 0; i < len; i++)
 *  {
 *    //    xor in the current byte against each byte of hval
 *    //    (which alone gaurantees that every bit of input will have
 *    //    an effect on the output)
 *    //hstr[0] = (hstr[0] & ( ~ str[i] ) ) | ((~ hstr [0]) & str[i]);
 *    //hstr[1] = (hstr[1] & ( ~ str[i] ) ) | ((~ hstr [1]) & str[i]);
 *    //hstr[2] = (hstr[2] & ( ~ str[i] ) ) | ((~ hstr [2]) & str[i]);
 *    //hstr[3] = (hstr[3] & ( ~ str[i] ) ) | ((~ hstr [3]) & str[i]);
 *
 *    hstr[0] ^= str[i];
 *    hstr[1] ^= str[i];
 *    hstr[2] ^= str[i];
 *    hstr[3] ^= str[i];
 *
 *    //    add some bits out of the middle as low order bits.
 *    hval = hval + (( hval >> 12) & 0x0000ffff) ;
 *
 *    //     swap bytes 0 with 3
 *    chtmp = hstr [0];
 *    hstr[0] = hstr[3];
 *    hstr [3] = chtmp;
 *
 *    //    rotate hval 3 bits to the left (thereby making the
 *    //    3rd msb of the above mess the hsb of the output hash)
 *    hval = (hval << 3 ) + (hval >> 29);
 *  }
 * return (hval);
 * }
 ****/

// This is a more portable hash function, compatible with the original.
// It should return the same value both on 32 and 64 bit architectures.
// The return type was changed to unsigned long hashes, and the other
// parts of the code updated accordingly.
// -- Fidelis

#if !defined (GER)

#if defined (CRM_WITH_OLD_HASH_FUNCTION)

crmhash_t strnhash(const char *str, long len)
{
  long i;
  // unsigned long hval;
  int32_t hval;
  crmhash_t tmp;

  // initialize hval
  hval = len;

  //  for each character in the incoming text:
  for (i = 0; i < len; i++)
  {
    //    xor in the current byte against each byte of hval
    //    (which alone gaurantees that every bit of input will have
    //    an effect on the output)

    tmp = str[i] & 0xFF;
    tmp = tmp | (tmp << 8) | (tmp << 16) | (tmp << 24);
    hval ^= tmp;

    //    add some bits out of the middle as low order bits.
    hval = hval + ((hval >> 12) & 0x0000ffff);

    //     swap most and min significative bytes
    tmp = (hval << 24) | ((hval >> 24) & 0xff);
    hval &= 0x00ffff00;             // zero most and min significative bytes of hval
    hval |= tmp;                    // OR with swapped bytes

    //    rotate hval 3 bits to the left (thereby making the
    //    3rd msb of the above mess the hsb of the output hash)
    hval = (hval << 3) + (hval >> 29);
  }
  return hval;
}

#else

crmhash_t strnhash(const char *str, size_t len)
{
  size_t i;
  crmhash_t hval;
  crmhash_t tmp;

  // initialize hval
  hval = len;

  //  for each character in the incoming text:
  for (i = 0; i < len; i++)
  {
    //    xor in the current byte against each byte of hval
    //    (which alone gaurantees that every bit of input will have
    //    an effect on the output)

    tmp = ((unsigned char *)str)[i];
    tmp = tmp | (tmp << 8);
    tmp = tmp | (tmp << 16);
    hval ^= tmp;

    //    add some bits out of the middle as low order bits.
    hval = hval + ((hval >> 12) & 0x0000ffff);

    //     swap most and min significant bytes
    tmp = (hval << 24) | ((hval >> 24) & 0xff);
    hval &= 0x00ffff00;             // zero most and min significant bytes of hval
    hval |= tmp;                    // OR with swapped bytes

    //    rotate hval 3 bits to the left (thereby making the
    //    3rd msb of the above mess the hsb of the output hash)
    hval = (hval << 3) | ((hval >> 29) & 0x7);
  }
  return hval;
}

#endif

crmhash64_t strnhash64(const char *str, size_t len)
{
  crmhash64_t ihash = strnhash(str, len);

  //
  //     build a 64-bit hash by changing the initial conditions and
  //     by using all but two of the characters and by overlapping
  //     the results by two bits.  This is intentionally evil and
  //     tangled.  Hopefully it will work.
  //
  if (len > 3)
    ihash = (ihash << 30) + strnhash(&str[1], len - 2);
  return ihash;
}

#else

crmhash_t strnhash(const char *str, size_t len)
{
  uint32_t a = 0;
  uint32_t b = 0;

  hashlittle2(str, len, &a, &b);
  return a;
}

crmhash64_t strnhash64(const char *str, size_t len)
{
  uint32_t a = 0;
  uint32_t b = 0;
  crmhash64_t h;

  hashlittle2(str, len, &a, &b);
  h = b;
  h = (h << 32) | a;
  return h;
}

#endif


////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
//    Cached mmap stuff.  Adapted from Win32 compatibility code from
//    Barry Jaspan.  Altered to not reveal the difference between a
//    mapped file pointer and one of Barry's 'map' structs.  In this
//    code (unlike Barry's patches), all that is ever seen are
//    pointers to memory (i.e. crm_mmap and crm_munmap have the same
//    API and semantics as with the libc mmap() and munmap() calls),
//    no structs are ever seen by the callers of this code.
//
//     Bugs in the POSIX code are my fault.  Bugs in the WIN32 code are
//     either mine or his.  So there.
//

///////////////////////////////////////////////////////////////////////////
//
//     This code section (from this line to the line below that states
//     that it is the end of the dual-licensed code section) is
//     copyright and owned by William S. Yerazunis.  In return for
//     addition of significant derivative work, Barry Jaspan is hereby
//     granted a full unlimited license to use this code section,
//     including license to relicense under other licenses.
//
////////////////////////////////////////////////////////////////////////////


//     An mmap cell.  This is how we cache.
//
typedef struct prototype_crm_mmap_cell
{
  char       *name;
  long        start;
  long        requested_len;
  long        actual_len;
  crmhash64_t modification_time_hash;  // st_mtime - time last modified
  void       *addr;
  long        prot;     //    prot flags to be used, in the mmap() form
                        //    that is, PROT_*, rather than O_*
  long mode;            //   Mode is things like MAP_SHARED or MAP_LOCKED

  int unmap_count;         //  counter - unmap this after UNMAP_COUNT_MAX
  struct prototype_crm_mmap_cell *next, *prev;
#if defined (WIN32)
  HANDLE fd, mapping;
#else
  int fd;
#endif
  long        user_actual_len;
  void       *user_addr;
} CRM_MMAP_CELL;



//  We want these to hang around but not be visible outside this file.

static CRM_MMAP_CELL *cache = NULL;



///////////////////////////////////////
//
// Calculate a 64-bit hash of the create+modify timestamps
// for future comparison.
// If the OS/file system supports nanosecond timestamp parts,
// include those in the hash.
//
// Q: Why the hash?
// A: For easier OS/platform independent timestamp comparison.
//    This way, we can keep all the OS/fs dependent stuff
//    in a single place: right here.
//
// Of course, the assumption here is that that 64-bit hash
// is 'good enough' to always deliver different hashes for
// files which have different timestamps on the same OS/fs
// within say, a day, apart.
//
static crmhash64_t calc_file_mtime_hash(struct stat *fs, const char *filename)
{
  time_t buf[6] = { 0 };

  buf[0] = fs->st_ctime;
  buf[1] = fs->st_mtime;
#if defined (HAVE_NSEC_STAT_TIME_NSEC)
  buf[2] = (time_t)fs->st_ctime_nsec;
  buf[3] = (time_t)fs->st_mtime_nsec;
#elif defined (HAVE_NSEC_STAT_TIM_TV_NSEC)
  buf[2] = (time_t)fs->st_ctim.tv_nsec;
  buf[3] = (time_t)fs->st_mtim.tv_nsec;
#elif defined (HAVE_NSEC_STAT_TIMENSEC)
  buf[2] = (time_t)fs->st_ctimensec;
  buf[3] = (time_t)fs->st_mtimensec;
#elif defined (WIN32)
  /*
   * From the MSVC docs:
   *
   * File Times and Daylight Saving Time
   *
   * You must take care when using file times if the user has
   * set the system to automatically adjust for daylight saving
   * time.
   *
   * To convert a file time to local time, use the
   * FileTimeToLocalFileTime function. However,
   * FileTimeToLocalFileTime uses the current settings for the
   * time zone and daylight saving time. Therefore, if it is
   * daylight saving time, it takes daylight saving time into
   * account, even if the file time you are converting is
   * in standard time.
   *
   * The FAT file system records times on disk in local time.
   * GetFileTime retrieves cached UTC times from the FAT file
   * system. When it becomes daylight saving time, the time
   * retrieved by GetFileTime is off an hour, because the cache
   * is not updated. When you restart the computer, the cached
   * time that GetFileTime retrieves is correct. FindFirstFile
   * retrieves the local time from the FAT file system and
   * converts it to UTC by using the current settings for the
   * time zone and daylight saving time. Therefore, if it is
   * daylight saving time, FindFirstFile takes daylight saving
   * time into account, even if the file time you are converting
   * is in standard time.
   *
   * The NTFS file system records times on disk in UTC.
   */
  if (filename != NULL)
  {
    HANDLE handle;
    WIN32_FIND_DATA fdata;

    handle = FindFirstFile(filename, &fdata);
    if (handle != INVALID_HANDLE_VALUE)
    {
      if (internal_trace)
      {
        fprintf(stderr,
                "The file found by calc_file_mtime_hash() is '%s'\n",
                fdata.cFileName);
      }
      buf[2] = (time_t)fdata.ftCreationTime.dwHighDateTime;
      buf[3] = (time_t)fdata.ftCreationTime.dwLowDateTime;
      buf[4] = (time_t)fdata.ftLastWriteTime.dwHighDateTime;
      buf[5] = (time_t)fdata.ftLastWriteTime.dwLowDateTime;

      FindClose(handle);
    }
  }
#endif

  return strnhash64((const char *)buf, sizeof(buf));
}

//////////////////////////////////////
//
//     Force an unmap (don't look at the unmap_count, just do it)
//     Watch out tho - this takes a CRM_MMAP_CELL, not a *ptr, so don't
//     call it from anywhere except inside this file.
//
static void crm_unmap_file_internal(CRM_MMAP_CELL *map)
{
#if defined (HAVE_MSYNC) || defined (HAVE_MUNMAP)
  int munmap_status;

#if defined (HAVE_MSYNC)
  if (map->prot & PROT_WRITE)
  {
    munmap_status = msync(map->addr, map->actual_len, MS_SYNC | MS_INVALIDATE);
    if (munmap_status != 0)
    {
      nonfatalerror_ex(SRC_LOC(),
                       "mmapped file sync failed for file '%s': error %d(%s)",
                       map->name,
                       errno,
                       errno_descr(errno)
      );
    }
  }
#endif
  munmap_status = munmap(map->addr, map->actual_len);
  if (munmap_status != 0)
  {
    fatalerror_ex(SRC_LOC(),
                  "Failed to release (unmap) the mmapped file '%s': error %d(%s)",
                  map->name,
                  errno,
                  errno_descr(errno)
    );
  }
  //  fprintf(stderr, "Munmap_status is %ld\n", munmap_status);

#if 0
  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  //
  //    The fix is that for files that were mmapped writably, to do
  //    a trivial read/write on the mapped file, to force the
  //    filesystem to repropagate it's caches.
  //
  if (map->prot & PROT_WRITE)
  {
    FEATURE_HEADER_STRUCT foo;
    lseek(map->fd, 0, SEEK_SET);
    read(map->fd, &foo, sizeof(foo));
    lseek(map->fd, 0, SEEK_SET);
    write(map->fd, &foo, sizeof(foo));
  }
#endif

  //     Although the docs say we can close the fd right after mmap,
  //     while leaving the mmap outstanding even though the fd is closed,
  //     actual testing versus several kernels shows this leads to
  //     broken behavior.  So, we close here instead.
  //
  close(map->fd);
  //  fprintf(stderr, "U");

  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  CRM_ASSERT(map->name != NULL);
  crm_touch(map->name);

#elif defined (WIN32)
  FlushViewOfFile(map->addr, 0);
  UnmapViewOfFile(map->addr);
  CloseHandle(map->mapping);
  CloseHandle(map->fd);

  //    Because mmap/munmap doesn't set atime, nor set the "modified"
  //    flag, some network filesystems will fail to mark the file as
  //    modified and so their cacheing will make a mistake.
  CRM_ASSERT(map->name != NULL);
  crm_touch(map->name);
#else
#error "please provide an munmap implementation for your platform"
#endif
}

/////////////////////////////////////////////////////
//
//     Hard-unmap by filename.   Do this ONLY if you
//      have changed the file by some means outside of
//      the mmap system (i.e. by writing via fopen/fwrite/fclose).
//
void crm_force_munmap_filename(char *filename)
{
  CRM_MMAP_CELL *p;

  //    Search for the file - if it's already mmaped, unmap it.
  //    Note that this is a while loop and traverses the list.
#if 0 // this one worked too, but just in case when you have multiple mmap()s to the same file, use the latter:
  for (p = cache; p != NULL; p = p->next)
  {
    if (strcmp(p->name, filename) == 0)
    {
      //   found it... force an munmap.
      crm_force_munmap_addr(p->user_addr);
      break;         //  because p WILL be clobbered during unmap.
    }
  }
#else
  for (p = cache; p != NULL;)
  {
    CRM_MMAP_CELL *next = p->next;

    if (strcmp(p->name, filename) == 0)
    {
      //   found it... force an munmap.
      crm_force_munmap_addr(p->user_addr);
      // because p WILL be clobbered during unmap:
      // when you get here, anything pointed at by p is damaged: free()d memory in function call above.
    }
    p = next;
  }
#endif
}


//////////////////////////////////////////////////////
//
//      Hard-unmap by address.  Do this ONLY if you
//      have changed the file by some means outside of
//      the mmap system (i.e. by writing via fopen/fwrite/fclose).
//
void crm_force_munmap_addr(void *addr)
{
  CRM_MMAP_CELL *p;

  //     step 1- search the mmap cache to see if we actually have this
  //     mmapped
  //
  p = cache;
  while (p != NULL && p->user_addr != addr)
    p = p->next;

  if (!p)
  {
    nonfatalerror("Internal fault - this code has tried to force unmap memory "
                  "that it never mapped in the first place.  ",
                  "Please file a bug report. ");
    return;
  }

  //   Step 2: we have the mmap cell of interest.  Mark it for real unmapping.
  //
  p->unmap_count = UNMAP_COUNT_MAX + 1;

  //   Step 3: use the standard munmap to complete the unmapping
  crm_munmap_file(addr);
}


//////////////////////////////////////////////////////
//
//      This is the wrapper around the "traditional" file unmap, but
//      does cacheing.  It keeps count of unmappings and only unmaps
//      when it needs to.
//
void crm_munmap_file(void *addr)
{
  CRM_MMAP_CELL *p;

  //     step 1- search the mmap cache to see if we actually have this
  //     mmapped
  //
  p = cache;
  while (p != NULL && p->user_addr != addr)
    p = p->next;

  if (!p)
  {
    nonfatalerror("Internal fault - this code has tried to unmap memory "
                  "that it never mapped in the first place.  ",
                  "Please file a bug report. ");
    return;
  }

  //   Step 2: we have the mmap cell of interest.  Do the right thing.
  //
  p->unmap_count = (p->unmap_count) + 1;
  if (p->unmap_count > UNMAP_COUNT_MAX)
  {
    crm_unmap_file_internal(p);
    //
    //    File now unmapped, take the mmap_cell out of the cache
    //    list as well.
    //
    if (p->prev != NULL)
    {
      p->prev->next = p->next;
    }
    else
    {
      CRM_ASSERT(cache == p);
      cache = p->next;
    }
    if (p->next != NULL)
    {
      p->next->prev = p->prev;
    }
    else
    {
      CRM_ASSERT(p->prev ? p->prev->next == NULL : 1);
      CRM_ASSERT(!p->prev ? cache == NULL : 1);
    }
    free(p->name);
    free(p);
  }
  else
  {
    if (p->prot & PROT_WRITE)
    {
#if defined (HAVE_MSYNC)
      int ret = msync(p->addr, p->actual_len, MS_SYNC | MS_INVALIDATE);
      if (ret != 0)
      {
        nonfatalerror_ex(SRC_LOC(),
                         "mmapped file sync failed for file '%s': error %d(%s)",
                         p->name,
                         errno,
                         errno_descr(errno)
        );
      }

#elif defined (WIN32)
      //unmap our view of the file, which will immediately write any
      //changes back to the file
      FlushViewOfFile(p->addr, 0);
      UnmapViewOfFile(p->addr);
      //and remap so we still have it open
      p->addr = MapViewOfFile(p->mapping,
                              ((p->mode & MAP_PRIVATE)
                               ? FILE_MAP_COPY
                               : ((p->prot & PROT_WRITE)
                                  ? FILE_MAP_WRITE
                                  : FILE_MAP_READ)),
                              0, 0, 0);
      //if the remap failed for some reason, just free everything
      //  and get rid of this cached mmap entry.
      if (p->addr == NULL)
      {
        CloseHandle(p->mapping);
        CloseHandle(p->fd);
        if (p->prev != NULL)
        {
          p->prev->next = p->next;
        }
        else
        {
          CRM_ASSERT(cache == p);
          cache = p->next;
        }
        if (p->next != NULL)
        {
          p->next->prev = p->prev;
        }
        else
        {
          CRM_ASSERT(p->prev ? p->prev->next == NULL : 1);
          CRM_ASSERT(!p->prev ? cache == NULL : 1);
        }
        free(p->name);
        free(p);
      }
#else
#error \
  "please provide a msync() alternative here (some systems do not have msync but perform this action in munmap itself --> you'll have to augment configure/sysincludes then."
#endif
    }
  }
}


/////////////////////////////////////////////////////////
//
//           Force an Unmap on every mmapped memory area we know about
void crm_munmap_all(void)
{
  while (cache != NULL)
  {
    cache->unmap_count = UNMAP_COUNT_MAX + 1;
    crm_munmap_file(cache->user_addr);
  }
}


//////////////////////////////////////////////////////////
//
//           MMap a file in (or get the map from the cache, if possible)
//             (length is how many bytes to get mapped, remember!)
//
//     prot flags are in the mmap() format - that is, PROT_, not O_ like open.
//      (it would be nice if length could be self-generated...)

void *crm_mmap_file(char *filename, long start, long requested_len, long prot, long mode, long *actual_len)
{
  CRM_MMAP_CELL *p;
  long pagesize = 0;
  struct stat statbuf = { 0 };

#if defined (HAVE_MMAP)
  mode_t open_flags;
#elif defined (WIN32)
  DWORD open_flags = 0;
  DWORD createmap_flags = 0;
  DWORD openmap_flags = 0;
#endif

  pagesize = getpagesize();    // see sysincludes for the actual 'mess' ;-)

  if ((start % pagesize) != 0 || start < 0)
  {
    untrappableerror_ex(SRC_LOC(),
                        "The system cannot memory map (mmap) any file when "
                        "the requested offset %ld is not on a system page "
                        "boundary.   Tough luck for file '%s'.",
                        start,
                        filename);
  }

  //    Search for the file - if it's already mmaped, just return it.
  for (p = cache; p != NULL; p = p->next)
  {
    if (strcmp(p->name, filename) == 0
        && p->prot == prot
        && p->mode == mode
        && p->start == start
        && p->requested_len == requested_len)
    {
      // check the mtime; if this differs between cache and stat
      // val, then someone outside our process has played with the
      // file and we need to unmap it and remap it again.
      struct stat statbuf;
      int k = stat(filename, &statbuf);
      if (k != 0 || p->modification_time_hash != calc_file_mtime_hash(&statbuf, filename))
      {
        // yep, someone played with it. unmap and remap
        crm_force_munmap_filename(filename);
      }
      else
      {
        //  nope, it looks clean.  We'll reuse it.
        if (actual_len)
          *actual_len = p->user_actual_len;
        return p->user_addr;
      }
    }
  }
  //    No luck - we couldn't find the matching file/start/len/prot/mode
  //    We need to add an mmap cache cell, and mmap the file.
  //
  p = (void *)calloc(1, sizeof(p[0]));
  if (p == NULL)
  {
    untrappableerror(" Unable to alloc enough memory for mmap cache.  ",
                     " This is unrecoverable.  Sorry.");
    return MAP_FAILED;    /* [i_a] unreachable code */
  }
  p->name = strdup(filename);
  p->start = start;
  p->requested_len = requested_len;
  p->prot = prot;
  p->mode = mode;

#if defined (HAVE_MMAP) && defined (HAVE_MODE_T)
  open_flags = O_RDWR;
  if (!(p->prot & PROT_WRITE) && (p->prot & PROT_READ))
    open_flags = O_RDONLY;
  if ((p->prot & PROT_WRITE) && !(p->prot & PROT_READ))
    open_flags = O_WRONLY;
  open_flags |= O_BINARY;
  if (internal_trace)
    fprintf(stderr, "MMAP file open mode: %ld\n", (long)open_flags);

  CRM_ASSERT(strcmp(p->name, filename) == 0);

  //   if we need to, we stat the file
  //if (p->requested_len < 0) -- [i_a] we ALWAYS need this stat() call: for the modified time below!
  {
    int k;
    k = stat(filename, &statbuf);
    if (k != 0)
    {
      free(p->name);
      free(p);
      if (actual_len)
        *actual_len = 0;
      return MAP_FAILED;
    }
  }

  //  and put in the mtime as well; make sure we call the calc routine before re-opening the file, just in case...
  p->modification_time_hash = calc_file_mtime_hash(&statbuf, filename);

  if (user_trace)
    fprintf(stderr, "MMAPping file %s for direct memory access.\n", filename);
  p->fd = open(filename, open_flags);
  if (p->fd < 0)
  {
    free(p->name);
    free(p);
    if (actual_len)
      *actual_len = 0;
    return MAP_FAILED;
  }

  //   If we didn't get a length, fill in the max possible length via statbuf
  p->actual_len = p->requested_len;
  if (p->actual_len < 0)
    p->actual_len = statbuf.st_size - p->start;

#if 0 // this code has moved up
  //  and put in the mtime as well
  p->modification_time = statbuf.st_mtime;
#endif

  //  fprintf(stderr, "m");
  p->addr = mmap(NULL,
                 p->actual_len,
                 p->prot,
                 p->mode,
                 p->fd,
                 p->start);
  //fprintf(stderr, "M");

  //     we can't close the fd now (the docs say yes, testing says no,
  //     we need to wait till we're really done with the mmap.)
  //close(p->fd);

  if (p->addr == MAP_FAILED)
  {
    close(p->fd);
    free(p->name);
    free(p);
    if (actual_len)
      *actual_len = 0;
    return MAP_FAILED;
  }

#elif defined (WIN32)
  if (p->mode & MAP_PRIVATE)
  {
    open_flags = GENERIC_READ;
    createmap_flags = PAGE_WRITECOPY;
    openmap_flags = FILE_MAP_COPY;
  }
  else
  {
    if (p->prot & PROT_WRITE)
    {
      open_flags = GENERIC_WRITE;
      createmap_flags = PAGE_READWRITE;
      openmap_flags = FILE_MAP_WRITE;
    }
    if (p->prot & PROT_READ)
    {
      open_flags |= GENERIC_READ;
      if (!(p->prot & PROT_WRITE))
      {
        createmap_flags = PAGE_READONLY;
        openmap_flags = FILE_MAP_READ;
      }
    }
  }
  if (internal_trace)
    fprintf(stderr, "MMAP file open mode: %ld\n", (long)open_flags);

  CRM_ASSERT(strcmp(p->name, filename) == 0);

  //  If we need to, we stat the file.
  //if (p->requested_len < 0) -- [i_a] we ALWAYS need this stat() call: for the modified time below!
  {
    int k;
    k = stat(filename, &statbuf);
    if (k != 0)
    {
      free(p->name);
      free(p);
      if (actual_len)
        *actual_len = 0;
      return MAP_FAILED;
    }
  }

  //  and put in the mtime as well; make sure we call the calc routine before re-opening the file, just in case...
  p->modification_time_hash = calc_file_mtime_hash(&statbuf, filename);

  if (user_trace)
    fprintf(stderr, "MMAPping file %s for direct memory access.\n", filename);

  p->fd = CreateFile(filename, open_flags, 0,
                     NULL, OPEN_EXISTING, 0, NULL);
  if (p->fd == INVALID_HANDLE_VALUE)
  {
    free(p->name);
    free(p);
    return MAP_FAILED;
  }

  p->actual_len = p->requested_len;
  if (p->actual_len < 0)
    p->actual_len = statbuf.st_size - p->start;

  p->mapping = CreateFileMapping(p->fd,
                                 NULL,
                                 createmap_flags, 0, requested_len,
                                 NULL);
  if (p->mapping == NULL)
  {
    CloseHandle(p->fd);
    free(p->name);
    free(p);
    return MAP_FAILED;
  }
  p->addr = MapViewOfFile(p->mapping, openmap_flags, 0, 0, 0);
  if (p->addr == NULL)
  {
    CloseHandle(p->mapping);
    CloseHandle(p->fd);
    free(p->name);
    free(p);
    return MAP_FAILED;
  }

  //  Jaspan-san says force-loading every page is a good thing
  //  under Windows.  I know it's a bad thing under Linux,
  //  so we'll only do it under Windows.
  {
    char one_byte;

    char *addr = (char *)p->addr;
    long i;
    for (i = 0; i < p->actual_len; i += pagesize)
      one_byte = addr[i];
  }
#else
#error "please provide a mmap() equivalent"
#endif

  //   Now, insert this fresh mmap into the cache list
  //
  p->unmap_count = 0;
  p->prev = NULL;
  p->next = cache;
  if (cache != NULL)
  {
    cache->prev = p;
  }
  cache = p;

  p->user_addr = p->addr;
  p->user_actual_len = p->actual_len;
  crm_correct_for_version_header(&p->user_addr, &p->user_actual_len);

  //   If the caller asked for the length to be passed back, pass it.
  if (actual_len)
    *actual_len = p->user_actual_len;

  return p->user_addr;
}



/*
 * Return pointer to CRM versioning header in mmap()ed memory, if such is available.
 *
 * Otherwise, return NULL.
 */
void *crm_get_header_for_mmap_file(void *addr)
{
  CRM_MMAP_CELL *p;

  p = cache;
  while (p != NULL && p->user_addr != addr)
    p = p->next;

  if (!p)
    return NULL;

  return p->addr;
}


/* [i_a] moved times() call to porting section */



///////////////////////////////////////////////////////////////////////
//
//         End of section of code dual-licensed to Yerazunis and Jaspan
//
///////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//
//     strntrn - translate characters of a string.
//
//     Original spec by Bill Yerazunis, original code by Raul Miller,
//     recode for CRM114 use by Bill Yerazunis.
//
//     This code section (crm_strntrn and subsidiary routines) is
//     dual-licensed to both William S. Yerazunis and Raul Miller,
//     including the right to reuse this code in any way desired,
//     including the right to relicense it under any other terms as
//     desired.
//
//////////////////////////////////////////////////////////////////////
//
//   We start out with two helper routines - one to invert a string,
//   and the other to expand string ranges.
//
//////////////////////////////////////////////////////////////////////
//
//   Given a string of characters, invert it - that is, the string
//   that was originally 0x00 to 0xFF but with all characters that
//   were in the incoming string omitted and the string repacked.
//
//   Returns a pointer to the fresh inversion, or NULL (on error)
//
//   The old string is unharmed.  Be careful of it.
//
//   REMEMBER TO FREE() THE RESULT OR ELSE YOU WILL LEAK MEMORY!!!


unsigned char *crm_strntrn_invert_string(unsigned char *str,
                                         long           len,
                                         long          *rlen)
{
  unsigned char *outstr;
  long i, j;

  //  create our output string space.  It will never be more than 256
  //  characters.  It might be less.  But we don't care.
  outstr = calloc(256, sizeof(outstr[0]));

  //  error out if there's a problem with MALLOC
  if (!outstr)
  {
    untrappableerror(
      "Can't allocate memory to invert strings for strstrn", "");
  }

  //  The string of all characters is the inverse of "" (the empty
  //  string), so a mainline string of "^" inverts here to the string
  //  of all characters from 0x00 to 0xff.
  //
  //  The string "^" (equivalent to total overall string "^^") is the
  //  string of all characters *except* ^; the mainline code suffices
  //  for that situation as well.
  //
  //  BUT THEN how does one specify the string of a single "^"?  Well,
  //  it's NOT of NOT of "NOT" ("^"), so "^^^" in the original, or
  //  "^^" here, is taken as just a literal "^" (one carat character).
  //
  if (len == 2 && strncmp((char *)str, "^^", 2) == 0)
  {
    outstr[0] = '^';
    *rlen = 1;
    return outstr;
  }

  //  No such luck.  Fill our map with "character present".
  //  fill it with 1's  ( :== "character present")
  //
  for (i = 0; i < 256; i++)
    outstr[i] = 1;

  //   for each character present in the input string, zero the output string.
  for (i = 0; i < len; i++)
    outstr[str[i]] = 0;

  //   outstr now is a map of the characters that should be present in the
  //   final output string.  Since at most this is 1:1 with the map (which may
  //   have zeros) we can just reuse outstr.
  //
  for (i = 0, j = 0; i < 256; i++)
    if (outstr[i])
    {
      outstr[j] = (unsigned char)i;
      j++;
    }
  CRM_ASSERT(j <= 256);

  //    The final string length is j characters long, in outstr.
  //    Don't forget to free() it later.  :-)

  //  fprintf(stdout, "Inversion: '%s' RLEN: %d\n", outstr, *rlen);
  *rlen = j;
  return outstr;
}

//   expand those hyphenated string ranges - input is str, of length len.
//    We return the new string, and the new length in rlen.
//
unsigned char *crm_strntrn_expand_hyphens(unsigned char *str,
                                          long           len,
                                          long          *rlen)
{
  long j, k, adj;
  unsigned char *r;

  //    How much space do we need for the expanded-hyphens string
  //    (note that the string might be longer than 256 characters, if
  //    the user specified overlapping ranges, either intentionally
  //    or unintentionally.
  //
  //    On the other hand, if the user used a ^ (invert) as the first
  //    character, then the result is gauranteed to be no longer than
  //    255 characters.
  //
  for (j = 1, adj = 0; j < len - 1; j++)
  {
    if ('-' == str[j])
    {
      adj += abs(str[j + 1] - str[j - 1]) - 2;
    }
  }

  //      Get the string length for our expanded strings
  //
  *rlen = adj + len;

  //      Get the space for our expanded string.
  r = calloc((1 + *rlen), sizeof(r[0]));        /* 1 + to avoid empty problems */
  if (!r)
  {
    untrappableerror(
      "Can't allocate memory to expand hyphens for strstrn", "");
  }

  //   Now expand the string, from "str" into "r"
  //

  for (j = 0, k = 0; j < len; j++)
  {
    r[k] = str[j];
    //  are we in a hyphen expression?  Check edge conditions too!
    if ('-' == str[j] && j > 0 && j < len - 1)
    {
      //  we're in a hyphen expansion
      if (j && j < len)
      {
        int delta;
        int m = str[j - 1];
        int n = str[j + 1];
        int c;

        //  is this an increasing or decreasing range?
        delta = m < n ? 1 : -1;

        //  run through the hyphen range.
        if (m != n)
        {
          for (c = m + delta; c != n; c += delta)
          {
            r[k++] = (unsigned char)c;
          }
          r[k++] = (unsigned char)n;
        }
        j += 1;
      }
    }
    else
    {
      //    It's not a range, so we just move along.  Move along!
      k++;
    }
  }

  //  fprintf(stderr, "Resulting range string: %s \n", r);
  //  return the char *string.
  return r;
}

//   strntrn - translate a string, like tr() but more fun.
//    This new, improved version not only allows inverted ranges
//     like 9-0 --> 9876543210 but also negation of strings and literals
//
//      flag of CRM_UNIQUE means "uniquify the incoming string"
//
//      flag of CRM_LITERAL means "don't interpret the alteration string"
//      so "^" and "-" regain their literal meaning
//
//      The modification is "in place", and datastrlen gets modified.
//       This routine returns a long >=0 strlen on success,
//        and a negative number on failure.

long strntrn(
  unsigned char *datastr,
  long          *datastrlen,
  long           maxdatastrlen,
  unsigned char *fromstr,
  long           fromstrlen,
  unsigned char *tostr,
  long           tostrlen,
  long           flags)
{
  long len = *datastrlen;
  long flen, tlen;
  unsigned char map[256];
  unsigned char *from = NULL;
  unsigned char *to = NULL;
  long j, k, last;

  //               If tostrlen == 0, we're deleting, except if
  //                 ASLO fromstrlen == 0, in which case we're possibly
  //                   just uniquing or maybe not even that.
  //
  int replace = tostrlen;

  CRM_ASSERT(len < maxdatastrlen);

  //     Minor optimization - if we're just uniquing, we don't need
  //     to do any of the other stuff.  We can just return now.
  //
  if (tostrlen == 0 && fromstrlen == 0)
  {
    // fprintf(stderr, "Fast exit from strntrn  \n");
    *datastrlen = len;
    return len;
  }


  //    If CRM_LITERAL, the strings are ready, otherwise build the
  //    expanded from-string and to-string.
  //
  if (CRM_LITERAL & flags)
  {
    //       Else - we're in literal mode; just copy the
    //       strings.
    from = calloc(fromstrlen, sizeof(from[0]));
    strncpy((char *)from,  (char *)fromstr, fromstrlen);
    flen = fromstrlen;
    to = calloc(tostrlen, sizeof(to[0]));
    strncpy((char *)to, (char *)tostr, tostrlen);
    tlen = tostrlen;
    if (from == NULL || to == NULL) return -1;
  }
  else
  {
    //  Build the expanded from-string
    if (fromstr[0] != '^')
    {
      from = crm_strntrn_expand_hyphens(fromstr, fromstrlen, &flen);
      if (!from) return -1;
    }
    else
    {
      unsigned char *temp;
      long templen;
      temp = crm_strntrn_expand_hyphens(fromstr + 1, fromstrlen - 1, &templen);
      if (!temp) return -1;

      from = crm_strntrn_invert_string(temp, templen, &flen);
      if (!from) return -1;

      free(temp);
    }

    //     Build the expanded to-string
    //
    if (tostr[0] != '^')
    {
      to = crm_strntrn_expand_hyphens(tostr, tostrlen, &tlen);
      if (!to) return -1;
    }
    else
    {
      unsigned char *temp;
      long templen;
      temp = crm_strntrn_expand_hyphens(tostr + 1, tostrlen - 1, &templen);
      if (!temp) return -1;

      to = crm_strntrn_invert_string(temp, templen, &tlen);
      if (!to) return -1;

      free(temp);
    }
  }

  //  If we're in <unique> mode, squish out any duplicated
  //   characters in the input data first.  We can do this as an in-place
  //    scan of the input string, and we always do it if <unique> is
  //     specified.
  //
  if (CRM_UNIQUE & flags)
  {
    unsigned char unique_map[256];

    //                        build the map of the uniqueable characters
    //
    for (j = 0; j < 256; j++)
      unique_map[j] = 1; // all characters are keepers at first...
    for (j = 0; j < flen; j++)
      unique_map[from[j]] = 0; //  but some need to be uniqued.

    //                          If the character has a 0 the unique map,
    //                          and it's the same as the prior character,
    //                          don't copy it.  Just move along.

    for (j = 0, k = 0, last = -1; j < len; j++)
    {
      if (datastr[j] != last || unique_map[datastr[j]])
      {
        last = datastr[k++] = datastr[j];
      }
    }
    len = k;
  }
  CRM_ASSERT(len < maxdatastrlen);

  //     Minor optimization - if we're just uniquing, we don't need

  //     Build the mapping array
  //
  if (replace)
  {
    //  This is replacement mode (not deletion mode) so we need
    //   to build the character map.  We
    //    initialize the map as each character maps to itself.
    //
    for (j = 0; j < 256; j++)
    {
      map[j] = (unsigned char)j;
    }

    //   go through and mod each character in the from-string to
    //   map into the corresponding character in the to-string
    //   (and start over in to-string if we run out)
    //
    for (j = 0, k = 0; j < flen; j++)
    {
      map[from[j]] = to[k];
      //   check- did we run out of characters in to-string, so
      //    that we need to start over in to-string?
      k++;
      if (k >= tlen)
      {
        k = 0;
      }
    }


    //    Finally, the map is ready.  We go through the
    //     datastring translating one character at a time.
    //
    for (j = 0; j < len; j++)
    {
      datastr[j] = map[datastr[j]];
    }
  }
  else
  {
    //  No, we are not in replace mode, rather we are in delete mode
    //  so the map now says whether we're keeping the character or
    //  deleting the character.
    for (j = 0; j < 256; j++)
    {
      map[j] = 1;
    }
    for (j = 0; j < flen; j++)
    {
      map[from[j]] = 0;
    }
    for (j = 0, k = 0; j < len; j++)
    {
      if (map[datastr[j]])
      {
        datastr[k++] = datastr[j];
      }
    }
    len = k;
  }
  CRM_ASSERT(len < maxdatastrlen);

  //          drop the storage that we allocated
  //
  free(from);
  free(to);
  *datastrlen = len;
  return len;
}

/////////////////////////////////////////////////////////////////
//
//   END of strntrn code (dual-licensed to both Yerazunis
//   and Miller
//
//////////////////////////////////////////////////////////////////

