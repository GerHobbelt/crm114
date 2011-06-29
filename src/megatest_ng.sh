#! /bin/sh
./crm114 -v 2>&1
./crm114 '-{window; output / \n***** checking CRM language features\n/}'
./crm114 bracktest.crm 
./crm114 escapetest.crm 
./crm114 fataltraptest.crm 
./crm114 inserttest_a.crm
./crm114 matchtest.crm  < matchtest_mt_ng_1.input
./crm114 backwardstest.crm  < backwardstest_mt_ng_1.input
./crm114 backwardstest.crm  < backwardstest_mt_ng_2.input
./crm114 overalterisolatedtest.crm 
./crm114 rewritetest.crm 
./crm114 skudtest.crm 
./crm114 statustest.crm 
./crm114 unionintersecttest.crm 
./crm114 beeptest.crm 
./crm114 defaulttest.crm
./crm114 defaulttest.crm --blah="command override"
./crm114 windowtest.crm  < windowtest_mt_ng_1.input
./crm114 windowtest_fromvar.crm  < windowtest_fromvar_mt_ng_1.input
./crm114 approxtest.crm  < approxtest_mt_ng_1.input
./crm114 mathalgtest.crm 
./crm114 mathrpntest.crm -q 1
./crm114 eval_infiniteloop.crm
./crm114 randomiotest.crm
./crm114 paolo_overvars.crm
./crm114 paolo_ov2.crm
./crm114 paolo_ov3.crm
./crm114 paolo_ov4.crm
./crm114 paolo_ov5.crm
./crm114 match_isolate_test.crm -e
./crm114 match_isolate_reclaim.crm -e
./crm114 call_return_test.crm
./crm114 translate_tr.crm
./crm114 zz_translate_test.crm
./crm114 quine.crm
./crm114 '-{window; isolate (:s:); syscall () (:s:) /echo one two three/; output /:*:s:/}'
for i in 1 $1 ; do ./crm114 '-{window; output / \n***** checking return and exit codes \n/}' ; done
./crm114 '-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: \n/}'
for i in 1 $1 ; do ./crm114 '-{window; output /\n***** check that failed syscalls will code right\n/}' ; done
./crm114 '-{window; isolate (:s:); syscall () () (:s:) /jibberjabber 2>&1 /; output / Status: :*:s: \n/}'

./crm114 indirecttest.crm
rm -f randtst.txt
rm -f i_test.css
rm -f q_test.css

for i in 1 $1 ; do ./crm114 '-{window; output /\n ****  Default (SBPH Markovian) classifier \n/}' ; done
./crm114 '-{learn (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_SBPH_Markovian_1.input
./crm114 '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_SBPH_Markovian_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** OSB Markovian classifier \n/}' ; done
./crm114 '-{learn <osb> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn <osb> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_OSB_Markovian_1.input
./crm114 '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_OSB_Markovian_2.input


rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** OSB Markov Unique classifier \n/}' ; done
./crm114 '-{learn <osb unique > (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn <osb unique > (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_OSB_Markov_Unique_1.input
./crm114 '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_OSB_Markov_Unique_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** OSB Markov Chisquared Unique classifier \n/}' ; done
./crm114 '-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_OSB_Markov_Chisquared_Unique_1.input
./crm114 '-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_OSB_Markov_Chisquared_Unique_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** OSBF Local Confidence (Fidelis) classifier \n/}' ; done
./crm114 '-{learn < osbf > (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < osbf > (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_OSBF_Local_Confidence_Fidelis_1.input
./crm114 '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_OSBF_Local_Confidence_Fidelis_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output / \n**** OSB Winnow classifier \n/}' ; done
./crm114 '-{learn <winnow> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input 
./crm114 '-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{learn <winnow> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input 
./crm114 '-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input 
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  < mt_ng_OSB_Winnow_1.input
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' < mt_ng_OSB_Winnow_2.input

for i in 1 $1 ; do ./crm114 '-{ window; output /\n\n**** Now verify that winnow learns affect only the named file (i_test.css)\n/}' ; done
./crm114 '-{learn <winnow> (i_test.css) /[[:graph:]]+/}' < COLOPHON_mt_ng_reference_3.input 
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' < mt_ng_OSB_Winnow_2.input

