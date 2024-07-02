#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long *counts = NULL;
static int count_n = 0;

BOOL WINAPI ConsoleHandler(DWORD dwType)
{
    switch(dwType) {
    case CTRL_C_EVENT:
        if (!counts) return FALSE;
        for (int i = 0; i < count_n; i++) {
            if (counts[i] != 0) {
                fprintf(stderr, "%d: %zu\n", i, counts[i]);
            }
        }
        free(counts);
        return FALSE;
    }
    return TRUE;
}

int main() {
    srand(time(NULL));
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE)) {
        fprintf(stderr, "handler init failed!\n");
        return 1;
    }
    unsigned long long i;
    count_n = 200;
    counts = malloc(sizeof(unsigned long long) * count_n);
    if (!counts) return 2;
    memset(counts, 0, count_n);
    int cuo, m = -1;
    for (i = 0 ;; i++) {
        cuo = 0;
        while (rand() % 6 != 5) {
            cuo++;
        }
        if (cuo >= count_n) {
            count_n += cuo;
            unsigned long long *temp = realloc(counts, sizeof(unsigned long long) * count_n);
            if (!temp) {
                free(counts);
                return 2;
            }
            counts = temp;
            memset(counts + (count_n - cuo), 0, cuo);
        }
        if (cuo > m) {
            m = cuo;
            fprintf(stderr, "new max at %zu: %d (%.10f%%)\n", i, cuo, pow(5./6., cuo) * 100);
        } else if (counts[cuo] == 0) {
            fprintf(stderr, "new addition at %zu: %d\n", i, cuo);
        }
        counts[cuo]++;
    }
    free(counts);
    return 0;
}
