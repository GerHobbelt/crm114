#! /bin/sh

echo "settings read-only rights"
#chmod -R a-w .
echo "creating vpath directories"
chmod a+w .
vp=`pwd`/_build
vi=`pwd`/_inst
mkdir ${vp}
mkdir ${vi}
chmod a-w .
# cd ${vi}
echo "running configure script"
cd ${vp}
pwd
../configure --srcdir=.. --prefix=${vi} $*
echo "running make"
#make
#make dvi
#make check
#make install
#make installcheck
#make uninstall
#make distuninstallcheck_dir=../_inst  distuninstallcheck
