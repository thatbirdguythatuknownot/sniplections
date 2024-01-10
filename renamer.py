import ast

def namegen_from_param(
    GEN_SEQ="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
    verifier = lambda *_, **__: True,
):
    assert GEN_SEQ
    LEN_SEQ = len(GEN_SEQ)
    idx_stack = [0]
    def generate():
        name = "".join(GEN_SEQ[idx] for idx in idx_stack)
        i = len(idx_stack) - 1
        while i >= 0:
            idx_stack[i] += 1
            if idx_stack[i] >= LEN_SEQ:
                idx_stack[i] = 0
            else:
                break
        else:
            idx_stack.append(0)
        return name
    def next_valid(*args, **kwargs):
        while not verifier(name := generate(), *args, **kwargs): pass
        return name
    return next_valid

class Renamer(ast.NodeTransformer):    
    def __init__(self, verifier=None, generator=None, taken=None):
        self.name_map = {}
        self.taken = taken = taken if taken is not None else set()
        if verifier is not None:
            self.verifier = verifier
        if generator is not None:
            self.generator = generator
        else:
            self.generator = namegen_from_param(
                verifier = lambda name, _:
                    name.isidentifier() and name not in taken
            )
    verifier = lambda _, name: all(map('_'.__eq__, name))
    def get_name(self, name):
        if name in self.name_map:
            return self.name_map[name]
        if not self.verifier(name):
            return None
        new_name = self.name_map[name] = self.generator(name)
        return new_name
    def copy_missing(_, old_node, new_node, copy_loc=True):
        for field in old_node._fields:
            if not hasattr(new_node, field) and hasattr(old_node, field):
                setattr(new_node, field, getattr(old_node, field))
        if copy_loc:
            return ast.copy_location(new_node, old_node)
        return new_node
    def new_from(*args, **kwargs):
        self = args[0]
        old_node = args[1]
        if len(args) == 3:
            kwargs |= args[2]
        return self.copy_missing(old_node, type(old_node)(**kwargs))
    def generic_visit(self, node):
        kwargs = {}
        deleted_fields = set()
        for field, value in ast.iter_fields(node):
            new_value = value
            if isinstance(value, list):
                copied = []
                for item in value:
                    new_item = item
                    if isinstance(item, ast.AST):
                        new_item = self.visit(item)
                        if new_item is None:
                            continue
                        elif not isinstance(new_item, ast.AST):
                            copied.extend(new_item)
                            continue
                    copied.append(new_item)
                if copied != value:
                    new_value = copied
            elif isinstance(value, ast.AST):
                new_value = self.visit(value)
            if new_value is value:
                continue
            if new_value is None:
                deleted_fields.add(field)
            else:
                kwargs[field] = new_value
        if deleted_fields or kwargs:
            node = self.new_from(node, kwargs)
            for field in deleted_fields:
                delattr(node, field)
        return node
    def visit_Name(self, node):
        if (name := self.get_name(node.id)) is not None:
            node = self.new_from(node, id=name)
        return self.generic_visit(node)
    def visit_ClassDef(self, node):
        if (name := self.get_name(node.name)) is not None:
            node = self.new_from(node, name=name)
        return self.generic_visit(node)
    def visit_Attribute(self, node):
        if (name := self.get_name(node.attr)) is not None:
            node = self.new_from(node, attr=name)
        return self.generic_visit(node)

def rename(source, renamer=Renamer, *args, **kwargs):
    tree = ast.parse(source)
    transformed = renamer(*args, **kwargs).visit(tree)
    ast.fix_missing_locations(transformed)
    return ast.unparse(transformed)
