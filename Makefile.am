# Process this file with automake to produce Makefile.in (in this,
# and all subdirectories).

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007-2008 Ger Hobbelt



# [i_a] instruct AC_LOCAL to correctly include the ./m4 directory too
ACLOCAL_AMFLAGS = -I $(top_srcdir)/m4


SEVEN_ZIP_OPTIONS=a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on
# SEVEN_ZIP_OPTIONS=a -si




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


MSVC_PRJ_FILES = win32/config_win32.h win32/config_win32.h.in \
	win32/crm114.sln win32/crm114.vcproj win32/cssdiff.vcproj win32/osbf-util.vcproj \
	win32/resource.h win32/crm114.ico win32/cssmerge.vcproj win32/cssutil.vcproj \
	win32/crm_bmp_prof.vcproj win32/crm_test_VT.vcproj \
	win32/libcrm114.vcproj win32/libcrm114_NG.vcproj win32/libcrm114_t1.vcproj \
	win32/getopt.h win32/getopt_ex.h win32/getopt.c win32/getopt_ex.c \
	win32/crm_win32_port.c \
	win32/megatest.bat win32/crm114rc.h win32/crm114rc.rc

EXTRA_DIST = bootstrap crm114.lsm.in HACKING README.1st \
	distribute_files_from_src_dir.sh fix_crlf.sh \
	m4/ac_environ.m4 m4/ac_func_memmove.m4 m4/ac_func_memmem.m4 \
	m4/ac_func_snprintf.m4 m4/ax_arg_enable.m4 \
	m4/ax_cflags_warn_all.m4 m4/vl_lib_readline.m4 \
	m4/ax_cflags_gcc_option.m4 m4/ax_diff_option.m4 \
	m4/ac___function___c99_vpredef_vars.m4 \
	m4/ac_nsec_stat_fields.m4 m4/ax_cc_pragma_pack.m4 \
	m4/ax_gnu_time.m4 \
	mk_distros.sh \
	config_vpath.sh \
	uncrustify.cfg \
	cleanup.bat autogen.sh CREDITS README.win32 BUGS \
	README.maintainer MAILINGLIST \
	$(MSVC_PRJ_FILES)

WIN32_BIN_EXTRA = \
	$(top_srcdir)/CREDITS \
	$(top_srcdir)/README.win32 \
	$(top_srcdir)/BUGS \
	$(top_srcdir)/MAILINGLIST \
	$(top_srcdir)/HACKING \
	$(top_srcdir)/README.1st \
	$(top_srcdir)/docs/classify_details.txt \
	$(top_srcdir)/docs/COLOPHON.txt \
	$(top_srcdir)/docs/CRM114_Mailfilter_HOWTO.txt \
	$(top_srcdir)/docs/FAQ.txt \
	$(top_srcdir)/docs/INTRO.txt \
	$(top_srcdir)/docs/knownbugs.txt \
	$(top_srcdir)/docs/QUICKREF.txt \
	$(top_srcdir)/docs/README \
	$(top_srcdir)/docs/VT_generic_Vector_Tokenization.txt \
	mailfilter/classifymail.crm \
	mailfilter/mailfilter.cf \
	mailfilter/mailfilter.crm \
	mailfilter/maillib.crm \
	mailfilter/mailreaver.crm \
	mailfilter/mailtrainer.crm \
	mailfilter/priolist.mfp \
	mailfilter/rewrites.mfp \
	mailfilter/blacklist.mfp \
	mailfilter/whitelist.mfp \
	mailfilter/whitelist.mfp.example



if MAINTAINER_MODE

# ChangeLog: NEWS
# 	-( test "X$(cvs2cl)" != "X" && $(cvs2cl) --prune --accum -U ../CVSROOT/users )

BillY-sync:
	$(SILENT) distribute_files_from_src_dir.sh

endif



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
	$(TAR) -c $(distdir) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).src.tar.gz
endif
endif
	$(am__remove_distdir)

dist-bzip2: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.bz2
if OS_HAS_TAR
if OS_HAS_BZIP2
	$(TAR) -c $(distdir) | $(BZIP2) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.bz2
endif
endif
	$(am__remove_distdir)

dist-lzma: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.lzma
if OS_HAS_TAR
if OS_HAS_LZMA
	$(TAR) -c $(distdir) | $(LZMA) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.lzma
endif
endif
	$(am__remove_distdir)

dist-tarZ: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.tar.Z
if OS_HAS_TAR
if OS_HAS_COMPRESS
	$(TAR) -c $(distdir) | $(COMPRESS) -c >$(ARCHIVE_BASE_NAME).src.tar.Z
