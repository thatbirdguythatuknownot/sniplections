# sniplections/c
Collection of C files.


**PDROC-d4.c**<br/>
Python discord Revival-of-Code segment 4, both parts, in C. Only works on Windows. Compiled with these commands:
```bash
clang -fopenmp=libiomp5 -march=core-avx2 -Ofast -Rpass-analysis=vectorize PDROC-d4.c
```

**unfull.c**<br/>
A process, that hides its own console window, to automatically quit a Roblox window if it's in full screen. Only works in Windows. Compiled with these commands:
```bash
clang -O3 -mavx2 unfull.c -o unfull.exe
```
