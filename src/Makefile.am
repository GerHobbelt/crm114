## Process this file with automake to produce Makefile.in

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007 Ger Hobbelt


#
#    Makefile for CRM114
#

DISTCLEANFILES = 
CLEANFILES = *.o *~ *.bak



PACKAGE = @PACKAGE_TARNAME@
TAR_FILENAME_POSTFIX = @TAR_FILENAME_POSTFIX@
ARCHIVE_BASE_NAME = $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-$(TAR_FILENAME_POSTFIX)


# VER_SUFFIX defines a version suffix for our installed executables, 
# handy when you want many versions of CRM114 coexisting.
#
# e.g.: VER_SUFFIX=27B-6, then if you make install, you get crm-27b-6
# 
VER_SUFFIX = @VER_SUFFIX@ 

#
#      The following forces the compiler to be GCC.  If you have trouble
#      with your default compiler, and you want to force GCC, uncomment it.
# CC=gcc
#
#     What version is this software (PLEASE don't change this just 
#     to mess with my brain. - wsy)
#
VERSION = @VERSION@
PREV_VERSION=20070917-BlameGerHobbelt
#PREV_VERSION=20070830-BlameGerHobbelt
#PREV_VERSION=20070828-BlameGerHobbelt
#PREV_VERSION=20070827-BlameGerHobbelt
#PREV_VERSION=20070826-BlameGerHobbelt
#PREV_VERSION = 20070815-BlameBadData
#PREV_VERSION = 20070810-BlameTheSegfault
#PREV_VERSION = 20070807-BlameTheInterns
#PREV_VERSION = 20070505-BlameIdiocy
#PREV_VERSION = 20070428-BlameSpamConf
#PREV_VERSION = 20061103-BlameDalkey
#PREV_VERSION = 20061010-BlameBratko
#PREV_VERSION = 20060920-BlameNico
#PREV_VERSION = 20060704a-BlameRobert
#PREV_VERSION = 20060619-MargarineBeast
#PREV_VERSION = 20060611-ButterBeast
#PREV_VERSION = 20060611-SomewhatTamedBeast
#PREV_VERSION = 20060606-VersionOfTheBeast
#PREV_VERSION = 20060429-ReaverReturn
#PREV_VERSION = 20060118-BlameTheReavers
#PREV_VERSION = 20051231a-BlameBarryAndPaolo
#PREV_VERSION = 20051001-BlameRaulMiller
#PREV_VERSION = crm114-20050721-BlameNeilArmstrong
# VERSION += [$(TRE_TARFILE)]

SILENT = @SILENT@

#
#
#      Are we compiling on a POSIX system or a Windows system?  NOTE:
#       WINDOWS IS UNSUPPORTED BY THE AUTHOR.  THE AUTHOR WILL
#	ACCEPT REASONABLE-LOOKING PATCHES BUT WINDOWS-ONLY 
#       BUG REPORTS THAT DO NOT REPRODUCE UNDER LINUX ET AL 
#       SIMPLY _CANNOT_ BE WORKED AS I HAVE NO WINDOWS MACHINES.  
#       SO, IF YOU FIND A WINDOWS-ONLY BUG, PLEASE PUT IT ON THE 
#       MAILING LIST SO WINDOWS PEOPLE CAN WORK THE BUG.
#
#SYSTEMTYPE = POSIX
#SYSTEMTYPE = WIN32
#
#
#  Tell the compiler full optimization "-O3", add debugging info "-g", 
#  and warn on all warnable things "-Wall".  Someday add -Wextra for
#  really picayune stuff.
#  
AM_CFLAGS = @AM_CFLAGS@
# AM_CFLAGS += -g -O3
#     turn this one on for no optimization, but debugging and time profiling.
#AM_CFLAGS += -g -O0 -Wall -pg -CB
#     turn this one on for really picayune debugging.

AM_CPPFLAGS = @AM_CPPFLAGS@
# AM_CPPFLAGS += -Wall
# AM_CPPFLAGS += -Wextra 
# AM_CPPFLAGS += -pedantic

#
#   Choose between static and dynamic linking (we recommend static linking)
#   Comment this out if you want dynamic linking
#
#    BSD users - do NOT use -static-libgcc!!!
AM_LDFLAGS = @AM_LDFLAGS@
#    use this one if you want program profiling.
#AM_LDFLAGS += -static -static-libgcc -pg
#
#   Any standard install flags?  We nominally use protection 755
INSTALLFLAGS = @INSTALLFLAGS@
INSTALLFLAGS += -m 755
#
#        uncomment the next line if you want to "strip" the debugger info 
#        from the binaries when installing.  This speeds up load, but 
#        you won't be able to submit meaningful CRM114 engine bug reports.
#        Default is don't strip.
# INSTALLFLAGS += -s
#
#   Define the TRE directory (used only for building distributions- note that
#   this must be HAND-UPDATED when new versions of TRE come out  )
TRE_TARFILE = tre-0.7.5
#
# ---------   If you're compiling under *BSD, check these out:
#
#    Simson Garfinkel suggests that you
#    uncomment the following to get a BSD-sane environment.  Leave them
#    untouched (commented out) for Linux builds.
# Add for FreeBSD
#CFLAGS += -I/usr/local/include
#LDFLAGS += -L/usr/local/lib
#LIBS += -lintl -liconv
#LDFLAGS += -dynamic-libgcc
#
#             Jeff Rice suggests the following for FreeBSD:
#CFLAGS += -I/usr/local/include -I${HOME}/include
#LDFLAGS += -L/usr/local/lib -L${HOME}/lib
#LIBS += -lintl -liconv
#



