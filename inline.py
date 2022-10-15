import builtins
import operator
from dis import dis
from opcode import opmap, _inline_cache_entries as caches

globals().update(opmap)
binary_ops = [operator.add, operator.and_, operator.floordiv, operator.lshift,
              operator.matmul, operator.mul, operator.mod, operator.or_,
              operator.pow, operator.rshift, operator.sub, operator.truediv,
              operator.xor, operator.iadd, operator.iand, operator.ifloordiv,
              operator.ilshift, operator.imatmul, operator.imul,
              operator.imod, operator.ior, operator.ipow, operator.irshift,
              operator.isub, operator.itruediv, operator.ixor]

FunctionType = type(lambda: 0)

def _check_function(obj):
    assert isinstance(obj, FunctionType), "not a function"

def _process(co_code, to=False):
    i = 0
    if not to:
        while i < len(co_code):
            index_skip = False
            if co_code[i] is EXTENDED_ARG:
                arg = co_code[-~i]
                del co_code[i], co_code[i]
                co_code[-~i] += arg * 256
                index_skip = True
            if co_code[i] is CACHE:
                del co_code[i], co_code[i]
            elif not index_skip:
                i += 2
    else:
        while i < len(co_code):
            arg = co_code[-~i]
            if arg > 0xffffff:
                co_code[i:i+2] = (EXTENDED_ARG, arg >> 24,
                    EXTENDED_ARG, arg>>16 & 255, EXTENDED_ARG, arg>>8 & 255,
                    co_code[i], arg & 255)
                i += 6
            elif arg > 0xffff:
                co_code[i:i+2] = (EXTENDED_ARG, arg >> 16,
                    EXTENDED_ARG, arg>>8 & 255, co_code[i], arg & 255)
                i += 4
            elif arg > 0xff:
                co_code[-~i] = arg & 255
                co_code.insert(i, arg >> 8)
                co_code.insert(i, EXTENDED_ARG)
                i += 2
            if num_cache := caches[co_code[i]]:
                co_code[i:i+2] += [CACHE, 0] * num_cache
                i += num_cache + num_cache
            i += 2
    return co_code

def _replace_const(code, const, value):
    if const in (co_consts := code.co_consts) and const != value:
        idx = co_consts.index(const)
        res = co_consts[:idx] + (value,) + co_consts[-~idx:]
        assert const not in res
        code = code.replace(co_consts=res)
    return code

def _inline_var(code, name, value, check_store=0, co_code=None):
    assign_allowed = check_store & 2
    if value in (co_consts := code.co_consts):
        idx = co_consts.index(value)
    co_code = co_code or bytearray(code.co_code)
    idx = len(co_consts)
    const_idx = idx + 1
    co_consts += (value,)
    localsplus = code.co_varnames + code.co_cellvars + code.co_freevars
    co_names = code.co_names
    i = 0
    while i < len(co_code):
        inst, arg = co_code[i], co_code[-~i]
        if ((inst is LOAD_FAST or inst is LOAD_CLOSURE
                    or inst is LOAD_DEREF or inst is LOAD_CLASSDEREF)
                and localsplus[arg] == name
                or
                inst is LOAD_NAME and co_names[arg] == name):
            co_code[i], co_code[-~i] = LOAD_CONST, idx
        elif inst is LOAD_GLOBAL and co_names[arg >> 1] == name:
            if arg & 1:
                co_code.insert(i, 0)
                co_code.insert(i, PUSH_NULL)
                i += 2
            co_code[i], co_code[-~i] = LOAD_CONST, idx
        elif check_store and ((C := inst is STORE_FAST or inst is STORE_DEREF)
                and localsplus[arg] == name
                or
                (inst is STORE_GLOBAL or inst is STORE_NAME)
                and co_names[arg] == name):
            if assign_allowed:
                prev_inst = co_code[i - 2]
                arg_idx = ~-i
                if prev_inst is COPY and co_code[arg_idx] == 1:
                    prev_inst = co_code[i - 4]
                    arg_idx = i - 3
                if prev_inst is LOAD_CONST:
                    value = co_consts[idx := co_code[arg_idx]]
                    if C:
                        del co_code[i - 2:i + 2]
                        i -= 2
                else:
                    break
            else:
                break
        elif (inst is BINARY_OP
                and co_code[i - 2] is co_code[i - 4] is LOAD_CONST):
            res = binary_ops[arg](
                co_consts[co_code[i - 3]],
                co_consts[co_code[~-i]],
            )
            if res in co_consts:
                _const_idx = co_consts.index(res)
            else:
                co_consts += (res,)
                _const_idx = const_idx
                const_idx += 1
            co_code[i - 3:i + 2] = [_const_idx]
            i -= 4
        i += 2
    return co_code, co_consts

