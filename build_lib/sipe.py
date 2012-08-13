from util import get_pkg_path_and_version, ant_glob
from os.path import join

def configure(ctx):
    path, version = get_pkg_path_and_version("pidgin-sipe")
    
    ctx.env.SIPE_PATH = join(path, "src")
    ctx.env.SIPE_VERSION = version

    ctx.env.append_value("DEFINES_SIPE_BUILD", "HAVE_CONFIG_H")
    ctx.env.append_value("INCLUDES_SIPE_BUILD", ["pidgin-sipe_config",
                                                 join(ctx.env.SIPE_PATH, "api")])

    headers = ["dlfcn", "inttypes", "langinfo", "locale", "memory", "stdint",
               "stdlib", "strings", "string", "sys/sockio", "sys/stat",
               "sys/types", "unistd"]
    for i in headers:
        ctx.check_cc(header_name=i + ".h", mandatory=False,
                      auto_add_header_name=True)

    ctx.define("ENABLE_NLS", 1)
    ctx.define("HAVE_DCGETTEXT", 1)
    ctx.define("HAVE_GETTEXT", 1)
    ctx.define("HAVE_BIND_TEXTDOMAIN_CODESET", 1)
    ctx.define("STDC_HEADERS", 1)
    ctx.define("PACKAGE_NAME", "pidgin-sipe")
    ctx.define("PACKAGE_STRING", "pidgin-sipe %s" % version)
    ctx.define("PACKAGE_TARNAME", "pidgin-sipe")
    ctx.define("PACKAGE_URL", "http://sipe.sourceforge.net")
    ctx.define("PACKAGE_VERSION", version)
    ctx.define("PACKAGE_BUGREPORT",
                "https://sourceforge.net/tracker/?atid=949931&group_id=194563")
    ctx.define("SIPE_TRANSLATIONS_URL",
                "https://www.transifex.net/projects/p/pidgin-sipe/r/mob/")

    ctx.write_config_header("pidgin-sipe_config/config.h")

def build(ctx):
    ctx.objects(target="libsipe",
                source=ant_glob(ctx, ctx.env.SIPE_PATH, "core", "*.c",
                                exclude=["sip-sec-krb5.c",
                                         "sip-sec-sspi.c",
                                         "sipe-mime.c",
                                         "sipe-win32dep.c"]),
                includes=join(ctx.env.SIPE_PATH, "api"),
                use="GLIB NSS SIPE_BUILD XML"
               )
