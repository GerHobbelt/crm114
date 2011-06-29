## Process this file with automake to produce Makefile.in

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007-2009 Ger Hobbelt <ger@hobbelt.com>


#
#    Makefile for CRM114
#

DISTCLEANFILES = diff/
CLEANFILES = *.o *~ *.bak




TAR_CMDLINE  = ${TAR} cf - "$$tardir"




PACKAGE = @PACKAGE_TARNAME@
TAR_FILENAME_POSTFIX = @TAR_FILENAME_POSTFIX@
# VER_SUFFIX defines a version suffix for our installed executables, 
# handy when you want many versions of CRM114 coexisting.
#
# e.g.: VER_SUFFIX=27B-6, then if you make install, you get crm-27b-6
# 
VER_SUFFIX = @VER_SUFFIX@ 
VERSION = @VERSION@
ARCHIVE_BASE_NAME = @PACKAGE_TARNAME@-@PACKAGE_VERSION@-@TAR_FILENAME_POSTFIX@

SILENT = @SILENT@
DIFF_FLAGS = @DIFF_FLAGS@





DATE=`date +%Y_%m_%d_%H_%M_%S`

#
#      The following forces the compiler to be GCC.  If you have trouble
#      with your default compiler, and you want to force GCC, uncomment it.
# CC=gcc
#
#     What version is this software (PLEASE don't change this just 
#     to mess with my brain. - wsy)
#
#VERSION = @VERSION@
PREV_VERSION=20081023-BlameMaxwell
#PREV_VERSION=20080923-BlameVT
#PREV_VERSION=20080630-BlameVT
#PREV_VERSION=20080618-BlameBadGlasses
#PREV_VERSION=20080502-BlameJason
#PREV_VERSION=20080326-BlameSentansoken
#PREV_VERSION=20080325-BlameSentansoken
#PREV_VERSION=20080317-BlameJoeLangeway_VT
#PREV_VERSION=20080217-BlameJoeLangeway_VT
#PREV_VERSION=20071201-BlameLittleStevie_VT
#PREV_VERSION=20071126-BlameGerHobbelt
#PREV_VERSION=20071015-BlameGerHobbelt
#PREV_VERSION=20070917-BlameGerHobbelt
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
# CFLAGS += -g -O3
#     turn this one on for no optimization, but debugging and time profiling.
# CFLAGS += -g -O0 -Wall -pg
#     turn this one on for really picayune debugging.

# CPPFLAGS += -Wall
# CPPFLAGS += -Wextra 
# CPPFLAGS += -pedantic

#
#   Choose between static and dynamic linking (we recommend static linking)
#   Comment this out if you want dynamic linking
#
#    BSD users - do NOT use -static-libgcc!!!
#
#    use this one if you want program profiling.
# LDFLAGS += -static -static-libgcc -pg
#
#   Any standard install flags?  We nominally use protection 755
INSTALLFLAGS = @INSTALLFLAGS@
#INSTALLFLAGS += -m 755
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
# --------------End of Mac OSX stuff -----------------------
#
#
#    End of user-configurable options... if you modify anything below
#    this line, you risk early insanity and blindness.
#
#


ECHO = echo
E = $(SILENT) $(ECHO)

