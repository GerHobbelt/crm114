## Process this file with automake to produce Makefile.in

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal

PACKAGE = @PACKAGE_TARNAME@
examplesdir = $(datadir)/doc/$(PACKAGE)/examples

SILENT = @SILENT@


examples_DATA = \
 megatest_knowngood.log \
 megatest.sh \
 pad.dat

examples_SCRIPTS = \
 pad.crm \
 shroud.crm

crmdir = $(examplesdir)/crmfilter

crm_DATA = \
 blacklist.mfp \
 priolist.mfp \
 rewrites.mfp \
 test_rewrites.mfp \
 whitelist.mfp \
 whitelist.mfp.example

EXTRA_DIST = $(examples_DATA) $(crm_DATA)


DISTCLEANFILES = 
CLEANFILES = *~ *.bak




