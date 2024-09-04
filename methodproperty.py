from opcode import (EXTENDED_ARG, HAVE_ARGUMENT as HAS_ARG,
                    _inline_cache_entries as _ICE, hasjrel,
                    opmap, opname, stack_effect)
from sys import _getframe as _f, version_info

if version_info < (3, 13):
    get_caches = lambda i: _ICE[i]
else:
    get_caches = lambda i: _ICE.get(opname[i], 0)

hasjrel = {*hasjrel}

def has_call(frame=None):
    if frame is None:
        frame = _f(2)
    effect = 0
    has_ext = False
    oparg = 0
    i = frame.f_lasti
    instr_skipn = 0
    len_code = len(cods := frame.f_code.co_code)
    while i < len_code:
        if instr_skipn:
            instr_skipn -= 1
            i += 2
            continue
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
        if instr in hasjrel:
            if "BACKWARD" in instr_name:
                raise RuntimeError("what-?")
            instr_skipn = oparg - 1
            continue
        if ("CALL" in instr_name
                and "INTRINSIC" not in instr_name
                and not effect):
            return True
    return False

class methodproperty(property):
    def callgetter(self, f):
        self.fcall = f
    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        if has_call(_f(1)) and hasattr(self, "fcall"):
            return lambda *a, **k: self.fcall(obj, *a, **k)
        return super().__get__(obj, objtype)
