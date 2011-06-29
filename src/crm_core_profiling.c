//  crm_bit_entropy.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007 William S. Yerazunis, all rights reserved.
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




#if !defined (CRM_WITHOUT_BMP_ASSISTED_ANALYSIS)

/*
 * Code to register and log profiling events; these events can be processed by an additional
 * tool to produce color graphs, etc. to help analyze CRM114 runs; classifiers, etc.
 *
 * Using the 'profile' script command, environment variable or command line option, a user can 'subscribe' specific
 * events, which will be logged in a 'profile trace' and saved to disc.
 *
 * When a 'profile trace file' already exists, data will be appended to it, in order to
 * enable the user running profiles spanning multiple runs.
 *
 * To minimize the impact on performance, no calculations are performed on the logged data while profiling;
 * the only 'manipulation' is that each event value is converted to a 64-bit floating point value, which
 * is logged to disc. This allows one system to log profile info while running CRM114 while the collected
 * data can be processed on another platform.
 */







static const struct crm_analysis_marker
{
    crm_analysis_instrument_t e;
    char                     *id;
} crm_analysis_marker_identifiers[] =
{
    { MARK_INIT, "INIT" },
    { MARK_SWITCH_CONTEXT, "SWITCH_CONTEXT" },
    { MARK_TERMINATION, "TERMINATION" },
    { MARK_DEBUG_INTERACTION, "DEBUG_INTERACTION" },
    { MARK_HASH_VALUE, "HASH_VALUE" },
    { MARK_HASH64_VALUE, "HASH64_VALUE" },
    { MARK_HASH_CONTINUATION, "HASH_CONTINUATION" },
    { MARK_OPERATION, "OPERATION" },
    { MARK_CLASSIFIER, "CLASSIFIER" },
    { MARK_CLASSIFIER_DB_TOTALS, "CLASSIFIER_DB_TOTALS" },
    { MARK_CHAIN_LENGTH, "CHAIN_LENGTH" },
    { MARK_HASHPROBE, "HASHPROBE" },
    { MARK_HASHPROBE_DIRECT_HIT, "HASHPROBE_DIRECT_HIT" },
    { MARK_HASHPROBE_HIT, "HASHPROBE_HIT" },
    { MARK_HASHPROBE_HIT_REFUTE, "HASHPROBE_HIT_REFUTE" },
    { MARK_HASHPROBE_MISS, "HASHPROBE_MISS" },
    { MARK_MICROGROOM, "MICROGROOM" },
    { MARK_CLASSIFIER_PARAMS, "MARK_CLASSIFIER_PARAMS" },
    { MARK_CSS_STATS_GROUP, "MARK_CSS_STATS_GROUP" },
    { MARK_CSS_STATS_HISTOGRAM, "MARK_CSS_STATS_HISTOGRAM" },
};


