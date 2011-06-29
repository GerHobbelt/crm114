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
	print_binary2decimal_int32.crm



test_UBERSCRIPTS = megatest.sh \
    	megatest.bat \
    	megatest_ng.sh \
	testscript.sh



EXTRA_DIST = megatest_knowngood.log \
	test_rewrites.mfp \
	$(test_SCRIPTS) $(test_UBERSCRIPTS) $(MEGATESTFILES)
 


TESTS = test_basics test_other test_megatest_ng test_mailfilter test_mailreaver



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




test: test_basics test_other test_megatest_ng test_mailfilter test_mailreaver


# series of basic regression tests for CRM114:
test_basics:
	$(E) "   ************************************"
	$(E) "test_basics CALLED"
	$(E) "   ************************************"


# test more complex CRM scripts in ../src
test_other:
	$(E) "   ************************************"
	$(E) "test_other CALLED"
	$(E) "   ************************************"


# test the mailfilter script
test_mailfilter:
	$(E) "   ************************************"
	$(E) "test_mailfilter CALLED"
	$(E) "   ************************************"


# test the mailreaver script
test_mailreaver:
	$(E) "   ************************************"
	$(E) "test_mailreaver CALLED"
	$(E) "   ************************************"



.PHONY: test test_basics test_other test_megatest test_megatest_ng test_mailfilter test_mailreaver megatest megatest_ng

