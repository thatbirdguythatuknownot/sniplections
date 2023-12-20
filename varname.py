from collections import deque
from dis import _inline_cache_entries as _ICE, opmap
from sys import _getframe as _out, version_info

globals().update({
    x: opmap.get(x, -1)
    for x in
        "LOAD_NAME LOAD_FAST LOAD_DEREF LOAD_CLASSDEREF LOAD_GLOBAL "
        "LOAD_ATTR CALL PRECALL EXTENDED_ARG KW_NAMES "
        "LOAD_FROM_DICT_OR_GLOBALS LOAD_FAST_CHECK LOAD_FAST_AND_CLEAR "
        "LOAD_FROM_DICT_OR_DEREF LOAD_SUPER_ATTR"
        .split()
})
if version_info == (3, 11):
    PRECALL_HEADER = 1 + _ICE[PRECALL]
else:
    PRECALL_HEADER = 0

ARG_OFFSET = (PRECALL_HEADER + 1 + _ICE[CALL]) * 2
FASTLOCAL_INSTRS = {
    LOAD_FAST, LOAD_DEREF, LOAD_CLASSDEREF, LOAD_FAST_CHECK,
    LOAD_FAST_AND_CLEAR, LOAD_FROM_DICT_OR_DEREF
}

def retrieve_oparg(instrs, i):
    assert not i & 1, "Odd instruction index"
    assert i >= 0, "Negative index"
    oparg = instrs[i + 1]
    i -= 2
    shift = 8
    while i >= 0 and instrs[i] == EXTENDED_ARG:
        oparg += instrs[i + 1] << shift
        shift += 8
        i -= 2
    return oparg, i

def skip_header(instrs, i):
    assert not i & 1, "Odd instruction index"
    assert i >= 0, "Negative index"
    seen_KW_NAMES = False
    while instrs[i] == EXTENDED_ARG:
        i -= 2
        if instrs[i] == KW_NAMES:
            assert not seen_KW_NAMES, "Duplicate KW_NAMES"
            seen_KW_NAMES = True
            i -= 2
    return i

def _varname(frame, i):
    code = frame.f_code
    instrs = code.co_code
    qname_list = deque()
    add = qname_list.appendleft
    addmany = qname_list.extendleft
    while i >= 0:
        opcode = instrs[i]
        oparg, i = retrieve_oparg(instrs, i)
        if opcode == LOAD_NAME or opcode == LOAD_FROM_DICT_OR_GLOBALS:
            add(code.co_names[oparg])
            break
        elif opcode == LOAD_GLOBAL:
            add(code.co_names[oparg >> 1])
            break
        elif opcode in FASTLOCAL_INSTRS:
            # local var
            if oparg < code.co_nlocals:
                add(code.co_varnames[oparg])
                break
            oparg -= code.co_nlocals
            
            # cell var
            n_deref = len(code.co_cellvars)
            if oparg < n_deref:
                add(code.co_cellvars[oparg])
                break
            oparg -= n_deref
            
            # free var
            add(code.co_freevars[oparg])
            break
        elif opcode == LOAD_ATTR:
            add('.' + code.co_names[oparg >> 1])
        elif opcode == LOAD_SUPER_ATTR:
            add('.' + code.co_names[oparg >> 2])
            if oparg & 2:
                name1, i = _varname(frame, i)
                if name1:
                    name0, i = _varname(frame, i)
                    if name0:
                        add(')')
                        addmany(name1)
                        add(", ")
                        addmany(name0)
                        add("super(")
            else:
                add("super()")
            break
    return qname_list, i

def varname(val):
    frame = _out(1)
    i = skip_header(frame.f_code.co_code, frame.f_lasti - ARG_OFFSET)
    qname_list, _ = _varname(frame, i)
    if not qname_list:
        raise ValueError(f"No name found for value: {val!r}")
    return "".join(qname_list)
