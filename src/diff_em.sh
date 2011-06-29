#! /bin/bash

rm -rf diff
mkdir diff 

for f in *.[ch] megatest.sh *.crm *.mfp ; do
	if [ -e ../../../1original/crm114/src/crm114.sourceforge.net/src/$f ] 
	then
		rm -f diff/$f.`date +%Y%m%d`.context.diff 2&>1
		rm -f diff/$f.`date +%Y%m%d`.unified.diff 2&>1
		diff -EbwBd ../../../1original/crm114/src/crm114.sourceforge.net/src/$f $f -I "^[[:blank:]]*};\?[[:blank:]]*\$" > diff/$f.`date +%Y%m%d`.normal.diff
		# don't keep empty diffs around, that's clutter
		if [ -s diff/$f.`date +%Y%m%d`.normal.diff ]
		then
			diff -EbwBd ../../../1original/crm114/src/crm114.sourceforge.net/src/$f $f -I "^[[:blank:]]*};\?[[:blank:]]*\$" -C3 > diff/$f.`date +%Y%m%d`.context.diff
			diff -EbwBd ../../../1original/crm114/src/crm114.sourceforge.net/src/$f $f -I "^[[:blank:]]*};\?[[:blank:]]*\$" -U3 > diff/$f.`date +%Y%m%d`.unified.diff
		else
			rm diff/$f.`date +%Y%m%d`.normal.diff
		fi
	fi
done

# and produce a 'global spanning' diff
diff -EbwBd ../../../1original/crm114/src/crm114.sourceforge.net/src/ . -I "^[[:blank:]]*};\?[[:blank:]]*\$" > diff/crm114.`date +%Y%m%d`.normal.diff
diff -EbwBd ../../../1original/crm114/src/crm114.sourceforge.net/src/ . -I "^[[:blank:]]*};\?[[:blank:]]*\$" -C3 > diff/crm114.`date +%Y%m%d`.context.diff
diff -EbwBd ../../../1original/crm114/src/crm114.sourceforge.net/src/ . -I "^[[:blank:]]*};\?[[:blank:]]*\$" -U3 > diff/crm114.`date +%Y%m%d`.unified.diff
