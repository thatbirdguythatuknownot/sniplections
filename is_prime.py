from cc import int_btshift # note that this requires /c_extensions/cc.c
from random import randrange

def is_prime(n, k=100):
    if n in {2, 3, 5, 7, 11, 13, 17, 19}:
        return True
    if n <= 20 or n % 6 not in {1, 5}:
        return False
    d, s = int_btshift(n_1 := n - 1)
    check = {1, n_1}
    for _ in range(k):
        a = randrange(2, n_1)
        x = pow(a, d, n)
        if x in check:
            continue
        for _ in range(s - 1):
            x = x*x % n
            if x == n_1:
                break
        else:
            return False
    return True
