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

//#include "crm114_sysincludes.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <locale.h>
#include <sys/times.h>
#include <signal.h>

#include <regex.h>

//
//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

//
//      How to do a register compilation
//
int crm_regcomp (regex_t *preg, char *regex, long regex_len, int cflags)
{
  static int null_errored = 0;
  //   Gnu REGEX can't handle embedded NULs in the pattern
  if (strlen (regex) < regex_len)
    {
      if (null_errored == 0)
	{
	  fatalerror5 ("The regex contains a NUL inside the stated length,",
		       "but your GNU regex library can't handle embedded NULs.  Therefore, treat all results WITH GREAT SUSPICION.", CRM_REGEX_HERE);
	  null_errored = 1;
	};
    };
  //  
  //   bug workaround for regex libraries that can't compile the null regex
  if (regex_len == 0)
    return (regcomp (preg, "()", cflags));
  //
  //   If we get here, we're OK on GNU Regex
  return (regcomp ( preg, regex, cflags));
}
//
//
//       How to do a regex execution from the compiled register
//
int crm_regexec ( regex_t *preg, char *string, long string_len,
		 size_t nmatch, regmatch_t pmatch[], int eflags, 
		  char *aux_string)
{
  static int null_errored = 0;
  int savedcrockchar;
  int regexresult;
  
  //   GRODY GRODY GRODY !!!  If using the GNU (or other POSIX) regex
  //   libraries, we have to crock in a NULL to end the regex search.
  //   We have to insert a NULL because the GNU regex libraries are
  //   set up on ASCIZ strings, not start/length strings.

  savedcrockchar = string[ string_len +1 ];
  string [ string_len + 1 ] = '\000';
  if (internal_trace)
    {
      fprintf (stderr, "    crocking in a NULL for the %c\n", 
	     savedcrockchar);
    };

  if (strlen (string) < string_len)
    {
      if (null_errored == 0)
	{
	  fprintf (stderr, "\nRegexec  strlen: %d, stated_len: %ld \n",
		   strlen (string), string_len);
	  nonfatalerror5 ("Your data window contained a NUL inside the stated length,",
			  "and the GNU regex libraries can't handle embedded NULs.  Treat all results with GREAT SUSPICION.", CRM_REGEX_HERE);
	  null_errored = 1;
	}
    };
  regexresult = regexec ( preg, string, nmatch, pmatch, eflags);
  
  //    and de-crock the nulled character
  string [ string_len + 1] = savedcrockchar;

  return (regexresult);
}


size_t crm_regerror (int errorcode, regex_t *preg, char *errbuf,
		     size_t errbuf_size)

{
  return (regerror (errorcode, preg, errbuf, errbuf_size));
}

void crm_regfree (regex_t *preg)
{
    return (regfree (preg));
};

char * crm_regversion ()
{
  static char verstr [128] = "Gnu Regex" ;

  return (verstr);
};
