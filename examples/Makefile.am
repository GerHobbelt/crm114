## Process this file with automake to produce Makefile.in

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal

PACKAGE = @PACKAGE_TARNAME@
VER_SUFFIX = @VER_SUFFIX@ 
VERSION = @VERSION@
SILENT = @SILENT@

examplesdir = $(datadir)/doc/$(PACKAGE)/examples
crmdir = $(examplesdir)/crmfilter


examples_DATA = \
 pad.dat

examples_SCRIPTS = \
 pad.crm \
 shroud.crm


crm_DATA = \
 blacklist.mfp \
 priolist.mfp \
 rewrites.mfp \
 whitelist.mfp \
 whitelist.mfp.example



EXTRA_DIST = $(examples_DATA) $(examples_SCRIPTS) \
	$(crm_DATA)



DISTCLEANFILES = 
CLEANFILES = *~ *.bak




