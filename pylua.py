from ast import *
from ast import _SINGLE_QUOTES, _Unparser
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
    def __init__(self, *, luau=False):
        self._source = []
        self._precedences = {}
        self._type_ignores = {}
        self._indent = 0
        self._had_handler = False
        self._is_call = False
        self._is_type_return = False
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
    def copy_change_node_ctx(self, node, ctx):
        node_copy = deepcopy(node)
        for node_ in walk(node_copy):
            if hasattr(node_, "ctx"):
                node_.ctx = ctx()
        return node_copy
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
            old_traverse, self.traverse = self.traverse, self.type_traverse
            res = getattr(self, method, self.type_generic_visit)(node)
            self.traverse = old_traverse
            return res
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
    def import_process(self, subnames_alias):
        subnames, alias_ = subnames_alias
        if self.luau:
            res = f"require(script.Parent.{'.'.join(subnames)})"
        else:
            res = f'dofile("./{"/".join(subnames)}.lua")'
        if len(subnames) > 1:
            subnames = subnames[1:]
            while subnames and (subname := subnames.pop()):
                res = f"{{{subname} = {res}}}"
        elif subnames[0] in ("math", "os"):
            if alias_ == subnames[0]:
                res = ""
            else:
                res = subnames[0]
        return res
    def visit_Import(self, node):
        self.fill("local ")
        self.interleave_write(lambda: self.write(", "), lambda x: x.asname if (x.asname or x.name) != x.name else "_"*(x.name not in ("math", "os")), node.names)
        self.write(" = ")
        self.interleave_write(lambda: self.write(", "), self.import_process, [(name.name.split('.'), name.asname or name.name) for name in node.names])
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
        if not node.level and node.module in ("math", "os") and (len(node.names) != 1 or node.names[0].name != "*"):
            for x in node.names:
                self.fill(f"local {x.asname or x.name} = {node.module}.{x.name}")
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
                if len(node.names) == 1 and node.names[0].name == "*":
                    if not node.level and node.module in ("math", "os"):
                        res = node.module
                    self.fill(f"for k, v in {res} ")
                    with self.block_end("do"):
                        self.fill("env[k] = v")
                else:
                    self.fill(f"local mod = {res}")
                    for x in node.names:
                        self.fill(f"env.{x.asname or x.name} = mod.{x.name}")
            else:
                if self.luau:
                    if len(node.names) == 1 and node.names[0].name == "*":
                        self.fill(f"for k, v in {res}:GetChildren() ")
                        with self.block_end("do"):
                            self.fill("if v:IsA('ModuleScript') ")
                            with self.block_end("then"):
                                self.fill(f"env[k] = require(v)")
                    else:
                        self.fill(f"local inst = {res}")
                        for x in node.names:
                            self.fill(f"env.{x.asname or x.name} = require(inst.{x.name})")
                else:
                    if len(node.names) == 1 and node.names[0].name == "*":
                        raise NotImplementedError("from ... import * not yet supported")
                    else:
                        for x in node.names:
                            self.fill(f'env.{x.asname or x.name} = dofile("{res}{x.name}.lua")')
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
            prev_target = self.copy_change_node_ctx(target, Load)
        if type_comment := self.get_type_comment(node):
            self.write(type_comment.replace("#", "--", 1))
    def visit_AugAssign(self, node):
        self.fill()
        self.traverse(node.target)
        if self.luau:
            if (op := node.op.__class__.__name__) in self.bitwise_replacements:
                target = self.copy_change_node_ctx(node.target, Load)
                self.write(" = ")
                with self.delimit(f"{self.bitwise_replacements[op]}(", ")"):
                    self.traverse(target)
                    self.write(", ")
                    self.traverse(node.value)
            else:
                self.write(f" {self.binop[op]}= ")
                self.traverse(node.value)
        else:
            target = self.copy_change_node_ctx(node.target, Load)
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
            self.write(" -- type: ")
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
            self.write(": " if self.luau else " -- type: ")
            self._is_type_return = True
            self.type_traverse(node.returns)
            self._is_type_return = False
        with self.block_end((self.get_type_comment(node) or "").replace("#", "--", 1)):
            if docstring := self.get_raw_docstring(node):
                self._write_docstring(docstring)
            if (params := node.args).defaults or params.kw_defaults:
                args = params.posonlyargs + params.args
                for arg, default in zip(
                        args + params.kwonlyargs,
                        [None] * (len(args) - len(params.defaults)) + params.defaults + params.kw_defaults
                ):
                    if default:
                        self.fill(f"if {arg.arg} == nil ")
                        with self.block_end("then"):
                            self.fill(f"{arg.arg} = ")
                            self.traverse(default)
            self.traverse(node.body)
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
        first = True
        for value in node.values:
            if first:
                first = False
            else:
                self.write(" .. ")
            if isinstance(value, Constant) and isinstance(value.value, str):
                self._write_str_avoiding_backslashes(value.value)
            elif isinstance(value, FormattedValue):
                self.visit_FormattedValue(value)
            else:
                raise ValueError(f"Unexpected node inside JoinedStr, {value!r}")
    def visit_FormattedValue(self, node):
        if node.format_spec:
            raise NotImplementedError("format specifier in f-strings is not yet implemented")
        self.set_precedence(_Precedence.CONCAT, node.value)
        self.traverse(node.value)
    _luau_type = {
        "str": "string",
        "int": "number",
        "float": "number",
        "bool": "boolean",
        "string": "string_",
        "number": "number_",
        "boolean": "boolean_",
    }
    def visit_Name(self, node):
        self.write(node.id)
    def type_visit_Name(self, node):
        self.write(self._luau_type.get(node.id, node.id))
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
            self.write("...")
        elif value is None:
            self.write("nil")
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
            if isinstance(k, Constant) and isinstance(k.value, str) and k.value.isalpha():
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
    def type_visit_Dict(self, node):
        old_traverse, self.traverse = self.traverse, self.type_traverse
        _Unparser.visit_Dict(self, node)
        self.traverse = old_traverse
    def type_visit_Tuple(self, node):
        if self._is_type_return:
            self._is_type_return = False
            with self.delimit("(", ")"):
                self.interleave(lambda: self.write(", "), self.type_traverse, node.elts)
        else:
            old_traverse, self.traverse = self.traverse, self.type_traverse
            self.visit_List(node)
            self.traverse = old_traverse
    unop = {"Invert": "~", "Not": "not ", "UAdd": "+", "USub": "-"}
    def visit_UnaryOp(self, node):
        operator = self.unop[node.op.__class__.__name__]
        if operator == "+":
            self.traverse(node.operand)
            return
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
        if operator_name == "MatMult":
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
    def type_visit_BinOp(self, node):
        operator_name = node.op.__class__.__name__
        if operator_name not in ("BitOr", "BitAnd"):
            raise NotImplementedError("type operations other than | or & are not supported")
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
            self.type_traverse(node.left)
            self.write(f" {operator} ")
            self.set_precedence(right_precedence, node.right)
            self.type_traverse(node.right)
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
            prev = node.left
            checked = {*()}
            def do_compare(item):
                nonlocal prev
                o, e = item
                if (n := o.__class__.__name__) in {"In", "NotIn"}:
                    raise NotImplementedError("membership operator not yet implemented")
                cmpop = self.cmpops[o.__class__.__name__]
                if n not in checked and n in {"Is", "IsNot"}:
                    print(f"warning: using {cmpop} for {n} node")
                    checked.add(n)
                self.traverse(prev)
                self.write(f" {cmpop} ")
                self.traverse(e)
                prev = e
            self.interleave(lambda: self.write(" and "), do_compare, zip(node.ops, node.comparators))
    boolops = {"And": "and", "Or": "or"}
    boolop_precedence = {"and": _Precedence.AND, "or": _Precedence.OR}
    def visit_BoolOp(self, node):
        operator = self.boolops[node.op.__class__.__name__]
        operator_precedence = self.boolop_precedence[operator]
        with self.require_parens(operator_precedence, node):
            s = f" {operator} "
            self.interleave(lambda: self.write(s), self.traverse, node.values)
    def visit_Attribute(self, node):
        self.set_precedence(_Precedence.ATOM, node.value)
        with self.delimit_if("(", ")", isinstance(node.value, (Constant, List, Tuple, Set, Dict))):
            self.traverse(node.value)
        if self._is_call:
            self._is_call = False
            self.write(":")
        else:
            self.write(".")
        self.write(node.attr)
    def visit_Call(self, node):
        self.set_precedence(_Precedence.ATOM, node.func)
        if isinstance(node.func, Attribute):
            self._is_call = True
        with self.delimit_if("(", ")", isinstance(node.func, (Constant, List, Tuple, Set, Dict))):
            self.traverse(node.func)
        with self.delimit("(", ")"):
            comma = False
            for e in node.args:
                if comma:
                    self.write(", ")
                else:
                    comma = True
                self.traverse(e)
            if node.keywords:
                print("warning: converting keywords to normal arguments")
            for e in node.keywords:
                if comma:
                    self.write(", ")
                else:
                    comma = True
                self.traverse(e)
    def visit_Subscript(self, node):
        self.set_precedence(_Precedence.ATOM, node.value)
        start = len(self._source)
        is_parenthesized = False
        with self.delimit_if("(", ")", isinstance(node.value, (Constant, List, Tuple, Set, Dict))):
            self.traverse(node.value)
        if isinstance(node.slice, Tuple):
            raise NotImplementedError("tuple subscripts are not supported")
        if isinstance(node.ctx, (Store, Del)):
            if isinstance(node.slice, Constant) and isinstance(node.slice.value, str) and node.slice.value.isalpha():
                self.write(f".{node.slice.value}")
            else:
                with self.delimit("[", "]"):
                    self.traverse(node.slice)
            return
        if not isinstance(node.slice, Slice):
            end = len(self._source)
            if isinstance(node.slice, Constant) and isinstance(node.slice.value, str) and node.slice.value.isalpha():
                self.write(f".{node.slice.value}")
            else:
                with self.delimit("[", "]"):
                    self.traverse(node.slice)
            self.write(" or ")
            self._source += self._source[start:end]
            self._source.insert(start, "(")
            is_parenthesized = True
        with self.delimit(":sub(", ")"):
            self.traverse(node.slice)
            if not isinstance(node.slice, Slice):
                self.write(", ")
                self.traverse(node.slice)
        if is_parenthesized:
            self.write(")")
    def type_visit_Subscript(self, node):
        if isinstance(node.value, Name):
            if node.value.id == "Optional":
                val = node.slice
                if isinstance(val, Tuple) and len(val.elts) == 1 and isinstance(val.elts[0], Starred):
                    val = val.elts[0]
                self.set_precedence(_Precedence.ATOM, val)
                self.type_traverse(val)
                self.write("?")
                return
            elif node.value.id in ("List", "list"):
                with self.delimit("{", "}"):
                    if isinstance(node.slice, (Tuple, Slice)):
                        raise NotImplementedError("list type subscript with tuple/slice is not supported")
                    else:
                        self.interleave(lambda: self.write(", "), self.type_traverse, node.slice)
                return
            elif node.value.id in ("Tuple", "tuple") and self._is_type_return:
                self._is_type_return = False
                with self.delimit("(", ")"):
                    if isinstance(node.slice, Tuple):
                        self.interleave(lambda: self.write(", "), self.type_traverse, node.slice.elts)
                    elif isinstance(node.slice, Slice):
                        raise NotImplementedError("tuple type subscript with slice is not supported")
                    else:
                        self.interleave(lambda: self.write(", "), self.type_traverse, node.slice)
                return
        else:
            if isinstance(node.slice, (Tuple, Slice)):
                raise NotImplementedError("tuple and slice subscripts are not supported")
            with self.delimit("[", "]"):
                self.type_traverse(node.slice)
    def _starred_handler(self, val):
        if isinstance(val, (List, Tuple, Set)):
            self.traverse(val.elts)
        elif isinstance(val, Constant) and isinstance(val.value, str):
            self.interleave_write(lambda: self.write(", "), ascii, val.value)
        elif isinstance(val, Call):
            print("warning: starred expression is ignored")
            self.traverse(val)
        else:
            with self.delimit("table.unpack(", ")"):
                self.traverse(val)
    def visit_Starred(self, node):
        self._starred_handler(node.value)
    def type_visit_Starred(self, node):
        if isinstance(node.value, Name) and node.value.id in self._luau_type:
            self.write(f"...{node.value.id}")
        else:
            self.type_traverse(node.value)
            self.write("...")
    def visit_Ellipsis(self, node):
        self.write("...")
    def visit_Slice(self, node):
        if node.lower:
            self.traverse(node.lower)
        else:
            self.write("1")
        if node.upper:
            self.write(", ")
            self.traverse(node.upper)
        elif node.step:
            self.write(", -1")
        if node.step:
            self.write(", ")
            self.traverse(node.step)
    def visit_Match(self, node):
        raise NotImplementedError("match cases are not supported")
    def visit_arg(self, node):
        self.write(node.arg)
        if node.annotation:
            if self.luau:
                self.write(": ")
                self.type_traverse(node.annotation)
            else:
                with self.delimit(" --[[type: ", "]]"):
                    self.type_traverse(node.annotation)
    def visit_arguments(self, node):
        first = True
        for element in node.posonlyargs + node.args + node.kwonlyargs:
            if first:
                first = False
            else:
                self.write(", ")
            self.traverse(element)
        if node.vararg:
            if node.kwonlyargs or node.kwarg:
                raise NotImplementedError("arguments after vararg are not supported")
            if first:
                first = False
            else:
                self.write(", ")
            print(f"warning: vararg {node.vararg.arg} will be converted into unnamed ellipsis")
            self.write("...")
            if node.vararg.annotation:
                if self.luau:
                    self.write(": ")
                    self.type_traverse(node.vararg.annotation)
                else:
                    with self.delimit(" --[[type: ", "]]"):
                        self.type_traverse(node.vararg.annotation)
    def visit_keyword(self, node):
        print("warning: kwargs will turn into normal vargs and keywords into normal args")
        if node.arg is None:
            self._starred_handler(node.value)
        else:
            with self.delimit("--[[", "=]] "):
                self.write(node.arg)
            self.traverse(node.value)
    def visit_Lambda(self, node):
        with self.delimit("(function(", ")"):
            self.traverse(node.args)
        with self.block_end():
            self.fill("return ")
            self.set_precedence(_Precedence.TEST, node.body)
            self.traverse(node.body)
        self.write(")")
    def visit_alias(self, node):
        raise NotImplementedError("alias should not be handled outside of Import/ImportFrom")
    def visit_withitem(self, node):
        raise NotImplementedError("with/async with is not supported")
    def visit_match_case(self, node):
        raise NotImplementedError("match cases are not supported")
    def visit_MatchSequence(self, node):
        raise NotImplementedError("match-case patterns are not supported")
    visit_MatchOr = visit_MatchAs = visit_MatchClass = visit_MatchMapping = visit_MatchStar = visit_MatchSequence

