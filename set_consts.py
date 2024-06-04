from contextlib import contextmanager
from ctypes import py_object
from sys import _getframe

@contextmanager
def set_consts(*args, **kwargs):
    if args:
        mapping, = args
    else:
        mapping = {}
    mapping |= kwargs
    # `2` for the frame number HEAVILY RELIES on the fact
    # that the only frame between this function and the
    # outside context is somewhere in `contextlib`
    tup = _getframe(2).f_code.co_consts
    changed = []
    for i, x in enumerate(tup):
        try:
            new_x = mapping[x]
            changed.append((i, x))
            py_object.from_address(id(tup) + tuple.__basicsize__ + i*tuple.__itemsize__).value = new_x
        except KeyError:
            pass
    yield
    for i, x in changed:
        py_object.from_address(id(tup) + tuple.__basicsize__ + i*tuple.__itemsize__).value = x
