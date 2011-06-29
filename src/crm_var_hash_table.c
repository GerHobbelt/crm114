//  crm_var_hash_table.c  - Controllable Regex Mutilator,  version v1.0
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




//      initialize the variable hash table (the vht)
//      and stuff in the "standards" (:_vars:, environment vars)
//
void crm_vht_init(int argc, char **argv)
{
    int uvstart = 0; // uvstart is the arg that the user sees (post "--")
    int uvlist = 0;
    char uvset[MAX_VARNAME];
    char posvars[MAX_VARNAME];


    //   create the variable hash table (one big one, shared )
    vht = (VHT_CELL **)calloc(vht_size, sizeof(vht[0]));
    if (!vht)
    {
        untrappableerror("Couldn't alloc VHT cell.\n",
                "No VHT cells, no variables, so no can run.  Sorry.");
    }
#ifndef CRM_DONT_ASSERT
    {
        int i;

        for (i = 0; i < vht_size; i++)
            CRM_ASSERT(vht[i] == NULL);
    }
#endif


    //    initialize the temporary (non-data-window) area...
    tdw = calloc(1, sizeof(tdw[0]));
    if (!tdw)
    {
        untrappableerror("Couldn't alloc tdw.\n"
                         "We need the TDW for isolated variables."
                         "Can't continue.  Sorry.\n", "");
    }
    tdw->filename = NULL;
    tdw->rdwr = 1;
    tdw->filedes = -1;
    tdw->filetext = calloc(data_window_size, sizeof(tdw->filetext[0]));
    tdw->filetext_allocated = 1;
    if (!tdw->filetext)
    {
        untrappableerror("Couldn't alloc tdw->filetext.\n"
                         "Without this space, you can't have any isolated "
                         "variables,\n and we're stuck.  Sorry.", "");
    }
    tdw->filetext[0] = 0;
    tdw->nchars = 0;
    tdw->hash = 0;
    tdw->mct = NULL;
    tdw->nstmts = -1;
    tdw->cstmt = -1;
    tdw->caller = NULL;

    //    install a few constants.

    crm_set_temp_var(":_nl:", "\n");
    crm_set_temp_var(":_ht:", "\t");
    crm_set_temp_var(":_bs:", "\b");
    crm_set_temp_var(":_sl:", "/");
    crm_set_temp_var(":_sc:", ";");
    crm_set_temp_var(":_cd:", "0");
    crm_set_temp_var("::", " ");

    // [i_a] extension: HIDDEN_DEBUG_FAULT_REASON_VARNAME keeps track of the last error/nonfatal/whatever error report:
    //
    // but only when we're running the debugger or _expect_ to run the debugger!
    if (debug_countdown > DEBUGGER_DISABLED_FOREVER)
    {
        // make sure the variable exists...
        crm_set_temp_var(HIDDEN_DEBUG_FAULT_REASON_VARNAME, "");
    }

    //   put the version string in as a variable.
    {
        char verstr[1025];
        verstr[0] = 0;
        strcat(verstr, VERSION);
        strcat(verstr, " ( ");
        strcat(verstr, crm_regversion());
        strcat(verstr, " )");
        crm_set_temp_var(":_crm_version:", verstr);
    }

    crm_set_temp_var(":_hosttype:", HOSTTYPE);

    //
    //    install the argc and argv values; restart argv values from [2]
    //    if a "--" metaflag is seen.
    //
    //    argv[0] and argv[1] are not overrideable by "--".
    crm_set_temp_var(":_arg0:", argv[0]);
    crm_set_temp_var(":_arg1:", argv[1]);

    //     Check to see if there's a "--" arg.  If so, mark uvstart
    //   (that is, "user var start" at that point)... but only the first "--".
    {
        int i, j;

        uvstart = 2;
        i = 0;
        j = 0;
        for (i = 2; argc > i; i++)
        {
            //   Check for the "--" metaflag
            if (strlen(argv[i]) == 2
                && strncmp(argv[i], "--", 2) == 0
                && uvstart == 2)
            {
                if (internal_trace)
                    fprintf(stderr, "Resetting uvstart counter to 2\n");
                uvstart = i + 1;
            }
            ;
        }
    };

    //       The user variables start at argv[uvstart]
    {
        int i, j;
        char anamebuf[255];

        j = 2;
        for (i = uvstart; argc > i; i++)
        {
            sprintf(anamebuf, ":_arg%d:", j);
            crm_set_temp_var(anamebuf, argv[i]);
            j++;
        }
        ;
        //
        //    and put the "user-visible" argc into a var as well.
        sprintf(anamebuf, "%d", j);
        crm_set_temp_var(":_argc:", anamebuf);
        //
        //   Go through argv, and place positional arguments (that is,
        //   arguments that don't contain any '-' preambles) into
        //   :_pos0:, :_pos1:, ...
        //
        //   :_pos0: is always the name of the CRM114 engine.
        //   :_pos1: is always the name of the program being run.
        //   :_pos2: and so on are the command line args.
        //
        //    prepare to treasure up the positional args
        posvars[0] = 0;
        j = 0;

        for (i = uvstart; i < argc; i++)
        {
            //
            //   check for the "-" sign; this is a positional argument only
            //                     if there is no "-" sign.
            if (argv[i][0] != '-')
            {
                sprintf(anamebuf, ":_pos%d:", j);
                crm_set_temp_var(anamebuf, argv[i]);
                j++;
                if (j > 0)
                    strcat(posvars, " ");
                strcat(posvars, argv[i]);
            }
        }
        sprintf(anamebuf, "%d", j);
        crm_set_temp_var(":_posc:", anamebuf);
        crm_set_temp_var(":_pos_str:", posvars);
        //
        //   and set the fault to be a null string for now.
        crm_set_temp_var(":_fault:", "");
        //
        //   set the current line number to a set of zeroes...
        crm_set_temp_var(":_cs:", "00000000");
        //
        //   Set the "lazy" intermediate variable to just a space.
        //   This will get rebound to point to the active lazy var.
        crm_set_temp_var(":_lazy:", " ");

        //    set the current pid and parent pid.
        {
            char pidstr[32];
            int pid;
#if defined (HAVE_GETPID)
            pid = (int)getpid();
            sprintf(pidstr, "%d", pid);
            crm_set_temp_var(":_pid:", pidstr);
#endif
#if defined (HAVE_GETPPID)
            pid = (int)getppid();
            sprintf(pidstr, "%d", pid);
            crm_set_temp_var(":_ppid:", pidstr);
#endif
        }
    };

    //      now, we shove the whole contents of the ENVIRON
    //      vector into the VHT.

    CRM_ASSERT(tempbuf != NULL);
    tempbuf[0] = 0;
    if (!ignore_environment_vars)
    {
#define FAKE_PWD 0x0001
#define FAKE_USER 0x0002

        int got_to_fake_em = FAKE_PWD | FAKE_USER;

        int i, j;

        i = 0;

        while (environ[i])
        {
            char *name;
            char *s;

            if (strlen(tempbuf) + strlen(environ[i]) < (data_window_size - 1000))
            {
                strcat(tempbuf, environ[i]);
                strcat(tempbuf, "\n");
            }
            else
            {
                untrappableerror("The ENVIRONMENT variables don't fit into the "
                                 "available space. \nThis is very broken.  Try "
                                 "a larger data window (with flag -w NNNNN), \nor "
                                 "drop the environment vars with "
                                 "the (with flag -e)", "");
            }
            j = (int)strcspn(environ[i], "="); /* this also takes care of any line _without_ an '=': treat it as a var with a null value */

            /* [i_a] GROT GROT GROT: patch Win32 specific 'faked' env. vars in here: USER, PWD, ... */
            if (strncmp(environ[i], "PWD=", 4) == 0)
            {
                got_to_fake_em &= ~FAKE_PWD;
            }
            else if (strncmp(environ[i], "USER=", 5) == 0)
            {
                got_to_fake_em &= ~FAKE_USER;
            }

            name = (char *)calloc((j + 200), sizeof(name[0]));
            if (!name)
            {
                untrappableerror("Couldn't alloc :_env_ space."
                                 "Can't continue.\n", "Stick a fork in us; we're _done_.");
            }
            s = strmov(name, ":_env_");
            memmove(s, environ[i], j);
            s += j;
            strcpy(s, ":");
            if (environ[i][j] != 0)
                j++; //  step past the equals sign.
            //value = strdup(&(environ[i][j]));
            crm_set_temp_var(name, &(environ[i][j]));
            free(name);
            //free(value);
            i++; //   and do the next environment variable
        }

        // [i_a] patch Win32 specific 'faked' env. vars in here: USER, PWD, ...
        // ALSO patch those buggers in here if the UNIX env. did not provide them
        // for some reason (stripped bare 'secure sandbox' environment or some?)
        if (got_to_fake_em & FAKE_PWD)
        {
#if 0
            char dirbuf[CRM_MAX(MAX_VARNAME, MAX_PATH) + 1];
            char fulldirbuf[CRM_MAX(MAX_VARNAME, MAX_PATH) + 1];
            DWORD dirbufsize = CRM_MAX(MAX_VARNAME, MAX_PATH) + 1;

            if (!GetCurrentDirectoryA(dirbufsize, dirbuf))
            {
                fatalerror_Win32("Cannot fetch the current directory (PWD)", NULL);
            }
            else
            {
                /*
                 *      From the MS docs:
                 *
                 *      In certain rare cases, if the specified directory is on the
                 *      current drive, the function might omit the drive letter and
                 *      colon from the path. Therefore, the size that is returned
                 *      by the function might be two characters less than the size
                 *      of the specified string, not including the terminating null
                 *      character. This behavior might occur in edge situations
                 *      such as in a services application. If you need the drive
                 *      letter, make a subsequent call to GetFullPathName to
                 *      retrieve the drive letter.
                 */
                if (!GetFullPathName(dirbuf, CRM_MAX(MAX_VARNAME, MAX_PATH) + 1, fulldirbuf, NULL))
                {
                    fatalerror_ex(SRC_LOC(),
                            "Cannot fetch the expanded current directory (PWD) for directory '%s'",
                            dirbuf);
                }
                else
                {
                    crm_set_temp_var(":_env_PWD:", dirbuf);
                    if (strlen(dirbuf) + WIDTHOF("PWD=") < (data_window_size - 1000))
                    {
                        strcat(tempbuf, "PWD=");
                        strcat(tempbuf, dirbuf);
                        strcat(tempbuf, "\n");
                    }
                    else
                    {
                        untrappableerror("The ENVIRONMENT variables don't fit into the "
                                         "available space. \nThis is very broken.  Try "
                                         "a larger data window (with flag -w NNNNN), \nor "
                                         "drop the environment vars with "
                                         "the (with flag -e)", "");
                    }
                }
            }
#else
            char dirbuf[DIRBUFSIZE_MAX];

            if (!mk_absolute_path(dirbuf, WIDTHOF(dirbuf), "."))
            {
                fatalerror("Cannot fetch the current directory (PWD)", "Stick a fork in us. We're _done_.");
            }
            else
            {
                crm_set_temp_var(":_env_PWD:", dirbuf);
                if (strlen(dirbuf) + WIDTHOF("PWD=") < (data_window_size - 1000))
                {
                    strcat(tempbuf, "PWD=");
                    strcat(tempbuf, dirbuf);
                    strcat(tempbuf, "\n");
                }
                else
                {
                    untrappableerror("The ENVIRONMENT variables don't fit into the "
                                     "available space. \nThis is very broken.  Try "
                                     "a larger data window (with flag -w NNNNN), \nor "
                                     "drop the environment vars with "
                                     "the (with flag -e)", "");
                }
            }
#endif
        }
        else if (got_to_fake_em & FAKE_USER)
        {
#if defined (HAVE_GETPWUID_R) && defined (HAVE_GETUID)
            struct passwd pwbuf;
            struct passwd *pwbufret = NULL;
            char buf[1024 * 5];

            if (getpwuid_r(getuid(), &pwbuf, buf, WIDTHOF(buf), &pwbufret)
                || !pwbufret
                || !pwbufret->pw_name)
            {
                fatalerror_ex(SRC_LOC(), "Cannot fetch the USER name for UID %ld: error = %d(%s)",
                        (long int)getuid(),
                        errno,
                        errno_descr(errno));
            }
            else
            {
                crm_set_temp_var(":_env_USER:", pwbufret->pw_name);
                if (strlen(pwbufret->pw_name) + WIDTHOF("USER=") < (data_window_size - 1000))
                {
                    strcat(tempbuf, "USER=");
                    strcat(tempbuf, pwbufret->pw_name);
                    strcat(tempbuf, "\n");
                }
                else
                {
                    untrappableerror("The ENVIRONMENT variables don't fit into the "
                                     "available space. \nThis is very broken.  Try "
                                     "a larger data window (with flag -w NNNNN), \nor "
                                     "drop the environment vars with "
                                     "the (with flag -e)", "");
                }
            }
#elif defined (HAVE_GETPWUID) && defined (HAVE_GETUID)
            struct passwd *pwbufret;

            pwbufret = getpwuid(getuid());
            if (!pwbufret
                || !pwbufret->pw_name)
            {
                fatalerror_ex(SRC_LOC(), "Cannot fetch the USER name for UID %ld: error = %d(%s)",
                        (long int)getuid(),
                        errno,
                        errno_descr(errno));
            }
            else
            {
                crm_set_temp_var(":_env_USER:", pwbufret->pw_name);
                if (strlen(pwbufret->pw_name) + WIDTHOF("USER=") < (data_window_size - 1000))
                {
                    strcat(tempbuf, "USER=");
                    strcat(tempbuf, pwbufret->pw_name);
                    strcat(tempbuf, "\n");
                }
                else
                {
                    untrappableerror("The ENVIRONMENT variables don't fit into the "
                                     "available space. \nThis is very broken.  Try "
                                     "a larger data window (with flag -w NNNNN), \nor "
                                     "drop the environment vars with "
                                     "the (with flag -e)", "");
                }
            }
#elif defined (HAVE_GETUSERNAMEA) // do not check for WIN32; it does not have to be defined for 64-bit WIN64 or WinCE or other, so do the 'proper autoconf thing' here.
            char userbuf[UNLEN + 1];
            DWORD userbufsize = UNLEN + 1;

            if (!GetUserNameA(userbuf, &userbufsize))
            {
                nonfatalerror_Win32("Cannot fetch the USER name.", NULL);
            }
            else
            {
                crm_set_temp_var(":_env_USER:", userbuf);
                if (strlen(userbuf) + WIDTHOF("USER=") < (data_window_size - 1000))
                {
                    strcat(tempbuf, "USER=");
                    strcat(tempbuf, userbuf);
                    strcat(tempbuf, "\n");
                }
                else
                {
                    untrappableerror("The ENVIRONMENT variables don't fit into the "
                                     "available space. \nThis is very broken.  Try "
                                     "a larger data window (with flag -w NNNNN), \nor "
                                     "drop the environment vars with "
                                     "the (with flag -e)", "");
                }
            }
#else
#error "Please provide suitable code for username detection for your platform."
#endif
        }
    }
    crm_set_temp_var(":_env_string:", tempbuf);

    //    see if argv [1] is a '-( whatever) arg, which limits the
    //    set of runtime parameters allowed on the command line.
    //    If so, we have the limit list.  We put spaces around the
    //    args so we can just use strstr(3) to see if an arg is permitted
    //    or if we should fault out.  Note that at this point,
    //    we've trashed the contents of uvlist (the parens and the
    //    trailing '--', if there was one.
    //
    if (strncmp(argv[1], "-(", 2) == 0)
    {
        int closepos;
        uvlist = 1;
        strcpy(uvset, " ");
        strncat(uvset, &argv[1][2], strlen(argv[1]) - 3);
        //   nuke the closing paren
        closepos = 2;
        while (uvset[closepos] != ')' && uvset[closepos] != 0)
            closepos++;
        uvset[closepos] = 0;
        strcat(uvset, " ");
        if (user_trace)
            fprintf(stderr, "UVset: =%s=\n", uvset);
    }
    //
    //
    //   go through argv again, but this time look for "--foo"
    //   and "--foo=bar" args.
    //
    {
        int i, j, k;
        char anamebuf[MAX_VARNAME];
        char avalbuf[MAX_VARNAME];
        int isok;

        i = 0;
        j = 0;
        k = 0;
        for (i = uvstart; argc > i; i++)
        {
            //   check for the "--" metaflag preamble
            if (strlen(argv[i]) > 2 && strncmp(argv[i], "--", 2) == 0)
            {
                isok = 1;
                if (uvlist == 1)
                {
                    isok = 0;
                    //   build a testable name out of the -- flagname
                    strcpy(anamebuf, " ");
                    j = 2;
                    k = 1;
                    while (argv[i][j] != 0 && argv[i][j] != '=')
                    {
                        anamebuf[k] = argv[i][j];
                        j++;
                        k++;
                    }
                    anamebuf[k] = 0;
                    strcat(anamebuf, " ");
                    //
                    //       now we have the var name, surrounded by spaces
                    //       we strstr() it to see if it's allowed or not.
                    if (strstr(uvset, anamebuf))
                        isok = 1;
                    //
                    //        Well, maybe the name by itself is too loose;
                    //        also allow name=value
                    strcpy(anamebuf, " ");
                    strcat(anamebuf, &argv[i][2]);
                    strcat(anamebuf, " ");
                    if (strstr(uvset, anamebuf))
                        isok = 1;
                }
                if (isok)
                {
                    if (internal_trace)
                        fprintf(stderr, "setting cmdline string %s", argv[i]);
                    strcpy(avalbuf, "SET");
                    j = 2;
                    k = 0;
                    //  copy the varname into anamebuf
                    anamebuf[k] = ':';
                    k++;
                    while (argv[i][j] != 0 && argv[i][j] != '=')
                    {
                        anamebuf[k] = argv[i][j];
                        j++;
                        k++;
                    }
                    anamebuf[k] = ':';
                    k++;
                    anamebuf[k] = 0;
                    if (argv[i][j] == '=')
                    {
                        j++;  //  skip over the = sign
                        k = 0;
                        while (argv[i][j] != 0)
                        {
                            avalbuf[k] = argv[i][j];
                            j++;
                            k++;
                        }
                        avalbuf[k] = 0;
                    }
                    if (user_trace)
                    {
                        fprintf(stderr, "\n Setting cmdline var '%s' to '%s'\n",
                                anamebuf, avalbuf);
                    }
                    crm_set_temp_var(anamebuf, avalbuf);
                }
                else
                {
                    fprintf(stderr,
                            "\n ***Warning*** "
                            "This program does not accept the "
                            "flag '%s',\n", anamebuf);
                    fprintf(stderr,
                            " so we'll just ignore it for now. \n");
                }
            }
        }
    }
}


