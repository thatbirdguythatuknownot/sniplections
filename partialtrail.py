@type.__call__
class partialtrail:
    __slots__ = "__trail"
    def __init__(self, /):
        self.__trail = ()
    def __getattr__(self, /, attr):
        # surprisingly efficient way to check
        if attr == '_' * len(attr):
            r = (False, ...)
        else:
            r = (False, attr)
        res = super().__new__(__class__)
        res.__trail = *self.__trail, r
        return res
    def __getitem__(self, /, v):
        res = super().__new__(__class__)
        res.__trail = *self.__trail, (True, v)
        return res
    def __call__(self, obj):
        types = []
        vals = []
        pholders = []
        for i, (typ, val) in enumerate(self.__trail):
            types.append(typ)
            vals.append(val)
            if val is ...:
                pholders.append(i)
        lastidx = len(types) - 1
        npholders = len(pholders)
        class Wrapper(type(obj)):
            __slots__ = "__obj", "__val", "__idx"
            def __init__(self, /, obj, nfilled):
                self.__obj = obj
                self.__val = ...
                self.__idx = nfilled
                assert nfilled < npholders, "what"
            def __eval(self, /, do_after=True, skiplast=False):
                obj = self.__obj
                if isinstance(obj, Wrapper):
                    idx = obj.__idx + 1
                    obj = obj.__eval(False)
                else:
                    idx = 0
                pidx = pholders[self.__idx]
                for i in range(idx, pidx - 1):
                    val = vals[i]
                    if types[i]:
                        obj = obj[val]
                    else:
                        obj = getattr(obj, val)
                if skiplast and pidx == lastidx:
                    return obj
                if types[pidx]:
                    obj = obj[self.__val]
                else:
                    obj = getattr(obj, self.__val)
                if do_after:
                    for i in range(pidx + 1, len(types) - int(skiplast)):
                        val = vals[i]
                        if types[i]:
                            obj = obj[val]
                        else:
                            obj = getattr(obj, val)
                return obj
            def __peval(self, /):
                idx = self.__idx
                if idx + 1 == npholders:
                    return self.__eval()
                return Wrapper(self, idx + 1)
            def __setv(self, /, val, setto, is_getitem):
                self.__check(is_getitem)
                idx = self.__idx
                assert idx + 1 == npholders, "not all placeholders filled!"
                pidx = pholders[idx]
                if pidx != lastidx:
                    self.__val = val
                    typ = types[-1]
                    val = vals[-1]
                else:
                    typ = is_getitem
                obj = self.__eval(skiplast=True)
                if typ:
                    obj[val] = setto
                else:
                    setattr(obj, val, setto)
            def __check(self, /, is_getitem):
                idx = self.__idx
                if types[pholders[self.__idx]] != is_getitem:
                    exp = '[]' if is_getitem else '.'
                    raise TypeError(f"filler mismatch! (expected: {exp})")
            def __getitem__(self, /, val):
                self.__check(True)
                self.__val = val
                return self.__peval()
            def __getattr__(self, /, attr):
                self.__check(False)
                self.__val = attr
                return self.__peval()
            def __setitem__(self, /, x, val):
                self.__setv(x, val, True)
            def __setattr__(self, /, attr, val):
                if attr.startswith("_Wrapper__"):
                    return object.__setattr__(self, attr, val)
                self.__setv(attr, val, True)
        return Wrapper(obj, 0)
