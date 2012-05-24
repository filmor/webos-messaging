#!/bin/sh
./waf -j3 || exit 1
./install.sh || exit 1

pushd build > /dev/null
ipkg-build ../ipkg_root
for i in *.ipk
do
    ar q ${i} ../package/pmPostInstall.script ../package/pmPreRemove.script
done
popd > /dev/null
