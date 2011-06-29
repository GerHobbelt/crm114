## Process this file with automake to produce Makefile.in

## $Id: Makefile.am,v 1.7 2004/08/18 13:26:30 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal



cdir = $(datadir)/doc/$(PACKAGE)/examples/tests


DISTCLEANFILES = 
CLEANFILES = *~ *.bak


SILENT = @SILENT@



c_SCRIPTS = \
 aliustest.crm \
 approxtest.crm \
 argtest.crm \
 backwardstest.crm \
 beeptest.crm \
 bracktest.crm \
 classifytest.crm \
 escapetest.crm \
 eval_infiniteloop.crm \
 exectest.crm \
 fataltraptest.crm \
 inserttest_a.crm \
 inserttest_b.crm \
 inserttest_c.crm \
 learntest.crm \
 match_isolate_test.crm \
 matchtest.crm \
 mathalgtest.crm \
 mathrpntest.crm \
 nestaliustest.crm \
 overalterisolatedtest.crm \
 paolo_overvars.crm \
 randomiotest.crm \
 rewritetest.crm \
 skudtest.crm \
 statustest.crm \
 traptest.crm \
 uncaughttraptest.crm \
 unionintersecttest.crm \
 userdirtest.crm \
 windowtest.crm \
 windowtest_fromvar.crm



megatest: FORCE
	$(SILENT) echo ""        
	$(SILENT) echo "    This runs a moderately interesting set of base tests"
	$(SILENT) echo "    to exercise much of CRM114 under TRE.  This takes about"
	$(SILENT) echo "    1 minute to run on a 1.6 GHz Pentium-M laptop.  Please"
	$(SILENT) echo "    be patient; you (hopefully) won't see anything till the"
	$(SILENT) echo "    full set of tests complete.  If you didn't use TRE, all"
	$(SILENT) echo "    bets are off."
	$(SILENT) echo ""
	$(SILENT) echo "    Lines of output that start with OK_IF_mumble are allowed"
	$(SILENT) echo "    to change values.  No other lines should.  If other lines"
	$(SILENT) echo "    do change, either your kit isn't quite right or your"
	$(SILENT) echo "    install is broken (or you've found a bug)."
	$(SILENT) echo ""
	$(srcdir)/megatest.sh > megatest_test.log 2>&1
	diff -EbwBd megatest_knowngood.log megatest_test.log & sleep 1
