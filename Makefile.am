# Process this file with automake to produce Makefile.in (in this,
# and all subdirectories).

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007 Ger Hobbelt

AUTOMAKE_OPTIONS = check-news


PACKAGE = @PACKAGE_TARNAME@
TAR_FILENAME_POSTFIX = @TAR_FILENAME_POSTFIX@
VER_SUFFIX = @VER_SUFFIX@ 
VERSION = @VERSION@
ARCHIVE_BASE_NAME = @PACKAGE_TARNAME@-@PACKAGE_VERSION@-@TAR_FILENAME_POSTFIX@

SILENT = @SILENT@
DIFF_FLAGS = @DIFF_FLAGS@


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
	m4/ax_cflags_gcc_option.m4 m4/ax_diff_option.m4 \
	mk_distros.sh \
	uncrustify.cfg


if MAINTAINER_MODE
ChangeLog: NEWS
	-( test "X$(cvs2cl)" != "X" && $(cvs2cl) --prune --accum -U ../CVSROOT/users )
endif 

stamp.year stamp.day stamp.month: ChangeLog configure.ac
	sh $(srcdir)/setversion


help: 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) help);

dist-7z: distdir
	-rm -f $(distdir).7z
	( test "X$(sevenzip)" != "X" && $(sevenzip) a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(distdir).7z $(distdir) )
	$(am__remove_distdir)

dist-diff: 
	$(SILENT) echo "Creating diff archives"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) dist-diff);

distribution: distdir dist-diff
	$(SILENT) echo "Creating src archives"; 
	( test "X$(sevenzip)" != "X" && $(sevenzip) a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(ARCHIVE_BASE_NAME).src.7z $(distdir) )
	( test "X$(tar)" != "X" && $(tar) -c $(distdir) | GZIP=$(GZIP_ENV) gzip -9 -c > $(ARCHIVE_BASE_NAME).src.tar.gz )
	( test "X$(tar)" != "X" && $(tar) -c $(distdir) | bzip2 -9 -c > $(ARCHIVE_BASE_NAME).src.tar.bz2 )
	-rm -f $(ARCHIVE_BASE_NAME).src.zip
	( test "X$(zip)" != "X" && $(zip) -rq $(ARCHIVE_BASE_NAME).src.zip $(distdir) )
	$(am__remove_distdir)


.PHONY: reindent depend dist-7z-all dist-diff dist-maint


reindent:
	$(SILENT) echo "Reformatting sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) reindent);

depend:
	$(SILENT) echo "Calculating dependencies for the sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) depend);

megatest:
	$(SILENT) echo "Running the regression test:"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) megatest);


