#define PY_SSIZE_T_CLEAN
#define Py_BUILD_CORE
#include "Python.h"

#include "internal/pycore_dict.h"
#include "internal/pycore_gc.h"
#include "internal/pycore_object.h"
#include "internal/pycore_pystate.h"
#include "internal/pycore_unicodeobject.h"
#undef Py_BUILD_CORE

#include <omp.h>

#if defined(likely) && defined(unlikely)
#  undef likely
#  undef unlikely
#endif /* likely && unlikely */
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define IDX_FROM_ORDER_v(vls, i) ((char *)(vls))[-3-i]

static inline void Py_INC_REFCNT(PyObject *ob, Py_ssize_t refcnt) {
    ob->ob_refcnt += refcnt;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_INC_REFCNT(ob, refcnt) Py_INC_REFCNT(_PyObject_CAST(ob), (refcnt))
#endif

static PyObject *pyzero;

#if 0
static inline PyDictObject *
new_dict(PyDictKeysObject *keys, PyDictValues *values, Py_ssize_t used)
{
    PyDictObject *mp;
#if PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = &_PyThreadState_GET()->interp->dict_state;
#ifdef Py_DEBUG
    assert(state->numfree != -1);
#endif
    if (likely(state->numfree != 0)) {
        mp = state->free_list[--state->numfree];
        _Py_NewReference((PyObject *)mp);
    }
    else
#endif
    {
        mp = PyObject_GC_New(PyDictObject, &PyDict_Type);
    }
    mp->ma_keys = keys;
    mp->ma_values = values;
    mp->ma_used = used;
    mp->ma_version_tag = DICT_NEXT_VERSION();
    return mp;
}

static PyObject *
c_map_except(PyObject *self,
             PyObject *const *args,
             Py_ssize_t nargs)
{
    PyObject *arg;
    PyDictObject *mp = (PyDictObject *)args[0];
    Py_ssize_t dsize;
    if (unlikely((dsize = mp->ma_used) == 0)) {
        return PyDict_New(); /* TODO: inline */
    }
    PySetObject *so;
    PyTypeObject *type;
    if (likely((type = Py_TYPE(arg = args[1])) == &PySet_Type)) {
        PyDictValues *dv;
        PyDictKeysObject *dk;
        so = (PySetObject *)arg;
        if (unlikely(so->used == 0)) {
            PyDictObject *copy;
            Py_ssize_t i, n;
            if (likely((dv = mp->ma_values) != NULL)) {
                Py_ssize_t size = (dk = mp->ma_keys)->dk_nentries + dk->dk_usable;
                size_t prefix_size = size + 2;
                prefix_size = (prefix_size + (size_t)(sizeof(PyObject *) - 1)) & ~(size_t)(sizeof(PyObject *) - 1);
                uint8_t *mem = PyMem_Malloc(refix_size + (size_t)size * (size_t)sizeof(PyObject *));
                mem[prefix_size - 1] = (uint8_t)prefix_size;
                PyDictValues *newvalues = (PyDictValues *)(mem + prefix_size);
                copy = PyObject_GC_New(PyDictObject, &PyDict_Type);
                memcpy(mem, ((uint8_t *)dv) - prefix_size, prefix_size - 1);
                copy->ma_values = newvalues;
                copy->ma_keys = dk;
                copy->ma_used = dsize;
                copy->ma_version_tag = DICT_NEXT_VERSION();
#ifdef Py_REF_DEBUG
                _Py_RefTotal++;
#endif
                dk->dk_refcnt++;
                PyObject **dv_values = dv->values, **copy_values = newvalues->values;
                for (i = 0, n = size; unlikely(i < n); i++) {
                    PyObject *value = dv_values[i];
                    Py_XINCREF(value);
                    copy_values[i] = value;
                }
                if (likely(_PyObject_GC_IS_TRACKED(mp))) {
                    PyGC_Head *gc = _Py_AS_GC((PyObject *)copy);
                    PyGC_Head *generation0 = _PyThreadState_GET()->interp->gc.generation0;
                    PyGC_Head *last = (PyGC_Head *)(generation0->_gc_prev);
                    _PyGCHead_SET_NEXT(last, gc);
                    _PyGCHead_SET_PREV(gc, last);
                    _PyGCHead_SET_NEXT(gc, generation0);
                    generation0->_gc_prev = (uintptr_t)gc;
                }
                return (PyObject *)copy;
            }
            if (likely(dsize >= ((n = (dk = mp->ma_keys)->dk_nentries) * 2) / 3)) {
                int is_not_unicode;
                Py_ssize_t keys_size = (sizeof(PyDictKeysObject)
                                        + ((size_t)1 << dk->dk_log2_index_bytes)
                                        + (((int64_t)1 << dk->dk_log2_size) << 1)/3 * likely(is_not_unicode = dk->dk_kind == DICT_KEYS_GENERAL) ? sizeof(PyDictKeyEntry) : sizeof(PyDictUnicodeEntry));
                PyDictKeysObject *keys = PyObject_Malloc(keys_size);
                memcpy(keys, dk, keys_size);
                PyObject **pkey, **pvalue;
                size_t offs;
                if (likely(is_not_unicode)) {
                    PyDictKeyEntry *ep0 = ((PyDictKeyEntry*)(&((int8_t*)(dk->dk_indices))[(size_t)1 << dk->dk_log2_index_bytes]));
                    pkey = &ep0->me_key;
                    pvalue = &ep0->me_value;
                    offs = sizeof(PyDictKeyEntry) / sizeof(PyObject *);
                }
                else {
                    PyDictUnicodeEntry *ep0 = ((PyDictUnicodeEntry*)(&((int8_t*)(dk->dk_indices))[(size_t)1 << dk->dk_log2_index_bytes]));
                    pkey = &ep0->me_key;
                    pvalue = &ep0->me_value;
                    offs = sizeof(PyDictUnicodeEntry) / sizeof(PyObject *);
                }
                for (i = 0; unlikely(i < n); i++) {
                    PyObject *value = *pvalue;
                    if (unlikely(value != NULL)) {
                        Py_INCREF(value);
                        Py_INCREF(*pkey);
                    }
                    pvalue += offs;
                    pkey += offs;
                }
#ifdef Py_REF_DEBUG
                _Py_RefTotal++;
#endif
                PyDictObject *new = (PyDictObject *)new_dict(keys, NULL, dsize);
                if (likely(_PyObject_GC_IS_TRACKED(mp))) {
                    PyGC_Head *gc = _Py_AS_GC((PyObject *)new);
                    PyGC_Head *generation0 = _PyThreadState_GET()->interp->gc.generation0;
                    PyGC_Head *last = (PyGC_Head *)(generation0->_gc_prev);
                    _PyGCHead_SET_NEXT(last, gc);
                    _PyGCHead_SET_PREV(gc, last);
                    _PyGCHead_SET_NEXT(gc, generation0);
                    generation0->_gc_prev = (uintptr_t)gc;
                }
                return (PyObject *)new;
            }
            copy = PyDict_New();
            Py_ssize_t log2size;
            if (likely(dsize == n && ((log2size = dk->dk_log2_size) == 3 ||
                                      (((int64_t)1 << log2size) >> 1 << 1) / 3 < dsize)))
            {
                int is_not_unicode;
                Py_ssize_t keys_size = (sizeof(PyDictKeysObject)
                                        + ((size_t)1 << dk->dk_log2_index_bytes)
                                        + (((int64_t)1 << log2size) << 1)/3 * likely(is_not_unicode = dk->dk_kind == DICT_KEYS_GENERAL) ? sizeof(PyDictKeyEntry) : sizeof(PyDictUnicodeEntry));
                PyDictKeysObject *keys = PyObject_Malloc(keys_size);
                memcpy(keys, dk, keys_size);
                PyObject **pkey, **pvalue;
                size_t offs;
                if (likely(is_not_unicode)) {
                    PyDictKeyEntry *ep0 = ((PyDictKeyEntry*)(&((int8_t*)(dk->dk_indices))[(size_t)1 << dk->dk_log2_index_bytes]));
                    pkey = &ep0->me_key;
                    pvalue = &ep0->me_value;
                    offs = sizeof(PyDictKeyEntry) / sizeof(PyObject *);
                }
                else {
                    PyDictUnicodeEntry *ep0 = ((PyDictUnicodeEntry*)(&((int8_t*)(dk->dk_indices))[(size_t)1 << dk->dk_log2_index_bytes]));
                    pkey = &ep0->me_key;
                    pvalue = &ep0->me_value;
                    offs = sizeof(PyDictUnicodeEntry) / sizeof(PyObject *);
                }
                for (i = 0; unlikely(i < n); i++) {
                    PyObject *value = *pvalue;
                    if (unlikely(value != NULL)) {
                        Py_INCREF(value);
                        Py_INCREF(*pkey);
                    }
                    pvalue += offs;
                    pkey += offs;
                }
                PyDictKeysObject *copy_keys = copy->ma_keys;
                if (likely(is_not_unicode)) {
                    PyDictKeyEntry *entries = (PyDictKeyEntry*)(&((int8_t*)(copy_keys->dk_indices))[(size_t)1 << copy_keys->dk_log2_index_bytes]);
                    for (i = 0, n = copy_keys->dk_nentries; unlikely(i < n); i++) {
                        Py_XDECREF(entries[i].me_key);
                        Py_XDECREF(entries[i].me_value);
                    }
                }
                else {
                    PyDictUnicodeEntry *entries = (PyDictUnicodeEntry*)(&((int8_t*)(copy_keys->dk_indices))[(size_t)1 << copy_keys->dk_log2_index_bytes]);
                    Py_ssize_t i, n;
                    for (i = 0, n = copy_keys->dk_nentries; unlikely(i < n); i++) {
                        Py_XDECREF(entries[i].me_key);
                        Py_XDECREF(entries[i].me_value);
                    }
                }
            #if PyDict_MAXFREELIST > 0
                struct _Py_dict_state *state = get_dict_state();
            #ifdef Py_DEBUG
                // free_keys_object() must not be called after _PyDict_Fini()
                assert(state->keys_numfree != -1);
            #endif
                if (DK_LOG_SIZE(keys) == PyDict_LOG_MINSIZE
                        && state->keys_numfree < PyDict_MAXFREELIST
                        && DK_IS_UNICODE(keys)) {
                    state->keys_free_list[state->keys_numfree++] = keys;
                    OBJECT_STAT_INC(to_freelist);
                    return;
                }
            #endif
                PyObject_Free(keys);
            }
        }
        setentry *entry, *table;
        size_t perturb, mask, si, di = 0;
        int probes, cmp;
        Py_hash_t hash;
        PyObject *dkey;
        int8_t dk_indices = (int8_t *)(dk = mp->ma_keys)->dk_indices;
        if (likely((dv = mp->ma_values) != NULL)) {
            while (1) {
                hash = ((PyASCIIObject *)(dkey = ((PyDictUnicodeEntry *)(&dk_indices[(size_t)1 << dk->dk_log2_index_bytes]))[IDX_FROM_ORDER_v(dv, di)].me_key))->hash;
              redo_1:
                si = (perturb = (size_t)hash) & (mask = so->mask);
                while (1) {
                    entry = &so->table[si];
                    probes = likely(si + 9 <= mask) ? 9 : 0;
                    do {
                        if (likely(entry->hash == 0 && entry->key == NULL)) {
                            goto pass_1;
                        }
                        if (unlikely(entry->hash == hash)) {
                            PyObject *startkey = entry->key;
                            if (unlikely(startkey == dkey)) {
                                goto found_1;
                            }
                            if (likely(PyUnicode_CheckExact(startkey)
                                       && PyUnicode_CheckExact(dkey)
                                       && _PyUnicode_EQ(startkey, dkey)))
                            {
                                goto found_1;
                            }
                            table = so->table;
                            Py_INCREF(startkey);
                            cmp = PyObject_RichCompareBool(startkey, dkey, Py_EQ);
                            Py_DECREF(startkey);
                            if (table != so->table || entry->key != startkey) {
                                goto redo_1;
                            }
                            if (likely(cmp > 0)) {
                                goto found_1;
                            }
                            mask = so->mask;
                        }
                        entry++;
                    } while (probes--);
                    si = (si * 5 + (perturb >>= 5) + 1) & mask;
                }
              found_1:
                // TODO
            }
        }
        else {
        }    
}
#endif /* 0 */

#define MAX_CANDIDATE_ITEMS 750
#define MAX_STRING_SIZE 40

#define MOVE_COST 2
#define CASE_COST 1

#define LEAST_FIVE_BITS(n) ((n) & 31)

static inline int
substitution_cost(char a, char b)
{
    if (LEAST_FIVE_BITS(a) != LEAST_FIVE_BITS(b)) {
        // Not the same, not a case flip.
        return MOVE_COST;
    }
    if (a == b) {
        return 0;
    }
    if (unlikely('A' <= a && a <= 'Z')) {
        a += ('a' - 'A');
    }
    if (unlikely('A' <= b && b <= 'Z')) {
        b += ('a' - 'A');
    }
    if (a == b) {
        return CASE_COST;
    }
    return MOVE_COST;
}

/* Calculate the Levenshtein distance between string1 and string2 */
static Py_ssize_t
levenshtein_distance(const char *a, size_t a_size,
                     const char *b, size_t b_size,
                     size_t max_cost)
{
    static size_t buffer[MAX_STRING_SIZE];

    // Both strings are the same (by identity)
    if (a == b) {
        return 0;
    }

    // Trim away common affixes.
    while (a_size && b_size && a[0] == b[0]) {
        a++; a_size--;
        b++; b_size--;
    }
    while (a_size && b_size && a[a_size-1] == b[b_size-1]) {
        a_size--;
        b_size--;
    }
    if (unlikely(a_size == 0 || b_size == 0)) {
        return (a_size + b_size) * MOVE_COST;
    }
    if (unlikely(a_size > MAX_STRING_SIZE || b_size > MAX_STRING_SIZE)) {
        return max_cost + 1;
    }

    // Prefer shorter buffer
    if (b_size < a_size) {
        const char *t = a; a = b; b = t;
        size_t t_size = a_size; a_size = b_size; b_size = t_size;
    }

    // quick fail when a match is impossible.
    if (unlikely((b_size - a_size) * MOVE_COST > max_cost)) {
        return max_cost + 1;
    }

    // Instead of producing the whole traditional len(a)-by-len(b)
    // matrix, we can update just one row in place.
    // Initialize the buffer row
    size_t tmp = MOVE_COST;
    for (size_t i = 0; likely(i < a_size); i++) {
        // cost from b[:0] to a[:i+1]
        buffer[i] = tmp;
        tmp += MOVE_COST;
    }

    size_t result = 0;
    for (size_t b_index = 0; likely(b_index < b_size); b_index++) {
        char code = b[b_index];
        // cost(b[:b_index], a[:0]) == b_index * MOVE_COST
        size_t distance = result = b_index * MOVE_COST;
        size_t minimum = SIZE_MAX;
        for (size_t index = 0; likely(index < a_size); index++) {

            // cost(b[:b_index+1], a[:index+1]) = min(
            //     // 1) substitute
            //     cost(b[:b_index], a[:index])
            //         + substitution_cost(b[b_index], a[index]),
            //     // 2) delete from b
            //     cost(b[:b_index], a[:index+1]) + MOVE_COST,
            //     // 3) delete from a
            //     cost(b[:b_index+1], a[index]) + MOVE_COST
            // )

            // 1) Previous distance in this row is cost(b[:b_index], a[:index])
            size_t substitute = distance + substitution_cost(code, a[index]);
            // 2) cost(b[:b_index], a[:index+1]) from previous row
            distance = buffer[index];
            // 3) existing result is cost(b[:b_index+1], a[index])

            size_t insert_delete = Py_MIN(result, distance) + MOVE_COST;
            result = Py_MIN(insert_delete, substitute);

            // cost(b[:b_index+1], a[:index+1])
            buffer[index] = result;
            if (result < minimum) {
                minimum = result;
            }
        }
        if (unlikely(minimum > max_cost)) {
            // Everything in this row is too big, so bail early.
            return max_cost + 1;
        }
    }
    return result;
}

typedef struct {
    Py_ssize_t size;
    const char *name;
} suggestiontype;

typedef struct {
    Py_ssize_t size;
    suggestiontype suggestions[2];
} suggestionlist;

static inline const char *
calculate_suggestions(suggestionlist dir,
                      suggestiontype name)
{
    Py_ssize_t dir_size = dir.size;
    if (unlikely(dir_size >= MAX_CANDIDATE_ITEMS)) {
        return NULL;
    }

    suggestiontype *dir_items = dir.suggestions;

    Py_ssize_t suggestion_distance = PY_SSIZE_T_MAX;
    const char *suggested = NULL;
    const char *name_str = name.name;
    Py_ssize_t name_size = name.size;

    for (int i = 0; i < dir_size; ++i) {
        suggestiontype item = dir_items[i];
        Py_ssize_t item_size = item.size;
        const char *item_str = item.name;
        // No more than 1/2 of the involved characters should need changed.
        Py_ssize_t max_distance = (name_size + item_size + 2) * MOVE_COST / 4;
        // Don't take matches we've already beaten.
        max_distance = Py_MIN(max_distance, suggestion_distance - 1);
        Py_ssize_t current_distance =
            levenshtein_distance(name_str, name_size,
                                 item_str, item_size, max_distance);
        if (current_distance > max_distance) {
            continue;
        }
        if (!suggested || current_distance < suggestion_distance) {
            suggested = item_str;
            suggestion_distance = current_distance;
        }
    }
    return suggested;
}

#define _PyUnicode_CONVERT_BYTES(from_type, to_type, begin, end, to) \
    do {                                                \
        to_type *_to = (to_type *)(to);                 \
        const from_type *_iter = (const from_type *)(begin);\
        const from_type *_end = (const from_type *)(end);\
        Py_ssize_t n = (_end) - (_iter);                \
/*        const from_type *_unrolled_end =              \
            _iter + _Py_SIZE_ROUND_DOWN(n, 4);          \
        while (_iter < (_unrolled_end)) {               \
            _to[0] = (to_type) _iter[0];                \
            _to[1] = (to_type) _iter[1];                \
            _to[2] = (to_type) _iter[2];                \
            _to[3] = (to_type) _iter[3];                \
            _iter += 4; _to += 4;                       \
        }                                               \ */ \
        while (_iter < (_end))                          \
            *_to++ = (to_type) *_iter++;                \
    } while (0)

#define LATIN1(ch)  \
    (ch < 128 \
     ? (PyObject*)&_Py_SINGLETON(strings).ascii[ch] \
     : (PyObject*)&_Py_SINGLETON(strings).latin1[ch - 128])

static inline PyObject* unicode_get_empty(void)
{
    _Py_DECLARE_STR(empty, "");
    return &_Py_STR(empty);
}

static inline PyObject* unicode_new_empty(void)
{
    PyObject *empty = unicode_get_empty();
    Py_INCREF(empty);
    return empty;
}

#define _Py_RETURN_UNICODE_EMPTY()   \
    do {                             \
        return unicode_new_empty();  \
    } while (0)

static inline PyObject*
get_latin1_char(Py_UCS1 ch)
{
    return Py_NewRef(LATIN1(ch));
}

static PyObject*
unicode_char(Py_UCS4 ch)
{
    PyObject *unicode;

    // static_assert(ch <= 0x10ffff, "ch > 0x10ffff");

    if (likely(ch < 256)) {
        return get_latin1_char(ch);
    }

    unicode = PyUnicode_New(1, ch);
    if (unlikely(unicode == NULL))
        return NULL;

    // static_assert(PyUnicode_KIND(unicode) != PyUnicode_1BYTE_KIND, "bad kind");
    if (PyUnicode_KIND(unicode) == PyUnicode_2BYTE_KIND) {
        PyUnicode_2BYTE_DATA(unicode)[0] = (Py_UCS2)ch;
    } else {
        // static_assert(PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND, "bad kind");
        PyUnicode_4BYTE_DATA(unicode)[0] = ch;
    }
    // static_assert(_PyUnicode_CheckConsistency(unicode, 1), "consistency check failed");
    return unicode;
}

static PyObject *
_PyUnicode_FromUCS1(const Py_UCS1 *s, Py_ssize_t size);
static PyObject *
_PyUnicode_FromUCS2(const Py_UCS2 *s, Py_ssize_t size);
static PyObject *
_PyUnicode_FromUCS4(const Py_UCS4 *s, Py_ssize_t size);

static const unsigned char ascii_linebreak[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
/*         0x000A, * LINE FEED */
/*         0x000B, * LINE TABULATION */
/*         0x000C, * FORM FEED */
/*         0x000D, * CARRIAGE RETURN */
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
/*         0x001C, * FILE SEPARATOR */
/*         0x001D, * GROUP SEPARATOR */
/*         0x001E, * RECORD SEPARATOR */
    0, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

#if LONG_BIT >= 128
#define BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define BLOOM_MASK unsigned long

static BLOOM_MASK bloom_linebreak = ~(BLOOM_MASK)0;

#define BLOOM(mask, ch)     ((mask &  (1UL << ((ch) & (BLOOM_WIDTH - 1)))))

#define BLOOM_LINEBREAK(ch)                                             \
    ((ch) < 128U ? ascii_linebreak[(ch)] :                              \
     (BLOOM(bloom_linebreak, (ch)) && Py_UNICODE_ISLINEBREAK(ch)))

static inline BLOOM_MASK
make_bloom_mask(int kind, const void* ptr, Py_ssize_t len)
{
#define BLOOM_UPDATE(TYPE, MASK, PTR, LEN)             \
    do {                                               \
        TYPE *data = (TYPE *)PTR;                      \
        TYPE *end = data + LEN;                        \
        Py_UCS4 ch;                                    \
        for (; data != end; data++) {                  \
            ch = *data;                                \
            MASK |= (1UL << (ch & (BLOOM_WIDTH - 1))); \
        }                                              \
        break;                                         \
    } while (0)

    /* calculate simple bloom-style bitmask for a given unicode string */

    BLOOM_MASK mask;

    mask = 0;
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        BLOOM_UPDATE(Py_UCS1, mask, ptr, len);
        break;
    case PyUnicode_2BYTE_KIND:
        BLOOM_UPDATE(Py_UCS2, mask, ptr, len);
        break;
    case PyUnicode_4BYTE_KIND:
        BLOOM_UPDATE(Py_UCS4, mask, ptr, len);
        break;
    default:
        Py_UNREACHABLE();
    }
    return mask;

#undef BLOOM_UPDATE
}

#define STRINGLIB_GET_EMPTY() unicode_get_empty()

#include "stringlib/asciilib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs1lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/replace.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/replace.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/replace.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#undef STRINGLIB_GET_EMPTY

PyObject*
_PyUnicode_FromASCII(const char *buffer, Py_ssize_t size)
{
    const unsigned char *s = (const unsigned char *)buffer;
    PyObject *unicode;
    if (size == 1) {
#ifdef Py_DEBUG
        static_assert((unsigned char)s[0] < 128, "(unsigned char)s[0] >= 128");
#endif
        return get_latin1_char(s[0]);
    }
    unicode = PyUnicode_New(size, 127);
    if (unlikely(!unicode))
        return NULL;
    memcpy(PyUnicode_1BYTE_DATA(unicode), s, size);
    // static_assert(_PyUnicode_CheckConsistency(unicode, 1), "consistency check failed");
    return unicode;
}

static PyObject*
_PyUnicode_FromUCS1(const Py_UCS1* u, Py_ssize_t size)
{
    PyObject *res;
    unsigned char max_char;

    if (size == 0) {
        _Py_RETURN_UNICODE_EMPTY();
    }
    // static_assert(size > 0, "size check failed");
    if (size == 1) {
        return get_latin1_char(u[0]);
    }

    max_char = ucs1lib_find_max_char(u, u + size);
    res = PyUnicode_New(size, max_char);
    if (unlikely(!res))
        return NULL;
    memcpy(PyUnicode_1BYTE_DATA(res), u, size);
    // static_assert(_PyUnicode_CheckConsistency(res, 1), "consistency check failed");
    return res;
}

static PyObject*
_PyUnicode_FromUCS2(const Py_UCS2 *u, Py_ssize_t size)
{
    PyObject *res;
    Py_UCS2 max_char;

    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();
    // static_assert(size > 0, "size check failed");
    if (size == 1)
        return unicode_char(u[0]);

    max_char = ucs2lib_find_max_char(u, u + size);
    res = PyUnicode_New(size, max_char);
    if (unlikely(!res))
        return NULL;
    if (likely(max_char >= 256))
        memcpy(PyUnicode_2BYTE_DATA(res), u, sizeof(Py_UCS2)*size);
    else {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS2, Py_UCS1, u, u + size, PyUnicode_1BYTE_DATA(res));
    }
    // static_assert(_PyUnicode_CheckConsistency(res, 1), "consistency check failed");
    return res;
}

static PyObject*
_PyUnicode_FromUCS4(const Py_UCS4 *u, Py_ssize_t size)
{
    PyObject *res;
    Py_UCS4 max_char;

    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();
    // static_assert(size > 0, "size check failed");
    if (size == 1)
        return unicode_char(u[0]);

    max_char = ucs4lib_find_max_char(u, u + size);
    res = PyUnicode_New(size, max_char);
    if (unlikely(!res))
        return NULL;
    if (unlikely(max_char < 256))
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS1, u, u + size,
                                 PyUnicode_1BYTE_DATA(res));
    else if (max_char < 0x10000)
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS2, u, u + size,
                                 PyUnicode_2BYTE_DATA(res));
    else
        memcpy(PyUnicode_4BYTE_DATA(res), u, sizeof(Py_UCS4)*size);
    // static_assert(_PyUnicode_CheckConsistency(res, 1), "consistency check failed");
    return res;
}

