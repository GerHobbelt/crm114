//  crm_fast_substring_compression.c  - CRM114 - version v1.0
//  Copyright 2001-2008  William S. Yerazunis, all rights reserved.
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




#if !CRM_WITHOUT_FSCM



/////////////////////////////////////////////////////////////////
//
//     Compression-match classification
//
//     This classifier is based on the use of the Lempel-Ziv LZ77
//     (published in 1977) algorithm for fast compression; more
//     compression implies better match.
//
//     The basic idea of LZ77 is to encode strings of N characters
//     as a small doublet (Nstart, Nlen), where Nstart and Nlen are
//     backward references into previously seen text.  If there's
//     no previous correct back-reference string, then don't compress
//     those characters.
//
//     Thus, LZ77 is a form of _universal_ compressor; it starts out knowing
//     nothing of what it's to compress, and develops compression
//     tables "on the fly".
//
//     It is well known that one way of doing text matching is to
//     compare relative compressibility - that is, given known
//     texts K1 and K2, the unknown text U is in the same class as K1
//     iff the LZ77 compression of K1|U is smaller (fewer bytes) than
//     the LZ77 compression of K2|U .  (remember you need to subtract
//     the compressed lengths of K1 and K2 respectively).
//
//     There are several ways to generate LZ compression fast; one
//     way is by forward pointers on N-letter prefixes.  Another
//     way is to decide on a maximum string depth and build transfer
//     tables.
//
//     One problem with LZ77 is that finding the best possible compression
//     is NP-hard.  Consider this example:
//
//         ABCDEFGHI DEFGHIJLMN BCDEFGHIJ JKLMNOPQ ABCDEFGHIJKLMNOP
//
//     Is it better to code the final part with the A-J segment
//     followed by the J-P segment, or with a literal ABC, then D-N,
//     then the literal string OP?  Without doing the actual math, you
//     can't easily decide which is the better compression.  In the
//     worst case, the problem becomes the "knapsack" problem and
//     is thus NP-hard.
//
//     To avoid this, we take the following heuristic for our first
//     coding attempt:
//
//          Look for the longest match of characters that
//          match the unknown at this point and use that.
//
//     In the worst case, this algorithm will traverse the entire
//     known text for each possible starting character, and possibly
//     do a local search if that starting character matches the
//     character in the unknown text.  Thus, the time to run is
//     bounded by |U| * |K|.
//
//     Depending on the degree of overlap between strings, this
//     heuristic will be at best no worse than half as good as the
//     best possible heuristic, and (handwave not-proven) not worse
//     than one quarter as good as the best possible heuristic.
//
//     As a speedup, we can use a prefix lookforward based on N
//     (the number of characters we reqire to match before switching
//     from literal characters to start/length ranges).   Each character
//     also carries an index, saying "for the N-character lookforward
//     prefix I am at the start of, you can find another example of this
//     at the following index."
//
//     For example, the string "ABC DEF ABC FGH ABC XYZ" would have
//     these entries inserted sequentially into the lookforward table:
//
//     ABC --> 0
//     BC  --> 1
//     C D --> 2
//      DE --> 3
//     DEF --> 4
//     EF  --> 5
//     F A --> 6
//      AB --> 7
//
//    At this point, note that "ABC" recurs again.  Since we want to
//    retain both references to "ABC" strings, we place the index of
//    the second ABC (== 8) into the "next occurrence" tag of the
//    first "ABC".  (or, more efficiently, set the second ABC to point
//    to the first ABC, and then have the lookforward table point to the
//    second ABC (thus, the chain of ABCs is actually in the reverse order
//    of their encounters).
//
//    For prefix lengths of 1, 2, and 3, the easiest method is to
//    direct-map the prefixes into a table.  The table lengths would
//    be 256, 65536, and 16 megawords (64 megabytes).  The first two are
//    eminently tractable; the third marginally so.  The situation
//    can be improved by looking only at the low order six bits of
//    the characters as addresses into the direct map table.  For normal
//    ASCII, this means that the control characters are mapped over the
//    capital letters, and the digits and punctuation are mapped over
//    lowercase letters, and uses up only 16 megabytes for the table entries.
//
//    Of course, a direct-mapped, 1:1 table is not the only possible
//    table.  It is also possible to create a hash table with overflow
//    chains.  For example, an initial two-character table (256Kbytes) yields
//    the start of a linear-search chain; this chain points to a linked list of
//    all of the third characters yet encountered.
//
//    Here's some empirical data to get an idea of the size of the
//    table actually required:
//
//    for SA's hard_ham
//       lines         words       three-byte sequences
//         114K          464K        121K
//          50K          210K         61K
//          25K          100K         47K
//          10K           47K         31K
//           5K           24K         21K
//
//    For SA's easy_ham_2:
//       lines         words       three-byte sequences
//         134K          675K          97K
//          25K          130K          28K
//
//    For SA's spam_2:
//       lines         words       three-byte sequences
//         197K          832K          211K
//         100K          435K          116K
//
//    So, it looks like in the long term (and in English) there is
//    an expectation of roughly as many 3-byte sequences as there are
//    lines of text, probably going asymptotic at around
//    a quarter of a million unique 3-byte sequences.  Note that
//    the real maximum is 2^24 or about 16 million three-byte
//    sequences; however some of them would never occur except in
//    binary encodings.
//
//
//     ----- Embedded Limited-Length Markov Table Option -----
//
//    Another method of accomplishing this (suitable for N of 3 and larger)
//    is to use transfer tables allocated only on an as-needed basis,
//    and to store the text in the tables directly (thus forming a sort of
//    Markov chain of successive characters in memory).
//
//    The root table contains pointers to the inital occurrence in the known
//    text of all 256 possible first bytes; each subsequent byte then
//    allocates another transfer table up to some predetermined limit on
//    length.
//
//    A disadvantage of this method is that it uses up more memory for
//    storage than the index chaining method; further it must (by necessity)
//    "cut off" at some depth thereby limiting the longest string that
//    we want to allocate another table for.  In the worst case, this
//    system generates a doubly diagonal tree with |K|^2 / 4 tables.
//    On the other hand, if there is a cutoff length L beyond which we
//    don't expand the tables, then only the tables that are needed get
//    allocated.  As a nice side effect, the example text becomes less
//    trivial to extract (although it's there, you have to write a
//    program to extract it rather than just using "strings", unlike the
//    bit entropy classifier where a well-populated array contains a lot
//    of uncertainty and it's very difficult to even get a single
//    byte unambiguously.
//
//    Some empirical data on this method:
//
//     N = 10
//       Text length (bytes)       Tables allocated
//           1211                       7198
//          69K                       232K
//          96K                       411K
//         204K                       791K
//
//     N=5
//       Text length (bytes)       Tables allocated
//            1210                      2368
//           42K                       47K
//           87K                       79K
//          183K                      114K
//          386K                      177K
//          841K                      245K
//         1800K                      342K
//         3566K                      488K
//         6070K                      954K
//         8806K                     1220K
//
//      N=4
//       Text length (bytes)       Tables allocated
//           87K                       40K
//          183K                       59K
//          338K                       89K
//          840K                      121K
//         1800K                      167K
//         3568K                      233K
//         6070K                      438K
//
//      N=3
//       Text length (bytes)       Tables allocated
//           87K                       14K
//          183K                       22K
//          386K                       31K
//          840K                       42K
//         1800K                       58K
//         3568K                       78K
//         6070K                      132K
//
//
//    Let's play with the numbers a little bit.  Note that
//    the 95 printable ASCII characters could have a theoretical
//    maximum of 857K sequences, and the full 128-character ASCII
//    (low bit off) is 2.09 mega-sequences.  If we casefold A-Z onto
//    a-z and all control characters to "space", then the resulting
//    69 characters is only 328K possible unique sequences.
//
//    A simpler method is to fold 0x7F-0x8F down onto 0x00-0x7F, and
//    then 0x00-0x3F onto 0x40-0x7F (yielding nothing but printable
//    characters- however, this does not preserve whitespace in any sense).
//    When folded this way, the SA hard_ham corpus (6 mbytes, 454 words, 114
//    K lines) yields 89,715 unique triplets.
//
//    Of course, for other languages, the statistical asymptote is
//    probably present, but casefolding in a non-Roman font is probably
//    going to give us weak results.
//
//    --- Other Considerations ---
//
//    Because it's unpleasant and CPU-consuming to read and write
//    non-binary-format statistics files (memory mapping is far
//    faster) it's slightly preferable to have statistics files
//    that don't have internal structures that change sizes (appending is
//    more pleasant than rewriting with new offsets).  Secondarily,
//    a lot of users are much more comfortable with the knowledge
//    that their statistics files won't grow without bound.  Therefore,
//    fixed-size structures are often preferred.
//
//
//   --- The text storage ---
//
//   In all but the direct-mapped table method, the text itself needs
//   to be stored because the indexing method only says where to look
//   for the first copy of any particular string head, not where all
//   of the copies are.  Thus, each byte of the text needs an index
//   (usually four bytes) of "next match" information.  This index
//   points to the start of the next string that starts with the
//   current N letters.
//
//   Note that it's not necessary for the string to be unique; the
//   next match chains can contain more than one prefix.  As long
//   as the final matching code knows that the first N bytes need
//   to be checked, there's no requirement that chains cannot be
//   shared among multiple N-byte prefixes.  Indeed, in the limit,
//   a simple sequential search can be emulated by a shared-chain
//   system with just ONE chain (each byte's "try next" pointer
//   points to the next byte in line).  These nonunique "try next"
//   chains may be a good way to keep the initial hash table
//   manageabley small.  However, how to efficiently do this
//   "in line" is unclear (the goal of in-line is to hide the
//   known text so that "strings" can't trivially extract it;
//   the obvious solution is to have two data structures (one by
//   bytes, one by uint32's, but the byte structure is then easily
//   perusable).
//
//   Another method would be to have the initial table point not
//   to text directly, but to a lookforward chain.  Within the chain,
//   cells are allocated only when the offset backward exceeds the
//   offset range allowed by the in-line offset size.  For one-byte
//   text and three-byte offsets, this can only happen if the text
//   grows beyond 16 megabytes of characters (64 megabyte footprint)
//
//    --- Hash Tables Revisited ---
//
//   Another method is to have multiple hash entries for every string
//   starting point.  For example, we might hash "ABC DEF ABC", "ABC DEF",
//   and "ABC" and put each of these into the hash table.
//
//   We might even consider that we can _discard_ the original texts
//   if our hash space is large enough that accidental clashes are
//   sufficiently rare.  For example, with a 64-bit hash, the risk of
//   any two particular strings colliding is 1E-19, and the risk of
//   any collision whatsoever does not approach 50% with less than
//   1 E 9 strings in the storage.
//
//     ------- Hashes and Hash Collisions -----
//
//   To see how the hashes would collide with the CRM114 function strnhash,
//   we ran the Spamassasin hard-ham corpus into three-byte groups, and
//   then hashed the groups.  Before hashing, there were 125,434 unique
//   three-byte sequences; after hashing, there were 124,616 unique hashes;
//   this is 818 hash collisions (a rate of 0.65%).  This is a bit higher
//   than predicted for truly randomly chosen inputs, but then again, the
//   SA corpus has very few bytes with the high order bit set.
//
//    ------ Opportunistic chain sharing -----
//
//   (Note- this is NOT being built just yet; it's just an idea) - the
//   big problem with 24-bit chain offsets is that it might not have
//   enough "reach" for the less common trigrams; in the ideal case any
//   matching substring is good enough and losing substrings is anathema.
//   However, if we have a number of sparse chains that are at risk for
//   not being reachable, we can merge those chains either together or
//   onto another chain that's in no dagner of running out of offsets.
//
//   Note that it's not absolutely necessary for the two chains to be
//   sorted together; as long as the chains are connected end-to-end,
//   the result will still be effective.
//
//    -----  Another way to deal with broken chains -----
//
//    (also NOT being built yet; this is just another possibility)
//   Another option: for systems where there are chains that are about
//   to break because the text is exceeding 16 megabytes (the reach of
//   a 24-bit offset), at the end of each sample document we can insert
//   a "dummy" forwarding cell that merely serves to preserve continuity
//   of any chain that might be otherwise broken because the N-letter prefix
//   string has not occured even once in the preceding 16 megacells.
//   (worst case analysis: there are 16 million three-byte prefixes, so
//   if all but ONE prefix was actually ever seen in a 16-meg block, we'd
//   have a minor edge-case problem for systems that did not do chain
//   folding.  With chain-folding down to 18 bits (256K different chains)
//   we'd have no problem at all, even in the worst corner case.)
//
//   However, we still need to insert these chain-fixers preemptively
//   if we want to use "small" lookforward cells, because small (24-bit)
//   cells don't have the reach to be able to index to the first occurrence
//   of a chain that's never been seen in the first 16 megacharacters.
//   This means that at roughly every 16-megacell boundary we would
//   insert a forwarding dummy block (worst case size of 256K cells, on
//   the average a lot fewer because some will actually get used in real
//   chains.) That sounds like a reasonable tradeoff in size, but the
//   bookkeeping to keep it all straight is goint to be painful to code and
//   test rigorously.
//
//
//     ------- Hashes-only storage ------
//
//   In this method, we don't bother to store the actual text _at all_,
//   but we do store chains of places where it occurred in the original
//   text.  In this case, we LEARN by sliding our window of strnhash(N)
//   characters over the text.  Each position generates a four-byte
//   backward index (which may be NULL) to the most recent previous
//   encounter of this prefix; this chain grows basically without limit.
//
//   To CLASSIFY, we again slide our strnhash(N) window over the text;
//   and for each offset position we gather the (possibly empty) list of
//   places where that hash occurred.  Because the indices are pre-sorted
//   (always in descending order) it is O(n) in the length of the chains
//   to find out any commonality because the chains can be traversed by the
//   "two finger method" (same as in the hyperspace classifier).  The
//   result for any specific starting point is the list of N longest matches
//   for the leading character position as seen so far.  If we choose to
//   "commit on N characters matching, then longest that starts in that
//   chain" then the set of possible matches is the tree of indices and
//   we want the longest branch.
//
//   This is perhaps most easily done by an N-finger method where we keep
//   a list of "fingers" to the jth, j+1, j+2... window positions; at each
//   position j+k we merely insure that there is an unbroken path from j+0
//   to j+k.  (we could speed this up significantly by creating a lookaside
//   list or array that contains ONLY the valid positions at j+k; moving the
//   window to k+1 means only having to check through that array to find at
//   least one member equal to the k+1 th window chain.  In this case, the
//   "two-finger" method suffices, and the calculation can be done "in place".
//   When the path breaks (that is, no feasible matches remain), we take
//   N + k - 1 to be the length of the matched substring and begin again
//   at j = N + k + j.
//
//   Another advantage of this method is that there is no stored text to
//   recover directly; a brute-force attack, one byte at a time, will
//   recover texts but not with absolute certainty as hash collisions
//   might lead to unexpected forks in the chain.
//
//
//     ------- Design Decision -----
//
//   Unless something better comes up,  if we just take the strnhash() of the
//   N characters in the prefix, we will likely get a fairly reasonable
//   distribution of hash values which we can then modulo down to whatever
//   size table we're actually using.  Thus, the size of the prefix and the
//   size of the hah table are both freely variable in this design.
//
//   We will use the "hash chains only" method to store the statistics
//   information (thus providing at least moderate obscuration of the
//   text, as well as moderately efficient storage.
//
//   As a research extension, we will allow an arbitrary regex to determine
//   the meaning of the character window; repeated regexing with k+1 starting
//   positions yield what we will define as "legitimately adjacent window
//   positions".  We specifically define that we do not care if these are
//   genuinely adjacent positions; we can define these things as we wish (and
//   thereby become space-invariant, if we so choose.
//
//   A question: should the regex define the next "character", or should
//   it define the entire next window?  The former allows more flexibility
//   (and true invariance over charactersets); the latter is easier to
//   implement and faster at runtime.  Decision: implment as "defines the
//   whole window".  Then we use the count of subexpressions to define our
//   window length; this would allow skipping arbitrary text - with all the
//   programming power and danger of abuse that entails. Under this paradigm,
//   the character regex is /(.)(.)(.)/ for an N=3 minimum chain.
//
//   A quick test shows that strnhash on [a-zA-Z0-9 .,-] shows no
//   duplications, nor any hash clashes when taken mod 256.  Thus,
//   using a Godel coding scheme (that is, where different offsets are
//   each multiplied by a unique prime number and then those products
//   are added together ) will *probably* give good hash results.
//   Because we're moduloing (taking only the low order bits) the
//   prime number "2" is a bit problematic and we may choose to skip it.
//   Note that a superincreasing property is not useful here.
//
//   Note that the entire SA corpus is only about 16 megabytes of
//   text, so a full index set of the SA corpus would be on the
//   order of 68 megabytes ( 4 megs of index, then another 64 megs
//   of index chains)
//
//   Note also that there is really no constraint that the chains start
//   at the low indices and move higher.  It is equally valid for the chains
//   to start at the most recent indices and point lower in memory; this
//   actually has some advantage in speed of indexing; each chain element
//   points to the _previous_ element and we do the two-finger merge
//   toward lower indices.
//
//   Note also that there is no place the actual text or even the actual
//   hashes of the text are stored.  All hashes that map to the same place
//   in the "seen at" table are deemed identical (and no record is kept);
//   similarly each cell of "saved text" is really only a pointer to the
//   most recent previous location where something that mapped to the
//   same hash table bucket was seen).  Reconstruction of the prior text is
//   hence marginal in confidence.
//
///////////////////////////////////////////////////////////


