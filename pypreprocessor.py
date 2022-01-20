import string
from regex import compile as pcompile, sub
letters = {*string.ascii_letters}
match_macro = pcompile(r'\$(\w+)(\((?:[^()]++|(?0))*+\))?\$').finditer

DEFAULT_DEFINE = {'_LINE': None, '_FILE': False}
new_defines = DEFAULT_DEFINE.copy

def py_preprocessor(s, filename='<string>'):
    assert type(filename) is str, "filename must be a string"
    defines = new_defines()
    funcdefines = {}
    lines = []
    apn_lines = lines.append
    for i, x in enumerate(s.split('\n')):
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
                        while 1:
                            y = x.find(',')
                            if y != -1:
                                param = x[:y].strip()
                                x = x[y + 1:]
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
            origline = x
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
        else:
            minus_pos = 0
            for pat in match_macro(x):
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

def preprocessor_compile(s, filename='<string>', *a, **k):
    return nopreprocessor_compile(py_preprocessor(s, filename), filename, *a, **k)
