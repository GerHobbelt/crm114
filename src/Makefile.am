## Process this file with automake to produce Makefile.in

## $Id: Makefile.am,v 1.15 2004/08/18 09:03:29 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal




#    Makefile for CRM114
#

DISTCLEANFILES = 
CLEANFILES = *.o *~ *.bak

# VER_SUFFIX defines a version suffix for our installed executables, 
# handy when you want many versions of CRM114 coexisting.
#
# e.g.: VER_SUFFIX=27B-6, then if you make install, you get crm-27b-6
# 
VER_SUFFIX=@VER_SUFFIX@ 

#
#      The following forces the compiler to be GCC.  If you have trouble
#      with your default compiler, and you want to force GCC, uncomment it.
# CC=gcc
#
#     What version is this software (PLEASE don't change this just 
#     to mess with my brain. - wsy)
#
VERSION = @VERSION@
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
#PREV_VERSION = 20051231a-BlameBarryAndPaoloa
#PREV_VERSION = 20051001-BlameRaulMiller
#PREV_VERSION = crm114-20050721-BlameNeilArmstrong
# VERSION += [$(TRE_TARFILE)]
#
#
#      Are we compiling on a POSIX system or a Windows system?  NOTE:
#       WINDOWS IS UNSUPPORTED BY THE AUTHOR.  THE AUTHOR WILL
#	ACCEPT REASONABLE-LOOKING PATCHES BUT WINDOWS-ONLY 
#       BUG REPORTS THAT DO NOT REPRODUCE UNDER LINUX ET AL _CANNOT_ BE WORKED.
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
#     turn this one on for no optimization debugging.
#AM_CFLAGS += -g -O0 -Wall 
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
AM_LDFLAGS += -static -static-libgcc
#
#   Any standard install flags?  We nominally use protection 755
INSTALLFLAGS = @INSTALLFLAGS@
INSTALLFLAGS += -m 755
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



#
#
#   These are the files needed to build the CRM114 engine; they don't 
#   include the side utilities
#
CFILES = crm_compiler.c crm_errorhandlers.c \
	crm_math_exec.c crm_var_hash_table.c crm_expandvar.c \
	crm_stmt_parser.c \
	crm_expr_alter.c crm_expr_match.c crm_css_maintenance.c \
	crm_markovian.c crm_osb_bayes.c crm_osb_hyperspace.c \
	crm_correlate.c crm_osb_winnow.c crm_winnow_maintenance.c \
	crm_osbf_bayes.c crm_osbf_maintenance.c crm_bit_entropy.c \
	crm_expr_window.c crm_expr_isolate.c crm_expr_file_io.c \
	crm_expr_syscall.c crm_expr_classify.c crm_expr_translate.c \
	crm_exec_engine.c crm_debugger.c crm_str_funcs.c \
	crm_preprocessor.c crmregex_gnu.c crmregex_tre.c \
	crm_util_errorhandlers.c crm_stats.c crm_expr_sks.c \
	crm_expr_svm.c crm_expr_fscm.c \
	crm_generic_port.c

HFILES = Makefile crm114_sysincludes.h crm114_structs.h crm114_config.h \
	crm114.h regex.h crm114_osbf.h config.h

OFILES = crm_compiler.o crm_errorhandlers.o \
	crm_math_exec.o crm_var_hash_table.o crm_expandvar.o \
	crm_stmt_parser.o \
	crm_expr_alter.o crm_expr_match.o crm_css_maintenance.o \
	crm_markovian.o crm_osb_bayes.o crm_osb_hyperspace.o \
	crm_correlate.o crm_osb_winnow.o crm_winnow_maintenance.o \
	crm_osbf_bayes.o crm_osbf_maintenance.o crm_bit_entropy.o \
	crm_expr_window.o crm_expr_isolate.o crm_expr_file_io.o \
	crm_expr_syscall.o crm_expr_classify.o crm_expr_translate.o \
	crm_exec_engine.o crm_debugger.o crm_str_funcs.o \
	crm_preprocessor.o \
	crm_expr_sks.o crm_stats.o crm_expr_svm.o crm_expr_fscm.o \
	crm_generic_port.o