def lua_transpiler(str_code, *, luau=False, **parser_kwargs):
    return _LuaTranspiler(luau=luau).visit(parse(str_code, **parser_kwargs))

if __name__ == "__main__":
    c = """
from . import a as d
from g import a as b, b as c, c as d
from l import *
from math import *
from math import sqrt, cbrt
import math, math as l, os, hi, os as gl
a, *[b, c.d], d = 5
a[b] &= 7
a[b] += 3
a: s = 3
a[b]: g = 7
a[b]: g
del g
del a, b['c'], d
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
def v() -> str: return 7 + 7
for a, b, c in {1, 2, 3}: print(a, b, c)
for x in {1, 2, 7}: print(x)
if 5: print('yes')
elif 7: print('ye')
else: print('y')
while True: pass
k = 5 if 10 else 7 if 3 else 2
k = {1: 3, 'b': 'ha', True: False}
k = +((a >> b) % c)
k = a is b is c == d != e < f > g >= h <= i
k = a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b and a + b
k = a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b or a + b
k = a.b
a.b(*a, *b, *[c, d], *'efg', *m, h=i, j=k, **l)
k = a[b]
k = a[b:c:d]
k = a[b::d]
k = a[:c:d]
k = a[b:c]
def g(l: T, g: T = 10, *b: T) -> Tuple[*T] | Optional[T]: pass
k = a + (lambda x: x + a)(2) + 3
k = f'haha... {o @ hell @ no} ...oh'
"""
    print(lua_transpiler(c, luau=True))