MEGATESTFILES = \
	mt_ng_Bit_Entropy_2.input \
	mt_ng_OSB_Markov_Chisquared_Unique_2.input \
	mt_ng_OSB_Markovian_1.input \
	mt_ng_OSB_Markov_Chisquared_Unique_1.input \
	mt_ng_String_Unigram_Hyperspace_2.input \
	mt_ng_String_Kernel_SVM_SKS_Unique_1.input \
	mt_ng_Support_Vector_Machine_SVM_1.input \
	mt_ng_Bit_Entropy_1.input \
	mt_ng_unigram_Winnow_1.input \
	mt_ng_Bytewise_Correlation_2.input \
	mt_ng_OSB_Winnow_1.input \
	mt_ng_Bit_Entropy_Toroid_1.input \
	mt_ng_Neural_Network_1.input \
	mt_ng_String_Kernel_SVM_SKS_1.input \
	mt_ng_OSB_3_letter_Hyperspace_1.input \
	mt_ng_Support_Vector_Machine_SVM_2.input \
	windowtest_fromvar_mt_ng_1.input \
	mt_ng_Neural_Network_2.input \
	mt_ng_Fast_Substring_Compression_Match_1.input \
	backwardstest_mt_ng_1.input \
	mt_ng_Unigram_Bayesian_2.input \
	mt_ng_Support_Vector_Machine_SVM_unigram_2.input \
	mt_ng_String_Hyperspace_2.input \
	mt_ng_SBPH_Markovian_1.input \
	mt_ng_OSB_Markov_Unique_2.input \
	mt_ng_Unigram_Hyperspace_1.input \
	mt_ng_Unigram_Bayesian_1.input \
	mt_ng_OSBF_Local_Confidence_Fidelis_2.input \
	mt_ng_String_Kernel_SVM_SKS_2.input \
	matchtest_mt_ng_1.input \
	mt_ng_OSB_Markovian_2.input \
	mt_ng_String_Unigram_Hyperspace_1.input \
	mt_ng_Unigram_Hyperspace_2.input \
	mt_ng_Vector_3_word_bag_Hyperspace_1.input \
	mt_ng_String_Hyperspace_1.input \
	approxtest_mt_ng_1.input \
	mt_ng_Clump_Pmulc_3.input \
	approxtest_mt_ng_2.input \
	mt_ng_OSB_Hyperspace_1.input \
	mt_ng_OSB_3_letter_Hyperspace_2.input \
	mt_ng_OSB_Hyperspace_2.input \
	mt_ng_Vector_3_word_bag_Hyperspace_2.input \
	mt_ng_Bit_Entropy_Toroid_2.input \
	windowtest_mt_ng_1.input \
	mt_ng_OSB_Markov_Unique_1.input \
	mt_ng_OSBF_Local_Confidence_Fidelis_1.input \
	mt_ng_unigram_Winnow_2.input \
	mt_ng_Fast_Substring_Compression_Match_2.input \
	backwardstest_mt_ng_2.input \
	mt_ng_Bytewise_Correlation_1.input \
	mt_ng_String_Kernel_SVM_SKS_Unique_2.input \
	mt_ng_Clump_Pmulc_1.input \
	mt_ng_OSB_Winnow_2.input \
	mt_ng_Clump_Pmulc_2.input \
	mt_ng_SBPH_Markovian_2.input \
	mt_ng_Support_Vector_Machine_SVM_unigram_1.input \
	COLOPHON_mt_ng_reference_3.input \
	FAQ_mt_ng_reference_4.input \
	INTRO_mt_ng_reference_2.input \
	QUICKREF_mt_ng_reference_1.input


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
	crm_stats.c crm_expr_sks.c \
	crm_expr_svm.c crm_fast_substring_compression.c \
	crm_generic_port.c crm_stdio_port.c \
	crm_versioning.c crm_core_profiling.c \
	crm_osb_markovian_alt.c crm_osb_bayes_alt.c crm_osb_hyperspace_alt.c \
	crm_osb_winnow_alt.c crm_osbf_bayes_alt.c

CFILES_4_BMP_PROF = \
	crm_bmp_image_io.cpp crm_bmp_prof_collect.cpp \
	crm_bmp_prof_report.cpp \
	crm_bmp_prof_scan.cpp crm_bmp_prof_utils.cpp

CFILES_4_LIBCRM114 = \
	libcrm114/if_kiss_bill.c \
	libcrm114/interface.c \
	libcrm114/hashtable_base.c \
	libcrm114/bayesian_base.c \
	libcrm114/hash_base.c \
	libcrm114/diag_base.c \
	libcrm114/bio_base.c \
	libcrm114/mmap_bio_base.c \
	libcrm114/socket_bio_base.c \
	libcrm114/memory_bio_base.c

HFILES_4_LIBCRM114 = \
	libcrm114/libcrm_statuscodes.h \
	libcrm114/libcrm_kiss_bill.h \
	libcrm114/libcrm_bio.h \
	libcrm114/libcrm_diag.h \
	libcrm114/libcrm114.h

