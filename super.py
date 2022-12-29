from _ctypes import PyObj_FromPtr
from ctypes import c_bool, c_char, c_int, c_short, c_void_p, cast, py_object, sizeof
from dis import opmap
from sys import _getframe
from types import CellType, MappingProxyType

NULL = type('<NULL>', (), {'__repr__': lambda _: '<NULL>', '__bool__': lambda _: False})() # python-safe NULL sentinel

SIZE_PTR = tuple.__itemsize__
CO_FAST_LOCAL = 0x20
CO_FAST_CELL = 0x40
CO_FAST_FREE = 0x80
HAS_CELL_BEGIN = {opmap['MAKE_CELL'], opmap['COPY_FREE_VARS']}

object_getattr = object.__getattribute__
dict_of = type('', (), {'__ror__': lambda _, x: x})()

def supercheck(type_, obj):
    if isinstance(obj, type) and issubclass(obj, type_):
        return obj
    if issubclass(type(obj), type_):
        return type(obj)
    else:
        class_attr = getattr(obj, '__class__', NULL)
        if (class_attr is not NULL and
                isinstance(class_attr, type) and
                class_attr is not type(obj) and
                issubclass(class_attr, type_)):
            return class_attr
    raise TypeError("super(type, obj): obj must be an instance or subtype of type")

def getattr_(self, name):
    try:
        return object_getattr(self, name)
    except AttributeError:
        return NULL

class Super:
    __slots__ = ('type', 'obj', 'obj_type')
    def __init__(self, type_=NULL, obj=NULL, /):
        if type_ is NULL:
            cframe = _getframe(1)
            co = cframe.f_code
            if co.co_argcount == 0:
                raise RuntimeError("super(): no arguments")
            co_nlocalsplus = len(co.co_cellvars) + len(co.co_freevars) + len(co.co_varnames)
            assert co_nlocalsplus > 0, "'assert co_nlocalsplus > 0' failed"
            # i don't know of any better way to get localsplus values here
            firstarg = c_void_p.from_address(fa_addr := c_void_p.from_address(id(cframe) + object.__basicsize__ + SIZE_PTR).value + SIZE_PTR * 8 + sizeof(c_int) * 2).value
            localsplus_names = py_object.from_address(lpn_addr := id(co) + object.__basicsize__ + SIZE_PTR * 4 + sizeof(c_int) * 11 + sizeof(c_short) * 2).value
            localsplus_kinds = py_object.from_address(lpn_addr + tuple.__itemsize__).value
            if firstarg is not None:
                firstarg = py_object.from_address(fa_addr).value
                if localsplus_kinds[0] & CO_FAST_CELL and cframe.f_lasti >= 0:
                    assert co.co_code[0] in HAS_CELL_BEGIN, "'assert co.co_code[0] in HAS_CELL_BEGIN' failed"
                    assert isinstance(firstarg, CellType), "'assert isinstance(firstarg, CellType)' failed"
                    obj = firstarg.cell_contents
                else:
                    obj = firstarg
            else:
                # there's also handling for the contents of firstarg if it's a cell
                # but we don't really have a reliable way to check that without
                # using the long ctypes stuff
                raise RuntimeError("super(): arg[0] deleted")
            i = co.co_nlocals + len([1 for x in localsplus_kinds if x & CO_FAST_CELL if not x & CO_FAST_LOCAL])
            while i < co_nlocalsplus:
                name = localsplus_names[i]
                assert isinstance(name, str), "'assert isinstance(name, str)' failed"
                if name == '__class__':
                    # PyObj_FromPtr and py_object.from_address is not working here
                    # so resort to locals checking
                    if '__class__' not in cframe.f_locals:
                        raise RuntimeError("super(): bad __class__ cell")
                type_ = cframe.f_locals['__class__']
                if not isinstance(type_, type):
                    raise RuntimeError("super(): __class__ is not a type (%s)" % type(type_).__name__)
                break
            if type_ is NULL:
                raise RuntimeError("super(): __class__ cell not found")
        self.type = type_
        if obj is None:
            obj = NULL
        if obj is not NULL:
            self.obj_type = supercheck(type_, obj)
        self.obj = obj
    def __repr__(self):
        if (obj_type := getattr_(self, 'obj_type')) is not NULL:
            return ("<super: <class '%s'>, <%s object>>" % (
                self.type.__name__ if self.type is not NULL else "NULL",
                obj_type.__name__
            ))
        else:
            return ("<super: <class '%s'>, NULL>" % 
                self.type.__name__ if self.type is not NULL else "NULL"
            )
    def __getattribute__(self, name):
        if (starttype := getattr_(self, 'obj_type')) is NULL:
            return object_getattr(self, name)
        if isinstance(name, str) and name == '__class__':
            return object_getattr(self, name)
        if (mro := getattr(starttype, '__mro__', NULL)) is NULL:
            return object_getattr(self, name)
        assert isinstance(mro, tuple), "'assert isinstance(mro, tuple)' failed"
        su_type = object_getattr(self, 'type')
        n = len(mro)
        for i in range(n-1):
            if su_type is mro[i]:
                break
        if (i := i + 1) >= n:
            return object_getattr(self, name)
        su_obj = None if (su_obj := object_getattr(self, 'obj')) is starttype else su_obj
        while i < n:
            dict_ = getattr(mro[i], '__dict__', NULL)
            assert dict_ is not NULL, "'assert dict_ is not NULL' failed"
            assert isinstance(dict_, (dict, MappingProxyType)), "'assert isinstance(dict_, (dict, MappingProxyType))' failed"
            if isinstance(dict_, MappingProxyType):
                dict_ = dict_ | dict_of
            try:
                res = dict_[name]
                f = getattr(type(res), '__get__', NULL)
                if f is not NULL:
                    res = f(res, su_obj, starttype)
                return res
            except KeyError:
                pass
            i += 1
        return object_getattr(self, name)
    def __get__(self, obj=NULL, type_=NULL):
        su_obj = object_getattr(self, 'obj')
        if obj is NULL or obj is None or su_obj is not NULL:
            return self
        su_type = object_getattr(self, 'type')
        if (type_self := type(self)) is not Super:
            return type_self(su_type, obj, NULL)
        else:
            obj_type = supercheck(su_type, obj)
            newobj = Super.__new__(Super)
            newobj.type = su_type
            newobj.obj = obj
            newobj.obj_type = obj_type
            return newobj
    @property
    def __thisclass__(self):
        return object_getattr(self, 'type')
    @property
    def __self__(self):
        return object_getattr(self, 'obj')
    @property
    def __self_class__(self):
        return object_getattr(self, 'obj_type')
