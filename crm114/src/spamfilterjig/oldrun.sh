#!/bin/bash
#<Spam Filter Evaluation run script. This script is created for the purpose of evaluating spam filters> Copyright 2005 Lynam & Cormack

#This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

#You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA 


case $# in
3) cpath=$1 ; runid=$2 ;output=$3 ;;
2) cpath=$1 ; runid=$2 ;output=results ;;
1) cpath=$1 ; runid=none ;output=results ;;
0) cpath=`pwd`;  output=results ;;
*) echo Usage $0 [corpus_path] [outputfile] ; exit 1;;
esac

rm -f $output

date > start_time


#install filter and initialize
./initialize
echo initialize complete

#run classifier on corpus index file
#format should be "judgement email_file_name genre"
exec 3< ${cpath}index # Assign new file descriptor and use for reading
while read -u3 line; do

#parsing index
judge=`expr "$line" : '\([^ ]*\)'`
file=`expr "$line" : '[^ ]* \([^ ]*\)'`
user=`expr "$line" : '[^ ]* [^ ]* \([^ ]*\)'`
genre=`expr "$line" : '[^ ]* [^ ]* [^ ]* \([^ ]*\)'`


#classification
if [ "$judge" == "ham" ] || [ "$judge" == "Ham" ] || [ "$judge" == "spam" ] || [ "$judge" == "Spam" ] || [ "$judge" == "unknown" ] ; then

echo classifying $cpath$file
classify=`./classify $cpath$file $user`
#parsing classification output format should be 'class=(ham|spam) score=X tfile=Y'
#where X is a number and Y is a tmp file name
class=`expr "$classify" : '.*class=\([^ ]*\)'`
score=`expr "$classify" : '.*score=\([^ ]*\)'`
tfile=`expr "$classify" : '.*tfile=\([^ ]*\)'`

#printing results
echo file=$cpath$file judge=$judge class=$class score=$score user=$user genre=$genre runid=$runid
echo file=$cpath$file judge=$judge class=$class score=$score user=$user genre=$genre runid=$runid >> $output
fi



#train part pasing judgement, email file, classification the system made, and tmp file
echo training $cpath$file $judge $user
#./train $judge $cpath$file $class $tfile $user
if [ "$judge" == "ham" ] ; then
./train $judge $cpath$file $class $tfile $user
fi
if [ "$judge" == "spam" ] ; then
./train $judge $cpath$file $class $tfile $user
fi
if [ "$judge" == "HAM" ] ; then
./train ham $cpath$file $class $tfile $user
fi
if [ "$judge" == "SPAM" ] ; then
./train spam $cpath$file $class $tfile $user
fi





#cleaning up tmp filea
if [[  $tfile != "" ]]; then
if [ -e $tfile ]; then 
rm $tfile
fi
fi

done

# Release file descriptor and tell classifier to clean up
exec 3<&-
./finalize
date > finish_time
 
