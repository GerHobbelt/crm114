#    Makefile for CRM114
#
#
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
VERSION = 20070428-BlameSpamConf
PREV_VERSION = 20061103-BlameDalkey
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
SYSTEMTYPE = POSIX
#SYSTEMTYPE = WIN32
#
#
#  Tell the compiler full optimization "-O3", add debugging info "-g", 
#  and warn on all warnable things "-Wall".  Someday add -Wextra for
#  really picayune stuff.
#  
CFLAGS += -g -O3 -Wall
#     turn this one on for no optimization debugging.
#CFLAGS += -g -O0 -Wall 
#     turn this one on for really picayune debugging.
#CFLAGS += -Wextra
#
#   Choose between static and dynamic linking (we recommend static linking)
#   Comment this out if you want dynamic linking
#
#    BSD users - do NOT use -static-libgcc!!!
LDFLAGS += -static -static-libgcc
#
#   Any standard install flags?  We nominally use protection 755
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
#   These are the files needed to build the CRM114 engine; they don't 
#   include the side utilities
#
CFILES = crm_main.c crm_compiler.c crm_errorhandlers.c \
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
	crm_util_errorhandlers.c 
#	crm_osb_neural.c \
#
HFILES = Makefile crm114_sysincludes.h crm114_structs.h crm114_config.h \
	crm114.h regex.h crm114_osbf.h 
#
OFILES = crm_main.o crm_compiler.o crm_errorhandlers.o \
	crm_exec_engine.o crm_preprocessor.o crm_var_hash_table.o \
	crm_math_exec.o crm_expandvar.o crm_stmt_parser.o \
	crm_expr_alter.o crm_expr_match.o crm_css_maintenance.o \
	crm_markovian.o	crm_osb_bayes.o crm_osb_hyperspace.o \
	crm_correlate.o crm_osb_winnow.o crm_winnow_maintenance.o \
	crm_osbf_bayes.o crm_osbf_maintenance.o crm_bit_entropy.o \
	crm_expr_window.o crm_expr_isolate.o  crm_expr_file_io.o \
	crm_expr_syscall.o crm_expr_classify.o crm_expr_translate.o \
	crm_debugger.o crm_str_funcs.o 
#	crm_osb_neural.o \

UTILFILES = osbf-util.c cssutil.c  cssdiff.c cssmerge.c 


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


#
#

all: crm114 cssutil cssdiff cssmerge osbf-util

crm114_gnu: $(OFILES) crmregex_gnu.o 
	#
	#   Note: if you haven't installed TRElib, the next step 
	#   will get an error.
	#       For TRElib, look in the TRE sub-directory of this kit.
	#   Remember to use ./configure --enable-static for static libs.
	#   You will also need to add "usr/local/lib/*" to /etc/ld.so.conf,
	#   and then run ldconfig (as root) to set the library up and make
	#   the library known to the static and runtime linkers.
	#
	$(CC) $(LDFLAGS) $(LIBS) $(OFILES) \
	  crmregex_gnu.o \
	  -lm -o crm114_gnu


crm114: $(OFILES) crmregex_tre.o
	#
	#   Note: if you haven't installed TRElib, the next step 
	#   will get an error.
	#       For TRElib, look in the TRE sub-directory of this kit.
	#   Remember to use ./configure --enable-static for static libs.
	#   You will also need to add /usr/local/lib to /etc/ld.so.conf,
	#   and then run ldconfig (as root) to set the library up and make
	#   the library known to the static and runtime linkers.
	#
	$(CC) $(LDFLAGS) $(LIBS) $(OFILES) \
	  crmregex_tre.o \
	  -lm -ltre -o crm114

#
#     Note To The Reader:  I'd really _love_ to have all of these
#     stanzas for each of the .c code modules be taken care of by
#     something that uses $(CFILE), but it isn't.  Maybe the autoconf
#     version would have that- but one change at a time, eh?
#

crm_main.o: crm_main.c  \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_main.c -o crm_main.o

crm_compiler.o: crm_compiler.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_compiler.c -o crm_compiler.o

crm_errorhandlers.o: crm_errorhandlers.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_errorhandlers.c -o crm_errorhandlers.o

crm_exec_engine.o: crm_exec_engine.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_exec_engine.c -o crm_exec_engine.o

crm_preprocessor.o: crm_preprocessor.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_preprocessor.c -o crm_preprocessor.o

crm_var_hash_table.o: crm_var_hash_table.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_var_hash_table.c -o crm_var_hash_table.o

crm_math_exec.o: crm_math_exec.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_math_exec.c -o crm_math_exec.o

crm_expandvar.o: crm_expandvar.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expandvar.c -o crm_expandvar.o

crm_stmt_parser.o: crm_stmt_parser.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_stmt_parser.c -o crm_stmt_parser.o

crm_expr_alter.o: crm_expr_alter.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_alter.c -o crm_expr_alter.o

crm_expr_match.o: crm_expr_match.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_match.c -o crm_expr_match.o

crm_expr_file_io.o: crm_expr_file_io.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_file_io.c -o crm_expr_file_io.o

