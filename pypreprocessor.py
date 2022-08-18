import string
from re import compile as pcompile, sub, error as re_error
letters = {*string.ascii_letters}
try:
    macropattern = pcompile(r'\$(\w+)(\((?:[^()]++|(?0))*+\))?\$')
except re_error:
    from regex import compile as pcompile, sub
    pcompile(r'\$(\w+)(\((?:[^()]++|(?0))*+\))?\$')

match_macros = macropattern.finditer
has_macro = macropattern.search

DEFAULT_DEFINE = {'_LINE': None, '_FILE': False}
new_defines = DEFAULT_DEFINE.copy

elif_directives = ['$elifdef ', '$eliffdef ', '$elif']
elif_directives = ['$elifndef ', '$elifnfdef ', '$elif']

class IterQueue:
    def __init__(self, iterator):
        self.iterator = iterator
        self.queue = []
    def __iter__(self):
        return self
    def __next__(self):
        if self.queue:
            return self.queue.pop()
        return next(self.iterator)

def py_preprocessor(s, filename='<string>'):
    assert type(filename) is str, "filename must be a string"
    defines = new_defines()
    funcdefines = {}
    lines = []
    apn_lines = lines.append
    has_ifdef = False
    has_ifdef_func = False
    has_if = False
    skip_stack = []
    NEW_SKIP = skip_stack.append
    POP_SKIP = skip_stack.pop
    ifmatch_stack = []
    NEW_IF_DIRECTIVE = ifmatch_stack.append
    POP_IF_DIRECTIVE = ifmatch_stack.pop
    condition_stack = []
    NEW_CONDITION = condition_stack.append
    POP_CONDITION = condition_stack.pop
    iterator = IterQueue(iter(s.split('\n')))
    NEW_ITERQUEUE_ENTRY = iterator.queue.append
    for i, x in enumerate(iterator):
        if (skip_stack and skip_stack[-1]
                and not (x.startswith(elif_directives[ifmatch_stack[-1]])
                         or x.startswith(nelif_directives[ifmatch_stack[-1]])
                         or x.startswith('$endif') and not x[6].isidentifier()
                         or x.strip() == '$else')):
            continue
        if x.startswith('$def '):
            x = x[5:].strip()
            assert x[0] in letters, f"not valid macro name in line {i+1}"
            y = x.find(' ')
            if y == -1 or not x[:y].isalnum():
                y = x.find('(')
                if y == -1 or not x[:y].isalnum():
                    defines[x[:y]] = ""
                else:
                    name = x[:y]
                    assert name[0] in letters and (not name[1:] or name[1:].isalnum()), f"not valid macro name:{name}:in line {i+1}"
                    x = x[y + 1:]
                    assert x[0] in letters or x[0] == ')', f"not valid macro in line {i+1}"
                    if x[0] == ')':
                        x = x[1:].strip()
                        funcdefines[name] = ([], x)
                    else:
                        list_of_params = []
                        apn = list_of_params.append
                        assert x.find(')') != -1, f"not valid macro in line {i+1}"
                        end_paren = x.find(')')
                        while 1:
                            y = x.find(',')
                            if y != -1 and y < end_paren:
                                param = x[:y].strip()
                                x = x[y + 1:]
                                end_paren -= y
                                assert param, f"cannot have empty macro param in line {i+1}"
                                assert param[0] in letters and (not param[1:] or param[1:].isalnum()), f"not valid macro param in line {i+1}"
                                apn(param)
                            else:
                                y = x.find(')')
                                param = x[:y].strip()
                                x = x[y + 1:]
                                if param:
                                    assert param[0] in letters and (not param[1:] or param[1:].isalnum()), f"not valid macro param in line {i+1}"
                                    apn(param)
                                break
                        funcdefines[name] = (list_of_params, x)
            else:
                defines[x[:y]] = x[y + 1:]
        elif x.startswith('$del '):
            x = x[5:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            if x in defines:
                if x == '_LINE':
                    defines[x] = None
                else:
                    del defines[x]
            elif x in funcdefines:
                del funcdefines[x]
            else:
                raise NameError(f"macro name '{x}' not defined")
        elif x.startswith('$ifdef '):
            x = x[7:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            NEW_IF_DIRECTIVE(0)
            if x in defines:
                NEW_CONDITION(1)
                NEW_SKIP(0)
            else:
                NEW_CONDITION(0)
                NEW_SKIP(1)
        elif x.startswith('$ifndef '):
            x = x[8:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            NEW_IF_DIRECTIVE(0)
            if x in defines:
                NEW_CONDITION(0)
                NEW_SKIP(1)
            else:
                NEW_CONDITION(1)
                NEW_SKIP(0)
        elif x.startswith('$iffdef '):
            x = x[8:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            NEW_IF_DIRECTIVE(1)
            if x in funcdefines:
                NEW_CONDITION(1)
                NEW_SKIP(0)
            else:
                NEW_CONDITION(0)
                NEW_SKIP(1)
        elif x.startswith('$ifnfdef '):
            x = x[9:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            NEW_IF_DIRECTIVE(1)
            if x in funcdefines:
                NEW_CONDITION(0)
                NEW_SKIP(1)
            else:
                NEW_CONDITION(1)
                NEW_SKIP(0)
        elif x.startswith('$elifdef '):
            x = x[9:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            assert not ifmatch_stack[-1], f"$elifdef doesn't match $ifdef in line {i+1}"
            if condition_stack[-1]:
                skip_stack[-1] = 1
                continue
            if x in defines:
                condition_stack[-1] = 1
                skip_stack[-1] = 0
            else:
                condition_stack[-1] = 0
                skip_stack[-1] = 1
        elif x.startswith('$elifndef '):
            x = x[10:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            assert not ifmatch_stack[-1], f"$elifndef doesn't match $if(n)def in line {i+1}"
            if condition_stack[-1]:
                skip_stack[-1] = 1
                continue
            if x in defines:
                condition_stack[-1] = 0
                skip_stack[-1] = 1
            else:
                condition_stack[-1] = 1
                skip_stack[-1] = 0
        elif x.startswith('$eliffdef '):
            x = x[10:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            assert ifmatch_stack[-1] == 1, f"$eliffdef doesn't match $iffdef in line {i+1}"
            if condition_stack[-1]:
                skip_stack[-1] = 1
                continue
            if x in funcdefines:
                condition_stack[-1] = 1
                skip_stack[-1] = 0
            else:
                condition_stack[-1] = 0
                skip_stack[-1] = 1
        elif x.startswith('$elifnfdef '):
            x = x[11:].strip()
            assert x[0] in letters and (not x[1:] or x[1:].isalnum()), f"not valid macro name:{x}:in line {i+1}"
            assert ifmatch_stack[-1] == 1, f"$elifnfdef doesn't match $if(n)fdef in line {i+1}"
            if condition_stack[-1]:
                skip_stack[-1] = 1
                continue
            if x in funcdefines:
                condition_stack[-1] = 0
                skip_stack[-1] = 1
            else:
                condition_stack[-1] = 1
                skip_stack[-1] = 0
        elif x.startswith('$endif') and not x[6].isidentifier():
            assert ifmatch_stack, f"$endif doesn't match any $if* directives in line {i+1}"
            POP_CONDITION()
            POP_SKIP()
            POP_IF_DIRECTIVE()
            NEW_ITERQUEUE_ENTRY(x[6:])
        else:
            while has_macro(x):
                minus_pos = 0
                for pat in match_macros(x):
                    name, args = pat.groups()
                    if args:
                        assert name in funcdefines, f"macro name '{name}' is not a macro function name in line {i+1}"
                        args = list(map(str.strip, args[1:-1].split(',')))
                        if args and args[-1] == '':
                            args = args[:-1]
                        assert all(args), f"cannot have empty argument in macro function call to {name} in line {i+1}"
                        param_list, replacement = funcdefines[name]
                        assert len(param_list) == len(args), f"length of arguments in macro function call to {name} does not match parameters length in line {i+1}"
                        for arg, param in zip(args, param_list):
                            replacement = sub(f"`{param}`", arg, replacement)
                        x = x[:pat.start()] + replacement + x[pat.end():]
                    else:
                        assert name in defines, f"macro name '{name}' is not a macro name in line {i+1}"
                        x = (x[:(patst := pat.start()) - minus_pos]
                            + (repl := f"{i + 1}"
                               if defines[name] is None
                               else (f"{filename!r}"
                                     if defines[name] is False
                                     else defines[name]))
                            + x[(patnd := pat.end()) - minus_pos:])
                        minus_pos += (patnd - patst) - len(repl)
            apn_lines(x)
    return '\n'.join(lines)

nopreprocessor_compile = __builtins__.compile

def preprocessor_compile(s, filename='<string>', mode='exec', *a, **k):
    return nopreprocessor_compile(py_preprocessor(s, filename), filename, mode, *a, **k)
