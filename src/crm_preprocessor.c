//  crm_preprocessor.c  - Controllable Regex Mutilator,  version v1.0
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




static void print_code_with_linenos(FILE *f, const char *str, int len)
{
    int i;
    int line;

    if (len < 0)
        len = (int)strlen(str);

    for (i = 0, line = 1; i < len;)
    {
        int e = (int)strcspn(str + i, "\n");

        fprintf(f, "%4d: %.*s\n", line++, e, str + i);

        i += e;
        if (str[i])
        {
            i++;
        }
    }
}



//      crm preprocessor - pre-process a CRM file to make it
//      palatable to the sorry excuse we have for a compiler.
int crm_preprocessor(CSL_CELL *csl, int flags)
{
    int lflag;
    int status;
    int i, j;
    int done;
    regex_t preg;
    int numinserts = 0;
    int maxinserts = DEFAULT_MAX_INSERTS;
    regmatch_t matches[3];

    //  regex commentary:
    //    we want to match both
    //      "\n[ ]*insert[ ]*[file][ ]*\n"
    //    and
    //      "^[ ]*insert[ ]*[file][ ]*\n"
    //
    //    This is the file insertion regex.  Note that it does NOT allow
    //    spaces in filenames, nor does it deal with embedded #comments
    //    but then again, the "fixes" to deal with spaces in filenames
    //    also don't deal wth embedded #comments, because #comments
    //    themselves aren't dealt with till lower down in the code..
    //
    //    However, there's another problem with the above.  The trailing
    //    newline may not be there - consider:
    //
    //      #insert foo.crm ; output /hello, world!\n/
    //
    //    which fails because we aren't regex_conforming.
    //    So, what we really need is to grab the next nonblank token, then
    //    either get a newline or a semicolon.

    char *insert_regex =
        "\n[[:blank:]]*(insert)[[:blank:]]+([[:graph:]]+)[[:blank:]]*[\n;]";

    if (internal_trace)
{
        fprintf(stderr, " preprocessor - #insert processing...\n");
}

    lflag = 0;
    i = 0;
    done = 0;

    //
    //     Compile the insert regex
    //
    i = crm_regcomp(&preg,
            insert_regex, (int)strlen(insert_regex),
            REG_EXTENDED | REG_ICASE | REG_NEWLINE);
    if (i != 0)
    {
        crm_regerror(i, &preg, tempbuf, data_window_size);
        untrappableerror(
                "Regular Expression Compilation Problem during INSERT processing:",
                tempbuf);
    }

    //
    //    Do the initial breaking pass
    //
    crm_break_statements(0, csl->nchars, csl);

    if (internal_trace)
    {
        fprintf(stderr,
                "After first pass, breaking statements we have -->>\n");
        print_code_with_linenos(stderr, csl->filetext, csl->nchars);
        fprintf(stderr,
                "<<--\nlength %d\n",
                csl->nchars);
    }

    while (!done)
    {
        int filenamelen;

        j  = crm_regexec(&preg, csl->filetext, csl->nchars,
                WIDTHOF(matches), matches, lflag, NULL);
        if (j != 0)
        {
            if (internal_trace)
                fprintf(stderr, "No insert files left to do.\n");
            done = 1;
        }
        else
        {
            char insertfilename[MAX_FILE_NAME_LEN];
            struct stat statbuf;
            char dirbuf[DIRBUFSIZE_MAX + 1];

            filenamelen = matches[2].rm_eo - matches[2].rm_so;
            if (filenamelen >= MAX_FILE_NAME_LEN)
            {
                untrappableerror_ex(SRC_LOC(), "PREPROCESSOR: specified script filename '%.*s' (len: %d) "
                                               "is too long; only filenames/paths up to %d characters are allowed!",
                        filenamelen, &csl->filetext[matches[2].rm_so],
                        filenamelen,
                        MAX_FILE_NAME_LEN - 1);
            }
            CRM_ASSERT(filenamelen < MAX_FILE_NAME_LEN);
            memmove(insertfilename, &csl->filetext[matches[2].rm_so], filenamelen);
            insertfilename[filenamelen] = 0;

            //   Check to see if this was a "delimited" insertfile name
            //   that is, wrapped in [filename] rather than plaintext -
            //   if it is, then do a variable expansion on it.
            if (insertfilename[0] == '['
                && insertfilename[filenamelen - 1] == ']')
            {
                if (user_trace)
                {
                    fprintf(stderr, "INSERT filename expand: '%s'",
                            insertfilename);
                }
                //  Get rid of the enclosing [ and ]
                filenamelen -= 2;
                memmove(insertfilename, insertfilename + 1, filenamelen);
                insertfilename[filenamelen] = 0;
                filenamelen = crm_nexpandvar(insertfilename,
                        filenamelen,
                        MAX_FILE_NAME_LEN, vht, tdw);
                insertfilename[filenamelen] = 0;
                if (user_trace)
				{
                    fprintf(stderr, " to '%s'\n", insertfilename);
            }
			}

            //   We have a filename; check to see if it will blow the
            //   gaskets on the filesystem or not:
            //
            if (filenamelen > MAX_FILE_NAME_LEN - 1)
            {
                untrappableerror("INSERT Filename was too long!  Here's the"
                                 "first part of it: ", insertfilename);
            }

#if 0
            //   To keep the matcher from looping, we change the string
            //   'insert' to 'insert=' .  Cool, eh?
            //
            csl->filetext[matches[1].rm_eo] = '=';            // smash in an "="
#else
            //   To keep the matcher from looping, we change the string
            //   'insert filename' to '#nsert filena\#'.  Cool, eh?
            //
            csl->filetext[matches[1].rm_so] = '#';                // smash in a "#"
            csl->filetext[matches[2].rm_eo - 1] = '#';            // smash in a "#"
            csl->filetext[matches[2].rm_eo - 2] = '\\';           // smash in a "\"
#endif

                   if (!mk_absolute_path(dirbuf, WIDTHOF(dirbuf), insertfilename))
{
return -1;
}
            //   stat the file - if 0, file exists
            status = stat(dirbuf, &statbuf);
				if (status)
			{
                if (user_trace)
                {
                    fprintf(stderr, "INSERT file '%s' does not exist. Checking if it's a relative path ('%s') to parent '%s'.\n", dirbuf, insertfilename, csl->filename);
                }

				// if file does NOT exist as-is, see if it's a relative path an retry using the path
				// to the current active file, i.e. the one containing this insert statement...
				if ( /* UNIX */ insertfilename[0] != '/'
					&& /* Windows */ !((insertfilename[1] == ':' && insertfilename[2] == '\\')
										|| insertfilename[0] == '\\')
					&& csl->filename)	
				{
		            char nf[CRM_MAX(MAX_FILE_NAME_LEN, DIRBUFSIZE_MAX) + 1];
					const char *sp;

                    if (!mk_absolute_path(nf, WIDTHOF(nf), csl->filename))
{
return -1;
}
                if (internal_trace)
                {
                    fprintf(stderr, "Retry: expand parent dir '%s' to abs path '%s'\n", csl->filename, nf);
                }
					sp = skip_path(nf);
					if (sp && sp != nf)
					{
						size_t len = CRM_MIN(WIDTHOF(nf), (sp - nf));

						strncpy(nf + len, insertfilename, WIDTHOF(nf) - len);
						nf[WIDTHOF(nf) - 1] = 0;
						strncpy(insertfilename, nf, WIDTHOF(insertfilename));
						insertfilename[WIDTHOF(insertfilename) - 1] = 0;

				   // retry:
	                   if (!mk_absolute_path(dirbuf, WIDTHOF(dirbuf), insertfilename))
{
return -1;
}
                if (user_trace)
                {
                    fprintf(stderr, "Retry: check for existing INSERT file: '%s' (derived from '%s')\n", dirbuf, insertfilename);
                }
						status = stat(dirbuf, &statbuf);
					}
				}
			}
            if (!status)
            {
                int fd;
                int rlen;
                    
                //
                //    OK, now we have to "insert" the file, but we have to
                //    do it gracefully.  In particular, the file itself
                //    must be loaded, then newline-fixupped, then
                //    we know it's actual size and can actually -insert-
                //    it.
                //
                //    We malloc a big chunk of memory, read the file in.
                //    We expand it there (with impunity), then
                //    we make a temporary copy in malloced memory,
                //    and do the real insertion.
                CSL_CELL *ecsl;
                char *insert_buf;
                if (user_trace)
                {
                    fprintf(stderr, "Inserting file '%s'.\n", dirbuf);
                }

                //   Loop prevention check - too many inserts?
                //
                numinserts++;
                if (numinserts > maxinserts)
                {
                    untrappableerror("Too many inserts!  Limit exceeded with "
                                     "filename: ", dirbuf);
                }

                // malloc space only when you know you're not out of insert count bounds anyway.
                ecsl = (CSL_CELL *)calloc(1, sizeof(ecsl[0]));
                insert_buf = calloc(max_pgmsize, sizeof(insert_buf[0]));
                if (!insert_buf || !ecsl)
                {
                    untrappableerror("Couldn't alloc enough memory to do "
                                     "the insert of file ", dirbuf);
                }

                ecsl->filetext = insert_buf;
                ecsl->filetext_allocated = 1;
                ecsl->nchars = 0;
                //   OK, we now have a buffer.  Read the file in...

                fd = open(dirbuf, O_RDONLY | O_BINARY);
                if (fd < 0)
                {
					rlen = 0;
                    untrappableerror("Couldn't open the insert file named: ", dirbuf);
                }
                else
                {
                    /* [i_a] make sure we never overflow the buffer: */
                    if (statbuf.st_size + 2 > max_pgmsize)
                    {
                        statbuf.st_size = max_pgmsize - 2;                         // 2: \n + NUL
                    }

                    rlen = read(fd,
                            ecsl->filetext,
                            statbuf.st_size);

                    close(fd);
                    if (rlen < 0)
                    {
                        rlen = 0;

                        untrappableerror("Cannot read from file (it may be locked?): ", dirbuf);
                    }
                }

                ecsl->nchars = rlen;
                /* [i_a] ^^^^^^^^ not: statbuf.st_size; -- as the MSVC documentation says:
                 * read returns the number of bytes read, which might be less than count
                 * if there are fewer than count bytes left in the file or if the file
                 * was opened in text mode, in which case each carriage return / line feed (CR-LF) pair
                 * is replaced with a single linefeed character. Only the single
                 * linefeed character is counted in the return value.
                 */

                //
                // file's read in, put in a trailing newline.
                // And add a NUL sentinel too when we're at it! But DON'T count that one too!
                //
                ecsl->filetext[ecsl->nchars++] = '\n';
                CRM_ASSERT(ecsl->nchars < max_pgmsize);
                ecsl->filetext[ecsl->nchars] = 0;
                ecsl->filename = strdup(dirbuf);
                // [i_a] ^^^^^^^^^^^ insertfilename / dirbuf will get out of scope soon and we need to keep this around till the end
                if (!ecsl->filename)
                {
                    untrappableerror("Cannot allocate preprocessor memory", "Stick a fork in us; we're _done_.");
                }
                ecsl->filename_allocated = 1;

                //
                //   now do the statement-break thing on this file
                crm_break_statements(0, ecsl->nchars, ecsl);
                //
                //   and we have the expanded text ready to insert.
                //
                //   will it fit?
                //
                if ((csl->nchars + ecsl->nchars + 64) > max_pgmsize)
                {
                    untrappableerror(" Program file buffer overflow when "
                                     "INSERTing file ", dirbuf);
                }

                //   Does the result end with a newline?  If not, fix it.
                if (ecsl->filetext[ecsl->nchars - 1] != '\n')
                {
                    ecsl->filetext[ecsl->nchars] = '\n';
                    ecsl->nchars++;
                    CRM_ASSERT(ecsl->nchars < max_pgmsize);
                    ecsl->filetext[ecsl->nchars] = 0;
                }

                //   Does the result end with two newlines?  Fix
                //   that, too.
                //if (ecsl->filetext[ecsl->nchars-1] == '\n'
                //    && ecsl->filetext[ecsl->nchars-2] == '\n')
                //  {
                //    ecsl->nchars--;
                //  }

                //  Make a hole in the csl->filetext
                //
                //   (note- Fidelis' points out that we need to pace
                //    off from the end of matches[0] so as to not smash
                //    trailing stuff on the line.
                //
                memmove(&(csl->filetext[matches[0].rm_eo + ecsl->nchars]),
                        &(csl->filetext[matches[0].rm_eo]),
                        csl->nchars - matches[0].rm_eo + 1);                 // +1 for 0!
                //
                //   and put the new text into that hole
                //
                memmove(&(csl->filetext[matches[0].rm_eo]),
                        ecsl->filetext,
                        ecsl->nchars);

                //   Mark the new length of the csl text.
                if (internal_trace)
                    fprintf(stderr, "Old length: %d, ", csl->nchars);
                csl->nchars += ecsl->nchars;
                if (internal_trace)
                    fprintf(stderr, "new length: %d\n ", csl->nchars);

                //    Now we clean up (de-malloc all that memory)
                free_stack_item(ecsl);
                //free (ecsl->filetext);
                //free (ecsl);
            }
            else
            {
                //
                //   Paolo's beautiful weasel hack to make missing
                //   insert files a trappable error.  The hitch is that
                //   the error "occurs" at preprocessor time, before the
                //   compiler has a chance to set up the trap addresses.
                //   So, Paolo presented the following waycool weasel hack.
                //   "If the file is missing, 'insert' a FAULT that has the
                //   fault message of "Missing insert file".  So, if we never
                //   actually execute the missing lines, there's no problem,
                //   and if we _do_, we can trap the error or not, as the
                //   the programmer chooses.
                //
                //  untrappableerror("I'm having a problem inserting file ", dirbuf);
                //
                char faulttext[MAX_PATTERN];
                int textlen;

                if (!act_like_Bill)
                {
                    untrappableerror_ex(SRC_LOC(),
                            "Couldn't insert the file named '%s' that you asked for.  "
                            "This is probably a bad thing.", dirbuf);
                }

                if (user_trace)
                {
                    fprintf(stderr, "Can't find '%s' to insert.  "
                                    "Inserting a FAULT instead\n",
                            dirbuf);
                }

                //
                //    Build the fault string.
                snprintf(faulttext, MAX_PATTERN,
                        "\n######  THE NEXT LINE WAS AUTO_INSERTED BECAUSE THE FILE COULDN'T BE FOUND\n"
                        "fault / Couldn't insert the file named '%s' that you asked for.  "
                        "This is probably a bad thing./\n",
                        dirbuf);
                faulttext[MAX_PATTERN - 1] = 0;
                textlen = (int)strlen(faulttext);

                if (csl->nchars + textlen > max_pgmsize)
                {
                    untrappableerror(" Program file buffer overflow when "
                                     "inserting a FAULT (INSERT file not found) message for file ", dirbuf);
                }

                //
                //       make a hole to put the fault string into.
                //
                memmove(&csl->filetext[matches[0].rm_eo + textlen],
                        &csl->filetext[matches[0].rm_eo],
                        csl->nchars - matches[0].rm_eo);
                //
                //   and put the new text into that hole
                //
                memmove(&csl->filetext[matches[0].rm_eo],
                        faulttext,
                        textlen);
                //   Mark the new length of the csl text.
                if (internal_trace)
                    fprintf(stderr, "Added %d chars to crmprogram\n",
                            textlen);
                csl->nchars += textlen;
            }
            // i = matches[1].rm_so + 1;
        }
        if (internal_trace)
        {
            fprintf(stderr, "----------Result after preprocessing-----\n");
            print_code_with_linenos(stderr, csl->filetext, -1);
            fprintf(stderr, "\n-------------end preprocessing------\n");
        }
    }

    //     define a hash of the expanded program for sanity checking on bugreps:
    //
    {
        char myhash[32];
        sprintf(myhash, "%08lX", (unsigned long int)strnhash(csl->filetext, csl->nchars));
        myhash[8] = 0;
        crm_set_temp_var(":_pgm_hash:", myhash, -1);
    }

    ///   GROT GROT GROT  for some reason, Gnu Regex segfaults if it
    //    tries to free this register.
    crm_regfree(&preg);
    //fprintf(stderr, "returning\n");
    return 0;
}

