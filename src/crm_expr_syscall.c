//  crm_expr_syscall.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2006  William S. Yerazunis, all rights reserved.
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



#ifdef WIN32
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







unsigned int WINAPI pusher_proc(void *lpParameter)
{
  DWORD bytesWritten;
  pusherparams *p = (pusherparams *)lpParameter;
  if (!WriteFile(p->to_minion, p->inbuf, p->inlen, &bytesWritten, NULL))
  {
          fprintf(stderr, "The pusher failed to send %ld input bytes to the minion.\n", (long)p->inlen);
  }
  free(p->inbuf);
  p->inbuf = NULL;
  if (p->internal_trace)
    fprintf (stderr, "pusher: input sent to minion.\n");

  //    if we don't want to keep this proc, we close it's input, and
  //    wait for it to exit.
  if (! p->keep_proc)
    {
      if (!CloseHandle (p->to_minion))
                  {
                          fprintf(stderr, "The pusher failed to close the minion input pipe.\n");
                  }

      if (internal_trace)
        fprintf (stderr, "minion input pipe closed\n");
    }
  if (p->internal_trace)
    fprintf (stderr, "pusher: exiting pusher\n");
  return 0;
}

unsigned int WINAPI sucker_proc(void *lpParameter)
{
  DWORD bytesRead;
  suckerparams *p = (suckerparams *)lpParameter;

#define OUTBUF_SIZE                     8192

  char obuf[OUTBUF_SIZE];

  /*  we're in the sucker process here- just throw away
      everything till we get EOF, then exit. */
  while (1)
    {
      Sleep (p->timeout);
      if (!ReadFile(p->from_minion, obuf,
               OUTBUF_SIZE, &bytesRead, NULL))
          {
                  fprintf(stderr, "The sucker failed to read any data from the minion.\n");
          }
      if (bytesRead == 0) break;
    }
  return 0;
#undef OUTBUF_SIZE
}
#endif

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
  long done;
  int keep_proc;
  int async_mode;
#if defined(HAVE_WORKING_FORK) && defined(HAVE_FORK) && defined(HAVE_PIPE)
  long charsread;
  int to_minion[2];
  int from_minion[2];
  int minion_exit_status;
  pid_t pusher;
  pid_t sucker;
  pid_t random_child;
#elif defined(WIN32)
  DWORD charsread;
  HANDLE to_minion[2];
  HANDLE from_minion[2];
  char sys_cmd_2nd[MAX_PATTERN];
#endif
  pid_t minion;
  int status;
  long timeout;

#ifdef WIN32
  SECURITY_ATTRIBUTES pipeSecAttr;
  HANDLE hminion;
#endif

#if defined(HAVE_WAITPID)
  if (user_trace)
    fprintf (stderr, "executing a SYSCALL statement");

  timeout = MINION_SLEEP_USEC;


  //  clean up any prior processes - note that
  //  we don't keep track of these.  For that matter, we have
  //  no context to keep track of 'em.
  //
  while ( (random_child = waitpid ( 0, &status, WNOHANG)) > 0 );
#elif defined(WIN32)
  timeout = MINION_SLEEP_USEC / 1000;   // need milliseconds for Sleep()
  if (MINION_SLEEP_USEC > 0 && timeout == 0)
    {
      timeout = 1;
    }
#endif

  //    get the flags
  //
  CRM_ASSERT(apb != NULL);
  keep_proc = 0;
  if (apb->sflags & CRM_KEEP)
    {
      if (user_trace)
        fprintf (stderr, "Keeping the process around if possible\n");
      keep_proc = 1;
    }
  async_mode = 0;
  if (apb->sflags & CRM_ASYNC)
    {
      if (user_trace)
        fprintf (stderr, "Letting the process go off on it's own");
      async_mode = 1;
    }

  //     Sanity check - <async> is incompatible with <keep>
  //
  if (keep_proc && async_mode)
    {
      nonfatalerror ("This syscall uses both async and keep, but async is "
                     "incompatible with keep.  Since keep is safer"
                     "we will use that.\n",
                     "You need to fix this program.");
      async_mode = 0;
    }

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

  //      get the command to execute
  //
  crm_get_pgm_arg (sys_cmd, MAX_PATTERN, apb->s1start, apb->s1len);
  cmd_len = crm_nexpandvar (sys_cmd, apb->s1len, MAX_PATTERN);
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

