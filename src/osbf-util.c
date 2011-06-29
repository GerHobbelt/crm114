//  osbf-util.c - utility for munging css files, version X0.1
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.0.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org .  Other licenses may be negotiated; contact the
//  author for details.
//
//  OBS: This program is a modified version of the original cssutil,
//       specific for the new osbf format. It is not compatible with
//       the original css format. -- Fidelis Assis
//
//  include some standard files

#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

#include "crm114_osbf.h"





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


#if !defined (CRM_WITHOUT_BMP_ASSISTED_ANALYSIS)
CRM_ANALYSIS_PROFILE_CONFIG analysis_cfg = { 0 };
#endif /* CRM_WITHOUT_BMP_ASSISTED_ANALYSIS */







static char version[] = "1.2";


void helptext(void)
{
    /* GCC warning: warning: string length <xxx> is greater than the length <509> ISO C89 compilers are required to support */
    fprintf(stdout,
            "osbf-util version %s - generic osbf file utility.\n"
            "Usage: osbfutil [options]... css-filename\n"
            "\n",
            VERSION);
    fprintf(stdout,
            "            -b   - brief; print only summary\n"
            "            -h   - print this help\n"
            "            -q   - quite mode; no warning messages\n"
            "            -r   - report then exit (no menu)\n"
            "            -s css-size  - if no css file found, create new\n"
            "                           one with this many buckets.\n"
            "            -S css-size  - same as -s, but round up to next\n"
            "                           2^n + 1 boundary.\n"
            "            -v   - print version and exit\n"
            "            -D   - dump css file to stdout in CSV format.\n"
            "            -R csv-file  - create and restore css from CSV.\n"
            "                           Options -s and -S are ignored when"
            "                           restoring.\n");
}

