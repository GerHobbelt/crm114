#! /bin/sh

#
# filter the produced output and reference file: we don't care about the PID: numbers in there.
#

# cmdline: filter <expected> <testresult>

reffile="$1"
testfile="$2"
difffile="$3"

rm -f "${difffile}"

rm -f "$0.temp.refout" 
rm -f "$0.temp.tstout" 

if [ ! -f "${reffile}" ] || [ ! -f "${testfile}" ]; then
  echo "One of the files to compare is missing: '${reffile}' -- '${testfile}'"
  exit 66
fi

sed -e "s,PID: [0-9]\+,PID: XXX,g" < "${reffile}" > "$0.temp.refout"
sed -e "s,PID: [0-9]\+,PID: XXX,g" < "${testfile}" > "$0.temp.tstout"

/usr/bin/diff -u -EbwBd --strip-trailing-cr "$0.temp.refout" "$0.temp.tstout" > "${difffile}"
retcode=$?

exit ${retcode}




