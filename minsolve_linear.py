from math import gcd, hypot

def solve(a, x, y):
    """
    Solve the linear equation Ax + By = C
        (order of parameters to function are C, A, B)
        such that (x, y) has the minimum possible distance to (0, 0)

    https://github.com/lapets/egcd/blob/main/src/egcd/egcd.py
    https://stackoverflow.com/questions/27458157
    https://math.stackexchange.com/questions/2674368
    """
    ox, oy = x, y
    x0, x1, y0, y1 = (1, 0, 0, 1)
    while y:
        x, q, y = y, x // y, x % y
        x0, x1 = x1, x0 - q*x1
        y0, y1 = y1, y0 - q*y1
    if x != 1:
        if a % x:
            raise ValueError(f"impossible to solve {ox}x + {oy}y = {a}")
        a //= x
    x0 *= a
    y0 *= a
    xygcd = gcd(ox, oy)
    xm = -ox // xygcd
    ym = oy // xygcd
    rx0 = x0
    ry0 = y0
    d = hypot(x0, y0)
    x0 += ym
    y0 += xm
    td = hypot(x0, y0)
    if cond := xm > 0 < ym or xm < 0 > ym:
        if td >= d:
            x0 = rx0 - ym
            y0 = ry0 - xm
            td = hypot(x0, y0)
            if td >= d:
                return res
            xm = -xm
            ym = -ym
    else:
        od = d
        ox0 = rx0
        oy0 = ry0
    while td < d:
        d = td
        rx0 = x0
        ry0 = y0
        x0 += ym
        y0 += xm
        td = hypot(x0, y0)
    if not cond:
        r2x0 = ox0
        r2y0 = oy0
        ox0 -= ym
        oy0 -= xm
        td2 = hypot(ox0, oy0)
        while td2 < od:
            od = td2
            r2x0 = ox0
            r2y0 = oy0
            ox0 -= ym
            oy0 -= xm
            td2 = hypot(ox0, oy0)
        if td2 <= td:
            return r2x0, r2y0
    return rx0, ry0
