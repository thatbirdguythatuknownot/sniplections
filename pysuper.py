from dis import opmap
from sys import _getframe
from types import CellType, CodeType, FrameType, MappingProxyType
from typing import Self, TypeVar

# python-safe NULL sentinel (NULL_T for type annotations)
NULL_T = type('<NULL>', (), {'__repr__': lambda _: '<NULL>',
                             '__bool__': lambda _: False})
NULL = NULL_T()

SIZE_PTR = tuple.__itemsize__
CO_FAST_LOCAL = 0x20
CO_FAST_CELL = 0x40
CO_FAST_FREE = 0x80
HAS_CELL_BEGIN = {opmap['MAKE_CELL'], opmap['COPY_FREE_VARS']}

object_getattr = object.__getattribute__

# to get the internal dict in a mappingproxy
dict_of = type('', (), {'__ror__': lambda _, x: x})()

T = TypeVar('T')

def supercheck(type_: type, obj: T) -> T | type[T]:
    if isinstance(obj, type) and issubclass(obj, type_):
        return obj
    
    if issubclass(type(obj), type_):
        return type(obj)
    else:
        class_attr: type = getattr(obj, '__class__', NULL)
        if (class_attr is not NULL
                and isinstance(class_attr, type)
                and class_attr is not type(obj)
                and issubclass(class_attr, type_)):
            return class_attr
    
    raise TypeError("super(type, obj): obj must be an instance or subtype of type")

def getattr_(self, name: str):
    try:
        return object_getattr(self, name)
    except AttributeError:
        return NULL

class Super:
    __slots__ = ('type', 'obj', 'obj_type')
    
    type: type # exists in T.mro
    obj: T
    obj_type: T | type[T]
    
    def __init__(self: Self, type_: type | NULL_T = NULL, obj=NULL, /) -> None:
        assert type_ is NULL or isinstance(type_, type), (
            "Super() argument 1 must be a type, not %.200s"
            % type(type_).__name__
        )
        
        if type_ is NULL:
            try:
                cframe: FrameType = _getframe(1)
            except ValueError as e:
                raise RuntimeError("Super(): no current frame") from e.__cause__
            
            co: CodeType = cframe.f_code
            if co.co_argcount == 0:
                raise RuntimeError("Super(): no arguments")
            
            len_cellvars = len(co.co_cellvars)
            len_freevars = len(co.co_freevars)
            co_nlocalsplus = co.co_nlocals + len_cellvars + len_freevars
            assert co_nlocalsplus > 0, "'assert co_nlocalsplus > 0' failed"
            
            if co.co_nlocals:
                obj = cframe.f_locals.get(co.co_varnames[0], NULL)
            else: # is this even possible?
                assert co.co_code[0] in HAS_CELL_BEGIN
                if len_cellvars:
                    obj = cframe.f_locals.get(co.co_cellvars[0], NULL)
                else:
                    obj = cframe.f_locals.get(co.co_freevars[0], NULL)
            if obj is NULL:
                raise RuntimeError("Super(): arg[0] deleted")
            
            if '__class__' not in co.co_freevars:
                raise RuntimeError("Super(): __class__ cell not found")
            if '__class__' not in cframe.f_locals:
                raise RuntimeError("Super(): bad __class__ cell")
            
            type_ = cframe.f_locals['__class__']
            if not isinstance(type_, type):
                raise RuntimeError(
                    "Super(): __class__ is not a type (%s)"
                    % type(type_).__name__
                )
        
        self.type = type_
        if obj is None:
            obj = NULL
        if obj is not NULL:
            self.obj_type = supercheck(type_, obj)
        self.obj = obj
    
    def __repr__(self: Self) -> str:
        if (obj_type := getattr_(self, 'obj_type')) is not NULL:
            return ("<super: <class '%s'>, <%s object>>" % (
                self.type.__name__ if self.type is not NULL else "NULL",
                obj_type.__name__
            ))
        else:
            return ("<super: <class '%s'>, NULL>" % 
                self.type.__name__ if self.type is not NULL else "NULL"
            )
    
    def __getattribute__(self: Self, name: str):
        if (starttype := getattr_(self, 'obj_type')) is NULL:
            return object_getattr(self, name)
        
        if isinstance(name, str) and name == '__class__':
            return object_getattr(self, name)
        
        if (mro := getattr(starttype, '__mro__', NULL)) is NULL:
            return object_getattr(self, name)
        assert isinstance(mro, tuple), \
            "'assert isinstance(mro, tuple)' failed"
        
        su_type = object_getattr(self, 'type')
        n = len(mro)
        for i in range(n-1):
            if su_type is mro[i]:
                break
        else:
            return object_getattr(self, name)
        i += 1
        
        su_obj = object_getattr(self, 'obj')
        su_obj = None if su_obj is starttype else su_obj
        while i < n:
            dict_ = getattr(mro[i], '__dict__', NULL)
            assert dict_ is not NULL, "'assert dict_ is not NULL' failed"
            assert isinstance(dict_, (dict, MappingProxyType)), \
                "'assert isinstance(dict_, (dict, MappingProxyType))' failed"
            
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
    
    def __get__(self, obj=NULL, _: type = NULL) -> Self:
        su_obj = object_getattr(self, 'obj')
        if obj is NULL or obj is None or su_obj is not NULL:
            return self
        
        su_type = object_getattr(self, 'type')
        if (type_self := type(self)) is not Super:
            return type_self(su_type, obj, NULL)
        
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
