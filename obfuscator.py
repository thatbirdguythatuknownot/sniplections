"""
Rules for the obfuscator:
    Allowed operators/symbols:
        .                   - attribute access
        ,                   - comma
        (...)               - call (... stands for the arguments)
        :=                  - walrus operator/assignment expression
                              can be disabled by passing `taken=False` to
                              the obfuscator
    Only dunders (e.g. `__some_dunder__`) or underscores (e.g. `____`) are
    the allowed identifiers (names/attributes).

    The obfuscator unparser class may not follow these rules when it
    encounters a node that cannot be obfuscated easily. It may also replace
    names that refer to builtins with the builtin's representation. Example:

    print(int("5"))

    ...will result in...

    <obfuscated print>(<obfuscated int>(<obfuscated "5">))

    Take note that this will not work when a builtin is assigned to, but the
    code only detects builtins assigned in the normal way.

    int = lambda x: x+2
    int(5)

    ...should result in...

    <obfuscated name 0>=lambda <obfuscated name 1>:<obfuscated name 1>.__add__(<obfuscated 2>)
    <obfuscated name 0>(<obfuscated 5>)

    ...and...

    globals()["int"] = lambda x: x+2
    int(5)

    ...should result in...

    <obfuscated globals>().__setitem__(<obfuscated "int">,lambda <obfuscated name 0>:<obfuscated name 0>.__add__(<obfuscated 2>))
    <obfuscated int>(5)
"""
import importlib, re
from ast import *
from ast import _Precedence, _Unparser
from builtins import *
from functools import cache, reduce
from math import log2, trunc

"""
def gbni(b): # get builtin name index
    return __builtins__.__dir__().index(b)

def gosi(b): # get object subclass index
    return object.__subclasses__().index(b)
"""

