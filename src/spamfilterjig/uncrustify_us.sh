#! /bin/sh

#
# uncrustify all source code:
# 

find ./ -type f -name '*.[ch]' -exec uncrustify -c uncrustify.cfg --replace "{}" \;
find ./ -type f -name '*.cpp' -exec uncrustify -c uncrustify.cfg --replace "{}" \;

