from random import SystemRandom, randint
from math import ceil

def inc_round(value, inc):
    minm = value - value%inc
    maxm = minm + inc
    return minm if maxm - value > value - minm else maxm

def fmt_time(secs):
    ret_l = []
    days = hours = mins = 0
    if secs >= 86400:
        days, secs = divmod(secs, 86400)
        ret_l.append(f"{days}d")
    if secs >= 3600:
        hours, secs = divmod(secs, 3600)
        ret_l.append(f"{hours}h")
    if secs >= 60:
        mins, secs = divmod(secs, 60)
        ret_l.append(f"{mins}m")
    if secs:
        ret_l.append(f"{secs}s")
    return ' '.join(ret_l)

def init_funcs(MIN_EXP_GET=15, MAX_EXP_GET=25, coef0=5/3, coef1=45/2, coef2=455/6, SEC_COOL=60, ROUND_EXP=5):
    AVG_EXP_GET = (MIN_EXP_GET + MAX_EXP_GET) / 2
    
    TIME_FMT = fmt_time(SEC_COOL)
    
    def get_level_exp(level):
        return inc_round(coef0*level**3 + coef1*level*level + coef2*level, ROUND_EXP)
    
    def get_required_minutes(needed_level, current_level, current_exp):
        needed_exp = get_level_exp(needed_level) - get_level_exp(current_level) - current_exp
        print(f"{needed_exp = }")
        if TIME_FMT:
            print(f"NOTE: have to wait {TIME_FMT} for a message to get exp again\n")
            print(f"Best case: {fmt_time((bc := ceil(needed_exp/MAX_EXP_GET)) * SEC_COOL)} ({bc} messages)\nWorst case: {fmt_time((wc := ceil(needed_exp/MIN_EXP_GET)) * SEC_COOL)} ({wc} messages)\n")
            print(f"average time (nonsimulated, {AVG_EXP_GET:g} exp / message): {fmt_time((avg := ceil(needed_exp / AVG_EXP_GET)) * SEC_COOL)} ({avg} messages)\n")
            cryptogen = SystemRandom()
            a = globals()['a'] = [(x := 0, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            b = globals()['b'] = [(x := 0, [(y := randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            print(f"--A--\n    average time (simulated, cryptographically secure): {fmt_time((a_ := ceil(sum([*map(len, a)]) / len(a))) * SEC_COOL)} ({a_} messages)\n")
            print(f"--B--\n    average time (simulated, normal pseudorandom): {fmt_time((b_ := ceil(sum([*map(len, b)]) / len(b))) * SEC_COOL)} ({b_} messages)\n")
        else:
            print(f"Best case: {ceil(needed_exp/MAX_EXP_GET)} messages\nWorst case: {ceil(needed_exp/MIN_EXP_GET)} messages\n")
            print(f"average time (nonsimulated, {AVG_EXP_GET:g} exp / message): {ceil(needed_exp / AVG_EXP_GET)} messages\n")
            cryptogen = SystemRandom()
            a = globals()['a'] = [(x := 0, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            b = globals()['b'] = [(x := 0, [(y := randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            print(f"--A--\n    average time (simulated, cryptographically secure): {ceil(sum([*map(len, a)]) / len(a))} messages\n")
            print(f"--B--\n    average time (simulated, normal pseudorandom): {ceil(sum([*map(len, b)]) / len(b))} messages\n")
    
    def get_required_minutes_exp(needed_level, current_exp):
        needed_exp = get_level_exp(needed_level) - current_exp
        print(f"{needed_exp = }")
        if TIME_FMT:
            print(f"NOTE: have to wait {TIME_FMT} for a message to get exp again\n")
            print(f"Best case: {fmt_time((bc := ceil(needed_exp/MAX_EXP_GET)) * SEC_COOL)} ({bc} messages)\nWorst case: {fmt_time((wc := ceil(needed_exp/MIN_EXP_GET)) * SEC_COOL)} ({wc} messages)\n")
            print(f"average time (nonsimulated, {AVG_EXP_GET:g} exp / message): {fmt_time((avg := ceil(needed_exp / AVG_EXP_GET)) * SEC_COOL)} ({avg} messages)\n")
            cryptogen = SystemRandom()
            a = globals()['a'] = [(x := 0, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            b = globals()['b'] = [(x := 0, [(y := randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            print(f"--A--\n    average time (simulated, cryptographically secure): {fmt_time((a_ := ceil(sum([*map(len, a)]) / len(a))) * SEC_COOL)} ({a_} messages)\n")
            print(f"--B--\n    average time (simulated, normal pseudorandom): {fmt_time((b_ := ceil(sum([*map(len, b)]) / len(b))) * SEC_COOL)} ({b_} messages)\n")
        else:
            print(f"Best case: {ceil(needed_exp/MAX_EXP_GET)} messages\nWorst case: {ceil(needed_exp/MIN_EXP_GET)} messages\n")
            print(f"average time (nonsimulated, {AVG_EXP_GET:g} exp / message): {ceil(needed_exp / AVG_EXP_GET)} messages\n")
            cryptogen = SystemRandom()
            a = globals()['a'] = [(x := 0, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            b = globals()['b'] = [(x := 0, [(y := randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            print(f"--A--\n    average time (simulated, cryptographically secure): {ceil(sum([*map(len, a)]) / len(a))} messages\n")
            print(f"--B--\n    average time (simulated, normal pseudorandom): {ceil(sum([*map(len, b)]) / len(b))} messages\n")
    
    def get_required_minutes_both_exp(needed_exp, current_exp):
        if TIME_FMT:
            print("NOTE: have to wait {TIME_FMT} for a message to get exp again\n")
            print(f"Best case: {fmt_time((bc := ceil(needed_exp/MAX_EXP_GET)) * SEC_COOL)} ({bc} messages)\nWorst case: {fmt_time((wc := ceil(needed_exp/MIN_EXP_GET)) * SEC_COOL)} ({wc} messages)\n")
            print(f"average time (nonsimulated, {AVG_EXP_GET:g} exp / message): {fmt_time((avg := ceil(needed_exp / AVG_EXP_GET)) * SEC_COOL)} ({avg} messages)\n")
            cryptogen = SystemRandom()
            a = globals()['a'] = [(x := 0, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            b = globals()['b'] = [(x := 0, [(y := randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            print(f"--A--\n    average time (simulated, cryptographically secure): {fmt_time((a_ := ceil(sum([*map(len, a)]) / len(a))) * SEC_COOL)} ({a_} messages)\n")
            print(f"--B--\n    average time (simulated, normal pseudorandom): {fmt_time((b_ := ceil(sum([*map(len, b)]) / len(b))) * SEC_COOL)} ({b_} messages)\n")
        else:
            print(f"Best case: {ceil(needed_exp/MAX_EXP_GET)} messages\nWorst case: {ceil(needed_exp/MIN_EXP_GET)} messages\n")
            print(f"average time (nonsimulated, {AVG_EXP_GET:g} exp / message): {ceil(needed_exp / AVG_EXP_GET)} messages\n")
            cryptogen = SystemRandom()
            a = globals()['a'] = [(x := 0, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            b = globals()['b'] = [(x := 0, [(y := randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < needed_exp, 0)])[1] for _ in range(100)]
            print(f"--A--\n    average time (simulated, cryptographically secure): {ceil(sum([*map(len, a)]) / len(a))} messages\n")
            print(f"--B--\n    average time (simulated, normal pseudorandom): {ceil(sum([*map(len, b)]) / len(b))} messages\n")
    
    def get_lost_messages(total_exp, nmessages):
        message_exp = AVG_EXP_GET*nmessages
        print(f"nonsimulated average exp and messages \"lost\": {(s := total_exp - message_exp)} exp ({ceil(s / AVG_EXP_GET)} messages)\n")
        cryptogen = SystemRandom()
        a = [sum([cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET) for _ in range(nmessages)]) for _ in range(100)]
        b = [sum([randint(MIN_EXP_GET, MAX_EXP_GET) for _ in range(nmessages)]) for _ in range(100)]
        a = globals()['a'] = [(x := 0, stop := total_exp - X, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < stop, 0)])[2] for X in a]
        b = globals()['b'] = [(x := 0, stop := total_exp - X, [(y := cryptogen.randint(MIN_EXP_GET, MAX_EXP_GET), x := x + y)[0] for _ in iter(lambda: x < stop, 0)])[2] for X in b]
        a_0 = ceil(sum([*map(sum, a)]) / len(a))
        a_1 = ceil(sum([*map(len, a)]) / len(a))
        b_0 = ceil(sum([*map(sum, b)]) / len(b))
        b_1 = ceil(sum([*map(len, b)]) / len(b))
        print(f"--A--\n    simulated average exp and messages \"lost\" (cryptographically secure): {a_0} exp ({a_1} messages)\n")
        print(f"--B--\n    simulated average exp and messages \"lost\" (normal pseudorandom): {b_0} exp ({b_1} messages)\n")
    
    for key, item in locals().items():
        if hasattr(item, '__call__'):
            globals()[key] = item
