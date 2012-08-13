from os.path import join
import waflib
from glob import glob
from re import match

def ant_glob(ctx, *args, **kwargs):
    exclude = kwargs.get("exclude", [])
    
    saved_exclude_regs = waflib.Node.exclude_regs
    for i in exclude:
        waflib.Node.exclude_regs += "\n**/" + i
    
    result = ctx.path.ant_glob(join(*args))
    waflib.Node.exclude_regs = saved_exclude_regs

    return result

def get_pkg_path_and_version(name):
    try:
        path = glob("deps/%s-*" % name)[-1]
        version = match("deps/%s-(?P<version>[^/]*)" % name, path).groups()[0]
        return (path, version)
    except IndexError:
        raise RuntimeError("Couldn't find %s sources" % name)

def get_pkg_path(name):
    return get_pkg_path_and_version(name)[0]

