from os.path import join, basename
import libpurple

def options(opt):
    opt.load('compiler_cxx')

def configure(conf):
    conf.load('compiler_cxx')

    conf.env.append_value("LIB_PALM_BUILD", ["lunaservice", "mojoluna",
                                  "mojocore", "cjson", "mojodb"])
    conf.env.append_value("DEFINES_PALM_BUILD", ["MOJ_LINUX", "DEVICE"])
    conf.env.append_value("INCLUDES_PALM_BUILD", [libpurple.get_path()])

def build(bld):
    source_dir = "src"
    av_glob = join(source_dir, "IMAccountValidator*.cpp")
    pt_glob = join(source_dir, "*.cpp")

    bld.program(target=join("bin", "imaccountvalidator"),
                source=bld.path.ant_glob(av_glob),
                use="GLIB BASE PALM_BUILD purple",
               )

    bld.program(target=join("bin", "imlibpurpletransport"),
                source=bld.path.ant_glob(pt_glob, excl=[av_glob]),
                use="GLIB BASE PALM_BUILD purple"
               )

