import importlib, re
from functools import cache, reduce
from math import log2, trunc

"""
def gbni(b): # get builtin name index
    for i, n in enumerate(__builtins__.__dict__):
        if b == n:
            return i

def gosi(b): # get object subclass index
    for i, n in enumerate(object.__subclasses__()):
        if b == n:
            return i
"""

def gifi(it, b): # get index from iterable
    for i, x in enumerate(it):
        if b == x:
            return i

"""
def _gp(y, D={}): # generate primes
    q = 2
    while q < y:
        if q not in D:
            yield q
            D[q * q] = [q]
        else:
            for p in D[q]:
                D.setdefault(p + q, []).append(p)
            del D[q]
        q += 1

@cache
def gpf(x): # greatest prime factor
    factors = [i for i in _gp(trunc(x**.5 + 1)) if not x % i]
    if factors:
        return factors[len(factors) >> 1]
    return x
"""

@cache
def gpf(x): # greatest . factor
    return next((i for i in range(trunc(x**.5 + 1), 1, -1) if not x % i), x)

class Obfuscator:
    def __init__(self, taken=None):
        self.no_walrus = taken is False
        if self.no_walrus:
            self.object_repr_pair = {
                int: "__name__.__len__().__class__", str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: "__name__.__class__.__class__.__base__)",
                complex: "__name__.__ne__(__name__).__invert__().__truediv__(__name__.__eq__(__name__)).__pow__(__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__invert__().__neg__())).__class__",
                re: ("{1}({2})", (__import__,), ("re",)),
                __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",)),
            }
        else:
            if taken is None or taken is True:
                taken = {*()}
            self.taken = taken
            self.object_repr_pair = {
                int: "__name__.__len__().__class__", str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: ("({0}:=__name__.__class__.__class__.__base__)", (), ()),
                complex: ("({0}:=({{0}}:=__name__.__ne__(__name__).__invert__()).__truediv__({{0}}:={{0}}.__neg__()).__pow__({{0}}.__truediv__({{0}}.__invert__().__neg__())).__class__)",
                          (), ()),
                re: ("({0}:={1}({2}))", (__import__,), ("re",)),
                __import__: ("({0}:=__builtins__.__getattribute__({1}))", (), ("__import__",)),
            }
            self._nonassigned = {complex, re, __import__}
    
    def nnu(self): # new name unmodified
        name = '_'
        while name in self.taken:
            name += '_'
        return name
    
    def nn(self): # new name
        name = self.nnu()
        self.taken.add(name)
        return name
    
    def porpv(self, x, name=None): # process object repr pair value
        v = self.object_repr_pair[x]
        if type(v) is not tuple:
            return v
        if self.no_walrus:
            self.object_repr_pair[x] = res = v[0].format(
                None,
                *map(lambda x: self.porpv(x)[0], v[1]),
                *map(self.gs, v[2])
            )
            return res, None
        if name is None:
            name = self.nn()
        self.object_repr_pair[x] = name
        res = v[0].format(name, *map(lambda x: self.porpv(x)[0], v[1]), *map(self.gs, v[2]))
        return res, name
    
    def on(self, x, name_=None, values={
            0: "__name__.__ne__(__name__)",
            1: "__name__.__eq__(__name__)"}): # obfuscate number
        if x < 0:
            res = on(inv := ~x)
            values[x] = f"{values[inv]}.__invert__()"
            return f"{res}.__invert__()"
        if x in values:
            return values[x]
        if not self.no_walrus:
            name = name_ or self.nn()
        else:
            name = None
        if x not in {2, 3}:
            # TODO: better stuff
            p = gpf(x)
            add = 0
            orig_x = x
            while x > 3 and p == x:
                x -= (t := x // 3)
                p = gpf(x)
                add += t
            q = x // p
            method = "mul"
            if q > 4 and (qb := log2(q)).is_integer():
                qb = trunc(qb)
                if p > 4 and (pb := log2(p)).is_integer() and pb < qb:
                    pb, qb = qb, trunc(pb)
                q = qb
                method = "lshift"
            res = f"{self.on(p, name)}.__{method}__({self.on(q, name)})" \
                  f"""{'.__invert__().__neg__()' if add == 1 else
                     f'.__add__({on(add, name)})' if add else ''}"""
            x = orig_x # for setting to values[x] later
        else:
            res = f"""{reduce("{}.__add__({})".format,
                              [f"__name__.__getitem__({on(0)})"]*x
                              if self.no_walrus else
                              [f"({name}:=__name__.__getitem__({on(0)}))"]
                              +[name]*(x-1))}.__len__()"""
        if self.no_walrus:
            return res
        values[x] = resname = self.nn() if name_ else name
        return f"({resname}:={res})"
    
    def gdci(self, c, name_=None): # get __doc__ and character index
        if self.no_walrus:
            for x, rep in self.object_repr_pair.items():
                if (r := x.__doc__.find(c)) >= 0:
                    return f"{self.porpv(x)[0]}.__doc__", r
        name = name_ or self.nnu()
        for x, rep in self.object_repr_pair.items():
            if (r := x.__doc__.find(c)) >= 0:
                if x in self._nonassigned:
                    self._nonassigned.remove(x)
                    assignable_name = self.nn() if name_ else name
                    rep, _ = self.porpv(x, assignable_name)
                rep = f"{rep}.__doc__".format(name)
                if ':=' in rep and name not in self.taken:
                    self.taken.add(name)
                return rep, r
    
    def gs(self, s, values={}): # get string
        if s in values:
            return values[s]
        if self.no_walrus:
            l = []
            for c in s:
                rep, idx = self.gdci(c)
                l.append(f"{rep}.__getitem__({self.on(idx)})")
            res = reduce("{}.__add__({})".format, l)
            values[s] = res
            return res
        name = self.nn()
        l = []
        for c in s:
            rep, idx = self.gdci(c, name)
            l.append(f"{rep}.__getitem__({self.on(idx, name)})")
        res = reduce("{}.__add__({})".format, l)
        values[s] = name
        return f"({name}:={res})"
    
    def clear(self):
        for f, new in [
                (self.on, {
                    0: "__name__.__ne__(__name__)",
                    1: "__name__.__eq__(__name__)"
                }),
                (self.gs, {})]:
            (dflt := f.__defaults__[-1]).clear()
            dflt.update(new)
        if self.no_walrus:
            self.object_repr_pair = {
                int: "__name__.__len__().__class__", str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: "__name__.__class__.__class__.__base__)",
                complex: "__name__.__ne__(__name__).__invert__().__truediv__(__name__.__eq__(__name__)).__pow__(__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__invert__().__neg__())).__class__",
                re: ("{1}({2})", (__import__,), ("re",)),
                __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",)),
            }
        else:
            self.taken.clear()
            self.object_repr_pair = {
                int: "__name__.__len__().__class__", str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: ("({0}:=__name__.__class__.__class__.__base__)", (), ()),
                complex: ("({0}:=({{0}}:=__name__.__ne__(__name__).__invert__()).__truediv__({{0}}:={{0}}.__neg__()).__pow__({{0}}.__truediv__({{0}}.__invert__().__neg__())).__class__)",
                          (), ()),
                re: ("({0}:={1}({2}))", (__import__,), ("re",)),
                __import__: ("({0}:=__builtins__.__getattribute__({1}))", (), ("__import__",)),
            }
            self._nonassigned = {complex, re, __import__}