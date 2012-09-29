from os.path import join
import waflib
from glob import glob
from re import match
from json import load

manifest = load(open("manifest.json"))

_prototype = {
        "output": "deps/",
        "name_version": "{name}-{version}",
        "root_path": "{name_version}",
        "filename": "{name_version}.{type}",
        "sf": "http://sourceforge.net/projects",
        "gh": "https://github.com/downloads",
        "version": ""
}

for name in manifest:
    d = dict(_prototype)
    d.update(manifest[name])
    d["name"] = name

    for i in ("root_path", "output", "url", "filename"):
        value = d[i]
        for j in xrange(10):
            new_val = value.format(**d)
            if new_val == value:
                break
            else:
                value = new_val

        d[i] = value

    manifest[name] = d

def get_libs(ctx):
    from waflib import Logs
    import tarfile

    for name in manifest:
        mf = manifest[name]
        filename = join("download", mf["filename"])

        if not os.path.exists(filename):
            from urllib import urlretrieve
            Logs.info("d: Retrieving {name} from {url}".format(**mf))
            out, msg = urlretrieve(mf["url"], filename)

        Logs.info("x: Unpacking {filename}".format(**mf[name]))
        tar = tarfile.open(filename)
        tar.extractall(mf["output"])


def get_pkg_path_and_version(name):
    try:
        mf = manifest[name]
        path = "{output}/{root_path}".format(**mf)
        version = mf["version"]
        return (path, version)
    except IndexError:
        raise RuntimeError("Couldn't find %s in the manifest" % name)

def get_pkg_path(name):
    return get_pkg_path_and_version(name)[0]

def ant_glob(ctx, *args, **kwargs):
    exclude = kwargs.get("exclude", [])
    
    saved_exclude_regs = waflib.Node.exclude_regs
    for i in exclude:
        waflib.Node.exclude_regs += "\n**/" + i
    
    result = ctx.path.ant_glob(join(*args))
    waflib.Node.exclude_regs = saved_exclude_regs

    return result

