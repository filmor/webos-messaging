from os import path

VERSION='0.1'
APPNAME='messaging-plugins'

TOOLDIR='build_lib'

PALM_PREFIX='org.webosinternals.'

def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.load('libpurple palm_programs', tooldir=TOOLDIR)
    opt.add_option('--protocols', action='store', default="msn,icq,jabber",
                   help="Protocols")

def configure(conf):
    conf.env.INCLUDES = []
    conf.env.PALM_PREFIX = PALM_PREFIX

    # TODO: Werror for my own code
    # TODO: Remove debugging for release
    conf.env.append_value("CFLAGS", ["-g3", "-Wall"])
    conf.env.append_value("CXXFLAGS", ["-g3", "-Wall"])
    conf.env.append_value("ARCH", "-march=armv7-a")
    conf.env.append_value("LIBPATH", [path.join("..", TOOLDIR, "lib")])
    conf.env.append_value("LINKFLAGS", "-Wl,-rpath=/var/usr/lib")

    conf.env.append_value("INCLUDES_GLIB", [path.join("..", TOOLDIR, "include")])
    conf.env.append_value("LIB_GLIB", ["glib-2.0", "gobject-2.0", "gmodule-2.0"])
    conf.env.append_value("LIBPATH_GLIB", [path.join("..", TOOLDIR, "lib")])

    # TODO: Parse plugin files to find static name, s.t. autoaccept works again
    plugins = [
        "Autoaccept",
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

    conf.env.APP_PATH = \
    "/media/cryptofs/apps/usr/palm/applications/org.webosinternals.purple"

    conf.load("libpurple palm_programs", tooldir=TOOLDIR)

def build(bld):
    bld.load('libpurple palm_programs', tooldir=TOOLDIR)

def install(bld):
    bld.load('libpurple palm_programs', tooldir=TOOLDIR)

