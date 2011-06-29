//  crm_expr_match.c  - Controllable Regex Mutilator,  version v1.0
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



//        And the match routine.  What a humungous mess...
int crm_expr_match (CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
  long i;
  long j;
  long k;
  long mc;
  char pch[MAX_PATTERN];
  long pchlen;
  char errstr[MAX_PATTERN+128];
  char *mdwptr;
  regex_t preg;
  int casep, nomultilinep, absentp, fromp, extended_regex_p, literal_pattern_p;
  int cflags, eflags;
  char *mtext;
  long mtextlen;
  long textoffset;
  char bindable_vars[MAX_PATTERN];
  long bindable_vars_len;
  char box_text[MAX_PATTERN];
  regmatch_t matches [MAX_SUBREGEX];
  long nmatches;
  long source_start;
  long source_len;
  long vtextoffset, vtextend, vtextstartlimit;
  long vpmstart, vpmend;
  long vmidx;
  //    And it all comes down to this, right here.  Matching a regex.
  //    This is the cruxpoint of the whole system.  We parse the
  //    program line args, get the flags out of the <> brackets, get the
  //    bound values out of the () parens, and the regex pattern out
  //    of the // delimiters.  Then we regcomp the pattern, and apply
  //    it to the data window (or to the variable if one is supplied).
  //    Then we either pass thru or fail to the failpoint.
  //
  //    get the flags out of the <> brackets
  //        cflags = REG_EXTENDED + REG_ICASE + REG_NEWLINE;
  //
  //  Translate to the cflags REG_EXTENDED, REG_ICASE, REG_NEWLINE
  //  (newline doesn't match wldcrd) and the eflags REG_NOTBOL (no
  //  newline at start) REG_NOTEOL (no newline at end)

  casep = 0;
  nomultilinep = 0;
  absentp = 0;
  fromp = 0;
  extended_regex_p = 1;
  literal_pattern_p = 0;

  //       Go through the flags
  //      is the ignore case flag set?
  CRM_ASSERT(apb != NULL);
  if (apb->sflags & CRM_NOCASE)
    {
      if (user_trace)
        fprintf (stderr, "  nocase turned on...\n");
      casep = 1;
    }
  //      is the "basic regex" (obsolete, but useful) flag set?
  if (apb->sflags & CRM_BASIC)
    {
      if (user_trace)
        fprintf (stderr, "  basic regex match turned on...\n");
      extended_regex_p = 0;
    }

  if (apb->sflags & CRM_NOMULTILINE)
    {
      if (user_trace)
        fprintf (stderr, "  nomultiline turned on...\n");
      nomultilinep = 1;
    }

  if (apb->sflags & CRM_ABSENT)
    {
      if (user_trace)
        fprintf (stderr, "  absent flag turned on...\n");
      absentp = 1;
    }

  if (apb->sflags & CRM_LITERAL)
    {
      if (user_trace)
        fprintf (stderr, "  literal pattern search turned on...\n");
      literal_pattern_p = 1;
    }


  //  default is NO special fromming...
  fromp = 0;

  if (apb->sflags & CRM_FROMSTART)
    {
      if (user_trace)
        fprintf (stderr, "  fromstart turned on...\n");
      fromp = CRM_FROMSTART;
    }

  if (apb->sflags & CRM_FROMNEXT)
    {
      if (user_trace)
        fprintf (stderr, "  fromnext turned on...\n");
      fromp = CRM_FROMNEXT;
    }

  if (apb->sflags & CRM_FROMEND)
    {
      if (user_trace)
        fprintf (stderr, "  fromend turned on...\n");
      fromp = CRM_FROMEND;
    }

  if (apb->sflags & CRM_NEWEND)
    {
      if (user_trace)
        fprintf (stderr, "  newend turned on...\n");
      fromp = CRM_NEWEND;
    }

  if (apb->sflags & CRM_BACKWARDS)
    {
      if (user_trace)
        fprintf (stderr, "  backwards search turned on...\n");
      fromp = CRM_BACKWARDS;
    }

  if (apb->sflags & CRM_FROMCURRENT)
    {
      if (user_trace)
        fprintf (stderr, "  from-current search turned on...\n");
      fromp = CRM_FROMCURRENT;
    }

  //   Now, from the flags, calculate the cflags and eflags
  cflags = casep * REG_ICASE
    + nomultilinep * REG_NEWLINE
    + extended_regex_p * REG_EXTENDED
    + literal_pattern_p * REG_LITERAL;

  eflags = 0; //

  //    get the bound values out of the () parenthesis
  //
  //   DANGER WILL ROBINSON!!  TAKE COVER, DOCTOR SMITH!!!  We
  //   have to be really careful here, because we need durable
  //   variable names to reference the VHT to, and an
  //   expandvar'ed variable doesn't have that durable text
  //   string somewhere.  So, we have to stuff the variable name
  //   in as a temp var and then immediately reassign it.
  //
  crm_get_pgm_arg (bindable_vars, MAX_PATTERN, apb->p1start, apb->p1len);
  if (internal_trace)
    fprintf (stderr, " bindable vars: ***%s***\n", bindable_vars);
  bindable_vars_len = crm_nexpandvar (bindable_vars, apb->p1len, MAX_PATTERN);


  //     here's where we look for a [] var-restriction
  //
  //     Experimentally, we're adding [ :foo: 123 456 ] to
  //     allow an externally specified start and length.
  crm_get_pgm_arg (box_text, MAX_PATTERN, apb->b1start, apb->b1len);

  //  Use crm_restrictvar to get start & length to look at.
  i = crm_restrictvar(box_text, apb->b1len,
                      &vmidx,
                      &mdwptr,
                      &source_start,
                      &source_len,
                      errstr);

  if (internal_trace)
    fprintf (stderr,
             "restricted: vmidx: %ld  mdw: %p   start: %ld  len: %ld\n",
             vmidx, mdwptr, source_start, source_len);
  if ( i < 0)
    {
      long curstmt;
      curstmt = csl->cstmt;
      if (i == -1)
        nonfatalerror (errstr, "");
      if (i == -2)
        fatalerror (errstr, "");
      //
      //     did the FAULT handler change the next statement to execute?
      //     If so, continue from there, otherwise, we FAIL.
      if (curstmt == csl->cstmt)
        {
          csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
          csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
        }
      goto nonfatal_route_outwards;
    }

  //    get the regex pattern out of the // slashes
  crm_get_pgm_arg (pch, MAX_PATTERN, apb->s1start, apb->s1len);
  if (internal_trace)
    fprintf (stderr, " match pattern: =%s=\n", pch );
  pchlen = crm_nexpandvar (pch, apb->s1len, MAX_PATTERN);

  if (user_trace)
    fprintf (stderr, " match pattern expands to =%s= len %ld flags %x %x \n",
             pch, pchlen, cflags, eflags);

  //    regcomp the pattern
  i = crm_regcomp (&preg, pch, pchlen, cflags);
  if ( i > 0)
    {
      long curstmt;
      curstmt = csl->cstmt;
      crm_regerror ( i, &preg, tempbuf, data_window_size);
      fatalerror ("Regular Expression Compilation Problem:", tempbuf);
      //
      //     did the FAULT handler change the next statement to execute?
      //     If so, continue from there, otherwise, we FAIL.
      if (curstmt == csl->cstmt)
        {
          csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
          csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
        }
      goto nonfatal_route_outwards;
    }


  //     Get the string to be matched upon...
#ifdef SUPERCEDED_BY_CRM_RESTRICTVAR
  vmidx = crm_vht_lookup (vht, svname, svnamelen);
  if (vht[vmidx] == NULL)
    {
      //
      //     There was no such variable, so we need to fail.  First,
      //     we'll save the current and fail locations, then we'll let
      //     the error handler attempt fixup.  If the handler exists,
      //     and changes the FAIL location, the handler's result
      //     stands, otherwise the match does a FAIL.
      //
      long curstmt;
      curstmt = csl->cstmt;
      nonfatalerror (" Attempt to match inside nonexistent variable ( always fails!) ",
                     svname);
      //
      //     did the FAULT handler change the next statement to execute?
      //     If so, continue from there, otherwise, we FAIL.
      if (curstmt == csl->cstmt)
        {
          csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
          csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
        }
      goto nonfatal_route_outwards;
    }
#endif

  //    do a check- if the vht->valtxt points into the
  //    cdw, or the tdw?
  mdw = cdw;    //  assume cdw unless otherwise proven...
  if (vht[vmidx]->valtxt == tdw->filetext)
    mdw=tdw;
  //  sanity check - must be tdw or cdw for searching!
  if (vht[vmidx]->valtxt != tdw->filetext
      && vht[vmidx]->valtxt != cdw->filetext)
    {
      long q;
      q = fatalerror ("Bogus text block (neither cdw nor tdw) on var ",
                box_text);
      if (q != 0)
        {
          if (engine_exit_base != 0)
            {
              exit (engine_exit_base + 7);
            }
          else
            exit (EXIT_FAILURE);
        }
    }
#ifdef SUPERCEDED
  vtextoffset = vht[vmidx]->vstart;
  vtextend = vtextoffset + vht[vmidx]->vlen;
  vpmstart = vht[vmidx]->mstart;
  vpmend =  vpmstart + vht[vmidx]->mlen;
#endif

  vtextoffset = source_start;
  vtextend = source_start + source_len;
  vpmstart = vht[vmidx]->mstart;
  vpmend = vpmstart + vht[vmidx]->mlen;

  //    set up the start/end of the text we're matching against

  //       default is CRM_FROMSTART
  textoffset = vtextoffset;
  if (fromp == CRM_FROMSTART)
    {
      textoffset = vtextoffset;
    }
  if (fromp == CRM_FROMCURRENT)
    {
      textoffset = vpmstart;
    }
  if (fromp == CRM_NEWEND)
    {
      textoffset = vpmstart;
    }
  if (fromp == CRM_FROMNEXT)
    {
      textoffset = vpmstart + 1 ;
    }
  if (fromp == CRM_FROMEND)
    {
      textoffset = vpmend ;
    }
  if (fromp == CRM_BACKWARDS)
    {
      if ( vpmstart > 0)
        { textoffset = vpmstart - 1; } else {textoffset = vpmstart; }
    }

  mtextlen = vtextend - textoffset;

#ifdef  SUPERCEDED_BY_CRM_RESTRICTVAR
  //
  //    did the user box-specify a different start?  Combine the restrictions!
  //
  //     1) start of search - must be inside vtextoffset + box_start
  if (textoffset < vtextoffset + box_start)
    textoffset = box_start + vtextoffset;
  //
  //     2) the area searched must be <= box_start+box_len + vtextoffset
  if (mtextlen+textoffset > box_start+box_length+vtextoffset)
    mtextlen = box_start + box_length +vtextoffset - textoffset;
  //
  //    3) the earliest point we'll allow a search to go is the start of
  //       the variable + box_start
  vtextstartlimit = vtextoffset + box_start;
#endif
  vtextstartlimit = source_start;
  if (internal_trace)
    {
      fprintf (stderr, "    start matchable zone: %ld, begin search %ld, length %ld\n",
               vtextstartlimit, textoffset, mtextlen);
    }

  //       Here is the crux.  Do the REGEX match, maybe in a loop
  //       if we're iterating to find a result with a different end
  //       point than previous matches.
  nmatches = MAX_SUBREGEX;
  matches[0].rm_so = 0;
  matches[0].rm_eo = 0;
  switch (fromp)
    {
    case CRM_NEWEND:
      {
        long oldend;
        long done;
        oldend = vpmend;
        done = 0;
        //        loop until we either get a match that goes
        //        past the previous match, or until we are
        //        at the end of the matchable text.
        while (textoffset <= vtextend - 1
               && done == 0)
          {
            textoffset++;
            mtextlen--;
            mtext = &mdw->filetext[textoffset];
            i = crm_regexec (&preg, mtext, mtextlen,
                             nmatches, matches, eflags, NULL);
            j = matches[0].rm_eo;
            if (( (textoffset + j) > oldend) && (i == 0)) done = 1;
          }
      }
      break;
    case CRM_BACKWARDS:
      {
        long oldstart;
        oldstart = vpmstart;
        i = -1;
        j = oldstart + 1;
        matches[0].rm_so = j;
        //        loop until we either get a match or until we have hit
        //        the start of this (possibly captured-variable) region.
        while (textoffset > vtextstartlimit
               && (i != 0 || j > oldstart)  )
          {
            textoffset--;
            mtextlen++;
            mtext = &mdw->filetext[textoffset];
            i = crm_regexec (&preg, mtext, mtextlen,
                             nmatches, matches, eflags, NULL);
            j = matches[0].rm_so;
          }
      }
    default:
      {
        mtext = &mdw->filetext[textoffset];
        i = crm_regexec ( &preg, mtext, mtextlen,
                          nmatches, matches, eflags, NULL);
      }
    }

  crm_regfree (&preg);

  //    and now we FAIL or not...


  if ((absentp == 0 && i != 0) || (absentp == 1 && i == 0))
    {
      if (user_trace && !absentp)
        fprintf (stderr, "Regex did not match, no absent flag, failing.\n");
      if (user_trace && absentp)
        fprintf (stderr, "Regex matched but with absent flag, failing.\n");
      csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
      csl->aliusstk [ csl->mct[csl->cstmt]->nest_level ] = -1;
    }
  else
    {
      if (user_trace)
        fprintf (stderr, "Regex matched.\n");
      //     if the match was succcessful, we may need to
      //     bind some variables, so see if there's a ()
      //     (note that we cannot use the grab_delimited_string
      //     routine results here, because crm_setvar uses
      //     indices into the program to define variable names,
      //     not char * strings.   We _could_ cheat and malloc
      //     the variable name (perhaps we should!) and then
      //     clean up when we're done, but this at least will
      //     work for now.

      //   CAREFUL HERE - we need to correct due to offsets
      //   from the front of the text

      //    Double careful- if some of the regex wasn't used
      //    (such as submatches) the indices and lengths will be -1.
      //    Don't bind those.

      //     set the "last match was here" data...
      vht[vmidx]-> mstart = matches[0].rm_so + textoffset;
      vht[vmidx]-> mlen = matches[0].rm_eo - matches[0].rm_so;

      if (bindable_vars_len > 0 && absentp )
        nonfatalerror ("This program specifies an 'absent' match, and also "
                       "tries to bind variables that, by the above, aren't "
                       "matched!  ",
                       "We'll ignore these variable bindings for now.");

      if ( bindable_vars_len > 0 && !absentp )
        {
          long vstart;
          long vlen;
          long vnext;
          long done;
          //              a place to put pre-rebind
          //              text/starts/lengths, so we can run
          //              reclamation on them later on.
          //
          char *index_texts[MAX_SUBREGEX];
          long index_starts[MAX_SUBREGEX],
            index_lengths[MAX_SUBREGEX];

          done = 0;          //  loop till we've captured all the vars
          mc = 0;
          vstart = 0;
                  while ( !done)
            {              // bind each variable
              //    find the start of the variable
              while (bindable_vars[vstart] > 0x0
                     && bindable_vars[vstart] < 0x021
                     && bindable_vars[vstart] != ')' )
                vstart++;
              if (bindable_vars[vstart] == ')'
                  || bindable_vars[vstart] == 0x0 )
                {
                  done = 1;
                }
              else
                {

                  //    Now, the next space or ) ends the variable
                  vlen = 0;
                  while (bindable_vars[vstart+vlen] >=0x21
                         && bindable_vars[vstart+vlen] != ')' )
                    vlen++;
                  //    have the next variable name, put out debug info.
                  if (internal_trace)
                    {
                      fprintf (stderr, "variable -");
                      for (k = 0; k < vlen; k++)
                        fprintf (stderr, "%c", bindable_vars[vstart+k]);
                      fprintf (stderr, "- will be assigned from var offsets %ld to %ld "
                               "(origin offsets %ld to %ld), value ",
                               (long) matches[mc].rm_so,
                               (long) matches[mc].rm_eo,
                               matches[mc].rm_so + textoffset,
                               matches[mc].rm_eo + textoffset );
                      for (k = matches[mc].rm_so + textoffset;
                           k < matches[mc].rm_eo + textoffset;
                           k++)
                        fprintf (stderr, "%c", mdw->filetext[k]);
                      fprintf (stderr, "\n");
                    }
                  vnext = vstart + vlen;
                  //  HERE'S THE DANGEROUS PART..  because varible
                  //  names have been expanded, we can't assume
                  //  that the variablename in the program text
                  //  will be usable.  So, we create the varname as a temp
                  //  var, and then can reassign it with impunity.
                  {
                    static char *vn;

                    //   DANGER here - we malloc the var, use it
                    //   in crm_set_windowed_nvar, and then free it.
                    //   Otherwise, we'd have a memory leak.
                    //
                    if (!vn)
                      vn = (char *) calloc ((MAX_VARNAME+16) , sizeof(vn[0]));
                    if (!vn)
                      untrappableerror("Couldn't malloc vn.\n Can't fix that.","");
                    strncpy (vn, &(bindable_vars[vstart]), vlen);
                    vn[vlen] = '\000';
                    if (strcmp (vn, ":_dw:") != 0)
                      {
                        {
                          long vi;
                          vi = crm_vht_lookup (vht, vn, vlen);
                          if (vht[vi] == NULL)
                            {
                              index_texts[mc] = 0;
                              index_starts[mc] = 0;
                              index_lengths[mc] = 0;
                            }
                          else
                            {
                              index_texts[mc] = vht[vi]->valtxt;
                              index_starts[mc] = vht[vi]->vstart;
                              index_lengths[mc] = vht[vi]->vlen;
                            }
                        }
                        //    watch out for nonparticipating () submatches...
                        //    (that is, submatches that weren't used because
                        //     of a|(b(c)) regexes.  These have .rm_so offsets
                        //      of < 0 .
                        if (matches[mc].rm_so >= 0)
                          crm_set_windowed_nvar (vn,
                                                 vlen,
                                                 mdw->filetext,
                                                 matches[mc].rm_so
                                                 + textoffset,
                                                 matches[mc].rm_eo
                                                 -matches[mc].rm_so,
                                                 csl->cstmt);
                      }
                    else
                      {
                        nonfatalerror ("This program tried to re-define the "
                                       "data window!  That's very deep and "
                                       "profound, but not acceptable.  ",
                                       "Therefore, I'm ignoring this "
                                       "re-definition.");
                      }
                    // free (vn);
                  }
                  //    and move on to the next binding (if any)
                  vstart = vnext;
                  mc++;
                  if (mc >= MAX_SUBREGEX)
                    nonfatalerror (
                                   "Exceeded MAX_SUBREGEX limit-too many parens in match",
                                   " Looks like you blew the gaskets on 'er.\n");

                }
            }
          //
          //      Now do cleanup/reclamation of old memory space, if needed.
          //
          //      Nasty trick here - we have to do these reclamations
          //      in a specific order, because during reclamation, the
          //      indicies we have in the index_starts will become
          //      altered in ways we don't have the ability to know
          //      here.  So, we need to do the greatest index_starts
          //      first, so that earlier index_starts won't be
          //      damaged.  If we do them last-first, then prior ones
          //      will still have correct starts and lengths for the
          //      reclamation.
          //
          //      Note that we don't have to worry about a reclaim on
          //      a var that was "non-participating", as the var will
          //      still be in use in the VHT and thus won't be reclaimed.
          {
            long i;
            long done = 0;
            long reclaimed;
            long maxstart, maxi;
            //      fprintf (stderr, "MC is %ld\n", mc);
            while (!done)
              {
                maxstart = 0;
                maxi = -1;
                for (i = 0; i < mc; i++)
                  {
                    if (index_texts[i] == tdw->filetext)
                      {
                        if (maxstart < index_starts[i])
                          {
                            maxi = i;
                            maxstart = index_starts[i];
                          }
                      }
                  }
                //  Now we know the last reclaim area; we can safely
                //  reclaim that area (and no other)
                if (maxi >= 0)
                  {
                    long j;
                    if (internal_trace)
                      fprintf (stderr," crm_comprss_tdw_section from match\n");
                    // because the prev shortening, current index_starts[maxi]
                    // + index_lengths[maxi] may go past the end of tdw->nchars
                    j = index_starts[maxi] + index_lengths[maxi];
                    if (j > tdw->nchars - 1) j = tdw->nchars;
                    reclaimed = crm_compress_tdw_section
                      (index_texts[maxi],
                       index_starts[maxi],
                       j);
                    // WAS index_starts[maxi] + index_lengths[maxi] );
                    if (internal_trace)
                      fprintf (stderr,
                               " [ MatchVar #%ld (s: %ld l: %ld) reclaimed %ld. ]\n",
                               maxi, index_starts[maxi], index_lengths[maxi],
                               reclaimed);
                    //   and zap out the reclaimed entry, so other entries
                    //   can also be reclaimed in the proper order.
                    index_starts[maxi] = -1;
                  }
                if (maxi == -1)
                  done = 1;
              }
          }
        }
    }


  if (0)
    {
    nonfatal_route_outwards:
      if (user_trace)
        fprintf (stderr, "The MATCH FAULTed and we're taking the TRAP out");
    }

  return (0);
}
