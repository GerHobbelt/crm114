//  crm114_structs.h  - structures for CRM114

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

#ifndef __CRM114_STRUCTS_H__
#define __CRM114_STRUCTS_H__

//    These are systemwide globals.  Sure, they should go into a global
//     struct, but that realization only occured to me in 2008.  Sorry.

long vht_size;
long cstk_limit;
long max_pgmlines;
long max_pgmsize;
long max_pgmsize;
long user_trace;
long internal_trace;
long debug_countdown;
long cmdline_break;
long cycle_counter;
long ignore_environment_vars;
long data_window_size;

// Number of hash table buckets.  Set from command line, read (only)
// by classifier learns.
long sparse_spectrum_file_length;

long microgroom_chain_length ;
long microgroom_stop_after;

float min_pmax_pmin_ratio;
long profile_execution;

int dontcare;
void *dontcareptr;

long prettyprint_listing;  //  0= none, 1 = basic, 2 = expanded, 3 = parsecode

long engine_exit_base;  //  All internal errors will use this number or higher;
                       //  the user programs can use lower numbers freely.


//        how should math be handled?
//        = 0 no extended (non-EVAL) math, use algebraic notation
//        = 1 no extended (non-EVAL) math, use RPN
//        = 2 extended (everywhere) math, use algebraic notation
//        = 3 extended (everywhere) math, use RPN
long q_expansion_mode;


//   structure of a vht cell
//  note - each file gets an entry, with the name of the file
//  being the name of the variable - no colons!
//
//  also note that there's no "next" pointer in a vht cell; this is because
//  we do in-table overflowing (if a table entry is in use, we use the next
//  available table entry, wrapping around.  It's easy to change in any case.
//
typedef struct mythical_vht_cell {
  char *filename;       // file where defined (or NULL)
  int filedesc;         // filedesc of defining file (or NULL)
  char *nametxt;        // block of text that hosts the variable name
  long nstart;          // index into nametxt to start of varname
  long nlen;            // length of name
  char *valtxt;         // text block that hosts the captured value
                        // vstart, vlen, mstart, and mlen are all measured
                        // from the _start_ of valtxt, mstart relative to
                        // vstart, etc!!!
  long vstart;          // zero-base index of start of variable (inclusive)
  long vlen;            // length of captured value : this plus vstart is where
                        //  you could put a NULL if you wanted to.
  long mstart;          // zero-base start of most recent match of this var
  long mlen;            // length of most recent match against this var; this
                        //   plus mstart is where you could put a NULL if you
                        //    wanted to.
  long linenumber;      // linenumber of this variable (if known, else -1)
  long lazy_redirects;  // how many lazy redirects are allowed (0 by default);
} VHT_CELL;

//   The argparse block is filled in at run time, though at least in
//    principle it could be done at microcompile time, but var-expansion
//     needs to be done at statement execution time..  so we don't fill it
//      in till we have to, then we cache the result.
//


typedef struct mythical_argparse_block {
  char *a1start;
  long a1len;
  char *p1start;
  long p1len;
  char *p2start;
  long p2len;
  char *p3start;
  long p3len;
  char *b1start;
  long b1len;
  char *s1start;
  long s1len;
  char *s2start;
  long s2len;
  unsigned long long sflags;
} ARGPARSE_BLOCK;



// structure of a microcompile table cell (one such per statement)
//
//  These table entries get filled in during microcompile operation.
//
typedef struct mythical_mct_cell {
  char *hosttxt;         // text file this statement lives in.
  ARGPARSE_BLOCK *apb;   // the argparse block for this statement
  long start;            // zero-base index of start of statement (inclusive)
  long fchar;            // zero-base index of non-blank stmt (for prettyprint)
  long achar;            // zero-base index of start of args;
  long stmt_utime;       // user time spent in this statement line;
  long stmt_stime;       // system time spent in this statement line;
  int stmt_type;         // statement type of this line
  int nest_level;        // nesting level of this statement
  int fail_index;        // if this statement failed, where would we go?
  int liaf_index;        // if this statement liafed, where would we go?
  int trap_index;        // if this statement faults, where would we go?
  int stmt_break;        // 1 if "break" on this stmt, 0 otherwise.
} MCT_CELL;

