#!/bin/sh

#
# testscript <scriptfile> <inputfile> <expected-output-file> <diff-process-script> <extra-script-args>
#
# runs CRM114 script <scriptfile> 
# feeding it <inputfile> on stdin (if the parameter is non-empty)
# and <extra-script-args> on the commandline
#
# output is redirected to a temporary file, which is compared against
# <expected-output-file> using the <diff-process-script>.
#
# Of course, the latter usually is nothing more than a little shell script
# around 'diff -u', but when you test non-text or 'imprecise' test cases 
# (checking the pR result for a classify is a good example of the latter)
# you may want to perform other actions to compare the results with what's
# expected.
#
# return exit code 0 on success; any other return code for test failure
#

CRM_BIN="/windows/G/prj/3actual/crm114/src/crm114"
BUILDDIR=.
SRCDIR=.
REFDIR="${SRCDIR}/ref"

report_error()
{
  echo ""
  echo "Format:"
  echo "  ./testscript.sh <scriptfile> <inputfile> <expected-output-file> <diff-process-script> <extra-script-args>"
  echo ""
  echo "**ERROR**:"
  echo "  $1"
}

report_warning()
{
  echo "**WARNING**:"
  echo "  $1"
  echo ""
}

debug_msg()
{
  if [ ! -z "${CRM114_MAKE_SCRIPTS_DEBUG}" ]; then
    echo "  $1"
  fi
}

write2ref=0

scriptfile="$1"
inputdata="$2"
expected="$3"
diffscript="$4"

tmpout="$0.tempout"
tmperr="$0.temperr"
tmpchk="$0.tempchk"
diffout="$0.diffout"
filtout="$0.filtout"

