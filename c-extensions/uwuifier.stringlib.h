#if LONG_BIT >= 128
#define STRINGLIB_BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define STRINGLIB_BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define STRINGLIB_BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define STRINGLIB_BLOOM_ADD(mask, ch) \
    ((mask |= (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH -1)))))
#define STRINGLIB_BLOOM(mask, ch)     \
    ((mask &  (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH -1)))))

#ifdef STRINGLIB_FAST_MEMCHR
#  define MEMCHR_CUT_OFF 15
#else
#  define MEMCHR_CUT_OFF 40
#endif

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(find_char)(const STRINGLIB_CHAR* s, Py_ssize_t n, STRINGLIB_CHAR ch)
{
    const STRINGLIB_CHAR *p, *e;

    p = s;
    e = s + n;
    if (n > MEMCHR_CUT_OFF) {
#ifdef STRINGLIB_FAST_MEMCHR
        p = STRINGLIB_FAST_MEMCHR(s, ch, n);
        if (p != NULL)
            return (p - s);
        return -1;
#else
        /* use memchr if we can choose a needle without too many likely
           false positives */
        const STRINGLIB_CHAR *s1, *e1;
        unsigned char needle = ch & 0xff;
        /* If looking for a multiple of 256, we'd have too
           many false positives looking for the '\0' byte in UCS2
           and UCS4 representations. */
        if (needle != 0) {
            do {
                void *candidate = memchr(p, needle,
                                         (e - p) * sizeof(STRINGLIB_CHAR));
                if (candidate == NULL)
                    return -1;
                s1 = p;
                p = (const STRINGLIB_CHAR *)
                        _Py_ALIGN_DOWN(candidate, sizeof(STRINGLIB_CHAR));
                if (*p == ch)
                    return (p - s);
                /* False positive */
                p++;
                if (p - s1 > MEMCHR_CUT_OFF)
                    continue;
                if (e - p <= MEMCHR_CUT_OFF)
                    break;
                e1 = p + MEMCHR_CUT_OFF;
                while (p != e1) {
                    if (*p == ch)
                        return (p - s);
                    p++;
                }
            }
            while (e - p > MEMCHR_CUT_OFF);
        }
#endif
    }
    while (p < e) {
        if (*p == ch)
            return (p - s);
        p++;
    }
    return -1;
}

#undef MEMCHR_CUT_OFF

LOCAL(Py_ssize_t)
STRINGLIB(count_char)(const STRINGLIB_CHAR *s, Py_ssize_t n,
                      const STRINGLIB_CHAR p0)
{
    Py_ssize_t i, count = 0;
    for (i = 0; i < n; i++) {
        if (UNLIKELY(s[i] == p0)) {
            count++;
        }
    }
    return count;
}

LOCAL(Py_ssize_t)
STRINGLIB(_lex_search)(const STRINGLIB_CHAR *needle, Py_ssize_t len_needle,
                       Py_ssize_t *return_period, int invert_alphabet)
{
    /* Do a lexicographic search. Essentially this:
           >>> max(needle[i:] for i in range(len(needle)+1))
       Also find the period of the right half.   */
    Py_ssize_t max_suffix = 0;
    Py_ssize_t candidate = 1;
    Py_ssize_t k = 0;
    // The period of the right half.
    Py_ssize_t period = 1;

    while (candidate + k < len_needle) {
        // each loop increases candidate + k + max_suffix
        STRINGLIB_CHAR a = needle[candidate + k];
        STRINGLIB_CHAR b = needle[max_suffix + k];
        // check if the suffix at candidate is better than max_suffix
        if (invert_alphabet ? (b < a) : (a < b)) {
            // Fell short of max_suffix.
            // The next k + 1 characters are non-increasing
            // from candidate, so they won't start a maximal suffix.
            candidate += k + 1;
            k = 0;
            // We've ruled out any period smaller than what's
            // been scanned since max_suffix.
            period = candidate - max_suffix;
        }
        else if (a == b) {
            if (k + 1 != period) {
                // Keep scanning the equal strings
                k++;
            }
            else {
                // Matched a whole period.
                // Start matching the next period.
                candidate += period;
                k = 0;
            }
        }
        else {
            // Did better than max_suffix, so replace it.
            max_suffix = candidate;
            candidate++;
            k = 0;
            period = 1;
        }
    }
    *return_period = period;
    return max_suffix;
}

