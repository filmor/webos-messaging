from os.path import join
from glob import glob
from util import ant_glob

# TODO: Get version from path
VERSION="2.10.0"

# TODO: Set these informations in ctx.env!!
def get_path():
    try:
        return glob("deps/pidgin-*/libpurple")[-1]
    except IndexError:
        raise RuntimeError("Couldn't find libpurple sources")

def get_protocol_path(name):
    return join(get_path(), "protocols", name)


def configure(conf):
    # TODO: Create new config environment
    conf.load('compiler_c')

    conf.check_cfg(atleast_pkgconfig_version='0.1')

    conf.check_cfg(package='libxml-2.0', uselib_store='XML',
                   args=['--cflags', '--libs'])

    conf.env.append_value("DEFINES_PURPLE_BUILD", ["HAVE_CONFIG_H",
                                                   "PURPLE_STATIC_PRPL"])
    conf.env.append_value("INCLUDES_PURPLE_BUILD", ["libpurple_config",
                                                    get_path()])

    conf.env.append_value("CFLAGS_PURPLE_BUILD", ["-fPIC"])
    conf.env.append_value("LIB_PURPLE_BUILD", ["gnutls"])

    headers = ["arpa/nameser_compat", "fcntl", "sys/time",
               "unistd", "locale", "signal", "stdint", "regex"]

    for i in headers:
        conf.check_cc(header_name=i + ".h", mandatory=False,
                      auto_add_header_name=True)

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
    conf.define("VERSION", VERSION)
    conf.define("DISPLAY_VERSION", VERSION)
    conf.define("DATADIR", ".")
    conf.define("SYSCONFDIR", ".")
    conf.define("HAVE_SSL", 1)
    conf.define("HAVE_ICONV", 1)
    conf.define("LIBDIR", ".")
    conf.define("SIZEOF_TIME_T", 4, quote=False)
    conf.define("STATIC_PROTO_INIT", "void static_proto_init() {}", quote=False)
    conf.define("HAVE_CONFIG_H", 1, quote=False)
    conf.define("HAVE_CYRUS_SASL", 1, quote=False)
    conf.define("HAVE_GNUTLS_PRIORITY_FUNCS", 1)
    conf.define("_GNU_SOURCE", 1, quote=False)
    conf.write_config_header('libpurple_config/config.h')
    conf.defines = {}

def build(bld):
    exclude = ["purple-client.c",
               "purple-client-example.c",
               "nullclient.c",
               "dbus-server.c",
               "protocols",
               "plugins",
               "win32",
               "tests"]

    use = " ".join(["GLIB XML GNUTLS PURPLE_BUILD BASE purple_plugins \
                     ssl_plugins"]
                   + bld.env.PROTOCOLS)

    bld.shlib(
                target="purple",
                source=ant_glob(bld, get_path(), "**", "*.c", exclude=exclude),
                export_includes=get_path(),
                includes=get_path(),
                use=use,
             )

