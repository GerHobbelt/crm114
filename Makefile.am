# Process this file with automake to produce Makefile.in (in this,
# and all subdirectories).

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007 Ger Hobbelt

GZIP_BIN       = @GZIP@
COMPRESS_BIN   = @COMPRESS@
LZMA_BIN       = @LZMA@
SEVENZIP_BIN   = @SEVEN_ZIP@
TAR_BIN        = @TAR@
SHAR_BIN       = @SHAR@
BZIP2_BIN      = @BZIP2@
ZIP_BIN        = @ZIP@
CVS2CL_BIN     = @CVS2CL@
UNCRUSTIFY_BIN = @UNCRUSTIFY@
DIFF_BIN       = @DIFF@
HTMLTIDY_BIN   = @HTMLTIDY@
DB2X_XSLTPROC  = @DB2X_XSLTPROC@
DB2X_MANXML    = @DB2X_MANXML@
DB2X_TEXIXML   = @DB2X_TEXIXML@


# [i_a] instruct AC_LOCAL to correctly include the ./m4 directory too
ACLOCAL_AMFLAGS = -I $(top_srcdir)/m4


SEVENZIP_OPTIONS=a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on
# SEVENZIP_OPTIONS=a -si




PACKAGE = @PACKAGE_TARNAME@
TAR_FILENAME_POSTFIX = @TAR_FILENAME_POSTFIX@
VER_SUFFIX = @VER_SUFFIX@ 
VERSION = @VERSION@
ARCHIVE_BASE_NAME = @PACKAGE_TARNAME@-@PACKAGE_VERSION@-@TAR_FILENAME_POSTFIX@

SILENT = @SILENT@
DIFF_FLAGS = @DIFF_FLAGS@

ECHO = echo
E = $(SILENT) $(ECHO)



#
# [i_a] WARNING: do NOT name these other than $(distdir).tar.* as other names, e.g.
#         $(ARCHIVE_BASE_NAME).src.tar.*
#       will NOT work as the generated makefile code cannot cope with this flexibility (yet).
#
#       It's that or I have overlooked a autoconf/automake/auto-whatever option... :-S
#
DIST_ARCHIVES =
if OS_HAS_TAR
if OS_HAS_GZIP
DIST_ARCHIVES += $(distdir).tar.gz
endif
if OS_HAS_BZIP2
DIST_ARCHIVES += $(distdir).tar.bz2
endif
if OS_HAS_COMPRESS
DIST_ARCHIVES += $(distdir).tar.Z
endif
endif




SUBDIRS = docs examples mailfilter man src tests


# docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = AUTHORS COPYING ChangeLog HACKING README TODO NEWS README.1st


DISTCLEANFILES = 
CLEANFILES = *~ *.bak
MAINTAINERCLEANFILES = configure.in aclocal.m4 Makefile.in


MSVC_PRJ_FILES = win32/config_win32.h \
	win32/crm114.sln win32/crm114.vcproj win32/cssdiff.vcproj win32/osbf-util.vcproj \
	win32/resource.h win32/crm114.ico win32/cssmerge.vcproj win32/cssutil.vcproj \
	win32/getopt.h win32/getopt_ex.h win32/getopt.c win32/getopt_ex.c \
	win32/crm_win32_port.c \
	win32/megatest.bat win32/crm114rc.h win32/crm114rc.rc 

EXTRA_DIST = bootstrap crm114.lsm.in HACKING README.1st setversion \
	stamp.year stamp.day stamp.month \
	distribute_files_from_src_dir.sh fix_crlf.sh \
	m4/ac_c_long_long.m4 m4/ac_environ.m4 m4/ac_func_memmove.m4 \
	m4/ac_func_snprintf.m4 m4/ax_cflags_warn_all_ansi.m4 \
	m4/ax_cflags_warn_all.m4 m4/vl_lib_readline.m4 \
	m4/ax_cflags_gcc_option.m4 m4/ax_diff_option.m4 \
	m4/ac___function___c99_vpredef_vars.m4 \
	m4/ac_nsec_stat_fields.m4 \
	mk_distros.sh \
	uncrustify.cfg \
	cleanup.bat autogen.sh CREDITS README.win32 BUGS \
	README.maintainer MAILINGLIST \
	$(MSVC_PRJ_FILES)