static suggestionlist scharindices_kwlist = {
    1,
    {
        {4, "char"},
    }
};

static PyObject *
c_scharindices(PyObject *self,
              PyObject *const *args,
              Py_ssize_t nargs,
              PyObject *kwargs)
{
    PyObject *kwarg, *_string, *_character;
    PyObject *result, **ob_item;
    const void *string;
    Py_UCS4 character;
    Py_ssize_t length, lenres, lenchar, lenkwarg;
    Py_ssize_t convresult, total, nkwargs = 0, idx = 0;
    int stringkind;

    total = nargs + (nkwargs = kwargs == NULL ? 0 : PyTuple_GET_SIZE(kwargs));
    if (likely(nargs > 0 && total < 3)) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method c.scharindices() missing "
                        "1 required positional argument: 'self'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.scharindices() got "
                     "more arguments than accepted (%zd > 2)",
                     total);
    }
    return NULL;

  valid_args:
    if (unlikely(nkwargs != 0)) {
        kwarg = PyTuple_GET_ITEM(kwargs, 0);
        lenkwarg = PyUnicode_GET_LENGTH(kwarg);
        if (unlikely(lenkwarg != 4 || memcmp(PyUnicode_DATA(kwarg), "char", 4) != 0)) {
            const char *suggestion = calculate_suggestions(scharindices_kwlist,
                                                           (suggestiontype){lenkwarg, (const char *)PyUnicode_DATA(kwarg)});
            if (suggestion != NULL) {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.scharindices() got "
                             "an unexpected keyword argument '%U'. Did "
                             "you mean: '%s'?",
                             kwarg, suggestion);
            }
            else {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.scharindices() got "
                             "an unexpected keyword argument '%U'",
                             kwarg);
            }
            return NULL;
        }
    }
    _string = args[0];
    if (unlikely(_string == NULL)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (unlikely(!PyUnicode_Check(_string))) {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.scharindices() needs a "
                     "'str' object for argument 'self', got '%.200s' object",
                     Py_TYPE(_string)->tp_name);
        return NULL;
    }
    string = PyUnicode_DATA(_string);
    length = PyUnicode_GET_LENGTH(_string);
    stringkind = PyUnicode_KIND(_string);
    if (unlikely(total == 1)) {
        character = '\n';
        goto got_character;
    }
    _character = args[1];
    if (unlikely(_character == NULL)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (likely(PyUnicode_Check(_character))) {
        if (unlikely((lenchar = PyUnicode_GET_LENGTH(_character)) != 1)) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.scharindices() expected "
                         "a character for argument 'char', but string "
                         "of length %zd found",
                         lenchar);
            return NULL;
        }
        character = PyUnicode_4BYTE_DATA(_character)[0];
    }
    else if (likely(PyLong_Check(_character))) {
        convresult = PyLong_AsSsize_t(_character);
        if (unlikely(convresult == -1 && PyErr_Occurred())) {
            return NULL;
        }
        if (unlikely(convresult < 0 || convresult > (Py_ssize_t)0x10ffff)) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.scharindices() argument "
                         "'char' out of bounds: %zd not in range(0x110000)",
                         convresult);
            return NULL;
        }
        character = Py_SAFE_DOWNCAST(convresult, Py_ssize_t, Py_UCS4);
    }
    else if (unlikely(_character == Py_None)) {
        character = '\n';
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.scharindices() needs any "
                     "of 'str', 'int', None, or nothing passed for "
                     "argument 'char', got '%.200s'",
                     Py_TYPE(_character)->tp_name);
        return NULL;
    }
  got_character:
#define ALGO(T) \
    if (unlikely(result == NULL)) { \
        return NULL; \
    } \
    if (PyTuple_GET_SIZE(result) == 0) { \
        return result; \
    } \
    PyObject *tmp; \
    ob_item = ((PyTupleObject *)result)->ob_item; \
    const T *p = s, *e = s + length; \
    if (likely(length > 40)) { \
        const T *s1, *e1; \
        unsigned char needle = ch & 0xff; \
  redo_algo ## T: \
        if (needle != 0) { \
            do { \
                void *candidate = memchr(p, needle, (e - p) * sizeof(T)); \
                if (candidate == NULL) { \
                    return result; \
                } \
                s1 = p; \
                p = (const T *)_Py_ALIGN_DOWN(candidate, sizeof(T)); \
                if (*p == ch) { \
                    ob_item[idx++] = tmp = PyLong_FromSize_t((size_t)(p - s)); \
                    if (tmp == NULL) { \
                        Py_DECREF(result); \
                        return NULL; \
                    } \
                    ++p; \
                    goto redo_algo ## T; \
                } \
                ++p; \
                if (p - s1 > 40) { \
                    continue; \
                } \
                if (e - p <= 40) { \
                    break; \
                } \
                e1 = p + 40; \
                while (unlikely(p != e1)) { \
                    if (unlikely(*p == ch)) { \
                        ob_item[idx++] = tmp = PyLong_FromSize_t((size_t)(p - s)); \
                        if (tmp == NULL) { \
                            Py_DECREF(result); \
                            return NULL; \
                        } \
                    } \
                    ++p; \
                } \
            } while (e - p > 40); \
        } \
    } \
    while (likely(p < e)) { \
        if (*p == ch) { \
            ob_item[idx++] = tmp = PyLong_FromSize_t((size_t)(p - s)); \
            if (tmp == NULL) { \
                Py_DECREF(result); \
                return NULL; \
            } \
        } \
        ++p; \
    }
    switch (stringkind) {
        case PyUnicode_1BYTE_KIND: {
            const Py_UCS1 ch = Py_SAFE_DOWNCAST(character, Py_UCS4, Py_UCS1), *s;
            result = PyTuple_New(lenres = ucs1lib_count_char((s = (const Py_UCS1 *)string), length, ch, 0));
            if (unlikely(result == NULL)) {
                return NULL;
            }
            PyObject *tmp;
            ob_item = ((PyTupleObject *)result)->ob_item;
            const Py_UCS1 *p = s, *e = s + length;
            if (likely(length > 15)) {
                while (likely((p = memchr(p, ch, length)) != NULL)) {
                    ob_item[idx++] = tmp = PyLong_FromSize_t((size_t)(p - s));
                    if (unlikely(tmp == NULL)) {
                        Py_DECREF(result);
                        return NULL;
                    }
                    ++p;
                }
                return result;
            }
            while (likely(p < e)) {
                if (*p == ch) {
                    ob_item[idx++] = tmp = PyLong_FromSize_t((size_t)(p - s));
                    if (unlikely(tmp == NULL)) {
                        Py_DECREF(result);
                        return NULL;
                    }
                }
                ++p;
            }
            break;
        }
        case PyUnicode_2BYTE_KIND: {
            const Py_UCS2 ch = Py_SAFE_DOWNCAST(character, Py_UCS4, Py_UCS2), *s;
            result = PyTuple_New(lenres = ucs2lib_count_char((s = (const Py_UCS2 *)string), length, ch, 0));
            ALGO(Py_UCS2)
            break;
        }
        default: {
            // static_assert(stringkind == PyUnicode_4BYTE_KIND, "invalid unicode string kind");
            const Py_UCS4 ch = character, *s;
            result = PyTuple_New(lenres = ucs4lib_count_char((s = (const Py_UCS4 *)string), length, ch, 0));
            ALGO(Py_UCS4)
            break;
        }
    }
    Py_SET_SIZE(result, lenres);
    return result;