//
// The brunt of the profile tracking code is this one:
//
// fmt parameter notifies us which extra arguments are what format:
//
// i = int
// L = long long int
// d = double
//
// NOTE: only the least significant 48 bits of 'extra' are stored.
//       You _can_ of course try to store larger values, e.g. 64-bit hashes, etc. in there
//       but these will all be TRUNCATED to 48 bits!
//
void crm_analysis_mark(CRM_ANALYSIS_PROFILE_CONFIG *cfg, crm_analysis_instrument_t marker, long long int extra, const char *fmt, ...)
{
    va_list args;
    int i;
    CRM_ANALYSIS_PROFILE_ELEMENT store;

#if defined (HAVE_QUERYPERFORMANCECOUNTER) && defined (HAVE_QUERYPERFORMANCEFREQUENCY)
    LARGE_INTEGER ticks;
#elif defined (HAVE_CLOCK_GETTIME) && defined (HAVE_STRUCT_TIMESPEC)
    struct timespec timer1;
#elif defined (HAVE_GETTIMEOFDAY) && defined (HAVE_STRUCT_TIMEVAL)
    struct timeval timer1;
#elif defined (HAVE_TIMES) && defined (HAVE_STRUCT_TMS)
    struct tms timer1;
    clock_t timer2;
#elif defined (HAVE_CLOCK)
    clock_t timer1;
#else
    // nada
#endif

    // exit this code asap when no work required!
    if (!cfg || !cfg->fd || (int)marker < 0 || (int)marker >= TOTAL_NUMBER_OF_MARKERS || !cfg->instruments[marker])
        return;

    memset(&store, 0, sizeof(store));         // memset() is 'better' than 'store={0} init', because on MSVC at least the latter does not touch the alignment bytes between the fields and having 'random data' in there may complicate header decoding

#if defined (HAVE_QUERYPERFORMANCECOUNTER) && defined (HAVE_QUERYPERFORMANCEFREQUENCY)
    if (!QueryPerformanceCounter(&ticks))
    {
        ticks.QuadPart = 0;
    }
    store.time_mark = ticks.QuadPart;
#elif defined (HAVE_CLOCK_GETTIME) && defined (HAVE_STRUCT_TIMESPEC)
    if (!clock_gettime(CLOCK_REALTIME, &timer1))
    {
        store.time_mark = ((int64_t)timer1.tv_sec) * 1000000000LL + timer1.tv_nsec;
    }
    else
    {
        store.time_mark = 0; // unknown; due to error
    }
#elif defined (HAVE_GETTIMEOFDAY) && defined (HAVE_STRUCT_TIMEVAL)
    if (!gettimeofday(&timer1))
    {
        store.time_mark = ((int64_t)timer1.tv_sec) * 1000000LL + timer1.tv_usec;
    }
    else
    {
        store.time_mark = 0; // unknown; due to error
    }
#elif defined (HAVE_TIMES) && defined (HAVE_STRUCT_TMS)
    struct tms timer1;
    timer2 = times(&timer1);
    if (times2 == (clock) - 1)
    {
        store.time_mark = 0; // unknown; due to error
    }
    else
    {
        store.time_mark = timer2;
    }
#elif defined (HAVE_CLOCK)
    timer1 = clock();
    if (times1 == (clock) - 1)
    {
        store.time_mark = 0; // unknown; due to error
    }
    else
    {
        store.time_mark = timer1;
    }
#else
    store.time_mark = 0;
#endif

    va_start(args, fmt);

    for (i = 0; i < WIDTHOF(store.value); i++)
    {
        switch (*fmt++)
        {
        case 0:
            // end of format spec: abortus provocatus
            fmt--;
            break;

        case 'i':
            store.value[i].as_int = va_arg(args, int);
            continue;

        case 'p':
            // packed int:
            {
                uint64_t v = va_arg(args, int);
                v <<= 32;
                v |= (va_arg(args, int) & 0xFFFFFFFFU);
                store.value[i].as_int = v;
            }
            continue;

        case 'L':
            store.value[i].as_int = va_arg(args, long long int);
            continue;

        case 'd':
            store.value[i].as_float = va_arg(args, double);
            continue;

        default:
            // your code is b0rked!
            CRM_ASSERT(!"should never get here");
            break;
        }
        break;
    }
    va_end(args);
    if (*fmt)
    {
        fclose(cfg->fd);
        cfg->fd = NULL;
        untrappableerror("Too many values specified in format for analysis marker.", "We're pooped.");
        return;
    }

    store.marker = extra;
    store.marker <<= 16;
    CRM_ASSERT(marker < 0x1000);
    store.marker |= (marker | (i << 12));

    // store element filled; now write it; fwrite takes care of buffering.
    if (1 != fwrite(&store, sizeof(store), 1, cfg->fd))
    {
        fclose(cfg->fd);
        cfg->fd = NULL;
        untrappableerror_ex(SRC_LOC(), "Cannot write analysis profile data to file '%s'! We're pooped. Error %d(%s)",
                cfg->filepath,
                errno,
                errno_descr(errno));
        return;
    }
}



//
// Determine both units of the timer value (return value) and the accuracy (clock_rez).
//
// Return 0 for unknown entities.
//
static int64_t crm_analysis_get_timer_freq(int64_t *clock_rez)
{
#if defined (HAVE_QUERYPERFORMANCECOUNTER) && defined (HAVE_QUERYPERFORMANCEFREQUENCY)
    LARGE_INTEGER freq;

    if (QueryPerformanceFrequency(&freq))
    {
        int64_t f = freq.QuadPart;
        *clock_rez = f;
        return f;
    }
    else
    {
        *clock_rez = 0;
        return 0;
    }
#elif defined (HAVE_CLOCK_GETTIME) && defined (HAVE_STRUCT_TIMESPEC)
    struct timespec timer1;
    if (!clock_getres(CLOCK_REALTIME, &timer1))
    {
        int64_t t = timer1.tv_sec;
        t *= 1000000000;
        t += timer1.tv_nsec;
        *clock_rez = t;
    }
    else
    {
        *clock_rez = 0;
        return 0;
    }
    return 1000000000;     // nanoseconds

#elif defined (HAVE_GETTIMEOFDAY) && defined (HAVE_STRUCT_TIMEVAL)
    *clock_rez = 0;     // unknown
    return 1000000;     // microseconds rez

#elif defined (HAVE_TIMES) && defined (HAVE_STRUCT_TMS)
#if defined (HAVE_SYSCONF)
    // from the man page: Applications should use sysconf(_SC_CLK_TCK) to determine the number of clock ticks per second as it may vary from system to system.
    long int ret = sysconf(_SC_CLK_TCK);
    if (ret == -1)
    {
        ret = 0;         // unknown
        return 0;        // unknown
    }
    *clock_rez = ret;
    return ret;

#else

    *clock_rez = 0;
    return 0;     // unknown

#endif // HAVE_SYSCONF

#elif defined (HAVE_CLOCK)
    *clock_rez = 0;            // unknown
    return CLOCKS_PER_SEC;     // this is the divisor, NOT the actual accuracy!

#else
    *clock_rez = 0;
    return 0;     // unknown

#endif
}



