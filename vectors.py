import math
from dataclasses import dataclass
from decimal import Decimal, getcontext
from fractions import Fraction
from math import atan2, degrees, isnan, radians
from scinum import SciNum, scinum

def sqrt(x):
    if isinstance(x, Fraction):
        res = (Decimal(x.numerator) / x.denominator).sqrt()
    elif isinstance(x, SciNum):
        res = sqrt(x.num)
    else:
        res = Decimal(x).sqrt()
    res = res if res % 1 else int(res)
    return type(x)(res, x.n_SF) if isinstance(x, SciNum) else res

class Degrees(SciNum):
    def __init__(self, deg, n_SF=math.inf):
        super().__init__(deg, n_SF)
        self.num %= 360
    def __str__(self):
        return super().__str__() + "\xb0"

UNEXP_KW = "{} got an unexpected keyword argument '{:.200s}'"
MULTIPVAL_KW = "{} got multiple values for argument '{:.200s}'"

class Vector:
    def __init__(self, *args, **kwargs):
        is_magdir = None
        x = y = 0
        n_SF = math.inf
        l_args = len(args)
        if l_args == 3:
            if kwargs:
                kw = next(iter(kwargs))
                fm = MULTIPVAL_KW if kw in {"x", "y", "n_SF", "magnitude", "direc"} else UNEXP_KW
                raise TypeError(fm.format("Vector()", kw))
            x, y, n_SF = args
        elif l_args == 2:
            for kw, v in kwargs.items():
                if kw == "n_SF":
                    n_SF = v
                else:
                    fm = MULTIPVAL_KW if kw in {"x", "y", "magnitude", "direc"} else UNEXP_KW
                    raise TypeError(fm.format("Vector()", kw))
            x, y = args
        elif l_args == 1:
            gave_y = False
            options = {"x", "magnitude"}
            for kw, v in kwargs.items():
                if kw == "n_SF":
                    n_SF = v
                elif kw == "y":
                    if is_magdir:
                        raise TypeError(UNEXP_KW.format("Vector()", kw))
                    is_magdir = False
                    options.remove("magnitude")
                    y = v
                elif kw == "direc":
                    if is_magdir is False:
                        raise TypeError(UNEXP_KW.format("Vector()", kw))
                    is_magdir = True
                    options.remove("x")
                    y = v
                else:
                    fm = MULTIPVAL_KW if kw in options else UNEXP_KW
                    raise TypeError(fm.format("Vector()", kw))
            x, = args
        elif not l_args:
            for kw, v in kwargs.items():
                if kw == "n_SF":
                    n_SF = v
                elif kw == "x":
                    if is_magdir:
                        raise TypeError(UNEXP_KW.format("Vector()", kw))
                    is_magdir = False
                    x = v
                elif kw == "y":
                    if is_magdir:
                        raise TypeError(UNEXP_KW.format("Vector()", kw))
                    is_magdir = False
                    y = v
                elif kw == "magnitude":
                    if is_magdir is False:
                        raise TypeError(UNEXP_KW.format("Vector()", kw))
                    is_magdir = True
                    x = v
                elif kw == "direc":
                    if is_magdir is False:
                        raise TypeError(UNEXP_KW.format("Vector()", kw))
                    is_magdir = True
                    y = v
                else:
                    raise TypeError(UNEXP_KW.format("Vector()", kw))
        else:
            raise TypeError(
                f"Vector() takes 3 positional arguments but {l_args} were given"
            )
        if is_magdir:
            self.magnitude = scinum(x, n_SF, orig=x)
            n_SF = self.magnitude.n_SF
            self.direc = scinum(y, n_SF, orig=y)
            theta = radians(self.direc.num)
            self.x = scinum(t := self.magnitude*math.cos(theta), n_SF, orig=t)
            self.y = scinum(t := self.magnitude*math.sin(theta), n_SF, orig=t)
        else:
            self.x = scinum(x, n_SF, orig=x)
            self.y = scinum(y, n_SF, orig=y)
            n_SF = min(self.x.n_SF, self.y.n_SF)
            self.magnitude = SciNum(sqrt(self.x.num**2 + self.y.num**2),
                                    min(self.x.n_SF, self.y.n_SF))
            self.direc = Degrees(degrees(atan2(y, x)), n_SF)
        self.n_SF = n_SF
    def __add__(self, other):
        x = self.x + other.x
        y = self.y + other.y
        return Vector(x, y, min(self.n_SF, other.n_SF))
    __radd__ = __add__
    def __sub__(self, other):
        return self + -other
    def __mul__(self, other):
        return type(self)(self.x * other.x, self.y * other.y,
                          min(self.n_SF, other.n_SF))
    __rmul__ = __mul__
    def __truediv__(self, other):
        return type(self)(self.x / other.x, self.y / other.y,
                          min(self.n_SF, other.n_SF))
    def __rtruediv__(self, other):
        return type(self)(other.x / self.x, other.y / self.y,
                          min(self.n_SF, other.n_SF))
    def __floordiv__(self, other):
        return type(self)(self.x // other.x, self.y // other.y,
                          min(self.n_SF, other.n_SF))
    def __rfloordiv__(self, other):
        return type(self)(other.x // self.x, other.y // self.y,
                          min(self.n_SF, other.n_SF))
    def __mod__(self, other):
        return type(self)(self.x % other.x, self.y % other.y,
                          min(self.n_SF, other.n_SF))
    def __rmod__(self, other):
        return type(self)(other.x % self.x, other.y % self.y,
                          min(self.n_SF, other.n_SF))
    def dot(self, other):
        return SciNum(self.x*other.x + self.y*other.y,
                      min(self.n_SF, other.n_SF))
    def cross(self, other):
        return SciNum(self.x*other.y - self.y*other.x,
                      min(self.n_SF, other.n_SF))
    def __neg__(self):
        return Vector(-self.x, -self.y, n_SF)
    def __repr__(self):
        return f"Vector(x={self.x!r}, y={self.y!r}, direc={self.direc!r})"
    def __str__(self):
        return f"({self.x!s}, {self.y!s}) | {self.magnitude!s} -> {self.direc!s}"

if __name__ == "__main__":
    u = Vector(5, 8)
    v = Vector(6, 2)
    print(u.dot(v))
    print(Vector(5, direc=0))
