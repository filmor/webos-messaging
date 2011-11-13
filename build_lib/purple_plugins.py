from libpurple import get_path
from os.path import join

def build(bld):
    plugins = [
            "autoaccept",
            "idle",
            "joinpart",
            "log_reader",
            "newline",
            "offlinemsg",
            "psychic",
        ]

    plugin_paths = [
            join(get_path(), "plugins", i + ".c")
            for i in plugins
            ]

    bld.objects(target="purple_plugins",
                source=plugin_paths,
                includes=get_path(),
                use="PURPLE_BUILD GLIB XML",
               )
