import json
import sys
import string
import subprocess
from os import path, extsep, makedirs

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



class Context(object):
    def __init__(self, filename, includes=[], substitutions={}):
        self._data = {}
        self._subs = substitutions
        self._images = []

        obj = json.load(open(filename))

        self._includes = includes + obj.get("@include", [])
        includes = self._includes + [filename]

        for i in includes:
            try:
                json_file = open(i)
            except IOError:
                json_file = open(path.join(path.dirname(filename), i))
            inc_obj = json.load(json_file)

            self._subs.update(
                dict([
                        (key, inc_obj[key])
                        for key in inc_obj
                        if not key.startswith("@")
                     ])
                )

            keys = [i for i in inc_obj if i.startswith("@") and i != "@include"]

            for key in keys:
                # Strip off '@'
                name = key[1:]
                if name in self._data:
                    _recursive_update(self._data[name], inc_obj[key])
                else:
                    self._data[name] = inc_obj[key]
        
        self._do_subs()
        self._do_icons()

    def get_images(self):
        return self._images

    def get_includes(self):
        return self._includes
    
    def _do_subs(self, substitutions={}):
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
        
    def _do_icons(self):
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

    def template_id(self):
        output = _apply_subs(self._data["template"], self._subs)
        return output["templateId"]

    def write(self, target):
        output = _apply_subs(self._data["template"], self._subs)
        template_id = output["templateId"]

        # TODO: Translations
        json.dump(output, open(target, "w"), indent=4, sort_keys=True)

from waflib import Utils, Task, Errors, Logs, Node
from waflib.Configure import conf
from Task import task_type_from_func, simple_task_type
from waflib.TaskGen import feature, before_method
from shutil import copyfile

def configure(self):
    self.find_program("convert", var="CONVERT")
    self.find_program("cp", var="COPY")

def build_account(self):
    self.context.write(self.outputs[0].abspath())
    return 0
task_type_from_func('account', func=build_account)

def do_copy(self):
    copyfile(inputs[0].abspath(), outputs[0].abspath())
task_type_from_func('copy_file', func=do_copy)
task_type_from_func('convert_image',
                 func="${CONVERT} ${CONVERT_FLAGS} ${SRC} ${TGT}")

@feature("account")
@before_method("process_source")
def init_account_task(self):
    proto = getattr(self, 'prototype', None)
    includes = [proto] if proto else []
    defines = getattr(self, 'defines', {})
    node = self.path.find_resource(self.source)
    install_path = getattr(self, 'install_path', None)

    if not node:
        self.bld.fatal("Could not find source %s" % self.source)

    # Bypass execution of process_source
    self.meths.remove('process_source')

    out_node = self.path.find_or_declare([self.target])

    tsk = self.create_task('account', node)
    tsk.install_path = install_path
    tsk.context = Context(node.abspath(),
                          includes=includes,
                          substitutions=defines)

    template_id = tsk.context.template_id()
    account_node = out_node.find_or_declare([template_id])
    account_node.find_or_declare(["images"]).mkdir()
    json_node = account_node.find_or_declare([template_id + ".json"])
    tsk.outputs = [json_node]

    install_files = [json_node]
    for from_path, to_path, size, type in tsk.context.get_images():
        from_node = self.path.find_resource([from_path])
        to_node = account_node.find_or_declare([to_path])
        ct = None
        if type != 'png' or size != None:
            ct = self.create_task('convert_image', from_node, [to_node])
            if size != None:
                ct.env.CONVERT_FLAGS = ["-geometry", "%sx%s" % (size, size)]
        else:
            ct = self.create_task('copy_file', from_node, [to_node])
        tsk.set_run_after(ct)
        install_files += [to_node]

    self.bld.install_files(path.join(install_path, template_id), install_files)

