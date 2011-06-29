#
#    Makefile for CRM114
#
# Copyright 2009 William S. Yerazunis.
# This file is under GPLv3, as described in COPYING.

#   If you want to install the executables somewhere else, change
#    BINDIR here.  Default is /usr/bin but you can change it to /usr/local/bin
#     if you prefer that sort of thing.
#
prefix?=/usr
BINDIR=${prefix}/bin

# VER_SUFFIX defines a version suffix for our installed executables,
# handy when you want many versions of CRM114 coexisting.
#
# e.g.: VER_SUFFIX=27B-6, then if you make install, you get crm-27b-6
#
VER_SUFFIX ?=

#
#      The following forces the compiler to be GCC.  If you have trouble
#      with your default compiler, and you want to force GCC, uncomment it.
# CC=gcc
#
#     What version is this software (PLEASE don't change this just
#     to mess with my brain. - wsy)
#
CRM114_ARCHIVE = /homes/crm114/archive
VERSION=20090423-BlameSteveJobs
PREV_VERSION=20090120-BlameSteveJobs
#PREV_VERSION=20081111-BlameBarack
#PREV_VERSION=20081023-BlameMaxwell
#PREV_VERSION=20080923-BlameVT
#PREV_VERSION=20080618-BlameBadGlasses
#PREV_VERSION=20080502-BlameJason
#PREV_VERSION=20080326-BlameSentansoken
#PREV_VERSION=20080217-BlameJoeLangeway_VT
#PREV_VERSION=20071201-BlameLittleStevie_VT
#PREV_VERSION=20071126-BlameGerHobbelt
#PREV_VERSION=20070917-BlameGerHobbelt
#VERSION=20070830-BlameGerHobbelt
#PREV_VERSION=20070828-BlameGerHobbelt
#PREV_VERSION=20070827-BlameGerHobbelt
#PREV_VERSION=20070826-BlameGerHobbelt
#PREV_VERSION = 20070815-BlameBadData
#PREV_VERSION = 20070810-BlameTheSegfault
#PREV_VERSION = 20070505-BlameIdiocy
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
#
#   Define the TRE directory (used only for building distributions- note that
#   this must be HAND-UPDATED when new versions of TRE come out  )
TRE_TARFILE = tre-0.7.5
# VERSION += [$(TRE_TARFILE)]
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
# CFLAGS += -DWIN32
#
#     Build our basic CFLAGS chunk
#
CFLAGS += -DVERSION='"$(VERSION)"'
#
#  Known Neighbord On, only for debugging
# CFLAGS += -DKNN_ON
#
#  Ben Graphic, only for debugging
# CFLAGS += -DBEN_GRAPHIC
#
#  Stab test, long accepted
# CFLAGS += -DSTAB_TEST
#
#  Stochastic setting, unused in crm_neural_net.c
# CFLAGS += -DSTOCHASTIC
#
#  Tell the compiler full optimization "-O3", add debugging info "-g",
#  and warn on all warnable things "-Wall".  Someday add -Wextra for
#  really picayune stuff.
#
#  Our default is with debugging, full optimization, and lots of warnings:
CFLAGS += -g -O3 -Wall
#     turn this one on for no optimization, but debugging
#CFLAGS += -g -O0 -Wall
#
#     Turn this one on for time profiling.
#CFLAGS += -pg
#     turn this one on for really picayune debugging:
# CFLAGS += -Wextra
#
#   Choose between static and dynamic linking (we recommend static linking)
#   Comment this out if you want dynamic linking
#
#    BSD users - do NOT use -static-libgcc!!!
LDFLAGS += -static -static-libgcc
#    use this one if you want program profiling.
#LDFLAGS += -static -static-libgcc -pg
#LDFLAGS += -pg
#
#   Any standard install flags?  We nominally use protection 755
INSTALLFLAGS += -m 755
#
#        uncomment the next line if you want to "strip" the debugger info
#        from the binaries when installing.  This speeds up load, but
#        you won't be able to submit meaningful CRM114 engine bug reports.
#        Default is don't strip.
# INSTALLFLAGS += -s
#
#
#        Libraries to link against.
LIBS += -lm
#
# ---------   If you're compiling under *BSD, check these out:
#
#    Simson Garfinkel suggests that you
#    uncomment the following to get a BSD-sane environment.  Leave them
#    untouched (commented out) for Linux builds.
# Add for FreeBSD:
#CFLAGS += -I/usr/local/include
#LDFLAGS += -L/usr/local/lib
#LIBS += -lintl -liconv
#LDFLAGS += -dynamic-libgcc
#
#    Jeff Rice suggests the following for FreeBSD:
#CFLAGS += -I/usr/local/include -I${HOME}/include
#LDFLAGS += -L/usr/local/lib -L${HOME}/lib
#LIBS += -lintl -liconv
#
#    Nico Kadel-Garcia has added these flags for systems without logl or sqrtf
#  BSD has no logl function
# CFLAGS += -DNO_LOGL
#
#  BSD has no sqrtf function
# CFLAGS += -DNO_SQRTF
# ------------------ end of *BSD stuff
#
# ------------------For Mac OSX, try this---------------
#
#    Yimin Wu (of the SVMx classifiers) suggests that to get
#    a good compile on the Mac OSX machines, try:
#
#   COMMENT OUT all the above "LDFLAGS += -static -static-libgcc -pg"
#
#    and add
#
#CFLAGS += -I/usr/local/include
#LDFLAGS += -l/usr/local/lib
#
#  and change final options on the main link line from:
#
#   "-lm -ltre -o crm114" to "-ltre -o crm114"
#
#  PowerPPC has no sqrtf function
# CFLAGS += -DNO_SQRTF
#
# --------------End of Mac OSX stuff -----------------------
#
#
#    End of user-configurable options... if you modify anything below
#    this line, you risk early insanity and blindness.
#
#
#   These are the files needed to build the CRM114 engine; they don't
#   include the side utilities
#
CFILES = crm_main.c crm_compiler.c crm_errorhandlers.c \
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
	crm_preprocessor.c crmregex_tre.c \
	crm_stats.c crm_expr_sks.c\
	crm_expr_svm.c crm_fast_substring_compression.c

