from os.path import join
from glob import glob
from util import ant_glob, get_pkg_path_and_version, get_pkg_path
from itertools import chain
from re import match

from waflib.Configure import conf

def configure(conf):
    conf.load('compiler_c intltool')

    path, version = get_pkg_path_and_version("pidgin")
    conf.env.PURPLE_PATH = join(path, "libpurple")
    conf.env.PURPLE_VERSION = version

    if conf.env.PURPLE_SSL:
        # Order matters: It seems like ssl-gnutls has to be loaded before
        # core-ssl to be found (see ssl.c:probe_ssl_plugins)
        conf.env.PURPLE_PLUGINS += ["ssl-" + conf.env.PURPLE_SSL, "ssl"]

        conf.env.append_value("LIB_PURPLE_BUILD", ["gnutls"])

    plugins = conf.env.PURPLE_PLUGINS
    protocols = conf.env.PURPLE_PROTOCOLS

    conf.check_cfg(atleast_pkgconfig_version='0.1')
    conf.check_cfg(package='libxml-2.0', uselib_store='XML',
                   args=['--cflags', '--libs'])

    conf.env.append_value("DEFINES_PURPLE_BUILD", ["HAVE_CONFIG_H"])
    conf.env.append_value("INCLUDES_PURPLE_BUILD", ["libpurple_config",
                                                    conf.env.PURPLE_PATH])
    conf.env.append_value("LIB_PURPLE_BUILD", ["resolv"])

    # We are going to build a shared library
    conf.env.append_value("CFLAGS_PURPLE_BUILD", ["-fPIC"])

    headers = ["arpa/nameser_compat", "fcntl", "sys/time",
               "unistd", "locale", "libintl", "signal", "stdint", "regex"]

    for i in headers:
        conf.check_cc(header_name=i + ".h", mandatory=False,
                      auto_add_header_name=True)

    conf.define("PURPLE_PLUGINS", 1)
    conf.define("HAVE_GETIFADDRS", 1)
    conf.define("HAVE_INET_NTOP", 1)
    conf.define("HAVE_INET_ATON", 1)
    conf.define("HAVE_GETADDRINFO", 1)
    conf.define("HAVE_STRUCT_TM_TM_ZONE", 1)
    conf.define("HAVE_TM_GMTOFF", 1)
    conf.define("HAVE_TIMEZONE", 1)
    conf.define("HAVE_TIMGM", 1)
    conf.define("HAVE_STRFTIME_Z_FORMAT", 1)
    conf.define("HAVE_FILENO", 1)
    conf.define("HAVE_STRUCT_SOCKADDR_SA_LEN", 1)
    conf.define("VERSION", conf.env.PURPLE_VERSION)
    conf.define("DISPLAY_VERSION", conf.env.PURPLE_VERSION)
    conf.define("DATADIR", ".")
    conf.define("SYSCONFDIR", ".")
    conf.define("PACKAGE_NAME", "libpurple")
    conf.define("HAVE_SSL", 1)
    conf.define("HAVE_ICONV", 1)
    conf.define("SIZEOF_TIME_T", 4, quote=False)
    conf.define("HAVE_CONFIG_H", 1, quote=False)
#    conf.define("HAVE_CYRUS_SASL", 1, quote=False)
    conf.define("HAVE_GNUTLS_PRIORITY_FUNCS", 1)
    conf.define("_GNU_SOURCE", 1, quote=False)

    conf.define("SSL_CERTIFICATES_DIR",
            join(conf.env.APP_PATH, "ca-certs")
    )
    conf.define("LIBDIR", conf.env.PLUGIN_PATH)

    conf.define("ENABLE_NLS", 1, quote=False)
    conf.define("PACKAGE", "libpurple")
    conf.define("LOCALEDIR",
                join(conf.env.APP_PATH, "share", "locale")
    )

    conf.define("STATIC_PROTO_INIT", "", quote=False)
    conf.write_config_header('libpurple_config/config.h')

    conf.load('protocols', tooldir=conf.env.TOOLDIR)


def build(bld):
    use = ["BASE", "GLIB", "XML", "GNUTLS", "PURPLE_BUILD"]

    bld.load('protocols', tooldir=bld.env.TOOLDIR)
    for name in bld.env.PURPLE_PLUGINS:
        l = bld.shlib(target="plugins/%s" % name,
                      source=join(bld.env.PURPLE_PATH, "plugins",
                                (join("ssl", name)
                                    if name.startswith("ssl")
                                    else name
                                )
                                + ".c"),
                      install_path=bld.env.PLUGIN_IPKG_PATH,
                      use=use)
        l.env.cshlib_PATTERN = "%s.so"

    exclude = ["purple-client.c",
               "purple-client-example.c",
               "nullclient.c",
               "dbus-server.c",
               "protocols",
               "plugins",
               "win32",
               "tests"]

    bld.shlib(
                target="purple",
                source=ant_glob(bld, bld.env.PURPLE_PATH, "**", "*.c", exclude=exclude),
                export_includes=bld.env.PURPLE_PATH,
                includes=bld.env.PURPLE_PATH,
                use=use + ["plugins"],
                install_path="${APP_PATH}/system/var/usr/lib"
             )
