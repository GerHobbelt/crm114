//  crm114_structs.h  - Controllable Regex Mutilator structures, version X0.1
//  Copyright 2001 William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 1.0.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org .  Other licenses may be negotiated; contact the
//  author for details.
//

#ifndef __CRM114_STRUCTS_H__
#define __CRM114_STRUCTS_H__



/* [i_a] unsure if totalhits[] and hits[] should be floating point or integer count arrays ... */
#if 10
// from OSBF Hayes: hmmm, unsigned long gives better precision than float...
typedef int64_t hitcount_t;
#else
typedef double hitcount_t;
#endif


/* the 32 bit unsigned hash values as used by CRM114 */
#if 10
typedef uint32_t crmhash_t;
#else
typedef unsigned long crmhash_t;
#endif
typedef uint64_t crmhash64_t;


/* [i_a] no variable instantiation in a common header file */
extern long vht_size;

extern long cstk_limit;

extern long max_pgmlines;

extern long max_pgmsize;

/* extern long max_pgmsize; [i_a] */

extern long user_trace;

extern long internal_trace;

extern long debug_countdown;

extern long cmdline_break;

extern long cycle_counter;

extern long ignore_environment_vars;

extern long data_window_size;

extern long sparse_spectrum_file_length;

extern long microgroom_chain_length;

extern long microgroom_stop_after;

extern double min_pmax_pmin_ratio;

extern long profile_execution;

extern long prettyprint_listing;  //  0= none, 1 = basic, 2 = expanded, 3 = parsecode

extern long engine_exit_base;  //  All internal errors will use this number or higher;
//  the user programs can use lower numbers freely.


//        how should math be handled?
//        = 0 no extended (non-EVAL) math, use algebraic notation
//        = 1 no extended (non-EVAL) math, use RPN
//        = 2 extended (everywhere) math, use algebraic notation
//        = 3 extended (everywhere) math, use RPN
extern long q_expansion_mode;


//   structure of a vht cell
//  note - each file gets an entry, with the name of the file
//  being the name of the variable - no colons!
//
//  also note that there's no "next" pointer in a vht cell; this is because
//  we do in-table overflowing (if a table entry is in use, we use the next
//  available table entry, wrapping around.  It's easy to change in any case.
//
typedef struct mythical_vht_cell
{
    char *filename;      // file where defined (or NULL)
    int   filedesc;      // filedesc of defining file (or NULL)
    char *nametxt;       // block of text that hosts the variable name
    long  nstart;        // index into nametxt to start of varname
    long  nlen;          // length of name
    char *valtxt;        // text block that hosts the captured value
                         // vstart, vlen, mstart, and mlen are all measured
                         // from the _start_ of valtxt, mstart relative to
                         // vstart, etc!!!
    long vstart;        // zero-base index of start of variable (inclusive)
    long vlen;          // length of captured value : this plus vstart is where
                        //  you could put a NULL if you wanted to.
    long mstart;        // zero-base start of most recent match of this var
    long mlen;          // length of most recent match against this var; this
                        //   plus mstart is where you could put a NULL if you
                        //    wanted to.
    long linenumber;     // linenumber of this variable (if known, else -1)
    long lazy_redirects; // how many lazy redirects are allowed (0 by default);
} VHT_CELL;

//   The argparse block is filled in at run time, though at least in
//    principle it could be done at microcompile time, but var-expansion
//     needs to be done at statement execution time..  so we don't fill it
//      in till we have to, then we cache the result.
//


typedef struct mythical_argparse_block
{
    char      *a1start;
    long       a1len;
    char      *p1start;
    long       p1len;
    char      *p2start;
    long       p2len;
    char      *p3start;
    long       p3len;
    char      *b1start;
    long       b1len;
    char      *s1start;
    long       s1len;
    char      *s2start;
    long       s2len;
    uint64_t   sflags;
} ARGPARSE_BLOCK;



