from ast import *
from ast import _Unparser, _Precedence, _SINGLE_QUOTES, _MULTI_QUOTES, _ALL_QUOTES
from gc import get_referrers
from itertools import product

names=([*'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ']
      + [*map(''.join, product('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ',
                               'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'))])
set_of_names = {*names}

conv_funcs = {97: ascii, 114: repr, 115: str}

class EqualWrapper:
    def __init__(self, val):
        self.val = val
    def __eq__(self, other):
        return dump(self.val) == dump(other)

class HasChanging(NodeVisitor):
    def __init__(self):
        self.has_changing = False
    def visit(self, node: AST):
        super().visit(node)
        return self.has_changing
    def visit_NamedExpr(self, node: NamedExpr):
        self.has_changing = True
    def visit_Call(self, node: Call):
        self.has_changing = True
    def visit_Subscript(self, node: Subscript):
        self.has_changing = True
    def visit_Starred(self, node: Starred):
        self.has_changing = type(node.ctx) is Load
    def visit_BinOp(self, node: BinOp):
        self.has_changing = True
    def visit_UnaryOp(self, node: UnaryOp):
        self.has_changing = True
    def visit_ListComp(self, node: ListComp):
        self.has_changing = True
    def visit_SetComp(self, node: SetComp):
        self.has_changing = True
    def visit_DictComp(self, node: DictComp):
        self.has_changing = True
    def visit_List(self, node: List):
        if type(node.ctx) is Load:
            for x in node.elts:
                if self.visit(x):
                    return
    def visit_Tuple(self, node: Tuple):
        if type(node.ctx) is Load:
            for x in node.elts:
                if self.visit(x):
                    return
    def visit_Set(self, node: Set):
        for x in node.elts:
            if self.visit(x):
                return
    def visit_Dict(self, node: Dict):
        for x in node.keys:
            if x is None:
                self.has_changing = True
                return
            if self.visit(x):
                return
        for x in node.values:
            if self.visit(x):
                return

