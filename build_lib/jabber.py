from libpurple import get_protocol_path
from util import ant_glob

def build(ctx):
    root_path = get_protocol_path("jabber")
    if "jabber" in ctx.env.PROTOCOLS:
        ctx.objects(target="jabber",
                    source=ant_glob(ctx, root_path, "**", "*.c",
                              exclude=["auth_cyrus.c", "win32"]),
                    includes=root_path,
                    defines=["PACKAGE=\"jabber\""],
                    use="GLIB BASE XML PURPLE_BUILD sasl")

