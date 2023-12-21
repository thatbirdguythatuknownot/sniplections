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
        "LOAD_FROM_DICT_OR_DEREF LOAD_SUPER_ATTR CACHE JUMP_FORWARD "
        "LOAD_CONST LOAD_METHOD BUILD_TUPLE STORE_FAST STORE_DEREF "
        "STORE_FAST_MAYBE_NULL STORE_NAME STORE_GLOBAL COPY "
        "CALL_INTRINSIC_1 LIST_TO_TUPLE PUSH_NULL BUILD_LIST "
        "LIST_APPEND LIST_EXTEND BUILD_CONST_KEY_MAP DICT_MERGE "
        "BUILD_MAP CALL_FUNCTION_EX"
        .split()
})
if NOT_311 := not (3,11) <= version_info < (3, 12):
    PRECALL_HEADER = 0
else:
    PRECALL_HEADER = 1 + _ICE[PRECALL]

ARG_OFFSET = (PRECALL_HEADER + 1 + _ICE[CALL]) * 2
FASTLOCAL_LOADS = {
    LOAD_FAST, LOAD_DEREF, LOAD_CLASSDEREF, LOAD_FAST_CHECK,
    LOAD_FAST_AND_CLEAR, LOAD_FROM_DICT_OR_DEREF
}
FASTLOCAL_STORES = {
    STORE_FAST, STORE_DEREF, STORE_FAST_MAYBE_NULL
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

def get_localname(code, oparg):
    # local var
    if oparg < code.co_nlocals:
        return code.co_varnames[oparg]
    oparg -= code.co_nlocals
    # cell var
    n_deref = len(code.co_cellvars)
    if oparg < n_deref:
        return code.co_cellvars[oparg]
    oparg -= n_deref
    # free var
    return code.co_freevars[oparg]

def is_posargs_end(instr):
    return (
        instr.opcode == LIST_TO_TUPLE
        or instr.opcode == CALL_INTRINSIC_1 and instr.arg == 6
        or instr.opcode == LOAD_CONST and instr.argval == ()
    )

def _varname(
    code, i, instrs=None, valid_jumps=None,
    find_idx=False, allow_const=False,
    reverse_append=False
):
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
    if reverse_append:
        add = qname_list.append
        addmany = qname_list.extend
    else:
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
        elif opcode in FASTLOCAL_LOADS:
            add(get_localname(code, oparg))
            break
        elif opcode == LOAD_ATTR or opcode == LOAD_METHOD:
            add(code.co_names[oparg >> NOT_311])
            add('.')
        elif opcode == LOAD_SUPER_ATTR:
            # attr
            add(code.co_names[oparg >> 2])
            add(").")
            if oparg & 2:
                # second super() arg
                name1, i, try_ = _varname_default(
                    code, i, instrs,
                    valid_jumps, allow_const=True,
                    reverse_append=True
                )
                # first super() arg
                name0, i, _ = _varname_default(
                    code, i, instrs,
                    valid_jumps, allow_const=True,
                    reverse_append=True,
                    try_=try_
                )
                # add in reverse order
                addmany(name1)
                add(", ")
                addmany(name0)
            add("super(")
            break
        elif opcode == CALL:
            narg = oparg
            instr = instrs[i]
            opcode = instr.opcode
            if opcode == PRECALL:
                i -= 1
                instr = instrs[i]
                opcode = instr.opcode
            if opcode == KW_NAMES:
                i -= 1
                kw = code.co_consts[instr.arg]
                nkw = len(kw)
            else:
                nkw = 0
                kw = ()
            still_simple = True
            add(')')
            add_comma = False
            for n in range(narg):
                valname, i, still_simple = _varname_default(
                    code, i, instrs,
                    valid_jumps, allow_const=True,
                    reverse_append=True,
                    try_=still_simple
                )
                if add_comma:
                    add(", ")
                else:
                    add_comma = True
                addmany(valname)
                if n < nkw:
                    add('=')
                    add(kw[~n])
            add('(')
            funcname, i, still_simple = _varname_default(
                code, i, instrs,
                valid_jumps, allow_const=True,
                reverse_append=True,
                try_=still_simple
            )
            addmany(funcname)
            if still_simple and instrs[i].opcode == PUSH_NULL:
                i -= 1
            break
        elif opcode == LOAD_CONST and allow_const:
            if (isinstance(instr.argval, int)
                    and qname_list and qname_list[0] == '.'):
                add(' ')
            add(instr.argrepr)
            break
        elif opcode == BUILD_TUPLE:
            still_simple = True
            add_comma = False
            for _ in range(oparg):
                elname, i, still_simple = _varname_default(
                    code, i, instrs,
                    valid_jumps, allow_const=True,
                    reverse_append=True,
                    try_=still_simple
                )
                if add_comma:
                    add(", ")
                else:
                    add_comma = True
                addmany(elname)
            break
        elif opcode in FASTLOCAL_STORES:
            assert instrs[i].opcode == COPY and instrs[i + 1].arg == 1, \
                   "Walrus expected; not found"
            add(get_localname(code, oparg))
            break
        elif opcode == STORE_NAME or opcode == STORE_GLOBAL:
            assert instrs[i].opcode == COPY and instrs[i + 1].arg == 1, \
                   "Walrus expected; not found"
            add(code.co_names[oparg])
            break
        elif opcode == CALL_FUNCTION_EX:
            add(')')
            still_simple = True
            add_comma = False
            do_posargs = True
            nprocessed = 0
            if oparg & 1:
                while still_simple:
                    instr = instrs[i]
                    if is_KW := instr.opcode == DICT_MERGE:
                        instr = instrs[i := i - 1]
                    opcode = instr.opcode
                    if opcode == BUILD_MAP and instr.arg == 1:
                        i -= 1
                        argname, i, still_simple = _varname_default(
                            code, i, instrs,
                            valid_jumps, allow_const=True,
                            reverse_append=True,
                            try_=still_simple
                        )
                        if still_simple:
                            instr = instrs[i]
                            assert instr.opcode == LOAD_CONST, \
                                   "Keyword argument expected; not found"
                            i -= 1
                            kwname = instr.argval
                        else:
                            kwname = "..."
                        if add_comma:
                            add(", ")
                        else:
                            add_comma = True
                        addmany(argname)
                        add('=')
                        add(kwname)
                    elif opcode == BUILD_MAP and instr.arg == 0:
                        i -= 1
                        continue
                    elif opcode == BUILD_CONST_KEY_MAP:
                        nkw = instr.arg
                        instr = instrs[i := i - 1]
                        assert instr.opcode == LOAD_CONST, \
                               "Keyword arguments expected; not found"
                        kw = instr.argval
                        i -= 1
                        for n in range(nkw):
                            argname, i, still_simple = _varname_default(
                                code, i, instrs,
                                valid_jumps, allow_const=True,
                                reverse_append=True,
                                try_=still_simple
                            )
                            if add_comma:
                                add(", ")
                            else:
                                add_comma = True
                            addmany(argname)
                            add('=')
                            add(kw[~n])
                    elif is_KW:
                        argname, i, still_simple = _varname_default(
                            code, i, instrs,
                            valid_jumps, allow_const=True,
                            reverse_append=True,
                            try_=still_simple
                        )
                        if add_comma:
                            add(", ")
                        else:
                            add_comma = True
                        addmany(argname)
                        add("**")
                    elif is_posargs_end(instr):
                        do_posargs = opcode != LOAD_CONST
                        i -= 1
                        break
                    else:
                        still_simple = False
                    if still_simple:
                        nprocessed += 1
            elif is_posargs_end(instrs[i]):
                do_posargs = opcode != LOAD_CONST
                i -= 1
            posarg_complex = False
            if do_posargs:
                while still_simple:
                    instr = instrs[i]
                    opcode = instr.opcode
                    if opcode == LIST_APPEND:
                        i -= 1
                        argname, i, still_simple = _varname_default(
                            code, i, instrs,
                            valid_jumps, allow_const=True,
                            reverse_append=True,
                            try_=still_simple
                        )
                        if not still_simple:
                            posarg_complex = True
                        if add_comma:
                            add(", ")
                        else:
                            add_comma = True
                        addmany(argname)
                    elif opcode == BUILD_LIST:
                        i -= 1
                        for _ in range(instr.arg):
                            argname, i, still_simple = _varname_default(
                                code, i, instrs,
                                valid_jumps, allow_const=True,
                                reverse_append=True,
                                try_=still_simple
                            )
                            if not still_simple:
                                posarg_complex = True
                            if add_comma:
                                add(", ")
                            else:
                                add_comma = True
                            addmany(argname)
                        break
                    else:
                        is_LIST_EXTEND = opcode == LIST_EXTEND
                        i -= is_LIST_EXTEND
                        argname, i, still_simple = _varname_default(
                            code, i, instrs,
                            valid_jumps, allow_const=True,
                            reverse_append=True,
                            try_=still_simple
                        )
                        if add_comma:
                            add(", ")
                        else:
                            add_comma = True
                        addmany(argname)
                        add('*')
                        if not is_LIST_EXTEND:
                            break
                    if still_simple:
                        nprocessed += 1
            if not still_simple and not posarg_complex:
                if add_comma:
                    add(", ")
                else:
                    add_comma = True
                if nprocessed > 0:
                   add(', ')
                add("...")
            add('(')
            funcname, i, still_simple = _varname_default(
                code, i, instrs,
                valid_jumps, allow_const=True,
                reverse_append=True,
                try_=still_simple
            )
            addmany(funcname)
            if still_simple and instrs[i].opcode == PUSH_NULL:
                i -= 1
            break
        elif opcode != CACHE:
            break
    return qname_list, i

def _varname_default(code, i, *args, default=("...",), try_=True, **kwargs):
    if try_:
        name, i = _varname(code, i, *args, **kwargs)
        success = bool(name)
    else:
        success = False
    if not success:
        name = default
    return name, i, success

def varname(val):
    frame = _out(1)
    i = skip_header(frame.f_code.co_code, frame.f_lasti - ARG_OFFSET)
    qname_list, _ = _varname(frame.f_code, i, find_idx=True)
    if not qname_list:
        raise ValueError(f"No name found for value: {val!r}")
    return "".join(qname_list)