HFILES = crm114_sysincludes.h crm114_structs.h crm114_config.h \
	crm114.h crm114_osbf.h config.h \
	config_vanilla_UNIX_sys_defaults.h crm_mjt_qsort.h \
	crm_pshpack2.h crm_pshpack1.h crm_pshpack4.h crm_poppack.h \
	crm_bmp_prof.h \
	../win32/config_win32.h

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
	crm_preprocessor.o crmregex_gnu.o crmregex_tre.o \
	crm_expr_sks.o crm_stats.o crm_expr_svm.o \
	crm_fast_substring_compression.o \
	crm_generic_port.o crm_stdio_port.o \
	crm_versioning.o crm_core_profiling.o \
	crm_osb_markovian_alt.o crm_osb_bayes_alt.o crm_osb_hyperspace_alt.o \
	crm_osb_winnow_alt.o crm_osbf_bayes_alt.o 





UTIL_C_FILES = crm_main.c osbf-util.c cssutil.c cssdiff.c cssmerge.c \
	testvectok.c crm_bmp_prof_main.cpp


CRMFILES = mailfilter.crm mailfilter.cf \
	mailreaver.crm mailtrainer.crm \
	mkcss.crm \
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
	gatlingsort.crm tenfold_validate.crm \
	test_svm.crm \
	classifymail.crm \
	tokendelimiterbugtest.crm bracesbugtest.crm \
	blowuptrapbugtest.crm trapseqbugtest.crm slashbugtest.crm \
	alius_w_comment.crm \
	userdirtest.crm \
	print_binary2decimal_int32.crm \
	match_re_fringe1.crm



TEXTFILES = COLOPHON.txt pad.dat \
	QUICKREF.txt INTRO.txt knownbugs.txt FAQ.txt things_to_do.txt \
	classify_details.txt README CRM114_Mailfilter_HOWTO.txt \
	procmailrc.recipe reto_procmail_recipe.recipe \
	priolist.mfp blacklist.mfp whitelist.mfp rewrites.mfp \
	priolist.mfp.example blacklist.mfp.example whitelist.mfp.example \
	crm114.spec \
	inoc_passwd.txt \
	GPL-License.txt \
	rename-gz \
	megatest.sh megatest_knowngood.log \
	make_vanilla_UNIX.sh Makefile.vanilla \
	exectest_s1.sh \
	$(MEGATESTFILES) megatest_ng.sh 



EXTRA_DIST = indent.sh.in diff_em.sh.in \
	$(CRMFILES) \
	$(TEXTFILES) \
	config_vanilla_UNIX_sys_defaults.h.in \
	../win32/config_win32.h.in



TESTS =



bin_PROGRAMS = crm114 cssutil osbf-util cssdiff cssmerge crm_bmp_prof
noinst_PROGRAMS = testvectok


crm114_SOURCES = $(CFILES) $(HFILES) $(EXTRA_DEPS) crm_main.c

cssutil_SOURCES = cssutil.c $(CFILES) $(HFILES) $(EXTRA_DEPS)

osbf_util_SOURCES = osbf-util.c $(CFILES) $(HFILES) $(EXTRA_DEPS)

cssdiff_SOURCES = cssdiff.c $(CFILES) $(HFILES) $(EXTRA_DEPS)

cssmerge_SOURCES = cssmerge.c $(CFILES) $(HFILES) $(EXTRA_DEPS)

testvectok_SOURCES = $(CFILES) $(HFILES) $(EXTRA_DEPS) testvectok.c

crm_bmp_prof_SOURCES = crm_bmp_prof_main.cpp $(CFILES_4_BMP_PROF) $(CFILES) $(HFILES) $(EXTRA_DEPS)

libcrm114_SOURCES = $(CFILES_4_LIBCRM114) $(HFILES) $(HFILES_4_LIBCRM114) $(EXTRA_DEPS)



# see for 'ELCFILES=' the documentation at http://www.gnu.org/software/automake/manual/automake.html#Emacs-Lisp
dist_lisp_LISP = crm114-mode.el
ELCFILES =




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



