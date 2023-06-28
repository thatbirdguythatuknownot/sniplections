#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <float.h>

#if defined(__OPTIMIZE__)

/* LIKELY(), UNLIKELY(), BLIKELY(), BUNLIKELY() definition */
/* Checks taken from
    https://github.com/python/cpython/blob/main/Objects/obmalloc.c
   Only use LIKELY()/UNLIKELY() when entirely sure that the argument
    is a boolean (0 or 1). Otherwise, use BLIKELY()/BUNLIKELY(). */
#if defined(__GNUC__) && (__GNUC__ > 2)
#  define UNLIKELY(value) __builtin_expect((value), 0)
#  define LIKELY(value) __builtin_expect((value), 1)
#  define BUNLIKELY(value) __builtin_expect(!!(value), 0)
#  define BLIKELY(value) __builtin_expect(!!(value), 1)
#else
#  define UNLIKELY(value) (value)
#  define LIKELY(value) (value)
#  define BUNLIKELY(value) (!!(value))
#  define BLIKELY(value) (!!(value))
#endif

/* ASSUME() definition */
#ifdef __clang__
#  define ASSUME(value) (void)__builtin_assume(value)
#elif defined(__GNUC__) && (__GNUC__ > 4) && (__GNUC_MINOR__ >= 5)
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79469
   `__builtin_object_size(( (void)(value), "" ), 2)` checks whether the
    expression is constant. */
#  define ASSUME(value) \
    (__builtin_object_size(( (void)(value), "" ), 2) \
        ? ((value) ? (void)0 : __builtin_unreachable()) \
        : (void)0 \
    )
#elif defined(_MSC_VER)
/* We don't have the "just to make sure it's constant" check here. */
#  define ASSUME(value) (void)__assume(value)
#else
#  define ASSUME(value) ((void)0)
#endif

/* LOCAL() definition */
#ifdef _MSC_VER
#  define LOCAL(type) static inline __forceinline type __fastcall
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#  define LOCAL(type) static inline __attribute__((always_inline)) type
#else
#  define LOCAL(type) static inline type
#endif

#endif /* __OPTIMIZE__ */

#define SET_ERROR_RET(err, msg) \
    PyErr_SetString(PyExc_ ## err ## Error, msg); \
    return NULL; \

#define FMT_ERROR_RET(err, msg, ...) \
    PyErr_Format(PyExc_ ## err ## Error, msg, __VA_ARGS__); \
    return NULL; \

LOCAL(unsigned long long)
rand_ull(void)
{
    unsigned long long r = 0;

    /*
    msb (max)   i*8
    14            0
    22            8
    30           16
    38           24
    46           32
    54           40
    62           48
    !!           56 (should not be reached)
    */
    for (int i = 0; i < sizeof(unsigned long long) - 1; i++) {
        r = (r << 8) ^ rand();
    }

    return r;
}

LOCAL(double)
rand_to_double(unsigned long long r)
{
    r %= 1ULL << DBL_MANT_DIG;

    return (double)r / (double)(1ULL << DBL_MANT_DIG);
}

LOCAL(double)
rand_double(void)
{
    return rand_to_double(rand_ull());
}

/* Generic helper macro to convert characters of different types.
   from_type and to_type have to be valid type names, begin and end
   are pointers to the source characters which should be of type
   "from_type *".  to is a pointer of type "to_type *" and points to the
   buffer where the result characters are written to. */
#define _PyUnicode_CONVERT_BYTES(from_type, to_type, begin, end, to) \
    do {                                                \
        to_type *_to = (to_type *)(to);                 \
        const from_type *_iter = (const from_type *)(begin);\
        const from_type *_end = (const from_type *)(end);\
        Py_ssize_t n = (_end) - (_iter);                \
        const from_type *_unrolled_end =                \
            _iter + _Py_SIZE_ROUND_DOWN(n, 4);          \
        while (_iter < (_unrolled_end)) {               \
            _to[0] = (to_type) _iter[0];                \
            _to[1] = (to_type) _iter[1];                \
            _to[2] = (to_type) _iter[2];                \
            _to[3] = (to_type) _iter[3];                \
            _iter += 4; _to += 4;                       \
        }                                               \
        while (_iter < (_end))                          \
            *_to++ = (to_type) *_iter++;                \
    } while (0)

#define STRINGLIB(F)            ucs1_##F
#define STRINGLIB_SIZEOF_CHAR   1
#define STRINGLIB_CHAR          Py_UCS1
#define STRINGLIB_FAST_MEMCHR memchr
#include "uwuifier.stringlib.h"
#undef STRINGLIB
#undef STRINGLIB_SIZEOF_CHAR
#undef STRINGLIB_CHAR
#undef STRINGLIB_FAST_MEMCHR

#define STRINGLIB(F)            ucs2_##F
#define STRINGLIB_SIZEOF_CHAR   2
#define STRINGLIB_CHAR          Py_UCS2
#if SIZEOF_WCHAR_T == 2
#define STRINGLIB_FAST_MEMCHR(s, c, n)              \
    (Py_UCS2 *)wmemchr((const wchar_t *)(s), c, n)
#endif
#include "uwuifier.stringlib.h"
#undef STRINGLIB
#undef STRINGLIB_SIZEOF_CHAR
#undef STRINGLIB_CHAR
#undef STRINGLIB_FAST_MEMCHR

#define STRINGLIB(F)            ucs4_##F
#define STRINGLIB_SIZEOF_CHAR   4
#define STRINGLIB_CHAR          Py_UCS4
#if SIZEOF_WCHAR_T == 4
#define STRINGLIB_FAST_MEMCHR(s, c, n)              \
    (Py_UCS2 *)wmemchr((const wchar_t *)(s), c, n)
#endif
#include "uwuifier.stringlib.h"
#undef STRINGLIB
#undef STRINGLIB_SIZEOF_CHAR
#undef STRINGLIB_CHAR
#undef STRINGLIB_FAST_MEMCHR

LOCAL(Py_UCS4)
any_getchar(const void *s, const Py_ssize_t i,
            const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ((const Py_UCS1 *)s)[i];
    case PyUnicode_2BYTE_KIND:
        return ((const Py_UCS2 *)s)[i];
    case PyUnicode_4BYTE_KIND:
        return ((const Py_UCS4 *)s)[i];
    default:
        ASSUME(0);
    }
}

LOCAL(void)
any_setchar(void *s, const Py_ssize_t i,
            const Py_UCS4 x, const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        ((Py_UCS1 *)s)[i] = x;
        break;
    case PyUnicode_2BYTE_KIND:
        ((Py_UCS2 *)s)[i] = x;
        break;
    case PyUnicode_4BYTE_KIND:
        ((Py_UCS4 *)s)[i] = x;
        break;
    default:
        ASSUME(0);
    }
}

LOCAL(int)
any_charisalpha(const void *s, const Py_ssize_t i,
                const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return isalpha(((const Py_UCS1 *)s)[i]);
    case PyUnicode_2BYTE_KIND:
        return isalpha(((const Py_UCS2 *)s)[i]);
    case PyUnicode_4BYTE_KIND:
        return isalpha(((const Py_UCS4 *)s)[i]);
    default:
        ASSUME(0);
    }
}