class RewriteExpression(NodeTransformer):
    def __init__(self, namespace: dict = {}, use_map: bool = False):
        self.namespace = namespace
        self.use_map = use_map
    def visit_Name(self, node: Name) -> Name:
        return Name(self.namespace.get(node.id, node.id), ctx=node.ctx)
    def visit_NamedExpr(self, node: NamedExpr) -> NamedExpr | Name:
        if node.target.id == node.value.id:
            return node.value
        return node
    def visit_JoinedStr(self, node: JoinedStr) -> JoinedStr | Constant:
        values = node.values
        for i, val in enumerate(values):
            if isinstance(val, FormattedValue):
                val.value = self.visit(val.value)
                if isinstance(val.value, Constant):
                    res = conv_funcs.get(val.conversion, lambda x: x)(val.value.value)
                    if type(res) is not str or val.format_spec is not None:
                        res = res.__format__(val.format_spec or '')
                    values[i] = Constant(res)
        i = 0
        while i < (length := len(values)):
            if isinstance(cur := values[i], Constant) and i + 1 < length and isinstance(nex := values[i + 1], Constant):
                values[i:i+2] = [Constant(f"{cur.value}{nex.value}")]
            else:
                i += 1
        if len(values) == 1 and isinstance(values[0], Constant):
            return values[0]
        return node
    def visit_ListComp(self, node: ListComp) -> ListComp | List:
        node.elt = self.visit(node.elt)
        for i, x in enumerate(gens := node.generators):
            gens[i] = self.visit(x)
        if len(node.generators) == 1:
            comp = node.generators[0]
            if not comp.ifs and not comp.is_async:
                tg = comp.target
                if isinstance(el := node.elt, Name) and isinstance(tg, Name) and tg.id == el.id:
                    if isinstance(it := comp.iter, List):
                        return self.visit(it)
                    elif isinstance(it, Tuple):
                        return List(elts=[*map(self.visit, it.elts)], ctx=it.ctx)
                    elif isinstance(it, Dict):
                        return List(elts=[*map(self.visit, it.keys)], ctx=Load())
                    return List(
                        elts=[
                            Starred(
                                value=self.visit(it),
                                ctx=Load()
                            )
                        ],
                        ctx=Load()
                    )
                elif (self.use_map and isinstance(el, Call) and not el.keywords and len(el.args) == 1
                          and isinstance(nm := el.args[0], Name) and isinstance(tg, Name) and nm.id != el.func.id
                          and nm.id == tg.id):
                    return List(
                        elts=[
                            Starred(
                                value=Call(
                                    func=Name(id='map', ctx=Load()),
                                    args=[
                                        el.func,
                                        comp.iter
                                    ],
                                    keywords=[]
                                ),
                                ctx=Load()
                            )
                        ],
                        ctx=Load()
                    )
        return node
    def visit_SetComp(self, node: SetComp) -> SetComp | Set:
        node.elt = self.visit(node.elt)
        for i, x in (gens := node.generators):
            gens[i] = self.visit(x)
        if len(node.generators) == 1:
            comp = node.generators[0]
            if not comp.ifs and not comp.is_async:
                if isinstance(node.elt, Name) and isinstance(comp.target, Name) and comp.target.id == node.elt.id:
                    if isinstance(it := comp.iter, Set):
                        return self.visit(it)
                    elif isinstance(it, (List, Tuple)):
                        return Set(elts=[*map(self.visit, it.elts)])
                    elif isinstance(it, Dict):
                        return Set(elts=[*map(self.visit, it.keys)])
                    return Set(
                        elts=[
                            Starred(
                                value=it,
                                ctx=Load()
                            )
                        ],
                        ctx=Load()
                    )
                elif (self.use_map and isinstance(el, Call) and not el.keywords and len(el.args) == 1
                          and isinstance(nm := el.args[0], Name) and isinstance(tg, Name) and nm.id != el.func.id
                          and nm.id == tg.id):
                    return Set(
                        elts=[
                            Starred(
                                value=Call(
                                    func=Name(id='map', ctx=Load()),
                                    args=[
                                        el.func,
                                        comp.iter
                                    ],
                                    keywords=[]
                                ),
                                ctx=Load()
                            )
                        ]
                    )
        return node
    def visit_BinOp(self, node: BinOp) -> AST:
        node.left = left = self.visit(node.left)
        node.right = right = self.visit(node.right)
        if (ltyp := type(left)) is (rtyp := type(right)):
            if ltyp is Constant:
                return Constant(eval(unparse(node)), ctx=Load())
            elif ltyp is List and (otyp := type(node.op)) is Add:
                return self.visit(List(elts=left.elts + right.elts, ctx=Load()))
            elif ltyp is Tuple and otyp is Add:
                return self.visit(Tuple(elts=left.elts + right.elts, ctx=Load()))
            elif ltyp is Set and otyp is BitOr:
                return self.visit(Set(elts=left.elts + right.elts))
        return node
    def visit_List(self, node: List) -> List:
        for i, x in enumerate(elts := node.elts):
            elts[i] = memb = self.visit(x)
            if isinstance(memb, Starred):
                if isinstance(val := memb.value, (List, Tuple)):
                    elts[i:i+1] = val.elts
                elif isinstance(val, Dict):
                    elts[i:i+1] = val.keys
        if len(elts) == 1 and type(el := elts[0]) is Starred:
            if (etyp := type(elv := el.value)) is ListComp:
                return elv
            elif etyp is GeneratorExp:
                return ListComp(elt=elv.elt, generators=elv.generators)
        return node
    def visit_Tuple(self, node: Tuple) -> Tuple:
        for i, x in enumerate(elts := node.elts):
            elts[i] = memb = self.visit(x)
            if isinstance(memb, Starred):
                if isinstance(val := memb.value, (List, Tuple)):
                    elts[i:i+1] = val.elts
                elif isinstance(val, Dict):
                    elts[i:i+1] = val.keys
        return node
    def visit_Set(self, node: Set) -> Set:
        for i, x in enumerate(elts := node.elts):
            elts[i] = memb = self.visit(x)
            if isinstance(memb, Starred):
                if isinstance(val := memb.value, (List, Tuple, Set)):
                    elts[i:i+1] = val.elts
                elif isinstance(val, Dict):
                    elts[i:i+1] = val.keys
        i = 0
        while i < len(elts):
            x = EqualWrapper(elts[i])
            if (count := elts.count(x)) > 1:
                elts.remove(x)
                while (count := elts.count(x)) > 1:
                    elts.remove(x)
            else:
                i += 1
        if not elts:
            elts.append(
                Starred(
                    value=Tuple(
                        elts=[],
                        ctx=Load()
                    ),
                    ctx=Load()
                )
            )
        elif len(elts) == 1 and type(el := elts[0]) is Starred:
            if (etyp := type(elv := el.value)) is SetComp:
                return elv
            elif etyp in {ListComp, GeneratorExp}:
                return SetComp(elt=elv.elt, generators=elv.generators)
        return node
    def visit_Dict(self, node: Dict) -> Dict:
        vals = node.values
        for i, x in enumerate(keys := node.keys):
            keys[i] = key = self.visit(x)
            vals[i] = val = self.visit(vals[i])
            if key is None and isinstance(val, Dict):
                keys[i:i+1] = val.keys
                vals[i:i+1] = val.values
        i = 0
        while i < len(keys):
            x = EqualWrapper(keys[i])
            if x is not None and (count := keys.count(x)) > 1:
                del keys[ind := keys.index(x)]
                del values[ind]
                while (count := keys.count(x)) > 1:
                    del keys[ind := keys.index(x)]
                    del values[ind]
            else:
                i += 1
        if len(keys) == 1 and keys[0] is None:
            if type(val := values[0]) is DictComp:
                return val
        return node
    def visit_BoolOp(self, node: BoolOp) -> AST:
        prev_haschange = False
        if (otyp := type(node.op)) is And:
            i = 0
            while i < (length := len(vals := node.values)):
                vals[i] = x = self.visit(vals[i])
                if (xtyp := type(x)) is Constant and not prev_haschange:
                    if x.value:
                        if length > 1:
                            del vals[i]
                            continue
                    else:
                        del vals[i+1:]
                        break
                elif HasChanging().visit(x):
                    prev_haschange = True
                elif prev_haschange:
                    prev_haschange = False
                else:
                    if xtyp in (List, Tuple, Set):
                        if x.elts:
                            if length > 1:
                                del vals[i]
                                continue
                        else:
                            del vals[i+1:]
                            break
                    elif xtyp is Dict:
                        if x.keys:
                            if length > 1:
                                del vals[i]
                                continue
                        else:
                            del vals[i+1:]
                            break
                i += 1
        else:
            i = 0
            while i < (length := len(vals := node.values)):
                if i + 1 == length:
                    break
                vals[i] = x = self.visit(vals[i])
                if (xtyp := type(x)) is Constant and not prev_haschange:
                    if x.value:
                        del vals[i+1:]
                        break
                    else:
                        del vals[i]
                        continue
                elif HasChanging().visit(x):
                    prev_haschange = True
                elif prev_haschange:
                    prev_haschange = False
                else:
                    if xtyp in {List, Tuple, Set}:
                        if x.elts:
                            del vals[i+1:]
                            break
                        elif length > 1:
                            del vals[i]
                            continue
                    elif xtyp is Dict:
                        if x.keys:
                            del vals[i+1:]
                            break
                        elif length > 1:
                            del vals[i]
                            continue
                i += 1
        if (length := len(node.values)) == 1:
            return node.values[0]
        return node
    def visit_IfExp(self, node: IfExp) -> AST:
        node.test = test = self.visit(node.test)
        node.body = left = self.visit(node.body)
        node.orelse = right = self.visit(node.orelse)
        if (ttyp := type(test)) is Constant:
            return (left, right)[not test.value]
        elif not HasChanging().visit(test):
            if ttyp in {List, Tuple, Set}:
                return (left, right)[not test.elts]
            elif ttyp is Dict:
                return (left, right)[not test.keys]
        return node
    def visit_Lambda(self, node: Lambda) -> Lambda:
        node_arguments = node.args
        posonlyargs = []
        args = []
        kwonlyargs = []
        d = {}
        pargs, nargs, kargs = node_arguments.posonlyargs, node_arguments.args, node_arguments.kwonlyargs
        names_set = set_of_names - (GetNames().visit(node)
                                   - set([x.arg for x in pargs])
                                   - set([x.arg for x in nargs])
                                   - set([x.arg for x in kargs]))
        i = 0
        for x in pargs:
            while (name := names[i]) in names_set:
                i += 1
            d[x.arg] = name
            posonlyargs.append(arg(arg=name))
            i += 1
        for x in nargs:
            while (name := names[i]) in names_set:
                i += 1
            d[x.arg] = name
            args.append(arg(arg=name))
            i += 1
        minus = i
        for x in kargs:
            while (name := names[i]) in names_set:
                i += 1
            d[x.arg] = name
            kwonlyargs.append(arg(arg=name))
            i += 1
        return Lambda(
            args=arguments(
                posonlyargs=posonlyargs,
                args=args,
                kwonlyargs=kwonlyargs,
                kw_defaults=node_arguments.kw_defaults,
                defaults=node_arguments.defaults
            ),
            body=fix_missing_locations(RewriteExpression(d, use_map=self.use_map).visit(node.body))
        )
    def visit_Call(self, node: Call) -> Call:
        node.func = func = self.visit(node.func)
        has_map = self.use_map and isinstance(func, Name) and func.id == "map"
        for i, x in enumerate(args := node.args):
            args[i] = arg = self.visit(x)
            if isinstance(arg, Starred):
                if isinstance(val := arg.value, (List, Tuple, Set)):
                    args[i:i+1] = val.elts
                elif isinstance(val, Dict):
                    args[i:i+1] = val.keys
            elif has_map:
                if isinstance(arg, (List, Tuple, Set)) and len(el := arg.elts) == 1 and isinstance(el := el[0], Starred):
                    args[i] = el.value
        i = 0
        kwds = node.keywords
        while i < len(kwds):
            kwds[i] = kwd = self.visit(kwds[i])
            if kwd.arg is None and type(val := kwd.value) is Dict:
                if not (values := val.values):
                    del kwds[i]
                    continue
                keys = val.keys
                if i > 0 and (kwd2 := kwds[i-1]).arg is None and type(val2 := kwd2.value) is Dict:
                    pos = i - 1
                    j = len(val2.keys)
                    keys = val2.keys + keys
                    val2.keys = keys
                    val2.values = values = val2.values + values
                    del kwds[i]
                else:
                    pos = i
                    j = 0
                while j < len(keys):
                    if isinstance(x := keys[j], Constant) and isinstance(val := x.value, str):
                        kwds.insert(pos, keyword(arg=val, value=values[j]))
                        del keys[j]
                        del values[j]
                    else:
                        j += 1
            i += 1
        return node