//           routine to put a variable into the temporary (tdw)
//           buffer.  names and values end up interleaved
//           sequentially, separated by newlines.  TDW really should have
//           been called the idw (Isolated Data Window) but it's too
//           late to fix it now.
//
void crm_set_temp_nvar(const char *varname, const char *value, int vallen)
{
    int namestart, namelen;
    int valstart;
    int i;
    int vnidx, vnlen;

    //       do the internal_trace thing
    if (internal_trace)
        fprintf(stderr, "  setting temp-area variable %s to value %s\n",
                varname, value);

    if (!crm_nextword(varname, (int)strlen(varname), 0, &vnidx, &vnlen))
    {
        nonfatalerror("Somehow, you are assigning a value to a variable with "
                      "an unprintable name.  I'll permit it for now, but "
                      "your program is probably broken.", "");
    }

    if ((strlen(varname) + vallen + tdw->nchars + 64) > data_window_size)
    {
        fatalerror("This program has overflowed the ISOLATEd data "
                   "area with a variable that's just too big.  "
                   "The bad variable was named: ",
                varname);
        if (engine_exit_base != 0)
        {
            exit(engine_exit_base + 22);
        }
        else
        {
            exit(EXIT_FAILURE);
        }
    }

    //       check- is this the first time we've seen this variable?  Or
    //       are we re-assigning a previous variable?
    i = crm_vht_lookup(vht, &varname[vnidx], vnlen);
    if (vht[i] == NULL)
    {
        //  never assigned this variable before, so we stick it in the
        //  tdr window.
        //
        //       do the name first.  Start with a newline.
        //  GROT GROT GROT
        tdw->filetext[tdw->nchars] = 'X';
        tdw->nchars++;
        namestart = tdw->nchars;
        namelen = vnlen;
        memmove(&(tdw->filetext[tdw->nchars]), &(varname[vnidx]), namelen);
        tdw->nchars = tdw->nchars + namelen;
        //
        //        and add a separator to prevent the varname from sharing
        //        an endpoint offset with the var value.
        tdw->filetext[tdw->nchars] = '=';
        tdw->nchars++;
        //
        //       and the value second
        valstart = tdw->nchars;
        memmove(&tdw->filetext[tdw->nchars], value, vallen);
        tdw->nchars = tdw->nchars + vallen;
        //
        //       add a separator again, so we don't get strings with overlapped
        //       ranges into the var hash table
        tdw->filetext[tdw->nchars] = 'Y';
        tdw->nchars++;
        //
        //        and put a NUL at the end of the tdw, so debuggers won't get
        //       all bent out of shape.
        tdw->filetext[tdw->nchars] = 0;
        //
        //      now, we whack the actual VHT.
        crm_setvar(NULL, 0,
                tdw->filetext, namestart, namelen,
                tdw->filetext, valstart, vallen,
                0, 0);
        //     that's it.
    }
    else
    {
        //   This variable is preexisting.  Perform an ALTER on it.
        //
        crm_destructive_alter_nvariable(&varname[vnidx], vnlen,
                value, vallen);
    }
}