WIN32_BIN_EXTRA = CREDITS README.win32 BUGS \
	MAILINGLIST \
	HACKING README.1st \
	docs/classify_details.txt \
	docs/COLOPHON.txt \
	docs/CRM114_Mailfilter_HOWTO.txt \
	docs/FAQ.txt \
	docs/INTRO.txt \
	docs/knownbugs.txt \
	docs/QUICKREF.txt \
	docs/README \
	docs/VT_generic_Vector_Tokenization.txt \
	mailfilter/classifymail.crm \
	mailfilter/mailfilter.cf \
	mailfilter/mailfilter.crm \
	mailfilter/maillib.crm \
	mailfilter/mailreaver.crm \
	mailfilter/mailtrainer.crm



if MAINTAINER_MODE

ChangeLog: NEWS
	-( test "X$(cvs2cl)" != "X" && $(cvs2cl) --prune --accum -U ../CVSROOT/users )
	
BillY-sync:
	$(SILENT) distribute_files_from_src_dir.sh 

endif 

stamp.year stamp.day stamp.month: ChangeLog configure.ac
	sh $(srcdir)/setversion


help: 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) help);


#
# with conditionals in automake-processed makefiles like these, pay SPECIAL attention ot
#   http://sources.redhat.com/ml/automake/2000-10/msg00108.html
# --> use AM_CONDITIONAL instead of makefile ifeq/ifneq/...
#

dist-gzip: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.gz
if OS_HAS_TAR
if OS_HAS_GZIP
	$(TAR_BIN) -c $(distdir) | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).src.tar.gz
endif
endif
	$(am__remove_distdir)

dist-bzip2: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.bz2
if OS_HAS_TAR
if OS_HAS_BZIP2
	$(TAR_BIN) -c $(distdir) | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.bz2
endif
endif
	$(am__remove_distdir)

dist-lzma: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.lzma
if OS_HAS_TAR
if OS_HAS_LZMA
	$(TAR_BIN) -c $(distdir) | $(LZMA_BIN) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.lzma
endif
endif
	$(am__remove_distdir)

dist-tarZ: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.Z
if OS_HAS_TAR
if OS_HAS_COMPRESS
	$(TAR_BIN) -c $(distdir) | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).src.tar.Z
endif
endif
	$(am__remove_distdir)

dist-7z: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.7z
if OS_HAS_SEVENZIP
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).src.7z $(distdir)
endif
	$(am__remove_distdir)

dist-shar: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.shar.gz
if OS_HAS_SHAR
if OS_HAS_GZIP
	$(SHAR_BIN) $(distdir) | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).src.shar.gz
endif
endif
	$(am__remove_distdir)

dist-zip: distdir
	-rm -f $(ARCHIVE_BASE_NAME).zip
if OS_HAS_ZIP
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).src.zip $(distdir)
endif
	$(am__remove_distdir)



diff: 
	$(SILENT) echo "Creating diff archives"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) diff);
	

