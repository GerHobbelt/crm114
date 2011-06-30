@echo off

echo === TEST 003 ===
.\crm114 -v 2>&1
.\crm114 "-{window; output / \n***** checking CRM language features\n/}"
echo === TEST 005 ===
.\crm114 bracktest.crm 
echo === TEST 007 ===
.\crm114 escapetest.crm 
echo === TEST 009 ===
.\crm114 fataltraptest.crm 
echo === TEST 011 ===
.\crm114 inserttest_a.crm

echo === TEST 014 ===
.\crm114 matchtest.crm  < matchtest_mt_ng_1.input



echo === TEST 062 ===
.\crm114 backwardstest.crm  < backwardstest_mt_ng_1.input
echo === TEST 069 ===
.\crm114 backwardstest.crm  < backwardstest_mt_ng_2.input
echo === TEST 076 ===
.\crm114 overalterisolatedtest.crm 
echo === TEST 078 ===
.\crm114 rewritetest.crm 
echo === TEST 080 ===
.\crm114 skudtest.crm 
echo === TEST 082 ===
.\crm114 statustest.crm 
echo === TEST 084 ===
.\crm114 unionintersecttest.crm 
echo === TEST 086 ===
.\crm114 beeptest.crm 
echo === TEST 088 ===
.\crm114 defaulttest.crm
echo === TEST 090 ===
.\crm114 defaulttest.crm --blah="command override"

echo === TEST 093 ===
.\crm114 windowtest.crm  < windowtest_mt_ng_1.input
echo === TEST 100 ===
.\crm114 windowtest_fromvar.crm  < windowtest_fromvar_mt_ng_1.input
echo === TEST 107 ===
.\crm114 approxtest.crm  < approxtest_mt_ng_1.input

echo === TEST 258 ===
.\crm114 mathalgtest.crm 
echo === TEST 260 ===
.\crm114 mathrpntest.crm -q 1
echo === TEST 262 ===
.\crm114 eval_infiniteloop.crm
echo === TEST 264 ===
.\crm114 randomiotest.crm
echo === TEST 266 ===
.\crm114 paolo_overvars.crm
echo === TEST 268 ===
.\crm114 paolo_ov2.crm
echo === TEST 270 ===
.\crm114 paolo_ov3.crm
echo === TEST 272 ===
.\crm114 paolo_ov4.crm
echo === TEST 274 ===
.\crm114 paolo_ov5.crm
echo === TEST 276 ===
.\crm114 match_isolate_test.crm -e
echo === TEST 278 ===
.\crm114 match_isolate_reclaim.crm -e
echo === TEST 280 ===
.\crm114 call_return_test.crm
echo === TEST 282 ===
.\crm114 translate_tr.crm
echo === TEST 284 ===
.\crm114 zz_translate_test.crm
echo === TEST 286 ===
.\crm114 quine.crm
echo === TEST 288 ===
.\crm114 "-{window; isolate (:s:); syscall () (:s:) /echo one two three/; output /:*:s:/}"
echo === TEST 290 ===
.\crm114 "-{window; output / \n***** checking return and exit codes \n/}"
.\crm114 "-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: \n/}"
echo === TEST 293 ===
.\crm114 "-{window; output /\n***** check that failed syscalls will code right\n/}"
.\crm114 "-{window; isolate (:s:); syscall () () (:s:) /jibberjabber 2>&1 /; output / Status: :*:s: \n/}"

echo === TEST 297 ===
.\crm114 indirecttest.crm

del randtst.txt
del i_test.css
del q_test.css

echo === TEST 304 ===
.\crm114 "-{window; output /\n ****  Default (SBPH Markovian) classifier \n/}"
.\crm114 "-{learn (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 307 ===
.\crm114 "-{learn (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 309 ===
.\crm114 "-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_SBPH_Markovian_1.input
echo === TEST 311 ===
.\crm114 "-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_SBPH_Markovian_2.input

del i_test.css 
del q_test.css
echo === TEST 316 ===
.\crm114 "-{window; output /\n**** OSB Markovian classifier \n/}"
.\crm114 "-{learn <osb> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 319 ===
.\crm114 "-{learn <osb> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 321 ===
.\crm114 "-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_OSB_Markovian_1.input
echo === TEST 323 ===
.\crm114 "-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_OSB_Markovian_2.input


del i_test.css 
del q_test.css
echo === TEST 329 ===
.\crm114 "-{window; output /\n**** OSB Markov Unique classifier \n/}"
.\crm114 "-{learn <osb unique > (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 332 ===
.\crm114 "-{learn <osb unique > (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 334 ===
.\crm114 "-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_OSB_Markov_Unique_1.input
echo === TEST 336 ===
.\crm114 "-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_OSB_Markov_Unique_2.input

