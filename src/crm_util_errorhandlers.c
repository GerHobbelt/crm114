//	crm_util_errorhandlers.c - Error handling routines to be used ONLY
//	in utilities, not in the CRM114 engine itself; these don't do
//	what you need for the full crm114 runtime.

// Copyright 2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

#include <stdio.h>
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
