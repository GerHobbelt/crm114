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


SEVENZIP_OPTIONS=a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on
# SEVENZIP_OPTIONS=a -si




PACKAGE = @PACKAGE_TARNAME@
TAR_FILENAME_POSTFIX = @TAR_FILENAME_POSTFIX@
VER_SUFFIX = @VER_SUFFIX@ 
VERSION = @VERSION@
ARCHIVE_BASE_NAME = @PACKAGE_TARNAME@-@PACKAGE_VERSION@-@TAR_FILENAME_POSTFIX@

SILENT = @SILENT@
DIFF_FLAGS = @DIFF_FLAGS@



DIST_ARCHIVES =
if OS_HAS_TAR
if OS_HAS_GZIP
DIST_ARCHIVES += $(ARCHIVE_BASE_NAME).src.tar.gz
endif
if OS_HAS_BZIP2
DIST_ARCHIVES += $(ARCHIVE_BASE_NAME).src.tar.bz2
endif
if OS_HAS_COMPRESS
DIST_ARCHIVES += $(ARCHIVE_BASE_NAME).src.tar.Z
endif
endif




SUBDIRS = docs examples mailfilter man src tests


# docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = AUTHORS COPYING ChangeLog HACKING README TODO NEWS README.1st


DISTCLEANFILES = 
CLEANFILES = *~ *.bak

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
	
dist-diff: diff	
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
	$(TAR_BIN) -c src/diff/*.unified.* | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.gz
	$(TAR_BIN) -c src/diff/*.normal.* | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.gz
	$(TAR_BIN) -c src/diff/*.context.* | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).context.diff.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR_BIN) -c src/diff/*.unified.* | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.bz2
	$(TAR_BIN) -c src/diff/*.normal.* | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.bz2
	$(TAR_BIN) -c src/diff/*.context.* | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).context.diff.tar.bz2
endif
if OS_HAS_COMPRESS
	$(TAR_BIN) -c src/diff/*.unified.* | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.Z
	$(TAR_BIN) -c src/diff/*.normal.* | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.Z
	$(TAR_BIN) -c src/diff/*.context.* | $(COMPRESS_BIN) -c >$(ARCHIVE_BASE_NAME).context.diff.tar.Z
endif
endif
if OS_HAS_SEVENZIP
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).unified.diff.7z src/diff/*.unified.* 
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).normal.diff.7z src/diff/*.normal.* 
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).context.diff.7z src/diff/*.context.* 
endif
if OS_HAS_ZIP
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).unified.diff.zip src/diff/*.unified.* 
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).normal.diff.zip src/diff/*.normal.* 
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).context.diff.zip src/diff/*.context.* 
endif

	

dist-win32-bin: distdir
	$(SILENT) echo "Creating MSVC binary archives from `pwd`"; 
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.7z
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.zip
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.tar.bz2
if OS_HAS_TAR
if OS_HAS_GZIP
	$(TAR_BIN) -c win32/bin/*.exe win32/bin/*.dll | GZIP=$(GZIP_ENV) $(GZIP_BIN) -c >$(ARCHIVE_BASE_NAME).Win32.bin.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR_BIN) -c win32/bin/*.exe win32/bin/*.dll | $(BZIP2_BIN) -9 -c >$(ARCHIVE_BASE_NAME).Win32.bin.tar.bz2
endif
if OS_HAS_COMPRESS
endif
endif
if OS_HAS_SEVENZIP
	$(SEVENZIP_BIN) $(SEVENZIP_OPTIONS) $(ARCHIVE_BASE_NAME).Win32.bin.7z win32/bin/*.exe win32/bin/*.dll 
endif
if OS_HAS_ZIP
	$(ZIP_BIN) -rq $(ARCHIVE_BASE_NAME).Win32.bin.zip win32/bin/*.exe win32/bin/*.dll 
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




dist dist-all: distribution






distcheck-hook:
	-echo "distcheck-hook!"

#distcheck-local:
#	-echo "distcheck-local!"
# -- doesn't work as advertized in the manual :-(










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


