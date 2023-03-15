from math import factorial, prod
from re import compile

term_coef = compile(r'(-?\d+)?[A-Za-z_]+$|(-?\d+)$').match

def choose(size, total):
    possibilities = [(total, [])]
    
    for _ in range(size - 1):
        possibilities = [
            (leftover - choice, values + [choice])
            for leftover, values in possibilities
            for choice in range(leftover + 1)
        ]
    
    results = [values.append(leftover) or values for leftover, values in possibilities]
    
    return results[::-1]

alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
def mexpand(alphb, n, m=None, *, Fc={0: 1, 1: 1}):
    a = []
    appn = a.append
    try:
        n = int(n)
    except:
        return '+'.join(alphb[:m])
    if m is None:
        m = len(alphb)
    else:
        try:
            m = int(m)
        except:
            return 0
    if n == 0:
        return 1
    if n == 1:
        return '+'.join(alphb[:m])
    alphb_coef = []
    alphb_vars = []
    apn_0 = alphb_coef.append
    apn_1 = alphb_vars.append
    fact_n = Fc.get(n)
    if fact_n is None: fact_n = Fc[n] = factorial(n)
    for th in alphb:
        mat = term_coef(th)
        if not mat:
            raise ValueError("terms must follow <integer?><variables> OR <integer> format")
        coef = mat.group(1)
        if coef is None:
            coef = mat.group(2)
            if coef is None:
                apn_0(1)
                apn_1(th)
                continue
        apn_0(int(coef))
        apn_1(th[len(coef):])
    for i in choose(m, n):
        numce, st, pos = 1,{},0
        coef_mult = 1
        for j in i:
            if j > 0:
                th = alphb_vars[pos]
                if th:
                    th_s = th if len(th) == 1 else f"({th})"
                    if th_s in st:
                        st[th_s] += 1
                    else:
                        st[th_s] = j
                val = alphb_coef[pos]
                if not th: val **= j
                coef_mult *= val
            fact_j = Fc.get(j)
            if fact_j is None: fact_j = Fc[j] = factorial(j)
            numce *= fact_j
            pos += 1
        numce = fact_n//numce
        s = ''.join([(f'{x}^{j}' if j != 1 else x) if j > 0 else '' for x, j in st.items()])
        if numce > 1 or coef_mult > 1:
            appn(f"{numce*coef_mult}{s}")
        else:
            appn(s or "1")
    return ' + '.join(a)

def mterm(l, n, n2, m=None):
    try:
        return mexpand(l,n,m).split(" + ")[n2]
    except:
        return ""

def mcoefficient(m, n, n2):
    try:
        b = choose(m,n)[n2]
        return factorial(n)//prod(map(factorial, b))
    except:
        return 0

def expanded(l, n):
    res = mexpand(l, n)
    return f"({' + '.join(l)})^{n} = {res}"
