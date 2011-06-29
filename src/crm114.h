//  crm114.h  - Controllable Regex Mutilator base declarations, version X0.1
//  Copyright William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 1.0.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org .  Other licenses may be negotiated; contact the
//  author for details.

#ifndef __CRM114_H__
#define __CRM114_H__


//
//    Global variables

//   The VHT (Variable Hash Table)
extern VHT_CELL **vht;       /* [i_a] no variable instantiation in a common header file */

//   The pointer to the global Current Stack Level (CSL) frame
extern CSL_CELL *csl;

//    the data window
extern CSL_CELL *cdw;

//    the temporarys data window (where argv, environ, newline etc. live)
extern CSL_CELL *tdw;

//    the pointer to a CSL that we use during matching.  This is flipped
//    to point to the right data window during matching.  It doesn't have
//    it's own data, unlike cdw and tdw.
extern CSL_CELL *mdw;

//    a pointer to the current statement argparse block.  This gets whacked
//    on every new statement.
//extern ARGPARSE_BLOCK *apb;


//    the command line argv[0]: the application path/name
extern char *prog_argv0;

//    the auxilliary input buffer (for WINDOW input)
extern char *newinputbuf;

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
extern char *inbuf;
extern char *outbuf;
extern char *tempbuf;





//    the microcompiler
int crm_microcompiler(CSL_CELL  *csl,
        VHT_CELL               **vht);


//  hash function for variable tables
crmhash_t strnhash(const char *str, size_t len);

crmhash64_t strnhash64(const char *str, size_t len);

//  string translate function - for the TRANSLATE function
int strntrn(
        unsigned char *datastr,
        int *datastrlen,
        int maxdatastrlen,
        unsigned char *fromstr,
        int fromstrlen,
        unsigned char *tostr,
        int tostrlen,
        uint64_t  flags);


//   basic math evaluator top function
int strmath(char *buf, int inlen, int maxlen, int *retstat);

//   basic math evaluator in RPN
int strpnmath(char *buf, int inlen, int maxlen, int *retstat);

//   basic math evaluator in RPN
int stralmath(char *buf, int inlen, int maxlen, int *retstat);

//   load a file with info in a partially filled out csl cell.
int crm_load_csl(CSL_CELL *csl);

//    alter a variable to another value (this is destructive!)
void crm_destructive_alter_variable(char *varname, char *newstr);
void crm_destructive_alter_nvariable(char *varname, int varlen,
        char *newstr, int newlen);

//  setting a program label in the VHT
void crm_setvar(
        char *filename,                 // file where first defined (or NULL)
        int   filedesc,                 // filedesc of defining file (or NULL)
        char *nametxt,                  // block of text hosting variable name
        int nstart,                   // index into nametxt to start varname
        int nlen,                     // length of name
        char *valtxt,                   // text block hosts the captured value
        int vstart,                   // index of start of cap. value
        int vlen,                     // length of captured value
        int linenumber,               // linenumber (if pgm, else -1)
        int lazy_redirects            // if nonzero, this is a lazy redirect
               );

//   put a variable and a value into the temporary area
void crm_set_temp_nvar(char *varname, char *value, int vallen);
void crm_set_temp_var(char *varname, char *value);

//   put a variable and a window-based value into the temp area
void crm_set_windowed_var(char *varname,
        char                   *text,
        int                    start,
        int                    len,
        int                    stmtnum);

//   put a counted-length var and a data-window-based value into the temp area.
void crm_set_windowed_nvar(char *varname,
        int                     varlen,
        char                    *valtext,
        int                     start,
        int                     len,
        int                     stmtnum);

//    set a program label.
void crm_setpgmlabel(int start, int end, int stmtnum);


//     preprocess the program... including fixing up semicolons.
int crm_preprocessor(CSL_CELL *csl, int flags);

void crm_break_statements(int ini, int nchars, CSL_CELL *csl);


//     suck on a stream and put it into a buffer.

char *crm_mapstream(FILE *instream); // read from instream till
// it goes dry, putting result into a buffer.

//     actually execute a compiled CRM file
int crm_invoke(void);

//     look up a variable line number (for GOTOs among other things)
int crm_lookupvarline(VHT_CELL **vht, char *text, int start, int len);


//      grab_delim_string looks thru char *in for the first occurrence
//      of delim[0].  It then looks for the next occurrence of delim[1],
//      (with an escape character of delim[2]),
//      and copies the resulting string (without the delims) into res,
//      null-terminating the result.  At most reslen-1 charscters
//      are copied, and at most inlen characters are checked.  The return
//      value of this function is the address of the closing delimiter in *in.
//
//      flags:  CRM_FIRST_CLOSE - first close delimiter found ends string.
//              CRM_LAST_CLOSE  - last close delimiter found ends string
//              CRM_COUNT_CLOSE - keep a count of open and close delims  NYI
//

char *grab_delimited_string(char *res, char *in, char *delim,
        int inlen, int reslen, int flags);


//   expand the variable in the input buffer (according to the :*: operator
int crm_expandvar(char *buf, int maxlen);


//   look up a vht cell, from a variable name.  Returns the VHT cell
//   it's either stored in, or ought to be stored in (i.e. check for a NULL
//   VHT cell before use).

int crm_vht_lookup(VHT_CELL **vht, const char *vname, int vlen);


//     crm_extractflag - given an arbitrary string cmd (start/len)
//     with words delimited by spaces, and a second string "flag"
//     (start/len).
//
//      1) does "flag" exist in "cmd"?
//      2) if so, where?
//      3) what is the start/len of flag in cmd?
//      4) what is the arg _after_ flag (start/len)
//
//     Return value - pointer to start of flag in cmd.  It's
//     unnecessary to return the length of flag, as we already know
//     what it is.  also modifies nextarg start and length.

int crm_extractflag(const char *cmd, int cmdl, const char *flag, int flagl,
        int *next, int *nextl);

//      initialize the vht, insert some some useful variables
void crm_vht_init(int argc, char **argv);


//      Surgically lengthen or shorten a window.   The window pointed to by
//      mdw gets delta extra characters added or cut at "where".  If the
//      allocated length is not enough, additional space can be malloced.
//      Finally, the vht is fixed up so everything still points "correctly".
void crm_slice_and_splice_window(CSL_CELL *mdw, int where, int delta);

//      Update the start and length of all captured variables whenever
//      the input buffer gets mangled.  Mangles are all expressed in
//      the form of a start point, and a delta.
void crm_updatecaptures(char *text, int loc, int delta);

//      A helper function to calculate what the proper changes are for
//      any marked point, given a dot and a delta on that dot.  (sl is
//      0 for a start, and 1 for an end mark).
int crm_mangle_offset(int mark, int dot, int delta, int sl);

//      Possibly reclaim storage in the given zone.
int crm_compress_tdw_section(char *oldtext, int oldstart, int oldend);

//      create a new .css file
int crm_create_cssfile(char *cssfile, int buckets,
        int major, int minor, int spectrum_start);


//      argslice - given a string, destructively slice it up on whitespace
//      boundaries into an argv-like array, and return argc and argv[].
//      WARNING WARNING WARNING this is a destructive operation on the
//      input string!  argc and argv are now suitable for parsing in
//      the usual ways.  Argc should be initialized with the length
//      of argv, it is clobbered with the number of args actually used.

int crm_argslice(char *input, int *argc, char **argv);

//    The magic flag parser.  Given a string of input, and the builtin
//    crm_flags array, returns the flags that are set.
//
//      for each input[i], is it equal to some member of flag_string[j]?
//         if YES, then
//                 out_code[i] gets the value of flag_code[j]
//                 count_code[j] gets incremented.
//         if NONE match, then out_code[j] is zero
//
//      This makes it easy to parse a flag set for presence
//
//      Note that this is a uint64_t- which limits us to no more than
//      64 discrete flags.
uint64_t crm_flagparse(char *input, int inlen); //  the user input


//     get the next word in the input.  (note- the regex stops only when
//     one hits a NULL, which may yield a slightly bogus result.
int crm_nextword(const char *input,
        int inlen,
        int starthere,
        int *start,
        int *len);

int crm_expr_clump_nn(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_pmulc_nn(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//   The big one - matching...
int crm_expr_match(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//   the learner... in variant forms...
int crm_expr_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_alt_bit_entropy_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);



//   The bigger one - classifying...
int crm_expr_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_mergeing...
int crm_expr_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_diffing...
int crm_expr_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_backuping...
int crm_expr_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_restoreing...
int crm_expr_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_infoing...
int crm_expr_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_analyzeing...
int crm_expr_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//   Additional tooling - css_createing...
int crm_expr_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_markov_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_bayes_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_neural_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_correlate_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_winnow_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_osb_hyperspace_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_bit_entropy_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_svm_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_sks_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_expr_fscm_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);
int crm_neural_net_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txt, int start, int len);




//  surgically alter a variable
int crm_expr_alter(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  EVAL - double-evaluate for indirectiion's sake.  Otherwise, it's just
//   like ALTER
int crm_expr_eval(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  WINDOW - do a windowing operation on a variable
int crm_expr_window(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  ISOLATE - do an isolation
int crm_expr_isolate(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_isolate_this(int *vptr,
        char *nametext, int namestart, int namelen,
        char *valuetext, int valuestart, int valuelen);

//  INPUT - do input
int crm_expr_input(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  OUTPUT - do an output
int crm_expr_output(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  SYSCALL - fork another process
int crm_expr_syscall(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  TRANSLATE - translate character sets
int crm_expr_translate(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

//  CLUMP and PMULC
int crm_expr_clump(CSL_CELL *csl, ARGPARSE_BLOCK *apb);
int crm_expr_pmulc(CSL_CELL *csl, ARGPARSE_BLOCK *apb);

// REDUCE - translate text to a phonetic/visual reduced/deobfuscated format
int crm_expr_reduce(CSL_CELL *csl, ARGPARSE_BLOCK *apb);


//      parse a CRM114 statement; this is mostly a setup routine for
//     the generic parser.
int crm_statement_parse(char           *in,
        int                            slen,
        ARGPARSE_BLOCK                 *apb);


//    and a generic parser routine for parsing a line according
//    to the type of quoting done.
int crm_generic_parse_line(
        char *txt,                       //   the start of the program line
        int   len,                       //   how int is the line
        int   maxargs,                   //   howm many things to search for (max)
        int  *ftype,                     //   type of thing found (index by schars)
        int  *fstart,                    //   starting location of found arg
        int  *flen                       //   length of found arg
                          );

//    and to avoid all the mumbo-jumbo, an easy way to get a copy of
//    an arg found by the declensional parser.
void crm_get_pgm_arg(char *to, int tolen, char *from, int fromlen);



//     The vector tokenizer - used to turn text into hash vectors.
//

int crm_vector_tokenize_selector
(
        ARGPARSE_BLOCK       *apb,          // The args for this line of code
        const char           *text,         // input string (null-safe!)
        int                   textlen,      //   how many bytes of input.
        int                   start_offset, //     start tokenizing at this byte.
        const char           *regex,        // the parsing regex (might be ignored)
        int                   regexlen,     //   length of the parsing regex
        const crmhash_t      *coeff_array,  // the pipeline coefficient control array
        int                   pipe_len,     //  how int a pipeline (== coeff_array row length)
        int                   pipe_iters,   //  how many rows are there in coeff_array
        crmhash_t            *features,     // where the output features go
        int                   featureslen,  //   how many output features (max)
        int                  *features_out, // how many longs did we actually use up
        int                  *next_offset   // next invocation should start at this offset
);

// this interface method is provided only for those that 'know what they're doing',
// i.e. (unit) test code such as testtocvek:
int crm_vector_tokenize
(
        const char          *text,            // input string (null-safe!)
        int                  textlen,         //   how many bytes of input.
        int                  start_offset,    //     start tokenizing at this byte.
        const char          *regex,           // the parsing regex (might be ignored)
        int                  regexlen,        //   length of the parsing regex
        const crmhash_t     *coeff_array,     // the pipeline coefficient control array
        int                  pipe_len,        //  how int a pipeline (== coeff_array col height)
        int                  pipe_iters,      //  how many rows are there in coeff_array
        crmhash_t           *features,        // where the output features go
        int                  featureslen,     //   how many output features (max)
        int                  features_stride, //   Spacing (in words) between features
        int                 *features_out,    // how many longs did we actually use up
        int                 *next_offset      // next invocation should start at this offset
);


//     crm execution-time debugging environment - an interpreter unto itself
//
int crm_debugger(void);

//     expand a variable or string with known length (8-bit and null-safe)

int crm_nexpandvar(char *buf, int inlen, int maxlen);

//     execute a FAULT triggering.
int crm_trigger_fault(char *reason);

//     do an microgroom of a hashed file.
int crm_microgroom(FEATUREBUCKET_TYPE *h,
        unsigned char                  *seen_features,
        int                            hs,
        unsigned int                   hindex);
void crm_packcss(FEATUREBUCKET_TYPE *h,
        unsigned char *seen_features,
        int hs, int packstart, int packlen);
void crm_packseg(FEATUREBUCKET_TYPE *h,
        unsigned char *seen_features,
        int hs, int packstart, int packlen);
//
//     and microgrooming for winnow files
int crm_winnow_microgroom(WINNOW_FEATUREBUCKET_STRUCT *h,
        unsigned char                                  *seen_features,
        unsigned int                                   hfsize,
        unsigned int                                   hindex);

void crm_pack_winnow_css(WINNOW_FEATUREBUCKET_STRUCT *h,
        unsigned char *xhashes,
        int hs, int packstart, int packlen);
void crm_pack_winnow_seg(WINNOW_FEATUREBUCKET_STRUCT *h,
        unsigned char *xhashes,
        int hs, int packstart, int packlen);



//     print out timings of each statement
void crm_output_profile(CSL_CELL *csl);

//     do basic math expressions
int crm_expr_math(char *instr, unsigned int inlen,
        char *outstr, unsigned int max_outlen,
        int *status_p);

//      var-expansion operators
//             simple (escapes and vars) expansion
int crm_nexpandvar(char *buf, int inlen, int maxlen);

//             complex (escapes, vars, strlens, and maths) expansion
int crm_qexpandvar(char *buf, int inlen, int maxlen, int *retstat);

//              generic (everything, as you want it, bitmasked) expansion
int crm_zexpandvar(char *buf,
        int              inlen,
        int              maxlen,
        int             *retstat,
        int              exec_bitmask);

//       Var-restriction operators  (do []-vars, like subscript and regex )
int crm_restrictvar(char *boxstring,
        int boxstrlen,
        int *vht_idx,
        char **outblock,
        int *outoffset,
        int *outlen,
        char *errstr);


//      crm114-specific regex compilation

int crm_regcomp(regex_t *preg, const char *regex, int regex1_len, int cflags);

int crm_regexec(regex_t * preg, const char *string, int string_len,
                size_t nmatch, regmatch_t pmatch[], int eflags,
                const char *aux_string);

size_t crm_regerror(int errocode, regex_t *preg, char *errbuf,
        size_t errbuf_size);

void crm_regfree(regex_t *preg);

char *crm_regversion(void);


//        Portable mmap/munmap
//


void *crm_mmap_file(char *filename, int start, int len, int prot, int mode, int advise, int *actual_len);

void crm_munmap_file(void *where);
void crm_munmap_file_internal(void *map);
void crm_munmap_all(void);
void crm_force_munmap_filename(char *filename);
void crm_force_munmap_addr(void *addr);
void *crm_get_header_for_mmap_file(void *addr);

//    Some statistics functions
//
double crm_norm_cdf(double x);
double crm_log(double x);
double norm_pdf(double x);
double normalized_gauss(double x, double s);
double crm_frand(void);




/* for use with vxxxxerror_ex et al */
#define SRC_LOC()               __LINE__, __FILE__, __FUNCTION__

#ifdef HAVE_STRINGIZE
#define CRM_STRINGIFY(e)        # e
#else
#error \
    "Comment this error line out if you are sure your system does not support the # preprocessor stringize operator. My Gawd, what system _is_ this?"
#define CRM_STRINGIFY(e)        "---\?\?\?---" /* \?\? to prevent trigraph warnings by GCC 4 et al */
#endif



//  helper routine for untrappable errors
#define untrappableerror(msg1, msg2) \
    untrappableerror_std(__LINE__, __FILE__, __FUNCTION__, msg1, msg2)

void untrappableerror_std(int lineno, const char *srcfile, const char *funcname, const char *msg1, const char *msg2)
__attribute__((__noreturn__));
void untrappableerror_ex(int lineno, const char *srcfile, const char *funcname, const char *msg, ...)
__attribute__((__noreturn__, __format__(__printf__, 4, 5)));
void untrappableerror_va(int lineno, const char *srcfile, const char *funcname, const char *msg, va_list args)
__attribute__((__noreturn__, __format__(__printf__, 4, 0)));

//  helper routine for fatal errors
#define fatalerror(msg1, msg2) \
    fatalerror_std(__LINE__, __FILE__, __FUNCTION__, msg1, msg2)

int fatalerror_std(int lineno, const char *srcfile, const char *funcname, const char *msg1, const char *msg2);
int fatalerror_ex(int lineno, const char *srcfile, const char *funcname, const char *msg, ...)
__attribute__((__format__(__printf__, 4, 5)));
int fatalerror_va(int lineno, const char *srcfile, const char *funcname, const char *msg, va_list args)
__attribute__((__format__(__printf__, 4, 0)));

//  helper routine for nonfatal errors
#define nonfatalerror(msg1, msg2) \
    nonfatalerror_std(__LINE__, __FILE__, __FUNCTION__, msg1, msg2)

int nonfatalerror_std(int lineno, const char *srcfile, const char *funcname, const char *msg1, const char *msg2);
int nonfatalerror_ex(int lineno, const char *srcfile, const char *funcname, const char *msg, ...)
__attribute__((__format__(__printf__, 4, 5)));
int nonfatalerror_va(int lineno, const char *srcfile, const char *funcname, const char *msg, va_list args)
__attribute__((__format__(__printf__, 4, 0)));


/*
 * Reset the nonfatalerror counters/handlers. This is useful when you run multiple scripts
 * one after the other from within a single crm app, such as crm_test.
 */
void reset_nonfatalerrorreporting(void);




#ifndef CRM_DONT_ASSERT

extern int trigger_debugger;

void crm_show_assert_msg(int lineno, const char *srcfile, const char *funcname, const char *msg);
void crm_show_assert_msg_ex(int lineno, const char *srcfile, const char *funcname, const char *msg, const char *extra_msg);

#define CRM_ASSERT(expr)                                         \
    do                                                           \
    {                                                            \
        if (!(expr))                                             \
        {                                                        \
            crm_show_assert_msg(__LINE__, __FILE__, __FUNCTION__ \
                               , CRM_STRINGIFY(expr));           \
        }                                                        \
    } while (0)

#define CRM_ASSERT_EX(expr, msg)                                    \
    do                                                              \
    {                                                               \
        if (!(expr))                                                \
        {                                                           \
            crm_show_assert_msg_ex(__LINE__, __FILE__, __FUNCTION__ \
                                  , CRM_STRINGIFY(expr), (msg));    \
        }                                                           \
    } while (0)

#define CRM_VERIFY(expr)                                         \
    do                                                           \
    {                                                            \
        if (!(expr))                                             \
        {                                                        \
            crm_show_assert_msg(__LINE__, __FILE__, __FUNCTION__ \
                               , CRM_STRINGIFY(expr));           \
        }                                                        \
    } while (0)

#else

#define CRM_ASSERT(expr)           /* do nothing */
#define CRM_ASSERT_EX(expr, msg)   /* do nothing */

#define CRM_VERIFY(expr)           (void)(expr)

#endif



/*
 * Error handling
 */


const char *errno_descr(int errno_number);
/* const char *syserr_descr(int errno_number); */

#if defined (WIN32)
/*
 * return a string containing the errorcode description.
 */
void Win32_syserr_descr(char **dst, size_t max_dst_len, DWORD errorcode, const char *arg);


#define fatalerror_Win32(msg, arg) \
    fatalerror_Win32_(SRC_LOC(), msg ": system error %d(0x%08x:%s)", arg)

static inline void fatalerror_Win32_(int lineno, const char *file, const char *funcname, const char *msg, const char *arg)
{
    DWORD error = GetLastError();
    char errbuf[MAX_PATTERN];
    char *errmsg = errbuf;

	Win32_syserr_descr(&errmsg, MAX_PATTERN, error, arg);

    fatalerror_ex(lineno, file, funcname, msg,
            (int)error,
            (int)error,
            errmsg);
}


#define nonfatalerror_Win32(msg, arg) \
    nonfatalerror_Win32_(SRC_LOC(), msg ": system error %d(0x%08x:%s)", arg)

static inline int nonfatalerror_Win32_(int lineno, const char *file, const char *funcname, const char *msg, const char *arg)
{
    DWORD error = GetLastError();
    char errbuf[MAX_PATTERN];
    char *errmsg = errbuf;
	
	Win32_syserr_descr(&errmsg, MAX_PATTERN, error, arg);

    return nonfatalerror_ex(lineno, file, funcname, msg,
            (int)error,
            (int)error,
            errmsg);
}


#endif


/*
 * Diagnostics: Memory checks / analysis
 */

#if defined (WIN32) && defined (_DEBUG)

extern _CrtMemState crm_memdbg_state_snapshot1;
extern int trigger_memdump;

int crm_dbg_report_function(int reportType, char *userMessage, int *retVal);
void crm_report_mem_analysis(void);

#endif



/*
 * Memory cleanup
 */
void free_hash_table(VHT_CELL **vht, size_t vht_size);
void free_arg_parseblock(ARGPARSE_BLOCK *apb);
void free_stack_item(CSL_CELL *csl);
void free_stack(CSL_CELL *csl);

void cleanup_expandvar_allocations(void);

void free_regex_cache(void);

void free_debugger_data(void);



/*
 * Extra support routines.
 */

// write count bytes of val val to file dst
int file_memset(FILE *dst, unsigned char val, int count);

const char *skip_path(const char *srcfile);

// dump var/string/... in src to dst
int memnCdump(FILE *dst, const char *src, int len);



void init_stdin_out_err_as_os_handles(void);
void cleanup_stdin_out_err_as_os_handles(void);
FILE *os_stdin(void);
FILE *os_stdout(void);
FILE *os_stderr(void);
int is_stdin_or_null(FILE *f);
int is_stdout_err_or_null(FILE *f);



/*
 * CRM114 version/portability header support
 */
int is_crm_headered_file(FILE *f);
int fwrite_crm_headerblock(FILE *f, CRM_PORTA_HEADER_INFO *classifier_info, const char *human_readable_message);
int crm_correct_for_version_header(void **ptr, int *len);
int crm_decode_header(void *src, int64_t acceptable_classifiers, int fast_only_native, CRM_DECODED_PORTA_HEADER_INFO *dst);



#endif /* __CRM114_H__ */

