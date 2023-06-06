from functools import wraps
from opcode import (_inline_cache_entries as ICE, EXTENDED_ARG, HAVE_ARGUMENT,
                    opmap, stack_effect)
from sys import _getframe

CACHE = opmap['CACHE']
PRECALL = opmap['PRECALL']
LENICE_PRECALL = ICE[PRECALL] * 2
CALL = opmap['CALL']
ICE_CALL = ICE[CALL]
LENICE_CALL = ICE_CALL * 2
MAKE_FUNCTION = opmap['MAKE_FUNCTION']
LOAD_CONST = opmap['LOAD_CONST']
LOAD_BUILD_CLASS = opmap['LOAD_BUILD_CLASS']

# shamelessly copied from my own code

def check_noarg_deco():
    try:
        f = _getframe(2)
    except ValueError:
        return False
    code = f.f_code.co_code
    op = f.f_lasti - LENICE_CALL
    if code[op] is not CALL or code[op + 1]:
        return False
    op -= LENICE_PRECALL + 4
    if code[op] is CACHE:
        ncaches = 0
        while op >= 0:
            while op >= 0 and code[op] is CACHE:
                ncaches += 1
                op -= 2
            if ncaches is ICE_CALL and code[op] is CALL:
                if not code[op + 1]:
                    ncaches = 0
                    op -= LENICE_PRECALL + 4
                    continue
            break
        if (op >= 0 and ncaches is ICE_CALL and code[op] is CALL
                and code[op + 1] >= 2):
            stacklevel = -code[op + 1]
            op -= LENICE_PRECALL + 4
            while op >= 0:
                pos = op - 2
                if code[op] >= HAVE_ARGUMENT:
                    shift = 1
                    oparg = code[op + 1]
                    if pos >= 0 and code[pos] is EXTENDED_ARG:
                        while pos >= 0 and code[pos] is EXTENDED_ARG:
                            oparg += code[pos + 1] << shift
                            shift += 1
                            pos -= 2
                else:
                    oparg = None
                stacklevel += stack_effect(code[op], oparg, jump=False)
                if not stacklevel:
                    break
                op = pos
            else:
                return False
            if (code[op] is LOAD_CONST and code[op + 2] is MAKE_FUNCTION
                    and op - 2 >= 0 and code[op - 2] is LOAD_BUILD_CLASS):
                return True
    elif code[op] is MAKE_FUNCTION:
        pos = op - 4
        shift = 1
        oparg = code[op - 1]
        if pos >= 0 and code[pos] is EXTENDED_ARG:
            while pos >= 0 and code[pos] is EXTENDED_ARG:
                oparg += code[pos + 1] << shift
                shift += 1
                pos -= 2
        codeobj = f.f_code.co_consts[oparg]
        return codeobj is not _getframe(1).f_code
    return False

# some utility functions above

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
