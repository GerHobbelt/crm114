//     These versions of the error handling routines are to be used ONLY
//     in utilities, but not in the CRM114 engine itself; these don't do
//     what you need for the full crm114 runtime.
//
//  include some standard files
#include "crm114_sysincludes.h"

long fatalerror ( char *str1, char *str2)
{
  fprintf (stderr, "ERROR: %s%s \n", str1, str2);
  exit (-1);
}
long nonfatalerror ( char *str1, char *str2)
{
  fprintf (stderr, "ERROR: %s%s \n", str1, str2);
  exit (-1);
}
long untrappableerror ( char *str1, char *str2)
{
  fprintf (stderr, "ERROR: %s%s \n", str1, str2);
  exit (-1);
}

