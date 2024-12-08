import math
import re
from dataclasses import dataclass
from decimal import Decimal, getcontext
from fractions import Fraction
from math import atan, degrees, isnan, radians
from scinum import SCINUM_FORMAT, SciNum, parse_scinum_match, scinum, sqrt

DIREC_FORMAT = "N(?:orth)?|S(?:outh)?|E(?:ast)?|W(?:est)?"

direc_p = re.compile(fr"""
\s*                              # optional leading whitespace,
(?:                              # followed by...
    {SCINUM_FORMAT}              # number or fraction,
    °?                           # followed by an optional degrees symbol,
    \s*                          # then optional whitespace
)?                               # ...(all optional)
(?P<dir1>{DIREC_FORMAT})         # after, a direction,
\s*(?:\bOF\s+)?                  # then optional "OF" (e.g. 5° North of West),
(?P<dir2>{DIREC_FORMAT})?        # an optional other direction,
\s*                              # and finally, optional trailing whitespace
""", re.VERBOSE | re.IGNORECASE).fullmatch

DIR_QUAD = "ENWS"

DIRFULL_QUAD = "East", "North", "West", "South"

def get_quad(dir_name):
    if (quad := DIR_QUAD.find(dir_name)) >= 0:
        return quad
    return DIRFULL_QUAD.index(dir_name)

def get_deg(dir_name):
    return 90 * get_quad(dir_name)

def try_parse_direc(x, n_SF=-1):
    assert isinstance(x, str)
    if m := direc_p(x):
        a = m["dir1"]
        b = m["dir2"]

        if m["num"] is None: # number group
            if b is not None:
                return (get_deg(a) + get_deg(b)) >> 1, n_SF
            return get_deg(a), n_SF

        elif b is not None:
            p = get_quad(a)
            q = get_quad(b)
            d, n_SF = parse_scinum_match(m, n_SF)

            if p != q and (q + 1) % 4 != p:
                if abs(p - q) == 2:
                    # opposite directions, i.e. North-South, East-West
                    raise ValueError("cannot have opposite directions: "
                                     f"{DIRFULL_QUAD[p]}-{DIRFULL_QUAD[q]}")
                d = -d

            return d + 90*q, n_SF

    return x, n_SF

class Degrees(SciNum):
    def __init__(self, deg, n_SF=math.inf):
        if isinstance(deg, str):
            deg, n_SF = try_parse_direc(deg, n_SF)
        super().__init__(deg, n_SF)
        self.num %= 360
    def __str__(self):
        return super().__str__() + '°'

UNEXP_KW = "{} got an unexpected keyword argument '{:.200s}'"
MULTIPVAL_KW = "{} got multiple values for argument '{:.200s}'"

class Vector:
    def __init__(self, *args, **kwargs):
        is_magdir = None
        x = y = 0
        n_SF = None
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

        if n_SF is None:
            n_SF = math.inf

        if is_magdir:
            self.magnitude = scinum(x, n_SF, orig=x)
            self.direc = Degrees(y, n_SF)
            n_SF = self.magnitude.n_SF
            theta = radians(self.direc.num)
            self.x = scinum(t := self.magnitude*math.cos(theta), n_SF, orig=t)
            self.y = scinum(t := self.magnitude*math.sin(theta), n_SF, orig=t)

        else:
            self.x = scinum(x, n_SF, orig=x)
            self.y = scinum(y, n_SF, orig=y)
            n_SF = min(self.x.n_SF, self.y.n_SF)

            x = self.x.num
            y = self.y.num
            self.magnitude = SciNum(sqrt(x*x + y*y), n_SF)

            if not x:
                atan_res = 90 if y > 0 else -90
            else:
                atan_res = degrees(atan(y/x))

            self.direc = Degrees(atan_res, n_SF)
            if x < 0:
                self.direc += 180
            elif y < 0:
                self.direc += 360

        self.n_SF = n_SF

    def __add__(self, other):
        if isinstance(other, Vector):
            x = self.x + other.x
            y = self.y + other.y
            return type(self)(x, y, min(self.n_SF, other.n_SF))
        return type(self)(self.x + other, self.y + other, self.n_SF)

    __radd__ = __add__

    def __sub__(self, other):
        return self + -other

    def __rsub__(self, other):
        return -self + other

    def __mul__(self, other):
        if isinstance(other, Vector):
            return type(self)(self.x * other.x, self.y * other.y,
                              min(self.n_SF, other.n_SF))
        return type(self)(self.x * other, self.y * other, self.n_SF)

    __rmul__ = __mul__

    def __truediv__(self, other):
        if isinstance(other, Vector):
            return type(self)(self.x / other.x, self.y / other.y,
                              min(self.n_SF, other.n_SF))
        return type(self)(self.x / other, self.y / other, self.n_SF)

    def __rtruediv__(self, other):
        if isinstance(other, Vector):
            return type(self)(other.x / self.x, other.y / self.y,
                              min(self.n_SF, other.n_SF))
        return type(self)(other / self.x, other / self.y, self.n_SF)

    def __floordiv__(self, other):
        if isinstance(other, Vector):
            return type(self)(self.x // other.x, self.y // other.y,
                              min(self.n_SF, other.n_SF))
        return type(self)(self.x // other, self.y // other, self.n_SF)

    def __rfloordiv__(self, other):
        if isinstance(other, Vector):
            return type(self)(other.x // self.x, other.y // self.y,
                              min(self.n_SF, other.n_SF))
        return type(self)(other // self.x, other // self.y, self.n_SF)

    def __mod__(self, other):
        return self - (self // other)

    def __rmod__(self, other):
        return other - (other // self)

    def __pow__(self, other, z=None):
        return type(self)(pow(self.x, other, z), pow(self.y, other, z),
                          min(self.n_SF, other.n_SF))

    def dot(self, other):
        return SciNum(self.x*other.x + self.y*other.y,
                      min(self.n_SF, other.n_SF))
    d = dot

    def cross(self, other):
        return SciNum(self.x*other.y - self.y*other.x,
                      min(self.n_SF, other.n_SF))
    c = cross

    def perp(self):
        return type(self)(self.y, -self.x, self.n_SF)
    P = perp

    def conjugate(self):
        return type(self)(self.x, -self.y, self.n_SF)
    C = conjugate

    def __neg__(self):
        return Vector(-self.x, -self.y, self.n_SF)

    def __repr__(self):
        return f"Vector(x={self.x!r}, y={self.y!r}, direc={self.direc!r})"

    def __str__(self):
        return f"({self.x!s}, {self.y!s}) | {self.magnitude!s} -> {self.direc!s}"

if __name__ == "__main__":
    u = Vector(5, 8)
    v = Vector(6, 2)
    print(u.dot(v))
    print(Vector(5, direc=0))
    a = Vector(35, direc=25, n_SF=-1)
    b = Vector(15, direc=90 - 10, n_SF=-1)
    c = Vector(SciNum(20, n_SF=2), direc=360 - 43, n_SF=-1)
    d = Vector(SciNum(40, n_SF=2), direc=180 + 28, n_SF=-1)
    print(a+b)
    print(a+c)
    print(c+d)
    print(Degrees("NW"))
    print(Degrees("E"))
    print(Degrees("7 E of S"))
    print(Degrees("7 W of S"))
    print(Degrees("-8 N of E"))
    print(Degrees("26 S of E"))
    print(Degrees("33 E of N"))
    print(Degrees("10 W of N"))
    print(Degrees("15.2/3.40 E of S", n_SF=-1))
    print(-Vector(5, 7))
    print(Vector(5, 7).perp())