for i in 1 $1 ; do ./crm114 '-{window; output /\n\n and now refute-learn into q_test.css\n/}' ; done
./crm114 '-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}' < FAQ_mt_ng_reference_4.input
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' < mt_ng_OSB_Winnow_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Unigram Bayesian classifier \n/}' ; done
./crm114 '-{learn <unigram> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn <unigram> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Unigram_Bayesian_1.input
./crm114 '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Unigram_Bayesian_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output / \n**** unigram Winnow classifier \n/}' ; done
./crm114 '-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input 
./crm114 '-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input 
./crm114 '-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input 
./crm114 '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  < mt_ng_unigram_Winnow_1.input
./crm114 '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' < mt_ng_unigram_Winnow_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** OSB Hyperspace classifier \n/}' ; done
./crm114 '-{learn <hyperspace unique> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn <hyperspace unique> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_OSB_Hyperspace_1.input
./crm114 '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_OSB_Hyperspace_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** OSB three-letter Hyperspace classifier \n/}' ; done
./crm114 '-{learn <hyperspace unique> (q_test.css) /.../}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn <hyperspace unique> (i_test.css) /.../}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_OSB_3_letter_Hyperspace_1.input
./crm114 '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_OSB_3_letter_Hyperspace_2.input


rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Unigram Hyperspace classifier \n/}' ; done
./crm114 '-{learn < hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Unigram_Hyperspace_1.input
./crm114 '-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Unigram_Hyperspace_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** String Hyperspace classifier \n/}' ; done
./crm114 '-{learn < hyperspace string> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < hyperspace string> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < hyperspace string> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_String_Hyperspace_1.input
./crm114 '-{ isolate (:s:); {classify <hyperspace string> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_String_Hyperspace_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** String Unigram Hyperspace classifier \n/}' ; done
./crm114 '-{learn < hyperspace string unigram> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < hyperspace string unigram> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < hyperspace string unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_String_Unigram_Hyperspace_1.input
./crm114 '-{ isolate (:s:); {classify <hyperspace string unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_String_Unigram_Hyperspace_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Vector 3-word-bag Hyperspace classifier \n/}' ; done
#    the "vector: blahblah" is coded by the desired length of the pipeline,
#    then the number of iterations of the pipe, then pipelen * iters 
#    integer coefficients.  Missing coefficients are taken as zero, 
#    extra coefficients are disregarded.
./crm114 '-{learn < hyperspace > (q_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 / }' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < hyperspace > (i_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < hyperspace > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ /vector: 3 1 1 1 1  /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Vector_3_word_bag_Hyperspace_1.input
./crm114 '-{ isolate (:s:); {classify <hyperspace > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ /vector: 3 1 1 1 1 /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Vector_3_word_bag_Hyperspace_2.input


rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Bit-Entropy classifier \n/}' ; done
./crm114 '-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Bit_Entropy_1.input
./crm114 '-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Bit_Entropy_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Bit-Entropy Toroid classifier \n/}' ; done
./crm114 '-{learn < entropy > (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 '-{learn < entropy > (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Bit_Entropy_Toroid_1.input
./crm114 '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Bit_Entropy_Toroid_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Fast Substring Compression Match Classifier \n/}' ; done
./crm114 -s 200000 '-{learn < fscm > (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 -s 200000 '-{learn < fscm > (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Fast_Substring_Compression_Match_1.input
./crm114 '-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Fast_Substring_Compression_Match_2.input

rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Neural Network Classifier \n/}' ; done
./crm114 -s 32768 '-{learn < neural append > (q_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input
./crm114 -s 32768 '-{learn < neural append > (i_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{learn < neural refute fromstart > (q_test.css) /[[:graph:]]+/}' < INTRO_mt_ng_reference_2.input
./crm114 '-{learn < neural refute fromstart > (i_test.css) /[[:graph:]]+/}' < QUICKREF_mt_ng_reference_1.input

./crm114 '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Neural_Network_1.input
./crm114 '-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Neural_Network_2.input



rm -f i_vs_q_test.css
rm -f i_test.css 
rm -f q_test.css
for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Support Vector Machine (SVM) unigram classifier \n/}' ; done
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (i_test.css) /[[:graph:]]+/; liaf}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (q_test.css) /[[:graph:]]+/; liaf }' < QUICKREF_mt_ng_reference_1.input
#    build the actual hyperplanes
./crm114 '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unigram unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }'