// structure of a microcompile table cell (one such per statement)
//
//  These table entries get filled in during microcompile operation.
//
typedef struct mythical_mct_cell
{
    char           *hosttxt;    // text file this statement lives in.
    ARGPARSE_BLOCK *apb;        // the argparse block for this statement
    long            start;      // zero-base index of start of statement (inclusive)
    long            fchar;      // zero-base index of non-blank stmt (for prettyprint)
    long            achar;      // zero-base index of start of args;
    long            stmt_utime; // user time spent in this statement line;
    long            stmt_stime; // system time spent in this statement line;
    int             stmt_type;  // statement type of this line
    int             nest_level; // nesting level of this statement
    int             fail_index; // if this statement failed, where would we go?
    int             liaf_index; // if this statement liafed, where would we go?
    int             trap_index; // if this statement faults, where would we go?
    int             stmt_break; // 1 if "break" on this stmt, 0 otherwise.
} MCT_CELL;

// structure of a control stack level cell.
//   Nota Bene:  CSL cells are used to both retain toplevel data about
//   any particular file being executed as well as being used to retain
//   data on any file that is data!  If a file is executable, then the
//   mct pointer is a pointer to the compiled MCT table, else the mct
//   pointer is a NULL and the file is not executable.
//
struct mythical_csl_cell;

typedef struct mythical_csl_cell
{
    char           *filename;                                //filename if any
    long            rdwr;                                    // 0=readonly, 1=rdwr
    long            filedes;                                 //  file descriptor it's open on (if any)
    char           *filetext;                                //  text buffer
    long            nchars;                                  //  characters of data we have
    crmhash_t       hash;                                    //  hash of this data (if done)
    MCT_CELL      **mct;                                     //  microcompile (if compiled)
    long            mct_size;                                // number of slots available in the MCT
    long            nstmts;                                  //  how many statements in the microcompile
    long            preload_window;                          //  do we preload the window or not?
    long            cstmt;                                   //  current executing statement of this file
    struct mythical_csl_cell *caller;                        //  pointer to this file's caller (if any)
    long return_vht_cell;                                    //  index into the VHT to stick the return value
    long calldepth;                                          //  how many calls deep is this stack frame
    long aliusstk[MAX_BRACKETDEPTH];                         // the status stack for ALIUS

    unsigned int filename_allocated : 1; // if the filename was allocated on the heap.
    unsigned int filetext_allocated : 1; // if the filetext was allocated on the heap.
    unsigned int mct_allocated      : 1; // if the mct collection was allocated on the heap.
} CSL_CELL;

typedef struct
{
    crmhash_t     hash;
    crmhash_t     key;
    unsigned long value;
} FEATUREBUCKET_STRUCT;


typedef struct
{
    unsigned char version[4];
    unsigned long flags;
    unsigned long skip_to;
} FEATURE_HEADER_STRUCT;


typedef struct
{
    crmhash_t hash;
    crmhash_t key;
#if defined (GER)
    double value;
#else
    float value;
#endif
} WINNOW_FEATUREBUCKET_STRUCT;

#define ENTROPY_RESERVED_HEADER_LEN 1024
typedef struct
{
    int32_t firlatstart;
    int32_t firlatlen;
    int32_t nodestart;
    int32_t nodeslen;
    int64_t totalbits;
} ENTROPY_HEADER_STRUCT;

typedef struct mythical_entropy_alphabet_slot
{
    int32_t count;
    int32_t nextcell;
} ENTROPY_ALPHABET_SLOT;

//  28 byte header, 24 bytes alph (52 tot).  Pare: 16 header, 16 alph (36 tot)
typedef struct mythical_entropy_cell
{
    double  fir_prior;
    int32_t fir_larger;
    int32_t fir_smaller;
    int32_t firlat_slot;
    //  long total_count;
    ENTROPY_ALPHABET_SLOT abet[ENTROPY_ALPHABET_SIZE];
} ENTROPY_FEATUREBUCKET_STRUCT;