#undef ALGO
}

PyDoc_STRVAR(c_scharindices__doc__, "\
c.scharindices(self, /, char='\\n')\n\
    Return a tuple of indices where char is located. Returns an empty \n\
    tuple if char is not found.\n\
\n\
      char\n\
        The character to search for. Can either be a one-character \n\
        string, a Unicode ordinal, or None (None becomes '\\n')\n\
");

static suggestionlist slineoffsets_kwlist = {
    1,
    {
        {9, "keepstart"},
    }
};

static PyObject *
c_slineoffsets(PyObject *self,
              PyObject *const *args,
              Py_ssize_t nargs,
              PyObject *kwargs)
{
    PyObject *kwarg, *_string, *_keepstart, *list;
    const void *string;
    Py_ssize_t nkwargs, total, lenkwarg, length, idx = 0;
    int stringkind, keepstart = 0;

    total = nargs + (nkwargs = kwargs == NULL ? 0 : PyTuple_GET_SIZE(kwargs));
    if (likely(nargs > 0 && total < 3)) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method c.slineoffsets() missing "
                        "1 required positional argument: 'self'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.slineoffsets() got "
                     "more arguments than accepted (%zd > 2)",
                     nargs);
    }
    return NULL;

  valid_args:
    if (unlikely(nkwargs != 0)) {
        kwarg = PyTuple_GET_ITEM(kwargs, 0);
        lenkwarg = PyUnicode_GET_LENGTH(kwarg);
        if (unlikely(lenkwarg != 9 || memcmp(PyUnicode_DATA(kwarg), "keepstart", 9) != 0)) {
            const char *suggestion = calculate_suggestions(slineoffsets_kwlist,
                                                           (suggestiontype){lenkwarg, (const char *)PyUnicode_DATA(kwarg)});
            if (suggestion != NULL) {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.slineoffsets() got "
                             "an unexpected keyword argument '%U'. Did "
                             "you mean: '%s'?",
                             kwarg, suggestion);
            }
            else {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.slineoffsets() got "
                             "an unexpected keyword argument '%U'",
                             kwarg);
            }
            return NULL;
        }
    }
    _string = args[0];
    if (unlikely(_string == NULL)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (unlikely(!PyUnicode_Check(_string))) {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.slineoffsets() needs a "
                     "'str' object for argument 'self', got '%.200s' object",
                     Py_TYPE(_string)->tp_name);
        return NULL;
    }
    string = PyUnicode_DATA(_string);
    length = PyUnicode_GET_LENGTH(_string);
    if (total == 1) {
        keepstart = 0;
    }
    else {
        _keepstart = args[1];
        if (unlikely(_keepstart == NULL)) {
            PyErr_BadArgument();
            return NULL;
        }
        keepstart = PyObject_IsTrue(_keepstart);
        if (unlikely(keepstart < 0)) {
            return NULL;
        }
    }
