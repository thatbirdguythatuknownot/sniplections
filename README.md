# sniplections
Collection of various snippets/files that I made or improved. Requires a copy-paste or `from file import *` to work properly.


**safe_eval.py**<br/>
Safe eval function which uses non-recursive methods to check for invalid names.

**string_assign.py**<br/>
Assign to variable with a string using its `id`.

**episode_list_script.py**<br/>
Used to list PAW Patrol episodes (with a twist). `a`, `b`, and `c` can be replaced with a `requests.get` from the relationship fandom.

**hexdump.py**<br/>
Hexdump program (ONLY ON WINDOWS). Modifiable specifications. Supposed to be run as a command or run as:
```powershell
py hexdump.py [options] file.extension
```
If file has no extension, it can be simply run as `file`. Full help message shown when run without arguments or as `py hexdump.py -?`.

**throwaway.py**<br/>
Decorator to "throw away" extra unpacking values. Works like such:
```py
@throwaway_extra_unpack
def a(x):
    b, c, d = x
    return b, c, d

print(a([1,2,3,4])) # (1, 2, 3)
```
Can also be used with variables in `for` loops.

**pypreprocessor.py**<br/>
Python preprocessor. To be used like this:
```py
$def macro_name everything else in this line is part of the macro
$def macro_func_name1() macro function
$def macro_func_name2(param_list,) doesn't need a trailing comma
$del macro_name
# everything after $del must only be the macro name
# to be used in code like this (macro_name must be defined using `$def`) (is not just limited to print()):
print($macro_name$)
print($macro_func_name1()$)
print($macro_func_name2(a, b, c)$)
```
To compile with preprocessing, use `preprocessor_compile(s)` or `compile(py_preprocessor(s))`.

**decompiler.py**<br/>
Python decompiler. To be given a compiled code argument and it will return a string that should, when executed, do the exact same thing as the compiled code argument. Is currently unfinished and only works for an early version of Python 3.11.0a2.

**decompiler2.py**<br/>
Same as **decompiler.py** but only works in Python 3.11.0a7.

**mtheorem.py**<br/>
Multinomial theorem expander.

**pylua.py**<br/>
Python to Lua transpiler. To use, do this:
```py
from pylua import lua_transpiler
python_code_as_string = ...
unparsed_vanilla_lua_code = lua_transpiler(python_code_as_string)
unparsed_roblox_lua_code = lua_transpiler(python_code_as_string, luau=True)
```

**dynamic_dispatch.py**<br/>
Overloading for Python functions. To be used like this:
```py
@dynamic_dispatch
def baz(x: int, y: int) -> int:
    return x + y

@dynamic_dispatch
def baz(x: int, y: float) -> float:
    return x * y

print(baz(2, 3)) # baz(x: int, y: int)
                 # output: 5
print(baz(2, 7.5)) # baz(x: int, y: float)
                   # output: 15.0
```

**runtime_mutate.py**<br/>
(Credits to <@!310449948011528192> (@dzshn#1312) for the original 3.10 code)<br/>
Mutate code in runtime. Works in 3.11 only. To be used like this:
```py
def uhm():
    x = 143
    mutate(
        LOAD_CONST,      7,
        STORE_FAST,      "a",
        LOAD_FAST,       "x",
        COPY,            1,
        LOAD_FAST,       "a",
        BINARY_OP,       "+",
        BINARY_OP,       "*=",
        STORE_FAST,      "x",
    )
    return x

print(uhm()) # 21450
```

**inline.py**<br/>
(Credits to <@!575681145929203724> (@denball#1376) for the original 3.10 code)<br/>
Inline variables, globals, builtins, and replace constants. Works in 3.11 only. To be used like this:
```py
from dis import dis

s = 5

@inline_globals(check_store=2)
@inline_var('c', 5, check_store=2, inline_arg=False)
def g(a, b):
    global s
    print(s)
    s += 7
    print(s)
    c += 2
    return a + b + c + s

print(g(5, 6))
# `print(s)` (first one): 5
# `print(s)` (second one): 12
# result: 30
```
Note that there should not be `s += 7` or `c += 2` in the bytecode.
