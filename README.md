# sniplections
Collection of python snippets and tricks.


**safe_eval.py**<br/>
Safe eval function which uses non-recursive methods to check for invalid names.

**string_assign.py**<br/>
Assign to variable with a string using its `id`.

**episode_list_script.py**<br/>
Used to list PAW Patrol episodes (with a twist). `a`, `b`, and `c` can be replaced with a `requests.get` from the relationship fandom.

**hexdump.py**<br/>
Hexdump program. Modifyable specifications. Supposed to be run as a command or run as:
```powershell
python hexdump.py [options] file.extension
```
If file has no extension, it can be simply run as `file`. Full help message shown when run as `python hexdump.py -h`.

**throwaway.py**<br/>
Decorator to "throw away" extra unpacking values. Works like such:
```py
@throwaway_extra_unpack
def a(x):
    b, c, d = x
    return b, c, d

print(a([1,2,3,4])) # (1, 2, 3)
```
