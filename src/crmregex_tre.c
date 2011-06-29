//	crmregex_tre.c -  CRM114 Regex redirection bounce package for TRE regex

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//	This file bounces CRM114 regex requests to whichever regex package
//	has been compiled and linked in to CRM114.
//
//	Adding a new regex package is relatively easy- just mimic the
//	ifdef stanzas below to map the functions
//
//         crm_regcomp
//         crm_regexec
//         crm_regerror
//         crm_regfree
//         crm_regversion
//
//	into whatever calls your preferred regex package uses.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

 //  Cache for regex compilations
typedef struct {
  char *regex;
  regex_t reg;
  long regex_len;
  int cflags;
  int status;
} REGEX_CACHE_BLOCK;


#if CRM_REGEX_CACHESIZE > 0
REGEX_CACHE_BLOCK regex_cache[CRM_REGEX_CACHESIZE] =
  { { NULL, {0, NULL}, 0, 0, 0} } ;
#endif	// CRM_REGEX_CACHESIZE > 0

// debug helper: print a counted regex on stderr, quoted, with trimmings
static void fpe_regex(char *before, char *regex, long regex_len, char *after)
{
  long i;

  if (before != NULL)
    fprintf(stderr, "%s", before);
  fputc('"', stderr);
  for (i = 0; i < regex_len; i++)
    fputc(regex[i], stderr);
  fputc('"', stderr);
  if (after != NULL)
    fprintf(stderr, "%s", after);
}

#if CRM_REGEX_CACHESIZE > 0

// debug helper: print supplied description, cache bucket number, regex
static void fpe_mishmash(char *str, unsigned int i, char *regex, int regex_len)
{
  char tmp[128];	// make sure this is big enough

  sprintf(tmp, "%sregex_cache[%u]: ", str, i);
  fpe_regex(tmp, regex, regex_len, "\n");
}

// debug helper: print a cache bucket with a supplied description
static void fpe_bucket(char *str, unsigned int i)
{
  fpe_mishmash(str, i, regex_cache[i].regex, regex_cache[i].regex_len);
}

#endif	//  CRM_REGEX_CACHESIZE > 0

