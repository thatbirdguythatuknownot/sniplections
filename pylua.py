from ast import *
from ast import _SINGLE_QUOTES
from contextlib import contextmanager
from copy import deepcopy
from enum import _simple_enum, IntEnum, auto

@_simple_enum(IntEnum)
class _Precedence:
    """Precedence table that originated from lua grammar."""
    TEST = auto()            # 'if'-'then'-'elseif'-'then'-'else'
    OR = auto()              # 'or'
    AND = auto()             # 'and'
    CMP = auto()             # '<', '>', '==', '>=', '<=', '~='
    EXPR = auto()
    BOR = EXPR               # '|'
    BXOR = auto()            # '~'
    BAND = auto()            # '&'
    SHIFT = auto()           # '<<', '>>'
    CONCAT = auto()          # '..'
    ARITH = auto()           # '+', '-'
    TERM = auto()            # '*', '/', '%', '//'
    UNARY = auto()           # unary 'not', '+', '-', '~'
    POWER = auto()           # '^'
    ATOM = auto()
    def next(self):
        try:
            return self.__class__(self + 1)
        except ValueError:
            return self

_ALL_QUOTES = (*_SINGLE_QUOTES, "[[")

class _LuaTranspiler(_Unparser):
    def __init__(self, *, _avoid_backslashes=False, luau=False):
        self._source = []
        self._precedences = {}
        self._type_ignores = {}
        self._indent = 0
        self._avoid_backslashes = _avoid_backslashes
        self._had_handler = False
        self.luau = luau
    def interleave_write(self, inter, f, seq):
        seq = iter(seq)
        try:
            while not (r := f(next(seq))): pass
        except StopIteration:
            pass
        else:
            self.write(r)
            for x in seq:
                if r := f(x):
                    inter()
                    self.write(r)
    @contextmanager
    def block(self, extra=None):
        """A context manager for preparing the source for blocks. It increases
        the indentation on enter and decreases the indentation on exit. If
        *extra* is given, it will be written before indenting.
        """
        if extra:
            self.write(extra)
        self._indent += 1
        yield
        self._indent -= 1
    @contextmanager
    def block_end(self, extra=None):
        """Like block, but with end.
        """
        if extra:
            self.write(extra)
        self._indent += 1
        yield
        self._indent -= 1
        self.fill("end")
    _luau_type = {
        "str": "string",
        "int": "number",
        "float": "number",
        "bool": "boolean",
    }
    def traverse(self, node, apply_func=lambda x: None):
        if isinstance(node, list):
            for item in node:
                apply_func(item)
                self.traverse(item, apply_func)
        else:
            if not apply_func(node):
                super().traverse(node)
    def type_traverse(self, node):
        if isinstance(node, list):
            for item in node:
                self.type_traverse(item)
        else:
            self.type_visit(node)
    def type_visit(self, node):
        method = f"visit_{node.__class__.__name__}"
        visitor = getattr(self, f"type_{method}", None)
        if not visitor:
            visitor = getattr(self, method, self.type_generic_visit)
        return visitor(node)
    def type_generic_visit(self, node):
        for field, value in iter_fields(node):
            if isinstance(value, list):
                for item in value:
                    if isinstance(item, AST):
                        self.type_visit(item)
            elif isinstance(value, AST):
                self.type_visit(value)
    def visit_FunctionType(self, node):
        if self.luau:
            with self.delimit("(", ")"):
                self.interleave(
                    lambda: self.write(", "), self.type_traverse, node.argtypes
                )
            self.write(" -> ")
            self.type_traverse(node.returns)
        else:
            with self.delimit("--[[", "]]"):
                with self.delimit("(", ")"):
                    self.interleave(
                        lambda: self.write(", "), self.type_traverse, node.argtypes
                    )
                self.write(" -> ")
                self.type_traverse(node.returns)
    def visit_Expr(self, node):
        self.fill()
        self.set_precedence(_Precedence.TEST, node.value)
        self.traverse(node.value)
    def visit_NamedExpr(self, node):
        raise NotImplementedError("named expressions are currently not supported")
    def import_process(self, subnames):
        if self.luau:
            res = f"require(script.Parent.{'.'.join(subnames)})"
        else:
            res = f'dofile("./{"/".join(subnames)}.lua")'
        if len(subnames) > 1:
            subnames = subnames[1:]
            while subnames and (subname := subnames.pop()):
                res = f"{{{subname} = {res}}}"
        elif subnames[0] in ("math", "os"):
            res = ""
        return res
    def visit_Import(self, node):
        self.fill("local ")
        names = [name.name.split('.') for name in node.names]
        self.interleave_write(lambda: self.write(", "), lambda x: x[0]*(x[0] not in ("math", "os")), names)
        self.write(" = ")
        self.interleave_write(lambda: self.write(", "), self.import_process, names)
    def visit_ImportFrom(self, node):
        if node.level > 1:
            if self.luau:
                res = f"script{'.Parent' * node.level}"
            else:
                res = "../" * (node.level - 1)
        elif node.level:
            if self.luau:
                res = "script"
            else:
                res = "./"
        else:
            if self.luau:
                res = "script.Parent"
            else:
                res = "./"
        names = [name.name for name in node.names]
        if not node.level and node.module in ("math", "os") and (len(names) != 1 or names[0] != "*"):
            for x in names:
                self.fill(f"local {x} = {node.module}.{x}")
            return
        self.fill()
        with self.block_end("do"):
            self.fill("local env = getfenv(1)")
            if node.module:
                if self.luau:
                    res = f"require({res}.{{}})"
                else:
                    res = f'dofile("{res}{{}}.lua")'
                res = res.format(node.module)
                if len(names) == 1 and names[0] == "*":
                    if not node.level and node.module in ("math", "os"):
                        res = node.module
                    self.fill(f"for k, v in {res} ")
                    with self.block_end("do"):
                        self.fill("env[k] = v")
                else:
                    self.fill(f"local mod = {res}")
                    for x in names:
                        self.fill(f"env.{x} = mod.{x}")
            else:
                if self.luau:
                    if len(names) == 1 and names[0] == "*":
                        self.fill(f"for k, v in {res}:GetChildren() ")
                        with self.block_end("do"):
                            self.fill("if v:IsA('ModuleScript') ")
                            with self.block_end("then"):
                                self.fill(f"env[k] = require(v)")
                    else:
                        self.fill(f"local inst = {res}")
                        for x in names:
                            self.fill(f"env.{x} = require(inst.{x})")
                else:
                    if len(names) == 1 and names[0] == "*":
                        raise NotImplementedError("from ... import * not yet supported")
                    else:
                        for x in names:
                            self.fill(f'env.{x} = dofile("{res}{x}.lua")')
    def assignment_apply(self, node):
        if isinstance(node, Starred):
            if isinstance(node.value, (List, Tuple)):
                self.interleave(lambda: self.write(", "), lambda x: self.traverse(x, self.assignment_apply), node.value.elts)
                return 1
            else:
                raise NotImplementedError("starred expression in assignment is not supported")
        elif isinstance(node, (List, Tuple)):
            self.interleave(lambda: self.write(", "), lambda x: self.traverse(x, self.assignment_apply), node.elts)
            return 1
    def visit_Assign(self, node):
        self.fill()
        prev_target = node.value
        for target in node.targets[::-1]:
            self.traverse(target, self.assignment_apply)
            self.write(" = ")
            self.traverse(prev_target)
            target = deepcopy(target)
            for node_ in walk(target):
                if hasattr(node_, "ctx"):
                    node_.ctx = Load()
            prev_target = target
        if type_comment := self.get_type_comment(node):
            self.write(type_comment.replace("#", "--", 1))
    def visit_AugAssign(self, node):
        self.fill()
        self.traverse(node.target)
        if self.luau:
            if (op := node.op.__class__.__name__) in self.bitwise_replacements:
                target = deepcopy(node.target)
                for node_ in walk(target):
                    if hasattr(node_, "ctx"):
                        node_.ctx = Load()
                self.write(" = ")
                with self.delimit(f"{self.bitwise_replacements[op]}(", ")"):
                    self.traverse(target)
                    self.write(", ")
                    self.traverse(node.value)
            else:
                self.write(f" {self.binop[op]}= ")
                self.traverse(node.value)
        else:
            target = deepcopy(node.target)
            for node_ in walk(target):
                if hasattr(node_, "ctx"):
                    node_.ctx = Load()
            self.write(" = ")
            self.traverse(target)
            self.write(f" {self.binop[node.op.__class__.__name__]} ")
            self.traverse(node.value)
    def visit_AnnAssign(self, node):
        if self.luau and isinstance(node.target, Name):
            self.fill("local ")
            self.traverse(node.target)
            self.write(": ")
            self.type_traverse(node.annotation)
            if node.value:
                self.write(" = ")
                self.traverse(node.value)
        else:
            if isinstance(node.target, Name):
                self.fill("local ")
                self.traverse(node.target)
                if node.value:
                    self.write(" = ")
                    self.traverse(node.value)
            elif node.value:
                self.fill()
                self.traverse(node.target)
                self.write(" = ")
                self.traverse(node.value)
            else:
                self.fill("-- ")
                self.traverse(node.target)
                self.write(": ")
                self.type_traverse(node.annotation)
                return
            self.write(" --> ")
            self.type_traverse(node.annotation)
    def visit_Pass(self, node):
        pass
    def visit_Continue(self, node):
        if self.luau:
            self.fill("continue")
        else:
            raise NotImplementedError("continue is not yet implemented")
    def visit_Delete(self, node):
        self.fill()
        self.interleave(lambda: self.write(", "), self.traverse, node.targets)
        self.write(" = nil")
    def visit_Assert(self, node):
        self.fill()
        with self.delimit("assert(", ")"):
            self.traverse(node.test)
            if node.msg:
                self.write(", ")
                self.traverse(node.msg)
    def visit_Global(self, node):
        pass
    def visit_Nonlocal(self, node):
        pass
    def visit_Await(self, node):
        raise NotImplementedError("async/await protocol is not implemented")
    def visit_Yield(self, node):
        raise NotImplementedError("yield is not implemented")
    def visit_YieldFrom(self, node):
        raise NotImplementedError("yield from is not implemented")
    def visit_Raise(self, node):
        self.fill()
        with self.delimit("error(", ")"):
            if not node.exc:
                if node.cause:
                    raise ValueError("node can't use cause without an exception")
                return
            if node.cause:
                p = []
                if isinstance(node.exc, Constant):
                    p.append(f"{node.exc.value}")
                else:
                    with self.delimit("tostring(", ")"):
                        self.traverse(node.exc)
                    self.write("..")
                p.append(" from ")
                if isinstance(node.cause, Constant):
                    p.append(f"{node.cause.value}")
                    self._write_str_avoiding_backslashes("".join(p), quote_types=_ALL_QUOTES)
                else:
                    self._write_str_avoiding_backslashes("".join(p), quote_types=_ALL_QUOTES)
                    self.write("..")
                    with self.delimit("tostring(", ")"):
                        self.traverse(node.cause)
            else:
                self.traverse(node.exc)
    def visit_Try(self, node):
        self.fill()
        with self.block_end("do"):
            self.fill("local try, except = pcall(")
            with self.block_end("function()"):
                self.traverse(node.body)
            self.write(")")
            self.fill("if not try ")
            with self.block("then"):
                for ex in node.handlers:
                    self.traverse(ex)
                self._had_handler = False
                self.fill("end")
            if node.orelse:
                self.fill()
                with self.block_end("else"):
                    self.traverse(node.orelse)
            else:
                self.fill("end")
            if node.finalbody:
                self.traverse(node.finalbody)
    def visit_TryStar(self, node):
        raise NotImplementedError("try-except* is not yet implemented")
    def visit_ExceptHandler(self, node):
        if self._had_handler:
            self.fill("else")
        else:
            self._had_handler = True
            self.fill()
        if node.type:
            self.write("if except == ")
            self.traverse(node.type)
            self.write(" then")
        with self.block():
            if node.name:
                self.fill(f"local {node.name} = except")
            self.traverse(node.body)
    def visit_ClassDef(self, node):
        raise NotImplementedError("classes are not yet implemented")
    def visit_FunctionDef(self, node):
        self.maybe_newline()
        if node.decorator_list:
            self.fill(f"local {node.name} = ")
            for deco in node.decorator_list:
                self.set_precedence(_Precedence.ATOM, deco)
                self.traverse(deco)
                self.write("(")
            self.write("function")
        else:
            self.fill(f"local function {node.name}")
        with self.delimit("(", ")"):
            self.traverse(node.args)
        if node.returns:
            self.write(": " if self.luau else " --> ")
            self.type_traverse(node.returns)
        with self.block_end((self.get_type_comment(node) or "").replace("#", "--", 1)):
            self._write_docstring_and_traverse_body(node)
        if node.decorator_list:
            self.write(")" * len(node.decorator_list))
    def visit_AsyncFunctionDef(self, node):
        raise NotImplementedError("async/await protocol is not implemented")
    def for_target_apply(self, node):
        if isinstance(node, Starred):
            if isinstance(node.value, (List, Tuple)):
                self.interleave(lambda: self.write(", "), lambda x: self.traverse(x, self.for_target_apply), node.value.elts)
                return 1
            else:
                raise NotImplementedError("starred expression in for loop targets is not supported")
        elif isinstance(node, (List, Tuple)):
            self.interleave(lambda: self.write(", "), lambda x: self.traverse(x, self.for_target_apply), node.elts)
            return 1
        elif not isinstance(node, Name):
            raise NotImplementedError("non-name targets in for loop header is not supported")
    def visit_For(self, node):
        self.fill("for ")
        self.set_precedence(_Precedence.TEST, node.target)
        self.traverse(node.target, self.for_target_apply)
        self.write(" in ")
        self.traverse(node.iter)
        self.write(" do")
        with self.block_end((self.get_type_comment(node) or "").replace("#", "--", 1)):
            self.traverse(node.body)
        if node.orelse:
            raise NotImplementedError("else clause after for loop is not supported")
    def visit_AsyncFor(self, node):
        raise NotImplementedError("async/await protocol is not implemented")
    def visit_If(self, node):
        self.fill("if ")
        self.traverse(node.test)
        with self.block(" then"):
            self.traverse(node.body)
        while node.orelse and len(node.orelse) == 1 and isinstance(node.orelse[0], If):
            node = node.orelse[0]
            self.fill("elseif ")
            self.traverse(node.test)
            with self.block(" then"):
                self.traverse(node.body)
        if node.orelse:
            self.fill()
            with self.block_end("else"):
                self.traverse(node.orelse)
        else:
            self.fill("end")
    def visit_While(self, node):
        self.fill("while ")
        self.traverse(node.test)
        with self.block_end(" do"):
            self.traverse(node.body)
        if node.orelse:
            raise NotImplementedError("else clause after while loop is not supported")
    def visit_With(self, node):
        raise NotImplementedError("with/async with is not supported")
    def visit_AsyncWith(self, node):
        raise NotImplementedError("with/async with is not supported")
    def _write_str_avoiding_backslashes(self, string, *, quote_types=_ALL_QUOTES):
        string, quote_types = self._str_literal_helper(string, quote_types=quote_types)
        quote_type = quote_types[0]
        if quote_type == "[[":
            self.write(f"[[{string}]]")
        else:
            self.write(f"{quote_type}{string}{quote_type}")
    def visit_JoinedStr(self, node):
        raise NotImplementedError("f-strings are not yet supported")
    def visit_FormattedValue(self, node):
        raise NotImplementedError("f-strings are not yet supported")
    def visit_Name(self, node):
        self.write(node.id)
        """
        elif isinstance(node.ctx, Load):
            if isinstance(node, Starred):
                if isinstance(node.value, (List, Tuple, Set)):
                    self.traverse(node.value.elts)
                elif isinstance(node.value, Constant) and isinstance(node.value.value, str):
                    self.interleave_write(lambda self.write(", "), ascii, node.value.value)
                elif not isinstance(node.value, Call):
                    with self.delimit("table.unpack(", ")"):
                        self.traverse(node.value)
                else:
                    self.traverse(node.value)
            else:
                self.traverse(node.value)
        """
    def _write_docstring(self, node):
        self.fill("--")
        self._write_str_avoiding_backslashes(node.value, quote_types=_ALL_QUOTES)
    def _write_constant(self, value):
        if isinstance(value, (float, complex)):
            # Substitute overflowing decimal literal for AST infinities,
            # and inf - inf for NaNs.
            self.write(
                repr(value)
                .replace("inf", _INFSTR)
                .replace("nan", f"({_INFSTR}-{_INFSTR})")
            )
        elif isinstance(value, str):
            self._write_str_avoiding_backslashes(value)
        elif isinstance(value, bool):
            self.write("ftarlusee"[value::2])
        else:
            self.write(repr(value))
    def visit_Constant(self, node):
        value = node.value
        if isinstance(value, tuple):
            with self.delimit("{", "}"):
                self.items_view(self._write_constant, value)
        elif value is ...:
            self._write_str_avoiding_backslashes("...")
        else:
            self._write_constant(node.value)
    def visit_List(self, node):
        with self.delimit("{", "}"):
            self.interleave(lambda: self.write(", "), self.traverse, node.elts)
    def visit_ListComp(self, node):
        raise NotImplementedError("comprehensions are not supported")
    def visit_GeneratorExp(self, node):
        raise NotImplementedError("comprehensions are not supported")
    def visit_SetComp(self, node):
        raise NotImplementedError("comprehensions are not supported")
    def visit_DictComp(self, node):
        raise NotImplementedError("comprehensions are not supported")
    def visit_comprehension(self, node):
        raise NotImplementedError("comprehensions are not supported")
    def visit_IfExp(self, node):
        if self.luau:
            with self.require_parens(_Precedence.TEST, node):
                self.set_precedence(_Precedence.OR, node.body, node.test)
                self.write("if ")
                self.traverse(node.test)
                self.write(" then ")
                self.traverse(node.body)
                while isinstance(node.orelse, IfExp):
                    node = node.orelse
                    self.set_precedence(_Precedence.OR, node.body, node.test)
                    self.write(" elseif ")
                    self.traverse(node.test)
                    self.write(" then ")
                    self.traverse(node.body)
                self.write(" else ")
                self.set_precedence(_Precedence.TEST, node.orelse)
                self.traverse(node.orelse)
        else:
            with self.delimit("(", ")[1]"):
                self.set_precedence(_Precedence.CMP, node.test)
                self.traverse(node.test)
                self.write(" and ")
                self.set_precedence(_Precedence.TEST, node.body, node.orelse)
                with self.delimit("{", "}"):
                    self.traverse(node.body)
                self.write(" or ")
                with self.delimit("{", "}"):
                    self.traverse(node.orelse)
    visit_Set = visit_List
    def visit_Dict(self, node):
        def write_key_value_pair(k, v):
            if isinstance(k, Constant) and isinstance(k.value, str):
                self.write(f"{k.value} = ")
            else:
                with self.delimit("[", "]"):
                    self.traverse(k)
                self.write(" = ")
            self.traverse(v)
        def write_item(item):
            k, v = item
            if k is None:
                with self.delimit("table.unpack(", ")"):
                    self.traverse(v)
            else:
                write_key_value_pair(k, v)
        with self.delimit("{", "}"):
            self.interleave(
                lambda: self.write(", "), write_item, zip(node.keys, node.values)
            )
    visit_Tuple = visit_List
    unop = {"Invert": "~", "Not": "not ", "UAdd": "+", "USub": "-"}
    def visit_UnaryOp(self, node):
        operator = self.unop[node.op.__class__.__name__]
        if self.luau and operator == "~":
            with self.delimit("bit32.bnot(", ")"):
                self.set_precedence(_Precedence.TEST, node.operand)
                self.traverse(node.operand)
        else:
            with self.require_parens(_Precedence.UNARY, node):
                self.write(operator)
                self.set_precedence(_Precedence.UNARY, node.operand)
                self.traverse(node.operand)
    binop = {
        "Add": "+",
        "Sub": "-",
        "Mult": "*",
        "Div": "/",
        "Mod": "%",
        "LShift": "<<",
        "RShift": ">>",
        "BitOr": "|",
        "BitXor": "~",
        "BitAnd": "&",
        "FloorDiv": "//",
        "Pow": "^",
    }
    bitwise_replacements = {
        "BitAnd": "bit32.band",
        "BitOr": "bit32.bor",
        "BitXor": "bit32.bxor",
        "LShift": "bit32.lshift",
        "RShift": "bit32.rshift",
    }
    binop_precedence = {
        "+": _Precedence.ARITH,
        "-": _Precedence.ARITH,
        "*": _Precedence.TERM,
        "/": _Precedence.TERM,
        "%": _Precedence.TERM,
        "<<": _Precedence.SHIFT,
        ">>": _Precedence.SHIFT,
        "|": _Precedence.BOR,
        "~": _Precedence.BXOR,
        "&": _Precedence.BAND,
        "//": _Precedence.TERM,
        "^": _Precedence.POWER,
    }
    binop_rassoc = frozenset(("^", ".."))
    def visit_BinOp(self, node):
        operator_name = node.op.__class__.__name__
        if operator_name == "MatMul":
            print("Lua transpiler warning: @ (left-associative matrix "
                  "multiplication) is being used for .. (right-associative "
                  "string concatenation)", flush=True)
            operator = ".."
            operator_precedence = _Precedence.CONCAT
        else:
            if self.luau and operator_name in self.bitwise_replacements:
                with self.delimit(f"{self.bitwise_replacements[operator_name]}(", ")"):
                    self.traverse(node.left)
                    self.write(", ")
                    self.traverse(node.right)
                return
            else:
                operator = self.binop[operator_name]
                operator_precedence = self.binop_precedence[operator]
        with self.require_parens(operator_precedence, node):
            if operator in self.binop_rassoc:
                left_precedence = operator_precedence.next()
                right_precedence = operator_precedence
            else:
                left_precedence = operator_precedence
                right_precedence = operator_precedence.next()
            self.set_precedence(left_precedence, node.left)
            self.traverse(node.left)
            self.write(f" {operator} ")
            self.set_precedence(right_precedence, node.right)
            self.traverse(node.right)
    cmpops = {
        "Eq": "==",
        "NotEq": "~=",
        "Lt": "<",
        "LtE": "<=",
        "Gt": ">",
        "GtE": ">=",
        "Is": "==",
        "IsNot": "~=",
    }
    def visit_Compare(self, node):
        with self.require_parens(_Precedence.CMP, node):
            self.set_precedence(_Precedence.CMP.next(), node.left, *node.comparators)
            self.traverse(node.left)
            checked = {*()}
            for o, e in zip(node.ops, node.comparators):
                if (n := o.__class__.__name__) in {"In", "NotIn"}:
                    raise NotImplementedError("membership operator not yet implemented")
                cmpop = self.cmpops[o.__class__.__name__]
                if n not in checked and n in {"Is", "IsNot"}:
                    print(f"warning: using {cmpop} for {n} node")
                    checked.add(n)
                self.write(f" {cmpop} ")
                self.traverse(e)

c = parse("""
from . import a
from g import a, b, c
from l import *
from math import *
from math import sqrt, cbrt
import math, os, hi
a, *[b, c.d], d = 5
a[b] &= 7
a[b] += 3
a: s = 3
a[b]: g = 7
a[b]: g
del g
del a, b[c], d
assert ahh
assert 7 == 2, "what"
raise "hm'm" from 'ah"h'
raise thing(b) from "lol"
raise "lol" from thing(b)
try:
    raise thing(a, b, c)
except thing:
    print("oh")
except otherthing as b:
    print(b)
except:
    print("AAAAAAAA")
else:
    print("ok good")
finally:
    print("hmm")
@ah + 2
@hm[3]
def v(): return 7 + 7
for a, b, c in {1, 2, 3}: print(a, b, c)
for x in {1, 2, 7}: print(x)
if 5: print('yes')
elif 7: print('ye')
else: print('y')
while True: pass
5 if 10 else 7 if 3 else 2
{1: 3, 'b': 'ha', True: False}
+((a >> b) % c)
""")
print(_LuaTranspiler(luau=False).visit(c))