//     GROT GROT GROT this routine needs to replaced for 8-bit-safeness.
//     Use ONLY where you can be sure no embedded NULs will be seen (i.e.
//     fixed strings in the early startup.
//
void crm_set_temp_var(const char *varname, const char *value)
{
    crm_set_temp_nvar(varname, value, (int)strlen(value));
}



//           routine to put a data-window-based (the cdw, that is)
//           variable into the VHT.  The text of the variable's name
//           goes into the tdw buffer, and the value stays in the main
//           data window (cdw) buffer.
//
//           This is equivalent to a "bind" operation - that is, the
//           pointers move around, but the data window doesn't get
//           changed.
//
//           Note - if you rebind a var, you should consider if your
//           routine should also evaluate the old area for reclamation.
//           (reclamation uses "crm_compress_tdw_section", see comments
//           further down in the code here)

void crm_set_windowed_nvar(char *varname,
        int                      varlen,
        char                    *valtext,
        int                      start,
        int                      len,
        int                      stmtnum)
{
    int i;
    int namestart, namelen;

    //       do the internal_trace thing
    if (internal_trace)
    {
        int i;
        fprintf(stderr, "  setting data-window variable %s to value ",
                varname);
#if 0
        for (i = start; i < start + len; i++)
            fprintf(stderr, "%c", valtext[i]);
#else
        fwrite_ASCII_Cfied(stderr, valtext + start, len);
#endif
        fprintf(stderr, "\n");
    }

    //    check and see if the variable is already in the VHT
    i = crm_vht_lookup(vht, varname, varlen);
    if (vht[i] == NULL)
    {
        //  nope, never seen this var before, add it into the VHT
        //       namestart is where we are now.
        if (internal_trace)
            fprintf(stderr, "... new var\n");
        //
        //      Put the name into the tdw memory area, add a & after it.
        //
        //       do the name first.  Start on a newline.
        tdw->filetext[tdw->nchars] = '\n';
        tdw->nchars++;
        namestart = tdw->nchars;
        namelen = varlen;
        memmove(&tdw->filetext[namestart], varname, varlen);
        tdw->nchars = tdw->nchars + namelen;
        //
        //     put in an "&" separator
        tdw->filetext[tdw->nchars] = '&';
        tdw->nchars++;
        //
        //      now, we whack the actual VHT.
        crm_setvar(NULL, 0,
                tdw->filetext, namestart, namelen,
                valtext, start, len,
                stmtnum, 0);
        //     that's it.
    }
    else
    {
        //     We've seen this var before.  But, there's a gotcha.
        //     If the var _was_ in the tdw, but is now being moved back
        //     to the cdw, or being rebound inside another tdw var,
        //     then the prior var value might now be dead- that is, "leaked
        //     memory", and now inaccessible.
        //
        {
            //     move the text/start/len values around to accomodate the new
            //     value.
            //
            if (internal_trace)
                fprintf(stderr, "... old var\n");
            crm_setvar(NULL, 0,
                    vht[i]->nametxt, vht[i]->nstart, vht[i]->nlen,
                    valtext, start, len,
                    stmtnum, 0);

            //       Do we need to repair the leaked memory?  Only necessary if the
            //       old text was in the tdw area; this is harmless if the area
            //       is in use by another var, but if we have removed the last
            //       reference to any tdw-based vars, we ought to reclaim them..
            //
            //       NOTE - we don't do it here since synchronicity issues
            //       between a var being rebound, reclamation happening,
            //       and then another var _in the same match_ being bound
            //       (to a old, unupdated set of offsets) is such a pain.
            //
            //       Instead, routines using this routine should also be sure
            //       to call crm_compress_tdw_section if there's a chance they
            //       should be releasing TDW memory. AFTER they've done  ALL the
            //       rebinding.  That way, all indices and offsets are in the VHT
            //       where they can be safely updated.
            //
        }
    }
}

