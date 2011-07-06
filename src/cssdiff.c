//	cssutil.c - utility for munging css files, version X0.1

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files

#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"





//
//    Global variables



/* [i_a] no variable instantiation in a common header file */
int vht_size = 0;

int cstk_limit = 0;

int max_pgmlines = 0;

int max_pgmsize = 0;

int user_trace = 0;

int internal_trace = 0;

int debug_countdown = 0;

int cmdline_break = 0;

int cycle_counter = 0;

int ignore_environment_vars = 0;

int data_window_size = 0;

int sparse_spectrum_file_length = 0;

int microgroom_chain_length = 0;

int microgroom_stop_after = 0;

double min_pmax_pmin_ratio = 0.0;

int profile_execution = 0;

int prettyprint_listing = 0;  //  0= none, 1 = basic, 2 = expanded, 3 = parsecode

int engine_exit_base = 0;  //  All internal errors will use this number or higher;
//  the user programs can use lower numbers freely.


//        how should math be handled?
//        = 0 no extended (non-EVAL) math, use algebraic notation
//        = 1 no extended (non-EVAL) math, use RPN
//        = 2 extended (everywhere) math, use algebraic notation
//        = 3 extended (everywhere) math, use RPN
int q_expansion_mode = 0;

int selected_hashfunction = 0;  //  0 = default

int act_like_Bill = 0;




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




//    the app path/name
char *prog_argv0 = NULL;

//    the auxilliary input buffer (for WINDOW input)
char *newinputbuf = NULL;

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
char *inbuf = NULL;
char *outbuf = NULL;
char *tempbuf = NULL;


#if !defined(CRM_WITHOUT_BMP_ASSISTED_ANALYSIS)
CRM_ANALYSIS_PROFILE_CONFIG analysis_cfg = { 0 };
#endif /* CRM_WITHOUT_BMP_ASSISTED_ANALYSIS */







static char version[] = "1.2";


static void helptext(void)
{
    fprintf(stderr, " This is csdiff, version %s\n", version);
    fprintf(stderr, " Copyright 2001-2007 W.S.Yerazunis.\n");
    fprintf(stderr,
            " This software is licensed under the GPL with ABSOLUTELY NO WARRANTY\n");
    fprintf(stderr, "Usage: cssdiff <cssfile1> <cssfile2>\n");
}






