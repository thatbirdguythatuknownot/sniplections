import builtins
from _ctypes import Py_INCREF
from ctypes import *
from gc import get_objects
from sys import _getframe

py_object_p = POINTER(py_object)

list_builtins = [*builtins.__dict__.values()]

@lambda c: c()
class NULL:
    def __repr__(self):
        return "<NULL>"

@lambda c: c()
class garbage:
    def __repr__(self):
        return "<garbage>"

class StackIter:
    def __init__(self, stack):
        self.stack = stack
        self.idx = 0
    def __iter__(self):
        return self
    def __next__(self):
        try:
            val = self.stack[self.idx]
        except ValueError:
            raise StopIteration from None
        except IndexError:
            raise StopIteration from None
        else:
            self.idx += 1
            return val

def get_stack(depth=0):
    if depth < 0:
        print("corruption warning: get_stack() argument depth < 0")
    frame = _getframe(depth + 1)
    addr = c_void_p.from_address(id(frame) + object.__basicsize__ + tuple.__itemsize__).value
    code_addr = addr + tuple.__itemsize__ * 4
    nlocplus = c_int.from_address(id(frame.f_code) + object.__basicsize__ + tuple.__itemsize__*4 + sizeof(c_int)*6 + sizeof(c_short)*2).value
    default_length = c_int.from_address(stacktop_addr := addr + tuple.__itemsize__ * 8).value + 1
    array_base = stacktop_addr + sizeof(c_int) * 2 + nlocplus * tuple.__itemsize__
    @lambda c:c(code_addr, stacktop_addr, array_base)
    class stack:
        def __init__(self, code_addr, stacktop_addr, array_base):
            self.code_addr = code_addr
            self.stacktop_addr = stacktop_addr
            self.array_base = array_base
        def __iter__(self):
            return StackIter(self)
        def __getitem__(self, idx):
            if hasattr(idx, "__index__"):
                idx = idx.__index__()
                i, cell = self._check(idx)
                if i:
                    return cell.value if i == 1 else garbage if i == -1 else NULL
                raise IndexError("stack index out of range")
            elif not isinstance(idx, slice):
                raise TypeError(f"stack indices must be integers or slices, not str")
            idx, stop, step = idx.start or 0, idx.stop or len(self), idx.step or 1
            res = []
            j = 1
            while len(self) > idx < stop:
                i, cell = self._check(idx)
                val = cell.value if i == 1 else garbage if i == -1 else NULL
                res.append(val)
                idx += step
                j += 1
            return res
        def __setitem__(self, idx, value):
            if hasattr(idx, "__index__"):
                idx = idx.__index__()
                i, cell = self._check(idx)
                if i == 1:
                    cell.value = value
                elif i:
                    py_object.from_address(cell).value = value
                else:
                    raise IndexError("stack index out of range")
            elif not isinstance(idx, slice):
                raise TypeError(f"stack indices must be integers or slices, not str")
            arr = [*self]
            arr[idx] = value
            assert (larr := len(arr)) <= len(self), "cannot have a resulting stack with more length than .co_stacksize"
            for idx, val in enumerate(range(larr)):
                i, cell = self._check(idx)
                if i == 1:
                    cell.value = val
                else:
                    py_object.from_address(cell).value = val
        def __repr__(self, reprf=0):
            f = _getframe()
            while f := f.f_back:
                if f.f_code is reprf.__code__:
                    return "stack: [...]"
            return f"stack: {[*self]}"
        __repr__.__defaults__ = (__repr__,)
        def __len__(self):
            return py_object.from_address(self.code_addr).value.co_stacksize
        def _check(self, idx):
            import builtins
            if idx < len(self):
                check = c_void_p.from_address(place := self.array_base + idx * tuple.__itemsize__).value
                if check:
                    consts = py_object.from_address(self.code_addr).value.co_consts
                    if (any(id(x) == check for x in consts)
                            or any(id(x) == check for x in get_objects())
                            or any(id(x) == check for x in list_builtins)):
                        return 1, py_object.from_address(place)
                    return -1, place
                return 2, place
            return 0, None
        def push(self, value):
            #for idx in range(len(self)):
            #    i, place = self._check(idx)
            #    if i != 1:
            #        break
            #else:
            #    raise ValueError("stack is full")
            stacktop = (stacktop_cell := c_int.from_address(self.stacktop_addr)).value + 1
            stacktop_cell.value += 1
            while stacktop >= 0:
                if (tup := self._check(stacktop))[0] == -1:
                    break
                stacktop -= 1
            else:
                c_int.from_address(self.code_addr + 3 * tuple.__itemsize__ + sizeof(c_int) * 4 + sizeof(c_short)).value += 1
                tup = (-1, self.array_base + tuple.__itemsize__ * stacktop_cell.value)
            print(c_ssize_t.from_address(self.array_base).value, id(7))
            Py_INCREF(value);
            c_void_p.from_address(tup[1]).value = id(value)
        def set_top(self, value):
            stacktop = (stacktop_cell := c_int.from_address(self.stacktop_addr)).value
            while stacktop >= 0:
                if stacktop and (tup := self._check(stacktop))[0] == 1 and tup[1].value is self.__class__.set_top:
                    stacktop -= 1
                    break
                stacktop -= 1
            else:
                raise ValueError("stack is empty")
            Py_INCREF(value)
            py_object.from_address(self.array_base + tuple.__itemsize__ * stacktop).value = value
        def pop(self):
            #for idx in range(len(self)):
            #    i, cell = self._check(idx)
            #    if i != 1:
            #        idx -= 1
            #        break
            #    prevcell = cell
            #if idx < 0:
            #    raise ValueError("stack is empty")
            stacktop = (stacktop_cell := c_int.from_address(self.stacktop_addr)).value
            if stacktop < 0:
                raise ValueError("stack is empty")
            cell = py_object.from_address(self.array_base + stacktop * tuple.__itemsize__)
            old = cell.value
            stacktop_cell.value -= 1
            return old
    return stack

def f():
    return (7, get_stack().push(5))

f()

def f():
    p1, p2 = get_stack().push, get_stack().pop
    p1(5)
    return (p2(),)

f()

fail = """
def f():
    get_stack().push(5)

f.__code__ = f.__code__.replace(co_code=f.__code__.co_code[:-4]+f.__code__.co_code[-2:])
from dis import dis
dis(f)
f()

def f():
    get_stack().push(5)
    return (get_stack().pop(),)

f()
"""

exec(fail)

def f():
    return (7, get_stack().set_top(5))

f()
