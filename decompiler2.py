from __future__ import annotations
import struct, types, typing, dis, a_c, ast, enum
# struct
_struct_unpack_iterator: type = type(struct.iter_unpack('B',b'0'))
struct_iter_unpack: Callable[[str, bytes], _struct_unpack_iterator] = struct.iter_unpack
# types
code: type = types.CodeType
# typing
Callable: typing._CallableType = typing.Callable
# dis
locals().update(dis._all_opmap)
opname: list[str] = dis._all_opname
locals().update({f'CACHE_ENTRIES_{opname[i]}': v for i, v in enumerate(dis._inline_cache_entries) if v})
bin_ops: list[str] = [v[1] for v in dis._nb_ops]
cmp_ops: tuple[str] = dis.cmp_op
# a_c
parse_exception_table: Callable[[code], list[tuple[int, int, int, int, bool]]] = a_c.parse_exception_table
# ast
## enum
_Precedence: enum.EnumType = ast._Precedence
NAMED_EXPR: _Precedence = _Precedence.NAMED_EXPR
TUPLE: _Precedence = _Precedence.TUPLE
YIELD: _Precedence = _Precedence.YIELD
TEST: _Precedence = _Precedence.TEST
boolop_prec: dict[str, _Precedence] = {'or': (OR := _Precedence.OR), 'and': (AND := _Precedence.AND)}
FACTOR: _Precedence = _Precedence.FACTOR
uop_prec: dict[str, _Precedence] = {'not': (NOT := _Precedence.NOT)} | {x: FACTOR for x in ['+', '-', '~']}
CMP: _Precedence = _Precedence.CMP
cmpop_prec: dict[str, _Precedence] = {x: CMP for x in ['<', '>', '==', '>=', '<=', '!=',
                                                       'in', 'not in', 'is', 'is not']}
binop_prec: dict[str, _Precedence] = {'|': (BOR := _Precedence.BOR), '^': (BXOR := _Precedence.BXOR),
                                      '&': (BAND := _Precedence.BAND), '<<': (SHIFT := _Precedence.SHIFT),
                                      '>>': (SHIFT := _Precedence.SHIFT), '+': (ARITH := _Precedence.ARITH),
                                      '-': (ARITH := _Precedence.ARITH), '*': (TERM := _Precedence.TERM),
                                      '@': (TERM := _Precedence.TERM), '/': (TERM := _Precedence.TERM),
                                      '%': (TERM := _Precedence.TERM), '//': (TERM := _Precedence.TERM),
                                      '**': (POWER := _Precedence.POWER)}
AWAIT: _Precedence = _Precedence.AWAIT
ATOM: _Precedence = _Precedence.ATOM
del struct, types, typing, dis, a_c, ast, enum

class UOp:
    def __init__(self, op: str, value: object):
        self.op: str = op
        self.value: object = value
        self.precedence: _Precedence = uop_prec[op]
    def __repr__(self) -> str:
        return f"UOp({self.op!r}, {self.value!r})"
    def __str__(self) -> str:
        if self.value.precedence < self.precedence:
            value_s: str = f"({self.value!s})"
        else:
            value_s: str = f"{self.value!s}"
        return f"{self.op}{value_s}"

class Trailer:
    def __init__(self, lhs: object, syms: tuple[str, str], rhs: object):
        self.lhs: object = lhs
        self.syms: tuple[str, str] = syms
        self.rhs: object = rhs
        self.precedence: _Precedence = ATOM
    def __repr__(self) -> str:
        return f"Trailer({self.lhs!r}, {self.syms!r}, {self.rhs!r})"
    def __str__(self) -> str:
        if self.lhs.precedence < self.precedence:
            lhs_s: str = f"({self.lhs!s})"
        else:
            lhs_s: str = f"{self.lhs!s}"
        return f"{lhs_s}{self.syms[0]}{self.rhs!s}{self.syms[1]}"

class Const:
    def __init__(self, value: object):
        self.value: object = value
        self.precedence: _Precedence = ATOM
    def __repr__(self) -> str:
        return f"Const({self.value!r})"
    def __str__(self) -> str:
        return f"{self.value!r}"

class Name:
    def __init__(self, value: object):
        self.value: object = value
        self.precedence: _Precedence = ATOM
    def __repr__(self) -> str:
        return f"Name({self.value!r})"
    def __str__(self) -> str:
        return f"{self.value!s}"

class Attr:
    def __init__(self, value: object, attr: str):
        self.value: object = value
        self.attr: str = attr
        self.precedence: _Precedence = ATOM
    def __repr__(self) -> str:
        return f"Attr({self.value!r}, {self.attr!r})"
    def __str__(self) -> str:
        if (value := self.value).precedence < self.precedence:
            value_s: str = f"({value!s})"
        elif type(value) is Const and type(value.value) is int:
            value_s: str = f"{value.value} "
        else:
            value_s: str = f"{value!s}"
        return f"{value_s}.{self.attr}"

class MatchSeq:
    def __init__(self, seq: list[object], value: str | None = "", indent_level: int = 1):
        self.seq: list[object] = seq
        self.value: str = value
        self.indent_level: int = indent_level
    def __repr__(self) -> str:
        return f"MatchSeq({self.seq!r}, {self.value!r}, indent_level={self.indent_level})"
    def __str__(self) -> str:
        if self.value:
            return f"{'    '*self.indent_level}case [{', '.join(map(str, self.seq))}]:\n{self.value}"
        return f"[{', '.join(map(str, self.seq))}]"

newline = '\n'
indent1 = '    '
class MatchStmt:
    def __init__(self, value: object, cases: list[MatchSeq] = None, indent_level: int = 0):
        self.value: object = value
        if cases is None:
            self.cases: list[MatchSeq] = []
        else:
            self.cases: list[MatchSeq] = cases
        self.indent_level: int = indent_level
    def __repr__(self) -> str:
        return f"MatchStmt({self.value!r}, {self.cases!r}, indent_level={self.indent_level})"
    def __str__(self) -> str:
        return f"{'    '*self.indent_level}match {self.value!s}:\n{newline.join(map(lambda x:f'{indent1*(self.indent_level+1)}', self.cases))}"

class TryStmt:
    def __init__(self, value: str, handlers: list[ExceptStmt] = None, indent_level: int = 0):
        self.value: str = value
        if handlers is None:
            self.handlers: list[ExceptStmt] = []
        else:
            self.handlers: list[ExceptStmt] = cases
        self.indent_level: int = indent_level
    def __repr__(self) -> str:
        return f"TryStmt({self.value!r}, {self.handlers!r}, indent_level={self.indent_level})"
    def __str__(self) -> str:
        return f"{'    '*self.indent_level}try:\n{self.value}{newline.join(map(lambda x:f'{indent1*(self.indent_level+1)}', self.handlers))}"

class ExceptStmt:
    def __init__(self, catching: object, value: str, target: Name | None = None, indent_level: int = 0):
        self.catching: object = catching
        self.value: str = value
        self.target: Name | None = target
        self.indent_level: int = indent_level
    def __repr__(self) -> str:
        return f"ExceptStmt({self.catching!r}, {self.value!r}, {self.target!r}, indent_level={self.indent_level})"
    def __str__(self) -> str:
        return f"{'    '*self.indent_level}except {self.catching!s}{f' as {self.target!s}' if self.target else ''}:\n{self.value}"

class BinOp:
    def __init__(self, lhs: object, op: str, rhs: object):
        self.lhs: object = lhs
        self.op: str = op
        self.rhs: object = rhs
        self.precedence: _Precedence = binop_prec[op]
    def __repr__(self) -> str:
        return f"BinOp({self.lhs!r}, {self.op!r}, {self.rhs!r})"
    def __str__(self) -> str:
        if self.lhs.precedence < self.precedence:
            lhs_s: str = f"({self.lhs!s})"
        else:
            lhs_s: str = f"{self.lhs!s}"
        if self.rhs.precedence < self.precedence:
            rhs_s: str = f"({self.rhs!s})"
        else:
            rhs_s: str = f"{self.rhs!s}"
        return f"{lhs_s} {self.op} {rhs_s}"