// structure of a control stack level cell.
//   Nota Bene:  CSL cells are used to both retain toplevel data about
//   any particular file being executed as well as being used to retain
//   data on any file that is data!  If a file is executable, then the
//   mct pointer is a pointer to the compiled MCT table, else the mct
//   pointer is a NULL and the file is not executable.
//
typedef struct mythical_csl_cell {
  char *filename;        //filename if any
  long rdwr;             // 0=readonly, 1=rdwr
  long filedes;          //  file descriptor it's open on (if any)
  char *filetext;        //  text buffer
  long nchars;           //  characters of data we have
  unsigned int hash;     //  hash of this data (if done)
  MCT_CELL **mct;        //  microcompile (if compiled)
  long nstmts;           //  how many statements in the microcompile
  long preload_window;   //  do we preload the window or not?
  long cstmt;            //  current executing statement of this file
  void *caller;          //  pointer to this file's caller (if any)
  long return_vht_cell;  //  index into the VHT to stick the return value
  long calldepth;        //  how many calls deep is this stack frame
  long aliusstk[MAX_BRACKETDEPTH]; // the status stack for ALIUS
} CSL_CELL;

//     A 1024-byte standardized header for our statistical files (well, the
//     new standard.  Old file types don't have this.  Forward migration
//     shall take place.  :-)

typedef struct {
  uint32_t start;
  uint32_t length;
  uint32_t tag;
} STATISTICS_FILE_CHUNK;

typedef struct {
  uint8_t file_ident_string [ STATISTICS_FILE_IDENT_STRING_MAX ];
                            //  Text description of this file.  This should
                            //  always start with "CRM114 Classdata " and then
                            //  the classifier name etc.  Embed versioning
                            //  information here (and get it back with strtod)
                            //  Please pad unused space with NULLs; don't
                            //  change the length (to make file-magic easier).
                            //  This is always chunks[0].
                            //
  STATISTICS_FILE_CHUNK chunks [ STATISTICS_FILE_NCHUNKS ];
                            //  The byte indexed chunks of data in this file,
                            //  by start, length, and tag.
                            //  chunks[1] points to this array itself.
                            //  A -1 length means "to the
                            //  end of the file"
                            //
  ////////////////////////////
  //      Following in the file are more data chunks.  Note that there's
  //      plenty of space here for pre-solves (such as an SVM might generate)
  //      but probably NOT enough space for individual examples to get their
  //      own chunks, unless you change the default number of chunks upward
  //      from 1024.
  ////////////////////////////
}   STATISTICS_FILE_HEADER_STRUCT;

typedef struct {
  unsigned int hash;
  unsigned int key;
  unsigned int value;
} FEATUREBUCKET_STRUCT;


typedef struct {
  unsigned char version[4];
  unsigned long flags;
  unsigned long skip_to;
} FEATURE_HEADER_STRUCT;


typedef struct {
  unsigned int hash;
  unsigned int key;
  float value;
} WINNOW_FEATUREBUCKET_STRUCT;

#define ENTROPY_RESERVED_HEADER_LEN 1024
typedef struct {
  long firlatstart;
  long firlatlen;
  long nodestart;
  long nodeslen;
  long long totalbits;
} ENTROPY_HEADER_STRUCT;

typedef struct mythical_entropy_alphabet_slot {
  long count;
  long nextcell;
} ENTROPY_ALPHABET_SLOT;

//  28 byte header, 24 bytes alph (52 tot).  Pare: 16 header, 16 alph (36 tot)
typedef struct mythical_entropy_cell {
  double fir_prior;
  long fir_larger;
  long fir_smaller;
  long firlat_slot;
  //  long total_count;
  ENTROPY_ALPHABET_SLOT abet[ENTROPY_ALPHABET_SIZE];
} ENTROPY_FEATUREBUCKET_STRUCT;


//   TMS struct - used for measurng process time.
typedef struct mythical_tms_struct {
  clock_t tms_utime;  // user time
  clock_t tms_stime;  // system time
  clock_t tms_cutime; // user time of children
  clock_t tms_cstime; // system time of children
} TMS_STRUCT;


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
#define CRM_ROUTINE 18
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
#define CRM_LAZY 33          // makes a "lazy" variable.
#define CRM_UNIMPLEMENTED 34


