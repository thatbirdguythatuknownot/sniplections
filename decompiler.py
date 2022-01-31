from struct import iter_unpack
from sys import _getframe, stderr
from collections.abc import Mapping, Sequence
from types import FunctionType as function, CellType as cell
from inspect import signature


from opcode import opmap, _nb_ops
from ctypes import py_object

vars().update(opmap)
addr = py_object.from_address
binop_symbols = list(map(lambda x: x[1], _nb_ops))

del opmap
del py_object
del _nb_ops

@lambda c:c()
class DEFAULT_VALUE:
    pass

def if_error_then(source, excs, mode=exec):
    frame = _getframe(1)
    globals_dict, locals_dict = frame.f_globals, frame.f_locals
    try:
        return mode(source, globals_dict, locals_dict)
    except BaseException as exc:
        print(exc)
        if type(exc) in excs:
            return mode(excs[type(exc)], globals_dict, locals_dict)
        raise

def assert_func(cond, message):
    assert cond, message

dict_iter = dict.__iter__
def merge_override(a, b, o):
    if not isinstance(a, dict):
        return (-1, DEFAULT_VALUE)
    if o is 1:
        return a.update(b)
    if isinstance(b, dict) and type(b).__iter__ is dict_iter:
        if b is a or not len(b):
            return (0, None)
        if not len(a):
            return a.update(b)
        for key, value in b.items():
            err = key in a
            if err:
                if not o: continue
                return (-1, key)
            else:
                a[key] = value
    else:
        for key in b.keys():
            status = key in a
            if status:
                if not o: continue
                return (-1, key)
            a[key] = b[key]
    return (0, None)

def addr2line(code_obj, addrq):
    prevlineno = 0
    for _, end, lineno in code_obj.co_lines():
        if addrq < end:
            prevlineno = lineno
        else:
            return prevlineno
    return -1

SEGFAULT = "addr(0)"

class Constant:
    def __init__(self, const):
        self.constant = const
    def __repr__(self):
        return f"{self.constant!r}"

class Name:
    def __init__(self, name):
        self.name = name
    def __repr__(self):
        return f"{self.name}"

binop_precedences = [
                          11, # '+'
                          9 , # '&'
                          12, # '//'
                          10, # '<<'
                          12, # '@'
                          12, # '*'
                          12, # '%'
                          7 , # '|'
                          14, # '**'
                          10, # '>>'
                          11, # '-'
                          12, # '/'
                          8 , # '^'
                    ]
inplace_start = len(binop_precedences)
binop_precedences += [-1] * len(binop_precedences) # inplace ops
ATOM_PRECEDENCE = 16
ASSIGNMENT_EXPR_PRECEDENCE = 0
OR_PRECEDENCE = 3
AND_PRECEDENCE = 4
COMPARISON_PRECEDENCE = 6

DEFAULT_NO_PRECEDENCE = -2

DEFAULT_OP = 0

class BinOp:
    def __init__(self, left, right, op=DEFAULT_OP,
                 precedence=DEFAULT_NO_PRECEDENCE,
                 has_spaces=True):
        # Very loose checking
        self.op = (binop_symbols[op]
                   if isinstance(op, int)
                   else op)
        self.precedence = (binop_precedences[op]
                           if precedence is DEFAULT_NO_PRECEDENCE
                           else precedence)
        self.op_raw = op
        boolean = False
        if (isinstance(left, checks)
                and left.precedence < self.precedence):
            self.left = f"({left!r})"
        else:
            self.left = (f"{left!r}", left)[(boolean:=isinstance(left, str))]
        if (isinstance(right, checks)
                and ((not right.precedence) and (boolean
                                                 or left.precedence >= 0)
                     or right.precedence < self.precedence)):
            self.right = f"({right!r})"
        else:
            self.right = f"{right!r}"
        self.has_spaces = has_spaces
    def __repr__(self):
        if self.has_spaces:
            return f"{self.left} {self.op} {self.right}"
        return f"{self.left}{self.op}{self.right}"

unop_symbols = [
    '-',
    '+',
    '~',
    'not ',
    'await ',
    'yield ',
    'yield from ',
]

NOT_PRECEDENCE = 5
PREFIX_UNARY_PRECEDENCE = 13
AWAIT_PRECEDENCE = 15
unop_precedence = (
                    [PREFIX_UNARY_PRECEDENCE] * 3
                  + [NOT_PRECEDENCE]   # 'not'
                  + [0] * 3
                  )

class UnOp:
    def __init__(self, value, op):
        self.op = unop_symbols[op]
        self.precedence = unop_precedence[op]
        if (isinstance(value, checks)
                and ((not value.precedence) and self.precedence >= 0
                     or value.precedence < self.precedence)):
            self.value = f"({value!r})"
        else:
            self.value = (f"{value!r}", value)[isinstance(value, str)]
    def __repr__(self):
        return f"{self.op}{self.value}"

