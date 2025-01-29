from math import log10

SILVER = S = Sv = Slv = -2
GOLD = L = Gl = Gld = -1
BLACK = K = Bk = Blk = 0
BROWN = N = Br = Bwn = 1
RED = R = Rd = Red = 2
ORANGE = O = Og = Ong = 3
YELLOW = Y = Yl = Ylw = 4
GREEN = G = Gn = Grn = 5
BLUE = B = Bl = Blu = 6
VIOLET = PURPLE = V = P = Vl = Pr = Vlt = Prp = 7
GREY = GRAY = A = Gr = Gry = 8
WHITE = W = Wh = Whi = 9

names = ("BLACK", "BROWN", "RED", "ORANGE", "YELLOW",
         "GREEN", "BLUE", "VIOLET", "GREY", "WHITE",
         "SILVER", "GOLD")

DEFAULT_TOLERANCE = 0
tolerance = [DEFAULT_TOLERANCE] * 12
tolerance[BROWN] = 1
tolerance[RED] = 2
tolerance[ORANGE] = 3
tolerance[YELLOW] = 4
tolerance[GREEN] = 0.5
tolerance[BLUE] = 0.25
tolerance[VIOLET] = 0.1
tolerance[GRAY] = 0.05
tolerance[GOLD] = 5
tolerance[SILVER] = 10

def p10(n, off=0):
    p = int(log10(n or 1) + off)//3
    if p < 0: p = 0
    elif p > 3: p = 3
    return n / 10**(p * 3), (None, "k", "M", "B")[p]

GCLO = "\N{GREEK CAPITAL LETTER OMEGA}"
PlMnS = "\N{PLUS-MINUS SIGN}"

def read(d0, d1, x0, x1, tol=None):
    band = [d0, d1, x0, x1]
    init = d0*10 + d1
    if tol is None:
        mult = x0
        tol = x1
    else:
        init = init*10 + x0
        mult = x1
        band.append(tol)
    num, suf = p10(init * 10**mult, off = int(d0 == 0))
    suf = " " + suf if suf else ""
    print("  ".join(f"{names[d]} ({d})" for d in band))
    print(f"{num:g}{suf} {GCLO} {PlMnS} {tolerance[tol]}%")