//      FLAGS FLAGS FLAGS
//       all of the valid CRM114 flags are listed here
//
//      GROT GROT GROT - You must keep this in synchrony with the
//      definitions of the keywords in crm_stmt_parser!!!  Yes, I'd
//      love to define it in one place and one place only, but I haven't
//      figured out a way to do that well.

//     match searchstart flags
#define CRM_FROMSTART     (1LLU << 0)
#define CRM_FROMNEXT      (1LLU << 1)
#define CRM_FROMEND       (1LLU << 2)
#define CRM_NEWEND        (1LLU << 3)
#define CRM_FROMCURRENT   (1LLU << 4)
//         match control flags
#define CRM_NOCASE        (1LLU << 5)
#define CRM_ABSENT        (1LLU << 6)
#define CRM_BASIC         (1LLU << 7)
#define CRM_BACKWARDS     (1LLU << 8)
#define CRM_LITERAL       (1LLU << 9)
#define CRM_NOMULTILINE   (1LLU << 10)
//         input/output/window flags
#define CRM_BYLINE        CRM_NOMULTILINE
#define CRM_BYCHAR        (1LLU << 11)
#define CRM_STRING        CRM_BYCHAR     // string is bychar.  I think...
#define CRM_BYCHUNK       (1LLU << 12)
#define CRM_BYEOF         (1LLU << 13)
#define CRM_EOFACCEPTS    (1LLU << 14)
#define CRM_EOFRETRY      (1LLU << 15)
#define CRM_APPEND        (1LLU << 16)
//           process control flags
#define CRM_KEEP          (1LLU << 17)
#define CRM_ASYNC         (1LLU << 18)
//        learn and classify
#define CRM_REFUTE        (1LLU << 19)
#define CRM_MICROGROOM    (1LLU << 20)
#define CRM_MARKOVIAN     (1LLU << 21)
#define CRM_OSB_BAYES     (1LLU << 22)       // synonym with OSB feature gen
#define CRM_OSB           CRM_OSB_BAYES
#define CRM_CORRELATE     (1LLU << 23)
#define CRM_OSB_WINNOW    (1LLU << 24)      //  synonym to Winnow feature combiner
#define CRM_WINNOW        CRM_OSB_WINNOW
#define CRM_CHI2          (1LLU << 25)
#define CRM_UNIQUE        (1LLU << 26)
#define CRM_ENTROPY       (1LLU << 27)
#define CRM_OSBF          (1LLU << 28)     // synonym with OSBF local rule
#define CRM_OSBF_BAYES    CRM_OSBF
#define CRM_HYPERSPACE    (1LLU << 29)
#define CRM_UNIGRAM       (1LLU << 30)
#define CRM_CROSSLINK     (1LLU << 31)
//
//        Flags that need to be sorted back in
//           input
#define CRM_READLINE      (1LLU << 32)
//           isolate flags
#define CRM_DEFAULT       (1LLU << 33)
//           SKS classifier
#define CRM_SKS          (1LLU << 34)
//           SVM classifier
#define CRM_SVM           (1LLU << 35)
//           FSCM classifier
#define CRM_FSCM          (1LLU << 36)
//           Neural Net classifier
#define CRM_NEURAL_NET    (1LLU << 37)
//
#define CRM_ERASE         (1LLU << 38)
//PCA classifier
#define CRM_PCA           (1LLU << 39)
//     and a struct to put them in.
typedef struct
{
  char * string;
  unsigned long long value;
} FLAG_DEF ;


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
  char *stmt_name;
  int stmt_code;
  int namelen;
  int is_executable;
  int minslashes;
  int maxslashes;
  int minparens;
  int maxparens;
  int minboxes;
  int maxboxes;
  long flags_allowed_mask;
} STMT_TABLE_TYPE;


//     The compiler file actually contains this "for real", the
//      extern here is merely a reference to it.
//
#ifndef BASE_COMPILER_TABLE_HERE
extern STMT_TABLE_TYPE stmt_table[];
#endif


//   these defines are for arg type... note that they must remain synched
//   IN THIS ORDER with the start chars and end chars in crm_statement_parse
//
#define CRM_ANGLES 0
#define CRM_PARENS 1
#define CRM_BOXES  2
#define CRM_SLASHES 3



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


#endif	// !__CRM114_STRUCTS_H__