int main(int argc, char **argv)
{
    int i, k;                  //  some random counters, when we need a loop
    int v;
    int sparse_spectrum_file_length = OSBF_DEFAULT_SPARSE_SPECTRUM_FILE_LENGTH;
    int user_set_css_length = 0;
    int hfsize;
    int64_t sum;              // sum of the hits... can be _big_.
    int brief = 0, quiet = 0, dump = 0, restore = 0;
    int opt, fields;
    int report_only = 0;

    int *bcounts;
    int maxchain;
    int curchain;
    int totchain;
    int fbuckets;
    int nchains;
    int ofbins;

    char cmdstr[255];
    char cssfile[255];
    char csvfile[255];
    unsigned char cmdchr[2];
    char crapchr[2];
    double cmdval;
    int zloop, cmdloop, version_index;

    //    the following for crm114.h's happiness

    init_stdin_out_err_as_os_handles();

    //   copy app path/name into global static...
    prog_argv0 = argv[0];

    user_trace = DEFAULT_USER_TRACE_LEVEL;
    internal_trace = DEFAULT_INTERNAL_TRACE_LEVEL;

    bcounts = calloc(OSBF_FEATUREBUCKET_VALUE_MAX, sizeof(bcounts[0]));

    {
        struct stat statbuf;                //  filestat buffer
        OSBF_FEATURE_HEADER_STRUCT *header; //  the header of the hash file
        OSBF_FEATUREBUCKET_STRUCT *hashes;  //  the text of the hash file

        // parse cmdline options
        while ((opt = getopt(argc, argv, "bDhR:rqs:S:vtT")) != -1)
        {
            switch (opt)
            {
            case 'b':
                brief = 1;      // brief, no 'bin value ...' lines
                break;

            case 'D':
                dump = 1;       // dump css file, no cmd menu
                break;

            case 'q':
                quiet = 1;      // quiet mode, no warning messages
                break;

            case 'R':
                {
                    FILE *f;
                    unsigned int key, hash, value;
                    OSBF_FEATURE_HEADER_STRUCT h;

                    // count lines to determine the number of buckets and check CSV format
                    if (user_trace)
                        fprintf(stderr, "Opening OSBF file %s for read\n", optarg);
                    if ((f = fopen(optarg, "rb")) != NULL)
                    {
                        // try to find the header reading first 2 "buckets"
                        if (fscanf
                                        (f, "%u;%u;%u\n", (unsigned int *)h.version,
                                        &(h.flags), &(h.buckets_start)) != 3)
                        {
                            fprintf(stderr,
                                    "\n %s is not in the right CSV format.\n",
                                    optarg);
                            exit(EXIT_FAILURE);
                        }
                        if (*((unsigned int *)h.version) != OSBF_VERSION)
                        {
                            fprintf(stderr,
                                    "\n %s is not an OSBF CSV file.\n", optarg);
                            fclose(f);
                            exit(EXIT_FAILURE);
                        }
                        if (fscanf(f, "%u;%u;%u\n", &(h.buckets), &hash, &value)
                            != 3)
                        {
                            fprintf(stderr,
                                    "\n %s is not in the right CSV format.\n",
                                    optarg);
                            exit(EXIT_FAILURE);
                        }

                        // start with -headersize buckets, discounting 2 "buckets" alread read
                        sparse_spectrum_file_length = 2 - h.buckets_start;

                        while (!feof(f))
                            if (fscanf(f, "%u;%u;%u\n", &key, &hash, &value) == 3)
                                sparse_spectrum_file_length++;
                            else
                            {
                                fprintf(stderr,
                                        "\n %s is not in the right CSV format.\n",
                                        optarg);
                                exit(EXIT_FAILURE);
                            }
                        fclose(f);

                        // check the number of buckets
                        if (sparse_spectrum_file_length != h.buckets)
                        {
                            fprintf(stderr,
                                    "\n Wrong number of buckets! %s is not in the right CSV format.\n",
                                    optarg);
                            exit(EXIT_FAILURE);
                        }
                        strcpy(csvfile, optarg);
                    }
                    else
                    {
                        fprintf(stderr,
                                "\n Couldn't open csv file %s; errno=%d.\n",
                                optarg, errno);
                        exit(EXIT_FAILURE);
                    }
                }
                restore = 1;    // restore css file, no cmd menu
                break;

            case 'r':
                report_only = 1; // print stats only, no cmd menu.
                break;

            case 's':        // set css size to option value
            case 'S':        // same as above but round up to next 2^n+1
                if (restore)
                {
                    fprintf(stderr,
                            "\nOptions -s, -S ignored when restoring.\n");
                    break;
                }
                if (sscanf(optarg, "%d", &sparse_spectrum_file_length))
                {
                    if (!quiet)
                        fprintf(stderr,
                                "\nOverride css creation length to %d\n",
                                sparse_spectrum_file_length);
                    user_set_css_length = 1;
                }
                else
                {
                    fprintf(stderr,
                            "On -%c flag: Missing or incomprehensible number of buckets.\n",
                            opt);
                    exit(EXIT_FAILURE);
                }
                if (opt == 'S') // round up to next 2^n+1
                {
                    int k;

                    k = (int)floor(log2(sparse_spectrum_file_length - 1));
                    while ((2 << k) + 1 < sparse_spectrum_file_length)
                        k++;
                    sparse_spectrum_file_length = (2 << k) + 1;
                    user_set_css_length = 1;
                }
                break;

            case 't':
                if (user_trace == 0)
                {
                    user_trace = 1;
                    fprintf(stderr, "User tracing on");
                }
                else
                {
                    user_trace = 0;
                    fprintf(stderr, "User tracing off");
                }
                break;

            case 'T':
                if (internal_trace == 0)
                {
                    internal_trace = 1;
                    fprintf(stderr, "Internal tracing on");
                }
                else
                {
                    internal_trace = 0;
                    fprintf(stderr, "Internal tracing off");
                }
                break;

            case 'v':
                fprintf(stderr, " This is osbf-util, version %s\n", version);
                fprintf(stderr, " Copyright 2004-2007 William S. Yerazunis.\n");
                fprintf(stderr,
                        " This software is licensed under the GPL with ABSOLUTELY NO WARRANTY\n");
                exit(EXIT_SUCCESS);

            default:
                helptext();
                exit(EXIT_SUCCESS);
                break;
            }
        }

        if (optind < argc)
        {
            strncpy(cssfile, argv[optind], WIDTHOF(cssfile));
            cssfile[WIDTHOF(cssfile) - 1] = 0;
            /* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
        }
        else
        {
            helptext();
            exit(EXIT_SUCCESS);
        }

        //       and stat it to get it's length
        k = stat(cssfile, &statbuf);
        //       quick check- does the file even exist?
        if (k == 0)
        {
            if (restore)
            {
                fprintf(stderr,
                        "\n.CSS file %s exists! Restore operation aborted.\n",
                        cssfile);
                exit(EXIT_FAILURE);
            }
            if (!quiet && user_set_css_length)
                fprintf(stderr,
                        "\n.CSS file %s exists; -s, -S options ignored.\n",
                        cssfile);
        }
        else
        {
            //      file didn't exist... create it
            if (!quiet && !restore)
                fprintf(stdout, "\nHad to create .CSS file %s with %u buckets\n",
                        cssfile, sparse_spectrum_file_length);
            if (crm_osbf_create_cssfile(cssfile,
                        sparse_spectrum_file_length,
                        OSBF_VERSION, 0
                        /* [i_a] unused anyway , OSBF_CSS_SPECTRA_START */) != EXIT_SUCCESS)
            {
                exit(EXIT_FAILURE);
            }
            k = stat(cssfile, &statbuf);
            CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
        }
        hfsize = statbuf.st_size;

        //
        //   mmap the hash file into memory so we can bitwhack it
        header = crm_mmap_file(cssfile,
                0,
                hfsize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                CRM_MADV_RANDOM,
                &hfsize);
        if (header == MAP_FAILED)
        {
            fprintf(stderr,
                    "\n Couldn't mmap file %s into memory; errno=%d(%s).\n",
                    cssfile, errno, errno_descr(errno));
            exit(EXIT_FAILURE);
        }
        if (*((unsigned int *)(header->version)) != OSBF_VERSION)
        {
            fprintf(stderr,
                    "\n %s is the wrong version. We're expecting a %s css file.\n",
                    cssfile, CSS_version_name[OSBF_VERSION]);
            crm_munmap_file((void *)header);
            exit(EXIT_FAILURE);
        }

        hashes = (OSBF_FEATUREBUCKET_STRUCT *)header + header->buckets_start;
        if (hashes == MAP_FAILED)
        {
            fprintf(stderr,
                    "\n Couldn't open RW file %s; errno=%d(%s).\n", cssfile, errno, errno_descr(errno));
            exit(EXIT_FAILURE);
        }

        //   from now on, hfsize is buckets, not bytes.
        hfsize = hfsize / sizeof(OSBF_FEATUREBUCKET_STRUCT);

        if (dump)
        {
            /* dump the css file */
            OSBF_FEATUREBUCKET_STRUCT *bucket;

            bucket = (OSBF_FEATUREBUCKET_STRUCT *)header;
            for (i = 0; i < hfsize; i++)
            {
                unsigned long int k;
                unsigned long int h;
                unsigned long int v;
                k = bucket[i].key;
                h = bucket[i].hash;
                v = bucket[i].value;

                fprintf(stdout, "%lu;%lu;%lu\n", k, h, v);
            }
        }

        if (restore)
        {
            FILE *f;
            OSBF_FEATUREBUCKET_STRUCT *bucket;

            // restore the css file  - note that if we DIDN'T create
            // it already, then this will fail.
            //
            if ((f = fopen(csvfile, "rb")) == NULL)
            {
                fprintf(stderr, "\n Couldn't open csv file %s; errno=%d(%s).\n",
                        csvfile, errno, errno_descr(errno));
                exit(EXIT_FAILURE);
            }
            else
            {
                bucket = (OSBF_FEATUREBUCKET_STRUCT *)header;
                for (i = 0; i < hfsize; i++)
                {
                    unsigned int k;
                    unsigned int h;
                    unsigned int v;

                    fscanf(f, "%u;%u;%u\n", &k, &h, &v);
                    // [i_a] GROT GROT GROT error checking (as always :-( )!!!!!!!
                    bucket[i].key = k;
                    bucket[i].hash = h;
                    bucket[i].value = v;
                }
                fclose(f);
            }
        }

        zloop = 1;
        while (zloop == 1 && !restore && !dump)
        {
            zloop = 0;
            crm_osbf_packcss(header, 0, header->buckets - 1);
            sum = 0;
            maxchain = 0;
            curchain = 0;
            totchain = 0;
            fbuckets = 0;
            nchains = 0;
            ofbins = 0;
            for (i = 0; i < header->buckets; i++)
            {
                sum += GET_BUCKET_VALUE(hashes[i]);
                if (GET_BUCKET_VALUE(hashes[i]) != 0)
                {
                    fbuckets++;
                    curchain++;
                    if (GET_BUCKET_VALUE(hashes[i]) >= OSBF_FEATUREBUCKET_VALUE_MAX)
                        ofbins++;
                }
                else
                {
                    if (curchain > 0)
                    {
                        totchain += curchain;
                        nchains++;
                        if (curchain > maxchain)
                            maxchain = curchain;
                        curchain = 0;
                    }
                }
            }

            version_index = *((unsigned int *)header->version);
            if (version_index < 0 || version_index > UNKNOWN_VERSION)
                version_index = UNKNOWN_VERSION;
            fprintf(stdout, "\n Sparse spectra file %s statistics:\n", cssfile);
            fprintf(stdout, "\n CSS file version                 : %12s",
                    CSS_version_name[version_index]);
            fprintf(stdout, "\n Header size (bytes)              : %12d",
                    (int)(header->buckets_start * sizeof(OSBF_FEATUREBUCKET_STRUCT)));
            fprintf(stdout, "\n Bucket size (bytes)              : %12d",
                    (int)sizeof(OSBF_FEATUREBUCKET_STRUCT));
            fprintf(stdout, "\n Total available buckets          : %12d",
                    header->buckets);
            fprintf(stdout, "\n Total buckets in use             : %12d",
                    fbuckets);
            fprintf(stdout, "\n Number of trainings              : %12ld",
                    (long int)header->learnings);
            fprintf(stdout, "\n Total buckets with value >= max  : %12d",
                    ofbins);
            fprintf(stdout, "\n Total hashed datums in file      : %12lld", (long long int)sum);
            fprintf(stdout, "\n Average datums per bucket        : %12.2f",
                    (fbuckets > 0) ? (sum * 1.0) / (fbuckets * 1.0) : 0.0);
            fprintf(stdout, "\n Number of chains                 : %12d",
                    nchains);
            fprintf(stdout, "\n Maximum length of overflow chain : %12d",
                    maxchain);
            fprintf(stdout, "\n Average length of overflow chain : %12.2f",
                    nchains > 0 ? (totchain * 1.0) / (nchains * 1.0) : 0.0);
            fprintf(stdout, "\n Average packing density          : %12.2f\n",
                    (fbuckets * 1.0) / (header->buckets * 1.0));
            for (i = 0; i < OSBF_FEATUREBUCKET_VALUE_MAX; i++)
                bcounts[i] = 0;
            for (v = 0; v < header->buckets; v++)
            {
                if (GET_BUCKET_VALUE(hashes[v]) < OSBF_FEATUREBUCKET_VALUE_MAX)
                    bcounts[GET_BUCKET_VALUE(hashes[v])]++;
            }

            if (!brief)
            {
                for (i = 0; i < OSBF_FEATUREBUCKET_VALUE_MAX; i++)
                {
                    if (bcounts[i] > 0)
                    {
                        fprintf(stdout, "\n bin value %8d found %9d times",
                                i, bcounts[i]);
                    }
                }
            }

            fprintf(stdout, "\n");
            cmdloop = 1;
            while (!report_only && cmdloop)
            {
                // clear command buffer
                cmdchr[0] = 0;
                fprintf(stdout, "Options:\n");
                fprintf(stdout, "   Z n - zero bins at or below a value\n");
                fprintf(stdout, "   S n - subtract a constant from all bins\n");
                fprintf(stdout, "   D n - divide all bins by a constant\n");
                fprintf(stdout, "   R - rescan\n");
                fprintf(stdout, "   P - pack\n");
                fprintf(stdout, "   Q - quit\n");
                fprintf(stdout, ">>> ");
                clearerr(stdin);
                fscanf(stdin, "%[^\n]", cmdstr);
                fscanf(stdin, "%c", crapchr);
                fields = sscanf(cmdstr, "%s %lf", cmdchr, &cmdval);
                if (strlen((char *)cmdchr) != 1)
                {
                    fprintf(stdout, "Unknown command: %s\n", cmdchr);
                    continue;
                }
                switch (tolower((int)cmdchr[0]))
                {
                case 'z':
                    if (fields != 2)
                    {
                        fprintf(stdout,
                                "Z command requires a numeric argument!\n");
                    }
                    else
                    {
                        fprintf(stdout, "Working...");
                        for (i = 0; i < header->buckets; i++)
                        {
                            if (GET_BUCKET_VALUE(hashes[i]) <= cmdval)
                            {
                                BUCKET_RAW_VALUE(hashes[i]) = 0;
                            }
                        }
                        fprintf(stdout, "done.\n");
                    }
                    break;

                case 's':
                    if (fields != 2)
                    {
                        fprintf(stdout,
                                "S command requires a numeric argument!\n");
                    }
                    else
                    {
                        int val = (int)cmdval;
                        fprintf(stdout, "Working...");
                        for (i = 0; i < header->buckets; i++)
                        {
                            if (GET_BUCKET_VALUE(hashes[i]) > val)
                            {
                                BUCKET_RAW_VALUE(hashes[i]) =
                                    GET_BUCKET_VALUE(hashes[i]) - val;
                            }
                            else
                            {
                                BUCKET_RAW_VALUE(hashes[i]) = 0;
                            }
                        }
                        fprintf(stdout, "done.\n");
                    }
                    break;

                case 'd':
                    {
                        int val = (int)cmdval;
                        if (fields != 2)
                        {
                            fprintf(stdout,
                                    "D command requires a numeric argument!\n");
                        }
                        else if (val == 0)
                        {
                            fprintf(stdout, "You can't divide by zero, nimrod!\n");
                        }
                        else
                        {
                            fprintf(stdout, "Working...");
                            for (i = 0; i < header->buckets; i++)
                            {
                                BUCKET_RAW_VALUE(hashes[i]) =
                                    GET_BUCKET_VALUE(hashes[i]) / val;
                            }
                            fprintf(stdout, "done.\n");
                        }
                    }
                    break;

                case 'r':
                    zloop = 1;
                    cmdloop = 0;
                    break;

                case 'p':
                    fprintf(stdout, "Working...");
                    crm_osbf_packcss(header, 0, header->buckets - 1);
                    zloop = 1;
                    cmdloop = 0;
                    break;

                case 'q':
                    fprintf(stdout, "Bye! \n");
                    cmdloop = 0;
                    break;

                default:
                    fprintf(stdout, "Unknown command: %c\n", cmdchr[0]);
                    break;
                }
            }
        }
    }
    return 0;
}




// bogus code to make link phase happy while we are in limbo between obsoleting this tool and
// getting cssXXXX script commands working in crm114 itself.
void free_stack_item(CSL_CELL *csl)
{ }