class AugAssign:
    def __init__(self, lhs: object, op: str, rhs: object):
        self.lhs: object = lhs
        self.op: str = op
        self.rhs: object = rhs
    def __repr__(self) -> str:
        return f"AugAssign({self.lhs!r}, {self.op!r}, {self.rhs!r})"
    def __str__(self) -> str:
        return f"{self.lhs!s} {self.op}= {self.rhs!s}"

class Call:
    def __init__(self, callb: object, args: list[object], positional_args: int,
                 total_args: int, kwnames: tuple[str] | None):
        self.callb: object = callb
        self.args: list[object] = args
        self.positional_args: int = positional_args
        self.kwnames: tuple[str] | None = kwnames
        self.precedence: _Precedence = ATOM
    def __repr__(self):
        return f"Call({self.callb!r}, {self.args!r}, {self.positional_args!r}, {self.kwnames!r})"
    def __str__(self):
        if self.kwnames:
            kwarg_s: str = f", {', '.join([f'{kw}={arg!s}' for kw, arg in zip(self.kwnames, self.args[self.positional_args:])])}"
        else:
            kwarg_s: str = ""
        if self.callb.precedence < self.precedence:
            rhs_s: str = f"({self.callb!s})"
        else:
            callb_s: str = f"{self.callb!s}"
        return f"{callb_s}({', '.join(map(str, self.args[:self.positional_args]))}{kwarg_s})"

class CmpOp:
    def __init__(self, lhs: object, op: str, rhs: object):
        self.lhs: object = lhs
        self.op: str = op
        self.rhs: object = rhs
        self.precedence: _Precedence = cmpop_prec[op]
    def __repr__(self) -> str:
        return f"CmpOp({self.lhs!r}, {self.op!r}, {self.rhs!r})"
    def __str__(self) -> str:
        if self.lhs.precedence < self.precedence:
            lhs_s: str = f"({self.lhs!s})"
        else:
            lhs_s: str = f"{self.lhs!s}"
        if self.rhs.precedence < self.precedence:
            rhs_s: str = f"({self.rhs!s})"
        else:
            rhs_s: str = f"{self.rhs!s}"
        return f"{lhs_s} {self.op} {rhs_s}"

class ClassDef:
    def __init__(self, name: str, args: list[object], kwnames: tuple[str], code_str: str, indent_level: int = 0):
        self.name: str = name
        self.args: list[object] = args
        self.kwnames = tuple[str] = kwnames
        self.positional_args: int = len(args) - len(kwnames)
        self.code_str: str = code_str
        self.indent_level: int = indent_level
    def __repr__(self) -> str:
        return f"ClassDef({self.name!r}, {self.args!r}, {self.kwnames!r}, {self.code_str!r}, indent_level={self.indent_level!r})"
    def __str__(self) -> str:
        if kwnames:
            args_s: str = f"({', '.join(self.args[:self.positional_args] + [f'{kw}={arg!s}' for kw, arg in zip(kwnames, self.args[self.positional_args:])])})"
        else:
            args_s: str = f"({', '.join(self.args)})" if self.args else ""
        return f"{'    '}class {self.name}{args_s}:\n{self.value}"

class TupleExpr:
    def __init__(self, els: list[object]):
        self.els: list[object] = els
        self.precedence = ATOM
    def __repr__(self) -> str:
        return f"TupleExpr({self.els!r})"
    def __str__(self) -> str:
        return f"({', '.join(map(str, self.els))})"

class SeqUnpack:
    def __init__(self, value: object, precedence: _Precedence = BOR):
        self.value: object = value
        self.precedence: _Precedence = precedence
    def __repr__(self) -> str:
        return f"SeqUnpack({self.value!r}, {self.precedence!r})"
    def __str__(self) -> str:
        if self.value.precedence < self.precedence:
            value_s: str = f"({self.value!s})"
        else:
            value_s: str = f"{self.value!s}"
        return f"*{value_s}"

class ListExpr:
    def __init__(self, els: list[object]):
        self.els: list[object] = els
        self.precedence = ATOM
    def __repr__(self) -> str:
        return f"ListExpr({self.els!r})"
    def __str__(self) -> str:
        return f"[{', '.join(map(str, self.els))}]"

class SetExpr:
    def __init__(self, els: list[object]):
        self.els: list[object] = els
        self.precedence = ATOM
    def __repr__(self) -> str:
        return f"SetExpr({self.els!r})"
    def __str__(self) -> str:
        return f"{{{', '.join(map(str, self.els))}}}"

class DictExpr:
    def __init__(self, keys: tuple[str], values: list[object]):
        self.keys: tuple[str] = keys
        self.values: list[object] = values
        self.precedence = ATOM
    def __repr__(self) -> str:
        return f"DictExpr({self.keys!r}, {self.values!r})"
    def __str__(self) -> str:
        return f"{{{', '.join(f'{value!s}' if key is None else f'{key!s}: {value!s}' for key, value in zip(self.keys, self.values))}}}"

class RaiseStmt:
    def __init__(self, exc: object, cause: object):
        self.exc: object = exc
        self.cause: object = cause
    def __repr__(self) -> str:
        return f"RaiseStmt({self.exc!r}, {self.cause!r})"
    def __str__(self) -> str:
        if self.exc:
            exc_s: str = f" {self.exc!s}"
        else:
            exc_s: str = ""
        if self.cause:
            cause_s: str = f" from {self.cause!s}"
        else:
            cause_s: str = ""
        return f"raise{exc_s}{cause_s}"

class SliceExpr:
    def __init__(self, start: object, stop: object, step: object):
        self.start: object = start
        self.stop: object = stop
        self.step: object = step
    def __repr__(self) -> str:
        return f"SliceExpr({self.start!r}, {self.stop!r}, {self.step!r})"
    def __str__(self) -> str:
        if self.start:
            start_s: str = f"{self.start!s}"
        else:
            start_s: str = ""
        if self.stop:
            stop_s: str = f"{self.stop!s}"
        else:
            stop_s: str = ""
        if self.step:
            return f"{start_s}:{stop_s}:{self.step!s}"
        return f"{start_s}:{stop_s}"

class Function:
    def __init__(self, annotations: dict, kwdefaults: dict, defaults: tuple, name: str, code_obj: code, code_str: str):
        self.annotations: dict = annotations
        self.kwdefaults: dict = kwdefaults
        self.defaults: tuple = defaults
        self.name: str = name
        self.code_obj: code = code_obj
        self.has_vararg: bool = (has_vararg := not not code_obj.co_flags & 4)
        self.positional_only_params: list[object]
        self.normal_params: list[object]
        self.keyword_only_params: list[object]
        if has_vararg:
            varnames: tuple[str] = code_obj.co_varnames
            argcount: int = code_obj.co_argcount
            self.positional_only_params = [
                *[AnnParam(name, annotations[name]) if name in annotations else name for name in varnames[:argcount]],
                SeqUnpack(Name(varnames[argcount + code_obj.co_kwonlyargcount])),
            ]
        elif posonlyargcount := code_obj.co_posonlyargcount:
            varnames: tuple[str] = code_obj.co_varnames
            self.positional_only_params = [AnnParam(name, annotations[name]) if name in annotations else name for name in varnames[:posonlyargcount]]
            if argcount := code_obj.co_argcount:
                self.normal_params = [AnnParam(name, annotations[name]) if name in annotations else name for name in varnames[posonlyargcount:argcount]]
            else:
                self.normal_params = []
        else:
            self.positional_only_params = []
            varnames = code_obj.co_varnames
            if argcount := code_obj.co_argcount:
                self.normal_params = [AnnParam(name, annotations[name]) if name in annotations else name for name in varnames[:argcount]]
            else:
                self.normal_params = []
        if kwonlyargcount := code_obj.co_kwonlyargcount:
            if code_obj.co_flags & 8:
                self.keyword_only_params = [
                    *[DefaultParam(param, kwdefaults[str_param])
                      if (str_param := param.param if type(param) is AnnParam else param) in kwdefaults
                      else param
                      for param in [AnnParam(name, annotations[name])
                                    if name in annotations
                                    else name
                                    for name in varnames[argcount:argcount + kwonlyargcount]]],
                    MapUnpack(Name(varnames[argcount + kwonlyargcount + has_vararg])),
                ]
            else:
                self.keyword_only_params = [DefaultParam(param, kwdefaults[str_param])
                                            if (str_param := param.param if type(param) is AnnParam else param) in kwdefaults
                                            else param
                                            for param in [AnnParam(name, annotations[name])
                                                          if name in annotations
                                                          else name
                                                          for name in varnames[argcount:argcount + kwonlyargcount]]]
        else:
            self.keyword_only_params = []
        if defaults:
            length: int = len(defaults)
            self.normal_params = [*self.normal_params[:-length], *[DefaultParam(param, default) for param, default in zip(self.normal_params[-length:], defaults[-argcount+posonlyargcount:])]]
            length -= argcount - posonlyargcount
            if length:
                self.positional_only_params = [*self.positional_only_params[:-length], *[DefaultParam(param, default) for param, default in zip(self.positional_only_params[-length:], defaults[:length])]]
        self.code_str: str = code_str
    def __repr__(self) -> str:
        return f"Function({self.annotations!r}, {self.kwdefaults!r}, {self.defaults!r}, {self.code_obj!r}, {self.code_str!r})"
    def __str__(self) -> str:
        args_s: str
        if self.has_vararg:
            args_s =  ', '.join(map(str, self.positional_only_params))
            if self.keyword_only_params:
                args_s = f"{args_s}, {', '.join(map(str, self.keyword_only_params))}" if args_s else ', '.join(map(str, self.keyword_only_params))
        else:
            if self.positional_only_params:
                args_s = f"{', '.join(map(str, self.positional_only_params))}, /"
            else:
                args_s = ""
            if self.normal_params:
                args_s = f"{args_s}, {', '.join(map(str, self.normal_params))}" if args_s else ', '.join(map(str, self.normal_params))
            if self.keyword_only_params:
                args_s = f"{args_s}, *, {', '.join(map(str, self.keyword_only_params))}" if args_s else ', '.join(map(str, self.keyword_only_params))
        return f"def {self.name}({args_s}):\n{self.code_str}"

