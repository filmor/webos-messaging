#!/usr/bin/env python

import json
import sys
import string
import subprocess
from os import path, extsep, makedirs
from shutil import copyfile

def _get_substitutions(vals):
    return dict([(key, vals[key]) for key in vals if not key.startswith("@")])


def _recursive_update(to_dict, from_dict):
    for key, val in from_dict.iteritems():
        if type(val) is dict:
            assert type(to_dict.get(key, {})) is dict
            new_to_dict = to_dict.setdefault(key, {})
            _recursive_update(new_to_dict, val)

        elif type(val) is list:
            l = to_dict.get(key, [])
            assert type(l) is list
            
            l += val
            if len(l):
                to_dict[key] = l

        else:
            to_dict[key] = val

def _apply_subs(val, subs):
    if type(val) is list:
        return [_apply_subs(i, subs) for i in val]
    elif type(val) is dict:
        return dict((key, _apply_subs(val[key], subs)) for key in val)
    elif isinstance(val, basestring):
        return string.Template(val).substitute(subs)
    else:
        return val


CONVERSION_RULE = 'convert "{from_path}" "{to_path}"'
SIZE_CONVERSION_RULE = 'convert -geometry {size}x{size} "{from_path}" "{to_path}"'

def _convert(from_path, to_path, size, type):
    if type == 'png' and size == None:
        copyfile(from_path, to_path)

    rule = CONVERSION_RULE if size == None else SIZE_CONVERSION_RULE
    cmd = rule.format(from_path=from_path, to_path=to_path, size=size)

    if subprocess.call(cmd, shell=True):
        raise OSError("Couldn't convert")

class Context(object):
    def __init__(self, filename, includes=[], substitutions={}):
        self._data = {}
        self._subs = substitutions
        self._images = []

        obj = json.load(open(filename))

        includes += obj.get("@include", [])
        includes += [filename]

        for i in includes:
            try:
                json_file = open(i)
            except IOError:
                json_file = open(path.join(path.dirname(filename), i))
            inc_obj = json.load(json_file)

            self._subs.update(_get_substitutions(inc_obj))

            keys = [i for i in inc_obj if i.startswith("@") and i != "@include"]

            for key in keys:
                # Strip off '@'
                name = key[1:]
                if name in self._data:
                    _recursive_update(self._data[name], inc_obj[key])
                else:
                    self._data[name] = inc_obj[key]
    
    def do_subs(self, substitutions={}):
        self._subs.update(substitutions)

        for key in self._subs:
            for i in xrange(20):
                val = string.Template(self._subs[key]).safe_substitute(self._subs)
                self._subs[key] = val
                # TODO: Handle $$
                if not "$" in val:
                    break

            if "$" in val:
                raise RuntimeError("Too many recursions")
        
    def do_icons(self):
        d = self._data.setdefault("icons", {})
        self._data["icons"] = _apply_subs(d, self._subs)

        for key, val in self._data["icons"].iteritems():
            self._subs[key] = val["name"]
            self._images.append((
                    val["path"],
                    val["name"],
                    val.get("size", None),
                    val.get("type", path.splitext(val["path"])[1][1:])
                ))

    def write(self, target, *args, **kwargs):
        self.do_subs()
        self.do_icons()

        output = _apply_subs(self._data["template"], self._subs)

        template_id = output["templateId"]
        out = path.join(target, template_id)

        for from_path, to_path, size, type in self._images:
            try:
                makedirs(path.join(out, path.dirname(to_path)))
            except OSError:
                pass

            print "Processing image %s" % (to_path,)
            _convert(from_path, path.join(out, to_path), size, type)

        # TODO: Translations
        out_file = path.join(out, template_id + ".json")
        json.dump(output, open(out_file, "w"), *args, **kwargs)

def create_account(name, out, pidgin_path, proto="prototype.json"):
    ctx = Context(name, includes=[proto],
                  substitutions={"pidgin_path": pidgin_path})
    ctx.write(out, indent=4, sort_keys=True)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description="Create account files")
    parser.add_argument('pidgin')
    parser.add_argument('output')
    parser.add_argument('filename')
    parser.add_argument('prototype')
    args = parser.parse_args()
    create_account(args.filename, args.output, args.pidgin, args.prototype)
