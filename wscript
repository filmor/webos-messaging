from os.path import join, basename

VERSION='0.1'
APPNAME='messaging-plugins'

PDK_PATH="/opt/PalmPDK"

subpaths = [
             "libpurple-2.10.0",
             "imaccountvalidator-1.0",
             "imlibpurpleservice-1.0",
             "imlibpurpletransport",
           ]

def do_recurse(f):
    def new_f(ctx):
        ctx.recurse(subpaths, mandatory=False)
        return f(ctx)
    new_f.__name__ = f.__name__
    return new_f

@do_recurse
def options(opt):
    opt.load('compiler_cxx boost')

@do_recurse
def configure(conf):
    conf.load('compiler_cxx boost')
    conf.parse_flags("-Wall -O2 -march=armv7", "BASIC")

    # PDK
    conf.env.append_value("INCLUDES_PDK", join(PDK_PATH, "include"))
    conf.env.append_value("LIBPATH_PDK", join(PDK_PATH, "device", "lib"))
    conf.env.append_value("LIB_PDK", ["pdl", "z"])

    conf.check_boost()

@do_recurse
def build(bld):
    pass