literalscript=0
if [ ! -f "${scriptfile}" ]; then
  # echo "sed proc = " 
  # echo `echo "${scriptfile}" | sed -e "s/^-{.*} */-{/" `
  if [ "x`echo "${scriptfile}" | sed -e "s/^-{.*} */-{/" `" = "x-{" ]; then
    debug_msg "going to use '${scriptfile}' as literal test script"
    literalscript=1
  else
    report_error "crm114 test script '${scriptfile}' does not exist"
    exit 66
  fi
fi

# diffscript may be NIL, must otherwise exist as file
if [ ! -z "${diffscript}" ]; then
  if [ ! -f "${diffscript}" ]; then
    report_error "diff script '${diffscript}' does not exist"
    exit 65
  fi
fi

# expected output may be absent; if it is absent, a fresh file
# will be created.containing a copy of the test output.
# this is handy to generate the 'expected output' files to start off with...
if [ ! -z "${expected}" ]; then
  if [ "x${expected}" = "x-" ]; then
    if [ ${literalscript} = 0 ]; then
      expected="${REFDIR}/${scriptfile}.refoutput"
      debug_msg "going to use expected outfile file '${expected}'..."
    else
      report_error "expected outfile file '${expected}' does not exist and cannot be 'generated' from the script filename as you specified a literal script!"
      exit 64
    fi
  fi
  if [ ! -f "${expected}" ]; then
    report_warning "expected outfile file '${expected}' does not exist: script will create this file!"
    write2ref=1
  fi
else
  report_error "expected outfile file '${expected}' does not exist"
  exit 63
fi

shift
shift
shift
shift
args=$@

# hmmm... weird; using $@ everywhere instead of this args above will strip off any but the first arg.
# must be screwing up somewhere...
# Hm. Mebbee the @xyz@ expansions in here done by configure? :-S

rm -f "${tmperr}" 
rm -f "${tmpout}"
rm -f "${tmpchk}"
rm -f "${diffout}"
rm -f "${filtout}"

# see if inputfile is a nonempty string; run CRM test scenario accordingly

echo "test: CRM114 '${scriptfile}' ${args}"

debug_msg "going to run CRM114 '${scriptfile}' with commandline: '$1' '$2' '$3' '$4' '$5' '$6' '$7' '$8' '$9'"

retcode=65536
if [ -z "${inputdata}" ]; then
  debug_msg "input is nil/empty"
  debug_msg ": crm114 ${scriptfile} '$1' '$2' '$3' '$4' '$5' '$6' '$7' '$8' '$9'"
  ${CRM_BIN} "${scriptfile}" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" 2> "${tmperr}" > "${tmpout}"
  retcode=$?
else
  # see if file exists; if not, assume input to be literal text
  if [ -f "${inputdata}" ]; then
    # run test:
    debug_msg ": crm114 ${scriptfile} '$1' '$2' '$3' '$4' '$5' '$6' '$7' '$8' '$9'"
    ${CRM_BIN} "${scriptfile}" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" < "${inputdata}" 2> "${tmperr}" > "${tmpout}"
    retcode=$?
  else
    debug_msg ": echo '${inputdata}' | crm114 ${scriptfile} '$1' '$2' '$3' '$4' '$5' '$6' '$7' '$8' '$9'"
    echo "${inputdata}" | ${CRM_BIN} "${scriptfile}" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" 2> "${tmperr}" > "${tmpout}"
    retcode=$?
  fi
fi


# now create a reference compare file:
echo "TEST SCRIPT + COMMANDLINE:"  > "${tmpchk}"
echo "${scriptfile} ${args}"      >> "${tmpchk}"
echo ""                           >> "${tmpchk}"
echo "RETURN CODE:"               >> "${tmpchk}"
echo "${retcode}"                 >> "${tmpchk}"
echo ""                           >> "${tmpchk}"
echo "STDERR TEST OUTPUT:"        >> "${tmpchk}"
if [ -f "${tmperr}" ]; then
  cat "${tmperr}"                 >> "${tmpchk}"
fi
echo ""                           >> "${tmpchk}"
echo "STDOUT TEST OUTPUT:"        >> "${tmpchk}"
if [ -f "${tmpout}" ]; then
  cat "${tmpout}"                 >> "${tmpchk}"
fi
echo ""                           >> "${tmpchk}"


if [ ${write2ref} = 1 ]; then
  # write reference file!
  cp "${tmpchk}" "${expected}"
fi


# and compare that chk file with the reference file:
retcode=65536
if [ ! -z "${diffscript}" ]; then
  targetname="${BUILDDIR}/`basename ${diffscript}`.temp"
  echo "targetname = ${targetname}"
  "${diffscript}" "${expected}" "${tmpchk}" "${filtout}" "${targetname}"
  retcode=$?
  # because the diffscript will certainly postprocess the data, we should
  # produce a 'true' (original) diff output for display when we FAIL:
  # (yet the result OK/FAIL is determined by the diffscript above!)
  /usr/bin/diff -u -EbwBd --strip-trailing-cr "${expected}" "${tmpchk}" > "${diffout}"
else
  /usr/bin/diff -u -EbwBd --strip-trailing-cr "${expected}" "${tmpchk}" > "${diffout}"
  retcode=$?
fi

debug_msg "CRM114_TEST_GEN = ${CRM114_TEST_GEN}"
debug_msg "srcdir = ${srcdir}"
debug_msg "home = ${HOME}"
debug_msg "CRM114_CHECK_OVERRIDE = ${CRM114_CHECK_OVERRIDE}"
debug_msg "exit ${CRM114_CHECK_OVERRIDE} vs. retcode = ${retcode}"

if [ ${retcode} == 0 ]; then
  echo -n "OK"
else
  echo "---- diff report: ----"
  cat "${diffout}"
  echo "----------------------"
  echo -n "FAIL"
fi
if [ ! -z "${CRM114_CHECK_OVERRIDE}" ]; then
  if [ ${CRM114_CHECK_OVERRIDE} == 0 ]; then
    echo "(OVERRIDEN:OK)"
  else
    echo "(OVERRIDEN:FAIL)"
  fi
  exit ${CRM114_CHECK_OVERRIDE}
else
  echo ""
  exit ${retcode}
fi



