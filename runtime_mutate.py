import sys
from _ctypes import Py_INCREF
from ctypes import POINTER, c_ushort, c_void_p, py_object
from dis import _inline_cache_entries, _nb_ops, deoptmap
from opcode import HAVE_ARGUMENT, opmap, opname
from types import CodeType

globals().update(opmap)
LOAD_GLOBAL_NULL = 256
LOAD_DEREF_FREE = 257
STORE_DEREF_FREE = 258
DELETE_DEREF_FREE = 259
opname += ["<256>", "LOAD_DEREF_FREE", "STORE_DEREF_FREE", "DELETE_DEREF_FREE"]

binary_ops = [op for _, op in _nb_ops]

OBJHEAD_SIZE = object.__basicsize__
PTR_SIZE = tuple.__itemsize__
SIZE_CodeType = CodeType.__basicsize__

def process_extargs_free(code, old_freevars_start):
    i = 0
    while i < len(code):
        if code[i] is EXTENDED_ARG:
            start = i
            arg = code[i + 1] << 8
            while code[i := i + 2] is EXTENDED_ARG:
                arg = (arg << 8) | (code[i + 1] << 8)
            code[start:i] = []
            i = start
            code[i + 1] |= arg
        if ("DEREF" in (name := opname[opcode := code[i]])
                and opcode is not LOAD_CLASSDEREF
                and code[i + 1] >= old_freevars_start):
            code[i] = globals()[f"{name}_FREE"]
        i += 2
    return code

def mutate(*bytecode):
    f = sys._getframe(1)
    co = f.f_code
    length_bytecode = len(bytecode)
    names = list(co.co_names)
    consts = list(co.co_consts)
    varnames = list(co.co_varnames)
    cellvars = list(co.co_cellvars)
    freevars = list(co.co_freevars)
    old_length_varnames = length_varnames = len(varnames)
    old_length_cellvars = length_cellvars = len(cellvars)
    length_freevars = len(freevars)
    old_freevars_start = old_length_varnames + old_length_cellvars
    idx1 = len(names)
    idx2 = len(consts)
    code = []
    def lplus_handler(arg, names_list, idx, offset,
                      alt_names_list=None, alt_offset=0):
        try:
            try:
                code.append(offset + names_list.index(arg))
            except ValueError:
                if not alt_names_list:
                    raise
                code.append(offset + alt_offset + alt_names_list.index(arg))
                return idx, True
            return idx, False
        except ValueError:
            names_list.append(arg)
            code.append(offset + idx)
            return idx + 1, None
    def name_handler(info, arg):
        nonlocal idx1
        try:
            code.append((names.index(arg) << (info >> 1)) | (info & 1))
        except ValueError:
            names.append(arg)
            code.append((idx1 << (info >> 1)) | (info & 1))
            idx1 += 1
    for opcode, arg in zip(it := iter(bytecode), it):
        if opcode is not LOAD_GLOBAL_NULL:
            opcode = opmap[deoptmap.get(name := opname[opcode], name)] # probably isn't necessary
            code.append(opcode)
        else:
            code.append(LOAD_GLOBAL)
            name_handler(3, arg)
            code.extend(b'\0\0' * _inline_cache_entries[LOAD_GLOBAL])
            continue
        if "FAST" in name or opcode is LOAD_CLOSURE:
            length_varnames, _ = lplus_handler(arg, varnames, length_varnames, 0)
        elif opcode is LOAD_CLASSDEREF:
            length_freevars, _ = lplus_handler(arg, freevars, length_freevars, old_freevars_start)
        elif "DEREF" in name:
            length_cellvars, used_alt = lplus_handler(
                arg, cellvars, length_cellvars, old_length_varnames,
                co.co_freevars, old_freevars_start
            )
            if used_alt:
                code[-2] = globals()[f"{name}_FREE"]
        elif opcode is MAKE_CELL:
            length_cellvars, _ = lplus_handler(arg, cellvars, length_cellvars, old_length_varnames)
        elif opcode is LOAD_GLOBAL:
            name_handler(2, arg)
        elif ("GLOBAL" in name or "NAME" in name or "ATTR" in name
                or "LOAD_METHOD" in name):
            name_handler(0, arg)
        elif opcode is BINARY_OP:
            code.append(binary_ops.index(arg))
        elif opcode is LOAD_CONST:
            try:
                code.append(consts.index(arg))
            except ValueError:
                consts.append(arg)
                code.append(idx2)
                idx2 += 1
        elif opcode < HAVE_ARGUMENT:
            it.__setstate__(length_bytecode - it.__length_hint__() - 1)
            code.append(0)
        else:
            code.append(arg)
        code.extend(b'\0\0' * _inline_cache_entries[opcode])
    lasti = f.f_lasti
    code = (
        process_extargs_free(list(co.co_code[:lasti+2]),
                             old_freevars_start)
      + code
      + process_extargs_free(list(co.co_code[lasti+2:]),
                             old_freevars_start)
    )
    i = 0
    delta_varnames = length_varnames - old_length_varnames
    delta_cellvars = length_cellvars - old_length_cellvars
    while i < len(code):
        opcode, arg = code[i], code[i + 1]
        if "DEREF" in (name := opname[opcode]):
            arg += delta_varnames
            if opcode > 256:
                opcode = globals()[name[:-5]]
                arg += delta_cellvars
            elif opcode is LOAD_CLASSDEREF:
                arg += delta_cellvars
            code[i + 1] = arg
        elif opcode is MAKE_CELL:
            code[i + 1] += delta_varnames
        if x := arg >> 24:
            code.insert(i, EXTENDED_ARG)
            code.insert(i + 1, x)
            i += 2
        if x := arg >> 16:
            code.insert(i, EXTENDED_ARG)
            code.append(i + 1, x & 255)
            i += 2
        if x := arg >> 8:
            code.insert(i, EXTENDED_ARG)
            code.append(i + 1, x & 255)
            i += 2
        code[i + 1] = arg & 255
        i += 2
    co = co.replace(co_code=bytes(code), co_nlocals=length_varnames,
                    co_varnames=tuple(varnames), co_cellvars=tuple(cellvars),
                    co_freevars=tuple(freevars), co_consts=tuple(consts),
                    co_names=tuple(names))
    frame_base_addr = c_void_p.from_address(id(f) + OBJHEAD_SIZE + PTR_SIZE).value
    py_object.from_address(frame_base_addr + PTR_SIZE*4).value = co
    Py_INCREF(co)
    c_void_p.from_address(frame_base_addr + PTR_SIZE*7).value = id(co) + SIZE_CodeType + lasti

if __name__ == "__main__":
    def uhm():
        x = 143
        mutate(
            LOAD_CONST,      7,
            STORE_FAST,      "a",
            LOAD_FAST,       "x",
            COPY,            1,
            LOAD_FAST,       "a",
            BINARY_OP,       "+",
            BINARY_OP,       "*=",
            STORE_FAST,      "x",
        )
        return x

    x = 143
    a = 7
    x *= x + a
    assert uhm() == x, "mutation failed"