UTILFILES = crm_main.c osbf-util.c cssutil.c cssdiff.c cssmerge.c 

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
	call_return_test.crm defaulttest.crm\
	shroud.crm quine.crm \
	bracktest.crm unionintersecttest.crm \
	statustest.crm windowtest_fromvar.crm paolo_overvars.crm \
	paolo_ov2.crm paolo_ov3.crm paolo_ov4.crm paolo_ov5.crm \
	pad.crm calc.crm \
	gatlingsort.crm \
	test_svm.crm \
	classifymail.crm 

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
	megatest.sh megatest_knowngood.log 



EXTRA_DIST = indent.sh diff_em.sh \
	config_win32.h crm114.sln crm114.vcproj cssdiff.vcproj osbf-util.vcproj \
	resource.h crm114.ico cssmerge.vcproj cssutil.vcproj \
	getopt.h getopt_ex.h getopt.c getopt_ex.c \
	crm_win32_port.c \
	megatest.bat crm114rc.h crm114rc.rc \
	$(UTILFILES) \
	$(CRMFILES) \
	$(TEXTFILES)




bin_PROGRAMS = crm114 cssutil osbfutil cssdiff cssmerge


crm114_SOURCES = $(CFILES) $(HFILES) crm_main.c

cssutil_SOURCES = cssutil.c crm_css_maintenance.c crm_util_errorhandlers.c crm_str_funcs.c $(HFILES)

osbfutil_SOURCES = osbf-util.c crm_osbf_maintenance.c crm_util_errorhandlers.c crm_str_funcs.c $(HFILES)

cssdiff_SOURCES = cssdiff.c crm_util_errorhandlers.c crm_str_funcs.c $(HFILES)

cssmerge_SOURCES = cssmerge.c crm_util_errorhandlers.c crm_str_funcs.c $(HFILES)


#
#

all: crm114 cssutil cssdiff cssmerge osbfutil

#
#    Done.  
#  
#    ***  Type "make help" to see what else you can make.
#

help: FORCE

#
#    You can make the following things:
#
#    make   -or-   make all   - builds the binaries.  This uses the
#                               TRE regex library
#    make crm114_gnu          - builds version with GNU regex instead
#                               of the TRE library.
#    make install             - builds and installs the binaries.  
#                               You must be root to do this.
#    make uninstall           - uninstalls the binaries.  You must
#                               be root to do this.
#    make megatest            - runs the full test suite ( takes
#                               about ~1 minute, TRE version _ONLY_! )
#    make clean               - deletes all .o and executables.  
#                               Don't do this on a binary kit!
#    make cssfiles            - builds a set of .css files from data
#                               in ./good.dir/* and ./spam.dir/*
#    make distribution        - builds the .gzballs for distribution
#                               of with your mods built in.

#
#   Note:  You've chosen to make a GNU regex library CRM114.  
#       Although CRM114 will work mostly correctly when
#	linked against the GNU regex library, a lot of functionality
#       won't be available, and "make megatest" will fail.
#
crm114_gnu: $(OFILES) crmregex_gnu.o crm_main.o
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
	$(CC) $(LDFLAGS) $(LIBS) crm_main.o $(OFILES) \
	  crmregex_tre.o \
	  -o crm114
#
#    Done.  Type "make help" to see what else you can make.
#



#
#     Note To The Reader:  I'd really _love_ to have all of these
#     stanzas for each of the .c code modules be taken care of by
#     something that uses $(CFILE), but it isn't.  Maybe the autoconf
#     version would have that- but one change at a time, eh?
#


crm_main.o: crm_main.c \
	$(HFILES)