#
#
# ------------------ end of *BSD stuff
#
#
#    End of user-configurable options... if you modify anything below
#    this line, you risk early insanity and blindness.
#
#

DIFF_FLAGS = @DIFF_FLAGS@

ECHO = echo
E = $(SILENT) $(ECHO)


#
#
#   These are the files needed to build the CRM114 engine; they don't 
#   include the side utilities
#
CFILES = crm_compiler.c crm_errorhandlers.c \
	crm_math_exec.c crm_var_hash_table.c crm_expandvar.c \
	crm_stmt_parser.c crm_vector_tokenize.c \
	crm_expr_alter.c crm_expr_match.c crm_css_maintenance.c \
	crm_markovian.c crm_osb_bayes.c crm_osb_hyperspace.c \
	crm_correlate.c crm_osb_winnow.c crm_winnow_maintenance.c \
	crm_osbf_bayes.c crm_osbf_maintenance.c crm_bit_entropy.c \
	crm_neural_net.c crm_expr_clump.c \
	crm_expr_window.c crm_expr_isolate.c crm_expr_file_io.c \
	crm_expr_syscall.c crm_expr_classify.c crm_expr_translate.c \
	crm_exec_engine.c crm_debugger.c crm_str_funcs.c \
	crm_preprocessor.c crmregex_gnu.c crmregex_tre.c \
	crm_util_errorhandlers.c crm_stats.c crm_expr_sks.c \
	crm_expr_svm.c crm_expr_fscm.c \
	crm_strcompress.c crm_expr_clump_nn.c \
	crm_generic_port.c crm_stdio_port.c \
	crm_versioning.c

HFILES = crm114_sysincludes.h crm114_structs.h crm114_config.h \
	crm114.h crm114_osbf.h crm_expr_clump_nn.h config.h \
	config_BillY.h crm_mjt_qsort.h

EXTRA_DEPS = Makefile

OFILES = crm_compiler.o crm_errorhandlers.o \
	crm_math_exec.o crm_var_hash_table.o crm_expandvar.o \
	crm_stmt_parser.o crm_vector_tokenize.o \
	crm_expr_alter.o crm_expr_match.o crm_css_maintenance.o \
	crm_markovian.o crm_osb_bayes.o crm_osb_hyperspace.o \
	crm_correlate.o crm_osb_winnow.o crm_winnow_maintenance.o \
	crm_osbf_bayes.o crm_osbf_maintenance.o crm_bit_entropy.o \
	crm_neural_net.o crm_expr_clump.o \
	crm_expr_window.o crm_expr_isolate.o crm_expr_file_io.o \
	crm_expr_syscall.o crm_expr_classify.o crm_expr_translate.o \
	crm_exec_engine.o crm_debugger.o crm_str_funcs.o \
	crm_preprocessor.o \
	crm_expr_sks.o crm_stats.o crm_expr_svm.o crm_expr_fscm.o \
	crm_strcompress.o crm_expr_clump_nn.o \
	crm_generic_port.o crm_stdio_port.o \
	crm_versioning.o



UTILFILES = crm_main.c osbf-util.c cssutil.c cssdiff.c cssmerge.c \
	testvectok.c

CRMFILES = mailfilter.crm mailfilter.cf \
	mailreaver.crm mailtrainer.crm \
	maillib.crm shuffle.crm \
	rewriteutil.crm \
	matchtest.crm windowtest.crm overalterisolatedtest.crm exectest.crm \
	learntest.crm classifytest.crm \
	escapetest.crm argtest.crm beeptest.crm skudtest.crm aliustest.crm \
	nestaliustest.crm eval_infiniteloop.crm \
	traptest.crm fataltraptest.crm uncaughttraptest.crm \
	inserttest_a.crm inserttest_b.crm inserttest_c.crm \
	backwardstest.crm approxtest.crm \
	mathalgtest.crm mathrpntest.crm \
	indirecttest.crm translate_tr.crm zz_translate_test.crm \
	rewritetest.crm test_rewrites.mfp \
	randomiotest.crm isolate_reclaim_test.crm \
	match_isolate_test.crm match_isolate_reclaim.crm \
	call_return_test.crm defaulttest.crm \
	shroud.crm quine.crm \
	bracktest.crm unionintersecttest.crm \
	statustest.crm windowtest_fromvar.crm paolo_overvars.crm \
	paolo_ov2.crm paolo_ov3.crm paolo_ov4.crm paolo_ov5.crm \
	pad.crm calc.crm \
	gatlingsort.crm tenfold_validate.crm \
	test_svm.crm \
	classifymail.crm \
	tokendelimiterbugtest.crm bracesbugtest.crm


