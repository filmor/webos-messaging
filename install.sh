#!/bin/sh

IPKG_ROOT=ipkg_root
PREFIX="org.webosinternals"
PACKAGE="${PREFIX}.purple"
PKG_ROOT=${IPKG_ROOT}/usr/palm/applications/${PACKAGE}

BIN_FILES="${PACKAGE}.transport ${PACKAGE}.validator"
LIB_FILES="libpurple.so"

rm -r ${IPKG_ROOT}/*

mkdir -p ${PKG_ROOT}/system
cp -rf files/var ${PKG_ROOT}/system
cp -rf files/etc ${PKG_ROOT}/system

mkdir -p ${PKG_ROOT}/system/var/usr/sbin
for i in ${BIN_FILES}
do
    cp build/${i} ${PKG_ROOT}/system/var/usr/sbin
done

mkdir -p ${PKG_ROOT}/system/var/usr/lib
for i in ${LIB_FILES}
do
    cp build/${i} ${PKG_ROOT}/system/var/usr/lib
    arm-none-linux-gnueabi-strip ${PKG_ROOT}/system/var/usr/lib/${i}
done

mkdir -p ${IPKG_ROOT}/CONTROL
cp package/* ${IPKG_ROOT}/CONTROL

# Accounts
PROTOCOLS="facebook icq msn google_talk jabber"
PIDGIN_DIRS=(deps/pidgin-*)

for i in ${PROTOCOLS}
do
    build_lib/account.py ${PIDGIN_DIRS[0]} \
        ${IPKG_ROOT}/usr/palm/accounts \
        accounts/${i}.json \
        accounts/prototype.json || exit -1
done

# Application
cp -r application/* ${PKG_ROOT}
