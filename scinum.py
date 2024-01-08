import math
import numbers
from decimal import Decimal, getcontext
from fractions import Fraction
from math import ceil, floor, log10 as m_log10, isinf, isnan, trunc

def log10(x):
    return x.log10() if isinstance(x, Decimal) else m_log10(x)

def num_dec_zeros(decimal):
    if isnan(decimal):
        return math.nan
    if isinf(decimal):
        return -math.inf
    return math.inf if decimal == 0 else -floor(log10(abs(decimal))) - 1

def nint_place_splitted(x):
    if truncated := trunc(abs(x)):
        s = "%i" % truncated
        t = s.rstrip('0')
        return (t_len := len(t), len(s) - t_len)
    return (0, 0)

def nint_places(x):
    if truncated := trunc(abs(x)):
        return len("%i" % truncated)
    return 0

def n_SF_round(x, n_SF):
    if int_places := nint_places(x):
        return n_SF - int_places
    if not isinf(dzeros := num_dec_zeros(x)):
        return n_SF + dzeros
    return 0

def ndec_places(x):
    if isinstance(x, SciNum):
        return ndec_places(x.num)
    if isinstance(x, int):
        return 0
    if isinstance(x, Decimal):
        return -x.as_tuple().exponent
    if isinstance(x, Fraction):
        y = x.denominator
        fives = 0
        while not y % 5:
            fives += 1
            y //= 5
        twos = (y & -y).bit_length() - 1
        if (y >> twos) != 1:
            return math.inf
        return max(twos, fives)
    j = 0
    while x := x % 1:
        j += 1
        x *= 10
    return j

def n_SFdec(x, n_SF):
    if int_places := nint_places(x):
        n_SF -= int_places
    elif not isinf(dzeros := num_dec_zeros(x)):
        n_SF += dzeros
    return max(ndec_places(x), n_SF)

def get_n_SF(x):
    if d_len := ndec_places(x):
        return nint_places(x) + d_len
    return nint_place_splitted(x)[0]

def scinum(*args, orig):
    if isinstance(orig, SciNum):
        return type(orig)(*args)
    return SciNum(*args)

def coerce(x, y):
    if isinstance(x, SciNum):
        a, b = coerce(x.num, y)
        return (scinum(a, x.n_SF, orig=x), scinum(b, orig=y))
    if isinstance(y, SciNum):
        a, b = coerce(x, y.num)
        return (scinum(a, orig=x), scinum(b, y.n_SF, orig=y))
    if isinstance(x, Fraction):
        if not isinstance(y, (numbers.Integral, Fraction)):
            return (x, Fraction(y))
    elif isinstance(y, Fraction):
        return (Fraction(x), y)
    elif isinstance(x, Decimal):
        if not isinstance(y, (numbers.Integral, Decimal)):
            return (x, Decimal(y))
    elif isinstance(y, Decimal):
        if not isinstance(x, numbers.Integral):
            return (Decimal(x), y)
    return (x, y)

