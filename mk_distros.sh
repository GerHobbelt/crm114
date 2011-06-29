#! /bin/bash

make distclean
find . -type f -exec chmod a-x {} \;
find . -type f -name '*.sh' -exec chmod a+x {} \;
find . -type f -name 'rename-gz' -exec chmod a+x {} \;
find . -type f -name 'configure' -exec chmod a+x {} \;
find . -type f -name 'setversion' -exec chmod a+x {} \;
find . -type f -name 'bootstrap' -exec chmod a+x {} \;
./autogen.sh
./configure
make dist-maint




