from os.path import join
from glob import glob
import build_lib

# TODO: Get version from path
VERSION="2.10.0"

def get_path():
    try:
        return glob("deps/pidgin-*/libpurple")[-1]
    except IndexError:
        raise RuntimeError("Couldn't find libpurple sources")

def options(opt):
    opt.load('compiler_c')

def configure(conf):
    # TODO: Create new config environment
    conf.load('compiler_c')

    conf.check_cfg(atleast_pkgconfig_version='0.1')

    conf.check_cfg(package='libxml-2.0', uselib_store='XML',
                   args=['--cflags', '--libs'])

    conf.check_cfg(package='gnutls', uselib_store='GNUTLS')

    conf.env.append_value("DEFINES_PURPLE_BUILD", ["HAVE_CONFIG_H"])
    conf.env.append_value("INCLUDES_PURPLE_BUILD", ["libpurple_config"])
    
    conf.define("VERSION", VERSION)
    conf.define("DATADIR", ".")
    conf.define("SYSCONFDIR", ".")
    conf.define("LIBDIR", ".")
    conf.define("SIZEOF_TIME_T", 4, quote=False)
    conf.define("STATIC_PROTO_INIT", "", quote=False)
    conf.define("HAVE_CONFIG_H", 1, quote=False)
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

    import waflib
    saved_exclude_regs = waflib.Node.exclude_regs

    for i in exclude:
        waflib.Node.exclude_regs += "\n**/" + i

    source_files = bld.path.ant_glob(join(get_path(), "**/*.c"))

    waflib.Node.exclude_regs = saved_exclude_regs

    bld.shlib(
                target="purple",
                source=source_files,
                export_includes=get_path(),
                includes=get_path(),
                use="GLIB XML GNUTLS PURPLE_BUILD BASE"
                )

