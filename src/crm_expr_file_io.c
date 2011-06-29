//  crm_file_io.c  - Controllable Regex Mutilator,  version v1.0
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



int crm_expr_input(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //           Allow input of text from stdin.
    FILE *fp = NULL;
    char temp_vars[MAX_PATTERN];
    int tvlen;
    int tvstart;
    char filename[MAX_FILE_NAME_LEN];
    int fnlen;
    char ifn[MAX_FILE_NAME_LEN];
    char fileoffset[MAX_FILE_NAME_LEN];
    int fileoffsetlen;
    char fileiolen[MAX_FILE_NAME_LEN];
    int fileiolenlen;
    int offset, iolen;
    //int vstart;
    //int vlen;
    int done;
    int till_eof;
    int use_readline;
    int file_was_fopened;
    int filesectlen;

    //         a couple of vars to bash upon
    int i, j;

    if (user_trace)
        fprintf(stderr, "executing an INPUT statement\n");

    //    set up the flags (if any)
    //
    CRM_ASSERT(apb != NULL);
    till_eof = 1;
    if (apb->sflags & CRM_BYLINE)
    {
        till_eof = 0;
        if (user_trace)
            fprintf(stderr, " reading one line mode\n");
    }

    use_readline = 0;
    if (apb->sflags & CRM_READLINE)
    {
        use_readline = 1;
        if (user_trace)
            fprintf(stderr, " Using READLINE input line editing\n");
    }

    //    get the list of variable names
    //
    tvlen = crm_get_pgm_arg(temp_vars, MAX_PATTERN, apb->p1start, apb->p1len);
    tvlen = crm_nexpandvar(temp_vars, tvlen, MAX_PATTERN, vht, tdw);
    CRM_ASSERT(tvlen < MAX_PATTERN);

    if (!crm_nextword(temp_vars, tvlen, 0, &tvstart, &tvlen))
    {
        // not a valid variable found in the arg.
        strcpy(temp_vars, ":_dw:");
        tvlen = (int)strlen(":_dw:");
        tvstart = 0;
    }
    else if (tvlen < 2)
    {
        nonfatalerror("The variable you're asking me to load the INPUT into has an utterly bogus name. ",
                "So I'll ignore the whole statement.");
        return 0;
    }


    if (internal_trace)
    {
        fprintf(stderr, "  inputting to var (len=%d): >>>%.*s<<<\n", tvlen, tvlen, temp_vars + tvstart);
    }

    //   and what file to get it from...
    //
    filesectlen = crm_get_pgm_arg(filename, MAX_FILE_NAME_LEN, apb->b1start, apb->b1len);
    if (crm_nextword(filename, filesectlen, 0, &i, &j))
    {
        if (j >= MAX_FILE_NAME_LEN)
        {
            nonfatalerror_ex(SRC_LOC(), "INPUT statement comes with a filename which is too long (len = %d) "
                                        "while the maximum allowed size is %d.",
                    j,
                    MAX_FILE_NAME_LEN - 1);
            return -1;
        }
        memmove(ifn, &filename[i], j);
        fnlen = crm_nexpandvar(ifn, j, MAX_FILE_NAME_LEN, vht, tdw);
        CRM_ASSERT(fnlen < MAX_FILE_NAME_LEN);
        ifn[fnlen] = 0;
        if (user_trace)
            fprintf(stderr, "  from filename >>>%s<<<\n", ifn);
    }
    else
    {
        fnlen = 0;
        if (user_trace)
            fprintf(stderr, "  from default: stdin\n");
    }


    //   and what offset we need to do before the I/O...
    //
    offset = 0;
    fileoffset[0] = 0;
    fileoffsetlen = 0;
    if (crm_nextword(filename, filesectlen, i + j, &i, &j))
    {
        if (j >= MAX_FILE_NAME_LEN)
        {
            nonfatalerror_ex(SRC_LOC(), "INPUT statement comes with a fileoffset expression which is too long (len = %d) "
                                        "while the maximum allowed size is %d.",
                    j,
                    MAX_FILE_NAME_LEN - 1);
            return -1;
        }
        memmove(fileoffset, &filename[i], j);
        fileoffsetlen = crm_qexpandvar(fileoffset, j, MAX_FILE_NAME_LEN, NULL, vht, tdw);
        fileoffset[fileoffsetlen] = 0;
        if (1 != sscanf(fileoffset, "%d", &offset))
        {
            nonfatalerror_ex(SRC_LOC(), "Failed to decode the input expression pre-IO file offset number '%s'.",
                    fileoffset);
        }
        if (user_trace)
        {
            fprintf(stderr, "  pre-IO seek to >>>%s<<< --> %d \n",
                    fileoffset, offset);
        }
    }
    // else default: 0

    //   and how many bytes to read
    //
    iolen = 0;
    if (crm_nextword(filename, filesectlen, i + j, &i, &j))
    {
        if (j >= MAX_FILE_NAME_LEN)
        {
            nonfatalerror_ex(SRC_LOC(), "INPUT statement comes with a length expression which is too long (len = %d) "
                                        "while the maximum allowed size is %d.",
                    j,
                    MAX_FILE_NAME_LEN - 1);
            return -1;
        }
        memmove(fileiolen, &filename[i], j);
        fileiolenlen = crm_qexpandvar(fileiolen, j, MAX_FILE_NAME_LEN, NULL, vht, tdw);
        fileiolen[fileiolenlen] = 0;
        CRM_ASSERT(*fileiolen != 0);
        if (1 != sscanf(fileiolen, "%d", &iolen))
        {
            nonfatalerror_ex(SRC_LOC(), "Failed to decode the input expression number of bytes to read: '%s'", fileiolen);
        }
        if (iolen < 0)
            iolen = 0;
        else if (iolen > data_window_size)
            iolen = data_window_size;
        if (user_trace)
            fprintf(stderr, "  and maximum length IO of >>>%s<<< --> %d\n",
                    fileiolen, iolen);
    }
    else
    {
        // default:
        iolen = data_window_size;
    }

    // [i_a] GROT GROT GROT: no checks if there's any cruft beyond the third param! :-(

    if (user_trace)
        fprintf(stderr, "Opening file %s for file I/O (reading)\n", ifn);

    fp = stdin;
    file_was_fopened = 0;
    if (fnlen > 0)
    {
        if (strcmp(ifn, "stdin") == 0
            || strcmp(ifn, "/dev/stdin") == 0
            || strcmp(ifn, "CON:") == 0
            || strcmp(ifn, "/dev/tty") == 0)
        {
            fp = os_stdin();
            file_was_fopened = 0;
        }
        else
        {
            file_was_fopened = 1;
            fp = fopen(ifn, "rb");
            if (fp == NULL)
            {
                fatalerror_ex(SRC_LOC(),
                        "For some reason, I was unable to read-open the file named '%s' (expanded from '%s'): error = %d(%s)",
                        ifn,
                        filename,
                        errno,
                        errno_descr(errno));
                goto input_no_open_bailout;
            }
        }
    }

    if (user_trace)
    {
        struct stat st = { 0 };
        int stret;

        stret = fstat(fileno(fp), &st);
        fprintf(stderr, "Opened file '%s' for file I/O (reading): handle %d (%s) (%d) - "
                        "stat.stret = %ld, "
                        "stat.st_dev = %ld, "
                        "stat.st_ino = %ld, "
                        "stat.st_mode = %ld, "
                        "stat.st_nlink = %ld, "
                        "stat.st_uid = %ld, "
                        "stat.st_gid = %ld, "
                        "stat.st_rdev = %ld, "
                        "stat.st_size = %ld, "
                        "stat.st_atime = %ld, "
                        "stat.st_mtime = %ld, "
                        "stat.st_ctim = %ld"
                        "\n",
                ifn,
                fileno(fp),
                (fp == os_stdin() ? "! stdin !" : "FILE"),
                file_was_fopened,
                (long)stret,
                (long)st.st_dev,
                (long)st.st_ino,
                (long)st.st_mode,
                (long)st.st_nlink,
                (long)st.st_uid,
                (long)st.st_gid,
                (long)st.st_rdev,
                (long)st.st_size,
                (long)st.st_atime,
                (long)st.st_mtime,
                (long)st.st_ctime
               );
    }

    done = 0;

#if 0
    if (!crm_nextword(temp_vars, tvlen, 0,  &vstart, &vlen) || vlen == 0)
    {
        done = 1;
    }
    else
#endif
    {
        //        must make a copy of the varname.
        //
        char vname[MAX_VARNAME];
        int ichar = 0;
        CRM_ASSERT(tvlen < MAX_VARNAME);
        memmove(vname, &(temp_vars[tvstart]), tvlen);
        vname[tvlen] = 0;

        //        If we have a seek requested, do an fseek.
        //        (Annoying But True: fseek on stdin does NOT error, it's
        //        silently _ignored_.  Who knew?
        if ((!file_was_fopened /* fp == stdin  -- hm, what to do here... */
             || isatty(fileno(fp))) && offset != 0)
        {
            nonfatalerror("Hmmm, a file offset on stdin or tty won't do much. ",
                    "I'll ignore it for now.");
        }
        else if (offset != 0)
        {
            if (0 != fseek(fp, offset, SEEK_SET))
            {
                if (errno == EBADF)
                {
                    nonfatalerror_ex(SRC_LOC(), "Dang, seems that this file '%s' isn't fseek()able!",
                            filename);
                }
                else
                {
                    nonfatalerror_ex(SRC_LOC(),
                            "Dang, seems that this file '%s' isn't fseek()able: error = %d(%s)",
                            filename,
                            errno,
                            errno_descr(errno));
                }
            }
        }

        //    are we supposed to use readline?
#ifdef HAVE_LIBREADLINE
        if (use_readline && is_stdin_or_null(fp))
        {
            char *chartemp;
            chartemp = readline("");
            if (!chartemp)
            {
                chartemp = strdup("");        // see the UNIX man page: readline() MAY return NULL!
            }
            if (strlen(chartemp) > data_window_size - 1)
            {
                nonfatalerror("Dang, this line of text is way too long: ",
                        chartemp);
            }
            strncpy(inbuf, chartemp, data_window_size);
            inbuf[data_window_size - 1] = 0; /* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
            free(chartemp);
        }
#else
        if (use_readline && is_stdin_or_null(fp))
        {
            fgets(inbuf, data_window_size - 1, (fp ? fp : stdin));
            inbuf[data_window_size - 1] = 0;
        }
#endif
        else
        {
            //        Are we in <byline> mode?  If so, read one char at a time.
            if (!till_eof)
            {
                //    grab characters in a loop, terminated by EOF or newline
                ichar = 0;
                if (feof(fp))
                    clearerr(fp);
                while (!feof(fp)
                       && ichar < (data_window_size >> SYSCALL_WINDOW_RATIO)
                       // [i_a] how about MAC and PC (CR and CRLF instead of LF as line terminators)? Quick fix here: */
                       && (till_eof || (ichar == 0 || (inbuf[ichar - 1] != '\r' && inbuf[ichar - 1] != '\n')))
                       && ichar <= iolen)
                {
                    int c = fgetc(fp);
                    if (c != EOF)
                    {
                        inbuf[ichar] = c;
                        ichar++;
                    }
                }
                if (ichar > 0 && inbuf[ichar] == '\n')
                    ichar--; //   get rid of any present newline
                // [i_a] how about MAC and PC (CR and CRLF instead of LF as line terminators)? Quick fix here: */
                if (ichar > 0 && inbuf[ichar] == '\r')
                    ichar--;      //   get rid of any present carriage return too (CRLF for MSDOS)
                inbuf[ichar] = 0; // and put a null on the end of it.
            }
            else
            {
                //    Nope, we are in full-block mode, read the whole block in
                //    a single I/O if we can.
                ichar = 0;
                if (feof(fp))
                    clearerr(fp);                        // reset any EOF
                ichar = (int)fread(inbuf, 1, iolen, fp); // do a block I/O
                if (ferror(fp))
                {
                    //     and close the input file if it's not stdin.
                    if (file_was_fopened)
                    {
                        fclose(fp);
                        fp = NULL;
                        file_was_fopened = 0;
                    }

                    fatalerror_ex(SRC_LOC(),
                            "For some reason, I got an error while trying to read data from the file named '%s' (expanded from '%s'): error = %d(%s)",
                            ifn,
                            filename,
                            errno,
                            errno_descr(errno));
                    goto input_no_open_bailout;
                }
                inbuf[ichar] = 0;                   // null at the end
            }
        }
        crm_set_temp_nvar(vname, inbuf, ichar);
    }

    //     and close the input file if it's not stdin.
    if (file_was_fopened)
    {
        fclose(fp);
    }

