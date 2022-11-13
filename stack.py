import builtins
from _ctypes import Py_INCREF
from ctypes import *
from gc import get_objects
from opcode import EXTENDED_ARG, opmap
from sys import _getframe

py_object_p = POINTER(py_object)

list_builtins = [*builtins.__dict__.values()]
needs_return = {opmap['POP_TOP'], opmap['PRINT_EXPR'], opmap['RETURN_VALUE']}

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
    stacksize_addr = c_void_p.from_address(code_addr).value + object.__basicsize__ + 4 * tuple.__itemsize__ + 4 * sizeof(c_int) + 2 * sizeof(c_short)
    nlocplus = c_int.from_address(id(frame.f_code) + object.__basicsize__ + tuple.__itemsize__*4 + sizeof(c_int)*6 + sizeof(c_short)*2).value
    default_length = c_int.from_address(stacktop_addr := addr + tuple.__itemsize__ * 8).value + 1
    array_base = stacktop_addr + sizeof(c_int) * 2 + nlocplus * tuple.__itemsize__
    class stack:
        def __init__(self, frame, code_addr, stacksize_addr, stacktop_addr, array_base):
            self.frame = frame
            self.code_addr = code_addr
            self.stacksize_addr = stacksize_addr
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
            stacktop = (stacktop_cell := c_int.from_address(self.stacktop_addr)).value
            idx = 0
            while idx <= stacktop:
                if (tup := self._check(idx))[0] == 1 and tup[1].value is self.__class__.push:
                    break
                idx += 1
            else:
                raise ValueError("stack is full")
            c_int.from_address(self.stacksize_addr).value += 1
            co_code = frame.f_code.co_code[frame.f_lasti+2:]
            i = 0
            while co_code[i] is EXTENDED_ARG:
                i += 2
            if co_code[i] in needs_return:
                Py_INCREF(value)
                tup[1].value = value
                stacktop_cell.value += 1
            return value
        def set_top(self, value):
            stacktop = (stacktop_cell := c_int.from_address(self.stacktop_addr)).value
            idx = 0
            while idx <= stacktop:
                if idx and (tup := self._check(idx))[0] == 1 and tup[1].value is self.__class__.set_top:
                    idx -= 1
                    break
                idx += 1
            else:
                raise ValueError("stack is empty")
            Py_INCREF(value)
            py_object.from_address(self.array_base + tuple.__itemsize__ * idx).value = value
        def pop(_, self=0):
            if _ is not self:
                Py_INCREF(_)
            stacktop = (stacktop_cell := c_int.from_address(self.stacktop_addr)).value
            idx = 0
            while idx <= stacktop:
                if idx and (tup := self._check(idx))[0] == 1 and tup[1].value is self.__class__.pop:
                    idx -= 1
                    break
                idx += 1
            else:
                raise ValueError("stack is empty")
            cell = py_object.from_address(self.array_base + idx * tuple.__itemsize__)
            old = cell.value
            stacktop_cell.value -= 1
            return old
    stack.pop.__defaults__ = (res:=stack(frame, code_addr, stacksize_addr, stacktop_addr, array_base),)
    return res