LOCAL(Py_ssize_t)
STRINGLIB(_factorize)(const STRINGLIB_CHAR *needle,
                      Py_ssize_t len_needle,
                      Py_ssize_t *return_period)
{
    /* Do a "critical factorization", making it so that:
       >>> needle = (left := needle[:cut]) + (right := needle[cut:])
       where the "local period" of the cut is maximal.

       The local period of the cut is the minimal length of a string w
       such that (left endswith w or w endswith left)
       and (right startswith w or w startswith left).

       The Critical Factorization Theorem says that this maximal local
       period is the global period of the string.

       Crochemore and Perrin (1991) show that this cut can be computed
       as the later of two cuts: one that gives a lexicographically
       maximal right half, and one that gives the same with the
       with respect to a reversed alphabet-ordering.

       This is what we want to happen:
           >>> x = "GCAGAGAG"
           >>> cut, period = factorize(x)
           >>> x[:cut], (right := x[cut:])
           ('GC', 'AGAGAG')
           >>> period  # right half period
           2
           >>> right[period:] == right[:-period]
           True

       This is how the local period lines up in the above example:
                GC | AGAGAG
           AGAGAGC = AGAGAGC
       The length of this minimal repetition is 7, which is indeed the
       period of the original string. */

    Py_ssize_t cut1, period1, cut2, period2, cut, period;
    cut1 = STRINGLIB(_lex_search)(needle, len_needle, &period1, 0);
    cut2 = STRINGLIB(_lex_search)(needle, len_needle, &period2, 1);

    // Take the later cut.
    if (cut1 > cut2) {
        period = period1;
        cut = cut1;
    }
    else {
        period = period2;
        cut = cut2;
    }

    *return_period = period;
    return cut;
}


#define SHIFT_TYPE uint8_t
#define MAX_SHIFT UINT8_MAX

#define TABLE_SIZE_BITS 6u
#define TABLE_SIZE (1U << TABLE_SIZE_BITS)
#define TABLE_MASK (TABLE_SIZE - 1U)

typedef struct STRINGLIB(_pre) {
    const STRINGLIB_CHAR *needle;
    Py_ssize_t len_needle;
    Py_ssize_t cut;
    Py_ssize_t period;
    Py_ssize_t gap;
    int is_periodic;
    SHIFT_TYPE table[TABLE_SIZE];
} STRINGLIB(prework);


LOCAL(void)
STRINGLIB(_preprocess)(const STRINGLIB_CHAR *needle, Py_ssize_t len_needle,
                       STRINGLIB(prework) *p)
{
    p->needle = needle;
    p->len_needle = len_needle;
    p->cut = STRINGLIB(_factorize)(needle, len_needle, &(p->period));
    p->is_periodic = (0 == memcmp(needle,
                                  needle + p->period,
                                  p->cut * STRINGLIB_SIZEOF_CHAR));
    if (p->is_periodic) {
        p->gap = 0; // unused
    }
    else {
        // A lower bound on the period
        p->period = Py_MAX(p->cut, len_needle - p->cut) + 1;
        // The gap between the last character and the previous
        // occurrence of an equivalent character (modulo TABLE_SIZE)
        p->gap = len_needle;
        STRINGLIB_CHAR last = needle[len_needle - 1] & TABLE_MASK;
        for (Py_ssize_t i = len_needle - 2; i >= 0; i--) {
            STRINGLIB_CHAR x = needle[i] & TABLE_MASK;
            if (x == last) {
                p->gap = len_needle - 1 - i;
                break;
            }
        }
    }
    // Fill up a compressed Boyer-Moore "Bad Character" table
    Py_ssize_t not_found_shift = Py_MIN(len_needle, MAX_SHIFT);
    for (Py_ssize_t i = 0; i < (Py_ssize_t)TABLE_SIZE; i++) {
        p->table[i] = Py_SAFE_DOWNCAST(not_found_shift,
                                       Py_ssize_t, SHIFT_TYPE);
    }
    for (Py_ssize_t i = len_needle - not_found_shift; i < len_needle; i++) {
        SHIFT_TYPE shift = Py_SAFE_DOWNCAST(len_needle - 1 - i,
                                            Py_ssize_t, SHIFT_TYPE);
        p->table[needle[i] & TABLE_MASK] = shift;
    }
}