//#define RECLAIM_ALL_EVERY_TIME 1
#ifdef RECLAIM_ALL_EVERY_TIME
//
//    How we compress out an area that might no longer be in use.
static int crm_recursive_compress_tdw_section
(char *oldtext, int oldstart, int oldend);

int crm_compress_tdw_section(char *oldtext, int oldstart, int oldend)
{
    //   let's court death, and do a FULL compress.
    return crm_recursive_compress_tdw_section
           (tdw->filetext, 0, tdw->nchars + 1);
}
int crm_recursive_compress_tdw_section
(char *oldtext, int oldstart, int oldend)
#else
int crm_compress_tdw_section(char *oldtext, int oldstart, int oldend)
#endif
{
    //  The algorithm basically checks to see if there is any region of
    //  the given tdw space that is not currently used by another var.
    //  All such regions are reclaimed with a slice-n-splice.  We return
    //  the number of reclaimed characters.
    //
    //  The algorithm starts out with start and end of the tenatively
    //  unused "to be killed" region.  It checks each member of the VHT
    //  in the TDW.  If the region overlaps, don't kill the overlapping
    //  part of the region.  If at any time the region length goes to 0,
    //  we know that there's no region left to kill.  (Option- if the
    //  gap is less than MAX_RECLAIMER_GAP chars, we don't bother moving
    //  it; we retain it as buffer.  This minimizes thrashing
    //
    //  NOTE that the END VALUES ONLY "oldend" and "newend" vars are
    //  NON-inclusive, they index the first NON-involved character
    //  (oldstart and newstart index "involved" characters, that we _do_
    //  include in our strings)
    //
    //  BIG ISSUE: As coded, this routine needs to leave a _buffer_ of at
    //  least one UNUSED character between each used (but isolated) string
    //  area.  Knowing when to get rid of extra copies of this character
    //  has been a big hassle.  Right now there may be a small leak here
    //  so if you can find it, please let me know!   Note that any fix that
    //  does not keep two adjacent isolated regions from merging (including
    //  when the first or second becomes a zero-length string!) will get
    //  the submittor a gentle smile and a pointer to this very comment.
    //  (the reason being that prior code that did not leave a buffer
    //  exhibited the property that if A and B were isolated but adjacent,
    //  and then A shrank to 0 length, then B would share the same start
    //  point, and an alteration to A would then *also* insert at the start
    //  point of B, causing A and B to become NONisolated and space-sharing.
    //  That said- enjoy the bug hunt.  :)

    int j, newstart, newend, reclaimed;

    j = newstart = newend = reclaimed = 0;

    //  return (0);

    j = newstart = newend = reclaimed = 0;
    if (internal_trace)
        fprintf(stderr, " [ Compressing isolated data.  Length %d chars, "
                        "start %d, len %d ]\n",
                tdw->nchars,
                oldstart,
                oldend - oldstart);

    //      If oldstart >= oldend, then there's no compression to be done.
    //
    if (oldstart >= oldend)
    {
        if (internal_trace)
            fprintf(stderr, " [ Zero-length compression string... don't do this! ]\n");
        return 0;
    }

    if (oldtext != tdw->filetext)
    {
        fatalerror(" Request to compress non-TDW data.  This is bogus. ",
                " Please file a bug report");
        return 0;
    }

    //    Look one character further to before and after;
    //if (oldstart > 3)  oldstart --;
    //if (oldend < data_window_size - 1) oldend ++;

    for (j = 0; j < vht_size; j++)
    {
        if (vht[j]  // is this slot in use?
            && vht[j]->valtxt == tdw->filetext
            //   Note that being part of :_iso: does NOT exclude from reclamation
            &&  0 != strncmp(&vht[j]->nametxt[vht[j]->nstart], ":_iso:", 6))
        {
            //    for convenience, we get nice short names:
            newstart = vht[j]->vstart - 1;
            newend = newstart + vht[j]->vlen + 2;
            //  leave some space no matter what...
            if (newend < newstart + 2)
                newend = newstart + 2;

            //    6 Possible cases:
            //      dead zone entirely before current var
            //      dead zone entirely after current var
            //      dead zone entirely inside current var
            //      dead zone overlaps front of current var
            //      dead zone overlaps back of current var
            //      dead zone split by current var
            //

            //      1: dead zone entirely before current var
            //
            //       <os------------oe>
            //                           <ns--------ne>
            //
            if (oldend <= newstart)
            {
                //   nothing to be done here - not overlapping
                goto end_of_vstring_tests;
            }

            //      2: dead zone entirely after current var
            //
            //                        <os------------oe>
            //       <ns--------ne>
            //
            if (newend <= oldstart)
            {
                //   nothing to be done here - not overlapping
                goto end_of_vstring_tests;
            }

            //   If we get this far, the dead zone in some way overlaps with
            //   our current variable.

            //      3: dead zone entirely inside a currently live var
            //
            //                 <os-------oe>
            //              <ns----------------ne>
            //
            //      So we terminate this procedure (nothing can be reclaimed)
            //
            if (oldstart >= newstart && oldend <= newend)
            {
                //   the dead zone is inside a non-dead var, so
                //   we can terminate our search right now.
                if (internal_trace)
                    fprintf(stderr, " [ Compression not needed after all. ]\n");
                return 0;
            }

            //      4: dead zone overlaps front of current var; we trim the
            //      dead zone to not include the current var.
            //
            //       <os------------oe>
            //              <ns--------ne>
            //
            if (oldstart < newstart && oldend <= newend)
            {
                //   The dead zone should not include the part that's
                //   also new variable.  So, we clip out the part
                //   that's still active.
                if (internal_trace)
                    fprintf(stderr, " [ Trimming tail off of compression. ]\n");
                //
                //     newstart is a "good" char, but since oldend is
                //     noninclusive, this is right.
                oldend = newstart;
                goto end_of_vstring_tests;
            }

            //      5: dead zone overlaps back of current var; trim the front off
            //      the dead zone.
            //
            //                   <os------------oe>
            //              <ns--------ne>
            //
            if (newstart <= oldstart && newend <= oldend)
            {
                if (internal_trace)
                    fprintf(stderr, " [ Trimming head off of compression. ]\n");
                //
                //     Newend is the first char that ISN'T in the var, so this
                //     is correct.
                oldstart = newend;
                goto end_of_vstring_tests;
            }
            //      6: dead zone split by current var - the dead zone is actually
            //      split into two distinct pieces.  In this case, we need to
            //      recurse on the two pieces.
            //
            //         <os--------------------oe>
            //               <ns--------ne>
            //
            if (oldstart <= newstart && newend <= oldend)
            {
                if (internal_trace)
                {
                    fprintf(stderr, " [ Compression split ]\n");
                    fprintf(stderr, " [ First part will be %d to %d ]\n",
                            oldstart, newstart);
                    fprintf(stderr, " [ Second part will be %d to %d ]\n",
                            newend, oldend);
                }
                //
                //      Tricky bit here - we have to do the aft (ne-oe
                //      section) first, so we don't move the os-ns
                //      section offsets.
                //

                //    was newend - 1, but should be same as case 3
                //   above (dead zone overlaps tail)
#ifdef RECLAIM_ALL_EVERY_TIME
                reclaimed = crm_recursive_compress_tdw_section(oldtext, newend, oldend);
                reclaimed += crm_recursive_compress_tdw_section(oldtext, oldstart, newstart);
#else
                reclaimed = crm_compress_tdw_section(oldtext, newend, oldend);
                reclaimed += crm_compress_tdw_section(oldtext, oldstart, newstart);
#endif
                //   Return here instead of executing common slice-and-splice
                //   tail, because each of our recursive children will do
                //   that for us.
                return reclaimed;
            }
        }
end_of_vstring_tests:
        //    Now, repeat with the name string - all name strings are protected
        if (vht[j]
            && vht[j]->nametxt == tdw->filetext)
        {
            newstart = vht[j]->nstart - 1;
            newend = newstart + vht[j]->nlen + 2;
            //  leave some space no matter what...
            if (newend < newstart + 4)
                newend = newstart + 2;

            //    Possible cases:
            //      dead zone entirely before current var
            //      dead zone entirely after current var
            //      dead zone entirely inside current var
            //      dead zone overlaps front of current var
            //      dead zone overlaps back of current var
            //      dead zone split by current var
            //
            //      dead zone entirely before current var
            //
            //       <os------------oe>
            //                           <ns--------ne>
            // OK
            if (oldend <= newstart)
            {
                //   nothing to be done here - not overlapping
                goto end_of_nstring_tests;
            }

            //      dead zone entirely after current var
            //
            //                        <os------------oe>
            //       <ns--------ne>
            //
            if (newend <= oldstart)
            {
                //   nothing to be done here - not overlapping
                goto end_of_nstring_tests;
            }

            //   If we get this far, the dead zone in some way overlaps with
            //   our current variable.

            //      dead zone entirely inside a currently live var
            //
            //                 <os-------oe>
            //              <ns----------------ne>
            //
            //      So we terminate this procedure (nothing can be reclaimed)
            //
            if (oldstart >= newstart && oldend <= newend)
            {
                //   the dead zone is inside a non-dead var, so
                //   we can terminate our search right now.
                if (internal_trace)
                    fprintf(stderr, " [ Compression not needed after all. ]\n");
                return 0;
            }

            //      dead zone overlaps front of current var; we trim the
            //      dead zone to not include the current var.
            //
            //       <os------------oe>
            //              <ns--------ne>
            //
            if (oldstart < newstart && oldend <= newend)
            {
                //   The dead zone should not include the part that's
                //   also new variable.  So, we clip out the part
                //   that's still active.
                if (internal_trace)
                    fprintf(stderr, " [ Trimming tail off of compression. ]\n");
                //
                //     newstart is a "good" char, but since oldend is
                //     noninclusive, this is right.
                oldend = newstart;
                goto end_of_nstring_tests;
            }

            //      dead zone overlaps back of current var; trim the front off
            //      the dead zone.
            //
            //                   <os------------oe>
            //              <ns--------ne>
            //
            if (newstart <= oldstart && newend <= oldend)
            {
                if (internal_trace)
                    fprintf(stderr, " [ Trimming head off of compression. ]\n");
                //
                //     Newend is the first char that ISN'T in the var, so this
                //     is correct.
                oldstart = newend;
                goto end_of_nstring_tests;
            }
            //      dead zone split by current var - the dead zone is actually
            //      split into two distinct pieces.  In this case, we need to
            //      recurse on the two pieces.
            //
            //         <os--------------------oe>
            //               <ns--------ne>
            //
            if (oldstart <= newstart && newend <= oldend)
            {
                if (internal_trace)
                {
                    fprintf(stderr, " [ Compression split ]\n");
                    fprintf(stderr, " [ First part will be %d to %d ]\n",
                            oldstart, newstart);
                    fprintf(stderr, " [ Second part will be %d to %d ]\n",
                            newend, oldend);
                }
                //
                //      Tricky bit here - we have to do the aft (ne-oe
                //      section) first, so we don't move the os-ns
                //      section offsets.
                //

                //    was newend - 1, but should be same as case 3
                //   above (dead zone overlaps tail)
#ifdef RECLAIM_ALL_EVERY_TIME
                reclaimed = crm_recursive_compress_tdw_section(oldtext, newend, oldend);
                reclaimed += crm_recursive_compress_tdw_section(oldtext, oldstart, newstart);
#else
                reclaimed = crm_compress_tdw_section(oldtext, newend, oldend);
                reclaimed += crm_compress_tdw_section(oldtext, oldstart, newstart);
#endif
                //   Return here instead of executing common slice-and-splice
                //   tail, because each of our recursive children will do
                //   that for us.
                return reclaimed;
            }
            //       and the semicolon to conform to the ANSI C standard
end_of_nstring_tests:
            ;
        }
    }
    //
    //   Well, we've now scanned the VHT, and oldstart/oldend are the
    //   actual dead zone (storage that really isn't used).
    //
    //   So, we can compress this storage out with a slice-and-splice
    //   return how many character cells we were able to reclaim.
    //
    {
        int cutlen;
        // cutlen is supposed to be negative for compress
        cutlen = oldstart - oldend - 1;
        if (cutlen > 0)
            fatalerror("Internal cut-length error in isolated var reclamation.",
                    "  Please file a bug report");

        //    Future Enhancement - dead zones of some small size should be
        //    allowed to stay.  This would speed up WINDOW a lot. (but we
        //    would need to expand the range of oldstart and oldend to
        //    actually reclaim those areas if storage really ran low.
        //    Maybe this should be compile-time or command-line parameter?)

        if (cutlen < 0)
        {
            if (internal_trace)
            {
                fprintf(stderr, " [ compression slice-splice at %d for %d chars ]\n",
                        oldstart, cutlen);
            }
            crm_slice_and_splice_window(tdw, oldstart, cutlen);
            if (internal_trace)
            {
                fprintf(stderr, " [ new isolated area will be %d bytes ]\n",
                        tdw->nchars);
            }
        }
        return -(cutlen);
    }
}