class AnnParam:
    def __init__(self, param: object, annotation: object):
        self.param: object = param
        self.annotation: object = annotation
    def __repr__(self) -> str:
        return f"AnnParam({self.param!r}, {self.annotation!r})"
    def __str__(self) -> str:
        return f"{self.param!s}: {self.annotation!s}"

class DefaultParam:
    def __init__(self, param: object, value: object):
        self.param: object = param
        self.value: object = value
    def __repr__(self) -> str:
        return f"DefaultParam({self.param!r}, {self.value!r})"
    def __str__(self) -> str:
        return f"{self.param!s} = {self.value!s}" if type(self.param) is AnnParam else f"{self.param!s}={self.value!s}"

class MapUnpack:
    def __init__(self, value: object, precedence: _Precedence = BOR):
        self.value: object = value
        self.precedence: _Precedence = precedence
    def __repr__(self) -> str:
        return f"MapUnpack({self.value!r}, {self.precedence!r})"
    def __str__(self) -> str:
        if self.value.precedence < self.precedence:
            value_s: str = f"({self.value!s})"
        else:
            value_s: str = f"{self.value!s}"
        return f"**{value_s}"

class CallEx:
    def __init__(self, func: object, pargs: list[object], kwargs: list[tuple[str | None, object]]):
        self.func: object = func
        self.pargs: list[object] = pargs
        self.kwargs: list[tuple[str | None, object]] = kwargs
    def __repr__(self) -> str:
        return f"CallEx({self.func!r}, {self.pargs!r}, {self.kwargs!r})"
    def __str__(self) -> str:
        if self.kwargs:
            kwargs_s: str = f", {', '.join(f'{key!s}={val!s}' if key else f'{val!s}' for key, val in self.kwargs)}"
        else:
            kwargs_s: str = ""
        return f"{self.func!s}({', '.join(map(str, self.pargs))}{kwargs_s})"

class Formatted:
    def __init__(self, expr: object, conv_fn: str | None, fmt_spec: str | None):
        self.expr: object = expr
        self.conv_fn: str | None = conv_fn
        self.fmt_spec: str | None = fmt_spec
    def __repr__(self) -> str:
        return f"Formatted({self.expr!r}, {self.conv_fn!r}, {self.fmt_spec!r})"
    def __str__(self) -> str:
        if self.conv_fn:
            conv_s: str = f"!{self.conv_fn}"
        else:
            conv_s: str = ""
        if self.fmt_spec is not None:
            spec_s: str = f":{self.fmt_spec}"
        else:
            spec_s: str = ""
        return f"{self.expr!s}{conv_s}{spec_s}"

class BuildString:
    def __init__(self, values: list[Formatted | Const]):
        self.values: list[Formatted | Const] = values
    def __repr__(self) -> str:
        return f"BuildString({self.values!r})"
    def __str__(self) -> str:
        return ''.join(map(str, self.values))

newline_if_in: set[type] = {AugAssign, ClassDef}
store_subscrs: set[int] = {STORE_SUBSCR, STORE_SUBSCR_ADAPTIVE, STORE_SUBSCR_DICT, STORE_SUBSCR_LIST_INT}
store_attrs: set[int] = {STORE_ATTR, STORE_ATTR_ADAPTIVE, STORE_ATTR_INSTANCE_VALUE, STORE_ATTR_SLOT,
                         STORE_ATTR_WITH_HINT}
stores: set[int] = {*store_subscrs, STORE_NAME, *store_attrs, STORE_GLOBAL, STORE_FAST, STORE_DEREF,
                    STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}