endif
endif
	$(am__remove_distdir)

dist-7z: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.7z
if OS_HAS_SEVEN_ZIP
	$(SEVEN_ZIP) $(SEVEN_ZIP_OPTIONS) $(ARCHIVE_BASE_NAME).src.7z $(distdir)
endif
	$(am__remove_distdir)

dist-shar: distdir
	-rm -f $(ARCHIVE_BASE_NAME).src.shar.gz
if OS_HAS_SHAR
if OS_HAS_GZIP
	$(SHAR) $(distdir) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).src.shar.gz
endif
endif
	$(am__remove_distdir)

dist-zip: distdir
	-rm -f $(ARCHIVE_BASE_NAME).zip
if OS_HAS_ZIP
	$(ZIP) -rq $(ARCHIVE_BASE_NAME).src.zip $(distdir)
endif
	$(am__remove_distdir)



diff:
	$(SILENT) echo "Creating diff archives";
	(cd src && $(MAKE) $(AM_MAKEFLAGS) diff);


# Note: the COPYING+ChangeLog in there is just to make sure the dist-diff
#       archivers have something to archive; if not, they'll fail in 'make distcheck' at least.
#       And not everyone has the same setup as me, so @ORIGINAL_BILLY_DISTRO_DIR@
#       may point at a bogus directory for them!
EXTRA_4_DIFF_DIST = $(top_srcdir)/COPYING $(top_srcdir)/ChangeLog