def _inline_builtins(code, check_store=0, co_code=None,
                     builtins_dict=__import__('builtins').__dict__):
    if check_store:
        builtins_dict = builtins_dict.copy()
        assign_allowed = check_store & 2
    else:
        assign_allowed = False
    co_code = co_code or bytearray(code.co_code)
    idx = len(co_consts := code.co_consts)
    localsplus = code.co_varnames + code.co_cellvars + code.co_freevars
    co_names = code.co_names
    i = 0
    while i < len(co_code):
        inst, arg = co_code[i], co_code[-~i]
        if inst is LOAD_GLOBAL and (name := co_names[arg >> 1]) in builtins_dict:
            value = builtins_dict[name]
            co_code[i] = LOAD_CONST
            if value in co_consts:
                co_code[-~i] = co_consts.index(value)
            else:
                co_consts += (builtins_dict[name],)
                co_code[-~i] = idx
                idx += 1
            if arg & 1:
                co_code.insert(i, 0)
                co_code.insert(i, PUSH_NULL)
                i += 2
        elif (inst is STORE_GLOBAL and check_store
                    and (name := co_names[arg]) in builtins_dict):
                if assign_allowed:
                    prev_inst = co_code[i - 2]
                    arg_idx = ~-i
                    if prev_inst is COPY and co_code[arg_idx] == 1:
                        prev_inst = co_code[i - 4]
                        arg_idx = i - 3
                    if prev_inst is LOAD_CONST:
                        builtins_dict[name] = co_consts[co_code[arg_idx]]
                        #del co_code[i - 2:i + 2]
                        #i -= 2
                    else:
                        del builtins_dict[name]
                else:
                    del builtins_dict[name]
        elif (inst is BINARY_OP
                and co_code[i - 2] is co_code[i - 4] is LOAD_CONST):
            res = binary_ops[arg](
                co_consts[co_code[i - 3]],
                co_consts[co_code[~-i]],
            )
            if res in co_consts:
                const_idx = co_consts.index(res)
            else:
                co_consts += (res,)
                const_idx = idx
                idx += 1
            co_code[i - 3:i + 2] = [const_idx]
            i -= 4
        i += 2
    return co_code, co_consts

