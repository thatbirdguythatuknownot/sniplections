"""
Rules for the obfuscator:
    Allowed operators/symbols:
        .                   - attribute access
        ,                   - comma
        (...)               - call (... stands for the arguments, if any)
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

# ensure that __builtins__ is always a module instead of a dict
import builtins as __builtins__
from sys import version_info
from warnings import warn
import importlib, re
from ast import *
from ast import _Precedence, _Unparser
from fractions import Fraction
from builtins import *
from functools import cache, reduce
from math import log2, trunc
from itertools import product

IS_311 = version_info >= (3, 11)
if not IS_311:
    print("\nThis obfuscator was primarily written for Python3.11, and some things may not work correctly in earlier versions.\n")
    warn(FutureWarning("use 3.11 or stuff might not work"))

def gbni(b): # get builtin name index
    return __builtins__.__dir__().index(b)

"""
def gosi(b): # get object subclass index
    return object.__subclasses__().index(b)
"""
def gifi(it, b): # get index from iterable
    if isinstance(it, dict):
        return next((k for k, v in it.items() if v == b), -1)
    try:
        return it.index(b)
    except AttributeError:
        return next((i for i, o in enumerate(it) if o == b), -1)
    except ValueError:
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

class no_hash_dict:
    def __init__(self, kwargs={}):
        self.keys = list(kwargs)
        self.vals = list(kwargs.values())
    def __contains__(self, val):
        return val in self.keys
    def __getitem__(self, key):
        return self.vals[self.keys.index(key)]
    def __setitem__(self, key, val):
        try:
            self.vals[self.keys.index(key)] = val
        except ValueError:
            self.keys.append(key)
            self.vals.append(val)
    def __repr__(self):
        return f"<no_hash_dict{{\n    keys={self.keys!r},\n    vals={self.vals!r}}}>"

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
        type: "__name__.__class__.__class__",
        dict: "__annotations__.__class__",
        True: "__name__.__eq__(__name__)",
        False: "__name__.__ne__(__name__)",
        None: "__name__.__getstate__()" if IS_311 else ("({0}:=__name__.__class__.__base__.__base__)", (), (), ()),
        list: ("({0}:=__name__.__dir__().__class__)", (), (), ()),
        tuple: ("({0}:=__name__.__class__.__bases__.__class__)", (), (), ()),
        object: ("({0}:=__name__.__class__.__base__)", (), (), ()),
        complex: ("({0}:=({0}:=__name__.__ne__(__name__).__invert__()).__neg__().__truediv__({0}.__add__({0}).__neg__()).__rpow__({0}).__class__)",
                  (), (), ()),
        open: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("open",), ()),
        oct: ("({0}:=__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1})))",
              ((__builtins__.__dir__, "oct"),), (), ()),
        re: ("({0}:={1}({2}))", (__import__,), ("re",), ()),
        __import__: ("({0}:=__builtins__.__getattribute__({1}))", (), ("__import__",), ()),
        slice: ("({0}:=__builtins__.__getattribute__({1}))", (), ("slice",), ()),
        globals: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("globals",), ()),
        chr: ("({0}:=__builtins__.__dict__.__getitem__({1}))", (), ("chr",), ()),
    }
    _default_cache_W = {
        "": "__name__.__class__()",
        0: ("({0}:=__name__.__class__().__len__())", (), (), ()),
        1: ("({0}:=__name__.__eq__(__name__).__pos__())", (), (), ()),
    }
    _default_object_repr_pair = {
        int: "__name__.__len__().__class__",
        str: "__name__.__class__",
        type: "__name__.__class__.__class__",
        dict: "__annotations__.__class__",
        True: "__name__.__eq__(__name__)",
        False: "__name__.__ne__(__name__)",
        None: "__name__.__getstate__()" if IS_311 else "__name__.__class__.__base__.__base__",
        list: "__name__.__dir__().__class__",
        tuple: "__name__.__class__.__bases__.__class__",
        object: "__name__.__class__.__base__",
        complex: "__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__add__(__name__.__eq__(__name__))).__rpow__(__name__.__ne__(__name__).__invert__()).__class__",
        open: ("__builtins__.__dict__.__getitem__({1})", (), ("open",), ()),
        oct: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
              ((__builtins__.__dir__, "oct"),), (), ()),
        re: ("{1}({2})", (__import__,), ("re",), ()),
        __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",), ()),
        slice: ("__builtins__.__getattribute__({1})", (), ("slice",), ()),
        globals: ("__builtins__.__dict__.__getitem__({1})", (), ("globals",), ()),
        chr: ("__builtins__.__dict__.__getitem__({1})", (), ("chr",), ()),
    }
    _default_cache = {
        "": "__name__.__class__()",
        0: "__name__.__len__().__class__()",
        1: "__name__.__eq__(__name__).__pos__()",
    }
    _default_nonassigned = {list, tuple, object, complex, open, oct, re, __import__, slice, globals, chr}
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
        self.ge_cache = no_hash_dict(self.object_repr_pair)
    
    def _uc(self, v, val): # update [get expression] cache
        self.ge_cache[v] = val
        return val
    
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
    
    def ge(self, v, name=None):
        """.ge(v): get expression
        Returns an obfuscated string that, when evaluated, is equivalent to `v`.
        Upon error, return -1. Upon not finding a suitable logic for `v`,
        return `None` obfuscated."""
        if v in self.ge_cache:
            try:
                return self.porpv(v)[0]
            except:
                return self.ge_cache[v]
        # Default to `None` obfuscated.
        # If `res` is unchanged, that means that `v` is one of the following:
        #     1. an instance of a builtin class that isn't
        #        supported by another of the obfuscator's methods
        #     2. an instance of a non-builtin class
        #     3. a (user-defined) function
        #     4. a generator
        # in that case, return `None` obfuscated.
        
        # For anyone questioning why use `.porpv()` for `None`, well of
        # course we could just use the string in the object repr pair,
        # but something might change that'll make it easier to do stuff
        # when we use `.porpv()`.
        is_porpvd = False
        res = self.porpv(None)[0]
        type_v = type(v)
        getattribute = object.__getattribute__
        if type_v is type(__builtins__):
            if v is __builtins__:
                res = "__builtins__"
            else:
                res = "{}({})".format(
                    self.ge(__import__),
                    self.gs(v.__name__)
                )
        elif is_num_or_str := type_v in (int, float):
            res = self.on(v)
        elif is_num_or_str := type_v is str:
            res = self.gs(v)
        elif is_porpvd := hasattr(v, "__hash__") and v.__hash__ and v in self.object_repr_pair:
            res = self.porpv(v)[0]
        elif v in __builtins__.__dict__.values():
            res = (
                "__builtins__.__getattribute__("
                "__builtins__.__dir__().__getitem__({}))"
            ).format(
                self.on(gbni(gifi(__builtins__.__dict__, v)))
            )
        elif (type_v is type or type in type_v.__bases__) and \
              getattribute(type_v, "__new__") is type.__new__:
            res = "{}({},{},{})".format(
                self.ge(type_v),
                self.gs(v.__name__),
                self.ge(v.__bases__),
                self.ge(dict(v.__dict__))
            )
        elif type_v is tuple:
            res = (
                "{}()".format(self.ge(tuple)) + \
                f"{'.__add__(({},))' * len(v)}"
            ).format(*map(self.ge, v))
        elif type_v in (list, set):
            # there's probably a better way to do this
            res = "{}({})".format(
                self.ge(type_v),
                self.ge(tuple(v)),
            )
        elif type_v is dict:
            res = "__annotations__.__class__({})".format(
                self.ge(tuple(v.items()))
            )
        else:
            try:
                if eval(repr(v)) == v:
                    res = "{}({})".format(
                        self.ge(eval),
                        self.gs(repr(v))
                    )
            except:
                pass
        if self.no_walrus:
            return self._uc(v, res)
        else:
            name = name or self.nn()
            if is_num_or_str:
                if ':=' in res:
                    _name, res = res[1:-1].split(':=', 1)
                    self.taken.remove(_name)
                self.cache[v] = name
            elif is_porpvd:
                self.object_repr_pair[v] = name
            self._uc(v, name)
            return f"({name}:={res})"
    
    def on(self, x, name_=None):
        """.on(x, name_=None): obfuscate number
        Obfuscate a number. `name_` is the temporary name used for temporary
        value assignments."""
        values = self.cache
        if x in values:
            val = values[x]
            if type(val) is str:
                return val
            return self.porpv(x, d=values)[0]
        name = None if self.no_walrus else (name_ or self.nn())
        if type(x) is float:
            numer, denom = Fraction(str(x)).as_integer_ratio()
            res = f"{self.on(numer, name)}.__truediv__({self.on(denom, name)})"
        elif type(x) is int:
            if x < 0:
                res = self.on(inv := ~x)
                values[x] = f"{values[inv]}.__invert__()"
                return f"{res}.__invert__()"
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
                        pb, qb = qb, trunc(pb) + 1
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
        else:
            raise TypeError((
                 "Obfuscator.on() argument must be a real number, "
                f"not '{type(x).__name__:200s}'"
            ))
        if self.no_walrus:
            values[x] = res
            return res
        values[x] = resname = self.nn() if name_ else name
        return f"({resname}:={res})"
    
    def gdci(self, c, name_=None):
        """.gdci(c, name_=None): get __doc__ and character index
        Returns (obfuscated representation of character `c`, index, True) or
        (obfuscated representation of `chr()`, obfuscated character ordinal,
         False)."""
        if type(c) is not str:
            raise TypeError("Obfuscator.gdci() expected string of length 1, "
                f"'{type(c).__name__:200s}' object found"
            )
        if len(c) != 1:
            raise ValueError("Obfuscator.gdci() expected a character, "
                f"but string of length {len(c)} found"
            )
        d = {}
        if self.no_walrus:
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
    
    def gs(self, s, name=None):
        """.gs(s): get string
        Gets the obfuscated representation of string `s`."""
        if type(s) is not str:
            raise TypeError("Obfuscator.gs() argument must be a string, "
                f"not '{type(c).__name__:200s}' object"
            )
        values = self.cache
        if s in values:
            return values[s]
        in_cache = {}
        for x in values:
            if not (isinstance(x, str) and x):
                continue
            i = s.find(x)
            if i != -1:
                in_cache[i] = (len(x), values[x])
        len_s = len(s)
        if self.no_walrus:
            l = []
            i = 0
            while i < len_s:
                if i in in_cache:
                    length, rep = in_cache[i]
                    l.append(rep)
                    i += length
                    continue
                rep, idx, to_getitem = self.gdci(s[i])
                l.append(f"{rep}{'.__getitem__'*to_getitem}({self.on(idx)})")
                i += 1
            res = reduce("{}.__add__({})".format, l)
            values[s] = res
            return res
        name = name or self.nn()
        l = []
        i = 0
        while i < len_s:
            if i in in_cache:
                length, rep = in_cache[i]
                l.append(rep)
                i += length
                continue
            rep, idx, to_getitem = self.gdci(s[i], name)
            l.append(f"{rep}{'.__getitem__'*to_getitem}({self.on(idx, name)})")
            i += 1
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
        self.ge_cache = no_hash_dict(self.object_repr_pair)

class Owobfuscwatowr(Obfuscator):
    def __init__(self, taken=None):
        super().__init__(taken)
        if not self.no_walrus:
            self.owo_names = product('wWmM', 'oODuUvVTQ', 'oOD0uUvVTQ')
    def nnu(self):
        for b, a, c in self.owo_names:
            if (name := a + b + c) not in self.taken:
                break
        return name
    def c(self):
        super().c()
        if not self.no_walrus:
            self.owo_names = product('wWmM', 'oODuUvVTQ', 'oOD0uUvVTQ')

builtins_dict = __builtins__.__dict__

# i'm finding a lot of flaws with this
# - caching when short circuiting
# - caching in any function call (so `random.random()` won't work)
# - NotImplemented (can't find a way to use __r*__ or flip compare dunders)
# literally just fix all those and this is fine but i can't do it easily
class UnparseObfuscate(_Unparser):
    type_cache = {}
    
    def __new__(cls, taken=None, *, init_obfuscator=Obfuscator):
        tup_classes = (cls, init_obfuscator)
        typ = cls.type_cache.get(tup_classes)
        if typ is None:
            class UnparserObfuscator(cls, init_obfuscator):
                __qualname__ = cls.__qualname__
            typ = cls.type_cache[tup_classes] = UnparserObfuscator
        inst = super().__new__(typ)
        return inst
    
    def __init__(self, taken=None, *, init_obfuscator=Obfuscator):
        self._source = []
        self._precedences = {}
        self._forbidden_named = {Module, Delete, Name, Assign, AugAssign,
                                 NamedExpr}
        self._indent = 0
        self.name_to_taken = {}
        self.unparse_cache = {}
        self._avoid_backslashes = True
        self.overridden_builtins = set()
        self._unparser = _Unparser()
        init_obfuscator.__init__(self, taken)
    
    def o(self, s):
        """.o(): obfuscate
        Obfuscate a string."""
        return self.visit(parse(s))
    
    def c(self):
        """.c(): clear
        Clears/resets obfuscator caches."""
        self.name_to_taken = {}
        self.unparse_cache = {}
        self.overridden_builtins = set()
        self._precedences = {}
        self._source = []
        self._indent = 0
        super().c()
    
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
        return getattr(self._unparser, f"visit_{name}", self._unparser.generic_visit)(node)
    
    def cvt_names(self, *names):
        t = []
        for name in names:
            t.append(self.name_to_taken.get(name, name))
        return t
    
    def get_name(self, id='', override=None, store=True):
        if not id:
            return self.nn()
        if not (name := self.name_to_taken.get(id)):
            if store and id in builtins_dict and id not in self.overridden_builtins:
                self.overridden_builtins.add(id)
            self.name_to_taken[id] = name = override or self.nn()
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
                self.write(res)
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
            value_unp = value[1:-1] if name else value
            if isinstance(x, Attribute):
                self.write(f".__setattr__({self.gs(x.attr)},{value_unp})")
            elif isinstance(x, Subscript):
                self.write(".__setitem__(")
                if isinstance(sl := x.slice, Slice):
                    with self.delimit(f"{self.porpv(slice)}(", ')'):
                        self.set_precedence(_Precedence.NAMED_EXPR, sl.start, sl.stop, sl.step)
                        self.interleave(lambda: self.write(','), self.traverse, (sl.start, sl.stop, sl.step))
                else:
                    self.traverse(sl)
                self.write(f",{value_unp})")
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
            return
        return self.not_implemented(node)
    
    def visit_AugAssign(self, node):
        if self._source:
            self.write("\n")
        if self.no_walrus:
            return self.not_implemented(node)
        bindunder = self.bindunder[node.op.__class__.__name__]
        dunder = f"__i{bindunder[2:]}"
        self.set_precedence(_Precedence.NAMED_EXPR, node.value)
        with self.buffered() as buffer:
            self.traverse(node.value)
        rhs = ''.join(buffer)
        if isinstance(t:=node.target, Name):
            with self.buffered() as buffer:
                node = Name(t.id, ctx=Load())
                self.set_precedence(node, _Precedence.NAMED_EXPR)
                self.traverse(node)
            res = ''.join(buffer)
            name = self.get_name(t.id)
            hasattr_s = self.ge(hasattr)
            if self.ident_check(hasattr_s[0]):
                hasattr_s = f" {hasattr_s}"
            self.write(f"({name}:={name}.{dunder}({rhs})if{hasattr_s}({res},{self.gs(dunder)})else {name}.{bindunder}({rhs}))")
            self.unparse_cache[t] = self.unparse_cache[t.id] = name
            return
        self.set_precedence(_Precedence.TUPLE, t.value)
        name = self.traverse(t.value)
        if isinstance(t, Attribute):
            self.write(f".__setattr__({self.gs(t.attr)},{name}.__getattribute__({self.gs(t.attr)}).{dunder}({rhs}))")
            return
        self.write(".__setitem__(")
        if isinstance(sl := t.slice, Slice):
            name_2 = self.get_name()
            with self.delimit(f"{name_2}:={self.porpv(slice)}(", ")"):
                self.set_precedence(_Precedence.NAMED_EXPR, sl.start, sl.stop, sl.step)
                self.interleave(lambda: self.write(','), self.traverse, (sl.start, sl.stop, sl.step))
        else:
            self.set_precedence(_Precedence.NAMED_EXPR, sl)
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
                    self.set_precedence(_Precedence.NAMED_EXPR, sl.start, sl.stop, sl.step)
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
    
    converts = {
        's': str,
        'r': repr,
        'a': ascii,
    }
    
    def visit_JoinedStr(self, node):
        in_add = False
        for val in node.values:
            if isinstance(val, Constant):
                rep = self.ge(val.value)
                if in_add:
                    with self.delimit(".__add__(", ")"):
                        self.write(rep)
                else:
                    self.write(rep)
                    in_add = True
            elif in_add:
                self.set_precedence(_Precedence.NAMED_EXPR, val.value)
                with self.delimit(".__add__(", ")"):
                    self.visit_FormattedValue(val)
            else:
                self.set_precedence(_Precedence.ATOM, val.value)
                self.visit_FormattedValue(val)
                in_add = True
    
    def visit_FormattedValue(self, node):
        format_s = self.ge(format)
        with self.buffered() as buffer:
            self.set_precedence(_Precedence.NAMED_EXPR, node.value)
            self.traverse(node.value)
        if has_conv := node.conversion != -1:
            conv_s = self.ge(self.converts[chr(node.conversion)])
            self.write(f"{conv_s}(")
        self.write(f"{format_s}(")
        self._source.extend(buffer)
        if fmtspec := node.format_spec:
            self.write(',')
            self.write(self.gs(fmtspec))
        self.write(')')
        if has_conv:
            self.write(')')
    
    def visit_Name(self, node):
        if isinstance(node.ctx, Store):
            self.write(self.get_name(node.id))
        else:
            if node.id in builtins_dict and node.id not in self.overridden_builtins:
                self.write(f"__builtins__.__getattribute__({self.gs(node.id)})")
            else:
                if not self.no_walrus:
                    is_not_taken = node.id not in self.name_to_taken
                    name = self.get_name(node.id, store=False)
                    if is_not_taken:
                        do_parens = False
                        if self.get_precedence(node) > _Precedence.NAMED_EXPR:
                            do_parens = True
                            self.write('(')
                        self.write(f"{name}:={node.id}")
                        if do_parens:
                            self.write(')')
                        return
                else:
                    name = node.id
                self.write(name)
    
    def visit_Constant(self, node):
        self.write(self.ge(node.value))
    
    def visit_List(self, node):
        fmt = f"{self.ge(list)}({{}})"
        if node.elts and all([not isinstance(x, Starred) or
                              not hasattr(x.value, 'elts') or
                              x.value.elts
                              for x in node.elts]):
            val = ""
            buf = None
            lenbuf = 0
            for x in node.elts:
                if not isinstance(x, Starred):
                    if buf is None:
                        buf = []
                    with self.buffered(buf):
                        if lenbuf:
                            self.write(',')
                        self.set_precedence(_Precedence.NAMED_EXPR, x)
                        self.traverse(x)
                        lenbuf += 1
                    continue
                if buf:
                    if lenbuf == 1:
                        buf.append(',')
                    if val:
                        val = f"{val}.__iadd__(({''.join(buf)}))"
                    else:
                        val = fmt.format(f"({''.join(buf)})")
                    buf = None
                    lenbuf = 0
                with self.buffered() as buffer:
                    self.set_precedence(_Precedence.NAMED_EXPR, x.value)
                    self.traverse(x.value)
                if val:
                    val = f"{val}.__iadd__({''.join(buffer)})"
                else:
                    val = fmt.format(''.join(buffer))
            if buf:
                if lenbuf == 1:
                    buf.append(',')
                if val:
                    val = f"{val}.__iadd__(({''.join(buf)}))"
                else:
                    val = fmt.format(f"({''.join(buf)})")
                buf = None
                lenbuf = 0
            self.write(val)
        else:
            self.write(val.format(""))
    
    def visit_Set(self, node):
        fmt = f"{self.ge(set)}({{}})"
        if node.elts and all([not isinstance(x, Starred) or
                              not hasattr(x.value, 'elts') or
                              x.value.elts
                              for x in node.elts]):
            val = ""
            buf = None
            lenbuf = 0
            for x in node.elts:
                if not isinstance(x, Starred):
                    if buf is None:
                        buf = []
                    with self.buffered(buf):
                        if lenbuf:
                            self.write(',')
                        self.set_precedence(_Precedence.NAMED_EXPR, x)
                        self.traverse(x)
                        lenbuf += 1
                    continue
                if buf:
                    if lenbuf == 1:
                        buf.append(',')
                    if val:
                        val = fmt.format(val, f"({''.join(buf)})")
                    else:
                        val = fmt.format(f"({''.join(buf)})")
                        fmt = f"{{}}.__ior__({fmt})"
                    buf = None
                    lenbuf = 0
                with self.buffered() as buffer:
                    self.set_precedence(_Precedence.NAMED_EXPR, x.value)
                    self.traverse(x.value)
                if val:
                    val = fmt.format(val, ''.join(buffer))
                else:
                    val = fmt.format(''.join(buffer))
                    fmt = f"{{}}.__ior__({fmt})"
            if buf:
                if lenbuf == 1:
                    buf.append(',')
                if val:
                    val = fmt.format(val, f"({''.join(buf)})")
                else:
                    val = fmt.format(f"({''.join(buf)})")
                buf = None
                lenbuf = 0
            self.write(val)
        else:
            self.write(fmt.format(""))
    
    def visit_Dict(self, node):
        if any(x is not None for x in node.keys):
            tzip = self.ge(zip)
        else:
            tzip = None
        fmt = f"{self.ge(dict)}({{}})"
        if node.keys and (tzip is not None or all([
                not hasattr(val, 'elts') or val.elts
                for key, x in zip(node.keys, node.values)
                if key is None])):
            val = ""
            buf = buf2 = None
            lenbuf = 0
            for key, x in zip(node.keys, node.values):
                if key is not None:
                    if buf is None:
                        buf = []
                        buf2 = []
                    with self.buffered(buf):
                        if lenbuf:
                            self.write(',')
                        self.set_precedence(_Precedence.NAMED_EXPR, key)
                        self.traverse(key)
                        lenbuf += 1
                    with self.buffered(buf2):
                        if lenbuf > 1:
                            self.write(',')
                        self.set_precedence(_Precedence.NAMED_EXPR, x)
                        self.traverse(x)
                    continue
                if buf:
                    if lenbuf == 1:
                        buf.append(',')
                        buf2.append(',')
                    if val:
                        val = fmt.format(val, f"{tzip}(({''.join(buf)}),({''.join(buf2)}))")
                    else:
                        val = fmt.format(f"{tzip}(({''.join(buf)}),({''.join(buf2)}))")
                        fmt = f"{{}}.__ior__({fmt})"
                    buf = None
                    lenbuf = 0
                with self.buffered() as buffer:
                    self.set_precedence(_Precedence.NAMED_EXPR, x)
                    self.traverse(x)
                if val:
                    val = fmt.format(val, ''.join(buffer))
                else:
                    val = fmt.format(''.join(buffer))
                    fmt = f"{{}}.__ior__({fmt})"
            if buf:
                if lenbuf == 1:
                    buf.append(',')
                    buf2.append(',')
                if val:
                    val = fmt.format(val, f"{tzip}(({''.join(buf)}),({''.join(buf2)}))")
                else:
                    val = fmt.format(f"{tzip}(({''.join(buf)}),({''.join(buf2)}))")
                buf = None
                lenbuf = 0
            self.write(val)
        else:
            self.write(fmt.format(""))
    
    def visit_Tuple(self, node):
        fmt = f"{self.ge(tuple)}({{}})"
        if node.elts and all([not isinstance(x, Starred) or
                              not hasattr(x.value, 'elts') or
                              x.value.elts
                              for x in node.elts]):
            val = ""
            buf = None
            lenbuf = 0
            for x in node.elts:
                if not isinstance(x, Starred):
                    if buf is None:
                        buf = []
                    with self.buffered(buf):
                        if lenbuf:
                            self.write(',')
                        self.set_precedence(_Precedence.NAMED_EXPR, x)
                        self.traverse(x)
                        lenbuf += 1
                    continue
                if buf:
                    if lenbuf == 1:
                        buf.append(',')
                    if val:
                        val = fmt.format(val, ''.join(buffer))
                    else:
                        val = f"({''.join(buf)})"
                        fmt = f"{{}}.__add__({fmt})"
                    buf = None
                    lenbuf = 0
                with self.buffered() as buffer:
                    self.set_precedence(_Precedence.NAMED_EXPR, x.value)
                    self.traverse(x.value)
                if val:
                    val = fmt.format(val, ''.join(buffer))
                else:
                    val = fmt.format(''.join(buffer))
                    fmt = f"{{}}.__add__({fmt})"
            if buf:
                if lenbuf == 1:
                    buf.append(',')
                if val:
                    val = fmt.format(val, f"({''.join(buf)})")
                else:
                    val = f"({''.join(buf)})"
                buf = None
                lenbuf = 0
            self.write(val)
        else:
            self.write(fmt.format(""))
    
    unsuf = {
        "Invert": ".__invert__()",
        "Not": ".__bool__().__ne__(__name__.__eq__(__name__))",
        "UAdd": ".__pos__()",
        "USub": ".__neg__()"
    }
    
    def visit_UnaryOp(self, node):
        self.set_precedence(_Precedence.ATOM, node.operand)
        self.traverse(node.operand)
        self.write(self.unsuf[node.op.__class__.__name__])
    
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
    
    def visit_BinOp(self, node):
        self.set_precedence(_Precedence.ATOM, node.left)
        self.traverse(node.left)
        self.write(f".{self.bindunder[node.op.__class__.__name__]}(")
        self.set_precedence(_Precedence.NAMED_EXPR, node.right)
        self.traverse(node.right)
        self.write(")")
    
    cmpfmts = {
        "Eq": ".__eq__({})",
        "NotEq": ".__ne__({})",
        "Lt": ".__lt__({})",
        "LtE": ".__le__({})",
        "Gt": ".__gt__({})",
        "GtE": ".__ge__({})",
        "In": ".__contains__({})",
        "NotIn": ".__contains__({}).__ne__(__name__.__eq__(__name__))",
        "Is": "is",
        "IsNot": "is not",
    }
    
    def ident_check(self, x):
        return x.isalpha() or x == '_'
    
    def visit_Compare(self, node):
        with self.require_parens(_Precedence.AND, node):
            for i, x in enumerate(node.ops):
                left = node.comparators[i - 1] if i != 0 else node.left
                right = node.comparators[i]
                name = x.__class__.__name__
                if i != 0:
                    left = node.comparators[i - 1]
                    if self._source and self.ident_check(self._source[-1][-1]):
                        self.write(" ")
                    self.write("and")
                else:
                    left = node.left
                if "In" in name:
                    left, right = right, left
                elif "Is" in name:
                    self.set_precedence(_Precedence.CMP.next(), left, right)
                    with self.buffered() as buffer:
                        self.traverse(left)
                    if self.ident_check(buffer[0][0]):
                        self.write(" ")
                    self._source.extend(buffer)
                    if self.ident_check(buffer[-1][-1]):
                        self.write(" ")
                    self.write(self.cmpfmts[name])
                    with self.buffered() as buffer:
                        self.traverse(right)
                    if self.ident_check(buffer[0][0]):
                        self.write(" ")
                    self._source.extend(buffer)
                    continue
                self.set_precedence(_Precedence.ATOM, left)
                with self.buffered() as buffer:
                    self.traverse(left)
                if i != 0 and self.ident_check(buffer[0][0]):
                    self.write(" ")
                self._source.extend(buffer)
                self.set_precedence(_Precedence.NAMED_EXPR, right)
                with self.buffered() as buffer:
                    self.traverse(right)
                self.write(self.cmpfmts[name].format(''.join(buffer)))
    
    def visit_BoolOp(self, node):
        op = self.boolops[node.op.__class__.__name__]
        prec = self.boolop_precedence[op]
        with self.require_parens(self.boolop_precedence[op], node):
            i = 0
            len_vals = len(node.values)
            while i < len_vals:
                left = node.values[i]
                right = node.values[i + 1]
                i += 2
                with self.buffered() as buffer:
                    self.traverse(left)
                if i != 0 and self.ident_check(buffer[0][0]):
                    self.write(" ")
                self._source.extend(buffer)
                self.write(op)
                with self.buffered() as buffer:
                    self.traverse(right)
                if i != 0 and self.ident_check(buffer[-1][-1]):
                    self.write(" ")
                self._source.extend(buffer)
    
    def visit_Attribute(self, node):
        attr = node.attr
        if attr.startswith('__') and attr.endswith('__'):
            self.set_precedence(_Precedence.ATOM, node.value)
            self.traverse(node.value)
            self.write('.')
            self.write(attr)
        else:
            self.set_precedence(_Precedence.NAMED_EXPR, node.value)
            getattr_s = self.ge(getattr)
            with self.delimit(f"{getattr_s}(", ")"):
                self.traverse(node.value)
                self.write(',')
                self.write(self.gs(node.attr))
    
    def visit_Call(self, node):
        self.set_precedence(_Precedence.ATOM, node.func)
        self.traverse(node.func)
        with self.delimit("(", ")"):
            comma = False
            for e in node.args:
                if comma:
                    self.write(",")
                else:
                    comma = True
                self.set_precedence(_Precedence.NAMED_EXPR, e)
                self.traverse(e)
            for e in node.keywords:
                if comma:
                    self.write(",")
                else:
                    comma = True
                self.traverse(e)
    
    def visit_Starred(self, node):
        self.write("*")
        self.set_precedence(_Precedence.EXPR, node.value)
        self.traverse(node.value)
    
    def visit_Ellipsis(self, node):
        self.write(self.ge(...))
    
    def visit_Slice(self, node):
        slice_s = self.ge(slice)
        with self.delimit(f"{slice_s}(", ")"):
            self.set_precedence(_Precedence.NAMED_EXPR, node.lower, node.upper, node.step)
            if node.lower:
                self.traverse(node.lower)
            if node.upper:
                self.traverse(node.upper)
            if node.step:
                self.traverse(node.step)
    
    visit_Match = visit_match_case = visit_MatchValue = visit_MatchSingleton = \
    visit_MatchSequence = visit_MatchStar = visit_MatchMapping = visit_MatchClass = \
    visit_MatchAs = visit_MatchOr = not_implemented
    visit_arg = visit_arguments = visit_keyword = not_implemented
    visit_Lambda = not_implemented
    visit_alias = not_implemented
    visit_withitem = not_implemented