./crm114 '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Support_Vector_Machine_SVM_unigram_1.input
./crm114 '-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Support_Vector_Machine_SVM_unigram_2.input


rm -f i_vs_q_test.css
rm -f i_test.css 
rm -f q_test.css

for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Support Vector Machine (SVM) classifier \n/}' ; done
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (i_test.css) /[[:graph:]]+/; liaf}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (q_test.css) /[[:graph:]]+/; liaf }' < QUICKREF_mt_ng_reference_1.input
#    build the actual hyperplanes
./crm114 '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }'

./crm114 '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_Support_Vector_Machine_SVM_1.input
./crm114 '-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Support_Vector_Machine_SVM_2.input


rm -f i_vs_q_test.css
rm -f i_test.css 
rm -f q_test.css

for i in 1 $1 ; do ./crm114 '-{window; output /\n**** String Kernel SVM (SKS) classifier \n/}' ; done
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (i_test.css) /[[:graph:]]+/; liaf}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (q_test.css) /[[:graph:]]+/; liaf }' < QUICKREF_mt_ng_reference_1.input
#    build the actual hyperplanes
./crm114 '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }'

./crm114 '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_String_Kernel_SVM_SKS_1.input
./crm114 '-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_String_Kernel_SVM_SKS_2.input


rm -f i_vs_q_test.css
rm -f i_test.css 
rm -f q_test.css

for i in 1 $1 ; do ./crm114 '-{window; output /\n**** String Kernel SVM (SKS) Unique classifier \n/}' ; done
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (i_test.css) /[[:graph:]]+/ / 0 0 100 0.001 1 1 4/; liaf}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/;  translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%^&*()/; learn [:one_paragraph:] < sks unique > (q_test.css) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ ; liaf }' < QUICKREF_mt_ng_reference_1.input
#    build the actual hyperplanes
./crm114 '-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks unique > /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ }'

./crm114 '-{ isolate (:s:);  translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' < mt_ng_String_Kernel_SVM_SKS_Unique_1.input
./crm114 '-{ isolate (:s:); translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_String_Kernel_SVM_SKS_Unique_2.input


rm -f i_vs_q_test.css
rm -f i_test.css 
rm -f q_test.css


for i in 1 $1 ; do ./crm114 '-{window ; output /\n**** Bytewise Correlation classifier \n/}' ; done
./crm114 '-{ isolate (:s:) {classify <correlate> ( INTRO_mt_ng_reference_2.input | QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Bytewise_Correlation_1.input
./crm114 '-{ isolate (:s:) {classify <correlate> ( INTRO_mt_ng_reference_2.input | QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' < mt_ng_Bytewise_Correlation_2.input

rm -f i_test.css 
rm -f q_test.css

for i in 1 $1 ; do ./crm114 '-{window; output /\n**** Clump \/ Pmulc Test \n/}' ; done
./crm114 '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump <bychunk> [:one_paragraph:] (i_test.css) /[[:graph:]]+/; output /./ ; liaf}' < INTRO_mt_ng_reference_2.input
./crm114 '-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump [:one_paragraph:] <bychunk> (i_test.css) /[[:graph:]]+/; output /./; liaf }' < QUICKREF_mt_ng_reference_1.input

#    Now see where our paragraphs go to
./crm114 '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }' < mt_ng_Clump_Pmulc_1.input
./crm114 '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }' < mt_ng_Clump_Pmulc_2.input

./crm114 '-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }' < mt_ng_Clump_Pmulc_3.input

rm -f i_test.css 
rm -f q_test.css