# Note: the COPYING+ChangeLog in there is just to make sure the dist-diff
#       archivers have something to archive; if not, they'll fail in 'make distcheck' at least.
#       And not everyone has the same setup as me, so @ORIGINAL_BILLY_DISTRO_DIR@
#       may point at a bogus directory for them!
dist-diff: diff	COPYING ChangeLog
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.7z
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.tar.bz2
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.zip
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.7z
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.tar.bz2
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.zip
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.7z
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.tar.bz2
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.zip
if OS_HAS_TAR
if OS_HAS_GZIP
	$(TAR_BIN) -c src/diff/*.unified.* COPYING ChangeLog | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.gz
	$(TAR_BIN) -c src/diff/*.normal.* COPYING ChangeLog | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.gz
	$(TAR_BIN) -c src/diff/*.context.* COPYING ChangeLog | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).context.diff.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR_BIN) -c src/diff/*.unified.* COPYING ChangeLog | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.bz2
	$(TAR_BIN) -c src/diff/*.normal.* COPYING ChangeLog | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.bz2
	$(TAR_BIN) -c src/diff/*.context.* COPYING ChangeLog | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).context.diff.tar.bz2
endif
if OS_HAS_COMPRESS
	$(TAR_BIN) -c src/diff/*.unified.* COPYING ChangeLog | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.Z
	$(TAR_BIN) -c src/diff/*.normal.* COPYING ChangeLog | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.Z
	$(TAR_BIN) -c src/diff/*.context.* COPYING ChangeLog | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).context.diff.tar.Z
endif
endif
if OS_HAS_SEVENZIP
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).unified.diff.7z src/diff/*.unified.* COPYING ChangeLog
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).normal.diff.7z src/diff/*.normal.* COPYING ChangeLog
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).context.diff.7z src/diff/*.context.* COPYING ChangeLog
endif
if OS_HAS_ZIP
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).unified.diff.zip src/diff/*.unified.* COPYING ChangeLog
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).normal.diff.zip src/diff/*.normal.* COPYING ChangeLog
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).context.diff.zip src/diff/*.context.* COPYING ChangeLog
endif

	

dist-win32-bin: distdir
	$(SILENT) echo "Creating MSVC binary archives from `pwd`"; 
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.7z
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.zip
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.tar.bz2
if OS_HAS_TAR
if OS_HAS_GZIP
	$(TAR_BIN) -c $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' ` | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).Win32.bin.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR_BIN) -c $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' ` | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).Win32.bin.tar.bz2
endif
if OS_HAS_COMPRESS
endif
endif
if OS_HAS_SEVENZIP
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).Win32.bin.7z $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' `
endif
if OS_HAS_ZIP
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).Win32.bin.zip $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' `
endif


distribution: distdir dist-diff dist-win32-bin 
	$(SILENT) echo "Creating src archives"; 
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.bz2
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.lzma
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.Z
	-rm -f $(ARCHIVE_BASE_NAME).src.7z
	-rm -f $(ARCHIVE_BASE_NAME).src.shar.gz
	-rm -f $(ARCHIVE_BASE_NAME).zip
if OS_HAS_TAR
if OS_HAS_GZIP
	$(TAR_BIN) -c $(distdir) | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).src.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR_BIN) -c $(distdir) | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.bz2
endif
if OS_HAS_LZMA
	$(TAR_BIN) -c $(distdir) | $(LZMA_BIN) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.lzma
endif
if OS_HAS_COMPRESS
	$(TAR_BIN) -c $(distdir) | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).src.tar.Z
endif
endif
if OS_HAS_SEVENZIP
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).src.7z $(distdir)
endif
if OS_HAS_SHAR
if OS_HAS_GZIP
	$(SHAR_BIN) $(distdir) | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).src.shar.gz
endif
endif
if OS_HAS_ZIP
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).src.zip $(distdir)
endif
	$(am__remove_distdir)



#
# [i_a] see the comment at 'DIST_ARCHIVES' why we need to copy the archives to the 'regular'
#       $(distdir).tar.* name(s) here -- if we don't 'make distcheck' will fail very quickly.
#
dist dist-all: distribution



#
# when hook is called, configure has not yet run in the dist dir...
#
dist-hook:
	cd $(top_distdir) 
	$(E) "   ************************************"
	$(E) "   dist-hook"
	$(E) "   ************************************"
	$(SILENT) echo "Copying the archive(s) to standard $(distdir).tar.xxx' names suitable for distcheck"; 
	-rm -f $(distdir).tar.gz
	-rm -f $(distdir).tar.bz2
	-rm -f $(distdir).tar.Z
if OS_HAS_TAR
if OS_HAS_GZIP
	$(LN_S) $(ARCHIVE_BASE_NAME).src.tar.gz $(distdir).tar.gz
endif
if OS_HAS_BZIP2
	$(LN_S) $(ARCHIVE_BASE_NAME).src.tar.bz2 $(distdir).tar.bz2
endif
if OS_HAS_COMPRESS
	$(LN_S) $(ARCHIVE_BASE_NAME).src.tar.Z $(distdir).tar.Z
endif
endif
	


clean-local:
	-rm -f $(distdir).*
	-rm -f $(ARCHIVE_BASE_NAME).*



distcheck-hook:
	$(E) "   ************************************"
	$(E) "   distcheck-hook!"
	$(E) "   ************************************"








.PHONY: reindent depend dist-7z dist-diff dist-maint dist-msvc distribution dist-all


reindent:
	$(SILENT) echo "Reformatting sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) reindent);

depend:
	$(SILENT) echo "Calculating dependencies for the sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) depend);

megatest:
	$(SILENT) echo "Running the regression test:"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) megatest);

test:
	$(SILENT) echo "Running the complete test set (this may take a LONG time):"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) test);


