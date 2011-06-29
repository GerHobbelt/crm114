#! /bin/sh

#
# find and convert files which contain TAB characters
#
for f in `find . -type f \( -name '*.[ch]' -o -name '*.htm?' \) -print `
do
	echo -------- $f ---------
	# convert TABs to space sequences, the trim all trailing whitespace and get rid of any lurking CRs too.
	expand $f | sed -e 's/ \+$//' | sed -e 's/\r$//' > $f.bak;
	mv -f $f $f~;
	mv -f $f.bak $f;
done;

#
# find and remove CR (CRLF line termination) from these files:
# (make sure we do NOT touch binary files!)
#
for f in `find . -type f \( -not \( -name '*.gif' -o -name '*.jpg' -o -name '*.bmp' -o -name '*.png' -o -name '*.dll' -o -name '*.exe' -o -name '*.mo' -o -name '*.ico' \) -a \( -name '*.?' -o -name 'Makefile*' -o -name '[A-Z][A-Z][A-Z]*' -o -name '*.???' -o -name 'config*' -o -name '*.??' -o -name '*.htm?' \) \) -print`
do
	echo -------- $f ---------
	# a bit redundant when we've got that sed s/\r$// up there, but alas, the find -print was to be
	# be more sophisticated when I did this: see end of script for notes.
	#
	# for now, this doesn't hurt, so keep it in here for the time when I find what the proper 
	#   find -exec grep -l ...
	# incantation should be!
	dos2unix $f;
done;





# Note: 
#            grep -l - e'\t' 
#       nor
#            grep -l -e '\011'
#       produces any reliable 'detect files with TABs' results on my box
#       (Suse 10.2/64-bit x86), so I simply expand 'em all by using
#            for `find ... -print`
#       instead of my initial
#            for `find ... -exec grep -l -e '\t' {} \;
#
#       The same goes for 
#            grep -l -e '\r$'
#       and
#            grep -l -e '\015$'
#
#       :-((
#

	


