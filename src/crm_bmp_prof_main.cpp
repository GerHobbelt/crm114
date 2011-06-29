#include "crm_bmp_prof.h"


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



CRM_ANALYSIS_PROFILE_CONFIG analysis_cfg = { 0 };

ANALYSIS_CONFIG cfg = { 0 };




// bogus code to make link phase happy while we are in limbo between obsoleting this tool and
// getting cssXXXX script commands working in crm114 itself.
void free_stack_item(CSL_CELL *csl)
{ }









/*
 * memory cleanup routine which is called at the end of the crm_bmp_prof run.
 *
 * Note: this routine *also* called when an error occurred (e.g. out of memory)
 *    so tread carefully here: do not assume all these pointers are filled.
 */
static void ana_final_cleanup(void)
{
    free(cfg.output_file_template);
    free(cfg.input_file);

    cleanup_stdin_out_err_as_os_handles();
}


static int show_help(const char *arg)
{
    if (!strcmp("mode", arg))
    {
        // display extra 'mode' help.
        fprintf(stdout, "No 'mode' info available yet.\n");
        return EXIT_SUCCESS;
    }
    fprintf(stderr, "Unidentified help argument '%s'. Cannot help.\n", arg);
    return EXIT_FAILURE;
}







int main(int argc, char **argv)
{
    int c;

    init_stdin_out_err_as_os_handles();
#if 0
    setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));
    setvbuf(stderr, stderr_buf, _IOFBF, sizeof(stderr_buf));
#endif

#if (defined (WIN32) || defined (_WIN32) || defined (_WIN64) || defined (WIN64)) && defined (_DEBUG)
    /*
     * Hook in our client-defined reporting function.
     * Every time a _CrtDbgReport is called to generate
     * a debug report, our function will get called first.
     */
    _CrtSetReportHook(crm_dbg_report_function);

    /*
     * Define the report destination(s) for each type of report
     * we are going to generate.  In this case, we are going to
     * generate a report for every report type: _CRT_WARN,
     * _CRT_ERROR, and _CRT_ASSERT.
     * The destination(s) is defined by specifying the report mode(s)
     * and report file for each report type.
     */
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    // Store a memory checkpoint in the s1 memory-state structure
    _CrtMemCheckpoint(&crm_memdbg_state_snapshot1);

    atexit(crm_report_mem_analysis);

    // Get the current bits
    c = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    // Set the debug-heap flag so that freed blocks are kept on the
    // linked list, to catch any inadvertent use of freed memory
#if 0
    c |= _CRTDBG_DELAY_FREE_MEM_DF;
#endif

    // Set the debug-heap flag so that memory leaks are reported when
    // the process terminates. Then, exit.
    //c |= _CRTDBG_LEAK_CHECK_DF;

    // Clear the upper 16 bits and OR in the desired freqency
    //c = (c & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;

    c |= _CRTDBG_CHECK_ALWAYS_DF;

    // Set the new bits
    _CrtSetDbgFlag(c);

//    // set a malloc marker we can use it in the leak dump at the end of the program:
//    (void)_calloc_dbg(1, 1, _CLIENT_BLOCK, __FILE__, __LINE__);
#endif

    //  fprintf(stderr, " args: %d\n", argc);
    //  for (c = 0; c < argc; c++)
    //    fprintf(stderr, " argi: %d, argv: %s\n", c, argv[c]);

    atexit(ana_final_cleanup);

#if defined (HAVE__SET_OUTPUT_FORMAT)
    _set_output_format(_TWO_DIGIT_EXPONENT);     // force MSVC (& others?) to produce floating point %f with 2 digits for power component instead of 3 for easier comparison with 'knowngood'.
#endif

    // force MSwin/Win32 console I/O into binary mode: treat \r\n and\n as completely different - like it is on *NIX boxes!
#if defined (HAVE__SETMODE) && defined (HAVE__FILENO) && defined (O_BINARY)
    (void)_setmode(_fileno(crm_stdin), O_BINARY);
    (void)_setmode(_fileno(crm_stdout), O_BINARY);
    (void)_setmode(_fileno(crm_stderr), O_BINARY);