del i_test.css 
del q_test.css
echo === TEST 341 ===
.\crm114 "-{window; output /\n**** OSB Markov Chisquared Unique classifier \n/}"
.\crm114 "-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 344 ===
.\crm114 "-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 346 ===
.\crm114 "-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_OSB_Markov_Chisquared_Unique_1.input
echo === TEST 348 ===
.\crm114 "-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_OSB_Markov_Chisquared_Unique_2.input

del i_test.css 
del q_test.css
echo === TEST 353 ===
.\crm114 "-{window; output /\n**** OSBF Local Confidence (Fidelis) classifier \n/}"
.\crm114 "-{learn <osbf> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 356 ===
.\crm114 "-{learn <osbf> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 358 ===
.\crm114 "-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_OSBF_Local_Confidence_Fidelis_1.input
echo === TEST 360 ===
.\crm114 "-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_OSBF_Local_Confidence_Fidelis_2.input

del i_test.css 
del q_test.css
echo === TEST 365 ===
.\crm114 "-{window; output / \n**** OSB Winnow classifier \n/}"
.\crm114 "-{learn <winnow> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input 
echo === TEST 368 ===
.\crm114 "-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 370 ===
.\crm114 "-{learn <winnow> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input 
echo === TEST 372 ===
.\crm114 "-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input 
echo === TEST 374 ===
.\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       "  < mt_ng_OSB_Winnow_1.input
echo === TEST 376 ===
.\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " < mt_ng_OSB_Winnow_2.input
echo === TEST 378 ===
.\crm114 "-{ window; output /\n\n**** Now verify that winnow learns affect only the named file (i_test.css)\n/}"
.\crm114 "-{learn <winnow> (i_test.css) /[[:graph:]]+/}" < COLOPHON_mt_ng_reference_3.input 
echo === TEST 381 ===
.\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " < mt_ng_OSB_Winnow_2.input
echo === TEST 383 ===
.\crm114 "-{window; output /\n\n and now refute-learn into q_test.css\n/}"
.\crm114 "-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}" < FAQ_mt_ng_reference_4.input 
echo === TEST 386 ===
.\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " < mt_ng_OSB_Winnow_2.input

del i_test.css 
del q_test.css
echo === TEST 391 ===
.\crm114 "-{window; output /\n**** Unigram Bayesian classifier \n/}"
.\crm114 "-{learn <unigram> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 394 ===
.\crm114 "-{learn <unigram> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 396 ===
.\crm114 "-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Unigram_Bayesian_1.input
echo === TEST 398 ===
.\crm114 "-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Unigram_Bayesian_2.input

del i_test.css 
del q_test.css
echo === TEST 403 ===
.\crm114 "-{window; output / \n**** unigram Winnow classifier \n/}"
.\crm114 "-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input 
echo === TEST 406 ===
.\crm114 "-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 408 ===
.\crm114 "-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input 
echo === TEST 410 ===
.\crm114 "-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input 
echo === TEST 412 ===
.\crm114 "-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       "  < mt_ng_unigram_Winnow_1.input
echo === TEST 414 ===
.\crm114 "-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " < mt_ng_unigram_Winnow_2.input

del i_test.css 
del q_test.css
echo === TEST 419 ===
.\crm114 "-{window; output /\n**** OSB Hyperspace classifier \n/}"
echo === TEST 421 ===
.\crm114 "-{learn <hyperspace unique> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 423 ===
.\crm114 "-{learn <hyperspace unique> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 425 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_OSB_Hyperspace_1.input
echo === TEST 427 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_OSB_Hyperspace_2.input

del i_test.css 
del q_test.css
echo === TEST 428 ===
.\crm114 "-{window; output /\n**** OSB three-letter Hyperspace classifier \n/}"
.\crm114 "-{learn <hyperspace unique> (q_test.css) /.../}" < QUICKREF_mt_ng_reference_1.input
.\crm114 "-{learn <hyperspace unique> (i_test.css) /.../}" < INTRO_mt_ng_reference_2.input
echo === TEST 429 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_OSB_3_letter_Hyperspace_1.input
echo === TEST 430 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /.../ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_OSB_3_letter_Hyperspace_2.input


del i_test.css 
del q_test.css
echo === TEST 434 ===
.\crm114 "-{window; output /\n**** Unigram Hyperspace classifier \n/}"
.\crm114 "-{learn < hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 435 ===
.\crm114 "-{learn < hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 437 ===
.\crm114 "-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Unigram_Hyperspace_1.input
echo === TEST 439 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Unigram_Hyperspace_2.input




