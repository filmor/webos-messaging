
def ant_glob(ctx, *args, **kwargs):
    import waflib
    from os.path import join

    exclude = kwargs.get("exclude", [])
    
    saved_exclude_regs = waflib.Node.exclude_regs
    for i in exclude:
        waflib.Node.exclude_regs += "\n**/" + i
    
    result = ctx.path.ant_glob(join(*args))
    waflib.Node.exclude_regs = saved_exclude_regs

    return result

