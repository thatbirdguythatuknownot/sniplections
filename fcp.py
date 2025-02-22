import itertools as it

def disjoint(ss):
    disj = {1 << i: {*s} for i, s in enumerate(ss)}
    cont = True
    while cont:
        cont = False
        for (o1, s1), (o2, s2) in it.combinations(disj.items(), 2):
            if s := s1 & s2:
                cont = True
                s1 -= s
                s2 -= s
                if not s1:
                    del disj[o1]
                if not s2:
                    del disj[o2]
                disj[o1 | o2] = s
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
        if k == o or k2 > o:
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