HFILES = Makefile crm114_sysincludes.h crm114_structs.h crm114_config.h \
	crm114.h crm114_osbf.h

OFILES = crm_main.o crm_compiler.o crm_errorhandlers.o \
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
	crm_preprocessor.o crmregex_tre.c \
	crm_expr_sks.o crm_stats.o crm_expr_svm.o \
	crm_fast_substring_compression.o

UTILFILES = osbf-util.c cssutil.c  cssdiff.c cssmerge.c crm_util_errorhandlers.c

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
	call_return_test.crm defaulttest.crm alternating_example_neural.crm \
	shroud.crm quine.crm \
	bracktest.crm unionintersecttest.crm \
	statustest.crm windowtest_fromvar.crm paolo_overvars.crm \
	paolo_ov2.crm paolo_ov3.crm paolo_ov4.crm paolo_ov5.crm \
	pad.crm calc.crm \
	gatlingsort.crm tenfold_validate.crm\
	test_svm.crm \
	classifymail.crm

TEXTFILES = COLOPHON.txt pad.dat \
	QUICKREF.txt INTRO.txt KNOWNBUGS.txt FAQ.txt THINGS_TO_DO.txt \
	CLASSIFY_DETAILS.txt README CRM114_Mailfilter_HOWTO.txt \
	Alice_In_Wonderland_Chap_1_And_2.txt Macbeth_Act_IV.txt \
	Hound_of_the_Baskervilles_first_500_lines.txt \
	The_Wind_in_the_Willows_Chap_1.txt \
	reto_procmailrc.recipe \
	priolist.mfp blacklist.mfp whitelist.mfp rewrites.mfp \
	priolist.mfp.example blacklist.mfp.example whitelist.mfp.example \
	crm114-mode.el \
	crm114.spec \
	inoc_passwd.txt \
	GPL_License.txt \
	megatest.sh megatest_knowngood.log


#
#

all: crm114 cssutil cssdiff cssmerge osbf-util
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
	#                               
	#    make crm114              - build CRM binary. This uses the
	#                               TRE regex library
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
	#    make distribtion         - builds the .gzballs for distribution
        #                               of with your mods built in.

crm114: $(OFILES)
	$(CC) $(LDFLAGS) $(LIBS) $(OFILES) \
	  -ltre -lm\
	  -o crm114
	#
	#    Done.  Type "make help" to see what else you can make.
	#

*.o: $(HFILES)
.c.o:
	$(CC)  $(CFLAGS) -c $*.c


cssutil: cssutil.o crm_css_maintenance.o crm_util_errorhandlers.o crm_str_funcs.o $(HFILES)
	$(CC)  $(CFLAGS) $(LDFLAGS) cssutil.o \
		crm_css_maintenance.o \
		crm_util_errorhandlers.o \
		crm_str_funcs.o -lm -ltre -o cssutil

osbf-util: osbf-util.o crm_osbf_maintenance.o crm_util_errorhandlers.o crm_str_funcs.o $(HFILES)
	$(CC)  $(CFLAGS) $(LDFLAGS) osbf-util.o \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		crm_osbf_maintenance.o -lm -ltre -o osbf-util

