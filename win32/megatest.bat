@echo off

echo === TEST 003 ===
.\crm114 -v 2>&1
echo === TEST 005 ===
.\crm114 bracktest.crm 
echo === TEST 007 ===
.\crm114 escapetest.crm 
echo === TEST 009 ===
.\crm114 fataltraptest.crm 
echo === TEST 011 ===
.\crm114 inserttest_a.crm

echo === TEST 014 ===
rem
rem grep arg -Ann: nn = find /N linenumber minus 4
rem the trick here is to have grep find the text; then strip the marker using MSDOS 'find /V'
rem and redirect the result to the stdin of crm114
rem
grep -A34 -e ".\crm114 matchtest.crm  -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 matchtest.crm  -EOF" | .\crm114 matchtest.crm  
goto L54      ".\crm114 matchtest.crm  -EOF" 
.\crm114 matchtest.crm  -EOF
	exact: you should see this foo ZZZ
	exact: you should NOT see this FoO ZZZ
	absent: There is no "f-word" here ZZZ
	absent: but there's a foo here ZZZ
	nocase: you should see this fOo ZZZ
	nocase: and there is no "f-word" here ZZZ
	nocase absent: and there is no "f-word" here ZZZ
	nocase absent: and there is a foo here ZZZ
	multiline:  this is a multiline test of foo
	multiline-- should see both lines ZZZ
	multiline:  this is a multiline test of "f-word"
	multiline-- should see both lines ZZZ
	nomultiline:  this is a nomultiline test of foo
	nomultiline-- should NOT see both lines ZZZ
	nomultiline:  this is a nomultiline test of "f-word"
	nomultiline-- should NOT see both lines ZZZ
	fromendchar:  should see this line foo followed by bar ZZZ
	fromendchar:  should NOT see this line bar followed by foo ZZZ
	fromnext: should not see foo but should see ffoooo ZZZ
	fromnext: should not see foo but should see f-oooo ZZZ
	newend: should not see foo but should see ffooo ZZZ
	newend: should not see foo and should not see f-ooo ZZZ
	indirect go to :twist:  ZZZ
	indirect go to :shout2:  ZZZ
	self-supplied: foo123bar 
	wugga
	smith 123 anderson ZZZ
	self-supplied: foo123bar 
	wugga
	smith 456 anderson ZZZ
	independent-start-end: foo bar 1 2 foo bar ZZZ
	independent-start-end: foo bar foo 1 2 bar ZZZ 
	independent-start-end: foo 1 foo bar 2 bar ZZZ
	independent-start-end: foo 2 bar 1 bar foo ZZZ
EOF



:L54
echo === TEST 062 ===
grep -A1 -e ".\crm114 backwardstest.crm  -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 backwardstest.crm  -EOF" | .\crm114 backwardstest.crm  
goto L60      ".\crm114 backwardstest.crm  -EOF" 
.\crm114 backwardstest.crm  -EOF
foo bar baz
EOF
:L60
echo === TEST 069 ===
grep -A1 -e ".\crm114 backwardstest.crm   -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 backwardstest.crm   -EOF" | .\crm114 backwardstest.crm   
goto L66      ".\crm114 backwardstest.crm   -EOF" 
.\crm114 backwardstest.crm   -EOF
bar foo baz
EOF
:L66
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
grep -A1 -e ".\crm114 windowtest.crm  -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 windowtest.crm  -EOF" | .\crm114 windowtest.crm  
goto L81      ".\crm114 windowtest.crm  -EOF" 
.\crm114 windowtest.crm  -EOF
	This is the test one result A this is the test two result A this is the test three result A this is the test four result A this is the test 5 result A this is the test six result A this is extra stuff and should never be seen.
EOF
:L81
echo === TEST 100 ===
grep -A1 -e ".\crm114 windowtest_fromvar.crm  -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 windowtest_fromvar.crm  -EOF" | .\crm114 windowtest_fromvar.crm  
goto L87      ".\crm114 windowtest_fromvar.crm  -EOF" 
.\crm114 windowtest_fromvar.crm  -EOF
	This is the test one result A this is the test two result A this is the test three result A this is the test four result A this is the test 5 result A this is the test six result A this is extra stuff and should trigger exit from the loop since it doesn't have the proper delimiter.
