//	crm_expr_syscall.c - system call expression handling

// Copyright 2001-2009 William S. Yerazunis.
// This file is under GPLv3, as described in COPYING.

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
extern char *inbuf;
extern char *outbuf;

#ifndef CRM_WINDOWS
// Normal options for UNIX/Linux

#else	// CRM_WINDOWS
typedef struct
{
  HANDLE to_minion;
  char *inbuf;
  long inlen;
  long internal_trace;
  long keep_proc;
} pusherparams;

typedef struct
{
  HANDLE from_minion;
  int timeout;
} suckerparams;

DWORD WINAPI pusher_proc(LPVOID lpParameter)
{
  DWORD bytesWritten;
  pusherparams *p = (pusherparams *)lpParameter;
  WriteFile(p->to_minion, p->inbuf, p->inlen, &bytesWritten, NULL);
  free(p->inbuf);
  if (p->internal_trace)
    fprintf (stderr, "pusher: input sent to minion.\n");

  //    if we don't want to keep this proc, we close it's input, and
  //    wait for it to exit.
  if (! p->keep_proc)
    {
      CloseHandle (p->to_minion);
      if (internal_trace)
        fprintf (stderr, "minion input pipe closed\n");
    }
  if (p->internal_trace)
    fprintf (stderr, "pusher: exiting pusher\n");
  return 0;
}

DWORD WINAPI sucker_proc(LPVOID lpParameter)
{
  DWORD bytesRead;
  suckerparams *p = (suckerparams *)lpParameter;
  char *outbuf = malloc(sizeof(char) * 8192);

  //  we're in the sucker process here- just throw away
  //  everything till we get EOF, then exit.
  while (1)
    {
      Sleep (p->timeout);
      ReadFile(p->from_minion, outbuf,
               8192, &bytesRead, NULL);
      if (bytesRead == 0) break;
    };
  return 0;
}
#endif	// CRM_WINDOWS

