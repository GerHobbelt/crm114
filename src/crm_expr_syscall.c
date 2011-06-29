//  crm_expr_syscall.c  - Controllable Regex Mutilator,  version v1.0
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
    void  *my_ptr;
    HANDLE to_minion;
    char  *inbuf;
    long   inlen;
    long   internal_trace;
    long   keep_proc;
} pusherparams;

typedef struct
{
    void  *my_ptr;
    HANDLE from_minion;
    int    timeout;
    long   internal_trace;
    long   keep_proc;
} suckerparams;







unsigned int WINAPI pusher_proc(void *lpParameter)
{
    DWORD bytesWritten;
    pusherparams *p = (pusherparams *)lpParameter;

    if (!WriteFile(p->to_minion, p->inbuf, p->inlen, &bytesWritten, NULL))
    {
		char buf[512];
		char *s = buf;

		Win32_syserr_descr(&s, 512, GetLastError(), NULL);

        fprintf(stderr,
            "The pusher failed to send %ld input bytes to the minion: %d(%s)\n",
            (long)p->inlen, GetLastError(), s);
    }
    free(p->inbuf);
    p->inbuf = NULL;
    if (p->internal_trace)
        fprintf(stderr, "pusher: input sent to minion.\n");

    //    if we don't want to keep this proc, we close it's input, and
    //    wait for it to exit.
    if (!p->keep_proc)
    {
        if (!CloseHandle(p->to_minion))
        {
            fprintf(stderr, "The pusher failed to close the minion input pipe.\n");
        }
        p->to_minion = 0;

        if (internal_trace)
            fprintf(stderr, "minion input pipe closed\n");
    }
    if (p->internal_trace)
        fprintf(stderr, "pusher: exiting pusher\n");

    // free the fire&forget memory block: */
    free(p->my_ptr);
    p = NULL;

    _endthreadex(0);
    return 0;
}



unsigned int WINAPI sucker_proc(void *lpParameter)
{
    DWORD bytesRead;
    suckerparams *p = (suckerparams *)lpParameter;

#define OUTBUF_SIZE                     8192

    char obuf[OUTBUF_SIZE];

    int eof = FALSE;
    int error;
    int status;

    DWORD exit_code;
    LARGE_INTEGER large_int;


    //  we're in the sucker process here- just throw away
    //  everything till we get EOF, then exit.
    for ( ; ;)
    {
        exit_code = (DWORD)-1;
        status = GetExitCodeProcess(p->from_minion, &exit_code);
        error = GetLastError();
        if (internal_trace)
            fprintf(stderr, "GetExitCodeProcess() = %d/%d, %ld\n", status, error, exit_code);

        memset(&large_int, 0, sizeof(large_int));
        status = GetFileSizeEx(p->from_minion, &large_int);
        error = GetLastError();
        if (internal_trace)
            fprintf(stderr, "GetFileSizeEx() = %d/%d, %ld:%ld\n", status, error, large_int.HighPart, large_int.LowPart);

        if ( /* status && error == ERROR_SUCCESS && */ !large_int.HighPart && !large_int.LowPart)
        {
            if (exit_code != STILL_ACTIVE)
            {
                // the syscall has terminated, there will not arrive anything anymore to fetch lateron...
                eof = TRUE;
            }
        }

        bytesRead = 0;
        if (!eof)
        {
            // ReadFile() will lock indefinitely when all output has been fetched
            // and the syscall has terminated, i.e. the exit code is known then.
            // Hence the !eof flag check here: it signals there won't be coming
            // any more.

            /*
             * From the Microsoft docs:
             *
             * If hFile is not opened with FILE_FLAG_OVERLAPPED and lpOverlapped is NULL,
             * the read operation starts at the current file position and ReadFile does
             * not return until the operation is complete, and then the system updates
             * the file pointer.
             *
             * If hFile is not opened with FILE_FLAG_OVERLAPPED and lpOverlapped is not
             * NULL, the read operation starts at the offset that is specified in the
             * OVERLAPPED structure. ReadFile does not return until the read operation
             * is complete, and then the system updates the file pointer.
             */

            if (large_int.HighPart || large_int.LowPart)
            {
                if (!ReadFile(p->from_minion, obuf,
                        OUTBUF_SIZE, &bytesRead, NULL))
                {
                    error = GetLastError();
                    switch (error)
                    {
                    case ERROR_HANDLE_EOF:
                        // At the end of the file.
                        eof = TRUE;
                        break;

                    case ERROR_IO_PENDING:
                        // I/O pending.
                        break;

                    default:
                        fprintf(stderr, "The sucker failed to read any data from the minion.\n");
                        eof = TRUE;  // make sure we exit this loop on error!
                        break;
                    }
                }
                error = GetLastError();

                switch (error)
                {
                case ERROR_HANDLE_EOF:
                    // At the end of the file
                    eof = TRUE;
                    break;
                }

#if 0
                if (bytesRead < OUTBUF_SIZE)
                {
                    eof = TRUE; // <-- this cinches it: since ReadFile() will not return until completed when in sync read mode.
                }
#endif
            }
        }

        if (eof)
            break;

        status = WaitForInputIdle(p->from_minion, p->timeout);
    }

    if (p->internal_trace)
        fprintf(stderr, "sucker: output fetched from minion.\n");

    //    if we don't want to keep this proc, we close it's output, and
    //    wait for it to exit.
    if (!p->keep_proc)
    {
        if (!CloseHandle(p->from_minion))
        {
            fprintf(stderr, "The sucker failed to close the minion output pipe.\n");
        }
        p->from_minion = 0;

        if (internal_trace)
            fprintf(stderr, "minion output pipe closed\n");
    }
    if (p->internal_trace)
        fprintf(stderr, "sucker: exiting sucker\n");

    // free the fire&forget memory block: */
    free(p->my_ptr);
    p = NULL;

    _endthreadex(0);
    return 0;

#undef OUTBUF_SIZE
}
#endif

