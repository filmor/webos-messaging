from glob import glob
from util import ant_glob
from os.path import join

VERSION = "2.1.23"

def get_path():
    try:
        return glob("deps/cyrus-sasl-*/")[-1]
    except IndexError:
        raise RuntimeError("Couldn't find cyrus-sasl sources")

def configure(ctx):
    ctx.env.append_value("INCLUDES_SASL_BUILD", [join(get_path(), "include"),
                                                 "./sasl_build"]
                        )

    ctx.env.append_value("CFLAGS_SASL_BUILD", ["-fPIC"])

    check = lambda **kwargs: ctx.check_cc(uselib_store="SASL_BUILD", **kwargs)

    ctx.define("_GNU_SOURCE", 1)
    ctx.define("HIER_DELIMITER", '/')
    ctx.define("PACKAGE", "cyrus-sasl")
    ctx.define("MAXHOSTNAMELEN", 255)
    ctx.define("SASL_PATH_ENV_VAR", "SASL_PATH")
    ctx.define("SASL_CONF_PATH_ENV_VAR", "SASL_CONF_PATH")
    ctx.define("CONFIGDIR", "")
    ctx.define("PLUGINDIR", "")
    ctx.define("VERSION", VERSION)

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

def build(ctx):
    exclude = [
            "windlopen.c", "dlopen.c", "getaddrinfo.c", "getnameinfo.c",
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

    ctx.objects(target="sasl",
                source=ant_glob(ctx, get_path(), "lib", "*.c",
                                exclude=exclude),
                includes=join(get_path(), "include"),
                export_includes=".",
                use="BASE SASL_BUILD",
               )
                