int crm_expr_syscall ( CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  //           Go off and fork a process, sending that process
  //           one pattern evaluated as input, and then accepting
  //           all the returns from that process as the new value
  //           for a variable.
  //
  //           syntax is:
  //               exec (:to:) (:from:) (:ctl:) /commandline/
  long inlen;
  long outlen;
  char from_var [MAX_VARNAME];
  char sys_cmd [MAX_PATTERN];
  long cmd_len;
  char keep_buf [MAX_PATTERN];
  long keep_len;
  char exp_keep_buf[MAX_PATTERN];
  long exp_keep_len;
  long vstart;
  long vlen;
  long done, charsread;
  int keep_proc;
  int async_mode;
  int to_minion[2];
  int from_minion[2];
  pid_t minion;
  int minion_exit_status;
  pid_t pusher;
  pid_t sucker;
  pid_t random_child;
  int status;
  long timeout;

#ifndef CRM_WINDOWS
  if (user_trace)
    fprintf (stderr, "executing an SYSCALL statement");
  timeout = MINION_SLEEP_USEC;

  //  clean up any prior processes - note that
  //  we don't keep track of these.  For that matter, we have
  //  no context to keep track of 'em.
  //
  while ( (random_child = waitpid ( 0, &status, WNOHANG)) > 0 );

#else	// CRM_WINDOWS
  SECURITY_ATTRIBUTES pipeSecAttr;
  HANDLE hminion;
  timeout = MINION_SLEEP_USEC / 1000;   // need milliseconds for Sleep()

  if (MINION_SLEEP_USEC > 0 && timeout < 1)
    {
      timeout = 1;
    }

#endif	// CRM_WINDOWS

  //    get the flags
  //
  keep_proc = 0;
  if (apb->sflags & CRM_KEEP)
    {
      if (user_trace)
	fprintf (stderr, "Keeping the process around if possible\n");
      keep_proc = 1;
    };
  async_mode = 0;
  if (apb->sflags & CRM_ASYNC)
    {
      if (user_trace)
	fprintf (stderr, "Letting the process go off on it's own");
      async_mode = 1;
    };

  //     Sanity check - <async> is incompatible with <keep>
  //
  if (keep_proc && async_mode)
    {
      nonfatalerror5 ("This syscall uses both async and keep, but async is "
		     "incompatible with keep.  Since keep is safer"
		     "we will use that.\n",
		      "You need to fix this program.", CRM_ENGINE_HERE);
      async_mode = 0;
    };

  //    get the input variable(s)
  //
  crm_get_pgm_arg (inbuf, data_window_size, apb->p1start, apb->p1len);
  inlen = crm_nexpandvar (inbuf, apb->p1len, data_window_size);
  if (user_trace)
    fprintf (stderr, "  command's input wil be: ***%s***\n", inbuf);

  //    now get the name of the variable where the return will be
  //    placed... this is a crock and should be fixed someday.
  //    the output goes only into a single var (the first one)
  //    so we extract that
  //
  crm_get_pgm_arg (from_var, MAX_PATTERN, apb->p2start, apb->p2len);
  outlen = crm_nexpandvar (from_var, apb->p2len, MAX_PATTERN);
  done = 0;
  vstart = 0;
  while (from_var[vstart] < 0x021 && from_var[vstart] > 0x0 )
    vstart++;
  vlen = 0;
  while (from_var[vstart+vlen] >= 0x021)
    vlen++;
  memmove (from_var, &from_var[vstart], vlen);
  from_var[vlen] = '\000';
  if (user_trace)
    fprintf (stderr, "   command output will overwrite var ***%s***\n",
	     from_var);


  //    now get the name of the variable (if it exists) where
  //    the kept-around minion process's pipes and pid are stored.
  crm_get_pgm_arg (keep_buf, MAX_PATTERN, apb->p3start, apb->p3len);
  keep_len = crm_nexpandvar (keep_buf, apb->p3len, MAX_PATTERN);
  if (user_trace)
    fprintf (stderr, "   command status kept in var ***%s***\n",
	     keep_buf);

  //      Get the command to execute
  //
  //    GROT GROT GROT
  //      In retrospect, putting the command to execute in /slashes/
  //      was a design error.  It's not a pattern to match, it's a
  //      source to operate on (in the meta sense, at least).  And,
  //      from a practical point of view, it means that pathnames with
  //      embedded slashes are a pain in the neck to write.  So- we'll
  //      allow the boxed [string] syntax as well as the slash /string/
  //      syntax for now.
  //    GROT GROT GROT
  if (apb->s1len > 0)
    {
      crm_get_pgm_arg (sys_cmd, MAX_PATTERN, apb->s1start, apb->s1len);
      cmd_len = crm_nexpandvar (sys_cmd, apb->s1len, MAX_PATTERN);
    };
  if (apb->b1len > 0)
    {
      crm_get_pgm_arg (sys_cmd, MAX_PATTERN, apb->b1start, apb->b1len);
      cmd_len = crm_nexpandvar (sys_cmd, apb->b1len, MAX_PATTERN);
    };

  if (user_trace)
    fprintf (stderr, "   command will be ***%s***\n", sys_cmd);

  //     Do we reuse an already-existing process?  Check to see if the
  //     keeper variable has it... note that we have to :* prefix it
  //     and expand it again.
  minion = 0;
  to_minion[0] = 0;
  from_minion[1] = 0;
  exp_keep_buf [0] = '\000';
  //  this is 8-bit-safe because vars are never wchars.
  strcat (exp_keep_buf, ":*");
  strncat (exp_keep_buf, keep_buf, keep_len);
  exp_keep_len = crm_nexpandvar (exp_keep_buf, keep_len+2, MAX_PATTERN);
  sscanf (exp_keep_buf, "MINION PROC PID: %d from-pipe: %d to-pipe: %d",
	  &minion,
	  &from_minion[0],
	  &to_minion[1]);

#ifndef CRM_WINDOWS
  //      if, no minion already existing, we create
  //      communications pipes and launch the subprocess.  This
  //      code borrows concepts from both liblaunch and from
  //      netcat (thanks, *Hobbit*!)
  //
  if (minion == 0)
    {
      long status1, status2;
      if (user_trace)
	fprintf (stderr, "  Must start a new minion.\n");
      status1 = pipe (to_minion);
      status2 = pipe (from_minion);
      if (status1 > 0 || status2 > 0)
	{
	  nonfatalerror5 ("Problem setting up the to/from pipes to a minion. ",
			  "Perhaps the system file descriptor table is full?",
			  CRM_ENGINE_HERE);
	  return (1);
	};
      minion = fork();

      if (minion < 0)
	{
	  nonfatalerror5 ("Tried to fork your minion, but it failed.",
			  "Your system may have run out of process slots",
			  CRM_ENGINE_HERE);
	  return (1);
	};

      if (minion == 0)
	{   //  START OF IN THE MINION
	  //
	  //    if minion == 0, then We're in the minion here
	  int retcode;
	  long vstart, vlen;
	  long varline;
	  //    close the ends of the pipes we don't need.
	  //
	  //    NOTE: if this gets messed up, you end up with a race
	  //    condition, because both master and minion processes
	  //    can both read and write both pipes (effectively a
	  //    process could write something out, then read it again
	  //    right back out of the pipe)!  So, it's REALLY REALLY
	  //    IMPORTANT that you use two pipe structures, (one for
	  //    each direction) and you keep track of which process
	  //    should write to which pipe!!!
	  //
	  close (to_minion[1]);
	  close (from_minion[0]);
	  dup2 (to_minion[0], fileno(stdin));
	  dup2 (from_minion[1], fileno(stdout));

	  //     Are we a syscall to a :label:, or should we invoke the
	  //     shell on an external command?
	  //
	  crm_nextword (sys_cmd, strlen (sys_cmd), 0, &vstart, &vlen);
	  varline = crm_lookupvarline (vht, sys_cmd, vstart, vlen);
	  if (varline > 0)
	    {
	      //	      sys_cmd[vstart+vlen] = '\0';
	      if (user_trace)
		fprintf (stderr, "FORK transferring control to line %s\n",
			 &sys_cmd[vstart]);

	      //    set the current pid and parent pid.
	      {
		char pidstr [32];
		long pid;
		pid = (long) getpid();
		sprintf (pidstr, "%ld", pid);
		crm_set_temp_var (":_pid:", pidstr);
		if (user_trace)
		  fprintf (stderr, "My new PID is %s\n", pidstr);
		pid = (long) getppid();
		sprintf (pidstr, "%ld", pid);
		crm_set_temp_var (":_ppid:", pidstr);
	      }
	      //   See if we have redirection of stdin and stdout
	      while (crm_nextword (sys_cmd, strlen (sys_cmd), vstart+vlen,
				   &vstart, &vlen))
		{
		  char filename[MAX_PATTERN];
		  if (sys_cmd[vstart] == '<')
		    {
		      strncpy (filename, &sys_cmd[vstart+1], vlen);
		      filename[vlen-1] = '\0';
		      if (user_trace)
			fprintf (stderr, "Redirecting minion stdin to %s\n",
				 filename);
		      dontcareptr = freopen (filename, "rb", stdin);
		    };
		  if (sys_cmd[vstart] == '>')
		    {
		      if (sys_cmd[vstart+1] != '>')
			{
			  strncpy (filename, &sys_cmd[vstart+1], vlen);
			  filename[vlen-1] = '\0';
			  if (user_trace)
			    fprintf (stderr,
				     "Redirecting minion stdout to %s\n",
				     filename);
			  dontcareptr = freopen (filename, "wb", stdout);
			}
		      else
			{
			  strncpy (filename, &sys_cmd[vstart+2], vlen);
			  filename[vlen-2] = '\0';
			  if (user_trace)
			    fprintf (stderr,
				     "Appending minion stdout to %s\n",
				     filename);
			  dontcareptr =  freopen (filename, "a+", stdout);
			}
		    };
		}
	      csl->cstmt = varline;
	      //   and note that this isn't a failure.
	      csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = 1;
	      //   The minion's real work should now start; get out of
	      //   the syscall code and go run something real.  :)
	      return (0);
	    }
	  else
	    {
	      if (user_trace)
		fprintf (stderr, "Systemcalling on shell command %s\n",
			 sys_cmd);
	      retcode = system (sys_cmd);
	      //
	      //       This code only ever happens if an error occurs...
	      //
	      if (retcode == -1 )
		{
		  char errstr [4096];
		  sprintf (errstr,
			   "The command was >%s< and returned exit code %d .",
			   sys_cmd, WEXITSTATUS (retcode));
		  nonfatalerror5 ("This program tried a shell command that "
				 "didn't run correctly. ",
				  errstr, CRM_ENGINE_HERE);
		  if (engine_exit_base != 0)
		    {
		      exit (engine_exit_base + 11);
		    }
		  else
		    exit (WEXITSTATUS (retcode ));
		};
	      exit ( WEXITSTATUS (retcode) );
	    };
	};      //    END OF IN THE MINION
    }
  else
    {
      if (user_trace)
	fprintf (stderr, "  reusing old minion PID: %d\n", minion);
    };
  //      Now, we're out of the minion for sure.
  //    so we close the pipe ends we know we won't be using.
  if (to_minion[0] != 0)
    {
      close (to_minion[0]);
      close (from_minion[1]);
    };
  //
  //   launch "pusher" process to send the buffer to the minion
  //    (this hint from Dave Soderberg).  This avoids the deadly
  //   embrace situation where both processes are waiting to read
  //   (or, equally, both processes have written and filled up
  //   their buffers, and are now held up waiting for the other
  //   process to empty some space in the output buffer)
  //
  if (strlen (inbuf) > 0)
    {
      pusher = fork ();
      //    we're in the "input pusher" process if we got here.
      //    shove the input buffer out to the minion
      if (pusher == 0)
	{
	  dontcare = write (to_minion[1], inbuf, inlen );
	  if (internal_trace)
	    fprintf (stderr, "pusher: input sent to minion.\n");
	  close (to_minion[1]);
	  if (internal_trace)
	    fprintf (stderr, "pusher: minion input pipe closed\n");
	  if (internal_trace)
	    fprintf (stderr, "pusher: exiting pusher\n");
	  //  The pusher always exits with success, so do NOT
	  //  do not use the engine_exit_base value
	  exit ( EXIT_SUCCESS );
	};
    };
  //    now we're out of the pusher process.
  //    if we don't want to keep this proc, we close it's input, and
  //    wait for it to exit.
  if (! keep_proc)
    {
      close (to_minion[1]);
      if (internal_trace)
	fprintf (stderr, "minion input pipe closed\n");
    }

  //   and see what is in the pipe for us.
  outbuf[0] = '\000';
  done = 0;
  outlen = 0;
  //   grot grot grot this only works if varnames are not widechars
  if (strlen (from_var) > 0)
    {
      if (async_mode == 0 && keep_proc == 0)
	{
	  usleep (timeout);
	  //   synchronous read- read till we hit EOF, which is read
	  //   returning a char count of zero.
	readloop:
	  if (internal_trace) fprintf (stderr, "SYNCH READ ");
	  usleep (timeout);
	  charsread =
	    read (from_minion[0],
		  &outbuf[done],
		  (data_window_size >> SYSCALL_WINDOW_RATIO) - done - 2);
	  done = done + charsread;
	  if ( charsread > 0
	       && done + 2 < (data_window_size >> SYSCALL_WINDOW_RATIO))
	    goto readloop;
	  if (done < 0) done = 0;
	  outbuf [done] = '\000';
	  outlen = done ;
	};
      if (keep_proc == 1 || async_mode == 1)
	{
	  //   we're in either 'keep' 'async' mode.  Set nonblocking mode, then
	  //   read it once; then put it back in regular mode.
	  //fcntl (from_minion[0], F_SETFL, O_NONBLOCK);
	  //	  usleep (timeout);
	  charsread = read (from_minion[0],
			    &outbuf[done],
			    (data_window_size >> SYSCALL_WINDOW_RATIO));
	  done = charsread;
	  if (done < 0) done = 0;
	  outbuf [done] = '\000';
	  outlen = done ;
	  //fcntl (from_minion[0], F_SETFL, 0);
	};

      //   If the minion process managed to fill our buffer, and we
      //   aren't "keep"ing it around, OR if the process is "async",
      //   then we should also launch a sucker process to
      //   asynchronously eat all of the stuff we couldn't get into
      //   the buffer.  The sucker proc just reads stuff and throws it
      //   away asynchronously... and exits when it gets EOF.
      //
      if ( async_mode ||
	   (outlen >= ((data_window_size >> SYSCALL_WINDOW_RATIO) - 2 )
	    && keep_proc == 0))
	{
	  sucker = fork ();
	  if (sucker == 0)
	    {
	      //  we're in the sucker process here- just throw away
	      //  everything till we get EOF, then exit.
	      while (1)
		{
		  usleep (timeout);
		  charsread = read (from_minion[0],
				    &outbuf[0],
				    data_window_size >> SYSCALL_WINDOW_RATIO );
		  //  in the sucker here, don't use engine_exit_base exit
		  if (charsread == 0) exit (EXIT_SUCCESS);
		};
	    };
	};

      //  and set the returned value into from_var.
      if (user_trace)
	fprintf (stderr, "SYSCALL output: %ld chars ---%s---.\n ",
		 outlen, outbuf);
      if (internal_trace)
	fprintf (stderr, "  storing return str in var %s\n", from_var);

      crm_destructive_alter_nvariable ( from_var, vlen, outbuf, outlen);
    };

  //  Record useful minion data, if possible.
  if (strlen (keep_buf) > 0)
    {
      sprintf (exp_keep_buf,
	       "MINION PROC PID: %d from-pipe: %d to-pipe: %d",
	       minion,
	       from_minion[0],
	       to_minion[1]);
      if (internal_trace)
	fprintf (stderr, "   saving minion state: %s \n",
		 exp_keep_buf);
      crm_destructive_alter_nvariable (keep_buf, keep_len,
				       exp_keep_buf,
				       strlen (exp_keep_buf));
    };
  //      If we're keeping this minion process around, record the useful
  //      information, like pid, in and out pipes, etc.
  if (keep_proc || async_mode)
    {
    }
  else
    {
      if (internal_trace)
	fprintf (stderr, "No keep, no async, so not keeping minion, closing everything.\n");

      //    de-zombify any dead minions;
      waitpid ( minion, &minion_exit_status, 0);

      //   we're not keeping it around, so close the pipe.
      //
      close (from_minion [0]);

      if ( crm_vht_lookup (vht, keep_buf, strlen (keep_buf)))
	{
	  char exit_value_string[MAX_VARNAME];
	  if (internal_trace)
	    fprintf (stderr, "minion waitpid result :%d; whacking %s\n",
		     minion_exit_status,
		     keep_buf);
	  sprintf (exit_value_string, "DEAD MINION, EXIT CODE: %d",
		   WEXITSTATUS (minion_exit_status));
	  if (keep_len > 0)
	    crm_destructive_alter_nvariable (keep_buf, keep_len,
					   exit_value_string,
					   strlen (exit_value_string));
	};
    };
#else	// CRM_WINDOWS
  //      if, no minion already existing, we create
  //      communications pipes and launch the subprocess.  This
  //      code borrows concepts from both liblaunch and from
  //      netcat (thanks, *Hobbit*!)
  //
  if (minion == 0)
    {
      int retcode;
      long vstart, vlen;
      long varline;

      if (user_trace)
        fprintf (stderr, "  Must start a new minion.\n");

      pipeSecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
      pipeSecAttr.bInheritHandle = TRUE;
      pipeSecAttr.lpSecurityDescriptor = NULL;

      status = CreatePipe(&to_minion[0], &to_minion[1], &pipeSecAttr, 2^10 * 32);
      status = CreatePipe(&from_minion[0], &from_minion[1], &pipeSecAttr, 2^10 * 32);

      crm_nextword (sys_cmd, strlen (sys_cmd), 0, &vstart, &vlen);
      varline = crm_lookupvarline (vht, sys_cmd, vstart, vlen);
      if (varline > 0)
        {
	  fatalerror5 (" Sorry, syscall to a label isn't implemented in this version", "", CRM_ENGINE_HERE);
        }
      else
        {
          STARTUPINFO si;
          PROCESS_INFORMATION pi;
          HANDLE stdout_save, stdin_save;
          HANDLE to_minion_write, from_minion_read;

          stdout_save = GetStdHandle(STD_OUTPUT_HANDLE);
          SetStdHandle(STD_OUTPUT_HANDLE, from_minion[1]);

          stdin_save = GetStdHandle(STD_INPUT_HANDLE);
          SetStdHandle(STD_INPUT_HANDLE, to_minion[0]);

          DuplicateHandle(GetCurrentProcess(), from_minion[0], GetCurrentProcess(), &from_minion_read , 0, FALSE, DUPLICATE_SAME_ACCESS);
          CloseHandle(from_minion[0]);
          from_minion[0] = from_minion_read;

          DuplicateHandle(GetCurrentProcess(), to_minion[1], GetCurrentProcess(), &to_minion_write , 0, FALSE, DUPLICATE_SAME_ACCESS);
          CloseHandle(to_minion[1]);
          to_minion[1] = to_minion_write;

          if (user_trace)
            fprintf (stderr, "systemcalling on shell command %s\n",
                     sys_cmd);


          ZeroMemory( &si, sizeof(si) );
          si.cb = sizeof(si);

          ZeroMemory( &pi, sizeof(pi) );

          retcode = CreateProcess(NULL, sys_cmd, NULL, NULL, TRUE , NULL, NULL, NULL, &si, &pi);

          if (!retcode)
            {
              char errstr [4096];
              sprintf (errstr, "The command was >>%s<< and returned exit code %d .",
                       sys_cmd, retcode);
              fatalerror5 ("This program tried a shell command that "
			   "didn't run correctly. ",
			   errstr, CRM_ENGINE_HERE);
	      { if (engine_exit_base != 0)
		  {
		    exit (engine_exit_base + 13);
		  }
		else
		  exit ( EXIT_FAILURE );
	      }
            }
	  else
            {
              minion = pi.dwProcessId;
              hminion = pi.hProcess;
              SetStdHandle(STD_OUTPUT_HANDLE, stdout_save);
              SetStdHandle(STD_INPUT_HANDLE, stdin_save);
              CloseHandle(pi.hThread);
            }
        };
    }
  else
    {
      if (user_trace)
        fprintf (stderr, "  reusing old minion PID: %d\n", minion);
      hminion = OpenProcess(PROCESS_ALL_ACCESS, 0, minion);
      if (hminion == NULL)
        fatalerror5 ("Couldn't open the existing minion process",
		     "", CRM_ENGINE_HERE);
    };
  //      Now, we're out of the minion for sure.
  //    so we close the pipe ends we know we won't be using.
  if (to_minion[0] != 0)
    {
      CloseHandle (to_minion[0]);
      CloseHandle (from_minion[1]);
    };
  //
  //   launch "pusher" process to send the buffer to the minion
  //    (this hint from Dave Soderberg). This avoids the deadly
  //   embrace situation where both processes are waiting to read
  //   (or, equally, both processes have written and filled up
  //   their buffers, and are now held up waiting for the other
  //   process to empty some space in the output buffer)
  //
  if (strlen (inbuf) > 0)
    {
      HANDLE hThread;
      pusherparams pp;
      char *inbuf_copy = malloc(sizeof(char) * inlen+1);
      int i;
      //Since the pusher thread may continue executing after the
      //syscall statement has finished, we need to make a copy of
      //inbuf for the pusher thread to use. The pusher process will
      //free the memory.
      for (i=0; i<inlen; i++)
        inbuf_copy[i] = inbuf[i];
      inbuf_copy[inlen] = 0;
      pp.inbuf = inbuf_copy;
      pp.inlen = inlen;
      pp.internal_trace = internal_trace;
      pp.keep_proc = keep_proc;
      pp.to_minion = to_minion[1];
      CreateThread(NULL, 0, pusher_proc , &pp , 0, &hThread);
    }

  //   and see what is in the pipe for us.
  outbuf[0] = '\000';
  done = 0;
  outlen = 0;
  //   grot grot grot this only works if varnames are not widechars
  if (strlen (from_var) > 0)
    {
      if (async_mode == 0)
        {
          Sleep (timeout);
          //   synchronous read- read till we hit EOF, which is read
          //   returning a char count of zero.
          readloop:
          if (internal_trace) fprintf (stderr, "SYNCH READ ");
          Sleep (timeout);
          charsread = 0;

          ReadFile(from_minion[0],
                   outbuf + done,
                   (data_window_size >> SYSCALL_WINDOW_RATIO) - done - 2,
                   &charsread, NULL);

          done = done + charsread;
          if (charsread > 0 && done + 2 < (data_window_size >> SYSCALL_WINDOW_RATIO))
            goto readloop;
          if (done < 0) done = 0;
            outbuf [done] = '\000';
          outlen = done ;
        }
      else
        {
          //   we're in 'async' mode. Just grab what we can
          ReadFile(from_minion[0],
                   &outbuf[done],
                   (data_window_size >> SYSCALL_WINDOW_RATIO), &charsread, NULL);
          done = charsread;
          if (done < 0) done = 0;
          outbuf [done] = '\000';
          outlen = done ;

        }

        //   If the minion process managed to fill our buffer, and we
        //   aren't "keep"ing it around, OR if the process is "async",
        //   then we should also launch a sucker process to
        //   asynchronously eat all of the stuff we couldn't get into
        //   the buffer.  The sucker proc just reads stuff and throws it
        //   away asynchronously... and exits when it gets EOF.
        //
        if ( async_mode || (outlen >= ((data_window_size >> SYSCALL_WINDOW_RATIO) - 2 )
             && keep_proc == 0))
          {
            HANDLE hThread;
            suckerparams sp;
            sp.from_minion = from_minion[0];
            sp.timeout = timeout;
            CreateThread(NULL, 0, sucker_proc , &sp , NULL, &hThread);
          }

          //  and set the returned value into from_var.
          if (user_trace)
            fprintf (stderr, "SYSCALL output: %ld chars ---%s---.\n ",
                     outlen, outbuf);
          if (internal_trace)
            fprintf (stderr, "  storing return str in var %s\n", from_var);

          crm_destructive_alter_nvariable ( from_var, vlen, outbuf, outlen);
      }

      //  Record useful minion data, if possible.
      if (strlen (keep_buf) > 0)
        {
          sprintf (exp_keep_buf,
                   "MINION PROC PID: %d from-pipe: %d to-pipe: %d",
                   minion,
                   from_minion[0],
                   to_minion[1]);
          if (internal_trace)
            fprintf (stderr, "   saving minion state: %s \n", exp_keep_buf);
          crm_destructive_alter_nvariable (keep_buf, keep_len,
                                           exp_keep_buf,
                                           strlen (exp_keep_buf));
        };

      //      If we're keeping this minion process around, record the useful
      //      information, like pid, in and out pipes, etc.
      if (!keep_proc && !async_mode)
        {
          DWORD exit_code;
          if (internal_trace)
            fprintf (stderr, "No keep, no async, so not keeping minion, closing everything.\n");

          //   no, we're not keeping it around, so close the pipe.
          //
          CloseHandle(from_minion [0]);

          WaitForSingleObject(hminion, INFINITE);
          if (!GetExitCodeProcess(hminion, &exit_code))
            {
              DWORD error = GetLastError();
            }
          if ( crm_vht_lookup (vht, keep_buf, strlen (keep_buf)))
            {
              char exit_value_string[MAX_VARNAME];
              if (internal_trace)
                fprintf (stderr, "minion exit code :%d; whacking %s\n",
                         exit_code,
                         keep_buf);
              sprintf (exit_value_string, "DEAD MINION, EXIT CODE: %d",
                       exit_code);
              if (keep_len > 0)
                crm_destructive_alter_nvariable (keep_buf, keep_len,
                                                 exit_value_string,
                                                 strlen (exit_value_string));
        };
      CloseHandle(hminion);
    };
#endif	// CRM_WINDOWS

  return (0);
};
