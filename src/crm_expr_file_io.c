//  crm_file_io.c  - Controllable Regex Mutilator,  version v1.0
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



int crm_expr_input(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    //           Allow input of text from stdin.
    FILE *fp;
    char temp_vars[MAX_PATTERN];
    long tvlen;
    char filename[MAX_FILE_NAME_LEN];
    long fnlen;
    char ifn[MAX_FILE_NAME_LEN];
    char fileoffset[MAX_FILE_NAME_LEN];
    long fileoffsetlen;
    char fileiolen[MAX_FILE_NAME_LEN];
    long fileiolenlen;
    long offset, iolen;
    long vstart;
    long vlen;
    long mc;
    long done;
    long till_eof;
    long use_readline;

    //         a couple of vars to bash upon
    long i, j;

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
    crm_get_pgm_arg(temp_vars, MAX_PATTERN, apb->p1start, apb->p1len);
    tvlen = crm_nexpandvar(temp_vars, apb->p1len,  MAX_PATTERN);

    //     If you think INPUT should read to the data window, uncomment this.
    //
    if (tvlen == 0)
    {
        strcpy(temp_vars, ":_dw:");
        tvlen = strlen(":_dw:");
    }

    if (internal_trace)
        fprintf(stderr, "  inputting to var: >>>%s<<<\n", temp_vars);

    //   and what file to get it from...
    //
    crm_get_pgm_arg(filename, MAX_FILE_NAME_LEN, apb->b1start, apb->b1len);
    crm_nextword(filename, apb->b1len, 0, &i, &j);
    memmove(ifn, &filename[i], j);
    fnlen = crm_nexpandvar(ifn, j, MAX_FILE_NAME_LEN);
    ifn[fnlen] = '\0';
    if (user_trace)
        fprintf(stderr, "  from filename >>>%s<<<\n", ifn);

    //   and what offset we need to do before the I/O...
    //
    offset = 0;
    crm_nextword(filename, apb->b1len, i + j, &i, &j);
    memmove(fileoffset, &filename[i], j);
    fileoffsetlen = crm_qexpandvar(fileoffset, j, MAX_FILE_NAME_LEN, NULL);
    fileoffset[fileoffsetlen] = '\0';
    if (1 != sscanf(fileoffset, "%ld", &offset))
    {
        if (user_trace)
            nonfatalerror("Failed to decode the input expression pre-IO file offset number: ", fileoffset);
    }
    if (user_trace)
        fprintf(stderr, "  pre-IO seek to >>>%s<<< --> %ld \n",
                fileoffset, offset);

    //   and how many bytes to read
    //
    iolen = 0;
    crm_nextword(filename, apb->b1len, i + j, &i, &j);
    memmove(fileiolen, &filename[i], j);
    fileiolenlen = crm_qexpandvar(fileiolen, j, MAX_FILE_NAME_LEN, NULL);
    fileiolen[fileiolenlen] = '\0';
    if (1 != sscanf(fileiolen, "%ld", &iolen))
    {
        if (user_trace)
            nonfatalerror("Failed to decode the input expression number of bytes to read: ", fileiolen);
    }
    if (fileiolenlen == 0 || iolen > data_window_size) iolen = data_window_size;
    if (user_trace)
        fprintf(stderr, "  and maximum length IO of >>>%s<<< --> %ld\n",
                fileiolen, iolen);

    if (user_trace)
        fprintf(stderr, "Opening file %s for file I/O (reading)\n", ifn);

    fp = stdin;
    if (fnlen  > 0 && strncmp("stdin", ifn, 6) != 0)
    {
        fp = fopen(ifn, "rb");
        if (fp == NULL)
        {
            fatalerror(
                "For some reason, I was unable to read-open the file named ",
                filename);
            goto input_no_open_bailout;
        }
    }

    done = 0;
    mc = 0;

    //   get the variable name
    crm_nextword(temp_vars, tvlen, 0,  &vstart, &vlen);

    if (vlen == 0)
    {
        done = 1;
    }
    else
    {
        //        must make a copy of the varname.
        //
        char vname[MAX_VARNAME];
        long ichar = 0;
        CRM_ASSERT(vlen < MAX_VARNAME);
        memmove(vname, &(temp_vars[vstart]), vlen);
        vname[vlen] = '\000';

        //        If we have a seek requested, do an fseek.
        //        (Annoying But True: fseek on stdin does NOT error, it's
        //        silently _ignored_.  Who knew?
        if (fp == stdin && offset != 0)
        {
            nonfatalerror("Hmmm, a file offset on stdin won't do much. ",
                          "I'll ignore it for now. ");
        }
        else
        if (offset != 0)
            if (0 != fseek(fp, offset, SEEK_SET))
            {
                if (errno == EBADF)
                {
                    nonfatalerror("Dang, seems that this file isn't fseek()able: ",
                                  filename);
                }
                else
                {
                    /* [i_a] GROT GROT GROT    fix this by having the errno+errno-string reported too! */
                    nonfatalerror("Dang, seems that this file isn't fseek()able: ",
                                  filename);
                }
            }

        //    are we supposed to use readline?
#ifdef HAVE_LIBREADLINE
        if (use_readline)
        {
            char *chartemp;
            chartemp = readline("");
            if (strlen(chartemp) > data_window_size - 1)
            {
                nonfatalerror("Dang, this line of text is way too long: ",
                              chartemp);
            }
            strncpy(inbuf, chartemp, data_window_size);
            inbuf[data_window_size - 1] = 0;/* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
            free(chartemp);
        }
        else
#endif
        //        Are we in <byline> mode?  If so, read one char at a time.
        if (!till_eof)
        {
            //    grab characters in a loop, terminated by EOF or newline
            ichar = 0;
            if (feof(fp)) clearerr(fp);
            while (!feof(fp)
                   && ichar < (data_window_size >> SYSCALL_WINDOW_RATIO)
                   && (till_eof || (ichar == 0 || inbuf[ichar - 1] != '\n'))
                   && ichar <= iolen)
            {
                inbuf[ichar] = fgetc(fp);
                ichar++;
            }
            if (ichar > 0 && inbuf[ichar] == '\n') ichar--; //   get rid of any present newline
            // [i_a] GROT GROT GROT: how about MAC and PC (CR and CRLF instead of LF as line terminators) */
            inbuf[ichar] = '\000';   // and put a null on the end of it.
        }
        else
        {
            //    Nope, we are in full-block mode, read the whole block in
            //    a single I/O if we can.
            ichar = 0;
            if (feof(fp)) clearerr(fp);            // reset any EOF
            ichar = fread(inbuf, 1, iolen, fp);    // do a block I/O
            inbuf[ichar] = '\000';                 // null at the end
        }
        crm_set_temp_nvar(vname, inbuf, ichar);
    }

    //     and close the input file if it's not stdin.
    if (fp != stdin) fclose(fp);

input_no_open_bailout:
    return 0;
}