LOCAL(int)
any_charisword(const void *s, const Py_ssize_t i,
               const int kind)
{
    int ch;
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        ch = ((const Py_UCS1 *)s)[i];
        if (ch < 128) return isalnum(ch) || ch == '_';
        break;
    case PyUnicode_2BYTE_KIND:
        ch = ((const Py_UCS2 *)s)[i];
        break;
    case PyUnicode_4BYTE_KIND:
        ch = ((const Py_UCS4 *)s)[i];
        break;
    default:
        ASSUME(0);
    }
    return _PyUnicode_IsAlpha(ch) || _PyUnicode_IsNumeric(ch);
}

LOCAL(int)
any_checkchar(const void *s, const Py_ssize_t i,
              const Py_UCS4 x, const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ((Py_UCS1 *)s)[i] == x;
    case PyUnicode_2BYTE_KIND:
        return ((Py_UCS2 *)s)[i] == x;
    case PyUnicode_4BYTE_KIND:
        return ((Py_UCS4 *)s)[i] == x;
    default:
        ASSUME(0);
    }
}

#define CASE(x, cond, handle) \
    case PyUnicode_##x##BYTE_KIND: { \
        Py_UCS##x *buf = (Py_UCS##x *)s; \
        for (Py_ssize_t i = 0; i < n; i++) { \
            if (cond) { \
                handle; \
            } \
        } \
        break; \
    } \

LOCAL(Py_ssize_t)
any_findspace(const void *s, const Py_ssize_t n,
              const int kind)
{
    switch (kind) {
    CASE(1, UNLIKELY(Py_UNICODE_ISSPACE(buf[i])), return i)
    CASE(2, UNLIKELY(Py_UNICODE_ISSPACE(buf[i])), return i)
    CASE(4, UNLIKELY(Py_UNICODE_ISSPACE(buf[i])), return i)
    default:
        ASSUME(0);
    }

    return -1;
}

LOCAL(Py_ssize_t)
any_findemotiable(const void *s, const Py_ssize_t n,
                  const int kind)
{
    switch (kind) {
    CASE(1, UNLIKELY(buf[i] == '.' || buf[i] == '!' ||
                     buf[i] == '?' || buf[i] == '\r' ||
                     buf[i] == '\n' || buf[i] == '\t'), return i)
    CASE(2, UNLIKELY(buf[i] == '.' || buf[i] == '!' ||
                     buf[i] == '?' || buf[i] == '\r' ||
                     buf[i] == '\n' || buf[i] == '\t'), return i)
    CASE(4, UNLIKELY(buf[i] == '.' || buf[i] == '!' ||
                     buf[i] == '?' || buf[i] == '\r' ||
                     buf[i] == '\n' || buf[i] == '\t'), return i)
    default:
        ASSUME(0);
    }

    return -1;
}

LOCAL(Py_ssize_t)
any_findchar(const void *s, const Py_ssize_t n,
             const Py_UCS4 c, const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1_find_char(s, n, (Py_UCS1)c);
    case PyUnicode_2BYTE_KIND:
        return ucs2_find_char(s, n, (Py_UCS2)c);
    case PyUnicode_4BYTE_KIND:
        return ucs4_find_char(s, n, c);
    default:
        ASSUME(0);
    }
}

LOCAL(Py_ssize_t)
any_find(const void *s, const Py_ssize_t n,
         const void *p, const Py_ssize_t m,
         const Py_ssize_t o, const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1_find(s, n, p, m, o);
    case PyUnicode_2BYTE_KIND:
        return ucs2_find(s, n, p, m, o);
    case PyUnicode_4BYTE_KIND:
        return ucs4_find(s, n, p, m, o);
    default:
        ASSUME(0);
    }
}

LOCAL(Py_ssize_t)
any_count(const void *s, const Py_ssize_t n,
          const void *p, const Py_ssize_t m,
          const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1_count(s, n, p, m);
    case PyUnicode_2BYTE_KIND:
        return ucs2_count(s, n, p, m);
    case PyUnicode_4BYTE_KIND:
        return ucs4_count(s, n, p, m);
    default:
        ASSUME(0);
    }
}

LOCAL(Py_ssize_t)
any_countchar(const void *s, const Py_ssize_t n,
              const Py_UCS4 c, const int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1_count_char(s, n, (Py_UCS1)c);
    case PyUnicode_2BYTE_KIND:
        return ucs2_count_char(s, n, (Py_UCS2)c);
    case PyUnicode_4BYTE_KIND:
        return ucs4_count_char(s, n, c);
    default:
        ASSUME(0);
    }
}

LOCAL(Py_ssize_t)
any_countspace(const void *s, const Py_ssize_t n,
               const int kind)
{
    Py_ssize_t count = 0;

    switch (kind) {
    CASE(1, UNLIKELY(Py_UNICODE_ISSPACE(buf[i])), count++)
    CASE(2, UNLIKELY(Py_UNICODE_ISSPACE(buf[i])), count++)
    CASE(4, UNLIKELY(Py_UNICODE_ISSPACE(buf[i])), count++)
    default:
        ASSUME(0);
    }

    return count;
}

LOCAL(Py_ssize_t)
any_countemotiable(const void *s, const Py_ssize_t n,
                   const int kind)
{
    Py_ssize_t count = 0;

    switch (kind) {
    CASE(1, UNLIKELY(buf[i] == '.' || buf[i] == '!' ||
                     buf[i] == '?' || buf[i] == '\r' ||
                     buf[i] == '\n' || buf[i] == '\t'), count++)
    CASE(2, UNLIKELY(buf[i] == '.' || buf[i] == '!' ||
                     buf[i] == '?' || buf[i] == '\r' ||
                     buf[i] == '\n' || buf[i] == '\t'), count++)
    CASE(4, UNLIKELY(buf[i] == '.' || buf[i] == '!' ||
                     buf[i] == '?' || buf[i] == '\r' ||
                     buf[i] == '\n' || buf[i] == '\t'), count++)
    default:
        ASSUME(0);
    }

    return count;
}

#undef CASE

LOCAL(void *)
ascii_askind(void const *data, Py_ssize_t len, int kind)
{
    void *result;

    switch (kind) {
    case PyUnicode_2BYTE_KIND:
        result = PyMem_New(Py_UCS2, len);
        if (UNLIKELY(!result))
            return PyErr_NoMemory();
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS2,
            (const Py_UCS1 *)data,
            ((const Py_UCS1 *)data) + len,
            result);
        return result;
    case PyUnicode_4BYTE_KIND:
        result = PyMem_New(Py_UCS4, len);
        if (UNLIKELY(!result))
            return PyErr_NoMemory();
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS4,
            (const Py_UCS1 *)data,
            ((const Py_UCS1 *)data) + len,
            result);
        return result;
    default:
        ASSUME(0);
    }
}

