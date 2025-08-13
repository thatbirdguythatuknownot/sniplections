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
from throwaway import throwaway_extra_unpack

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
from dynamic_dispatch import dynamic_dispatch

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
from runtime_mutate import mutate

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
from inline import inline_globals, inline_var

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

**obfuscator.py**<br/>
Python code obfuscator. To be used like this:
```py
from obfuscator import UnparseObfuscate

obfuscate = UnparseObfuscate() # obfuscator = UnparseObfuscate(taken=False)
                               #: use above for "no walrus" mode
output = obfuscate.o('"abc"') #: the `o` method means "obfuscate this string"
print(output) #: depends on Python version
```
Output:
```py
(_:=(__:=(_______:=(____:=(____:=__name__.__ne__(__name__).__invert__()).__neg__().__truediv__(____.__add__(____).__neg__()).__rpow__(____).__class__).__doc__.__getitem__((______:=(___:=__name__.__getitem__((_____:=__name__.__class__().__len__()))).__add__(___).__add__(___).__len__())).__add__(__name__.__len__().__class__.__module__.__getitem__(_____)).__add__(____.__name__.__getitem__(_____)))))
```

**privattr.py**<br/>
Private- actually protected- attribute metaclass. To be used like this:
```py
from privattr import Private

class Foo(metaclass=Private):
    attr1: Private
    attr2: Private
    ...
```

**stack.py**<br/>
Python object stack manipulator (3.11). Works like this:
```py
from stack import get_stack

def f():
    get_stack().push(5)
    return (7, get_stack().pop())

print(f()) # (5, 7)
```

**is_prime.py**<br/>
Check if a number is prime. Requires `/c_extensions/cc.c`.

**get_code.py**<br/>
Get the line(s) of code of a specific statement, along with its start and end line. Works only for Python 3.8+ and with (temporary) Python files. To be used like this:
```py
from get_code import get_line_and_code
a = get_line_and_code()[2]
print(a) # a = get_line_and_code()[2]
```

**vm.py**<br/>
A VM made in Python to display text generated with pseudo-assembly instructions for Advent of Code 2022, Day 10. Used like:
```
python vm.py <columns> "<symbols>" <lines>
<input from stdin>
```

**pysuper.py**<br/>
A thing (`pysuper.Super`) that replicates built-in `super()`. Python 3.11 only.

**astgolfer.py**<br/>
A Python code golfer. To be used like this:
```py
import ast
from astgolfer import RewriteExpression, GolfUnparser

code = "[int(bb) for bb in aokfoasfal]"
tree = ast.parse(code)
tree = ast.fix_missing_locations(RewriteExpression(use_map=True).visit(tree))

print(GolfUnparser().visit(tree)) # [*map(int,aokfoasfal)]
```

**padic_expansion.py**<br/>
Expands rational numbers with a number `p` which satisfies `2 <= p <= 62`. To be used like this:
```py
from padic_expansion import itoo # means input-to-output

print(itoo('3 -5/11 15')) # 110021100211002
print(itoo_cycle('3 -5/11')) # (11002)
```

**min_diff_digits.py**<br/>
Get 2 `n`-digit numbers (3rd argument to `.get_min_digits`, defaults to length of first argument floor divided by 2) in a list of digits which have the least difference between them. Example:
```py
from min_diff_digits import get_min_digits
print(get_min_digits("234678")) # (39, ['4', '2', '6'], ['3', '8', '7'])
print(get_min_digits("1234567890")) # (247, ['5', '0', '1', '2', '3'], ['4', '9', '8', '7', '6'])
```

**pybuild_class.py**<br/>
A thing (`pybuild_class.build_class`) that replicates built-in `__build_class__()`. Python 3.11 only.

**get_minutes.py**<br/>
Gets required minutes (or messages) to reach a certain level or XP using current XP amount (Polaris system, coefficients to cubic function can be provided, by default MEE6 configuration). Example:
```py
from get_minutes import init_funcs
init_funcs() # initialize
print(get_required_minutes_exp(100, 0)) # amount of minutes/messages required to get level 100 from level 0; provides output in nonsimulated, simulated (cryptographically secure), and simulated (normal pseudorandom)
```
Output:
```
needed_exp = 1899250.0
NOTE: have to wait 1m for a message to get exp again

Best case: 52d 18h 10m (75970 messages)
Worst case: 87d 22h 17m (126617 messages)

average time (nonsimulated, 20 exp / message): 65d 22h 43m (94963 messages)

--A--
    average time (simulated, cryptographically secure): 65d 22h 49m (94969 messages)

--B--
    average time (simulated, normal pseudorandom): 65d 22h 38m (94958 messages)
```