del i_test.css 
del q_test.css
echo === TEST 434 ===
.\crm114 "-{window; output /\n**** String Hyperspace classifier \n/}"
.\crm114 "-{learn < hyperspace string> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 435 ===
.\crm114 "-{learn < hyperspace string> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 437 ===
.\crm114 "-{ isolate (:s:); {classify < hyperspace string> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_String_Hyperspace_1.input
echo === TEST 439 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace string> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_String_Hyperspace_2.input



del i_test.css 
del q_test.css
echo === TEST 434 ===
.\crm114 "-{window; output /\n**** String Unigram Hyperspace classifier \n/}"
.\crm114 "-{learn < hyperspace string unigram> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 435 ===
.\crm114 "-{learn < hyperspace string unigram> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 437 ===
.\crm114 "-{ isolate (:s:); {classify < hyperspace string unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_String_Unigram_Hyperspace_1.input
echo === TEST 439 ===
.\crm114 "-{ isolate (:s:); {classify <hyperspace string unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_String_Unigram_Hyperspace_2.input

rm -f i_test.css 
rm -f q_test.css
.\crm114 "-{window; output /\n**** Vector 3-word-bag Hyperspace classifier \n/}"
rem #    the "vector: blahblah" is coded by the desired length of the pipeline,
rem #    then the number of iterations of the pipe, then pipelen * iters 
rem #    integer coefficients.  Missing coefficients are taken as zero, 
rem #    extra coefficients are disregarded.
.\crm114 "-{learn < hyperspace > (q_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1 / }" < QUICKREF_mt_ng_reference_1.input
.\crm114 "-{learn < hyperspace > (i_test.css) /[[:graph:]]+/ /vector: 3 1 1 1 1/}" < INTRO_mt_ng_reference_2.input
.\crm114 "-{ isolate (:s:); {classify < hyperspace > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ /vector: 3 1 1 1 1  /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Vector_3_word_bag_Hyperspace_1.input
.\crm114 "-{ isolate (:s:); {classify <hyperspace > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ /vector: 3 1 1 1 1 /; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Vector_3_word_bag_Hyperspace_2.input


del i_test.css 
del q_test.css
echo === TEST 445 ===
.\crm114 "-{window; output /\n**** Bit-Entropy classifier \n/}"
.\crm114 "-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 448 ===
.\crm114 "-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 450 ===
.\crm114 "-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Bit_Entropy_1.input
echo === TEST 452 ===
.\crm114 "-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Bit_Entropy_2.input

del i_test.css 
del q_test.css
echo === TEST 457 ===
.\crm114 "-{window; output /\n**** Bit-Entropy Toroid classifier \n/}"
.\crm114 "-{learn < entropy > (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 460 ===
.\crm114 "-{learn < entropy > (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 462 ===
.\crm114 "-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Bit_Entropy_Toroid_1.input
echo === TEST 464 ===
.\crm114 "-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Bit_Entropy_Toroid_2.input

del i_test.css 
del q_test.css
echo === TEST 469 ===
.\crm114 "-{window; output /\n**** Fast Substring Compression Match Classifier \n/}"
.\crm114 -s 200000 "-{learn < fscm > (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 472 ===
.\crm114 -s 200000 "-{learn < fscm > (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 474 ===
.\crm114 "-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Fast_Substring_Compression_Match_1.input
echo === TEST 476 ===
.\crm114 "-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Fast_Substring_Compression_Match_2.input

del i_test.css 
del q_test.css
echo === TEST 481 ===
.\crm114 "-{window; output /\n**** Neural Network Classifier \n/}"
.\crm114 -s 32768 "-{learn < neural append > (q_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input
echo === TEST 484 ===
.\crm114 -s 32768 "-{learn < neural append > (i_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 486 ===
.\crm114 "-{learn < neural refute fromstart > (q_test.css) /[[:graph:]]+/}" < INTRO_mt_ng_reference_2.input
echo === TEST 488 ===
.\crm114 "-{learn < neural refute fromstart > (i_test.css) /[[:graph:]]+/}" < QUICKREF_mt_ng_reference_1.input

echo === TEST 491 ===
.\crm114 "-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Neural_Network_1.input
echo === TEST 505 ===
.\crm114 "-{ isolate (:s:); {classify < neural > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Neural_Network_2.input



del i_test.css 
del q_test.css

echo === TEST 560 ===
.\crm114 "-{window; output /\n**** Alternate Neural Network Classifier test script \n/}"
.\crm114 alternating_example_neural.crm