//  define statement types for microcompile
//
#define CRM_BOGUS 0
#define CRM_NOOP 1
#define CRM_EXIT 2
#define CRM_OPENBRACKET 3
#define CRM_CLOSEBRACKET 4
#define CRM_LABEL 5
#define CRM_GOTO 6
#define CRM_MATCH 7
#define CRM_FAIL 8
#define CRM_LIAF 9
#define CRM_ACCEPT 10
#define CRM_TRAP 11
#define CRM_FAULT 12
#define CRM_INPUT 13
#define CRM_OUTPUT 14
#define CRM_WINDOW 15
#define CRM_ALTER 16
#define CRM_CALL 17
#define CRM_ROUTINE 18  // [i_a] NOT IMPLEMENTED
#define CRM_RETURN 19
#define CRM_SYSCALL 20
#define CRM_LEARN 21
#define CRM_CLASSIFY 22
#define CRM_ISOLATE 23
#define CRM_HASH 24
#define CRM_INTERSECT 25
#define CRM_UNION 26
#define CRM_EVAL 27
#define CRM_ALIUS 28
#define CRM_TRANSLATE 29
#define CRM_DEBUG 30
#define CRM_CLUMP 31         // make clusters out of tokens
#define CRM_PMULC 32         // pmulc translates tokens to cluster names
#define CRM_LAZY 33          // makes a "lazy" variable.  // [i_a] NOT IMPLEMENTED
#define CRM_REDUCE 34
#define CRM_UNIMPLEMENTED 35


//      FLAGS FLAGS FLAGS
//       all of the valid CRM114 flags are listed here
//
//      GROT GROT GROT - You must keep this in synchrony with the
//      definitions of the keywords in crm_stmt_parser!!!  Yes, I'd
//      love to define it in one place and one place only, but I haven't
//      figured out a way to do that well.

//     match searchstart flags
#define CRM_FROMSTART     (1 << 0)
#define CRM_FROMNEXT      (1 << 1)
#define CRM_FROMEND       (1 << 2)
#define CRM_NEWEND        (1 << 3)
#define CRM_FROMCURRENT   (1 << 4)
//         match control flags
#define CRM_NOCASE        (1 << 5)
#define CRM_ABSENT        (1 << 6)
#define CRM_BASIC         (1 << 7)
#define CRM_BACKWARDS     (1 << 8)
#define CRM_LITERAL       (1 << 9)
#define CRM_NOMULTILINE   (1 << 10)      // should be merged with byline
//         input/output/window flags
#define CRM_BYLINE        (1 << 10)      //  Should be merged with nomultiline
#define CRM_BYCHAR        (1 << 11)
#define CRM_STRING        CRM_BYCHAR     // string is bychar.  I think...
#define CRM_BYCHUNK       (1 << 12)
#define CRM_BYEOF         (1 << 13)
#define CRM_EOFACCEPTS    (1 << 14)
#define CRM_EOFRETRY      (1 << 15)
#define CRM_APPEND        (1 << 16)
//           process control flags
#define CRM_KEEP          (1 << 17)
#define CRM_ASYNC         (1 << 18)
//        learn and classify
#define CRM_REFUTE        (1 << 19)
#define CRM_MICROGROOM    (1 << 20)
#define CRM_MARKOVIAN     (1 << 21)
#define CRM_OSB_BAYES     (1 << 22)       // synonym with OSB feature gen
#define CRM_OSB           CRM_OSB_BAYES
#define CRM_CORRELATE     (1 << 23)
#define CRM_OSB_WINNOW    (1 << 24)      //  synonym to Winnow feature combiner
#define CRM_WINNOW        CRM_OSB_WINNOW
#define CRM_CHI2          (1 << 25)
#define CRM_UNIQUE        (1 << 26)
#define CRM_ENTROPY       (1 << 27)
#define CRM_OSBF          (1 << 28)     // synonym with OSBF local rule
#define CRM_OSBF_BAYES    CRM_OSBF
#define CRM_HYPERSPACE    (1 << 29)
#define CRM_UNIGRAM       (1 << 30)
#define CRM_CROSSLINK     (1LL << 31)
//
//        Flags that need to be sorted back in
//           input
#define CRM_READLINE      (1LL << 32)
//           isolate flags
#define CRM_DEFAULT       (1LL << 33)
//           SKS classifier
#define CRM_SKS           (1LL << 34)
//           SVM classifier
#define CRM_SVM           (1LL << 35)
//           FSCM classifier
#define CRM_FSCM          (1LL << 36)
//           SCM classifier
#define CRM_SCM           (1LL << 37)
//           Neural Net classifier
#define CRM_NEURAL_NET    (1LL << 38)

