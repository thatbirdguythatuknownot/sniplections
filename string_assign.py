# assign to a string using `memset` and `getsizeof`. Root of code from a `id(a)+getsizeof(a)-len(a)-1` snippet and made because of
# <@!745530441838231662> 's demand for a way to change a string (didn't notice changing the actual strings are needed). Made out of
# inspiration and curiosity.

from ctypes import memset
from sys import byteorder, getsizeof

def kind(string):
     kind = 1
     for x in string:
         if (o := ord(x)) > 0xffff:
             return 4
         elif o > 0xff:
             kind = 2
     return kind

def s_assign_1byte(string, otherstring, start=0, end=None):
    assert kind(string) == 1 and kind(otherstring) <= 1, "argument 1 must have the same kind as argument 2"
    len_string = len(string)
    if start < 0:
        start += len_string
        assert start >= 0, "start index out of bounds"
    if end is None: end = len_string
    elif end < 0:
        end += len_string
        assert end >= 0, "end index out of bounds"
    ref = id(string)+getsizeof(string)-len(string)+start-1
    if (end := end - start) <= 0: return ref
    for i, x in enumerate(otherstring):
        if i == end: break
        memset(ref+i, ord(x), 1)
    return ref

def s_assign_2byte(string, otherstring, start=0, end=None):
    assert kind(string) == 2 and kind(otherstring) <= 2, "argument 1 must have the same/greater kind as argument 2"
    len_string = len(string)
    if start < 0:
        start += len_string
        assert start >= 0, "start index out of bounds"
    if end is None: end = len_string
    elif end < 0:
        end += len_string
        assert end >= 0, "end index out of bounds"
    ref = id(string)+getsizeof(string)-(len(string)-start+1)*2
    if (end := end - start) <= 0: return ref
    for i, x in enumerate(otherstring):
        if i == end: break
        setat = ref+2*i
        ord_x = ord(x)
        if byteorder == 'little':
           memset(setat, ord_x >> 8, 2)
           memset(setat, ord_x & 0xff, 1)
        else:
           memset(setat, ord_x & 0xff, 2)
           memset(setat, ord_x >> 8, 1)
    return ref

def s_assign_4byte(string, otherstring, start=0, end=None):
    assert kind(string) == 4 and kind(otherstring) <= 4, "argument 1 must have the same/greater kind as argument 2"
    len_string = len(string)
    if start < 0:
        start += len_string
        assert start >= 0, "start index out of bounds"
    if end is None: end = len_string
    elif end < 0:
        end += len_string
        assert end >= 0, "end index out of bounds"
    ref = id(string)+getsizeof(string)-(len(string)-start+1)*4
    if (end := end - start) <= 0: return ref
    for i, x in enumerate(otherstring):
        if i == end: break
        setat = ref+4*i
        ord_x = ord(x)
        if byteorder == 'little':
            memset(setat, ord_x >> 16, 3)
            memset(setat, ord_x >> 8 & 0xff, 2)
            memset(setat, ord_x & 0xff, 1)
        else:
            setat += 1
            memset(setat, ord_x & 0xff, 3)
            memset(setat, ord_x >> 8 & 0xff, 2)
            memset(setat, ord_x >> 16, 1)
    return ref

def s_assign(string, otherstring, start=0, end=None):
    len_string = len(string)
    if start < 0:
        start += len_string
        assert start >= 0, "start index out of bounds"
    if end is None: end = len_string
    elif end < 0:
        end += len_string
        assert end >= 0, "end index out of bounds"
    kind1 = kind(string)
    kind2 = kind(otherstring)
    if kind2 <= kind1:
        if kind1 == 1:
            return s_assign_1byte(string, otherstring, start, end)
        elif kind1 == 2:
            return s_assign_2byte(string, otherstring, start, end)
        else:
            return s_assign_4byte(string, otherstring, start, end)
    raise TypeError("argument 1 kind has a lesser kind than argument 2")