def gifi(it, b): # get index from iterable
    try:
        return it.index(b)
    except AttributeError:
        return next((i for i, o in enumerate(it) if o == b), -1)
    except ValueError:
        pass
    return -1

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
    Obfuscate Python code. Outputs code with underscore-only names,
    dunder-only attributes, and no constants.

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
    _default_object_repr_pair_W = {
        int: "__name__.__len__().__class__",
        str: "__name__.__class__",
        type: "__loader__.__class__",
        dict: "__annotations__.__class__",
        True: "__name__.__eq__(__name__)",
        False: "__name__.__ne__(__name__)",
        None: "___name__.__getstate__()",
        object: ("({0}:=__name__.__class__.__base__)", (), (), ()),
        complex: ("({0}:=({0}:=__name__.__ne__(__name__).__invert__()).__neg__().__truediv__({0}.__add__({0}).__neg__()).__rpow__({0}).__class__)",
                  (), (), ()),
        open: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("open",), ()),
        oct: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
              ((__builtins__.__dir__, "oct"),), (), ()),
        re: ("({0}:={1}({2}))", (__import__,), ("re",), ()),
        __import__: ("({0}:=__builtins__.__getattribute__({1}))", (), ("__import__",), ()),
        setattr: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
                  ((__builtins__.__dir__, "setattr"),), (), ()),
        slice: ("({0}:=__builtins__.__getattribute__({1}))", (), ("slice",), ()),
        globals: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("globals",), ()),
        chr: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("chr",), ()),
    }
    _default_cache_W = {
        "": "__name__.__class__()",
        0: "__name__.__class__().__len__()",
        1: ("({0}:=__name__.__eq__(__name__).__pos__())", (), (), ()),
    }
    _default_object_repr_pair = {
        int: "__name__.__len__().__class__",
        str: "__name__.__class__",
        type: "__loader__.__class__",
        dict: "__annotations__.__class__",
        True: "__name__.__eq__(__name__)",
        False: "__name__.__ne__(__name__)",
        None: "___name__.__getstate__()",
        object: "__name__.__class__.__base__",
        complex: "__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__add__(__name__.__eq__(__name__))).__rpow__(__name__.__ne__(__name__).__invert__()).__class__",
        open: ("__builtins__.__dict__.__getitem__({1})", (), ("open",), ()),
        oct: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
              ((__builtins__.__dir__, "oct"),), (), ()),
        re: ("{1}({2})", (__import__,), ("re",), ()),
        __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",), ()),
        setattr: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
                  ((__builtins__.__dir__, "setattr"),), (), ()),
        slice: ("__builtins__.__getattribute__({1})", (), ("slice",), ()),
        globals: ("__builtins__.__dict__.__getitem__({1})", (), ("globals",), ()),
        chr: ("__builtins__.__dict__.__getitem__({1})", (), ("chr",), ()),
    }
    _default_cache = {
        "": "__name__.__class__()",
        0: "__name__.__class__().__len__()",
        1: "__name__.__eq__(__name__).__pos__()",
    }
    _default_nonassigned = {object, complex, open, oct, re, __import__, setattr, slice, globals, chr}
    def __init__(self, taken=None):
        self.forbidden_chars = set()
        self.no_walrus = taken is False
        if self.no_walrus:
            self.object_repr_pair = self._default_object_repr_pair.copy()
            self.cache = self._default_cache.copy()
        else:
            if taken is None or taken is True:
                taken = set()
            self.taken = taken
            self.object_repr_pair = self._default_object_repr_pair_W.copy()
            self.cache = self._default_cache_W.copy()
            self._nonassigned = self._default_nonassigned.copy()
    
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
    
    def porpv(self, x, name=None, d=None):
        """.porpv(x, name=None, d=None): process object repr pair value
        Get a value from `d` (`d` defaults to the internal
        object-to-representation mapping) using `x` as the key and process
        it so the proper string value is returned. Resulting string is
        assigned to `name` and `name` is assigned to `d[x]`."""
        if d is None:
            d = self.object_repr_pair
        v = d[x]
        if type(v) is str:
            return v, v
        if any(c in b for a in v[2] for b in a for c in self.forbidden_chars):
            return ".*.", ".*."
        if self.no_walrus:
            d[x] = res = v[0].format(
                None,
                *map(lambda y: self.on(gifi(y[0](), y[1])) if isinstance(y, tuple) else self.porpv(y, d=d)[0], v[1]), 
                *map(self.gs, v[2]),
                *map(self.on, v[3])
            )
            return res, None
        if name is None:
            name = self.nn()
        res = v[0].format(
            name,
            *map(lambda y: self.on(gifi(y[0](), y[1]), name) if isinstance(y, tuple) else self.porpv(y, d=d)[0], v[1]), 
            *map(self.gs, v[2]),
            *map(self.on, v[3])
        )
        if ".*." in res:
            return ".*.", ".*."
        if x in self._nonassigned:
            self._nonassigned.remove(x)
        d[x] = name
        return res, name
    
    def on(self, x, name_=None):
        """.on(x, name_=None): obfuscate number
        Obfuscate a number. `name_` is the temporary name used for temporary
        value assignments."""
        values = self.cache
        if x < 0:
            res = self.on(inv := ~x)
            values[x] = f"{values[inv]}.__invert__()"
            return f"{res}.__invert__()"
        if x in values:
            val = values[x]
            if type(val) is str:
                return val
            return self.porpv(x, d=values)[0]
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
        Returns (obfuscated representation of character `c`, index, True) or
        (obfuscated representation of `chr()`, obfuscated character ordinal,
         False)."""
        d = {}
        if self.no_walrus:
            for x, rep in self.object_repr_pair.items():
                if x.__doc__ and (r := doci.__doc__.find(c)) >= 0:
                    d[x] = [(doci, "__doc__", rep)]
                if hasattr(x, "__name__") and x.__name__ and (namei := x.__name__.find(c)) >= 0:
                    if not d.get(x):
                        d[x] = [(namei, "__name__", rep)]
                    else:
                        d[x].append((namei, "__name__", rep))
                if hasattr(x, "__module__") and x.__module__ and (modi := x.__module__.find(c)) >= 0:
                    if not d.get(x):
                        d[x] = [(modi, "__module__", rep)]
                    else:
                        d[x].append((modi, "__module__", rep))
            self.forbidden_chars.add(c)
            while d:
                x, (r, a, _) = min(map(lambda x: (x[0], min(x[1], key=lambda y: y[0])), d.items()), key=lambda x: x[1][0])
                rep = self.porpv(x)[0]
                if ".*." not in rep:
                    break
                del d[x]
            self.forbidden_chars.remove(c)
            if d:
                return f"{rep}.{a}", r, True
        else:
            name = name_ or self.nnu()
            for x, rep in self.object_repr_pair.items():
                if x.__doc__ and (doci := x.__doc__.find(c)) >= 0:
                    d[x] = [(doci, "__doc__", rep)]
                if hasattr(x, "__name__") and x.__name__ and (namei := x.__name__.find(c)) >= 0:
                    if not d.get(x):
                        d[x] = [(namei, "__name__", rep)]
                    else:
                        d[x].append((namei, "__name__", rep))
                if hasattr(x, "__module__") and x.__module__ and (modi := x.__module__.find(c)) >= 0:
                    if not d.get(x):
                        d[x] = [(modi, "__module__", rep)]
                    else:
                        d[x].append((modi, "__module__", rep))
            self.forbidden_chars.add(c)
            while d:
                x, (r, a, rep) = min(map(lambda x: (x[0], min(x[1], key=lambda y: y[0])), d.items()), key=lambda x: x[1][0])
                if x in self._nonassigned:
                    rep = self.porpv(x, self.nn() if name_ else name)[0]
                if ".*." not in rep:
                    break
                del d[x]
            self.forbidden_chars.remove(c)
            if d:
                rep = f"{rep}.{a}"
                if ':=' in rep and not name_:
                    self.taken.add(name)
                return rep, r, True
        return self.porpv(chr)[0], ord(c), False
    
    def gs(self, s):
        """.gs(s): get string
        Gets the obfuscated representation of string `s`."""
        values = self.cache
        if s in values:
            return values[s]
        if self.no_walrus:
            l = []
            for c in s:
                rep, idx, to_getitem = self.gdci(c)
                l.append(f"{rep}{'.__getitem__'*to_getitem}({self.on(idx)})")
            res = reduce("{}.__add__({})".format, l)
            values[s] = res
            return res
        name = self.nn()
        l = []
        for c in s:
            rep, idx, to_getitem = self.gdci(c, name)
            l.append(f"{rep}{'.__getitem__'*to_getitem}({self.on(idx, name)})")
        res = reduce("{}.__add__({})".format, l)
        values[s] = name
        return f"({name}:={res})"
    
    def c(self):
        """.c(): clear
        Clears/resets obfuscator caches."""
        self.forbidden_chars = set()
        if self.no_walrus:
            self.object_repr_pair = self._default_object_repr_pair.copy()
            self.cache = self._default_cache.copy()
        else:
            self.taken.clear()
            self.object_repr_pair = self._default_object_repr_pair_W.copy()
            self.cache = self._default_cache_W.copy()
            self._nonassigned = self._default_nonassigned.copy()

builtins_dict = __builtins__.__dict__

class UnparseObfuscate(_Unparser, Obfuscator):
    def __init__(self, taken=None):
        self._source = []
        self._precedences = {}
        self._forbidden_named = {Module, Delete, Name, Assign, AugAssign,
                                 NamedExpr}
        self.name_to_taken = {}
        self.unparse_cache = {}
        self._avoid_backslashes = True
        self.overridden_builtins = set()
        self._unparser = _Unparser()
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
    
    def get_name(self, id='', override=None, store=True):
        if not id:
            return self.nn()
        if not (name := self.name_to_taken.get(id)):
            if store and id in builtins_dict and id not in self.overridden_builtins:
                self.overridden_builtins.add(id)
            name = override or self.nn()
            self.name_to_taken[id] = name
        return name
    
    def traverse(self, node, name=None):
        if isinstance(node, list):
            for item in node:
                self.traverse(item)
        else:
            s = None
            do_parens = False
            if res := (self.unparse_cache.get(node)
                    or self.unparse_cache.get(s := self._unparser.visit(node))):
                if s:
                    self._unparser._source.clear()
                return res
            self._unparser._source.clear()
            if not self.no_walrus and name is None and node.__class__ not in self._forbidden_named:
                name = self.get_name()
                if self.get_precedence(node) > _Precedence.NAMED_EXPR:
                    do_parens = True
                    self.write('(')
                self.write(f"{name}:=")
            NodeVisitor.visit(self, node)
            if do_parens:
                self.write(')')
            if name:
                self.unparse_cache[node] = self.unparse_cache[s] = name
                return name
    
    def anon_traverse(self, node):
        self.traverse(node, name=False)
    
    def visit_Module(self, node):
        self.anon_traverse(node.body)
    
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
                        lambda _name: self.write(f"({self.get_name(_name.asname)}:={name}({self.gs(_name.name)}))"),
                        it)
    
    visit_ImportFrom = not_implemented
    
    def _chain_assign(self, item):
        *targs, value = item
        last_idx = len(targs) - 1
        with self.buffered() as buffer:
            name = self.traverse(value)
        value = ''.join(buffer)
        for i, x in enumerate(targs):
            if isinstance(x, Name):
                value = f"({self.get_name(x.id, name)}:={value})"
                if name:
                    name = None
                continue
            elif i != last_idx:
                return
            self.traverse(x.value)
            ic = i != 0 or isinstance(item[-1], NamedExpr)
            if isinstance(x, Attribute):
                self.write(f".__setattr__({self.gs(x.attr)},{value[ic:-ic]})")
            elif isinstance(x, Subscript):
                self.write(".__setitem__(")
                if isinstance(sl := x.slice, Slice):
                    with self.delimit(f"{self.porpv(slice)}(", ')'):
                        self.interleave(lambda: self.write(','), self.traverse, (sl.start, sl.stop, sl.step))
                else:
                    self.traverse(sl)
                self.write(f",{value[ic:-ic]})")
            else:
                return
            break
        else:
            self.write(value)
    
    def visit_Assign(self, node):
        if not self.no_walrus:
            if isinstance(node.value, Tuple):
                if len(node.targets) != 1:
                    return self.not_implemented(node)
                try:
                    it = zip(node.targets[0].elts, node.value.elts, strict=True)
                except AttributeError:
                    return self.not_implemented(node)
            else:
                it = node.targets + [node.value]
            with self.buffered() as buffer:
                self._chain_assign(it)
            if buffer and buffer[-1] is not None:
                self._source.extend(buffer)
        return self.not_implemented(node)
    
    def visit_AugAssign(self, node):
        if self.no_walrus:
            return self.not_implemented(node)
        dunder = f"__i{self.bindunder[node.op.__class__.__name__][2:]}"
        self.set_precedence(node.value, _Precedence.NAMED_EXPR)
        with self.buffered() as buffer:
            self.traverse(node.value)
        rhs = ''.join(buffer)
        if isinstance(t:=node.target, Name):
            with self.buffered() as buffer:
                self.visit(Name(t.id, ctx=Load()))
                self.visit(t)
            res, name = buffer
            self.write(f"({name}:={res}.{dunder}({rhs}))")
            self.unparse_cache[t] = self.unparse_cache[t.id] = name
            return
        name = self.get_name()
        with self.delimit(f"({name}:=", ')'):
            self.traverse(t.value)
        if isinstance(t, Attribute):
            self.write(f".__setattr__({self.gs(t.attr)}, {name}.__getattribute__({self.gs(t.attr)}).{dunder}({rhs}))")
            return
        self.write(".__setitem__(")
        if isinstance(sl := t.slice, Slice):
            name_2 = self.get_name()
            with self.delimit(f"{name_2}:={self.porpv(slice)}(", ")"):
                self.interleave(lambda: self.write(','), self.traverse, (sl.start, sl.stop, sl.step))
        else:
            name_2 = self.traverse(sl)
        self.write(f",{name}.__getitem__({name_2}).{dunder}({rhs}))")
    
    visit_AnnAssign = visit_Return = not_implemented
    
    def visit_Pass(self, node):
        self.write(self.porpv(None)[0])
    
    visit_Break = visit_Continue = not_implemented
    
    def _delete_inner(self, node):
        if isinstance(node, Name):
            if node.id in self.overridden_builtins:
                self.overridden_builtins.remove(node.id)
            else:
                self.write(f"{self.porpv(globals)}().__delitem__({self.gs(self.get_name(node.id, store=False))})")
            return
        self.traverse(node.value)
        if isinstance(node, Attribute):
            self.write(f".__delattr__({self.gs(node.attr)})")
        else:
            self.write(".__delitem__(")
            if isinstance(sl := node.slice, Slice):
                with self.delimit(f"{self.porpv(slice)}(", ')'):
                    self.interleave(lambda: self.write(','), self.traverse, (sl.start, sl.stop, sl.step))
            else:
                self.traverse(sl)
            self.write(')')
    
    def visit_Delete(self, node):
        self.interleave(lambda: self.write(','), self._delete_inner, node.targets)
    
    visit_Assert = visit_Global = visit_Nonlocal = visit_Await = visit_Yield = visit_YieldFrom = not_implemented
    visit_Raise = visit_Try = visit_TryStar = visit_ExceptHandler = not_implemented
    
    visit_ClassDef = visit_FunctionDef = not_implemented # TODO
    
    visit_AsyncFunctionDef = not_implemented
    
    visit_For = not_implemented # TODO
    
    visit_AsyncFor = not_implemented
    visit_If = not_implemented
    
    visit_While = visit_With = not_implemented # TODO
    visit_AsyncWith = not_implemented
    
    visit_FormattedValue = not_implemented # TODO
    
    def visit_Name(self, node):
        if isinstance(node.ctx, Store):
            self.write(self.get_name(node.id))
        else:
            if node.id in builtins_dict and node.id not in self.overridden_builtins:
                self.write(f"__builtins__.__getattribute__({self.gs(node.id)})")
            else:
                self.write(self.get_name(node.id, store=False))
    
    bindunder = {
        "Add": "__add__",
        "Sub": "__sub__",
        "Mult": "__mul__",
        "MatMult": "__matmul__",
        "Div": "__truediv__",
        "Mod": "__mod__",
        "LShift": "__lshift__",
        "RShift": "__rshift__",
        "BitOr": "__or__",
        "BitXor": "__xor__",
        "BitAnd": "__and__",
        "FloorDiv": "__floordiv__",
        "Pow": "__pow__",
    }
