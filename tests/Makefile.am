## Process this file with automake to produce Makefile.in

## $Id: Makefile.am,v 1.7 2004/08/18 13:26:30 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal


DISTCLEANFILES =
CLEANFILES = *~ *.bak


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



test_UBERSCRIPTS = megatest.sh \
    	megatest.bat \
    	megatest_ng.sh \
	testscript.sh \
	statustest.filter.sh



EXTRA_DIST = megatest_knowngood.log \
	test_rewrites.mfp \
	$(test_SCRIPTS) $(test_UBERSCRIPTS) $(MEGATESTFILES) \
	megatest.sh.in \
    	megatest_ng.sh.in \
	testscript.sh.in \
	statustest.filter.sh.in




# TESTS = test_basics test_other test_megatest_ng test_mailfilter test_mailreaver
TESTS = 

# check_PROGRAMS = test_basics test_other test_mailfilter test_mailreaver
check_PROGRAMS = 

test: test_basics test_other test_classifiers test_mailfilter test_mailreaver


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



test_timing_announcement:
	$(E) ""
	$(E) "   This runs a moderately interesting set of base tests"
	$(E) "   to exercise much of CRM114 under TRE.  This takes about"
	$(E) "   30 minutes to run on a 3 GHz AMD/XP machine.  Please"
	$(E) "   be patient; you (hopefully) won't see anything till the"
	$(E) "   full set of tests complete.  If you didn't use TRE, all"
	$(E) "   bets are off."
	$(E) ""
	$(E) "   Lines of output that start with OK_IF_mumble are allowed"
	$(E) "   to change values.  No other lines should.  If other lines"
	$(E) "   do change, either your kit isn't quite right or your"
	$(E) "   install is broken (or you've found a bug)."
	$(E) ""


check-hook:
	$(E) "************************************"
	$(E) "* check-hook CALLED"
	$(E) "************************************"

check-local:
	$(E) "************************************"
	$(E) "* check-local CALLED"
	$(E) "************************************"




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
		match_isolate_test_test \
		match_isolate_reclaim_test \
		call_return_test_test \
		translate_tr_test \
		zz_translate_test_test \
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













bracktest_test: testscript.sh bracktest.crm
	./testscript.sh bracktest.crm "" -

escapetest_test: testscript.sh escapetest.crm
	./testscript.sh escapetest.crm "" -

fataltraptest_test: testscript.sh fataltraptest.crm
	./testscript.sh fataltraptest.crm "" -

inserttest_a_test: testscript.sh inserttest_a.crm
	./testscript.sh inserttest_a.crm "" -

matchtest_test: testscript.sh matchtest.crm
	./testscript.sh matchtest.crm     matchtest_mt_ng_1.input -

backwardstest_test1: testscript.sh backwardstest.crm
	./testscript.sh backwardstest.crm backwardstest_mt_ng_1.input -

backwardstest_test2: testscript.sh backwardstest.crm
	./testscript.sh backwardstest.crm backwardstest_mt_ng_2.input "ref/backwardstest.crm.2.refoutput"

overalterisolatedtest_test: testscript.sh overalterisolatedtest.crm
	./testscript.sh overalterisolatedtest.crm "" -

rewritetest_test: testscript.sh rewritetest.crm
	./testscript.sh rewritetest.crm "" -

skudtest_test: testscript.sh skudtest.crm
	./testscript.sh skudtest.crm "" -

statustest_test: testscript.sh statustest.crm statustest.filter.sh
	./testscript.sh statustest.crm "" - "./statustest.filter.sh"

unionintersecttest_test: testscript.sh unionintersecttest.crm
	./testscript.sh unionintersecttest.crm "" -

beeptest_test: testscript.sh beeptest.crm
	./testscript.sh beeptest.crm "" -

defaulttest_test1: testscript.sh defaulttest.crm
	./testscript.sh defaulttest.crm "" -

defaulttest_test2: testscript.sh defaulttest.crm
	./testscript.sh defaulttest.crm  "" "ref/defaulttest.crm.2.refoutput" "" --blah="command override"

