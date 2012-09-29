from build_lib.util import get_pkg_path, join

top="."
VERSION='0.9.4'
APPNAME='org.webosinternals.purple'

TOOLDIR='build_lib'

PALM_PREFIX='org.webosinternals.'

def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.load('libpurple palm_programs', tooldir=TOOLDIR)

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

    conf.env.PURPLE_PLUGINS = plugins
    conf.env.PURPLE_SSL = "gnutls"

    conf.env.PALM_APP_PREFIX = "/media/cryptofs/apps"
    conf.env.APP_IPKG_PATH = "/usr/palm/applications/org.webosinternals.purple"
    conf.env.PLUGIN_IPKG_PATH = join(conf.env.APP_IPKG_PATH, "plugins")
    conf.env.SYSTEM_IPKG_PATH = join(conf.env.APP_IPKG_PATH, "system")

    conf.env.APP_PATH = join(conf.env.PALM_APP_PREFIX,
                             conf.env.APP_IPKG_PATH[1:])
    conf.env.PLUGIN_PATH = join(conf.env.PALM_APP_PREFIX,
                                conf.env.PLUGIN_IPKG_PATH[1:])

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
    bld.install_files("${APP_IPKG_PATH}", start_dir.ant_glob("share/ca-certs/**/*"),
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

def get_libs(ctx):
    ctx.load('util', tooldir=TOOLDIR)

    from waflib import Logs
    import json, os, urllib, tarfile, os.path
    
    manifest = json.load(open("manifest.json"))

    prototype = {
            "output": "deps/",
            "name_version": "{name}-{version}",
            "filename": "{name_version}.{type}",
            "sf": "http://sourceforge.net/projects",
            "gh": "https://github.com/downloads"
    }

    for name in manifest:
        d = dict(prototype)
        d.update(manifest[name])
        d["name"] = name

        for i in ("output", "url", "type", "filename"):
            value = d[i]
            for j in xrange(10):
                new_val = value.format(**d)
                if new_val == value:
                    break
                else:
                    value = new_val

            d[i] = value

        manifest[name] = d

    dl_dir = "download"

    try:
        os.makedirs(dl_dir)
    except:
        pass

    for name in manifest:
        filename = os.path.join(dl_dir, manifest[name]["filename"])

        if not os.path.exists(filename):
            Logs.info("d: Retrieving {name} from {url}".format(**manifest[name]))
            out, msg = urllib.urlretrieve(manifest[name]["url"], filename)

        Logs.info("x: Unpacking {filename}".format(**manifest[name]))
        tar = tarfile.open(filename)
        tar.extractall(manifest[name]["output"])