int crm_expr_syscall(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //           Go off and fork a process, sending that process
    //           one pattern evaluated as input, and then accepting
    //           all the returns from that process as the new value
    //           for a variable.
    //
    //           syntax is:
    //               syscall (:to:) (:from:) (:ctl:) /commandline/ /[alt-OS]alt-commandline/ ...
    long inlen;
    long outlen;
    char from_var[MAX_VARNAME];
    char sys_cmd[MAX_PATTERN];
    int cmd_len;
    char keep_buf[MAX_PATTERN];
    int keep_len;
    char exp_keep_buf[MAX_PATTERN];
    int exp_keep_len;
    int vstart;
    int vlen;
    int done;
    int keep_proc;
    int async_mode;

#if defined (HAVE_WORKING_FORK) && defined (HAVE_FORK) && defined (HAVE_PIPE)
    long charsread;
    int to_minion[2];
    int from_minion[2];
    int minion_exit_status;
    pid_t pusher;
    pid_t sucker;
    pid_t random_child;
#elif defined (WIN32)
    DWORD charsread;
    HANDLE to_minion[2];
    HANDLE from_minion[2];
    char sys_cmd_2nd[MAX_PATTERN];
#endif
    pid_t minion;
    int status;
    long timeout;
    int cnt;

#if defined (HAVE_WAITPID)
    if (user_trace)
        fprintf(stderr, "executing a SYSCALL statement");

    timeout = MINION_SLEEP_USEC;


    //  clean up any prior processes - note that
    //  we don't keep track of these.  For that matter, we have
    //  no context to keep track of 'em.
    //
    while ((random_child = waitpid(0, &status, WNOHANG)) > 0)
        ;
#elif defined (WIN32)
    timeout = MINION_SLEEP_USEC / 1000; // need milliseconds for Sleep()
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
            fprintf(stderr, "Keeping the process around if possible\n");
        keep_proc = 1;
    }
    async_mode = 0;
    if (apb->sflags & CRM_ASYNC)
    {
        if (user_trace)
            fprintf(stderr, "Letting the process go off on it's own");
        async_mode = 1;
    }

    //     Sanity check - <async> is incompatible with <keep>
    //
    if (keep_proc && async_mode)
    {
        nonfatalerror("This syscall uses both async and keep, but async is "
                      "incompatible with keep.  Since keep is safer"
                      "we will use that.\n",
            "You need to fix this program.");
        async_mode = 0;
    }

    //    get the input variable(s)
    //
    crm_get_pgm_arg(inbuf, data_window_size, apb->p1start, apb->p1len);
    inlen = crm_nexpandvar(inbuf, apb->p1len, data_window_size);
    if (user_trace)
        fprintf(stderr, "  command's input wil be: ***%s***\n", inbuf);

    //    now get the name of the variable where the return will be
    //    placed... this is a crock and should be fixed someday.
    //    the output goes only into a single var (the first one)
    //    so we extract that
    //
    crm_get_pgm_arg(from_var, MAX_PATTERN, apb->p2start, apb->p2len);
    outlen = crm_nexpandvar(from_var, apb->p2len, MAX_PATTERN);
    done = 0;
#if 0
    vstart = 0;
    while (from_var[vstart] < 0x021 && from_var[vstart] > 0x0)
        vstart++;
    vlen = 0;
    while (from_var[vstart + vlen] >= 0x021)
        vlen++;
#else
    crm_nextword(from_var, outlen, 0, &vstart, &vlen);
#endif
    memmove(from_var, &from_var[vstart], vlen);
    from_var[vlen] = 0;
    if (user_trace)
        fprintf(stderr, "   command output will overwrite var ***%s***\n",
            from_var);
    // [i_a] make sure we 'zero' the output variable, so it'll be empty when an error occurs.
    // This way, old content doesn't stay around when errors are not checked in a CRM script.
    if (*from_var)
    {
        crm_destructive_alter_nvariable(from_var, vlen, "", 0);
    }



    //    now get the name of the variable (if it exists) where
    //    the kept-around minion process's pipes and pid are stored.
    crm_get_pgm_arg(keep_buf, MAX_PATTERN, apb->p3start, apb->p3len);
    keep_len = crm_nexpandvar(keep_buf, apb->p3len, MAX_PATTERN);
    if (user_trace)
        fprintf(stderr, "   command status kept in var ***%s***\n",
            keep_buf);

    //      get the command to execute
    //
    crm_get_pgm_arg(sys_cmd, MAX_PATTERN, apb->s1start, apb->s1len);
    cmd_len = crm_nexpandvar(sys_cmd, apb->s1len, MAX_PATTERN);
    if (user_trace)
        fprintf(stderr, "   command will be ***%s***\n", sys_cmd);

    //      get possible alternative command to execute: if the given OS matches ours, we exec this one instead!
    //
    if (apb->s2start != NULL)
    {
        char alt_sys_cmd[MAX_PATTERN];
        long alt_cmd_len;
        char *p;
        char *os;

        crm_get_pgm_arg(alt_sys_cmd, MAX_PATTERN, apb->s2start, apb->s2len);
        alt_cmd_len = crm_nexpandvar(alt_sys_cmd, apb->s2len, MAX_PATTERN);
        p = strchr(alt_sys_cmd, ']');
        os = NULL;
        if (p != NULL)
        {
            os = strnchr(alt_sys_cmd, '[', p - alt_sys_cmd);
        }
        if (p == NULL || os == NULL)
        {
            nonfatalerror(
                "Failed to decode the alternative syscall commandline: The OS has probably NOT been defined.\n"
                "The alt command line looks like this: ",
                alt_sys_cmd);
        }
        else
        {
            if (user_trace)
            {
                fprintf(stderr,
                    "   alternative command will be (for OS: %.*s) '%s', while our OS is [%s]\n",
                    (int)(p + 1 - os),
                    os,
                    p + 1,
                    HOSTTYPE);
            }
            if (strncasecmp(os + 1, HOSTTYPE, strlen(HOSTTYPE)) == 0)
            {
                // matching OS! --> override the syscall commandline!
                strcpy(sys_cmd, p + 1);
                cmd_len = strlen(sys_cmd);
            }
        }
    }

    //     Do we reuse an already-existing process?  Check to see if the
    //     keeper variable has it... note that we have to :* prefix it
    //     and expand it again.
    minion = 0;
    to_minion[0] = 0;
    to_minion[1] = 0;
    from_minion[0] = 0;
    from_minion[1] = 0;
    //exp_keep_buf[0] = 0;
    //  this is 8-bit-safe because vars are never wchars.
    if (keep_buf[0])
    {
        strcpy(exp_keep_buf, ":*");
        strncat(exp_keep_buf, keep_buf, keep_len);
        exp_keep_len = crm_nexpandvar(exp_keep_buf, keep_len + 2, MAX_PATTERN);
    }
    else
    {
        exp_keep_buf[0] = 0;
        exp_keep_len = 0;
    }