# #   
# #   Now building the TRE regex version of CRM114.  If you don't have
# #   TRE already, you will need to fetch it from the website and
# #   then ./configure --enable-static to get the static TRE libs,
# #   then build TRE, add /usr/local/lib to /etc/ld.so.conf, then 
# #   to set the library up.  Then you can build CRM114 itself.
# #
# crm114$(EXEEXT): $(OFILES) crm_main.o
# 	$(E) ""
# 	$(E) "   Now building the TRE regex version of CRM114.  If you don't have"
# 	$(E) "   TRE already, you will need to fetch it from the website and"
# 	$(E) "   then ./configure --enable-static to get the static TRE libs,"
# 	$(E) "   then build TRE, add /usr/local/lib to /etc/ld.so.conf, then"
# 	$(E) "   to set the library up.  Then you can build CRM114 itself."
# 	$(E) ""
# 	$(CC) $(LDFLAGS) $(LIBS) crm_main.o $(OFILES) \
# 	  -o crm114$(EXEEXT)
# 	$(E) ""
# 	$(E) "   Done.  Type \"make help\" to see what else you can make."
# 	$(E) ""
# 


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

crm_expr_sks.o: crm_expr_sks.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_svm.o: crm_expr_svm.c \
	$(HFILES) $(EXTRA_DEPS)

crm_fast_substring_compression.o: crm_fast_substring_compression.c \
	$(HFILES) $(EXTRA_DEPS)

crm_expr_clump.o: crm_expr_clump.c \
	$(HFILES) $(EXTRA_DEPS)

crm_stats.o: crm_stats.c \
	$(HFILES) $(EXTRA_DEPS)

crm_neural_net.o: crm_neural_net.c \
	$(HFILES) $(EXTRA_DEPS)

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

crm_core_profiling.o: crm_core_profiling.c \
	$(HFILES) $(EXTRA_DEPS)

crm_osb_markovian_alt.o: crm_osb_markovian_alt.c \
	$(HFILES) $(EXTRA_DEPS)
	
crm_osb_bayes_alt.o: crm_osb_bayes_alt.c \
	$(HFILES) $(EXTRA_DEPS)
	
crm_osb_hyperspace_alt.o: crm_osb_hyperspace_alt.c \
	$(HFILES) $(EXTRA_DEPS)
	
crm_osb_winnow_alt.o: crm_osb_winnow_alt.c \
	$(HFILES) $(EXTRA_DEPS)
	
crm_osbf_bayes_alt.o: crm_osbf_bayes_alt.c \
	$(HFILES) $(EXTRA_DEPS)

crm_bmp_prof_main.o: crm_bmp_prof_main.cpp \
	$(HFILES) $(EXTRA_DEPS)

crm_bmp_image_io.o: crm_bmp_image_io.cpp \
	$(HFILES) $(EXTRA_DEPS)

crm_bmp_prof_collect.o: crm_bmp_prof_collect.cpp \
	$(HFILES) $(EXTRA_DEPS)

crm_bmp_prof_report.o: crm_bmp_prof_report.cpp \
	$(HFILES) $(EXTRA_DEPS)

crm_bmp_prof_scan.o: crm_bmp_prof_scan.cpp \
	$(HFILES) $(EXTRA_DEPS)

crm_bmp_prof_utils.o: crm_bmp_prof_utils.cpp \
	$(HFILES) $(EXTRA_DEPS)








clean-local:
	-rm -f crm114$(EXEEXT)
	-rm -f cssutil$(EXEEXT)
	-rm -f osbf-util$(EXEEXT)
	-rm -f cssdiff$(EXEEXT)
	-rm -f cssmerge$(EXEEXT)
	-rm -f testvectok$(EXEEXT)
	-rm -f crm_test_hashes$(EXEEXT)
	-rm -f crm_bmp_prof$(EXEEXT)
	-rm -f crm114_gnu$(EXEEXT)
	-rm -f crm114_tre$(EXEEXT)
	-rm -f crm114
	-rm -f cssutil
	-rm -f osbf-util
	-rm -f cssdiff
	-rm -f cssmerge
	-rm -f testvectok
	-rm -f crm_test_hashes
	-rm -f crm_bmp_prof
	-rm -f crm114_gnu
	-rm -f crm114_tre
	-rm -f *.o
	-rm -rf diff/
	$(E) "   if the next line doesn't work in your version of make,"
	$(E) "    don't worry.  It just means you have TRE in another place."
	$(E) "    You can cd to the TRE directory and 'make clean' manually"
	-( [ "$(TRE_TARFILE)" ] && cd $(TRE_TARFILE) && make clean )




