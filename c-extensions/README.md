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
clang -Ofast -march=native -c -I"C:\Program Files\Python311\include" a_c.c -o a_c.o
clang -Ofast -march=native -shared -o "C:\Program Files\Python311\Lib\a_c.pyd" a_c.o -lPython311 -L"C:\Program Files\Python311\libs"
```

**fast_itertools.c**<br/>
A faster version of [`more_itertools`](https://pypi.org/project/more-itertools/). Built using the same commands as `a_c.c` above.

**c.c**<br/>
Compiled with these commands:
```bash
clang -c -I"C:\Program Files\Python311\include" -fopenmp=libiomp5 -Ofast -march=core-avx2 -Rpass-analysis=vectorize c.c -o c.o
clang -fopenmp=libiomp5 -Ofast -march=core-avx2 -shared -o "C:\Program Files\Python311\Lib\c.pyd" c.o -l"Python311" -L"C:\Program Files\Python311\libs"
```

**cc.c**<br/>
Compiled with these commands:
```bash
clang -c -I"C:\Program Files\Python311\include" -fopenmp=libiomp5 -Ofast -march=core-avx2 -Rpass-analysis=vectorize cc.c -o cc.o
clang -fopenmp=libiomp5 -Ofast -march=core-avx2 -shared -o "C:\Program Files\Python311\Lib\cc.pyd" cc.o -l"Python311" -L"C:\Program Files\Python311\libs"
```

**md5.c**<br/>
A module for `md5` hashing functions. Compiled with the same commands as `c.c` above.

**keys.c**<br/>
A module with a single function `get_keys()` to return currently pressed keys. Currently only works on windows. Built with the following commands:
```bash
clang -O2 -c -I"C:\Program Files\Python311\include" keys.c -o keys.o
clang -O2 -shared -o "C:\Program Files\Python311\Lib\keys.pyd" keys.o -lPython311 -L"C:\Program Files\Python311\libs"
```

**uwuifier.c**<br/>
A module with functions (main one is `uwuify()`) to uwuify a text. Compiled with the same commands as `keys.c` above.

**pdist_cext.c**<br/>
For [this repo](https://github.com/trygvrad/python-speedup-comparison). Built with:
```bash
clang -O2 -ffast-math -c -I"C:\Program Files\Python311\include" -I"C:\Program Files\Python311\Lib\site-packages\numpy\core\include" pdist_cext.c -o pdist_cext.o
clang -O2 -shared -o "C:\Program Files\Python311\Lib\pdist_cext.pyd" pdist_cext.o -lPython311 -L"C:\Program Files\Python311\libs"
```
