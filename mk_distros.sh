#! /bin/bash

./autogen.sh
./configure
make dist-all
make dist-bzip2
make dist-7z
make dist-diff