**electronconfig.py**<br/>
Generate the electron configuration for a specific atomic number. Interactive mode (`py electronconfig.py`) accepts both names, abbreviations, and atomic numbers. Example (for non-interactive mode):
```py
from electronconfig import config_to_print, gen_config, parse_element_string
print(gen_config(5), config_to_print(5)) # [(1, 's', 2), (2, 's', 2), (2, 'p', 1)] 1s²2s²2p¹
number = parse_element_string('boron')[0]
print(gen_config(number), config_to_print(number)) # [(1, 's', 2), (2, 's', 2), (2, 'p', 1)] 1s²2s²2p¹
```
Example (for interactive mode):
```
> sodium cl o 6
EC sodium = 1s²2s²2p⁶3s¹
EC cl (chlorine) = 1s²2s²2p⁶3s²3p⁵
EC o (oxygen) = 1s²2s²2p⁴
EC 6 (carbon) = 1s²2s²2p²
```

**check_noarg_deco.py**<br/>
Check if the function is the final one being called in the decorator (e.g. `func` in `@func`, result of `func()` in `@func()`). Works only in 3.11.

**punnett_square.py**<br/>
Punnett square generation in Python. For example, this:
```py
from punnett_square import gen_punnett, string_gen_punnett
traits = {'R': 'Round', 'r': 'Wrinkled', 'Y': 'Yellow', 'y': 'Green'}
print(string_gen_punnett(gen_punnett('Rr', 'RR', traits=traits)))
print("\n")
print(string_gen_punnett(gen_punnett('RrYy', 'RrYy', traits=traits)))
```
Outputs this:
```
    +------------+------------+
    |     R      |     R      |
+---+------------+------------+
| R | RR (Round) | RR (Round) |
+---+------------+------------+
| r | Rr (Round) | Rr (Round) |
+---+------------+------------+

Genotype Ratio:
  RR:Rr
  2 :2

Phenotype Ratio:
  Round : 4


     +---------------------+---------------------+------------------------+------------------------+
     |         RY          |         Ry          |           rY           |           ry           |
+----+---------------------+---------------------+------------------------+------------------------+
| RY | RRYY (Round/Yellow) | RRYy (Round/Yellow) | RrYY (Round/Yellow)    | RrYy (Round/Yellow)    |
+----+---------------------+---------------------+------------------------+------------------------+
| Ry | RRYy (Round/Yellow) | RRyy (Round/Green)  | RrYy (Round/Yellow)    | Rryy (Round/Green)     |
+----+---------------------+---------------------+------------------------+------------------------+
| rY | RrYY (Round/Yellow) | RrYy (Round/Yellow) | rrYY (Wrinkled/Yellow) | rrYy (Wrinkled/Yellow) |
+----+---------------------+---------------------+------------------------+------------------------+
| ry | RrYy (Round/Yellow) | Rryy (Round/Green)  | rrYy (Wrinkled/Yellow) | rryy (Wrinkled/Green)  |
+----+---------------------+---------------------+------------------------+------------------------+

Genotype Ratio:
  RRYY:RRYy:RrYY:RrYy:RRyy:Rryy:rrYY:rrYy:rryy
   1  : 2  : 2  : 4  : 1  : 2  : 1  : 2  : 1

Phenotype Ratio:
     Round/Yellow : 9
      Round/Green : 3
  Wrinkled/Yellow : 3
   Wrinkled/Green : 1
```

