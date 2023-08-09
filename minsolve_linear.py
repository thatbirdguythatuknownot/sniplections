from math import dist, gcd

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
        x, (q, y) = y, divmod(x, y)
        x0, x1 = x1, x0 - q*x1
        y0, y1 = y1, y0 - q*y1
    if a % x:
        raise ValueError(f"impossible to solve {ox}x + {oy}y = {a}")
    a //= x
    x0 *= a
    y0 *= a
    xygcd = gcd(ox, oy)
    xm, ym = -ox // xygcd, oy // xygcd
    t = x0 + ym, y0 + xm
    td = dist(t, (0, 0))
    d = dist(res := (x0, y0), (0, 0))
    if cond := xm > 0 < ym or xm < 0 > ym:
        if td >= d:
            t = x0 - ym, y0 - xm
            td = dist(t, (0, 0))
            if td >= d:
                return res
            xm = -xm
            ym = -ym
    else:
        od = d
        ox0 = x0
        oy0 = y0
    while td < d:
        d = td
        x0, y0 = res = t
        t = x0 + ym, y0 + xm
        td = dist(t, (0, 0))
    if not cond:
        d2 = od
        x0 = ox0
        y0 = oy0
        t = x0 - ym, y0 - xm
        td2 = dist(t, (0, 0))
        while td2 < d2:
            d2 = td2
            x0, y0 = t
            t = x0 - ym, y0 - xm
            td2 = dist(t, (0, 0))
        if td2 <= td:
            return x0, y0
    return res