dist-diff: diff $(EXTRA_4_DIFF_DIST)
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
	$(TAR) -c src/diff/*.unified.* $(EXTRA_4_DIFF_DIST) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.gz
	$(TAR) -c src/diff/*.normal.* $(EXTRA_4_DIFF_DIST) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.gz
	$(TAR) -c src/diff/*.context.* $(EXTRA_4_DIFF_DIST) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).context.diff.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR) -c src/diff/*.unified.* $(EXTRA_4_DIFF_DIST) | $(BZIP2) -9 -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.bz2
	$(TAR) -c src/diff/*.normal.* $(EXTRA_4_DIFF_DIST) | $(BZIP2) -9 -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.bz2
	$(TAR) -c src/diff/*.context.* $(EXTRA_4_DIFF_DIST) | $(BZIP2) -9 -c >$(ARCHIVE_BASE_NAME).context.diff.tar.bz2
endif
if OS_HAS_COMPRESS
	$(TAR) -c src/diff/*.unified.* $(EXTRA_4_DIFF_DIST) | $(COMPRESS) -c >$(ARCHIVE_BASE_NAME).unified.diff.tar.Z
	$(TAR) -c src/diff/*.normal.* $(EXTRA_4_DIFF_DIST) | $(COMPRESS) -c >$(ARCHIVE_BASE_NAME).normal.diff.tar.Z
	$(TAR) -c src/diff/*.context.* $(EXTRA_4_DIFF_DIST) | $(COMPRESS) -c >$(ARCHIVE_BASE_NAME).context.diff.tar.Z
endif
endif
if OS_HAS_SEVEN_ZIP
	$(SEVEN_ZIP) $(SEVEN_ZIP_OPTIONS) $(ARCHIVE_BASE_NAME).unified.diff.7z src/diff/*.unified.* $(EXTRA_4_DIFF_DIST)
	$(SEVEN_ZIP) $(SEVEN_ZIP_OPTIONS) $(ARCHIVE_BASE_NAME).normal.diff.7z src/diff/*.normal.* $(EXTRA_4_DIFF_DIST)
	$(SEVEN_ZIP) $(SEVEN_ZIP_OPTIONS) $(ARCHIVE_BASE_NAME).context.diff.7z src/diff/*.context.* $(EXTRA_4_DIFF_DIST)
endif
if OS_HAS_ZIP
	$(ZIP) -rq $(ARCHIVE_BASE_NAME).unified.diff.zip src/diff/*.unified.* $(EXTRA_4_DIFF_DIST)
	$(ZIP) -rq $(ARCHIVE_BASE_NAME).normal.diff.zip src/diff/*.normal.* $(EXTRA_4_DIFF_DIST)
	$(ZIP) -rq $(ARCHIVE_BASE_NAME).context.diff.zip src/diff/*.context.* $(EXTRA_4_DIFF_DIST)
endif



dist-win32-bin: distdir
	$(SILENT) echo "Creating MSVC binary archives from `pwd`";
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.7z
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.zip
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).Win32.bin.tar.bz2
	-mkdir win32/bin
if OS_HAS_TAR
if OS_HAS_GZIP
	$(TAR) -c $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' ` | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).Win32.bin.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR) -c $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' ` | $(BZIP2) -9 -c >$(ARCHIVE_BASE_NAME).Win32.bin.tar.bz2
endif
if OS_HAS_COMPRESS
endif
endif
if OS_HAS_SEVEN_ZIP
	$(SEVEN_ZIP) $(SEVEN_ZIP_OPTIONS) $(ARCHIVE_BASE_NAME).Win32.bin.7z $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' `
endif
if OS_HAS_ZIP
	$(ZIP) -rq $(ARCHIVE_BASE_NAME).Win32.bin.zip $(WIN32_BIN_EXTRA) `find win32/bin/ -name '*.exe' -o -name '*.dll' `
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
	$(TAR) -c $(distdir) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).src.tar.gz
endif
if OS_HAS_BZIP2
	$(TAR) -c $(distdir) | $(BZIP2) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.bz2
endif
if OS_HAS_LZMA
	$(TAR) -c $(distdir) | $(LZMA) -9 -c >$(ARCHIVE_BASE_NAME).src.tar.lzma
endif
if OS_HAS_COMPRESS
	$(TAR) -c $(distdir) | $(COMPRESS) -c >$(ARCHIVE_BASE_NAME).src.tar.Z
endif
endif
if OS_HAS_SEVEN_ZIP
	$(SEVEN_ZIP) $(SEVEN_ZIP_OPTIONS) $(ARCHIVE_BASE_NAME).src.7z $(distdir)
endif
if OS_HAS_SHAR
if OS_HAS_GZIP
	$(SHAR) $(distdir) | GZIP=$(GZIP_ENV) $(GZIP) -c >$(ARCHIVE_BASE_NAME).src.shar.gz
endif
endif
if OS_HAS_ZIP
	$(ZIP) -rq $(ARCHIVE_BASE_NAME).src.zip $(distdir)
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
#
# EXTRA: if the 'bin' subdir does not exist for some reason, do not fail due to that omission; just create
# it. This is done to ensure the dist-win32 target builds within a 'make distcheck' run.
#
#dist-hook: dist-gzip dist-bzip2 dist-tarZ
# ^^^ you'll get an infinite loop when you add these dependencies!
dist-hook:
	cd $(top_distdir)
	$(E) "   ************************************"
	$(E) "   dist-hook"
	$(E) "   ************************************"
	-mkdir win32/bin
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
	cd $(distdir)/_build \
		&& echo "rm -rf xyz"








.PHONY: reindent depend dist-7z dist-diff dist-maint dist-msvc distribution dist-all diff


reindent:
	$(E) "Reformatting sources";
	(cd src && $(MAKE) $(AM_MAKEFLAGS) reindent);

depend:
	$(E) "Calculating dependencies for the sources";
	(cd src && $(MAKE) $(AM_MAKEFLAGS) depend);

megatest:
	$(E) "Running the regression test:";
	(cd src && $(MAKE) $(AM_MAKEFLAGS) megatest);

megatest_ng:
	$(E) "Running the regression test:";
	(cd src && $(MAKE) $(AM_MAKEFLAGS) megatest_ng);

test:
	$(E) "Running the complete test set (this may take a LONG time):";
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test);

timing_tests:
	$(E) "Running the timing test set (this may take a LONG time):";
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) timing_tests);

# original BillY test script:
test_megatest:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test_megatest);

# 'NextGen', i.e. an edited ('improved') version of BillY's test script:
test_megatest_ng:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test_megatest_ng);

# series of basic regression tests for CRM114:
test_basics:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test_basics);

# test more complex CRM scripts
test_other:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test_other);

# test the mailfilter/mailreaver/mailtrainer scripts
test_fundamental_mail:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test_fundamental_mail);


test_classifiers:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) test_classifiers);

script:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) script);

initial_tests1:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) initial_tests1);

fringe_cases_compiler_tests1:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) fringe_cases_compiler_tests1);

# slower tests:
initial_tests2:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) initial_tests2);

further_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) further_tests);

all_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) all_classifier_tests);

# classifier tests grouped:
markovian_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) markovian_classifier_tests);

OSBF_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) OSBF_classifier_tests);

OSB_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) OSB_classifier_tests);

Hyperspace_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) Hyperspace_classifier_tests);

Bit_Entropy_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) Bit_Entropy_classifier_tests);

FSCM_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) FSCM_classifier_tests);

Neural_Net_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) Neural_Net_classifier_tests);

SVM_SKS_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) SVM_SKS_classifier_tests);

Correlator_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) Correlator_classifier_tests);

CLUMP_classifier_tests:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) CLUMP_classifier_tests);

mk_test_sets:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) mk_test_sets);





