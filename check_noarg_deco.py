from sys import _getframe
from opcode import (_inline_cache_entries as ICE, EXTENDED_ARG, opmap,
                    stack_effect, HAVE_ARGUMENT)

CACHE = opmap['CACHE']
PRECALL = opmap['PRECALL']
LENICE_PRECALL = ICE[PRECALL] * 2
CALL = opmap['CALL']
ICE_CALL = ICE[CALL]
LENICE_CALL = ICE_CALL * 2
MAKE_FUNCTION = opmap['MAKE_FUNCTION']
LOAD_CONST = opmap['LOAD_CONST']
LOAD_BUILD_CLASS = opmap['LOAD_BUILD_CLASS']

def check_noarg_deco():
    try:
        f = _getframe(2)
    except ValueError:
        return False
    code = f.f_code.co_code
    op = f.f_lasti - LENICE_CALL
    if code[op] is not CALL or code[op + 1]:
        return False
    op -= LENICE_PRECALL + 4
    if code[op] is CACHE:
        ncaches = 0
        while op >= 0:
            while op >= 0 and code[op] is CACHE:
                ncaches += 1
                op -= 2
            if ncaches is ICE_CALL and code[op] is CALL:
                if not code[op + 1]:
                    ncaches = 0
                    op -= LENICE_PRECALL + 4
                    continue
            break
        if (op >= 0 and ncaches is ICE_CALL and code[op] is CALL
                and code[op + 1] >= 2):
            stacklevel = -code[op + 1]
            op -= LENICE_PRECALL + 4
            while op >= 0:
                pos = op - 2
                if code[op] >= HAVE_ARGUMENT:
                    shift = 1
                    oparg = code[op + 1]
                    if pos >= 0 and code[pos] is EXTENDED_ARG:
                        while pos >= 0 and code[pos] is EXTENDED_ARG:
                            oparg += code[pos + 1] << shift
                            shift += 1
                            pos -= 2
                else:
                    oparg = None
                stacklevel += stack_effect(code[op], oparg, jump=False)
                if not stacklevel:
                    break
                op = pos
            else:
                return False
            if (code[op] is LOAD_CONST and code[op + 2] is MAKE_FUNCTION
                    and op - 2 >= 0 and code[op - 2] is LOAD_BUILD_CLASS):
                return True
    elif code[op] is MAKE_FUNCTION:
        pos = op - 4
        shift = 1
        oparg = code[op - 1]
        if pos >= 0 and code[pos] is EXTENDED_ARG:
            while pos >= 0 and code[pos] is EXTENDED_ARG:
                oparg += code[pos + 1] << shift
                shift += 1
                pos -= 2
        codeobj = f.f_code.co_consts[oparg]
        return codeobj is not _getframe(1).f_code
    return False
