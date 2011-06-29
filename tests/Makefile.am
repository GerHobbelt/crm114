## Process this file with automake to produce Makefile.in

## $Id: Makefile.am,v 1.7 2004/08/18 13:26:30 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal
## Copyright (C) 2007-2009 Ger Hobbelt <ger@hobbelt.com>


DISTCLEANFILES =
CLEANFILES = *~ *.bak $(refdir)/*~ $(refdir)/*.bak


if OS_HAS_GNU_TIME
TIMER          = @GNU_TIME@ -v
else
TIMER          = @TIME@
endif

CRM114         = @abs_top_builddir@/src/crm114


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
	$(refdir)/alius_w_comment2.crm.refoutput \
	$(refdir)/alius_w_comment.crm.refoutput \
	$(refdir)/alternating_example_neural.crm.refoutput \
	$(refdir)/approxtest.crm.refoutput \
	$(refdir)/argtest.crm.refoutput \
	$(refdir)/argtest_test2.refoutput \
	$(refdir)/argtest_test3.refoutput \
	$(refdir)/argtest_test4.refoutput \
	$(refdir)/argtest_test5.refoutput \
	$(refdir)/argtest_test6.refoutput \
	$(refdir)/argtest_test7.refoutput \
	$(refdir)/argtest_test8.refoutput \
	$(refdir)/backwardstest.crm.2.refoutput \
	$(refdir)/backwardstest.crm.refoutput \
	$(refdir)/beeptest.crm.refoutput \
	$(refdir)/Bit_Entropy_test1.step1.refoutput \
	$(refdir)/Bit_Entropy_test1.step2.refoutput \
	$(refdir)/Bit_Entropy_test1.step3.refoutput \
	$(refdir)/Bit_Entropy_test1.step4.refoutput \
	$(refdir)/Bit_Entropy_Toroid_test1.step1.refoutput \
	$(refdir)/Bit_Entropy_Toroid_test1.step2.refoutput \
	$(refdir)/Bit_Entropy_Toroid_test1.step3.refoutput \
	$(refdir)/Bit_Entropy_Toroid_test1.step4.refoutput \
	$(refdir)/bracktest.crm.refoutput \
	$(refdir)/Bytewise_Correlation_test1.step1.refoutput \
	$(refdir)/Bytewise_Correlation_test1.step2.refoutput \
	$(refdir)/call_return_test2.crm.refoutput \
	$(refdir)/call_return_test3.crm.refoutput \
	$(refdir)/call_return_test4.crm.refoutput \
	$(refdir)/call_return_test.crm.refoutput \
	$(refdir)/Clump_Pmulc_test1.step1.refoutput \
	$(refdir)/Clump_Pmulc_test1.step2.refoutput \
	$(refdir)/Clump_Pmulc_test1.step3.refoutput \
	$(refdir)/Clump_Pmulc_test1.step4.refoutput \
	$(refdir)/Clump_Pmulc_test1.step5.refoutput \
	$(refdir)/crm114_version.refoutput \
	$(refdir)/defaulttest.crm.2.refoutput \
	$(refdir)/defaulttest.crm.refoutput \
	$(refdir)/double_trap_error1.crm.refoutput \
	$(refdir)/double_trap_error2.crm.refoutput \
	$(refdir)/double_trap_error3.crm.refoutput \
	$(refdir)/double_trap_error4.crm.refoutput \
	$(refdir)/double_trap_error5.crm.refoutput \
	$(refdir)/escapetest.crm.refoutput \
	$(refdir)/eval_infiniteloop.crm.refoutput \
	$(refdir)/eval_ref_test1.crm.refoutput \
	$(refdir)/evil_eval_test1.crm.refoutput \
	$(refdir)/exec_async_leaktest1.crm.refoutput \
	$(refdir)/exectest.crm.refoutput \
	$(refdir)/Fast_Substring_Compression_Match_test1.step1.refoutput \
	$(refdir)/Fast_Substring_Compression_Match_test1.step2.refoutput \
	$(refdir)/Fast_Substring_Compression_Match_test1.step3.refoutput \
	$(refdir)/Fast_Substring_Compression_Match_test1.step4.refoutput \
	$(refdir)/fataltrapprogramerr2.crm.refoutput \
	$(refdir)/fataltrapprogramerr.crm.refoutput \
	$(refdir)/fataltraptest.crm.refoutput \
	$(refdir)/illegal_alter_null_var1.crm.refoutput \
	$(refdir)/illegal_alter_var1.crm.refoutput \
	$(refdir)/illegal_CSS_files_w_decent_error_message1.crm.refoutput \
	$(refdir)/illegal_CSS_files_w_decent_error_message2.crm.refoutput \
	$(refdir)/illegal_CSS_files_w_decent_error_message3.crm.refoutput \
	$(refdir)/illegal_empty_alter1.crm.refoutput \
	$(refdir)/illegal_empty_alter2.crm.refoutput \
	$(refdir)/illegal_empty_isolate1.crm.refoutput \
	$(refdir)/illegal_empty_isolate2.crm.refoutput \
	$(refdir)/illegal_eval_var1.crm.refoutput \
	$(refdir)/illegal_eval_var2.crm.refoutput \
	$(refdir)/illegal_indirect_isolate1.crm.refoutput \
	$(refdir)/illegal_isolate_null_var1.crm.refoutput \
	$(refdir)/illegal_isolate_null_var2.crm.refoutput \
	$(refdir)/illegal_isolate_var1.crm.refoutput \
	$(refdir)/illegal_isolate_var2.crm.refoutput \
	$(refdir)/illegal_isolate_var3.crm.refoutput \
	$(refdir)/illegal_stmt1.crm.refoutput \
	$(refdir)/illegal_stmt2.crm.refoutput \
	$(refdir)/indirecttest.crm.refoutput \
	$(refdir)/inserttest_a.crm.refoutput \
	$(refdir)/isolatephenomenontest.crm.refoutput \
	$(refdir)/legal_eval_to_null_var1.crm.refoutput \
	$(refdir)/legal_eval_var1.crm.refoutput \
	$(refdir)/legal_indirect_alter1.crm.refoutput \
	$(refdir)/legal_indirect_isolate1.crm.refoutput \
	$(refdir)/legal_show_null_var1.crm.refoutput \
	$(refdir)/match_attr_fromend_newend.crm.refoutput \
	$(refdir)/match_isolate_reclaim.crm.refoutput \
	$(refdir)/match_isolate_test.crm.refoutput \
	$(refdir)/match_re_fringe1.crm.refoutput \
	$(refdir)/matchtest.crm.refoutput \
	$(refdir)/mathalgtest.crm.refoutput \
	$(refdir)/mathrpntest.crm.refoutput \
	$(refdir)/Neural_Network_test1.step1.refoutput \
	$(refdir)/Neural_Network_test1.step2.refoutput \
	$(refdir)/Neural_Network_test1.step3.refoutput \
	$(refdir)/Neural_Network_test1.step4.refoutput \
	$(refdir)/Neural_Network_test1.step5.refoutput \
	$(refdir)/Neural_Network_test1.step6.refoutput \
	$(refdir)/Neural_Network_test1.step7.refoutput \
	$(refdir)/Neural_Network_test1.step8.refoutput \
	$(refdir)/OSB_3_letter_Hyperspace_test1.step1.refoutput \
	$(refdir)/OSB_3_letter_Hyperspace_test1.step2.refoutput \
	$(refdir)/OSB_3_letter_Hyperspace_test1.step3.refoutput \
	$(refdir)/OSB_3_letter_Hyperspace_test1.step4.refoutput \
	$(refdir)/OSBF_Local_Confidence_test1.step1.refoutput \
	$(refdir)/OSBF_Local_Confidence_test1.step2.refoutput \
	$(refdir)/OSBF_Local_Confidence_test1.step3.refoutput \
	$(refdir)/OSBF_Local_Confidence_test1.step4.refoutput \
	$(refdir)/OSB_Hyperspace_test1.step1.refoutput \
	$(refdir)/OSB_Hyperspace_test1.step2.refoutput \
	$(refdir)/OSB_Hyperspace_test1.step3.refoutput \
	$(refdir)/OSB_Hyperspace_test1.step4.refoutput \
	$(refdir)/OSB_Markov_Chisquared_Unique_test1.step1.refoutput \
	$(refdir)/OSB_Markov_Chisquared_Unique_test1.step2.refoutput \
	$(refdir)/OSB_Markov_Chisquared_Unique_test1.step3.refoutput \
	$(refdir)/OSB_Markov_Chisquared_Unique_test1.step4.refoutput \
	$(refdir)/OSB_Markovian_test1.step1.refoutput \
	$(refdir)/OSB_Markovian_test1.step2.refoutput \
	$(refdir)/OSB_Markovian_test1.step3.refoutput \
	$(refdir)/OSB_Markovian_test1.step4.refoutput \
	$(refdir)/OSB_Markov_Unique_test1.step1.refoutput \
	$(refdir)/OSB_Markov_Unique_test1.step2.refoutput \
	$(refdir)/OSB_Markov_Unique_test1.step3.refoutput \
	$(refdir)/OSB_Markov_Unique_test1.step4.refoutput \
	$(refdir)/OSB_Winnow_test1.step10.refoutput \
	$(refdir)/OSB_Winnow_test1.step1.refoutput \
	$(refdir)/OSB_Winnow_test1.step2.refoutput \
	$(refdir)/OSB_Winnow_test1.step3.refoutput \
	$(refdir)/OSB_Winnow_test1.step4.refoutput \
	$(refdir)/OSB_Winnow_test1.step5.refoutput \
	$(refdir)/OSB_Winnow_test1.step6.refoutput \
	$(refdir)/OSB_Winnow_test1.step7.refoutput \
	$(refdir)/OSB_Winnow_test1.step8.refoutput \
	$(refdir)/OSB_Winnow_test1.step9.refoutput \
	$(refdir)/overalterisolatedtest.crm.refoutput \
	$(refdir)/paolo_ov2.crm.refoutput \
	$(refdir)/paolo_ov3.crm.refoutput \
	$(refdir)/paolo_ov4.crm.refoutput \
	$(refdir)/paolo_ov5.crm.refoutput \
	$(refdir)/paolo_overvars.crm.refoutput \
	$(refdir)/print_binary2decimal_int32.crm.refoutput \
	$(refdir)/quine.crm.refoutput \
	$(refdir)/randomiotest.crm.refoutput \
	$(refdir)/rewritetest.crm.refoutput \
	$(refdir)/SBPH_Markovian_test1.step1.refoutput \
	$(refdir)/SBPH_Markovian_test1.step2.refoutput \
	$(refdir)/SBPH_Markovian_test1.step3.refoutput \
	$(refdir)/SBPH_Markovian_test1.step4.refoutput \
	$(refdir)/skudtest.crm.refoutput \
	$(refdir)/statustest.crm.refoutput \
	$(refdir)/String_Hyperspace_test1.step1.refoutput \
	$(refdir)/String_Hyperspace_test1.step2.refoutput \
	$(refdir)/String_Hyperspace_test1.step3.refoutput \
	$(refdir)/String_Hyperspace_test1.step4.refoutput \
	$(refdir)/String_Kernel_SVM_test1.step1.refoutput \
	$(refdir)/String_Kernel_SVM_test1.step2.refoutput \
	$(refdir)/String_Kernel_SVM_test1.step3.refoutput \
	$(refdir)/String_Kernel_SVM_test1.step4.refoutput \
	$(refdir)/String_Kernel_SVM_test1.step5.refoutput \
	$(refdir)/String_Kernel_SVM_Unique_test1.step1.refoutput \
	$(refdir)/String_Kernel_SVM_Unique_test1.step2.refoutput \
	$(refdir)/String_Kernel_SVM_Unique_test1.step3.refoutput \
	$(refdir)/String_Kernel_SVM_Unique_test1.step4.refoutput \
	$(refdir)/String_Kernel_SVM_Unique_test1.step5.refoutput \
	$(refdir)/String_Unigram_Hyperspace_test1.step1.refoutput \
	$(refdir)/String_Unigram_Hyperspace_test1.step2.refoutput \
	$(refdir)/String_Unigram_Hyperspace_test1.step3.refoutput \
	$(refdir)/String_Unigram_Hyperspace_test1.step4.refoutput \
	$(refdir)/Support_Vector_Machine_test1.step1.refoutput \
	$(refdir)/Support_Vector_Machine_test1.step2.refoutput \
	$(refdir)/Support_Vector_Machine_test1.step3.refoutput \
	$(refdir)/Support_Vector_Machine_test1.step4.refoutput \
	$(refdir)/Support_Vector_Machine_test1.step5.refoutput \
	$(refdir)/Support_Vector_Machine_Unigram_test1.step1.refoutput \
	$(refdir)/Support_Vector_Machine_Unigram_test1.step2.refoutput \
	$(refdir)/Support_Vector_Machine_Unigram_test1.step3.refoutput \
	$(refdir)/Support_Vector_Machine_Unigram_test1.step4.refoutput \
	$(refdir)/Support_Vector_Machine_Unigram_test1.step5.refoutput \
	$(refdir)/syscall123_test.refoutput \
	$(refdir)/syscall_exit_test.refoutput \
	$(refdir)/syscall_jibber_test.refoutput \
	$(refdir)/translate_tr.crm.refoutput \
	$(refdir)/trap_inside_call_issue1.crm.refoutput \
	$(refdir)/Unigram_Bayesian_test1.step1.refoutput \
	$(refdir)/Unigram_Bayesian_test1.step2.refoutput \
	$(refdir)/Unigram_Bayesian_test1.step3.refoutput \
	$(refdir)/Unigram_Bayesian_test1.step4.refoutput \
	$(refdir)/Unigram_Hyperspace_test1.step1.refoutput \
	$(refdir)/Unigram_Hyperspace_test1.step2.refoutput \
	$(refdir)/Unigram_Hyperspace_test1.step3.refoutput \
	$(refdir)/Unigram_Hyperspace_test1.step4.refoutput \
	$(refdir)/Unigram_Winnow_test1.step1.refoutput \
	$(refdir)/Unigram_Winnow_test1.step2.refoutput \
	$(refdir)/Unigram_Winnow_test1.step3.refoutput \
	$(refdir)/Unigram_Winnow_test1.step4.refoutput \
	$(refdir)/Unigram_Winnow_test1.step5.refoutput \
	$(refdir)/Unigram_Winnow_test1.step6.refoutput \
	$(refdir)/unionintersecttest.crm.refoutput \
	$(refdir)/vanilla_eval_bug0001.crm.refoutput \
	$(refdir)/vanilla_isolate_bug0001.crm.refoutput \
	$(refdir)/Vector_3_word_bag_Hyperspace_test1.step1.refoutput \
	$(refdir)/Vector_3_word_bag_Hyperspace_test1.step2.refoutput \
	$(refdir)/Vector_3_word_bag_Hyperspace_test1.step3.refoutput \
	$(refdir)/Vector_3_word_bag_Hyperspace_test1.step4.refoutput \
	$(refdir)/whitespace_test1.crm.refoutput \
	$(refdir)/windowtest.crm.refoutput \
	$(refdir)/windowtest_fromvar.crm.refoutput \
	$(refdir)/zz_translate_test.crm.refoutput \
	$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step1.refoutput \
	$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step2.refoutput \
	$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step3.refoutput \
	$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step4.refoutput \
	$(refdir)/ALT_OSBF_Local_Confidence_test1.step1.refoutput \
	$(refdir)/ALT_OSBF_Local_Confidence_test1.step2.refoutput \
	$(refdir)/ALT_OSBF_Local_Confidence_test1.step3.refoutput \
	$(refdir)/ALT_OSBF_Local_Confidence_test1.step4.refoutput \
	$(refdir)/ALT_OSB_Hyperspace_test1.step1.refoutput \
	$(refdir)/ALT_OSB_Hyperspace_test1.step2.refoutput \
	$(refdir)/ALT_OSB_Hyperspace_test1.step3.refoutput \
	$(refdir)/ALT_OSB_Hyperspace_test1.step4.refoutput \
	$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step1.refoutput \
	$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step2.refoutput \
	$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step3.refoutput \
	$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step4.refoutput \
	$(refdir)/ALT_OSB_Markovian_test1.step1.refoutput \
	$(refdir)/ALT_OSB_Markovian_test1.step2.refoutput \
	$(refdir)/ALT_OSB_Markovian_test1.step3.refoutput \
	$(refdir)/ALT_OSB_Markovian_test1.step4.refoutput \
	$(refdir)/ALT_OSB_Markov_Unique_test1.step1.refoutput \
	$(refdir)/ALT_OSB_Markov_Unique_test1.step2.refoutput \
	$(refdir)/ALT_OSB_Markov_Unique_test1.step3.refoutput \
	$(refdir)/ALT_OSB_Markov_Unique_test1.step4.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step10.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step1.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step2.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step3.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step4.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step5.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step6.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step7.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step8.refoutput \
	$(refdir)/ALT_OSB_Winnow_test1.step9.refoutput \
	$(refdir)/ALT_SBPH_Markovian_test1.step1.refoutput \
	$(refdir)/ALT_SBPH_Markovian_test1.step2.refoutput \
	$(refdir)/ALT_SBPH_Markovian_test1.step3.refoutput \
	$(refdir)/ALT_SBPH_Markovian_test1.step4.refoutput \
	$(refdir)/ALT_String_Hyperspace_test1.step1.refoutput \
	$(refdir)/ALT_String_Hyperspace_test1.step2.refoutput \
	$(refdir)/ALT_String_Hyperspace_test1.step3.refoutput \
	$(refdir)/ALT_String_Hyperspace_test1.step4.refoutput \
	$(refdir)/ALT_String_Unigram_Hyperspace_test1.step1.refoutput \
	$(refdir)/ALT_String_Unigram_Hyperspace_test1.step2.refoutput \
	$(refdir)/ALT_String_Unigram_Hyperspace_test1.step3.refoutput \
	$(refdir)/ALT_String_Unigram_Hyperspace_test1.step4.refoutput \
	$(refdir)/ALT_Unigram_Bayesian_test1.step1.refoutput \
	$(refdir)/ALT_Unigram_Bayesian_test1.step2.refoutput \
	$(refdir)/ALT_Unigram_Bayesian_test1.step3.refoutput \
	$(refdir)/ALT_Unigram_Bayesian_test1.step4.refoutput \
	$(refdir)/ALT_Unigram_Hyperspace_test1.step1.refoutput \
	$(refdir)/ALT_Unigram_Hyperspace_test1.step2.refoutput \
	$(refdir)/ALT_Unigram_Hyperspace_test1.step3.refoutput \
	$(refdir)/ALT_Unigram_Hyperspace_test1.step4.refoutput \
	$(refdir)/ALT_Unigram_Winnow_test1.step1.refoutput \
	$(refdir)/ALT_Unigram_Winnow_test1.step2.refoutput \
	$(refdir)/ALT_Unigram_Winnow_test1.step3.refoutput \
	$(refdir)/ALT_Unigram_Winnow_test1.step4.refoutput \
	$(refdir)/ALT_Unigram_Winnow_test1.step5.refoutput \
	$(refdir)/ALT_Unigram_Winnow_test1.step6.refoutput \
	$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step1.refoutput \
	$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step2.refoutput \
	$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step3.refoutput \
	$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step4.refoutput
	

	
	


test_SCRIPTS = \
	matchtest.crm windowtest.crm overalterisolatedtest.crm exectest.crm \
	exectimingtest1.crm exectimingtest2.crm \
	learntest.crm classifytest.crm \
	escapetest.crm argtest.crm beeptest.crm skudtest.crm aliustest.crm \
	nestaliustest.crm eval_infiniteloop.crm eval_ref_test1.crm \
	traptest.crm fataltraptest.crm uncaughttraptest.crm \
	fataltrapprogramerr.crm whitespace_test1.crm \
	evil_eval_test1.crm fataltrapprogramerr2.crm \
	inserttest_a.crm inserttest_b.crm inserttest_c.crm \
	backwardstest.crm approxtest.crm \
	mathalgtest.crm mathrpntest.crm \
	indirecttest.crm translate_tr.crm zz_translate_test.crm \
	rewritetest.crm exec_async_leaktest1.crm \
	randomiotest.crm isolate_reclaim_test.crm \
	match_isolate_test.crm isolatephenomenontest.crm \
	match_isolate_reclaim.crm match_attr_fromend_newend.crm \
	call_return_test.crm defaulttest.crm alternating_example_neural.crm \
	quine.crm call_return_test2.crm call_return_test3.crm call_return_test4.crm \
	bracktest.crm unionintersecttest.crm \
	statustest.crm windowtest_fromvar.crm paolo_overvars.crm \
	tenfold_validate.crm \
	tenfold_validate_ex.crm \
	tenfold_validate_mailreaver.crm \
	tokendelimiterbugtest.crm bracesbugtest.crm \
	blowuptrapbugtest.crm trapseqbugtest.crm slashbugtest.crm \
	alius_w_comment.crm \
	alius_w_comment2.crm \
	userdirtest.crm \
	paolo_ov2.crm \
	paolo_ov3.crm \
	paolo_ov4.crm \
	paolo_ov5.crm \
	print_binary2decimal_int32.crm \
	match_re_fringe1.crm \
	legal_eval_to_null_var1.crm \
	legal_show_null_var1.crm \
	double_trap_error1.crm \
	double_trap_error2.crm \
	double_trap_error3.crm \
	double_trap_error4.crm \
	double_trap_error5.crm \
	illegal_stmt1.crm \
	illegal_stmt2.crm \
	illegal_alter_null_var1.crm \
	illegal_isolate_null_var1.crm \
	illegal_isolate_null_var2.crm \
	illegal_isolate_var1.crm \
	illegal_isolate_var2.crm \
	illegal_CSS_files_w_decent_error_message2.crm \
	illegal_empty_alter2.crm \
	illegal_alter_var1.crm \
	illegal_CSS_files_w_decent_error_message3.crm \
	illegal_eval_var1.crm \
	illegal_eval_var2.crm \
	illegal_empty_isolate1.crm \
	illegal_CSS_files_w_decent_error_message1.crm \
	illegal_isolate_var3.crm \
	illegal_empty_alter1.crm \
	illegal_empty_isolate2.crm \
	illegal_indirect_isolate1.crm \
	legal_indirect_isolate1.crm \
	legal_indirect_alter1.crm \
	legal_eval_var1.crm \
	vanilla_eval_bug0001.crm \
	vanilla_isolate_bug0001.crm \
	trap_inside_call_issue1.crm



FUNDAMENTAL_TEST_REFFILES = \
	$(srcdir)/scowl/english-upper.10 \
	$(srcdir)/scowl/english-upper.35 \
	$(srcdir)/scowl/english-upper.40 \
	$(srcdir)/scowl/english-upper.50 \
	$(srcdir)/scowl/english-upper.60 \
	$(srcdir)/scowl/english-upper.70 \
	$(srcdir)/scowl/english-upper.80 \
	$(srcdir)/scowl/english-upper.95 \
	$(srcdir)/scowl/english-words.10 \
	$(srcdir)/scowl/english-words.20 \
	$(srcdir)/scowl/english-words.35 \
	$(srcdir)/scowl/english-words.40 \
	$(srcdir)/scowl/english-words.50 \
	$(srcdir)/scowl/english-words.55 \
	$(srcdir)/scowl/english-words.60 \
	$(srcdir)/scowl/english-words.70 \
	$(srcdir)/scowl/english-words.80



FUNDAMENTAL_TEST_PREREQS = \
	ref_dir \
	crm114_binaries \
	testscript.sh \
	mk_testcases.sh \
	tenfold_validate_ex.crm \
	tenfold_validate_mailreaver.crm \
	$(srcdir)/mk_uniq_files.awk \
	$(FUNDAMENTAL_TEST_REFFILES) \
	scowl_dir \
	scowl/english-upper-all \
	scowl/english-words-all



EXTRA_DIST = megatest_knowngood.log \
	test_rewrites.mfp \
	$(test_SCRIPTS) \
	$(MEGATEST_INPUT_FILES) \
	$(TEST_REFERENCE_FILES) \
	$(FUNDAMENTAL_TEST_REFFILES) \
	megatest.bat \
	megatest.sh.in \
	megatest_ng.sh.in \
	testscript.sh.in \
	crm114_statustest.filter.sh.in \
	crm114_classify.filter.sh.in \
	crm114_classify.filter.awk \
	whitelist.mfp.example \
	crm114_isolate_test.filter.sh.in \
	exectest_s1.sh.in \
	mk_testcases.sh.in \
	mk_uniq_files.awk



# WARNING: the 'ref' files should NOT be a dependency in this set, as that would prevent us
#          from being able to auto-generate the reference files when those are absent.
TEST_PREREQUISITES = 	\
	$(MEGATEST_INPUT_FILES) \
	ref_dir \
	crm114_binaries \
	testscript.sh \
	crm114_statustest.filter.sh \
	crm114_classify.filter.sh \
	crm114_classify.filter.awk \
	crm114_isolate_test.filter.sh \
	exectest_s1.sh




# TESTS = test_basics test_other test_classifiers test_fundamental_mail
TESTS =

check_PROGRAMS =

XFAIL_TESTS =
#	exectest_test




test: test_basics test_other test_classifiers test_fundamental_mail


crm114_binaries: 
	(cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) all);
	
mailreaver_scripts:
	(cd $(top_builddir)/mailfilter && $(MAKE) $(AM_MAKEFLAGS) all);
	
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
	$(DIFF) -u $(DIFF_FLAGS) megatest_knowngood.log megatest_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(DIFF) $(DIFF_FLAGS) megatest_knowngood.log megatest_test.log & sleep 1


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
	$(DIFF) -u $(DIFF_FLAGS) megatest_knowngood.log megatest_ng_test.log & sleep 1
	$(E) "=============================================================="
	$(E) "=================== MEGATEST DIFF REPORT ====================="
	$(E) "===================      (regular)       ===================="
	$(E) "=============================================================="
	$(DIFF) $(DIFF_FLAGS) megatest_knowngood.log megatest_ng_test.log & sleep 1






# series of basic regression tests for CRM114:
test_basics:                    \
		initial_tests1	\
		fringe_cases_compiler_tests1 \
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


# test the mailreaver/mailtrainer script set with a fundamental test set
test_fundamental_mail:          \
		mk_test_sets	\
		mailreaver_scripts \
		fundamental_tenfold_tests \
		test_timing_minutes_announcement
	$(E) "************************************"
	$(E) "* test_fundamental_mail CALLED"
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
	$(E) "   *** 3 MINUTES *** to run on a 3 GHz AMD/XP machine.  Please"
	$(E) "   be patient; you (hopefully) won't see anything till the"
	$(E) "   full set of tests complete.  If you didn't use TRE, all"
	$(E) "   bets are off."
	$(E) ""
	$(E) "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

#	-sleep 10


# check everything except neural: too slow :-(
check-local:                    \
		test_basics \
		test_other \
		test_classifiers
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
		fringe_cases_compiler_tests1 \
		initial_tests2  \
		further_tests  \
		all_classifier_tests
	$(E) "************************************"
	$(E) "* test group: script (all)"
	$(E) "************************************"


initial_tests1:              \
		version_test \
		bracktest_test \
		alius_w_comment2_test \
		alius_w_comment1_test \
		escapetest_test \
		argtest_test1 \
		argtest_test2 \
		argtest_test3 \
		argtest_test4 \
		argtest_test5 \
		argtest_test6 \
		argtest_test7 \
		argtest_test8 \
		whitespace_test1 \
		evil_eval_test1 \
		fataltraptest_test \
		fataltrapprogramerr_test \
		fataltrapprogramerr2_test \
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
		eval_ref_test1 \
		randomiotest_test \
		paolo_overvars_test \
		paolo_ov2_test \
		paolo_ov3_test \
		paolo_ov4_test \
		paolo_ov5_test \
		print_binary2decimal_int32_test \
		match_re_fringe1_test

fringe_cases_compiler_tests1:	\
		legal_eval_to_null_var1_test \
		legal_show_null_var1_test \
		double_trap_error1_test \
		double_trap_error2_test \
		double_trap_error3_test \
		double_trap_error4_test \
		double_trap_error5_test \
		illegal_stmt1_test \
		illegal_stmt2_test \
		illegal_alter_null_var1_test \
		illegal_isolate_null_var1_test \
		illegal_isolate_null_var2_test \
		illegal_isolate_var1_test \
		illegal_isolate_var2_test \
		illegal_CSS_files_w_decent_error_message2_test \
		illegal_empty_alter2_test \
		illegal_alter_var1_test \
		illegal_CSS_files_w_decent_error_message3_test \
		illegal_eval_var1_test \
		illegal_eval_var2_test \
		illegal_empty_isolate1_test \
		illegal_CSS_files_w_decent_error_message1_test \
		illegal_isolate_var3_test \
		illegal_empty_alter1_test \
		illegal_empty_isolate2_test \
		illegal_indirect_isolate1_test \
		legal_indirect_isolate1_test \
		legal_indirect_alter1_test \
		legal_eval_var1_test \
		vanilla_eval_bug0001_test \
		vanilla_isolate_bug0001_test \
		trap_inside_call_issue1_test



# slower tests:
initial_tests2:              \
		indirecttest_test \
		isolatephenomenon_test \
		match_isolate_reclaim_test \
		match_attr_fromend_newend_test \
		call_return_test1 \
		call_return_test2 \
		call_return_test3 \
		call_return_test4 \
		translate_tr_test \
		zz_translate_test \
		quine_test

further_tests:                            \
		match_isolate_test \
		exectest_test \
		syscall123_test \
		syscall_exit_test \
		syscall_jibber_test \
		exec_async_leaktest1

all_classifier_tests:                        \
		markovian_classifier_tests \
		OSBF_classifier_tests \
		OSB_classifier_tests \
		WINNOW_classifier_tests \
		Hyperspace_classifier_tests \
		ALT_markovian_classifier_tests \
		ALT_OSBF_classifier_tests \
		ALT_OSB_classifier_tests \
		ALT_WINNOW_classifier_tests \
		ALT_Hyperspace_classifier_tests \
		Bit_Entropy_classifier_tests \
		FSCM_classifier_tests \
		Neural_Net_classifier_tests \
		SVM_SKS_classifier_tests \
		Correlator_classifier_tests \
		CLUMP_classifier_tests

#
# extra tests to check the performance of CRM114
#
timing_tests:					\
		exectimingtest1_test \
		exectimingtest2_test



# classifier tests grouped:

markovian_classifier_tests:                        \
		Unigram_Bayesian_test1 \
		SBPH_Markovian_test1

OSBF_classifier_tests:                        \
		OSBF_Local_Confidence_test1

OSB_classifier_tests:                        \
		OSB_Markovian_test1 \
		OSB_Markov_Unique_test1 \
		OSB_Markov_Chisquared_Unique_test1

WINNOW_classifier_tests:                        \
		OSB_Winnow_test1 \
		Unigram_Winnow_test1

Hyperspace_classifier_tests:                        \
		OSB_Hyperspace_test1 \
		OSB_3_letter_Hyperspace_test1 \
		Unigram_Hyperspace_test1 \
		String_Hyperspace_test1 \
		String_Unigram_Hyperspace_test1 \
		Vector_3_word_bag_Hyperspace_test1

ALT_markovian_classifier_tests:                        \
		ALT_Unigram_Bayesian_test1 \
		ALT_SBPH_Markovian_test1

ALT_OSBF_classifier_tests:                        \
		ALT_OSBF_Local_Confidence_test1

ALT_OSB_classifier_tests:                        \
		ALT_OSB_Markovian_test1 \
		ALT_OSB_Markov_Unique_test1 \
		ALT_OSB_Markov_Chisquared_Unique_test1

ALT_WINNOW_classifier_tests:                        \
		ALT_OSB_Winnow_test1 \
		ALT_Unigram_Winnow_test1

ALT_Hyperspace_classifier_tests:                        \
		ALT_OSB_Hyperspace_test1 \
		ALT_OSB_3_letter_Hyperspace_test1 \
		ALT_Unigram_Hyperspace_test1 \
		ALT_String_Hyperspace_test1 \
		ALT_String_Unigram_Hyperspace_test1 \
		ALT_Vector_3_word_bag_Hyperspace_test1

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
	./testscript.sh '-{}' "" "$(refdir)/crm114_version.refoutput" "" -v

alius_w_comment1_test: $(TEST_PREREQUISITES) alius_w_comment.crm
	$(SILENT)\
	./testscript.sh alius_w_comment.crm "" -

alius_w_comment2_test: $(TEST_PREREQUISITES) alius_w_comment2.crm
	$(SILENT)\
	./testscript.sh alius_w_comment2.crm "" -

bracktest_test: $(TEST_PREREQUISITES) bracktest.crm
	$(SILENT)\
	./testscript.sh bracktest.crm "" -

escapetest_test: $(TEST_PREREQUISITES) escapetest.crm
	$(SILENT)\
	./testscript.sh escapetest.crm "" -

argtest_test1: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" -

argtest_test2: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test2.refoutput" "" --_arg8=hooba --

argtest_test3: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test3.refoutput" "" -- --_arg8=hooba

argtest_test4: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test4.refoutput" "" --_arg8=hooba -- --more_bogus -t -T

argtest_test5: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test5.refoutput" "" --_arg8=hooba -- more_bogus -t -T complete_loony

argtest_test6: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test6.refoutput" "" '-( agent=smith target=neo target=trinity )' --

argtest_test7: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test7.refoutput" "" -- '-( agent=smith target=neo target=trinity )'

argtest_test8: $(TEST_PREREQUISITES) argtest.crm
	$(SILENT)\
	./testscript.sh argtest.crm "" "$(refdir)/argtest_test8.refoutput" "" -u .

whitespace_test1: $(TEST_PREREQUISITES) whitespace_test1.crm
	$(SILENT)\
	./testscript.sh whitespace_test1.crm "" - "" ABC

evil_eval_test1: $(TEST_PREREQUISITES) evil_eval_test1.crm
	$(SILENT)\
	./testscript.sh evil_eval_test1.crm "" -

fataltraptest_test: $(TEST_PREREQUISITES) fataltraptest.crm
	$(SILENT)\
	./testscript.sh fataltraptest.crm "" -

fataltrapprogramerr_test: $(TEST_PREREQUISITES) fataltrapprogramerr.crm
	$(SILENT)\
	./testscript.sh fataltrapprogramerr.crm "" -

fataltrapprogramerr2_test: $(TEST_PREREQUISITES) fataltrapprogramerr2.crm
	$(SILENT)\
	./testscript.sh fataltrapprogramerr2.crm "" -

inserttest_a_test: $(TEST_PREREQUISITES) inserttest_a.crm
	$(SILENT)\
	./testscript.sh inserttest_a.crm "" -

matchtest_test: $(TEST_PREREQUISITES) matchtest.crm
	$(SILENT)\
	./testscript.sh matchtest.crm $(abs_srcdir)/matchtest_mt_ng_1.input -

backwardstest_test1: $(TEST_PREREQUISITES) backwardstest.crm
	$(SILENT)\
	./testscript.sh backwardstest.crm $(abs_srcdir)/backwardstest_mt_ng_1.input -

backwardstest_test2: $(TEST_PREREQUISITES) backwardstest.crm
	$(SILENT)\
	./testscript.sh backwardstest.crm $(abs_srcdir)/backwardstest_mt_ng_2.input "$(refdir)/backwardstest.crm.2.refoutput"

overalterisolatedtest_test: $(TEST_PREREQUISITES) overalterisolatedtest.crm
	$(SILENT)\
	./testscript.sh overalterisolatedtest.crm "" -




# $(builddir)/test_rewrites.mfp: $(srcdir)/test_rewrites.mfp:
# ^^^^ that's a circular dependency when builddir==srcdir - which is the common situation
#$(builddir)/test_rewrites.mfp:
#	$(SILENT)\
#	if [ ! -f "$(builddir)/test_rewrites.mfp" ]; then               \
#	  cp $(srcdir)/test_rewrites.mfp $(builddir)/test_rewrites.mfp; \
#	fi

rewritetest_test: $(TEST_PREREQUISITES) rewritetest.crm
	$(SILENT)\
	./testscript.sh rewritetest.crm "" -




# $(builddir)/whitelist.mfp.example: $(srcdir)/whitelist.mfp.example
# ^^^^ that's a circular dependency when builddir==srcdir - which is the common situation
#$(builddir)/whitelist.mfp.example: 
#	$(SILENT)\
#	if [ ! -f "$(builddir)/whitelist.mfp.example" ]; then                   \
#	  cp $(srcdir)/whitelist.mfp.example $(builddir)/whitelist.mfp.example; \
#	fi

skudtest_test: $(TEST_PREREQUISITES) skudtest.crm $(builddir)/whitelist.mfp.example
	$(SILENT)\
	./testscript.sh skudtest.crm "" -




statustest_test: $(TEST_PREREQUISITES) statustest.crm exectest_s1.sh
	$(SILENT)\
	./testscript.sh statustest.crm "" - "$(builddir)/crm114_statustest.filter.sh"

exectest_test: $(TEST_PREREQUISITES) exectest.crm exectest_s1.sh test_timing_seconds_announcement
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh exectest.crm "" - "$(builddir)/crm114_statustest.filter.sh"

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
	./testscript.sh windowtest.crm $(abs_srcdir)/windowtest_mt_ng_1.input -

windowtest_fromvar_test: $(TEST_PREREQUISITES) windowtest_fromvar.crm
	$(SILENT)\
	./testscript.sh windowtest_fromvar.crm $(abs_srcdir)/windowtest_fromvar_mt_ng_1.input -

approxtest_test: $(TEST_PREREQUISITES) approxtest.crm
	$(SILENT)\
	./testscript.sh approxtest.crm $(abs_srcdir)/approxtest_mt_ng_1.input -

mathalgtest_test: $(TEST_PREREQUISITES) mathalgtest.crm
	$(SILENT)\
	./testscript.sh mathalgtest.crm  "" -

mathrpntest_test: $(TEST_PREREQUISITES) mathrpntest.crm
	$(SILENT)\
	./testscript.sh mathrpntest.crm  "" - "" -q 1

eval_infiniteloop_test: $(TEST_PREREQUISITES) eval_infiniteloop.crm
	$(SILENT)\
	./testscript.sh eval_infiniteloop.crm "" -

eval_ref_test1: $(TEST_PREREQUISITES) eval_ref_test1.crm
	$(SILENT)\
	./testscript.sh eval_ref_test1.crm "" -

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

print_binary2decimal_int32_test: $(TEST_PREREQUISITES) print_binary2decimal_int32.crm
	$(SILENT)\
	./testscript.sh print_binary2decimal_int32.crm "01234567@ABCDEFG" -

match_re_fringe1_test: $(TEST_PREREQUISITES) match_re_fringe1.crm
	$(SILENT)\
	./testscript.sh match_re_fringe1.crm "" -

match_isolate_test: $(TEST_PREREQUISITES) match_isolate_test.crm test_timing_seconds_announcement
	$(SILENT)\
	./testscript.sh match_isolate_test.crm "" - "$(builddir)/crm114_isolate_test.filter.sh" -e

match_attr_fromend_newend_test: $(TEST_PREREQUISITES) match_attr_fromend_newend.crm test_timing_seconds_announcement
	$(SILENT)\
	./testscript.sh match_attr_fromend_newend.crm "" -

isolatephenomenon_test: $(TEST_PREREQUISITES) isolatephenomenontest.crm test_timing_seconds_announcement
	$(SILENT)\
	./testscript.sh isolatephenomenontest.crm "" - "$(builddir)/crm114_isolate_test.filter.sh" -e

match_isolate_reclaim_test: $(TEST_PREREQUISITES) match_isolate_reclaim.crm
	$(SILENT)\
	./testscript.sh match_isolate_reclaim.crm "" - "$(builddir)/crm114_isolate_test.filter.sh" -e

call_return_test1: $(TEST_PREREQUISITES) call_return_test.crm
	$(SILENT)\
	./testscript.sh call_return_test.crm "" -

call_return_test2: $(TEST_PREREQUISITES) call_return_test2.crm
	$(SILENT)\
	./testscript.sh call_return_test2.crm "" -

call_return_test3: $(TEST_PREREQUISITES) call_return_test3.crm
	$(SILENT)\
	./testscript.sh call_return_test3.crm "" -

call_return_test4: $(TEST_PREREQUISITES) call_return_test4.crm
	$(SILENT)\
	./testscript.sh call_return_test4.crm "" -

translate_tr_test: $(TEST_PREREQUISITES) translate_tr.crm
	$(SILENT)\
	./testscript.sh translate_tr.crm "" -

zz_translate_test: $(TEST_PREREQUISITES) zz_translate_test.crm
	$(SILENT)\
	./testscript.sh zz_translate_test.crm "" -

quine_test: $(TEST_PREREQUISITES) quine.crm
	$(SILENT)\
	./testscript.sh quine.crm "" -


syscall123_test: $(TEST_PREREQUISITES)
	$(SILENT)\
	./testscript.sh '-{window; isolate (:s:); syscall () (:s:) /echo one two three/; output /:*:s:/}' "" "$(refdir)/syscall123_test.refoutput"


# ***** checking return and exit codes

syscall_exit_test: $(TEST_PREREQUISITES)
	$(SILENT)\
	./testscript.sh '-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: :*:_nl:/}' "" "$(refdir)/syscall_exit_test.refoutput"

# ***** check that failed syscalls will code right
syscall_jibber_test: $(TEST_PREREQUISITES)
	$(SILENT)\
	./testscript.sh '-{window; isolate (:s:); syscall () () (:s:) /jibberjabber 2>&1 /; output / Status: :*:s: :*:_nl:/}' "" "$(refdir)/syscall_jibber_test.refoutput"

indirecttest_test: $(TEST_PREREQUISITES) indirecttest.crm
	$(SILENT)\
	./testscript.sh indirecttest.crm "" -

# test for no more pipe handle leaks as reported by Trevel L. Adams, july 2008
exec_async_leaktest1: $(TEST_PREREQUISITES) exec_async_leaktest1.crm
	$(SILENT)\
	./testscript.sh exec_async_leaktest1.crm "" -





# test illegal language constructs: these SHOULD produce error reports!
# ... Well, almost all of them; SOME are legal constructs, though a bit 'on the edge':
	
legal_eval_to_null_var1_test: $(TEST_PREREQUISITES) legal_eval_to_null_var1.crm
	$(SILENT)\
	./testscript.sh legal_eval_to_null_var1.crm "" -

legal_show_null_var1_test: $(TEST_PREREQUISITES) legal_show_null_var1.crm
	$(SILENT)\
	./testscript.sh legal_show_null_var1.crm "" -

double_trap_error1_test: $(TEST_PREREQUISITES) double_trap_error1.crm
	$(SILENT)\
	./testscript.sh double_trap_error1.crm "" -

double_trap_error2_test: $(TEST_PREREQUISITES) double_trap_error2.crm
	$(SILENT)\
	./testscript.sh double_trap_error2.crm "" -

double_trap_error3_test: $(TEST_PREREQUISITES) double_trap_error3.crm
	$(SILENT)\
	./testscript.sh double_trap_error3.crm "" -

double_trap_error4_test: $(TEST_PREREQUISITES) double_trap_error4.crm
	$(SILENT)\
	./testscript.sh double_trap_error4.crm "" -

double_trap_error5_test: $(TEST_PREREQUISITES) double_trap_error5.crm
	$(SILENT)\
	./testscript.sh double_trap_error5.crm "" -

illegal_stmt1_test: $(TEST_PREREQUISITES) illegal_stmt1.crm
	$(SILENT)\
	./testscript.sh illegal_stmt1.crm "" -

illegal_stmt2_test: $(TEST_PREREQUISITES) illegal_stmt2.crm
	$(SILENT)\
	./testscript.sh illegal_stmt2.crm "" -

illegal_alter_null_var1_test: $(TEST_PREREQUISITES) illegal_alter_null_var1.crm
	$(SILENT)\
	./testscript.sh illegal_alter_null_var1.crm "" -

illegal_isolate_null_var1_test: $(TEST_PREREQUISITES) illegal_isolate_null_var1.crm
	$(SILENT)\
	./testscript.sh illegal_isolate_null_var1.crm "" -

illegal_isolate_null_var2_test: $(TEST_PREREQUISITES) illegal_isolate_null_var2.crm
	$(SILENT)\
	./testscript.sh illegal_isolate_null_var2.crm "" -

illegal_isolate_var1_test: $(TEST_PREREQUISITES) illegal_isolate_var1.crm
	$(SILENT)\
	./testscript.sh illegal_isolate_var1.crm "" -

illegal_isolate_var2_test: $(TEST_PREREQUISITES) illegal_isolate_var2.crm
	$(SILENT)\
	./testscript.sh illegal_isolate_var2.crm "" -

illegal_CSS_files_w_decent_error_message2_test: $(TEST_PREREQUISITES) illegal_CSS_files_w_decent_error_message2.crm
	$(SILENT)\
	./testscript.sh illegal_CSS_files_w_decent_error_message2.crm "" -

illegal_empty_alter2_test: $(TEST_PREREQUISITES) illegal_empty_alter2.crm
	$(SILENT)\
	./testscript.sh illegal_empty_alter2.crm "" -

illegal_alter_var1_test: $(TEST_PREREQUISITES) illegal_alter_var1.crm
	$(SILENT)\
	./testscript.sh illegal_alter_var1.crm "" -

illegal_CSS_files_w_decent_error_message3_test: $(TEST_PREREQUISITES) illegal_CSS_files_w_decent_error_message3.crm
	$(SILENT)\
	./testscript.sh illegal_CSS_files_w_decent_error_message3.crm "" -

illegal_eval_var1_test: $(TEST_PREREQUISITES) illegal_eval_var1.crm
	$(SILENT)\
	./testscript.sh illegal_eval_var1.crm "" -

illegal_eval_var2_test: $(TEST_PREREQUISITES) illegal_eval_var2.crm
	$(SILENT)\
	./testscript.sh illegal_eval_var2.crm "" - "" -l 1

illegal_empty_isolate1_test: $(TEST_PREREQUISITES) illegal_empty_isolate1.crm
	$(SILENT)\
	./testscript.sh illegal_empty_isolate1.crm "" -

illegal_CSS_files_w_decent_error_message1_test: $(TEST_PREREQUISITES) illegal_CSS_files_w_decent_error_message1.crm
	$(SILENT)\
	./testscript.sh illegal_CSS_files_w_decent_error_message1.crm "" -

illegal_isolate_var3_test: $(TEST_PREREQUISITES) illegal_isolate_var3.crm
	$(SILENT)\
	./testscript.sh illegal_isolate_var3.crm "" -

illegal_empty_alter1_test: $(TEST_PREREQUISITES) illegal_empty_alter1.crm
	$(SILENT)\
	./testscript.sh illegal_empty_alter1.crm "" -

illegal_empty_isolate2_test: $(TEST_PREREQUISITES) illegal_empty_isolate2.crm
	$(SILENT)\
	./testscript.sh illegal_empty_isolate2.crm "" -

illegal_indirect_isolate1_test: $(TEST_PREREQUISITES) illegal_indirect_isolate1.crm
	$(SILENT)\
	./testscript.sh illegal_indirect_isolate1.crm "" -

legal_indirect_isolate1_test: $(TEST_PREREQUISITES) legal_indirect_isolate1.crm
	$(SILENT)\
	./testscript.sh legal_indirect_isolate1.crm "" -

legal_indirect_alter1_test: $(TEST_PREREQUISITES) legal_indirect_alter1.crm
	$(SILENT)\
	./testscript.sh legal_indirect_alter1.crm "" -

legal_eval_var1_test: $(TEST_PREREQUISITES) legal_eval_var1.crm
	$(SILENT)\
	./testscript.sh legal_eval_var1.crm "" - "" -l 5

vanilla_eval_bug0001_test: $(TEST_PREREQUISITES) vanilla_eval_bug0001.crm
	$(SILENT)\
	./testscript.sh vanilla_eval_bug0001.crm "" -

vanilla_isolate_bug0001_test: $(TEST_PREREQUISITES) vanilla_isolate_bug0001.crm
	$(SILENT)\
	./testscript.sh vanilla_isolate_bug0001.crm "" -

trap_inside_call_issue1_test: $(TEST_PREREQUISITES) trap_inside_call_issue1.crm
	$(SILENT)\
	./testscript.sh trap_inside_call_issue1.crm "" -



exectimingtest1_test: $(TEST_PREREQUISITES) exectimingtest1.crm test_timing_minutes_announcement
	$(SILENT)\
	$(TIMER) $(CRM114) -p exectimingtest1.crm

exectimingtest2_test: $(TEST_PREREQUISITES) exectimingtest2.crm test_timing_minutes_announcement
	$(SILENT)\
	$(TIMER) $(CRM114) -u ../mailfilter -p ../tests/exectimingtest2.crm







# ****  Default (SBPH Markovian) classifier **********************************************************
SBPH_Markovian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Default (SBPH Markovian) classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f randtst.txt
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/SBPH_Markovian_test1.step1.refoutput"

	$(SILENT)\
	./testscript.sh '-{learn (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/SBPH_Markovian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_SBPH_Markovian_1.input "$(refdir)/SBPH_Markovian_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_SBPH_Markovian_2.input "$(refdir)/SBPH_Markovian_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** OSB Markovian classifier **********************************************************
OSB_Markovian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markovian classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <osb> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Markovian_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <osb> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Markovian_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Markovian_1.input "$(refdir)/OSB_Markovian_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Markovian_2.input "$(refdir)/OSB_Markovian_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** OSB Markov Unique classifier **********************************************************
OSB_Markov_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markov Unique classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <osb unique > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Markov_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <osb unique > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Markov_Unique_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Markov_Unique_1.input "$(refdir)/OSB_Markov_Unique_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Markov_Unique_2.input "$(refdir)/OSB_Markov_Unique_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** OSB Markov Chisquared Unique classifier **********************************************************
OSB_Markov_Chisquared_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Markov Chisquared Unique classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Markov_Chisquared_Unique_1.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Markov_Chisquared_Unique_2.input "$(refdir)/OSB_Markov_Chisquared_Unique_test1.step4.refoutput"

# **** OSBF Local Confidence (Fidelis) classifier **********************************************************
OSBF_Local_Confidence_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSBF Local Confidence (Fidelis) classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < osbf > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSBF_Local_Confidence_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < osbf > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSBF_Local_Confidence_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSBF_Local_Confidence_Fidelis_1.input "$(refdir)/OSBF_Local_Confidence_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSBF_Local_Confidence_Fidelis_2.input "$(refdir)/OSBF_Local_Confidence_test1.step4.refoutput"

# **** OSB Winnow classifier **********************************************************
OSB_Winnow_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Winnow classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <winnow> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Winnow_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Winnow_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Winnow_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Winnow_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }       '  $(abs_srcdir)/mt_ng_OSB_Winnow_1.input "$(refdir)/OSB_Winnow_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/OSB_Winnow_test1.step6.refoutput"
	$(E) ""
	$(E) "Now verify that winnow learns affect only the named file (i_test.css)"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{learn <winnow> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/COLOPHON_mt_ng_reference_3.input "$(refdir)/OSB_Winnow_test1.step7.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/OSB_Winnow_test1.step8.refoutput"
	$(E) ""
	$(E) "and now refute-learn into q_test.css"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/FAQ_mt_ng_reference_4.input "$(refdir)/OSB_Winnow_test1.step9.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/OSB_Winnow_test1.step10.refoutput"

# **** Unigram Bayesian classifier **********************************************************
Unigram_Bayesian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Unigram Bayesian classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <unigram> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Bayesian_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Bayesian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Unigram_Bayesian_1.input "$(refdir)/Unigram_Bayesian_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Unigram_Bayesian_2.input "$(refdir)/Unigram_Bayesian_test1.step4.refoutput"

# **** unigram Winnow classifier **********************************************************
Unigram_Winnow_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* unigram Winnow classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Winnow_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Winnow_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Winnow_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Winnow_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }       '  $(abs_srcdir)/mt_ng_unigram_Winnow_1.input "$(refdir)/Unigram_Winnow_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_unigram_Winnow_2.input "$(refdir)/Unigram_Winnow_test1.step6.refoutput"

# **** OSB Hyperspace classifier **********************************************************
OSB_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Hyperspace_1.input "$(refdir)/OSB_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Hyperspace_2.input "$(refdir)/OSB_Hyperspace_test1.step4.refoutput"

# **** OSB three-letter Hyperspace classifier **********************************************************
OSB_3_letter_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB three-letter Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (q_test.css) /.../}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <hyperspace unique> (i_test.css) /.../}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_3_letter_Hyperspace_1.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_3_letter_Hyperspace_2.input "$(refdir)/OSB_3_letter_Hyperspace_test1.step4.refoutput"


# **** Unigram Hyperspace classifier **********************************************************
Unigram_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Unigram Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Unigram_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Unigram_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Unigram_Hyperspace_1.input "$(refdir)/Unigram_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Unigram_Hyperspace_2.input "$(refdir)/Unigram_Hyperspace_test1.step4.refoutput"

# **** String Hyperspace classifier **********************************************************
String_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace string> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_String_Hyperspace_1.input "$(refdir)/String_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace string> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_String_Hyperspace_2.input "$(refdir)/String_Hyperspace_test1.step4.refoutput"

# **** String Unigram Hyperspace classifier **********************************************************
String_Unigram_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Unigram Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string unigram> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Unigram_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace string unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Unigram_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace string unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_String_Unigram_Hyperspace_1.input "$(refdir)/String_Unigram_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace string unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_String_Unigram_Hyperspace_2.input "$(refdir)/String_Unigram_Hyperspace_test1.step4.refoutput"

# **** Vector 3-word-bag Hyperspace classifier **********************************************************
#    the "vector: blahblah" is coded by the desired length of the pipeline,
#    then the number of iterations of the pipe, then pipelen * iters
#    integer coefficients.  Missing coefficients are taken as zero,
#    extra coefficients are disregarded.
Vector_3_word_bag_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Vector 3-word-bag Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace > (q_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 / }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < hyperspace > (i_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < hyperspace > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ /vector: 3 1 1 1 1 1  /; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Vector_3_word_bag_Hyperspace_1.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <hyperspace > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 /; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Vector_3_word_bag_Hyperspace_2.input "$(refdir)/Vector_3_word_bag_Hyperspace_test1.step4.refoutput"


# ****  ALT.(SBPH Markovian) classifier **********************************************************
ALT_SBPH_Markovian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.* (SBPH Markovian) classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f randtst.txt
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.markovian> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_SBPH_Markovian_test1.step1.refoutput"

	$(SILENT)\
	./testscript.sh '-{learn <alt.markovian> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_SBPH_Markovian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:) {classify <alt.markovian> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_SBPH_Markovian_1.input "$(refdir)/ALT_SBPH_Markovian_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:) {classify <alt.markovian> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_SBPH_Markovian_2.input "$(refdir)/ALT_SBPH_Markovian_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** ALT.OSB Markovian classifier **********************************************************
ALT_OSB_Markovian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.OSB Markovian classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.osb> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_Markovian_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.osb> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_Markovian_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <alt.osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Markovian_1.input "$(refdir)/ALT_OSB_Markovian_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <alt.osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Markovian_2.input "$(refdir)/ALT_OSB_Markovian_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** ALT.OSB Markov Unique classifier **********************************************************
ALT_OSB_Markov_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.OSB Markov Unique classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.osb unique > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_Markov_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.osb unique > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_Markov_Unique_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <alt.osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Markov_Unique_1.input "$(refdir)/ALT_OSB_Markov_Unique_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.01 -v pR=0.7"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <alt.osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Markov_Unique_2.input "$(refdir)/ALT_OSB_Markov_Unique_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** ALT.OSB Markov Chisquared Unique classifier **********************************************************
ALT_OSB_Markov_Chisquared_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.OSB Markov Chisquared Unique classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.osb unique chi2> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.osb unique chi2> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Markov_Chisquared_Unique_1.input "$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Markov_Chisquared_Unique_2.input "$(refdir)/ALT_OSB_Markov_Chisquared_Unique_test1.step4.refoutput"

# **** ALT.OSBF Local Confidence (Fidelis) classifier **********************************************************
ALT_OSBF_Local_Confidence_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.OSBF Local Confidence (Fidelis) classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.osbf> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSBF_Local_Confidence_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.osbf> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSBF_Local_Confidence_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSBF_Local_Confidence_Fidelis_1.input "$(refdir)/ALT_OSBF_Local_Confidence_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSBF_Local_Confidence_Fidelis_2.input "$(refdir)/ALT_OSBF_Local_Confidence_test1.step4.refoutput"

# **** ALT.OSB Winnow classifier **********************************************************
ALT_OSB_Winnow_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.OSB Winnow classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_Winnow_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow refute> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_Winnow_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_Winnow_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow refute> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_Winnow_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }       '  $(abs_srcdir)/mt_ng_OSB_Winnow_1.input "$(refdir)/ALT_OSB_Winnow_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/ALT_OSB_Winnow_test1.step6.refoutput"
	$(E) ""
	$(E) "Now verify that alt.winnow learns affect only the named file (i_test.css)"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/COLOPHON_mt_ng_reference_3.input "$(refdir)/ALT_OSB_Winnow_test1.step7.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/ALT_OSB_Winnow_test1.step8.refoutput"
	$(E) ""
	$(E) "and now refute-learn into q_test.css"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow refute > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/FAQ_mt_ng_reference_4.input "$(refdir)/ALT_OSB_Winnow_test1.step9.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_OSB_Winnow_2.input "$(refdir)/ALT_OSB_Winnow_test1.step10.refoutput"

# **** ALT.MARKOVIAN Unigram Bayesian classifier **********************************************************
ALT_Unigram_Bayesian_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.MARKOVIAN Unigram Bayesian classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.markovian unigram> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_Unigram_Bayesian_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.markovian unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_Unigram_Bayesian_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.markovian unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Unigram_Bayesian_1.input "$(refdir)/ALT_Unigram_Bayesian_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.markovian unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Unigram_Bayesian_2.input "$(refdir)/ALT_Unigram_Bayesian_test1.step4.refoutput"

# **** unigram ALT.Winnow classifier **********************************************************
ALT_Unigram_Winnow_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* unigram ALT.Winnow classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow unigram > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_Unigram_Winnow_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow unigram refute> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_Unigram_Winnow_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_Unigram_Winnow_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.winnow unigram refute> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_Unigram_Winnow_test1.step4.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }       '  $(abs_srcdir)/mt_ng_unigram_Winnow_1.input "$(refdir)/ALT_Unigram_Winnow_test1.step5.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}      '  $(abs_srcdir)/mt_ng_unigram_Winnow_2.input "$(refdir)/ALT_Unigram_Winnow_test1.step6.refoutput"

# **** ALT.Hyperspace classifier **********************************************************
ALT_OSB_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* ALT.Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace unique> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace unique> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_Hyperspace_1.input "$(refdir)/ALT_OSB_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_Hyperspace_2.input "$(refdir)/ALT_OSB_Hyperspace_test1.step4.refoutput"

# **** OSB three-letter ALT.Hyperspace classifier **********************************************************
ALT_OSB_3_letter_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* OSB three-letter ALT.Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace unique> (q_test.css) /.../}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace unique> (i_test.css) /.../}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_OSB_3_letter_Hyperspace_1.input "$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_OSB_3_letter_Hyperspace_2.input "$(refdir)/ALT_OSB_3_letter_Hyperspace_test1.step4.refoutput"


# **** Unigram ALT.Hyperspace classifier **********************************************************
ALT_Unigram_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Unigram ALT.Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_Unigram_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_Unigram_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Unigram_Hyperspace_1.input "$(refdir)/ALT_Unigram_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Unigram_Hyperspace_2.input "$(refdir)/ALT_Unigram_Hyperspace_test1.step4.refoutput"

# **** String ALT.Hyperspace classifier **********************************************************
ALT_String_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String ALT.Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace string> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_String_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace string> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_String_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace string> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_String_Hyperspace_1.input "$(refdir)/ALT_String_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace string> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_String_Hyperspace_2.input "$(refdir)/ALT_String_Hyperspace_test1.step4.refoutput"

# **** String Unigram ALT.Hyperspace classifier **********************************************************
ALT_String_Unigram_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Unigram ALT.Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace string unigram> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_String_Unigram_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace string unigram> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_String_Unigram_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace string unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_String_Unigram_Hyperspace_1.input "$(refdir)/ALT_String_Unigram_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace string unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_String_Unigram_Hyperspace_2.input "$(refdir)/ALT_String_Unigram_Hyperspace_test1.step4.refoutput"

# **** Vector 3-word-bag ALT.Hyperspace classifier **********************************************************
#    the "vector: blahblah" is coded by the desired length of the pipeline,
#    then the number of iterations of the pipe, then pipelen * iters
#    integer coefficients.  Missing coefficients are taken as zero,
#    extra coefficients are disregarded.
ALT_Vector_3_word_bag_Hyperspace_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Vector 3-word-bag ALT.Hyperspace classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace> (q_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 / }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn <alt.hyperspace> (i_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 1/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step2.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ /vector: 3 1 1 1 1 1  /; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Vector_3_word_bag_Hyperspace_1.input "$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step3.refoutput"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify <alt.hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ /vector: 3 1 1 1 1 1 /; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Vector_3_word_bag_Hyperspace_2.input "$(refdir)/ALT_Vector_3_word_bag_Hyperspace_test1.step4.refoutput"


# **** Bit-Entropy classifier **********************************************************
Bit_Entropy_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Bit-Entropy classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Bit_Entropy_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Bit_Entropy_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v entropy=50 -v o_entropy=370 -v jumps=4 -v o_jumps=7 -v prob=2.0 -v pR=3.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Bit_Entropy_1.input "$(refdir)/Bit_Entropy_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v file_features=300 -v entropy=50 -v o_entropy=370 -v jumps=40 -v o_jumps=7 -v prob=5.0 -v pR=5.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Bit_Entropy_2.input "$(refdir)/Bit_Entropy_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** Bit-Entropy Toroid classifier **********************************************************
Bit_Entropy_Toroid_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Bit-Entropy Toroid classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < entropy > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Bit_Entropy_Toroid_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{learn < entropy > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Bit_Entropy_Toroid_test1.step2.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v entropy=100 -v jumps=10 -v prob=1.0 -v pR=4.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Bit_Entropy_Toroid_1.input "$(refdir)/Bit_Entropy_Toroid_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v entropy=25 -v jumps=2 -v prob=1.0e-5 -v pR=1.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Bit_Entropy_Toroid_2.input "$(refdir)/Bit_Entropy_Toroid_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** Fast Substring Compression Match Classifier **********************************************************
Fast_Substring_Compression_Match_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Fast Substring Compression Match Classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < fscm > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Fast_Substring_Compression_Match_test1.step1.refoutput" "" -s 200000 
	$(SILENT)\
	./testscript.sh '-{learn < fscm > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Fast_Substring_Compression_Match_test1.step2.refoutput" "" -s 200000 
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Fast_Substring_Compression_Match_1.input "$(refdir)/Fast_Substring_Compression_Match_test1.step3.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	./testscript.sh '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Fast_Substring_Compression_Match_2.input "$(refdir)/Fast_Substring_Compression_Match_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** Neural Network Classifier **********************************************************
Neural_Network_test1: $(TEST_PREREQUISITES) test_timing_minutes_announcement
	$(E) "*******************************************"
	$(E) "* Neural Network Classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{learn < neural append > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Neural_Network_test1.step1.refoutput" "" -s 32768 -RT
	$(SILENT)\
	./testscript.sh '-{learn < neural append > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Neural_Network_test1.step2.refoutput" "" -s 32768 -RT
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{learn < neural refute fromstart > (q_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Neural_Network_test1.step3.refoutput" "" -RT
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{learn < neural refute fromstart > (i_test.css) /[[:graph:]]+/}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Neural_Network_test1.step4.refoutput" "" -RT
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	CRM114_CHECK_FILTER_ARGS="-v icnr=0.2 -v ocnr=0.2 -v prob=0.2 -v pR=10.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Neural_Network_1.input "$(refdir)/Neural_Network_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh" -RT
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	CRM114_CHECK_FILTER_ARGS="-v icnr=0.2 -v ocnr=0.2 -v prob=0.2 -v pR=10.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Neural_Network_2.input "$(refdir)/Neural_Network_test1.step6.refoutput" "$(builddir)/crm114_classify.filter.sh" -RT
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	CRM114_CHECK_FILTER_ARGS="-v icnr=0.2 -v ocnr=0.2 -v prob=0.2 -v pR=10.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Neural_Network_test1.step7.refoutput" "$(builddir)/crm114_classify.filter.sh" -RT
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	CRM114_CHECK_FILTER_ARGS="-v icnr=0.2 -v ocnr=0.2 -v prob=0.2 -v pR=10.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Neural_Network_test1.step8.refoutput" "$(builddir)/crm114_classify.filter.sh" -RT



# **** Alternate Neural Network Classifier test script **********************************************************
Neural_Network_test2: $(TEST_PREREQUISITES) test_timing_minutes_announcement alternating_example_neural.crm
	$(E) "*******************************************"
	$(E) "* Alternate Neural Network Classifier test script "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v icnr=0.4 -v ocnr=0.4 -v prob=0.4 -v pR=20.0"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh alternating_example_neural.crm "" - "$(builddir)/crm114_classify.filter.sh" -RT


# **** Support Vector Machine (SVM) unigram classifier **********************************************************
Support_Vector_Machine_Unigram_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Support Vector Machine (SVM) unigram classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_vs_q_test.css
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; learn [:one_paragraph:] < svm unigram unique > (i_test.css) /[[:graph:]]+/; liaf}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; learn [:one_paragraph:] < svm unigram unique > (q_test.css) /[[:graph:]]+/; liaf }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unigram unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }' "" "$(refdir)/Support_Vector_Machine_Unigram_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.1 -v pR=0.2"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Support_Vector_Machine_SVM_unigram_1.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=4e-4 -v o_prob=0.9946 -v pR=0.03 -vo_pR=22.69"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Support_Vector_Machine_SVM_unigram_2.input "$(refdir)/Support_Vector_Machine_Unigram_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** Support Vector Machine (SVM) classifier **********************************************************
Support_Vector_Machine_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Support Vector Machine (SVM) classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_vs_q_test.css
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; learn [:one_paragraph:] < svm unique > (i_test.css) /[[:graph:]]+/; liaf}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Support_Vector_Machine_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; learn [:one_paragraph:] < svm unique > (q_test.css) /[[:graph:]]+/; liaf }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Support_Vector_Machine_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }' "" "$(refdir)/Support_Vector_Machine_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=4e-3 -v o_prob=0.8694 -v pR=0.04 -v o_pR=8.23"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Support_Vector_Machine_SVM_1.input "$(refdir)/Support_Vector_Machine_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=8e-4 -v pR=0.04 -v o_pR=14.775"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Support_Vector_Machine_SVM_2.input "$(refdir)/Support_Vector_Machine_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** String Kernel SVM (SKS) classifier **********************************************************
String_Kernel_SVM_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Kernel SVM (SKS) classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_vs_q_test.css
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; learn [:one_paragraph:] < sks > (i_test.css) /[[:graph:]]+/; liaf}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Kernel_SVM_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; learn [:one_paragraph:] < sks > (q_test.css) /[[:graph:]]+/; liaf }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Kernel_SVM_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }' "" "$(refdir)/String_Kernel_SVM_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=0.0004 -v pR=0.03"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_String_Kernel_SVM_SKS_1.input "$(refdir)/String_Kernel_SVM_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v prob=2e-4 -v pR=0.04"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_String_Kernel_SVM_SKS_2.input "$(refdir)/String_Kernel_SVM_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** String Kernel SVM (SKS) Unique classifier **********************************************************
String_Kernel_SVM_Unique_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* String Kernel SVM (SKS) Unique classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_vs_q_test.css
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/; translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (i_test.css) /[[:graph:]]+/ / 0 0 100 0.001 1 1 4/; liaf}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/String_Kernel_SVM_Unique_test1.step1.refoutput"
	$(SILENT)\
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?:*:_nl::*:_nl:/;  translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (q_test.css) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ ; liaf }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/String_Kernel_SVM_Unique_test1.step2.refoutput"
	$(E) ""
	$(E) "  build the actual hyperplanes"
	$(E) ""
	$(SILENT)\
	./testscript.sh '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks unique > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }' "" "$(refdir)/String_Kernel_SVM_Unique_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:);  translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_String_Kernel_SVM_SKS_Unique_1.input "$(refdir)/String_Kernel_SVM_Unique_test1.step4.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:); translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_String_Kernel_SVM_SKS_Unique_2.input "$(refdir)/String_Kernel_SVM_Unique_test1.step5.refoutput" "$(builddir)/crm114_classify.filter.sh"


# **** Bytewise Correlation classifier **********************************************************
Bytewise_Correlation_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Bytewise Correlation classifier "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_vs_q_test.css
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:) {classify <correlate> ( $(abs_srcdir)/INTRO_mt_ng_reference_2.input | $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Bytewise_Correlation_1.input "$(refdir)/Bytewise_Correlation_test1.step1.refoutput" "$(builddir)/crm114_classify.filter.sh"
	$(SILENT)\
	CRM114_CHECK_FILTER_ARGS="-v features=10 -v prob=0.004 -v pR=0.1"; export CRM114_CHECK_FILTER_ARGS; \
	./testscript.sh '-{ isolate (:s:) {classify <correlate> ( $(abs_srcdir)/INTRO_mt_ng_reference_2.input | $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I :*:_nl::*:s::*:_nl:/} alius { output / type Q :*:_nl::*:s::*:_nl:/ }}'  $(abs_srcdir)/mt_ng_Bytewise_Correlation_2.input "$(refdir)/Bytewise_Correlation_test1.step2.refoutput" "$(builddir)/crm114_classify.filter.sh"

# **** Clump \/ Pmulc Test **********************************************************
Clump_Pmulc_test1: $(TEST_PREREQUISITES) test_timing_seconds_announcement
	$(E) "*******************************************"
	$(E) "* Clump \/ Pmulc Test "
	$(E) "*******************************************"
	-$(SILENT)rm -f i_test.css
	-$(SILENT)rm -f q_test.css
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?:*:_nl::*:_nl:){5}/; clump <bychunk> [:one_paragraph:] (i_test.css) /[[:graph:]]+/; output /./ ; liaf}'  $(abs_srcdir)/INTRO_mt_ng_reference_2.input "$(refdir)/Clump_Pmulc_test1.step1.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?:*:_nl::*:_nl:){5}/; clump [:one_paragraph:] <bychunk> (i_test.css) /[[:graph:]]+/; output /./; liaf }'  $(abs_srcdir)/QUICKREF_mt_ng_reference_1.input "$(refdir)/Clump_Pmulc_test1.step2.refoutput"
	$(E) ""
	$(E) "Now see where our paragraphs go to"
	$(E) ""
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: :*:_nl::*:s::*:_nl:/} alius { output / Unsure result :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Clump_Pmulc_1.input "$(refdir)/Clump_Pmulc_test1.step3.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: :*:_nl::*:s::*:_nl:/} alius { output / Unsure result :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Clump_Pmulc_2.input "$(refdir)/Clump_Pmulc_test1.step4.refoutput"
	$(SILENT)\
	CRM114_CHECK_OVERRIDE=0; export CRM114_CHECK_OVERRIDE; \
	./testscript.sh '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: :*:_nl::*:s::*:_nl:/} alius { output / Unsure result :*:_nl::*:s::*:_nl:/ } }'  $(abs_srcdir)/mt_ng_Clump_Pmulc_3.input "$(refdir)/Clump_Pmulc_test1.step5.refoutput"



TESTSET_CATALOG_FILES = \
		testcases/test00000001/good.txt \
		testcases/test00000001/bad.txt

mk_test_sets: 				\
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)

#
# HACK: just check for this single file; too lazy to check for all of 'em
#
testcases/test00000001/good.txt:	\
		mk_testcases.sh \
		$(srcdir)/mk_uniq_files.awk \
		scowl/english-upper-all \
		scowl/english-words-all
	$(E) "*******************************************"
	$(E) "* Generating fundamental test sets"
	$(E) "*******************************************"
	$(SILENT)rm -rf testcases/
	$(SILENT)./mk_testcases.sh

testcases/test00000001/bad.txt:	\
		testcases/test00000001/good.txt


scowl_dir:
	-$(MKDIR_P) scowl


scowl/english-upper-all: \
		$(FUNDAMENTAL_TEST_REFFILES)
	-$(MKDIR_P) scowl
	$(SILENT)rm -f scowl/english-upper-all
	$(SILENT)for f in `find "$(srcdir)/scowl" -type f -name '*upper*' | sort`; \
	  do ( \
	    cat "$$f" >> scowl/english-upper-all \
	  ); done

scowl/english-words-all: \
		$(FUNDAMENTAL_TEST_REFFILES)
	-$(MKDIR_P) scowl
	$(SILENT)rm -f scowl/english-words-all
	$(SILENT)for f in `find "$(srcdir)/scowl" -type f -name '*words*' | sort`; \
	  do ( \
	    cat "$$f" >> scowl/english-words-all \
	  ); done
	
fundamental_tenfold_tests: 			\
		fundamental_tenfold_test1 \
		fundamental_tenfold_test2 \
		fundamental_tenfold_test3 \
		fundamental_tenfold_test4 \
		fundamental_tenfold_test5 \
		fundamental_tenfold_test6 \
		fundamental_tenfold_test7 \
		fundamental_tenfold_test8 \
		fundamental_tenfold_test9 \
		fundamental_tenfold_test10 \
		fundamental_tenfold_test11 \
		fundamental_tenfold_test12 \
		fundamental_tenfold_test13 \
		fundamental_tenfold_test14 \
		fundamental_tenfold_test15 \
		fundamental_tenfold_test16 \
		fundamental_tenfold_test17 \
		fundamental_tenfold_test18 \
		fundamental_tenfold_test19 \
		fundamental_tenfold_test20 \
		fundamental_tenfold_test21 \
		fundamental_tenfold_test22 \
		fundamental_tenfold_test23 \
		fundamental_tenfold_test24 \
		fundamental_tenfold_test25 \
		fundamental_tenfold_test26 \
		fundamental_tenfold_test27 \
		fundamental_tenfold_test28 \
		fundamental_tenfold_test29 \
		fundamental_tenfold_test30 \
		fundamental_tenfold_test31




fundamental_tenfold_test1: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test1.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000001/good.txt" "$(abs_builddir)/testcases/test00000001/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test2: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test2.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000002/good.txt" "$(abs_builddir)/testcases/test00000002/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test3: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test3.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000003/good.txt" "$(abs_builddir)/testcases/test00000003/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test4: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test4.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000004/good.txt" "$(abs_builddir)/testcases/test00000004/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test5: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test5.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000005/good.txt" "$(abs_builddir)/testcases/test00000005/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test6: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test6.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000006/good.txt" "$(abs_builddir)/testcases/test00000006/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test7: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test7.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000007/good.txt" "$(abs_builddir)/testcases/test00000007/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test8: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test8.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000008/good.txt" "$(abs_builddir)/testcases/test00000008/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test9: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test9.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000009/good.txt" "$(abs_builddir)/testcases/test00000009/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test10: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test10.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000010/good.txt" "$(abs_builddir)/testcases/test00000010/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test11: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test11.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000011/good.txt" "$(abs_builddir)/testcases/test00000011/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test12: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test12.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000012/good.txt" "$(abs_builddir)/testcases/test00000012/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test13: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test13.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000013/good.txt" "$(abs_builddir)/testcases/test00000013/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test14: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test14.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000014/good.txt" "$(abs_builddir)/testcases/test00000014/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test15: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test15.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000015/good.txt" "$(abs_builddir)/testcases/test00000015/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test16: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test16.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000016/good.txt" "$(abs_builddir)/testcases/test00000016/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test17: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test17.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000017/good.txt" "$(abs_builddir)/testcases/test00000017/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test18: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test18.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000018/good.txt" "$(abs_builddir)/testcases/test00000018/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test19: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test19.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000019/good.txt" "$(abs_builddir)/testcases/test00000019/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test20: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test20.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000020/good.txt" "$(abs_builddir)/testcases/test00000020/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test21: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test21.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000021/good.txt" "$(abs_builddir)/testcases/test00000021/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test22: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test22.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000022/good.txt" "$(abs_builddir)/testcases/test00000022/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test23: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test23.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000023/good.txt" "$(abs_builddir)/testcases/test00000023/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test24: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test24.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000024/good.txt" "$(abs_builddir)/testcases/test00000024/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test25: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test25.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000025/good.txt" "$(abs_builddir)/testcases/test00000025/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test26: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test26.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000026/good.txt" "$(abs_builddir)/testcases/test00000026/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test27: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test27.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000027/good.txt" "$(abs_builddir)/testcases/test00000027/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test28: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test28.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000028/good.txt" "$(abs_builddir)/testcases/test00000028/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test29: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test29.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000029/good.txt" "$(abs_builddir)/testcases/test00000029/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test30: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test30.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000030/good.txt" "$(abs_builddir)/testcases/test00000030/bad.txt" --show_partitions --results=stderr

fundamental_tenfold_test31: \
		$(FUNDAMENTAL_TEST_PREREQS) \
		$(TESTSET_CATALOG_FILES)
	./testscript.sh "$(abs_builddir)/tenfold_validate_mailreaver.crm" "" "$(refdir)/fundamental_tenfold_test31.refoutput" "" -u "$(builddir)/../mailfilter" "$(abs_builddir)/testcases/test00000031/good.txt" "$(abs_builddir)/testcases/test00000031/bad.txt" --show_partitions --results=stderr










cleanup_test:
	-rm -f *.css
	-rm -f e1.out
	-rm -f e2.out
	-rm -f randtst.txt
	-rm -rf *.temp.*
	-rm -rf testcases/


#./tests/test_rewrites.mfp
#./tests/e2.out
#./tests/whitelist.mfp.example



clean-local: cleanup_test







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
#check-am: all-am
#	$(SILENT)\
#	srcdir=$(srcdir); export srcdir; \
#	builddir=$(builddir); export builddir; \
#	$(MAKE) $(AM_MAKEFLAGS) $(check_PROGRAMS); \
#	$(MAKE) $(AM_MAKEFLAGS) check-TESTS check-local; \
#	$(MAKE) $(AM_MAKEFLAGS) cleanup_test


.PHONY: 		\
		test \
		test_basics \
		test_other \
		test_megatest \
		test_megatest_ng \
		test_fundamental_mail \
		megatest megatest_ng \
		test_classifiers \
		mailreaver_scripts \
		crm114_binaries \
		ref_dir \
		scowl_dir \
		mk_test_sets \
		test_timing_minutes_announcement \
		test_timing_seconds_announcement


