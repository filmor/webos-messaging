from os.path import join
from util import ant_glob, get_pkg_path_and_version, get_pkg_path

def configure(ctx):
    ctx.load("sipe account", tooldir=ctx.env.TOOLDIR)

def _get_accounts(name):
    if name == "jabber":
        return ("jabber", "google_talk", "facebook")
    else:
        return (name,)

def build(ctx):
    ctx.load("account", tooldir="build_lib")
    for name in "msn icq jabber novell yahoo sipe sametime".split(" "):
        use = ["BASE", "GLIB", "XML", "GNUTLS", "PURPLE_BUILD"]
        root_path = join(ctx.env.PURPLE_PATH, "protocols")
        icon_path = join(ctx.env.PIDGIN_PATH, "pidgin", "pixmaps", "protocols")
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
            exclude += ["purple-media.c", "tests*.c"]
            path = "purple"
            use = ["SIPE_BUILD",  "libsipe"] + use
            icon_path = join(ctx.env.SIPE_PATH, "pixmaps")

        elif name == "sametime":
            mw_root, mw_version = get_pkg_path_and_version("meanwhile")
            ctx.objects(target="meanwhile",
                        source=ant_glob(ctx, mw_root, "src", "**", "*.c"),
                        use="GLIB"
                       )
            includes.append(join(mw_root, "src"))
            use = ["meanwhile"] + use

        root_path = join(root_path, path if path else name)
        ctx.shlib(target="protocols/%s" % name,
                  source=ant_glob(ctx, root_path, "**", "*.c", exclude=exclude),
                  includes=[root_path] + includes,
                  use=" ".join(use),
                  install_path="${PLUGIN_IPKG_PATH}"
                 )

        for n in _get_accounts(name):
            ctx(
                    features="account",
                    target="accounts",
                    source="accounts/%s.json" % n,
                    install_path="/usr/palm/accounts",
                    prototype="accounts/prototype.json",
                    defines={"icon_path": icon_path}
                )