//
//     Destructive alteration of a preexisting variable, which can be
//     anywhere.  If the variable is not preexisting, we create it and
//     toss a nonfatal error.
//

void crm_destructive_alter_nvariable(const char *varname, int varlen,
        const char *newstr, int newlen)
{
    int i;
    int vhtindex, oldlen, delta;
    int vlen;

    //      get the first variable name and verify it exists.
    //
    // [i_a] GROT GROT GROT: this 'trimming of whitespace' etc. should be completely unnecessary anyway.
    //       inspect code using it and get rid of this.
    if (crm_nextword(varname, varlen, 0, &i, &vlen))
    {
        vhtindex = crm_vht_lookup(vht, &(varname[i]), vlen);
        if (vht[vhtindex] == NULL)
        {
            // IGNORE FOR NOW
            crm_set_temp_nvar(&varname[i], newstr, newlen);
            nonfatalerror_ex(SRC_LOC(), "Attempt to alter the value of a nonexistent "
                                        "variable, so I'm creating an ISOLATED variable.  "
                                        "I hope that's OK.  The nonexistent variable is: "
                                        "%d/%d: '%.*s'/'%.*s'",
                    vlen, varlen, vlen, &(varname[i]), varlen, varname);
            return;
        }
    }
    else
    {
        fatalerror("Attempt to alter the value of a nonexistent variable.",
                "");
        return;
    }

    //     make enough space in the input buffer to accept the new value
    oldlen = vht[vhtindex]->vlen;
    delta = newlen - oldlen;
    mdw = NULL;
    if (tdw->filetext == vht[vhtindex]->valtxt)
        mdw = tdw;
    if (cdw->filetext == vht[vhtindex]->valtxt)
        mdw = cdw;
    //   GROT GROT GROT  get rid of this if we go to MAPped file vars.
    if (mdw == NULL)
    {
        nonfatalerror(" Bogus text block containing variable: ", varname);
        return;
    }
    //
    if (user_trace) // major debug
    {
        int i;
        // fprintf(stderr, "\n     surgery on the var %s\n ", varname);
        fprintf(stderr, " surgery on the var >");
#if 0
        for (i = 0; i < varlen; i++)
            fprintf(stderr, "%c", varname[i]);
#else
        fwrite_ASCII_Cfied(stderr, varname, varlen);
#endif
        fprintf(stderr, "<\n");
        //fprintf(stderr, "new value is: \n***%s***\n", newstr);
        fprintf(stderr, " new value is ***>");
#if 0
        for (i = 0; i < newlen; i++)
            fprintf(stderr, "%c", newstr[i]);
#else
        fwrite_ASCII_Cfied(stderr, newstr, newlen);
#endif
        fprintf(stderr, "<***\n");
    }
    //     slice and splice the mdw text area, to make the right amount of
    //     space...
    crm_slice_and_splice_window(mdw, vht[vhtindex]->vstart, delta);
    //
    //      Zap the mstart and mlen markers so that searches are reset to start
    //      of the variable.  Note that we have to do this _after_ we slice
    //      and splice, otherwise we mangle our own mstart and mlen.
    vht[vhtindex]->mstart = vht[vhtindex]->vstart;
    vht[vhtindex]->mlen = 0;
    //
    //     now we have space, and we can put in the characters from
    //     the new pattern
    memmove(&(mdw->filetext[vht[vhtindex]->vstart]),
            newstr,
            newlen);
}


