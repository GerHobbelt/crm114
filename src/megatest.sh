./crm114 -v 2>&1
./crm114 bracktest.crm 
./crm114 escapetest.crm 
./crm114 fataltraptest.crm 
./crm114 inserttest_a.crm
./crm114 matchtest.crm  <<-EOF
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
./crm114 backwardstest.crm  <<-EOF
foo bar baz
EOF
./crm114 backwardstest.crm  <<-EOF
bar foo baz
EOF
./crm114 overalterisolatedtest.crm 
./crm114 rewritetest.crm 
./crm114 skudtest.crm 
./crm114 statustest.crm 
./crm114 unionintersecttest.crm 
./crm114 beeptest.crm 
./crm114 defaulttest.crm
./crm114 defaulttest.crm --blah="command override"
./crm114 windowtest.crm  <<-EOF
	This is the test one result A this is the test two result A this is the test three result A this is the test four result A this is the test 5 result A this is the test six result A this is extra stuff and should never be seen.
EOF
./crm114 windowtest_fromvar.crm  <<-EOF
	This is the test one result A this is the test two result A this is the test three result A this is the test four result A this is the test 5 result A this is the test six result A this is extra stuff and should trigger exit from the loop since it doesn't have the proper delimiter.
EOF
./crm114 approxtest.crm  <<-EOF
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
./crm114 '-{window; output / \n***** checking return and exit codes \n/}'
./crm114 '-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: \n/}'
./crm114 '-{window; output /\n***** check that failed syscalls will code right\n/}'
./crm114 '-{window; isolate (:s:); syscall () () (:s:) /jibberjabber 2>&1 /; output / Status: :*:s: \n/}'

./crm114 indirecttest.crm
rm -f randtst.txt
rm -f i_test.css
rm -f q_test.css

./crm114 '-{window; output /\n ****  Default (SBPH Markovian) classifier \n/}'
./crm114 '-{learn (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** OSB Markovian classifier \n/}'
./crm114 '-{learn <osb> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn <osb> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <osb> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF


rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** OSB Markov Unique classifier \n/}'
./crm114 '-{learn <osb unique > (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn <osb unique > (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** OSB Markov Chisquared Unique classifier \n/}'
./crm114 '-{learn <osb unique chi2> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn <osb unique chi2> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify <osb unique chi2 > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <osb unique chi2> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** OSBF Local Confidence (Fidelis) classifier \n/}'
./crm114 '-{learn < osbf > (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn < osbf > (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output / \n**** OSB Winnow classifier \n/}'
./crm114 '-{learn <winnow> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt 
./crm114 '-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{learn <winnow> (i_test.css) /[[:graph:]]+/}' < INTRO.txt 
./crm114 '-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}' < QUICKREF.txt 
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
./crm114 '-{ window; output /\n\n**** Now verify that winnow learns affect only the named file (i_test.css)\n/}'
./crm114 '-{learn <winnow> (i_test.css) /[[:graph:]]+/}' < COLOPHON.txt 
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
./crm114 '-{window; output /\n\n and now refute-learn into q_test.css\n/}'
./crm114 '-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}' < FAQ.txt 
./crm114 '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** Unigram Bayesian classifier \n/}'
./crm114 '-{learn <unigram> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn <unigram> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output / \n**** unigram Winnow classifier \n/}'
./crm114 '-{learn <winnow unigram > (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt 
./crm114 '-{learn <winnow unigram refute> (q_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{learn <winnow unigram> (i_test.css) /[[:graph:]]+/}' < INTRO.txt 
./crm114 '-{learn <winnow unigram refute> (i_test.css) /[[:graph:]]+/}' < QUICKREF.txt 
./crm114 '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <winnow unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** OSB Hyperspace classifier \n/}'
./crm114 '-{learn <hyperspace> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn <hyperspace> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <hyperspace> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** Unigram Hyperspace classifier \n/}'
./crm114 '-{learn < hyperspace unigram> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn < hyperspace unigram> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify < hyperspace unigram> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <hyperspace unigram> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF


rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** Bit-Entropy classifier \n/}'
./crm114 '-{learn < entropy unique crosslink> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn < entropy unique crosslink> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify < entropy unique crosslink> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify <entropy unique crosslink> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF

rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window; output /\n**** Bit-Entropy Toroid classifier \n/}'
./crm114 '-{learn < entropy > (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
./crm114 '-{learn < entropy > (i_test.css) /[[:graph:]]+/}' < INTRO.txt
./crm114 '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:); {classify < entropy > ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF


rm -f i_test.css 
rm -f q_test.css
./crm114 '-{window ; output /\n**** Bytewise Correlation classifier \n/}'
./crm114 '-{ isolate (:s:) {classify <correlate> ( INTRO.txt | QUICKREF.txt ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
./crm114 '-{ isolate (:s:) {classify <correlate> ( INTRO.txt | QUICKREF.txt ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
CRM114 is a language designed to write filters in.  It caters to
filtering email, system log streams, html, and other marginally
human-readable ASCII that may occasion to grace your computer.
EOF
rm -f i_test.css 
rm -f q_test.css