//////////////////////////////////////////
//
//        And here's where we do output

int crm_expr_output(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    long i, j;

    //    output a string, usually to stdout unless otherwise
    //    specified in the output statement.
    //
    //    We do variable substitutions here....
    //  char outfile[MAX_VARNAME];
    //long outfilelen;
    //char ofn[MAX_VARNAME];
    long outtextlen;
    FILE *outf;

    char filename[MAX_FILE_NAME_LEN];
    char fnam[MAX_FILE_NAME_LEN];
    long fnlen;
    char fileoffset[MAX_FILE_NAME_LEN];
    long fileoffsetlen;
    char fileiolen[MAX_FILE_NAME_LEN];
    long fileiolenlen;
    long offset, iolen;


    if (user_trace)
        fprintf(stderr, " Executing an OUTPUT statement\n");

    //    get the output file name
    //
    //   What file name?
    //
    CRM_ASSERT(apb != NULL);
    crm_get_pgm_arg(filename, MAX_FILE_NAME_LEN, apb->b1start, apb->b1len);
    crm_nextword(filename, apb->b1len, 0, &i, &j);
    memmove(fnam, &filename[i], j);
    fnlen = crm_nexpandvar(fnam, j, MAX_FILE_NAME_LEN);
    fnam[fnlen] = '\0';
    if (user_trace)
        fprintf(stderr, "  filename >>>%s<<<\n", fnam);

    //   and what offset we need to do before the I/O...
    //
    offset = 0;
    crm_nextword(filename, apb->b1len, i + j, &i, &j);
    memmove(fileoffset, &filename[i], j);
    fileoffsetlen = crm_qexpandvar(fileoffset, j, MAX_FILE_NAME_LEN, NULL);
    fileoffset[fileoffsetlen] = '\0';
    if (1 != sscanf(fileoffset, "%ld", &offset))
    {
        if (user_trace)
            nonfatalerror("Failed to decode the output expression pre-IO file offset number: ", fileoffset);
    }
    if (user_trace)
        fprintf(stderr, "  pre-IO seek to >>>%s<<< --> %ld \n",
                fileoffset, offset);

    //   and how many bytes to read
    //
    iolen = 0;
    crm_nextword(filename, apb->b1len, i + j, &i, &j);
    memmove(fileiolen, &filename[i], j);
    fileiolenlen = crm_qexpandvar(fileiolen, j, MAX_FILE_NAME_LEN, NULL);
    fileiolen[fileiolenlen] = '\0';
    if (1 != sscanf(fileiolen, "%ld", &iolen))
    {
        if (user_trace)
            nonfatalerror("Failed to decode the output expression number of bytes to read: ", fileiolen);
    }
    if (fileiolenlen == 0 || iolen > data_window_size) iolen = data_window_size;
    if (user_trace)
        fprintf(stderr, "  and maximum length IO of >>>%s<<< --> %ld\n",
                fileiolen, iolen);


    outf = stdout;
    if (fnlen > 0)
    {
        if (user_trace)
            fprintf(stderr, "Opening file %s for I/O (writing)\n", fnam);
        if (strcmp(fnam, "stderr") == 0)
        {
            outf = stderr;
        }
        else if (strcmp(fnam, "stdout") != 0)
        {
            if (apb->sflags & CRM_APPEND
                || fileoffsetlen > 0)
            {
                outf = fopen(fnam, "r+b");
                //
                //    If the file didn't exist already, that open would fail.
                //     so we retry with "w+".
                if (!outf) outf = fopen(fnam, "w+b");
                //
                //     And make sure the file pointer is at EOF.
                if (outf != 0)
                    (void)fseek(outf, 0, SEEK_END);
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
        crm_get_pgm_arg(outbuf, data_window_size,
                        apb->s1start, apb->s1len);
        outtextlen = apb->s1len;
        if (internal_trace)
            fprintf(stderr, "  outputting with pattern %s\n", outbuf);

        //      Do variable substitution on outbuf.
        outtextlen = crm_nexpandvar(outbuf, outtextlen, data_window_size);

        //      Do the seek if necessary
        //
        if (fileoffsetlen > 0)
        {
            //      fprintf (stderr, "SEEKING to %ld\n", offset);
            rewind(outf);
            (void)fseek(outf, offset, SEEK_SET);
        }

        //      Write at most iolen bytes
        //
        if (fileiolenlen > 0)
            if (iolen < outtextlen)
                outtextlen = iolen;

        //   and send it to outf
        //
        fwrite(outbuf, outtextlen, 1, outf);
        fflush(outf);
        if (outf != stdout && outf != stderr) fclose(outf);
    }

    return 0;
}