#define ALGO(T, LINEBREAK_CHECK) \
    Py_ssize_t i, j; \
    int checked_for = 0; \
    PyObject *sub; \
 \
    list = PyList_New(keepstart); \
    if (unlikely(list == NULL)) { \
        return NULL; \
    } \
    if (keepstart) { \
        PyList_SET_ITEM(list, 0, pyzero); \
    } \
    const T *str = (const T *)string; \
    for (i = j = 0; i < length; ) { \
        while (likely(i < length && !LINEBREAK_CHECK)) { \
            ++i; \
        } \
        if (likely(i < length)) { \
            if (str[i] == '\r' && i + 1 < length && str[i+1] == '\n') { \
                i += 2; \
            } \
            else { \
                ++i; \
            } \
        } \
        if (unlikely(!checked_for)) { \
            if (PyUnicode_CheckExact(_string) && i == length) { \
                break; \
            } \
            checked_for = 1; \
        } \
        sub = PyLong_FromSsize_t(i); \
        if (unlikely(sub == NULL)) { \
            Py_DECREF(list); \
            return NULL; \
        } \
        if (unlikely(PyList_Append(list, sub) != 0)) { \
            Py_DECREF(list); \
            Py_DECREF(sub); \
            return NULL; \
        } \
        else { \
            Py_DECREF(sub); \
        } \
    }
    if (likely(PyUnicode_IS_ASCII(_string))) {
        ALGO(Py_UCS1, ascii_linebreak[str[i]])
        return list;
    }
    switch (PyUnicode_KIND(_string)) {
        case PyUnicode_1BYTE_KIND: {
            ALGO(Py_UCS1, ascii_linebreak[str[i]])
            break;
        }
        case PyUnicode_2BYTE_KIND: {
            ALGO(Py_UCS2, (BLOOM(bloom_linebreak, str[i]) && Py_UNICODE_ISLINEBREAK(str[i])))
            break;
        }
        default: {
            // static_assert(stringkind == PyUnicode_4BYTE_KIND, "invalid unicode string kind");
            ALGO(Py_UCS4, (BLOOM(bloom_linebreak, str[i]) && Py_UNICODE_ISLINEBREAK(str[i])))
            break;
        }
    }
    return list;
