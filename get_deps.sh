#!/bin/sh

# Get Pidgin
wget 'http://sourceforge.net/projects/pidgin/files/latest/download?source=files' -O deps/pidgin.tar.bz2
# wget 'ftp://ftp.andrew.cmu.edu/pub/cyrus-mail/cyrus-sasl-2.1.25.tar.gz' -O deps/cyrus-sasl.tar.gz
wget 'http://sourceforge.net/projects/meanwhile/files/meanwhile/1.0.2/meanwhile-1.0.2.tar.gz/download' -O deps/meanwhile.tar.gz
wget 'http://downloads.sourceforge.net/project/sipe/sipe/pidgin-sipe-1.13.2/pidgin-sipe-1.13.2.tar.bz2' -O deps/pidgin-sipe.tar.bz2

pushd deps > /dev/null
# tar -xvzf cyrus-sasl.tar.gz
tar -xvjf pidgin.tar.bz2
tar -xvzf meanwhile.tar.gz
tar -xvjf pidgin-sipe.tar.bz2
# rm cyrus-sasl.tar.gz
rm pidgin.tar.bz2
rm pidgin-sipe.tar.bz2
rm meanwhile.tar.gz
popd > /dev/null

wget https://github.com/downloads/filmor/webos-messaging/libs.tar.bz2
tar -xvjf libs.tar.bz2
rm libs.tar.bz2

