#!/bin/sh
./waf -j3 || exit 1
./install.sh || exit 1

pushd build > /dev/null
ipkg-build ../ipkg_root
popd > /dev/null