TEXTFILES = COLOPHON.txt pad.dat \
	QUICKREF.txt INTRO.txt knownbugs.txt FAQ.txt things_to_do.txt \
	classify_details.txt README CRM114_Mailfilter_HOWTO.txt \
	procmailrc.recipe reto_procmail_recipe.recipe \
	priolist.mfp blacklist.mfp whitelist.mfp rewrites.mfp \
	priolist.mfp.example blacklist.mfp.example whitelist.mfp.example \
	crm114-mode.el \
	crm114.spec \
	inoc_passwd.txt \
	GPL-License.txt \
	rename-gz \
	megatest.sh megatest_knowngood.log megatest_knowngood.orginal.log \
	make_BillY.sh Makefile.BillY



EXTRA_DIST = indent.sh.in diff_em.sh.in \
	$(UTILFILES) \
	$(CRMFILES) \
	$(TEXTFILES)




bin_PROGRAMS = crm114 cssutil osbf-util cssdiff cssmerge testvectok


crm114_SOURCES = $(CFILES) $(HFILES) crm_main.c 1010-megatest-diff.txt \
		merge-results-29-oct-2007.txt x.bak

cssutil_SOURCES = cssutil.c \
	crm_css_maintenance.c crm_util_errorhandlers.c crm_str_funcs.c crm_generic_port.c crm_stdio_port.c \
	crm_versioning.c \
	$(HFILES) $(EXTRA_DEPS)

osbf_util_SOURCES = osbf-util.c \
	crm_osbf_maintenance.c crm_util_errorhandlers.c crm_str_funcs.c crm_generic_port.c crm_stdio_port.c \
	crm_versioning.c \
	$(HFILES) $(EXTRA_DEPS)

cssdiff_SOURCES = cssdiff.c \
	crm_util_errorhandlers.c crm_str_funcs.c crm_generic_port.c crm_stdio_port.c \
	crm_versioning.c \
	$(HFILES) $(EXTRA_DEPS)

cssmerge_SOURCES = cssmerge.c \
	crm_util_errorhandlers.c crm_str_funcs.c crm_generic_port.c crm_stdio_port.c \
	crm_versioning.c \
	$(HFILES) $(EXTRA_DEPS)

testvectok_SOURCES = $(CFILES) $(HFILES) $(EXTRA_DEPS) testvectok.c



#
#

done_all: 
	$(E) ""
	$(E) "   Done."
	$(E) ""
	$(E) "   ***  Type \"make help\" to see what else you can make."
	$(E) ""

help:
	$(E) ""
	$(E) "    You can make the following things:"
	$(E) ""
	$(E) "    make   -or-   make all   - builds the binaries.  This uses the"
	$(E) "                               TRE regex library"
	$(E) "    make crm114_gnu          - builds version with GNU regex instead"
	$(E) "                               of the TRE library."
	$(E) "    make install             - builds and installs the binaries."
	$(E) "                               You must be root to do this."
	$(E) "    make uninstall           - uninstalls the binaries.  You must"
	$(E) "                               be root to do this."
	$(E) "    make megatest            - runs the full test suite ( takes"
	$(E) "                               about ~1 minute, TRE version _ONLY_! )"
	$(E) "    make clean               - deletes all .o and executables."
	$(E) "                               Don't do this on a binary kit!"
	$(E) "    make cssfiles            - builds a set of .css files from data"
	$(E) "                               in ./good.dir/* and ./spam.dir/*"
	$(E) "    make distribution        - builds the .gzballs for distribution"
	$(E) "                               of with your mods built in."

#
#   Note:  You've chosen to make a GNU regex library CRM114.  
#       Although CRM114 will work mostly correctly when
#	linked against the GNU regex library, a lot of functionality
#       won't be available, and "make megatest" will fail.
#
crm114_gnu: $(OFILES) crmregex_gnu.o crm_main.o
	$(E) ""
	$(E) "   Note:  You've chosen to make a GNU regex library CRM114."
	$(E) "       Although CRM114 will work mostly correctly when"
	$(E) "       linked against the GNU regex library, a lot of functionality"
	$(E) "       won't be available, and \"make megatest\" will fail."
	$(E) ""
	$(CC) $(LDFLAGS) $(LIBS) crm_main.o $(OFILES) \
	  crmregex_gnu.o \
	  -o crm114_gnu


