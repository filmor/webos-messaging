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
- ar, tar and bzip2 or gz (should come with every proper unix system)


Building
--------

To build the plugins you need to drop the sources (pidgin and cyrus-sasl) into
the deps/ subdirectory and extract them. After that you can configure the
package using:

    $ ./waf configure

If you want to use other protocols than msn, icq and jabber (which are default
for now because I use them ;)), use the protocols-flag:

    $ ./waf configure --protocols=aim,jabber

After that you can build everything at once (TODO: allow split builds) using:

    $ ./waf

Building an ipkg is done via (NOT IMPLEMENTED YET):

    $ ./waf make_ipkg

You might need to copy some libraries from your webOS device into
`build_lib/lib`, especially `libstdc++.so`, since the one deployed in the
PDK is broken.
