import json
import sys
import string

def substitute(template, subs):
    if type(template) is str or type(template) is unicode:
        return string.Template(template).substitute(subs)
    elif type(template) is list:
        return [ substitute(i, subs) for i in template ]
    elif type(template) is dict:
        return dict([ (key, substitute(template[key], subs)) for key in template ])
    else:
        return template


def merge_into(template, vals):
    substitutions = {}

    for key in vals:
        if not key.startswith("@"):
            substitutions[key] = vals[key]

    template.update(vals.get("@override", {}))

    # TODO: @include
    return substitute(template, substitutions)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(-1)

    tmpl = json.load(open(sys.argv[1]))
    vals = json.load(open(sys.argv[2]))
    print json.dumps(merge_into(tmpl, vals), indent=4)
