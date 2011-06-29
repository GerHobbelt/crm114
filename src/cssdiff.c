//  cssutil.c - utility for munging css files, version X0.1
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.0.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org .  Other licenses may be negotiated; contact the
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


#include <getopt.h>



//
//    Global variables

int user_trace = 0;

int internal_trace = 0;

int engine_exit_base = 0;  //  All internal errors will use this number or higher;
//  the user programs can use lower numbers freely.

int selected_hashfunction = 0;  //  0 = default


//    the app path/name
char *prog_argv0 = NULL;







static char version[] = "1.2";


static void helptext(void)
{
    fprintf(stderr, " This is csdiff, version %s\n", version);
    fprintf(stderr, " Copyright 2001-2007 W.S.Yerazunis.\n");
    fprintf(stderr
           , " This software is licensed under the GPL with ABSOLUTELY NO WARRANTY\n");
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
        FEATUREBUCKET_TYPE *h1, *h2;          //  the text of the hash file

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
        h1 = crm_mmap_file(argv[optind]
                          , 0
                          , hfsize1
                          , PROT_READ | PROT_WRITE
                          , MAP_SHARED
                          , CRM_MADV_RANDOM
                          , &hfsize1);
        if (h1 == MAP_FAILED)
        {
            fprintf(stderr, "\n MMAP failed on file %s\n"
                   , argv[optind]);
            exit(EXIT_FAILURE);
        }
        hfsize1 = hfsize1 / sizeof(FEATUREBUCKET_TYPE);

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
        h2 = crm_mmap_file(argv[optind + 1]
                          , 0
                          , hfsize2
                          , PROT_READ | PROT_WRITE
                          , MAP_SHARED
                          , CRM_MADV_RANDOM
                          , &hfsize2);
        if (h2 == MAP_FAILED)
        {
            fprintf(stderr, "\n MMAP failed on file %s\n"
                   , argv[optind + 1]);
            exit(EXIT_FAILURE);
        }

        hfsize2 = hfsize2 / sizeof(FEATUREBUCKET_TYPE);

        fprintf(stderr, "Sparse spectra file %s has %d bins total\n"
               , argv[optind], hfsize1);


        fprintf(stdout, "Sparse spectra file %s has %d bins total\n"
               , argv[optind + 1], hfsize2);

        //
        //
        if (hfsize1 != hfsize2)
        {
            fprintf(stderr
                   , "\n.CSS files %s, %s :\n lengths differ: %d vs %d.\n"
                   , argv[optind], argv[optind + 1], hfsize1, hfsize2);
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

