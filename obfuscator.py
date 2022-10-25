import importlib
from functools import reduce

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
    
    def on(self, x): # obfuscate number
        #if x in {2, 3}:
        #    name = self.nn()
        #    return reduce("{}.__add__({})".format, [])
        return x
    
    def gdci(self, c, name=None): # get __doc__ and character index
        if not name:
            name = self.nnu()
        for x, rep in map(lambda x: (x[0], f"{x[1]}.__doc__".format(name)), type_classrepr_pair):
            if (r := x.__doc__.find(c)) >= 0:
                if ':=' in rep and name not in self.taken:
                    self.taken.add(name)
                return rep, r
    
    def gs(self, s): # get string
        name = self.nn()
        l = []
        for c in s:
            rep, idx = self.gdci(c, name)
            l.append(f"{rep}.__getitem__({self.on(idx)})")
        res = reduce("{}.__add__({})".format, l)
        if ':=' not in res:
            self.taken.remove(name)
        return res

globals().update({x: getattr(obfuscator, x) for x in dir(obfuscator) if '_' not in x})

obj_subclasses_repr = "__name__.__class__.__class__.__base__.__subclasses__()"
obj_subclasses = eval(obj_subclasses_repr)
import_func = f"__builtins__.__getattribute__({gs('__import__')})"
tcpa(__import__('re'), f"{import_func}({gs('re')})")