del i_vs_q_test.css
del i_test.css 
del q_test.css
echo === TEST 571 ===
.\crm114 "-{window; output /\n**** Support Vector Machine (SVM) unigram classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (i_test.css) /[[:graph:]]+/; liaf}" < INTRO_mt_ng_reference_2.input
echo === TEST 574 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (q_test.css) /[[:graph:]]+/; liaf }" < QUICKREF_mt_ng_reference_1.input
echo === TEST 576 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css| i_vs_q_test.css ) < svm unigram unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }"

echo === TEST 580 ===
.\crm114 "-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Support_Vector_Machine_SVM_unigram_1.input

echo === TEST 583 ===
.\crm114 "-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Support_Vector_Machine_SVM_unigram_2.input

del i_vs_q_test.css
del i_test.css 
del q_test.css


echo === TEST 591 ===
.\crm114 "-{window; output /\n**** Support Vector Machine (SVM) classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (i_test.css) /[[:graph:]]+/; liaf}" < INTRO_mt_ng_reference_2.input
echo === TEST 594 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (q_test.css) /[[:graph:]]+/; liaf }" < QUICKREF_mt_ng_reference_1.input
echo === TEST 596 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }"

echo === TEST 600 ===
.\crm114 "-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_Support_Vector_Machine_SVM_1.input

echo === TEST 603 ===
.\crm114 "-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Support_Vector_Machine_SVM_2.input

del i_vs_q_test.css
del i_test.css 
del q_test.css

echo === TEST 610 ===
.\crm114 "-{window; output /\n**** String Kernel SVM (SKS) classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (i_test.css) /[[:graph:]]+/; liaf}" < INTRO_mt_ng_reference_2.input
echo === TEST 613 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (q_test.css) /[[:graph:]]+/; liaf }" < QUICKREF_mt_ng_reference_1.input
echo === TEST 615 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks > /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ }"
echo === TEST 618 ===
.\crm114 "-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_String_Kernel_SVM_SKS_1.input
echo === TEST 620 ===
.\crm114 "-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_String_Kernel_SVM_SKS_2.input

del i_vs_q_test.css
del i_test.css 
del q_test.css

echo === TEST 626 ===
.\crm114 "-{window; output /\n**** String Kernel SVM (SKS) Unique classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%%^&*()/; learn [:one_paragraph:] < sks unique > (i_test.css) /[[:graph:]]+/ / 0 0 100 0.001 1 0.5 1 4/; liaf}" < INTRO_mt_ng_reference_2.input
echo === TEST 629 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/;  translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%%^&*()/; learn [:one_paragraph:] < sks unique > (q_test.css) /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ ; liaf }" < QUICKREF_mt_ng_reference_1.input
echo === TEST 631 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks unique > /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ }"
echo === TEST 634 ===
.\crm114 "-{ isolate (:s:);  translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" < mt_ng_String_Kernel_SVM_SKS_Unique_1.input
echo === TEST 636 ===
.\crm114 "-{ isolate (:s:); translate /.,!?@#$%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_String_Kernel_SVM_SKS_Unique_2.input

del i_vs_q_test.css
del i_test.css 
del q_test.css


echo === TEST 644 ===
.\crm114 "-{window ; output /\n**** Bytewise Correlation classifier \n/}"
.\crm114 "-{ isolate (:s:) {classify <correlate> ( INTRO_mt_ng_reference_2.input | QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Bytewise_Correlation_1.input

echo === TEST 648 ===
.\crm114 "-{ isolate (:s:) {classify <correlate> ( INTRO_mt_ng_reference_2.input | QUICKREF_mt_ng_reference_1.input ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" < mt_ng_Bytewise_Correlation_2.input

del i_test.css 
del q_test.css

echo === TEST 661 ===
.\crm114 "-{window; output /\n**** Clump \/ Pmulc Test \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump <bychunk> [:one_paragraph:] (i_test.css) /[[:graph:]]+/; output /./ ; liaf}" < INTRO_mt_ng_reference_2.input
echo === TEST 664 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /([[:graph:]]+.*?\n\n){5}/; clump [:one_paragraph:] <bychunk> (i_test.css) /[[:graph:]]+/; output /./; liaf }" < QUICKREF_mt_ng_reference_1.input

rem    Now see where our paragraphs go to
echo === TEST 668 ===
.\crm114 "-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }" < mt_ng_Clump_Pmulc_1.input
echo === TEST 682 ===
.\crm114 "-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }" < mt_ng_Clump_Pmulc_2.input

echo === TEST 695 ===
.\crm114 "-{ isolate (:s:); { pmulc  ( i_test.css) (:s:) <bychunk> /[[:graph:]]+/  [:_dw:]   ; output /Likely result: \n:*:s:\n/} alius { output / Unsure result \n:*:s:\n/ } }" < mt_ng_Clump_Pmulc_3.input




del i_test.css 
del q_test.css


