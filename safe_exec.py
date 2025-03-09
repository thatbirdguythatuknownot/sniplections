import ast
import builtins
import math
import re
import traceback

ALLOWED_TYPES = {
    ast.Module, ast.Delete, ast.Assign, ast.AugAssign, ast.If, ast.Expr,
    ast.Pass, ast.BoolOp, ast.NamedExpr, ast.BinOp, ast.UnaryOp, ast.Lambda,
    ast.IfExp, ast.Set, ast.ListComp, ast.SetComp, ast.Compare, ast.Call,
    ast.FormattedValue, ast.JoinedStr, ast.Constant, ast.Subscript,
    ast.Starred, ast.Name, ast.List, ast.Tuple, ast.Slice,
}

ALLOWED_ISINST = (
    ast.expr_context, ast.boolop, ast.operator, ast.unaryop, ast.cmpop,
    ast.comprehension, ast.arguments, ast.arg, ast.keyword,
)

_OPEN_BRACKETS = "([{"
_CLOSE_BRACKETS = ")]}"
OPEN_BRACKETS = dict(zip(_OPEN_BRACKETS, _CLOSE_BRACKETS))
CLOSE_BRACKETS = {*_CLOSE_BRACKETS}
BRACKETS_PAT = re.compile(f"[{re.escape(
    _OPEN_BRACKETS + _CLOSE_BRACKETS
)}]")

class CheckAbort(Exception):
    __slots__ = "node",
    def __init__(self, node):
        self.node = node

class ASTChecker(ast.NodeVisitor):
    def __init__(self, src):
        self.src = src
    def check(self):
        tree = ast.parse(self.src)
        try:
            self.visit(tree)
        except CheckAbort as e:
            node = e.node
            slno = node.lineno
            elno = node.end_lineno
            scol = node.col_offset
            ecol = node.end_col_offset
            lines = self.src.splitlines(keepends=True)
            sline = lines[slno - 1]
            if slno == elno:
                linfo = f"lineno {slno}"
                cinfo = f"col {scol + 1}-{ecol - 1}"
                info = f"{linfo}, {cinfo}"
                joined = sline[scol : ecol]
            else:
                sinfo = f"lineno {slno}, col {scol + 1}"
                einfo = f"lineno {elno}, col {ecol - 1}"
                info = f"{sinfo} -- {einfo}"
                sline = sline[scol:]
                eline = lines[elno - 1][:ecol]
                joined = (
                    sline
                    + "".join(lines[slno : elno - 1])
                    + eline
                )
            print(
                "Invalid expression!\n"
                f"Offending part ({info}):\n"
            )
            print(joined, end="\n\n")
            return None
        else:
            return tree
    def generic_visit(self, node):
        if type(node) not in ALLOWED_TYPES:
            for typ in ALLOWED_ISINST:
                if isinstance(node, typ):
                    break
            else:
                raise CheckAbort(node)
        super().generic_visit(node)
    def visit_Attribute(self, node):
        if node.attr.startswith('_'):
            raise CheckAbort(node)
        super().visit(node.value)

def complete(string):
    # TODO: fully complete unopened/unclosed brackets
    full = ""
    stack = []
    while True:
        full += string
        for m in BRACKETS_PAT.finditer(string):
            if pair := OPEN_BRACKETS.get(char := m[0]):
                stack.append(pair)
                continue
            try:
                if stack.pop() != char:
                    raise IndexError
            except IndexError:
                return full
        if not stack:
            break
        try:
            string = input("...> ")
        except EOFError:
            full += "".join(reversed(stack))
            break
        except KeyboardInterrupt:
            return None
    return full

if __name__ == "__main__":
    bins = {}

    for name in (
        "abs", "any", "all", "complex", "float", "int", "len", "list",
        "min", "map", "max", "pow", "set", "sorted", "sum", "tuple",
    ):
        bins[name] = getattr(builtins, name)

    for name in dir(math):
        if not name.startswith('_'):
            bins[name] = getattr(math, name)

    glob = {}

    print(
r"""\
Calculator!
Use the variable '_' to get the previous answer result.
To define functions, use the following syntax:

f = lambda x: 2*x + 3

This is equivalent to `f(x) = 2x + 3`.
"""
    )

    while True:
        glob["__builtins__"] = bins.copy()
        try:
            expression = complete(input("calc> "))
            if expression is None:
                continue
            if ASTChecker(expression).check():
                try:
                    result = eval(expression, glob)
                except SyntaxError:
                    exec(expression, glob)
                else:
                    glob['_'] = result
                    print(result)
        except SyntaxError as e:
            traceback.print_exception(e, limit=0)
        except EOFError:
            pass
        except KeyboardInterrupt:
            print("\n\nAdios!")
            raise SystemExit(0) from None
        except Exception as e:
            t = e.__traceback__
            while n := t.tb_next:
                t = n
            b = e
            traceback.print_exception(e.with_traceback(t))
