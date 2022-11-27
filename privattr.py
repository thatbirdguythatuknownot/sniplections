from sys import _getframe
from types import FunctionType

class Private:
    def __new__(_meta, name, bases, ns, **kwargs):
        if "__annotations__" not in ns:
            return type.__new__(type, name, bases, ns)
        attributes = {k for k, v in ns["__annotations__"].items() if v is _meta}
        all_code = set()
        for f in ns.values():
            if isinstance(f, FunctionType):
                all_code.add(f.__code__)
            elif isinstance(f, property):
                if f.fdel: all_code.add(f.fdel.__code__)
                if f.fget: all_code.add(f.fget.__code__)
                if f.fset: all_code.add(f.fset.__code__)
        old___getattribute__ = ns.get("__getattribute__")
        old___setattr__ = ns.get("__setattr__")
        def __getattribute___wrap(self, name):
            if name in attributes:
                frame = _getframe(1)
                while frame and frame.f_code not in all_code:
                    frame = frame.f_back
                if frame:
                    return old___getattribute__(self, name) if old___getattribute__ else super(type(self), self).__getattribute__(name)
                raise AttributeError(f"'{type(self).__name__}' object has no attribute '{name}'")
            else:
                return old___getattribute__(self, name) if old___getattribute__ else super(type(self), self).__getattribute__(name)
        def __setattr___wrap(self, name, value):
            if name in attributes:
                frame = _getframe(1)
                while frame and frame.f_code not in all_code:
                    frame = frame.f_back
                if frame:
                    return old___setattr__(self, name, value) if old___setattr__ else super(type(self), self).__setattr__(name, value)
                raise AttributeError(f"'{type(self).__name__}' object has no attribute '{name}'")
            else:
                return old___setattr__(self, name, value) if old___setattr__ else super(type(self), self).__setattr__(name, value)
        ns["__getattribute__"], ns["__setattr__"] = __getattribute___wrap, __setattr___wrap
        return type.__new__(type, name, bases, ns)
