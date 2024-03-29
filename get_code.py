from dis import findlinestarts
from itertools import islice, takewhile
from linecache import getline
from sys import _getframe

def get_line_and_code(depth=0):
    lasti = (frame := _getframe(depth + 1)).f_lasti
    lineno = frame.f_lineno
    lines = [*takewhile(lambda x: x[0] != lasti, findlinestarts(frame.f_code))]
    last_mention = ~next(
        (i for i, x in enumerate(reversed(lines)) if i and x[1] == lineno),
        0)
    if last_mention == -1:
        code = getline(__file__, lineno)
        end_lineno = lineno
    else:
        end_lineno = max(x[1] for x in lines[last_mention:])
        is_old = end_lineno == lineno + 1
        code = ''.join([getline(__file__, i) for i in range(lineno, end_lineno + is_old)])
        end_lineno -= is_old
    code = code.rstrip('\r\n')
    return lineno, end_lineno, code
