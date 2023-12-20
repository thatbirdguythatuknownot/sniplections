from collections import deque
from dis import (_inline_cache_entries as _ICE, get_instructions,
                 hasjabs, hasjrel, opmap)
from sys import _getframe as _out, version_info

hasjabs = {*hasjabs}
hasjrel = {*hasjrel}
hasj = hasjabs | hasjrel

globals().update({
    x: opmap.get(x, -1)
    for x in
        "LOAD_NAME LOAD_FAST LOAD_DEREF LOAD_CLASSDEREF LOAD_GLOBAL "
        "LOAD_ATTR CALL PRECALL EXTENDED_ARG KW_NAMES "
        "LOAD_FROM_DICT_OR_GLOBALS LOAD_FAST_CHECK LOAD_FAST_AND_CLEAR "
        "LOAD_FROM_DICT_OR_DEREF LOAD_SUPER_ATTR CACHE JUMP_FORWARD"
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

def skip_header(instrs, i):
    assert not i & 1, "Odd instruction index"
    assert i >= 0, "Negative index"
    seen_KW_NAMES = False
    while instrs[i] == EXTENDED_ARG or instrs[i] == CACHE:
        i -= 2
        if instrs[i] == KW_NAMES:
            assert not seen_KW_NAMES, "Duplicate KW_NAMES"
            seen_KW_NAMES = True
            i -= 2
    return i

def _varname(code, i, instrs=None, valid_jumps=None, find_idx=False):
    if instrs is None:
        instrs = [*get_instructions(code)]
    if find_idx:
        assert not i & 1, "Odd instruction index"
        for j, instr in enumerate(instrs):
            if i == instr.offset:
                i = j
                break
        else:
            raise IndexError(f"cannot find index {i} in code sequence")
    if valid_jumps is None:
        valid_jumps = set()
        invalid = set()
        for j in range(i):
            instr = instrs[j]
            if instr.opcode == JUMP_FORWARD and instr.argval not in invalid:
                valid_jumps.add(instr.argval)
            elif instr.opcode in hasj:
                if instr.argval in valid_jumps:
                    valid_jumps.remove(instr.argval)
                    invalid.add(instr.argval)
    qname_list = deque()
    add = qname_list.appendleft
    addmany = qname_list.extendleft
    orig_i = i
    while i >= 0:
        instr = instrs[i]
        opcode = instr.opcode
        oparg = instr.arg
        i -= 1
        if instr.is_jump_target and instr.offset not in valid_jumps:
            # part of a complex expression; break
            i = orig_i
            qname_list.clear()
            break
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
            add(code.co_names[oparg >> 1])
            add('.')
        elif opcode == LOAD_SUPER_ATTR:
            # attr
            add(code.co_names[oparg >> 2])
            add(").")
            if oparg & 2:
                # second super() arg
                name1, i = _varname(code, i, instrs)
                if not name1:
                    name0 = name1 = ("...",)
                else:
                    # first super() arg
                    name0, i = _varname(code, i, instrs)
                    if not name0:
                       name0 = ("...",)
                # add in reverse order
                addmany(name1)
                add(", ")
                addmany(name0)
            add("super(")
            break
    return qname_list, i

def varname(val):
    frame = _out(1)
    i = skip_header(frame.f_code.co_code, frame.f_lasti - ARG_OFFSET)
    qname_list, _ = _varname(frame.f_code, i, find_idx=True)
    if not qname_list:
        raise ValueError(f"No name found for value: {val!r}")
    return "".join(qname_list)
