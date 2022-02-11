from math import factorial

def choose(size, total):
    possibilities = [(total, [])]
    
    for _ in range(size - 1):
        possibilities = (
            (leftover - choice, values + [choice])
            for leftover, values in possibilities
            for choice in range(leftover + 1)
        )
    
    results = [values+[leftover] for leftover, values in possibilities]
    
    return list(reversed(results))

alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
def mexpand(alphb, n, m=None):
    a = []
    m = len(alphb) if m is None else m
    appn = a.append
    try:
        n = int(n)
    except:
        n = 1
    try:
        m = int(m)
    except:
        return 0
    if n == 0:
        return 1
    if n == 1:
        for i in range(m):
            appn(alphb[i])
        return '+'.join(a)
    for i in choose(m, n):
        numce, st, pos = 1,[],0
        apn = st.append
        for j in i:
            if j > 0:
                if j == 1:
                    apn(alphb[pos] if len(alphb[pos]) == 1 else f"({alphb[pos]})")
                else:
                    apn('^'.join([alphb[pos] if len(alphb[pos]) == 1 else f"({alphb[pos]})",str(j)]))
            numce *= factorial(j)
            pos += 1
        numce = int(factorial(n)/numce)
        if numce > 1:
            appn(''.join([str(numce)]+st))
        else:
            appn(''.join(t.replace("(", "").replace(")", "") if len(alphb[i]) == 1 else t for i, t in enumerate(st)))
    return ' + '.join(a)

def mterm(l, n, n2, m=None):
    try:
        return mexpand(l,n,m).split(" + ")[n2-1]
    except:
        return ""

def mcoefficient(m, n, n2):
    try:
        b = choose(m,n)[n2-1]
    except:
        return 0
    else:
        b[:] = map(lambda x: factorial(x), b)
        c = 1
        for t in b:
            c *= t
        return int(factorial(n)/c)

def expanded(l, n):
    res = mexpand(l, n)
    return f"({' + '.join(l)})^{n} = {res}"
