Messaging Plugins for webOS
===========================

This package aims to complement the messaging support on webOS (esp. on the
Touchpad) and enable everything that is supported by libpurple.

Dependencies
------------

For now you'll need the following:

- Python
- An ARM-Linux cross-compiler toolchain (like the one from the PDK),
    version `4.3.3`
- The PDK itself (for libxml2)
- The pidgin source package (I guess every version between 2 and 3 should work,
    just use the newest one ;))
- cyrus-sasl, version `>=2.1` if you want to use jabber
- ipkg-utils for ipkg-build

Building
--------

To build the plugins you need to drop the sources (pidgin and cyrus-sasl) into
the deps/ subdirectory and extract them. You will also have to get some
libraries, either from your webOS device or using:

    $ ./get_libs.sh

Those libs are only needed to compile the plugins, they won't get deployed. I'm
actually looking for a better solution here, but that's the way it is for now :)

If you want to get the files from your device just follow this procedure, try to
compile and get those files from your device that are listed as missing. You
will definitely need `libstdc++.so` from the device as the one provided with the
cross-compiler is broken.

After that you can configure the
package using:

    $ ./waf configure

Remember to prepare your environment for cross-compiling, e.g. by setting
`CHOST=arm-none-linux-gnueabi`.

If you want to use other protocols than msn, icq and jabber (which are default
for now because I use them ;)), use the protocols-flag:

    $ ./waf configure --protocols=aim,jabber

After that you can build everything at once (TODO: allow split builds) using:

    $ ./waf

Building an ipkg is done via:

    $ ./make_ipkg.sh

Have fun :)
