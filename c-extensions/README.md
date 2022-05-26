# sniplections/c-extensions
Collection of C extensions and possibly Cython scripts. Probably all of the C extension functions are *really* unsafe.


**algos_c.c**<br/>
Collection of algorithms and Advent of Code (AoC) 2021 solutions, with format `[_(format)_]AOC2021_(day)[_(part)]`. Includes:<br/>
• Day 1 Part 1 & 2<br/>
• Day 6 Part 2<br/>
• Day 7 Part 1<br/>
• Day 8 Part 1 & 2<br/>
• Day 9 Part 1<br/>
• Day 10 Part 1 & 2<br/>
• Day 11 Part 1 & 2<br/>
• Day 12<br/>
• Day 17 Part 1 & 2<br/>
• Day 21 Part 1 & 2<br/>
*some may not work*

**a_c.c**<br/>
Once used to compare raw C extension speed with Cython, now possibly even faster than Cython. Built with these commands:
```bash
gcc -fprofile-use -c -I"C:\Program Files\Python311\include" -O2 a_c.c
gcc -fprofile-use -O2 -shared -o "C:\Program Files\Python311\Lib\a_c.pyd" a_c.o -lPython311 -L"C:\Program Files\Python311\libs"
```