#if defined (HAVE_DUP2) && defined (HAVE_WORKING_FORK) && defined (HAVE_FORK)    \
    && defined (HAVE_PIPE) && defined (HAVE_WAITPID) && defined (HAVE_SYSTEM)
    cnt = 0;
    if (*exp_keep_buf)
    {
        cnt = sscanf(exp_keep_buf, "MINION PROC PID: %d from-pipe: %d to-pipe: %d",
            &minion,
            &from_minion[0],
            &to_minion[1]);
    }
        if (!(cnt == 3
              || (cnt == 0 && (*exp_keep_buf == 0 || strncmp(exp_keep_buf, "DEAD MINION", WIDTHOF("DEAD MINION") - 1) == 0))))
    {
        nonfatalerror("Failed to decode the syscall minion setup: ", exp_keep_buf);
        return 1;
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
            fprintf(stderr, "  Must start a new minion.\n");
        status1 = pipe(to_minion);
        status2 = pipe(from_minion);
        if (status1 > 0 || status2 > 0)
        {
            nonfatalerror("Problem setting up the to/from pipes to a minion. ",
                "Perhaps the system file descriptor table is full?");
            return 1;
        }
        minion = fork();

        if (minion < 0)
        {
            nonfatalerror("Tried to fork your minion, but it failed.",
                "Your system may have run out of process slots");
            return 1;
        }

        if (minion == 0)
        {
            //  START OF IN THE MINION
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
            close(to_minion[1]);
            close(from_minion[0]);
            dup2(to_minion[0], fileno(stdin));
            dup2(from_minion[1], fileno(stdout));

            //     Are we a syscall to a :label:, or should we invoke the
            //     shell on an external command?
            //
            crm_nextword(sys_cmd, strlen(sys_cmd), 0, &vstart, &vlen);
            varline = crm_lookupvarline(vht, sys_cmd, vstart, vlen);
            if (varline > 0)
            {
                //              sys_cmd[vstart+vlen] = 0;
                if (user_trace)
                    fprintf(stderr, "FORK transferring control to line %s\n",
                        &sys_cmd[vstart]);

                //    set the current pid and parent pid.
                {
                    char pidstr[32];
                    long pid;
#if defined (HAVE_GETPID)
                    pid = (long)getpid();
                    sprintf(pidstr, "%ld", pid);
                    crm_set_temp_var(":_pid:", pidstr);
                    if (user_trace)
                        fprintf(stderr, "My new PID is %s\n", pidstr);
#endif
#if defined (HAVE_GETPPID)
                    pid = (long)getppid();
                    sprintf(pidstr, "%ld", pid);
                    crm_set_temp_var(":_ppid:", pidstr);
#endif
                }
                //   See if we have redirection of stdin and stdout
                while (crm_nextword(sys_cmd, strlen(sys_cmd), vstart + vlen,
                           &vstart, &vlen))
                {
                    char filename[MAX_PATTERN];
                    if (sys_cmd[vstart] == '<')
                    {
                        /* [i_a] make sure no buffer overflow is going to happen here */
                        if (vlen - 1 >= MAX_PATTERN)
                            vlen = MAX_PATTERN - 1 + 1;
                        strncpy(filename, &sys_cmd[vstart + 1], vlen - 1);
                        CRM_ASSERT(vlen - 1 < MAX_PATTERN);
                        filename[vlen - 1] = 0;
                        if (user_trace)
                            fprintf(stderr, "Redirecting minion stdin to %s\n",
                                filename);
                        freopen(filename, "rb", stdin);
                    }
                    if (sys_cmd[vstart] == '>')
                    {
                        if (sys_cmd[vstart + 1] != '>')
                        {
                            /* [i_a] make sure no buffer overflow is going to happen here */
                            if (vlen - 1 >= MAX_PATTERN)
                                vlen = MAX_PATTERN - 1 + 1;
                            strncpy(filename, &sys_cmd[vstart + 1], vlen - 1);
                            CRM_ASSERT(vlen - 1 < MAX_PATTERN);
                            filename[vlen - 1] = 0;
                            if (user_trace)
                                fprintf(stderr,
                                    "Redirecting minion stdout to %s\n",
                                    filename);
                            freopen(filename, "wb", stdout);
                        }
                        else
                        {
                            /* [i_a] make sure no buffer overflow is going to happen here */
                            if (vlen - 2 >= MAX_PATTERN)
                                vlen = MAX_PATTERN - 1 + 2;
                            strncpy(filename, &sys_cmd[vstart + 2], vlen - 2);
                            CRM_ASSERT(vlen - 2 < MAX_PATTERN);
                            filename[vlen - 2] = 0;
                            if (user_trace)
                                fprintf(stderr,
                                    "Appending minion stdout to %s\n",
                                    filename);
                            freopen(filename, "ab+", stdout);
                        }
                    }
                }
                csl->cstmt = varline;
                //   and note that this isn't a failure.
                csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = 1;
                //   The minion's real work should now start; get out of
                //   the syscall code and go run something real.  :)
                return 0;
            }
            else
            {
                if (user_trace)
                    fprintf(stderr, "Systemcalling on shell command %s\n",
                        sys_cmd);
                retcode = system(sys_cmd);
                //
                //       This code only ever happens if an error occurs...
                //
                if (retcode == -1)
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
                        exit(engine_exit_base + 11);
                    }
                    else
                    {
                        exit(WEXITSTATUS(retcode));
                    }
                }
                exit(WEXITSTATUS(retcode));
            }
        }      //    END OF IN THE MINION
    }
    else
    {
        if (user_trace)
            fprintf(stderr, "  reusing old minion PID: %d\n", minion);
    }
    //      Now, we're out of the minion for sure.
    //    so we close the pipe ends we know we won't be using.
    if (to_minion[0] != 0)
    {
        close(to_minion[0]);
        close(from_minion[1]);
    }
    //
    //   launch "pusher" process to send the buffer to the minion
    //    (this hint from Dave Soderberg).  This avoids the deadly
    //   embrace situation where both processes are waiting to read
    //   (or, equally, both processes have written and filled up
    //   their buffers, and are now held up waiting for the other
    //   process to empty some space in the output buffer)
    //
    if (strlen(inbuf) > 0)
    {
        pusher = fork();
        //    we're in the "input pusher" process if we got here.
        //    shove the input buffer out to the minion
        if (pusher == 0)
        {
            write(to_minion[1], inbuf, inlen);
            if (internal_trace)
                fprintf(stderr, "pusher: input sent to minion.\n");
            close(to_minion[1]);
            if (internal_trace)
                fprintf(stderr, "pusher: minion input pipe closed\n");
            if (internal_trace)
                fprintf(stderr, "pusher: exiting pusher\n");
            //  The pusher always exits with success, so do NOT
            //  do not use the engine_exit_base value
            exit(EXIT_SUCCESS);
        }
    }
    //    now we're out of the pusher process.
    //    if we don't want to keep this proc, we close it's input, and
    //    wait for it to exit.
    if (!keep_proc)
    {
        close(to_minion[1]);
        if (internal_trace)
            fprintf(stderr, "minion input pipe closed\n");
    }

    //   and see what is in the pipe for us.
    outbuf[0] = 0;
    done = 0;
    outlen = 0;
    //   grot grot grot this only works if varnames are not widechars
    if (*from_var)
    {
        if (async_mode == 0 && keep_proc == 0)
        {
            usleep(timeout);
            //   synchronous read- read till we hit EOF, which is read
            //   returning a char count of zero.
            readloop:
            if (internal_trace)
                fprintf(stderr, "SYNCH READ ");
            usleep(timeout);
            charsread =
                read(from_minion[0],
                    &outbuf[done],
                    (data_window_size >> SYSCALL_WINDOW_RATIO) - done - 2);
            done = done + charsread;
            if (charsread > 0
                && done + 2 < (data_window_size >> SYSCALL_WINDOW_RATIO))
                goto readloop;
            if (done < 0)
                done = 0;
            outbuf[done] = 0;
            outlen = done;
        }
        if (keep_proc == 1 || async_mode == 1)
        {
            //   we're in either 'keep' 'async' mode.  Set nonblocking mode, then
            //   read it once; then put it back in regular mode.
            //fcntl (from_minion[0], F_SETFL, O_NONBLOCK);
            //      usleep (timeout);
            charsread = read(from_minion[0],
                &outbuf[done],
                (data_window_size >> SYSCALL_WINDOW_RATIO));
            done = charsread;
            if (done < 0)
                done = 0;
            outbuf[done] = 0;
            outlen = done;
            //fcntl (from_minion[0], F_SETFL, 0);
        }

        //   If the minion process managed to fill our buffer, and we
        //   aren't "keep"ing it around, OR if the process is "async",
        //   then we should also launch a sucker process to
        //   asynchronously eat all of the stuff we couldn't get into
        //   the buffer.  The sucker proc just reads stuff and throws it
        //   away asynchronously... and exits when it gets EOF.
        //
        if (async_mode
            || (outlen >= ((data_window_size >> SYSCALL_WINDOW_RATIO) - 2)
                && keep_proc == 0))
        {
            sucker = fork();
            if (sucker == 0)
            {
                //  we're in the sucker process here- just throw away
                //  everything till we get EOF, then exit.
                while (1)
                {
                    usleep(timeout);
                    charsread = read(from_minion[0],
                        &outbuf[0],
                        data_window_size >> SYSCALL_WINDOW_RATIO);
                    //  in the sucker here, don't use engine_exit_base exit
                    if (charsread == 0)
                        exit(EXIT_SUCCESS);
                }
            }
        }

        //  and set the returned value into from_var.
        if (user_trace)
            fprintf(stderr, "SYSCALL output: %ld chars ---%s---.\n ",
                outlen, outbuf);
        if (internal_trace)
            fprintf(stderr, "  storing return str in var %s\n", from_var);

        crm_destructive_alter_nvariable(from_var, vlen, outbuf, outlen);
    }

    //  Record useful minion data, if possible.
    if (strlen(keep_buf) > 0)
    {
        sprintf(exp_keep_buf,
            "MINION PROC PID: %d from-pipe: %d to-pipe: %d",
            minion,
            from_minion[0],
            to_minion[1]);
        if (internal_trace)
            fprintf(stderr, "   saving minion state: %s \n",
                exp_keep_buf);
        crm_destructive_alter_nvariable(keep_buf, keep_len,
            exp_keep_buf,
            strlen(exp_keep_buf));
    }
    //      If we're keeping this minion process around, record the useful
    //      information, like pid, in and out pipes, etc.
    if (keep_proc || async_mode)
    { }
    else
    {
        if (internal_trace)
            fprintf(stderr, "No keep, no async, so not keeping minion, closing everything.\n");

        //    de-zombify any dead minions;
        waitpid(minion, &minion_exit_status, 0);

        //   we're not keeping it around, so close the pipe.
        //
        close(from_minion[0]);

        if (crm_vht_lookup(vht, keep_buf, strlen(keep_buf)))
        {
            char exit_value_string[MAX_VARNAME];
            if (internal_trace)
                fprintf(stderr, "minion waitpid result :%d; whacking %s\n",
                    minion_exit_status,
                    keep_buf);
            sprintf(exit_value_string, "DEAD MINION, EXIT CODE: %d",
                WEXITSTATUS(minion_exit_status));
            if (keep_len > 0)
                crm_destructive_alter_nvariable(keep_buf, keep_len,
                    exit_value_string,
                    strlen(exit_value_string));
        }
    }
