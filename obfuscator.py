"""
Rules:
    Allowed operators:
        attribute access:                               "."
        item separator:                                 ","
        call operator, with args but no kwargs:         "([args])"
    The only builtin (provided by Python) identifiers that are allowed to be used are dunders
        For example, sum is not allowed but __import__ is. Instead, use __builtins__:
        __builtins__.__getattribute__([expression that builds the string name of what you want])
        If the identifier needed is local or global, instead of part of __builtins__, use
        __builtins__ to get either globals or locals and use as needed.
        other methods can be used, but __builtins__ access is probably the 
        shortest and most version compatible way to do it
    The only (direct) attribute access allowed is dunder attributes
        the following is to be used instead of someobj.someattr:
        someobj.__getattribute__([expression that builds the string "someattr"])
    No keywords
    No constant expressions
    No statements (such as "x = y")
    
    Additionally, if walrus assignment is enabled, the following may be used:
        walrus operator (:=)
"""


import importlib, re
from ast import *
from ast import _Precedence, _Unparser
from functools import cache, reduce
from math import log2, trunc


#def gbni(b): # get builtin name index
#    return __builtins__.__dir__().index(b)

#def gosi(b): # get object subclass index
#    return __object__.__subclasses__().index(b)


def gifi(it, b): # get index from iterable
    try:
        # will work if it is list, tuple, or string
        return it.index(b)
    except:
        # this is for if it is a set, dict, or other such iterable
        return ([i for i, o in it if o == b] or [None])[0]


#def _gp(y, D={}): # generate primes
#    q = 2
#    while q < y:
#        if q not in D:
#            yield q
#            D[q * q] = [q]
#        else:
#            for p in D[q]:
#                D.setdefault(p + q, []).append(p)
#            del D[q]
#        q += 1

@cache
def gpf(x): # greatest prime factor
    factors = [i for i in _gp(trunc(x**.5 + 1)) if not x % i]
    if factors:
        return factors[len(factors) >> 1]
    return x


@cache
def gpf(x): # greatest . factor
    return next((i for i in range(trunc(x**.5 + 1), 1, -1) if not x % i), x)