LOCAL(void)
copy_fromLE_toU2p(const void *from, void *to, Py_ssize_t len,
                  int kind0, int kind1)
{
    switch (kind0) {
    case PyUnicode_1BYTE_KIND:
        if (kind1 == PyUnicode_2BYTE_KIND) {
            _PyUnicode_CONVERT_BYTES(
                Py_UCS1, Py_UCS2,
                from,
                ((const Py_UCS1 *)from) + len,
                to);
        }
        else {
            _PyUnicode_CONVERT_BYTES(
                Py_UCS1, Py_UCS4,
                from,
                ((const Py_UCS1 *)from) + len,
                to);
        }
        break;
    case PyUnicode_2BYTE_KIND:
        if (kind1 == PyUnicode_2BYTE_KIND) {
            memcpy(to, from, len + len);
        }
        else {
            _PyUnicode_CONVERT_BYTES(
                Py_UCS2, Py_UCS4,
                from,
                ((const Py_UCS2 *)from) + len,
                to);
        }
        break;
    case PyUnicode_4BYTE_KIND:
        memcpy(to, from, 4*len);
        break;
    default:
        ASSUME(0);
    }
}

LOCAL(void)
copy_fromU2p_to1B(const void *from, void *to, Py_ssize_t len,
                  int kind)
{
    switch (kind) {
    case PyUnicode_2BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(
            Py_UCS2, Py_UCS1,
            from,
            ((const Py_UCS2 *)from) + len,
            to);
        break;
    case PyUnicode_4BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(
            Py_UCS4, Py_UCS1,
            from,
            ((const Py_UCS4 *)from) + len,
            to);
        break;
    default:
        ASSUME(0);
    }
}

LOCAL(Py_UCS4)
handle_capital_sigma(int kind, const void *data, Py_ssize_t length, Py_ssize_t i)
{
    Py_ssize_t j;
    int final_sigma;
    Py_UCS4 c = 0;   /* initialize to prevent gcc warning */
    /* U+03A3 is in the Final_Sigma context when, it is found like this:

     \p{cased}\p{case-ignorable}*U+03A3!(\p{case-ignorable}*\p{cased})

    where ! is a negation and \p{xxx} is a character with property xxx.
    */
    for (j = i - 1; j >= 0; j--) {
        c = PyUnicode_READ(kind, data, j);
        if (!_PyUnicode_IsCaseIgnorable(c))
            break;
    }
    if ((final_sigma = j >= 0 && _PyUnicode_IsCased(c))) {
        for (j = i + 1; j < length; j++) {
            c = PyUnicode_READ(kind, data, j);
            if (!_PyUnicode_IsCaseIgnorable(c))
                break;
        }
        final_sigma = j == length || !_PyUnicode_IsCased(c);
    }
    return final_sigma ? 0x3C2 : 0x3C3;
}

LOCAL(int)
lower_ucs4(int kind, const void *data, Py_ssize_t length, Py_ssize_t i,
           Py_UCS4 c, Py_UCS4 *mapped)
{
    /* Obscure special case. */
    if (c == 0x3A3) {
        mapped[0] = handle_capital_sigma(kind, data, length, i);
        return 1;
    }
    return _PyUnicode_ToLowerFull(c, mapped);
}

LOCAL(Py_ssize_t)
do_lower(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res,
         Py_UCS4 *maxchar)
{
    Py_ssize_t i, k = 0;

    for (i = 0; i < length; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, data, i), mapped[3];
        int n_res, j;
        if (UNLIKELY(c == 0x3A3)) {
            mapped[0] = handle_capital_sigma(kind, data, length, i);
            n_res = 1;
        }
        else {
            n_res = _PyUnicode_ToLowerFull(c, mapped);
        }
        for (j = 0; j < n_res; j++) {
            *maxchar = Py_MAX(*maxchar, mapped[j]);
            res[k++] = mapped[j];
        }
    }
    return k;
}

LOCAL(PyObject *)
case_lower(PyObject *self)
{
    PyObject *res;
    Py_ssize_t length, newlength;
    int kind, outkind;
    const void *data;
    void *outdata;
    Py_UCS4 maxchar = 0, *tmp, *tmpend;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    length = PyUnicode_GET_LENGTH(self);
    if (UNLIKELY((size_t) length > PY_SSIZE_T_MAX / (3 * sizeof(Py_UCS4)))) {
        SET_ERROR_RET(Overflow, "string is too long");
    }
    tmp = PyMem_Malloc(sizeof(Py_UCS4) * 3 * length);
    if (UNLIKELY(tmp == NULL)) return PyErr_NoMemory();
    newlength = do_lower(kind, data, length, tmp, &maxchar);
    res = PyUnicode_New(newlength, maxchar);
    if (UNLIKELY(res == NULL)) goto leave;
    tmpend = tmp + newlength;
    outdata = PyUnicode_DATA(res);
    outkind = PyUnicode_KIND(res);
    switch (outkind) {
    case PyUnicode_1BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS1, tmp, tmpend, outdata);
        break;
    case PyUnicode_2BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS2, tmp, tmpend, outdata);
        break;
    case PyUnicode_4BYTE_KIND:
        memcpy(outdata, tmp, sizeof(Py_UCS4) * newlength);
        break;
    default:
        Py_UNREACHABLE();
    }
  leave:
    PyMem_Free(tmp);
    return res;
}

