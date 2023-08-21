def init():
    import builtins as B
    from ctypes import py_object
    class balsglo(dict):
        __slots__ = ()
        def __getitem__(self, x):
            obj = B.dict.__getitem__(self, x)
            if B.isinstance(obj, B.property) and B.getattr(obj.fget, "__outsideproperty__", None):
                return obj.fget()
            return obj
        def __setitem__(self, x, y):
            obj = self.get(x)
            if B.isinstance(obj, B.property) and B.getattr(obj.fget, "__outsideproperty__", None):
                if obj.fset:
                    return obj.fset(y)
                raise ValueError(f"outside property {x!r} has no setter")
            return B.dict.__setitem__(self, x, y)
        def __delitem__(self, x):
            obj = self.get(x)
            if B.isinstance(obj, B.property) and B.getattr(obj.fget, "__outsideproperty__", None):
                if obj.fdel:
                    return obj.fdel()
                raise ValueError(f"outside property {x!r} has no deleter")
            return B.dict.__delitem__(self, x)
    py_object.from_address(id(globals()) + tuple.__itemsize__).value = balsglo

def oproperty(f):
    import builtins as B
    f.__outsideproperty__ = True
    return B.property(f)
