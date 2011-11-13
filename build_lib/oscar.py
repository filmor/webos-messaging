from libpurple import get_protocol_path
from os.path import join
from util import ant_glob

def build(ctx):
    root_path = get_protocol_path("oscar")
    oscar_objects = False

    for name in ("aim", "icq"):
        if name in ctx.env.PROTOCOLS:
            if not oscar_objects:
                ctx.objects(target="oscar",
                            source=ant_glob(ctx, root_path, "*.c",
                                            exclude=["libicq.c", "libaim.c"]
                                           ),
                            includes=root_path,
                            use="PURPLE_BUILD GLIB BASE")
                oscar_objects = True

            ctx.objects(target=name,
                        source=join(root_path, "lib" + name + ".c"),
                        includes=root_path,
                        use="PURPLE_BUILD GLIB BASE oscar")

