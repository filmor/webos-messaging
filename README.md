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
just launch

    $ ./get_deps.sh

in the root directory. You will also have to get some libraries, either from
your webOS device or using:

    $ ./get_libs.sh

Those libs are only needed to compile the plugins, they won't get deployed. I'm
actually looking for a better solution here, but that's the way it is for now :)

If you want to get the files from your device just follow this procedure: Try to
compile and get those files from your device that are listed as missing. You
will definitely need `libstdc++.so` from the device as the one provided with the
cross-compiler is broken.

After that you can configure the
package using:

    $ ./waf configure

Remember to prepare your environment for cross-compiling, e.g. by setting
`CHOST=arm-none-linux-gnueabi`.

After configuring you can build everything at once using the following command,
where you might want to consider using the flag `-jN`, with N being the number
of processors + 1:

    $ ./waf

Building an ipkg is done via:

    $ ./make_ipkg.sh

Have fun :)