LOCAL(PyObject *)
word_replace_impl(PyObject *text)
{
    Py_ssize_t maxchar, i, j, ires, sn, n, n2, new_size;
    int r1 = 0, r2 = 0, resr = 0;
    int kind;
    char *res;
    const char *sbuf;
    const void *buf1, *buf2;
    PyObject *u;

    new_size = sn = PyUnicode_GET_LENGTH(text);
    if (UNLIKELY(sn < 4)) {
        return Py_NewRef(text);
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(text);
    kind = PyUnicode_KIND(text);
    sbuf = PyUnicode_DATA(text);

#define SAMELEN_REPLACE(S1, S2, LEN) \
    do { \
        if (UNLIKELY(PyUnicode_1BYTE_KIND < kind)) { \
            buf1 = ascii_askind(S1, LEN, kind); \
            if (UNLIKELY(!buf1)) goto error; \
            r1 = 1; \
            i = any_find(sbuf, sn, buf1, LEN, 0, kind); \
            if (i < 0) { \
                PyMem_Free((void *)buf1); \
                break; \
            } \
            buf2 = ascii_askind(S2, LEN, kind); \
            if (UNLIKELY(!buf2)) goto error; \
            r2 = 1; \
            n2 = kind * LEN; \
        } \
        else { \
            i = ucs1_find((const void *)sbuf, sn, \
                          (const void *)S1, LEN, 0); \
            if (i < 0) break; \
            buf1 = S1; \
            buf2 = S2; \
            n2 = LEN; \
        } \
        if (!resr) { \
            res = PyMem_New(char, kind * sn); \
            if (UNLIKELY(!res)) goto error; \
            memcpy(res, sbuf, kind * sn); \
            sbuf = res; \
            resr = 1; \
        } \
        memcpy(res + kind*i, buf2, n2); \
        i += LEN; \
        while (1) { \
            i = any_find(sbuf + kind*i, sn-i, buf1, LEN, i, kind); \
            if (i == -1) break; \
            memcpy(res + kind*i, buf2, n2); \
            i += LEN; \
        } \
    } while (0); \

#define NORMAL_REPLACE(S1, L1, S2, L2) \
    do { \
        int free_sbuf = 0; \
        if (UNLIKELY(PyUnicode_1BYTE_KIND < kind)) { \
            buf1 = ascii_askind(S1, L1, kind); \
            if (UNLIKELY(!buf1)) goto error; \
            r1 = 1; \
            n = any_count(sbuf, sn, buf1, L1, kind); \
            if (n == 0) { \
                PyMem_Free((void *)buf1); \
                break; \
            } \
            buf2 = ascii_askind(S2, L2, kind); \
            if (UNLIKELY(!buf2)) goto error; \
            r2 = 1; \
            n2 = kind * L2; \
        } \
        else { \
            n = ucs1_count((const void *)sbuf, sn, (const void *)S1, L1); \
            if (n == 0) break; \
            buf1 = S1; \
            buf2 = S2; \
            n2 = L2; \
        } \
        if (UNLIKELY(L1 < L2 && L2 - L1 > (PY_SSIZE_T_MAX - sn) / n)) { \
                PyErr_SetString(PyExc_OverflowError, \
                                "replace string is too long"); \
                goto error; \
        } \
        new_size = sn + n * (L2 - L1); \
        if (UNLIKELY(new_size > (PY_SSIZE_T_MAX / kind))) { \
            PyErr_SetString(PyExc_OverflowError, \
                            "replace string is too long"); \
            goto error; \
        } \
        if (!resr) { \
            res = PyMem_New(char, kind * new_size); \
            if (UNLIKELY(!res)) goto error; \
            resr = 1; \
        } \
        else if (new_size != sn) { \
            res = PyMem_New(char, kind * new_size); \
            if (UNLIKELY(!res)) goto error; \
            free_sbuf = 1; \
        } \
        ires = i = 0; \
        while (n-- > 0) { \
            /* look for next match */ \
            j = any_find(sbuf + kind * i, sn - i, \
                         buf1, L1, i, kind); \
            if (j == -1) break; \
            else if (j > i) { \
                /* copy unchanged part [i:j] */ \
                memcpy(res + kind * ires, \
                       sbuf + kind * i, \
                       kind * (j-i)); \
                ires += j - i; \
            } \
            /* copy substitution string */ \
            memcpy(res + kind * ires, buf2, n2); \
            ires += L2; \
            i = j + L1; \
        } \
        if (i < sn) { \
            /* copy tail [i:] */ \
            memcpy(res + kind * ires, \
                   sbuf + kind * i, \
                   kind * (sn-i)); \
        } \
        if (free_sbuf) { \
            PyMem_Free((void *)sbuf); \
        } \
        sbuf = res; \
        sn = new_size; \
    } while (0); \

    NORMAL_REPLACE("small", 5, "smol", 4)
    NORMAL_REPLACE("cute", 4, "kawaii~", 7)
    SAMELEN_REPLACE("fluff", "floof", 5)
    NORMAL_REPLACE("love", 4, "luv", 3)
    NORMAL_REPLACE("stupid", 6, "baka", 4)
    NORMAL_REPLACE("idiot", 5, "baka", 4)
    SAMELEN_REPLACE("what", "nani", 4)
    SAMELEN_REPLACE("meow", "nya~", 4)
    NORMAL_REPLACE("roar", 4, "rawrr~", 6)

    if (resr) {
        u = PyUnicode_New(new_size, maxchar);
        if (UNLIKELY(!u)) goto error;
        memcpy(PyUnicode_DATA(u), res, kind * new_size);
        return u;
    }

    return Py_NewRef(text);
error:
    if (UNLIKELY(r1)) PyMem_Free((void *)buf1);
    if (UNLIKELY(r2)) PyMem_Free((void *)buf2);
    if (resr) PyMem_Free((void *)sbuf);
    return NULL;
#undef SAMELEN_REPLACE
#undef NORMAL_REPLACE
}

static PyObject *
word_replace(PyObject *Py_UNUSED(_), PyObject *text)
{
    if (UNLIKELY(!PyUnicode_CheckExact(text))) {
        FMT_ERROR_RET(Type,
                      "'word_replace' expected 'str', got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

    return word_replace_impl(text);
}

PyDoc_STRVAR(word_replace__doc__,
"word_replace(text: str) -> str\n\
\n\
    Replaces words that are keys in the word replacement pairs to the\
values specified.\n\
\n\
    Word replacement pairs:\n\
        \"small\" -> \"smol\"\n\
        \"cute\" -> \"kawaii~\"\n\
        \"fluff\" -> \"floof\"\n\
        \"love\" -> \"luv\"\n\
        \"stupid\" -> \"baka\"\n\
        \"idiot\" -> \"baka\"\n\
        \"what\" -> \"nani\"\n\
        \"meow\" -> \"nya~\"\n\
        \"roar\" -> \"rawrr~\"\n");

LOCAL(PyObject *)
nyaify_impl(PyObject *text)
{
    Py_ssize_t maxchar, i, count, sn, ol, n;
    int kind;
    char *res;
    const char *sbuf;
    PyObject *u;

    ol = sn = PyUnicode_GET_LENGTH(text);
    if (UNLIKELY(sn < 3)) {
        return Py_NewRef(text);
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(text);
    kind = PyUnicode_KIND(text);
    sbuf = PyUnicode_DATA(text);

    n = any_countchar(sbuf, sn, 'n', kind);
    if (UNLIKELY(n == 0)) {
        return Py_NewRef(text);
    }

    u = PyUnicode_New(sn + n, maxchar);
    if (UNLIKELY(!u)) return NULL;
    res = PyUnicode_DATA(u);

    count = 0;
    while (sn > 0) {
        i = any_findchar(sbuf, sn, 'n', kind);
        if (i < 0) break;
        else if (i) {
            memcpy(res, sbuf, kind * (i + 1));
            sbuf += kind * (i + 1);
            res += kind * (i + 1);
            sn -= i + 1;
        }
        else {
            any_setchar(res, 0, 'n', kind);
            sbuf += kind;
            res += kind;
            sn--;
        }
        if (any_checkchar(sbuf, 0, 'a', kind) ||
            any_checkchar(sbuf, 0, 'e', kind) ||
            any_checkchar(sbuf, 0, 'i', kind) ||
            any_checkchar(sbuf, 0, 'o', kind) ||
            any_checkchar(sbuf, 0, 'u', kind))
        {
            if (!any_checkchar(sbuf, 1, 'a', kind) &&
                !any_checkchar(sbuf, 1, 'e', kind) &&
                !any_checkchar(sbuf, 1, 'o', kind) &&
                !any_checkchar(sbuf, 1, 'u', kind))
            {
                any_setchar(res, 0, 'y', kind);
                memcpy(res + kind, sbuf, kind * 2);
                sbuf += kind * 2;
                res += kind * 3;
                sn -= 2;
                count++;
            }
        }
    }

    if (sn > 0) {
        memcpy(res, sbuf, kind * sn);
    }

    if (count != n) {
        if (UNLIKELY(PyUnicode_Resize(&u, ol + count) < 0)) {
            return NULL;
        }
    }

    return u;
}

static PyObject *
nyaify(PyObject *Py_UNUSED(_), PyObject *text)
{
    if (UNLIKELY(!PyUnicode_CheckExact(text))) {
        FMT_ERROR_RET(Type,
                      "'nyaify' expected 'str', got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

    return nyaify_impl(text);
}

PyDoc_STRVAR(nyaify__doc__,
"nyaify(text: str) -> str\n\
\n\
    Nyaifies a string by adding a 'y' between an 'n' and a vowel.\n");

LOCAL(PyObject *)
char_replace_impl(PyObject *text)
{
    Py_ssize_t pi, i, j, sn;
    int kind, no_lb = 1;
    char *res;
    const char *sbuf;
    Py_UCS1 chtmp, ch1 = 'l', ch2 = 'r';
    PyObject *u;

    sn = PyUnicode_GET_LENGTH(text);
    if (UNLIKELY(sn < 3)) {
        return Py_NewRef(text);
    }

    kind = PyUnicode_KIND(text);
    sbuf = PyUnicode_DATA(text);

    u = PyUnicode_New(sn, PyUnicode_MAX_CHAR_VALUE(text));
    if (UNLIKELY(!u)) return NULL;
    res = PyUnicode_DATA(u);

redo:
    i = any_findchar(sbuf, sn, ch1, kind);
    if (i < 0) {
        if (!ch2) goto end;
        i = any_findchar(sbuf, sn, ch2, kind);
        if (i < 0) goto end;
        ch1 = ch2;
    }
    else if (ch2) {
        j = any_findchar(sbuf, sn, ch2, kind);
        if (j < 0) {
            ch2 = 0;
        }
        else if (j < i) {
            pi = i;
            i = j;
            j = pi;
            chtmp = ch2;
            ch2 = ch1;
            ch1 = chtmp;
        }
    }
    if (i) {
        memcpy(res, sbuf, kind * i);
        sbuf += kind * i;
        res += kind * i;
        sn -= i + 1;
    }
    else {
        sn--;
    }
    if ((no_lb || !any_checkchar(sbuf, -1, 'w', kind)) &&
        !(sn > 0 && any_checkchar(sbuf, 1, 'w', kind)))
    {
        any_setchar(res, 0, 'w', kind);
    }
    else {
        any_setchar(res, 0, ch1, kind);
    }
    sbuf += kind;
    res += kind;
    no_lb = 0;
    goto redo;

end:
    if (sn > 0) {
        memcpy(res, sbuf, kind * sn);
    }

    return u;
}

static PyObject *
char_replace(PyObject *Py_UNUSED(_), PyObject *text)
{
    if (UNLIKELY(!PyUnicode_CheckExact(text))) {
        FMT_ERROR_RET(Type,
                      "'char_replace' expected 'str', got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

    return char_replace_impl(text);
}

PyDoc_STRVAR(char_replace__doc__,
"char_replace(text: str) -> str\n\
\n\
    Replace certain characters with 'w'.\n");

LOCAL(PyObject *)
stutter_impl(PyObject *text, float strength)
{
    Py_ssize_t maxchar, i, count, sn, ol, n, new_size;
    int kind;
    char *res;
    const char *sbuf;
    PyObject *u;

    if (UNLIKELY(strength <= 0. || isnan(strength))) {
        return Py_NewRef(text);
    }
    else if (strength >= 1.) {
        strength = 1.;
    }

    ol = sn = PyUnicode_GET_LENGTH(text);
    if (UNLIKELY(sn == 0)) {
        return Py_NewRef(text);
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(text);
    kind = PyUnicode_KIND(text);
    sbuf = PyUnicode_DATA(text);

    n = any_countspace(sbuf, sn, kind);
    if (UNLIKELY(n == 0)) {
        int ch = any_getchar(sbuf, 0, kind);
        if (isalpha(ch) && rand_double() < strength) {
            u = PyUnicode_New(sn + 2, maxchar);
            if (UNLIKELY(!u)) return NULL;
            res = PyUnicode_DATA(u);
            any_setchar(res, 0, ch, kind);
            any_setchar(res, 1, '-', kind);
            memcpy(res + kind*2, sbuf, kind * sn);
            return u;
        }
        return Py_NewRef(text);
    }

    new_size = sn + n+n;
    int ch = any_getchar(sbuf, 0, kind);
    int start_stutter;
    if ((start_stutter = isalpha(ch) && rand_double() < strength)) {
        new_size += 2;
    }
    u = PyUnicode_New(new_size, maxchar);
    if (UNLIKELY(!u)) return NULL;
    res = PyUnicode_DATA(u);
    if (start_stutter) {
        any_setchar(res, 0, ch, kind);
        any_setchar(res, 1, '-', kind);
        res += kind * 2;
    }

    count = start_stutter;
    while (sn > 0) {
        i = any_findspace(sbuf, sn, kind);
        if (i < 0) break;
        else if (i) {
            memcpy(res, sbuf, kind * (i + 1));
            sbuf += kind * (i + 1);
            res += kind * (i + 1);
            sn -= i + 1;
        }
        else {
            any_setchar(res, 0, any_getchar(sbuf, 0, kind), kind);
            sbuf += kind;
            res += kind;
            sn--;
        }
        if (any_charisalpha(sbuf, 0, kind) && rand_double() < strength) {
            Py_UCS4 ch = any_getchar(sbuf, 0, kind);
            any_setchar(res, 0, ch, kind);
            any_setchar(res, 1, '-', kind);
            any_setchar(res, 2, ch, kind);
            sbuf += kind;
            res += kind * 3;
            sn--;
            count++;
        }
    }

    if (sn > 0) {
        memcpy(res, sbuf, kind * sn);
    }

    if (count != n) {
        if (UNLIKELY(PyUnicode_Resize(&u, ol + count+count) < 0)) {
            return NULL;
        }
    }

    return u;
}

static PyObject *
stutter(PyObject *Py_UNUSED(_),
        PyObject *const *args,
        Py_ssize_t nargs,
        PyObject *kwnames)
{
    float strength;
    PyObject *text, *strength_o = NULL;

    Py_ssize_t nkwargs = kwnames ? Py_SIZE(kwnames) : 0;
    Py_ssize_t total_args = nargs + nkwargs;

    if (UNLIKELY(total_args < 1 || total_args > 2)) {
        FMT_ERROR_RET(Type,
                      "'stutter' expected 1 or 2 args, got %zd",
                      total_args);
    }

    if (kwnames) {
        PyObject *key;

        switch (nkwargs) {
        case 2:
            key = PyTuple_GET_ITEM(kwnames, 0);
            if (LIKELY(memcmp(PyUnicode_DATA(key), "text", 5) == 0)) {
                text = args[0];
                key = PyTuple_GET_ITEM(kwnames, 1);
                if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "strength", 9))) {
                    FMT_ERROR_RET(Type,
                                  "'stutter' received unexpected keyword "
                                  "argument '%S'",
                                  key);
                }
                strength_o = args[1];
            }
            else if (memcmp(PyUnicode_DATA(key), "strength", 9) == 0) {
                strength_o = args[0];
                key = PyTuple_GET_ITEM(kwnames, 1);
                if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "text", 5))) {
                    FMT_ERROR_RET(Type,
                                  "'stutter' received unexpected keyword "
                                  "argument '%S'",
                                  key);
                }
                text = args[1];
            }
            else {
                FMT_ERROR_RET(Type,
                              "'stutter' received unexpected keyword "
                              "argument '%S'",
                              key);
            }
            break;
        case 1:
            key = PyTuple_GET_ITEM(kwnames, 0);
            if (LIKELY(memcmp(PyUnicode_DATA(key), "strength", 9) == 0)) {
                if (UNLIKELY(nargs == 0)) {
                    SET_ERROR_RET(Type,
                                  "'stutter' missing required argument "
                                  "'text'");
                }
                strength_o = args[1];
            }
            else if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "text", 5))) {
                FMT_ERROR_RET(Type,
                              "'stutter' received unexpected keyword "
                              "argument '%S'",
                              key);
            }
            text = args[0];
        case 0:
            break;
        default:
            ASSUME(0);
        }
    }
    else {
        if (nargs == 2) {
            strength_o = args[1];
        }
        text = args[0];
    }

    if (UNLIKELY(!PyUnicode_CheckExact(text))) {
        FMT_ERROR_RET(Type,
                      "'stutter' expected 'str' for 'text' argument, "
                      "got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

    if (UNLIKELY(strength_o && !PyFloat_CheckExact(strength_o))) {
        FMT_ERROR_RET(Type,
                      "'stutter' expected 'float' for 'strength' "
                      "argument, got '%.200s'",
                      Py_TYPE(strength_o)->tp_name);
    }

    if (strength_o) {
        strength = PyFloat_AS_DOUBLE(strength_o);
    }
    else {
        return Py_NewRef(text);
    }

    return stutter_impl(text, strength);
}

PyDoc_STRVAR(stutter__doc__,
"stutter(text: str, strength: float = 0.0) -> str\n\
\n\
    Adds stuttering to a string by replacing a single character at the\n\
    beginning of a word with itself, a dash, and itself again.\n");

static struct {
    const Py_ssize_t len;
    const Py_UCS2 data[11];
} emoji_table[32] = {
    {7, {'r', 'a', 'w', 'r', ' ', 'x', '3'}},
    {3, {'O', 'w', 'O'}},
    {3, {'U', 'w', 'U'}},
    {3, {'o', '.', 'O'}},
    {3, {'-', '.', '-'}},
    {3, {'>', 'w', '<'}},
    {6, {'(', 0x2445, 0x2D8, 0xA4B3, 0x2D8, ')'}},
    {5, {'(', 0xA20D, 0x1D17, 0xA20D, ')'}},
    {5, {'(', 0x2D8, 0x3C9, 0x2D8, ')'}},
    {8, {'(', 'U', ' ', 0x1D55, ' ', 'U', 0x2741, ')'}},
    {3, {0x3C3, 0x3C9, 0x3C3}},
    {3, {0xF2, 0x3C9, 0xF3}},
    {10, {'(', '/', '/', '/', 0x2EC, '/', '/', '/', 0x273F, ')'}},
    {7, {'(', 'U', ' ', 0xF34F, ' ', 'U', ')'}},
    {11, {'(', ' ', 0x361, 'o', ' ', 0x3C9, ' ', 0x361, 'o', ' ', ')'}},
    {3, {0x298, 'w', 0x298}},
    {2, {':', '3'}},
    {2, {':', '3'}},
    {2, {'X', 'D'}},
    {6, {'n', 'y', 'a', 'a', '~', '~'}},
    {3, {'m', 'y', 'a'}},
    {3, {'>', '_', '<'}},
    {4, {0x298, '/', '/', 0x298}},
    {3, {0xf3, 'm', 0xf2}},
    {9, {0x298, '/', '/', 0x298, ' ', 0x298, '/', '/', 0x298}},
    {4, {'r', 'a', 'w', 'r'}},
    {2, {'^', '^'}},
    {4, {'^', '^', ';', ';'}},
    {8, {'(', 0x2C6, ' ', 0xFECC, ' ', 0x2C6, ')'}},
    {5, {'^', 0x2022, 0xFECC, 0x2022, '^'}},
    {8, {'/', '(', '^', 0x2022, 0x3C9, 0x2022, '^', ')'}},
    {6, {'(', 0x273F, 'o', 0x3C9, 'o', ')'}},
};

LOCAL(PyObject *)
emoji_impl(PyObject *text, float strength)
{
    unsigned long long r;
    Py_ssize_t maxchar, uchar, uchar2, i, count, sn, ol, n, new_size;
    int kind, ukind;
    char *res;
    const char *sbuf;
    PyObject *u, *u2;

    if (UNLIKELY(strength <= 0. || isnan(strength))) {
        return Py_NewRef(text);
    }
    else if (strength >= 1.) {
        strength = 1.;
    }

    ol = sn = PyUnicode_GET_LENGTH(text);
    if (UNLIKELY(sn == 0)) {
        return Py_NewRef(text);
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(text);
    kind = PyUnicode_KIND(text);
    sbuf = PyUnicode_DATA(text);

    n = any_countemotiable(sbuf, sn, kind)*12;
    if (UNLIKELY(n == 0)) {
        return Py_NewRef(text);
    }

    uchar = Py_MAX(maxchar, 0xFECC);
    ukind = Py_MAX(kind, PyUnicode_2BYTE_KIND);
    u = PyUnicode_New(sn + n, uchar);
    if (UNLIKELY(!u)) return NULL;
    res = PyUnicode_DATA(u);

    count = 0;
    while (sn > 0) {
        i = any_findemotiable(sbuf, sn, kind);
        if (i < 0) break;
        else if (i) {
            copy_fromLE_toU2p(sbuf, res, i, kind, ukind);
            sbuf += kind * i;
            res += ukind * i;
            sn -= i + 1;
        }
        else {
            sn--;
        }
        r = rand_ull();
        if (rand_to_double(r) < strength) {
            any_setchar(res, 0, ' ', ukind);
            r &= 31;
            const Py_ssize_t elen = emoji_table[r].len;
            copy_fromLE_toU2p(emoji_table[r].data, res + ukind,
                              elen, PyUnicode_2BYTE_KIND, ukind);
            any_setchar(res, elen + 1, ' ', ukind);
            res += ukind * (elen + 2);
            count += elen + (sn > 0);
        }
        else {
            any_setchar(res, 0, any_getchar(sbuf, 0, kind), ukind);
            res += ukind;
        }
        sbuf += kind;
    }

    if (sn > 0) {
        copy_fromLE_toU2p(sbuf, res, sn, kind, ukind);
    }

    if (count != n) {
        if (count == 0) {
            Py_DECREF(u);
            return Py_NewRef(text);
        }
        new_size = ol + count;
        uchar2 = PyUnicode_MAX_CHAR_VALUE(u);
        if (uchar2 <= 0xFF) {
            u2 = PyUnicode_New(new_size, uchar2);
            if (UNLIKELY(u2 == NULL)) {
                Py_DECREF(u);
                return NULL;
            }
            copy_fromU2p_to1B(PyUnicode_DATA(u),
                              PyUnicode_DATA(u2), new_size, ukind);
            Py_DECREF(u);
            u = u2;
        }
        else if (UNLIKELY(PyUnicode_Resize(&u, new_size) < 0)) {
            return NULL;
        }
    }

    return u;
}

static PyObject *
emoji(PyObject *Py_UNUSED(_),
      PyObject *const *args,
      Py_ssize_t nargs,
      PyObject *kwnames)
{
    float strength;
    PyObject *text, *strength_o = NULL;

    Py_ssize_t nkwargs = kwnames ? Py_SIZE(kwnames) : 0;
    Py_ssize_t total_args = nargs + nkwargs;

    if (UNLIKELY(total_args < 1 || total_args > 2)) {
        FMT_ERROR_RET(Type,
                      "'emoji' expected 1 or 2 args, got %zd",
                      total_args);
    }

    if (kwnames) {
        PyObject *key;

        switch (nkwargs) {
        case 2:
            key = PyTuple_GET_ITEM(kwnames, 0);
            if (LIKELY(memcmp(PyUnicode_DATA(key), "text", 5) == 0)) {
                text = args[0];
                key = PyTuple_GET_ITEM(kwnames, 1);
                if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "strength", 9))) {
                    FMT_ERROR_RET(Type,
                                  "'emoji' received unexpected keyword "
                                  "argument '%S'",
                                  key);
                }
                strength_o = args[1];
            }
            else if (memcmp(PyUnicode_DATA(key), "strength", 9) == 0) {
                strength_o = args[0];
                key = PyTuple_GET_ITEM(kwnames, 1);
                if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "text", 5))) {
                    FMT_ERROR_RET(Type,
                                  "'emoji' received unexpected keyword "
                                  "argument '%S'",
                                  key);
                }
                text = args[1];
            }
            else {
                FMT_ERROR_RET(Type,
                              "'emoji' received unexpected keyword "
                              "argument '%S'",
                              key);
            }
            break;
        case 1:
            key = PyTuple_GET_ITEM(kwnames, 0);
            if (LIKELY(memcmp(PyUnicode_DATA(key), "strength", 9) == 0)) {
                if (UNLIKELY(nargs == 0)) {
                    SET_ERROR_RET(Type,
                                  "'emoji' missing required argument "
                                  "'text'");
                }
                strength_o = args[1];
            }
            else if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "text", 5))) {
                FMT_ERROR_RET(Type,
                              "'emoji' received unexpected keyword "
                              "argument '%S'",
                              key);
            }
            text = args[0];
        case 0:
            break;
        default:
            ASSUME(0);
        }
    }
    else {
        if (nargs == 2) {
            strength_o = args[1];
        }
        text = args[0];
    }

    if (UNLIKELY(!PyUnicode_CheckExact(text))) {
        FMT_ERROR_RET(Type,
                      "'emoji' expected 'str' for 'text' argument, "
                      "got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

    if (UNLIKELY(strength_o && !PyFloat_CheckExact(strength_o))) {
        FMT_ERROR_RET(Type,
                      "'emoji' expected 'float' for 'strength' "
                      "argument, got '%.200s'",
                      Py_TYPE(strength_o)->tp_name);
    }

    if (strength_o) {
        strength = PyFloat_AS_DOUBLE(strength_o);
    }
    else {
        return Py_NewRef(text);
    }

    return emoji_impl(text, strength);
}

PyDoc_STRVAR(emoji__doc__,
"emoji(text: str, strength: float = 0.0) -> str\n\
\n\
    Replaces punctuation/newline/tab characters with emoticons.\n");

LOCAL(PyObject *)
tildify_impl(PyObject *text, float strength)
{
    Py_ssize_t maxchar, i, count, sn, ol, n;
    int kind, no_lb = 1;
    char *res;
    const char *sbuf;
    PyObject *u;

    ol = sn = PyUnicode_GET_LENGTH(text);
    if (UNLIKELY(sn == 0)) {
        return Py_NewRef(text);
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(text);
    kind = PyUnicode_KIND(text);
    sbuf = PyUnicode_DATA(text);

    n = any_countspace(sbuf, sn, kind);
    if (UNLIKELY(n == 0)) {
        return Py_NewRef(text);
    }

    u = PyUnicode_New(sn + n, maxchar);
    if (UNLIKELY(!u)) return NULL;
    res = PyUnicode_DATA(u);

    count = 0;
    while (sn > 0) {
        i = any_findspace(sbuf, sn, kind);
        if (i < 0) break;
        else if (i) {
            memcpy(res, sbuf, kind * i);
            sbuf += kind * i;
            res += kind * i;
            sn -= i + 1;
        }
        else {
            sn--;
        }
        if ((no_lb || any_charisword(sbuf, -1, kind)) &&
            rand_double() < strength)
        {
            any_setchar(res, 0, '~', kind);
            res += kind;
            count++;
        }
        any_setchar(res, 0, any_getchar(sbuf, 0, kind), kind);
        sbuf += kind;
        res += kind;
        no_lb = 0;
    }

    if (sn > 0) {
        memcpy(res, sbuf, kind * sn);
    }

    if (count != n) {
        if (UNLIKELY(PyUnicode_Resize(&u, ol + count) < 0)) {
            return NULL;
        }
    }

    return u;
}

static PyObject *
tildify(PyObject *Py_UNUSED(_),
        PyObject *const *args,
        Py_ssize_t nargs,
        PyObject *kwnames)
{
    float strength;
    PyObject *text, *strength_o = NULL;

    Py_ssize_t nkwargs = kwnames ? Py_SIZE(kwnames) : 0;
    Py_ssize_t total_args = nargs + nkwargs;

    if (UNLIKELY(total_args < 1 || total_args > 2)) {
        FMT_ERROR_RET(Type,
                      "'tildify' expected 1 or 2 args, got %zd",
                      total_args);
    }

    if (kwnames) {
        PyObject *key;

        switch (nkwargs) {
        case 2:
            key = PyTuple_GET_ITEM(kwnames, 0);
            if (LIKELY(memcmp(PyUnicode_DATA(key), "text", 5) == 0)) {
                text = args[0];
                key = PyTuple_GET_ITEM(kwnames, 1);
                if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "strength", 9))) {
                    FMT_ERROR_RET(Type,
                                  "'tildify' received unexpected keyword "
                                  "argument '%S'",
                                  key);
                }
                strength_o = args[1];
            }
            else if (memcmp(PyUnicode_DATA(key), "strength", 9) == 0) {
                strength_o = args[0];
                key = PyTuple_GET_ITEM(kwnames, 1);
                if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "text", 5))) {
                    FMT_ERROR_RET(Type,
                                  "'tildify' received unexpected keyword "
                                  "argument '%S'",
                                  key);
                }
                text = args[1];
            }
            else {
                FMT_ERROR_RET(Type,
                              "'tildify' received unexpected keyword "
                              "argument '%S'",
                              key);
            }
            break;
        case 1:
            key = PyTuple_GET_ITEM(kwnames, 0);
            if (LIKELY(memcmp(PyUnicode_DATA(key), "strength", 9) == 0)) {
                if (UNLIKELY(nargs == 0)) {
                    SET_ERROR_RET(Type,
                                  "'tildify' missing required argument "
                                  "'text'");
                }
                strength_o = args[1];
            }
            else if (BUNLIKELY(memcmp(PyUnicode_DATA(key), "text", 5))) {
                FMT_ERROR_RET(Type,
                              "'tildify' received unexpected keyword "
                              "argument '%S'",
                              key);
            }
            text = args[0];
        case 0:
            break;
        default:
            ASSUME(0);
        }
    }
    else {
        if (nargs == 2) {
            strength_o = args[1];
        }
        text = args[0];
    }

    if (UNLIKELY(!PyUnicode_CheckExact(text))) {
        FMT_ERROR_RET(Type,
                      "'tildify' expected 'str' for 'text' argument, "
                      "got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

    if (UNLIKELY(strength_o && !PyFloat_CheckExact(strength_o))) {
        FMT_ERROR_RET(Type,
                      "'tildify' expected 'float' for 'strength' "
                      "argument, got '%.200s'",
                      Py_TYPE(strength_o)->tp_name);
    }

    if (strength_o) {
        strength = PyFloat_AS_DOUBLE(strength_o);
    }
    else {
        return Py_NewRef(text);
    }

    return tildify_impl(text, strength);
}

PyDoc_STRVAR(tildify__doc__,
"tildify(text: str, strength: float = 0.0) -> str\n\
\n\
    Adds some tildes to spaces.\n");

static PyObject *
uwuify(PyObject *Py_UNUSED(_),
       PyObject *const *args,
       Py_ssize_t nargs,
       PyObject *kwnames)
{
    int has_t = 0, has_SS = 0, has_ES = 0, has_TS = 0;
    float SS = 0.2, ES = 0.1, TS = 0.1;
    PyObject *text, *SS_o = NULL, *ES_o = NULL, *TS_o = NULL;

    Py_ssize_t nkwargs = kwnames ? Py_SIZE(kwnames) : 0;
    Py_ssize_t total_args = nargs + nkwargs;

    if (UNLIKELY(total_args < 1 || total_args > 4)) {
        FMT_ERROR_RET(Type,
                      "'uwuify' expected 1 to 4 args, got %zd",
                      total_args);
    }
    else if (UNLIKELY(nargs > 1)) {
        FMT_ERROR_RET(Type,
                      "'uwuify' expected only 1 positional arg, got %zd",
                      nargs);
    }

    if (kwnames) {
        PyObject *key_o;
        const void *key;

        for (Py_ssize_t i = 0; i < nkwargs; i++) {
            key_o = PyTuple_GET_ITEM(kwnames, i);
            key = PyUnicode_DATA(key_o);
            if (memcmp(key, "text", 5) == 0) {
                text = args[nargs + i];
                has_t = 1;
            }
            else if (memcmp(key, "stutter_strength", 17) == 0) {
                SS_o = args[nargs + i];
                has_SS = 1;
            }
            else if (memcmp(key, "emoji_strength", 15) == 0) {
                ES_o = args[nargs + i];
                has_ES = 1;
            }
            else if (memcmp(key, "tilde_strength", 15) == 0) {
                TS_o = args[nargs + i];
                has_TS = 1;
            }
            else {
                FMT_ERROR_RET(Type,
                              "'uwuify' received unexpected keyword "
                              "argument '%S'",
                              key_o);
            }
        }

        if (nargs) {
            if (has_t) {
                SET_ERROR_RET(Type,
                              "'uwuify' received multiple values for "
                              "argument 'text'");
            }
            text = args[0];
        }
        else if (!has_t) {
            SET_ERROR_RET(Type,
                          "'uwuify' missing required argument 'text'");
        }
    }
    else {
        text = args[0];
        goto skip_keyarg_checks;
    }

    if (has_SS) {
        if (!PyFloat_CheckExact(SS_o)) {
        FMT_ERROR_RET(Type,
                      "'tildify' expected 'float' for 'stutter_strength' "
                      "argument, got '%.200s'",
                      Py_TYPE(SS_o)->tp_name);
        }
        SS = PyFloat_AS_DOUBLE(SS_o);
    }

    if (has_ES) {
        if (!PyFloat_CheckExact(ES_o)) {
            FMT_ERROR_RET(Type,
                          "'tildify' expected 'float' for 'emoji_strength' "
                          "argument, got '%.200s'",
                          Py_TYPE(ES_o)->tp_name);
        }
        ES = PyFloat_AS_DOUBLE(ES_o);
    }

    if (has_TS) {
        if (!PyFloat_CheckExact(TS_o)) {
            FMT_ERROR_RET(Type,
                          "'tildify' expected 'float' for 'tilde_strength' "
                          "argument, got '%.200s'",
                          Py_TYPE(TS_o)->tp_name);
        }
        TS = PyFloat_AS_DOUBLE(TS_o);
    }

skip_keyarg_checks:
    if (!PyUnicode_CheckExact(text)) {
        FMT_ERROR_RET(Type,
                      "'tildify' expected 'str' for 'text' argument, "
                      "got '%.200s'",
                      Py_TYPE(text)->tp_name);
    }

#define DO_FUNC(func, ...) \
    tmp = func(__VA_ARGS__); \
    if (UNLIKELY(tmp == NULL)) { \
        Py_DECREF(text); \
        return NULL; \
    } \
    text = tmp;

    PyObject *tmp = case_lower(text);
    if (UNLIKELY(tmp == NULL)) return NULL;
    text = tmp;

    DO_FUNC(word_replace_impl, text)
    DO_FUNC(nyaify_impl, text)
    DO_FUNC(char_replace_impl, text)
    DO_FUNC(stutter_impl, text, SS)
    DO_FUNC(emoji_impl, text, ES)
    DO_FUNC(tildify_impl, text, TS)

    return text;
}

PyDoc_STRVAR(uwuify__doc__,
"uwuify(text: str, *, stutter_strength: float = 0.2,\n\
                      emoji_strength: float = 0.1,\n\
                      tilde_strength: float = 0.1) -> str\n\
\n\
    Takes a string and returns an uwuified version of it.\n");

#define METH_O_DOC(name) \
    { #name, (PyCFunction)name, METH_O, name##__doc__ },

#define METH_FAST_KEYWORDS_DOC(name) \
    { #name, (PyCFunction)name, METH_FASTCALL | METH_KEYWORDS, \
      name##__doc__ },

static PyMethodDef uwuifier_methods[] = {
    METH_O_DOC(word_replace)
    METH_O_DOC(nyaify)
    METH_O_DOC(char_replace)

    METH_FAST_KEYWORDS_DOC(stutter)
    METH_FAST_KEYWORDS_DOC(emoji)
    METH_FAST_KEYWORDS_DOC(tildify)
    METH_FAST_KEYWORDS_DOC(uwuify)

    {NULL}
};

static struct PyModuleDef uwuifiermodule = {
    PyModuleDef_HEAD_INIT,
    "uwuifier",
    ":3",
    0,
    uwuifier_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_uwuifier(void)
{
    srand((unsigned)time(NULL));
    return PyModule_Create(&uwuifiermodule);
}