#   
#   Now building the TRE regex version of CRM114.  If you don't have
#   TRE already, you will need to fetch it from the website and
#   then ./configure --enable-static to get the static TRE libs,
#   then build it, add /usr/local/lib to /etc/ld.so.conf, then 
#   to set the library up.
#
crm114: $(OFILES) crmregex_tre.o crm_main.o
	$(E) ""
	$(E) "   Now building the TRE regex version of CRM114.  If you don't have"
	$(E) "   TRE already, you will need to fetch it from the website and"
	$(E) "   then ./configure --enable-static to get the static TRE libs,"
	$(E) "   then build it, add /usr/local/lib to /etc/ld.so.conf, then"
	$(E) "   to set the library up."
	$(E) ""
	$(CC) $(LDFLAGS) $(LIBS) crm_main.o $(OFILES) \
	  crmregex_tre.o \
	  -o crm114
	$(E) ""
	$(E) "   Done.  Type \"make help\" to see what else you can make."
	$(E) ""



#
#     Note To The Reader:  I'd really _love_ to have all of these
#     stanzas for each of the .c code modules be taken care of by
#     something that uses $(CFILE), but it isn't.  Maybe the autoconf
#     version would have that- but one change at a time, eh?
#


crm_main.o: crm_main.c \
	$(HFILES) $(EXTRA_DEPS)

crm_compiler.o: crm_compiler.c \
	$(HFILES) $(EXTRA_DEPS)

crm_errorhandlers.o: crm_errorhandlers.c \
	$(HFILES) $(EXTRA_DEPS)

crm_exec_engine.o: crm_exec_engine.c \
	$(HFILES) $(EXTRA_DEPS)

crm_preprocessor.o: crm_preprocessor.c \
	$(HFILES) $(EXTRA_DEPS)

crm_var_hash_table.o: crm_var_hash_table.c \
	$(HFILES) $(EXTRA_DEPS)

crm_math_exec.o: crm_math_exec.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expandvar.o: crm_expandvar.c \
	$(HFILES) $(EXTRA_DEPS)

crm_stmt_parser.o: crm_stmt_parser.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_alter.o: crm_expr_alter.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_match.o: crm_expr_match.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_file_io.o: crm_expr_file_io.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_syscall.o: crm_expr_syscall.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_classify.o: crm_expr_classify.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_clump_nn.o: crm_expr_clump_nn.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_translate.o: crm_expr_translate.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_window.o: crm_expr_window.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_isolate.o: crm_expr_isolate.c \
	$(HFILES) $(EXTRA_DEPS)

crm_css_maintenance.o: crm_css_maintenance.c \
	$(HFILES) $(EXTRA_DEPS)

crm_winnow_maintenance.o: crm_winnow_maintenance.c \
	$(HFILES) $(EXTRA_DEPS)

crm_osbf_maintenance.o: crm_osbf_maintenance.c \
	$(HFILES) $(EXTRA_DEPS)

crm_markovian.o: crm_markovian.c \
	$(HFILES) $(EXTRA_DEPS)

crm_osb_bayes.o: crm_osb_bayes.c \
	$(HFILES) $(EXTRA_DEPS)

crm_osbf_bayes.o: crm_osbf_bayes.c \
	$(HFILES) $(EXTRA_DEPS)

crm_osb_hyperspace.o: crm_osb_hyperspace.c \
	$(HFILES) $(EXTRA_DEPS)

crm_strcompress.o: crm_strcompress.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_sks.o: crm_expr_sks.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_svm.o: crm_expr_svm.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_fscm.o: crm_expr_fscm.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_clump.o: crm_expr_clump.c \
	$(HFILES) $(EXTRA_DEPS)

crm_stats.o: crm_stats.c \
	$(HFILES) $(EXTRA_DEPS)

crm_neural_net.o: crm_neural_net.c \
	$(HFILES) $(EXTRA_DEPS)

#crm_osb_neural.o: crm_osb_neural.c \
#	$(HFILES) $(EXTRA_DEPS)

crm_bit_entropy.o: crm_bit_entropy.c \
	$(HFILES) $(EXTRA_DEPS)

crm_osb_winnow.o: crm_osb_winnow.c \
	$(HFILES) $(EXTRA_DEPS)

crm_correlate.o: crm_correlate.c \
	$(HFILES) $(EXTRA_DEPS)

crm_debugger.o: crm_debugger.c \
	$(HFILES) $(EXTRA_DEPS)

crm_str_funcs.o: crm_str_funcs.c \
	$(HFILES) $(EXTRA_DEPS)

crm_vector_tokenize.o: crm_vector_tokenize.c \
	$(HFILES) $(EXTRA_DEPS)

crmregex_tre.o: crmregex_tre.c \
	$(HFILES) $(EXTRA_DEPS)

crmregex_gnu.o: crmregex_gnu.c \
	$(HFILES) $(EXTRA_DEPS)

crm_util_errorhandlers.o: crm_util_errorhandlers.c \
	$(HFILES) $(EXTRA_DEPS)

cssutil.o: cssutil.c  \
	$(HFILES) $(EXTRA_DEPS)

osbf-util.o: osbf-util.c  \
	$(HFILES) $(EXTRA_DEPS)

