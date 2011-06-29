# Process this file with automake to produce Makefile.in (in this,
# and all subdirectories).

## $Id: Makefile.am,v 1.14 2004/08/19 09:36:09 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal

AUTOMAKE_OPTIONS = check-news

PACKAGE = @PACKAGE_TARNAME@

SUBDIRS = docs examples mailfilter man src tests

# docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = AUTHORS COPYING ChangeLog HACKING README TODO NEWS README.1st


DISTCLEANFILES = 
CLEANFILES = *~ *.bak

EXTRA_DIST = bootstrap crm114.lsm.in HACKING README.1st setversion \
	stamp.year stamp.day stamp.month \
	distribute_files_from_src_dir.sh fix_crlf.sh \
	m4/ac_c_long_long.m4 m4/ac_environ.m4 m4/ac_func_memmove.m4 \
	m4/ac_func_snprintf.m4 m4/ax_cflags_warn_all_ansi.m4 \
	m4/ax_cflags_warn_all.m4 m4/vl_lib_readline.m4 \
	m4/ax_cflags_gcc_option.m4 mk_distros.sh


if MAINTAINER_MODE
ChangeLog: NEWS
	-( if [ "$(cvs2cl)" != "" ]; then $(cvs2cl) --prune --accum -U ../CVSROOT/users ; fi )
endif 

stamp.year stamp.day stamp.month: ChangeLog configure.ac
	sh $(srcdir)/setversion


dist-7z: distdir
	-rm -f $(distdir).7z
	7z a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(distdir).7z $(distdir)
	$(am__remove_distdir)

dist-diff: 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) diff);
	tar cvjf  $(distdir).diffs.tar.bz2 src/diff
	7z a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(distdir).diffs.7z src/diff



.PHONY: reindent depend dist-7z-all dist-diff


reindent:
	echo "Reformatting sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) reindent);

depend:
	echo "Calculating dependencies for the sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) depend);

megatest:
	echo "Running the regression test:"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) megatest);


