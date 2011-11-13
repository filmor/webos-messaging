from libpurple import get_protocol_path
from os.path import join

def build(bld):
    if "msn" in bld.env.PROTOCOLS:
        bld.objects(
                target="msn",
                source=bld.path.ant_glob(join(get_protocol_path("msn"), "*.c")),
                defines=["PACKAGE_NAME=\"msn\""],
                use="PURPLE_BUILD GLIB BASE"
                )