#undef ALGO
}

PyDoc_STRVAR(c_slineoffsets__doc__, "\
c.slineoffsets(self, /, keepstart=False)\n\
    Return the beginning of lines as offsets. Start the list \n\
    with 0 if keepstart is a truthy value.\n\
");

static suggestionlist sgetline_kwlist = {
    2,
    {
        {4, "line"},
        {8, "keepends"},
    }
};

static PyObject *
c_sgetline(PyObject *self,
          PyObject *const *args,
          Py_ssize_t nargs,
          PyObject *kwargs)
{
    PyObject *kwarg, *_string, *_lineno, *_keepends;
    PyObject *result = NULL;
    const void *string;
    void *kwargdata;
    Py_ssize_t lineno, length, lenkwarg;
    Py_ssize_t total, nkwargs = 0, curline = 1;
    int stringkind, keepends = 0;
    int lineno_given, keepends_given;

    total = nargs + (nkwargs = kwargs == NULL ? 0 : PyTuple_GET_SIZE(kwargs));
    if (likely(nargs > 0 && (total == 2 || total == 3))) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method c.sgetline() missing "
                        "1 required positional argument: 'self'");
    }
    else if (unlikely(total == 1)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method c.sgetline() missing "
                        "1 required argument: 'line'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.sgetline() got "
                     "more arguments than accepted (%zd > 3)",
                     total);
    }
    return NULL;

  valid_args:
    switch (nargs) {
    case 3:
        _keepends = args[2];
        if (unlikely(_keepends == NULL)) {
            PyErr_BadArgument();
            return NULL;
        }
        keepends = PyObject_IsTrue(_keepends);
        if (unlikely(keepends < 0)) {
            return NULL;
        }
        keepends_given = 1;
    case 2:
        _lineno = args[1];
        lineno_given = 1;
    default:
        _string = args[0];
    }
    assert(nkwargs < 3);
    for (Py_ssize_t i = 0; i < nkwargs; ++i) {
        kwarg = PyTuple_GET_ITEM(kwargs, i);
        lenkwarg = PyUnicode_GET_LENGTH(kwarg);
        kwargdata = PyUnicode_DATA(kwarg);
        if (likely(lenkwarg == 4 && memcmp(kwargdata, "line", 4) == 0)) {
            if (unlikely(lineno_given)) {
                PyErr_SetString(PyExc_TypeError,
                                "unbound method c.sgetline() argument 'line' "
                                "is given two times (as positional argument 1 and as "
                                "keyword argument 'line')");
                return NULL;
            }
            _lineno = args[nargs + i];
        }
        else if (likely(lenkwarg == 8 && memcmp(kwargdata, "keepends", 8) == 0)) {
            if (unlikely(keepends_given)) {
                PyErr_SetString(PyExc_TypeError,
                                "unbound method c.sgetline() argument 'keepends' "
                                "is given two times (as positional argument 2 and as "
                                "keyword argument 'keepends')");
                return NULL;
            }
            _keepends = args[nargs + i];
            if (unlikely(_keepends == NULL)) {
                PyErr_BadArgument();
                return NULL;
            }
            keepends = PyObject_IsTrue(_keepends);
            if (unlikely(keepends < 0)) {
                return NULL;
            }
        }
        else {
            const char *suggestion = calculate_suggestions(sgetline_kwlist,
                                                           (suggestiontype){lenkwarg, (const char *)kwargdata});
            if (suggestion != NULL) {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.sgetline() got "
                             "an unexpected keyword argument '%U'. Did "
                             "you mean: '%s'?",
                             kwarg, suggestion);
            }
            else {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.sgetline() got "
                             "an unexpected keyword argument '%U'",
                             kwarg);
            }
            return NULL;
        }
    }
    _string = args[0];
    if (unlikely(_string == NULL)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (unlikely(!PyUnicode_Check(_string))) {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.sgetline() needs a "
                     "'str' object for argument 'self', got '%.200s' object",
                     Py_TYPE(_string)->tp_name);
        return NULL;
    }
    string = PyUnicode_DATA(_string);
    length = PyUnicode_GET_LENGTH(_string);
    stringkind = PyUnicode_KIND(_string);
    if (unlikely(_lineno == NULL)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (likely(PyLong_Check(_lineno))) {
        lineno = PyLong_AsSsize_t(_lineno);
        if (unlikely(lineno == -1 && PyErr_Occurred())) {
            return NULL;
        }
        if (unlikely(lineno == 0)) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.sgetline() argument "
                         "'line' (%zd) must not be zero",
                         lineno, PY_SSIZE_T_MAX);
            return NULL;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.sgetline() needs "
                     "'int' object for argument 'line', got '%200s'",
                     Py_TYPE(_lineno)->tp_name);
        return NULL;
    }
