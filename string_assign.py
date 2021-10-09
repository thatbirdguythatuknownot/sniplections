# assign to a string using `memset` and `getsizeof`. Root of code from a `id(a)+getsizeof(a)-len(a)-1` snippet and made because of
# <@!745530441838231662> 's demand for a way to change a string (didn't notice changing the actual strings are needed). Made out of
# inspiration and curiosity.

from ctypes import memset
from sys import getsizeof

def kind(string):
     kind = 1
     for x in string:
         if (o := ord(x)) > 0xffff:
             return 4
         elif o > 0xff:
             kind = 2
     return kind

def s_assign_1byte(string, otherstring):
    assert len(string) == len(otherstring), "argument 1 must have the same length as argument 2"
    assert kind(string) == 1 and kind(otherstring) <= 1, "argument 1 must have the same kind as argument 2"
    ref = id(string)+getsizeof(string)-len(string)-1
    for i, x in enumerate(otherstring):
        memset(ref+i, ord(x), 1)
    return ref

def s_assign_2byte(string, otherstring):
    assert len(string) == len(otherstring), "argument 1 must have the same length as argument 2"
    assert kind(string) == 2 and kind(otherstring) <= 2, "argument 1 must have the same/greater kind as argument 2"
    ref = id(string)+getsizeof(string)-(len(string)*2)-2
    for i, x in enumerate(otherstring):
        setat = ref+2*i
        ord_x = ord(x)
        memset(setat, (o := ord_x >> 8), 2)
        memset(setat, ord_x ^ (o << 8), 1)
    return ref

def s_assign_4byte(string, otherstring):
    assert len(string) == len(otherstring), "argument 1 must have the same length as argument 2"
    assert kind(string) == 4 and kind(otherstring) <= 4, "argument 1 must have the same/greater kind as argument 2"
    ref = id(string)+getsizeof(string)-(len(string)*4)-4
    for i, x in enumerate(otherstring):
        setat = ref+4*i
        ord_x = ord(x)
        memset(setat, ord_x >> 16, 3)
        memset(setat, (ord_x & 0xffff) >> 8, 2)
        memset(setat, ord_x & 0xff, 1)
    return ref

def s_assign(string, otherstring):
    assert len(string) == len(otherstring), "argument 1 must have the same length as argument 2"
    kind1 = kind(string)
    kind2 = kind(otherstring)
    if kind2 <= kind1:
        if kind1 == 1:
            return s_assign_1byte(string, otherstring)
        elif kind1 == 2:
            return s_assign_2byte(string, otherstring)
        else:
            return s_assign_4byte(string, otherstring)
    raise TypeError("argument 1 kind has a lesser kind than argument 2")
