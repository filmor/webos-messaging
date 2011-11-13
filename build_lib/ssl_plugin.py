from libpurple import get_path
from os.path import join

# Only build gnutls-variant for now

def build(ctx):
    source_files = [ join(get_path(), "plugins", "ssl", i + ".c")
                     for i in ("ssl", "ssl-gnutls")
                   ]

    ctx.objects(target="ssl_plugins",
                source=source_files,
                use="GLIB PURPLE_BUILD GNUTLS BASE",
                )