windowtest_test: testscript.sh windowtest.crm
	./testscript.sh windowtest.crm windowtest_mt_ng_1.input -

windowtest_fromvar_test: testscript.sh windowtest_fromvar.crm
	./testscript.sh windowtest_fromvar.crm windowtest_fromvar_mt_ng_1.input -

approxtest_test: testscript.sh approxtest.crm
	./testscript.sh approxtest.crm approxtest_mt_ng_1.input -

mathalgtest_test: testscript.sh mathalgtest.crm
	./testscript.sh mathalgtest.crm  "" -

mathrpntest_test: testscript.sh mathrpntest.crm
	./testscript.sh mathrpntest.crm  "" - "" -q 1

eval_infiniteloop_test: testscript.sh eval_infiniteloop.crm
	./testscript.sh eval_infiniteloop.crm "" -

randomiotest_test: testscript.sh randomiotest.crm
	./testscript.sh randomiotest.crm "" -

paolo_overvars_test: testscript.sh paolo_overvars.crm
	./testscript.sh paolo_overvars.crm "" -

paolo_ov2_test: testscript.sh paolo_ov2.crm
	./testscript.sh paolo_ov2.crm "" -

paolo_ov3_test: testscript.sh paolo_ov3.crm
	./testscript.sh paolo_ov3.crm "" -

paolo_ov4_test: testscript.sh paolo_ov4.crm
	./testscript.sh paolo_ov4.crm "" -

paolo_ov5_test: testscript.sh paolo_ov5.crm
	./testscript.sh paolo_ov5.crm "" -

match_isolate_test_test: testscript.sh match_isolate_test.crm
	./testscript.sh match_isolate_test.crm "" - "" -e

match_isolate_reclaim_test: testscript.sh match_isolate_reclaim.crm
	./testscript.sh match_isolate_reclaim.crm "" - "" -e

call_return_test_test: testscript.sh call_return_test.crm
	./testscript.sh call_return_test.crm "" -

translate_tr_test: testscript.sh translate_tr.crm
	./testscript.sh translate_tr.crm "" -

zz_translate_test_test: testscript.sh zz_translate_test.crm
	./testscript.sh zz_translate_test.crm "" -

quine_test: testscript.sh quine.crm
	./testscript.sh quine.crm "" -


syscall123_test: testscript.sh
	./testscript.sh '-{window; isolate (:s:); syscall () (:s:) /echo one two three/; output /:*:s:/}' "" "ref/syscall123_test.refoutput"

# ***** checking return and exit codes

syscall_exit_test: testscript.sh
	./testscript.sh '-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: \n/}' "" "ref/syscall_exit_test.refoutput"

# ***** check that failed syscalls will code right
syscall_jibber_test: testscript.sh
	./testscript.sh '-{window; isolate (:s:); syscall () () (:s:) /jibberjabber 2>&1 /; output / Status: :*:s: \n/}' "" "ref/syscall_jibber_test.refoutput"

indirecttest_test: testscript.sh
	./testscript.sh indirecttest.crm "" -

# ****  Default (SBPH Markovian) classifier ************************************************************************************************************************************************************************************************
SBPH_Markovian_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Default (SBPH Markovian) classifier "
	$(E) "*******************************************"
	-rm -f randtst.txt
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/SBPH_Markovian_test1.step1.refoutput"
	./testscript.sh '-{learn (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/SBPH_Markovian_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_SBPH_Markovian_1.input "ref/SBPH_Markovian_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_SBPH_Markovian_2.input "ref/SBPH_Markovian_test1.step4.refoutput"

# **** OSB Markovian classifier ************************************************************************************************************************************************************************************************
OSB_Markovian_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markovian classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <osb> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_Markovian_test1.step1.refoutput"
	./testscript.sh '-{learn <osb> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSB_Markovian_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_OSB_Markovian_1.input "ref/OSB_Markovian_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_OSB_Markovian_2.input "ref/OSB_Markovian_test1.step4.refoutput"


