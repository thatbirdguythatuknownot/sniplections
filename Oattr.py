from dis import get_instructions, _inline_cache_entries as _ICE
from opcode import opmap
from sys import _getframe as _f

jump_nocond = {*map(opmap.get, ("JUMP_FORWARD", "JUMP_BACKWARD",
                                "JUMP_BACKWARD_NO_INTERRUPT"))}

LOAD_ATTR = opmap['LOAD_ATTR']

def check_frame(frame):
    instrs = [*get_instructions(frame.f_code)]
    lasti = frame.f_lasti
    offset_cache = []
    to_check = []
    i = 0
    while i < len(instrs):
        instr = instrs[i]
        i += 1
        if i > len(offset_cache):
            offset_cache.append(instr.offset)
        if instr.offset == lasti:
            if instr.opcode in jump_nocond:
                lasti = instr.argval
            else:
                if lasti == frame.f_lasti:
                    try:
                        instr = instrs[i]
                    except IndexError:
                        instr = None
                break
            try:
                i = offset_cache.index(lasti)
                continue
            except ValueError:
                pass
    else:
        instr = None
    return instr is not None and instr.opcode == LOAD_ATTR

class O_meta(type):
    def __getattribute__(cls, attr):
        inst = cls()
        attrs = object.__getattribute__(inst, "attrs")
        attrs.append(attr)
        if check_frame(_f(1)):
            return inst
        return attrs

class O(metaclass=O_meta):
    def __init__(self):
        self.attrs = []
    def __getattribute__(self, attr):
        frame = _f(1)
        if frame.f_code is _f(0).f_code:
            return super().__getattribute__(attr)
        attrs = self.attrs
        attrs.append(attr)
        if check_frame(frame):
            return self
        return attrs

O.a.b.c.d.e
