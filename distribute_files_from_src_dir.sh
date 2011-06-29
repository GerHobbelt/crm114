#! /bin/sh

#
# copy files from the src/ directory to their intended locations.
#
# this script assumes you're running this script as part of the cleanup/autoconf migration process
# based on the CRM114 bleeding edge code from paolo et al (which should be located in src/  )
# 

check_file()
{
    echo "copying             : src/$2 -- $1/$2$3"
    if [ -f src/$2 ]
    then
        cp src/$2 $1/$2$3
    fi
}

process_crm()
{
    echo "copying[processed]  : src/$2 -- $1/$2$3"
    if [ -f src/$2 ]
    then
        cat src/$2 | sed -e 's,#! *\/usr\/bin\/crm,#! @abs_top_builddir@/src/crm114,g' > $1/$2$3
    fi
}

process_shfile()
{
    echo "copying[procd/shell]: src/$2 -- $1/$2$3"
    if [ -f src/$2 ]
    then
        cat src/$2 | sed -e 's,\./crm114,@abs_top_builddir@/src/crm114,g' > $1/$2$3
        chmod a+x $1/$2$3
    fi
}

process_crm         tests                unionintersecttest.crm            .in
process_crm         tests                aliustest.crm                     .in
process_crm         tests                approxtest.crm                    .in
process_crm         tests                argtest.crm                       .in
process_crm         tests                backwardstest.crm                 .in
process_crm         tests                beeptest.crm                      .in
process_crm         tests                bracktest.crm                     .in
process_crm         tests                classifytest.crm                  .in
process_crm         tests                escapetest.crm                    .in
process_crm         tests                eval_infiniteloop.crm             .in
process_crm         tests                exectest.crm                      .in
process_crm         tests                fataltraptest.crm                 .in
process_crm         tests                inserttest_a.crm                  .in
process_crm         tests                inserttest_b.crm                  .in
process_crm         tests                inserttest_c.crm                  .in
process_crm         tests                learntest.crm                     .in
process_crm         tests                match_isolate_test.crm            .in
process_crm         tests                matchtest.crm                     .in
process_crm         tests                mathalgtest.crm                   .in
process_crm         tests                mathrpntest.crm                   .in
process_crm         tests                nestaliustest.crm                 .in
process_crm         tests                overalterisolatedtest.crm         .in
process_crm         tests                paolo_overvars.crm                .in
process_crm         tests                randomiotest.crm                  .in
process_crm         tests                rewritetest.crm                   .in
process_crm         tests                skudtest.crm                      .in
process_crm         tests                statustest.crm                    .in
process_crm         tests                traptest.crm                      .in
process_crm         tests                uncaughttraptest.crm              .in
process_crm         tests                unionintersecttest.crm            .in
process_crm         tests                userdirtest.crm                   .in
process_crm         tests                windowtest.crm                    .in
process_crm         tests                windowtest_fromvar.crm            .in
process_crm         tests                tenfold_validate.crm              .in
process_crm         tests                tokendelimiterbugtest.crm         .in
process_crm         tests                bracesbugtest.crm                 .in
process_crm         tests                blowuptrapbugtest.crm             .in
process_crm         tests                indirecttest.crm                  .in
process_crm         tests                isolate_reclaim_test.crm          .in
process_crm         tests                match_isolate_reclaim.crm         .in
process_crm         tests                slashbugtest.crm                  .in
process_crm         tests                trapseqbugtest.crm                .in
process_crm         tests                translate_tr.crm                  .in
process_crm         tests                call_return_test.crm              .in
process_crm         tests                alternating_example_neural.crm    .in
process_crm         tests                defaulttest.crm                   .in
process_crm         tests                alius_w_comment.crm               .in
process_crm         tests                zz_translate_test.crm             .in
process_crm         tests                quine.crm                         .in
process_crm         tests                print_binary2decimal_int32.crm    .in
process_crm         tests                rewritetest.crm                   .in
process_crm         tests                paolo_ov2.crm                     .in
process_crm         tests                paolo_ov3.crm                     .in
process_crm         tests                paolo_ov4.crm                     .in
process_crm         tests                paolo_ov5.crm                     .in


check_file          tests                megatest_knowngood.log
check_file          tests                megatest.sh                        .BillY