input_no_open_bailout:
    return 0;
}

//////////////////////////////////////////
//
//        And here's where we do output

int crm_expr_output(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    int i, j;

    //    output a string, usually to stdout unless otherwise
    //    specified in the output statement.
    //
    //    We do variable substitutions here....
    //  char outfile[MAX_VARNAME];
    //int outfilelen;
    //char ofn[MAX_VARNAME];
    int outtextlen;
    FILE *outf;

    char filename[MAX_FILE_NAME_LEN];
    char fnam[MAX_FILE_NAME_LEN];
    int fnlen;
    char fileoffset[MAX_FILE_NAME_LEN];
    int fileoffsetlen;
    char fileiolen[MAX_FILE_NAME_LEN];
    int fileiolenlen;
    int offset, iolen;
    int file_was_fopened;
    int filesectlen;



    if (user_trace)
        fprintf(stderr, " Executing an OUTPUT statement\n");

    //    get the output file name
    //
    //   What file name?
    //

    fnam[0] = 0;
    fnlen = 0;

    offset = 0;
    fileoffset[0] = 0;
    fileoffsetlen = 0;

    iolen = 0;
    fileiolen[0] = 0;
    fileiolenlen = 0;

    CRM_ASSERT(apb != NULL);
    filesectlen = crm_get_pgm_arg(filename, MAX_FILE_NAME_LEN, apb->b1start, apb->b1len);
    if (crm_nextword(filename, filesectlen, 0, &i, &j))
    {
        if (j >= MAX_FILE_NAME_LEN)
        {
            nonfatalerror_ex(SRC_LOC(), "OUTPUT statement comes with a filename which is too long (len = %d) "
                                        "while the maximum allowed size is %d.",
                    j,
                    MAX_FILE_NAME_LEN - 1);
            return -1;
        }
        memmove(fnam, &filename[i], j);
        fnlen = crm_nexpandvar(fnam, j, MAX_FILE_NAME_LEN, vht, tdw);
        CRM_ASSERT(fnlen < MAX_FILE_NAME_LEN);
        fnam[fnlen] = 0;
        if (user_trace)
            fprintf(stderr, "  filename >>>%s<<<\n", fnam);

        //   and what offset we need to do before the I/O...
        //
        if (crm_nextword(filename, filesectlen, i + j, &i, &j))
        {
            if (j >= MAX_FILE_NAME_LEN)
            {
                nonfatalerror_ex(SRC_LOC(), "OUTPUT statement comes with a offset expression which is too long (len = %d) "
                                            "while the maximum allowed size is %d.",
                        j,
                        MAX_FILE_NAME_LEN - 1);
                return -1;
            }
            memmove(fileoffset, &filename[i], j);
            fileoffsetlen = crm_qexpandvar(fileoffset, j, MAX_FILE_NAME_LEN, NULL, vht, tdw);
            fileoffset[fileoffsetlen] = 0;
            if (*fileoffset && 1 != sscanf(fileoffset, "%d", &offset))
            {
                return nonfatalerror("Failed to decode the output expression pre-IO file offset number: ",
                        fileoffset);
            }
            if (user_trace)
            {
                fprintf(stderr, "  pre-IO seek to >>>%s<<< --> %d\n",
                        fileoffset, offset);
            }

            //   and how many bytes to read
            //
            if (crm_nextword(filename, filesectlen, i + j, &i, &j))
            {
                if (j >= MAX_FILE_NAME_LEN)
                {
                    nonfatalerror_ex(SRC_LOC(), "OUTPUT statement comes with a length expression which is too long (len = %d) "
                                                "while the maximum allowed size is %d.",
                            j,
                            MAX_FILE_NAME_LEN - 1);
                    return -1;
                }
                memmove(fileiolen, &filename[i], j);
                fileiolenlen = crm_qexpandvar(fileiolen, j, MAX_FILE_NAME_LEN, NULL, vht, tdw);
                fileiolen[fileiolenlen] = 0;
                if (*fileiolen && 1 != sscanf(fileiolen, "%d", &iolen))
                {
                    return nonfatalerror("Failed to decode the output expression number of bytes to read: ", fileiolen);
                }
                if (iolen < 0)
                    iolen = 0;
                else if (! * fileiolen || iolen > data_window_size)
                    iolen = data_window_size;
                if (user_trace)
                {
                    fprintf(stderr, "  and maximum length IO of >>>%s<<< --> %d\n",
                            fileiolen, iolen);
                }
            }
        }
    }

    // [i_a] GROT GROT GROT  and of course here too, there's no check what-so-ever to see
    // if there's any cruft in the parameter beyond the third item above.

    outf = stdout;
    file_was_fopened = 0;
    if (fnlen > 0)
    {
        if (user_trace)
            fprintf(stderr, "Opening file %s for I/O (writing)\n", fnam);
        if (strcmp(fnam, "stderr") == 0
            || strcmp(fnam, "/dev/stderr") == 0)
        {
            outf = stderr; // [i_a] intentional: append to our possibly redirected stderr FILE handle, don't close it as we go on using it!
            file_was_fopened = 0;
        }
        else if (strcmp(fnam, "stdout") == 0
                 || strcmp(fnam, "con:") == 0
                 || strcmp(fnam, "/dev/tty") == 0
                 || strcmp(fnam, "/dev/stdout") == 0)
        {
            outf = stdout; // [i_a] intentional: append to our possibly redirected stdout FILE handle, don't close it as we go on using it!
            file_was_fopened = 0;
        }
        else
        {
            file_was_fopened = 1;
            if (apb->sflags & CRM_APPEND
                || fileoffsetlen > 0)
            {
                outf = fopen(fnam, "rb+");
                //
                //    If the file didn't exist already, that open would fail.
                //     so we retry with "w+".
                if (!outf)
                    outf = fopen(fnam, "wb+");
                //
                //     And make sure the file pointer is at EOF.
                if (outf != 0)
                {
                    (void)fseek(outf, 0, SEEK_END);
                }
            }
            else
            {
                outf = fopen(fnam, "wb");
            }
        }
    }

    //
    //   could we open the file?
    //
    if (outf == 0)
    {
        fatalerror("For some reason, I was unable to write-open the file named",
                fnam);
    }
    else
    {
        //   Yep, file is open, go for the writing.
        //
        outtextlen = crm_get_pgm_arg(outbuf, data_window_size, apb->s1start, apb->s1len);
        if (internal_trace)
            fprintf(stderr, "  outputting with pattern (len = %d) %.*s\n", outtextlen, outtextlen, outbuf);
        //      Do variable substitution on outbuf.
        outtextlen = crm_nexpandvar(outbuf, outtextlen, data_window_size, vht, tdw);

        //      Do the seek if necessary
        //
        CRM_ASSERT(fileoffsetlen >= 0);
        if (fileoffsetlen > 0)
        {
            if ((!file_was_fopened || isatty(fileno(outf))) && offset != 0)
            {
                nonfatalerror("Hmmm, a file offset on stdout/stderr or tty won't do much. ",
                        "I'll ignore it for now. ");
            }
            else
            {
                //      fprintf(stderr, "SEEKING to %d\n", offset);
                rewind(outf);
                (void)fseek(outf, offset, SEEK_SET);
            }
        }

        //      Write at most iolen bytes
        //
        if (fileiolenlen > 0)
        {
            if (iolen < outtextlen)
                outtextlen = iolen;
        }

        //   and send it to outf
        //
        if (outtextlen > 0)
        {
            int ret = fwrite4stdio(outbuf, outtextlen, outf);
            if (ret != outtextlen)
            {
                fatalerror_ex(SRC_LOC(),
                        "Could not write %d bytes to file '%s': "
                        "errno = %d(%s)\n",
                        outtextlen,
                        fnam,
                        errno,
                        errno_descr(errno));
            }
        }
        // [i_a] not needed to flush each time if the output is not stdout/stderr: this is faster
        if (isatty(fileno(outf)))
        {
            CRM_ASSERT(!file_was_fopened);
            fflush(outf);
        }
        if (file_was_fopened)
        {
            fclose(outf);
        }
    }

    return 0;
}

