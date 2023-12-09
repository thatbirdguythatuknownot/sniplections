def get_min_digits(x, digs="0123456789", n=None):
    base = len(digs)
    if n is None:
        n = len(x) >> 1
    check = {*map(digs.index, x)}
    l_check = len(check)
    R_l_check_d2_m1_TO_0 = range((l_check >> 1) - 1, -1, -1)
    assert -1 not in check # and l_check >= 2*n
    check_l = sorted(check)
    mfdd = md = float('inf')
    ma = mb = -1
    for i in range(l_check - 1):
        a0, b0 = check_l[i], check_l[i+1]
        if (fdd := b0 - a0) > mfdd:
            continue
        elif fdd < mfdd:
            mfdd = fdd
        check_filter = check - {a0, b0}
        a_digs, b_digs = [a0], [b0]
        for _ in range(n - 1):
            b_, a_ = min(check_filter), max(check_filter)
            check_filter -= {b_, a_}
            a_digs.append(a_)
            b_digs.append(b_)
            if not check_filter:
                break
        a = [digs[x] for x in a_digs]
        b = [digs[x] for x in b_digs]
        a_intval = sum(base**x * a_digs[~x] for x in R_l_check_d2_m1_TO_0)
        b_intval = sum(base**x * b_digs[~x] for x in R_l_check_d2_m1_TO_0)
        if (diff := abs(b_intval - a_intval)) < md:
            md, ma, mb = diff, a, b
    return md, mb, ma
