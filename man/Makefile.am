## Process this file with automake to produce Makefile.in
## $Id: Makefile.am,v 1.6 2004/08/19 09:30:21 vanbaal Exp $

## Copyright (C) 2003 Stichting LogReport Foundation logreport@logreport.org
## Copyright (C) 2003 Tilburg University http://www.uvt.nl/
## Copyright (C) 2003, 2004 Joost van Baal

## zoem hacks based upon work by Stijn van Dongen.

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

TROFF = groff
COL = col

SILENT = @SILENT@


## Zoem is a an interpretive macro language, for creating mark-up languages, by
## Stijn van Dongen. Information about zoem can be found on the zoem webpage at
## http://micans.org/zoem/ .
## We need zoem >= 04-027
ZOEM = zoem

azms = crm.azm crm114.azm cssdiff.azm cssmerge.azm cssutil.azm
txts = crm.txt crm114.txt cssdiff.txt cssmerge.txt cssutil.txt
htmls = crm.html crm114.html cssdiff.html cssmerge.html cssutil.html
pss = crm.ps crm114.ps cssdiff.ps cssmerge.ps cssutil.ps
typetargets = $(txts) $(htmls)
tmpfiles = crm.zmt crm.zmr crm114.zmt crm114.zmr cssdiff.zmt cssdiff.zmr cssmerge.zmt cssmerge.zmr cssutil.zmt cssutil.zmr

# doc_DATA = $(typetargets) $(azms)
# [i_a] quick hack as I haven't got time to find and try this zoem tool...
doc_DATA = 
# man_MANS = crm.1 crm114.1 cssdiff.1 cssmerge.1 cssutil.1
# [i_a] quick hack as I haven't got time to find and try this zoem tool...
man_MANS = 

# EXTRA_DIST = $(azms) $(typetargets) $(man_MANS)
# [i_a] quick hack as I haven't got time to find and try this zoem tool...
EXTRA_DIST = $(azms)
DISTCLEANFILES = $(tmpfiles)
CLEANFILES = *~ *.bak
MAINTAINERCLEANFILES = $(typetargets) $(man_MANS)

VERSION = @PACKAGE_VERSION@
PACKAGE = @PACKAGE_TARNAME@

docdir = $(datadir)/doc/$(PACKAGE)

SUFFIXES = .1 .5 .8 .html .azm .ps .txt

.azm.1:
	$(ZOEM) -d roff -i $< -o $@
	$(ZOEM) -d roff -i $< -o $@

.azm.5:
	$(ZOEM) -d roff -i $< -o $@
	$(ZOEM) -d roff -i $< -o $@

.azm.8:
	$(ZOEM) -d roff -i $< -o $@
	$(ZOEM) -d roff -i $< -o $@

.1.ps:
	$(TROFF) -man $< > $@

.5.ps:
	$(TROFF) -man $< > $@

.8.ps:
	$(TROFF) -man $< > $@

.azm.html:
	$(ZOEM) -d html -i $< -o $@
	$(ZOEM) -d html -i $< -o $@

.1.txt:
	$(TROFF) -t -e -mandoc -Tlatin1 $< | $(COL) -bxp > $@

.5.txt:
	$(TROFF) -t -e -mandoc -Tlatin1 $< | $(COL) -bxp > $@

.8.txt:
	$(TROFF) -t -e -mandoc -Tlatin1 $< | $(COL) -bxp > $@