//
//     Set up statement breaks.
//
//     If we're not in a nesting (paren, angle, box, slash) then
//     we need to assure that there are newlines before and after
//     any { and }, and that there is a newline after every ; and
//     before every #.
//
//     If we ARE in a nesting, then all characters pass unchanged.
//
//     Note that this is an "in-place" mutilation, not a copying mutilation.
//

void crm_break_statements(int ini, int nchars, CSL_CELL *csl)
{
    int seennewline;
    int in_comment;
    int neednewline;
    int paren_nest, angle_nest, box_nest, slash_nest;
    int var_delim;

    enum
    {
        PP_UNKNOWN = 0,
        PP_LABEL_START,
        PP_LABEL_END,
        PP_ACTION_START,
        PP_ACTION_END,
        PP_ARGUMENT_SECTION_START,
    } statement_state;
    int i;

    seennewline = 1;
    neednewline = 0;
    in_comment = 0;
    var_delim = 0;

    // statement state machine which has been added to treat stuff like this:
    //   output /x/output /y/
    //
    // 0: begin of statement
    // 1: start of label statement
    // 2: end of label in label statement
    // 3: parsing an action/command (terminated by whitespace or argument sections or ...)
    // 4: end of action/command: expecting whitespace + argument sections
    // 5: parsing argument section(s)
    statement_state = PP_UNKNOWN;

    paren_nest = slash_nest = angle_nest = box_nest = 0;

    if (internal_trace)
        fprintf(stderr, "  preprocessor - breaking statements...\n");
    for (i = ini; i < ini + nchars; i++)
    {
        //    now, no matter what, we're looking at a non-quoted character.
        //
        //   are we looking at a nonprinting character?
        if (csl->filetext[i] < 0x021)
        {
            if (statement_state == PP_ACTION_START)
            {
                statement_state = PP_ACTION_END;
            }

            if (csl->filetext[i] == '\n'
                || csl->filetext[i] == '\r')
            {
                //    get rid of extraneous newlines (MAC, MSDOS and UNIX style supported).
                //if (internal_trace)
                //  fprintf(stderr, "  newline .");
                seennewline = 1;
                neednewline = 0;
                in_comment = 0;
                statement_state = PP_UNKNOWN;
                //    Userbug containment - a newline closes all nests
                paren_nest = slash_nest = angle_nest = box_nest = 0;
            }
            //   other nonprinting characters do not change things.
        }
        else
        {
            //   we don't do any processing inside a comment!
            if (in_comment)
            {
                //   inside a comment, we don't do squat to printing chars.
                //    unless it's an escaped hash; in that case
                //     it's end-of-comment
                if (csl->filetext[i] == '\\'
                    && i + 1 < ini + nchars
                    && csl->filetext[i + 1] == '#')
                {
                    i++;
#if 0               // [i_a] 20080325 - fix for alius_w_comment.crm processing...
                    neednewline = 1;
                    seennewline = 0;
#endif
                    in_comment = 0;
                    statement_state = PP_UNKNOWN;
                }
            }
            else
            {
                //    we are looking at a printing character, so maybe we have
                //    to add a newline.  Or maybe not...
                if (neednewline)
                {
                    if ((csl->nchars + 1) > max_pgmsize)
                        untrappableerror("Program file buffer overflow - "
                                         "post-inserting newline to: ",
                                &(csl->filetext[i]));
                    //    we need a newline and are looking at a printingchar
                    //    so we need to insert a newline.
                    memmove(&(csl->filetext[i + 1]),
                            &(csl->filetext[i]),
                            strlen(&csl->filetext[i]) + 1);
                    csl->filetext[i] = '\n';
                    i++;
                    csl->nchars++;
                    nchars++;
                    neednewline = 0;
                    seennewline = 1;
                    statement_state = PP_UNKNOWN;
                }
                //
                switch (csl->filetext[i])
                {
                case '\\':
                    //     if it's a backslash at the end of a line,
                    //     delete +both+ the backslash and newline, making
                    //     one big line out of it.
                    //
                    //     We do this whether or not we're in a nesting.
                    if (csl->filetext[i + 1] == '\n' || csl->filetext[i + 1] == '\r')
                    {
                        static const char *crlf_mode_descr[] =
                        {
                            "UNIX", NULL, "MAC", "MSDOS"
                        };
                        int crlf_mode = (csl->filetext[i + 1] == '\n'
                                         ? 0                        /* UNIX */
                                         : (csl->filetext[i + 1] == '\r' && csl->filetext[i + 2] == '\n')
                                         ? 3                        /* MSDOS */
                                         : 2                        /* MAC */
                                        );

                        if (internal_trace)
                            fprintf(stderr, " backquoted %s EOL - splicing.\n",
                                    crlf_mode_descr[crlf_mode]
                                   );
                        // (2 | crlf_mode) --> 2 for UNIX/MAC, 3 for MSDOS   :-)
                        memmove(&(csl->filetext[i]),
                                &(csl->filetext[i + (2 | crlf_mode)]),
                                strlen(&csl->filetext[i + (2 | crlf_mode)]) + 1);
                        csl->nchars -= (2 | crlf_mode);
                        nchars -= (2 | crlf_mode);
                        i--;
                    }
                    else
                    {
                        //   Otherwise, we _always_ step over the next
                        //   character- it can't change nesting, it can't
                        //   close a string.  Thus, the preprocessor will
                        //   do nothing to it.
                        //
                        //
                        //    TRICKY BIT HERE !!!  Notice that we do
                        //   this '\' step-over test _BEFORE_ we do
                        //   any other character testing, so the '\'
                        //   gets to do it's escape magic before
                        //   anything else can operate - and it
                        //   _preempts_ any other character's
                        //   actions.
                        //
                        i++;
                    }
                    break;

                case '{':
                case '}':
                    //    put an unquoted '{' or '}' onto it's own line.
                    //   do we need to put in a prefix new line?
                    //            if (internal_trace)
                    //
                    //   Are we inside a nesting?
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        if (!seennewline)
                        {
                            if ((csl->nchars + 1) > max_pgmsize)
                                untrappableerror("Program buffer overflow when"
                                                 "post-inserting newline on:",
                                        &csl->filetext[i]);
                            if (internal_trace)
                                fprintf(stderr, " preinserting a newline.\n");
                            memmove(&(csl->filetext[i + 1]),
                                    &(csl->filetext[i]),
                                    strlen(&csl->filetext[i]) + 1);
                            csl->filetext[i] = '\n';
                            csl->nchars++;
                            nchars++;
                            i++;
                        }
                        seennewline = 0;
                        //   and mark that we need a newline before any more
                        //   printable characters come through.
                        neednewline = 1;
                        statement_state = PP_UNKNOWN;
                    }
                    break;

                case ';':
                    //       we can replace non-escaped semicolons with
                    //       newlines.
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        if (seennewline)                         //  we just saw a newline
                        {
                            //  was preceded by a newline so just get rid
                            //  of the ;
                            if (internal_trace)
                                fprintf(stderr,
                                        "superfluous semicolon, *poof*.\n");
                            memmove(&(csl->filetext[i]),
                                    &(csl->filetext[i + 1]),
                                    strlen(&csl->filetext[i]) + 1);
                            csl->nchars--;
                            nchars--;
                            i--;
                            neednewline = 0;
                            seennewline = 1;
                            statement_state = PP_UNKNOWN;
                        }
                        else
                        {
                            //   this was not preceded by a newline,
                            //  so we just replace the semicolon with a
                            //  newline before any printed characters
                            if (internal_trace)
                                fprintf(stderr, " statement break semi.\n"
                                                "--> \\n \n");
                            csl->filetext[i] = '\n';
                            neednewline = 0;
                            seennewline = 1;
                            statement_state = PP_UNKNOWN;
                        }
                    }
                    break;

                case '#':
                    //      now, we're in a comment - everything should be
                    //      done only with the comment thing enabled.
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        in_comment = 1;
                    }
                    break;

                case '(':
                    //      Update nesting if necessary
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        paren_nest = 1;
                        statement_state = PP_ARGUMENT_SECTION_START;
                    }
                    break;

                case ')':
                    //      Update nesting if necessary
                    if (paren_nest == 1
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        paren_nest = 0;
                    }
                    break;

                case '<':
                    //      Update nesting if necessary
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        angle_nest = 1;
                        statement_state = PP_ARGUMENT_SECTION_START;
                    }
                    break;

                case '>':
                    //      Update nesting if necessary
                    if (paren_nest == 0
                        && angle_nest == 1
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        angle_nest = 0;
                    }
                    break;

                case '[':
                    //      Update nesting if necessary
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        box_nest = 1;
                        var_delim = 0;
                        statement_state = PP_ARGUMENT_SECTION_START;
                    }
                    break;

                case ']':
                    //      Update nesting if necessary
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 1
                        && slash_nest == 0)
                    {
                        box_nest = 0;
                    }
                    break;

                case ':':
                    //      see also crm_generic_parse_line(): handle the special case [:var: /regex/]
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 1
                        && slash_nest == 0)
                    {
                        var_delim++;
                    }
                    else if (paren_nest == 0
                             && angle_nest == 0
                             && box_nest == 0
                             && slash_nest == 0)
                    {
                        if (statement_state == PP_UNKNOWN)
                        {
                            statement_state = PP_LABEL_START;
                        }
                        else if (statement_state == PP_LABEL_START)
                        {
                            statement_state = PP_LABEL_END;
                        }
                    }
                    break;

                case '/':
                    //      Update nesting if necessary
                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0)
                    {
                        slash_nest = !slash_nest;
                        statement_state = PP_ARGUMENT_SECTION_START;
                    }
                    else if (paren_nest == 0
                             && angle_nest == 0
                             && box_nest == 1
                             && var_delim >= 2)
                    {
                        //      see also crm_generic_parse_line(): handle the special case [:var: /regex/]
                        slash_nest = !slash_nest;
                        statement_state = PP_ARGUMENT_SECTION_START;
                    }
                    break;

                default:
                    //      none of the above - it's a normal printing
                    //  character - we can just do the
                    //  clearing of all the "seen/need" flags
                    seennewline = 0;
                    neednewline = 0;

                    if (paren_nest == 0
                        && angle_nest == 0
                        && box_nest == 0
                        && slash_nest == 0)
                    {
                        if (statement_state >= PP_ARGUMENT_SECTION_START /* PP_ACTION_END */)
                        {
                            // detected the start of another command on the same line
                            // or the end of the list of args for the previous cmd, e.g.
                            //
                            //   cmd1 cmd2
                            //   cmd1 /arg1/ /arg2/ cmd2
                            //
                            // WARNING: as we cannot simply detect the difference between
                            // the sequence
                            //   cmd1 cmd2
                            // and the legal construct:
                            //   insert filename
                            // we've decided NOT to cope with the (rather illegal) 'cmd1 cmd2' sequence
                            // if there are no argument sections in between the two. Hence the condition
                            //  (statement_state >= PP_ARGUMENT_SECTION_START)
                            // instead of
                            //  (statement_state >= PP_ACTION_END)
                            // above.

                            if (!seennewline)
                            {
                                if ((csl->nchars + 1) > max_pgmsize)
                                    untrappableerror("Program buffer overflow when"
                                                     "pre-inserting newline on:",
                                            &csl->filetext[i]);
                                if (internal_trace)
                                    fprintf(stderr, " preinserting a newline.\n");
                                memmove(&(csl->filetext[i + 1]),
                                        &(csl->filetext[i]),
                                        strlen(&csl->filetext[i]) + 1);
                                csl->filetext[i] = '\n';
                                csl->nchars++;
                                nchars++;
                                i++;
                            }
                            statement_state = PP_UNKNOWN;
                        }
                        // start of a new action/command?
                        if (statement_state == PP_UNKNOWN)
                        {
                            statement_state = PP_ACTION_START;
                        }
                    }
                    break;
                }
            }
        }
    }
}