EOF
:L87
echo === TEST 107 ===
grep -A144 -e ".\crm114 approxtest.crm  -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 approxtest.crm  -EOF" | .\crm114 approxtest.crm  
goto L236      ".\crm114 approxtest.crm  -EOF" 
.\crm114 approxtest.crm  -EOF
(foo) {1}
(fou){~}
(foo) {1}
(fou){~0}
(foo) {1}
(fou){~1}
(foo) {1}
(fou){~2}
(fou){~3}
(fuu){~}
(fuu){~0}
(fuu){~1}
(fuu){~2}
(fuu){~3}
(fou){#}
(fou){#0}
(fou){#1}
(fou){#2}
(fou){#3}
(fou){# ~1}
(fou){#0 ~1}
(fou){#1 ~1}
(fou){#2 ~1}
(fou){#3 ~1}
(fuu){#}
(fuu){#0}
(fuu){#1}
(fuu){#2}
(fuu){#3}
(fuu){# ~1}
(fuu){#0 ~1}
(fuu){#1 ~1}
(fuu){#2 ~1}
(fuu){#3 ~1}
(fuu){# ~2}
(fuu){#0 ~2}
(fuu){#1 ~2}
(fuu){#2 ~2}
(fuu){#3 ~2}
(fuu){# ~3}
(fuu){#0 ~3}
(fuu){#1 ~3}
(fuu){#2 ~3}
(fuu){#3 ~3}
(fuu){# ~}
(fuu){#0 ~}
(fuu){#1 ~}
(fuu){#2 ~}
(fuu){#3 ~}
(fou){#}
(fou){#0}
(fou){+1 -1}
(fou){+2 -2}
(fou){+3 -3}
(fou){# ~1}
(fou){#0 ~1}
(fou){+1 -1 ~1}
(fou){+2 -2 ~1}
(fou){+3 -3 ~1}
(fou){# ~2}
(fou){#0 ~2}
(fou){+1 -1 ~2}
(fou){+2 -2 ~2}
(fou){+3 -3 ~2}
(fou){# ~3}
(fou){#0 ~3}
(fou){+1 -1 ~3}
(fou){+2 -2 ~3}
(fou){+3 -3 ~3}
(fou){# ~}
(fou){#0 ~}
(fou){+1 -1 ~}
(fou){+2 -2 ~}
(fou){+3 -3 ~}
(fuu){#}
(fuu){#0}
(fuu){+1 -1}
(fuu){+2 -2}
(fuu){+3 -3}
(fuu){# ~1}
(fuu){#0 ~1}
(fuu){+1 -1 ~1}
(fuu){+2 -2 ~1}
(fuu){+3 -3 ~1}
(fuu){# ~2}
(fuu){#0 ~2}
(fuu){+1 -1 ~2}
(fuu){+2 -2 ~2}
(fuu){+3 -3 ~2}
(fuu){# ~3}
(fuu){#0 ~3}
(fuu){+1 -1 ~3}
(fuu){+2 -2 ~3}
(fuu){+3 -3 ~3}
(fuu){# ~}
(fuu){#0 ~}
(fuu){+1 -1 ~}
(fuu){+2 -2 ~}
(fuu){+3 -3 ~}
(anaconda){~}
(anaonda){ 1i + 1d < 1 }
(anaonda){ 1i + 1d < 2 }
(ananda){ 1i + 1d < 1 }
(ananda){ 1i + 1d < 2 }
(ananda){ 1i + 1d < 3 }
(ana123conda){ 1i + 1d < 2 }
(ana123conda){ 1i + 1d < 3 }
(ana123conda){ 1i + 1d < 4 }
(ana123cona){ 1i + 1d < 4 }
(ana123cona){ 1i + 1d < 5 }
(ana123coa){ 1i + 1d < 4 }
(ana123coa){ 1i + 1d < 5 }
(ana123coa){ 1i + 1d < 6 }
(ana123ca){ 1i + 1d < 4 }
(ana123a){ 1i + 1d < 4 }
(ana123a){ 1i + 1d < 3 }
(anukeonda){~}
(anaconda){ 1i + 1d < 1}
(anaconda){ 1i + 1d < 1, #1}
(anaconda){ 1i + 1d < 1 #1 ~10 }
(anaconda){ #1, ~1, 1i + 1d < 1 }
(anaconda){ #1 ~1 1i + 1d < 1 }
(anacnda){ #1 ~1 1i + 1d < 1 }
(agentsmith){~}
(annndersen){~}
(anentemstn){~}
(anacda){~}
(anacda){ #1 ~1 1i + 1d < 1 }
(znacnda){ #1 ~1 1i + 1d < 1 }
(znacnda){ #1 ~2 1i + 1d < 1 }
(znacnda){ #1 ~3 1i + 1d < 1 }
(znacnda){ #1 ~3 1i + 1d < 2 }
(anac){~1}(onda){~1}
(aac){~1}(onda){~1}
(ac){~1}(onda){~1}
(anac){~1}(oda){~1}
(aac){~1}(oa){~1}
(ac){~1}(oa){~1}
(anac){~1}(onda){~1}
(anZac){~1}(onZda){~1}
(anZZac){~1}(onZda){~1}
(anZac){~1}(onZZda){~1}
([a-n]){3,100}
([a-n]){3,100}?
EOF
:L236

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
.\crm114 "-{learn (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 307 ===
.\crm114 "-{learn (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 309 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 
echo === TEST 311 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}"

del i_test.css 
del q_test.css
echo === TEST 316 ===
.\crm114 "-{window; output /\n**** OSB Markovian classifier \n/}"
.\crm114 "-{learn <osb> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 319 ===
.\crm114 "-{learn <osb> (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 321 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }"
echo === TEST 323 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}"


del i_test.css 
del q_test.css
echo === TEST 329 ===
.\crm114 "-{window; output /\n**** OSB Markov Unique classifier \n/}"
.\crm114 "-{learn <osb unique > (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 332 ===
.\crm114 "-{learn <osb unique > (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 334 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }"
echo === TEST 336 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}"

del i_test.css 
del q_test.css
echo === TEST 341 ===
.\crm114 "-{window; output /\n**** OSB Markov Chisquared Unique classifier \n/}"
.\crm114 "-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 344 ===
.\crm114 "-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 346 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }"
echo === TEST 348 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_test.css 
del q_test.css
echo === TEST 353 ===
.\crm114 "-{window; output /\n**** OSBF Local Confidence (Fidelis) classifier \n/}"
.\crm114 "-{learn < osbf > (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 356 ===
.\crm114 "-{learn < osbf > (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 358 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 360 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}"

del i_test.css 
del q_test.css
echo === TEST 365 ===
.\crm114 "-{window; output / \n**** OSB Winnow classifier \n/}"
.\crm114 "-{learn <winnow> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt 
echo === TEST 368 ===
.\crm114 "-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 370 ===
.\crm114 "-{learn <winnow> (i_test.css) /[[:graph:]]+/}" < INTRO.txt 
echo === TEST 372 ===
.\crm114 "-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}" < QUICKREF.txt 
echo === TEST 374 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       "  
echo === TEST 376 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " 
echo === TEST 378 ===
.\crm114 "-{ window; output /\n\n**** Now verify that winnow learns affect only the named file (i_test.css)\n/}"
.\crm114 "-{learn <winnow> (i_test.css) /[[:graph:]]+/}" < COLOPHON.txt 
echo === TEST 381 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " 
echo === TEST 383 ===
.\crm114 "-{window; output /\n\n and now refute-learn into q_test.css\n/}"
.\crm114 "-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}" < FAQ.txt 
echo === TEST 386 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " 

del i_test.css 
del q_test.css
echo === TEST 391 ===
.\crm114 "-{window; output /\n**** Unigram Bayesian classifier \n/}"
.\crm114 "-{learn <unigram> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 394 ===
.\crm114 "-{learn <unigram> (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 396 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 398 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}"

del i_test.css 
del q_test.css
echo === TEST 403 ===
.\crm114 "-{window; output / \n**** unigram Winnow classifier \n/}"
.\crm114 "-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt 
echo === TEST 406 ===
.\crm114 "-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 408 ===
.\crm114 "-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}" < INTRO.txt 
echo === TEST 410 ===
.\crm114 "-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}" < QUICKREF.txt 
echo === TEST 412 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       "  
echo === TEST 414 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      " 

del i_test.css 
del q_test.css
echo === TEST 419 ===
.\crm114 "-{window; output /\n**** OSB Hyperspace classifier \n/}"
echo === TEST 421 ===
.\crm114 "-{learn <hyperspace unique> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 423 ===
.\crm114 "-{learn <hyperspace unique> (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 425 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 427 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_test.css 
del q_test.css
echo === TEST 432 ===
.\crm114 "-{window; output /\n**** Unigram Hyperspace classifier \n/}"
.\crm114 "-{learn < hyperspace unique unigram> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 435 ===
.\crm114 "-{learn < hyperspace unique unigram> (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 437 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 439 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 


del i_test.css 
del q_test.css
echo === TEST 445 ===
.\crm114 "-{window; output /\n**** Bit-Entropy classifier \n/}"
.\crm114 "-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 448 ===
.\crm114 "-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 450 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 452 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_test.css 
del q_test.css
echo === TEST 457 ===
.\crm114 "-{window; output /\n**** Bit-Entropy Toroid classifier \n/}"
.\crm114 "-{learn < entropy > (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 460 ===
.\crm114 "-{learn < entropy > (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 462 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 464 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_test.css 
del q_test.css
echo === TEST 465 ===
.\crm114 "-{window; output /\n**** Fast Substring Compression Match Classifier \n/}"
.\crm114 "-{learn < fscm > (q_test.css) /[[:graph:]]+/}" < QUICKREF.txt
echo === TEST 466 ===
.\crm114 "-{learn < fscm > (i_test.css) /[[:graph:]]+/}" < INTRO.txt
echo === TEST 467 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 468 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify < fscm > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_vs_q_test.css
del i_test.css 
del q_test.css
echo === TEST 470 ===
.\crm114 "-{window; output /\n**** Support Vector Machine (SVM) unigram classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (i_test.css) /[[:graph:]]+/; liaf}" < INTRO.txt
echo === TEST 473 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unigram unique > (q_test.css) /[[:graph:]]+/; liaf }" < QUICKREF.txt
echo === TEST 475 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css| i_vs_q_test.css ) < svm unigram unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }"

echo === TEST 479 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 

echo === TEST 482 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify < svm unigram unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_vs_q_test.css
del i_test.css 
del q_test.css


echo === TEST 490 ===
.\crm114 "-{window; output /\n**** Support Vector Machine (SVM) classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (i_test.css) /[[:graph:]]+/; liaf}" < INTRO.txt
echo === TEST 493 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < svm unique > (q_test.css) /[[:graph:]]+/; liaf }" < QUICKREF.txt
echo === TEST 495 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < svm unique > /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ }"

echo === TEST 499 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 

echo === TEST 502 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify < svm unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 1e-3 1 0.5 1 1/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_vs_q_test.css
del i_test.css 
del q_test.css

echo === TEST 503 ===
.\crm114 "-{window; output /\n**** String Kernel SVM (SKS) classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (i_test.css) /[[:graph:]]+/; liaf}" < INTRO.txt
echo === TEST 504 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; learn [:one_paragraph:] < sks > (q_test.css) /[[:graph:]]+/; liaf }" < QUICKREF.txt
echo === TEST 505 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks > /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ }"
echo === TEST 506 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 507 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); {classify < sks > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 
del i_vs_q_test.css
del i_test.css 
del q_test.css

echo === TEST 508 ===
.\crm114 "-{window; output /\n**** String Kernel SVM (SKS) Unique classifier \n/}"
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/; translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%%^&*()/; learn [:one_paragraph:] < sks unique > (i_test.css) /[[:graph:]]+/ / 0 0 100 0.001 1 0.5 1 4/; liaf}" < INTRO.txt
echo === TEST 509 ===
.\crm114 "-{ match <fromend> (:one_paragraph:) /[[:graph:]]+.*?\n\n/;  translate [:one_paragraph:] (:one_paragraph:) /.,!?@#$%%^&*()/; learn [:one_paragraph:] < sks unique > (q_test.css) /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ ; liaf }" < QUICKREF.txt
echo === TEST 510 ===
rem    build the actual hyperplanes
.\crm114 "-{window; learn ( i_test.css | q_test.css | i_vs_q_test.css ) < sks unique > /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ }"
echo === TEST 511 ===
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:);  translate /.,!?@#$%%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ [:_dw:]   ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }" 
echo === TEST 512 ===
echo But fear not, we _do_ have the document you want. | .\crm114 "-{ isolate (:s:); translate /.,!?@#$%%^&*()/; {classify < sks unique > ( i_test.css | q_test.css | i_vs_q_test.css ) (:s:) /[[:graph:]]+/ /0 0 100 0.001 1 0.5 1 4/ [:_dw:] ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

del i_vs_q_test.css
del i_test.css 
del q_test.css


echo === TEST 520 ===
.\crm114 "-{window ; output /\n**** Bytewise Correlation classifier \n/}"
echo to do basic mathematics and inequality testing, either only in EVALs | .\crm114 "-{ isolate (:s:) {classify <correlate> ( INTRO.txt | QUICKREF.txt ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 

echo === TEST 514 ===
grep -A3 -e ".\crm114 test407  -EOF" %0 | %SystemRoot%\system32\find /V ".\crm114 test407  -EOF" | .\crm114 "-{ isolate (:s:) {classify <correlate> ( INTRO.txt | QUICKREF.txt ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}" 
goto L412      ".\crm114 test407  -EOF" 
.\crm114 test407  -EOF
CRM114 is a language designed to write filters in.  It caters to
filtering email, system log streams, html, and other marginally
human-readable ASCII that may occasion to grace your computer.
EOF
:L412

del i_test.css 
del q_test.css

:ende
