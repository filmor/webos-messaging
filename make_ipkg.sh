#!/bin/sh
./waf -j3 || exit 1
rm -rf ipkg_root
./waf install --destdir=ipkg_root || exit 1

find ipkg_root/ -type f -exec arm-none-linux-gnueabi-strip {} \; 2> /dev/null

pushd build > /dev/null
ipkg-build ../ipkg_root
for i in *.ipk
do
    ar q ${i} ../package/pmPostInstall.script ../package/pmPreRemove.script
done
popd > /dev/null