class SciNum(numbers.Real):
    def __init__(self, num, n_SF=math.inf):
        if isinstance(num, SciNum):
            if isinf(n_SF) or n_SF < 0:
                n_SF = num.n_SF
            num = num.num
        if isinstance(num, str):
            num = Fraction(num)
        elif not isinstance(num, (numbers.Integral, Fraction, Decimal)):
            num = Decimal(num)
        if not num:
            n_SF = not isinf(n_SF) and bool(n_SF)
        elif n_SF < 0:
            n_SF = get_n_SF(num)
        if not isinf(n_SF):
            sf_round = n_SF_round(num, n_SF)
            if isinstance(num, Decimal):
                if (to_set := max(sf_round, n_SF)) and to_set > 0:
                    getcontext().prec = max(getcontext().prec, to_set)
                getcontext().rounding = ROUND_HALF_UP
            num = round(num, sf_round)
        self.num = num
        self.n_SF = n_SF
    def __add__(self, other):
        A = self.num
        B = other.num if isinstance(other, SciNum) else other
        n = min(n_SFdec(A, self.n_SF),
                n_SFdec(B, other.n_SF if isinstance(other, SciNum) else math.inf))
        t = self.num + B
        if not isinf(n):
            if isinstance(t, Decimal):
                if n and n > 0:
                    getcontext().prec = max(getcontext().prec, n)
                getcontext().rounding = ROUND_HALF_UP
            t = round(t, n)
        return type(self)(t, n + nint_places(t))
    __radd__ = __add__
    def __sub__(self, other):
        return self + -other
    def __rsub__(self, other):
        return other + -self
    def __mul__(self, other):
        A, B = coerce(self, other)
        return type(self)(A.num * B.num, min(A.n_SF, B.n_SF))
    __rmul__ = __mul__
    def __truediv__(self, other):
        A, B = coerce(self, other)
        return type(self)(A.num / B.num, min(A.n_SF, B.n_SF))
    def __rtruediv__(self, other):
        B, A = coerce(self, other)
        return type(self)(A.num / B.num, min(A.n_SF, B.n_SF))
    def __floordiv__(self, other):
        A, B = coerce(self, other)
        return type(self)(A.num // B.num, min(A.n_SF, B.n_SF))
    def __rfloordiv__(self, other):
        B, A = coerce(self, other)
        return type(self)(A.num // B.num, min(A.n_SF, B.n_SF))
    def __mod__(self, other):
        A, B = coerce(self, other)
        return type(self)(A.num % B.num, min(A.n_SF, B.n_SF))
    def __rmod__(self, other):
        B, A = coerce(self, other)
        return type(self)(A.num % B.num, min(A.n_SF, B.n_SF))
    def __pow__(self, other, z=None):
        A, B = coerce(self, other)
        return type(self)(pow(A.num, B.num, z), min(A.n_SF, B.n_SF))
    def __rpow__(self, other):
        B, A = coerce(self, other)
        return type(self)(A.num ** B.num, min(A.n_SF, B.n_SF))
    def __neg__(self):
        return type(self)(-self.num, self.n_SF)
    def __pos__(self):
        return type(self)(+self.num, self.n_SF)
    def __abs__(self):
        return type(self)(abs(self.num), self.n_SF)
    def __round__(self, ndigits=None):
        return type(self)(round(self.num, ndigits), self.n_SF)
    def __ceil__(self):
        return type(self)(ceil(self.num), self.n_SF)
    def __floor__(self):
        return type(self)(floor(self.num), self.n_SF)
    def __trunc__(self):
        return type(self)(trunc(self.num), self.n_SF)
    def __float__(self):
        return float(self.num)
    def __eq__(self, other):
        if self.num == other:
            return not isinstance(other, SciNum) or self.n_SF == other.n_SF
        return False
    def __lt__(self, other):
        if self.num < other:
            return not isinstance(other, SciNum) or self.n_SF <= other.n_SF
        return False
    def __le__(self, other):
        return self == other or self < other
    def __repr__(self):
        if not isinf(self.n_SF):
            sf_s = f", n_SF={self.n_SF}"
        else:
            sf_s = ""
        return f"{type(self).__name__}({self.num!r}{sf_s})"
    def __str__(self):
        if not self.num:
            return "0." if self.n_SF else "0"
        if isinf(self.n_SF):
            fmt = "17g"
        else:
            fmt = f"{max(self.n_SF - nint_places(self.num), 0)}f"
        return f"{self.num:.{fmt}}"

if __name__ == "__main__":
    print(SciNum("890", 3) * SciNum("22", 2))
    print(SciNum("890", 3) * SciNum("12", 2))
    print(SciNum("5", 3) * SciNum("2", 2))
    print(SciNum("5", 3) / SciNum("2", 2))
    print(SciNum("1", 3) / SciNum("7", 2))
    print(repr(SciNum(1/8, -1)))
