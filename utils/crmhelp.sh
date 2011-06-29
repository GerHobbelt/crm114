#!/bin/sh
# $Id:
# (C) >ten.fs.sresu@alpoo< oloap - GPLv2 http://www.gnu.org/licenses/gpl.txt

QPATH="$2 . /usr/doc/crm114
  /usr/share/doc/crm114
  /usr/local/doc/crm114
  /usr/local/share/doc/crm114"

CPATH="$2 . /usr/local/bin /usr/bin /bin"

[ $# = 0 ] && {
  cat <<EOU
  Usage: ${0##*/} statement
     to get help and grammar infos on crm114 'statement' [path-to-QUICKREF.txt].
     Default search path:
     $QPATH
     Default crm binary search path:
     $CPATH
EOU
  exit
}

for d in $QPATH;do
  [ -d $d ] || continue
  [ -s $d/QUICKREF.txt ] && {
    QREF=$d/QUICKREF.txt
    break
  }
done
[ "$QREF" ] || {
  echo -e "Can't find QUICKREF.txt anywhere in:\n$QPATH"
  exit 1
}
for d in $CPATH;do
  [ -d $d ] || continue
  [ -x $d/crm ] && {
    CRM=$d/crm
    break
  }
  [ -x $d/crm114 ] && {
    CRM=$d/crm114
    break
  }
done
[ "$CRM" ] || {
  echo -e "Can't find crm114 binary anywhere in:\n$CPATH"
  exit 1
}


(echo -en "-Help on crm114 statement: \"$1\" from $QREF -\nversion:"
  head -1 $QREF
  cat <<EOL
-------------------------------------------------------------------------------
EOL
  $CRM '-{
  match (:z: :h:) /.*\n(:*:_arg2:[[:blank:]]([^\n]|\n[[:blank:]])+).*/
  output /:*:h:\n/
  }' $1 -e < $QREF
  cat <<EOL
-------------------------------------------------------------------------------
EOL
)| pager

