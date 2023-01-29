from fractions import Fraction

alph = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

def expand(m, n, p):
    rp = range(p)
    finv_p = Fraction(1, p)
    l = []
    v = [m]
    while True:
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
        v.append(m)
    to_rep = l[v.index(m):]
    len_l = len(l)
    len_torep = len(to_rep)
    nrep = 0
    while len_l < 15:
        len_l += len_torep
        nrep += 1
    return (l + to_rep * nrep)[:15]

def itoo(arg):
    p, A = arg.split()
    m, n = A.split('/')
    return ''.join(alph[x] for x in expand(int(m), int(n), int(p))[::-1])