crm_compiler.o: crm_compiler.c \
	$(HFILES)

crm_errorhandlers.o: crm_errorhandlers.c \
	$(HFILES)

crm_exec_engine.o: crm_exec_engine.c \
	$(HFILES)

crm_preprocessor.o: crm_preprocessor.c \
	$(HFILES)

crm_var_hash_table.o: crm_var_hash_table.c \
	$(HFILES)

crm_math_exec.o: crm_math_exec.c \
	$(HFILES)

crm_expandvar.o: crm_expandvar.c \
	$(HFILES)

crm_stmt_parser.o: crm_stmt_parser.c \
	$(HFILES)

crm_expr_alter.o: crm_expr_alter.c \
	$(HFILES)

crm_expr_match.o: crm_expr_match.c \
	$(HFILES)

crm_expr_file_io.o: crm_expr_file_io.c \
	$(HFILES)

crm_expr_syscall.o: crm_expr_syscall.c \
	$(HFILES)

crm_expr_classify.o: crm_expr_classify.c \
	$(HFILES)

crm_expr_translate.o: crm_expr_translate.c \
	$(HFILES)

crm_expr_window.o: crm_expr_window.c \
	$(HFILES)

crm_expr_isolate.o: crm_expr_isolate.c \
	$(HFILES)

crm_css_maintenance.o: crm_css_maintenance.c \
	$(HFILES)

crm_winnow_maintenance.o: crm_winnow_maintenance.c \
	$(HFILES)

crm_osbf_maintenance.o: crm_osbf_maintenance.c \
	$(HFILES)

crm_markovian.o: crm_markovian.c \
	$(HFILES)

crm_osb_bayes.o: crm_osb_bayes.c \
	$(HFILES)

crm_osbf_bayes.o: crm_osbf_bayes.c \
	$(HFILES)

crm_osb_hyperspace.o: crm_osb_hyperspace.c \
	$(HFILES)

crm_expr_sks.o: crm_expr_sks.c \
	$(HFILES)

crm_expr_svm.o: crm_expr_svm.c \
	$(HFILES)

crm_expr_fscm.o: crm_expr_fscm.c \
	$(HFILES)

crm_stats.o: crm_stats.c \
	$(HFILES)

#crm_osb_neural.o: crm_osb_neural.c \
#	$(HFILES)

crm_bit_entropy.o: crm_bit_entropy.c \
	$(HFILES)

crm_osb_winnow.o: crm_osb_winnow.c \
	$(HFILES)

crm_correlate.o: crm_correlate.c \
	$(HFILES)

crm_debugger.o: crm_debugger.c \
	$(HFILES)

crm_str_funcs.o: crm_str_funcs.c \
	$(HFILES)

crmregex_tre.o: crmregex_tre.c \
	$(HFILES)

crmregex_gnu.o: crmregex_gnu.c \
	$(HFILES)

crm_util_errorhandlers.o: crm_util_errorhandlers.c \
	$(HFILES)

cssutil.o: cssutil.c  \
	$(HFILES)

osbf-util.o: osbf-util.c  \
	$(HFILES)

cssdiff.o: cssdiff.c  \
	$(HFILES)

cssmerge.o: cssmerge.c  \
	$(HFILES)

crm_generic_port.o: crm_generic_port.c  \
	$(HFILES)




cssutil: cssutil.o crm_css_maintenance.o crm_util_errorhandlers.o crm_str_funcs.o 
	$(CC) $(LDFLAGS) $(LIBS) cssutil.o \
		crm_css_maintenance.o \
		crm_util_errorhandlers.o \
		crm_str_funcs.o -o cssutil

osbfutil: osbf-util.o crm_osbf_maintenance.o crm_util_errorhandlers.o crm_str_funcs.o 
	$(CC) $(LDFLAGS) $(LIBS) osbf-util.o \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		crm_osbf_maintenance.o -o osbfutil