int main(int argc, char **argv)
{
    int i, j, k; //  some random counters, when we need a loop
    int hfsize1, hfsize2;

    int f1, f2;
    int sim, diff, dom1, dom2, hclash, kclash;

    int opt;

    init_stdin_out_err_as_os_handles();

    //   copy app path/name into global static...
    prog_argv0 = argv[0];


    user_trace = DEFAULT_USER_TRACE_LEVEL;
    internal_trace = DEFAULT_INTERNAL_TRACE_LEVEL;

    {
        struct stat statbuf;                  //  filestat buffer
        FEATUREBUCKET_STRUCT *h1, *h2;          //  the text of the hash file

        // parse cmdline options
        while ((opt = getopt(argc, argv, "v")) != -1)
        {
            switch (opt)
            {
            case 'v':
            default:
                helptext();
                exit(EXIT_SUCCESS);
                break;
            }
        }

        if (optind >= argc)
        {
            fprintf(stderr, "Error: missing both css file arguments.\n\n");
            helptext();
            exit(EXIT_FAILURE);
        }
        if (optind + 1 >= argc)
        {
            fprintf(stderr, "Error: missing second css file argument.\n\n");
            helptext();
            exit(EXIT_FAILURE);
        }


        //             quick check- does the first file even exist?
        k = stat(argv[optind], &statbuf);
        if (k != 0)
        {
            fprintf(stderr, "\n CSS file '%s' not found.\n", argv[optind]);
            exit(EXIT_FAILURE);
        }
        //
        hfsize1 = statbuf.st_size;
        //         mmap the hash file into memory so we can bitwhack it
        h1 = crm_mmap_file(argv[optind],
                           0,
                           hfsize1,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED,
                           CRM_MADV_RANDOM,
                           &hfsize1);
        if (h1 == MAP_FAILED)
        {
            fprintf(stderr, "\n MMAP failed on file %s\n",
                    argv[optind]);
            exit(EXIT_FAILURE);
        }
        hfsize1 = hfsize1 / sizeof(FEATUREBUCKET_STRUCT);

        //
        //  and repeat the process for the second file:
        k = stat(argv[optind + 1], &statbuf);
        //             quick check- does the file even exist?
        if (k != 0)
        {
            fprintf(stderr, "\n.CSS file '%s' not found.\n", argv[optind + 1]);
            exit(EXIT_FAILURE);
        }

        hfsize2 = statbuf.st_size;
        //         mmap the hash file into memory so we can bitwhack it
        h2 = crm_mmap_file(argv[optind + 1],
                           0,
                           hfsize2,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED,
                           CRM_MADV_RANDOM,
                           &hfsize2);
        if (h2 == MAP_FAILED)
        {
            fprintf(stderr, "\n MMAP failed on file %s\n",
                    argv[optind + 1]);
            exit(EXIT_FAILURE);
        }

        hfsize2 = hfsize2 / sizeof(FEATUREBUCKET_STRUCT);

        fprintf(stderr, "Sparse spectra file %s has %d bins total\n",
                argv[optind], hfsize1);


        fprintf(stdout, "Sparse spectra file %s has %d bins total\n",
                argv[optind + 1], hfsize2);

        //
        //
        if (hfsize1 != hfsize2)
        {
            fprintf(stderr,
                    "\n.CSS files %s, %s :\n lengths differ: %d vs %d.\n",
                    argv[optind], argv[optind + 1], hfsize1, hfsize2);
            fprintf(stderr, "\n This is not a fatal error, but be warned.\n");
        }

        f1 = 0;
        f2 = 0;
        sim  = 0;
        diff = 0;
        dom1 = 0;
        dom2 = 0;
        hclash = 0;
        kclash = 0;
        //
        //   The algorithm - for each file,
        //                      for each bucket in each file
        //                          find corresponding bucket in other file
        //                              increment dom1 or dom2 as appropriate
        //                              always increment sim and diff
        //                          end
        //                      end
        //                      divide sim and diff by 2, as they are doublecounted
        //                      print statistics and exit.
        //
        // start at 1 - no need to check bin 0 (version).
        for (i = 1; i < hfsize1; i++)
        {
            if (h1[i].key != 0)
            {
                f1 += h1[i].value;
                k = h1[i].hash % hfsize2;
                if (k == 0)
                    k = 1;
                while (h2[k].value != 0
                      && (h2[k].hash != h1[i].hash
                         || h2[k].key != h1[i].key))
                {
                    k++;
                    if (k >= hfsize2)
                        k = 1;
                }

                //   Now we've found the corresponding (or vacant) slot in
                //   h2.  Do our tallies...
                j = h1[i].value;
                if (j > h2[k].value)
                    j = h2[k].value;
                sim +=  j;

                j = h1[i].value - h2[k].value;
                if (j < 0)
                    j = -j;
                diff += j;

                j = h1[i].value - h2[k].value;
                if (j < 0)
                    j = 0;
                dom1 += j;
            }
        }
        //
        //      And repeat for file 2.
        for (i = 1; i < hfsize2; i++)
        {
            if (h2[i].key != 0)
            {
                f2 += h2[i].value;
                k = h2[i].hash % hfsize1;
                if (k == 0)
                    k = 1;
                while (h1[k].value != 0
                      && (h1[k].hash != h2[i].hash
                         || h1[k].key != h2[i].key))
                {
                    k++;
                    if (k >= hfsize1)
                        k = 1;
                }

                //   Now we've found the corresponding (or vacant) slot in
                //   h1.  Do our tallies...
                j = h2[i].value;
                if (j > h1[k].value)
                    j = h1[k].value;
                sim +=  j;

                j = h1[k].value - h2[i].value;
                if (j < 0)
                    j = -j;
                diff += j;

                j = h2[i].value - h1[k].value;
                if (j < 0)
                    j = 0;
                dom2 += j;
            }
        }

        fprintf(stdout, "\n File 1 total features            : %12d", f1);
        fprintf(stdout, "\n File 2 total features            : %12d\n", f2);

        fprintf(stdout, "\n Similarities between files       : %12d", sim / 2);
        fprintf(stdout, "\n Differences between files        : %12d\n", diff / 2);

        fprintf(stdout, "\n File 1 dominates file 2          : %12d", dom1);
        fprintf(stdout, "\n File 2 dominates file 1          : %12d\n", dom2);
    }
    return EXIT_SUCCESS;
}



// bogus code to make link phase happy while we are in limbo between obsoleting this tool and
// getting cssXXXX script commands working in crm114 itself.
void free_stack_item(CSL_CELL *csl)
{}



