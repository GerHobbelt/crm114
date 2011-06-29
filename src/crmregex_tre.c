//       CRM114 Regex redirection bounce package this file bounces
//       CRM114 regex requests to whichever regex package has been
//       compiled and linked in to CRM114.
//
//       Adding a new regex package is relatively easy- just mimic the
//       ifdef stanzas below to map the functions
//
//         crm_regcomp
//         crm_regexec
//         crm_regerror
//         crm_regfree
//         crm_regversion
//
//      into whatever calls your preferred regex package uses.
//

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"


#if defined (HAVE_TRE_REGEX)

#if 0
#undef REGEX_CACHE_RANDOM_ACCESS
#undef REGEX_CACHE_LINEAR_SEARCH
//#define REGEX_CACHE_RANDOM_ACCESS
#define REGEX_CACHE_LINEAR_SEARCH
#endif


#if CRM_REGEX_CACHESIZE > 0

//  Cache for regex compilations
typedef struct
{
    char    *regex;
    regex_t *preg; // ptr to struct of {long, void*}
    int      regex_len;
    int      cflags;
    int      status;
} REGEX_CACHE_BLOCK;


static REGEX_CACHE_BLOCK regex_cache[CRM_REGEX_CACHESIZE] = { { NULL, NULL, 0, 0, 0 } };

#endif