check_file          tests        test_rewrites.mfp
check_file          tests                mt_ng_Bit_Entropy_2.input
check_file                tests                mt_ng_OSB_Markov_Chisquared_Unique_2.input
check_file                tests                mt_ng_OSB_Markovian_1.input
check_file                tests                mt_ng_OSB_Markov_Chisquared_Unique_1.input
check_file                tests                mt_ng_String_Unigram_Hyperspace_2.input
check_file                tests                mt_ng_String_Kernel_SVM_SKS_Unique_1.input
check_file                tests                mt_ng_Support_Vector_Machine_SVM_1.input
check_file                tests                mt_ng_Bit_Entropy_1.input
check_file                tests                mt_ng_unigram_Winnow_1.input
check_file                tests                mt_ng_Bytewise_Correlation_2.input
check_file                tests                mt_ng_OSB_Winnow_1.input
check_file                tests                mt_ng_Bit_Entropy_Toroid_1.input
check_file                tests                mt_ng_Neural_Network_1.input
check_file                tests                mt_ng_String_Kernel_SVM_SKS_1.input
check_file                tests                mt_ng_OSB_3_letter_Hyperspace_1.input
check_file                tests                mt_ng_Support_Vector_Machine_SVM_2.input
check_file                tests                windowtest_fromvar_mt_ng_1.input
check_file                tests                mt_ng_Neural_Network_2.input
check_file                tests                mt_ng_Fast_Substring_Compression_Match_1.input
check_file                tests                backwardstest_mt_ng_1.input
check_file                tests                mt_ng_Unigram_Bayesian_2.input
check_file                tests                mt_ng_Support_Vector_Machine_SVM_unigram_2.input
check_file                tests                mt_ng_String_Hyperspace_2.input
check_file                tests                mt_ng_SBPH_Markovian_1.input
check_file                tests                mt_ng_OSB_Markov_Unique_2.input
check_file                tests                mt_ng_Unigram_Hyperspace_1.input
check_file                tests                mt_ng_Unigram_Bayesian_1.input
check_file                tests                mt_ng_OSBF_Local_Confidence_Fidelis_2.input
check_file                tests                mt_ng_String_Kernel_SVM_SKS_2.input
check_file                tests                matchtest_mt_ng_1.input
check_file                tests                mt_ng_OSB_Markovian_2.input
check_file                tests                mt_ng_String_Unigram_Hyperspace_1.input
check_file                tests                mt_ng_Unigram_Hyperspace_2.input
check_file                tests                mt_ng_Vector_3_word_bag_Hyperspace_1.input
check_file                tests                mt_ng_String_Hyperspace_1.input
check_file                tests                approxtest_mt_ng_1.input
check_file                tests                mt_ng_Clump_Pmulc_3.input
check_file                tests                approxtest_mt_ng_2.input
check_file                tests                mt_ng_OSB_Hyperspace_1.input
check_file                tests                mt_ng_OSB_3_letter_Hyperspace_2.input
check_file                tests                mt_ng_OSB_Hyperspace_2.input
check_file                tests                mt_ng_Vector_3_word_bag_Hyperspace_2.input
check_file                tests                mt_ng_Bit_Entropy_Toroid_2.input
check_file                tests                windowtest_mt_ng_1.input
check_file                tests                mt_ng_OSB_Markov_Unique_1.input
check_file                tests                mt_ng_OSBF_Local_Confidence_Fidelis_1.input
check_file                tests                mt_ng_unigram_Winnow_2.input
check_file                tests                mt_ng_Fast_Substring_Compression_Match_2.input
check_file                tests                backwardstest_mt_ng_2.input
check_file                tests                mt_ng_Bytewise_Correlation_1.input
check_file                tests                mt_ng_String_Kernel_SVM_SKS_Unique_2.input
check_file                tests                mt_ng_Clump_Pmulc_1.input
check_file                tests                mt_ng_OSB_Winnow_2.input
check_file                tests                mt_ng_Clump_Pmulc_2.input
check_file                tests                mt_ng_SBPH_Markovian_2.input
check_file                tests                mt_ng_Support_Vector_Machine_SVM_unigram_1.input
check_file                tests                COLOPHON_mt_ng_reference_3.input
check_file                tests                FAQ_mt_ng_reference_4.input
check_file                tests                INTRO_mt_ng_reference_2.input
check_file                tests                QUICKREF_mt_ng_reference_1.input


process_shfile      tests                megatest.sh                        .in
process_shfile      tests                megatest_ng.sh                     .in

check_file          tests                whitelist.mfp.example



process_crm         mailfilter        classifymail.crm        .in
process_crm         mailfilter        mailfilter.crm          .in
process_crm         mailfilter        maillib.crm             .in
process_crm         mailfilter        mailreaver.crm          .in
process_crm         mailfilter        mailtrainer.crm         .in

check_file        mailfilter        mailfilter.cf



check_file        docs                classify_details.txt
check_file        docs                COLOPHON.txt
check_file        docs                QUICKREF.txt
check_file        docs                INTRO.txt
check_file        docs                knownbugs.txt
check_file        docs                FAQ.txt
check_file        docs                things_to_do.txt
check_file        docs                README 
check_file        docs                CRM114_Mailfilter_HOWTO.txt 
check_file        docs                inoc_passwd.txt 
check_file        docs                procmailrc.recipe 
check_file        docs                reto_procmail_recipe.recipe 
check_file        docs                VT_generic_Vector_Tokenization.txt



process_crm         examples        pad.crm                           .in
process_crm         examples        shroud.crm                        .in

check_file        examples        blacklist.mfp
check_file        examples        priolist.mfp
check_file        examples        rewrites.mfp
check_file        examples        whitelist.mfp
check_file        examples        whitelist.mfp.example
process_crm       examples        rewriteutil.crm                 .in

check_file        examples        pad.dat



check_file        docs            COLOPHON.txt
check_file        docs            INTRO.txt
check_file        docs            QUICKREF.txt
check_file        docs            CRM114_Mailfilter_HOWTO.txt
check_file        docs            knownbugs.txt
check_file        docs            classify_details.txt