**make_class.py**<br/>
Original idea from <@!396290259907903491> (Ava#4982). Decorator to make a class from a function (doesn't necessarily turn the function into a class). Example:
```py
from make_class import make

@make
def MyClass():
    def __init__(self, first, second):
        self.first = first
        self.second = second
    def sum(self):
        return self.first + self.second

a = MyClass(1, 2)
print(a.sum()) # 3
```

**minsolve_linear.py**<br/>
Solve the equation `Ax + By = C` in a way that `(x, y)` has the minimum possible distance to `(0, 0)`. Example:
```py
from minsolve_linear import solve

# solve 8x - 2y = 14
print(solve(14, 8, -2)) # (2, 1)
```

**oproperty.py**<br/>
Original idea from <@!1060338987694293052> (zombiiess). Properties outside of a class. Currently only supports globals. Example:
```py
from oproperty import init, oproperty

init()

@oproperty
def f(): return 5

print(f) # 5
```

**increm.py**<br/>
Original idea from <@!241962659505766402> (programming_enjoyer). Incrementing operator. Works for <3.12 and requires the `einspect` package is installed. Example:
```py
import increm
a = 2
print(a, --a, a, ++a, a) # 2 1 1 2 2
```
Do note that anything past the first two chained `-`/`+` will NOT be interpreted as an increment, so `++++a` will only do the equivalent of `+(+(a := a + 1))` in normal Python.

**game.py**<br/>
I made this because I was bored and I had no internet. Tried to make a thing where eventually all the values balance out but that... something is wrong with the formulas for now. Enjoy.

**varname.py**<br/>
Original idea from <@!88336074710982656> (L3viathan). Get a variable's (qualified) name. Works on 3.11+. Example:
```py
from varname import varname

class A:
    a = 5

print(varname(A.a), varname(A)) # A.a A
```

**msgtohash.py**<br/>
Utility function for autogenerating code in `/c/getfreer.c`.

**renamer.py**<br/>
Renamer class and function. Works like so:
```py
from renamer import rename

src = "_ = 5"
print(rename(src)) # A = 5
```
More descriptive examples might be provided in the future.

**dundeobfuscator.py**<br/>
Dunder obfuscation deobfuscator class utilizing `renamer.Renamer`. Example:
```py
from dundeobfuscator import deobfuscate
print(deobfuscate("(__ := 'a').__getattribute__('join')((__builtins__.chr(__builtins__.ord(__)+1),)*5)"))
```
Output:
```
(A := 'a').join((__builtins__.chr(__builtins__.ord('a') + 1),) * 5)
```

**scinum.py**, **vectors.py**<br/>
Quality-of-life classes to help with my Physics classes. May be slow. Example:
```py
from vectors import Vector
a = Vector(35, direc="25 N of E", n_SF=-1)
b = Vector(15, direc="10 E of N", n_SF=-1)
# prints in the format of (x, y) | magnitude -> direction (in degrees)
print(a + b) # (35, 30) | 46 -> 41°
```

**recursive_bool.py**<br/>
A recursive bool implementation according to the rules set by <@!754441517925662750> (nongshimkamjag). Example:
```py
from recursive_bool import rbool

print(rbool([[], []]))  # False
print(rbool([[]]))  # False
print(rbool([]))  # False
print(rbool([[0]]))  # True
```

**Oattr.py**<br/>
A class that returns attributes chain-accessed as a list, I guess. Original idea from <@!207934404922703873> (andrej730). Works on 3.12+. Example:
```py
from Oattr import O

print(O.a) # ['a']
print(O.a.b.c.d.e) # ['a', 'b', 'c', 'd', 'e']
```

**nonlocals_proxy.py**<br/>
A `nonlocals()` function. Tested on 3.12+. Example:
```py
from nonlocals_proxy import nonlocals

def a():
    b = 25
    def g():
        nonlocal b
        nonlocals()['b'] *= 2
    print(b)
    g()
    print(b)

a()
```
Output:
```
25
50
```

**set_consts.py**<br/>
A context manager to set the constants in a context. Tested on 3.12+. Example:
```py
from set_consts import set_consts

print('abc')

with set_consts(abc=5):
    print('abc')

print('abc')
```
Output:
```
abc
5
abc
```

**methodproperty.py**<br/>
A `property()` with a `callgetter`. Tested on 3.12+. Example:
```py
class A:
    @methodproperty
    def t(self):
        return 5
    @t.callgetter
    def _(self, a, b):
        return a + b

a = A()
print(a.t)  # 5
print(a.t(1, 2))  # 3
```

**shapegetter.py**<br/>
A function to get the shape of the (first) assignment the callee function's call is being unpacked in. Returns `None` if it's neither directly passed to an assignment OR if the callee function's call is used as an expression. Tested on 3.12+. Example:
```py
from shapegetter import get_assign_shape

def g():
    shape = get_assign_shape()
    match shape:
        case [_, _, _]:
            return "now", "three", "args"
        case [_, _]:
            return "two", "arguments"
    return "one"

a = g()
print(a)
a, b = g()
print(a, b)
a, b, c = g()
print(a, b, c)
```
Output:
```
one
two arguments
now three args
```

**resistorread.py**<br/>
Reads resistors' color sequence and outputs their ohm-tolerance equivalent (TODO: vice versa and temperature coefficient too). Example:
```py
from resistorread import *

read(BLUE, RED, BROWN, GOLD)  # 620 Ω ± 5%
```

**partialtrail.py**<br/>
Rough wrapper for dynamically filled trailers (subscripts and attributes). Used like this:
```py
from partialtrail import partialtrail

# separated `view` variable to be clearer
view = partialtrail[...][::-1]
x = view([range(5), range(0, 10, 2), range(0, 15, 3)])
print(x[0])
print(x[2])

view = partialtrail[1]._
v = ["abc", "xyz", "def"]
for x in v:
    print(view(x).encode())
```
Output:
```
range(4, -1, -1)
range(12, -3, -3)
b'b'
b'y'
b'e'
```

**fcp.py**<br/>
Calculates the number of possibilities of combining any number of sets, in order, without replacement. [Original code](https://gist.github.com/thatbirdguythatuknownot/426e54ea57e616a347971186c674ee8b) by <@!207089259730042881> (lakmatiol). Example:
```py
from fcp import solve

# Ways to create a 3-digit even number with no repeating digits:
# 1. set of digits 1 - 9
# 2. set of digits 0 - 9
# 3. set of even digits 0, 2, 4, 6, 8
sets = {*range(1, 10)}, {*range(10)}, {*range(0, 10, 2)}
print(solve(*sets), "ways")  # 328 ways
```

**safe_exec.py**<br/>
Safe calculator I guess? Doesn't expose a `safe_exec()` function yet.

**getsource.py**<br/>
Attempt to get the source of a function with the `getsource.get_source()` method. Tested in 3.13. Example:
```py
import getsource as gs

def half(a: int) -> float:
    return a / 2

print(gs.get_source(half))
```
Output:
```
def half(a: int) -> float:
    return a / 2
```

**cvt_baybayin.py**<br/>
Convert to/from Baybayin.