//
//      How to do a register compilation
//
int crm_regcomp(regex_t *preg, const char *regex, int regex_len, int cflags)
{
    //       compile it with the TRE regex compiler
    //
    //    bug workaround - many regex compilers don't compile the null
    //    regex correctly, but _do_ compile "()" correctly, which
    //    matches the same thing).
    if (regex_len == 0)
    {
        return regncomp(preg, "()", 2, cflags);
    }

    //   Are we cacheing compiled regexes?  Maybe not...
#if CRM_REGEX_CACHESIZE == 0
    if (internal_trace)
    {
        int i;
        fprintf(stderr, "\ncompiling regex '%s', len %d, in hex: ",
            regex, regex_len);
        for (i = 0; i < regex_len; i++)
        {
            fprintf(stderr, "%02X", (unsigned char)regex[i]);
        }
        fprintf(stderr, "\n");
    }

    return regncomp(preg, regex, regex_len, cflags);

#else

    //   We are cacheing.  Scan our cache set for the compiled versions
    //   of this regex.  Note that a length of 0 means "empty bucket".
    {
        int i, j;
        int found_it = CRM_REGEX_CACHESIZE;
        int rtsize = sizeof(regex_t);
        regex_t *ppreg_temp = NULL;
        char *regex_temp = NULL;
        int rlen_temp = 0;
        int cflags_temp = 0;
        int status_temp = 0;

        if (internal_trace)
        {
            fprintf(stderr, "Checking the regex cache for '%s'\n", regex);
            for (j = 0; j < CRM_REGEX_CACHESIZE; j++)
            {
                if (regex_cache[j].regex_len > 0) fprintf(stderr,
                        "  regex[%d]:'%s' cf=%#x(%#x) len=%d(%d)\n",
                        j, regex_cache[j].regex, regex_cache[j].cflags, cflags,
                        (int)regex_cache[j].regex_len, regex_len);
            }
        }

        i = 0;
        j = 0;

#if defined (REGEX_CACHE_LINEAR_SEARCH)
        if (internal_trace)
        {
            fprintf(stderr, " [<- rex cache linear]\n");
        }
        //
        //          Linear Search uses a strict LRU algorithm to cache
        //          the precompiled regexes.
        //
        while (i < CRM_REGEX_CACHESIZE)
        {
            if (regex_len == regex_cache[i].regex_len
                && cflags == regex_cache[i].cflags
                && strncmp(regex_cache[i].regex, regex, regex_len) == 0)
            {
                //  We Found It!   Put it into the _temp vars...
                if (internal_trace)
                    fprintf(stderr, "found it.\n");
                ppreg_temp  = regex_cache[i].preg;
                regex_temp  = regex_cache[i].regex;
                rlen_temp   = regex_len;
                cflags_temp = cflags;
                status_temp = regex_cache[i].status;
                found_it = i;
                break;
            }
            i++;
        }
#elif defined (REGEX_CACHE_RANDOM_ACCESS)
        if (internal_trace)
        {
            fprintf(stderr, " [<- rex cache random]\n");
        }
        //
        //             Random Access uses an associative cache based on
        //             the hash of the regex (mod the size of the cache).
        //
        i = strnhash(regex, regex_len) % CRM_REGEX_CACHESIZE;
        if (regex_len == regex_cache[i].regex_len
            && cflags == regex_cache[i].cflags
            && strncmp(regex_cache[i].regex, regex, regex_len) == 0)
        {
            //  We Found It!   Put it into the _temp vars...
            if (internal_trace)
                fprintf(stderr, "found it.\n");
            ppreg_temp  = regex_cache[i].preg;
            regex_temp  = regex_cache[i].regex;
            rlen_temp   = regex_len;
            cflags_temp = cflags;
            status_temp = regex_cache[i].status;
            found_it = i;
        }
        if (internal_trace)
        {
            fprintf(stderr,
                "  strnhash (regex, regex_len) %% %d = %d\n", CRM_REGEX_CACHESIZE, i);
        }
#else
#error "Must have #define'd ONE of these: REGEX_CACHE_RANDOM_ACCESS, REGEX_CACHE_LINEAR_SEARCH"
#endif
    if (internal_trace)
    {
        if (found_it == CRM_REGEX_CACHESIZE)
            fprintf(stderr, "couldn't find it\n");
        else
            fprintf(stderr, "found it at [%d].\n", i);
    }
    //    note that on exit, i now is the index where we EITHER found
    //     the good data, or failed to do so, and found_it tells us which.
    //
    if (found_it == CRM_REGEX_CACHESIZE)
    {
        //  We didn't find it.  Do the compilation instead, putting
        //   the results into the _temp vars.
        if (internal_trace)
            fprintf(stderr, "couldn't find it\n");
        regex_temp = (char *)calloc((regex_len + 1), sizeof(regex_temp[0]));
        memcpy(regex_temp, regex, regex_len);
        CRM_ASSERT(regex_temp[regex_len] == 0);
        rlen_temp = regex_len;
        cflags_temp = cflags;
        if (internal_trace)
            fprintf(stderr, "Compiling '%s' (len %d).\n", regex_temp, rlen_temp);
        ppreg_temp = (regex_t *)calloc(rtsize, sizeof(ppreg_temp[0]));
        if (ppreg_temp == NULL)
        {
            fatalerror("Unable to allocate a pattern register buffer header.  ",
                "This is hopeless.  ");
        }
        status_temp = regncomp(ppreg_temp, regex_temp, rlen_temp, cflags_temp);

        //  We will always stuff the _temps in at 0
        //   and pretend that this was at the last index, so it
        //    moves everything else further down the list.


#if defined (REGEX_CACHE_LINEAR_SEARCH)
        if (internal_trace)
        {
            fprintf(stderr, " [-> rex cache linear]\n");
        }

        //   If we're in linear search, we move 0 through i-1 down to 1
        //   through i and then we stuff the _temp vars into the [i] cache
        //   area.  Note that if it was the final slot (at
        //   CRM_REGEX_CACHESIZE), we have to free the resources up or
        //   we'll leak them.
        //
        //                           Free the resources first, if needed.
        //
        i = CRM_REGEX_CACHESIZE - 1;
        if (regex_cache[i].preg != NULL)
        {
            regfree(regex_cache[i].preg);
            free(regex_cache[i].preg);
        }
        if (regex_cache[i].regex != NULL)
            free(regex_cache[i].regex);
        regex_cache[i].regex = NULL;
        regex_cache[i].regex_len = 0;

        //       If needed, slide 0 through i-1 down to 1..i, to make room
        //       at [0]
        //
        if (i != 0)
        {
            for (j = i; j > 0; j--)
            {
                regex_cache[j].preg      = regex_cache[j - 1].preg;
                regex_cache[j].regex     = regex_cache[j - 1].regex;
                regex_cache[j].regex_len = regex_cache[j - 1].regex_len;
                regex_cache[j].cflags    = regex_cache[j - 1].cflags;
                regex_cache[j].status    = regex_cache[j - 1].status;
            }
            i = 0;
        }

        //   and always stuff the _temps (which are correct) in at [0]

#elif defined (REGEX_CACHE_RANDOM_ACCESS)
        if (internal_trace)
        {
            fputs(" [-> rex cache random]\n", stderr);
        }
        //
        //      In a random access system, we just overwrite the single
        //      slot that we expected our regex to be in...

        //                           Free the resources first, if needed.
        //
        if (regex_cache[i].preg != NULL)
        {
            regfree(regex_cache[i].preg);
            free(regex_cache[i].preg);
        }
        if (regex_cache[i].regex != NULL)
            free(regex_cache[i].regex);
        regex_cache[i].regex = NULL;
        regex_cache[i].regex_len = 0;

        //   and  stuff the _temps (which are correct) in at [i]
#else
#error "Must have #define'd ONE of these: REGEX_CACHE_RANDOM_ACCESS, REGEX_CACHE_LINEAR_SEARCH"
#endif
        regex_cache[i].preg      = ppreg_temp;
        regex_cache[i].regex     = regex_temp;
        regex_cache[i].regex_len = rlen_temp;
        regex_cache[i].status    = status_temp;
        regex_cache[i].cflags    = cflags_temp;
    }

    //  Just about done.  Set up the return preg..
    memcpy(preg, ppreg_temp, rtsize);
    if (internal_trace)
        fprintf(stderr, " About to return:\n"
                        " preg->re_nsub=%d preg->value=%p, i=%d, status=%d\n",
            (int)preg->re_nsub, preg->value, i, regex_cache[i].status);
    return regex_cache[i].status;
}
#endif //CRM_REGEX_CACHESIZE == 0
}


