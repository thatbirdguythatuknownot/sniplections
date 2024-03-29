from fractions import Fraction

alph = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

def expand_cycle(m, n, p):
    rp = range(p)
    finv_p = Fraction(1, p)
    l = []
    v = {m: 0}
    idx = 0
    while idx := idx + 1:
        m_p = m%p
        if m_p == 0:
            l.append(0)
            m //= p
        else:
            for z in rp:
                if n * z % p == m_p:
                    break
            l.append(z)
            m = (finv_p * (Fraction(m, n) - z)).numerator
        if m in v:
            break
        v[m] = idx
    i = v[m]
    return l[:i], l[i:]

def expand(m, n, p, limit=15):
    p_to_limit = p**limit
    x = m * pow(n, -1, p_to_limit) % p_to_limit
    return [x // p**k % p for k in range(limit)]

def itoo_cycle(arg):
    p, A = arg.split()
    m, n = A.split('/')
    unique, torep = expand_cycle(int(m), int(n), int(p))
    unique.reverse()
    torep.reverse()
    return f"({''.join(alph[x] for x in torep)})" + ''.join(alph[x] for x in unique)

def itoo(arg):
    p, A, lim = arg.split()
    m, n = A.split('/')
    return ''.join(alph[x] for x in expand(int(m), int(n), int(p), int(lim))[::-1])