cssdiff.o: cssdiff.c  \
	$(HFILES) $(EXTRA_DEPS)

cssmerge.o: cssmerge.c  \
	$(HFILES) $(EXTRA_DEPS)

testvectok.o: testvectok.c  \
	$(HFILES) $(EXTRA_DEPS)

crm_generic_port.o: crm_generic_port.c \
	$(HFILES) $(EXTRA_DEPS)

crm_stdio_port.o: crm_stdio_port.c \
	$(HFILES) $(EXTRA_DEPS)

crm_versioning.o: crm_versioning.c \
	$(HFILES) $(EXTRA_DEPS)


cssutil: cssutil.o crm_css_maintenance.o crm_util_errorhandlers.o crm_str_funcs.o crm_generic_port.o crm_stdio_port.o crm_versioning.o
	$(CC) $(LDFLAGS) $(LIBS) cssutil.o \
		crm_css_maintenance.o \
		crm_util_errorhandlers.o \
		crm_str_funcs.o \
		crm_generic_port.o \
		crm_stdio_port.o \
		crm_versioning.o \
		-o cssutil
	$(E) ""
	$(E) "   Done.  Type \"make help\" to see what else you can make."
	$(E) ""

osbf-util: osbf-util.o crm_osbf_maintenance.o crm_util_errorhandlers.o crm_str_funcs.o crm_generic_port.o crm_stdio_port.o crm_versioning.o
	$(CC) $(LDFLAGS) $(LIBS) osbf-util.o \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		crm_osbf_maintenance.o \
		crm_generic_port.o \
		crm_stdio_port.o \
		crm_versioning.o \
		-o osbf-util
	$(E) ""
	$(E) "   Done.  Type \"make help\" to see what else you can make."
	$(E) ""

cssdiff: cssdiff.o crm_util_errorhandlers.o crm_str_funcs.o crm_generic_port.o crm_stdio_port.o crm_versioning.o
	$(CC) $(LDFLAGS) $(LIBS) cssdiff.o \
		crm_util_errorhandlers.o \
		crm_str_funcs.o \
		crm_generic_port.o \
		crm_stdio_port.o \
		crm_versioning.o \
		-o cssdiff
	$(E) ""
	$(E) "   Done.  Type \"make help\" to see what else you can make."
	$(E) ""

cssmerge: cssmerge.o crm_util_errorhandlers.o crm_str_funcs.o crm_generic_port.o crm_stdio_port.o crm_versioning.o
	$(CC) $(LDFLAGS) $(LIBS) cssmerge.o \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		crm_generic_port.o \
		crm_stdio_port.o \
		crm_versioning.o \
		-o cssmerge
	$(E) ""
	$(E) "   Done.  Type \"make help\" to see what else you can make."
	$(E) ""


testvectok: testvectok.o $(OFILES) crmregex_tre.o
	$(CC) $(LDFLAGS) $(LIBS) testvectok.o \
		$(OFILES) \
		crmregex_tre.o \
		-o testvectok
	$(E) ""
	$(E) "   Done.  Type \"make help\" to see what else you can make."
	$(E) ""


clean:
	-rm -f crm114
	-rm -f cssutil
	-rm -f osbf-util
	-rm -f osbfutil
	-rm -f cssdiff
	-rm -f cssmerge
	-rm -f testvectok
	-rm -f crm114
	-rm -f crm114_gnu
	-rm -f crm114_tre
	-rm -f *.o
	$(E) "   if the next line doesn't work in your version of make,"
	$(E) "   don't worry.  cd down into the TRE directory and 'make clean' manually"
	-( [ "$(TRE_TARFILE)" ] && cd $(TRE_TARFILE) && make clean )


install: install_engine install_utils
	$(E) ""
	$(E) "    Done.  Type "make help" to see what else you can make."
	$(E) ""


#
#    Installing Utilities
#	
install_utils: cssmerge cssutil cssdiff osbf-util
	$(E) ""
	$(E) "    Installing Utilities"
	$(E) ""
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	install $(INSTALLFLAGS) cssdiff $(DESTDIR)$(bindir)/cssdiff$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssmerge $(DESTDIR)$(bindir)/cssmerge$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssutil $(DESTDIR)$(bindir)/cssutil$(VER_SUFFIX)
	install $(INSTALLFLAGS) osbf-util $(DESTDIR)$(bindir)/osbf-util$(VER_SUFFIX)
	$(E) "   if you don't have EMACS installed in the default place, you"
	$(E) "   won't get the crm114-mode.el automatically."
	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)



# use different name so we can keep it around along with default one
# and same VER_SUFFIX postfix 

install_engine: crm114
	$(E) ""
	$(E) "    Installing engine"
	$(E) ""
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	install $(INSTALLFLAGS) crm114 $(DESTDIR)$(bindir)/crm$(VER_SUFFIX)


