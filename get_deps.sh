#!/bin/sh

# Get Pidgin
wget 'http://sourceforge.net/projects/pidgin/files/latest/download?source=files' -O deps/pidgin.tar.bz2
wget 'ftp://ftp.andrew.cmu.edu/pub/cyrus-mail/cyrus-sasl-2.1.25.tar.gz' -O deps/cyrus-sasl.tar.gz

pushd deps > /dev/null
tar -xvzf cyrus-sasl.tar.gz
tar -xvjf pidgin.tar.bz2
rm cyrus-sasl.tar.gz
rm pidgin.tar.bz2
popd > /dev/null

wget https://github.com/downloads/filmor/webos-messaging/libs.tar.bz2
tar -xvjf libs.tar.bz2
rm libs.tar.bz2

