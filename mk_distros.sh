#! /bin/bash

make distclean
find . -type f -exec chmod a-x {} \;
find . -type f -name '*.sh' -exec chmod a+x {} \;
find . -type f -name '*.crm' -exec chmod a+x {} \;
find . -type f -name '*.crm.in' -exec chmod a+x {} \;
find . -type f -name 'rename-gz' -exec chmod a+x {} \;
find . -type f -name 'configure' -exec chmod a+x {} \;
find . -type f -name 'setversion' -exec chmod a+x {} \;
find . -type f -name 'bootstrap' -exec chmod a+x {} \;

if [ ! -e src/megatest_knowngood.orginal.log ]
then 
  cp src/megatest_knowngood.log src/megatest_knowngood.orginal.log
fi
./autogen.sh
./configure
make
make megatest
# cp src/megatest_test.log src/megatest_knowngood.log
make distribution
