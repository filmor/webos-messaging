from os.path import join
from util import ant_glob, get_pkg_path_and_version

def configure(ctx):
    ctx.load("sipe", tooldir="build_lib")

def build(ctx):
    for name in ctx.options.protocols.split(","):
        use = ["BASE", "GLIB", "XML", "GNUTLS", "PURPLE_BUILD"]
        root_path = join(ctx.env.PURPLE_PATH, "protocols")
        exclude = ["win32"]
        includes = []
        path = None
        if name == "jabber":
            # bld.load("sasl", tooldir="build_lib")
            # use += ["sasl", "SASL_BUILD"]
            exclude += ["auth_cyrus.c"]
        elif name == "icq":
            exclude += ["libaim.c"]
            path = "oscar"
        elif name == "aim":
            exclude += ["libicq.c"]
            path = "oscar"
        elif name == "yahoo":
            exclude += ["libyahoojp.c"]

        elif name == "sipe":
            ctx.load("sipe", tooldir="build_lib")
            root_path = join(ctx.env.SIPE_PATH, "src")
            exclude += ["purple-media.c"]
            path = "purple"
            use += ["SIPE_BUILD"]

        elif name == "sametime":
            mw_root, mw_version = get_pkg_path_and_version("meanwhile")
            ctx.objects(target="obj_meanwhile",
                        source=ant_glob(ctx, mw_root, "src", "**", "*.c"),
                        use="GLIB"
                       )
            includes.append(join(mw_root, "src"))
            use.append("obj_meanwhile")

        root_path = join(root_path, path if path else name)
        ctx.shlib(target="protocols/%s" % name,
                  source=ant_glob(ctx, root_path, "**", "*.c", exclude=exclude),
                  includes=[root_path] + includes,
                  use=" ".join(use),
                  install_path="${PLUGIN_IPKG_PATH}"
                 )