cssdiff: cssdiff.o crm_util_errorhandlers.o crm_str_funcs.o 
	$(CC) $(LDFLAGS) $(LIBS) cssdiff.o \
		crm_util_errorhandlers.o \
		crm_str_funcs.o -o cssdiff

cssmerge: cssmerge.o crm_util_errorhandlers.o crm_str_funcs.o 
	$(CC) $(LDFLAGS) $(LIBS) cssmerge.o \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		-o cssmerge


clean:
	-rm -f crm114
	-rm -f cssutil
	-rm -f osbf-util
	-rm -f osbfutil
	-rm -f cssdiff
	-rm -f cssmerge
	-rm -f crm114
	-rm -f crm114_gnu
	-rm -f crm114_tre
	-rm -f *.o
#   if the next line doesn't work in your version of make, 
#    don't worry.  cd down into the TRE directory and 'make clean' manually
	-( [ "$(TRE_TARFILE)" ] && cd $(TRE_TARFILE) && make clean )


install: FORCE
	$(MAKE) install_engine install_utils

#
#    Done.  Type "make help" to see what else you can make.
#

#
#    Installing Utilities
#	
install_utils:
	$(MAKE) cssmerge cssutil cssdiff osbfutil
	install $(INSTALLFLAGS) cssdiff $(bindir)/cssdiff$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssmerge $(bindir)/cssmerge$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssutil $(bindir)/cssutil$(VER_SUFFIX)
	install $(INSTALLFLAGS) osbfutil $(bindir)/osbfutil$(VER_SUFFIX)
	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)

#   ^^^ if you don't have EMACS installed in the default place, you
#   won't get the crm114-mode.el automatically.


# use different name so we can keep it around along with default one
# and same VER_SUFFIX postfix 

#
#    Installing engine
#
install_engine: crm114
	install $(INSTALLFLAGS) crm114 $(bindir)/crm$(VER_SUFFIX)


install_binary_only: FORCE
	install $(INSTALLFLAGS) crm114 $(bindir)/crm$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssdiff $(bindir)/cssdiff$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssmerge $(bindir)/cssmerge$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssutil $(bindir)/cssutil$(VER_SUFFIX)	
	install $(INSTALLFLAGS) osbfutil $(bindir)/osbfutil$(VER_SUFFIX)
	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


install-exec-hook:
	cd $(DESTDIR)$(bindir) && $(LN_S) crm114 crm


uninstall-hook:
	rm $(DESTDIR)$(bindir)/crm