class Obfuscator:
    """Obfuscator(taken=None)
    Obfuscate Python code. Outputs code with underscore-only names
    and dunder-only attributes, using no constants.

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
        True: "__spec__.__eq__(__spec__)",
        False: "__spec__.__ne__(__spec__)",
        None: "__name__.__getstate__()",
        object: ("({0}:=__name__.__class__.__base__)", (), (), ()),
        complex: ("({0}:=({{0}}:=__name__.__ne__(__name__).__invert__()).__truediv__({{0}}:={{0}}.__neg__()).__pow__({{0}}.__truediv__({{0}}.__invert__().__neg__())).__class__)",
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
        "": ("({0}:=__name__.__getitem__({1}({2},{3},{2})))", (slice, None), (), (0,)),
    }
    _default_object_repr_pair = {
        int: "__name__.__len__().__class__",
        str: "__name__.__class__",
        type: "__loader__.__class__",
        dict: "__annotations__.__class__",
        True: "__spec__.__eq__(__spec__)",
        False: "__spec__.__ne__(__spec__)",
        None: "__name__.__getstate__()",
        object: "__name__.__class__.__base__",
        complex: "__name__.__ne__(__name__).__invert__().__truediv__(__name__.__eq__(__name__)).__pow__(__name__.__eq__(__name__).__truediv__(__name__.__eq__(__name__).__invert__().__neg__())).__class__",
        open: ("__builtins__.__dict__.__getitem__({1})", (), ("open",), ()),
        oct: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
              ((__builtins__.__dir__, "oct"),), (), ()),
        re: ("{1}({2})", (__import__,), ("re",), ()),
        __import__: ("__builtins__.__getattribute__({1})", (), ("__import__",), ()),
        setattr: ("__builtins__.__dict__.__getitem__(__builtins__.__dir__().__getitem__({1}))",
                  ((__builtins__.__dir__, "setattr"),), (), ()),
        slice: ("__builtins__.__getattribute__({1})", (), ("slice",), ()),
        globals: ("__builtins__.__dict__.__getitem__({1})", (), ("globals",), ()),
        "": ("__name__.__getitem__({1}({2},{3},{2}))", (slice, None), (), (0,)),
    }
    _default_nonassigned = {object, complex, open, oct, re, __import__, setattr, slice, globals, ""}
    def __init__(self, taken=None):
        self.cache = {
            0: "__name__.__len__().__class__()",
            1: "__name__.__eq__(__name__).__int__()",
        }
        self.no_walrus = taken is False
        if self.no_walrus:
            self.object_repr_pair = self._default_object_repr_pair.copy()
        else:
            if taken is None or taken is True:
                taken = set()
            self.taken = taken
            self.object_repr_pair = self._default_object_repr_pair_W.copy()
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
    
    def porpv(self, x, name=None):
        """.porpv(x, name=None): process object repr pair value
        Get a value from the internal object-to-representation mapping using
        `x` as the key and process it so the proper string value is returned.
        Resulting string is assigned to `name` and `name` is assigned to
        `.object_repr_pair[x]`."""
        v = self.object_repr_pair[x]
        if type(v) is not tuple:
            return v, v
        if self.no_walrus:
            self.object_repr_pair[x] = res = v[0].format(
                None,
                *map(lambda x: self.on(gifi(x[0](), x[1]), name) if isinstance(x, tuple) else self.porpv(x)[0], v[1]), 
                *map(self.gs, v[2]),
                *map(self.on, v[3])
            )
            return res, None
        if x in self._nonassigned:
            self._nonassigned.remove(x)
        if name is None:
            name = self.nn()
        self.object_repr_pair[x] = name
        res = v[0].format(
            name,
            *map(lambda x: self.on(gifi(x[0](), x[1]), name) if isinstance(x, tuple) else self.porpv(x)[0], v[1]), 
            *map(self.gs, v[2]),
                *map(self.on, v[3])
        )
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
                if x.__doc__ and (r := x.__doc__.find(c)) >= 0:
                    return f"{self.porpv(x)[0]}.__doc__", r
        name = name_ or self.nnu()
        for x, rep in self.object_repr_pair.items():
            if x.__doc__ and (r := x.__doc__.find(c)) >= 0:
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
        values = self.cache
        if s in values:
            return values[s]
        if s == "":
            res, values[s] = self.porpv("")
            return res
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
        self.cache = {
            0: "().__len__()",
            1: "((),).__len__()",
        }
        if self.no_walrus:
            self.object_repr_pair = self._default_object_repr_pair.copy()
        else:
            self.taken.clear()
            self.object_repr_pair = self._default_object_repr_pair_W.copy()
            self._nonassigned = self._default_nonassigned.copy()

builtins_dict = __builtins__.__dict__

class UnparseObfuscate(_Unparser, Obfuscator):
    def __init__(self, taken=None):
        self._source = []
        self._precedences = {}
        self._forbidden_named = {Module, Delete, Name}
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
            if name is None and node.__class__ not in self._forbidden_named:
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
                        lambda _name: f"({self.get_name(_name.asname)}:={name}({self.gs(_name.name)}))",
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
        if not buffer or buffer[-1] is None:
            return self.not_implemented(node)
        self._source.extend(buffer)
    
    def visit_AugAssign(self, node):
        dunder = f"__i{self.bindunder[node.op.__class__.__name__][2:]}"
        self.set_precedence(node.value, _Precedence.NAMED_EXPR)
        with self.buffered() as buffer:
            self.traverse(node.value)
        rhs = ''.join(buffer)
        if isinstance(t:=node.target, Name):
            self.write(f"({t.id}:={t.id}.{dunder}({rhs}))")
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
        self.write(self.get_name(node.id, store=isinstance(node.ctx, Store)))
    
    
    
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
    

# s="#########################";o=Obfuscator();o.porpv(open);s;o.on(1);s;o.gs('w');s;o.gs('write');s;o.gs('Hello world!\n')