#if defined(HAVE_DUP2) && defined(HAVE_WORKING_FORK) && defined(HAVE_FORK) && defined(HAVE_PIPE) && defined(HAVE_WAITPID) && defined(HAVE_SYSTEM)
  if (3 != sscanf (exp_keep_buf, "MINION PROC PID: %d from-pipe: %d to-pipe: %d",
          &minion,
          &from_minion[0],
          &to_minion[1]))
                  {
//                        nonfatalerror("Failed to decode the minion setup: ", exp_keep_buf);
                  }

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
          nonfatalerror ("Problem setting up the to/from pipes to a minion. ",
                         "Perhaps the system file descriptor table is full?");
          return (1);
        }
      minion = fork();

      if (minion < 0)
        {
          nonfatalerror ("Tried to fork your minion, but it failed.",
                      "Your system may have run out of process slots");
          return (1);
        }

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
              //              sys_cmd[vstart+vlen] = '\0';
              if (user_trace)
                fprintf (stderr, "FORK transferring control to line %s\n",
                         &sys_cmd[vstart]);

              //    set the current pid and parent pid.
              {
                char pidstr [32];
                long pid;
#if defined(HAVE_GETPID)
                                pid = (long) getpid();
                sprintf (pidstr, "%ld", pid);
                crm_set_temp_var (":_pid:", pidstr);
                                if (user_trace)
                  fprintf (stderr, "My new PID is %s\n", pidstr);
#endif
#if defined(HAVE_GETPPID)
                pid = (long) getppid();
                sprintf (pidstr, "%ld", pid);
                crm_set_temp_var (":_ppid:", pidstr);
#endif
                          }
              //   See if we have redirection of stdin and stdout
              while (crm_nextword (sys_cmd, strlen (sys_cmd), vstart+vlen,
                                   &vstart, &vlen))
                {
                  char filename[MAX_PATTERN];
                  if (sys_cmd[vstart] == '<')
                    {
                                /* [i_a] make sure no buffer overflow is going to happen here */
                                if (vlen-1 >= MAX_PATTERN)
                                        vlen = MAX_PATTERN - 1+1;
                      strncpy (filename, &sys_cmd[vstart+1], vlen-1);
                          CRM_ASSERT(vlen-1 < MAX_PATTERN);
                      filename[vlen-1] = '\0';
                      if (user_trace)
                        fprintf (stderr, "Redirecting minion stdin to %s\n",
                                 filename);
                      freopen (filename, "rb", stdin);
                    }
                  if (sys_cmd[vstart] == '>')
                    {
                      if (sys_cmd[vstart+1] != '>')
                        {
                                /* [i_a] make sure no buffer overflow is going to happen here */
                                if (vlen-1 >= MAX_PATTERN)
                                        vlen = MAX_PATTERN - 1+1;
                          strncpy (filename, &sys_cmd[vstart+1], vlen-1);
                          CRM_ASSERT(vlen-1 < MAX_PATTERN);
                          filename[vlen-1] = '\0';
                          if (user_trace)
                            fprintf (stderr,
                                     "Redirecting minion stdout to %s\n",
                                     filename);
                          freopen (filename, "wb", stdout);
                        }
                      else
                        {
                                /* [i_a] make sure no buffer overflow is going to happen here */
                                if (vlen-2 >= MAX_PATTERN)
                                        vlen = MAX_PATTERN - 1+2;
                          strncpy (filename, &sys_cmd[vstart+2], vlen-2);
                          CRM_ASSERT(vlen-2 < MAX_PATTERN);
                          filename[vlen-2] = '\0';
                          if (user_trace)
                            fprintf (stderr,
                                     "Appending minion stdout to %s\n",
                                     filename);
                          freopen (filename, "ab+", stdout);
                        }
                    }
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
                  nonfatalerror_ex(SRC_LOC(),
                           "This program tried a shell command that "
                           "didn't run correctly.\n"
                           "The command was >%s< and returned exit code %d.\n"
                           "errno = %d(%s)",
                           sys_cmd,
                           WEXITSTATUS(retcode),
                           errno,
                           errno_descr(errno));
                  if (engine_exit_base != 0)
                    {
                      exit (engine_exit_base + 11);
                    }
                  else
                    exit (WEXITSTATUS (retcode ));
                }
              exit ( WEXITSTATUS (retcode) );
            }
        }      //    END OF IN THE MINION
    }
  else
    {
      if (user_trace)
        fprintf (stderr, "  reusing old minion PID: %d\n", minion);
    }
  //      Now, we're out of the minion for sure.
  //    so we close the pipe ends we know we won't be using.
  if (to_minion[0] != 0)
    {
      close (to_minion[0]);
      close (from_minion[1]);
    }
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
          write (to_minion[1], inbuf, inlen );
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
        }
    }
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
        }
      if (keep_proc == 1 || async_mode == 1)
        {
          //   we're in either 'keep' 'async' mode.  Set nonblocking mode, then
          //   read it once; then put it back in regular mode.
          //fcntl (from_minion[0], F_SETFL, O_NONBLOCK);
          //      usleep (timeout);
          charsread = read (from_minion[0],
                            &outbuf[done],
                            (data_window_size >> SYSCALL_WINDOW_RATIO));
          done = charsread;
          if (done < 0) done = 0;
          outbuf [done] = '\000';
          outlen = done ;
          //fcntl (from_minion[0], F_SETFL, 0);
        }

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
                }
            }
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
        fprintf (stderr, "   saving minion state: %s \n",
                 exp_keep_buf);
      crm_destructive_alter_nvariable (keep_buf, keep_len,
                                       exp_keep_buf,
                                       strlen (exp_keep_buf));
    }
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
        }
    }
