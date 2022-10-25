import importlib
from functools import cache, reduce

type_classrepr_pair = [(int, "__name__.__len__().__class__"), (str, "__name__.__class__"),
                       (type, "__name__.__class__.__class__"),
                       (complex, "({0}:=__name__.__len__().__class__().__invert__()).__truediv__({0}:={0}.__neg__()).__pow__({0}.__truediv__({0}.__invert__().__neg__())).__class__")]

def tcpa(o, rep): # type_classrepr_pair append
    type_classrepr_pair.append((o, rep))

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
def spf(x): # smallest prime factor
    factors = [i for i in _gp((x**.5 + 1).__trunc__()) if not x % i]
    if factors:
        return factors[len(factors) >> 1]
    return x

@lambda c:c()
class obfuscator:
    def __init__(self, taken=None):
        if taken is None:
            taken = {*()}
        self.taken = taken
    
    def nnu(self): # new name unmodified
        name = '_'
        while name in self.taken:
            name += '_'
        return name
    
    def nn(self): # new name
        name = self.nnu()
        self.taken.add(name)
        return name
    
    def on(self, x, name_=None, values={
            0: "__name__.__ne__(__name__)",
            1: "__name__.__eq__(__name__)"}): # obfuscate number
        if x in values:
            return values[x]
        elif x not in {2, 3}:
            # TODO: better stuff
            p = spf(x)
            add = 0
            orig_x = x
            while x > 3 and p == x:
                x -= (t := x // 3)
                p = spf(x)
                add += t
            name = name_ or self.nn()
            res = f"({{}}:={self.on(p, name)}.__mul__({self.on(x // p, name)})" \
                  f"{f'.__add__({on(add, name)})' if add else ''})"
            x = orig_x # for setting to values[x] later
        else:
            name = name_ or self.nn()
            res = f"""({{}}:={reduce("{}.__add__({})".format,
                                     [f"({name}:=__name__.__getitem__({on(0)}))"]
                                     +[name]*(x-1))}.__len__())"""
        values[x] = resname = self.nn() if name_ else name
        return res.format(resname)
    
    @cache
    def gdci(self, c, name=None): # get __doc__ and character index
        if not name:
            name = self.nnu()
        for x, rep in map(lambda x: (x[0], f"{x[1]}.__doc__".format(name)), type_classrepr_pair):
            if (r := x.__doc__.find(c)) >= 0:
                if ':=' in rep and name not in self.taken:
                    self.taken.add(name)
                return rep, r
    
    def gs(self, s, values={}): # get string
        if s in values:
            return values[s]
        name = self.nn()
        l = []
        for c in s:
            rep, idx = self.gdci(c, name)
            l.append(f"{rep}.__getitem__({self.on(idx)})")
        res = reduce("{}.__add__({})".format, l)
        values[s] = name
        return f"({name}:={res})"

globals().update({x: getattr(obfuscator, x) for x in dir(obfuscator) if '_' not in x})

obj_subclasses_repr = "__name__.__class__.__class__.__base__.__subclasses__()"
obj_subclasses = eval(obj_subclasses_repr)
import_func = f"__builtins__.__getattribute__({gs('__import__')})"
tcpa(__import__('re'), f"{import_func}({gs('re')})")