uninstall: FORCE
	-rm -rf $(bindir)/crm$(VER_SUFFIX)
	-rm -rf $(bindir)/crmg$(VER_SUFFIX)
	-rm -rf $(bindir)/crma$(VER_SUFFIX)
	-rm -rf $(bindir)/cssdiff$(VER_SUFFIX)
	-rm -rf $(bindir)/cssmerge$(VER_SUFFIX)
	-rm -rf $(bindir)/cssutil$(VER_SUFFIX)
	-rm -rf $(bindir)/osbf-util$(VER_SUFFIX)
	-rm -rf $(bindir)/osbfutil$(VER_SUFFIX)
	-rm -rf $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


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
megatest: FORCE
	-cp $(srcdir)/*.crm ./           2>&1
	-cp $(srcdir)/*.mfp ./           2>&1
	-cp $(srcdir)/*.example ./       2>&1
	-cp $(srcdir)/QUICKREF.txt ./    2>&1
	-cp $(srcdir)/INTRO.txt ./       2>&1
	-cp $(srcdir)/COLOPHON.txt ./    2>&1
	-cp $(srcdir)/FAQ.txt ./         2>&1
	$(srcdir)/megatest.sh > megatest_test.log 2>&1
	diff -EbwBd $(srcdir)/megatest_knowngood.log megatest_test.log & sleep 1


#
#   This may take a LONG TIME to run, up to 10 minutes per megabyte
#   of text.  Please adjust your expectations accordingly.
#
#    Also, be advised that relearning the same text more than once
#    can be ill-advised.  Remove the .css files first if you are
#    relearning your entire corpus (new .css files will be created
#    if they don't already exist)
#
#   If your text corpi are very large, you may need to rebuild 
#   crm114 with a larger default window size - or alter the commands
#   below to use a bigger -w than your default
#
#   If you have NOT yet set up your mailfilter.cf and mailfilter.mfp
#   files, hit ^C NOW.  REALLY!!!  Fix that, and rerun.
cssfiles: spam.dir good.dir
	sleep 5
	export DATE $(date +%Y_%m_%d_%H_%M_%S)
	cssutil -b -r spam.css
	cssutil -b -r nonspam.css
	cp spam.css spam__$(DATE).txt
	cp nonspam.css nonspam__$(DATE).txt
	mailtrainer.crm \
		--good=./good.dir/ \
		--spam=./spam.dir/ \
		--repeat=5 \
		--random

#    ^^^ run mailtrainer.crm on the spam.dir and good.dir directories.
#
#  MUCH STUFF DEPRECATED!!!  Use mailtrainer.crm instead...
#  cp spamtext.txt saved_spamtext__$(DATE).txt
#  cp nonspamtext.txt saved_nonspamtext__$(DATE).txt
#  ./crm114 mailfilter.crm --learnspam --force < saved_spamtext__$(DATE).txt
#  ./crm114 mailfilter.crm --learnnonspam --force < saved_nonspamtext__$(DATE).txt

#    If all went well with the above, you will have backup copies of
#    your spam and nonspam .css files.
#    You may now delete these files with impunity, or you may 
#    choose to keep them around as backup.



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
	cp crm114 cssutil cssdiff cssmerge osbfutil \
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
	tar -cvf crm114-$(VERSION).i386.tar crm114-$(VERSION).i386
	rm -rf crm114-$(VERSION).i386

i386_gzip: i386_tarfile
	rm -rf crm114-$(VERSION).i386.tar.gz
	gzip crm114-$(VERSION).i386.tar

css_tarfile: spam.css nonspam.css
	rm -rf crm114-$(VERSION).css
	mkdir crm114-$(VERSION).css
	cp spam.css nonspam.css \
	crm114-$(VERSION).css
	tar -cvf crm114-$(VERSION).css.tar crm114-$(VERSION).css
	rm -rf crm114-$(VERSION).css

css_gzip: css_tarfile
	rm -rf crm114-$(VERSION).css.tar.gz
	gzip crm114-$(VERSION).css.tar

distribution: install src_gzip i386_gzip css_gzip
	md5sum crm114-$(VERSION).*.tar.gz

rsync:  FORCE
	rsync -zrltDv \
	  $(CFILES) $(HFILES) $(UTILFILES) $(CRMFILES) $(TEXTFILES)\
	  wsy@crm114.sourceforge.net:/home/groups/c/cr/crm114/htdocs/src/

diff: FORCE
	-(sh diff_em.sh );


# original: indent -npcs -nprs -npsl -nsaf -nsai -nsaw -nut -bli0 -nlp *.c *.h
reindent:
	indent -bad -bap -bl -bli0 -bls -cbi4 -cdw -cli0 -di8 -hnl -i4 -l80 -lp -nbc -nce -ncs -npcs -nprs -npsl -nsaf -nsai -nsaw -nsob -nut -pi4 -sc -ss -T index_t -T hitcount_t -T regex_t -T regmatch_t -T regamatch_t -T regaparams_t -T Qitem_t -T regoff_t -T tre_char_t -T COOCCURRENCE_SCORE_TYPE *.c *.h


.SUFFIXES = .c .o

.c.o:
	$(COMPILE) -c $< -o $@


FORCE:

.PHONY: FORCE reindent depend diff rsync


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