install-exec-local: install_engine install_utils
	$(E) ""
	$(E) "    Done.  Type "make help" to see what else you can make."
	$(E) ""


#
#    Installing Utilities
#	
install_utils: cssmerge$(EXEEXT) cssutil$(EXEEXT) cssdiff$(EXEEXT) osbf-util$(EXEEXT)
	$(E) ""
	$(E) "    Installing Utilities"
	$(E) ""
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	install $(INSTALLFLAGS) cssdiff$(EXEEXT)   $(DESTDIR)$(bindir)/cssdiff$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) cssmerge$(EXEEXT)  $(DESTDIR)$(bindir)/cssmerge$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) cssutil$(EXEEXT)   $(DESTDIR)$(bindir)/cssutil$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) osbf-util$(EXEEXT) $(DESTDIR)$(bindir)/osbf-util$(VER_SUFFIX)$(EXEEXT)
	$(E) "   if you don't have EMACS installed in the default place, you"
	$(E) "   won't get the crm114-mode.el automatically."

#	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)



# use different name so we can keep it around along with default one
# and same VER_SUFFIX postfix 

install_engine: crm114$(EXEEXT)
	$(E) ""
	$(E) "    Installing engine"
	$(E) ""
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	install $(INSTALLFLAGS) crm114$(EXEEXT) $(DESTDIR)$(bindir)/crm114$(VER_SUFFIX)$(EXEEXT)


install_binary_only: crm114$(EXEEXT) cssdiff$(EXEEXT) cssmerge$(EXEEXT) cssutil$(EXEEXT) osbf-util$(EXEEXT)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	install $(INSTALLFLAGS) crm114$(EXEEXT)    $(DESTDIR)$(bindir)/crm114$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) cssdiff$(EXEEXT)   $(DESTDIR)$(bindir)/cssdiff$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) cssmerge$(EXEEXT)  $(DESTDIR)$(bindir)/cssmerge$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) cssutil$(EXEEXT)   $(DESTDIR)$(bindir)/cssutil$(VER_SUFFIX)$(EXEEXT)
	install $(INSTALLFLAGS) osbf-util$(EXEEXT) $(DESTDIR)$(bindir)/osbf-util$(VER_SUFFIX)$(EXEEXT)

#	-install $(INSTALLFLAGS) crm114-mode.el $(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


install-exec-hook: crm114$(EXEEXT)
	cd $(DESTDIR)$(bindir) && $(LN_S) crm114$(VER_SUFFIX)$(EXEEXT) crm


uninstall-hook:
	-rm -f $(DESTDIR)$(bindir)/crm


uninstall-local:
	-rm -f $(DESTDIR)$(bindir)/crm114$(VER_SUFFIX)$(EXEEXT)
	-rm -f $(DESTDIR)$(bindir)/cssdiff$(VER_SUFFIX)$(EXEEXT)
	-rm -f $(DESTDIR)$(bindir)/cssmerge$(VER_SUFFIX)$(EXEEXT)
	-rm -f $(DESTDIR)$(bindir)/cssutil$(VER_SUFFIX)$(EXEEXT)
	-rm -f $(DESTDIR)$(bindir)/osbf-util$(VER_SUFFIX)$(EXEEXT)

#	-rm -f $(DESTDIR)$(prefix)/share/emacs/site-lisp/crm114-mode.el$(VER_SUFFIX)


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
megatest: crm114$(EXEEXT) megatest.sh megatest_knowngood.log $(MEGATESTFILES) 
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
	$(srcdir)/megatest.sh 2 > megatest_test.log 2>&1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (unified)       ====================="
	$(E) "=============================================================="
	$(DIFF) -u $(DIFF_FLAGS) $(srcdir)/megatest_knowngood.log megatest_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(DIFF) $(DIFF_FLAGS) $(srcdir)/megatest_knowngood.log megatest_test.log & sleep 1