#ifdef NEED_PRIME_NUMBERS
///////////////////////////////////////////////////////////////
//
//     Some prime numbers to use as weights.
//
//     GROT GROT GROT  note that we have a 1 here instead of a 2 as the
//     first prime number!  That's strictly an artifice to use all of the
//     hash bits and is not an indication that we don't know that 2 is prime.

static unsigned int primes[260] = {
    1,      3,      5,      7,     11,     13,     17,     19,     23,     29,
    31,     37,     41,     43,     47,     53,     59,     61,     67,     71,
    73,     79,     83,     89,     97,     101,    103,    107,    109,    113,
    127,    131,    137,    139,    149,    151,    157,    163,    167,    173,
    179,    181,    191,    193,    197,    199,    211,    223,    227,    229,
    233,    239,    241,    251,    257,    263,    269,    271,    277,    281,
    283,    293,    307,    311,    313,    317,    331,    337,    347,    349,
    353,    359,    367,    373,    379,    383,    389,    397,    401,    409,
    419,    421,    431,    433,    439,    443,    449,    457,    461,    463,
    467,    479,    487,    491,    499,    503,    509,    521,    523,    541,
    547,    557,    563,    569,    571,    577,    587,    593,    599,    601,
    607,    613,    617,    619,    631,    641,    643,    647,    653,    659,
    661,    673,    677,    683,    691,    701,    709,    719,    727,    733,
    739,    743,    751,    757,    761,    769,    773,    787,    797,    809,
    811,    821,    823,    827,    829,    839,    853,    857,    859,    863,
    877,    881,    883,    887,    907,    911,    919,    929,    937,    941,
    947,    953,    967,    971,    977,    983,    991,    997,   1009,   1013,
    1019,   1021,   1031,   1033,   1039,   1049,   1051,   1061,   1063,   1069,
    1087,   1091,   1093,   1097,   1103,   1109,   1117,   1123,   1129,   1151,
    1153,   1163,   1171,   1181,   1187,   1193,   1201,   1213,   1217,   1223,
    1229,   1231,   1237,   1249,   1259,   1277,   1279,   1283,   1289,   1291,
    1297,   1301,   1303,   1307,   1319,   1321,   1327,   1361,   1367,   1373,
    1381,   1399,   1409,   1423,   1427,   1429,   1433,   1439,   1447,   1451,
    1453,   1459,   1471,   1481,   1483,   1487,   1489,   1493,   1499,   1511,
    1523,   1531,   1543,   1549,   1553,   1559,   1567,   1571,   1579,   1583,
    1597,   1601,   1607,   1609,   1613,   1619,   1621,   1627,   1637,   1657
};
#endif


////////////////////////////////////////////////////////////////
//
//     Headers and self-identifying files are a good idea; we'll
//     have it happen here.
//

#define FILE_IDENT_STRING "CRM114 Classdata Fast Substring Compression "



//    The prefix array maps a hash to the most recent occurrence of
//    that hash in the text.

typedef struct
{
    uint32_t index;
} FSCM_PREFIX_TABLE_CELL;

typedef struct
{
    uint32_t next;
} FSCM_HASH_CHAIN_CELL;

#ifdef USING_HASH_COMPARE
static int hash_compare(const unsigned int *a, const unsigned int *b)
{
    FSCM_HASH_CHAIN_CELL *pa, *pb;

    pa = (FSCM_HASH_CHAIN_CELL *)a;
    pb = (FSCM_HASH_CHAIN_CELL *)b;
    if (pa->next < pb->next)
        return -1;

    if (pa->next > pb->next)
        return 1;

    return 0;
}
#endif



////////////////////////////////////////////////////////////////////////
//
//    How to learn in FSCM - two parts:
//      1) append our structures to the statistics file in
//         the FSCM_CHAR_STRUCT format;
//      2) index the new structures; if no prior exists on the chain,
//         let previous link be 0, otherwise it's the prior value in the
//         hash table.
//

int crm_fast_substring_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //     learn the compression version of this input window as
    //     belonging to a particular type.  Note that the word regex
    //     is ignored in this classifier.
    //
    //     learn <flags> (classname)
    //
    int i, j;

    char htext[MAX_PATTERN];         //  the hash name buffer
    char hashfilename[MAX_PATTERN];  // the hashfile name
    FILE *hashf;                     // stream of the hashfile
    unsigned int textoffset, textmaxoffset;
    int hlen;
    int cflags, eflags;

    struct stat statbuf;    //  for statting the statistics file

    char *file_pointer;
    STATISTICS_FILE_HEADER_STRUCT *file_header;
    FSCM_PREFIX_TABLE_CELL *prefix_table;    //  the prefix indexing table,
    unsigned int prefix_table_size = 200000;
    FSCM_HASH_CHAIN_CELL *chains, *newchains;    //  the chain area
    unsigned int newchainstart;                  //  offset in cells of new chains

    int sense;
    int microgroom;
    int unique;
    int use_unigram_features;
    int fev;

    //        unk_hashes is tempbuf, but casting-aliased to FSCM chains
    int unk_hashcount;
    crmhash_t *unk_hashes;

    unk_hashes = (crmhash_t *)tempbuf;



    statbuf.st_size = 0;
    fev = 0;

    if (user_trace)
        fprintf(stderr, "executing an FSCM LEARN\n");


    //           extract the hash file name
    hlen = crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, MAX_PATTERN, vht, tdw);

    ///////////////////////////////////////////////////////
    //
    //            set our cflags, if needed.  The defaults are
    //            "case" and "affirm", (both zero valued).
    //            and "microgroom" disabled.   Many of these make no
    //            sense (or are unimplemented yet) for FSCM.
    //
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
        fev = fatalerror("FSCM Refute is NOT SUPPORTED YET\n", "");
        return fev;

#if 0 // unreachable code
        if (user_trace)
            fprintf(stderr, " refuting learning\n");
#endif
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
    // i = 0;
    // while (htext[i] < 0x021) i++;
    // j = i;
    // while (htext[j] >= 0x021) j++;
    if (!crm_nextword(htext, hlen, 0, &i, &j) || j == 0)
    {
        fev = nonfatalerror_ex(SRC_LOC(),
                "\nYou didn't specify a valid filename: '%.*s'\n",
                hlen,
                htext);
        return fev;
    }
    //             filename starts at i,  ends at j. null terminate it.
    htext[j] = 0;
    strcpy(hashfilename, &htext[i]);

    if (user_trace)
        fprintf(stderr, "Target file file %s\n", hashfilename);

    //   No need to do any parsing of a box restriction.
    //   We got txtptr, txtstart, and txtlen from the caller.
    //
    textoffset = txtstart;
    textmaxoffset = txtstart + txtlen;

    {
        int nosuchfile;
        //    Check to see if we need to create the file; if we need
        //    it, we create it here.
        //
        nosuchfile = stat(hashfilename, &statbuf);
        if (nosuchfile)
        {
            //  Note that "my_header" is a local buffer.
            STATISTICS_FILE_HEADER_STRUCT my_header;

            strcpy((char *)my_header.file_ident_string,
                    "CRM114 Classdata FSCM V2 (hashed) ");

            if (user_trace)
                fprintf(stderr, "No such statistics file %s; must create it.\n",
                        hashfilename);
            //   Set the size of the hash table.
            if (sparse_spectrum_file_length == 0)
                sparse_spectrum_file_length = FSCM_DEFAULT_HASH_TABLE_SIZE;

            /////////////////////////////////////////////////
            //     START OF STANDARD HEADER SETUP
            //   header info chunk 0 - the header chunking info itself
            my_header.chunks[0].start = 0;
            my_header.chunks[0].length = sizeof(STATISTICS_FILE_HEADER_STRUCT);
            my_header.chunks[0].tag = 0;
            //
            //   header info, chunk 1 - the ident string
            my_header.chunks[1].start = my_header.chunks[0].length;
            my_header.chunks[1].length = STATISTICS_FILE_IDENT_STRING_MAX;
            my_header.chunks[1].tag = 1;
            //
            //   header info, chunk 2 - the "arbitrary data"
            my_header.chunks[2].start = my_header.chunks[1].start
                                        + my_header.chunks[1].length;
            my_header.chunks[2].length = STATISTICS_FILE_ARB_DATA;
            my_header.chunks[2].tag = 2;
            //    END OF STANDARD HEADER SETUP
            //////////////////////////////////////////////////

            //   header info chunk 3 - the prefix hastable, fixed size
            my_header.chunks[3].start = my_header.chunks[2].start
                                        + my_header.chunks[2].length;
            my_header.chunks[3].length = sparse_spectrum_file_length;
            my_header.chunks[3].tag = 3;
            //   ... and the length of that hash table will be, in cells:
            my_header.data[0] =
                sparse_spectrum_file_length / sizeof(FSCM_PREFIX_TABLE_CELL);
            //
            //   header info chunk 4 - the previous-seen pointers, growing.
            my_header.chunks[4].start = my_header.chunks[3].start
                                        + my_header.chunks[3].length;
            //    Although the starting length is really zero, zero is a sentinel
            //    so we start at 1 bucket further in...
            my_header.chunks[4].length = sizeof(FSCM_HASH_CHAIN_CELL);
            my_header.chunks[4].tag = 4;

            //    Write out the initial file header..
            hashf = fopen(hashfilename, "wb+");
            if (!hashf)
            {
                fev = nonfatalerror_ex(SRC_LOC(),
                        "\n Couldn't open your new CSS file %s for writing; errno=%d(%s)\n",
                        hashfilename,
                        errno,
                        errno_descr(errno));
                return fev;
            }
            else
            {
                if (1 != fwrite(&my_header,
                            sizeof(STATISTICS_FILE_HEADER_STRUCT),
                            1,
                            hashf))
                {
                    fev = nonfatalerror_ex(SRC_LOC(),
                            "\n Couldn't write header to file %s; errno=%d(%s)\n",
                            hashfilename, errno, errno_descr(errno));
                    fclose(hashf);
                    return fev;
                }
                fclose(hashf);
            }
        }
    }

    if (sense > 0)
    {
        /////////////////
        //    THIS PATH TO LEARN A TEXT
        //      1) Make room!  Append enough unsigned int zeroes that
        //         we will have space for our hashes.
        //      2) MMAP the file
        //      3) actually write the hashes
        //      4) adjust the initial-look table to point to those hashes;
        //          while modifying those hashes to point to the most recent
        //          previous hashes;
        //      5) MSYNC the output file.  As we already did a file system
        //          write it should not be necessary to do the mtime write.
        //
        /////////////////
        //    Write out the initial previous-seen hash table (all zeroes):

        {
            FSCM_PREFIX_TABLE_CELL my_zero;
            my_zero.index = 0;
            hashf = fopen(hashfilename, "ab+");
            if (!hashf)
            {
                fev = nonfatalerror_ex(SRC_LOC(),
                        "\n Couldn't open your CSS file %s for appending; errno=%d(%s)\n",
                        hashfilename,
                        errno,
                        errno_descr(errno));
                return fev;
            }
            else
            {
                for (i = 0; i < sparse_spectrum_file_length; i++)
                {
                    if (1 != fwrite(&my_zero,
                                sizeof(FSCM_PREFIX_TABLE_CELL),
                                1,
                                hashf))
                    {
                        fev = nonfatalerror_ex(SRC_LOC(),
                                "\n Couldn't write space fill for hashes to file %s; errno=%d(%s)\n",
                                hashfilename, errno, errno_descr(errno));
                        fclose(hashf);
                        return fev;
                    }
                }

                //    ... and write a single 32-bit zero to cover index zero.
                if (1 != fwrite(&(my_zero), sizeof(FSCM_PREFIX_TABLE_CELL), 1, hashf))
                {
                    fev = nonfatalerror_ex(SRC_LOC(),
                            "\n Couldn't write index 0 init data to file %s; errno=%d(%s)\n",
                            hashfilename, errno, errno_descr(errno));
                    fclose(hashf);
                    return fev;
                }

                //     All written; the file now exists with the right setup.
                fclose(hashf);
            }
        }

        //    We need one 32-bit zero for each character in the to-be-learned
        //    text; we'll soon clobber the ones that are in previously
        //    seen chains to chain members (the others can stay as zeroes).
        {
            FSCM_HASH_CHAIN_CELL my_zero;

            //   Now it's time to generate the actual string hashes.
            //   By default (no regex) it's a string kernel, length 3,
            //   but it can be any prefix one desires.
            //
            //   Generate the hashes.
            crm_vector_tokenize_selector
            (apb,                                             // the APB
                    vht,
                    tdw,
                    txtptr + txtstart,                                   // intput string
                    txtlen,                                              // how many bytes
                    0,                                                   // starting offset
                    NULL,                                                // custom tokenizer
                    NULL,                                                // custom matrix
                    unk_hashes,                                          // where to put the hashed results
                    data_window_size / sizeof(unk_hashes[0]),            //  max number of hashes
                    &unk_hashcount);                                     // how many hashes we actually got

            if (internal_trace)
            {
                int display_count = CRM_MIN(16, unk_hashcount);
                fprintf(stderr, "Total %d hashes - first %d values:", unk_hashcount, display_count);
                for (i = 0; i < display_count; i++)
                {
                    if (i % 8 == 0)
                    {
                        fprintf(stderr, "\n#%d..%d:", i, CRM_MIN(i + 7, display_count - 1));
                    }
                    fprintf(stderr, " %08lx", (unsigned long int)unk_hashes[i]);
                }
                fprintf(stderr, "\n");
            }

            //  Now a nasty bit.  Because there might be retained hashes of the
            //  file, we need to force an unmap-by-name which will allow a remap
            //  with the new file length later on.
            if (internal_trace)
                fprintf(stderr,
                        "mmapping file %s for known state\n", hashfilename);
#if 0 // not needed for force_unmap to work as expected.
            crm_mmap_file
            (hashfilename, 0, 1, PROT_READ | PROT_WRITE,
                    MAP_SHARED, CRM_MADV_RANDOM, NULL);
#endif
            crm_force_munmap_filename(hashfilename);
            if (internal_trace)
                fprintf(stderr,
                        "UNmmapped file %s for known state\n", hashfilename);
            if (user_trace)
                fprintf(stderr, "Opening FSCM file %s for append.\n",
                        hashfilename);
            hashf = fopen(hashfilename, "ab+");
            if (!hashf)
            {
                fev = nonfatalerror_ex(SRC_LOC(),
                        "\n Couldn't open your CSS file %s for appending; errno=%d(%s)\n",
                        hashfilename,
                        errno,
                        errno_descr(errno));
                return fev;
            }
            else
            {
                if (user_trace)
                    fprintf(stderr, "Writing to hash file %s\n", hashfilename);
                my_zero.next = 0;
                //     Note the "+ 2" here - to put in a sentinel in the output file
                for (i = 0; i < unk_hashcount + 2; i++)
                {
                    if (1 != fwrite(&my_zero,
                                sizeof(FSCM_HASH_CHAIN_CELL),
                                1,
                                hashf))
                    {
                        fev = nonfatalerror_ex(SRC_LOC(),
                                "\n Couldn't write hash sequence to file %s; errno=%d(%s)\n",
                                hashfilename, errno, errno_descr(errno));
                        fclose(hashf);
                        return fev;
                    }
                }
                fclose(hashf);
            }
        }

        //    Now the file has the space; we can now mmap it and set up our
        //    working pointers.

        stat(hashfilename, &statbuf);
        if (internal_trace)
            fprintf(stderr, "mmapping2 file %s\n", hashfilename);
        file_pointer =
            crm_mmap_file(hashfilename,
                    0, statbuf.st_size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, CRM_MADV_RANDOM, NULL);
        if (internal_trace)
            fprintf(stderr, "mmapped2 file %s\n", hashfilename);

        //  set up our pointers for the prefix table and the chains
        file_header = (STATISTICS_FILE_HEADER_STRUCT *)file_pointer;
        prefix_table_size = file_header->data[0];
        prefix_table = (FSCM_PREFIX_TABLE_CELL *)
                       &file_pointer[file_header->chunks[3].start];
        chains = (FSCM_HASH_CHAIN_CELL *)
                 &file_pointer[file_header->chunks[4].start];

        //    Note the little two-step dance to recover the starting location
        //    of this next chain.
        //
        //      Our new start here !
        newchainstart =
            (file_header->chunks[4].start
             + file_header->chunks[4].length)
            / sizeof(FSCM_HASH_CHAIN_CELL);
        if (internal_trace)
            fprintf(stderr,
                    "Chain field: %u (entries %u) new chainstart offset %u\n",
                    (unsigned int)(file_header->chunks[4].start
                                   / sizeof(FSCM_HASH_CHAIN_CELL)),
                    (unsigned int)(file_header->chunks[4].length
                                   / sizeof(FSCM_HASH_CHAIN_CELL)),
                    newchainstart);

        newchains = (FSCM_HASH_CHAIN_CELL *)
                    &chains[newchainstart];

        //      ... and this is the new updated length.
        file_header->chunks[4].length += (unk_hashcount + 2)
                                         * sizeof(FSCM_HASH_CHAIN_CELL);


        //   For each hash, insert it into the prefix table
        //   at the right place (that is, at hash mod prefix_table_size).
        //   If the table had a zero, it becomes nonzero.  If the table
        //   is nonzero, the corresponding slot in the newly written
        //   part of the chain zone gets the old address and the hashtable
        //   gets the new address.


        if (internal_trace)
        {
            fprintf(stderr,
                    "\n\nPrefix table size: %u, starting at offset %u\n",
                    prefix_table_size, newchainstart);
        }
        for (i = 0; i < unk_hashcount; i++)
        {
            unsigned int pti, cind;
            pti = unk_hashes[i] % prefix_table_size;
            //   Put in our chain entry
            if (internal_trace)
                fprintf(stderr,
                        "offset %d icn: %u hash %08lx tableslot %u"
                        " (prev offset %u)\n",
                        i,
                        i + newchainstart,
                        (unsigned long int)unk_hashes[i],
                        pti,
                        prefix_table[pti].index
                       );

            if (internal_trace)
            {
                cind = prefix_table[pti].index;
                while (cind != 0)
                {
                    fprintf(stderr,
                            " ... now location %u forwards to %u \n",
                            cind, chains[cind].next);
                    cind = chains[cind].next;
                }
            }
            //  Desired State:
            // chains [old] points to chains [older]chains (which is prior state)
            // chains[new] points to chains[old]
            // prefix_table [pti] = chains [new]
            chains[i + newchainstart].next = prefix_table[pti].index;
            prefix_table[pti].index = i + newchainstart;
        }

        //  forcibly let go of the mmap to force an msync
        if (internal_trace)
            fprintf(stderr, "UNmmapping file %s\n", hashfilename);
        crm_force_munmap_filename(hashfilename);
    }
    return 0;
}

//     A helper (but crucial) function - given an array of hashes and,
//     and a prefix_table / chain array pair, calculate the compressibility
//     of the hashes; this is munged (reports say best accuracy if
//     raised to the 1.5 power) and that is the returned value.
//
//
//   The algorithm here is actually suboptimal.
//   We take the first match presented (i.e. where unk_hashes[i] maps
//   to a nonzero cell in the prefix table) then for each additional
//   hash (i.e. unk_hashes[i+j] where there is a consecutive chain
//   in the chains[q, q+1, q+2]; we sum the J raised to the q_exponent
//   power for each such chain and report that result back.
//
//   The trick we employ here is that for each starting position q
//   all possible solutions are on q's chain, but also on q+1's
//   chain, on q+2's chain, on q+3's chain, and so on.
//
//   At this point, we can go two ways: we can use a single index (q)
//   chain and search forward through the entire chain, or we can use
//   multiple indices and an n-way merge of n chains to cut the number of
//   comparisons down significantly.
//
//   Which is optimal here?  Let's assume the texts obey something like
//   Zipf's law (Nth term is 1/Nth as likely as the 1st term).  Then the
//   probabile number of comparisons to find a string of length Q in
//   a text of length |T| by using the first method is
//          (1/N) + ( 1/ N) + ... = Q * (1/N) and we
//   can stop as soon as we find a string of Q elements (but since we
//   want the longest one, we check all |T| / N occurrences and that takes
//   |T| * Q / N^2 comparisons, and we need roughly |U| comparisons
//   overall, it's |T| * |U| * Q / N^2 .
//
//   In the second method (find all chains of length Q or longer) we
//   touch each of the Q chain members once. The number of members of
//   each chain is roughly |T| / N and we need Q such chains, so the
//   time is |T| * Q / N.  However, at the next position
//   we can simply drop the first chain's constraint; all of the other
//   calculations have already been done once; essentially this search
//   can be carried out *in parallel*; this cuts the work by a factor of
//   the length of the unkonown string.   However, dropping the constraint
//   is very tricky programming and so we aren't doing that right now.
//
//   We might form the sets where chain 1 and chain 2 are sequential in the
//   memory.  We then find where chains 2 and 3 are sequential in the
//   memory; where chains 3 and 4 are sequential, etc.  This is essentially
//   a relational database "join" operation, but with "in same record"
//   being replaced with "in sequential slots".
//
//   Assume we have a vector "V" of indices carrying each chain's current
//   putative pointer for a sequential match. (assume the V vector is
//   as long as the input string).
//
// 0) We initialize the indexing vector "V" to the first element of each
//    chan (or NULL for never-seen chains), and "start", "end", and
//    "total" to zero.
// 1) We set the chain index-index "C" to 0 (the leftmost member
//    of the index vector V).
// 2.0) We do a two-finger merge between the C'th and C + 1 chains,
//    moving one chain link further on the lesser index in each cycle.
//    (NB the current build method causes link indicess to be descending in
//    numerical value; so we step to the next link on the _greater_ of the
//    two chains.
//    2a) we stop the two-finger merge when:
//         V[C] == V[C+1] + 1
//         in which case
//              >> C++,
//              >> if C > end then end = C
//              >> go to 2.0 (repeat the 2-finger merge);
//    2b) if the two-finger merge runs out of chain on either the
//        C chain or the C++ chain (that is, a NULL):
//        >>  set the "out of chain" V element back to the innitial state;
//        >>  go back one chain pair ( "C = C--")
//            If V[C] == NULL
//              >> report out (end-start) as a maximal match (incrementing
//                 total by some amount),
//              >> move C over to "end" in the input stream
//              >> reset V[end+1]back to the chain starts.  Anything further
//                 hasn't been touched and so can be left alone.
//              >> go to 2.0
//
//    This algorithm still has the flaw thatn for the input string
//    ABCDE, the subchain BCDE is searched when the input string is at A.
//    and then again at B.  However, any local matches BC... are
//    gauranteed to be captured in the AB... match (we would look at
//    only the B's that follwed A's, not all of the B's even, so perhaps
//    this isn't much extra work after all.
//
//     Note: the reason for passing array of hashes rather than the original
//     string is that the calculation of the hashes is necessary and it's
//     more efficient to do it once and reuse.  Also, it means that the
//     hashes can be computed with a non-orthodox (i.e. not a string kernel)
//     method and that might take serious computes and many regexecs.

////////////////////////////////////////////////////////////////////////
//
//      Helper functions for the fast match.
//
////////////////////////////////////////////////////////////////////////

//     given a starting point, does it exist on a chain?
static unsigned int chain_search_one_chain
(
        FSCM_HASH_CHAIN_CELL *chains,
        unsigned int          chain_start,
        unsigned int          must_match
)
{
    unsigned int retval;
    unsigned int curch;
    static unsigned int cached_chain_start = 0;
    static unsigned int cached_chain_must_match = 0;
    static unsigned int cached_chain_prior = 0;

    retval = 0;
    curch = chain_start;

    if (internal_trace)
    {
        unsigned int j;
        j = chain_start;
        fprintf(stderr, "...chaintrace from %u: (next: %u)",
                j, chains[j].next);
        while (j != 0)
        {
            fprintf(stderr, " %u", j);
            j = chains[j].next;
        }
        fprintf(stderr, "\n");
    }

    if (internal_trace)
        fprintf(stderr, "   ... chain_search_one_chain chain %u mustmatch %u\n",
                chain_start, must_match);

    //  See if the caches can help us
    if (cached_chain_start == chain_start
        && cached_chain_must_match >= must_match)
    {
        if (internal_trace)
            fprintf(stderr, "   (using cache) ");
        curch = cached_chain_prior;
    }


    //  reset the cache either way.
    cached_chain_must_match = must_match;
    cached_chain_start = chain_start;

    while (curch > must_match && curch > 0)
    {
        if (internal_trace)
            fprintf(stderr, "   .... from %u to %u\n",
                    curch, chains[curch].next);
        cached_chain_prior = curch;
        curch = chains[curch].next;
    }
    if (curch == must_match)
    {
        if (internal_trace)
            fprintf(stderr, "   ...Success at chainindex %u\n", curch);
        return curch;
    }

    if (internal_trace)
        fprintf(stderr, "   ...Failed\n");
    return 0;
}


//    From here, how long does this chain run?
//
//      Do NOT implement this recursively, as a document matched against
//       itself will recurse for each character, so unless your compiler
//        can fix tail recursion, you'll blow the stack on long documents.
//
static unsigned int chain_run_length
(
        FSCM_HASH_CHAIN_CELL *chains,              //  the known-text chains
        unsigned int         *unk_indexes,         //  index vector to head of each chain
        unsigned int          unk_len,             //  length of index vctor
        unsigned int          starting_symbol,     //  symbol where we start
        unsigned int          starting_chain_index //  where it has to match (in chainspace)
)
{
    unsigned int offset;
    unsigned int search_chain;
    int in_loop;

    if (internal_trace)
        fprintf(stderr,
                "Looking for a chain member at symbol %u chainindex %u\n",
                starting_symbol, starting_chain_index);
    in_loop = 1;
    offset = 0;
    while ((starting_symbol + offset < unk_len) && in_loop)
    {
        search_chain = unk_indexes[starting_symbol + offset];
        if (internal_trace)
            fprintf(stderr,
                    "..searching at [symbol %u offset %u] chainindex %u\n",
                    starting_symbol, offset, search_chain);
        in_loop = chain_search_one_chain
                  (chains, search_chain, starting_chain_index + offset);
        offset++;
    }
    offset--;
    if (internal_trace)
        fprintf(stderr,
                "chain_run_length finished at chain index %u (offset %u)\n",
                starting_chain_index + offset, offset);
    return offset;
}

//        Note- the two-finger algorithm works- but it's actually kind of
//        hard to program in terms of it's asymmetry.  So instead, we use a
//        simpler repeated search algorithm with a cache at the bottom
//        level so we don't repeatedly search the same (failing) elements
//        of the chain).
//
//
//      NB: if this looks a little like how the genomics BLAST
//      match algorithm runs, yeah... I get that feeling too, although
//      I have not yet found a good description of how BLAST actually works
//      inside, and so can't say if this would be an improvement.  However,
//      it does beg the question of whether a BLAST-like algorithm might
//      work even _better_ for text matching.  Future note: use additional
//      flag <blast> to allow short interruptions of match stream.
//
//     longest_run_starting_here returns the length of the longest match
//      found that starts at exactly index[starting_symbol]
//
static unsigned int longest_run_starting_here
(
        FSCM_HASH_CHAIN_CELL *chains,         // array of interlaced chain cells
        unsigned int         *unk_indexes,    //  index vector to head of each chain
        unsigned int          unk_len,        //  length of index vector
        unsigned int          starting_symbol //  index of starting symbol
)
{
    unsigned int chain_index_start;    //  Where in the primary chain we are.
    unsigned int this_run, max_run;

    if (internal_trace)
        fprintf(stderr, "\n*** longest_run: starting at symbol %u\n",
                starting_symbol);

    chain_index_start = unk_indexes[starting_symbol];
    this_run = max_run = 0;
    if (chain_index_start == 0)
    {
        if (internal_trace)
            fprintf(stderr, "longest_run: no starting chain here; returning\n");
        return 0;    // edge case - no match
    }
    //   If we got here, we had at +least+ a max run of one match found
    //    (that being chain_index_start)
    this_run = max_run = 1;
    if (internal_trace)
        fprintf(stderr, "longest_run: found a first entry (chain %u)\n",
                chain_index_start);

    while (chain_index_start != 0)
    {
        if (internal_trace)
            fprintf(stderr, "Scanning chain starting at %u\n",
                    chain_index_start);
        this_run = chain_run_length
                   (chains, unk_indexes, unk_len,
                starting_symbol + 1, chain_index_start + 1) + 1;
        //
        if (internal_trace)
            fprintf(stderr,
                    "longest_run: chainindex run at %u is length %u\n",
                    chain_index_start, this_run);
        if (this_run > max_run)
        {
            if (internal_trace)
                fprintf(stderr, "longest_run: new maximum\n");
            max_run = this_run;
        }
        else
        {
            if (internal_trace)
                fprintf(stderr, "longest_run: not an improvement\n");
        }
        //     And go around again till we hit a zero chain index
        chain_index_start = chains[chain_index_start].next;
    }
    if (internal_trace)
        fprintf(stderr, "Final result at symbol %u run length is %u\n",
                starting_symbol, max_run);
    return max_run;
}

