import importlib, re
from ast import *
from ast import _Precedence, _Unparser
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
    """Obfuscator(taken=None)
    Obfuscate Python code. Outputs code with underscore-only names
    and dunder-only attributes.

        taken = None
            Predefined set of names. Will be cleared if called by `.c()`.
            By default None, which is then replaced with an empty set. Passing
            True will also create an empty set.
            If `taken = False`, "no walrus" mode is activated.

    Function documentation:
        .<abbreviated name>(<args>): <expanded name> [-- <flags>]
            <description>

    Flags:
        W -> requires walrus operator
    """
    def __init__(self, taken=None):
        self.gs_values = {}
        self.on_values = {
            0: "__name__.__ne__(__name__)",
            1: "__name__.__eq__(__name__)"
        }
        self.no_walrus = taken is False
        if self.no_walrus:
            self.object_repr_pair = {
                int: "__name__.__len__().__class__",
                str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: "__name__.__class__.__class__.__base__",
                dict: "__builtins__.__dict__.__class__",
                complex: "__name__.__ne__(__name__).__invert__().__truediv__(__name__.__eq__(__name__)).__pow__(__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__invert__().__neg__())).__class__",
                open: ("__builtins__.__dict__.__getitem__({1})", (), ("open",)),
                oct: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
                      ((__builtins__.__dir__, "oct"),), ()),
                re: ("{1}({2})", (__import__,), ("re",)),
                __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",)),
                setattr: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
                          ((__builtins__.__dir__, "setattr"),), ()),
                slice: ("__builtins__.__getattribute__({1})", (), ("slice",)),
                dict.fromkeys: ("{1}.__dict__.__getitem__({2})", (dict,), ("fromkeys",)),
                True: "__name__.__eq__(__name__)",
                False: "__name__.__ne__(__name__)",
                None: ("{1}({2}).__getitem__({3})", (dict.fromkeys,), ('_', '_')),
            }
        else:
            if taken is None or taken is True:
                taken = set()
            self.taken = taken
            self.object_repr_pair = {
                int: "__name__.__len__().__class__",
                str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: ("({0}:=__name__.__class__.__class__.__base__)", (), ()),
                dict: "__builtins__.__dict__.__class__",
                complex: ("({0}:=({{0}}:=__name__.__ne__(__name__).__invert__()).__truediv__({{0}}:={{0}}.__neg__()).__pow__({{0}}.__truediv__({{0}}.__invert__().__neg__())).__class__)",
                          (), ()),
                open: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("open",)),
                oct: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
                      ((__builtins__.__dir__, "oct"),), ()),
                re: ("({0}:={1}({2}))", (__import__,), ("re",)),
                __import__: ("({0}:=__builtins__.__getattribute__({1}))", (), ("__import__",)),
                setattr: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
                          ((__builtins__.__dir__, "setattr"),), ()),
                slice: ("({0}:=__builtins__.__getattribute__({1}))", (), ("slice",)),
                dict.fromkeys: ("({0}:={1}.__dict__.__getitem__({2}))", (dict,), ("fromkeys",)),
                True: "__name__.__eq__(__name__)",
                False: "__name__.__ne__(__name__)",
                None: ("({0}:={1}({2}).__getitem__({3}))", (dict.fromkeys,), ('_', '_')),
            }
            self._nonassigned = {complex, open, oct, re, __import__}
    
    def nnu(self):
        """.nnu(): new name unmodified -- W
        Add a new name without adding it to `.taken`."""
        name = '_'
        while name in self.taken:
            name += '_'
        return name
    
    def nn(self):
        """.nn(): new name -- W
        Add a new name, also adding it to `.taken`."""
        name = self.nnu()
        self.taken.add(name)
        return name
    
    def porpv(self, x, name=None):
        """.porpv(x, name=None): process object repr pair value
        Get a value from the internal object-to-representation mapping using
        `x` as the key and process it so the proper string value is returned.
        Resulting string is assigned to `name` and `name` is assigned to
        `.object_repr_pair[x]`."""
        v = self.object_repr_pair[x]
        if type(v) is not tuple:
            return v
        if self.no_walrus:
            self.object_repr_pair[x] = res = v[0].format(
                None,
                *map(lambda x: self.on(gifi(x[0](), x[1]), name) if isinstance(x, tuple) else self.porpv(x)[0], v[1]), 
                *map(self.gs, v[2])
            )
            return res, None
        if name is None:
            name = self.nn()
        self.object_repr_pair[x] = name
        res = v[0].format(
            name,
            *map(lambda x: self.on(gifi(x[0](), x[1]), name) if isinstance(x, tuple) else self.porpv(x)[0], v[1]), 
            *map(self.gs, v[2])
        )
        return res, name
    
    def on(self, x, name_=None):
        """.on(x, name_=None): obfuscate number
        Obfuscate a number. `name_` is the temporary name used for temporary
        value assignments."""
        values = self.on_values
        if x < 0:
            res = self.on(inv := ~x)
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
                     f'.__add__({self.on(add, name)})' if add else ''}"""
            x = orig_x # for setting to values[x] later
        else:
            res = f"""{reduce("{}.__add__({})".format,
                              [f"__name__.__getitem__({self.on(0)})"]*x
                              if self.no_walrus else
                              [f"({name}:=__name__.__getitem__({self.on(0)}))"]
                              +[name]*(x-1))}.__len__()"""
        if self.no_walrus:
            return res
        values[x] = resname = self.nn() if name_ else name
        return f"({resname}:={res})"
    
    def gdci(self, c, name_=None):
        """.gdci(c, name_=None): get __doc__ and character index
        Returns (obfuscated representation of `c`, index in
        `<obfuscated>`)."""
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
    
    def gs(self, s):
        """.gs(s): get string
        Gets the obfuscated representation of string `s`."""
        values = self.gs_values
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
    
    def c(self):
        """.c(): clear
        Clears/resets obfuscator caches."""
        for name, new in [
                ("on", {
                    0: "__name__.__ne__(__name__)",
                    1: "__name__.__eq__(__name__)"
                }),
                ("gs", {})]:
            setattr(self, f"{name}_values", new)
        if self.no_walrus:
            self.object_repr_pair = {
                int: "__name__.__len__().__class__",
                str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: "__name__.__class__.__class__.__base__",
                dict: "__builtins__.__dict__.__class__",
                complex: "__name__.__ne__(__name__).__invert__().__truediv__(__name__.__eq__(__name__)).__pow__(__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__invert__().__neg__())).__class__",
                open: ("__builtins__.__dict__.__getitem__({1})", (), ("open",)),
                oct: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
                      ((__builtins__.__dir__, "oct"),), ()),
                re: ("{1}({2})", (__import__,), ("re",)),
                __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",)),
                setattr: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
                          ((__builtins__.__dir__, "setattr"),), ()),
                slice: ("__builtins__.__getattribute__({1})", (), ("slice",)),
                dict.fromkeys: ("{1}.__dict__.__getitem__({2})", (dict,), ("fromkeys",)),
                True: "__name__.__eq__(__name__)",
                False: "__name__.__ne__(__name__)",
                None: ("{1}({2}).__getitem__({3})", (dict.fromkeys,), ('_', '_')),
            }
        else:
            self.taken.clear()
            self.object_repr_pair = {
                int: "__name__.__len__().__class__",
                str: "__name__.__class__",
                type: "__name__.__class__.__class__",
                object: ("({0}:=__name__.__class__.__class__.__base__)", (), ()),
                dict: "__builtins__.__dict__.__class__",
                complex: ("({0}:=({{0}}:=__name__.__ne__(__name__).__invert__()).__truediv__({{0}}:={{0}}.__neg__()).__pow__({{0}}.__truediv__({{0}}.__invert__().__neg__())).__class__)",
                          (), ()),
                open: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("open",)),
                oct: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
                      ((__builtins__.__dir__, "oct"),), ()),
                re: ("({0}:={1}({2}))", (__import__,), ("re",)),
                __import__: ("({0}:=__builtins__.__getattribute__({1}))", (), ("__import__",)),
                setattr: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
                          ((__builtins__.__dir__, "setattr"),), ()),
                slice: ("({0}:=__builtins__.__getattribute__({1}))", (), ("slice",)),
                dict.fromkeys: ("({0}:={1}.__dict__.__getitem__({2}))", (dict,), ("fromkeys",)),
                True: "__name__.__eq__(__name__)",
                False: "__name__.__ne__(__name__)"
                None: ("({0}:={1}({2}).__getitem__({3}))", (dict.fromkeys,), ('_', '_')),
            }
            self._nonassigned = {complex, open, oct, re, __import__}

builtins_dict = __builtins__.__dict__

class UnparseObfuscate(_Unparser, Obfuscator):
    def __init__(self, taken=None):
        self._source = []
        self._precedences = {}
        self.name_to_taken = {}
        self.overridden_builtins = set()
        Obfuscator.__init__(self, taken)
    
    def not_implemented(self, node):
        print(f"""<string>:Line {node.lineno}{
                f' -> Line {node.end_lineno}'
                if node.lineno != node.end_lineno else
                ''
              }:Column {node.col_offset}{
                f' -> Column {node.end_col_offset}'
                if node.col_offset != node.end_col_offset else
                ''
              }: .visit_{(name := type(node).__name__)}() is not implemented""")
        return getattr(super(), f"visit_{name}", self.generic_visit)(node)
    
    def get_name(self, id):
        if not (name := self.name_to_taken.get(id)):
            if id in builtins_dict and id not in self.overridden_builtins:
                self.overridden_builtins.add(id)
            name = self.nn()
            self.name_to_taken[id] = name
        return name
    
    def visit_Module(self, node):
        self.traverse(node.body)
    
    def visit_Expr(self, node):
        self.set_precedence(_Precedence.YIELD, node.value)
        self.traverse(node.value)
    
    def visit_NamedExpr(self, node):
        with self.require_parens(_Precedence.NAMED_EXPR, node):
            self.set_precedence(_Precedence.ATOM, (t := node.target), node.value)
            if isinstance(t, Name):
                t.id = self.get_name(t.id)
            self.traverse(node.target)
            self.write(":=")
            self.traverse(node.value)
    
    def visit_Import(self, node):
        val, name = self.porpv(__import__)
        it = iter(nn := node.names)
        self.write(f"({self.get_name((_name := next(it)).asname)}:={val}({self.gs(_name.name)}))")
        self.interleave(lambda: self.write(','),
                        lambda _name: f"({self.get_name(_name.asname)}:={name}({self.gs(_name.name)}))",
                        it)
    
    visit_ImportFrom = not_implemented
    
    def _chain_assign(self, item):
        *targs, value = item
        last_idx = len(targs) - 1
        with self.buffered() as buffer:
            self.traverse(value)
        value = ''.join(buffer)
        for i, x in enumerate(targs):
            if isinstance(x, Name):
                value = f"({self.get_name(x.id)}:={value})"
                continue
            elif i != last_idx:
                return
            self.traverse(x.value)
            ic = i != 0
            if isinstance(x, Attribute):
                self.write(f".__setattr__({self.gs(x.attr)},{value[ic:-ic]})")
            elif isinstance(x, Subscript):
                with self.delimit(f".__setitem__({self.porpv(slice)}(", f"),{value[ic:-ic]})"):
                    if isinstance(sl := x.slice, Slice):
                        self.traverse(lambda: self.write(','), self.traverse, (sl.start, sl.stop, sl.step))
                    else:
                        self.traverse(sl)
            break
        else:
            self.write(value)
    
    def visit_Assign(self, node):
        if isinstance(node.value, Tuple):
            try:
                it = zip(*map(lambda x: x.elts, node.targets), node.value.elts, strict=True)
            except AttributeError:
                return self.not_implemented(node)
            with self.buffered() as buffer:
                self.interleave(lambda: self.write(','), self._chain_assign, it)
            if not buffer or buffer[-1] is None:
                return self.not_implemented(node)
            self._source.extend(buffer)

UnparseObfuscate().visit(parse("a, b = None, True"))
