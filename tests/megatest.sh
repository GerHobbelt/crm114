#!/bin/sh
#
# $Id: megatest.sh,v 1.3 2004/12/31 22:27:24 oopla Exp $
# (C) like original - added stuff as 'Public Domain'
# original megatest.sh, but handles options and can set which crm* to use
# and which test to run on cmdline
# 
PROG=${0##*/}

# defaults in autoconfiscated
t=
T=0			# list of tests - 0=all
crm=./src/crm114	# binary under test
d=./tests		# testes .crm directory

while [ "$1" ];do
  case $1 in
    -h*|--h*) 
      echo "Usage: $PROG [crm114-VERSION path-to-tests "
      echo "                   [\"test1 test2 ...\" [crm args]]]"
      echo "       Defaults: ./src/crm114 ./tests test=0=all"
      echo "       Known tests:"
      grep "[ ]*[^ ]\+[|]0).\+" < $0 | \
        sed "s,[ ]*\([^ ]\+\)[|]0).*/\([^ ]\+\) .*,\1	- \2,"
      #for t in $all;do
      #  p=`grep "[ ]*$t[|]0).*" < $0|sed "s/^ *//"|tr -s " "|cut -d" " -f4`
      #  p=${p##*/}
      #  p=${p%% *}
      #  echo "-      $t = $p"
      #done
      exit 0
      ;;
    -c*) shift; crm=$1 ;;
    -d*) shift; d=$1 ;;
    --) shift; break ;;	# only crm args in $@ now
    -*) echo "$PROG: unknown option: $1"; exit 1;;
    *) T="$T $1" ;;
  esac
  shift
done

all="b e f m k o r s t u p d w wf ax ma mr el x rio sbp osb cor"

CRM=`which $crm`
if [ "$CRM" ]; then
  echo "Using $CRM :"
  crm=$CRM
  $crm -v
  sleep 2
else
  echo -ne "\a\nCan't find $crm, or it is not executable. [?=help]: "
  read r 
  [ "$r" = "?" ] || exit 1
  exec $0 -h
fi

for t in $T;do
case "$t" in 
  b|0) $crm $@ $d/bracktest.crm ;;
  e|0) $crm $@ $d/escapetest.crm ;;
  f|0) $crm $@ $d/fataltraptest.crm ;;
  m|0) $crm $@ $d/matchtest.crm  <<-EOF
	exact: you should see this foo ZZZ
	exact: you should NOT see this FoO ZZZ
	absent: There is no "f-word" here ZZZ
	absent: but there's a foo here ZZZ'
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
   ;;
  k|0) $crm $@ $d/backwardstest.crm  <<-EOF
foo bar baz
EOF
       $crm $@ $d/backwardstest.crm  <<-EOF
bar foo baz
EOF
   ;;
  o|0) $crm $@ $d/overalterisolatedtest.crm ;;
  r|0) $crm $@ $d/rewritetest.crm ;;
  s|0) $crm $@ $d/skudtest.crm ;;
  t|0) $crm $@ $d/statustest.crm ;;
  u|0) $crm $@ $d/unionintersecttest.crm ;;
  p|0) $crm $@ $d/beeptest.crm ;;
  d|0) $crm $@ $d/userdirtest.crm ;;
  w|0) $crm $@ $d/windowtest.crm  <<-EOF
	This is the test one result A this is the test two result A this is the test three result A this is the test four result A this is the test 5 result A this is the test six result A this is extra stuff and should never be seen.
EOF
   ;;
  wf|0) $crm $@ $d/windowtest_fromvar.crm  <<-EOF
	This is the test one result A this is the test two result A this is the test three result A this is the test four result A this is the test 5 result A this is the test six result A this is extra stuff and should trigger exit from the loop since it doesn't have the proper delimiter.
