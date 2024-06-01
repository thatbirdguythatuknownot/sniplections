from sys import _getframe
from gc import get_referrers

NAME_ERROR_MSG = "name '{:.200s}' is not defined"
NONINIT_ERROR_MSG = "nonlocal variable '{:.200s}' is not initialized"
UNBOUND_ERROR_MSG = "nonlocal variable '{:.200s}' is unbound"

class _nonlocals_proxy:
    __module__ = 'builtins'
    def __init__(self, frame=None):
        if frame is None:
            frame = _getframe(1)
        func = get_referrers(frame.f_code)[0]
        self.__func = func
        self.__closure_arr = func.__closure__ or ()
    def __get_closure_from_name(self, k):
        code = self.__func.__code__
        try:
            try:
                idx = code.co_cellvars.index(k)
            except ValueError:
                idx = len(code.co_cellvars) + code.co_freevars.index(k)
            return self.__closure_arr[idx]
        except ValueError:
            raise KeyError(NAME_ERROR_MSG.format(k)) from None
        except IndexError:
            raise KeyError(NONINIT_ERROR_MSG.format(k)) from None
    def __getitem__(self, k):
        cell = self.__get_closure_from_name(k)
        try:
            return cell.cell_contents
        except ValueError:
            raise KeyError(UNBOUND_ERROR_MSG.format(k)) from None
    def __setitem__(self, k, v):
        cell = self.__get_closure_from_name(k)
        cell.cell_contents = v
    def __len__(self):
        return len(self.__closure_arr)
    def __contains__(self, k):
        try:
            self[k]
        except KeyError:
            return False
        else:
            return True
    def __or__(self, other):
        if not isinstance(other, (dict, _nonlocals_proxy)):
            return NotImplemented
        return {**self, **other}
    def __ror__(self, other):
        if not isinstance(other, (dict, _nonlocals_proxy)):
            return NotImplemented
        return {**other, **self}
    def __ior__(self, other):
        if not isinstance(other, (dict, _nonlocals_proxy)):
            return NotImplemented
        self.update(other)
    def __str__(self):
        d = {**self}
        return f"{d!r}"
    def __repr__(self):
        return f"_nonlocals_proxy({self!s})"
    def __reversed__(self):
        return reversed(self.keys())
    def update(self, *args, **kwargs):
        if args:
            try:
                E, = args
            except ValueError as e:
                e.add_note("_nonlocals_proxy.update() expected 0 or 1 "
                           "positional arguments")
                raise
            if isinstance(E, (dict, _nonlocals_proxy)) or hasattr(E, "keys"):
                for key in E:
                    self[key] = E[key]
            else:
                for key, value in E:
                    self[key] = value
        for key in kwargs:
            self[key] = E[key]
    def copy(self):
        return {**self}
    def keys(self):
        code = self.__func.__code__
        return [key for key, _ in self.items()]
    def values(self):
        return list(self.__closure_arr)
    def items(self):
        code = self.__func.__code__
        return list(zip(code.co_cellvars + code.co_freevars,
                        self.__closure_arr))
    def get(self, key, /, default=None):
        try:
            return self[key]
        except KeyError:
            return default
    def setdefault(self, key, /, default=None):
        cell = self.__get_closure_from_name(key)
        try:
            return cell.cell_contents
        except ValueError:
            cell.cell_contents = default
            return default

def nonlocals():
    return _nonlocals_proxy(_getframe(1))