//      Surgically lengthen or shorten a window.  The window pointed
//      to by mdw gets delta extra characters added or cut at "where".
//      (more precisely, just _before_ "where" - the insert/delete
//      point is just before the "where'th" character, and the
//      where'th character will be the first one moved.  If the
//      allocated length is not enough, additional space can be
//      malloced.  Finally, the vht is fixed up so everything still
//      points "correctly".
//
void crm_slice_and_splice_window(CSL_CELL *mdw, int where, int delta)
{
    char *taildest;
    char *tailsrc;
    int taillen;

    //    these are to keep the compiler quiet.
    taildest = NULL;
    tailsrc = NULL;
    taillen = 0;

    if (delta + mdw->nchars > data_window_size - 10)
    {
        fatalerror(" Data window trying to get too long.",
                " Try increasing the data window maximum size.");
        goto bailout;
    }

    if (delta == 0)
    {
        if (internal_trace)
        {
            fprintf(stderr, " zero delta, no buffer hackery required\n");
        }
        return;
    }

    // bump chars in input window delta places
    if (internal_trace)
    {
        fprintf(stderr, "moving text in window %p,", mdw->filetext);
        fprintf(stderr, " starting at %d, ", where);
        fprintf(stderr, "delta length is %d\n", delta);
    }

    if (delta > 0)
    {
        // lengthening alteration...
        taildest = &(mdw->filetext[where + delta]);
        tailsrc = &(mdw->filetext[where]);
        taillen = mdw->nchars - where;
    }

    if (delta < 0) //   shortening alteration
    {
        taildest = &(mdw->filetext[where]);
        tailsrc = &(mdw->filetext[where - delta]); //  delta is minus already!!
        taillen = mdw->nchars - where + delta;
        //      taillen = mdw->nchars + 1 - where;
    }
    if (internal_trace)
        fprintf(stderr,
                "buffer sliding, tailsrc: %p, taildest: %p, length: %d\n",
                tailsrc, taildest, taillen);

    //     and move the actual data
    if (taillen + 1 > 0)
        memmove(taildest, tailsrc, taillen + 1);

    //     update the length of the window as well.
    mdw->nchars = mdw->nchars + delta;

    //      and update all of our captured variables to have the right ptrs.
    crm_updatecaptures(mdw->filetext,
            where,
            delta);
bailout:
    // semicolon to conform to the ANSI C standard
    ;
}

//        allow_data_window_to_grow
#ifdef no_dont_do_this_yet
//    Grow the window to hold the incoming text, if needed.
//    Grow it by 4x each time.
while (delta + mdw->nchars > data_window_size - 1)
{
    char *ndw;
    int odws, i;
    odws = data_window_size;
    data_window_size = 4 * data_window_size;
    nonfatalerror(" Data window trying to get too long.",
            " increasing data window... ");
    ndw = (char *)calloc(data_window_size, sizeof(ndw[0]));
    if (!ndw)
    {
        untrappableerror("Couldn't alloc ndw.  This is bad too.\n", "");
    }

    //  now copy the old data window into the new one
    memmove(ndw, mdw->filetext, odws);

    //   and update the outstanding pointers, like the ones in the
    //   vht...
    for (i = 0; i < vht_size; i++)
        if (vht[i] != NULL)
        {
            if (vht[i]->nametxt == mdw->filetext)
                vht[i]->nametxt = ndw;
            if (vht[i]->valtxt == mdw->filetext)
                vht[i]->valtxt = ndw;
        }

    //    and lastly, point the cdw or tdw to the new larger window.
    free(mdw->filetext);
    mdw->filetext = ndw;
}
#endif


//
//    crm_vht_lookup - given a char *start, int len, varnam
//    finds and returns the vht index of the variable
//    or the index of the appropriate NULL slot to put
//    the var in, if not found.

