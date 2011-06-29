//     These versions of the error handling routines are to be used ONLY
//     in utilities, but not in the CRM114 engine itself; these don't do
//     what you need for the full crm114 runtime.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

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
long fatalerror5 ( char *str1, char *str2)
{
  fprintf (stderr, "ERROR: %s%s \n", str1, str2);
  exit (-1);
}
long nonfatalerror5 ( char *str1, char *str2)
{
  fprintf (stderr, "ERROR: %s%s \n", str1, str2);
  exit (-1);
}
long untrappableerror5 ( char *str1, char *str2)
{
  fprintf (stderr, "ERROR: %s%s \n", str1, str2);
  exit (-1);
}

