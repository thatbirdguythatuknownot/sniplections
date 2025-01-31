from check_noarg_deco import check_noarg_deco
from functools import wraps
from opcode import opmap

fast_to_name = {}
for x in 'LOAD', 'STORE', 'DELETE':
    fast_to_name[opmap[opname := f"{x}_FAST"]] = opmap[f"{x}_NAME"]
    globals()[opname] = opmap[opname]

def make(*args, **kwargs):
    def wrapper(fn):
        # you better not have an EXTENDED_ARG
        setted = {*(names := [*fn.__code__.co_names])}
        idx_cvt = [None] * len(fn.__code__.co_varnames)
        for i, name in enumerate(fn.__code__.co_varnames):
            if name not in setted:
                idx_cvt[i] = len(names)
                names.append(name)
                setted.add(name)
            elif idx_cvt[i] is None:
                idx_cvt[i] = names.find(name)
        code_ = bytearray(fn.__code__.co_code)
        for i, opcode in enumerate(code_):
            if (opcode is STORE_FAST or opcode is LOAD_FAST
                    or opcode is DELETE_FAST):
                code_[i] = fast_to_name[opcode]
                code_[i + 1] = idx_cvt[code_[i + 1]]
        fn.__code__ = fn.__code__.replace(co_code=bytes(code_),
                                          co_names=tuple(names))
        cls = __build_class__(fn, fn.__name__, *args, **kwargs)
        return wraps(fn)(lambda *A, **K: cls(*A, **K))
    if check_noarg_deco():
        fn = args[0]
        args = ()
        return wrapper(fn)
    return wrapper
