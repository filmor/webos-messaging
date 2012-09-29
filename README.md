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
- ipkg-utils for ipkg-build
- ImageMagick's for convert

Building
--------

To get all the source dependencies (pidgin, pidgin-sipe and meanwhile for now)
and support libraries that are used during link-time just launch

    $ ./waf get_libs

After that you can configure the
package using:

    $ ./waf configure

Remember to prepare your environment for cross-compiling, e.g. by setting
`CHOST=arm-none-linux-gnueabi` or by explicitly choosing
`CC=arm-none-linux-gnueabi-gcc` and `CXX=arm-none-linux-gnueabi-g++`.

After configuring you can build everything at once using the following command,
where you might want to consider using the flag `-jN`, with N being the number
of processors + 1:

    $ ./waf

Building an ipkg is done via:

    $ ./make_ipkg.sh

Have fun :)