#define ALGO(T, NEWSTR, CASTNEW, LINEBREAK_CHECK) \
    Py_ssize_t i, j; \
    int checked_for = 0; \
 \
    const T *str = (const T *)string; \
    if (likely(lineno > 0)) { \
        for (i = j = 0; i < length; ) { \
            Py_ssize_t eol; \
            while (likely(i < length && !LINEBREAK_CHECK)) { \
                ++i; \
            } \
            eol = i; \
            if (likely(i < length)) { \
                if (str[i] == '\r' && i + 1 < length && str[i+1] == '\n') { \
                    i += 2; \
                } \
                else { \
                    ++i; \
                } \
                if (unlikely(keepends)) { \
                    eol = i; \
                } \
            } \
            else if (unlikely(!checked_for)) { \
                if (PyUnicode_CheckExact(_string) && lineno != 1) { \
                    break; \
                } \
                checked_for = 1; \
            } \
            if (unlikely(curline++ == lineno)) { \
                result = NEWSTR((CASTNEW)(str + j), eol - j); \
                if (unlikely(result == NULL)) { \
                    return NULL; \
                } \
                break; \
            } \
            j = i; \
        } \
        if (result == NULL) { \
            PyErr_Format(PyExc_TypeError, \
                         "unbound method c.sgetline() argument " \
                         "'lineno' out of bounds (%zd > %zd)", \
                         lineno, curline - 1); \
            return NULL; \
        } \
    } \
    else { \
        lineno = -lineno; \
        const Py_ssize_t maxidx = length - 1; \
        for (i = j = maxidx; i >= 0; ) { \
            Py_ssize_t eol; \
            while (likely(i >= 0 && !LINEBREAK_CHECK)) { \
                --i; \
            } \
            eol = i; \
            if (likely(i >= 0)) { \
                if (str[i] == '\n' && i - 1 >= 0 && str[i-1] == '\r') { \
                    i -= 2; \
                } \
                else { \
                    --i; \
                } \
            } \
            else if (unlikely(!checked_for)) { \
                if (PyUnicode_CheckExact(_string) && lineno != 1) { \
                    break; \
                } \
                checked_for = 1; \
            } \
            if (unlikely(curline++ == lineno)) { \
                result = NEWSTR((CASTNEW)(str + eol + 1), j - eol); \
                if (unlikely(result == NULL)) { \
                    return NULL; \
                } \
                break; \
            } \
            j = unlikely(keepends) ? eol : i; \
        } \
        if (result == NULL) { \
            PyErr_Format(PyExc_TypeError, \
                         "unbound method c.sgetline() argument " \
                         "'lineno' out of bounds (-%zd < -%zd)", \
                         lineno, curline - 1); \
            return NULL; \
        } \
    }
    if (likely(PyUnicode_IS_ASCII(_string))) {
        ALGO(Py_UCS1, _PyUnicode_FromASCII, const char *, ascii_linebreak[str[i]])
        return result;
    }
    switch (PyUnicode_KIND(_string)) {
        case PyUnicode_1BYTE_KIND: {
            ALGO(Py_UCS1, _PyUnicode_FromUCS1, const Py_UCS1 *, ascii_linebreak[str[i]])
            break;
        }
        case PyUnicode_2BYTE_KIND: {
            ALGO(Py_UCS2, _PyUnicode_FromUCS2, const Py_UCS2 *, (BLOOM(bloom_linebreak, str[i]) && Py_UNICODE_ISLINEBREAK(str[i])))
            break;
        }
        default: {
            // static_assert(stringkind == PyUnicode_4BYTE_KIND, "invalid unicode string kind");
            ALGO(Py_UCS4, _PyUnicode_FromUCS4, const Py_UCS4 *, (BLOOM(bloom_linebreak, str[i]) && Py_UNICODE_ISLINEBREAK(str[i])))
            break;
        }
    }
    return result;
#undef ALGO
}

PyDoc_STRVAR(c_sgetline__doc__, "\
c.sgetline(self, /, line, keepends=False)\n\
    Get a line from a string. Works like str.splitlines().\n\
\n\
      line\n\
        The line to get. Can be negative.\n\
      keepends\n\
        Whether to keep the line ending or not.\n\
");

static PyObject *
c_split(PyObject *self,
        PyObject *const *args,
        Py_ssize_t nargs,
        PyObject *kwargs)
{
    PyObject *kwarg;
    PyObject *_num, *_n, *quot_obj, *quot1_obj = NULL, *list;
    PyObject **list_items;
    size_t num, n, quot, rem, repsize, prev_repsize;
    Py_ssize_t nkwargs, lenkwarg, total, maxiter, j, i = 0;
    int thread_num;

    total = nargs + (nkwargs = kwargs == NULL ? 0 : PyTuple_GET_SIZE(kwargs));
    if (likely(nargs == 2)) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method c.split() missing "
                        "1 required positional argument: 'self'");
    }
    else if (nargs == 1) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method c.split() missing "
                        "1 required argument: 'n'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.split() got "
                     "more arguments than accepted (%zd > 2)",
                     nargs);
    }
    return NULL;

  valid_args:
    if (unlikely(nkwargs != 0)) {
        kwarg = PyTuple_GET_ITEM(kwargs, 0);
        lenkwarg = PyUnicode_GET_LENGTH(kwarg);
        if (unlikely(lenkwarg != 1 || *(const char *)PyUnicode_DATA(kwarg) != 'n')) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.split() got "
                         "an unexpected keyword argument '%U'",
                         kwarg);
            return NULL;
        }
    }
    num = PyLong_AsSize_t(args[0]);
    if (unlikely(num == -1 && PyErr_Occurred())) {
        return NULL;
    }
    n = PyLong_AsSize_t(args[1]);
    if (unlikely(n == -1 && PyErr_Occurred())) {
        return NULL;
    }
    quot = num / n;
    rem = num % n;
    quot_obj = PyLong_FromSize_t(quot);
    if (unlikely(quot_obj == NULL)) {
        return NULL;
    }
    list = PyList_New(n);
    if (unlikely(list == NULL)) {
        Py_DECREF(quot_obj);
        Py_XDECREF(quot1_obj);
        return NULL;
    }
    list_items = ((PyListObject *)list)->ob_item;
    if (rem) {
        quot1_obj = PyLong_FromSize_t(quot + 1);
        if (unlikely(quot1_obj == NULL)) {
            Py_DECREF(list);
            return NULL;
        }
        Py_INC_REFCNT(quot1_obj, rem - 1);
        while (likely(i < rem)) {
            list_items[i++] = quot1_obj;
        }
    }
    quot_obj = PyLong_FromSize_t(quot);
    if (unlikely(quot_obj == NULL)) {
        Py_DECREF(list);
        return NULL;
    }
    Py_INC_REFCNT(quot_obj, n - rem - 1);
    while (likely(i < n)) {
        list_items[i++] = quot_obj;
    }
    return list;
}