cssdiff: cssdiff.o crm_util_errorhandlers.o crm_str_funcs.o $(HFILES)
	$(CC)  $(CFLAGS) $(LDFLAGS) \
		crm_util_errorhandlers.o \
		crm_str_funcs.o -ltre cssdiff.o -o cssdiff

cssmerge: cssmerge.o crm_util_errorhandlers.o crm_str_funcs.o $(HFILES)
	$(CC)  $(CFLAGS) $(LDFLAGS) \
		cssmerge.o crm_str_funcs.o \
		crm_util_errorhandlers.o \
		-ltre -o cssmerge

clean:
	-rm -f crm114
	-rm -f cssutil
	-rm -f osbf-util
	-rm -f cssdiff
	-rm -f cssmerge
	-rm -f crm114
	-rm -f crm114_tre
	-rm -f *.o

install: FORCE
	$(MAKE) install_engine install_utils
	#
	#    Done.  Type "make help" to see what else you can make.
	#

install_utils:
	#
	#    Installing Utilities
	#
	$(MAKE) cssmerge cssutil cssdiff osbf-util
	install $(INSTALLFLAGS) cssdiff $(BINDIR)/cssdiff$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssmerge $(BINDIR)/cssmerge$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssutil $(BINDIR)/cssutil$(VER_SUFFIX)
	install $(INSTALLFLAGS) osbf-util $(BINDIR)/osbf-util$(VER_SUFFIX)
	#   if you don't have EMACS installed in the default place, you
	#   won't get the crm114-mode.el automatically.
	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


# use different name so we can keep it around along with default one
# and same VER_SUFFIX postfix
install_engine: crm114
	#
	#    Installing engine
	#
	install $(INSTALLFLAGS) crm114 $(BINDIR)/crm$(VER_SUFFIX)

install_binary_only: FORCE
	install $(INSTALLFLAGS) crm114 $(BINDIR)/crm$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssdiff $(BINDIR)/cssdiff$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssmerge $(BINDIR)/cssmerge$(VER_SUFFIX)
	install $(INSTALLFLAGS) cssutil $(BINDIR)/cssutil$(VER_SUFFIX)
	install $(INSTALLFLAGS) osbf-util $(BINDIR)/osbf-util$(VER_SUFFIX)
	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)

uninstall: FORCE
	-rm -rf $(BINDIR)/crm$(VER_SUFFIX)
	-rm -rf $(BINDIR)/crmg$(VER_SUFFIX)
	-rm -rf $(BINDIR)/crma$(VER_SUFFIX)
	-rm -rf $(BINDIR)/cssdiff$(VER_SUFFIX)
	-rm -rf $(BINDIR)/cssmerge$(VER_SUFFIX)
	-rm -rf $(BINDIR)/cssutil$(VER_SUFFIX)
	-rm -rf $(BINDIR)/osbf-util$(VER_SUFFIX)
	-rm -rf $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


megatest: FORCE
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
	###  The >> in the output forces line-at-a-time buffering.
	rm -rf megatest_test.log
	./megatest.sh >>megatest_test.log 2>>megatest_test.log
	# ./megatest.sh >megatest_test.log 2>&1
	#./megatest.sh 2>&1 | tee megatest_test.log
	#diff -U 5 megatest_knowngood.log megatest_test.log & sleep 1
	diff megatest_knowngood.log megatest_test.log & sleep 1


neurotest: FORCE
	./nn_segfault_test.sh

cssfiles: spam.dir good.dir
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
	sleep 5
	#   Here we go...
	#
	export DATE $(date +%Y_%m_%d_%H_%M_%S)
	#  MUCH STUFF DEPRECATED!!!  Use mailtrainer.crm instead...
	#  cp spamtext.txt saved_spamtext__$(DATE).txt
	#  cp nonspamtext.txt saved_nonspamtext__$(DATE).txt
	cssutil -b -r spam.css
	cssutil -b -r nonspam.css
	cp spam.css spam__$(DATE).txt
	cp nonspam.css nonspam__$(DATE).txt
	#  ./crm114 mailfilter.crm --learnspam --force < saved_spamtext__$(DATE).txt
	#  ./crm114 mailfilter.crm --learnnonspam --force < saved_nonspamtext__$(DATE).txt
	#
	#    run mailtrainer.crm on the spam.dir and good.dir directories.
	mailtrainer.crm \
		--good=./good.dir/ \
		--spam=./spam.dir/ \
		--repeat=5 \
		--random

	#    If all went well with the above, you will have backup copies of
	#    your spam and nonspam .css files.
	#    You may now delete these files with impunity, or you may
	#    choose to keep them around as backup.