#elif defined (WIN32)
    if (0)
    {
        fatalerror(" Sorry, syscall is completely b0rked in this version", "");
        return 1;
    }
    else
    {
        HANDLE hminion = 0;
        HANDLE hminion_thread_pi = 0;
        HANDLE pusher_thread_handle;
        HANDLE sucker_thread_handle;
        cnt = 0;

        if (*exp_keep_buf)
        {
            cnt = sscanf(exp_keep_buf, "MINION PROC PID: %d from-pipe: %p to-pipe: %p",
                &minion,
                &from_minion[0],
                &to_minion[1]);
        }
        if (!(cnt == 3
              || (cnt == 0 && (*exp_keep_buf == 0 || strncmp(exp_keep_buf, "DEAD MINION", WIDTHOF("DEAD MINION") - 1) == 0))))
        {
            nonfatalerror("Failed to decode the syscall minion setup: ", exp_keep_buf);
            return 1;
        }

        status = 1;

        //      if, no minion already existing, we create
        //      communications pipes and launch the subprocess.  This
        //      code borrows concepts from both liblaunch and from
        //      netcat (thanks, *Hobbit*!)
        //
        if (minion == 0)
        {
            int vstart2, vlen2;
            int varline;

            if (user_trace)
                fprintf(stderr, "  Must start a new minion.\n");

            crm_nextword(sys_cmd, strlen(sys_cmd), 0, &vstart2, &vlen2);
            varline = crm_lookupvarline(vht, sys_cmd, vstart2, vlen2);
            if (varline > 0)
            {
                fatalerror(" Sorry, syscall to a label isn't implemented in this version", "");
                status = 0;
            }
            else
            {
                SECURITY_ATTRIBUTES pipeSecAttr;
                STARTUPINFO si;
                PROCESS_INFORMATION pi = { 0 };
                HANDLE stdout_save, stdin_save;
                DWORD error = 0;

                static const char *prefixes[] =
                {
                    "",
                    "cmd /c ",
                    "command /c ",
                    "rundll32 SHELL32.DLL,ShellExec_RunDLL runas ",
                };
                int i;

                pipeSecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
                pipeSecAttr.bInheritHandle = TRUE;   // must be TRUE or the handles generated cannot be used by the system child process running the syscall commandline.
                pipeSecAttr.lpSecurityDescriptor = NULL;


                // Get the handle to the current STDOUT.
                stdout_save = GetStdHandle(STD_OUTPUT_HANDLE);
                stdin_save = GetStdHandle(STD_INPUT_HANDLE);

                // Create a pipe for the child process's STDOUT: read+write handles
                status = CreatePipe(&from_minion[0], &from_minion[1], &pipeSecAttr, 0 /* 2 ^ 10 * 32 */);
                if (!status)
                {
                    fatalerror_Win32("Failed to create minion pipe #1", sys_cmd);
                }
                // Ensure the read handle to the pipe for STDOUT is not inherited.
                status = SetHandleInformation(from_minion[0], HANDLE_FLAG_INHERIT, 0);
                if (!status)
                {
                    fatalerror_Win32("Failed to set minion pipe #1 attributes", sys_cmd);
                }

                status = CreatePipe(&to_minion[0], &to_minion[1], &pipeSecAttr, 0 /* 2 ^ 10 * 32 */);
                if (!status)
                {
                    fatalerror_Win32("Failed to create minion pipe #2", sys_cmd);
                }
                // Ensure the write handle to the pipe for STDIN is not inherited.
                status = SetHandleInformation(to_minion[1], HANDLE_FLAG_INHERIT, 0);
                if (!status)
                {
                    fatalerror_Win32("Failed to set minion pipe #1 attributes", sys_cmd);
                }

                if (user_trace)
                {
                    fprintf(stderr, "systemcalling on shell command %s\n",
                        sys_cmd);
                }

                // MSVC spec says 'sys_cmd' will be edited inside CreateProcess. Keep the original around for
                // the second attempt:
                strcpy(sys_cmd_2nd, sys_cmd);

                for (i = 0; i < WIDTHOF(prefixes); i++)
                {
                    /* this might be an 'internal' command: try a second (and another) time, now after prefixing it with 'cmd /c', etc: */
                    strcpy(sys_cmd, prefixes[i]);
                    strncat(sys_cmd, sys_cmd_2nd, WIDTHOF(sys_cmd) - strlen(sys_cmd));
                    sys_cmd[WIDTHOF(sys_cmd) - 1] = 0;

                    ZeroMemory(&si, sizeof(si));
                    // si.cb = sizeof(si);
                    si.cb = sizeof(STARTUPINFO);
                    si.hStdError = from_minion[1];
                    si.hStdOutput = from_minion[1];
                    si.hStdInput = to_minion[0];
                    si.dwFlags |= STARTF_USESTDHANDLES;

                    si.dwFlags |= STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;

                    si.lpTitle = "crm114 child process";

                    ZeroMemory(&pi, sizeof(pi));

                    // Create the child process.
                    status = CreateProcess(NULL,
                        sys_cmd,                                              // command line
                        NULL,                                                 // process security attributes
                        NULL,                                                 // primary thread security attributes
                        TRUE,                                                 // handles are inherited
                        CREATE_DEFAULT_ERROR_MODE /* | CREATE_NEW_CONSOLE */, // creation flags
                        NULL,                                                 // use parent's environment
                        NULL,                                                 // use parent's current directory
                        &si,                                                  // STARTUPINFO pointer
                        &pi);                                                 // receives PROCESS_INFORMATION
                    if (!status)
                    {
                        error = GetLastError();

                        if (error == ERROR_FILE_NOT_FOUND)
                        {
                            /* this might be an 'internal' command: try a second time, now after prefixing it with 'cmd /c': */
                            continue;
                        }
                    }
                    break;
                }

                if (!status)
                {
                    char errmsg[MAX_PATTERN];
                    char *s = errmsg;

                    Win32_syserr_descr(&s, MAX_PATTERN, error, sys_cmd);

                    // use the undamaged sys_cmd entry for error reporting...
                    nonfatalerror_ex(
                        SRC_LOC(), "This program tried a shell command that "
                                   "didn't run correctly.\n"
                                   "command >>%s<< - CreateProcess returned %ld(0x%08lx:%s)\n",
                        sys_cmd_2nd,
                        (long)error,
                        (long)error,
                        errmsg);
                }
                else
                {
                    //      CloseHandle(pi.hProcess);
                    //     CloseHandle(pi.hThread);

                    minion = pi.dwProcessId;
                    hminion = pi.hProcess;
                    hminion_thread_pi = pi.hThread;

#if 0
                    if (!SetStdHandle(STD_OUTPUT_HANDLE, stdout_save))
                    {
                        fatalerror_Win32("Failed to reset stdout for crm114", sys_cmd);
                    }
                    if (!SetStdHandle(STD_INPUT_HANDLE, stdin_save))
                    {
                        fatalerror_Win32("Failed to reset stdin for crm114", sys_cmd);
                    }
#endif
#if 0
                    if (!CloseHandle(pi.hThread))
                    {
                        fatalerror_Win32("Failed to close the execution thread handle for minion",
                            sys_cmd);
                    }
#endif
                }
            }
        }
        else
        {
            if (user_trace)
                fprintf(stderr, "  reusing old minion PID: %d\n", minion);
            hminion = OpenProcess(PROCESS_ALL_ACCESS, 0, minion);
            if (hminion == NULL)
            {
                fatalerror_Win32("Couldn't open the existing minion process", sys_cmd);
                status = 0;
            }
            hminion_thread_pi = 0;
        }

        if (status)
        {
            int error;

#if 0
            //      Now, we're out of the minion for sure.
            //    so we close the pipe ends we know we won't be using.
            if (to_minion[0] != 0)
            {
                if (!CloseHandle(to_minion[0]))
                {
                    fatalerror_Win32("Failed to close read pipe for minion", sys_cmd);
                }
                if (!CloseHandle(from_minion[1]))
                {
                    fatalerror_Win32("Failed to close write pipe for minion", sys_cmd);
                }
            }
#endif

            // Ensure the read handle to the pipe for STDOUT is inheritable.
            status = SetHandleInformation(from_minion[0], HANDLE_FLAG_INHERIT, 1);
            if (!status)
            {
                fatalerror_Win32("Failed to minion pipe #1 attributes", sys_cmd);
            }

            // Ensure the write handle to the pipe for STDIN is inheritable.
            status = SetHandleInformation(to_minion[1], HANDLE_FLAG_INHERIT, 1);
            if (!status)
            {
                fatalerror_Win32("Failed to minion pipe #1 attributes", sys_cmd);
            }

            // before we go off and push or pull data to/from the child, make sure it has initialized properly:
            status = WaitForInputIdle(hminion, INFINITE);
            error = GetLastError();
            if (status && error)
            {
                fatalerror_Win32("Child seems to have failed to initialize", sys_cmd);
            }

            //
            //   launch "pusher" process to send the buffer to the minion
            //    (this hint from Dave Soderberg). This avoids the deadly
            //   embrace situation where both processes are waiting to read
            //   (or, equally, both processes have written and filled up
            //   their buffers, and are now held up waiting for the other
            //   process to empty some space in the output buffer)
            //
            if (strlen(inbuf) > 0)
            {
                unsigned int hThread;
                pusherparams *pp;
                char *inbuf_copy = (char *)calloc((inlen + 1), sizeof(inbuf_copy[0]));
                int i;

                if (!inbuf_copy)
                {
                    fatalerror_Win32("Failed to allocate buffer memory for passing on to the sucker thread", sys_cmd);
                }

                pp = (pusherparams *)calloc(1, sizeof(pp[0]));
                if (!pp)
                {
                    fatalerror_Win32("Failed to allocate memory for passing on to the sucker thread", sys_cmd);
                }

                // Since the pusher thread may continue executing after the
                // syscall statement has finished, we need to make a copy of
                // inbuf for the pusher thread to use. The pusher process will
                // free the memory.
                for (i = 0; i < inlen; i++)
                    inbuf_copy[i] = inbuf[i];
                inbuf_copy[inlen] = 0;

                pp->my_ptr = pp;
                pp->inbuf = inbuf_copy;
                pp->inlen = inlen;
                pp->internal_trace = internal_trace;
                pp->keep_proc = keep_proc;
                pp->to_minion = to_minion[1];
                pusher_thread_handle = (HANDLE)_beginthreadex(NULL,
                    0,
                    pusher_proc,
                    pp,
                    CREATE_SUSPENDED,
                    &hThread);
                if (pusher_thread_handle == 0)
                {
                    fatalerror_ex(SRC_LOC(),
                        "Failed to init the pusher thread: "
                        "error code %d (%s)",
                        errno,
                        errno_descr(errno));
                }
                else
                {
                    DWORD ret = ResumeThread(pusher_thread_handle);
                    if (ret < 0)
                    {
                        fatalerror_Win32("Failed to start the pusher thread", sys_cmd);
                    }
                    else
                    {
                        // do NOT wait until all data has been sent to the child process.
#if 0
                        WaitForSingleObject(pusher_thread_handle, INFINITE);
#endif
                    }
                }
            }

            //   and see what is in the pipe for us.
            outbuf[0] = 0;
            done = 0;
            outlen = 0;

            //   grot grot grot this only works if varnames are not widechars
            if (*from_var)
            {
                if (async_mode == 0)
                {
                    int eof = FALSE;
                    long readlen;
                    LARGE_INTEGER large_int;
                    DWORD file_flags;
                    DWORD chars_available;
                    DWORD chars_avail_in_this_message;
                    DWORD exit_code;


                    // Sleep(timeout);
                    // wait until all data has been fetched from the child process.

                    status = WaitForInputIdle(hminion, INFINITE);
                    error = GetLastError();

                    //   synchronous read- read till we hit EOF, which is read
                    //   returning a char count of zero.
                    for ( ; ;)
                    {
                        if (internal_trace)
                            fprintf(stderr, "SYNCH READ ");
                        charsread = 0;

                        /*
                         * From the Microsoft docs:
                         *
                         * If hFile is not opened with FILE_FLAG_OVERLAPPED and lpOverlapped is NULL,
                         * the read operation starts at the current file position and ReadFile does
                         * not return until the operation is complete, and then the system updates
                         * the file pointer.
                         *
                         * If hFile is not opened with FILE_FLAG_OVERLAPPED and lpOverlapped is not
                         * NULL, the read operation starts at the offset that is specified in the
                         * OVERLAPPED structure. ReadFile does not return until the read operation
                         * is complete, and then the system updates the file pointer.
                         */

                        exit_code = (DWORD)-1;
                        status = GetExitCodeProcess(hminion, &exit_code);
                        error = GetLastError();
                        if (internal_trace)
                            fprintf(stderr, "GetExitCodeProcess() = %d/%d, %ld\n", status, error, exit_code);
#if 0
                        if (status && error == ERROR_SUCCESS && exit_code == STILL_ACTIVE)
                        {
                            // command is still running; expect more input lateron
                            eof = FALSE;
                        }
#endif

                        status = GetFileSizeEx(from_minion[0], &large_int);
                        error = GetLastError();
                        if (internal_trace)
                            fprintf(stderr,
                                "GetFileSizeEx() = %d/%d, %ld:%ld\n",
                                status,
                                error,
                                large_int.HighPart,
                                large_int.LowPart);

                        if (status && error == ERROR_SUCCESS && !large_int.HighPart && !large_int.LowPart)
                        {
                            if (exit_code != STILL_ACTIVE)
                            {
                                // the syscall has terminated, there will not arrive anything anymore to fetch lateron...
                                eof = TRUE;
                            }
                        }
                        // else: eof = FALSE

#if 0
                        file_flags = 0;
                        status = GetHandleInformation(from_minion[0], &file_flags);
                        error = GetLastError();
                        if (internal_trace)
                            fprintf(stderr, "GetHandleInformation() = %d/%d, %08lX\n", status, error, file_flags);

                        readlen = (data_window_size >> SYSCALL_WINDOW_RATIO) - done - 2;

                        status = PeekNamedPipe(from_minion[0],
                            outbuf + done,
                            readlen,
                            &charsread,
                            &chars_available,
                            &chars_avail_in_this_message);
                        error = GetLastError();
                        if (internal_trace)
                            fprintf(stderr, "PeekNamedPipe() = %d/%d, read:%ld, avail: %ld, in msg: %ld\n", status, error,
                                charsread,
                                chars_available,
                                chars_avail_in_this_message);
                        // kinda funny: chars_avail_in_this_message is -chars_available (yep, minus!)
#endif

                        CRM_ASSERT(charsread == 0);
                        if (!eof)
                        {
                            // ReadFile() will lock indefinitely when all output has been fetched
                            // and the syscall has terminated, i.e. the exit code is known then.
                            // Hence the !eof flag check here: it signals there won't be coming
                            // any more.
                            readlen = (data_window_size >> SYSCALL_WINDOW_RATIO) - done - 2;

                            if (large_int.HighPart || large_int.LowPart)
                            {
                                if (!ReadFile(from_minion[0],
                                        outbuf + done,
                                        readlen,
                                        &charsread, NULL))
                                {
                                    error = GetLastError();
                                    switch (error)
                                    {
                                    case ERROR_HANDLE_EOF:
                                        // At the end of the file.
                                        eof = TRUE;
                                        break;

                                    case ERROR_IO_PENDING:
                                        // I/O pending.
                                        break;

                                    default:
                                        fatalerror_Win32("Failed to sync-read any data from the minion",
                                            sys_cmd);
                                        break;
                                    }
                                }
                                error = GetLastError();

                                switch (error)
                                {
                                case ERROR_HANDLE_EOF:
                                    // At the end of the file
                                    eof = TRUE;
                                    break;
                                }
                            }
                        }

#if 0
                        if (charsread < readlen)
                        {
                            eof = TRUE; // <-- this cinches it: since ReadFile() will not return until completed when in sync read mode.
                        }
#endif

                        done = done + charsread;
                        if (!eof /* && charsread > 0 */ && done + 2 <
                            (data_window_size >> SYSCALL_WINDOW_RATIO))
                        {
                            status = WaitForInputIdle(hminion, timeout);
                            // wait a little while before we try to fetch another bit of data...
                            continue; // goto readloop;
                        }
                        if (done < 0)
                            done = 0;
                        outbuf[done] = 0;
                        outlen = done;
                        break;
                    }
                }
                else
                {
                    //   we're in 'async' mode. Just grab what we can
                    int anything_to_read = TRUE;
                    DWORD exit_code;
                    LARGE_INTEGER large_int;

                    exit_code = (DWORD)-1;
                    status = GetExitCodeProcess(hminion, &exit_code);
                    error = GetLastError();
                    if (internal_trace)
                        fprintf(stderr, "GetExitCodeProcess() = %d/%d, %ld\n", status, error, exit_code);

                    memset(&large_int, 0, sizeof(large_int));
                    status = GetFileSizeEx(from_minion[0], &large_int);
                    error = GetLastError();
                    if (internal_trace)
                        fprintf(stderr,
                            "GetFileSizeEx() = %d/%d, %ld:%ld\n",
                            status,
                            error,
                            large_int.HighPart,
                            large_int.LowPart);

                    if ( /* status && error == ERROR_SUCCESS && */ !large_int.HighPart && !large_int.LowPart)
                    {
                        if (exit_code != STILL_ACTIVE)
                        {
                            // the syscall has terminated, there will not arrive anything anymore to fetch lateron...
                            anything_to_read = FALSE;
                        }
                    }

                    charsread = 0;
                    if (anything_to_read)
                    {
                        // ReadFile() will lock indefinitely when all output has been fetched
                        // and the syscall has terminated, i.e. the exit code is known then.
                        // Hence the !eof flag check here: it signals there won't be coming
                        // any more.
                        if (large_int.HighPart || large_int.LowPart)
                        {
                            if (!ReadFile(from_minion[0],
                                    &outbuf[done],
                                    (data_window_size >> SYSCALL_WINDOW_RATIO), &charsread, NULL))
                            {
                                fatalerror_Win32("Failed to async read any data from the minion", sys_cmd);
                            }
                        }
                    }

                    done = charsread;
                    if (done < 0)
                        done = 0;
                    outbuf[done] = 0;
                    outlen = done;
                }

                //   If the minion process managed to fill our buffer, and we
                //   aren't "keep"ing it around, OR if the process is "async",
                //   then we should also launch a sucker process to
                //   asynchronously eat all of the stuff we couldn't get into
                //   the buffer.  The sucker proc just reads stuff and throws it
                //   away asynchronously... and exits when it gets EOF.
                //
                if (async_mode || (outlen >= ((data_window_size >> SYSCALL_WINDOW_RATIO) - 2)
                                   && keep_proc == 0))
                {
                    unsigned int hThread;
                    // allocate fire & forget memory to pass to the sucker thread to use...
                    suckerparams *sp = (suckerparams *)calloc(1, sizeof(sp[0]));
                    if (!sp)
                    {
                        fatalerror_Win32("Failed to allocate memory for passing on to the sucker thread", sys_cmd);
                    }
                    sp->my_ptr = sp;
                    sp->from_minion = from_minion[0];
                    sp->timeout = timeout;
                    sp->internal_trace = internal_trace;
                    sp->keep_proc = keep_proc;

                    sucker_thread_handle = (HANDLE)_beginthreadex(NULL,
                        0,
                        sucker_proc,
                        sp,
                        CREATE_SUSPENDED,
                        &hThread);
                    if (sucker_thread_handle == 0)
                    {
                        fatalerror_ex(SRC_LOC(),
                            "Failed to start the sucker thread: "
                            "error code %d (%s)",
                            errno,
                            errno_descr(errno));
                    }
                    else
                    {
                        DWORD ret = ResumeThread(sucker_thread_handle);
                        if (ret < 0)
                        {
                            fatalerror_Win32("Failed to start the sucker thread", sys_cmd);
                        }
                        else
                        {
                            // do NOT wait until all data has been fetched from the child process.
#if 0
                            WaitForSingleObject(sucker_thread_handle, INFINITE);
#endif
                        }
                    }
                }

                //  and set the returned value into from_var.
                if (user_trace)
                    fprintf(stderr, "SYSCALL output: %ld chars ---%s---.\n ",
                        outlen, outbuf);
                if (internal_trace)
                    fprintf(stderr, "  storing return str in var %s\n", from_var);

                crm_destructive_alter_nvariable(from_var, vlen, outbuf, outlen);
            }

            //  Record useful minion data, if possible.
            if (strlen(keep_buf) > 0)
            {
                sprintf(exp_keep_buf,
                    "MINION PROC PID: %d from-pipe: %p to-pipe: %p",
                    minion,
                    from_minion[0],
                    to_minion[1]);
                if (internal_trace)
                    fprintf(stderr, "   saving minion state: %s\n", exp_keep_buf);
                crm_destructive_alter_nvariable(keep_buf, keep_len,
                    exp_keep_buf,
                    strlen(exp_keep_buf));
            }

            //      If we're keeping this minion process around, record the useful
            //      information, like pid, in and out pipes, etc.
            if (!keep_proc && !async_mode)
            {
                DWORD exit_code;
                if (internal_trace)
                    fprintf(stderr,
                        "No keep, no async, so not keeping minion, closing everything.\n");

                if (WAIT_FAILED == WaitForSingleObject(hminion, INFINITE))
                {
                    fatalerror_Win32("Failed while waiting for the minion to terminate", sys_cmd);
                }

                if (!GetExitCodeProcess(hminion, &exit_code))
                {
                    fatalerror_Win32("Failed to grab the exit code from the system call", sys_cmd);
                }
                if (crm_vht_lookup(vht, keep_buf, strlen(keep_buf)))
                {
                    char exit_value_string[MAX_VARNAME];
                    if (internal_trace)
                    {
                        fprintf(stderr, "minion exit code :%d; whacking %s\n",
                            exit_code,
                            keep_buf);
                    }
                    sprintf(exit_value_string, "DEAD MINION, EXIT CODE: %d",
                        exit_code);
                    if (keep_len > 0)
                    {
                        crm_destructive_alter_nvariable(keep_buf, keep_len,
                            exit_value_string,
                            strlen(exit_value_string));
                    }
                }
                if (!CloseHandle(hminion))
                {
                    fatalerror_Win32("Failed to close the system call minion handle", sys_cmd);
                }
                if (hminion_thread_pi != NULL && !CloseHandle(hminion_thread_pi))
                {
                    fatalerror_Win32("Failed to close the system call minion handle", sys_cmd);
                }
            }
        }
    }
#else
    fatalerror(" Sorry, syscall is not supported in this version", "");
#endif
    return 0;
}







