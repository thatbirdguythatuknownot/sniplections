from struct import iter_unpack
from types import FunctionType as function, CodeType as code
from copy import copy

class CodeWrap:
    def __init__(self, x):
        self.__code__ = x

def throwaway_extra_unpack(a, modify_inplace=False):
    co_code = bytearray(a.__code__.co_code)
    CO_CODE_REPLACE = co_code.replace
    unpacknums = set()
    UNPACKNUMS_ADD = unpacknums.add
    for_iter_arg_index = [(0,0)]
    for_iter_not_done = 0
    for_iter_ind = 0
    for i, (x, y) in enumerate(iter_unpack('BB', co_code)):
        if x is 0x5d:
            for_iter_not_done = y
            for_iter_ind = i * 2 + 1
        elif x is 0x5c:
            if for_iter_not_done:
                co_code[for_iter_ind] += 1
            j = 0
            h = i*2+2
            h_orig = h
            while j < y:
                ch = co_code[h]
                if ch is 0x3c or ch is 0x5a or ch is 0x5f or ch is 0x61 or ch is 0x7d or ch is 0x89:
                    j += 1
                h += 2
            UNPACKNUMS_ADD((y, bytes(co_code[h_orig:h])))
        if for_iter_not_done: for_iter_not_done -= 1
    for x,y in unpacknums:
        co_code = CO_CODE_REPLACE(bytes((0x5c, x))+y, bytes((0x5e, x))+y+b'\1\0')
    if modify_inplace:
        co_consts_list = list(a.__code__.co_consts)
        for i, x in enumerate(co_consts_list):
            if type(x) is code:
                copied_code = CodeWrap(copy(x))
                throwaway_extra_unpack(copied_code, True)
                co_consts_list[i] = copied_code.__code__
        a.__code__ = a.__code__.replace(co_code=bytes(co_code), co_consts=tuple(co_consts_list))
    else:
        co_consts_list = list(a.__code__.co_consts)
        for i, x in enumerate(co_consts_list):
            if type(x) is code:
                copied_code = CodeWrap(copy(x))
                throwaway_extra_unpack(copied_code, True)
                co_consts_list[i] = copied_code.__code__
        return function(a.__code__.replace(co_code=bytes(co_code), co_consts=tuple(co_consts_list)), globals(), a.__name__, a.__defaults__, a.__closure__ or ())