class GetNames(NodeVisitor):
    def __init__(self):
        self._set = {*()}
    def visit(self, node: AST) -> set:
        super().visit(node)
        return self._set
    def visit_Name(self, node: Name):
        self._set.add(node.id)

class RewriteFunctions(NodeTransformer):
    def __init__(self, use_map=False):
        self.use_map = use_map
    def visit_FunctionDef(self, node: FunctionDef) -> Assign:
        node_arguments = node.args
        posonlyargs = []
        args = []
        kwonlyargs = []
        d = {}
        pargs, nargs, kargs = node_arguments.posonlyargs, node_arguments.args, node_arguments.kwonlyargs
        names_set = (GetNames().visit(node)
                     - set([x.arg for x in pargs])
                     - set([x.arg for x in nargs])
                     - set([x.arg for x in kargs]))
        i = 0
        for x in pargs:
            while (name := names[i]) in names_set:
                i += 1
            d[x.arg] = name
            posonlyargs.append(arg(arg=name))
            i += 1
        for x in nargs:
            while (name := names[i]) in names_set:
                i += 1
            d[x.arg] = name
            args.append(arg(arg=name))
            i += 1
        minus = i
        for x in kargs:
            while (name := names[i]) in names_set:
                i += 1
            d[x.arg] = name
            kwonlyargs.append(arg(arg=name))
            i += 1
        return Assign(
            targets=[Name(id=node.name, ctx=Store())],
            value=Lambda(
                args=arguments(
                    posonlyargs=posonlyargs,
                    args=args,
                    kwonlyargs=kwonlyargs,
                    kw_defaults=node_arguments.kw_defaults,
                    defaults=node_arguments.defaults
                ),
                body=fix_missing_locations(RewriteExpression(d, use_map=self.use_map).visit(node.body[0].value))
            )
        )