crm_expr_syscall.o: crm_expr_syscall.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_syscall.c -o crm_expr_syscall.o

crm_expr_classify.o: crm_expr_classify.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_classify.c -o crm_expr_classify.o

crm_expr_translate.o: crm_expr_translate.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_translate.c -o crm_expr_translate.o

crm_expr_window.o: crm_expr_window.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_window.c -o crm_expr_window.o

crm_expr_isolate.o: crm_expr_isolate.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_expr_isolate.c -o crm_expr_isolate.o

crm_css_maintenance.o: crm_css_maintenance.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_css_maintenance.c -o crm_css_maintenance.o

crm_winnow_maintenance.o: crm_winnow_maintenance.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_winnow_maintenance.c -o crm_winnow_maintenance.o

crm_osbf_maintenance.o: crm_osbf_maintenance.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_osbf_maintenance.c -o crm_osbf_maintenance.o

crm_markovian.o: crm_markovian.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_markovian.c -o crm_markovian.o

crm_osb_bayes.o: crm_osb_bayes.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_osb_bayes.c -o crm_osb_bayes.o

crm_osbf_bayes.o: crm_osbf_bayes.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_osbf_bayes.c -o crm_osbf_bayes.o

crm_osb_hyperspace.o: crm_osb_hyperspace.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_osb_hyperspace.c -o crm_osb_hyperspace.o

#crm_osb_neural.o: crm_osb_neural.c \
#	$(HFILES)
#	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_osb_neural.c -o crm_osb_neural.o

crm_bit_entropy.o: crm_bit_entropy.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_bit_entropy.c -o crm_bit_entropy.o

crm_osb_winnow.o: crm_osb_winnow.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_osb_winnow.c -o crm_osb_winnow.o

crm_correlate.o: crm_correlate.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_correlate.c -o crm_correlate.o

crm_debugger.o: crm_debugger.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_debugger.c -o crm_debugger.o

crm_str_funcs.o: crm_str_funcs.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crm_str_funcs.c -o crm_str_funcs.o

crmregex_tre.o: crmregex_tre.c \
	$(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c crmregex_tre.c -o crmregex_tre.o

crmregex_gnu.o: crmregex_gnu.c $(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -c crmregex_gnu.c -o crmregex_gnu.o

crm_util_errorhandlers.o: crm_util_errorhandlers.c $(HFILES)
	$(CC) $(CFLAGS) -D$(SYSTEMTYPE) -c crm_util_errorhandlers.c \
	-o crm_util_errorhandlers.o

cssutil: cssutil.c crm_css_maintenance.o crm_util_errorhandlers.o $(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c cssutil.c -o cssutil.o  
	$(CC)  $(CFLAGS) $(LDFLAGS) cssutil.o \
		crm_css_maintenance.o \
		crm_util_errorhandlers.o \
		crm_str_funcs.o -lm -ltre -o cssutil

osbf-util: osbf-util.c crm_osbf_maintenance.o crm_util_errorhandlers.o $(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c osbf-util.c -o osbf-util.o  
	$(CC)  $(CFLAGS) $(LDFLAGS) osbf-util.o \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		crm_osbf_maintenance.o -lm -ltre -o osbf-util

cssdiff: cssdiff.c crm_util_errorhandlers.o $(HFILES)
	$(CC)  $(CFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' -c cssdiff.c -o cssdiff.o
	$(CC)  $(CFLAGS) $(LDFLAGS) \
		crm_util_errorhandlers.o \
		crm_str_funcs.o -ltre cssdiff.o -o cssdiff

cssmerge: cssmerge.c crm_util_errorhandlers.o $(HFILES)
	$(CC)  $(CFLAGS) $(LDFLAGS) -D$(SYSTEMTYPE) -DVERSION='"$(VERSION)"' \
		crm_str_funcs.o \
		crm_util_errorhandlers.o \
		-ltre cssmerge.c -o cssmerge

clean:
	-rm -f crm114
	-rm -f cssutil
	-rm -f osbf-util
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
	$(MAKE) install_crm114 install_utils

install_crm114: crm114
	#
	#   NOTE - the default CRM114 now uses the TRE regex lib, not GNU
	#          If you want the GNU one (not recommended) you should
	#          use 'install_gnu' as the make target.
	#    
	$(MAKE) install_engine

install_utils:
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
	install $(INSTALLFLAGS) crm114 $(BINDIR)/crm$(VER_SUFFIX)

install_gnu: crm114_gnu
	install $(INSTALLFLAGS) crm114_gnu $(BINDIR)/crmg$(VER_SUFFIX)

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
	./megatest.sh  > megatest_test.log 2>&1
	diff megatest_knowngood.log megatest_test.log &	sleep 1


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



tarfile: all
	rm -rf crm114-$(VERSION).src
	mkdir crm114-$(VERSION).src
	cp $(CFILES) \
	$(HFILES) \
	$(UTILFILES) \
	$(CRMFILES) \
	$(TEXTFILES) \
	crm114-$(VERSION).src
	# cp -a $(TRE_TARFILE) crm114-$(VERSION).src
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

FORCE:
