# Process this file with automake to produce Makefile.in (in this,
# and all subdirectories).

## $Id: Makefile.am,v 1.14 2004/08/19 09:36:09 vanbaal Exp $

## This script is free software; you can distribute it and/or modify it
## under the terms of the GNU GPL.  See the file COPYING.

## Copyright (C) 2003, 2004 Joost van Baal

AUTOMAKE_OPTIONS = check-news

PACKAGE = @PACKAGE_TARNAME@

SUBDIRS = docs examples mailfilter man src tests

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = AUTHORS COPYING ChangeLog HACKING README TODO NEWS README.1st

DISTCLEANFILES = 
CLEANFILES = *~ *.bak */*~ */*.bak */*/*~ */*/*.bak

EXTRA_DIST = bootstrap crm114.lsm.in HACKING README.1st setversion \
  stamp.day stamp.month stamp.year

ChangeLog: NEWS
	/usr/bin/cvs2cl --prune --accum -U ../CVSROOT/users

year.stamp day.stamp month.stamp: ChangeLog configure.ac
	sh ./setversion

.PHONY: reindent 


reindent:
	echo "Reformatting sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) reindent);

depend:
	echo "Calculating dependencies for the sources"; 
	(cd src && $(MAKE) $(AM_MAKEFLAGS) depend);


