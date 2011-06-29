//  testvectok.c  - Controllable Regex Mutilator,  version v1.0
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

//  include the routine declarations file
#include "crm114.h"

//  and include OSBF declarations
#include "crm114_osbf.h"

//
//    Global variables


/* [i_a] no variable instantiation in a common header file */
long vht_size = 0;

long cstk_limit = 0;

long max_pgmlines = 0;

long max_pgmsize = 0;

long user_trace = 0;

long internal_trace = 0;

int debug_countdown = 0;

long cmdline_break = 0;

long cycle_counter = 0;

long ignore_environment_vars = 0;

long data_window_size = 0;

long sparse_spectrum_file_length = 0;

long microgroom_chain_length = 0;

long microgroom_stop_after = 0;

double min_pmax_pmin_ratio = 0.0;

long profile_execution = 0;

long prettyprint_listing = 0;  //  0= none, 1 = basic, 2 = expanded, 3 = parsecode

long engine_exit_base = 0;  //  All internal errors will use this number or higher;
//  the user programs can use lower numbers freely.


//        how should math be handled?
//        = 0 no extended (non-EVAL) math, use algebraic notation
//        = 1 no extended (non-EVAL) math, use RPN
//        = 2 extended (everywhere) math, use algebraic notation
//        = 3 extended (everywhere) math, use RPN
long q_expansion_mode = 0;

int selected_hashfunction = 0;  //  0 = default





//   The VHT (Variable Hash Table)
VHT_CELL **vht = NULL;

//   The pointer to the global Current Stack Level (CSL) frame
CSL_CELL *csl = NULL;

//    the data window
CSL_CELL *cdw = NULL;

//    the temporarys data window (where argv, environ, newline etc. live)
CSL_CELL *tdw = NULL;

//    the pointer to a CSL that we use during matching.  This is flipped
//    to point to the right data window during matching.  It doesn't have
//    it's own data, unlike cdw and tdw.
CSL_CELL *mdw = NULL;

////    a pointer to the current statement argparse block.  This gets whacked
////    on every new statement.
//ARGPARSE_BLOCK *apb = NULL;




//    the command line argc, argv
int prog_argc = 0;
char **prog_argv = NULL;

//    the auxilliary input buffer (for WINDOW input)
char *newinputbuf = NULL;

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
char *inbuf = NULL;
char *outbuf = NULL;
char *tempbuf = NULL;





// from crm_vector_tokenize.c
int main(void)
{
    char input[1024];
    int i, j;
    int ret;
    int k;
    crmhash_t feavec[2048];

    char my_regex[256];

    static const crmhash_t coeff[] =
    {
        1, 3, 0, 0, 0
        , 1, 0, 5, 0, 0
        , 1, 0, 0, 11, 0
        , 1, 0, 0, 0, 23
    };

    stdout = os_stdout();
    stderr = os_stderr();
    stdin = os_stdin();

    strcpy(my_regex, "[[:alpha:]]+");
    fprintf(stdout, "Enter a test string: ");
    fgets(input, sizeof(input), stdin);
    input[sizeof(input) - 1] = 0;
    fprintf(stdout, "Input = '%s'\n", input);
    // fscanf(stdin, "%1023s", input);
    // fprintf(stdout, "Input = '%s'\n", input);

    ret = crm_vector_tokenize(
            input
                             , strlen(input)
                             , 0
                             , my_regex
                             , strlen(my_regex)
                             , coeff
                             , 5
                             , 4
                             , feavec
                             , 2048
                             , 1
                             , &j
                             , &i);

    for (k = 0; k < j; k++)
    {
        fprintf(stdout, "feature[%4d] = %12ld (%08lX)\n", k, (long)feavec[k], (long)feavec[k]);
    }

    fprintf(stdout, "... and next_offset is %d\n", i);
    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}







// just copied here to make this bugger compile ASAP:


void free_stack_item(CSL_CELL *csl)
{
    if (!csl)
        return;

    if (csl->mct && csl->mct_allocated)
    {
        int i;

        for (i = 0; i < csl->mct_size; i++)
        {
            MCT_CELL *cp = csl->mct[i];

            if (cp != NULL)
            {
                free(cp->apb);
                cp->apb = NULL;
                // free(cp->hosttxt);
                free(cp);
                csl->mct[i] = NULL;
            }
        }
        free(csl->mct);
        csl->mct = NULL;
    }

    if (csl->filename_allocated)
    {
        free(csl->filename);
    }
    csl->filename = NULL;
    if (csl->filetext_allocated)
    {
        free(csl->filetext);
    }
    csl->filetext = NULL;
    free(csl);
}