//     compress_me is the top-level calculating routine which calls
//     all of the prior routines in the right way.

static double compress_me
(
        unsigned int         *unk_indexes, //  prefix chain-entry table
        unsigned int          unk_len,     // length of the entry table
        FSCM_HASH_CHAIN_CELL *chains,      // array of interlaced chain cells
        double                q_exponent   // exponent of match
)
{
    unsigned int current_symbol, this_run_length;
    double total_score, incr_score;

    int blast_lookback; // Only use if BLAST is desired.

    total_score = 0.0;
    current_symbol = 0;
    blast_lookback = 0;

    while (current_symbol < unk_len)
    {
        this_run_length = longest_run_starting_here
                          (chains, unk_indexes, unk_len, current_symbol);
        incr_score = 0;
        if (this_run_length > 1)
        {
            //this_run_length += blast_lookback;
            incr_score = pow(this_run_length, q_exponent);
            //blast_lookback = this_run_length;
        }
        //blast_lookback --;
        //if (blast_lookback < 0) blast_lookback = 0;
        //if (this_run_length > 2)
        //	fprintf (stderr, " %ld", this_run_length);
        //else
        //	fprintf (stderr, "_");
        total_score = total_score + incr_score;
        if (internal_trace)
            fprintf(stderr,  "Offset %u compresses %u score %lf\n",
                    current_symbol, this_run_length, incr_score);
        current_symbol = current_symbol + this_run_length;
        if (this_run_length == 0)
            current_symbol++;
    }
    return total_score;
}


