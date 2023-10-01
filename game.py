from copy import deepcopy
from fractions import Fraction
from math import ceil, trunc
from os import system
from traceback import print_exception

system('')

INF = float('inf')

clamp = lambda x, a, b: b if x >= b else a if x < a else x
def func(step, game, cond, x, y):
    if cond:
        t = y, x
        return step[t][0] if t in step else game[y][x]

def update(step, game, x, y, a, v, u):
    game[y][x] += u
    step[y, x] = (a, game[y][x])

def f1():
    global counter
    counter -= 1
    return counter >= 0

def f2():
    t = game[0][0]
    for row in game:
        for v in row:
            if v != t: return True
    return False

def fraction(x, y=None):
    if 1:
        if y is not None: return x/y
        else:
            try:
                return int(x)
            except ValueError:
                return float(x)
    if y is not None and type(x) is Fraction:
        return x/y
    res = Fraction(x, y)
    if res.denominator == 1: return res.numerator
    return res

def fton(x):
    if isinstance(x, Fraction):
        if x.denominator == 1: return x.numerator
        x = f"{trunc(x)}{repr(float(x%1))[1:]}"
    return x

N = 20
while True:
    try:
        N = int(I := input("Size of square [DEFAULT 20]> ").strip())
    except ValueError as e:
        if not I: break
        print('please enter a valid number!')
        e.__traceback__ = None
        print_exception(e)
    except EOFError:
        break
    else:
        break