int crm_vht_lookup(VHT_CELL **vht, const char *vname, size_t vlen)
{
    crmhash_t hc;
    int i, j, k;
    int done;
    int vsidx;
    int vslen;

    j = 0; // just so J is used.

    //   Consistency scan - look for those null varnames!  Do this every
    //   time!
    if (internal_trace || user_trace)
    {
        int i, j;
        for (i = 0; i < vht_size; i++)
        {
            if (vht[i] != NULL && vht[i]->nlen < 2)
            {
                fprintf(stderr, "Short length for i=%d: %d\n", i, vht[i]->nlen);
            }
            if (vht[i] != NULL && vht[i]->nlen > 1)
            {
                int corrupted = 0;

                if (vht[i]->nametxt[vht[i]->nstart] != ':')
                {
                    fprintf(stderr, "Ztart corrupted; ");
                    corrupted = 1;
                }
                if (vht[i]->nametxt[vht[i]->nstart + vht[i]->nlen - 1] != ':')
                {
                    fprintf(stderr, "Zend corrupted; ");
                    corrupted = 1;
                }
                if (corrupted)
                {
                    fprintf(stderr, " i=%d len=%d name='",
                            i, vht[i]->nlen);
#if 0
                    for (j = 0; j < vht[i]->nlen; j++)
                        fprintf(stderr, "%c",
                                vht[i]->nametxt[vht[i]->nstart + j]);
#else
                    fwrite_ASCII_Cfied(stderr, vht[i]->nametxt + vht[i]->nstart, vht[i]->nlen);
#endif
                    fprintf(stderr, "'\n");
                }
            }
        }
    }


    if (crm_nextword(vname, vlen, 0, &vsidx, &vslen))
    {
        if (internal_trace)
        {
            fprintf(stderr, " variable len %d, name is -", vslen);
#if 0
            for (k = vsidx; k < vsidx + vslen; k++)
                fprintf(stderr, "%c", vname[k]);
#else
            fwrite_ASCII_Cfied(stderr, vname + vsidx, vslen);
#endif

            fprintf(stderr, "- .\n");
        }

        hc = strnhash(&vname[vsidx], vslen) % vht_size;

        //  go exploring - find either an empty cell (meaning that this
        //  is the first time this variable name has been entered into the
        //  vht) or find the variable already entered.  Or find that we've
        //  gone the whole way 'round the vht, in which case the vht is full
        //  and we should print ut a message and fatal error away (or maybe
        //  even build a bigger vht?)

        i = hc;

        //   consider a "wrap" to have occurred if we even think about
        //   the slot just before the hashcoded slot

        done = 0;
        while (!done)
        {
            //  is there anything here yet?
            if (vht[i] == NULL)
            {
                if (internal_trace)
                {
                    int ic;
                    fprintf(stderr, "  var ");
#if 0
                    for (ic = 0; ic < vlen; ic++)
                        fprintf(stderr, "%c", vname[ic]);
#else
                    fwrite_ASCII_Cfied(stderr, vname, vlen);
#endif
                    fprintf(stderr, "(len %d) not at %d (empty)\n", (int)vlen, i);
                    fprintf(stderr, "Returning the index where it belonged.\n");
                }
                return i;
            }

            //  there's something here - is it what we have been seeking
            if (vlen == vht[i]->nlen
                && memcmp(&((vht[i]->nametxt)[vht[i]->nstart]),
                        vname,
                        vlen) == 0)
            {
                //  Yes, we found it.
                if (internal_trace)
                {
                    int ic;
                    fprintf(stderr, "  var '");
#if 0
                    for (ic = 0; ic < vht[i]->nlen; ic++)
                        fprintf(stderr, "%c", (vht[i]->nametxt)[ic + vht[i]->nstart]);
#else
                    fwrite_ASCII_Cfied(stderr, vht[i]->nametxt + vht[i]->nstart, vht[i]->nlen);
#endif
                    fprintf(stderr, " (len %d) found at %d (",
                            (int)vlen, i);
                    if (vht[i]->valtxt == cdw->filetext)
                    {
                        fprintf(stderr, "(main)");
                    }
                    else
                    {
                        fprintf(stderr, "(isol)");
                    }
                    fprintf(stderr, " s: %d, l:%d)\n",
                            vht[i]->vstart, vht[i]->vlen);
                }
                return i;
            }
            else
            {
                if (internal_trace)
                {
                    int ic;
                    fprintf(stderr, "\n Hash clash (at %d): wanted %s (len %d)",
                            i, vname, (int)vlen);
                    fprintf(stderr, " but found '");
#if 0
                    for (ic = 0; ic < vht[i]->nlen; ic++)
                        fprintf(stderr, "%c", (vht[i]->nametxt)[ic + vht[i]->nstart]);
#else
                    fwrite_ASCII_Cfied(stderr, vht[i]->nametxt + vht[i]->nstart, vht[i]->nlen);
#endif
                    fprintf(stderr, "' instead.");
                }
            }

            i++;
            //  check wraparound
            if (i >= vht_size)
                i = 0;

            //   check for hash table full - if it is, right now we
            //   do a fatal error.  Eventually we should just resize the
            //   hash table.  Even better- we should keep track of the number
            //   of variables, and thereby resize automatically whenever we
            //   get close to overflow.
            if (i == (hc - 1))
            {
                /* static */
                char badvarname[MAX_VARNAME];
                strncpy(badvarname, &vname[vsidx], (vslen < MAX_VARNAME ? vslen : MAX_VARNAME - 1));
                badvarname[(vslen < MAX_VARNAME ? vslen : MAX_VARNAME - 1)] = 0;
                if (internal_trace)
                {
                    int index;
                    fprintf(stderr, "Variable Hash Table Dump\n");
                    for (index = 0; index < vht_size; index++)
                    {
                        int ic;
                        fprintf(stderr, "  var '");
#if 0
                        for (ic = 0; ic < vht[index]->nlen; ic++)
                        {
                            fprintf(stderr, "%c",
                                    (vht[index]->nametxt)[ic + vht[index]->nstart]);
                        }
#else
                        fwrite_ASCII_Cfied(stderr, vht[index]->nametxt + vht[index]->nstart, vht[index]->nlen);
#endif
                        fprintf(stderr, "'[%d] found at %d (",
                                vht[index]->nlen,  index);
                        if (vht[index]->valtxt == cdw->filetext)
                        {
                            fprintf(stderr, "(main)");
                        }
                        else
                        {
                            fprintf(stderr, "(isol)");
                        }
                        fprintf(stderr, " s: %d, l:%d)\n",
                                vht[index]->vstart, vht[index]->vlen);
                    }
                }
                fatalerror(" Variable hash table overflow while looking "
                           "for variable: ",
                        badvarname);
                done = 1;
                return 0;
            }
        }
    }
    return 0;
}


//
//    crm_setvar - set the value of a variable into the VHT, putting a
//    new cell in if necessary.  Note that this ONLY modifies the VHT
//    data itself.  It does NOT do any of the background work like
//    copying data at all, copying varnames into the tdw, keeping track
//    of the cdw and tdw usage, etc.
//
void crm_setvar(
        char *filename,
        int   filedesc,
        char *nametxt,
        int   nstart,
        int   nlen,
        char *valtxt,
        int   vstart,
        int   vlen,
        int   linenumber,
        int   lazy_redirects
               )
{
    int i, j;   // some indices to bang on

    //  first off, see if the variable is already stored.

    i = crm_vht_lookup(vht, &(nametxt[nstart]), nlen);


    if (vht[i] == NULL)
    {
        //    Nope, this is an empty VHT slot

        //  allocate a fresh, empty VHT cell
        vht[i] = (VHT_CELL *)calloc(1, sizeof(vht[i][0]));
        if (!vht[i])
        {
            untrappableerror("Couldn't alloc space for VHT cell.  We need VHT cells for variables.  We can't continue.", "");
        }

        //  fill in the name info data
        vht[i]->filename = filename;
        vht[i]->filedesc = filedesc;
        vht[i]->nametxt = nametxt;
        vht[i]->nstart = nstart;
        vht[i]->nlen = nlen;
        vht[i]->vstart = 0;
        vht[i]->vlen = 0;
        vht[i]->lazy_redirects = lazy_redirects;
        //  and now that the slot has proper initial information,
        //  we can use the same code as is used in an update to do
        //  the initial setting of values.  This is good because
        //  if we someday change the way variable values are stored,
        //  we need change it only in one place.
    }
    else
    {
        //   The cell is already here.  :)
    }
    //   Either way, the cell is now here, so we can set the value.
    //
    vht[i]->valtxt = valtxt;
    vht[i]->vstart = vstart;
    vht[i]->vlen = vlen;
    vht[i]->mstart = vstart;
    vht[i]->mlen = 0;
    vht[i]->linenumber = linenumber;
    vht[i]->lazy_redirects = lazy_redirects;

    if (internal_trace)
    {
        j = 0;
        fprintf(stderr, "  Successful set value of ");

#if 0
        //for (j = 0; j < vht[i]->nlen; j++)
        //        fprintf(stderr, "%c", vht[i]->nametxt[vht[i]->nstart+j]);
        fwrite4stdio(&(vht[i]->nametxt[vht[i]->nstart]), vht[i]->nlen, stderr);
#else
        fwrite_ASCII_Cfied(stderr, vht[i]->nametxt + vht[i]->nstart, vht[i]->nlen);
#endif

        fprintf(stderr, " at vht entry %d ", i);

        fprintf(stderr, " with value -");
#if 0
        //      for (j = 0; j < vht[i]->vlen; j++)
        //        fprintf(stderr, "%c", vht[i]->valtxt[vht[i]->vstart+j]);
        fwrite4stdio(&(vht[i]->valtxt[vht[i]->vstart]), vht[i]->vlen, stderr);
#else
        fwrite_ASCII_Cfied(stderr, vht[i]->valtxt + vht[i]->vstart, vht[i]->vlen);
#endif

        fprintf(stderr, "- (start %d, length %d)",
                vht[i]->vstart, vht[i]->vlen);

        fprintf(stderr, "and %d lazy redirects", vht[i]->lazy_redirects);

        fprintf(stderr, "\n");
    }
}

//  look up what the line number is of a variable.
//
int crm_lookupvarline(VHT_CELL **vht, char *text, int start, int len)
{
    int i;   // some indices to bang on

    i = crm_vht_lookup(vht, &(text[start]), len);


    //    GROT GROT GROT
    //    We should check here for GOTOing a label that isn't in
    //    the current file (i.e. the equivalent of a C "longjmp").
    if (vht[i] != NULL)
    {
        // Yes, we found it.  Return the line number
        if (internal_trace)
            fprintf(stderr, "  looked up ... line number %d\n",
                    vht[i]->linenumber);
        return vht[i]->linenumber;
    }
    else
    {
#if 0
        int q;
        char *deathfu;
        deathfu = (char *)calloc((len + 10), sizeof(deathfu[0]));
        if (!deathfu)
        {
            untrappableerror("Couldn't alloc 'deathfu'.\n  Time to die. ", "");
        }
        strncpy(deathfu, &(csl->filetext[start]), len);
        deathfu[len] = 0;
        q = fatalerror("Control Referencinge a non-existent variable- this"
                       "is almost always a very _bad_ thing",
                deathfu);
        //  If fatalerror found a TRAP for this error, cstmt now points to
        //  the TRAP - 1.  We want to go to the trap itself, no auto-incr...
        if (q == 0)
            return csl->cstmt + 1;

#endif
    }
    return -1;
}