//
// convert set of args in human readable form to fully configured analsyis profile
// setup.
//
// PLUS: if profile file is not yet open, open it for write/append. YES, we
// append(!) to an existing profile file, so we can cover multiple runs in a single
// profile file for ease of use.
//
// Return non-zero value on error.
//
int crm_init_analysis(CRM_ANALYSIS_PROFILE_CONFIG *cfg, const char *args, int args_length)
{
    int state;
    int64_t clock_rez;
    int64_t clock_freq = crm_analysis_get_timer_freq(&clock_rez);
    int switch_context = 0;
    int textoffset = 0;
    struct stat st;

    if (!cfg || !args || args_length == 0 || args_length < -1 || !*args)
        return -1;

    if (args_length == -1)
    {
        args_length = (int)strlen(args);
    }

    for (state = 0; ; state++)
    {
        int start;
        int len;
        const char *wp;

        if (!crm_nextword(args, args_length, textoffset, &start, &len))
        {
            break;
        }
        textoffset = start;
        wp = args + textoffset;
        textoffset += len;

        switch (state)
        {
        case 0:
            // filename to use:
            if (cfg->fd)
            {
                if (strlen(cfg->filepath) != len || strncmp(cfg->filepath, wp, len))
                {
                    // different profile file selected now.
                    //
                    // write 'switch context' marker to previous file
                    switch_context = 1;

                    fclose(cfg->fd);
                    cfg->fd = NULL;
                    free(cfg->filepath);
                }
                // same file as before. That's okay. Do nada to it. Write INIT SUBmarker to be sure we can later find if someone messed up.
                break;
            }
            // open the file for binary append:
            cfg->filepath = malloc(len + 1);
            if (!cfg->filepath)
            {
                untrappableerror("We're out of memory while setting up the analysis session.", "We're pooped.");
                return -1;
            }
            memcpy(cfg->filepath, wp, len);
            cfg->filepath[len] = 0;

            cfg->fd = fopen(cfg->filepath, "ab");
            if (!cfg->fd)
            {
                char dirbuf[DIRBUFSIZE_MAX];

                fatalerror_ex(SRC_LOC(), "We cannot open the analysis file '%s' while setting up the analysis session. "
                                         "We're pooped. (full path: '%s') errno=%d(%s)",
                        cfg->filepath,
                        mk_absolute_path(dirbuf, WIDTHOF(dirbuf), cfg->filepath),
                        errno,
                        errno_descr(errno));
                free(cfg->filepath);
                return 1;
            }

            // write an INIT MAIN marker to start things off...
            continue;

        default:
            // next are the markers to enable:
            if (len >= 40 || len < 1)
            {
                nonfatalerror_ex(
                        SRC_LOC(), "Illegal analysis marker specified: '%.*s'. "
                                   "Check the documentation for a list of recognized markers. We'll continue with the rest if you don't mind.",
                        len,
                        wp);
            }
            else
            {
                char str[40];
                int i;
                int enable;

                len--;
                switch (*wp++)
                {
                case '+':
                    enable = 2;             // enable
                    break;

                case '-':
                    enable = 0;             // disable
                    break;

                case '!':
                    enable = 1;             // toggle
                    break;

                default:
                    enable = 2;             // default: enable
                    wp--;                   // restore position
                    len++;
                    break;
                }

                for (i = 0; i < len; i++)
                {
                    str[i] = (crm_isalnum(wp[i]) ? wp[i] : '_');                    // yes, this allows for screwed type words; but it was MEANT to unify '-' and '_'. I don't care if you throw in '?' or other wickedry - that's fine with me.
                }
                str[i] = 0;

                // decode special case:
                if (!*str)
                {
                    for (i = WIDTHOF(crm_analysis_marker_identifiers); --i >= 0;)
                    {
                        char *p = &cfg->instruments[crm_analysis_marker_identifiers[i].e];

                        // hit: ENABLE/DISABLE/TOGGLE that flag!
                        *p = (char)((enable >> 1) | (enable & !*p));                         // couldn't keep from this little play :-)
                    }
                }
                else
                {
                    // individual marker requested
                    int matchcount = 0;
                    char *p = NULL;

                    for (i = WIDTHOF(crm_analysis_marker_identifiers); --i >= 0;)
                    {
                        if (!strncasecmp(str, crm_analysis_marker_identifiers[i].id, len))
                        {
                            p = &cfg->instruments[crm_analysis_marker_identifiers[i].e];
                            if (strlen(crm_analysis_marker_identifiers[i].id) == len)
                            {
                                // if it's a full length match, set it to unique anyhow:
                                matchcount = 1;
                                break;
                            }
                            else
                            {
                                // count partial hits: when only one hit, it's unambiguous
                                matchcount++;
                            }
                        }
                    }
                    if (matchcount == 1)
                    {
                        // hit: ENABLE/DISABLE/TOGGLE that flag!
                        *p = (char)((enable >> 1) | (enable & !*p));                       // couldn't keep from this little play :-)
                    }
                    else if (matchcount == 0)
                    {
                        nonfatalerror_ex(
                                SRC_LOC(), "Unidentified analysis marker specified: '%.*s'. "
                                           "Check the documentation for a list of recognized markers. We'll continue with the rest if you don't mind.",
                                len,
                                wp);
                    }
                    else
                    {
                        nonfatalerror_ex(
                                SRC_LOC(), "Ambiguous analysis marker shorthand specified: '%.*s'. "
                                           "Check the documentation for a list of recognized markers. We'll continue with the rest if you don't mind.",
                                len,
                                wp);
                    }
                }
            }
            break;
        }
    }

    // sanity check: see which markers are enabled and turn on any others that those depend upon.
    //
    // 'dependents' are enabled by setting bit 1 (0x02); any subsequent 're-init' actions will discard that bit: see en/dis/toggle expression above.
    if (cfg->instruments[MARK_HASHPROBE_MISS])
    {
        cfg->instruments[MARK_HASHPROBE] |= 0x02;
    }
    if (cfg->instruments[MARK_HASHPROBE_DIRECT_HIT])
    {
        cfg->instruments[MARK_HASHPROBE_HIT] |= 0x02;
    }
    if (cfg->instruments[MARK_HASHPROBE_HIT_REFUTE])
    {
        cfg->instruments[MARK_HASHPROBE_HIT] |= 0x02;
    }
    if (cfg->instruments[MARK_HASHPROBE_HIT])
    {
        cfg->instruments[MARK_HASHPROBE] |= 0x02;
    }
    if (cfg->instruments[MARK_HASHPROBE])
    {
        cfg->instruments[MARK_CLASSIFIER_DB_TOTALS] |= 0x02;
    }
    if (cfg->instruments[MARK_CHAIN_LENGTH])
    {
        cfg->instruments[MARK_CLASSIFIER] |= 0x02;
    }
    if (cfg->instruments[MARK_MICROGROOM])
    {
        cfg->instruments[MARK_CLASSIFIER] |= 0x02;
    }
    if (cfg->instruments[MARK_CLASSIFIER_PARAMS])
    {
        cfg->instruments[MARK_CLASSIFIER] |= 0x02;
    }
    if (cfg->instruments[MARK_CSS_STATS_HISTOGRAM])
    {
        cfg->instruments[MARK_CSS_STATS_GROUP] |= 0x02;
    }
    if (cfg->instruments[MARK_CSS_STATS_GROUP])
    {
        cfg->instruments[MARK_CLASSIFIER] |= 0x02;
    }
    if (cfg->instruments[MARK_CLASSIFIER_DB_TOTALS])
    {
        cfg->instruments[MARK_CLASSIFIER] |= 0x02;
    }
    if (cfg->instruments[MARK_CLASSIFIER])
    {
        cfg->instruments[MARK_OPERATION] |= 0x02;
    }

    if (cfg->instruments[MARK_TERMINATION])
    {
        cfg->instruments[MARK_INIT] |= 0x02;
    }
    if (cfg->instruments[MARK_SWITCH_CONTEXT])
    {
        cfg->instruments[MARK_INIT] |= 0x02;
    }

    if (cfg->instruments[MARK_HASH_VALUE])
    {
        cfg->instruments[MARK_HASH_CONTINUATION] |= 0x02;
    }
    if (cfg->instruments[MARK_HASH64_VALUE])
    {
        cfg->instruments[MARK_HASH_CONTINUATION] |= 0x02;
    }
    if (cfg->instruments[MARK_VT_HASH_VALUE])
    {
        cfg->instruments[MARK_HASH_CONTINUATION] |= 0x02;
    }
    if (cfg->instruments[MARK_HASH_CONTINUATION] & 0x01)
    {
        cfg->instruments[MARK_HASH_VALUE] |= 0x02;
        cfg->instruments[MARK_HASH64_VALUE] |= 0x02;
        cfg->instruments[MARK_VT_HASH_VALUE] |= 0x02;
    }

    // if the file is empty, write a special header:
    if (fstat(fileno(cfg->fd), &st))
    {
        fclose(cfg->fd);
        cfg->fd = NULL;
        fatalerror_ex(SRC_LOC(), "We cannot stat the analysis file '%s' while setting up the analysis session. "
                                 "We're pooped.",
                cfg->filepath);
        free(cfg->filepath);
        return 1;
    }

    // to ensure we can 'sync' to another INIT header again, even when the profile file we're appending to is corrupted
    // due to a previous CRM114 crash when not all profile data was written, we 'pad' the existing file with NUL bytes
    // for AT LEAST one FULL profile element, then we write another header.
    //
    // We even do this when the file is okay - we don't check - just to make sure it ALWAYS works.
    {
        CRM_ANALYSIS_PROFILE_ELEMENT store;

        memset(&store, 0, sizeof(store));     // memset() is 'better' than 'store={0} init', because on MSVC at least the latter does not touch the alignment bytes between the fields and having 'random data' in there may complicate header decoding

        if (st.st_size != 0)
        {
            int padding = sizeof(store) - (st.st_size % sizeof(store));

            // pad to store element boundary:
            if (padding > 0)
            {
                if (1 != fwrite(&store, padding, 1, cfg->fd))
                {
                    fclose(cfg->fd);
                    cfg->fd = NULL;
                    untrappableerror_ex(SRC_LOC(), "Cannot pad existing analysis profile file '%s'! We're pooped. Error %d(%s)",
                            cfg->filepath,
                            errno,
                            errno_descr(errno));
                    return 1;
                }
            }

            // now write a NUL store element:
            if (1 != fwrite(&store, sizeof(store), 1, cfg->fd))
            {
                fclose(cfg->fd);
                cfg->fd = NULL;
                untrappableerror_ex(SRC_LOC(), "Cannot write NUL session sentinel to existing analysis profile file '%s'! We're pooped. Error %d(%s)",
                        cfg->filepath,
                        errno,
                        errno_descr(errno));
                return 1;
            }
        }

        // write header itself:
        store.marker = 0x123456789ABCDEF0ULL;
        store.value[0].as_int = 0x8081828384858687ULL;
        store.value[1].as_float = 1.0;
        store.value[2].as_int = 0x9091929394959697ULL;
        store.time_mark = 0xA0A1A2A3A4A5A6A7ULL;

        // store element filled; now write it; fwrite takes care of buffering.
        if (1 != fwrite(&store, sizeof(store), 1, cfg->fd))
        {
            fclose(cfg->fd);
            cfg->fd = NULL;
            untrappableerror_ex(SRC_LOC(), "Cannot write analysis profile file header to file '%s'! We're pooped. Error %d(%s)",
                    cfg->filepath,
                    errno,
                    errno_descr(errno));
            return 1;
        }
    }

    if (switch_context)
    {
        crm_analysis_mark(cfg, MARK_SWITCH_CONTEXT, 0, "");
    }

    crm_analysis_mark(cfg, MARK_INIT, 0, "LLi", (long long int)clock_freq, (long long int)clock_rez, (int)selected_hashfunction);

    return 0;
}



int crm_terminate_analysis(CRM_ANALYSIS_PROFILE_CONFIG *cfg)
{
    if (!cfg)
        return -1;

    crm_analysis_mark(cfg, MARK_TERMINATION, 0, "");

    if (cfg->fd)
    {
        fclose(cfg->fd);
        cfg->fd = NULL;
    }
    free(cfg->filepath);
    CRM_ASSERT(cfg->filepath == NULL);

    return 0;
}





#endif