//      How to do an Improved FSCM CLASSIFY of some text.
//
int crm_fast_substring_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //      classify the compressed version of this text
    //      as belonging to a particular type.
    //
    //       Much of this code should look very familiar- it's cribbed from
    //       the code for LEARN
    //
    int i, k;

    char ptext[MAX_PATTERN]; //  the regex pattern
    int plen;

    //  the hash file names
    int htext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN;

    //  the match statistics variable
    char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
    int stext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);

    int slen;
    char svrbl[MAX_PATTERN]; //  the match statistics text buffer
    int svlen;
    int fnameoffset;
    int eflags;
    int cflags;
    int use_unique;
    int not_microgroom = 1;
    int use_unigram_features;

    struct stat statbuf;    //  for statting the hash file
    regex_t regcb;

    //   Total hits per statistics file - one hit is nominally equivalent to
    //   compressing away one byte
    //  int totalhits[MAX_CLASSIFIERS];
    //
    //  int totalfeatures;   //  total features
    double tprob;       //  total probability in the "success" domain.

    double ptc[MAX_CLASSIFIERS]; // current running probability of this class

    //    Classificer Coding Clarification- we'll do one file at a time, so
    //    these variables are moved to point to different statistics files
    //    in a loop.
    char *file_pointer;
    STATISTICS_FILE_HEADER_STRUCT *file_header; // the
    FSCM_PREFIX_TABLE_CELL *prefix_table;       //  the prefix indexing table,
    unsigned int prefix_table_size = 200000;
    FSCM_HASH_CHAIN_CELL *chains; //  the chain area
    unsigned int *unk_indexes;

    int fn_start_here;
    char htext[MAX_PATTERN];    // the text of the names (unparsed)
    int htextlen;
    char hfname[MAX_PATTERN];   //  the current file name
    int fnstart, fnlen;
    char hashfilenames[MAX_CLASSIFIERS][MAX_FILE_NAME_LEN]; //  names (parsed)
    int hashfilebytelens[MAX_CLASSIFIERS];
    int hashfilechainentries[MAX_CLASSIFIERS];
    int succhash;       // how many hashfilenames are "success" files?
    int vbar_seen;      // did we see '|' in classify's args?
    int maxhash;
    int bestseen;
    double scores[MAX_CLASSIFIERS]; //  per-classifier raw score.

    // We'll generate our unknown string's hashes directly into tempbuf.
    int unk_hashcount;
    crmhash_t *unk_hashes;

    unk_hashes = (crmhash_t *)tempbuf;

    if (internal_trace)
        fprintf(stderr, "executing a Fast Substring Compression CLASSIFY\n");


    //           extract the hash file names
    htextlen = crm_get_pgm_arg(htext, htext_maxlen, apb->p1start, apb->p1len);
    htextlen = crm_nexpandvar(htext, htextlen, htext_maxlen, vht, tdw);
    CRM_ASSERT(htextlen < MAX_PATTERN);

    //           extract the "this is a compressible character" regex.
    //     Note that by and large this is not used!
    //
    plen = crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    plen = crm_nexpandvar(ptext, plen, MAX_PATTERN, vht, tdw);

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

    if (user_trace)
        fprintf(stderr, "Status out var %s (len %d)\n",
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
        cflags = REG_ICASE | cflags;
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
            fprintf(stderr, " unique engaged - repeated features are ignored\n");
    }

    use_unigram_features = 0;
    if (apb->sflags & CRM_UNIGRAM)
    {
        use_unigram_features = 1;
        if (user_trace)
            fprintf(stderr, " using only unigram features.\n");
    }


    //      Create our hashes; we do this once outside the loop and
    //      thus save time inside the loop.



    crm_vector_tokenize_selector
    (apb,                                             // the APB
            vht,
            tdw,
            txtptr + txtstart,                                   // intput string
            txtlen,                                              // how many bytes
            0,                                                   // starting offset
            NULL,                                                // custom tokenizer
            NULL,                                                // custom coeff matrix
            unk_hashes,                                          // where to put the hashed results
            data_window_size / sizeof(unk_hashes[0]),            //  max number of hashes
            &unk_hashcount);                                     // how many hashes we actually got

    if (internal_trace)
    {
        int display_count = CRM_MIN(16, unk_hashcount);
        fprintf(stderr, "Total %d hashes - first %d values:", unk_hashcount, display_count);
        for (i = 0; i < display_count; i++)
        {
            if (i % 8 == 0)
            {
                fprintf(stderr, "\n#%d..%d:", i, CRM_MIN(i + 7, display_count - 1));
            }
            fprintf(stderr, " %08lx", (unsigned long int)unk_hashes[i]);
        }
        fprintf(stderr, "\n");
    }


    if (user_trace)
        fprintf(stderr, "Total of %u initial features.\n", unk_hashcount);

    unk_indexes = (unsigned int *)calloc(unk_hashcount + 1, sizeof(unk_indexes[0]));

    //       Now, we parse the filenames and do a mmap/match/munmap loop
    //       on each file.  The resulting number of hits is stored in the
    //       the loop to open the files.

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
        fprintf(stderr, "Classify list: -%s- \n", htext);
    fn_start_here = 0;
    fnlen = 1;

    while (fnlen > 0 && ((maxhash < MAX_CLASSIFIERS - 1)))
    {
        if (crm_nextword(htext, htextlen, fn_start_here, &fnstart, &fnlen)
            && fnlen > 0)
        {
            strncpy(hfname, &htext[fnstart], fnlen);
            fn_start_here = fnstart + fnlen + 1;
            hfname[fnlen] = 0;

            strncpy(hashfilenames[maxhash], hfname, fnlen);
            hashfilenames[maxhash][fnlen] = '\000';
            if (user_trace)
                fprintf(stderr,
                        "Classifying with file -%s- succhash=%d, maxhash=%d\n",
                        hashfilenames[maxhash], succhash, maxhash);
            if (hfname[0] == '|' && hfname[1] == '\000')
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
                }
                vbar_seen++;
            }
            else
            {
                //  be sure the file exists
                //             stat the file to get it's length
                k = stat(hfname, &statbuf);
                //             quick check- does the file even exist?
                if (k != 0)
                {
                    nonfatalerror
                    ("Nonexistent Classify table named: ",
                            hfname);
                }
                else
                {
                    //  file exists - do the open/process/close
                    //
                    hashfilebytelens[maxhash] = statbuf.st_size;

                    //  mmap the hash file into memory so we can bitwhack it
                    file_pointer =
                        crm_mmap_file(hfname,
                                0, hashfilebytelens[maxhash],
                                PROT_READ,
                                MAP_SHARED, CRM_MADV_RANDOM,
                                NULL);

                    if (file_pointer == MAP_FAILED)
                    {
                        nonfatalerror
                        ("Couldn't memory-map the table file :",
                                hfname);
                    }
                    else
                    {
                        //    GROT GROT GROT
                        //    GROT  Actually implement this someday!!!
                        //     Check to see if this file is the right version
                        //    GROT GROT GROT

                        //  set up our pointers for the prefix table and
                        //   the chains
                        file_header =
                            (STATISTICS_FILE_HEADER_STRUCT *)file_pointer;
                        prefix_table_size = file_header->data[0];
                        if (internal_trace)
                            fprintf(stderr,
                                    "Prefix table at %u, chains at %u\n",
                                    (unsigned int)file_header->chunks[3].start,
                                    (unsigned int)file_header->chunks[4].start);

                        prefix_table = (FSCM_PREFIX_TABLE_CELL *)
                                       &file_pointer[file_header->chunks[3].start];

                        chains = (FSCM_HASH_CHAIN_CELL *)
                                 &file_pointer[file_header->chunks[4].start];

                        //  GROT GROT GROT  pointer arithmetic is gross!!!
                        hashfilechainentries[maxhash] =
                            file_header->chunks[4].length
                            / sizeof(FSCM_HASH_CHAIN_CELL);

                        if (internal_trace)
                            fprintf(stderr,
                                    " Prefix table size = %d\n",
                                    prefix_table_size);
                        //    initialize the index vector to the chain starts
                        //    (some of which are NULL).
                        for (i = 0; i < unk_hashcount; i++)
                        {
                            unsigned int uhmpts;
                            uhmpts = unk_hashes[i] % prefix_table_size;
                            unk_indexes[i] = prefix_table[uhmpts].index;
                            if (internal_trace)
                            {
                                fprintf(stderr,
                                        "unk_hashes[%d] = %08lx, index = %u, prefix_table[%u] = %u\n",
                                        i, (unsigned long int)unk_hashes[i], uhmpts,
                                        uhmpts, prefix_table[uhmpts].index);
                            }
                        }
                        //  Now for the nitty-gritty - run the compression
                        //   of the unknown versus tis statistics file.
                        scores[maxhash] = compress_me
                                          (unk_indexes,
                                unk_hashcount,
                                chains,
                                1.5);
                    }
                    maxhash++;
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

    //    do we have at least 1 valid .css file at both sides of '|'?
    if (!vbar_seen || succhash <= 0 || (maxhash <= succhash))
    {
        return nonfatalerror("Couldn't open at least 1 .css file per SUCC | FAIL category "
                             "for classify().\n", "Hope you know what are you doing.");
    }

    ///////////////////////////////////////////////////////////
    //
    //   To translate score (which is exponentiated compression) we
    //   just normalize to a sum of 1.000 .  Note that we start off
    //   with a minimum per-class score of "tiny" to avoid divide-by-zero
    //   problems (zero scores on everything => divide by zero)

    tprob = 0.0;
    for (i = 0; i < MAX_CLASSIFIERS; i++)
        ptc[i] = 0.0;

    for (i = 0; i < maxhash; i++)
    {
        ptc[i] = scores[i];
        if (ptc[i] < 0.001)
            ptc[i] = 0.001;
        tprob += ptc[i];
    }
    //     Renormalize probabilities
    for (i = 0; i < maxhash; i++)
    {
        ptc[i] /= tprob;
    }

    if (user_trace)
    {
        for (k = 0; k < maxhash; k++)
        {
            fprintf(stderr, "Match for file %d: compress: %f prob: %f\n",
                    k, scores[k], ptc[k]);
        }
    }

    bestseen = 0;
    for (i = 0; i < maxhash; i++)
    {
        if (ptc[i] > ptc[bestseen])
        {
            bestseen = i;
        }
    }

    //     Reset tprob to contain sum of probabilities of success classes.
    tprob = 0.0;
    for (k = 0; k < succhash; k++)
    {
        tprob += ptc[k];
    }

    if (svlen > 0)
    {
        char buf[1024];
        double accumulator;
        double remainder;
        double overall_pR;
        int m;
        buf[0] = '\000';
        accumulator = 1000 * DBL_MIN;
        for (m = 0; m < succhash; m++)
        {
            accumulator += ptc[m];
        }
        remainder = 1000 * DBL_MIN;
        for (m = succhash; m < maxhash; m++)
        {
            remainder += ptc[m];
        }

        if (internal_trace)
        {
            fprintf(stderr, "succ: %d, max: %d, acc: %lf, rem: %lf\n",
                    succhash, maxhash, accumulator, remainder);
        }

        overall_pR = 10 * (log10(accumulator) - log10(remainder));

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

        remainder = 1000 * DBL_MIN;
        for (m = 0; m < maxhash; m++)
        {
            if (bestseen != m)
            {
                remainder += ptc[m];
            }
        }

        sprintf(buf,
                "Best match to file #%d (%s) prob: %6.4f  pR: %6.4f \n",
                bestseen,
                hashfilenames[bestseen],
                ptc[bestseen],
                10 * (log10(ptc[bestseen]) - log10(remainder))
               );

        if (strlen(stext) + strlen(buf) <= stext_maxlen)
            strcat(stext, buf);
        sprintf(buf, "Total features in input file: %d\n", unk_hashcount);
        if (strlen(stext) + strlen(buf) <= stext_maxlen)
            strcat(stext, buf);
        for (k = 0; k < maxhash; k++)
        {
            // int m; -- 'local 'm' hides decl of the same name in outer scope, so says MSVC. And we don't need this one to do that, so use the outer scope one.
            remainder = 1000 * DBL_MIN;
            for (m = 0; m < maxhash; m++)
            {
                if (k != m)
                {
                    remainder += ptc[m];
                }
            }
            sprintf(buf,
                    "#%d (%s):"
                    " features: %d, chcs: %6.2f, prob: %3.2e, pR: %6.2f\n",
                    k,
                    hashfilenames[k],
                    hashfilechainentries[k],
                    scores[k],
                    ptc[k],
                    10 * (log10(ptc[k]) - log10(remainder)));

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
        crm_destructive_alter_nvariable(svrbl, svlen,
                stext, (int)strlen(stext));
    }


    //  cleanup time!

    //  and let go of the regex buffery
    if (ptext[0] != '\0')
        crm_regfree(&regcb);


    if (tprob > 0.5)
    {
        //   all done... if we got here, we should just continue execution
        if (user_trace)
            fprintf(stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
    }
    else
    {
        if (user_trace)
            fprintf(stderr, "CLASSIFY was a FAIL, skipping forward.\n");
        //    and do what we do for a FAIL here
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        return 0;
    }
    //
    // regcomp_failed:
    return 0;
}


#else /* CRM_WITHOUT_FSCM */

int crm_expr_fscm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}

#endif




int crm_expr_fscm_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}


int crm_expr_fscm_css_migrate(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
            "You may want to run 'crm -v' to see which classifiers are available.\n",
            "FSCM");
}




/*
 |
 |      to:     %c      %d/%u        %ld/%lu   %lld/%llu
 |             %02x    %04x/%08x      %08x      %016llx
 |             char       int        long      long long
 |from:
 | [u]int8_t    X         X            X           X
 | [u]int16_t   .         X            X           X
 | [u]int32_t   .         X            X           X
 | [u]int64_t   .         .            .           X
 |
 */



