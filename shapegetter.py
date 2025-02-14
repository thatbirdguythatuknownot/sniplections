from opcode import (EXTENDED_ARG, HAVE_ARGUMENT as HAS_ARG,
                    _inline_cache_entries as _ICE, hasjrel,
                    opmap, opname, stack_effect)
from sys import _getframe as _f, version_info

if version_info < (3, 13):
    get_caches = lambda i: _ICE[i]
else:
    get_caches = lambda i: _ICE.get(opname[i], 0)

hasjrel = {*hasjrel}

UNPACK_SEQUENCE = opmap['UNPACK_SEQUENCE']
UNPACK_EX = opmap['UNPACK_EX']
COPY = opmap['COPY']

def get_assign_shapes(frame=None):
    if frame is None:
        frame = _f(2)
    shapes = []
    copies = 1
    stack_i = []
    stack_len = []
    cur = top = None
    start_effect = effect = 1
    has_ext = False
    oparg = 0
    instr_skipn = 0
    cods = frame.f_code.co_code
    i = frame.f_lasti + 2*get_caches(cods[frame.f_lasti]) + 2
    len_code = len(cods)
    while i < len_code:
        if instr_skipn:
            instr_skipn -= 1
            i += 2
            continue
        if not stack_len:
            if top is not None:
                copies -= 1
                shapes.append(top)
                cur = top = None
                start_effect = effect
            if not copies:
                break
        instr = cods[i]
        instr_name = opname[instr]
        arg = cods[i + 1]
        i += 2
        if num := get_caches(instr):
            i += num * 2
        oparg |= arg
        if instr is EXTENDED_ARG:
            oparg <<= 8
            has_ext = True
            continue
        elif has_ext:
            has_ext = False
        else:
            oparg = arg
        effect += stack_effect(instr,
                               oparg if instr >= HAS_ARG else None,
                               jump=True)
        if effect < 0:
            break
        is_unpack = False
        if is_unpack := instr is UNPACK_SEQUENCE:
            star_idx = -1
            real_len = oparg
        elif is_unpack := instr is UNPACK_EX:
            star_idx = oparg & 0xFF
            real_len = star_idx + (oparg >> 8) + 1
        elif not stack_len:
            if instr is COPY and oparg == effect - 1:
                copies += 1
                start_effect = effect
        if is_unpack:
            temp = cur
            cur = [None] * real_len
            if star_idx >= 0:
                cur[star_idx] = True
            if temp is None:
                temp = top = cur
            else:
                temp[stack_i[-1]] = cur
                stack_i[-1] += 1
            stack_i.append(0)
            stack_len.append(real_len)
            start_effect = effect
        else:
            if instr in hasjrel:
                if "BACKWARD" in instr_name:
                    raise RuntimeError("what-?")
                instr_skipn = oparg - 1
            elif "STORE" in instr_name and effect < start_effect:
                start_effect = effect
                if not stack_len:
                    copies -= 1
                    shapes.append(None)
                else:
                    stack_i[-1] += 1
                    if stack_i[-1] >= stack_len[-1]:
                        stack_i.pop()
                        stack_len.pop()
    return shapes

def get_assign_shape(frame=None):
    if frame is None:
        frame = _f(2)
    shapes = get_assign_shapes(frame)
    return shapes[0] if shapes else None