PyDoc_STRVAR(c_split__doc__, "\
c.split(self, /, n)\n\
    Split an integer into n groups. Returns a list sorted from greatest\n\
    to least.\n\
\n\
      n\n\
        The number of groups.\n\
");

static PyObject *
c_to_bytes(PyObject *self,
           PyObject *const *args,
           Py_ssize_t nargs)
{
    PyObject *_string;
    const char *string;
    Py_ssize_t total;

    if (unlikely(nargs != 1)) {
        if (nargs == 0) {
            PyErr_SetString(PyExc_TypeError,
                            "unbound method c.to_bytes() missing "
                            "1 required positional argument: 'self'");
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.to_bytes() got "
                         "more arguments than accepted (%zd > 1)",
                         nargs);
        }
        return NULL;
    }
    if (unlikely(!PyUnicode_Check(_string = args[0]))) {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.to_bytes() got "
                     "a '%.200s' object instead of a 'str' object",
                     Py_TYPE(_string)->tp_name);
        return NULL;
    }
    return PyBytes_FromStringAndSize((const char *)PyUnicode_DATA(_string),
                                     PyUnicode_GET_LENGTH(_string) * PyUnicode_KIND(_string));
}

PyDoc_STRVAR(c_to_bytes__doc__, "\
c.to_bytes(self, /)\n\
    Return the string internal buffer as a bytes object.\n\
");

static PyObject *
c_ints_to_string(PyObject *self,
                 PyObject *const *args,
                 Py_ssize_t nargs)
{
    PyObject *item, *res, *seq, **data;
    Py_ssize_t i, size, intsize;
    digit first_digit;

    if (unlikely(nargs != 1)) {
        if (nargs == 0) {
            PyErr_SetString(PyExc_TypeError,
                            "unbound method c.ints_to_string() missing "
                            "1 required positional argument: 'seq'");
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.ints_to_string() got "
                         "more arguments than accepted (%zd > 1)",
                         nargs);
        }
        return NULL;
    }
    if (unlikely(!PyList_CheckExact(seq = args[0]))) {
        PyErr_Format(PyExc_TypeError,
                     "unbound method c.ints_to_string() got "
                     "a '%.200s' object instead of a 'list' object",
                     Py_TYPE(seq)->tp_name);
        return NULL;
    }

    if (unlikely((size = Py_SIZE(seq)) == 0)) {
        _Py_RETURN_UNICODE_EMPTY();
    }
    else if (size == 1) {
        item = ((PyListObject *)seq)->ob_item[0];
        if (unlikely(!PyLong_Check(item))) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.ints_to_string() got "
                         "a '%.200s' object at element 0 instead of "
                         "a 'int' instance",
                         Py_TYPE(item)->tp_name);
            return NULL;
        }
        if ((intsize = Py_SIZE(item)) == 0) {
            return get_latin1_char(0);
        }
        first_digit = ((PyLongObject *)item)->ob_digit[0];
        if (unlikely(intsize != 1 || first_digit > 255)) {
            PyErr_SetString(PyExc_TypeError,
                            "unbound method c.ints_to_string(): "
                            "integer at element 0 does not satisfy "
                            "0 <= n <= 255");
            return NULL;
        }
        return get_latin1_char(first_digit);
    }
    data = ((PyListObject *)seq)->ob_item;
    res = PyUnicode_New(size, 255);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    Py_UCS1 *strdata = PyUnicode_1BYTE_DATA(res);
    for (i = 0; i < size; ++i) {
        item = data[i];
        if (unlikely(!PyLong_Check(item))) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method c.ints_to_string() got "
                         "a '%.200s' object at element %zd instead of "
                         "a 'int' instance",
                         Py_TYPE(item)->tp_name, i);
            return NULL;
        }
        if ((intsize = Py_SIZE(item)) != 0) {
            first_digit = ((PyLongObject *)item)->ob_digit[0];
            if (unlikely(intsize != 1 || first_digit > 255)) {
                PyErr_Format(PyExc_TypeError,
                             "unbound method c.ints_to_string(): "
                             "integer at element %zd does not satisfy "
                             "0 <= n <= 255",
                             i);
                return NULL;
            }
            strdata[i] = first_digit;
        }
        else{
            strdata[i] = 0;
        }
    }
    return res;
}

PyDoc_STRVAR(c_ints_to_string__doc__, "\
c.ints_to_string(seq, /)\n\
    Return the string equivalent of the sequence.\n\
");

#define METH_FASTCALL_KEYWORDS_NODOC(name) \
    {#name, (PyCFunction)(void(*)(void))c_ ## name, METH_FASTCALL | METH_KEYWORDS, ""},

#define METH_FASTCALL_NODOC(name) \
    {#name, (PyCFunction)(void(*)(void))c_ ## name, METH_FASTCALL, ""},

#define METH_FASTCALL_KEYWORDS_DOC(name) \
    {#name, (PyCFunction)(void(*)(void))c_ ## name, METH_FASTCALL | METH_KEYWORDS, c_ ## name ## __doc__},

#define METH_FASTCALL_DOC(name) \
    {#name, (PyCFunction)(void(*)(void))c_ ## name, METH_FASTCALL, c_ ## name ## __doc__},

static PyMethodDef CMethods[] = {
    METH_FASTCALL_KEYWORDS_DOC(scharindices)
    METH_FASTCALL_KEYWORDS_DOC(sgetline)
    METH_FASTCALL_KEYWORDS_DOC(slineoffsets)
    METH_FASTCALL_KEYWORDS_DOC(split)

    METH_FASTCALL_DOC(to_bytes)
    METH_FASTCALL_DOC(ints_to_string)

    {NULL}
};

PyDoc_STRVAR(c___doc__,
             "C extension");

static struct PyModuleDef cmodule = {
    PyModuleDef_HEAD_INIT,
    "c",
    c___doc__,
    -1,
    CMethods,
};

Py_EXPORTED_SYMBOL PyObject *
PyInit_c(void)
{
    const Py_UCS2 linebreak[] = {
        0x000A, /* LINE FEED */
        0x000D, /* CARRIAGE RETURN */
        0x001C, /* FILE SEPARATOR */
        0x001D, /* GROUP SEPARATOR */
        0x001E, /* RECORD SEPARATOR */
        0x0085, /* NEXT LINE */
        0x2028, /* LINE SEPARATOR */
        0x2029, /* PARAGRAPH SEPARATOR */
    };
    bloom_linebreak = make_bloom_mask(
        PyUnicode_2BYTE_KIND, linebreak,
        Py_ARRAY_LENGTH(linebreak));
    pyzero = PyLong_FromLong(0L);
    if (unlikely(pyzero == NULL)) {
        return NULL;
    }
    return PyModule_Create(&cmodule);
}