//
//      How to do a register compilation
//
int crm_regcomp (regex_t *preg, char *regex, long regex_len, int cflags)
{
  //       compile it with the TRE regex compiler
  //
  //    bug workaround - many regex compilers don't compile the null
  //    regex correctly, but _do_ compile "()" correctly, which
  //    matches the same thing).
  if (regex_len == 0)
    {
      return (regncomp (preg, "()", 2, cflags));
    };

  //   Are we cacheing compiled regexes?  Maybe not...
#if CRM_REGEX_CACHESIZE == 0

  if (internal_trace)
    fpe_regex("compiling regex ", regex, regex_len, "\n");

  return ( regncomp (preg, regex, regex_len, cflags));

#else	// !CRM_REGEX_CACHESIZE == 0

  // We are cacheing.  Scan our cache for the compiled version of this
  // regex.  A NULL pointer to regex means "empty bucket".
  {
    unsigned int i;	// subscript of bucket found or filled
			// ..unsigned cuz strnhash() val can have high bit set
    int found_it;	// boolean
    REGEX_CACHE_BLOCK new;

#ifdef REGEX_CACHE_LINEAR_SEARCH
    //
    //          Linear Search uses a strict LRU algorithm to cache
    //          the precompiled regexes, where used means compiled.
    //
    found_it = 0;
    for (i = 0; i < CRM_REGEX_CACHESIZE && regex_cache[i].regex != NULL; i++)
      {
	if (regex_len == regex_cache[i].regex_len
	    && cflags == regex_cache[i].cflags
	    && strncmp (regex_cache[i].regex, regex, regex_len) == 0)
	  {
	    //  We Found It!  i is where
	    found_it = 1;
	    break;		// don't increment i
	  };
      };

    if (i == CRM_REGEX_CACHESIZE)	// ran off end, not found, cache full
      i = CRM_REGEX_CACHESIZE - 1;	// bucket to throw away
#endif	// REGEX_CACHE_LINEAR_SEARCH

#ifdef REGEX_CACHE_RANDOM_ACCESS
    //
    //             Random Access uses an associative cache based on
    //             the hash of the regex (mod the size of the cache).
    //
    found_it = 0;
    i = strnhash (regex, regex_len) % (unsigned)CRM_REGEX_CACHESIZE;
    if (regex_cache[i].regex != NULL
	&& regex_len == regex_cache[i].regex_len
	&& cflags == regex_cache[i].cflags
	&& strncmp (regex_cache[i].regex, regex, regex_len) == 0)
      {
	//  We Found It!  i is where
	found_it = 1;
      };
#endif	// REGEX_CACHE_RANDOM_ACCESS

    if (internal_trace)
      fpe_mishmash((found_it ? "found in " : "not found in "),
		   i, regex, regex_len);
    if ( ! (found_it))
      {
	// copy and compile new regex into new
	new.regex = (char *) malloc (regex_len);
	if (new.regex == NULL)
	  fatalerror5("Can't allocate cache copy of new regex", "",
		      CRM_ENGINE_HERE);
	memcpy (new.regex, regex, regex_len);
	new.regex_len = regex_len;
	new.cflags = cflags;
	new.status =
	  regncomp (&new.reg, new.regex, new.regex_len, new.cflags);

	// i is the bucket to throw away, if any
	// i may or may not be where new stuff will go
	if (regex_cache[i].regex != NULL)
	  {
	    if (internal_trace)
	      fpe_bucket("discarding ", i);
	    regfree (&regex_cache[i].reg);
	    free (regex_cache[i].regex);
	  }
      }

#ifdef REGEX_CACHE_LINEAR_SEARCH
    if ( !found_it)
      {
	// i is first free; shift array up one into bucket i
	while (i > 0)
	  {
	    regex_cache[i] = regex_cache[i - 1];
	    i--;
	  }
	// i is now 0, which is where to put the new stuff
      };
#endif	// REGEX_CACHE_LINEAR_SEARCH

    if ( !found_it)
      {
	// for both cache algorithms, i is now the bucket
	// to fill in with the new regex
	regex_cache[i] = new;
	if (internal_trace)
	  fpe_bucket("new ", i);
      }

    //  Just about done.  Set up the return values.
    *preg = regex_cache[i].reg;

    return (regex_cache[i].status);
  };
#endif	// !CRM_REGEX_CACHESIZE == 0
}
//
//
//       How to do a regex execution from the compiled register
//
int crm_regexec ( regex_t *preg, char *string, long string_len,
		 size_t nmatch, regmatch_t pmatch[], int eflags,
		  char *aux_string)
{
  if (!string)
    {
      nonfatalerror5("crm_regexec - Regular Expression Execution Problem:\n",
		     "NULL pointer to the string to match .", CRM_ENGINE_HERE);
      return (REG_NOMATCH);
    };
  if (aux_string == NULL
      || strlen (aux_string) < 1)
    {
      return (regnexec (preg, string, string_len, nmatch, pmatch, eflags));
    }
  else
    {
      int i;
      //  parse out the aux string for approximation parameters
      regamatch_t mblock;
      regaparams_t pblock;
      mblock.nmatch = nmatch;
      mblock.pmatch = pmatch;
      sscanf (aux_string, "%d %d %d %d",
	      &pblock.cost_subst,
	      &pblock.cost_ins,
	      &pblock.max_cost,
	      &pblock.cost_del);
      if (user_trace)
	fprintf (stderr,
	 "Using approximate match.  Costs: Subst %d Ins %d Max %d Del %d \n",
		 pblock.cost_subst,
		 pblock.cost_ins,
		 pblock.max_cost,
		 pblock.cost_del);

      //  now we can run the actual match
      i = reganexec (preg, string, string_len, &mblock, pblock, eflags);
      if (user_trace)
	fprintf (stderr, "approximate Regex match returned %d .\n", i);
      return (i);
    };
}


size_t crm_regerror (int errorcode, regex_t *preg, char *errbuf,
		     size_t errbuf_size)

{
  return (regerror (errorcode, preg, errbuf, errbuf_size));
};

void crm_regfree (regex_t *preg)
{
#if CRM_REGEX_CACHESIZE > 0
  //  nothing!  yes indeed, if we are using cacheing, we don't free
  //  till and unless we decache, so crm_regfree is a noop.
  return;
#else	// !CRM_REGEX_CACHESIZE > 0
   return (regfree (preg));
#endif	// !CRM_REGEX_CACHESIZE > 0
};

char *crm_regversion ()
{
  return (tre_version());
};