# **** OSB Markov Unique classifier ************************************************************************************************************************************************************************************************
OSB_Markov_Unique_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markov Unique classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <osb unique > (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_Markov_Unique_test1.step1.refoutput"
	./testscript.sh '-{learn <osb unique > (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSB_Markov_Unique_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_OSB_Markov_Unique_1.input "ref/OSB_Markov_Unique_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_OSB_Markov_Unique_2.input "ref/OSB_Markov_Unique_test1.step4.refoutput"

# **** OSB Markov Chisquared Unique classifier ************************************************************************************************************************************************************************************************
OSB_Markov_Chisquared_Unique_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markov Chisquared Unique classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_Markov_Chisquared_Unique_test1.step1.refoutput"
	./testscript.sh '-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSB_Markov_Chisquared_Unique_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_OSB_Markov_Chisquared_Unique_1.input "ref/OSB_Markov_Chisquared_Unique_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_OSB_Markov_Chisquared_Unique_2.input "ref/OSB_Markov_Chisquared_Unique_test1.step4.refoutput"

# **** OSBF Local Confidence (Fidelis) classifier ************************************************************************************************************************************************************************************************
OSBF_Local_Confidence_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSBF Local Confidence (Fidelis) classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < osbf > (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSBF_Local_Confidence_test1.step1.refoutput"
	./testscript.sh '-{learn < osbf > (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSBF_Local_Confidence_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_OSBF_Local_Confidence_Fidelis_1.input "ref/OSBF_Local_Confidence_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_OSBF_Local_Confidence_Fidelis_2.input "ref/OSBF_Local_Confidence_test1.step4.refoutput"

# **** OSB Winnow classifier ************************************************************************************************************************************************************************************************
OSB_Winnow_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSB Winnow classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <winnow> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_Winnow_test1.step1.refoutput"
	./testscript.sh '-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSB_Winnow_test1.step2.refoutput"
	./testscript.sh '-{learn <winnow> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSB_Winnow_test1.step3.refoutput"
	./testscript.sh '-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_Winnow_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  mt_ng_OSB_Winnow_1.input "ref/OSB_Winnow_test1.step5.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  mt_ng_OSB_Winnow_2.input "ref/OSB_Winnow_test1.step6.refoutput"
	$(E) ""
	$(E) "Now verify that winnow learns affect only the named file (i_test.css)"
	$(E) ""
	./testscript.sh '-{learn <winnow> (i_test.css) /[[:graph:]]+/}'  COLOPHON_mt_ng_reference_3.input "ref/OSB_Winnow_test1.step7.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  mt_ng_OSB_Winnow_2.input "ref/OSB_Winnow_test1.step8.refoutput"
	$(E) ""
	$(E) "and now refute-learn into q_test.css"
	$(E) ""
	./testscript.sh '-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}'  FAQ_mt_ng_reference_4.input "ref/OSB_Winnow_test1.step9.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  mt_ng_OSB_Winnow_2.input "ref/OSB_Winnow_test1.step10.refoutput"

# **** Unigram Bayesian classifier ************************************************************************************************************************************************************************************************
Unigram_Bayesian_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Unigram Bayesian classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <unigram> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Unigram_Bayesian_test1.step1.refoutput"
	./testscript.sh '-{learn <unigram> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Unigram_Bayesian_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Unigram_Bayesian_1.input "ref/Unigram_Bayesian_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Unigram_Bayesian_2.input "ref/Unigram_Bayesian_test1.step4.refoutput"

# **** unigram Winnow classifier ************************************************************************************************************************************************************************************************
Unigram_Winnow_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* unigram Winnow classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Unigram_Winnow_test1.step1.refoutput"
	./testscript.sh '-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Unigram_Winnow_test1.step2.refoutput"
	./testscript.sh '-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Unigram_Winnow_test1.step3.refoutput"
	./testscript.sh '-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Unigram_Winnow_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  mt_ng_unigram_Winnow_1.input "ref/Unigram_Winnow_test1.step5.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      '  mt_ng_unigram_Winnow_2.input "ref/Unigram_Winnow_test1.step6.refoutput"

# **** OSB Hyperspace classifier ************************************************************************************************************************************************************************************************
OSB_Hyperspace_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSB Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <hyperspace unique> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_Hyperspace_test1.step1.refoutput"
	./testscript.sh '-{learn <hyperspace unique> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/OSB_Hyperspace_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_OSB_Hyperspace_1.input "ref/OSB_Hyperspace_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_OSB_Hyperspace_2.input "ref/OSB_Hyperspace_test1.step4.refoutput"

# **** OSB three-letter Hyperspace classifier ************************************************************************************************************************************************************************************************
OSB_3_letter_Hyperspace_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* OSB three-letter Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn <hyperspace unique> (q_test.css) /.../}'  QUICKREF_mt_ng_reference_1.input "ref/OSB_3_letter_Hyperspace_test1.step1.refoutput"
	./testscript.sh '-{learn <hyperspace unique> (i_test.css) /.../}'  INTRO_mt_ng_reference_2.input "ref/OSB_3_letter_Hyperspace_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_OSB_3_letter_Hyperspace_1.input "ref/OSB_3_letter_Hyperspace_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_OSB_3_letter_Hyperspace_2.input "ref/OSB_3_letter_Hyperspace_test1.step4.refoutput"


# **** Unigram Hyperspace classifier ************************************************************************************************************************************************************************************************
Unigram_Hyperspace_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Unigram Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Unigram_Hyperspace_test1.step1.refoutput"
	./testscript.sh '-{learn < hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Unigram_Hyperspace_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Unigram_Hyperspace_1.input "ref/Unigram_Hyperspace_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Unigram_Hyperspace_2.input "ref/Unigram_Hyperspace_test1.step4.refoutput"

# **** String Hyperspace classifier ************************************************************************************************************************************************************************************************
String_Hyperspace_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* String Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < hyperspace string> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/String_Hyperspace_test1.step1.refoutput"
	./testscript.sh '-{learn < hyperspace string> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/String_Hyperspace_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace string> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_String_Hyperspace_1.input "ref/String_Hyperspace_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace string> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_String_Hyperspace_2.input "ref/String_Hyperspace_test1.step4.refoutput"

# **** String Unigram Hyperspace classifier ************************************************************************************************************************************************************************************************
String_Unigram_Hyperspace_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* String Unigram Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < hyperspace string unigram> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/String_Unigram_Hyperspace_test1.step1.refoutput"
	./testscript.sh '-{learn < hyperspace string unigram> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/String_Unigram_Hyperspace_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace string unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_String_Unigram_Hyperspace_1.input "ref/String_Unigram_Hyperspace_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace string unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_String_Unigram_Hyperspace_2.input "ref/String_Unigram_Hyperspace_test1.step4.refoutput"

# **** Vector 3-word-bag Hyperspace classifier ************************************************************************************************************************************************************************************************
#    the "vector: blahblah" is coded by the desired length of the pipeline,
#    then the number of iterations of the pipe, then pipelen * iters
#    integer coefficients.  Missing coefficients are taken as zero,
#    extra coefficients are disregarded.
Vector_3_word_bag_Hyperspace_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Vector 3-word-bag Hyperspace classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < hyperspace > (q_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 / }'  QUICKREF_mt_ng_reference_1.input "ref/Vector_3_word_bag_Hyperspace_test1.step1.refoutput"
	./testscript.sh '-{learn < hyperspace > (i_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1/}'  INTRO_mt_ng_reference_2.input "ref/Vector_3_word_bag_Hyperspace_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ /vector: 3 1 1 1 1 1  /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Vector_3_word_bag_Hyperspace_1.input "ref/Vector_3_word_bag_Hyperspace_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Vector_3_word_bag_Hyperspace_2.input "ref/Vector_3_word_bag_Hyperspace_test1.step4.refoutput"


# **** Bit-Entropy classifier ************************************************************************************************************************************************************************************************
Bit_Entropy_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Bit-Entropy classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Bit_Entropy_test1.step1.refoutput"
	./testscript.sh '-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Bit_Entropy_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Bit_Entropy_1.input "ref/Bit_Entropy_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Bit_Entropy_2.input "ref/Bit_Entropy_test1.step4.refoutput"

# **** Bit-Entropy Toroid classifier ************************************************************************************************************************************************************************************************
Bit_Entropy_Toroid_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Bit-Entropy Toroid classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < entropy > (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Bit_Entropy_Toroid_test1.step1.refoutput"
	./testscript.sh '-{learn < entropy > (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Bit_Entropy_Toroid_test1.step2.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Bit_Entropy_Toroid_1.input "ref/Bit_Entropy_Toroid_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Bit_Entropy_Toroid_2.input "ref/Bit_Entropy_Toroid_test1.step4.refoutput"


# **** Fast Substring Compression Match Classifier ************************************************************************************************************************************************************************************************
Fast_Substring_Compression_Match_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Fast Substring Compression Match Classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < fscm > (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Fast_Substring_Compression_Match_test1.step1.refoutput" "" -s 200000 
	./testscript.sh '-{learn < fscm > (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Fast_Substring_Compression_Match_test1.step2.refoutput" "" -s 200000 
	./testscript.sh '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Fast_Substring_Compression_Match_1.input "ref/Fast_Substring_Compression_Match_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Fast_Substring_Compression_Match_2.input "ref/Fast_Substring_Compression_Match_test1.step4.refoutput"

# **** Neural Network Classifier ************************************************************************************************************************************************************************************************
Neural_Network_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Neural Network Classifier "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{learn < neural append > (q_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Neural_Network_test1.step1.refoutput" "" -s 32768 
	./testscript.sh '-{learn < neural append > (i_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Neural_Network_test1.step2.refoutput" "" -s 32768 
	./testscript.sh '-{learn < neural refute fromstart > (q_test.css) /[[:graph:]]+/}'  INTRO_mt_ng_reference_2.input "ref/Neural_Network_test1.step3.refoutput"
	./testscript.sh '-{learn < neural refute fromstart > (i_test.css) /[[:graph:]]+/}'  QUICKREF_mt_ng_reference_1.input "ref/Neural_Network_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Neural_Network_1.input "ref/Neural_Network_test1.step5.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Neural_Network_2.input "ref/Neural_Network_test1.step6.refoutput"



# **** Alternate Neural Network Classifier test script ************************************************************************************************************************************************************************************************
Neural_Network_test2: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Alternate Neural Network Classifier test script "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh alternating_example_neural.crm "" -

# **** Support Vector Machine (SVM) unigram classifier ************************************************************************************************************************************************************************************************
Support_Vector_Machine_Unigram_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Support Vector Machine (SVM) unigram classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (i_test.css) /[[:graph:]]+/; liaf}'  INTRO_mt_ng_reference_2.input "ref/Support_Vector_Machine_Unigram_test1.step1.refoutput"
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (q_test.css) /[[:graph:]]+/; liaf }'  QUICKREF_mt_ng_reference_1.input "ref/Support_Vector_Machine_Unigram_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unigram unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }' "ref/Support_Vector_Machine_Unigram_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Support_Vector_Machine_SVM_unigram_1.input "ref/Support_Vector_Machine_Unigram_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Support_Vector_Machine_SVM_unigram_2.input "ref/Support_Vector_Machine_Unigram_test1.step5.refoutput"


# **** Support Vector Machine (SVM) classifier ************************************************************************************************************************************************************************************************
Support_Vector_Machine_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Support Vector Machine (SVM) classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (i_test.css) /[[:graph:]]+/; liaf}'  INTRO_mt_ng_reference_2.input "ref/Support_Vector_Machine_test1.step1.refoutput"
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (q_test.css) /[[:graph:]]+/; liaf }'  QUICKREF_mt_ng_reference_1.input "ref/Support_Vector_Machine_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }' "ref/Support_Vector_Machine_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_Support_Vector_Machine_SVM_1.input "ref/Support_Vector_Machine_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Support_Vector_Machine_SVM_2.input "ref/Support_Vector_Machine_test1.step5.refoutput"