class GolfUnparser(_Unparser):
    def items_view(self, traverser, items):
        """Traverse and separate the given *items* with a comma and append it to
        the buffer. If *items* is a single item sequence, a trailing comma
        will be added."""
        if len(items) == 1:
            traverser(items[0])
            self.write(",")
        else:
            self.interleave(lambda: self.write(","), traverser, items)
    def visit_NamedExpr(self, node):
        with self.require_parens(_Precedence.TUPLE, node):
            self.set_precedence(_Precedence.ATOM, node.target, node.value)
            self.traverse(node.target)
            self.write(":=")
            self.traverse(node.value)
    def visit_Assign(self, node):
        self.fill()
        for target in node.targets:
            self.traverse(target)
            self.write("=")
        self.traverse(node.value)
        if type_comment := self.get_type_comment(node):
            self.write(type_comment)
    def visit_AugAssign(self, node):
        self.fill()
        self.traverse(node.target)
        self.write(f"{self.binop[node.op.__class__.__name__]}=")
        self.traverse(node.value)
    def visit_AnnAssign(self, node):
        self.fill()
        with self.delimit_if("(", ")", not node.simple and isinstance(node.target, Name)):
            self.traverse(node.target)
        self.write(":")
        self.traverse(node.annotation)
        if node.value:
            self.write("=")
            self.traverse(node.value)
    def visit_List(self, node):
        with self.delimit("[", "]"):
            self.interleave(lambda: self.write(","), self.traverse, node.elts)
    def visit_DictComp(self, node):
        with self.delimit("{", "}"):
            self.traverse(node.key)
            self.write(":")
            self.traverse(node.value)
            for gen in node.generators:
                self.traverse(gen)
    def visit_Set(self, node):
        if node.elts:
            with self.delimit("{", "}"):
                self.interleave(lambda: self.write(","), self.traverse, node.elts)
        else:
            # `{}` would be interpreted as a dictionary literal, and
            # `set` might be shadowed. Thus:
            self.write('{*()}')
    def visit_comprehension(self, node):
        if node.is_async:
            if self._source[-1][-1] not in {']',')','}','"',"'"}:
                self.write(" ")
            self.write("async for")
        else:
            if self._source[-1][-1] not in {']',')','}','"',"'"}:
                self.write(" ")
            self.write("for")
        self.set_precedence(_Precedence.TUPLE, node.target)
        if (cond := isinstance(node.target, Name)):
            self.write(" ")
        self.traverse(node.target)
        if cond:
            self.write(" ")
        self.write("in")
        inner_src = []
        old_write = self.write
        self.write = alt_write = lambda t: inner_src.append(t)
        self.set_precedence(_Precedence.TEST.next(), node.iter, *node.ifs)
        self.traverse(node.iter)
        if inner_src[0][0] not in {'[','(','{','"',"'"}:
            old_write(" ")
        if not node.ifs:
            self._source.extend(inner_src)
        else:
            for if_clause in node.ifs:
                if inner_src[-1][-1] not in {']', ')', '}', '"', "'"}:
                    alt_write(" ")
                alt_write("if")
                self._source.extend(inner_src)
                self.traverse(if_clause)
                if inner_src[0][0] not in {'[','(','{','"',"'"}:
                    old_write(" ")
        self.write = old_write
    def visit_IfExp(self, node):
        with self.require_parens(_Precedence.TEST, node):
            self.set_precedence(_Precedence.TEST.next(), node.body, node.test)
            self.traverse(node.body)
            if self._source[-1][-1] not in {']', ')', '}', '"', "'"}:
                self.write(" ")
            self.write("if")
            inner_src = []
            old_write = self.write
            self.write = alt_write = lambda t: inner_src.append(t)
            self.traverse(node.test)
            if inner_src[0][0] not in {'[','(','{','"',"'"}:
                old_write(" ")
            self._source.extend(inner_src)
            if inner_src[-1][-1] not in {']', ')', '}', '"', "'"}:
                old_write(" ")
            inner_src.clear()
            old_write("else")
            self.set_precedence(_Precedence.TEST, node.orelse)
            self.traverse(node.orelse)
            if inner_src[0][0] not in {'[','(','{','"',"'"}:
                old_write(" ")
            self._source.extend(inner_src)
            self.write = old_write
    def visit_Dict(self, node):
        def write_key_value_pair(k, v):
            self.traverse(k)
            self.write(":")
            self.traverse(v)
        def write_item(item):
            k, v = item
            if k is None:
                # for dictionary unpacking operator in dicts {**{'y': 2}}
                # see PEP 448 for details
                self.write("**")
                self.set_precedence(_Precedence.EXPR, v)
                self.traverse(v)
            else:
                write_key_value_pair(k, v)
        with self.delimit("{", "}"):
            self.interleave(
                lambda: self.write(","), write_item, zip(node.keys, node.values)
            )
    def visit_Tuple(self, node):
        with self.delimit("(", ")"):
            self.items_view(self.traverse, node.elts)
    def visit_BinOp(self, node):
        operator = self.binop[node.op.__class__.__name__]
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
            self.write(operator)
            self.set_precedence(right_precedence, node.right)
            self.traverse(node.right)
    def visit_Compare(self, node):
        with self.require_parens(_Precedence.CMP, node):
            self.set_precedence(_Precedence.CMP.next(), node.left, *node.comparators)
            self.traverse(node.left)
            for o, e in zip(node.ops, node.comparators):
                self.write(self.cmpops[o.__class__.__name__])
                self.traverse(e)
    def visit_BoolOp(self, node):
        operator = self.boolops[node.op.__class__.__name__]
        operator_precedence = self.boolop_precedence[operator]
        def increasing_level_traverse(node):
            self.set_precedence(operator_precedence, node)
            self.traverse(node)
        with self.require_parens(operator_precedence, node):
            s_before = f" {operator}"
            seq = iter(node.values)
            try:
                increasing_level_traverse(next(seq))
            except StopIteration:
                pass
            else:
                for x in seq:
                    if self._source[-1][-1] not in {']', ')', '}', '"', "'"}:
                        self.write(" ")
                    self.write(operator)
                    old_write = self.write
                    inner_src = []
                    self.write = lambda t: inner_src.append(t)
                    increasing_level_traverse(x)
                    self.write = old_write
                    if inner_src[0][0] not in {'[', '(', '{', '"', "'"}:
                        self.write(" ")
                    self._source.extend(inner_src)
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
                self.traverse(e)
            if node.keywords:
                pe = node.keywords[0]
                if comma:
                    self.write(',')
                else:
                    comma = True
                self.traverse(pe)
                for e in node.keywords[1:]:
                    if not pe.arg and not e.arg:
                        self.write("|")
                        pe = e
                        e = e.value
                    else:
                        pe = e
                        if comma:
                            self.write(",")
                        else:
                            comma = True
                    self.traverse(e)
    def visit_arg(self, node):
        self.write(node.arg)
        if node.annotation:
            self.write(":")
            self.traverse(node.annotation)
    def visit_arguments(self, node):
        first = True
        # normal arguments
        all_args = node.posonlyargs + node.args
        defaults = [None] * (len(all_args) - len(node.defaults)) + node.defaults
        for index, elements in enumerate(zip(all_args, defaults), 1):
            a, d = elements
            if first:
                first = False
            else:
                self.write(",")
            self.traverse(a)
            if d:
                self.write("=")
                self.traverse(d)
            if index == len(node.posonlyargs):
                self.write(",/")
        # varargs, or bare '*' if no varargs but keyword-only arguments present
        if node.vararg or node.kwonlyargs:
            if first:
                first = False
            else:
                self.write(",")
            self.write("*")
            if node.vararg:
                self.write(node.vararg.arg)
                if node.vararg.annotation:
                    self.write(":")
                    self.traverse(node.vararg.annotation)
        # keyword-only arguments
        if node.kwonlyargs:
            for a, d in zip(node.kwonlyargs, node.kw_defaults):
                self.write(",")
                self.traverse(a)
                if d:
                    self.write("=")
                    self.traverse(d)
        # kwargs
        if node.kwarg:
            if first:
                first = False
            else:
                self.write(",")
            self.write("**" + node.kwarg.arg)
            if node.kwarg.annotation:
                self.write(":")
                self.traverse(node.kwarg.annotation)
    def visit_Lambda(self, node):
        with self.require_parens(_Precedence.TEST, node):
            self.write("lambda ")
            self.traverse(node.args)
            self.write(":")
            self.set_precedence(_Precedence.TEST, node.body)
            self.traverse(node.body)