EOF
#'
   ;;
  ax|0) $crm $@ $d/approxtest.crm  <<-EOF
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
EOF
   ;;
  ma|0) $crm $@ $d/mathalgtest.crm  ;;
  mr|0) $crm $@ $d/mathrpntest.crm -q 1 ;;
  el|0) $crm $@ $d/eval_infiniteloop.crm ;;
   x|0) $crm $@ $d/exectest.crm ;;
  po|0) $crm $@ $d/paolo_overvars.crm ;;
  mis|0) $crm $@ $d/match_isolate_test.crm -e ;;
  crt|0) $crm $@ $d/call_return_test.crm ;;
  rio|0) $crm $@ $d/randomiotest.crm #
         rm -f randtst.txt
	 ;;
	 # wierd format for the sed(1) in usage()
  cre|0) #/check_return_and_exit_codes .
         $crm $@ '-{window; output / \n***** checking return and exit codes \n/}'
         $crm $@ '-{window; isolate (:s:); syscall () () (:s:) /exit 123/; output / Status: :*:s: \n/}'
	 ;;
  sbp|0) #/learn-classify_by_SBPH .
         rm -f i_test.css q_test.css
         $crm $@ '-{window; output /\n ****  Default (SBPH Markovian) classifier \n/}'
         $crm $@ '-{learn (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
         $crm $@ '-{learn (i_test.css) /[[:graph:]]+/}' < INTRO.txt
         $crm $@ '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
         $crm $@ '-{ isolate (:s:) {classify ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         rm -f i_test.css q_test.css
	 ;;
  osb|0) #/learn-classify_by_OSB .
         rm -f i_test.css q_test.css
         $crm $@ '-{window; output /\n**** OSB Markovian classifier \n/}'
         $crm $@ '-{learn <osb> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
         $crm $@ '-{learn <osb> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
         $crm $@ '-{isolate (:s:); output /OSB: / {classify <osb> ( i_test.css | q_test.css ) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
         $crm $@ '-{isolate (:s:); output /OSB: / {classify <osb> ( i_test.css | q_test.css ) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         rm -f i_test.css q_test.css
	 ;;
  osbu|0) #/learn-classify_by_OSB-Unique .
         rm -f i_test.css q_test.css
         $crm $@ '-{window; output /\n**** OSB Markov Unique classifier \n/}'
         $crm $@ '-{learn <osb unique> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
         $crm $@ '-{learn <osb unique> (i_test.css) /[[:graph:]]+/}' < INTRO.txt
         $crm $@ '-{isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
to do basic mathematics and inequaxlity testing, either only in EVALs
EOF
         $crm $@ '-{isolate (:s:); {classify <osb unique> ( i_test.css | q_test.css ) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         rm -f i_test.css q_test.css
	 ;;
  osbf|0) #/classify_by_OSBF .
         $crm $@ '-{window; output /\n**** OSBF Local Confidence (Fidelis) classifier \n/}'
         $crm $@ '-{learn < osbf > (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt
         $crm $@ '-{learn < osbf > (i_test.css) /[[:graph:]]+/}' < INTRO.txt
         $crm $@ '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
         $crm $@ '-{ isolate (:s:); {classify <osbf> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         rm -f i_test.css q_test.css
         ;;
  osbw|0) #/classify_by_OSB-Winnow .
         $crm $@ '-{window; output / \n**** OSB Winnow classifier \n/}'
         $crm $@ '-{learn <winnow> (q_test.css) /[[:graph:]]+/}' < QUICKREF.txt 
         $crm $@ '-{learn <winnow refute> (q_test.css) /[[:graph:]]+/}' < INTRO.txt
         $crm $@ '-{learn <winnow> (i_test.css) /[[:graph:]]+/}' < INTRO.txt 
         $crm $@ '-{learn <winnow refute> (i_test.css) /[[:graph:]]+/}' < QUICKREF.txt 
         $crm $@ '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:)/[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ } }       '  <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
         $crm $@ '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         $crm $@ '-{ window; output /\n\n**** Now verify that winnow learns affect only the named file (i_test.css)\n/}'
         $crm $@ '-{learn <winnow> (i_test.css) /[[:graph:]]+/}' < COLOPHON.txt 
         $crm $@ '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         $crm $@ '-{window; output /\n\n**** and now refute-learn into q_test.css\n/}'
         $crm $@ '-{learn <winnow refute > (q_test.css) /[[:graph:]]+/}' < FAQ.txt 
         $crm $@ '-{ isolate (:s:); {classify <winnow> ( i_test.css | q_test.css ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}      ' <<-EOF
But fear not, we _do_ have the document you want. 
EOF
         rm -f i_test.css q_test.css
         ;;
  cor|0) #/classify_by_CORRELATE .
         $crm $@ '-{window ; output /\n**** Bytewise Correlation classifier \n/}'
         $crm $@ '-{ isolate (:s:) {classify <correlate> ( INTRO.txt | QUICKREF.txt ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
to do basic mathematics and inequality testing, either only in EVALs
EOF
         $crm $@ '-{ isolate (:s:) {classify <correlate> ( INTRO.txt | QUICKREF.txt ) (:s:) /[[:graph:]]+/ ; output / type I \n:*:s:\n/} alius { output / type Q \n:*:s:\n/ }}' <<-EOF
CRM114 is a language designed to write filters in.  It caters to
filtering email, system log streams, html, and other marginally
human-readable ASCII that may occasion to grace your computer.
EOF
         rm -f i_test.css q_test.css
         ;;
  *) echo -e "\nDon't know this test: $t"
     echo " available tests are: $all"
     echo " try $0 -h"
     ;;
esac
done