//      Update the start and length of all captured variables whenever
//      a buffer gets mangled.  Mangles are all expressed in
//      the form of a start point and a delta.
//
//     Note to the Reader - yes, I consider the nonlinearity of this
//     function to be a grossitude.  Not quite an obscenity, but definitely
//     a wart.

void crm_updatecaptures(char *text, int loc, int delta)
{
    int vht_index;
    int i;
    int ostart = 0, oend = 0;
    int nstart = 0, nend = 0;

    if (internal_trace)
        fprintf(stderr, "\n    updating captured values start %d len %d\n",
                loc, delta);

    //        check each VHT entry for a need to relocate
    for (vht_index = 0; vht_index < vht_size; vht_index++)
    {
        //      is this an actual entry?
        if (vht[vht_index] != NULL)
        {
            if (vht[vht_index]->valtxt == text)
            {
                // start of valtext block check
                // value text area
                if (internal_trace)
                {
                    fprintf(stderr, "\n  checking var ");
#if 0
                    for (i = 0; i < vht[vht_index]->nlen; i++)
                        fprintf(stderr, "%c",
                                vht[vht_index]->nametxt[vht[vht_index]->nstart + i]);
#else
                    fwrite_ASCII_Cfied(stderr, vht[vht_index]->nametxt + vht[vht_index]->nstart,  vht[vht_index]->nlen);
#endif
                    fprintf(stderr, " ");
                    fprintf(stderr, " s: %d, l:%d/%d, e:%d n:%d ~ %d ...",
                            vht[vht_index]->vstart,
                            vht[vht_index]->vlen,
                            vht[vht_index]->nlen,
                            vht[vht_index]->vstart + vht[vht_index]->vlen,
                            vht[vht_index]->nstart,
                            vht[vht_index]->nstart + vht[vht_index]->nlen
                           );
                }
                ostart = nstart = vht[vht_index]->vstart;
                oend = nstart = ostart + vht[vht_index]->vlen;
                nstart = crm_mangle_offset(ostart, loc, delta, 0);
                nend = crm_mangle_offset(oend, loc, delta, 1);
                if (internal_trace)
                    fprintf(stderr, "\n   index %d vstart/vlen upd: %d, %d ",
                            vht_index,
                            vht[vht_index]->vstart, vht[vht_index]->vlen);
                vht[vht_index]->vstart = nstart;
                vht[vht_index]->vlen = nend - nstart;
                if (internal_trace)
                    fprintf(stderr, "to %d, %d.\n",
                            vht[vht_index]->vstart,
                            vht[vht_index]->vlen);
                //
                //        And do the same for mstart/mlen (match start/length)
                ostart = vht[vht_index]->mstart;
                oend = ostart + vht[vht_index]->mlen;
                nstart = crm_mangle_offset(ostart, loc, delta, 0);
                nend = crm_mangle_offset(oend, loc, delta, 1);
                if (internal_trace)
                    fprintf(stderr, "\n index %d mstart/mlen upd: %d, %d  ",
                            vht_index,
                            vht[vht_index]->mstart, vht[vht_index]->mlen);
                vht[vht_index]->mstart = nstart;
                vht[vht_index]->mlen = nend - nstart;
                if (internal_trace)
                    fprintf(stderr, "to %d, %d.\n",
                            vht[vht_index]->mstart,
                            vht[vht_index]->mlen);
            }
            //      Don't forget entries that may be varNAMES, not just
            //      var values!
            if (vht[vht_index]->nametxt == text)
            {
                int orig_len;
                //
                //    Same thing here...
                //
                ostart = nstart = vht[vht_index]->nstart;
                orig_len = vht[vht_index]->nlen;
                oend = nend = ostart + orig_len;
                if (orig_len == 0)
                    fprintf(stderr, "CRUD on %d", vht_index);
                nstart = crm_mangle_offset(ostart, loc, delta, 0);
                nend = crm_mangle_offset(oend, loc, delta, 1);
                if (oend - ostart != orig_len)
                    fprintf(stderr, "Length change on %d!  Was %d, now %d ",
                            vht_index,
                            orig_len,
                            oend - ostart);

                if (internal_trace)
                    fprintf(stderr,
                            "\n      index %d nstart/nlen upd: %d, %d  ",
                            vht_index,
                            vht[vht_index]->nstart, vht[vht_index]->nlen);
                vht[vht_index]->nstart = nstart;
                vht[vht_index]->nlen = nend - nstart;
                if (internal_trace)
                    fprintf(stderr, "to %d, %d.\n",
                            vht[vht_index]->nstart,
                            vht[vht_index]->nlen);
            }
        }
    }
    if (internal_trace)
        fprintf(stderr, "\n    end of updates\n");
}

//
//      How to calculate the new offsets of the start and end
//      (that is, a "mark"), given a location (dot) and a delta of that
//      location.  Dot doesn't move... only mark does.
//
//      sl is Start v. End - do we treat this mangle as altering the
//      _start_ of a var, or the _end_ ?  (this is because we don't move
//      a Start if Dot is the same, but we do move an End.  Alternatively,
//      this is "is "dot considered to be before or after a mark with the
//      same value)
//

int crm_mangle_offset(int mark, int dot, int delta, int sl)
{
    int absdelta;

    absdelta = delta;
    if (absdelta < 0)
        absdelta = -absdelta;

    if (sl == 0)
    {
        //     HOW WE DEAL WITH START POINTS
        //     (that is, "dot" is considered to follow "mark")
        //
        //   are we earlier than dot?  If so, we can't be changed by dot.
        //
        // edge condition for start:
        //
        //         Mark   ==>     Mark
        //         Dot             Dot
        //
        if (mark <= dot)
            return mark;

        //   are we beyond the reach of dot and delta?  If so, we just slide.
        //
        // edge condition:
        //
        //         Mark   ==>        Mark
        //         Dot+Delta        Dot
        //

        if ((dot + absdelta) < mark)
            return mark + delta;

        //   Neither - we're in the range where dot and mark can affect us
        //
        //   If delta is positive, we can just slide further out.
        if (delta > 0)
            return mark + delta;

        //
        //   but, if delta is negative (a deletion) then we can move toward
        //   dot, but not earlier than dot.
        mark = mark + delta; //  delta is negative, so we ADD it to subtract!
        if (mark < dot)
            mark = dot;
        return mark;
    }
    else
    {
        //     HOW WE DEAL WITH END POINTS
        //     (that is, "dot" is considered to be in front of "mark")
        //
        //   are we earlier than dot?  If so, we can't be changed by dot.
        //
        // edge condition for finish points:
        //
        //         Mark   ==>     Mark
        //         Dot           Dot
        //
        if (mark < dot)
            return mark;

        //   are we beyond the reach of dot and delta?  If so, we just slide.
        //
        // edge condition:
        //
        //         Mark   ==>        Mark
        //         Dot+Delta          Dot
        //

        if ((dot + absdelta) <= mark)
            return mark + delta;

        //   Neither - we're in the range where dot and mark can affect us
        //
        //   If delta is positive, we can just slide further out.
        if (delta > 0)
            return mark + delta;

        //
        //   but, if delta is negative (a deletion) then we can move toward
        //   dot, but not earlier than dot.
        mark = mark + delta; //  delta is negative, so we ADD it to subtract!
        if (mark < dot)
            mark = dot;
        return mark;
    }
}

//
//
//     crm_buffer_gc - garbage-collect a buffer.  This isn't a perfect
//     solution, but it will work.  (i.e. it's slow and annoying)//
//
//     The algorithm:
//      - find the lowest index currently used (takes 1 pass thru VHT)
//      - find the highest user of that index (takes 1 pass thru VHT)
//        * - see if any block overlaps that block
//      - find the next lowest starting block
//

int crm_buffer_gc(CSL_CELL *zdw)
{
    fprintf(stderr, "Sorry, GC is not yet implemented");
    exit(EXIT_FAILURE);
    return 0;
}




void free_hash_table(VHT_CELL **vht, size_t vht_size)
{
    size_t i;

    if (!vht)
        return;

    for (i = 0; i < vht_size; i++)
    {
        if (vht[i] == NULL)
            continue;

        free(vht[i]->filename);
        free(vht[i]);
    }
    free(vht);
}

