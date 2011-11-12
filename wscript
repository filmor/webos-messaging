from os.path import join, basename

VERSION='0.1'
APPNAME='messaging-plugins'

PDK_PATH="/opt/PalmPDK"

PURPLE_PATH="libpurple-2.10.0"

subpaths = [
             PURPLE_PATH,
           ]

palm_programs = [
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
    conf.parse_flags("-Wall -Werror -O2 -march=armv7-a", "BASE")
    conf.env.INCLUDES = []
    conf.env.CFLAGS = []
    conf.env.CXXFLAGS = []
    conf.env.append_value('LINKFLAGS', ['--as-needed', '--no-add-needed'])
    conf.env.append_value("INCLUDES_BASE_PALM",
                            ["include", join("..", PURPLE_PATH)]
                         )

    conf.env.append_value("INCLUDES_GLIB", ["../include"])
    conf.env.append_value("LIB_GLIB", ["glib-2.0"])
    conf.env.append_value("LIBPATH_GLIB", ["../libs"])    

    conf.env.append_value("LIB_BASE_PALM", ["lunaservice", "mojoluna",
                                            "mojocore", "cjson", "mojodb",
                                            "sanitize"])

    conf.env.append_value("LIBPATH_BASE", ["../libs"])
    conf.env.append_value("DEFINES_BASE_PALM", ["MOJ_LINUX", "BOOST_NO_TYPEID"])

    conf.check_boost()

@do_recurse
def build(bld):
    for path in palm_programs:
        index = path.rfind('-')
        name = path[:index] if index > 0 else path
        bld.program(target=join("bin", name),
                    source=bld.path.ant_glob(join(path, "src/*.cpp")),
                    includes=join(path, "inc"),
                    use="GLIB BASE_PALM BASE purple",
                    )
