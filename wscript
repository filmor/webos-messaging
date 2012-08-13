from build_lib.util import join, get_pkg_path

VERSION='0.9.4'
APPNAME='org.webosinternals.purple'

TOOLDIR='build_lib'

PALM_PREFIX='org.webosinternals.'

def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.load('libpurple palm_programs', tooldir=TOOLDIR)
    opt.add_option('--protocols', action='store',
            default="msn,icq,jabber,novell,yahoo,sipe,sametime",
                   help="Protocols")

def configure(conf):
    conf.env.INCLUDES = []
    conf.env.PALM_PREFIX = PALM_PREFIX
    conf.env.TOOLDIR = TOOLDIR

    # TODO: Werror for my own code
    # TODO: Remove debugging for release
    conf.env.append_value("CFLAGS", ["-g3", "-Wall"])
    conf.env.append_value("CXXFLAGS", ["-g3", "-Wall"])
    conf.env.append_value("ARCH", "-march=armv7-a")
    conf.env.append_value("LIBPATH", [join("..", TOOLDIR, "lib")])
    conf.env.append_value("LINKFLAGS", "-Wl,-rpath=/var/usr/lib")

    conf.env.append_value("INCLUDES_GLIB", [join("..", TOOLDIR, "include")])
    conf.env.append_value("LIB_GLIB", ["glib-2.0", "gobject-2.0", "gmodule-2.0"])
    conf.env.append_value("LIBPATH_GLIB", [join("..", TOOLDIR, "lib")])

    conf.env.append_value("INCLUDES_NSS", [join("..", TOOLDIR, "include", i)
                                           for i in ("nss", "nspr")])
    conf.env.append_value("LIB_NSS", [])
    conf.env.append_value("LIBPATH_NSS", [join("..", TOOLDIR, "lib")])

    plugins = [
        "autoaccept",
        "idle",
        "joinpart",
        "log_reader",
        "newline",
        "offlinemsg",
        "psychic",
        ]

    conf.env.PURPLE_PROTOCOLS = conf.options.protocols.split(",")
    conf.env.PURPLE_PLUGINS = plugins
    conf.env.PURPLE_SSL = "gnutls"

    conf.env.PALM_APP_PREFIX = "/media/cryptofs/apps"
    conf.env.APP_IPKG_PATH = "/usr/palm/applications/org.webosinternals.purple"
    conf.env.PLUGIN_IPKG_PATH = join(conf.env.APP_IPKG_PATH, "plugins")
    conf.env.SYSTEM_IPKG_PATH = join(conf.env.APP_IPKG_PATH, "system")

    conf.env.APP_PATH = join(conf.env.PALM_APP_PREFIX, conf.env.APP_IPKG_PATH)
    conf.env.PLUGIN_PATH = join(conf.env.PALM_APP_PREFIX,
            conf.env.PLUGIN_IPKG_PATH)

    conf.load("libpurple palm_programs", tooldir=TOOLDIR)

def build(bld):
    bld.load('libpurple palm_programs', tooldir=TOOLDIR)

    start_dir = bld.path.find_dir("application")
    bld.install_files("${APP_IPKG_PATH}", start_dir.ant_glob("**/*"),
                      cwd=start_dir, relative_trick=True)

    start_dir = bld.path.find_dir("package")
    bld.install_files("${SYSTEM_IPKG_PATH}", start_dir.ant_glob("var/**/*"),
                      cwd=start_dir, relative_trick=True)

    start_dir = bld.path.find_dir(get_pkg_path("pidgin"))
    bld.install_files("${APP_IPKG_PATH}", start_dir.ant_glob("ca-certs/**/*"),
                      cwd=start_dir, relative_trick=True)

    # TODO: Locales

    bld(
            features="subst",
            target="control",
            source="package/control",
            install_path="/CONTROL",
            VERSION=VERSION,
            APPNAME=APPNAME
            )


def install(bld):
    bld.load('libpurple palm_programs application', tooldir=TOOLDIR)