# **** String Kernel SVM (SKS) classifier ************************************************************************************************************************************************************************************************
String_Kernel_SVM_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* String Kernel SVM (SKS) classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (i_test.css) /[[:graph:]]+/; liaf}'  INTRO_mt_ng_reference_2.input "ref/String_Kernel_SVM_test1.step1.refoutput"
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (q_test.css) /[[:graph:]]+/; liaf }'  QUICKREF_mt_ng_reference_1.input "ref/String_Kernel_SVM_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }' "ref/String_Kernel_SVM_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_String_Kernel_SVM_SKS_1.input "ref/String_Kernel_SVM_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_String_Kernel_SVM_SKS_2.input "ref/String_Kernel_SVM_test1.step5.refoutput"


# **** String Kernel SVM (SKS) Unique classifier ************************************************************************************************************************************************************************************************
String_Kernel_SVM_Unique_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* String Kernel SVM (SKS) Unique classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (i_test.css) /[[:graph:]]+/ / 0 0 100 0.001 1 1 4/; liaf}'  INTRO_mt_ng_reference_2.input "ref/String_Kernel_SVM_Unique_test1.step1.refoutput"
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/;  translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (q_test.css) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ ; liaf }'  QUICKREF_mt_ng_reference_1.input "ref/String_Kernel_SVM_Unique_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks unique > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }' "ref/String_Kernel_SVM_Unique_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:);  translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }'  mt_ng_String_Kernel_SVM_SKS_Unique_1.input "ref/String_Kernel_SVM_Unique_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_String_Kernel_SVM_SKS_Unique_2.input "ref/String_Kernel_SVM_Unique_test1.step5.refoutput"