def decompiler_expr_stack(code_obj: code, stack: list[object]) -> tuple[tuple[list[object], tuple[str]], int]:
    consts: tuple = code_obj.co_consts
    names: tuple = code_obj.co_names
    localsplus: tuple = code_obj.co_varnames + code_obj.co_cellvars + code_obj.co_freevars
    length_passed_stack: int = len(stack)
    PUSH: Callable[[object], None] = stack.append
    POP: Callable[[int | None], object] = stack.pop
    bytecode: list[tuple[int, int]] = [*struct_iter_unpack('BB', code_obj._co_code_adaptive)]
    length_bytecode: int = len(bytecode)
    bytecode_counter: int = 0
    kwnames: tuple[str] | None = None
    function_count: int = 0
    while bytecode_counter < length_bytecode:
        opcode, oparg = bytecode[bytecode_counter]
        bytecode_counter += 1
        if opcode is CACHE or opcode is NOP:
            continue
        elif opcode is POP_TOP:
            return POP()
        elif opcode is PUSH_NULL:
            PUSH(None)
            if bytecode[bytecode_counter][0] is LOAD_BUILD_CLASS:
                return None
            function_count += 1
        elif opcode is BINARY_OP_ADAPTIVE:
            right = POP()
            stack[-1] = BinOp(stack[-1], bin_ops[oparg], right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif (opcode is BINARY_OP_ADD_FLOAT or opcode is BINARY_OP_ADD_INT
                or opcode is BINARY_OP_ADD_UNICODE):
            right = POP()
            stack[-1] = BinOp(stack[-1], "+", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is BINARY_OP_INPLACE_ADD_UNICODE:
            right = POP()
            stack[-1] = AugAssign(stack[-1], "+", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is BINARY_OP_MULTIPLY_FLOAT or opcode is BINARY_OP_MULTIPLY_INT:
            right = POP()
            stack[-1] = AugAssign(stack[-1], "*", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is UNARY_POSITIVE:
            stack[-1] = UOp("+", stack[-1])
        elif opcode is UNARY_NEGATIVE:
            stack[-1] = UOp("-", stack[-1])
        elif opcode is UNARY_NOT:
            stack[-1] = UOp("not ", stack[-1])
        elif opcode is BINARY_OP_SUBTRACT_FLOAT or opcode is BINARY_OP_SUBTRACT_INT:
            right = POP()
            stack[-1] = AugAssign(stack[-1], "-", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is UNARY_INVERT:
            stack[-1] = UOp("~", stack[-1])
        elif (opcode is BINARY_SUBSCR_ADAPTIVE or opcode is BINARY_SUBSCR_DICT
                or opcode is BINARY_SUBSCR_GETITEM or opcode is BINARY_SUBSCR_LIST_INT
                or opcode is BINARY_SUBSCR_TUPLE_INT or opcode is BINARY_SUBSCR):
            sub: object = POP()
            stack[-1] = Trailer(stack[-1], ('[', ']'), sub)
            bytecode_counter += CACHE_ENTRIES_BINARY_SUBSCR
        elif (opcode is CALL_ADAPTIVE or opcode is CALL_PY_EXACT_ARGS or opcode is CALL_PY_WITH_DEFAULTS
                or opcode is CALL):
            is_meth: bool = stack[-oparg-2] is not None
            total_args: int = oparg + is_meth
            callb: object = stack[-total_args-1]
            positional_args: int = total_args - ((not not kwnames) and len(kwnames))
            args: list[object] = stack[-total_args:]
            res: Call = Call(callb, args, positional_args, total_args, kwnames)
            kwnames = None
            stack[-total_args-is_meth-1:] = res
            bytecode_counter += CACHE_ENTRIES_CALL
        elif (opcode is COMPARE_OP_ADAPTIVE or opcode is COMPARE_OP_FLOAT_JUMP
                or opcode is COMPARE_OP_INT_JUMP or opcode is COMPARE_OP_STR_JUMP):
            right: object = POP()
            stack[-1] = CmpOp(stack[-1], cmp_ops[oparg], right)
            bytecode_counter += CACHE_ENTRIES_COMPARE_OP
        elif (opcode is LOAD_ATTR_ADAPTIVE or opcode is LOAD_ATTR_INSTANCE_VALUE
                or opcode is LOAD_ATTR_MODULE or opcode is LOAD_ATTR_SLOT
                or opcode is LOAD_ATTR_WITH_HINT):
            stack[-1] = Attr(stack[-1], names[oparg])
            bytecode_counter += CACHE_ENTRIES_LOAD_ATTR
        elif not stack:
            if opcode is LOAD_CONST__LOAD_FAST:
                PUSH(consts[oparg])
            elif opcode is LOAD_FAST__LOAD_CONST or opcode is LOAD_FAST__LOAD_FAST:
                PUSH(localsplus[oparg])
            elif opcode is LOAD_GLOBAL_ADAPTIVE or opcode is LOAD_GLOBAL_BUILTIN or opcode is LOAD_GLOBAL_MODULE:
                if oparg & 1:
                    PUSH(None)
                PUSH(names[oparg >> 1])
        elif (opcode is LOAD_METHOD_ADAPTIVE or opcode is LOAD_METHOD_CLASS
              or opcode is LOAD_METHOD_MODULE or opcode is LOAD_METHOD_NO_DICT
              or opcode is LOAD_METHOD_WITH_DICT or opcode is LOAD_METHOD_WITH_DICT
              or opcode is LOAD_METHOD):
            stack[-1] = Attr(stack[-1], names[oparg])
            function_count += 1
            bytecode_counter += CACHE_ENTRIES_LOAD_METHOD
        elif (opcode is PRECALL_ADAPTIVE or opcode is PRECALL_BOUND_METHOD
                or opcode is PRECALL_BUILTIN_CLASS or opcode is PRECALL_BUILTIN_FAST_WITH_KEYWORDS
                or opcode is PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS
                or opcode is PRECALL_NO_KW_BUILTIN_FAST or opcode is PRECALL_NO_KW_BUILTIN_O
                or opcode is PRECALL_NO_KW_ISINSTANCE or opcode is PRECALL_NO_KW_LEN
                or opcode is PRECALL_NO_KW_LIST_APPEND or opcode is PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST
                or opcode is PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS or opcode is PRECALL_NO_KW_METHOD_DESCRIPTOR_O
                or opcode is PRECALL_NO_KW_STR_1 or opcode is PRECALL_NO_KW_TUPLE_1
                or opcode is PRECALL_NO_KW_TYPE_1 or opcode is PRECALL_PYFUNC or opcode is PRECALL):
            # `if not function_count` only designed for class definitions
            if not function_count:
                args = stack[length_passed_stack:]
                stack[length_passed_stack:] = []
                return ((args, kwnames), bytecode_counter-1)
            bytecode_counter += CACHE_ENTRIES_PRECALL
        else:
            return (([stack.pop() if stack else None], ()), bytecode_counter-1)
    return (([stack.pop() if stack else None], ()), bytecode_counter)

def decompiler(code_obj: code, *, exc_table_index: int | None = None,
               indent_level: int = 0, closures: tuple[object] = (),
               disable_return_none: bool = False) -> str:
    consts: tuple = code_obj.co_consts
    names: tuple = code_obj.co_names
    localsplus: tuple = code_obj.co_varnames + code_obj.co_cellvars + code_obj.co_freevars
    stack: list[object] = []
    PUSH: Callable[[object], None] = stack.append
    POP: Callable[[int | None], object] = stack.pop
    lines: list[str] = []
    new_line: Callable[[str], None] = lines.append
    bytecode: list[tuple[int, int]] = [*struct_iter_unpack('BB', code_obj._co_code_adaptive)]
    length_bytecode: int = len(bytecode)
    exc_table: list[tuple[int, int, int, int, bool]] = parse_exception_table(code_obj)
    exc_table_length: int = len(exc_table)
    exc_table_idx: int = exc_table_index or 0
    end_exc_idx: int = 0
    bytecode_counter: int = 0
    kwnames: tuple[str] | None = None
    is_extended_arg: bool = False
    def handle_MATCH_MAPPING(start: int) -> None:
        nonlocal bytecode, bytecode_counter
        end_idx: int = bytecode_counter + bytecode[start][1] + 1
        tuple_keys: tuple[str, ...] = consts[bytecode[start + 5][1]]
        length_map: int = len(tuple_keys)
        # TODO: handle the rest of MATCH_MAPPING
        bytecode_counter = end_idx
    def handle_MATCH_SEQUENCE(start: int, is_nested: bool = False) -> MatchSeq | None:
        nonlocal bytecode, bytecode_counter, indent_level
        if not is_nested:
            end_idx: int = bytecode_counter + bytecode[start][1] + 1
            if bytecode[start-2][0] is not POP_TOP:
                PUSH(MatchStmt(stack[-1], indent_level=0))
        length_seq: int = consts[bytecode[start + 2][1]]
        final_pos: list[int] = [*range(length_seq)]
        bytecode_counter = start + 6
        while (instr := bytecode[bytecode_counter])[0] is SWAP:
            final_pos[-1], final_pos[-(oparg := instr[1])] = final_pos[-oparg], final_pos[-1]
            bytecode_counter += 1
        res: list[object] = [None] * length_seq
        for real_pos in final_pos:
            if (opcode := instr[0]) is LOAD_CONST:
                res[real_pos] = Const(consts[instr[1]])
                bytecode_counter += 3
            elif opcode is MATCH_MAPPING:
                handle_MATCH_MAPPING(bytecode_counter)
            elif opcode is MATCH_SEQUENCE:
                res[real_pos] = handle_MATCH_SEQUENCE(bytecode_counter)
            elif opcode is STORE_NAME:
                res[real_pos] = Name(names[instr[1]])
                bytecode_counter += 1
            elif opcode is STORE_GLOBAL:
                res[real_pos] = Name(names[instr[1] >> 1])
                bytecode_counter += 1
            elif (opcode is STORE_FAST or opcode is STORE_DEREF
                    or opcode is STORE_FAST__LOAD_FAST or opcode is STORE_FAST__STORE_FAST):
                res[real_pos] = Name(localsplus[instr[1]])
                bytecode_counter += 1
            else:
                val: Name | Attr
                if opcode is LOAD_NAME:
                    val = Name(names[instr[1]])
                elif opcode is LOAD_GLOBAL:
                    val = Name(names[instr[1] >> 1])
                elif (opcode is LOAD_FAST or opcode is LOAD_CLOSURE
                        or opcode is LOAD_DEREF or opcode is LOAD_CLASSDEREF):
                    val = Name(names[instr[1]])
                else:
                    raise NotImplementedError(f"match sequence: {opname[opcode]} ({opcode})")
                bytecode_counter += 1
                while (instr := bytecode[bytecode_counter])[0] is LOAD_ATTR:
                    val = Attr(val, names[instr[1]])
                    bytecode_counter += 1
                res[real_pos] = val
                bytecode_counter += 2
            instr = bytecode[bytecode_counter]
        if is_nested:
            return MatchSeq(res)
        code_start: int = bytecode_counter
        code_end: int = end_idx - 1
        while bytecode[code_end - 1][0] is POP_TOP:
            code_end -= 1
        bytecode_counter = end_idx
        stack[-1].cases.append(
            MatchSeq(
                res,
                decompiler(
                    code_obj.replace(
                        co_code=b''.join(
                            map(bytes, bytecode[code_start:code_end])
                        )
                    ),
                    indent_level=indent_level + 2
                ),
                indent_level=indent_level + 1
            )
        )
    def handle_UNPACK_SEQUENCE(num_el: int) -> TupleExpr:
        nonlocal bytecode, bytecode_counter
        list_els: list[object] = [None] * num_el
        idx: int = 0
        while idx < num_el:
            start: int = bytecode_counter
            if (opcode := (instr := bytecode[bytecode_counter])[0]) is UNPACK_SEQUENCE:
                bytecode_counter += 1
                list_els[idx] = handle_UNPACK_SEQUENCE(instr[1])
                idx += 1
                continue
            elif opcode is UNPACK_EX:
                bytecode_counter += 1
                list_els[idx] = handle_UNPACK_EX(instr[1])
                idx += 1
                continue
            while opcode not in stores:
                bytecode_counter += 1
                opcode = (instr := bytecode[bytecode_counter])[0]
            expr: object
            ([expr], ()), _ = decompiler_expr_stack(code_obj.replace(co_code=code_obj.co_code[start*2:bytecode_counter*2]), stack)
            if (opcode is STORE_SUBSCR or opcode is STORE_SUBSCR_ADAPTIVE
                    or opcode is STORE_SUBSCR_DICT or opcode is STORE_SUBSCR_LIST_INT):
                list_els[idx] = Trailer(POP(), ('[', ']'), expr)
            elif opcode is STORE_NAME:
                list_els[idx] = Name(names[instr[1]])
                PUSH(expr)
            elif (opcode is STORE_ATTR or opcode is STORE_ATTR_ADAPTIVE
                    or opcode is STORE_ATTR_INSTANCE_VALUE or opcode is STORE_ATTR_SLOT
                    or opcode is STORE_ATTR_WITH_HINT):
                list_els[idx] = Attr(expr, names[instr[1]])
            elif opcode is STORE_GLOBAL:
                list_els[idx] = Name(names[instr[1] >> 1])
                PUSH(expr)
            else:
                # opcode in {STORE_FAST, STORE_DEREF, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}
                list_els[idx] = Name(localsplus[instr[1]])
                PUSH(expr)
            idx += 1
            bytecode_counter += 1
        return TupleExpr(list_els)
    def handle_UNPACK_EX(oparg: int) -> TupleExpr:
        nonlocal bytecode_counter
        list_els: list[object] = [None] * (num_el := 1 + (before_star := oparg & 0xFF) + (oparg >> 8))
        idx: int = 0
        while idx < num_el:
            start: int = bytecode_counter
            if idx != before_star:
                if (opcode := (instr := bytecode[bytecode_counter])[0]) is UNPACK_SEQUENCE:
                    bytecode_counter += 1
                    list_els[idx] = handle_UNPACK_SEQUENCE(instr[1])
                    idx += 1
                    continue
                elif opcode is UNPACK_EX:
                    bytecode_counter += 1
                    list_els[idx] = handle_UNPACK_EX(instr[1])
                    idx += 1
                    continue
                while opcode not in stores:
                    bytecode_counter += 1
                    opcode = (instr := bytecode[bytecode_counter])[0]
                expr: object
                ([expr], ()), _ = decompiler_expr_stack(code_obj.replace(co_code=code_obj.co_code[start*2:bytecode_counter*2]), stack)
                if (opcode is STORE_SUBSCR or opcode is STORE_SUBSCR_ADAPTIVE
                        or opcode is STORE_SUBSCR_DICT or opcode is STORE_SUBSCR_LIST_INT):
                    list_els[idx] = Trailer(POP(), ('[', ']'), expr)
                elif opcode is STORE_NAME:
                    list_els[idx] = Name(names[instr[1]])
                    PUSH(expr)
                elif (opcode is STORE_ATTR or opcode is STORE_ATTR_ADAPTIVE
                        or opcode is STORE_ATTR_INSTANCE_VALUE or opcode is STORE_ATTR_SLOT
                        or opcode is STORE_ATTR_WITH_HINT):
                    list_els[idx] = Attr(expr, names[instr[1]])
                elif opcode is STORE_GLOBAL:
                    list_els[idx] = Name(names[instr[1] >> 1])
                    PUSH(expr)
                else:
                    # opcode in {STORE_FAST, STORE_DEREF, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}
                    list_els[idx] = Name(localsplus[instr[1]])
                    PUSH(expr)
            else:
                if (opcode := (instr := bytecode[bytecode_counter])[0]) is UNPACK_SEQUENCE:
                    bytecode_counter += 1
                    list_els[idx] = SeqUnpack(handle_UNPACK_SEQUENCE(instr[1]))
                    idx += 1
                    continue
                elif opcode is UNPACK_EX:
                    bytecode_counter += 1
                    list_els[idx] = SeqUnpack(handle_UNPACK_EX(instr[1]))
                    idx += 1
                    continue
                while opcode not in stores:
                    bytecode_counter += 1
                    opcode = (instr := bytecode[bytecode_counter])[0]
                expr: object
                ([expr], ()), _ = decompiler_expr_stack(code_obj.replace(co_code=code_obj.co_code[start*2:bytecode_counter*2]), stack)
                if (opcode is STORE_SUBSCR or opcode is STORE_SUBSCR_ADAPTIVE
                        or opcode is STORE_SUBSCR_DICT or opcode is STORE_SUBSCR_LIST_INT):
                    list_els[idx] = SeqUnpack(Trailer(POP(), ('[', ']'), expr))
                elif opcode is STORE_NAME:
                    list_els[idx] = SeqUnpack(Name(names[instr[1]]))
                    PUSH(expr)
                elif (opcode is STORE_ATTR or opcode is STORE_ATTR_ADAPTIVE
                        or opcode is STORE_ATTR_INSTANCE_VALUE or opcode is STORE_ATTR_SLOT
                        or opcode is STORE_ATTR_WITH_HINT):
                    list_els[idx] = SeqUnpack(Attr(expr, names[instr[1]]))
                elif opcode is STORE_GLOBAL:
                    list_els[idx] = SeqUnpack(Name(names[instr[1] >> 1]))
                    PUSH(expr)
                else:
                    # opcode in {STORE_FAST, STORE_DEREF, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}
                    list_els[idx] = SeqUnpack(Name(localsplus[instr[1]]))
                    PUSH(expr)
            idx += 1
            bytecode_counter += 1
        return TupleExpr(list_els)
    while bytecode_counter < length_bytecode:
        if (opcode := (instr := bytecode[bytecode_counter])[0]) is EXTENDED_ARG or opcode is EXTENDED_ARG_QUICK:
            oparg = instr[1]
            while opcode is EXTENDED_ARG or opcode is EXTENDED_ARG_QUICK:
                opcode = (instr := bytecode[bytecode_counter := bytecode_counter + 1])[0]
                oparg = (oparg << 8) | instr[1]
        else:
            oparg = instr[1]
        bytecode_counter += 1
        if (opcode is CACHE or opcode is NOP or opcode is JUMP_BACKWARD_QUICK
                or opcode is IMPORT_STAR or opcode is SETUP_ANNOTATIONS
                or opcode is IMPORT_FROM or opcode is JUMP_FORWARD
                or opcode is RERAISE or opcode is MAKE_CELL
                or opcode is JUMP_BACKWARD or opcode is RESUME_QUICK
                or opcode is RESUME):
            continue
        elif opcode is POP_TOP:
            if obj := POP():
                if (otyp := type(obj)) is MatchStmt:
                    new_line(f"{obj!s}")
                else:
                    new_line(f"{'    '*indent_level}{obj!s}")
        elif opcode is PUSH_NULL:
            PUSH(None)
        elif opcode is BINARY_OP_ADAPTIVE or opcode is BINARY_OP:
            right = POP()
            stack[-1] = BinOp(stack[-1], bin_ops[oparg], right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif (opcode is BINARY_OP_ADD_FLOAT or opcode is BINARY_OP_ADD_INT
                or opcode is BINARY_OP_ADD_UNICODE):
            right = POP()
            stack[-1] = BinOp(stack[-1], "+", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is BINARY_OP_INPLACE_ADD_UNICODE:
            right = POP()
            stack[-1] = AugAssign(stack[-1], "+", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is BINARY_OP_MULTIPLY_FLOAT or opcode is BINARY_OP_MULTIPLY_INT:
            right = POP()
            stack[-1] = AugAssign(stack[-1], "*", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is UNARY_POSITIVE:
            stack[-1] = UOp("+", stack[-1])
        elif opcode is UNARY_NEGATIVE:
            stack[-1] = UOp("-", stack[-1])
        elif opcode is UNARY_NOT:
            stack[-1] = UOp("not ", stack[-1])
        elif opcode is BINARY_OP_SUBTRACT_FLOAT or opcode is BINARY_OP_SUBTRACT_INT:
            right = POP()
            stack[-1] = AugAssign(stack[-1], "-", right)
            bytecode_counter += CACHE_ENTRIES_BINARY_OP
        elif opcode is UNARY_INVERT:
            stack[-1] = UOp("~", stack[-1])
        elif (opcode is BINARY_SUBSCR_ADAPTIVE or opcode is BINARY_SUBSCR_DICT
                or opcode is BINARY_SUBSCR_GETITEM or opcode is BINARY_SUBSCR_LIST_INT
                or opcode is BINARY_SUBSCR_TUPLE_INT or opcode is BINARY_SUBSCR):
            sub: object = POP()
            stack[-1] = Trailer(stack[-1], ('[', ']'), sub)
            bytecode_counter += CACHE_ENTRIES_BINARY_SUBSCR
        elif (opcode is CALL_ADAPTIVE or opcode is CALL_PY_EXACT_ARGS or opcode is CALL_PY_WITH_DEFAULTS
                or opcode is CALL):
            is_meth: bool = stack[-oparg-2] is not None
            total_args: int = oparg + is_meth
            callb: object = stack[-total_args-1]
            positional_args: int = total_args - ((not not kwnames) and len(kwnames))
            args: list[object] = stack[-total_args:]
            res: Call = Call(callb, args, positional_args, total_args, kwnames)
            kwnames = None
            stack[-total_args-is_meth-1:] = [res]
            bytecode_counter += CACHE_ENTRIES_CALL
        elif (opcode is COMPARE_OP_ADAPTIVE or opcode is COMPARE_OP_FLOAT_JUMP
                or opcode is COMPARE_OP_INT_JUMP or opcode is COMPARE_OP_STR_JUMP
                or opcode is COMPARE_OP):
            right: object = POP()
            stack[-1] = CmpOp(stack[-1], cmp_ops[oparg], right)
            bytecode_counter += CACHE_ENTRIES_COMPARE_OP
        elif opcode is GET_LEN or opcode is MATCH_KEYS:
            raise NotImplementedError(f"{opname[opcode]} ({opcode})")
        elif opcode is MATCH_MAPPING:
            handle_MATCH_MAPPING(bytecode_counter)
        elif opcode is MATCH_SEQUENCE:
            handle_MATCH_SEQUENCE(bytecode_counter)
        elif opcode is PUSH_EXC_INFO:
            start, end, _, _, _ = exc_table[exc_table_idx]
            end_exc_idx = bytecode_counter + bytecode[bytecode_counter - 2][1] - 1
            PUSH(TryStmt(decompiler(code_obj.replace(co_code=b''.join(map(bytes, bytecode[start:end])))), indent_level=indent_level+1))
            exc_table_idx += 1
        elif opcode is CHECK_EXC_MATCH:
            # TODO: fix CHECK_EXC_MATCH if it works wrong
            exc_catch: object = POP()
            exc_target: Name | None
            if (opcode := (instr := bytecode[bytecode_counter + 1])[0]) is STORE_NAME:
                exc_target = Name(names[instr[1]])
            elif opcode is STORE_GLOBAL:
                exc_target = Name(names[instr[1] >> 1])
            elif (opcode is STORE_FAST or opcode is STORE_DEREF
                    or opcode is STORE_FAST__LOAD_FAST or opcode is STORE_FAST__STORE_FAST):
                exc_target = Name(localsplus[instr[1]])
            else:
                exc_target = None
            exc_code, exc_table_idx = decompiler(
                code_obj.replace(
                    co_code=b''.join(
                        map(bytes, bytecode[bytecode_counter + 2:exc_table[exc_table_idx][1]])
                    )
                ),
                indent_level=indent_level + 1,
                exc_table_index=exc_table_idx + 1,
            )
            stack[-1].handlers.append(ExceptStmt(exc_catch, exc_code, exc_target, indent_level=indent_level))
            if (exc_table_idx := exc_table_idx + 1) == exc_table_length or not exc_table[exc_table_idx][4]:
                new_line(f"{'    '*indent_level}{POP()!s}")
            bytecode_counter = end_exc_idx
            if bytecode[bytecode_counter][0] is POP_EXCEPT:
                bytecode_counter += bytecode[bytecode_counter + 4][1] + 5
        elif (opcode is CHECK_EG_MATCH or opcode is WITH_EXCEPT_START or opcode is GET_AITER
                or opcode is GET_ANEXT or opcode is BEFORE_ASYNC_WITH or opcode is BEFORE_WITH
                or opcode is END_ASYNC_FOR or opcode is GET_ITER or opcode is GET_YIELD_FROM_ITER
                or opcode is YIELD_VALUE or opcode is ASYNC_GEN_WRAP
                or opcode is PREP_RERAISE_STAR or opcode is POP_EXCEPT
                or opcode is FOR_ITER or opcode is JUMP_IF_FALSE_OR_POP
                or opcode is JUMP_IF_TRUE_OR_POP or opcode is POP_JUMP_FORWARD_IF_FALSE
                or opcode is POP_JUMP_FORWARD_IF_TRUE or opcode is SEND
                or opcode is POP_JUMP_FORWARD_IF_NOT_NONE or opcode is POP_JUMP_FORWARD_IF_NONE
                or opcode is GET_AWAITABLE or opcode is SET_ADD
                or opcode is MAP_ADD or opcode is POP_JUMP_BACKWARD_IF_NOT_NONE
                or opcode is POP_JUMP_BACKWARD_IF_NONE or opcode is POP_JUMP_FORWARD_IF_FALSE
                or opcode is POP_JUMP_BACKWARD_IF_TRUE or opcode is MATCH_CLASS
                or opcode is JUMP_BACKWARD_NO_INTERRUPT):
            # TODO: all these opcodes
            raise NotImplementedError(f"{opname[opcode]} ({opcode})")
        elif (opcode is LOAD_ATTR_ADAPTIVE or opcode is LOAD_ATTR_INSTANCE_VALUE
                or opcode is LOAD_ATTR_MODULE or opcode is LOAD_ATTR_SLOT
                or opcode is LOAD_ATTR_WITH_HINT):
            stack[-1] = Attr(stack[-1], names[oparg])
            bytecode_counter += CACHE_ENTRIES_LOAD_ATTR
        elif opcode is LOAD_CONST__LOAD_FAST or opcode is LOAD_CONST:
            PUSH(Const(consts[oparg]))
        elif opcode is LOAD_FAST__LOAD_CONST or opcode is LOAD_FAST__LOAD_FAST:
            PUSH(Name(localsplus[oparg]))
        elif (opcode is LOAD_GLOBAL_ADAPTIVE or opcode is LOAD_GLOBAL_BUILTIN or opcode is LOAD_GLOBAL_MODULE
                or opcode is LOAD_GLOBAL):
            if oparg & 1:
                PUSH(None)
            PUSH(Name(names[oparg >> 1]))
        elif (opcode is LOAD_METHOD_ADAPTIVE or opcode is LOAD_METHOD_CLASS
              or opcode is LOAD_METHOD_MODULE or opcode is LOAD_METHOD_NO_DICT
              or opcode is LOAD_METHOD_WITH_DICT or opcode is LOAD_METHOD_WITH_DICT
              or opcode is LOAD_METHOD):
            stack[-1] = Attr(stack[-1], names[oparg])
            bytecode_counter += CACHE_ENTRIES_LOAD_METHOD
        elif (opcode is STORE_SUBSCR or opcode is STORE_SUBSCR_ADAPTIVE
                or opcode is STORE_SUBSCR_DICT or opcode is STORE_SUBSCR_LIST_INT):
            sub = POP()
            container = POP()
            if (type_TOS := type(TOS := POP())) is AugAssign:
                new_line(f"{'    '*indent_level}{TOS!s}")
            else:
                new_line(f"{'    '*indent_level}{container!s}[{sub!s}] = {TOS!s}")
        elif opcode is DELETE_SUBSCR:
            sub = POP()
            new_line(f"{'    '*indent_level}del {POP()!s}[{sub!s}]")
        elif (opcode is PRECALL_ADAPTIVE or opcode is PRECALL_BOUND_METHOD
                or opcode is PRECALL_BUILTIN_CLASS or opcode is PRECALL_BUILTIN_FAST_WITH_KEYWORDS
                or opcode is PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS
                or opcode is PRECALL_NO_KW_BUILTIN_FAST or opcode is PRECALL_NO_KW_BUILTIN_O
                or opcode is PRECALL_NO_KW_ISINSTANCE or opcode is PRECALL_NO_KW_LEN
                or opcode is PRECALL_NO_KW_LIST_APPEND or opcode is PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST
                or opcode is PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS or opcode is PRECALL_NO_KW_METHOD_DESCRIPTOR_O
                or opcode is PRECALL_NO_KW_STR_1 or opcode is PRECALL_NO_KW_TUPLE_1
                or opcode is PRECALL_NO_KW_TYPE_1 or opcode is PRECALL_PYFUNC or opcode is PRECALL):
            bytecode_counter += CACHE_ENTRIES_PRECALL
        elif opcode is PRINT_EXPR:
            new_line(f"{'    '*indent_level}{POP()!s}")
        elif opcode is LOAD_BUILD_CLASS:
            class_code_obj: code = consts[bytecode[bytecode_counter][1]]
            name: str = consts[bytecode[bytecode_counter + 2][1]]
            code_str: str = decompiler(class_code_obj.replace(co_code=class_code_obj.co_code[0:2]+class_code_obj.co_code[10:]), indent_level=indent_level+1)
            bytecode_counter += 3
            args: tuple[list[object], tuple[str]]
            args, bytecode_counter = decompiler_expr_stack(code_obj.replace(co_code=code_obj.co_code[bytecode_counter*2:]), stack)
            stack[-1] = ClassDef(name, args[0], args[1], code_str)
            bytecode_counter += CACHE_ENTRIES_PRECALL + CACHE_ENTRIES_CALL + 2
        elif opcode is LOAD_ASSERTION_ERROR:
            # there would be a conditional check in POP_JUMP_FORWARD_IF_TRUE
            # to see if it's followed by LOAD_ASSERTION_ERROR
            # so it's safe to POP() the stack here
            expr: object = POP()
            end_idx: int = bytecode[bytecode_counter:].index((RAISE_VARARGS, 1))
            message: object
            ([message], ()), bytecode_counter = decompiler_expr_stack(code_obj.replace(co_code=code_obj.co_code[bytecode_counter*2:end_idx*2]), stack)
            bytecode_counter += 1
            new_line(f"{'    '*indent_level}{AssertStmt(expr, message)!s}")
        elif opcode is LIST_TO_TUPLE:
            stack[-1] = TupleExpr(stack[-1].els)
        elif opcode is RETURN_VALUE:
            val: object = POP()
            if not (disable_return_none and type(val) is Const and val.value is None):
                new_line(f"{'    '*indent_level}return {val!s}")
        elif opcode is STORE_NAME:
            print(lines)
            if (type_TOS := type(val := POP())) is AugAssign:
                new_line(f"{'    '*indent_level}{val!s}")
            elif type_TOS is ClassDef:
                new_line(f"{val!s}")
            else:
                new_line(f"{names[oparg]} = {val!s}")
        elif opcode is DELETE_NAME:
            new_line(f"del {names[oparg]}")
        elif (opcode is UNPACK_SEQUENCE or opcode is UNPACK_SEQUENCE_ADAPTIVE
                or opcode is UNPACK_SEQUENCE_LIST or opcode is UNPACK_SEQUENCE_TUPLE
                or opcode is UNPACK_SEQUENCE_TWO_TUPLE):
            new_line(f"{'    '*indent_level}{handle_UNPACK_SEQUENCE(oparg)!s} = {POP()!s}")
        elif opcode is UNPACK_EX:
            new_line(f"{'    '*indent_level}{handle_UNPACK_EX(oparg)!s} = {POP()!s}")
        elif (opcode is STORE_ATTR or opcode is STORE_ATTR_ADAPTIVE
                or opcode is STORE_ATTR_INSTANCE_VALUE or opcode is STORE_ATTR_SLOT
                or opcode is STORE_ATTR_WITH_HINT):
            obj: object = POP()
            if (type_TOS := type(val := POP())) is AugAssign:
                new_line(f"{'    '*indent_level}{val!s}")
            elif type_TOS is ClassDef:
                new_line(f"{val!s}")
            else:
                new_line(f"{obj!s}.{names[oparg]} = {val!s}")
            bytecode_counter += CACHE_ENTRIES_STORE_ATTR
        elif opcode is DELETE_ATTR:
            new_line(f"del {POP()!s}.{names[oparg]}")
        elif opcode is STORE_GLOBAL:
            if (type_TOS := type(val := POP())) is AugAssign:
                new_line(f"{'    '*indent_level}{val!s}")
            elif type_TOS is ClassDef:
                new_line(f"{val!s}")
            else:
                new_line(f"{names[oparg >> 1]} = {val!s}")
        elif opcode is DELETE_GLOBAL:
            new_line(f"del {names[oparg >> 1]}")
        elif opcode is SWAP:
            if (oparg == 2
                    and (bytecode[bytecode_counter][0] in stores
                         or bytecode[bytecode_counter + 1][0] in store_attrs
                         or bytecode[bytecode_counter + 2][0] in store_subscrs)):
                val: object = TupleExpr(stack[-2:])
                del stack[-2:]
                new_line(f"{'    '*indent_level}{handle_UNPACK_SEQUENCE(2)!s} = {val!s}")
            elif (oparg == 3
                    and (bytecode[bytecode_counter][0] in stores
                         or bytecode[bytecode_counter + 1][0] in store_attrs
                         or bytecode[bytecode_counter + 2][0] in store_subscrs)):
                val: object = TupleExpr(stack[-3:])
                del stack[-3:]
                new_line(f"{'    '*indent_level}{handle_UNPACK_SEQUENCE(3)!s} = {val!s}")
            else:
                stack[-1], stack[-oparg] = stack[-oparg], stack[-1]
        elif opcode is LOAD_NAME:
            PUSH(Name(names[oparg]))
        elif opcode is BUILD_TUPLE:
            if oparg:
                stack[-oparg:] = [TupleExpr(stack[-oparg:])]
            else:
                PUSH(TupleExpr([]))
        elif opcode is BUILD_LIST:
            if oparg:
                stack[-oparg:] = [ListExpr(stack[-oparg:])]
            else:
                PUSH(ListExpr([]))
        elif opcode is BUILD_SET:
            if oparg:
                stack[-oparg:] = [SetExpr(stack[-oparg:])]
            else:
                PUSH(SetExpr([]))
        elif opcode is BUILD_MAP:
            if oparg == 1:
                val: object = POP()
                stack[-1] = DictExpr((stack[-1].value,), [val])
            elif oparg:
                stack[-oparg:] = [DictExpr(POP().value, stack[-oparg:])]
            else:
                PUSH(DictExpr((),[]))
        elif opcode is LOAD_ATTR:
            stack[-1] = Attr(stack[-1], names[oparg])
        elif opcode is IMPORT_NAME:
            del stack[-2:] # pop out unneeded stack items
            from_list: list[str] = []
            while (opcode := (instr := bytecode[bytecode_counter])[0]) is IMPORT_FROM:
                if (opcode := (instr2 := bytecode[bytecode_counter + 1])[0]) is STORE_NAME:
                    from_list.append((names[instr[1]], names[instr2[1]]))
                elif opcode is STORE_GLOBAL:
                    from_list.append((names[instr[1]], names[instr2[1] >> 1]))
                else:
                    # opcode in {STORE_FAST, STORE_DEREF, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}
                    from_list.append((names[instr[1]], localsplus[instr2[1]]))
                bytecode_counter += 2
            bytecode_counter += 1
            if not from_list:
                if opcode is IMPORT_STAR:
                    new_line(f"{'    '*indent_level}from {names[oparg]} import *")
                elif opcode is STORE_NAME:
                    if (name_as := names[instr[1]]) == (name := names[oparg]):
                        new_line(f"{'    '*indent_level}import {name}")
                    else:
                        new_line(f"{'    '*indent_level}import {name} as {name_as}")
                    
                elif opcode is STORE_GLOBAL:
                    if (name_as := names[instr[1] >> 1]) == (name := names[oparg]):
                        new_line(f"{'    '*indent_level}import {name}")
                    else:
                        new_line(f"{'    '*indent_level}import {name} as {name_as}")
                else:
                    # opcode in {STORE_FAST, STORE_DEREF, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}
                    if (name_as := localsplus[instr[1]]) == (name := names[oparg]):
                        new_line(f"{'    '*indent_level}import {name}")
                    else:
                        new_line(f"{'    '*indent_level}import {name} as {name_as}")
            else:
                new_line(f"{'    '*indent_level}from {names[oparg]} import {', '.join(map(lambda x: f'{x[0]} as {x[1]}' if x[0] != x[1] else f'{x[0]}', from_list))}")
        elif opcode is IS_OP:
            right: object = POP()
            stack[-1] = CmpOp(stack[-1], "is not" if oparg else "is", right)
        elif opcode is CONTAINS_OP:
            right: object = POP()
            stack[-1] = CmpOp(stack[-1], "not in" if oparg else "in", right)
        elif opcode is COPY:
            PUSH(stack[-oparg])
        elif (opcode is LOAD_FAST or opcode is LOAD_CLOSURE or opcode is LOAD_DEREF
                or opcode is LOAD_CLASSDEREF):
            PUSH(Name(localsplus[oparg]))
        elif(opcode is STORE_FAST or opcode is STORE_DEREF
                    or opcode is STORE_FAST__LOAD_FAST or opcode is STORE_FAST__STORE_FAST):
            if (type_TOS := type(val := POP())) is AugAssign:
                new_line(f"{'    '*indent_level}{val!s}")
            elif type_TOS is ClassDef:
                new_line(f"{val!s}")
            else:
                new_line(f"{localsplus[oparg]} = {val!s}")
        elif opcode is DELETE_FAST or opcode is DELETE_DEREF:
            new_line(f"del {localsplus[oparg]}")
        elif opcode is RAISE_VARARGS:
            cause: object
            exc: object
            cause = exc = None
            if oparg == 2:
                cause = POP()
            if oparg:
                exc = POP()
            new_line(f"{'    '*indent_level}{RaiseStmt(exc, cause)}")
        elif opcode is MAKE_FUNCTION:
            func_code_obj: code = POP().value
            closure: tuple[object] = POP().els if oparg & 8 else ()
            annotations: dict[str, str] = dict(zip((TOS := POP().els)[::2], TOS[1::2])) if oparg & 4 else ()
            kwdefaults: dict[str, keys] = dict(zip((TOS := POP()).keys, TOS.values)) if oparg & 2 else {}
            defaults: tuple[object] = POP().els if oparg & 1 else ()
            function_str: str = decompiler(
                func_code_obj,
                indent_level=indent_level + 1,
                closures=closure,
                disable_return_none=True,
            )
            if (opcode := (instr := bytecode[bytecode_counter])[0]) is STORE_NAME:
                name = names[instr[1]]
            elif opcode is STORE_GLOBAL:
                name = names[instr[1] >> 1]
            else:
                # opcode in {STORE_FAST, STORE_DEREF, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST}
                name = localsplus[instr[1]]
            new_line(f"{'    '*indent_level}{Function(annotations, kwdefaults, defaults, name, func_code_obj, function_str)!s}")
            bytecode_counter += 1 # skip STORE_*
        elif opcode is BUILD_SLICE:
            step: object = None
            if oparg == 3:
                step = None if type(step := POP()) is Const and step.value is None else step
            if type(stop := POP()) is Const and stop.value is None:
                stop = None
            stack[-1] = SliceExpr(None if type(start := stack[-1]) is Const and start.value is None else start, stop, step)
        elif opcode is CALL_FUNCTION_EX:
            kwargs: list[tuple[str | None, object]]
            if oparg & 1:
                kwargs = [*zip((TOS := POP()).keys, TOS.values)]
            else:
                kwargs = []
            pargs: list[object] = TOS.els if type(TOS := POP()) is TupleExpr else [SeqUnpack(TOS, precedence=TEST)]
            stack[-1] = CallEx(stack[-1], pargs, kwargs)
        elif opcode is LIST_APPEND:
            stack[-oparg-1].els.append(POP())
        elif opcode is COPY_FREE_VARS:
            new_line(f"{'    '*indent_level}nonlocal {', '.join(map(str, closures))}")
        elif opcode is FORMAT_VALUE:
            fmt_spec: str | None = POP().value if (have_fmt_spec := oparg & 4) else None
            stack[-1] = Formatted(stack[-1], (None, 's', 'r', 'a')[oparg & 3], fmt_spec)
        elif opcode is BUILD_CONST_KEY_MAP:
            stack[-oparg:] = [DictExpr(POP().value, stack[-oparg:])]
        elif opcode is BUILD_STRING:
            stack[-oparg:] = [BuildString(stack[-oparg:])]
        elif opcode is LIST_EXTEND:
            if type(TOS := POP()) is ListExpr:
                stack[-oparg].els.extend(TOS.els)
            else:
                stack[-oparg].els.append(SeqUnpack(TOS, precedence=TEST))
        elif opcode is SET_UPDATE:
            if type(TOS := POP()) is SetExpr:
                stack[-oparg].els.extend(TOS.els)
            else:
                stack[-oparg].els.append(SeqUnpack(TOS))
        elif opcode is DICT_MERGE or opcode is DICT_UPDATE:
            if type(TOS := POP()) is DictExpr:
                (dictionary := stack[-oparg]).keys += TOS.keys
                dictionary.values.extend(TOS.values)
            else:
                (dictionary := stack[-oparg]).keys += (None,)
                dictionary.values.append(MapUnpack(TOS, precedence=(TEST, BOR)[opcode is DICT_UPDATE]))
        elif opcode is KW_NAMES:
            kwnames = consts[oparg]
    if exc_table_index is not None:
        return '\n'.join(lines) if lines else f"{'   '*indent_level}pass", exc_table_idx
    return '\n'.join(lines) if lines else f"{'    '*indent_level}pass"

print(decompiler(compile("""
(a, *[b, c], d) = 3
a, b = b, a
a, b, c = c, b, a
from a import *
from a import b as c, c as d, d, e, f as g
import a
import a as b
def g(x, /, b):
    a(x, *f, l=7, **b)
""",'a','exec'), disable_return_none=True))
print(decompiler(compile("a(x,*f,g,*h,l=2,g=7,**b,mm=3)",'a','exec'), disable_return_none=True))
print(decompiler(compile("(_:=[print,chr,int,(___:=(((_:=(([]==[])+([{}]!=[])))+(_<<_))+(__:=(~~([]==[])))+_+_+(_/_))*(_+_+_)+_/_),____:=(_i:=([{___}]==[{__}]))+___+_i+_i+_i+_i+_i+_i+_i+_i+_i+_i+_/_+_,(_____:=(__+_))*_____+____,(__+_+_+_+_+_+_+_+_+_+_+_+_)*_**_+__+_+__+__,(_+_+_+_+_<<_____>>__<<_<<_//_%_%_)*(__:=_-_)+_+__+____+_+_+_+__+_])[~~([]!=[])](_[[]==[]](_[__:=(~~([]==[])+([]==[]))](_[___:=~~([]==[])+([]==[])+~~([]==[])]))+_[[]==[]](_[__:=(~~([]==[])+([]==[]))](_[___+(~~([]==[]))]))+_[[]==[]](_[__:=(~~([]==[])+([]==[]))](_[___+__]))+_[[]==[]](_[__:=(~~([]==[])+([]==[]))](_[___+__+([]==[])]))+_[[]==[]](_[__:=(~~([]==[])+([]==[]))](_[___*__+(__//__)])))",'a','exec').replace(co_names=('print','chr','int','a','b','c','d','e','f','g','h','i')), disable_return_none=True))