static inline Py_ssize_t
STRINGLIB(_two_way)(const STRINGLIB_CHAR *haystack, Py_ssize_t len_haystack,
                    STRINGLIB(prework) *p)
{
    // Crochemore and Perrin's (1991) Two-Way algorithm.
    // See http://www-igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260
    const Py_ssize_t len_needle = p->len_needle;
    const Py_ssize_t cut = p->cut;
    Py_ssize_t period = p->period;
    const STRINGLIB_CHAR *const needle = p->needle;
    const STRINGLIB_CHAR *window_last = haystack + len_needle - 1;
    const STRINGLIB_CHAR *const haystack_end = haystack + len_haystack;
    SHIFT_TYPE *table = p->table;
    const STRINGLIB_CHAR *window;

    if (p->is_periodic) {
        Py_ssize_t memory = 0;
      periodicwindowloop:
        while (window_last < haystack_end) {
            for (;;) {
                Py_ssize_t shift = table[(*window_last) & TABLE_MASK];
                window_last += shift;
                if (shift == 0) {
                    break;
                }
                if (window_last >= haystack_end) {
                    return -1;
                }
            }
          no_shift:
            window = window_last - len_needle + 1;
            Py_ssize_t i = Py_MAX(cut, memory);
            for (; i < len_needle; i++) {
                if (needle[i] != window[i]) {
                    window_last += i - cut + 1;
                    memory = 0;
                    goto periodicwindowloop;
                }
            }
            for (i = memory; i < cut; i++) {
                if (needle[i] != window[i]) {
                    window_last += period;
                    memory = len_needle - period;
                    if (window_last >= haystack_end) {
                        return -1;
                    }
                    Py_ssize_t shift = table[(*window_last) & TABLE_MASK];
                    if (shift) {
                        // A mismatch has been identified to the right
                        // of where i will next start, so we can jump
                        // at least as far as if the mismatch occurred
                        // on the first comparison.
                        Py_ssize_t mem_jump = Py_MAX(cut, memory) - cut + 1;
                        memory = 0;
                        window_last += Py_MAX(shift, mem_jump);
                        goto periodicwindowloop;
                    }
                    goto no_shift;
                }
            }
            return window - haystack;
        }
    }
    else {
        Py_ssize_t gap = p->gap;
        period = Py_MAX(gap, period);
        Py_ssize_t gap_jump_end = Py_MIN(len_needle, cut + gap);
      windowloop:
        while (window_last < haystack_end) {
            for (;;) {
                Py_ssize_t shift = table[(*window_last) & TABLE_MASK];
                window_last += shift;
                if (shift == 0) {
                    break;
                }
                if (UNLIKELY(window_last >= haystack_end)) {
                    return -1;
                }
            }
            window = window_last - len_needle + 1;
            for (Py_ssize_t i = cut; i < gap_jump_end; i++) {
                if (needle[i] != window[i]) {
                    window_last += gap;
                    goto windowloop;
                }
            }
            for (Py_ssize_t i = gap_jump_end; i < len_needle; i++) {
                if (needle[i] != window[i]) {
                    window_last += i - cut + 1;
                    goto windowloop;
                }
            }
            for (Py_ssize_t i = 0; i < cut; i++) {
                if (needle[i] != window[i]) {
                    window_last += period;
                    goto windowloop;
                }
            }
            return window - haystack;
        }
    }
    return -1;
}