NSQ = N*N
LI = N - 1
RN = range(N)
game = [[0]*N for _ in RN]
nonsim_steps = set()
steps = []
nsteps = 0
i = 0
TE = 0
ns = 1
flip = False
onstep = None
display_only = False
maxn = minn = 0
while True:
    try:
        system('cls')
        if onstep is not None:
            game, i, minn, maxn = onstep
            onstep = None
        for row in game:
            for x in row:
                num = (x - minn if minn < 0 else x)*256 / (maxn - minn or 1)
                rgb = round(num if flip else 256-num)
                if rgb < 0: rgb = 0
                elif rgb > 255: rgb = 255
                print(f"\x1b[48;2;{rgb};{rgb};{rgb}m  ", end="")
            print("\x1b[0m")
        print(minn, "(min,", ("white", "black")[flip] + ") ->", maxn, "(max,", ("black", "white")[flip] + ")")
        counter = ns
        Cf = f1
        unchanged = not display_only
        while unchanged:
            a = None
            try:
                a, *b = map(fraction, t := input(
                    '\ninput X Y[ V] OR'
                    '\ninput N to set all square values to N OR'
                    '\nCtrl+B to run the simulation and display until Ctrl+C is pressed OR'
                    '\nCtrl+E[ N] to exaggerate the value discrepancies by N [DEFAULT 1000] OR'
                    '\nCtrl+N[ N] to multiply all values by N [DEFAULT 100] OR'
                    '\nCtrl+O[ N] to normalize all values to N [DEFAULT total energy] OR'
                    '\nCtrl+P to print the next state index and number of states OR'
                    '\nCtrl+Q to print the total energy and ACTUAL total energy OR'
                    '\nCtrl+R[ N] to set the number of steps per Enter to N [DEFAULT 1] OR'
                    '\nCtrl+S to get the maximum and minimum values OR'
                    '\nCtrl+S X Y to get the value at square (X, Y) OR'
                    '\nCtrl+S X to get the values at column X OR'
                    '\nCtrl+T Y to get the values at row Y OR'
                    '\nCtrl+U N to run the simulation N times OR'
                    '\nCtrl+U to run the simulation until balance is achieved OR'
                    '\nCtrl+W to flip the signs of the values OR'
                    '\nCtrl+W+W to flip black/white on the display OR'
                    '\nCtrl+X to clear the simulation OR'
                    '\nCtrl+Y[ N] to redo N steps [DEFAULT 1] OR'
                    '\nCtrl+Y+Y[ N] to preload N steps [DEFAULT 500] OR'
                    '\nCtrl+Z to undo a step OR'
                    '\nsimply Enter to continue simulation> '
                ).strip().split())
                b, *c = b
            except ValueError as e:
                if not t: break
                if len(t) == 1:
                    if t[0] == '\x19':
                        if i == nsteps:
                            print("cannot go forward! no next states")
                            unchanged = 1
                        else:
                            step, TE = steps[i]
                            for (y, x), (_, v) in step.items():
                                game[y][x] = v
                            i += 1
                            unchanged = False
                    elif t[0] == '\x18':
                        a = 0
                    elif t[0] == '\x13':
                        A = -INF
                        B = INF
                        Ap = Bp = None
                        for y, row in enumerate(game):
                            for x, v in enumerate(row):
                                if v > A: A = v; Ap = (x, y)
                                if v < B: B = v; Bp = (x, y)
                        print(Ap, fton(A))
                        print(Bp, fton(B))
                        unchanged = 1
                    elif t[0] == '\x15':
                        Cf = f2
                        break
                    elif t[0] == '\x10':
                        print(i, nsteps)
                        unchanged = 1
                    elif t[0] == '\x11':
                        TE2 = 0
                        for row in game:
                            for v in row:
                                TE2 += v
                        print(fton(TE), fton(TE2))
                        unchanged = 1
                    elif t[0] == '\x0e':
                        step = {}
                        for y, row in enumerate(game):
                            for x, v in enumerate(row):
                                step[y, x] = (v, v := v * 100)
                                row[x] = v
                        steps[i:] = (step, TE),
                        nonsim_steps.add(i)
                        nsteps = i = i + 1
                        TE *= 100
                        unchanged = False
                    elif t[0] == '\x05':
                        step = {}
                        TE2 = 0
                        for row in game:
                            for v in row:
                                TE2 += v
                        TE2 = fraction(TE2, NSQ)
                        for y, row in enumerate(game):
                            for x, v in enumerate(row):
                                row[x] = v2 = TE2 + (v - TE2)*1000
                                step[y, x] = (v, v2)
                        steps[i:] = (step, TE),
                        nonsim_steps.add(i)
                        nsteps = i = i + 1
                        unchanged = False
                    elif t[0] == '\x0f':
                        step = {}
                        TE2 = 0
                        for row in game:
                            for v in row:
                                TE2 += v
                        if not TE2: TE2 = 1
                        z = -(TE == 0)
                        n = TE or 1
                        for y, row in enumerate(game):
                            for x in RN:
                                step[y, x] = (row[x], v := fraction(row[x]*n, TE2) - z)
                                row[x] = v
                        steps[i:] = (step, TE),
                        nonsim_steps.add(i)
                        nsteps = i = i + 1
                        unchanged = False
                    elif t[0] == '\x12':
                        ns = counter = 1
                        print('set number of steps per Enter')
                        unchanged = 1
                    elif t[0] == '\x17':
                        step = {}
                        for y in RN:
                            row = game[y]
                            for x, v in enumerate(row):
                                step[y, x] = (v, -v)
                                row[x] = -v
                        steps[i:] = (step, TE),
                        nonsim_steps.add(i)
                        nsteps = i = i + 1
                        TE = -TE
                        unchanged = False
                    elif t[0] == '\x17\x17':
                        flip = not flip
                        unchanged = False
                    elif t[0] == '\x19\x19':
                        counter = 500
                        onstep = deepcopy(game), i, minn, maxn
                        break
                    elif t[0] == '\x02':
                        display_only = True
                        break
                    if a is not None:
                        step = {}
                        for y in RN:
                            row = game[y]
                            for x in RN:
                                if row[x] != a:
                                    step[y, x] = (row[x], a)
                                    row[x] = a
                        steps[i:] = (step, TE),
                        nonsim_steps.add(i)
                        nsteps = i = i + 1
                        maxn = minn = a
                        TE = a * NSQ
                        unchanged = False
                elif t:
                    if t[0] == '\x13':
                        try:
                            a = int(t[1])
                            b = int(t[2])
                        except IndexError:
                            if a > LI:
                                print("out of range!")
                            else:
                                for row in game:
                                    print(fton(row[a]))
                            unchanged = 1
                        except ValueError as E:
                            e = E
                        else:
                            if a > LI or b > LI:
                                print("out of range!")
                            else:
                                print(fton(game[b][a]))
                            unchanged = 1
                    elif t[0] == '\x14':
                        try:
                            a = int(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            if a > LI:
                                print("out of range!")
                            else:
                                for v in game[a]:
                                    print(fton(v))
                            unchanged = 1
                    elif t[0] == '\x15':
                        try:
                            counter = int(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            break
                    elif t[0] == '\x0e':
                        try:
                            n = fraction(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            step = {}
                            for y, row in enumerate(game):
                                for x, v in enumerate(row):
                                    step[y, x] = (v, v := v * n)
                                    row[x] = v
                            steps[i:] = (step, TE),
                            nonsim_steps.add(i)
                            nsteps = i = i + 1
                            TE *= n
                            unchanged = False
                    elif t[0] == '\x05':
                        try:
                            n = int(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            step = {}
                            TE2 = 0
                            for row in game:
                                for v in row:
                                    TE2 += v
                            TE2 = fraction(TE2, NSQ)
                            for y, row in enumerate(game):
                                for x, v in enumerate(row):
                                    row[x] = v2 = TE2 + (v - TE2)*n
                                    step[y, x] = (v, v2)
                            steps[i:] = (step, TE),
                            nonsim_steps.add(i)
                            nsteps = i = i + 1
                            unchanged = False
                    elif t[0] == '\x0f':
                        try:
                            n = fraction(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            step = {}
                            TE2 = 0
                            for row in game:
                                for v in row:
                                    TE2 += v
                            if not TE2: TE2 = 1
                            z = -(n == 0)
                            n = n or 1
                            for y, row in enumerate(game):
                                for x in RN:
                                    step[y, x] = (row[x], v := fraction(row[x]*n, TE2) - z)
                                    row[x] = v
                            steps[i:] = (step, TE),
                            nonsim_steps.add(i)
                            nsteps = i = i + 1
                            TE = n
                            unchanged = False
                    elif t[0] == '\x12':
                        try:
                            ns = int(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            counter = ns
                            print('set number of steps per Enter')
                            unchanged = 1
                    elif t[0] == '\x19':
                        try:
                            stp = int(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            j = clamp(i + stp, 0, nsteps)
                            if c := i > j:
                                i -= 1
                                j -= 1
                                dirc = -1
                            else:
                                dirc = 1
                            for k in range(i, j, dirc):
                                step, TE = steps[k]
                                for (y, x), v in step.items():
                                    game[y][x] = v[not c]
                            i = j + c
                            unchanged = False
                    elif t[0] == '\x19\x19':
                        try:
                            counter = int(t[1])
                        except ValueError as E:
                            e = E
                        else:
                            onstep = deepcopy(game), i, minn, maxn
                            break
                if unchanged is True:
                    e.__traceback__ = None
                    print_exception(e)
                elif unchanged:
                    unchanged = True
            except EOFError:
                if i:
                    i -= 1
                    step, TE = steps[i]
                    for (y, x), (v, _) in step.items():
                        game[y][x] = v
                    unchanged = False
                else:
                    print("cannot reverse! no previous states")
            else:
                if a > LI or b > LI:
                    print("out of range!")
                    unchanged = 1
                else:
                    v = game[b][a]
                    v2 = None
                    if not c:
                        v2 = v + 1
                        game[b][a] = v2
                        TE += 1
                    elif c[0] != v:
                        TE -= v
                        game[b][a] = v2 = c[0]
                        TE += c[0]
                    if v2 is not None:
                        steps[i:] = ({(b, a): (v, v2)}, TE),
                        nonsim_steps.add(i)
                        nsteps = i = i + 1
                    unchanged = False
        else:
            if not display_only:
                continue
        print("press Ctrl+C to break...")
        try:
            maxn = -INF
            minn = INF
            while Cf():
                if i < nsteps and i not in nonsim_steps:
                    step, TE = steps[i]
                    for (y, x), (_, v) in step.items():
                        game[y][x] = v
                        if v > maxn: maxn = v
                        if v < minn: minn = v
                    i += 1
                    continue
                step = {}
                #TE2 = 0
                for y in RN:
                    Ya = y != 0
                    Yb = y != LI
                    for x in RN:
                        v = ov = func(step, game, True, x, y)
                        Xa = x != 0
                        Xb = x != LI
                        T = [
                             (func(step, game, c, a, b), a, b)
                             for c, a, b in (
                                 (Xa and Ya, x - 1, y - 1),
                                 (Xa       , x - 1, y    ),
                                 (Ya       , x    , y - 1),
                                 (Xb and Yb, x + 1, y + 1),
                                 (Xb       , x + 1, y    ),
                                 (Yb       , x    , y + 1),
                                 (Xa and Yb, x - 1, y + 1),
                                 (Xb and Ya, x + 1, y - 1),
                             )
                        ]
                        U = [x is not None and x < v for x, _, _ in T]
                        r = sum(U)
                        if r:
                            bal = fraction(sum(V := [x for x, _, _ in T if x is not None]) + v, len(V) + 1)
                            v = fraction(v - bal, r + 1)
                            for (m, a, b), c in zip(T, U):
                                if c:
                                    update(step, game, a, b, m, ov, v)
                                    vv = game[b][a]
                                    if vv > maxn: maxn = vv
                                    if vv < minn: minn = vv
                            v = ov - v*r
                            if v != ov:
                                t = (y, x)
                                if t not in step: step[t] = (ov, v)
                                game[y][x] = v
                            if v > maxn: maxn = v
                            if v < minn: minn = v
                        #TE2 += v
                steps[i:] = (step, TE),
                if i in nonsim_steps: nonsim_steps.remove(i)
                nsteps = i = i + 1
        except KeyboardInterrupt:
            pass
        #try: input()
        #except EOFError: pass
    except KeyboardInterrupt:
        if display_only:
            display_only = False
            continue
        raise