# **** Bytewise Correlation classifier ************************************************************************************************************************************************************************************************
Bytewise_Correlation_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Bytewise Correlation classifier "
	$(E) "*******************************************"
	-rm -f i_vs_q_test.css
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{ isolate (:s:) {classify <correlate> ( INTRO_mt_ng_reference_2.input | QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Bytewise_Correlation_1.input "ref/Bytewise_Correlation_test1.step1.refoutput"
	./testscript.sh '-{ isolate (:s:) {classify <correlate> ( INTRO_mt_ng_reference_2.input | QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}'  mt_ng_Bytewise_Correlation_2.input "ref/Bytewise_Correlation_test1.step2.refoutput"

# **** Clump \/ Pmulc Test ************************************************************************************************************************************************************************************************
Clump_Pmulc_test1: testscript.sh test_timing_announcement
	$(E) "*******************************************"
	$(E) "* Clump \/ Pmulc Test "
	$(E) "*******************************************"
	-rm -f i_test.css
	-rm -f q_test.css
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump <bychunk> [:one_paragraph:] (i_test.css) /[[:graph:]]+/; output /./ ; liaf}'  INTRO_mt_ng_reference_2.input "ref/Clump_Pmulc_test1.step1.refoutput"
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump [:one_paragraph:] <bychunk> (i_test.css) /[[:graph:]]+/; output /./; liaf }'  QUICKREF_mt_ng_reference_1.input "ref/Clump_Pmulc_test1.step2.refoutput"
	$(E) ""
	$(E) "Now see where our paragraphs go to"
	$(E) ""
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }'  mt_ng_Clump_Pmulc_1.input "ref/Clump_Pmulc_test1.step3.refoutput"
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }'  mt_ng_Clump_Pmulc_2.input "ref/Clump_Pmulc_test1.step4.refoutput"
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }'  mt_ng_Clump_Pmulc_3.input "ref/Clump_Pmulc_test1.step5.refoutput"


cleanup_test:
	-rm -f i_test.css
	-rm -f q_test.css




.PHONY: test test_basics test_other test_megatest test_megatest_ng test_mailfilter test_mailreaver megatest megatest_ng test_classifiers