LOCAL(Py_ssize_t)
STRINGLIB(_two_way_find)(const STRINGLIB_CHAR *haystack,
                         Py_ssize_t len_haystack,
                         const STRINGLIB_CHAR *needle,
                         Py_ssize_t len_needle)
{
    STRINGLIB(prework) p;
    STRINGLIB(_preprocess)(needle, len_needle, &p);
    return STRINGLIB(_two_way)(haystack, len_haystack, &p);
}


LOCAL(Py_ssize_t)
STRINGLIB(_two_way_count)(const STRINGLIB_CHAR *haystack,
                          Py_ssize_t len_haystack,
                          const STRINGLIB_CHAR *needle,
                          Py_ssize_t len_needle)
{
    STRINGLIB(prework) p;
    STRINGLIB(_preprocess)(needle, len_needle, &p);
    Py_ssize_t index = 0, count = 0;
    while (1) {
        Py_ssize_t result;
        result = STRINGLIB(_two_way)(haystack + index,
                                     len_haystack - index, &p);
        if (result == -1) {
            return count;
        }
        count++;
        index += result + len_needle;
    }
    return count;
}

#undef SHIFT_TYPE
#undef NOT_FOUND
#undef SHIFT_OVERFLOW
#undef TABLE_SIZE_BITS
#undef TABLE_SIZE
#undef TABLE_MASK

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(default_find)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                        const STRINGLIB_CHAR* p, Py_ssize_t m)
{
    const Py_ssize_t w = n - m;
    Py_ssize_t mlast = m - 1;
    Py_ssize_t gap = mlast;
    const STRINGLIB_CHAR last = p[mlast];
    const STRINGLIB_CHAR *const ss = &s[mlast];

    unsigned long mask = 0;
    for (Py_ssize_t i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, p[i]);
        if (p[i] == last) {
            gap = mlast - i - 1;
        }
    }
    STRINGLIB_BLOOM_ADD(mask, last);

    for (Py_ssize_t i = 0; i <= w; i++) {
        if (ss[i] == last) {
            /* candidate match */
            Py_ssize_t j;
            for (j = 0; j < mlast; j++) {
                if (s[i+j] != p[j]) {
                    break;
                }
            }
            if (j == mlast) {
                /* got a match! */
                return i;
            }
            /* miss: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
            else {
                i = i + gap;
            }
        }
        else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
        }
    }
    return -1;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(default_count)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                         const STRINGLIB_CHAR* p, Py_ssize_t m)
{
    const Py_ssize_t w = n - m;
    Py_ssize_t mlast = m - 1, count = 0;
    Py_ssize_t gap = mlast;
    const STRINGLIB_CHAR last = p[mlast];
    const STRINGLIB_CHAR *const ss = &s[mlast];

    unsigned long mask = 0;
    for (Py_ssize_t i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, p[i]);
        if (p[i] == last) {
            gap = mlast - i - 1;
        }
    }
    STRINGLIB_BLOOM_ADD(mask, last);

    for (Py_ssize_t i = 0; i <= w; i++) {
        if (ss[i] == last) {
            /* candidate match */
            Py_ssize_t j;
            for (j = 0; j < mlast; j++) {
                if (s[i+j] != p[j]) {
                    break;
                }
            }
            if (j == mlast) {
                /* got a match! */
                count++;
                i = i + mlast;
                continue;
            }
            /* miss: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
            else {
                i = i + gap;
            }
        }
        else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
        }
    }
    return count;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(adaptive_find)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                         const STRINGLIB_CHAR* p, Py_ssize_t m)
{
    const Py_ssize_t w = n - m;
    Py_ssize_t mlast = m - 1;
    Py_ssize_t gap = mlast;
    Py_ssize_t hits = 0, res;
    const STRINGLIB_CHAR last = p[mlast];
    const STRINGLIB_CHAR *const ss = &s[mlast];

    unsigned long mask = 0;
    for (Py_ssize_t i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, p[i]);
        if (p[i] == last) {
            gap = mlast - i - 1;
        }
    }
    STRINGLIB_BLOOM_ADD(mask, last);

    for (Py_ssize_t i = 0; i <= w; i++) {
        if (ss[i] == last) {
            /* candidate match */
            Py_ssize_t j;
            for (j = 0; j < mlast; j++) {
                if (s[i+j] != p[j]) {
                    break;
                }
            }
            if (j == mlast) {
                /* got a match! */
                return i;
            }
            hits += j + 1;
            if (hits > m / 4 && w - i > 2000) {
                res = STRINGLIB(_two_way_find)(s + i, n - i, p, m);
                return res == -1 ? -1 : res + i;
            }
            /* miss: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
            else {
                i = i + gap;
            }
        }
        else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
        }
    }
    return -1;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(adaptive_count)(const STRINGLIB_CHAR* s, Py_ssize_t n,
                          const STRINGLIB_CHAR* p, Py_ssize_t m)
{
    const Py_ssize_t w = n - m;
    Py_ssize_t mlast = m - 1, count = 0;
    Py_ssize_t gap = mlast;
    Py_ssize_t hits = 0, res;
    const STRINGLIB_CHAR last = p[mlast];
    const STRINGLIB_CHAR *const ss = &s[mlast];

    unsigned long mask = 0;
    for (Py_ssize_t i = 0; i < mlast; i++) {
        STRINGLIB_BLOOM_ADD(mask, p[i]);
        if (p[i] == last) {
            gap = mlast - i - 1;
        }
    }
    STRINGLIB_BLOOM_ADD(mask, last);

    for (Py_ssize_t i = 0; i <= w; i++) {
        if (ss[i] == last) {
            /* candidate match */
            Py_ssize_t j;
            for (j = 0; j < mlast; j++) {
                if (s[i+j] != p[j]) {
                    break;
                }
            }
            if (j == mlast) {
                /* got a match! */
                count++;
                i = i + mlast;
                continue;
            }
            hits += j + 1;
            if (hits > m / 4 && w - i > 2000) {
                res = STRINGLIB(_two_way_count)(s + i, n - i, p, m);
                return res + count;
            }
            /* miss: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
            else {
                i = i + gap;
            }
        }
        else {
            /* skip: check if next character is part of pattern */
            if (!STRINGLIB_BLOOM(mask, ss[i+1])) {
                i = i + m;
            }
        }
    }
    return count;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(find)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sub, Py_ssize_t sub_len,
                Py_ssize_t offset)
{
    Py_ssize_t pos;

    if (LIKELY(str_len < 30000 || sub_len < 6)) {
        pos = STRINGLIB(default_find)(str, str_len, sub, sub_len);
    }
    else if ((sub_len >> 2) * 3 < (str_len >> 2)) {
        pos = STRINGLIB(_two_way_find)(str, str_len, sub, sub_len);
    }
    else {
        pos = STRINGLIB(adaptive_find)(str, str_len, sub, sub_len);
    }

    if (pos >= 0)
        pos += offset;

    return pos;
}


Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(count)(const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                 const STRINGLIB_CHAR* sub, Py_ssize_t sub_len)
{
    Py_ssize_t count;

    if (LIKELY(str_len < 30000 || sub_len < 6)) {
        count = STRINGLIB(default_count)(str, str_len, sub, sub_len);
    }
    else if ((sub_len >> 2) * 3 < (str_len >> 2)) {
        count = STRINGLIB(_two_way_count)(str, str_len, sub, sub_len);
    }
    else {
        count = STRINGLIB(adaptive_count)(str, str_len, sub, sub_len);
    }

    return count;
}
