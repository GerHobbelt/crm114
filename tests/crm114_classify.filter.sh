#! /bin/sh

#
# filter the produced output and reference file: 
# handle slight alterations to classify reports; use env.vars to get tolerances: that's a hack
# as 'testscript.sh lacks a method to pass US arguments otherwise.
#

# cmdline: filter <expected> <testresult> <basename-for-tempfiles>

reffile="$1"
testfile="$2"
difffile="$3"
targetname="$4"

awkargs=${CRM114_CHECK_FILTER_ARGS}

BUILDDIR=.
SRCDIR=.
REFDIR="${SRCDIR}/ref"

rm -f "${difffile}"

rm -f "${targetname}.refout" 
rm -f "${targetname}.tstout" 

if [ ! -f "${reffile}" ] || [ ! -f "${testfile}" ]; then
  echo "One of the files to compare is missing: '${reffile}' -- '${testfile}'"
  exit 66
fi
if [ ! -f "${SRCDIR}/crm114_classify.filter.awk" ]; then
  echo "The mandatory AWK classify result filter script is missing: '${SRCDIR}/crm114_classify.filter.awk'"
  exit 67
fi
2echo "AWK filter args: ${awkargs}"


gawk -f "${SRCDIR}/crm114_classify.filter.awk" ${awkargs} < "${reffile}" > "${targetname}.refout"
gawk -f "${SRCDIR}/crm114_classify.filter.awk" ${awkargs} < "${testfile}" > "${targetname}.tstout"


# if [ ! -z "${CRM114_MAKE_SCRIPTS_DEBUG}" ] || [ 1=1 ]; then
if [ ! -z "${CRM114_MAKE_SCRIPTS_DEBUG}" ]; then
  echo "------------------------------------------------------"
  cat "${targetname}.refout"
  echo "-----------------------------------------------------"
  cat "${targetname}.tstout"
  echo "------------------------------------------------------"
fi


/usr/bin/diff -u -EbwBd --strip-trailing-cr "${targetname}.refout" "${targetname}.tstout" > "${difffile}"
retcode=$?
if [ ! -z "${CRM114_MAKE_SCRIPTS_DEBUG}" ]; then
  echo "-- processed diff: -----------------------------------"
  cat "${difffile}"
  echo "------------------------------------------------------"
fi

exit ${retcode}



