from ctypes import c_int, py_object, pythonapi
from einspect import impl, orig
from opcode import hasfree, haslocal, hasname, opmap, opname
from sys import _getframe

PF_LTF = pythonapi.PyFrame_LocalsToFast
PF_LTF.argtypes = py_object,
PF_LTF.restype = c_int
set_names = {*hasfree + haslocal + hasname}

globals().update({x: opmap.get(x, -1) for x in ['UNARY_POSITIVE', 'CALL_INTRINSIC_1', *map(opname.__getitem__, set_names)]})

@impl(int)
def __pos__(self):
    frame = _getframe(1)
    a = frame.f_lasti
    code = frame.f_code.co_code
    opcode = code[a - 2]
    if opcode not in set_names:
        return orig(int).__pos__(self)
    if glob := opcode == LOAD_GLOBAL:
        name = frame.f_code.co_names[code[a - 1] >> 1]
    elif opcode == LOAD_NAME:
        name = frame.f_code.co_names[code[a - 1]]
        glob = -1
    else:
        name = (frame.f_code.co_varnames + frame.f_code.co_cellvars + frame.f_code.co_cellvars)[code[a - 1]]
    i = 0
    if CALL_INTRINSIC_1 < 0:
        while code[a] is UNARY_POSITIVE:
            i += 1
            a += 2
    else:
        while code[a] is CALL_INTRINSIC_1 and code[a + 1] == 5:
            i += 1
            a += 2
    if i < 2:
        return orig(int).__pos__(self)
    self += 1
    if glob is True:
        frame.f_globals[name] = self
    elif glob:
        (frame.f_locals if name in frame.f_locals else frame.f_globals)[name] = self
    else:
        frame.f_locals[name] = self
        PF_LTF(frame)
    return self
