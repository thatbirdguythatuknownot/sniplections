def disjoint(ss):
    disj = {}
    for i, s1 in enumerate(ss):
        o1 = 1 << i
        s1 = {*s1}
        for o2, s2 in [*disj.items()]:
            if s1 > s2:
                s1 -= s2
                disj[o1 | o2] = s2
                del disj[o2]
                continue
            if s1 < s2:
                s2 -= s
                disj[o1 | o2] = s1
                break
            if s1 == s2:
                disj[o1 | o2] = s1
                del disj[o2]
                break
            if s := s1 & s2:
                s1 -= s
                s2 -= s
                disj[o1 | o2] = s
        else:
            disj[o1] = s1
    return disj

def counting(disj, li, k=1):
    total = 0
    if k == li:
        for o in disj:
            if k & o:
                total += disj[o]
        return total
    k2 = k * 2
    for o in disj:
        if not k & o:
            continue
        if k == o:
            total += disj[o] * counting(disj, li, k2)
            continue
        rec = disj[o]
        disj[o] -= 1
        total += rec * counting(disj, li, k2)
        disj[o] = rec
    return total

def solve(*inp):
    disj = disjoint(inp)
    for k in disj:
        disj[k] = len(disj[k])
    return counting(disj, 1 << len(inp)-1)
