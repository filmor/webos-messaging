from glob import glob
from util import ant_glob
from os.path import join

VERSION = "1.0.2"

def get_path():
    try:
        return glob("deps/meanwhile-*/")[-1]
    except IndexError:
        raise RuntimeError("Couldn't find meanwhile sources")
    
def configure(ctx):
    ctx.env.append_value("INCLUDES_MEANWHILE_BUILD", [join(get_path(), "include"),
                                                 "./sasl_build"]
                        )

    ctx.define("_GNU_SOURCE", 1)
    ctx.define("HIER_DELIMITER", '/')
    ctx.define("PACKAGE", "cyrus-sasl")
    ctx.define("MAXHOSTNAMELEN", 255)
    ctx.define("SASL_PATH_ENV_VAR", "SASL_PATH")
    ctx.define("SASL_CONF_PATH_ENV_VAR", "SASL_CONF_PATH")
    ctx.define("CONFIGDIR", "")
    ctx.define("PLUGINDIR", "")
    ctx.define("VERSION", VERSION)

    for p in plugins:
        if p == "cram":
            p = "crammd5"
        elif p == "gssapi":
            p = "gssapiv2"

        ctx.define("STATIC_%s" % p.upper(), 1, quote=False)

    headers = [
            "dirent", "dlfcn", "fcntl", "gssapi", "inttypes", "limits",
            "malloc", "memory", "ndir", "paths", "stdarg", "stdint", "stdlib",
            "strings", "string", "sysexits", "syslog", "sys/dir", "sys/file",
            "sys/ndir", "sys/param", "sys/stat", "sys/time", "sys/types",
            "sys/uio", "sys/wait", "unistd", "varargs", "netdb"
            ]

    for i in headers:
        ctx.check_cc(uselib_store="SASL_BUILD", header_name=i + ".h",
                     auto_add_header_name=True, mandatory=False)

    functions = [
            "getaddrinfo", "getdomainname", "gethostname", "getnameinfo",
            "printf", 
            ]

    for i in functions:
        ctx.check_cc(uselib_store="SASL_BUILD", function_name=i,
                     mandatory=False)

    ctx.env.include_key = ["netinet/in.h", "stdlib.h", "sys/types.h",
                           "sys/socket.h", "string.h", "netdb.h", "stdio.h"]
    ctx.write_config_header("sasl_build/config.h", headers=True)
    ctx.env.include_key = []

def build(ctx):
    exclude = [
            "windlopen.c", "getaddrinfo.c", "getnameinfo.c",
            "snprintf.c"
            ]

    def create_symlink(ctx):
        from os import symlink
        try:
            symlink(join("..", get_path(), "include"), "build/sasl")
        except OSError as e:
            if e.errno == 17:
                pass
            else:
                raise

    # HACK!
    ctx.add_pre_fun(create_symlink)

    ctx.objects(target="sasl_plugins",
                source=[join(get_path(), "plugins", i + ".c")
                        for i in plugins + ["plugin_common"]],
                includes=join(get_path(), "include"),
                use="BASE SASL_BUILD"
               )

    ctx.objects(target="sasl",
                source=ant_glob(ctx, get_path(), "lib", "*.c",
                                exclude=exclude),
                includes=join(get_path(), "include"),
                export_includes=".",
                use="BASE SASL_BUILD sasl_plugins",
               )
                
# TODO: Custom cleaner!
def clean(ctx):
    pass