#elif defined(WIN32)
  if (01)
  {
  fatalerror (" Sorry, syscall is completely b0rked in this version", "");
  return 0;
  }
  else
  {
  if (3 != sscanf (exp_keep_buf, "MINION PROC PID: %ld from-pipe: %p to-pipe: %p",
          &minion,
          &from_minion[0],
          &to_minion[1]))
                  {
//                        nonfatalerror("Failed to decode the minion setup: ", exp_keep_buf);
                  }

  //      if, no minion already existing, we create
  //      communications pipes and launch the subprocess.  This
  //      code borrows concepts from both liblaunch and from
  //      netcat (thanks, *Hobbit*!)
  //
  if (minion == 0)
    {
      int retcode;
      long vstart2, vlen2;
      long varline;

      if (user_trace)
        fprintf (stderr, "  Must start a new minion.\n");

      pipeSecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
      pipeSecAttr.bInheritHandle = TRUE;
      pipeSecAttr.lpSecurityDescriptor = NULL;

      status = CreatePipe(&to_minion[0], &to_minion[1], &pipeSecAttr, 2^10 * 32);
          if (!status)
          {
                  fatalerror_Win32("Failed to create minion pipe #1");
          }
      status = CreatePipe(&from_minion[0], &from_minion[1], &pipeSecAttr, 2^10 * 32);
          if (!status)
          {
                  fatalerror_Win32("Failed to create minion pipe #2");
          }

      crm_nextword (sys_cmd, strlen (sys_cmd), 0, &vstart2, &vlen2);
      varline = crm_lookupvarline (vht, sys_cmd, vstart2, vlen2);
      if (varline > 0)
        {
            fatalerror (" Sorry, syscall to a label isn't implemented in this version", "");
        }
      else
        {
          STARTUPINFO si;
                  PROCESS_INFORMATION pi = {0};
          HANDLE stdout_save, stdin_save;
          HANDLE to_minion_write, from_minion_read;
                  DWORD error = 0;

          stdout_save = GetStdHandle(STD_OUTPUT_HANDLE);
          if (!SetStdHandle(STD_OUTPUT_HANDLE, from_minion[1]))
                  {
                          fatalerror_Win32("Failed to redirect stdout for minion");
                  }

          stdin_save = GetStdHandle(STD_INPUT_HANDLE);
          if (!SetStdHandle(STD_INPUT_HANDLE, to_minion[0]))
                  {
                          fatalerror_Win32("Failed to redirect stdin for minion");
                  }

          if (!DuplicateHandle(GetCurrentProcess(), from_minion[0], GetCurrentProcess(), &from_minion_read , 0, FALSE, DUPLICATE_SAME_ACCESS))
                  {
                          fatalerror_Win32("Failed to dup the read handle for minion");
                  }
          if (!CloseHandle(from_minion[0]))
                  {
                          fatalerror_Win32("Failed to close the read handle after dup for minion");
                  }
          from_minion[0] = from_minion_read;

          if (!DuplicateHandle(GetCurrentProcess(), to_minion[1], GetCurrentProcess(), &to_minion_write , 0, FALSE, DUPLICATE_SAME_ACCESS))
                  {
                          fatalerror_Win32("Failed to dup the write handle for minion");
                  }
          if (!CloseHandle(to_minion[1]))
                  {
                          fatalerror_Win32("Failed to close the write handle after dup for minion");
                  }
          to_minion[1] = to_minion_write;

          if (user_trace)
            fprintf (stderr, "systemcalling on shell command %s\n",
                     sys_cmd);

          ZeroMemory( &si, sizeof(si) );
          si.cb = sizeof(si);

          ZeroMemory( &pi, sizeof(pi) );

                  /* MSVC spec says 'sys_cmd' will be edited inside CreateProcess. Keep the original around for
                     the second attempt: */
                  strcpy(sys_cmd_2nd, sys_cmd);

          retcode = CreateProcess(NULL, sys_cmd, NULL, NULL, TRUE , 0, NULL, NULL, &si, &pi);

          if (!retcode)
          {
              error = GetLastError();

                          if (error == ERROR_FILE_NOT_FOUND)
                          {
                                  /* this might be an 'internal' command: try a second time, now after prefixing it with 'cmd /c': */
                                  strcpy(sys_cmd, "cmd /c ");
                                  strncat(sys_cmd, sys_cmd_2nd, WIDTHOF(sys_cmd) - WIDTHOF("cmd /c "));

                              retcode = CreateProcess(NULL, sys_cmd, NULL, NULL, TRUE , 0, NULL, NULL, &si, &pi);

                                  error = GetLastError();
                          }
                  }

                          if (!retcode)
                                {
                                  char *errmsg = Win32_syserr_descr(error);

                                  // use the undamaged sys_cmd entry for error reporting...
                                  fatalerror_ex(SRC_LOC(), "This program tried a shell command that "
                                                          "didn't run correctly.\n"
                                                                                                                  "command >>%s<< - CreateProcess returned %ld($%lx:%s)\n",
                                                   sys_cmd_2nd,
                                                                                                   (long)error,
                                                                                                   (long)error,
                                                                                                   errmsg);

                           if (engine_exit_base != 0)
                          {
                                exit(engine_exit_base + 13);
                          }
                        else
                        {
                          exit(EXIT_FAILURE);
                           }
            }
          else
            {
              minion = pi.dwProcessId;
              hminion = pi.hProcess;
              if (!SetStdHandle(STD_OUTPUT_HANDLE, stdout_save))
                          {
                                  fatalerror_Win32("Failed to reset stdout for minion");
                          }
              if (!SetStdHandle(STD_INPUT_HANDLE, stdin_save))
                          {
                                  fatalerror_Win32("Failed to reset stdin for minion");
                          }
              if (!CloseHandle(pi.hThread))
                          {
                                  fatalerror_Win32("Failed to close the execution thread handle for minion");
                          }
            }
        }
    }
  else
    {
      if (user_trace)
        fprintf (stderr, "  reusing old minion PID: %d\n", minion);
      hminion = OpenProcess(PROCESS_ALL_ACCESS, 0, minion);
      if (hminion == NULL)
          {
        fatalerror_Win32("Couldn't open the existing minion process");
          }
    }
  //      Now, we're out of the minion for sure.
  //    so we close the pipe ends we know we won't be using.
  if (to_minion[0] != 0)
    {
      if (!CloseHandle (to_minion[0]))
          {
                  fatalerror_Win32("Failed to close read pipe for minion");
          }
      if (!CloseHandle (from_minion[1]))
          {
                  fatalerror_Win32("Failed to close write pipe for minion");
          }
    }
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
      unsigned int hThread;
          pusherparams pp = {0};
      char *inbuf_copy = calloc((inlen+1), sizeof(inbuf_copy[0]) );
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
          if (!_beginthreadex(NULL, 0, pusher_proc, &pp, 0, &hThread))
          {
                  fatalerror_ex(SRC_LOC(),
                        "Failed to start the pusher thread: "
                        "error code %d (%s)",
                        errno,
                        errno_descr(errno));
          }
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

          if (!ReadFile(from_minion[0],
                   outbuf + done,
                   (data_window_size >> SYSCALL_WINDOW_RATIO) - done - 2,
                   &charsread, NULL))
                  {
                          fatalerror_Win32("Failed to sync-read any data from the minion");
                  }

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
          if (!ReadFile(from_minion[0],
                   &outbuf[done],
                   (data_window_size >> SYSCALL_WINDOW_RATIO), &charsread, NULL))
                  {
                          fatalerror_Win32("Failed to async read any data from the minion");
                  }

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
            unsigned int hThread;
                        suckerparams sp = {0};
            sp.from_minion = from_minion[0];
            sp.timeout = timeout;
            if (!_beginthreadex(NULL, 0, sucker_proc, &sp, 0, &hThread))
                  {
                  fatalerror_ex(SRC_LOC(),
                        "Failed to start the sucker thread: "
                        "error code %d (%s)",
                        errno,
                        errno_descr(errno));
                  }
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
                   "MINION PROC PID: %ld from-pipe: %p to-pipe: %p",
                   minion,
                   from_minion[0],
                   to_minion[1]);
          if (internal_trace)
            fprintf (stderr, "   saving minion state: %s\n", exp_keep_buf);
          crm_destructive_alter_nvariable (keep_buf, keep_len,
                                           exp_keep_buf,
                                           strlen (exp_keep_buf));
        }

      //      If we're keeping this minion process around, record the useful
      //      information, like pid, in and out pipes, etc.
      if (!keep_proc && !async_mode)
        {
          DWORD exit_code;
          if (internal_trace)
            fprintf (stderr, "No keep, no async, so not keeping minion, closing everything.\n");

          //   no, we're not keeping it around, so close the pipe.
          //
          if (!CloseHandle(from_minion [0]))
                  {
                          fatalerror_Win32("Failed to close the read pipe for non-async minion");
                  }

          if (WAIT_FAILED == WaitForSingleObject(hminion, INFINITE))
                  {
                          fatalerror_Win32("Failed while waiting for the minion to terminate");
                  }

          if (!GetExitCodeProcess(hminion, &exit_code))
                  {
                          fatalerror_Win32("Failed to grab the exit code from the system call");
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
        }
      if (!CloseHandle(hminion))
          {
                  fatalerror_Win32("Failed to close the system call minion handle");
          }
    }
          }
#else
  fatalerror(" Sorry, syscall is not supported in this version", "");
#endif
  return (0);
}