#endif

    //   copy program path/name into global static...
    prog_argv0 = argv[0];

    for ( ; ;)
    {
        // int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static const struct option long_options[] =
        {
            { "output", 1, 0, 'o' },
            { "input", 1, 0, 'i' },
            { "mode", 1, 0, 'm' },
            { "skip-errors", 2, 0, 'c' },
            { "version", 0, 0, 128 },
            { "verbose", 2, 0, 'v' },
            { "help", 0, 0, 'h' },
            { 0, 0, 0, 0 }
        };


        c =
#if defined (HAVE_GETOPT_LONG_EX)
            getopt_long_only_ex
#elif defined (HAVE_GETOPT_LONG_ONLY)
            getopt_long_only
#else
            getopt_long
#endif
            (argc, argv,
             "i:o:v::h::?::m:c",
             long_options, &option_index
#if defined (HAVE_GETOPT_LONG_EX)
            , (void(*) (void *, const char *, ...))fprintf, stderr
#elif defined (HAVE_GETOPT_LONG_ONLY)
#else
#endif
            );
        switch (c)
        {
#if 0
        case 0:
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            continue;
#endif

        case 'i':
            if (optarg)
            {
                struct stat st;
                int k;

                k = stat(optarg, &st);
                if (k)
                {
                    fprintf(stderr, "Specified input file '%s' does not exist or cannot be accessed: error %d(%s)\n",
                            optarg,
                            errno,
                            errno_descr(errno));
                    return EXIT_FAILURE;
                }
                cfg.input_file = strdup(optarg);
            }
            else
            {
                fprintf(stderr, "no input file specified. We can't do anything.\n");
                return EXIT_FAILURE;
            }
            continue;

        case 'o':
            if (optarg)
            {
                cfg.output_file_template = strdup(optarg);
            }
            else
            {
                fprintf(stderr, "no input file specified. We can't do anything.\n");
                return EXIT_FAILURE;
            }
            continue;

        case 128:
            fprintf(stdout, "crm_bmp_prof v0.1 build %s (%s%s) (host: %s)\n", TAR_FILENAME_POSTFIX, VERSION, VER_SUFFIX, HOSTTYPE);
            return EXIT_SUCCESS;

        case 'v':
            if (optarg)
            {
                char *e = NULL;
                long v = strtol(optarg, &e, 10);

                if ((!e || !*e) && v > 0 && v < 10)
                {
                    cfg.verbosity = (int)v;
                }
                else
                {
                    fprintf(stderr, "invalid verbosity argument '%s' specified: range 0..9 is accepted.\n", optarg);
                    return EXIT_FAILURE;
                }
            }
            else
            {
                cfg.verbosity++;
            }
            continue;

        case 'c':
            cfg.skip_errors = 1;
            continue;

        case '?':
        case 'h':
            if (optarg)
            {
                return show_help(optarg);
            }
            fprintf(stdout, "crm_bmp_prof v0.1 build %s (%s%s : %s)\n", TAR_FILENAME_POSTFIX, VERSION, VER_SUFFIX, HOSTTYPE);
            fprintf(stdout, "\n");
            fprintf(stdout, "Usage: crm_bmp_prof <options>\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "Where <options>:\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  --output <filenametemplate>\n");
            fprintf(stdout, "  -o <filenametemplate>\n");
            fprintf(stdout, "             specifies the file path where the output produced by\n");
            fprintf(stdout, "             crm_bmp_prof will be stored. May contain a %%d format\n");
            fprintf(stdout, "             specifier a la printf(3p) to produce numbered output files.\n");
            fprintf(stdout, "             this is mandatory when reporting in 'video' mode. (See\n");
            fprintf(stdout, "             '--help mode' for more info about this latter bit.)\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  --input <filename>\n");
            fprintf(stdout, "  -i <filename>\n");
            fprintf(stdout, "             specifies the file which contains the profile data collected\n");
            fprintf(stdout, "             from CRM114 while executing it with the '-A' analysis option.\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  --mode <modespec>\n");
            fprintf(stdout, "  -m <modespec>\n");
            fprintf(stdout, "             instruct the crm_bmp_prof analysis tool what to do with the\n");
            fprintf(stdout, "             input: which data to look at, how to process it and which\n");
            fprintf(stdout, "             part of it should be reported/displayed and in what way.\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "             Run '--help mode' to see a detailed description of\n");
            fprintf(stdout, "             <>modespec> and examples of use.\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  -c\n");
            fprintf(stdout, "  --skip-errors\n");
            fprintf(stdout, "             Report but otherwise ignore capture (input) file errors.\n");
            fprintf(stdout, "             This is handy when reporting from profile captures which include\n");
            fprintf(stdout, "             crashed or otherwise badly aborted CRM114 runs.\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  --version\n");
            fprintf(stdout, "             display the version, revision and build info for this tool\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  --help\n");
            fprintf(stdout, "  -h\n");
            fprintf(stdout, "  -?\n");
            fprintf(stdout, "             show this on-line help.\n");
            fprintf(stdout, "\n");
            fprintf(stdout, "  --verbode [<n>]\n");
            fprintf(stdout, "             set verbosity level to optional <n> (range 0..9); default: 1\n");
            fprintf(stdout, "\n");
            return EXIT_SUCCESS;

        case 'm':
            // ignore for now.
            continue;

        default:
            fprintf(stderr, "?? getopt returned character code 0%o('%c') ??\n", c, (crm_isprint(c) ? c : '?'));
            return EXIT_FAILURE;

        case - 1:
            if (optind < argc)
            {
                fprintf(stderr, "non-option ARGV-elements have been specified: ");
                while (optind < argc)
                    fprintf(stderr, "%s ", argv[optind++]);
                fprintf(stderr, "\n");
                fprintf(stderr, "This is not supported!\n");
                return EXIT_FAILURE;
            }
            break;
        }
        break;
    }

    if (cfg.output_file_template && cfg.input_file)
    {
        int x = 1280;
        int y = 720;
        int size = calculate_BMP_image_size(x, y);
        BmpFileStructure *dst;
        FILE *of;
        FILE *inf;

        dst = (BmpFileStructure *)calloc(1, size);

        create_BMP_file_header(dst, size, x, y);

        // write black BMP file.
        of = fopen(cfg.output_file_template, "wb");
        if (of)
        {
            if (1 != fwrite(dst, size, 1, of))
            {
                fprintf(stderr, "Cannot write BMP to file '%s': error %d(%s)\n",
                        cfg.output_file_template,
                        errno, errno_descr(errno));
                fclose(of);
                free(dst);
                return EXIT_FAILURE;
            }
            fclose(of);
        }
        else
        {
            fprintf(stderr, "Cannot open BMP target file '%s' for writing: error %d(%s)\n",
                    cfg.output_file_template,
                    errno, errno_descr(errno));
            free(dst);
            return EXIT_FAILURE;
        }


        // open the profile file for analysis
        inf = fopen(cfg.input_file, "rb");
        if (!inf)
        {
            fprintf(stderr, "Cannot open analysis profile input file '%s' for reading: error %d(%s)\n",
                    cfg.input_file,
                    errno, errno_descr(errno));
            free(dst);
            return EXIT_FAILURE;
        }
        else
        {
            CRM_ANALYSIS_PROFILE_ELEMENT *store;
            int read_size;
            struct stat st;
            int k;
            int store_size = 512;
            off_t inf_pos;

            k = fstat(fileno(inf), &st);
            if (k)
            {
                fprintf(stderr, "Cannot stat analysis profile input file '%s': error %d(%s)\n",
                        cfg.input_file,
                        errno, errno_descr(errno));
                fclose(inf);
                free(dst);
                return EXIT_FAILURE;
            }
            read_size = st.st_size;
            if (read_size % sizeof(store[0]))
            {
                fprintf(stderr, "You're pointing at an invalid analysis profile input file '%s' (size = %d units + %d bytes): error %d(%s)\n",
                        cfg.input_file,
                        read_size / (int)sizeof(store[0]),
                        read_size % (int)sizeof(store[0]),
                        errno, errno_descr(errno));
                fclose(inf);
                free(dst);
                return EXIT_FAILURE;
            }
            read_size /= sizeof(store[0]);

            if (cfg.verbosity >= 1)
            {
                fprintf(stdout, "Input file '%s' contains %d units, including header markers.\n",
                        cfg.input_file,
                        read_size);
            }

            store = (CRM_ANALYSIS_PROFILE_ELEMENT *)calloc(store_size, sizeof(store[0]));
            if (!store)
            {
                fprintf(stderr, "Out of memory while trying to scan analysis profile input file '%s'.\n",
                        cfg.input_file);
                fclose(inf);
                free(dst);
                return EXIT_FAILURE;
            }

            if (decipher_input_header(inf, &read_size))
            {
                fclose(inf);
                free(dst);
                free(store);
                return EXIT_FAILURE;
            }
            inf_pos = ftell(inf);

            if (scan_to_determine_input_ranges(inf, read_size, store, store_size))
            {
                fclose(inf);
                free(dst);
                free(store);
                return EXIT_FAILURE;
            }

            // rewind
            if (fseek(inf, inf_pos, SEEK_SET))
            {
                fprintf(stderr, "Cannot rewind analysis profile input file '%s': error %d(%s)\n",
                        cfg.input_file,
                        errno,
                        errno_descr(errno));
                fclose(inf);
                free(dst);
                free(store);
                return EXIT_FAILURE;
            }

            if (collect_and_display_requested_data(inf, read_size, store, store_size))
            {
                fclose(inf);
                free(dst);
                free(store);
                return EXIT_FAILURE;
            }

            if (produce_reports(&report_data))
            {
                fclose(inf);
                free(dst);
                free(store);
                return EXIT_FAILURE;
            }

            fclose(inf);
            free(store);
        }
        free(dst);
    }
    free(cfg.input_file);
    free(cfg.output_file_template);

    return EXIT_SUCCESS;
}