#define CRM_FLAT          (1LL << 39)


//
//     and a struct to put them in.
typedef struct
{
    char     *string;
    uint64_t  value;
} FLAG_DEF;


//*****************************************************************
//
//     The following table describes the statements allowed in CRM114.
//
//      Each entry is one line of STMT_TABLE_TYPE, and gives the text
//      representation of the command, the internal dispatch code,
//      whether the statement is "executable" or not, what the minimum
//      and maximum number of slash-groups, paren-groups, and box-groups
//      are for the statement to make sense, and what flags are allowed
//      for that statement.
//

typedef struct
{
    char    *stmt_name;
    int      stmt_code;
    int      namelen;
    int      is_executable;
    int      minslashes;
    int      maxslashes;
    int      minparens;
    int      maxparens;
    int      minboxes;
    int      maxboxes;
    uint64_t flags_allowed_mask;
} STMT_TABLE_TYPE;




//   these defines are for arg type... note that they must remain synched
//   IN THIS ORDER with the start chars and end chars in crm_statement_parse
//
#define CRM_ANGLES 0
#define CRM_PARENS 1
#define CRM_BOXES  2
#define CRM_SLASHES 3
// additional #define's in this range which are used internally by crm_generic_parse_line()
// NOTE: these _must_ be negative numbers!
#define CRM_FIND_ACTION      (-1)
#define CRM_PARSE_ACTION     (-2)
#define CRM_PARSE_LABEL      (-3)
#define CRM_FIND_ARG_SECTION (-4)



//   The possible exit codes
#define CRM_EXIT_OK 0
#define CRM_EXIT_ERROR 1
#define CRM_EXIT_FATAL 2
#define CRM_EXIT_APOCALYPSE 666


//   The ORable exec codes for crm_zexpandvar; OR together the ones
//   you want to enable for zexpandvar.  Nexpandvar is ansi|stringvar|redirect,
//   and qexpandvar is "all of them".  :)
#define CRM_EVAL_ANSI               0x01
#define CRM_EVAL_STRINGVAR          0x02
#define CRM_EVAL_REDIRECT           0x04
#define CRM_EVAL_STRINGLEN          0x08
#define CRM_EVAL_MATH               0x10



//    The possible cache actions
#define CRM_MMAP_CACHE_UNUSED 0
//   active makes it really mapped (or reactivates a released mmap)
#define CRM_MMAP_CACHE_ACTIVE 1
//   release marks the slot reusable, but doesn't unmap (yet)
#define CRM_MMAP_CACHE_RELEASE 2
//   drop really unmaps
#define CRM_MMAP_CACHE_DROP 3






/*
 * CRM114 version/portability header support
 */


#define CRM_PORTABILITY_HEADER_SEQUENCE               "#!\n#CRM114:"
#define CRM_PORTABILITY_HEADER_SEQUENCE_LENGTH        (WIDTHOF(CRM_PORTABILITY_HEADER_SEQUENCE) - 1)
#define CRM_PORTABILITY_HEADER_TEXT_SENTINEL          "\x1A\x04\x05\0x0C\0x1C"
#define CRM_PORTABILITY_HEADER_TEXT_SENTINEL_LENGTH   (WIDTHOF(CRM_PORTABILITY_HEADER_TEXT_SENTINEL) - 1)


typedef struct crm_porta_header_info
{
    int64_t classifier_bits;
    union
    {
        char args[512];
        union
        {
            int32_t i32;
            int64_t i64;
            float f;
            double d;
        } params[16];             /* assuming 16-byte alignment as worst case, this'll fit nicely in 512 bytes */
    } v;
} CRM_PORTA_HEADER_INFO;






/*
 * See the documentation in CRM_VERSIONING.C for these:
 */


typedef struct
{
    unsigned char marker[16];      /* the HEX(FF) byte + 15 bytes of NUL filler */
    int32_t       i32;
} i32_align_test_t;

typedef struct
{
    unsigned char marker[16];      /* the HEX(FF) byte + 15 bytes of NUL filler */
    int64_t       i64;
} i64_align_test_t;

typedef struct
{
    unsigned char marker[16];      /* the HEX(FF) byte + 15 bytes of NUL filler */
    float         f;
} float_align_test_t;

typedef struct
{
    unsigned char marker[16];      /* the HEX(FF) byte + 15 bytes of NUL filler */
    double        d;
} double_align_test_t;

typedef union
{
    unsigned char       filler[32];
    i32_align_test_t    i32;
    i64_align_test_t    i64;
    float_align_test_t  f;
    double_align_test_t d;
} align_test_t;


typedef union
{
    unsigned char filler[16];
    int32_t       i32;
} i32_endian_test_t;

typedef union
{
    unsigned char filler[16];
    int64_t       i64;
} i64_endian_test_t;

typedef union
{
    unsigned char filler[16];
    float         f;
} float_endian_test_t;

typedef union
{
    unsigned char filler[16];
    double        d;
} double_endian_test_t;


typedef union
{
    unsigned char filler[16];
    int32_t       i32;
} i32_arg_t;






typedef struct
{
    //    Header (6)              The character sequence 'CRM114'
    char crm_identifier[16];
    //      M:Alignment
    //        int32_t (32)
    align_test_t i32_a;
    //        int64_t (32)
    align_test_t i64_a;
    //        float (32)
    align_test_t float_a;
    //        double (32)
    align_test_t double_a;
    //      M:Endianess
    //        int32_t (16)
    i32_endian_test_t i32_e;
    //        int64_t (16)
    i64_endian_test_t i64_e;
    //        float:value1 (16)
    float_endian_test_t float_e_v1;
    //        float:value2 (16)
    float_endian_test_t float_e_v2;
    float_endian_test_t float_e_NaN;
    //        float:filler (96)
    float_endian_test_t float_e_filler[6];
    //        double:
    double_endian_test_t double_e_v1;
    double_endian_test_t double_e_v2;
    double_endian_test_t double_e_filler[6];
    //    Header2 (6)              The character sequence 'CRM114'
    char crm_identifier_2[16];
    //    Host Type (32)
    char host_type[32];
    //    Version (48)
    char version_indentifier[48];

    //    Format Version  4
    i32_arg_t format_version;
    //    (Endianess      0

    //    INT Word Size   4                 4 --> sizeof(int)
    i32_arg_t int_size;
    //    LONG Size       4                 8 --> sizeof(long)
    i32_arg_t long_size;
    //    LONG LONG Size  4                 8 --> sizeof(long long)
    //                                      0 --> 'long long' is unknown type on this box
    i32_arg_t long_long_size;
    //    INT32 Size      4                 4 --> sizeof(int32_t)
    i32_arg_t i32_size;
    //    INT64 Size      4                 8 --> sizeof(int64_t)
    i32_arg_t i64_size;
    //    INT32 Alignment 0                 must be 'auto-detected' using the sections above)
    //    INT64 Alignment 0                 must be 'auto-detected' using the sections above)
    //    FLOAT Size      4                 4 --> sizeof(float)
    i32_arg_t float_size;
    //    DOUBLE Size     4                 8 --> sizeof(double)
    i32_arg_t double_size;
    //    (FLOAT/DOUBLE
    //     IEEE Endianess 0                 must be 'auto-detected' using the sections above)

    //    Classifier      8                 e.g. CRM_OSB_WINNOW
    //    Classifier Arguments
    //                    512               This depends on the classifier and platform: arbitrary content.
    CRM_PORTA_HEADER_INFO classifier_info;
} crm_porta_bin_header_block;



typedef struct
{
    // decoded elements from the header:
    crm_porta_bin_header_block binary_section;

    char *human_readable_message;

    int header_version;

    int text_section_size;
    int binary_section_size;
    int integer_endianess;
    int floating_point_endianess;
} CRM_DECODED_PORTA_HEADER_INFO;






#endif /* __CRM114_STRUCTS_H__ */


