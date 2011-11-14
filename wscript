from os.path import join, basename, splitext
from os import chdir
from glob import glob
import waflib

VERSION='0.1'
APPNAME='messaging-plugins'

modules = {}

modules.update(
                (splitext(basename(i))[0], waflib.Context.load_module(i))
                for i in glob("build_lib/*.py")
              )

palm_programs = [
        "imaccountvalidator-1.0",
        "imlibpurpleservice-1.0",
        "imlibpurpletransport",
        ]

def do_recurse(f):
    # TODO Only import what is needed (which we can see from options)
    def new_f(ctx):
        f(ctx)
        for m in modules.values():
            m.__dict__.get(f.__name__, lambda x:None)(ctx)

    new_f.__name__ = f.__name__
    return new_f

@do_recurse
def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.add_option('--protocols', action='store', default="msn",
                   help="Protocols")

@do_recurse
def configure(conf):
    conf.parse_flags("-Wall -Werror -O2 -march=armv7-a", "BASE")
    conf.env.INCLUDES = []
    conf.env.CFLAGS = []
    conf.env.CXXFLAGS = []
    conf.env.append_value('LINKFLAGS_BASE', [])
    conf.env.append_value("LIBPATH_BASE", ["../libs"])

    conf.env.append_value("INCLUDES_GLIB", ["../include"])
    conf.env.append_value("LIB_GLIB", ["glib-2.0", "gobject-2.0"])
    conf.env.append_value("LIBPATH_GLIB", ["../libs"])

    conf.env.PROTOCOLS = conf.options.protocols.split(",")

@do_recurse
def build(bld):
    pass