megatest_ng: crm114$(EXEEXT) megatest_ng.sh $(MEGATESTFILES) megatest_knowngood.log
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
	$(srcdir)/megatest_ng.sh 2 > megatest_ng_test.log 2>&1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (unified)       ====================="
	$(E) "=============================================================="
	$(DIFF) -u $(DIFF_FLAGS) $(srcdir)/megatest_knowngood.log megatest_ng_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(DIFF) $(DIFF_FLAGS) $(srcdir)/megatest_knowngood.log megatest_ng_test.log & sleep 1






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
	$(E) "   MUCH STUFF DEPRECATED!!!  Use mailreaver.crm instead..."
	$(builddir)/cssutil -b -r spam.css
	$(builddir)/cssutil -b -r nonspam.css
	cp spam.css spam__$(DATE).txt
	cp nonspam.css nonspam__$(DATE).txt
	$(E) ""
	$(E) "   run mailtrainer.crm on the spam.dir and good.dir directories."
	$(srcdir)/mailtrainer.crm \
		--good=./good.dir/ \
		--spam=./spam.dir/ \
		--repeat=5 \
		--random
	$(E) "   If all went well with the above, you will have backup copies of"
	$(E) "   your spam and nonspam .css files."
	$(E) "   You may now delete these files with impunity, or you may "
	$(E) "   choose to keep them around as backup."




rsync: $(CFILES) $(HFILES) $(UTIL_C_FILES) $(CFILES_4_BMP_PROF) $(CRMFILES) $(TEXTFILES) $(EXTRA_DIST) \
		config.h.in \
		config_vanilla_UNIX_sys_defaults.h.in \
		../win32/config_win32.h.in
	rsync -zrltDv \
	  $(CFILES) $(HFILES) $(UTIL_C_FILES) $(CFILES_4_BMP_PROF) $(CRMFILES) $(TEXTFILES) $(EXTRA_DIST) \
		config.h.in \
		config_vanilla_UNIX_sys_defaults.h.in \
		../win32/config_win32.h.in
	  wsy@crm114.sourceforge.net:/home/groups/c/cr/crm114/htdocs/src/



diff: diff_em.sh $(CFILES) $(HFILES) $(UTIL_C_FILES) $(CFILES_4_BMP_PROF) $(CRMFILES) $(TEXTFILES) $(EXTRA_DIST)
	-( sh ./diff_em.sh );



# original: indent -npcs -nprs -npsl -nsaf -nsai -nsaw -nut -bli0 -nlp *.c *.h
# indent -bad -bap -bl -bli0 -bls -cbi4 -cdw -cli0 -di8 -hnl -i4 -l80 -lp -nbc -nce -ncs -npcs -nprs -npsl -nsaf -nsai -nsaw -nsob -nut -pi4 -sc -ss -T index_t -T hitcount_t -T regex_t -T regmatch_t -T regamatch_t -T regaparams_t -T Qitem_t -T regoff_t -T tre_char_t -T COOCCURRENCE_SCORE_TYPE *.c *.h
reindent: indent.sh $(CFILES) $(HFILES) $(UTIL_C_FILES) $(CFILES_4_BMP_PROF) \
		config.h.in \
		config_vanilla_UNIX_sys_defaults.h.in \
		../win32/config_win32.h.in
	-( sh ./indent.sh $(CFILES) $(HFILES) $(UTIL_C_FILES) $(CFILES_4_BMP_PROF) \
		config.h.in \
		config_vanilla_UNIX_sys_defaults.h.in \
		../win32/config_win32.h.in \
	 )





.PHONY: reindent depend rsync done_all megatest help clean install uninstall-hook uninstall test megatest_ng megatest




depend:
	makedepend -- $(CFLAGS) -- *.[ch]

# DO NOT DELETE THIS LINE -- make depend depends on it.
