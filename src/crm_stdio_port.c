//  crm_win32_port.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//

// make sure the stdin/out/err redef trick is NOT done in sysincludes:
#define DONT_REDEF_STDIN_OUT_ERR 1

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"




FILE *crm_stdin = NULL;
FILE *crm_stderr = NULL;
FILE *crm_stdout = NULL;




FILE *os_stdin(void)
{
  return stdin;
}

FILE *os_stdout(void)
{
  return stdout;
}

FILE *os_stderr(void)
{
  return stderr;
}


void init_stdin_out_err_as_os_handles(void)
{
  crm_stdin = os_stdin();
  crm_stdout = os_stdout();
  crm_stderr = os_stderr();
}


void cleanup_stdin_out_err_as_os_handles(void)
{
  if (crm_stdin != NULL && crm_stdin != os_stdin())
  {
    fclose(crm_stdin);
    crm_stdin = NULL;
  }
  if (crm_stdout != NULL && crm_stdout != os_stdout() && crm_stdout != os_stderr() && crm_stdout != crm_stderr)
  {
    fclose(crm_stdout);
    crm_stdout = NULL;
  }
  if (crm_stderr != NULL && crm_stderr != os_stdout() && crm_stderr != os_stderr())
  {
    fclose(crm_stderr);
    crm_stderr = NULL;
  }
}


int is_stdin_or_null(FILE *f)
{
  return (f == NULL) || (f == os_stdin());
}

int is_stdout_err_or_null(FILE *f)
{
  return (f == NULL) || (f == os_stdout()) || (f == os_stderr());
}

