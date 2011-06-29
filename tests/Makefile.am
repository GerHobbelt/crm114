## Process this file with automake to produce Makefile.in

## $Id: Makefile.am,v 1.7 2004/08/18 13:26:30 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007-2008 Ger Hobbelt


DISTCLEANFILES =
CLEANFILES = *~ *.bak $(refdir)/*~ $(refdir)/*.bak


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


TAR_CMDLINE  = ${TAR_BIN} cf - "$$tardir"



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




testdir = $(datadir)/doc/$(PACKAGE)/examples/tests
refdir = $(srcdir)/ref




ECHO = echo
E = $(SILENT) $(ECHO)





MEGATEST_INPUT_FILES = \
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

TEST_REFERENCE_FILES =                            \
	$(refdir)/String_Hyperspace_test1.step4.refoutput \
        $(refdir)/OSBF_Local_Confidence_test1.step2.refoutput \
        $(refdir)/Neural_Network_test1.step2.refoutput \
        $(refdir)/Bit_Entropy_Toroid_test1.step4.refoutput \
        $(refdir)/Support_Vector_Machine_test1.step4.refoutput \
        $(refdir)/Support_Vector_Machine_Unigram_test1.step1.refoutput \
        $(refdir)/OSB_Markovian_test1.step3.refoutput \
        $(refdir)/Unigram_Winnow_test1.step4.refoutput \
        $(refdir)/String_Unigram_Hyperspace_test1.step1.refoutput \
        $(refdir)/Support_Vector_Machine_test1.step1.refoutput \
        $(refdir)/OSB_Markovian_test1.step1.refoutput \
        $(refdir)/syscall_exit_test.refoutput \
        $(refdir)/Unigram_Winnow_test1.step3.refoutput \
        $(refdir)/OSB_Winnow_test1.step1.refoutput \
        $(refdir)/windowtest_fromvar.crm.refoutput \
        $(refdir)/Neural_Network_test1.step4.refoutput \
        $(refdir)/OSB_Markov_Chisquared_Unique_test1.step2.refoutput \
        $(refdir)/backwardstest.crm.2.refoutput \
        $(refdir)/rewritetest.crm.refoutput \
        $(refdir)/Unigram_Winnow_test1.step1.refoutput \
        $(refdir)/quine.crm.refoutput \
        $(refdir)/Vector_3_word_bag_Hyperspace_test1.step1.refoutput \
        $(refdir)/OSB_Winnow_test1.step9.refoutput \
        $(refdir)/match_isolate_test.crm.refoutput \
        $(refdir)/paolo_ov2.crm.refoutput \
        $(refdir)/mathrpntest.crm.refoutput \
        $(refdir)/Fast_Substring_Compression_Match_test1.step4.refoutput \
        $(refdir)/Vector_3_word_bag_Hyperspace_test1.step2.refoutput \
        $(refdir)/OSB_Markov_Unique_test1.step3.refoutput \
        $(refdir)/String_Kernel_SVM_Unique_test1.step5.refoutput \
        $(refdir)/paolo_ov3.crm.refoutput \
        $(refdir)/bracktest.crm.refoutput \
        $(refdir)/OSB_Winnow_test1.step6.refoutput \
        $(refdir)/windowtest.crm.refoutput \
        $(refdir)/Unigram_Hyperspace_test1.step2.refoutput \
        $(refdir)/paolo_ov4.crm.refoutput \
        $(refdir)/Bit_Entropy_Toroid_test1.step1.refoutput \
        $(refdir)/escapetest.crm.refoutput \
        $(refdir)/Support_Vector_Machine_test1.step5.refoutput \
        $(refdir)/OSB_Winnow_test1.step5.refoutput \
        $(refdir)/Unigram_Bayesian_test1.step2.refoutput \
        $(refdir)/String_Hyperspace_test1.step3.refoutput \
        $(refdir)/Unigram_Bayesian_test1.step4.refoutput \
        $(refdir)/defaulttest.crm.refoutput \
        $(refdir)/syscall123_test.refoutput \
        $(refdir)/inserttest_a.crm.refoutput \
        $(refdir)/OSBF_Local_Confidence_test1.step3.refoutput \
        $(refdir)/SBPH_Markovian_test1.step2.refoutput \
        $(refdir)/OSBF_Local_Confidence_test1.step1.refoutput \
        $(refdir)/OSB_Hyperspace_test1.step2.refoutput \
        $(refdir)/Unigram_Bayesian_test1.step1.refoutput \
        $(refdir)/Clump_Pmulc_test1.step1.refoutput \
        $(refdir)/OSB_Markovian_test1.step4.refoutput \
        $(refdir)/Clump_Pmulc_test1.step2.refoutput \
        $(refdir)/matchtest.crm.refoutput \
        $(refdir)/Unigram_Hyperspace_test1.step3.refoutput \
        $(refdir)/Clump_Pmulc_test1.step3.refoutput \
        $(refdir)/Bit_Entropy_test1.step3.refoutput \
        $(refdir)/skudtest.crm.refoutput \
        $(refdir)/beeptest.crm.refoutput \
        $(refdir)/SBPH_Markovian_test1.step1.refoutput \
        $(refdir)/statustest.crm.refoutput \
        $(refdir)/OSB_3_letter_Hyperspace_test1.step2.refoutput \
        $(refdir)/overalterisolatedtest.crm.refoutput \
        $(refdir)/match_isolate_reclaim.crm.refoutput \
        $(refdir)/Unigram_Winnow_test1.step2.refoutput \
        $(refdir)/OSB_Markov_Chisquared_Unique_test1.step3.refoutput \
        $(refdir)/Bit_Entropy_test1.step4.refoutput \
        $(refdir)/SBPH_Markovian_test1.step3.refoutput \
        $(refdir)/call_return_test.crm.refoutput \
        $(refdir)/Fast_Substring_Compression_Match_test1.step2.refoutput \
        $(refdir)/unionintersecttest.crm.refoutput \
        $(refdir)/Neural_Network_test1.step6.refoutput \
        $(refdir)/approxtest.crm.refoutput \
        $(refdir)/randomiotest.crm.refoutput \
        $(refdir)/String_Unigram_Hyperspace_test1.step3.refoutput \
        $(refdir)/Clump_Pmulc_test1.step5.refoutput \
        $(refdir)/OSB_Hyperspace_test1.step3.refoutput \
        $(refdir)/String_Unigram_Hyperspace_test1.step2.refoutput \
        $(refdir)/paolo_overvars.crm.refoutput \
        $(refdir)/String_Kernel_SVM_test1.step2.refoutput \
        $(refdir)/Unigram_Bayesian_test1.step3.refoutput \
        $(refdir)/OSBF_Local_Confidence_test1.step4.refoutput \
        $(refdir)/OSB_Markov_Unique_test1.step4.refoutput \
        $(refdir)/OSB_Markov_Chisquared_Unique_test1.step1.refoutput \
        $(refdir)/eval_infiniteloop.crm.refoutput \
        $(refdir)/Fast_Substring_Compression_Match_test1.step3.refoutput \
        $(refdir)/Bit_Entropy_test1.step2.refoutput \
        $(refdir)/String_Kernel_SVM_Unique_test1.step3.refoutput \
        $(refdir)/zz_translate_test.crm.refoutput \
        $(refdir)/String_Hyperspace_test1.step1.refoutput \
        $(refdir)/OSB_Markov_Unique_test1.step2.refoutput \
        $(refdir)/Clump_Pmulc_test1.step4.refoutput \
        $(refdir)/Vector_3_word_bag_Hyperspace_test1.step3.refoutput \
        $(refdir)/Unigram_Hyperspace_test1.step4.refoutput \
        $(refdir)/String_Kernel_SVM_test1.step1.refoutput \
        $(refdir)/fataltraptest.crm.refoutput \
        $(refdir)/Neural_Network_test1.step1.refoutput \
        $(refdir)/paolo_ov5.crm.refoutput \
        $(refdir)/String_Unigram_Hyperspace_test1.step4.refoutput \
        $(refdir)/String_Kernel_SVM_Unique_test1.step2.refoutput \
        $(refdir)/OSB_3_letter_Hyperspace_test1.step1.refoutput \
        $(refdir)/String_Kernel_SVM_test1.step5.refoutput \
        $(refdir)/Support_Vector_Machine_test1.step2.refoutput \
        $(refdir)/SBPH_Markovian_test1.step4.refoutput \
        $(refdir)/crm114_version.refoutput \
        $(refdir)/syscall_jibber_test.refoutput \
        $(refdir)/Unigram_Hyperspace_test1.step1.refoutput \
        $(refdir)/OSB_Winnow_test1.step2.refoutput \
        $(refdir)/Support_Vector_Machine_Unigram_test1.step2.refoutput \
        $(refdir)/OSB_Winnow_test1.step10.refoutput \
        $(refdir)/Unigram_Winnow_test1.step6.refoutput \
        $(refdir)/translate_tr.crm.refoutput \
        $(refdir)/Neural_Network_test1.step5.refoutput \
        $(refdir)/Unigram_Winnow_test1.step5.refoutput \
        $(refdir)/alternating_example_neural.crm.refoutput \
        $(refdir)/Support_Vector_Machine_Unigram_test1.step5.refoutput \
        $(refdir)/Bytewise_Correlation_test1.step1.refoutput \
        $(refdir)/Bit_Entropy_Toroid_test1.step3.refoutput \
        $(refdir)/OSB_Markov_Unique_test1.step1.refoutput \
        $(refdir)/Bit_Entropy_Toroid_test1.step2.refoutput \
        $(refdir)/Support_Vector_Machine_test1.step3.refoutput \
        $(refdir)/String_Kernel_SVM_test1.step3.refoutput \
        $(refdir)/Support_Vector_Machine_Unigram_test1.step4.refoutput \
        $(refdir)/OSB_Winnow_test1.step7.refoutput \
        $(refdir)/backwardstest.crm.refoutput \
        $(refdir)/OSB_3_letter_Hyperspace_test1.step4.refoutput \
        $(refdir)/Bytewise_Correlation_test1.step2.refoutput \
        $(refdir)/indirecttest.crm.refoutput \
        $(refdir)/Neural_Network_test1.step3.refoutput \
        $(refdir)/defaulttest.crm.2.refoutput \
        $(refdir)/String_Kernel_SVM_Unique_test1.step1.refoutput \
        $(refdir)/Vector_3_word_bag_Hyperspace_test1.step4.refoutput \
        $(refdir)/OSB_Winnow_test1.step8.refoutput \
        $(refdir)/Bit_Entropy_test1.step1.refoutput \
        $(refdir)/String_Kernel_SVM_Unique_test1.step4.refoutput \
        $(refdir)/mathalgtest.crm.refoutput \
        $(refdir)/Fast_Substring_Compression_Match_test1.step1.refoutput \
        $(refdir)/String_Hyperspace_test1.step2.refoutput \
        $(refdir)/String_Kernel_SVM_test1.step4.refoutput \
        $(refdir)/OSB_Winnow_test1.step4.refoutput \
        $(refdir)/OSB_Winnow_test1.step3.refoutput \
        $(refdir)/OSB_Markov_Chisquared_Unique_test1.step4.refoutput \
        $(refdir)/Support_Vector_Machine_Unigram_test1.step3.refoutput \
        $(refdir)/OSB_Hyperspace_test1.step4.refoutput \
        $(refdir)/OSB_Hyperspace_test1.step1.refoutput \
        $(refdir)/OSB_3_letter_Hyperspace_test1.step3.refoutput \
        $(refdir)/OSB_Markovian_test1.step2.refoutput


test_SCRIPTS = \
	matchtest.crm windowtest.crm overalterisolatedtest.crm exectest.crm \
	learntest.crm classifytest.crm \
	escapetest.crm argtest.crm beeptest.crm skudtest.crm aliustest.crm \
	nestaliustest.crm eval_infiniteloop.crm \
	traptest.crm fataltraptest.crm uncaughttraptest.crm \
	inserttest_a.crm inserttest_b.crm inserttest_c.crm \
	backwardstest.crm approxtest.crm \
	mathalgtest.crm mathrpntest.crm \
	indirecttest.crm translate_tr.crm zz_translate_test.crm \
	rewritetest.crm \
	randomiotest.crm isolate_reclaim_test.crm \
	match_isolate_test.crm match_isolate_reclaim.crm \
	call_return_test.crm defaulttest.crm alternating_example_neural.crm \
	quine.crm \
	bracktest.crm unionintersecttest.crm \
	statustest.crm windowtest_fromvar.crm paolo_overvars.crm \
	tenfold_validate.crm \
	tokendelimiterbugtest.crm bracesbugtest.crm \
	blowuptrapbugtest.crm trapseqbugtest.crm slashbugtest.crm \
	alius_w_comment.crm \
	userdirtest.crm \
	paolo_ov2.crm \
	paolo_ov3.crm \
	paolo_ov4.crm \
	paolo_ov5.crm \
	print_binary2decimal_int32.crm



EXTRA_DIST = megatest_knowngood.log \
	test_rewrites.mfp \
	$(test_SCRIPTS) \
	$(MEGATEST_INPUT_FILES) $(TEST_REFERENCE_FILES) \
	megatest.bat \
	megatest.sh.in \
	megatest_ng.sh.in \
	testscript.sh.in \
	statustest.filter.sh.in \
	crm114_error_report.filter.sh.in \
	crm114_versiontest.filter.sh.in \
	crm114_classify.filter.sh.in \
	crm114_classify.filter.awk \
	whitelist.mfp.example
	


# WARNING: the 'ref' files should NOT be a dependency in this set, as that would prevent us
#          from being able to auto-generate the reference files when those are absent.
TEST_PREREQUISITES = 	\
	$(MEGATEST_INPUT_FILES) \
	ref_dir \
	crm114_binaries \
	testscript.sh \
	statustest.filter.sh \
	crm114_error_report.filter.sh \
	crm114_versiontest.filter.sh \
	crm114_classify.filter.sh \
	crm114_classify.filter.awk




# TESTS = test_basics test_other test_megatest_ng test_mailfilter test_mailreaver
TESTS = 

check_PROGRAMS = 



test: test_basics test_other test_classifiers test_mailfilter test_mailreaver


crm114_binaries: 
	$(E) "Creating diff archives"; 
	(cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) all);
	
ref_dir:
	$(MKDIR_P) $(refdir)


# original BillY test script:
test_megatest: $(test_SCRIPTS) $(MEGATESTFILES) megatest_knowngood.log megatest.sh
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
	./megatest.sh 2 > megatest_test.log 2>&1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (unified)       ====================="
	$(E) "=============================================================="
	$(DIFF_BIN) -u $(DIFF_FLAGS) megatest_knowngood.log megatest_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(DIFF_BIN) $(DIFF_FLAGS) megatest_knowngood.log megatest_test.log & sleep 1


# 'NextGen', i.e. an edited ('improved') version of BillY's test script:
test_megatest_ng: $(test_SCRIPTS) $(MEGATESTFILES) megatest_knowngood.log megatest_ng.sh
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
	./megatest_ng.sh 2 > megatest_ng_test.log 2>&1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (unified)       ====================="
	$(E) "=============================================================="
	$(DIFF_BIN) -u $(DIFF_FLAGS) megatest_knowngood.log megatest_ng_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(DIFF_BIN) $(DIFF_FLAGS) megatest_knowngood.log megatest_ng_test.log & sleep 1






# series of basic regression tests for CRM114:
test_basics:                    \
		initial_tests1	\
		initial_tests2 
	$(E) "************************************"
	$(E) "* test_basics CALLED"
	$(E) "************************************"


# test more complex CRM scripts in ../src
test_other:                    \
		further_tests  
	$(E) "************************************"
	$(E) "* test_other CALLED"
	$(E) "************************************"


# test the mailfilter script
test_mailfilter:
	$(E) "************************************"
	$(E) "* test_mailfilter CALLED"
	$(E) "************************************"


# test the mailreaver script
test_mailreaver:
	$(E) "************************************"
	$(E) "* test_mailreaver CALLED"
	$(E) "************************************"


test_classifiers:			\
		all_classifier_tests
	$(E) "************************************"
	$(E) "* test_classifiers CALLED"
	$(E) "************************************"



test_timing_seconds_announcement:
	$(E) "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	$(E) ""
	$(E) "   This runs a moderately interesting set of base tests"
	$(E) "   to exercise much of CRM114 under TRE.  This takes several"
	$(E) "   seconds to run on a 3 GHz AMD/XP machine.  Please"
	$(E) "   be patient; you (hopefully) won't see anything till the"
	$(E) "   full set of tests complete.  If you didn't use TRE, all"
	$(E) "   bets are off."
	$(E) ""
	$(E) "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

test_timing_minutes_announcement:
	$(E) "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	$(E) ""
	$(E) "   This runs a moderately interesting set of base tests"
	$(E) "   to exercise much of CRM114 under TRE.  This takes about"
	$(E) "   *** 15 MINUTES *** to run on a 3 GHz AMD/XP machine.  Please"
	$(E) "   be patient; you (hopefully) won't see anything till the"
	$(E) "   full set of tests complete.  If you didn't use TRE, all"
	$(E) "   bets are off."
	$(E) ""
	$(E) "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

#	-sleep 10


# check everything except neural: too slow :-(
check-local:                    



bogus-local1:
		initial_tests1	\
		initial_tests2  \
		further_tests  \
		markovian_classifier_tests \
		OSBF_classifier_tests \
		OSB_classifier_tests \
		Hyperspace_classifier_tests \
		Bit_Entropy_classifier_tests \
		FSCM_classifier_tests \
		SVM_SKS_classifier_tests \
		Correlator_classifier_tests \
		CLUMP_classifier_tests
	$(E) "*******************************************"
	$(E) "*               TESTING DONE!"
	$(E) "*  All 'megatest'-derived test cases have"
	$(E) "*  been checked: OK (some may have been"
	$(E) "*  'OVERRIDDEN', please check the output"
	$(E) "*  when you worry about those..."
	$(E) "*"
	$(E) "*******************************************"




script:                      \
		initial_tests1	\
		initial_tests2  \
		further_tests  \
		markovian_classifier_tests \
		OSBF_classifier_tests \
		OSB_classifier_tests \
		Hyperspace_classifier_tests \
		Bit_Entropy_classifier_tests \
		FSCM_classifier_tests \
		Neural_Net_classifier_tests \
		SVM_SKS_classifier_tests \
		Correlator_classifier_tests \
		CLUMP_classifier_tests
	$(E) "************************************"
	$(E) "* test group: script (all)"
	$(E) "************************************"


initial_tests1:              \
		version_test \
		bracktest_test \
		escapetest_test \
		fataltraptest_test \
		inserttest_a_test \
		matchtest_test \
		backwardstest_test1 \
		backwardstest_test2 \
		overalterisolatedtest_test \
		rewritetest_test \
		skudtest_test \
		statustest_test \
		unionintersecttest_test \
		beeptest_test \
		defaulttest_test1 \
		defaulttest_test2 \
		windowtest_test \
		windowtest_fromvar_test \
		approxtest_test \
		mathalgtest_test \
		mathrpntest_test \
		eval_infiniteloop_test \
		randomiotest_test \
		paolo_overvars_test \
		paolo_ov2_test \
		paolo_ov3_test \
		paolo_ov4_test \
		paolo_ov5_test

# slower tests:
initial_tests2:              \
		match_isolate_test \
		match_isolate_reclaim_test \
		call_return_test \
		translate_tr_test \
		zz_translate_test \
		quine_test

further_tests:                            \
		syscall123_test \
		syscall_exit_test \
		syscall_jibber_test \
		indirecttest_test

all_classifier_tests:                        \
		markovian_classifier_tests \
		OSBF_classifier_tests \
		OSB_classifier_tests \
		Hyperspace_classifier_tests \
		Bit_Entropy_classifier_tests \
		FSCM_classifier_tests \
		Neural_Net_classifier_tests \
		SVM_SKS_classifier_tests \
		Correlator_classifier_tests \
		CLUMP_classifier_tests




# classifier tests grouped:

markovian_classifier_tests:                        \
		SBPH_Markovian_test1 \
		OSB_Markovian_test1 \
		OSB_Markov_Unique_test1 \
		OSB_Markov_Chisquared_Unique_test1

OSBF_classifier_tests:                        \
		OSBF_Local_Confidence_test1

OSB_classifier_tests:                        \
		OSB_Winnow_test1 \
		Unigram_Bayesian_test1 \
		Unigram_Winnow_test1

Hyperspace_classifier_tests:                        \
		OSB_Hyperspace_test1 \
		OSB_3_letter_Hyperspace_test1 \
		Unigram_Hyperspace_test1 \
		String_Hyperspace_test1 \
		String_Unigram_Hyperspace_test1 \
		Vector_3_word_bag_Hyperspace_test1

Bit_Entropy_classifier_tests:                        \
		Bit_Entropy_test1 \
		Bit_Entropy_Toroid_test1

FSCM_classifier_tests:                        \
		Fast_Substring_Compression_Match_test1

Neural_Net_classifier_tests:                        \
		Neural_Network_test1 \
		Neural_Network_test2

SVM_SKS_classifier_tests:                        \
		Support_Vector_Machine_Unigram_test1 \
		Support_Vector_Machine_test1 \
		String_Kernel_SVM_test1 \
		String_Kernel_SVM_Unique_test1

Correlator_classifier_tests:                        \
		Bytewise_Correlation_test1

CLUMP_classifier_tests:                        \
		Clump_Pmulc_test1













version_test: $(TEST_PREREQUISITES)
	$(SILENT)\
	./testscript.sh '-{}' "" "$(refdir)/crm114_version.refoutput" "$(builddir)/crm114_versiontest.filter.sh" -v

bracktest_test: $(TEST_PREREQUISITES) bracktest.crm
	$(SILENT)\
	./testscript.sh bracktest.crm "" -

escapetest_test: $(TEST_PREREQUISITES) escapetest.crm
	$(SILENT)\
	./testscript.sh escapetest.crm "" -

fataltraptest_test: $(TEST_PREREQUISITES) fataltraptest.crm
	$(SILENT)\
	./testscript.sh fataltraptest.crm "" - "$(builddir)/crm114_error_report.filter.sh"

inserttest_a_test: $(TEST_PREREQUISITES) inserttest_a.crm
	$(SILENT)\
	./testscript.sh inserttest_a.crm "" -

matchtest_test: $(TEST_PREREQUISITES) matchtest.crm
	$(SILENT)\
	./testscript.sh matchtest.crm $(srcdir)/matchtest_mt_ng_1.input - "$(builddir)/crm114_error_report.filter.sh"

backwardstest_test1: $(TEST_PREREQUISITES) backwardstest.crm
	$(SILENT)\
	./testscript.sh backwardstest.crm $(srcdir)/backwardstest_mt_ng_1.input -

backwardstest_test2: $(TEST_PREREQUISITES) backwardstest.crm
	$(SILENT)\
	./testscript.sh backwardstest.crm $(srcdir)/backwardstest_mt_ng_2.input "$(refdir)/backwardstest.crm.2.refoutput"

overalterisolatedtest_test: $(TEST_PREREQUISITES) overalterisolatedtest.crm
	$(SILENT)\
	./testscript.sh overalterisolatedtest.crm "" -

rewritetest_test: $(TEST_PREREQUISITES) rewritetest.crm $(srcdir)/test_rewrites.mfp
	$(SILENT)\
	if [ ! -f "$(builddir)/test_rewrites.mfp" ]; then               \
          cp $(srcdir)/test_rewrites.mfp $(builddir)/test_rewrites.mfp; \
        fi;                                                             \
	./testscript.sh rewritetest.crm "" -

skudtest_test: $(TEST_PREREQUISITES) skudtest.crm $(srcdir)/whitelist.mfp.example
	$(SILENT)\
	if [ ! -f "$(builddir)/whitelist.mfp.example" ]; then                   \
          cp $(srcdir)/whitelist.mfp.example $(builddir)/whitelist.mfp.example; \
        fi;                                                                     \
	./testscript.sh skudtest.crm "" -

statustest_test: $(TEST_PREREQUISITES) statustest.crm
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh statustest.crm "" - "$(builddir)/statustest.filter.sh"

unionintersecttest_test: $(TEST_PREREQUISITES) unionintersecttest.crm
	$(SILENT)\
	./testscript.sh unionintersecttest.crm "" -

beeptest_test: $(TEST_PREREQUISITES) beeptest.crm
	$(SILENT)\
	./testscript.sh beeptest.crm "" -

defaulttest_test1: $(TEST_PREREQUISITES) defaulttest.crm
	$(SILENT)\
	./testscript.sh defaulttest.crm "" -

defaulttest_test2: $(TEST_PREREQUISITES) defaulttest.crm
	$(SILENT)\
	./testscript.sh defaulttest.crm  "" "$(refdir)/defaulttest.crm.2.refoutput" "" "--blah=command override"

windowtest_test: $(TEST_PREREQUISITES) windowtest.crm
	$(SILENT)\
	./testscript.sh windowtest.crm $(srcdir)/windowtest_mt_ng_1.input -

windowtest_fromvar_test: $(TEST_PREREQUISITES) windowtest_fromvar.crm
	$(SILENT)\
	./testscript.sh windowtest_fromvar.crm $(srcdir)/windowtest_fromvar_mt_ng_1.input -

approxtest_test: $(TEST_PREREQUISITES) approxtest.crm
	$(SILENT)\
	./testscript.sh approxtest.crm $(srcdir)/approxtest_mt_ng_1.input -

mathalgtest_test: $(TEST_PREREQUISITES) mathalgtest.crm
	$(SILENT)\
	./testscript.sh mathalgtest.crm  "" -

mathrpntest_test: $(TEST_PREREQUISITES) mathrpntest.crm
	$(SILENT)\
	./testscript.sh mathrpntest.crm  "" - "" -q 1

eval_infiniteloop_test: $(TEST_PREREQUISITES) eval_infiniteloop.crm
	$(SILENT)\
	./testscript.sh eval_infiniteloop.crm "" - "$(builddir)/crm114_error_report.filter.sh"

randomiotest_test: $(TEST_PREREQUISITES) randomiotest.crm
	$(SILENT)\
	./testscript.sh randomiotest.crm "" -

paolo_overvars_test: $(TEST_PREREQUISITES) paolo_overvars.crm
	$(SILENT)\
	./testscript.sh paolo_overvars.crm "" -

paolo_ov2_test: $(TEST_PREREQUISITES) paolo_ov2.crm
	$(SILENT)\
	./testscript.sh paolo_ov2.crm "" -

paolo_ov3_test: $(TEST_PREREQUISITES) paolo_ov3.crm
	$(SILENT)\
	./testscript.sh paolo_ov3.crm "" -

paolo_ov4_test: $(TEST_PREREQUISITES) paolo_ov4.crm
	$(SILENT)\
	./testscript.sh paolo_ov4.crm "" -

paolo_ov5_test: $(TEST_PREREQUISITES) paolo_ov5.crm
	$(SILENT)\
	./testscript.sh paolo_ov5.crm "" -

match_isolate_test: $(TEST_PREREQUISITES) match_isolate_test.crm test_timing_seconds_announcement
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh match_isolate_test.crm "" - "" -e

match_isolate_reclaim_test: $(TEST_PREREQUISITES) match_isolate_reclaim.crm
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh match_isolate_reclaim.crm "" - "" -e

call_return_test: $(TEST_PREREQUISITES) call_return_test.crm
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh call_return_test.crm "" -

translate_tr_test: $(TEST_PREREQUISITES) translate_tr.crm
	$(SILENT)\
	./testscript.sh translate_tr.crm "" -

zz_translate_test: $(TEST_PREREQUISITES) zz_translate_test.crm
	$(SILENT)\
	./testscript.sh zz_translate_test.crm "" -

quine_test: $(TEST_PREREQUISITES) quine.crm
	$(SILENT)\
	./testscript.sh quine.crm "" -


syscall123_test: testscript.sh
	$(SILENT)\
	./testscript.sh '-{window; isolate (:s:); syscall () (:s:) /echo one two three/; output /:*:s:/}' "" "$(refdir)/syscall123_test.refoutput"

# ***** checking return and exit codes

syscall_exit_test: testscript.sh
	$(SILENT)\
	./testscript.sh '-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: \n/}' "" "$(refdir)/syscall_exit_test.refoutput"

# ***** check that failed syscalls will code right
syscall_jibber_test: testscript.sh
	$(SILENT)\
	./testscript.sh '-{window; isolate (:s:); syscall () () (:s:) /jibberjabber 2>&1 /; output / Status: :*:s: \n/}' "" "$(refdir)/syscall_jibber_test.refoutput"

indirecttest_test: testscript.sh
	$(SILENT)\
	./testscript.sh indirecttest.crm "" -

# ****  Default (SBPH Markovian) classifier ************************************************************************************************************************************************************************************************
SBPH_Markovian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Default (SBPH Markovian) classifier "
	$(E) "*******************************************"
	-rm -f randtst.txt
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/SBPH_Markovian_test1.step1.refoutput"

	$(SILENT)\
	./testscript.sh '-{learn (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/SBPH_Markovian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_SBPH_Markovian_1.input "$(refdir)/SBPH_Markovian_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_SBPH_Markovian_2.input "$(refdir)/SBPH_Markovian_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** OSB Markovian classifier ************************************************************************************************************************************************************************************************
OSB_Markovian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markovian classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <osb> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Markovian_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <osb> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Markovian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_OSB_Markovian_1.input "$(refdir)/OSB_Markovian_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_OSB_Markovian_2.input "$(refdir)/OSB_Markovian_test1.step4.refoutput"


# **** OSB Markov Unique classifier ************************************************************************************************************************************************************************************************
OSB_Markov_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markov Unique classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <osb unique > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Markov_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <osb unique > (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Markov_Unique_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_OSB_Markov_Unique_1.input "$(refdir)/OSB_Markov_Unique_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_OSB_Markov_Unique_2.input "$(refdir)/OSB_Markov_Unique_test1.step4.refoutput"

# **** OSB Markov Chisquared Unique classifier ************************************************************************************************************************************************************************************************
OSB_Markov_Chisquared_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markov Chisquared Unique classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_OSB_Markov_Chisquared_Unique_1.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_OSB_Markov_Chisquared_Unique_2.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step4.refoutput"

# **** OSBF Local Confidence (Fidelis) classifier ************************************************************************************************************************************************************************************************
OSBF_Local_Confidence_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSBF Local Confidence (Fidelis) classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < osbf > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSBF_Local_Confidence_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < osbf > (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSBF_Local_Confidence_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_OSBF_Local_Confidence_Fidelis_1.input "$(refdir)/OSBF_Local_Confidence_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_OSBF_Local_Confidence_Fidelis_2.input "$(refdir)/OSBF_Local_Confidence_test1.step4.refoutput"

# **** OSB Winnow classifier ************************************************************************************************************************************************************************************************
OSB_Winnow_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Winnow classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <winnow> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Winnow_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Winnow_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Winnow_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Winnow_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  $(srcdir)/mt_ng_OSB_Winnow_1.input "$(refdir)/OSB_Winnow_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  $(srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/OSB_Winnow_test1.step6.refoutput"
	$(E) ""
	$(E) "Now verify that winnow learns affect only the named file (i_test.css)"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{learn <winnow> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/COLOPHON_mt_ng_reference_3.input "$(refdir)/OSB_Winnow_test1.step7.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  $(srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/OSB_Winnow_test1.step8.refoutput"
	$(E) ""
	$(E) "and now refute-learn into q_test.css"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/FAQ_mt_ng_reference_4.input "$(refdir)/OSB_Winnow_test1.step9.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  $(srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/OSB_Winnow_test1.step10.refoutput"

# **** Unigram Bayesian classifier ************************************************************************************************************************************************************************************************
Unigram_Bayesian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Unigram Bayesian classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <unigram> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Bayesian_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <unigram> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Bayesian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Unigram_Bayesian_1.input "$(refdir)/Unigram_Bayesian_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Unigram_Bayesian_2.input "$(refdir)/Unigram_Bayesian_test1.step4.refoutput"

# **** unigram Winnow classifier ************************************************************************************************************************************************************************************************
Unigram_Winnow_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* unigram Winnow classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Winnow_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Winnow_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Winnow_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Winnow_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  $(srcdir)/mt_ng_unigram_Winnow_1.input "$(refdir)/Unigram_Winnow_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  $(srcdir)/mt_ng_unigram_Winnow_2.input "$(refdir)/Unigram_Winnow_test1.step6.refoutput"

# **** OSB Hyperspace classifier ************************************************************************************************************************************************************************************************
OSB_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_OSB_Hyperspace_1.input "$(refdir)/OSB_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_OSB_Hyperspace_2.input "$(refdir)/OSB_Hyperspace_test1.step4.refoutput"

# **** OSB three-letter Hyperspace classifier ************************************************************************************************************************************************************************************************
OSB_3_letter_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB three-letter Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (q_test.css) /.../}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (i_test.css) /.../}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_OSB_3_letter_Hyperspace_1.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_OSB_3_letter_Hyperspace_2.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step4.refoutput"


# **** Unigram Hyperspace classifier ************************************************************************************************************************************************************************************************
Unigram_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Unigram Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Unigram_Hyperspace_1.input "$(refdir)/Unigram_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Unigram_Hyperspace_2.input "$(refdir)/Unigram_Hyperspace_test1.step4.refoutput"

# **** String Hyperspace classifier ************************************************************************************************************************************************************************************************
String_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace string> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_String_Hyperspace_1.input "$(refdir)/String_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace string> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_String_Hyperspace_2.input "$(refdir)/String_Hyperspace_test1.step4.refoutput"

# **** String Unigram Hyperspace classifier ************************************************************************************************************************************************************************************************
String_Unigram_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Unigram Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string unigram> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Unigram_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string unigram> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Unigram_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace string unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_String_Unigram_Hyperspace_1.input "$(refdir)/String_Unigram_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace string unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_String_Unigram_Hyperspace_2.input "$(refdir)/String_Unigram_Hyperspace_test1.step4.refoutput"

# **** Vector 3-word-bag Hyperspace classifier ************************************************************************************************************************************************************************************************
#    the "vector: blahblah" is coded by the desired length of the pipeline,
#    then the number of iterations of the pipe, then pipelen * iters
#    integer coefficients.  Missing coefficients are taken as zero,
#    extra coefficients are disregarded.
Vector_3_word_bag_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Vector 3-word-bag Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace > (q_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 / }'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace > (i_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ /vector: 3 1 1 1 1 1  /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Vector_3_word_bag_Hyperspace_1.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Vector_3_word_bag_Hyperspace_2.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step4.refoutput"


# **** Bit-Entropy classifier ************************************************************************************************************************************************************************************************
Bit_Entropy_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Bit-Entropy classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Bit_Entropy_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Bit_Entropy_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v entropy=50 -v o_entropy=370 -v jumps=4 -v o_jumps=7 -v prob=2.0 -v pR=3.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Bit_Entropy_1.input "$(refdir)/Bit_Entropy_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v file_features=300 -v entropy=50 -v o_entropy=370 -v jumps=40 -v o_jumps=7 -v prob=5.0 -v pR=5.0"; export CRM114_CHECK_FILTER_ARGS; \
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Bit_Entropy_2.input "$(refdir)/Bit_Entropy_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** Bit-Entropy Toroid classifier ************************************************************************************************************************************************************************************************
Bit_Entropy_Toroid_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Bit-Entropy Toroid classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < entropy > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Bit_Entropy_Toroid_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < entropy > (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Bit_Entropy_Toroid_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v entropy=100 -v jumps=10 -v prob=1.0 -v pR=4.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Bit_Entropy_Toroid_1.input "$(refdir)/Bit_Entropy_Toroid_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v entropy=25 -v jumps=2 -v prob=1.0e-5 -v pR=1.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Bit_Entropy_Toroid_2.input "$(refdir)/Bit_Entropy_Toroid_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** Fast Substring Compression Match Classifier ************************************************************************************************************************************************************************************************
Fast_Substring_Compression_Match_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Fast Substring Compression Match Classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < fscm > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Fast_Substring_Compression_Match_test1.step1.refoutput" "" -s 200000 
	$(SILENT)\
	./testscript.sh '-{learn < fscm > (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Fast_Substring_Compression_Match_test1.step2.refoutput" "" -s 200000 
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Fast_Substring_Compression_Match_1.input "$(refdir)/Fast_Substring_Compression_Match_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Fast_Substring_Compression_Match_2.input "$(refdir)/Fast_Substring_Compression_Match_test1.step4.refoutput"

# **** Neural Network Classifier ************************************************************************************************************************************************************************************************
Neural_Network_test1: $(TEST_PREREQUISITES) test_timing_minutes_announcement
	$(E) "*******************************************"
	$(E) "* Neural Network Classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < neural append > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Neural_Network_test1.step1.refoutput" "" -s 32768 
	$(SILENT)\
	./testscript.sh '-{learn < neural append > (i_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Neural_Network_test1.step2.refoutput" "" -s 32768 
	$(SILENT)\
	./testscript.sh '-{learn < neural refute fromstart > (q_test.css) /[[:graph:]]+/}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Neural_Network_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < neural refute fromstart > (i_test.css) /[[:graph:]]+/}'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Neural_Network_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Neural_Network_1.input "$(refdir)/Neural_Network_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Neural_Network_2.input "$(refdir)/Neural_Network_test1.step6.refoutput"



# **** Alternate Neural Network Classifier test script ************************************************************************************************************************************************************************************************
Neural_Network_test2: $(TEST_PREREQUISITES) test_timing_minutes_announcement
	$(E) "*******************************************"
	$(E) "* Alternate Neural Network Classifier test script "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh alternating_example_neural.crm "" -

# **** Support Vector Machine (SVM) unigram classifier ************************************************************************************************************************************************************************************************
Support_Vector_Machine_Unigram_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Support Vector Machine (SVM) unigram classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (i_test.css) /[[:graph:]]+/; liaf}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (q_test.css) /[[:graph:]]+/; liaf }'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unigram unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }' "" "$(refdir)/Support_Vector_Machine_Unigram_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.1 -v pR=0.2"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Support_Vector_Machine_SVM_unigram_1.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=1e-4 -v pR=0.03 -vo_pR=22.74"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Support_Vector_Machine_SVM_unigram_2.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** Support Vector Machine (SVM) classifier ************************************************************************************************************************************************************************************************
Support_Vector_Machine_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Support Vector Machine (SVM) classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (i_test.css) /[[:graph:]]+/; liaf}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Support_Vector_Machine_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (q_test.css) /[[:graph:]]+/; liaf }'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Support_Vector_Machine_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }' "" "$(refdir)/Support_Vector_Machine_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=4e-3 -v o_prob=0.1275 -v pR=0.04 -v o_pR=8.36"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Support_Vector_Machine_SVM_1.input "$(refdir)/Support_Vector_Machine_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=2e-4 -v pR=0.04 -v o_pR=14.775"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Support_Vector_Machine_SVM_2.input "$(refdir)/Support_Vector_Machine_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** String Kernel SVM (SKS) classifier ************************************************************************************************************************************************************************************************
String_Kernel_SVM_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Kernel SVM (SKS) classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (i_test.css) /[[:graph:]]+/; liaf}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Kernel_SVM_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (q_test.css) /[[:graph:]]+/; liaf }'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Kernel_SVM_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }' "" "$(refdir)/String_Kernel_SVM_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.0004 -v pR=0.03"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_String_Kernel_SVM_SKS_1.input "$(refdir)/String_Kernel_SVM_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=2e-4 -v pR=0.04"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_String_Kernel_SVM_SKS_2.input "$(refdir)/String_Kernel_SVM_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** String Kernel SVM (SKS) Unique classifier ************************************************************************************************************************************************************************************************
String_Kernel_SVM_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Kernel SVM (SKS) Unique classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (i_test.css) /[[:graph:]]+/ / 0 0 100 0.001 1 1 4/; liaf}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Kernel_SVM_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/;  translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (q_test.css) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ ; liaf }'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Kernel_SVM_Unique_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks unique > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }' "" "$(refdir)/String_Kernel_SVM_Unique_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:);  translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  $(srcdir)/mt_ng_String_Kernel_SVM_SKS_Unique_1.input "$(refdir)/String_Kernel_SVM_Unique_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_String_Kernel_SVM_SKS_Unique_2.input "$(refdir)/String_Kernel_SVM_Unique_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** Bytewise Correlation classifier ************************************************************************************************************************************************************************************************
Bytewise_Correlation_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Bytewise Correlation classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:) {classify <correlate> ( $(srcdir)/INTRO_mt_ng_reference_2.input | $(srcdir)/QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Bytewise_Correlation_1.input "$(refdir)/Bytewise_Correlation_test1.step1.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:) {classify <correlate> ( $(srcdir)/INTRO_mt_ng_reference_2.input | $(srcdir)/QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  $(srcdir)/mt_ng_Bytewise_Correlation_2.input "$(refdir)/Bytewise_Correlation_test1.step2.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** Clump \/ Pmulc Test ************************************************************************************************************************************************************************************************
Clump_Pmulc_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Clump \/ Pmulc Test "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump <bychunk> [:one_paragraph:] (i_test.css) /[[:graph:]]+/; output /./ ; liaf}'  $(srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Clump_Pmulc_test1.step1.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump [:one_paragraph:] <bychunk> (i_test.css) /[[:graph:]]+/; output /./; liaf }'  $(srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Clump_Pmulc_test1.step2.refoutput"
	$(E) ""
	$(E) "Now see where our paragraphs go to"
	$(E) ""
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Clump_Pmulc_1.input "$(refdir)/Clump_Pmulc_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Clump_Pmulc_2.input "$(refdir)/Clump_Pmulc_test1.step4.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }'  $(srcdir)/mt_ng_Clump_Pmulc_3.input "$(refdir)/Clump_Pmulc_test1.step5.refoutput"


cleanup_test:
	-rm -f i_test.css
	-rm -f i_vs_q_test.css
	-rm -f q_test.css
	-rm -f randtst.txt
	-rm -rf *.temp.*
	-rm -rf *.temperr
	-rm -rf *.tempchk
	-rm -rf *.diffout
	-rm -rf *.tempout
	-rm -rf *.filtout

#./tests/test_rewrites.mfp
#./tests/e2.out
#./tests/whitelist.mfp.example

clean: cleanup_test







#
# because automake doesn't produce a 'make check' rule that I want...
#
# original code:
# check-am: all-am
#	$(MAKE) $(AM_MAKEFLAGS) $(check_PROGRAMS)
#	$(MAKE) $(AM_MAKEFLAGS) check-TESTS check-local
#
# Le Hack:
#
check-am: all-am
	$(SILENT)\
	srcdir=$(srcdir); export srcdir; \
	builddir=$(builddir); export builddir; \
	$(MAKE) $(AM_MAKEFLAGS) $(check_PROGRAMS); \
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS check-local; \
	$(MAKE) $(AM_MAKEFLAGS) cleanup_test


.PHONY: test test_basics test_other test_megatest test_megatest_ng test_mailfilter test_mailreaver megatest megatest_ng test_classifiers


# ERROR: files left in build directory after distclean:
# ./crm114-20080326-BlameSentansoken-Ger-2244.src.tar.bz2
# ./crm114-20080326-BlameSentansoken-Ger-2244.Win32.bin.tar.bz2
# ./crm114-20080326-BlameSentansoken-Ger-2244.unified.diff.zip
# ./crm114-20080326-BlameSentansoken-Ger-2244.src.tar.gz
# ./crm114-20080326-BlameSentansoken-Ger-2244.unified.diff.tar.gz
# ./crm114-20080326-BlameSentansoken-Ger-2244.normal.diff.tar.gz
# ./crm114-20080326-BlameSentansoken-Ger-2244.Win32.bin.tar.gz
# ./src/diff/crm114.20080501.normal.diff
# ./src/diff/crm114.20080501.unified.diff
# ./src/diff/crm114.20080501.context.diff
# ./crm114-20080326-BlameSentansoken-Ger-2244.normal.diff.zip
# ./crm114-20080326-BlameSentansoken-Ger-2244.context.diff.tar.gz
# ./crm114-20080326-BlameSentansoken-Ger-2244.unified.diff.tar.bz2
# ./crm114-20080326-BlameSentansoken-Ger-2244.src.zip
# ./crm114-20080326-BlameSentansoken-Ger-2244.context.diff.zip
# ./crm114-20080326-BlameSentansoken-Ger-2244.normal.diff.tar.bz2
# ./crm114-20080326-BlameSentansoken-Ger-2244.Win32.bin.zip
# ./crm114-20080326-BlameSentansoken-Ger-2244.src.shar.gz
# ./tests/crm114_versiontest.filter.sh.temp.tstout
# ./tests/testscript.sh.tempout
# ./tests/test_rewrites.mfp
# ./tests/statustest.filter.sh.temp.refout
# ./tests/testscript.sh.temperr
# ./tests/crm114_error_report.filter.sh.temp.tstout
# ./tests/crm114_error_report.filter.sh.temp.refout
# ./tests/testscript.sh.tempchk
# ./tests/crm114_classify.filter.sh.temp.refout
# ./tests/e2.out
# ./tests/i_test.css
# ./tests/crm114_classify.filter.sh.temp.tstout
# ./tests/statustest.filter.sh.temp.tstout
# ./tests/whitelist.mfp.example
# ./tests/crm114_versiontest.filter.sh.temp.refout
# ./tests/testscript.sh.diffout
# ./crm114-20080326-BlameSentansoken-Ger-2244.context.diff.tar.bz2