tarfile: all check_permissions
	rm -rf crm114-$(VERSION).src
	mkdir -m 0755 crm114-$(VERSION).src
	cp $(CFILES) \
	$(HFILES) \
	$(UTILFILES) \
	$(CRMFILES) \
	$(TEXTFILES) \
	crm114-$(VERSION).src
	tar -cvf crm114-$(VERSION).src.tar crm114-$(VERSION).src
	rm -rf crm114-$(VERSION).src
	chmod 0644 crm114-$(VERSION).src.tar

src_gzip: tarfile
	rm -rf crm114-$(VERSION).src.tar.gz
	gzip crm114-$(VERSION).src.tar
	chmod 0644 crm114-$(VERSION).src.tar.gz

i386_tarfile: all
	rm -rf crm114-$(VERSION).i386
	mkdir -m 0755 crm114-$(VERSION).i386
	cp crm114 cssutil cssdiff cssmerge osbf-util \
	Makefile \
	CRM114_Mailfilter_HOWTO.txt \
	COLOPHON.txt \
	QUICKREF.txt INTRO.txt KNOWNBUGS.txt FAQ.txt THINGS_TO_DO.txt \
	mailfilter.crm mailfilter.cf \
	mailreaver.crm mailtrainer.crm \
	maillib.crm shuffle.crm \
	rewriteutil.crm rewrites.mfp \
	whitelist.mfp blacklist.mfp priolist.mfp \
	priolist.mfp.example blacklist.mfp.example whitelist.mfp.example \
	crm114-mode.el \
	GPL_License.txt \
	crm114-$(VERSION).i386
	tar -cvf crm114-$(VERSION).i386.tar crm114-$(VERSION).i386
	rm -rf crm114-$(VERSION).i386

i386_gzip: i386_tarfile
	rm -rf crm114-$(VERSION).i386.tar.gz
	gzip crm114-$(VERSION).i386.tar
	chmod 0644 crm114-$(VERSION).i386.tar.gz

css_tarfile: spam.css nonspam.css
	rm -rf crm114-$(VERSION).css
	mkdir crm114-$(VERSION).css
	cp spam.css nonspam.css \
	crm114-$(VERSION).css
	tar -cvf crm114-$(VERSION).css.tar crm114-$(VERSION).css
	rm -rf crm114-$(VERSION).css
	chmod 0644 crm114-$(VERSION).css.tar

css_gzip: css_tarfile
	rm -rf crm114-$(VERSION).css.tar.gz
	gzip crm114-$(VERSION).css.tar
	chmod 0644 crm114-$(VERSION).css.tar.gz

distribution: install src_gzip i386_gzip
	md5sum crm114-$(VERSION).*.tar.gz

upload_tarball: distribution
	rsync -zrltDv 	\
	crm114-$(VERSION).src.tar.gz \
	crm114-$(VERSION).i386.tar.gz \
	wsy,crm114@web.sourceforge.net:htdocs/tarballs/

upload_tarball_latest: i386_gzip
	rsync -zrltDv crm114-$(VERSION).i386.tar.gz \
	wsy,crm114@web.sourceforge.net:htdocs/tarballs/crm114-latest.i386.tar.gz
	rsync -zrltDv crm114-$(VERSION).src.tar.gz \
	wsy,crm114@web.sourceforge.net:htdocs/tarballs/crm114-latest.src.tar.gz
	echo $(VERSION) > 00-LATEST-IS
	rsync -auv 00-LATEST-IS \
	wsy,crm114@web.sourceforge.net:htdocs/tarballs/

rsync:  FORCE check_permissions
	echo "Options Indexes" > .htaccess
	chmod 0644 .htaccess
	rsync -zprltDv .htaccess \
	  $(CFILES) $(HFILES) $(UTILFILES) $(CRMFILES) $(TEXTFILES)\
	  wsy,crm114@web.sourceforge.net:htdocs/src

check_permissions:
	#
	# Setting file permissions
	#
	find -name .git -prune -o -type d ! -perm 2775 -exec chmod 2775 '{}' \; -print
	find -name .git -prune -o -type f \
		! -name \*\.crm \
		! -name \*\.sh \
		! -name crm114 \
		! -perm 0644 \
		-exec chmod 0644 '{}' \; -print
	find -name .git -prune -o -type f -name \*\.crm ! -perm 0755 -exec chmod 0755 '{}' \; -print
	find -name .git -prune -o -type f -name \*\.sh ! -perm 0755 -exec chmod 0755 '{}' \; -print
	find -name .git -prune -o -type f -name crm114 ! -perm 0755 -exec chmod 0755 '{}' \; -print

FORCE:
