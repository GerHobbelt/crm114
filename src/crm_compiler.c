//	crm114_compiler.c - CRM114 microcompiler

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




//     Here's the real statement description table.
//
static const STMT_TABLE_TYPE stmt_table[] =
{
    //  text         internal       nlen exec special  min max   min max   min max  min max  flags
    //   rep           code               ?    flags   angles   slashargs  parens    boxes
    { "\n",        CRM_BOGUS,         1,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },    /* [i_a] this is also the item used for the CRM_BOGUS sentinel */
    { ":",         CRM_LABEL,         1,  0,     0,    0,  0,     0,  0,    0,  1,   0,  0,    0 },    /* [i_a] added - LABEL must be at position; assert in code checks this */
    { ";",         CRM_NOOP,          1,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },    /* [i_a] added */
    { "#",         CRM_NOOP,          1,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "insert",    CRM_INSERT,        6,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "noop",      CRM_NOOP,          4,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "{",         CRM_OPENBRACKET,   1,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "}",         CRM_CLOSEBRACKET,  1,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "accept",    CRM_ACCEPT,        6,  1,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "alius",     CRM_ALIUS,         5,  1,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "alter",     CRM_ALTER,         5,  1,     0,    0,  0,     1,  1,    1,  1,   0,  0,    0 },
    { "call",      CRM_CALL,          4,  1,     0,    0,  0,     1,  1,    0,  1,   0,  1,  CRM_KEEP },
    { "cssmerge",  CRM_CSS_MERGE,     8,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_UNIQUE | CRM_MICROGROOM | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    //  text         internal       nlen exec special  min max   min max   min max  min max  flags
    //   rep           code               ?    flags   angles   slashargs  parens    boxes
    { "cssdiff",   CRM_CSS_DIFF,      7,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_UNIQUE | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "cssbackup", CRM_CSS_BACKUP,    9,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "cssrestore", CRM_CSS_RESTORE, 10,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "cssinfo",   CRM_CSS_INFO,      7,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "cssanalyze", CRM_CSS_ANALYZE, 10,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "csscreate", CRM_CSS_CREATE,    9,  1,     1,    0,  1,     0,  1,    1,  2,   0,  1,  CRM_DEFAULT | CRM_NOCASE | CRM_BASIC | CRM_NOMULTILINE
      | CRM_LITERAL | CRM_BYCHUNK
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "cssmigrate", CRM_CSS_MIGRATE, 10,  1,     0,    0,  0,     0,  1,    1,  1,   1,  1,  CRM_DEFAULT | CRM_BASIC
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_AUTODETECT | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "debug",     CRM_DEBUG,         5,  0,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "eval",      CRM_EVAL,          4,  1,     0,    0,  0,     1,  1,    0,  1,   0,  0,  CRM_KEEP },
    { "exit",      CRM_EXIT,          4,  1,     0,    0,  0,     0,  1,    0,  0,   0,  0,    0 },
    { "fail",      CRM_FAIL,          4,  1,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "fault",     CRM_FAULT,         5,  1,     0,    0,  0,     0,  1,    0,  0,   0,  0,    0 },
    { "goto",      CRM_GOTO,          4,  0,     0,    0,  0,     1,  1,    0,  0,   0,  0,    0 },
    //  text         internal       nlen exec special  min max   min max   min max  min max  flags
    //   rep           code               ?    flags   angles   slashargs  parens    boxes
    { "hash",      CRM_HASH,          4,  1,     0,    0,  0,     1,  1,    1,  1,   0,  0,    0 },
    { "input",     CRM_INPUT,         5,  1,     0,    0,  1,     0,  0,    0,  1,   0,  1,  CRM_BYLINE | CRM_READLINE | CRM_KEEP },
    { "intersect", CRM_INTERSECT,     9,  1,     0,    0,  0,     0,  0,    1,  1,   1,  1,  CRM_KEEP },
    { "isolate",   CRM_ISOLATE,       7,  1,     0,    0,  1,     0,  1,    1,  1,   0,  0,  CRM_DEFAULT | CRM_KEEP },
    { "lazy",      CRM_LAZY,          4,  1,     0,    0,  1,     0,  1,    1,  1,   0,  0,  CRM_DEFAULT },
    { "learn",     CRM_LEARN,         5,  1,     0,    0,  1,     0,  2,    1,  1,   0,  1,  CRM_NOCASE | CRM_BASIC | CRM_NOMULTILINE | CRM_LITERAL
      | CRM_BYCHUNK
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_REFUTE | CRM_ERASE | CRM_APPEND | CRM_ABSENT
      | CRM_NEURAL_NET | CRM_MARKOVIAN | CRM_FROMSTART
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "classify",  CRM_CLASSIFY,      8,  1,     0,    0,  1,     0,  2,    1,  2,   0,  1,  CRM_NOCASE | CRM_BASIC | CRM_NOMULTILINE | CRM_LITERAL
      | CRM_BYCHUNK
      | CRM_MICROGROOM | CRM_UNIQUE | CRM_UNIGRAM | CRM_CHI2 | CRM_CROSSLINK | CRM_STRING
      | CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
      | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
      | CRM_NEURAL_NET | CRM_MARKOVIAN
      | CRM_ALT_OSB_BAYES | CRM_ALT_OSB_WINNOW | CRM_ALT_OSBF | CRM_ALT_MARKOVIAN | CRM_ALT_HYPERSPACE },
    { "liaf",      CRM_LIAF,          4,  1,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "match",     CRM_MATCH,         5,  1,     0,    0,  1,     1,  1,    0,  1,   0,  1,
      CRM_ABSENT | CRM_NOCASE | CRM_LITERAL | CRM_FROMSTART
      | CRM_FROMCURRENT | CRM_FROMNEXT | CRM_FROMEND | CRM_NEWEND
      | CRM_BACKWARDS | CRM_NOMULTILINE | CRM_BASIC | CRM_KEEP },
    { "mutate",    CRM_MUTATE,        7,  1,     1,    0,  1,     1,  1,    0,  1,   0,  1,  CRM_NOCASE | CRM_BYCHAR | CRM_NOMULTILINE | CRM_BYLINE
      | CRM_BYCHUNK | CRM_UNIQUE | CRM_BASIC | CRM_DEFAULT | CRM_STRING | CRM_ABSENT },
    { "output",    CRM_OUTPUT,        6,  1,     0,    0,  1,     0,  1,    0,  0,   0,  1,  CRM_APPEND },
    { "clump",     CRM_CLUMP,         5,  1,     0,    0,  1,     0,  2,    1,  2,   1,  1,  CRM_UNIQUE | CRM_UNIGRAM | CRM_BYCHUNK | CRM_REFUTE },
    { "pmulc",     CRM_PMULC,         5,  1,     0,    0,  1,     0,  1,    1,  2,   1,  1,  CRM_UNIQUE | CRM_UNIGRAM | CRM_BYCHUNK | CRM_REFUTE },
    //  text         internal       nlen exec special  min max   min max   min max  min max  flags
    //   rep           code               ?    flags   angles   slashargs  parens    boxes
    { "return",    CRM_RETURN,        6,  1,     0,    0,  0,     0,  1,    0,  0,   0,  0,    0 },
    { "routine",   CRM_ROUTINE,       7,  1,     0,    0,  0,     0,  0,    0,  0,   0,  0,    0 },
    { "sort",      CRM_SORT,          4,  1,     0,    0,  1,     0,  1,    0,  1,   0,  1,  CRM_NOCASE | CRM_BYCHAR | CRM_NOMULTILINE | CRM_BYLINE
      | CRM_BYCHUNK | CRM_UNIQUE | CRM_BASIC | CRM_DEFAULT | CRM_STRING | CRM_BACKWARDS },
    { "syscall",   CRM_SYSCALL,       7,  1,     0,    0,  1,     0,  1,    0,  3,   0,  1,  CRM_KEEP | CRM_ASYNC },
    { "translate", CRM_TRANSLATE,     9,  1,     0,    0,  1,     0,  2,    0,  1,   0,  1,  CRM_UNIQUE | CRM_LITERAL },
    { "trap",      CRM_TRAP,          4,  1,     0,    0,  0,     1,  1,    0,  1,   0,  0,    0 },
    { "union",     CRM_UNION,         5,  1,     0,    0,  0,     0,  0,    1,  1,   1,  1,  CRM_KEEP },
    { "window",    CRM_WINDOW,        6,  1,     0,    0,  1,     0,  2,    0,  2,   0,  0,  CRM_NOCASE | CRM_BYCHAR | CRM_BYCHUNK | CRM_BYEOF
      | CRM_EOFACCEPTS | CRM_EOFRETRY | CRM_LITERAL },
};



/*
 * code to print the script language specification, whole or part.
 *
 * When opcode_id < 0, show all instructions. Otherwise, show only the one.
 */
int show_instruction_spec(int opcode_id, show_instruction_spec_writer_cb *cb, void *propagator)
{
    int i;
    int j;
    int max_nl = 0;
    int n;

    for (i = 0; i < WIDTHOF(stmt_table); i++)
    {
        if (stmt_table[i].namelen > max_nl)
        {
            max_nl = stmt_table[i].namelen;
        }
    }

    n = 0;
    for (i = 0; i < WIDTHOF(stmt_table); i++)
    {
        const STMT_TABLE_TYPE *s = &stmt_table[i];

        if (opcode_id < 0 || opcode_id == s->stmt_code)
        {
            if (s->is_executable)
            {
                // print one statement:
                char buf[64 * 60];
                char *d = buf;
                size_t dlen = sizeof(buf);

                if (n > 0 && (*cb)("\n", -1, propagator) < 0)
                    return -1;

                snprintf(d, dlen, "%*s ", -max_nl, s->stmt_name);
                if ((*cb)(d, -1, propagator) < 0)
                    return -1;

                j = 0;

                if (s->flags_allowed_mask)
                {
                    if ((*cb)("<", -1, propagator) < 0)
                        return -1;

                    if (show_instruction_flags(s->flags_allowed_mask, cb, propagator) < 0)
                        return -1;

                    if (s->has_non_standard_flags)
                    {
                        if ((*cb)(" ...", -1, propagator) < 0)
                            return -1;
                    }

                    if ((*cb)("> ", -1, propagator) < 0)
                        return -1;

                    j = 1;
                }

                // <>
                for (/* j = 0 */; j < s->minangles; j++)
                {
                    if ((*cb)("<...> ", -1, propagator) < 0)
                        return -1;
                }
                for (; j < s->maxangles; j++)
                {
                    if ((*cb)("<optional> ", -1, propagator) < 0)
                        return -1;
                }

                // ()
                for (j = 0; j < s->minparens; j++)
                {
                    if ((*cb)("(...) ", -1, propagator) < 0)
                        return -1;
                }
                for (; j < s->maxparens; j++)
                {
                    if ((*cb)("(optional) ", -1, propagator) < 0)
                        return -1;
                }

                // []
                for (j = 0; j < s->minboxes; j++)
                {
                    if ((*cb)("[...] ", -1, propagator) < 0)
                        return -1;
                }
                for (; j < s->maxboxes; j++)
                {
                    if ((*cb)("[optional] ", -1, propagator) < 0)
                        return -1;
                }

                // /.../
                for (j = 0; j < s->minslashes; j++)
                {
                    if ((*cb)("/.../ ", -1, propagator) < 0)
                        return -1;
                }
                for (; j < s->maxslashes; j++)
                {
                    if ((*cb)("/optional/ ", -1, propagator) < 0)
                        return -1;
                }

                n++;
            }
        }
    }
    return 0;
}




//   Get a file into a memory buffer.  We can either prep to execute
//    it, or use it as read-only data, or as read-write data.
//
int crm_load_csl(CSL_CELL *csl)
{
    struct stat statbuf;        //  status buffer - for statting files
    int i;

    //   open it first
    csl->filedes = -1;
    if (csl->rdwr)
    {
        csl->filedes = open(csl->filename, O_RDWR);
    }
    else
    {
        csl->filedes = open(csl->filename, O_RDONLY);
    }

    if (csl->filedes < 0)
    {
        if (errno == ENAMETOOLONG)
        {
            untrappableerror("Couldn't open the file (filename too long): ",
                             csl->filename);
        }
        else
        {
            char dirbuf[DIRBUFSIZE_MAX];

            untrappableerror_ex(SRC_LOC(), "Couldn't open the file '%s' (full path: '%s')",
                                csl->filename, mk_absolute_path(dirbuf, WIDTHOF(dirbuf), csl->filename));
        }
    }
    else
    {
        if (internal_trace)
            fprintf(stderr, "file open on file descriptor %d\n", csl->filedes);

        csl->nchars = 0;
        // and stat the file descriptor
        if (fstat(csl->filedes, &statbuf))
        {
            untrappableerror_ex(SRC_LOC(), "Cannot stat file %s, error: %d(%s)", csl->filename, errno, errno_descr(errno));
        }
        else
        {
            csl->nchars = statbuf.st_size;
            if (internal_trace)
                fprintf(stderr, "file is %d bytes\n", csl->nchars);
            if (csl->nchars + 2048 > max_pgmsize)
            {
                CRM_ASSERT(csl->filedes >= 0);
                close(csl->filedes);
                untrappableerror("Your program is too big.  ",
                                 " You need to use smaller programs or the -P flag, ");
            }
        }

        //   and read in the source file
        csl->filetext = (char *)calloc(max_pgmsize, sizeof(csl->filetext[0]));
        csl->filetext_allocated = 1;
        if (csl->filetext == NULL)
        {
            CRM_ASSERT(csl->filedes >= 0);
            close(csl->filedes);
            untrappableerror("alloc of the file text failed", csl->filename);
        }
        if (internal_trace)
            fprintf(stderr, "File text alloc'd at %p\n", csl->filetext);

        //    put in a newline at the beginning
        csl->filetext[0] = '\n';

        /* [i_a] make sure we never overflow the buffer: */
        if (csl->nchars + 1 + 3 > max_pgmsize)
        {
            csl->nchars = max_pgmsize - 1 - 3;
        }
        //     read the file in...
        CRM_ASSERT(csl->nchars + 1 + 3 <= max_pgmsize);
        i = read(csl->filedes, &(csl->filetext[1]), csl->nchars);
        /*
         * From the MS docs: If fd is invalid, the file is not open for reading,
         *     or the file is locked, the invalid parameter handler is invoked, as
         *     described in Parameter Validation. If execution is allowed to continue,
         *     the function returns -1 and sets errno to EBADF.
         */
        if (i < 0)
        {
            CRM_ASSERT(csl->filedes >= 0);
            close(csl->filedes);
            untrappableerror("Cannot read from file (it may be locked?): ", csl->filename);
        }

        /*
         * Close the file handle now we're done. (This is important when we have nested files:
         * there's only so many file handles to go around.)
         */
        CRM_ASSERT(csl->filedes >= 0);
        close(csl->filedes);


        // [i_a] WARNING: ALWAYS make sure the program ends with TWO newlines so that we have a program
        //       which comes with an 'empty' statement at the very end. This is mandatory to ensure
        //       a valid 'alius' fail forward target is available at the end of the program at all times.

        i++;
        csl->filetext[i] = '\n';
        i++;
        csl->filetext[i] = '\n';
        i++;
        csl->filetext[i] = 0;
        CRM_ASSERT(i < max_pgmsize);
        csl->nchars = i;

        csl->hash = strnhash(csl->filetext, csl->nchars);
        if (user_trace)
        {
            fprintf(stderr, "Hash of program: 0x%08lX, length %d bytes: (%s)\n-->\n%s",
                    (unsigned long int)csl->hash, csl->nchars, csl->filename, csl->filetext);
        }
    }

    return 0;
}


//      The CRM114 microcompiler.  It takes in a partially completed
//      csl struct which is the program, the program length, and
//      returns a completed microcompile table for that particular
//      program.  Side effect: it also sets some variables in the
//      variable hash table (a.k.a. the VHT)

int crm_microcompiler(CSL_CELL *csl, VHT_CELL **vht)
{
    //    ***   This is the CRM114 microcompiler.  It's a 5-pass
    //   compiler, but each pass is really simple.
    //   pass 1) count lines and allocate microcompile table
    //   pass 2) run thru file, matching on first word in statement, setting
    //        statement type code.  If it's a label statement, also add
    //        an entry to the variables hash table.  If the statement
    //        assigns a value, we _don't_ put the value itself into the
    //        variable hash table until we actually execute the statement
    //   pass 3) run thru file, setting bracket nesting level.  If
    //        bracket level ever goes below 0 or is nonzero at the end
    //        of the file, issue a warning.
    //   pass 4) run thru file, setting FAIL and LIAF targets

    //
    //    HACK ALERT HACK ALERT HACK ALERT
    //
    //    THIS WHOLE PARSER IS A PIECE OF JUNK.  IT REALLY NEEDS TO BE
    //    REDONE IN BISON.  MAYBE FOR V 2.0?
    //
    //    NOTE: this is redone to be table-driven; it's still a piece of
    //    junk but it's _good_ junk.  And it will allow us to do JITting
    //    of programs (1 pass to get labels, then JIT each statement as we
    //    encounter it the first time.  This should make programs run
    //    significantly faster as we never parse twice, and we only parse
    //    the full statement if we are about to execute it.
    //
    //    HACK ALERT HACK ALERT HACK ALERT


    //  i, j, and k are random beat-upon-me longs
    int i, j, k;

    //  a counter to use when iterating thru statements
    int stmtnum;

    //  number of statements actually used
    int numstmts;

    //   how many chars in this program
    int pgmlength;

    //   pointer to the chars
    char *pgmtext;

    // how deep a nesting of brackets does this file have?
    int bracketlevel;

    // index of first char of this statement
    int sindex;

    // index of first nonblank character in the line?
    int nbindex;

    // length of the first nonblank string on the line?
    int nblength;

    // index of first character in the arguments to any particular statement
    int aindex;

    // length of this statement
    int slength;

    //  have we seen an action statement yet?
    int seenaction;

    //    //   counters for looking through the statemt archetype table.
    //    int stab_idx;

    /* int stab_max; */

    if (internal_trace)
        fprintf(stderr, "Starting phase 1 of microcompile.\n");

    seenaction = 0;
    pgmlength = csl->nchars;
    pgmtext = csl->filetext;

    //  ***  Microcompile phase 1 ****
    //  *** Allocation of the microcompile table

    //  get a line count
    j = 0;     //  j will be our line counter here.

    //    preprocessor has already run; all statements have been
    //   properly line-broken and we just count the '\n's.
    for (i = 0; i < pgmlength; i++)
    {
        if (pgmtext[i] == '\n')
            j++;
    }

    csl->nstmts = j;

    //  now, allocate the microcompile table
    if (user_trace)
    {
        fprintf(stderr, "Program statements: %d, program length %d\n",
                j, pgmlength);
    }

    csl->mct_size = csl->nstmts + 10;
    csl->mct = (MCT_CELL **)calloc(csl->mct_size, sizeof(csl->mct[0]));
    csl->mct_allocated = 1;
    if (!csl->mct)
    {
        untrappableerror("Couldn't alloc MCT table.\n"
                         "This is where your compilation results go, "
                         "so if we can't compile, we can't run.  Sorry.", "");
    }
    if (internal_trace)
    {
        fprintf(stderr, "microcompile table at %p\n", (void *)csl->mct);
    }

    //  alloc all of the statement cells.
    for (i = 0; i < csl->mct_size; i++)
    {
        csl->mct[i] = (MCT_CELL *)calloc(1, sizeof(csl->mct[i][0]));
        if (!csl->mct[i])
        {
            untrappableerror("Couldn't alloc MCT cell. This is very bad.\n", "");
        }
        csl->mct[i]->trap_index = -1;  // make sure fatalerror and warnings fail fatally until we're done compiling.
    }

    // ***  Microcompile phase 2 - set statement types

    //   iterate through the statements, setting types.
    //   i is our character counter
    //
    //    HACK ALERT HACK ALERT HACK ALERT
    //
    //    THIS WHOLE PARSER IS A PIECE OF JUNK.  IT REALLY NEEDS TO BE
    //    REDONE IN BISON.  MAYBE FOR V 2.0?
    //
    //    HACK ALERT HACK ALERT HACK ALERT

    if (internal_trace)
        fprintf(stderr, "Starting phase 2 of microcompile.\n");

    stmtnum = 0;
    sindex = 0;
    bracketlevel = 0;


    //    Since we don't know how big the stmt_table actually is,
    //    we go through it once, looking for the NULL sentinel statement.
    //    This tells us how many entries there are; we also set up the
    //    namelens for the statement types.
    //
    //    [i_a] ^^^ nonsense. We know. WIDTHOF().
    //
    //    for (stab_idx = 0; stmt_table[stab_idx].stmt_name != NULL; stab_idx++)
    //    {
    //        if (stmt_table[stab_idx].namelen == 0)
    //            stmt_table[stab_idx].namelen = strlen(stmt_table[stab_idx].stmt_name);
    //    }
    /* stab_max = stab_idx; */

    //
    //    now the statement table should be set up.

    while (stmtnum <= csl->nstmts && sindex < pgmlength)
    {
        int stab_stmtcode;
        int argc;

        // int stab_done;
        int stab_index = 0;

        //      the strcspn below will fail if there's an unescaped
        //      semicolon embedded in a string (or, for that matter, an
        //      explicit newline).  Fortunately, the preprocessor fixes the
        //      former and the latter is explicitly prohibited by the language.
        //
        slength = (int)strcspn(&pgmtext[sindex], "\r\n");

        CRM_ASSERT(stmtnum < csl->mct_size);
        // allocate and fill in the mct table entry for this line
        csl->mct[stmtnum]->hosttxt = pgmtext;
#if !FULL_PARSE_AT_COMPILE_TIME
        csl->mct[stmtnum]->apb = NULL;
#else
#endif
        csl->mct[stmtnum]->start = sindex;
        csl->mct[stmtnum + 1]->start = sindex + slength + 1;
        csl->mct[stmtnum]->stmt_utime = 0;
        csl->mct[stmtnum]->stmt_stime = 0;
        csl->mct[stmtnum]->stmt_exec_count = 0;
        csl->mct[stmtnum]->stmt_type = CRM_BOGUS;
        csl->mct[stmtnum]->stmt_def = &stmt_table[0];
        csl->mct[stmtnum]->nest_level = bracketlevel;
        csl->mct[stmtnum]->fail_index = -1;
        csl->mct[stmtnum]->liaf_index = -1;
        csl->mct[stmtnum]->trap_index = 0; // -1;
        csl->mct[stmtnum]->stmt_break = 0;
        csl->cstmt = stmtnum;
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        csl->cstmt_recall = stmtnum;
        csl->next_stmt_due_to_fail = -1;
        csl->next_stmt_due_to_trap = -1;
        csl->next_stmt_due_to_jump = -1;
#endif

        //  skip nbindex to the first nonblank
        //  GROT GROT GROT  here we define nonblank as values > 0x21
        //  GROT GROT GROT  which absolutely _sucks_ in terms of coding
        //  GROT GROT GROT  portability, but it's what we have.
        nbindex = sindex;
#if 0
        while (pgmtext[nbindex] < 0x021 && nbindex < slength + sindex)
            nbindex++;

        //  save up the first nonblank char:
        csl->mct[stmtnum]->fchar = nbindex;

        // and set up the start of arguments as well, they start at the first
        // nonblank after the first blank after the command...

        aindex = nbindex;
        while (pgmtext[aindex] > 0x021 && aindex < slength + sindex)
            aindex++;
        nblength = aindex - nbindex;

        while (pgmtext[aindex] < 0x021 && aindex < slength + sindex)
            aindex++;
#else
        // skip any leading whitespace
        nbindex = skip_blanks(pgmtext, nbindex, slength + sindex);

        //  save up the first nonblank char:
        csl->mct[stmtnum]->fchar = nbindex;

        // skip leading comments too!
        aindex = skip_comments_and_blanks(pgmtext, nbindex, slength + sindex);
        if (aindex < slength + sindex)
        {
            // only skip LEADING comments if there's ANYTHING FOLLOWING it!
            nbindex = aindex;
        }

        // and set up the start of arguments as well, they start at the first
        // nonblank after the first blank after the command...

        aindex = skip_command_token(pgmtext, nbindex, slength + sindex);
        nblength = aindex - nbindex;

        aindex = skip_blanks(pgmtext, aindex, slength + sindex);
#endif

        csl->mct[stmtnum]->achar = aindex;

        //    We can now sweep thru the statement archetype table from 0
        //    to stab_max and compare the strlens and strings themselves.
        //
        //stab_done = 0;
        stab_stmtcode = CRM_BOGUS;

        if (nblength == 0)
        {
            //                    Empty lines are noops.

            //stab_done = 1;
            stab_stmtcode = CRM_NOOP;
        }
        else if (pgmtext[nbindex] == '#')
        {
            //                            Comment lines are also NOOPS

            //stab_done = 1;
            stab_stmtcode = CRM_NOOP;
        }
        else if (pgmtext[nbindex] == ':'
                && pgmtext[nbindex + nblength - 1] == ':')
        {
            //                             :LABEL: lines get special treatment

            stab_index = 1;
            stab_stmtcode = CRM_LABEL;
            CRM_ASSERT(stmt_table[stab_index].stmt_code == stab_stmtcode);
            k = (int)strcspn(&pgmtext[nbindex + 1], ":");
            crm_setvar(NULL, NULL, -1, pgmtext, nbindex, k + 2,                    NULL, 0, 0,  stmtnum, -1);
        }
#if 0
        else if (strncasecmp(&pgmtext[nbindex], "insert=", 7) == 0)
        {
            //                 INSERTs get special handling (NOOPed..)

            //stab_done = 1;
            stab_stmtcode = CRM_NOOP;
        }
#endif
        else
        {
            /* i = -1; */

            // make sure we can detect/fail if we hit some unidentified/unsupported command
            stab_stmtcode = CRM_UNIMPLEMENTED;

            //                      Now a last big loop for the rest of the stmts.
            for (i = 0; i < WIDTHOF(stmt_table); i++)
            {
                if (nblength == stmt_table[i].namelen
                   &&  strncasecmp(&pgmtext[nbindex],
                                   stmt_table[i].stmt_name,
                                   nblength) == 0)
                {
                    /* stab_done = 1; */
                    stab_stmtcode = stmt_table[i].stmt_code;
                    stab_index = i;
                    //   Deal with executable statements and WINDOW
                    if (stab_stmtcode == CRM_WINDOW && !seenaction)
                        csl->preload_window = 0;
                    //   and mark off the executable statements
                    if (stmt_table[i].is_executable)
                        seenaction = 1;
                    break;
                }
                //if (i >= stab_max)
                //  stab_done = 1;
            }

            // GROT GROT GROT [i_a] this parser should really be adapted so it
            // can better check for syntax errors at the location where such happen.
            //
            // Here's an example which could use some additional checks.
            switch (stab_stmtcode)
            {
            default:
                break;

            case CRM_LIAF:
            case CRM_FAIL:
            case CRM_ALIUS:
                // these commands MUST reside within a {} scope block to work as expected.
                if (bracketlevel <= 0)
                {
                    fatalerror_ex(SRC_LOC(),
                                  "Your program doesn't have a { } bracket-group surrounding the '%s' command. "
                                  "Check your source code.",
                                  stmt_table[stab_index].stmt_name);
                }
                break;
            }
            switch (stab_stmtcode)
            {
            default:
                break;

            case CRM_ALIUS:
                {
                    // make sure a {} precedes this opcode
                    int previdx;
                    int hit_closing_brace = 0;

                    for (previdx = stmtnum - 1; previdx >= 0; previdx--)
                    {
                        // accept any preceding comments before we've got to hit the mandatory closing brace before this statement
                        switch (csl->mct[previdx]->stmt_type)
                        {
                        case CRM_CLOSEBRACKET:
                            hit_closing_brace = 1;
                            break;

                        default:
                            // do NOT accept any other opcodes, except a very few special ones...
                            break;

                        case CRM_NOOP:
                            // comment: that's OK
                            continue;

                            // case CRM_LABEL:  -- would that be acceptable too? I don't think so.
                        }
                        break;
                    }

                    CRM_ASSERT(i >= 0);
                    CRM_ASSERT(i < WIDTHOF(stmt_table));
                    if (!hit_closing_brace)
                    {
                        fatalerror_ex(SRC_LOC(),
                                      "Your program doesn't have a { } bracket-group preceding the '%s' command. "
                                      "Check your source code.",
                                      stmt_table[stab_index].stmt_name);
                    }
                }
                break;
            }
        }

        // check for errors, don't wait until running the exec_engine to catch them!
        if (stab_stmtcode == CRM_UNIMPLEMENTED)
        {
            int width = CRM_MIN(1024, nblength);

            fatalerror_ex(SRC_LOC(),
                          "Statement %d(%s) NOT YET IMPLEMENTED !!! Check your source code. "
                          "Here's the text:\n%.*s%s",
                          csl->cstmt,
                          (csl->filename ? csl->filename : "\?\?\?"),
                          width,
                          &pgmtext[nbindex],
                          (nblength > width
                           ? "(...truncated)"
                           : "")
                         );
        }

        // [i_a] extension: HIDDEN_DEBUG_FAULT_REASON_VARNAME keeps track of the last error/nonfatal/whatever error report:
        if (stab_stmtcode == CRM_DEBUG)
        {
            // but only when we're running the debugger or _expect_ to run the debugger!
            if (debug_countdown <= DEBUGGER_DISABLED_FOREVER)
            {
                CRM_ASSERT(DEBUGGER_DISABLED_FOREVER + 1 < -1);
                debug_countdown = DEBUGGER_DISABLED_FOREVER + 1;                 // special signal: debugger disabled... for now.

                // and make sure the variable exists...
                crm_set_temp_var(HIDDEN_DEBUG_FAULT_REASON_VARNAME, "", -1, 0);
            }
        }

        //            Fill in the MCT entry with what we've learned.
        //
        csl->mct[stmtnum]->stmt_type = stab_stmtcode;
        CRM_ASSERT(stab_index >= 0);
        CRM_ASSERT(stab_index < WIDTHOF(stmt_table));
        csl->mct[stmtnum]->stmt_def = &stmt_table[stab_index];

        if (stab_stmtcode == CRM_OPENBRACKET)
        {
            bracketlevel++;
            if (internal_trace)
            {
                fprintf(stderr, "Open brace at stmt %d: bracket level = %d at line '%.*s'\n",
                        stmtnum, bracketlevel,
                        slength + 256, pgmtext + sindex);
            }
        }
        else if (stab_stmtcode == CRM_CLOSEBRACKET)
        {
            bracketlevel--;
            if (internal_trace)
            {
                fprintf(stderr, "Closing brace at stmt %d: bracket level = %d at line '%.*s'\n",
                        stmtnum, bracketlevel,
                        slength + 256, pgmtext + sindex);
            }
            // hack - reset the bracketlevel here, as a bracket is "outside"
            //  the nestlevel, not inside.
            csl->mct[stmtnum]->nest_level = bracketlevel;
        }

        // now we know where the line ends and where the arguments start: parse 'em all.
        //
        // We've delayed parsing the args so we could validate their counts and all
        // as we have that bit from the command match above: csl->mct[stmtnum]->stmt_def !
#if FULL_PARSE_AT_COMPILE_TIME
        switch (stab_stmtcode)
        {
        case CRM_BOGUS:
        case CRM_NOOP:
            break;

        default:
            // because the way crm_statement_parse() works, we must include the command itself;
            // this will be changed lateron when we overhaul the preproc+compiler as this means
            // we are scanning extra bytes uselessly here.
            argc = crm_statement_parse(pgmtext + nbindex /* aindex */,
                                       slength + sindex - nbindex /* aindex */,
                                       csl->mct[stmtnum],
                                       &csl->mct[stmtnum]->apb);
            break;
        }
#endif

        if (0)         // (internal_trace)
        {
            fprintf(stderr, "\nStmt %3d type %2d ",
                    stmtnum, csl->mct[stmtnum]->stmt_type);
            fwrite_ASCII_Cfied(stderr,
                               pgmtext + csl->mct[stmtnum]->start,
                               (csl->mct[stmtnum + 1]->start - 1) - csl->mct[stmtnum]->start);
        }

#ifdef STAB_TEST
        if (stab_stmtcode != csl->mct[stmtnum]->stmt_type)
        {
            fprintf(stderr, "Darn!  Microcompiler stab error (not your fault!)\n"
                            "Please file a bug report if you can.  The data is:\n");
            fprintf(stderr,
                    "Stab got %d, Ifstats got %d, on line %d with len %d\n",
                    stab_stmtcode,
                    csl->mct[stmtnum]->stmt_type,
                    stmtnum,
                    nblength);
            fprintf(stderr, "String was >>>");
            fwrite4stdio(&pgmtext[nbindex], nblength, stderr);
            fprintf(stderr, "<<<\n\n");
        }
#endif

        //    check for bracket level underflow....
        if (bracketlevel < 0)
        {
            fatalerror(" Your program seems to achieve a negative nesting",
                       "level, which is quite likely bogus.");
        }

        // move on to the next statement
        sindex = sindex + slength;

        if (sindex < pgmlength)
        {
            // skip the newline now:
            // (this is not nice and should be solved in a better way in the new compiler...)
            if (pgmtext[sindex] == '\r' && (sindex + 1 < pgmlength) && pgmtext[sindex + 1] == '\n')
            {
                sindex += 2;
            }
            else if (pgmtext[sindex] == '\r' || pgmtext[sindex] == '\n')
            {
                sindex++;
            }
        }
        stmtnum++;
    }

    numstmts = stmtnum - 1;
    CRM_ASSERT(numstmts <= csl->mct_size);


    if (internal_trace)
    {
        fprintf(stderr, "\nCompiled program listing:\n");
        for (stmtnum = 0; stmtnum <= numstmts; stmtnum++)
        {
            const STMT_TABLE_TYPE *stmt_def;
            ARGPARSE_BLOCK *apb;

            fprintf(stderr, "  stmt %4d: type %3d, ",
                    stmtnum, csl->mct[stmtnum]->stmt_type);
            stmt_def = csl->mct[stmtnum]->stmt_def;
            if (stmt_def)
            {
                fprintf(stderr, "command: '%-16s', exec: %s, nonstd: %s, ",
                        (stmt_def->stmt_name[0] != '\n' ? stmt_def->stmt_name : "---"),
                        (stmt_def->is_executable ? "Y" : "n"),
                        (stmt_def->has_non_standard_flags ? "Y" : "n"));
            }
            fprintf(stderr, "nest level: %3d, fail index: %4d, liaf_index: %4d, stmt_break: %4d, trap_line: %4d\n",
                    csl->mct[stmtnum]->nest_level,
                    csl->mct[stmtnum]->fail_index,
                    csl->mct[stmtnum]->liaf_index,
                    csl->mct[stmtnum]->stmt_break,
                    csl->mct[stmtnum]->trap_index);
            fprintf(stderr, "    code: ");
            fwrite_ASCII_Cfied(stderr,
                               csl->mct[stmtnum]->hosttxt + csl->mct[stmtnum]->start,
                               (csl->mct[stmtnum + 1]->start - 1) - csl->mct[stmtnum]->start);
            apb = &csl->mct[stmtnum]->apb;

            fprintf(stderr, "\n      args:\n");
            if (apb->a1len)
                fprintf(stderr, "        angles 1:      <%.*s>\n", apb->a1len, apb->a1start);
            if (apb->b1len)
                fprintf(stderr, "        boxes 1:       [%.*s]\n", apb->b1len, apb->b1start);
            if (apb->p1len)
                fprintf(stderr, "        parentheses 1: (%.*s)\n", apb->p1len, apb->p1start);
            if (apb->p2len)
                fprintf(stderr, "        parentheses 2: (%.*s)\n", apb->p2len, apb->p2start);
            if (apb->p3len)
                fprintf(stderr, "        parentheses 3: (%.*s)\n", apb->p3len, apb->p3start);
            if (apb->s1len)
                fprintf(stderr, "        slashes 1:     /%.*s/\n", apb->s1len, apb->s1start);
            if (apb->s2len)
                fprintf(stderr, "        slashes 2:     /%.*s/\n", apb->s2len, apb->s2start);
        }
    }

    //  check to be sure that the brackets close!

    if (bracketlevel != 0)
    {
        fatalerror("\nDang!  The curly braces don't match up!\n",
                   "Check your source code. ");
    }


    //  Phase 3 of microcompiler- set FAIL and LIAF targets for each line
    //  in the MCT.

    {
        int stack[MAX_BRACKETDEPTH];
        int sdx;

        if (internal_trace)
            fprintf(stderr, "Starting phase 3 of microcompile.\n");

        //  set initial stack values
        sdx = 0;
        stack[sdx] = 0;

        //   Work downwards first, assigning LIAF targets
        for (stmtnum = 0; stmtnum <= numstmts; stmtnum++)
        {
            switch (csl->mct[stmtnum]->stmt_type)
            {
            case  CRM_OPENBRACKET:
                {
                    //  if we're open bracket, we're the new LIAF target,
                    //  but we ourselves LIAF to the previous open bracket.
                    csl->mct[stmtnum]->liaf_index = stack[sdx];
                    sdx++;
                    stack[sdx] = stmtnum;
                }
                break;

            case CRM_CLOSEBRACKET:
                {
                    //  if we're a close bracket, we LIAF not to the current
                    //  open bracket, but to the one before it, so pop the
                    //  stack and LIAF there.
                    sdx--;
                    CRM_ASSERT(sdx >= 0);
                    csl->mct[stmtnum]->liaf_index = stack[sdx];
                }
                break;

            default:
                {
                    //   Most statements use the current liaf
                    csl->mct[stmtnum]->liaf_index = stack[sdx];
                }
                break;
            }
        }

        //   Work upwards next, assigning the fail targets
        sdx = 0;
        // [i_a] point the fail target at the last statement in the program. This statement MUST be EMPTY.
        stack[sdx] = numstmts;
        CRM_ASSERT(numstmts <= csl->mct_size);
        for (stmtnum = numstmts; stmtnum >= 0; stmtnum--)
        {
            switch (csl->mct[stmtnum]->stmt_type)
            {
            case  CRM_CLOSEBRACKET:
                {
                    //  if we're close bracket, we're the new FAIL target,
                    //  but we ourselves FAIL to the next close bracket
                    csl->mct[stmtnum]->fail_index = stack[sdx];
                    sdx++;
                    stack[sdx] = stmtnum;
                }
                break;

            case CRM_OPENBRACKET:
                {
                    //  if we're an open bracket, we FAIL not to the current
                    //  CLOSE bracket, but to the one before it, so pop the
                    //  stack and FAIL there.
                    sdx--;
                    CRM_ASSERT(sdx >= 0);
                    csl->mct[stmtnum]->fail_index = stack[sdx];
                }
                break;

            default:
                {
                    //   Most statements use the current liaf
                    csl->mct[stmtnum]->fail_index = stack[sdx];
                }
                break;
            }
        }


        //   Work upwards again, assigning the TRAP targets
        sdx = 0;
        // [i_a] point the trap target at the last statement in the program. This statement MUST be EMPTY.
        stack[sdx] = numstmts;
        CRM_ASSERT(numstmts <= csl->mct_size);
        for (stmtnum = numstmts; stmtnum >= 0; stmtnum--)
        {
            switch (csl->mct[stmtnum]->stmt_type)
            {
            case  CRM_TRAP:
                {
                    //  if we're the TRAP statement, we change the TRAP target,
                    //  but we ourselves TRAP to the next TRAP statement
                    csl->mct[stmtnum]->trap_index = stack[sdx];
                    stack[sdx] = stmtnum;
                }
                break;

            case CRM_OPENBRACKET:
                {
                    //  if we're an open bracket, we trap not to the current
                    //  level's TRAP statement, but to the one before it, so pop the
                    //  stack and aim TRAP there.
                    sdx--;
                    CRM_ASSERT(sdx >= 0);
                    csl->mct[stmtnum]->trap_index = stack[sdx];
                }
                break;

            case CRM_CLOSEBRACKET:
                {
                    //  if we're a close bracket, we keep our current trap target
                    //  but move down one level in the stack
                    stack[sdx + 1] = stack[sdx];
                    sdx++;
                    csl->mct[stmtnum]->trap_index = stack[sdx];
                }
                break;

            default:
                {
                    //   Most statements use the current TRAP level
                    csl->mct[stmtnum]->trap_index = stack[sdx];
                }
                break;
            }
        }

        //  print out statement info if desired
        if (prettyprint_listing > 0)
        {
            for (stmtnum = 0; stmtnum <= numstmts; stmtnum++)
            {
                fprintf(stderr, "\n");
                if (prettyprint_listing > 1)
                    fprintf(stderr, "%4.4d ", stmtnum);
                if (prettyprint_listing > 2)
                    fprintf(stderr, "{%2.2d}", csl->mct[stmtnum]->nest_level);

                if (prettyprint_listing > 3)
                {
                    fprintf(stderr, " <<%2.2d>>",
                            csl->mct[stmtnum]->stmt_type);

#if 0
                    fprintf(stderr, " L%4.4d F%4.4d T%4.4d",
                            csl->mct[stmtnum]->liaf_index,
                            csl->mct[stmtnum]->fail_index,
                            csl->mct[stmtnum]->trap_index);
#else
                    if (csl->mct[stmtnum]->liaf_index != -1)
                    {
                        fprintf(stderr, " L%4.4d",
                                csl->mct[stmtnum]->liaf_index);
                    }
                    else
                    {
                        fprintf(stderr, " L----");
                    }
                    if (csl->mct[stmtnum]->fail_index != -1)
                    {
                        fprintf(stderr, " F%4.4d",
                                csl->mct[stmtnum]->fail_index);
                    }
                    else
                    {
                        fprintf(stderr, " F----");
                    }
                    if (csl->mct[stmtnum]->trap_index != -1)
                    {
                        fprintf(stderr, " T%4.4d",
                                csl->mct[stmtnum]->trap_index);
                    }
                    else
                    {
                        fprintf(stderr, " T----");
                    }
#endif
                }
                if (prettyprint_listing > 1)
                    fprintf(stderr, " :  ");

                //  space over two spaces per indent
                for (k = 0; k < csl->mct[stmtnum]->nest_level; k++)
                    fprintf(stderr, "  ");

                //   print out text of the first statement:
                if (prettyprint_listing > 4)
                    fprintf(stderr, "-");

                k = csl->mct[stmtnum]->fchar;
                k = skip_command_token(pgmtext, k, csl->mct[stmtnum]->achar);
                fwrite_ASCII_Cfied(stderr,
                                   pgmtext + csl->mct[stmtnum]->fchar,
                                   k - csl->mct[stmtnum]->fchar);

                if (prettyprint_listing > 4)
                    fprintf(stderr, "-");

                fprintf(stderr, " ");
                //  and if there are args, print them out as well.
                if (csl->mct[stmtnum]->achar < csl->mct[stmtnum + 1]->start - 1)
                {
                    if (prettyprint_listing > 4)
                        fprintf(stderr, "=");
                    fwrite_ASCII_Cfied(stderr,
                                       pgmtext + csl->mct[stmtnum]->achar,
                                       (csl->mct[stmtnum + 1]->start - 1) - csl->mct[stmtnum]->achar);
                    if (prettyprint_listing > 4)
                        fprintf(stderr, "=");
                }
            }
            fprintf(stderr, "\n");
        }

        //    Finally got to the end.  Fill in the last bits of the CSL
        //  with the new information, and return.

        CRM_ASSERT(numstmts <= csl->mct_size);
        csl->nstmts = numstmts;

        if (internal_trace)
            fprintf(stderr, "microcompile completed\n");
    }

    if (internal_trace)
    {
        fprintf(stderr, "\nCompiled program listing (COMPLETED):\n");
        for (stmtnum = 0; stmtnum <= numstmts; stmtnum++)
        {
            const STMT_TABLE_TYPE *stmt_def;
            ARGPARSE_BLOCK *apb;

            fprintf(stderr, "  stmt %4d: type %3d, ",
                    stmtnum, csl->mct[stmtnum]->stmt_type);
            stmt_def = csl->mct[stmtnum]->stmt_def;
            if (stmt_def)
            {
                fprintf(stderr, "command: '%-16s', exec: %s, nonstd: %s, ",
                        (stmt_def->stmt_name[0] != '\n' ? stmt_def->stmt_name : "---"),
                        (stmt_def->is_executable ? "Y" : "n"),
                        (stmt_def->has_non_standard_flags ? "Y" : "n"));
            }
            fprintf(stderr, "nest level: %3d, fail index: %4d, liaf_index: %4d, stmt_break: %4d, trap_line: %4d\n",
                    csl->mct[stmtnum]->nest_level,
                    csl->mct[stmtnum]->fail_index,
                    csl->mct[stmtnum]->liaf_index,
                    csl->mct[stmtnum]->stmt_break,
                    csl->mct[stmtnum]->trap_index);
            fprintf(stderr, "    code: ");
            fwrite_ASCII_Cfied(stderr,
                               csl->mct[stmtnum]->hosttxt + csl->mct[stmtnum]->start,
                               (csl->mct[stmtnum + 1]->start - 1) - csl->mct[stmtnum]->start);
            apb = &csl->mct[stmtnum]->apb;

            fprintf(stderr, "\n      args:\n");
            if (apb->a1len)
                fprintf(stderr, "        angles 1:      <%.*s>\n", apb->a1len, apb->a1start);
            if (apb->b1len)
                fprintf(stderr, "        boxes 1:       [%.*s]\n", apb->b1len, apb->b1start);
            if (apb->p1len)
                fprintf(stderr, "        parentheses 1: (%.*s)\n", apb->p1len, apb->p1start);
            if (apb->p2len)
                fprintf(stderr, "        parentheses 2: (%.*s)\n", apb->p2len, apb->p2start);
            if (apb->p3len)
                fprintf(stderr, "        parentheses 3: (%.*s)\n", apb->p3len, apb->p3start);
            if (apb->s1len)
                fprintf(stderr, "        slashes 1:     /%.*s/\n", apb->s1len, apb->s1start);
            if (apb->s2len)
                fprintf(stderr, "        slashes 2:     /%.*s/\n", apb->s2len, apb->s2start);
        }
    }

    return 0;
}