install_binary_only: crm114 cssdiff cssmerge cssutil osbf-util
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	install $(INSTALLFLAGS) crm114 $(DESTDIR)$(bindir)/crm$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssdiff $(DESTDIR)$(bindir)/cssdiff$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssmerge $(DESTDIR)$(bindir)/cssmerge$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssutil $(DESTDIR)$(bindir)/cssutil$(VER_SUFFIX)	
	install $(INSTALLFLAGS) osbf-util $(DESTDIR)$(bindir)/osbf-util$(VER_SUFFIX)
	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


install-exec-hook: crm114
	cd $(DESTDIR)$(bindir) && $(LN_S) crm114 crm


uninstall-hook:
	rm $(DESTDIR)$(bindir)/crm


uninstall:
	-rm -rf $(DESTDIR)$(bindir)/crm$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/crmg$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/crma$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/cssdiff$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/cssmerge$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/cssutil$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/osbf-util$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(bindir)/osbfutil$(VER_SUFFIX)
	-rm -rf $(DESTDIR)$(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


#
#        This runs a moderately interesting set of base tests 
#        to exercise much of CRM114 under TRE.  This takes about
#        1 minute to run on a 1.6 GHz Pentium-M laptop.  Please
#	 be patient; you (hopefully) won't see anything till the
#	 full set of tests complete.  If you didn't use TRE, all 
#        bets are off.
#
#	Lines of output that start with OK_IF_mumble are allowed
#       to change values.  No other lines should.  If other lines
#       do change, either your kit isn't quite right or your
#       install is broken (or you've found a bug).
#
megatest: crm114
	$(E) ""
	$(E) "   This runs a moderately interesting set of base tests"
	$(E) "   to exercise much of CRM114 under TRE.  This takes about"
	$(E) "   1 minute to run on a 1.6 GHz Pentium-M laptop.  Please"
	$(E) "   be patient; you (hopefully) won't see anything till the"
	$(E) "   full set of tests complete.  If you didn't use TRE, all"
	$(E) "   bets are off."
	$(E) ""
	$(E) "   Lines of output that start with OK_IF_mumble are allowed"
	$(E) "   to change values.  No other lines should.  If other lines"
	$(E) "   do change, either your kit isn't quite right or your"
	$(E) "   install is broken (or you've found a bug)."
	$(E) ""
	-( if [ ! -e ./inserttest_a.crm      ] ; then cp $(srcdir)/*.crm ./           ; fi )
	-( if [ ! -e ./rewrites.mfp          ] ; then cp $(srcdir)/*.mfp ./           ; fi )
	-( if [ ! -e ./blacklist.mfp.example ] ; then cp $(srcdir)/*.example ./       ; fi )
	-( if [ ! -e ./QUICKREF.txt          ] ; then cp $(srcdir)/QUICKREF.txt ./    ; fi )
	-( if [ ! -e ./INTRO.txt             ] ; then cp $(srcdir)/INTRO.txt ./       ; fi )
	-( if [ ! -e ./COLOPHON.txt          ] ; then cp $(srcdir)/COLOPHON.txt ./    ; fi )
	-( if [ ! -e ./FAQ.txt               ] ; then cp $(srcdir)/FAQ.txt ./         ; fi )
	$(srcdir)/megatest.sh 2 > megatest_test.log 2>&1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (unified)       ====================="
	$(E) "=============================================================="
	$(diff) -u $(DIFF_FLAGS) $(srcdir)/megatest_knowngood.log megatest_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(diff) $(DIFF_FLAGS) $(srcdir)/megatest_knowngood.log megatest_test.log & sleep 1


cssfiles: spam.dir good.dir
	$(E) ""
	$(E) "   This may take a LONG TIME to run, up to 10 minutes per megabyte"
	$(E) "   of text.  Please adjust your expectations accordingly."
	$(E) ""
	$(E) "   Also, be advised that relearning the same text more than once"
	$(E) "   can be ill-advised.  Remove the .css files first if you are"
	$(E) "   relearning your entire corpus (new .css files will be created"
	$(E) "   if they don't already exist)"
	$(E) ""
	$(E) "   If your text corpi are very large, you may need to rebuild"
	$(E) "   crm114 with a larger default window size - or alter the commands"
	$(E) "   below to use a bigger -w than your default"
	$(E) ""
	$(E) "   If you have NOT yet set up your mailfilter.cf and mailfilter.mfp"
	$(E) "   files, hit ^C NOW.  REALLY!!!  Fix that, and rerun."
	sleep 5
	$(E) "   Here we go..."
	$(E) ""
	export DATE $(date +%Y_%m_%d_%H_%M_%S)
	$(E) "   MUCH STUFF DEPRECATED!!!  Use mailtrainer.crm instead..."
	cssutil -b -r spam.css
	cssutil -b -r nonspam.css
	cp spam.css spam__$(DATE).txt
	cp nonspam.css nonspam__$(DATE).txt
	$(E) ""
	$(E) "   run mailtrainer.crm on the spam.dir and good.dir directories."
	mailtrainer.crm \
		--good=./good.dir/ \
		--spam=./spam.dir/ \
		--repeat=5 \
		--random
	$(E) "   If all went well with the above, you will have backup copies of"
	$(E) "   your spam and nonspam .css files."
	$(E) "   You may now delete these files with impunity, or you may "
	$(E) "   choose to keep them around as backup."




tarfile: all
	rm -rf crm114-$(VERSION).src
	mkdir crm114-$(VERSION).src
	cp $(CFILES) \
	$(HFILES) \
	$(UTILFILES) \
	$(CRMFILES) \
	$(TEXTFILES) \
	crm114-$(VERSION).src
	cp -a BSD_build_files crm114-$(VERSION).src
	tar -cvf crm114-$(VERSION).src.tar crm114-$(VERSION).src
	rm -rf crm114-$(VERSION).src

src_gzip: tarfile
	rm -rf crm114-$(VERSION).src.tar.gz
	gzip crm114-$(VERSION).src.tar

i386_tarfile: all
	rm -rf crm114-$(VERSION).i386
	mkdir crm114-$(VERSION).i386
	cp crm114 cssutil cssdiff cssmerge osbf-util \
	Makefile \
	CRM114_Mailfilter_HOWTO.txt \
	COLOPHON.txt \
	QUICKREF.txt INTRO.txt knownbugs.txt FAQ.txt things_to_do.txt \
	mailfilter.crm mailfilter.cf \
	mailreaver.crm mailtrainer.crm \
	maillib.crm shuffle.crm \
	rewriteutil.crm rewrites.mfp \
	whitelist.mfp blacklist.mfp priolist.mfp \
	priolist.mfp.example blacklist.mfp.example whitelist.mfp.example \
	crm114-mode.el \
	GPL-License.txt \
	crm114-$(VERSION).i386
	$(tar) -cvf crm114-$(VERSION).i386.tar crm114-$(VERSION).i386
	rm -rf crm114-$(VERSION).i386

i386_gzip: i386_tarfile
	rm -rf crm114-$(VERSION).i386.tar.gz
	gzip crm114-$(VERSION).i386.tar

css_tarfile: spam.css nonspam.css
	rm -rf crm114-$(VERSION).css
	mkdir crm114-$(VERSION).css
	cp spam.css nonspam.css \
	crm114-$(VERSION).css
	$(tar) -cvf crm114-$(VERSION).css.tar crm114-$(VERSION).css
	rm -rf crm114-$(VERSION).css

css_gzip: css_tarfile
	rm -rf crm114-$(VERSION).css.tar.gz
	gzip crm114-$(VERSION).css.tar

distribution: install src_gzip i386_gzip css_gzip
	md5sum crm114-$(VERSION).*.tar.gz

rsync:  FORCE
	rsync -zrltDv \
	  $(CFILES) $(HFILES) $(UTILFILES) $(CRMFILES) $(TEXTFILES) \
	  wsy@crm114.sourceforge.net:/home/groups/c/cr/crm114/htdocs/src/

diff: diff_em.sh
	-(sh ./diff_em.sh );

dist-diff: diff
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.7z
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.tar.bz2
	( test "X$(sevenzip)" != "X" && $(sevenzip) a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(ARCHIVE_BASE_NAME).unified.diff.7z diff/*.unified.* )
	( test "X$(tar)" != "X" && $(tar) -c diff/*.unified.* | GZIP=$(GZIP_ENV) gzip -9 -c > $(ARCHIVE_BASE_NAME).unified.diff.tar.gz )
	( test "X$(tar)" != "X" && $(tar) -c diff/*.unified.* | bzip2 -9 -c > $(ARCHIVE_BASE_NAME).unified.diff.tar.bz2 )
	-rm -f $(ARCHIVE_BASE_NAME).unified.diff.zip
	( test "X$(zip)" != "X" && $(zip) -rq $(ARCHIVE_BASE_NAME).unified.diff.zip diff/*.unified.* )
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.7z
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.tar.bz2
	( test "X$(sevenzip)" != "X" && $(sevenzip) a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(ARCHIVE_BASE_NAME).normal.diff.7z diff/*.normal.* )
	( test "X$(tar)" != "X" && $(tar) -c diff/*.normal.* | GZIP=$(GZIP_ENV) gzip -9 -c > $(ARCHIVE_BASE_NAME).normal.diff.tar.gz )
	( test "X$(tar)" != "X" && $(tar) -c diff/*.normal.* | bzip2 -9 -c > $(ARCHIVE_BASE_NAME).normal.diff.tar.bz2 )
	-rm -f $(ARCHIVE_BASE_NAME).normal.diff.zip
	( test "X$(zip)" != "X" && $(zip) -rq $(ARCHIVE_BASE_NAME).normal.diff.zip diff/*.normal.* )
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.7z
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.tar.gz
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.tar.bz2
	( test "X$(sevenzip)" != "X" && $(sevenzip) a -t7z -m0=lzma -mx=9 -mfb=273 -md=64m -ms=on $(ARCHIVE_BASE_NAME).context.diff.7z diff/*.context.* )
	( test "X$(tar)" != "X" && $(tar) -c diff/*.context.* | GZIP=$(GZIP_ENV) gzip -9 -c > $(ARCHIVE_BASE_NAME).context.diff.tar.gz )
	( test "X$(tar)" != "X" && $(tar) -c diff/*.context.* | bzip2 -9 -c > $(ARCHIVE_BASE_NAME).context.diff.tar.bz2 )
	-rm -f $(ARCHIVE_BASE_NAME).context.diff.zip
	( test "X$(zip)" != "X" && $(zip) -rq $(ARCHIVE_BASE_NAME).context.diff.zip diff/*.context.* )
	mv $(ARCHIVE_BASE_NAME).*.diff.* ../


# original: indent -npcs -nprs -npsl -nsaf -nsai -nsaw -nut -bli0 -nlp *.c *.h
# indent -bad -bap -bl -bli0 -bls -cbi4 -cdw -cli0 -di8 -hnl -i4 -l80 -lp -nbc -nce -ncs -npcs -nprs -npsl -nsaf -nsai -nsaw -nsob -nut -pi4 -sc -ss -T index_t -T hitcount_t -T regex_t -T regmatch_t -T regamatch_t -T regaparams_t -T Qitem_t -T regoff_t -T tre_char_t -T COOCCURRENCE_SCORE_TYPE *.c *.h
reindent: indent.sh
	-( sh ./indent.sh $(CFILES) $(HFILES) )


.SUFFIXES = .c .o

.c.o:
	$(COMPILE) -c $< -o $@


FORCE:

.PHONY: reindent depend rsync done_all megatest help clean install uninstall-hook uninstall




depend:
	makedepend -- $(CFLAGS) -- *.[ch]

# DO NOT DELETE THIS LINE -- make depend depends on it.

crm_bit_entropy.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_bit_entropy.o: crm114.h
crm_compiler.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_compiler.o: crm114.h
crm_correlate.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_correlate.o: crm114.h
crm_css_maintenance.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_css_maintenance.o: crm114.h
crm_debugger.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_debugger.o: crm114.h
crm_errorhandlers.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_errorhandlers.o: crm114.h
crm_exec_engine.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_exec_engine.o: crm114.h
crm_expandvar.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expandvar.o: crm114.h
crm_expr_alter.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_alter.o: crm114.h
crm_expr_classify.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_classify.o: crm114.h crm114_osbf.h
crm_expr_clump_nn.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_clump_nn.o: crm114.h crm_expr_clump_nn.h
crm_expr_file_io.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_file_io.o: crm114.h
crm_expr_isolate.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_isolate.o: crm114.h
crm_expr_match.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_match.o: crm114.h
crm_expr_svm.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_svm.o: crm114.h
crm_expr_syscall.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_syscall.o: crm114.h
crm_expr_translate.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_translate.o: crm114.h
crm_expr_window.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_expr_window.o: crm114.h
crm_generic_port.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_generic_port.o: crm114.h
crm_main.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
crm_main.o: crm114_osbf.h
crm_markovian.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_markovian.o: crm114.h
crm_math_exec.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_math_exec.o: crm114.h
crm_osb_bayes.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_osb_bayes.o: crm114.h
crm_osbf_bayes.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_osbf_bayes.o: crm114.h crm114_osbf.h
crm_osbf_maintenance.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_osbf_maintenance.o: crm114.h crm114_osbf.h
crm_osb_hyperspace.o: crm114_sysincludes.h 
crm_osb_hyperspace.o: crm114_config.h
crm_osb_hyperspace.o: crm114_structs.h crm114.h
crm_osb_winnow.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_osb_winnow.o: crm114.h
crm_preprocessor.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_preprocessor.o: crm114.h
crmregex_gnu.o: crm114_sysincludes.h 
crmregex_gnu.o: crm114_config.h
crmregex_gnu.o: crm114_structs.h crm114.h
crmregex_tre.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crmregex_tre.o: crm114.h
crm_stats.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
crm_stmt_parser.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_stmt_parser.o: crm114.h
crm_strcompress.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_strcompress.o: crm114.h
crm_str_funcs.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_str_funcs.o: crm114.h
crm_util_errorhandlers.o: crm114_sysincludes.h
crm_var_hash_table.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_var_hash_table.o: crm114.h
crm_win32_port.o: crm114_sysincludes.h crm114_config.h crm114_structs.h
crm_win32_port.o: crm114.h
crm_winnow_maintenance.o: crm114_sysincludes.h crm114_config.h
crm_winnow_maintenance.o: crm114_structs.h crm114.h
cssdiff.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
cssmerge.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
cssutil.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
osbf-util.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
osbf-util.o: crm114_osbf.h
crm_versioning.o: crm114_sysincludes.h crm114_config.h crm114_structs.h crm114.h
crm_versioning.o: crm114_osbf.h
