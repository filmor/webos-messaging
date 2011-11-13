from os.path import join, basename
import libpurple

palm_programs = [
        "imaccountvalidator-1.0",
        "imlibpurpleservice-1.0",
        "imlibpurpletransport",
        ]

def options(opt):
    opt.load('compiler_cxx')

def configure(conf):
    conf.load('compiler_cxx')

    conf.env.append_value("LIB", ["lunaservice", "mojoluna",
                                  "mojocore", "cjson", "mojodb",
                                  "sanitize"])
    conf.env.append_value("DEFINES", ["MOJ_LINUX"])
    conf.env.append_value("INCLUDES", [libpurple.get_path()])

def build(bld):
    for path in palm_programs:
        index = path.rfind('-')
        name = path[:index] if index > 0 else path
        bld.program(target=join("bin", name),
                    source=bld.path.ant_glob(join(path, "src/*.cpp")),
                    includes=join(path, "inc"),
                    use="GLIB BASE purple",
                    )
