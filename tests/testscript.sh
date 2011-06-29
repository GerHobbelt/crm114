#!/bin/sh
cd "$(dirname "$0")"
if [ ! -r "$(basename "$0")" ]; then
	echo please use explicit path when running this script
	exit 1
fi
cp ../examples/test_rewrites.mfp .
cp ../examples/whitelist.mfp .

set -e
PATH="../src"
:|crm114 aliustest.crm
:|crm114 approxtest.crm
:|crm114 argtest.crm
:|crm114 backwardstest.crm
:|crm114 beeptest.crm
:|crm114 bracktest.crm
:|crm114 classifytest.crm
:|crm114 escapetest.crm
:|crm114 eval_infiniteloop.crm
:|crm114 exectest.crm
:|crm114 fataltraptest.crm
:|crm114 inserttest_a.crm
:|crm114 inserttest_b.crm
:|crm114 inserttest_c.crm
:|crm114 learntest.crm
:|crm114 matchtest.crm
:|crm114 mathalgtest.crm
:|crm114 mathrpntest.crm
:|crm114 nestaliustest.crm
:|crm114 overalterisolatedtest.crm
:|crm114 rewritetest.crm
:|crm114 skudtest.crm
:|crm114 statustest.crm
:|crm114 traptest.crm
if :|crm114 uncaughttraptest.crm; then
	echo uncaughttraptest failed
	false
else
	true
fi
:|crm114 unionintersecttest.crm
:|crm114 userdirtest.crm
:|crm114 windowtest.crm
:|crm114 windowtest_fromvar.crm
