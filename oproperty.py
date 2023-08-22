def init():
    import builtins as B
    from ctypes import py_object
    def check(o):
        if B.isinstance(o, B.property):
            return B.getattr(o, "__outsideproperty__", None) or B.getattr(o.fget, "__outsideproperty__", None)
        return False
    class balsglo(dict):
        __slots__ = ()
        def __getitem__(self, x):
            return obj.fget() if check(obj := B.dict.__getitem__(self, x)) else obj
        def __setitem__(self, x, y):
            obj = self.get(x)
            if check(obj := self.get(x)):
                if obj.fset:
                    return obj.fset(y)
                raise ValueError(f"outside property {x!r} has no setter")
            return B.dict.__setitem__(self, x, y)
        def __delitem__(self, x):
            if check(obj := self.get(x)):
                if obj.fdel:
                    return obj.fdel()
                raise ValueError(f"outside property {x!r} has no deleter")
            return B.dict.__delitem__(self, x)
    py_object.from_address(id(globals()) + tuple.__itemsize__).value = balsglo

def oproperty(f=None, fset=None, fdel=None):
    import builtins as B
    def wrap(f):
        f.__outsideproperty__ = True
        f = B.property(f)
        if B.callable(fset): f.setter(fset)
        if B.callable(fdel): f.deleter(fdel)
        return f
    if f is not None:
        return wrap(f)
    return wrap

class olazyproperty(property):
    def __init__(self, f):
        import builtins as B
        B.property.__init__(self, f)
    def setter(self, f):
        import builtins as B
        B.property.setter(self, f)
    def getter(self, f):
        import builtins as B
        B.property.getter(self, f)
    def deleter(self, f):
        import builtins as B
        B.property.deleter(self, f)
    def init(self):
        self.__outsideproperty__ = True