class Subscript:
    def __init__(self, value, sub):
        if (isinstance(value, checks)
            and value.precedence < ATOM_PRECEDENCE):
            self.value = f"({value!r})"
        else:
            self.value = (f"{value!r}", value)[isinstance(value, str)]
        self.sub = sub
    def __repr__(self):
        return f"{self.value}[{self.sub!r}]"

class Iterator:
    def __init__(self, value, is_async):
        self.value = value
        self.is_async = is_async
    def __repr__(self):
        return f"{self.value!r}"

class Dictionary:
    def __init__(self, maps):
        self.maps = maps
    def __repr__(self):
        return f"{{{', '.join(self.maps)}}}"
    def __setitem__(self, key, value):
        self.maps.append(f"**{{{key!r}: {value!r}}}")
    def append(self, x):
        self.maps.append(x)

class Length:
    def __init__(self, value):
        self.value = value
    def __repr__(self):
        return f"len({self.value!r})"

class ForLoop:
    def __init__(self, iterable, assignments, block_string):
        self.iterable = iterable
        self.assignments = assignments
        self.block_string = block_string
    def __repr__(self):
        return (f"for {', '.join(map(repr, self.assignments))} in "
                f"{self.iterable}:\n{self.block_string}")

class Call:
    def __init__(self, func, vargs, kwargs={}):
        self.func = func
        self.vargs = vargs
        self.kwargs = kwargs
    def __repr__(self):
        kwarg_string = ''
        for key, value in self.kwargs.items():
            kwarg_string += f", {key}={value!r}"
        varg_string = ', '.join(map(repr, self.vargs))
        if not varg_string:
            kwarg_string = kwarg_string[2:]
        return f"{self.func}({varg_string}{kwarg_string})"

class FunctionDef:
    def __init__(self, func_orig, mode='exec',
                 name=None, indent=0, is_async=1):
        self.func = func_orig
        self.indent = indent
        self.mode = mode
        self.name = name
        self.precedence = 1
        self.is_async = not is_async
    def __repr__(self):
        if self.mode == 'eval':
            sigstr = f"{signature(self.func)!s}"[1:-1]
            return (f"lambda{f' {sigstr}' if sigstr else ''}: "
                    f"{decompile(self.func.__code__, mode='eval')}")
        block_string = decompile(self.func.__code__, indent=self.indent + 1)
        async_string = ("async ","")[self.is_async]
        return (f"{async_string}def {self.name}{signature(self.func)!s}:\n"
                f"{block_string or f'{self.indent + 1}pass'}")

checks = (BinOp, UnOp, FunctionDef)

class SliceExpr:
    def __init__(self, start='', stop='', step=''):
        self.start = (
            ''
            if isinstance(start, Constant) and start.constant is None
            else (f"{start!r}", start)[isinstance(start, str)]
        )
        self.stop = (
            ''
            if isinstance(stop, Constant) and stop.constant is None
            else (f"{stop!r}", stop)[isinstance(stop, str)]
        )
        self.step = (
            ''
            if isinstance(step, Constant) and step.constant is None
            else (f"{step!r}", step)[isinstance(step, str)]
        )
    def __repr__(self):
        if self.step:
            return f"{self.start}:{self.stop}:{self.step}"
        return f"{self.start}:{self.stop}"

class FString:
    def __init__(self, value, fmt=None, _boolean_0=True, _boolean_1=True):
        self.value = value
        self.fmt = fmt
        self.is_fmt_spec = _boolean_0
        self._boolean_1 = _boolean_1
    def __repr__(self):
        if self.fmt:
            mid = ':' if self.is_fmt_spec else '!'
            quote = "'" if self._boolean_1 else '"'
            return f"f{quote}{{{self.value!r}{mid}{self.fmt}}}{quote}"
        return f"f'{{{self.value!r}}}'"

class can_reverse:
    def __init__(self, value):
        self.value = value
        self.cur_value = DEFAULT_VALUE
    def __iter__(self):
        return self
    def __next__(self):
        if (ret := self.cur_value) is not DEFAULT_VALUE:
            self.cur_value = DEFAULT_VALUE
            return ret
        return next(self.value)

compare_symbols = ['<', '<=', '==', '!=', '>', '>=']

