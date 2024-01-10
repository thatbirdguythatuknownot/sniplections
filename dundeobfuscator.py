import ast
import contextlib
import re
import renamer
import sys
import traceback

NO_VALUE = object()
VOID_VALUE = object()
MAX_REPR_THRESHOLD = 100

valid_dunder = re.compile(r"__(?!_)\w+(?<!_)__").fullmatch

class Evaluator(renamer.Renamer):
    marked_nosub = set()
    def __init__(self, partial_only=False, frame=None, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.unparse_map = {}
        self.unparse_node_map = {}
        self.name_to_node = {}
        self.definitions = {}
        frame = sys._getframe(1) if frame is None else frame
        for name in "__name__", "__builtins__", "__annotations__", "__spec__", "__loader__", "__package__", "__doc__":
            if name in frame.f_globals:
                self.set_definition(name, frame.f_globals[name])
        if not partial_only:
            self.get_name = super().get_name
    def get_definition(self, name, default=NO_VALUE):
        return self.definitions.get(name, default)
    def set_definition(self, name, value):
        self.definitions[name] = value
    def get_name(self, name):
        if name in self.name_map:
            return self.name_map[name]
        if not self.verifier(name):
            return None
        return name
    def get_repr(self, val):
        return ast.unparse(val) if isinstance(val, ast.AST) else repr(val)
    def get_noded(self, node):
        if isinstance(node, ast.Name):
            return None
        return self.unparse_node_map.get(ast.dump(node))
    def set_noded(self, old_node, new_node):
        self.unparse_node_map[ast.dump(old_node)] = new_node
        self.unparse_map[self.get_repr(old_node)] = self.get_repr(new_node)
        return new_node
    def add_name_nodes(self, name, *args, override=False):
        if not override and name in self.name_to_node:
            self.name_to_node[name] += args
            return
        self.name_to_node[name] = [*args]
    def maybe_change(*args, **kwargs):
        self = args[0]
        node = args[1]
        if len(args) == 3:
            kwargs |= args[2]
        to_delete = []
        for field, new_val in kwargs.items():
            if hasattr(node, field) and getattr(node, field) is new_val:
                to_delete.append(field)
        for field in to_delete:
            del kwargs[field]
        return self.set_noded(node, self.new_from(node, kwargs) if kwargs else node)
    def _handle_eval(self, node):
        node_repr = self.get_repr(node)
        if node_repr not in self.marked_nosub:
            if (
                (val := self.safe_eval_check(node)) is not NO_VALUE
                and val is not VOID_VALUE
            ):
                if len(val_repr := repr(val)) <= MAX_REPR_THRESHOLD:
                    self.unparse_map[node_repr] = val_repr
                    node_dump = ast.dump(node)
                    node = ast.copy_location(ast.Constant(value=val), node)
                    self.unparse_node_map[node_dump] = node
                    return node
                self.marked_nosub.add(node_repr)
        return None
    def handle_eval(self, node):
        res = self._handle_eval(node)
        if res is None:
            res = self.set_noded(node, self.generic_visit(node))
        return res
    @contextlib.contextmanager
    def wrap_try(_, catch=BaseException, print_exc=True):
        try:
            yield
        except catch:
            if print_exc:
                traceback.print_exc()
    @contextlib.contextmanager
    def freeze_defs(self):
        definitions = {}
        old_get = self.get_definition
        old_set = self.set_definition
        def new_get(name, default=NO_VALUE):
            if (val := definitions.get(name, NO_VALUE)) is not NO_VALUE:
                return val
            return old_get(name, default)
        def new_set(name, value):
            definitions[name] = value
        self.get_definition = new_get
        self.set_definition = new_set
        yield definitions
        self.get_definition = old_get
        self.set_definition = old_set
    def visit(self, node):
        if unparsed := self.get_noded(node):
            return unparsed
        method = "visit_" + node.__class__.__name__
        supervisitor = getattr(super(), method, None)
        if supervisitor is not None:
            node = supervisitor(node)
        visitor = getattr(self, method, None)
        if visitor is not None:
            return visitor(node)
        return self.set_noded(node, self.generic_visit(node))
    def visit_Name(self, node):
        name = node.id
        if isinstance(node.ctx, ast.Load):
            node = self.handle_eval(node)
        else:
            old_node = node
            if isinstance(node.ctx, ast.Store):
                new_name = super().get_name(name := node.id)
                if new_name is not None and new_name != name:
                    node = self.new_from(node, id=new_name)
            self.set_noded(old_node, node)
        return self.generic_visit(node)
    def visit_NamedExpr(self, node):
        targ = self.visit(node.target)
        val = self.visit(node.value)
        set_name = False
        if (
            (eval_val := self.safe_eval_check(val)) is not NO_VALUE
            and eval_val is not VOID_VALUE
        ):
            set_name = True
            self.set_definition(targ.id, eval_val)
            val = ast.copy_location(ast.Constant(value=eval_val), node)
        else:
            self.set_definition(targ.id, VOID_VALUE)
        node = self.maybe_change(node, target=targ, value=val)
        if set_name:
            self.add_name_nodes(targ.id, node, override=True)
        return node
    def visit_Call(self, node):
        res = self._handle_eval(node)
        if res is not None:
            return res
        func = node.func
        res = node
        if (
            isinstance(func, ast.Attribute)
            and not node.keywords and len(node.args) == 1
        ):
            if (
                func.attr == "__getattribute__"
                and (val := self.safe_eval(node.args[0])) is not NO_VALUE
                and isinstance(val, str)
            ):
                res = ast.copy_location(ast.Attribute(
                    value=func.value,
                    attr=val,
                    ctx=func.ctx,
                ), node)
                res = self.visit(res)
            elif func.attr == "__getitem__":
                res = ast.copy_location(ast.Subscript(
                    value=func.value,
                    slice=node.args[0],
                    ctx=func.ctx,
                ), node)
                res = self.visit(res)
        return self.set_noded(node, self.generic_visit(res))
    visit_BinOp = visit_UnaryOp = handle_eval
    def visit_IfExp(self, node):
        test = self.visit(node.test)
        if (valtest := self.safe_eval(test)) is not NO_VALUE:
            res = ast.copy_location(node.body if valtest else node.orelse, node)
            res = self.visit(res)
        else:
            with self.freeze_defs() as body_defs: #, body_nn):
                body = self.visit(node.body)
            with self.freeze_defs() as orelse_defs: #, orelse_nn):
                orelse = self.visit(node.orelse)
            if body_defs or orelse_defs:
                for name, value in body_defs.items():
                    if name in orelse_defs:
                        if orelse_defs.pop(name) == value:
                            self.set_definition(name, value)
                            # self.add_name_nodes(name, body_nn[name], orelse_nn[name])
                            continue
                    self.set_definition(name, VOID_VALUE)
                for name in orelse_defs:
                    self.set_definition(name, VOID_VALUE)
            res = node
        return self.set_noded(node, self.generic_visit(res))
    def safe_eval(self, node):
        return getattr(self, "safe_eval_" + node.__class__.__name__,
                       self.generic_safe_eval)(node)
    def safe_eval_check(self, node):
        exp = self.safe_eval(node)
        if exp is not NO_VALUE:
            with self.wrap_try(catch=SyntaxError, print_exc=False):
                compile(repr(exp), "<test>", "exec")
                return exp
        return NO_VALUE
    generic_safe_eval = lambda _, __: NO_VALUE
    def safe_eval_list(self, list_):
        res = []
        for elem in list_:
            if (elem := self.safe_eval(elem)) is NO_VALUE:
                return NO_VALUE
            res.append(elem)
        return res
    def safe_eval_Name(self, node):
        assert isinstance(node.ctx, ast.Load)
        if (
            (val := self.get_definition(node.id)) is not NO_VALUE
            and val is not VOID_VALUE
        ):
            return self.safe_eval(val) if isinstance(val, ast.AST) else val
        return NO_VALUE
    def safe_eval_Constant(self, node):
        return node.value
    def safe_eval_Slice(self, node):
        lower = None
        if node.lower and (lower := self.safe_eval(node.lower)) is NO_VALUE:
            return NO_VALUE
        upper = None
        if node.upper and (upper := self.safe_eval(node.upper)) is NO_VALUE:
            return NO_VALUE
        step = None
        if node.step and (step := self.safe_eval(node.step)) is NO_VALUE:
            return NO_VALUE
        return slice(lower, upper, step)
    def safe_eval_Attribute(self, node):
        assert isinstance(node.ctx, ast.Load)
        if (val := self.safe_eval(node.value)) is NO_VALUE:
            return NO_VALUE
        return getattr(val, node.attr, NO_VALUE)
    def safe_eval_Subscript(self, node):
        assert isinstance(node.ctx, ast.Load)
        if (val := self.safe_eval(node.value)) is NO_VALUE:
            return NO_VALUE
        if (item := self.safe_eval(node.slice)) is NO_VALUE:
            return NO_VALUE
        with self.wrap_try():
            return val[item]
        return NO_VALUE
    def safe_eval_Call(self, node):
        if (
            not isinstance(node.func, ast.Attribute) or
            not valid_dunder(node.func.attr)
        ):
            return NO_VALUE
        if (func := self.safe_eval(node.func)) is NO_VALUE:
            return NO_VALUE
        if not node.keywords and len(node.args) < 3:
            if (args := self.safe_eval(node.args)) is not NO_VALUE:
                with self.wrap_try():
                    return func(*args)
        return NO_VALUE
    def safe_eval_BinOp(self, node):
        if (left := self.safe_eval(node.left)) is NO_VALUE:
            return NO_VALUE
        if (right := self.safe_eval(node.right)) is NO_VALUE:
            return NO_VALUE
        with self.wrap_try():
            match node.op:
                case ast.Add():
                    return left + right
                case ast.Sub():
                    return left - right
                case ast.Mult():
                    return left * right
                case ast.MatMult():
                    return left @ right
                case ast.Div():
                    return left / right
                case ast.FloorDiv():
                    return left // right
                case ast.Mod():
                    return left % right
                case ast.Pow():
                    return left ** right
                case ast.LShift():
                    return left << right
                case ast.RShift():
                    return left >> right
                case ast.BitAnd():
                    return left & right
                case ast.BitXor():
                    return left ^ right
                case ast.BitOr():
                    return left | right
        return NO_VALUE
    def safe_eval_UnaryOp(self, node):
        if (operand := self.safe_eval(node.operand)) is NO_VALUE:
            return NO_VALUE
        with self.wrap_try():
            match node.op:
                case ast.Invert():
                    return ~operand
                case ast.UAdd():
                    return +operand
                case ast.USub():
                    return -operand
                case ast.Not():
                    return not operand

class FullEvaluator(Evaluator):
    visit_BoolOp = Evaluator.handle_eval
    def safe_eval_Tuple(self, node):
        assert isinstance(node.ctx, ast.Load)
        if (elts := self.safe_eval_list(node.elts)) is not NO_VALUE:
            return (*elts,)
        return NO_VALUE
    def safe_eval_List(self, node):
        assert isinstance(node.ctx, ast.Load)
        return self.safe_eval_list(node.elts)
    def safe_eval_BoolOp(self, node):
        it = iter(node.values)
        is_or = isinstance(node.op, ast.Or)
        value = not is_or
        for item in node.values:
            if bool(value) ^ is_or:
                value = self.safe_eval(item)
                if value is NO_VALUE:
                    return NO_VALUE
        return value

def deobfuscate(source, evaluator=Evaluator, raw=False, *args, _depth=1, **kwargs):
    tree = source if isinstance(source, ast.AST) else ast.parse(source)
    transformed = evaluator(*args, frame=sys._getframe(_depth), **kwargs).visit(tree)
    ast.fix_missing_locations(transformed)
    return transformed if raw else ast.unparse(transformed)
