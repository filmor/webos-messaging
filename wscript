from os.path import join, basename, splitext
from os import chdir
from glob import glob

VERSION='0.1'
APPNAME='messaging-plugins'

TOOLDIR='build_lib'

def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.load('libpurple palm_programs', tooldir=TOOLDIR)
    opt.add_option('--protocols', action='store', default="msn,icq,jabber",
                   help="Protocols")

def configure(conf):
    conf.parse_flags("-Wall -Werror -O2 -march=armv7-a", "BASE")
    conf.env.INCLUDES = []
    conf.env.CFLAGS = []
    conf.env.CXXFLAGS = []
    conf.env.ARCH = "-march=armv7-a"
    conf.env.append_value('LINKFLAGS_BASE', [])
    conf.env.append_value("LIBPATH_BASE", [join("..", TOOLDIR, "lib")])

    conf.env.append_value("INCLUDES_GLIB", [join("..", TOOLDIR, "include")])
    conf.env.append_value("LIB_GLIB", ["glib-2.0", "gobject-2.0", "gmodule-2.0"])
    conf.env.append_value("LIBPATH_GLIB", [join("..", TOOLDIR, "lib")])

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

    conf.load("libpurple palm_programs", tooldir=TOOLDIR)

def build(bld):
    bld.load('libpurple palm_programs', tooldir=TOOLDIR)

def install(bld):
    bld.load('libpurple palm_programs', tooldir=TOOLDIR)