def _inline_globals(code, glob, check_store=0, co_code=None):
    if check_store:
        glob = glob.copy()
        assign_allowed = check_store & 2
    else:
        assign_allowed = False
    co_code = co_code or bytearray(code.co_code)
    idx = len(co_consts := code.co_consts)
    localsplus = code.co_varnames + code.co_cellvars + code.co_freevars
    co_names = code.co_names
    i = 0
    while i < len(co_code):
        inst, arg = co_code[i], co_code[-~i]
        if inst is LOAD_GLOBAL and (name := co_names[arg >> 1]) in glob:
            value = glob[name]
            co_code[i] = LOAD_CONST
            if value in co_consts:
                co_code[-~i] = co_consts.index(value)
            else:
                co_consts += (glob[name],)
                co_code[-~i] = idx
                idx += 1
            if arg & 1:
                co_code.insert(i, 0)
                co_code.insert(i, PUSH_NULL)
                i += 2
        elif (inst is STORE_GLOBAL and check_store
                    and (name := co_names[arg]) in glob):
                if assign_allowed:
                    prev_inst = co_code[i - 2]
                    arg_idx = ~-i
                    if prev_inst is COPY and co_code[arg_idx] == 1:
                        prev_inst = co_code[i - 4]
                        arg_idx = i - 3
                    if prev_inst is LOAD_CONST:
                        glob[name] = co_consts[co_code[arg_idx]]
                        #del co_code[i - 2:i + 2]
                        #i -= 2
                    else:
                        del glob[name]
                else:
                    del glob[name]
        elif (inst is BINARY_OP
                and co_code[i - 2] is co_code[i - 4] is LOAD_CONST):
            res = binary_ops[arg](
                co_consts[co_code[i - 3]],
                co_consts[co_code[~-i]],
            )
            if res in co_consts:
                const_idx = co_consts.index(res)
            else:
                co_consts += (res,)
                const_idx = idx
                idx += 1
            co_code[i - 3:i + 2] = [const_idx]
            i -= 4
        i += 2
    return co_code, co_consts

def _create_func(f, new_code):
    result = FunctionType(
        new_code,
        f.__globals__,
        f.__name__,
        f.__defaults__,
        f.__closure__,
    )
    result.__kwdefaults__ = f.__kwdefaults__
    result.__orig_func__ = f
    return result

def replace_const(const, value):
    def _inner(obj):
        if isinstance(obj, FunctionType):
            return _create_func(obj, _replace_const(obj.__code__, const, value))
        return value if obj == const else obj
    return _inner

def inline_var(name, value, *, check_store=0, inline_args=True):
    def _inner(func):
        _check_function(func)
        code = func.__code__
        if (not inline_args
                and name in code.co_varnames[:code.co_argcount
                                             + code.co_kwonlyargcount
                                             + (code.co_flags & 4 != 0)
                                             + (code.co_flags & 8 != 0)]):
            return func
        co_code = [*code.co_code]
        _process(co_code)
        _, co_consts = _inline_var(code, name, value, check_store, co_code)
        _process(co_code, to=True)
        return _create_func(func, code.replace(co_code=bytes(co_code),
                                               co_consts=co_consts))
    return _inner

# TODO: better inline_method/inline_attr

def inline_builtins(func=None, /, *, check_store=0):
    def _inner(func):
        _check_function(func)
        code = func.__code__
        co_code = [*code.co_code]
        _process(co_code)
        _, co_consts = _inline_builtins(code, check_store, co_code)
        _process(co_code, to=True)
        return _create_func(func, code.replace(co_code=bytes(co_code),
                                               co_consts=co_consts))
    return _inner if func is None else _inner(func)

def inline_globals(func=None, /, *, check_store=0):
    def _inner(func):
        _check_function(func)
        code = func.__code__
        co_code = [*code.co_code]
        _process(co_code)
        _, co_consts = _inline_globals(code, func.__globals__, check_store, co_code)
        _process(co_code, to=True)
        return _create_func(func, code.replace(co_code=bytes(co_code),
                                               co_consts=co_consts))
    return _inner if func is None else _inner(func)

def cmp(f):
    print(f'Comparing {f.__qualname__}...')
    orig = f
    while hasattr(orig,'__orig_func__'):
        orig = orig.__orig_func__
 
    if orig is f:
        print('No difference')
        return f
 
    print('Original code:')
    dis(orig)
    print('\nOptimized code:')
    dis(f)
    print('\n')
    return f

builtins.cmp = cmp

if __name__ == '__main__':
    s = 5

    @inline_globals(check_store=2)
    @inline_var('c', 5, check_store=2, inline_args=False)
    def g(a, b):
        global s
        print(s)
        s += 7
        print(s)
        c += 2
        res = a + b + c
        c += a
        res2 = a + b + c
        return res, res2

    dis(g)