def _decompile(code_object, mode, indent,
               the_code=None, NEW_LINE=None, PUSH_N_POP=None):
    if NEW_LINE is None:
        lines = []
        NEW_LINE = lines.append
    co_consts = code_object.co_consts
    co_names = code_object.co_names
    co_varnames = code_object.co_varnames
    co_cellvars = code_object.co_cellvars
    co_freevars = code_object.co_freevars
    co_stacksize = code_object.co_stacksize
    localsplus = co_varnames + co_cellvars + co_freevars
    co_nlocals = code_object.co_nlocals
    if PUSH_N_POP is None:
        stack = []
        BASIC_PUSH = stack.append
        PUSH = lambda x: (BASIC_PUSH(x),
                          assert_func(len(stack) <= co_stacksize,
                          "assert(STACK_LEVEL() <= co->co_stacksize)"))[0]
        POP = stack.pop
    else:
        PUSH, POP = PUSH_N_POP
    skip = 0
    oldoparg = 0
    the_code = code_object.co_code if the_code is None else the_code
    iter_over = can_reverse(iter_unpack('BB', the_code))
    for bytecode_counter, (opcode, oparg) in enumerate(iter_over):
        if skip:
            skip -= 1
            continue
        if oldoparg:
            oparg |= oldoparg << 8
            oldoparg = 0
        if opcode is NOP:
            continue
        elif (opcode is LOAD_CLOSURE
              or opcode is LOAD_FAST
              or opcode is LOAD_DEREF):
            PUSH(Name(localsplus[oparg]))
        elif opcode is LOAD_CONST:
            PUSH(Constant(co_consts[oparg]))
        elif opcode is STORE_FAST or opcode is STORE_DEREF:
            value = POP()
            if isinstance(value, BinOp) and value.op_raw >= inplace_start:
                NEW_LINE(f"{value!r}")
            else:
                NEW_LINE(f"{localsplus[oparg]} = {value!r}")
        elif opcode is POP_TOP:
            value = POP()
            if not isinstance(value, str):
                NEW_LINE(f"{value!r}")
            else:
                NEW_LINE(value)
        elif opcode is ROT_TWO:
            stack[-1], stack[-2] = stack[-2], stack[-1]
        elif opcode is ROT_THREE:
            stack[-1], stack[-2], stack[-3] = stack[-2], stack[-3], stack[-1]
        elif opcode is ROT_FOUR:
            (stack[-1], stack[-2],
             stack[-3], stack[-4]) = (stack[-2], stack[-3],
                                      stack[-4], stack[-1])
        elif opcode is DUP_TOP:
            opcode, oparg = next(iter_over)
            target = ""
            assignments = []
            assignments_push = assignments.append
            if opcode is STORE_NAME or opcode is STORE_GLOBAL:
                target = co_names[oparg]
            elif opcode is STORE_FAST or opcode is STORE_DEREF:
                target = localsplus[oparg]
            else:
                PUSH(stack[-1])
                continue
            assignments_push(target)
            opcode, oparg = next(iter_over)
            while opcode is DUP_TOP:
                opcode, oparg = next(iter_over)
                if opcode is STORE_NAME or opcode is STORE_GLOBAL:
                    target = co_names[oparg]
                elif opcode is STORE_FAST or opcode is STORE_DEREF:
                    target = localsplus[oparg]
                assignments_push(target)
                opcode, oparg = next(iter_over)
            if opcode is STORE_NAME or opcode is STORE_GLOBAL:
                target = co_names[oparg]
                assignments_push(target)
            elif opcode is STORE_FAST or opcode is STORE_DEREF:
                target = localsplus[oparg]
                assignments_push(target)
            else:
                if not assignments and target:
                    PUSH(BinOp(target, POP(), ":=", 0))
                    continue
                assignments[:] = assignments[::-1]
                assignments_pop = assignments.pop
                target = assignments_pop()
                value = POP()
                while assignments:
                    value = BinOp(target, value, ":=", 0)
                    target = assignments_pop()
                PUSH(BinOp(target, value, ":=", 0))
                iter_over.cur_value = (opcode, oparg)
                continue
            if mode == 'eval':
                assignments[:] = assignments[::-1]
                assignments_pop = assignments.pop
                target = assignments_pop()
                value = POP()
                while assignments:
                    value = BinOp(target, value, ":=", 0)
                    target = assignments_pop()
                PUSH(BinOp(target, value, ":=", 0))
            else:
                assignments_pop = assignments.pop
                target = assignments_pop()
                value = POP()
                while assignments:
                    value = BinOp(target, value, "=", -1)
                    target = assignments_pop()
                NEW_LINE(f"{BinOp(target, value, '=', -1)!r}")
        elif opcode is DUP_TOP_TWO:
            PUSH(stack[-2])
            PUSH(stack[-1])
        elif opcode is UNARY_POSITIVE:
            stack[-1] = UnOp(stack[-1], 1)
        elif opcode is UNARY_NEGATIVE:
            stack[-1] = UnOp(stack[-1], 0)
        elif opcode is UNARY_NOT:
            stack[-1] = UnOp(stack[-1], 3)
        elif opcode is UNARY_INVERT:
            stack[-1] = UnOp(stack[-1], 2)
        elif opcode is BINARY_SUBSCR:
            sub = POP()
            stack[-1] = Subscript(stack[-1], sub)
        elif opcode is LIST_APPEND:
            v = POP()
            the_list = stack[-oparg]
            the_list.append(v)
        elif opcode is SET_ADD:
            v = POP()
            the_set = stack[-oparg]
            the_set.append(v)
        elif opcode is STORE_SUBSCR:
            sub = POP()
            container = POP()
            value = POP()
            if isinstance(value, BinOp) and value.op_raw >= inplace_start:
                NEW_LINE(f"{value!r}")
            else:
                NEW_LINE(f"{Subscript(container, sub)!r} = {value!r}")
        elif opcode is DELETE_SUBSCR:
            sub = POP()
            NEW_LINE(f"del {Subscript(POP(), sub)!r}")
        elif opcode is PRINT_EXPR:
            NEW_LINE(f"{POP()!r}")
        elif opcode is RAISE_VARARGS:
            cause = exc = None
            if not oparg:
                NEW_LINE("raise")
            elif oparg is 1:
                NEW_LINE(f"raise {POP()!r}")
            elif oparg is 2:
                cause = POP()
                NEW_LINE(f"raise {POP()!r} from {cause!r}")
            else:
                raise SystemError("bad RAISE_VARARGS oparg") from None
        elif opcode is RETURN_VALUE:
            assert_func(len(stack) is 1, "assert(EMPTY())")
            NEW_LINE(f"{'return '*(mode=='exec')}{POP()!r}")
        elif opcode is GET_AITER:
            stack[-1] = Iterator(stack[-1], True)
        elif opcode is GET_ANEXT:
            continue
        elif opcode is GET_AWAITABLE:
            PUSH(UnOp(POP(), 4))
            next(iter_over);next(iter_over)
        elif opcode is YIELD_FROM:
            PUSH(UnOp(POP(), 6))
        elif opcode is YIELD_VALUE:
            PUSH(UnOp(POP(), 5))
        elif opcode is GEN_START:
            continue
        elif opcode is POP_EXCEPT:
            stack[-3:] = []
            continue
        elif opcode is POP_EXCEPT_AND_RERAISE:
            stack[-4:] = []
            NEW_LINE("raise")
        elif opcode is RERAISE:
            stack[-3:] = []
            NEW_LINE("raise")
        elif opcode is END_ASYNC_FOR:
            stack[-3:] = []
        elif opcode is LOAD_ASSERTION_ERROR:
            PUSH(Constant(AssertionError))
        elif opcode is LOAD_BUILD_CLASS:
            PUSH(Constant(__build_class__))
        elif opcode is STORE_NAME or opcode is STORE_GLOBAL:
            value = POP()
            if isinstance(value, BinOp) and value.op_raw >= inplace_start:
                NEW_LINE(f"{value!r}")
            else:
                NEW_LINE(f"{co_names[oparg]} = {value!r}")
        elif opcode is DELETE_NAME or opcode is DELETE_GLOBAL:
            NEW_LINE(f"del {co_names[oparg]}")
        elif opcode is LOAD_NAME or opcode is LOAD_GLOBAL:
            PUSH(Name(co_names[oparg]))
        elif opcode is UNPACK_SEQUENCE:
            seq = POP()
            assert len(seq) == oparg, "PyObject_Length(TOP()) == oparg"
            stack.extend(seq)
        elif opcode is UNPACK_EX:
            raise NotImplementedError("UNPACK_EX")
            # totalargs = 1 + (oparg & 0xFF) + (oparg >> 8)
            # seq = POP()
            # stack.extend(seq)
        elif opcode is STORE_ATTR:
            attr = BinOp(POP(), Name(co_names[oparg]),
                         '.', ATOM_PRECEDENCE, False)
            value = POP()
            if isinstance(value, BinOp) and value.op_raw >= inplace_start:
                NEW_LINE(f"{value!r}")
            else:
                NEW_LINE(f"{attr!r} = {value!r}")
        elif opcode is DELETE_ATTR:
            attr = BinOp(POP(), Name(co_names[oparg]),
                         '.', ATOM_PRECEDENCE, False)
            NEW_LINE(f"del {attr!r}")
        elif opcode is DELETE_FAST or opcode is DELETE_DEREF:
            NEW_LINE(f"del {localsplus[oparg]}")
        elif opcode is MAKE_CELL:
            continue
        elif opcode is LOAD_CLASSDEREF:
            assert_func(
                oparg >= 0 and oparg < len(localsplus),
                "assert(oparg >= 0 and oparg < frame->f_code->co_nlocalsplus)",
            )
            PUSH(Name(localsplus[oparg]))
        elif opcode is COPY_FREE_VARS:
            if mode == 'eval':
                continue
            assert_func(oparg == len(co_freevars),
                   "assert(oparg == co->co_nfreevars)")
            offset = co_nlocals + len(co_cellvars)
            NEW_LINE(f"nonlocal {', '.join(localsplus[offset:offset + oparg])}")
        elif opcode is BUILD_STRING:
            to_string = stack[-oparg:]
            stack[-oparg:] = []
            PUSH(Constant(''.join(map(lambda x: f"{x!s}", to_string))))
        elif opcode is BUILD_TUPLE:
            tup = tuple(stack[-oparg:])
            stack[-oparg:] = []
            PUSH(tup)
        elif opcode is BUILD_LIST:
            the_list = list(stack[-oparg:])
            stack[-oparg:] = []
            PUSH(the_list)
        elif opcode is LIST_TO_TUPLE:
            PUSH(tuple(POP()))
        elif opcode is LIST_EXTEND:
            iterable = POP()
            stack[-oparg].append(f"*{iterable!r}")
        elif opcode is SET_UPDATE:
            iterable = POP()
            stack[-oparg].add(f"*{iterable!r}")
        elif opcode is BUILD_SET:
            the_set = set(stack[-oparg:])
            stack[-oparg:] = []
            PUSH(the_set)
        elif opcode is BUILD_MAP:
            PUSH(dict(zip(stack[-oparg::2], stack[-oparg+1::2])))
            stack[-oparg:] = []
        elif opcode is SETUP_ANNOTATIONS:
            continue
        elif opcode is BUILD_CONST_KEY_MAP:
            keys = stack[-1].constant
            if not isinstance(keys, tuple) or len(keys) != oparg:
                raise SystemError(
                    "bad BUILD_CONST_KEY_MAP keys argument"
                ) from None
            the_map = {keys[oparg - i]: stack[-i - 1]
                       for i in range(oparg, 0, -1)}
            POP()
            stack[-oparg:] = []
            PUSH(the_map)
        elif opcode is DICT_UPDATE:
            update = POP()
            dictionary = stack[-oparg]
            if isinstance(dictionary, Dictionary):
                dictionary.append(f"**{update!r}")
            else:
                stack[-oparg] = Dictionary(
                    [f"**{dictionary!r}", f"**{update!r}"]
                )
        elif opcode is DICT_MERGE:
            update = POP()
            dictionary = stack[-oparg]
            if isinstance(dictionary, Dictionary):
                dictionary.append(f"**{update!r}")
            else:
                stack[-oparg] = Dictionary(
                    [f"**{dictionary!r}", f"**{update!r}"]
                )
        elif opcode is MAP_ADD:
            key, value = stack[-2:]
            stack[-2:] = []
            stack[-oparg][key] = value
        elif opcode is LOAD_ATTR:
            stack[-1] = BinOp(stack[-1], Name(co_names[oparg]),
                              '.', ATOM_PRECEDENCE, False)
        elif opcode is COMPARE_OP:
            right = POP()
            stack[-1] = BinOp(stack[-1], right,
                              compare_symbols[oparg],
                              COMPARISON_PRECEDENCE)
        elif opcode is IS_OP:
            right = POP()
            stack[-1] = BinOp(stack[-1], right,
                              ('is', 'is not')[oparg],
                              COMPARISON_PRECEDENCE)
        elif opcode is CONTAINS_OP:
            right = POP()
            stack[-1] = BinOp(stack[-1], right,
                              ('in', 'not in')[oparg],
                              COMPARISON_PRECEDENCE)
        elif opcode is JUMP_IF_NOT_EXC_MATCH:
            raise NotImplementedError("JUMP_IF_NOT_EXC_MATCH")
        elif opcode is IMPORT_NAME:
            fromlist = POP().constant
            if fromlist:
                NEW_LINE(f"from {stack[-1]} import {', '.join(fromlist)}")
                if fromlist[0] != '*':
                    skip(2*len(fromlist))
            else:
                NEW_LINE(f"import {stack[-1]}")
        elif opcode is IMPORT_STAR:
            continue
        elif opcode is IMPORT_FROM:
            continue
        elif opcode is JUMP_FORWARD:
            raise NotImplementedError("JUMP_FORWARD")
        elif opcode is POP_JUMP_IF_FALSE:
            raise NotImplementedError("POP_JUMP_IF_FALSE")
        elif opcode is POP_JUMP_IF_TRUE:
            raise NotImplementedError("POP_JUMP_IF_TRUE")
        elif opcode is JUMP_IF_FALSE_OR_POP:
            raise NotImplementedError("JUMP_IF_FALSE_OR_POP")
        elif opcode is JUMP_IF_TRUE_OR_POP:
            raise NotImplementedError("JUMP_IF_TRUE_OR_POP")
        elif opcode is JUMP_ABSOLUTE:
            raise NotImplementedError("JUMP_ABSOLUTE")
        elif opcode is GET_LEN:
            PUSH(Length(stack[-1]))
        elif opcode is MATCH_CLASS:
            raise NotImplementedError("MATCH_CLASS")
        elif opcode is MATCH_MAPPING:
            PUSH(isinstance(stack[-1], Mapping))
        elif opcode is MATCH_SEQUENCE:
            PUSH(isinstance(stack[-1], Sequence))
        elif opcode is MATCH_KEYS:
            raise NotImplementedError("MATCH_KEYS")
        elif opcode is GET_ITER or opcode is GET_YIELD_FROM_ITER:
            stack[-1] = Iterator(stack[-1], False)
        elif opcode is FOR_ITER:
            iterable = POP()
            offset = oparg
            opcode, oparg = next(iter_over)
            assignments = []
            assignments_push = assignments.append
            bytecode_counter += 1
            if opcode is UNPACK_SEQUENCE:
                after = oparg
                cut_start = cut_end = 0
                for i in range(0, after):
                    opcode, oparg = next(iter_over)
                    if (opcode is UNPACK_EX
                            or not cut_end and opcode is UNPACK_SEQUENCE):
                        raise NotImplementedError("unsupported oparg after "
                                                  "FOR_ITER->UNPACK_SEQUENCE")
                    elif opcode is STORE_FAST or opcode is STORE_DEREF:
                        assignments_push(Name(localsplus[oparg]))
                    elif opcode is STORE_NAME or opcode is STORE_GLOBAL:
                        assignments_push(Name(co_names[oparg]))
                    elif opcode is STORE_ATTR:
                        if cut_end:
                            _decompile(
                                code_object, mode='eval', indent=0,
                                the_code=the_code[cut_start:cut_end],
                                NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                            )
                            attr = BinOp(POP(), Name(co_names[oparg]),
                                         '.', ATOM_PRECEDENCE, False)
                            assignments_push(attr)
                            cut_start = cut_end = 0
                        else:
                            raise SystemError("no l-value for STORE_ATTR")
                    elif opcode is STORE_SUBSCR:
                        if cut_end:
                            _decompile(
                                code_object, mode='eval', indent=0,
                                the_code=the_code[cut_start:cut_end],
                                NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                            )
                            sub = POP()
                            assignments_push(Subscript(POP(), sub))
                            cut_start = cut_end = 0
                        else:
                            raise SystemError("no values for STORE_SUBSCR")
                    else:
                        if cut_end:
                            cut_end += 1
                        else:
                            cut_start = cut_end = bytecode_counter + i
            else:
                cut_start = cut_end = i = 0
                done = False
                while not done:
                    if opcode is UNPACK_EX:
                        raise NotImplementedError("unsupported oparg after "
                                                  "FOR_ITER")
                    elif opcode is STORE_FAST or opcode is STORE_DEREF:
                        assignments.append(Name(localsplus[oparg]))
                        done = True
                    elif opcode is STORE_NAME or opcode is STORE_GLOBAL:
                        assignments_push(Name(co_names[oparg]))
                        done = True
                    elif opcode is STORE_ATTR:
                        if cut_end:
                            _decompile(
                                code_object, mode='eval', indent=0,
                                the_code=the_code[cut_start*2:cut_end*2],
                                NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                            )
                            attr = BinOp(POP(), Name(co_names[oparg]),
                                         '.', ATOM_PRECEDENCE, False)
                            assignments.append(attr)
                            cut_start = cut_end = 0
                        else:
                            raise SystemError("no l-value for STORE_ATTR")
                        done = True
                    elif opcode is STORE_SUBSCR:
                        if cut_end:
                            _decompile(
                                code_object, mode='eval', indent=0,
                                the_code=the_code[cut_start*2:cut_end*2],
                                NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                            )
                            sub = POP()
                            assignments.append(Subscript(POP(), sub))
                            cut_start = cut_end = 0
                        else:
                            raise SystemError("no values for STORE_SUBSCR")
                        done = True
                    else:
                        if cut_end:
                            cut_end += 1
                        else:
                            cut_start = cut_end = bytecode_counter + i
                    opcode, oparg = next(iter_over)
                    i += 1
                after = i
            jump_to = bytecode_counter + offset - 1
            bytecode_counter += after + 1
            block = _decompile(
                code_object, mode='exec', indent=indent+1,
                the_code=the_code[bytecode_counter*2:jump_to*2],
            ) or f"{(indent+1) * '    '}pass"
            skip = offset + i
            NEW_LINE(ForLoop(iterable, assignments, block))
        elif opcode is BEFORE_ASYNC_WITH:
            raise NotImplementedError("BEFORE_ASYNC_WITH")
            opcode = -1
            contexts = []
            contexts_push = contexts.append
            [*map(next, [iter_over]*3)]
            opcode, oparg = next(iter_over)
            assignments = []
            assignments_push = assignments.append
            bytecode_counter += 1
            while True:
                if opcode is UNPACK_SEQUENCE:
                    after = oparg
                    cut_start = cut_end = 0
                    for i in range(0, after):
                        opcode, oparg = next(iter_over)
                        if (opcode is UNPACK_EX
                                or not cut_end and opcode is UNPACK_SEQUENCE):
                            raise NotImplementedError(
                                "unsupported oparg after "
                                "BEFORE_ASYNC_WITH->UNPACK_SEQUENCE"
                            )
                        elif opcode is STORE_FAST or opcode is STORE_DEREF:
                            assignments_push(Name(localsplus[oparg]))
                        elif opcode is STORE_NAME or opcode is STORE_GLOBAL:
                            assignments_push(Name(co_names[oparg]))
                        elif opcode is STORE_ATTR:
                            if cut_end:
                                _decompile(
                                    code_object, mode='eval', indent=0,
                                    the_code=the_code[cut_start:cut_end],
                                    NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                                )
                                attr = BinOp(POP(), Name(co_names[oparg]),
                                             '.', ATOM_PRECEDENCE, False)
                                assignments_push(attr)
                                cut_start = cut_end = 0
                            else:
                                raise SystemError("no l-value for STORE_ATTR")
                        elif opcode is STORE_SUBSCR:
                            if cut_end:
                                _decompile(
                                    code_object, mode='eval', indent=0,
                                    the_code=the_code[cut_start:cut_end],
                                    NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                                )
                                sub = POP()
                                assignments_push(Subscript(POP(), sub))
                                cut_start = cut_end = 0
                            else:
                                raise SystemError("no values for STORE_SUBSCR")
                        else:
                            if cut_end:
                                cut_end += 1
                            else:
                                cut_start = cut_end = bytecode_counter + i
                else:
                    cut_start = cut_end = i = 0
                    done = False
                    while not done:
                        if opcode is UNPACK_EX:
                            raise NotImplementedError("unsupported oparg after"
                                                      " BEFORE_ASYNC_WITH")
                        elif opcode is STORE_FAST or opcode is STORE_DEREF:
                            assignments.append(Name(localsplus[oparg]))
                            done = True
                        elif opcode is STORE_NAME or opcode is STORE_GLOBAL:
                            assignments_push(Name(co_names[oparg]))
                            done = True
                        elif opcode is STORE_ATTR:
                            if cut_end:
                                _decompile(
                                    code_object, mode='eval', indent=0,
                                    the_code=the_code[cut_start*2:cut_end*2],
                                    NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                                )
                                attr = BinOp(POP(), Name(co_names[oparg]),
                                             '.', ATOM_PRECEDENCE, False)
                                assignments.append(attr)
                                cut_start = cut_end = 0
                            else:
                                raise SystemError("no l-value for STORE_ATTR")
                            done = True
                        elif opcode is STORE_SUBSCR:
                            if cut_end:
                                _decompile(
                                    code_object, mode='eval', indent=0,
                                    the_code=the_code[cut_start*2:cut_end*2],
                                    NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                                )
                                sub = POP()
                                assignments.append(Subscript(POP(), sub))
                                cut_start = cut_end = 0
                            else:
                                raise SystemError("no values for STORE_SUBSCR")
                            done = True
                        elif opcode is POP_TOP:
                            continue
                        else:
                            if cut_end:
                                cut_end += 1
                            else:
                                cut_start = cut_end = bytecode_counter + i
                        opcode, oparg = next(iter_over)
                        i += 1
                    after = i
                contexts_push(WithContext(POP(), assignments))
                opcode, oparg = next(iter_over)
                assignments = []
                assignments_push = assignments.append
                bytecode_counter += after + 1
                if opcode is not NOP:
                    bytecode_counter += 1
                    fin = (
                        bytecode_counter
                      + the_code[bytecode_counter*2:].find(BEFORE_ASYNC_WITH)
                    )
                    _decompile(
                        code_object, mode='eval', indent=0,
                        the_code=the_code[bytecode_counter*2:fin*2],
                        NEW_LINE=NEW_LINE, PUSH_N_POP=(PUSH, POP),
                    )
                    bytecode_counter = fin + 1
                    [*map(next,[iter_over]*4)]
                    opcode, oparg = next(iter_over)
                else:
                    break
        elif opcode is BEFORE_WITH:
            raise NotImplementedError("BEFORE_WITH")
        elif opcode is WITH_EXCEPT_START:
            raise NotImplementedError("WITH_EXCEPT_START")
        elif opcode is PUSH_EXC_INFO:
            raise NotImplementedError("PUSH_EXC_INFO")
        elif opcode is LOAD_METHOD:
            obj = stack[-1]
            stack[-1] = BinOp(obj, Name(co_names[oparg]),
                              '.', ATOM_PRECEDENCE, False)
        elif opcode is CALL_FUNCTION_KW or opcode is CALL_METHOD_KW:
            kwnames = POP().constant
            kwlength = len(kwnames)
            nargs = oparg - kwlength
            kwargs = stack[-kwlength:]
            stack[-kwlength:] = []
            vargs = stack[-nargs:]
            if nargs:
                vargs = stack[-nargs:]
                stack[-nargs:] = []
            else:
                vargs = []
            stack[-1] = Call(stack[-1], vargs, dict(zip(kwnames, kwargs)))
        elif opcode is CALL_FUNCTION or opcode is CALL_METHOD:
            args = stack[-oparg:]
            stack[-oparg:] = []
            stack[-1] = Call(stack[-1], args)
        elif opcode is CALL_FUNCTION_EX:
            kwargs = None
            if oparg & 1:
                kwargs = POP()
                if not type(kwargs) is dict:
                    d = {}
                    status, val = merge_override(d, kwargs, 2)
                    if status is -1:
                        if val is DEFAULT_VALUE:
                            raise SystemError("bad internal call")
                        raise TypeError(f"{stack[-2]!r} got multiple values "
                                        f"for keyword argument '{val}'")
                    kwargs = d
            callargs = POP()
            func = stack[-1]
            if not type(callargs) is tuple:
                if (not hasattr(callargs, "__iter__")
                        and not isinstance(callargs, Sequence)):
                    raise TypeError(f"{func!r} argument "
                                    "after * must be an iterable, not "
                                    f"'{type(callargs).__name__:.200s}'")
                callargs = tuple(callargs)
            vargs_string = ', '.join(callargs)
            kwargs_string = ', '.join(kwargs)
            if not vargs_string:
                stack[-1] = f"{func!r}({kwargs_string})"
            elif not kwargs_string:
                stack[-1] = f"{func!r}({vargs_string})"
            else:
                stack[-1] = f"{func!r}({vargs_string}, {kwargs_string})"
        elif opcode is MAKE_FUNCTION:
            codeobj = POP().constant
            if oparg & 8:
                assert_func(type(closure:=POP()) is tuple,
                       "assert(PyTuple_CheckExact(TOP()))")
                func = function(
                    codeobj,
                    globals(),
                    closure=tuple(map(cell, closure)),
                )
            else:
                func = function(codeobj, globals())
            if oparg & 4:
                assert_func(type(annotations:=POP()) is tuple,
                       "assert(PyTuple_CheckExact(TOP()))")
                func.__annotations__ = dict(zip(annotations[::2],
                                                annotations[1::2]))
            if oparg & 2:
                assert_func(type(kwdefaults:=POP()) is dict,
                       "assert(PyDict_CheckExact(TOP()))")
                func.__kwdefaults__ = kwdefaults
            if oparg & 1:
                defaults = POP()
                if type(defaults) is Constant:
                    defaults = defaults.constant
                assert_func(type(defaults) is tuple,
                       "assert(PyTuple_CheckExact(TOP()))")
                func.__defaults__ = defaults
            flags = codeobj.co_flags
            is_async = flags & 128 or flags & 256 or flags & 512
            opcode, oparg = next(iter_over)
            if opcode is STORE_FAST or opcode is STORE_DEREF:
                NEW_LINE(FunctionDef(func, 'exec',
                                     localsplus[oparg], indent, is_async))
            elif opcode is STORE_NAME or opcode is STORE_GLOBAL:
                NEW_LINE(FunctionDef(func, 'exec',
                                     co_names[oparg], indent, is_async))
            else:
                iter_over.cur_value = (opcode, oparg)
                PUSH(FunctionDef(func, 'eval', None, 0, is_async))
        elif opcode is BUILD_SLICE:
            if oparg is 3:
                step = POP()
            else:
                step = Constant(None)
            stop = POP()
            stack[-1] = SliceExpr(stack[-1], stop, step)
        elif opcode is FORMAT_VALUE:
            fmt_spec = POP().constant if (oparg & 4) is 4 else None
            value = stack[-1]
            conv_fn = ['','s','r','a'][oparg & 3]
            if fmt_spec:
                if (isinstance(value, Constant)
                        and isinstance(value.constant, str)):
                    firstchar = f"{value.constant!r}"[0]
                    if firstchar is "'":
                        stack[-1] = FString(value, fmt_spec, _boolean_1=False)
                    else:
                        stack[-1] = FString(value, fmt_spec)
                    continue
                stack[-1] = FString(value, fmt_spec, _boolean_0=True)
            elif conv_fn:
                if (isinstance(value, Constant)
                        and isinstance(value.constant, str)):
                    firstchar = f"{value.constant!r}"[0]
                    if firstchar is "'":
                        stack[-1] = FString(
                            value, conv_fn,
                            _boolean_0=False, _boolean_1=False
                        )
                    else:
                        stack[-1] = FString(value, conv_fn, _boolean_0=False)
                    continue
                stack[-1] = FString(value, conv_fn, _boolean_0=False)
            else:
                if not (isinstance(value, Constant)
                        and isinstance(value.constant, str)):
                    stack[-1] = FString(value)
        elif opcode is ROT_N:
            top = stack[-1]
            stack[:-oparg - 1] = stack[:-oparg] + [top]
        elif opcode is COPY:
            assert_func(oparg, "assert(oparg != 0)")
            PUSH(stack[-oparg])
        elif opcode is BINARY_OP:
            rhs = POP()
            assert_func(0 <= oparg, "assert(0 <= oparg)")
            assert_func(oparg < len(binop_symbols),
                   "assert((unsigned)oparg < Py_ARRAY_LENGTH(binary_ops))")
            assert_func(binop_symbols[oparg], "assert(binary_ops[oparg])")
            stack[-1] = BinOp(stack[-1], rhs, oparg)
        elif opcode is EXTENDED_ARG:
            oldoparg = oparg
            continue
        else:
            stderr.write(
                f"XXX lineno: {addr2line(code_object, bytecode_counter)}, "
                f"opcode: {opcode}\n",
            )
            raise SystemError("unknown opcode")
    if 'lines' in locals() and lines:
        indent_spaces = indent * "    "
        if (len(lines) is 1 and lines[0] == "return None"
                and indent):
            return f"{indent_spaces}pass"
        elif lines[-1] == "return None":
            lines = lines[:-1]
        return '\n'.join(map(lambda x:f"{indent_spaces}{x}", lines))
    else:
        return stack[0]

def decompile(code_object, mode='exec', indent=0):
    return if_error_then(
        "_decompile(code_object, mode, indent)",
        {
            #IndexError: SEGFAULT,
        },
        eval,
    )