//
//
//       How to do a regex execution from the compiled register
//
int crm_regexec(regex_t *preg, const char *string, int string_len,
                size_t nmatch, regmatch_t pmatch[], int eflags,
                const char *aux_string)
{
    if (!string)
    {
        nonfatalerror("crm_regexec - Regular Expression Execution Problem:\n",
            "NULL pointer to the string to match .");
        return REG_NOMATCH;
    }
    if (aux_string == NULL
        || strlen(aux_string) < 1)
    {
        return regnexec(preg, string, string_len, nmatch, pmatch, eflags);
    }
    else
    {
        int i;
        //  parse out the aux string for approximation parameters
        regamatch_t mblock;
        regaparams_t pblock;
        mblock.nmatch = nmatch;
        mblock.pmatch = pmatch;
        pblock.cost_subst = 0;
        pblock.cost_ins = 0;
        pblock.max_cost = 0;
        pblock.cost_del = 0;
        if (4 != sscanf(aux_string, "%d %d %d %d",
                &pblock.cost_subst,
                &pblock.cost_ins,
                &pblock.max_cost,
                &pblock.cost_del))
        {
            fatalerror("Failed to decode 4 numeric cost parameters for approximate matching ",
                aux_string);
        }
        if (user_trace)
            fprintf(stderr,
                "Using approximate match.  Costs: Subst %d Ins %d Max %d Del %d\n",
                pblock.cost_subst,
                pblock.cost_ins,
                pblock.max_cost,
                pblock.cost_del);

        //  now we can run the actual match
        i = reganexec(preg, string, string_len, &mblock, pblock, eflags);
        if (user_trace)
            fprintf(stderr, "approximate Regex match returned %d.\n", i);
        return i;
    }
}


size_t crm_regerror(int errorcode, regex_t *preg, char *errbuf,
                    size_t errbuf_size)
{
    return regerror(errorcode, preg, errbuf, errbuf_size);
}

void crm_regfree(regex_t *preg)
{
#if CRM_REGEX_CACHESIZE > 0
    //  nothing!  yes indeed, if we are using cacheing, we don't free
    //  till and unless we decache, so crm_regfree is a noop.
    return;

#else
    regfree(preg);
#endif
}

char *crm_regversion(void)
{
    static char vs[129];

    CRM_ASSERT(strlen(tre_version()) < 129);
    strcat(vs, (char *)tre_version());
    return vs;
}


void free_regex_cache(void)
{
    int i;

    for (i = 0; i < WIDTHOF(regex_cache); i++)
    {
        if (regex_cache[i].preg != NULL)
        {
            regfree(regex_cache[i].preg);
            free(regex_cache[i].preg);
        }
        if (regex_cache[i].regex != NULL)
        {
            free(regex_cache[i].regex);
        }
    }
    memset(regex_cache, 0, sizeof(regex_cache));
}


#endif

