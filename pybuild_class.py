from types import CellType, FunctionType

# python-safe NULL sentinel (NULL_T for type annotations)
NULL_T = type('<NULL>', (), {'__repr__': lambda _: '<NULL>',
                             '__bool__': lambda _: False})
NULL = NULL_T()

def update_bases(bases: tuple[type], args) -> tuple[type]:
    new_bases: list[type] | bool = False # optimize if check below
    for i, base in enumerate(args):
        if isinstance(base, type):
            if new_bases:
                # If we already have made a replacement, then we append every
                # normal base, otherwise just skip it.
                new_bases.append(base)
            continue
        if not hasattr(base, '__mro_entries__'):
            if new_bases:
                new_bases.append(base)
            continue
        new_base = base.__mro_entries__(bases)
        if not isinstance(new_base, tuple):
            raise TypeError("__mro_entries__ must return a tuple")
        if not new_bases:
            new_bases = [*args[:i], new_base]
        else:
            new_bases.append(new_base)
    if not new_bases:
        return bases
    return (*new_bases,)

def calculate_metaclass(metatype: type, bases: tuple[type]) -> type:
    # Determine the proper metatype to deal with this,
    # and check for metatype conflicts while we're at it.
    # Note that if some other metatype wins to contract,
    # it's possible that its instances are not types.
    
    winner = metatype
    for tmptype in map(type, bases):
        if issubclass(winner, tmptype):
            continue
        if issubclass(tmptype, winner):
            winner = tmptype
            continue
        # else:
        raise TypeError("metaclass conflict: "
                        "the metaclass of a derived class "
                        "must be a (non-strict) subclass "
                        "of the metaclasses of all its bases")
    return winner

def build_class(func: FunctionType, name: str, *orig_bases, **mkw):
    if not isinstance(func, FunctionType): # Better be callable
        raise TypeError("build_class: func must be a function")
    if not isinstance(name, str):
        raise TypeError("build_class: name is not a string")
    
    bases: tuple[type] = update_bases(orig_bases, orig_bases)
    
    meta: type | NULL
    isclass: bool = 0
    if mkw:
        try:
            meta = mkw["metaclass"]
            del mkw["metaclass"]
            # metaclass is explicitly given, check if it's indeed a class
            isclass = isinstance(meta, type)
        except KeyError:
            meta = NULL
    else:
        meta = NULL
    if meta is NULL:
        # if there are no bases, use type; else get the type of the first base
        meta = type if not bases else type(bases[0])
        isclass = True # meta is really a class
    
    if isclass:
        # meta is really a class, so check for a more derived
        # metaclass, or possible metaclass conflicts:
        winner = calculate_metaclass(meta, bases)
        if winner is not meta:
            meta = winner
    # else: meta is not a class, so we cannot do the metaclass
    # calculation, so we will use the explicitly given object as it is
    if not hasattr(meta, "__prepare__"):
        ns = {}
    else:
        ns = meta.__prepare__(name, bases, **mkw)
    if not isinstance(ns, dict):
        raise TypeError(
            "%.200s.__prepare__() must return a mapping, not %.200s"
            % (meta.__name__ if isclass else "<metaclass>",
               type(ns).__name__))
    cell = exec(func.__code__, func.__globals__, ns)
    if bases is not orig_bases:
        ns["__orig_bases__"] = orig_bases
    cls = meta(name, bases, ns, **mkw)
    if isinstance(cls, type) and isinstance(cell, CellType):
        try:
            cell_cls = cell.cell_contents
            if cell_cls is not cls:
                raise TypeError(
                    "__class__ set to %.200R defining %.200R as %.200R"
                    % (cell_cls, name, cls))
        except ValueError as e:
            raise RuntimeError(
                "__class__ not set defining %.200R as %.200R. "
                "Was __classcell__ propagated to type.__new__?"
                % (name, cls)) from e.__cause__
    return cls
