#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define Py_BUILD_CORE
#include "internal/pycore_runtime_init.h"
#include "internal/pycore_long.h"
#undef Py_BUILD_CORE

#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define unlikely(x) __builtin_expect((x), 0)
#define likely(x) __builtin_expect((x), 1)

static inline PyTupleObject *
tuple_alloc_unsafe(Py_ssize_t size)
{
    PyTupleObject *op = NULL;
#if PyTuple_NFREELISTS > 0
    struct _Py_tuple_state tup = _PyInterpreterState_GET()->tuple;
    if (size < PyTuple_MAXSAVESIZE) {
        Py_ssize_t index = size - 1;
        op = tup.free_list[index];
        if (op != NULL) {
            tup.free_list[index] = (PyTupleObject *)op->ob_item[0];
            tup.numfree[index]--;
#ifdef Py_TRACE_REFS
            Py_SET_SIZE(op, size);
            Py_SET_TYPE(op, &PyTuple_Type);
#endif
            _Py_NewReference((PyObject *)op);
            return op;
        }
    }
    if (op == NULL) {
#endif
        op = PyObject_GC_NewVar(PyTupleObject, &PyTuple_Type, size);
# if PyTuple_NFREELISTS > 0
    }
#endif
    return op;
}

static const double
func_a_c_f(const double x)
{
    return fmod(x, 1.0);
}

static const size_t
func_a_c_i(const double x)
{
    return (size_t)x;
}

static inline const double
inline_a_c_f(const register double x)
{
    return fmod(x, 1.0);
}

static inline const size_t
inline_a_c_i(const register double x)
{
    return (size_t)x;
}

static PyObject *
a_c_something_safe1(PyObject *self, PyObject *x)
{
    PyObject *tup;
    double x_double;

    if (__builtin_expect(x == NULL, 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "NULL argument passed to a_c.something_safe1()");
        return NULL;
    }
    if (__builtin_expect(!PyFloat_CheckExact(x), 0)) {
        PyErr_Format(PyExc_TypeError,
                     "1st argument to a_c.something_safe1 must "
                     "be of type float, instead found argument of "
                     "type %.200s",
                     Py_TYPE(x)->tp_name);
        return NULL;
    }
    x_double = PyFloat_AsDouble(x);
    tup = PyTuple_Pack(2,
                       PyLong_FromSize_t(func_a_c_i(x_double)),
                       PyFloat_FromDouble(func_a_c_f(x_double)));
    if (__builtin_expect(tup == NULL, 0)) {
        return NULL;
    }
    return tup;
}

static PyObject *
a_c_something_safe2(PyObject *self, PyObject *x)
{
    register PyObject *tup;
    register double x_double;

    if (__builtin_expect(x == NULL, 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "NULL argument passed to a_c.something_safe2()");
        return NULL;
    }
    x_double = PyFloat_AsDouble(x);
    if (__builtin_expect(!PyFloat_CheckExact(x), 0)) {
        PyErr_Format(PyExc_TypeError,
                     "1st argument to a_c.something_safe2 must "
                     "be of type float, instead found argument of "
                     "type %.200s",
                     Py_TYPE(x)->tp_name);
        return NULL;
    }
    tup = PyTuple_Pack(
        2,
        PyLong_FromSize_t(inline_a_c_i(x_double)),
        PyFloat_FromDouble(inline_a_c_f(x_double))
    );
    if (__builtin_expect(tup == NULL, 0)) {
        return NULL;
    }
    return tup;
}

static binaryfunc long_mul;
static PyTypeObject *long_type;
static PyObject *one;

static inline int
int_inc(PyObject **x)
{
    int carry = 1;
    digit *digits = ((PyLongObject *)*x)->ob_digit;
    Py_ssize_t i, size = Py_SIZE(*x);
    for (i = 0; i < size && carry; ++i) {
        if (digits[i] < PyLong_MASK) {
            digits[i] += 1;
            if (unlikely(i > 0)) {
                memset(digits, 0, sizeof(digit) * i);
            }
            return 1;
        }
    }

    PyLongObject *temp = _PyLong_New(size + 1);
    if (unlikely(temp == NULL)) return 0;
    digits = temp->ob_digit;
    memset(digits, 0, sizeof(digit) * size);
    digits[size] = 1;
    *x = (PyObject *)temp;
    return 1;
}

static PyObject *
a_c_factorial(PyObject *self, PyObject *x)
{
    size_t partial, n, n_part, j, i, c;
    int large_fact = 0;
    PyObject *res, *temp, *temp2;

    if (__builtin_expect(Py_TYPE(x) == long_type, 1)) {
        const Py_ssize_t long_arg_size = Py_SIZE(x);
        if (likely(long_arg_size == 1)) {
            n = ((PyLongObject *)x)->ob_digit[0];
        }
        else if (long_arg_size == 0) {
            n = 1;
        }
        else {
            const digit *ob_digit = ((PyLongObject *)x)->ob_digit;;
            switch (long_arg_size) {
            case 2:
                n = (size_t)ob_digit[0] | ((size_t)ob_digit[1] << PyLong_SHIFT);
#if SIZEOF_SIZE_T == 8
                break;
            case 3:
                n = ((size_t)ob_digit[0]
                     | (((size_t)ob_digit[1]
                         | ((size_t)ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
#endif
                if (unlikely(
#if SIZEOF_SIZE_T == 8
                             ob_digit[2] >= 16))
#else
                             ob_digit[1] >= 4))
#endif
                {
                    large_fact = 1;
                    n = SIZE_MAX;
                }
                break;
            default:
                if (likely(long_arg_size > 0)) {
                    large_fact = 1;
                    n = SIZE_MAX;
                    break;
                }
                n = 1;
                break;
            }
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "non-int passed to a_c.something_safe2()");
        return NULL;
    }

#define LIMITED_FACTORIAL(c) \
    partial = i; \
    if (n < c) { \
        for (; ++i < n;) partial *= i; \
        temp = PyLong_FromSize_t(partial); \
        if (unlikely(temp == NULL)) goto error; \
        temp2 = long_mul(res, temp); \
        Py_DECREF(res); \
        Py_DECREF(temp); \
        if (unlikely(temp2 == NULL)) return NULL; \
        return temp2; \
    } \
    else { \
        for (; ++i < c;) partial *= i; \
        temp = PyLong_FromSize_t(partial); \
        if (unlikely(temp == NULL)) goto error; \
        temp2 = long_mul(res, temp); \
        Py_DECREF(res); \
        Py_DECREF(temp); \
        if (unlikely(temp2 == NULL)) return NULL; \
        res = temp2; \
    } \

    if (n < 21) {
        partial = n;
        for (i = 1; ++i < n;) partial *= i;
        res = PyLong_FromSize_t(partial);
        if (unlikely(res == NULL)) goto error;
        return res;
    }
    else {
        i = 21;
        res = PyLong_FromSize_t(2432902008176640000ULL);
        if (unlikely(res == NULL)) goto error;
    }

    LIMITED_FACTORIAL(34)

    LIMITED_FACTORIAL(46)

    LIMITED_FACTORIAL(57)

    LIMITED_FACTORIAL(67)
    LIMITED_FACTORIAL(77)
    LIMITED_FACTORIAL(87)

    LIMITED_FACTORIAL(96)
    LIMITED_FACTORIAL(105)
    LIMITED_FACTORIAL(114)
    LIMITED_FACTORIAL(123)
    LIMITED_FACTORIAL(132)
    LIMITED_FACTORIAL(141)

#define REPEATED_FACTORIAL(delta, rep) \
    for (j = 0; j < rep; ++j) { \
        c += delta; \
        LIMITED_FACTORIAL(c); \
    } \

    c = 141;

    REPEATED_FACTORIAL(8, 14)
    REPEATED_FACTORIAL(7, 45)
    REPEATED_FACTORIAL(6, 176)
    REPEATED_FACTORIAL(5, 1102)
    REPEATED_FACTORIAL(4, 14601)
    REPEATED_FACTORIAL(3, 858903)
    REPEATED_FACTORIAL(2, 2146162525ULL) /* tested value only */

#undef REPEATED_FACTORIAL
#undef LLIMITED_FACTORIAL

    temp = PyLong_FromSize_t(c);
    if (unlikely(temp == NULL)) goto error;
    n_part = Py_MIN(n, SIZE_MAX);
    for (; c < n_part; ++c) {
        temp2 = long_mul(res, temp);
        Py_DECREF(res);
        if (unlikely(temp2 == NULL)) {
            Py_DECREF(temp);
            return NULL;
        }
        res = temp2;
        if (unlikely(!int_inc(&temp))) {
            Py_DECREF(temp);
            return NULL;
        }
    }
    if (unlikely(large_fact)) {
        const Py_ssize_t x_size = Py_SIZE(x);
        while (true) {
            temp2 = long_mul(res, temp);
            Py_DECREF(res);
            if (unlikely(temp2 == NULL)) {
                Py_DECREF(temp);
                return NULL;
            }
            res = temp2;
            if (unlikely(!int_inc(&temp))) {
                Py_DECREF(temp);
                return NULL;
            }
            if (x_size > Py_SIZE(temp)) {
                continue;
            }
            else if (x_size == Py_SIZE(temp)) {
                int status = PyObject_RichCompareBool(x, temp, Py_EQ);
                if (!status) {
                    continue;
                }
                Py_DECREF(temp);
                if (status) break;
                goto error;
            }
        }
    }

    temp2 = long_mul(res, x);
    Py_DECREF(res);
    if (unlikely(temp2 == NULL)) {
        return NULL;
    }
    return temp2;
  error:
    Py_DECREF(res);
    return NULL;
}

static PyObject *
a_c_something_unsafe1(PyObject *self, PyObject *x)
{
    PyTupleObject *tup;
    double x_double;

    x_double = ((PyFloatObject *)x)->ob_fval;
    tup = (PyTupleObject *)tuple_alloc_unsafe(2);
    tup->ob_item[0] = PyLong_FromSize_t(func_a_c_i(x_double));
    tup->ob_item[1] = PyFloat_FromDouble(func_a_c_f(x_double));
    return (PyObject *)tup;
}

static PyObject *
a_c_something_unsafe2(PyObject *self, PyObject *x)
{
    const register double x_double = ((PyFloatObject *)x)->ob_fval;
    register PyTupleObject *tup = (PyTupleObject *)tuple_alloc_unsafe(2);
    tup->ob_item[0] = PyLong_FromSize_t(inline_a_c_i(x_double));
    tup->ob_item[1] = PyFloat_FromDouble(inline_a_c_f(x_double));
    return (PyObject *)tup;
}

static inline PyObject *
pick(register PyObject **input, register Py_ssize_t k,
     register Py_ssize_t size_input, register PyObject **picked,
     register PyListObject *picked_list, register Py_ssize_t *index,
     register Py_ssize_t next_valid)
{
    register Py_ssize_t size_picked;
    if ((size_picked = *index + 1) == k) {
    }
    for (register Py_ssize_t i = next_valid; i < size_input; ++i) {
        picked[(*index)++] = input[i];
        if ((size_picked = *index + 1) == k) {
        }
    }
    return *input;
}

static inline Py_ssize_t
factorial(register Py_ssize_t x)
{
    for (register Py_ssize_t i = x-1; i > 1; ++i) x *= i;
    return x;
}

static PyObject *
a_c_combinations_iter(register PyObject *self,
                      register PyObject *const *args,
                      register Py_ssize_t nargs,
                      register PyObject *kwnames)
{
    register PyListObject *input;
    const register Py_ssize_t k = PyLong_AsSsize_t(args[1]);
    const register Py_ssize_t size_input = Py_SIZE(input = (PyListObject *)args[0]);
    register PyListObject *l = (PyListObject *)PyList_New(factorial(size_input) / (factorial(k) * factorial(size_input - k)));
    return PySeqIter_New(pick(input->ob_item, k, size_input, l->ob_item, l, &(Py_ssize_t){0}, 0));
}

static PyObject *
a_c_combinations(register PyObject *self,
                      register PyObject *const *args,
                      register Py_ssize_t nargs,
                      register PyObject *kwnames)
{
    register PyListObject *input;
    const register Py_ssize_t k = PyLong_AsSsize_t(args[1]);
    const register Py_ssize_t size_input = Py_SIZE(input = (PyListObject *)args[0]);
    register PyListObject *l = (PyListObject *)PyList_New(factorial(size_input) / (factorial(k) * factorial(size_input - k)));
    return pick(input->ob_item, k, size_input, l->ob_item, l, &(Py_ssize_t){0}, 0);
}

#ifdef USE_UNDOCUMENTED
#define FLOAT_TO_DOUBLE(x) (((PyFloatObject *)x)->ob_fval)
#else
#define FLOAT_TO_DOUBLE(x) PyFloat_AsDouble(x)
#endif

/* Now using METH_FASTCALL | METH_KEYWORDS convention */
static PyObject *
a_c_hello(PyObject *self,
          PyObject *const *args,
          Py_ssize_t nargs,
          PyObject *kwnames)
{
    /*
    // check for args size
    if (nargs != 4) {
        PyErr_Format(PyExc_TypeError,
                     "hello() expected 4 arguments (got %zd)",
                     nargs);
        return NULL;
    }
    */ 
    const double theta = FLOAT_TO_DOUBLE(args[0]) * 0.0174532925199432957692369076848861271344287188854172545609719143889;
    PyObject* vertices = args[1];
    const double pivot_x = FLOAT_TO_DOUBLE(args[2]);
    const double pivot_y = FLOAT_TO_DOUBLE(args[3]);

    const double dc = cos(theta);
    const double ds = sin(theta);

    for (Py_ssize_t i = 0; __builtin_expect(i < 4, 1); i++) {
        const Py_ssize_t I7 = i * 7;

        const double x = FLOAT_TO_DOUBLE(PyList_GET_ITEM(vertices, I7)) - pivot_x;
        const double y = FLOAT_TO_DOUBLE(PyList_GET_ITEM(vertices, I7 + 1)) - pivot_y;

        PyList_SET_ITEM(vertices, I7, PyFloat_FromDouble(dc*x - ds*y + pivot_x));
        PyList_SET_ITEM(vertices, I7 + 1, PyFloat_FromDouble(ds*x - dc*y + pivot_y));
    };
    Py_INCREF(vertices);
    return vertices;
}

static Py_ssize_t
parse_varint_fast(uint_fast8_t *seq, Py_ssize_t *idx)
{
    Py_ssize_t b;
    Py_ssize_t val = (b = (Py_ssize_t)seq[(*idx)++]) & 63;
    while (__builtin_expect(b & 64, 0)) {
        val = (val << 6) | ((b = (Py_ssize_t)seq[(*idx)++]) & 63);
    }
    return val;
}

static PyObject *zero;
static PyObject *false_or_true[2];

static PyObject *
a_c_parse_exception_table(PyObject *self,
                          PyObject *const *args,
                          Py_ssize_t nargs)
{
    PyBytesObject *arg;
    uint_fast8_t *seq = (uint_fast8_t *)(arg = (PyBytesObject *)((PyCodeObject *)args[0])->co_exceptiontable)->ob_sval;
    const Py_ssize_t length = ((PyVarObject *)arg)->ob_size;
    PyObject *res = PyList_New(length >> 2);
    PyObject **ob_item = ((PyListObject *)res)->ob_item;
    Py_ssize_t res_idx = 0;
    Py_ssize_t idx = 0;
    PyTupleObject *tup;
    PyObject **tup_items;
    PyObject *integer;
    while (__builtin_expect(idx < length, 1)) {
        tup = (PyTupleObject *)(ob_item[res_idx++] = (PyObject *)PyTuple_New(5));
        tup_items = tup->ob_item;
        Py_ssize_t start, end, dl, depth;
#define ASSIGN_1DIGIT(assign_to, ssize_var, obj_var, expr) \
    if (__builtin_expect(((ssize_var) = (expr)) < _PY_NSMALLPOSINTS, 1)) { \
        (obj_var) = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+(ssize_var)]; \
        Py_INCREF((obj_var)); \
    } \
    else { \
        (obj_var) = (PyObject *)&(PyLongObject){ \
            _PyVarObject_IMMORTAL_INIT(&PyLong_Type, 1), \
            .ob_digit = {(digit)(ssize_var)}, \
        }; \
    } \
    (assign_to) = (obj_var);

        ASSIGN_1DIGIT(tup_items[0], start, integer, parse_varint_fast(seq, &idx))
        ASSIGN_1DIGIT(tup_items[1], end, integer, start + parse_varint_fast(seq, &idx) + 1)
        parse_varint_fast(seq, &idx);
        Py_INCREF(zero);
        tup_items[2] = zero;
        ASSIGN_1DIGIT(tup_items[3], depth, integer, (dl = parse_varint_fast(seq, &idx)) >> 1)
        PyObject *elm = false_or_true[dl & 1];
        Py_INCREF(elm);
        tup_items[4] = elm;
        ((PyVarObject *)tup)->ob_size = 5;
#undef ASSIGN_1DIGIT
    }
    ((PyVarObject *)res)->ob_size = res_idx;
    return res;
}

typedef struct {
    PyObject_HEAD
    PyObject *start;
    PyObject *stop;
    PyObject *step;
    PyObject *length;
} rangeobject;

static PyObject *
a_c_compute_range_length(PyObject *self,
                          PyObject *const *args,
                          Py_ssize_t nargs)
{
    int cmp_result;
    Py_ssize_t size_long;
    PyObject *lo, *hi, *step;
    PyObject *diff = NULL;
    PyObject *tmp1 = NULL, *tmp2 = NULL, *result;

    rangeobject *arg = (rangeobject *)args[0];

    if (__builtin_expect((size_long = Py_SIZE(step = arg->step)) > 0, 1)) {
        lo = arg->start;
        hi = arg->stop;
        Py_INCREF(step);
    }
    else {
        lo = arg->stop;
        hi = arg->start;
        step = PyNumber_Negative(step);
        if (__builtin_expect(step == NULL, 0)) {
            return NULL;
        }
    }

    cmp_result = PyObject_RichCompareBool(lo, hi, Py_GE);
    if (__builtin_expect(cmp_result != 0, 0)) {
        Py_DECREF(step);
        if (__builtin_expect(cmp_result == -1, 0)) {
            return NULL;
        }
        Py_INCREF(zero);
        return zero;
    }

    if (__builtin_expect((tmp1 = PyNumber_Subtract(hi, lo)) == NULL, 0)) {
        Py_DECREF(step);
        return NULL;
    }
    if (__builtin_expect((diff = PyNumber_Subtract(tmp1, one)) == NULL, 0)) {
        Py_DECREF(step);
        Py_DECREF(tmp1);
        return NULL;
    }
    Py_DECREF(tmp1);
    if (__builtin_expect((tmp2 = PyNumber_FloorDivide(diff, step)) == NULL, 0)) {
        Py_DECREF(step);
        Py_DECREF(diff);
        return NULL;
    }
    Py_DECREF(step);
    Py_DECREF(diff);
    if (__builtin_expect((result = PyNumber_Add(tmp2, one)) == NULL, 0)) {
        Py_DECREF(tmp2);
        return NULL;
    }
    Py_DECREF(tmp2);

    return result;
}

static PyObject *
a_c_is_even(PyObject *self,
            PyObject *const *args,
            Py_ssize_t nargs)
{
    PyObject *res = __builtin_expect(*((char *)args[0] + offsetof(PyLongObject, ob_digit)) & 1, 0) ? Py_False : Py_True;
    Py_INCREF(res);
    return res;
}

static PyObject *
a_c_remove_dupes(PyObject *self,
                 PyObject *const *args,
                 Py_ssize_t nargs)
{
    PyListObject *arg;
    PyObject **els, *item, *startkey;
    PyTypeObject *tp;
    setentry *table, *entry, *freeslot = NULL;
    Py_ssize_t set_fill, set_used;
    size_t perturb, mask, idx;
    int probes, cmp;
    Py_hash_t hash;
    PySetObject *passed = (PySetObject *)PyType_GenericAlloc(&PySet_Type, 0);
    set_fill = set_used = passed->fill = passed->used = passed->finger = 0;
    passed->mask = 7;
    passed->hash = -1;
    passed->table = passed->smalltable;
    passed->weakreflist = NULL;
    Py_ssize_t length = Py_SIZE(arg = (PyListObject *)args[0]);
    if (length < 2) {
        return (PyObject *)passed;
    }
    els = arg->ob_item;
    for (Py_ssize_t i = 0; __builtin_expect(i < length, 0);) {
        hash = __builtin_expect((tp = Py_TYPE(item = els[i]))->tp_hash != NULL, 1) ? (*tp->tp_hash)(item) : -1;
        if (__builtin_expect(hash == -1, 0)) {
            if (__builtin_expect(PyErr_Occurred() != NULL, 1)) {
                PyErr_Clear();
            }
            if (__builtin_expect(tp->tp_dict == NULL, 0)) {
                (void) PyType_Ready(tp);
                hash = (*tp->tp_hash)(item);
                if (__builtin_expect(hash != -1, 1)) {
                    goto redo_contains;
                }
                if (__builtin_expect(PyErr_Occurred() != NULL, 1)) {
                    PyErr_Clear();
                }
            }
            PyObject *item2;
            for (Py_ssize_t j = 0; __builtin_expect(j < i, 0); j++) {
                if (__builtin_expect(item == (item2 = els[j]) ||
                                     PyObject_RichCompareBool(item, item2, Py_EQ), 0)) {
                    goto was_passed;
                }
            }
            i++;
            continue;
        }
 redo_contains:
        perturb = hash;
        mask = passed->mask;
        idx = (size_t)hash & mask;
        while (1) {
            entry = &passed->table[idx];
            probes = (idx + 9 <= mask) ? 9 : 0;
            do {
                if (__builtin_expect(entry->hash == 0 && entry->key == NULL, 1)) {
                    goto found_unused_or_dummy;
                }
                if (__builtin_expect(entry->hash == hash, 0)) {
                    if (__builtin_expect((startkey = entry->key) == item, 1)) {
                        goto was_passed;
                    }
                    if (__builtin_expect(
                            PyUnicode_CheckExact(startkey)
                            && PyUnicode_CheckExact(item)
                            && _PyUnicode_EQ(startkey, item),
                            0))
                    {
                        goto was_passed;
                    }
                    table = passed->table;
                    Py_INCREF(startkey);
                    if (__builtin_expect(PyObject_RichCompareBool(startkey, item, Py_EQ), 1)) {
                        Py_DECREF(startkey);
                        goto was_passed;
                    }
                    Py_DECREF(startkey);
                    if (__builtin_expect(table != passed->table || entry->key != startkey, 0)) {
                        goto redo_contains;
                    }
                    mask = passed->mask;
                }
                else if (__builtin_expect(entry->hash == -1, 1)) {
                    freeslot = entry;
                }
                entry++;
            } while (probes--);
            idx = (idx*5 + (perturb >>= 5) + 1) & mask;
        }
  was_passed:
        Py_DECREF(item);
        memcpy(els + i, els + i + 1, sizeof(PyObject *) * (length - i));
        length--;
        continue;
  found_unused_or_dummy:
        Py_INCREF(item);
        set_used = passed->used++;
        if (__builtin_expect(freeslot == NULL, 1)) {
            goto found_unused;
        }
        freeslot->key = item;
        freeslot->hash = hash;
        i++;
        continue;
  found_unused:
        set_fill = passed->fill++;
        entry->key = item;
        entry->hash = hash;
        if (__builtin_expect((size_t)set_fill*5 >= mask*3, 0)) {
            Py_ssize_t minused = set_used>50000 ? set_used*2 : set_used*4;
            size_t newmask;
            setentry *newtable;
            setentry small_copy[8];
            size_t newsize = 8 <= (size_t)minused ? 1 << (64-__builtin_clzll((size_t)minused)) : 8;
            mask = passed->mask;
            int is_oldtable_malloced = (table = passed->table) != passed->smalltable;
            if (__builtin_expect(newsize == 8, 0)) {
                newtable = passed->smalltable;
                if (__builtin_expect(newtable == table, 1)) {
                    if (__builtin_expect(passed->fill == passed->used, 0)) {
                        goto done_resize;
                    }
                    memcpy(small_copy, table, sizeof(small_copy));
                    table = small_copy;
                }
            }
            else {
                newtable = (setentry *)PyMem_Calloc(newsize, sizeof(setentry));
            }
            passed->mask = (Py_ssize_t)(newmask = newsize - 1);
            passed->table = newtable;
            setentry *stop_point = table + mask;
            if (__builtin_expect(set_fill == set_used, 0)) {
                for (entry = table; __builtin_expect(entry <= stop_point, 0); entry++) {
  redo_moving1:
                    if (__builtin_expect(entry->key != NULL, 1)) {
                        setentry *entry2;
                        perturb = hash;
                        idx = hash & newmask;
                        size_t j;
                        while (1) {
                            entry2 = &newtable[idx];
                            if (__builtin_expect(entry2->key == NULL, 0)) {
                                entry2->key = entry->key;
                                entry2->hash = entry->hash;
                                if (__builtin_expect(entry++ > stop_point, 0)) {
                                    goto finalize_resize;
                                }
                                goto redo_moving1;
                            }
                            if (__builtin_expect(idx + 9 <= newmask, 1)) {
                                for (j = 0; __builtin_expect(j < 9, 0); j++) {
                                    if (__builtin_expect((++entry2)->key == NULL, 0)) {
                                        entry2->key = entry->key;
                                        entry2->hash = entry->hash;
                                        if (__builtin_expect(entry++ > stop_point, 0)) {
                                            goto finalize_resize;
                                        }
                                        goto redo_moving1;
                                    }
                                }
                            }
                            idx = (idx*5 + (perturb >>= 5) + 1) & newmask;
                        }
                    }
                }
            }
            else {
                passed->fill = set_used;
                PyObject *key;
                for (entry = table; __builtin_expect(entry <= stop_point, 0); entry++) {
  redo_moving2:
                    if (__builtin_expect((key = entry->key) != NULL && key->ob_refcnt != 0, 1)) {
                        setentry *entry2;
                        perturb = hash;
                        idx = hash & newmask;
                        size_t j;
                        while (1) {
                            entry2 = &newtable[idx];
                            if (__builtin_expect(entry2->key == NULL, 0)) {
                                entry2->key = entry->key;
                                entry2->hash = entry->hash;
                                if (__builtin_expect(entry++ > stop_point, 0)) {
                                    goto finalize_resize;
                                }
                                goto redo_moving2;
                            }
                            if (__builtin_expect(idx + 9 <= newmask, 1)) {
                                for (j = 0; j < 9; j++) {
                                    if (__builtin_expect((++entry2)->key == NULL, 0)) {
                                        entry2->key = entry->key;
                                        entry2->hash = entry->hash;
                                        if (__builtin_expect(entry++ > stop_point, 0)) {
                                            goto finalize_resize;
                                        }
                                        goto redo_moving2;
                                    }
                                }
                            }
                            idx = (idx*5 + (perturb >>= 5) + 1) & newmask;
                        }
                    }
                }
            }
  finalize_resize:
            if (__builtin_expect(is_oldtable_malloced, 1)) {
                PyMem_Free(table);
            }
        }
  done_resize:
        i++;
    }
    Py_SET_SIZE(arg, length);
    return (PyObject *)passed;
}

static PyObject *
a_c_fast_min(PyObject *self,
             PyObject *const *args,
             Py_ssize_t nargs,
             PyObject *kwnames)
{
    PyObject **seq;
    Py_ssize_t seq_length;
    PyObject *item;
    PyObject *val;
    PyObject *maxitem = NULL;
    PyObject *maxval = NULL;
    PyObject *keyfunc = NULL;
    PyObject *defaultobj = NULL;
    Py_ssize_t i;
    if (__builtin_expect(kwnames == NULL, 1)) {
        goto no_kwargs_min;
    }
    PyObject **kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    Py_ssize_t nkwargs = Py_SIZE(kwnames);
    PyASCIIObject *keyitem;
    Py_ssize_t item_length;
    char *item_data;
    size_t required_items = 2 - (nargs != 1);
    for (i = 0; __builtin_expect(i < nkwargs, 0); i++) {
        if (__builtin_expect(
                (item_length = (keyitem = (PyASCIIObject *)kwnames_items[i])->length) == 3 &&
                (item_data = (char *)(keyitem + 1))[0] == 'k' && item_data[1] == 'e' && item_data[2] == 'y',
                1))
        {
            keyfunc = args[nargs + i];
            if (__builtin_expect(--required_items == 0, 1)) {
                break;
            }
        }
        else if (__builtin_expect(
                 item_length == 7 &&
                 item_data[0] == 'd' && item_data[1] == 'e' && item_data[2] == 'f' &&
                 item_data[3] == 'a' && item_data[4] == 'u' && item_data[5] == 'l' &&
                 item_data[6] == 't',
                 1))
        {
            defaultobj = args[nargs + i];
            if (__builtin_expect(--required_items == 0, 1)) {
                break;
            }
        }
    }
 no_kwargs_min:
    if (__builtin_expect(nargs != 1, 0)) {
        seq = (PyObject **)args;
        seq_length = nargs;
        goto fast_path_min_no_default;
    }
    PyTypeObject *tp;
    PyObject *it;
    PyObject *arg;
    unsigned long islist;
    if (__builtin_expect((islist = (tp = Py_TYPE(arg = args[0]))->tp_flags & Py_TPFLAGS_LIST_SUBCLASS) != 0 ||
        (tp->tp_flags & Py_TPFLAGS_TUPLE_SUBCLASS) != 0, 1))
    {
        seq = islist ? ((PyListObject *)arg)->ob_item : ((PyTupleObject *)arg)->ob_item;
        seq_length = Py_SIZE(arg);
        goto fast_path_min;
    }
    else {
        it = tp->tp_iter(arg);
    }
    PyObject *(*nextfunc)(PyObject *) = Py_TYPE(it)->tp_iternext;
    if (__builtin_expect(keyfunc != NULL, 0)) {
        while (__builtin_expect((item = nextfunc(it)) != NULL, 1)) {
            val = PyObject_CallOneArg(keyfunc, item); /* XX: some way we can optimize this */
            if (__builtin_expect(maxval == NULL, 0)) {
                maxitem = item;
                maxval = val;
            }
            else {
                if (__builtin_expect(PyObject_RichCompareBool(val, maxval, Py_LT), 0)) {
                    Py_DECREF(maxval);
                    Py_DECREF(maxitem);
                    maxval = val;
                    maxitem = item;
                }
                else {
                    Py_DECREF(item);
                    Py_DECREF(val);
                }
            }
        }
        PyErr_Clear();
        if (__builtin_expect(maxitem == NULL, 0)) {
            if (__builtin_expect(defaultobj != NULL, 1)) {
                Py_INCREF(defaultobj);
                return defaultobj;
            }
            Py_RETURN_NONE;
        }
        Py_INCREF(maxitem);
        return maxitem;
    }
    while (__builtin_expect((item = nextfunc(it)) != NULL, 1)) {
        if (__builtin_expect(maxitem == NULL, 0)) {
            maxitem = item;
        }
        else {
            if (__builtin_expect(PyObject_RichCompareBool(item, maxitem, Py_LT), 0)) {
                Py_DECREF(maxitem);
                maxitem = item;
            }
            else {
                Py_DECREF(item);
            }
        }
    }
    PyErr_Clear();
    if (__builtin_expect(maxitem == NULL, 0)) {
        if (__builtin_expect(defaultobj != NULL, 1)) {
            Py_INCREF(defaultobj);
            return defaultobj;
        }
        Py_RETURN_NONE;
    }
    Py_INCREF(maxitem);
    return maxitem;
  fast_path_min:
    if (__builtin_expect(keyfunc != NULL, 0)) {
        for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
            val = PyObject_CallOneArg(keyfunc, (item = seq[i])); /* XX: some way we can optimize this */
            if (__builtin_expect(maxval == NULL, 0)) {
                maxitem = item;
                maxval = val;
            }
            else {
                if (__builtin_expect(PyObject_RichCompareBool(val, maxval, Py_LT), 0)) {
                    Py_DECREF(maxval);
                    maxval = val;
                    maxitem = item;
                }
                else {
                    Py_DECREF(val);
                }
            }
        }
        if (__builtin_expect(maxitem == NULL, 0)) {
            if (__builtin_expect(defaultobj != NULL, 1)) {
                Py_INCREF(defaultobj);
                return defaultobj;
            }
            Py_RETURN_NONE;
        }
        Py_INCREF(maxitem);
        return maxitem;
    }
    for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
        if (__builtin_expect(maxitem == NULL, 0)) {
            maxitem = seq[i];
        }
        else if (__builtin_expect(PyObject_RichCompareBool((item = seq[i]), maxitem, Py_LT), 0)) {
            maxitem = item;
        }
    }
    if (__builtin_expect(maxitem == NULL, 0)) {
        if (__builtin_expect(defaultobj != NULL, 1)) {
            Py_INCREF(defaultobj);
            return defaultobj;
        }
        Py_RETURN_NONE;
    }
    Py_INCREF(maxitem);
    return maxitem;
  fast_path_min_no_default:
    if (__builtin_expect(keyfunc != NULL, 0)) {
        for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
            val = PyObject_CallOneArg(keyfunc, (item = seq[i])); /* XX: some way we can optimize this */
            if (__builtin_expect(maxval == NULL, 0)) {
                maxitem = item;
                maxval = val;
            }
            else {
                if (__builtin_expect(PyObject_RichCompareBool(val, maxval, Py_LT), 0)) {
                    Py_DECREF(maxval);
                    maxval = val;
                    maxitem = item;
                }
                else {
                    Py_DECREF(val);
                }
            }
        }
        if (__builtin_expect(maxitem == NULL, 0)) {
            Py_RETURN_NONE;
        }
        Py_INCREF(maxitem);
        return maxitem;
    }
    for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
        if (__builtin_expect(maxitem == NULL, 0)) {
            maxitem = seq[i];
        }
        else if (__builtin_expect(PyObject_RichCompareBool((item = seq[i]), maxitem, Py_LT), 0)) {
            maxitem = item;
        }
    }
    if (__builtin_expect(maxitem == NULL, 0)) {
        Py_RETURN_NONE;
    }
    Py_INCREF(maxitem);
    return maxitem;
}

static PyObject *
a_c_fast_max(PyObject *self,
             PyObject *const *args,
             Py_ssize_t nargs,
             PyObject *kwnames)
{
    PyObject **seq;
    Py_ssize_t seq_length;
    PyObject *item;
    PyObject *val;
    PyObject *maxitem = NULL;
    PyObject *maxval = NULL;
    PyObject *keyfunc = NULL;
    PyObject *defaultobj = NULL;
    Py_ssize_t i;
    if (__builtin_expect(kwnames == NULL, 1)) {
        goto no_kwargs_max;
    }
    PyObject **kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    Py_ssize_t nkwargs = Py_SIZE(kwnames);
    PyASCIIObject *keyitem;
    Py_ssize_t item_length;
    char *item_data;
    size_t required_items = 2 - (nargs != 1);
    for (i = 0; __builtin_expect(i < nkwargs, 0); i++) {
        if (__builtin_expect(
                memcmp(PyUnicode_AsUTF8(kwnames_items[i]), "key", 3) == 0,
                1))
        {
            keyfunc = args[nargs + i];
            if (__builtin_expect(--required_items == 0, 1)) {
                break;
            }
        }
        else if (__builtin_expect(
                 memcmp(PyUnicode_AsUTF8(kwnames_items[i]), "default", 7) == 0,
                 1))
        {
            defaultobj = args[nargs + i];
            if (__builtin_expect(--required_items == 0, 1)) {
                break;
            }
        }
    }
  no_kwargs_max:
    if (__builtin_expect(nargs != 1, 0)) {
        seq = (PyObject **)args;
        seq_length = nargs;
        goto fast_path_max_no_default;
    }
    PyTypeObject *tp;
    PyObject *it;
    PyObject *arg;
    unsigned long islist;
    if (__builtin_expect((islist = (tp = Py_TYPE(arg = args[0]))->tp_flags & Py_TPFLAGS_LIST_SUBCLASS) != 0 ||
        (tp->tp_flags & Py_TPFLAGS_TUPLE_SUBCLASS) != 0, 1))
    {
        seq = islist ? ((PyListObject *)arg)->ob_item : ((PyTupleObject *)arg)->ob_item;
        seq_length = Py_SIZE(arg);
        goto fast_path_max;
    }
    else {
        it = tp->tp_iter(arg);
    }
    PyObject *(*nextfunc)(PyObject *) = Py_TYPE(it)->tp_iternext;
    if (__builtin_expect(keyfunc != NULL, 0)) {
        while (__builtin_expect((item = nextfunc(it)) != NULL, 1)) {
            val = PyObject_CallOneArg(keyfunc, item); /* XX: some way we can optimize this */
            if (__builtin_expect(maxval == NULL, 0)) {
                maxitem = item;
                maxval = val;
            }
            else {
                if (__builtin_expect(PyObject_RichCompareBool(val, maxval, Py_GT), 0)) {
                    Py_DECREF(maxval);
                    Py_DECREF(maxitem);
                    maxval = val;
                    maxitem = item;
                }
                else {
                    Py_DECREF(item);
                    Py_DECREF(val);
                }
            }
        }
        PyErr_Clear();
        if (__builtin_expect(maxitem == NULL, 0)) {
            if (__builtin_expect(defaultobj != NULL, 1)) {
                Py_INCREF(defaultobj);
                return defaultobj;
            }
            Py_RETURN_NONE;
        }
        Py_INCREF(maxitem);
        return maxitem;
    }
    while (__builtin_expect((item = nextfunc(it)) != NULL, 1)) {
        if (__builtin_expect(maxitem == NULL, 0)) {
            maxitem = item;
        }
        else {
            if (__builtin_expect(PyObject_RichCompareBool(item, maxitem, Py_GT), 0)) {
                Py_DECREF(maxitem);
                maxitem = item;
            }
            else {
                Py_DECREF(item);
            }
        }
    }
    PyErr_Clear();
    if (__builtin_expect(maxitem == NULL, 0)) {
        if (__builtin_expect(defaultobj != NULL, 1)) {
            Py_INCREF(defaultobj);
            return defaultobj;
        }
        Py_RETURN_NONE;
    }
    Py_INCREF(maxitem);
    return maxitem;
  fast_path_max:
    if (__builtin_expect(keyfunc != NULL, 0)) {
        for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
            val = PyObject_CallOneArg(keyfunc, (item = seq[i])); /* XX: some way we can optimize this */
            if (__builtin_expect(maxval == NULL, 0)) {
                maxitem = item;
                maxval = val;
            }
            else {
                if (__builtin_expect(PyObject_RichCompareBool(val, maxval, Py_GT), 0)) {
                    Py_DECREF(maxval);
                    maxval = val;
                    maxitem = item;
                }
                else {
                    Py_DECREF(val);
                }
            }
        }
        if (__builtin_expect(maxitem == NULL, 0)) {
            if (__builtin_expect(defaultobj != NULL, 1)) {
                Py_INCREF(defaultobj);
                return defaultobj;
            }
            Py_RETURN_NONE;
        }
        Py_INCREF(maxitem);
        return maxitem;
    }
    for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
        if (__builtin_expect(maxitem == NULL, 0)) {
            maxitem = seq[i];
        }
        else if (__builtin_expect(PyObject_RichCompareBool((item = seq[i]), maxitem, Py_GT), 0)) {
            maxitem = item;
        }
    }
    if (__builtin_expect(maxitem == NULL, 0)) {
        if (__builtin_expect(defaultobj != NULL, 1)) {
            Py_INCREF(defaultobj);
            return defaultobj;
        }
        Py_RETURN_NONE;
    }
    Py_INCREF(maxitem);
    return maxitem;
  fast_path_max_no_default:
    if (__builtin_expect(keyfunc != NULL, 0)) {
        for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
            val = PyObject_CallOneArg(keyfunc, (item = seq[i])); /* XX: some way we can optimize this */
            if (__builtin_expect(maxval == NULL, 0)) {
                maxitem = item;
                maxval = val;
            }
            else {
                if (__builtin_expect(PyObject_RichCompareBool(val, maxval, Py_GT), 0)) {
                    Py_DECREF(maxval);
                    maxval = val;
                    maxitem = item;
                }
                else {
                    Py_DECREF(val);
                }
            }
        }
        if (__builtin_expect(maxitem == NULL, 0)) {
            Py_RETURN_NONE;
        }
        Py_INCREF(maxitem);
        return maxitem;
    }
    for (i = 0; __builtin_expect(i < seq_length, 1); i++) {
        if (__builtin_expect(maxitem == NULL, 0)) {
            maxitem = seq[i];
        }
        else if (__builtin_expect(PyObject_RichCompareBool((item = seq[i]), maxitem, Py_GT), 0)) {
            maxitem = item;
        }
    }
    if (__builtin_expect(maxitem == NULL, 0)) {
        Py_RETURN_NONE;
    }
    Py_INCREF(maxitem);
    return maxitem;
}
static inline void _print_bits(unsigned long long a__, size_t size_x, const char *x_string) {
    size_t bits__ = size_x * 8;
    printf("%s: ", x_string);
    while (bits__--) putchar(a__ &(1ULL << bits__) ? '1' : '0');
    putchar('\n');
}

#define print_bits(x) _print_bits((x), sizeof(x), #x)

static PyObject *
a_c_bitwise_in(PyObject *self,
               PyObject *const *args,
               Py_ssize_t nargs)
{
    PyObject *a;
    size_t real_size_a;
    size_t size_a = (real_size_a = (size_t)Py_SIZE(a = args[0])) - (size_t)1;
    if (__builtin_expect((Py_ssize_t)size_a <= (Py_ssize_t)-1, 0)) {
        Py_RETURN_TRUE;
    }
    PyObject *b;
    size_t size_b = (size_t)Py_SIZE(b = args[1]) - (size_t)1;
    if (__builtin_expect((Py_ssize_t)size_b <= (Py_ssize_t)-1, 0)) {
        Py_RETURN_FALSE;
    }
    digit *a_ddigits, *b_ddigits;
    if (__builtin_expect(size_a == size_b, 1)) {
        if (__builtin_expect(size_a == 0, 1)) {
            size_t a_num = ((PyLongObject *)a)->ob_digit[0];
            size_t b_num = ((PyLongObject *)b)->ob_digit[0];
            if (__builtin_expect(a_num > b_num, 0)) {
                Py_RETURN_FALSE;
            }
            size_t a_num_mask = ((size_t)1<<((size_t)35 - (size_t)__builtin_clz(a_num))) - 1;
            int r = ((b_num >>  0) & a_num_mask) == a_num;
            r |= ((b_num >>  1) & a_num_mask) == a_num;
            r |= ((b_num >>  2) & a_num_mask) == a_num;
            r |= ((b_num >>  3) & a_num_mask) == a_num;
            r |= ((b_num >>  4) & a_num_mask) == a_num;
            r |= ((b_num >>  5) & a_num_mask) == a_num;
            r |= ((b_num >>  6) & a_num_mask) == a_num;
            r |= ((b_num >>  7) & a_num_mask) == a_num;
            r |= ((b_num >>  8) & a_num_mask) == a_num;
            r |= ((b_num >>  9) & a_num_mask) == a_num;
            r |= ((b_num >> 10) & a_num_mask) == a_num;
            r |= ((b_num >> 11) & a_num_mask) == a_num;
            r |= ((b_num >> 12) & a_num_mask) == a_num;
            r |= ((b_num >> 13) & a_num_mask) == a_num;
            r |= ((b_num >> 14) & a_num_mask) == a_num;
            r |= ((b_num >> 15) & a_num_mask) == a_num;
            r |= ((b_num >> 16) & a_num_mask) == a_num;
            r |= ((b_num >> 17) & a_num_mask) == a_num;
            r |= ((b_num >> 18) & a_num_mask) == a_num;
            r |= ((b_num >> 19) & a_num_mask) == a_num;
            r |= ((b_num >> 20) & a_num_mask) == a_num;
            r |= ((b_num >> 21) & a_num_mask) == a_num;
            r |= ((b_num >> 22) & a_num_mask) == a_num;
            r |= ((b_num >> 23) & a_num_mask) == a_num;
            r |= ((b_num >> 24) & a_num_mask) == a_num;
            r |= ((b_num >> 25) & a_num_mask) == a_num;
            r |= ((b_num >> 26) & a_num_mask) == a_num;
            r |= ((b_num >> 27) & a_num_mask) == a_num;
            r |= ((b_num >> 28) & a_num_mask) == a_num;
            r |= ((b_num >> 29) & a_num_mask) == a_num;
            r |= ((b_num >> 30) & a_num_mask) == a_num;
            r |= ((b_num >> 31) & a_num_mask) == a_num;
            if (r == 0) {
                Py_RETURN_FALSE;
            }
            Py_RETURN_TRUE;
        }
        else if (__builtin_expect(size_a == 1, 1)) {
            a_ddigits = ((PyLongObject *)a)->ob_digit;
            b_ddigits = ((PyLongObject *)b)->ob_digit;
            size_t a_num = (size_t)a_ddigits[0] | (a_ddigits[1] << 30);
            size_t b_num = (size_t)b_ddigits[0] | (b_ddigits[1] << 30);
            if (__builtin_expect(a_num > b_num, 0)) {
                Py_RETURN_FALSE;
            }
            size_t a_num_mask = ((size_t)1<<((size_t)65 - (size_t)__builtin_clzll(a_num))) - 1;
            int r = ((b_num >>  0) & a_num_mask) == a_num;
            r |= ((b_num >>  1) & a_num_mask) == a_num;
            r |= ((b_num >>  2) & a_num_mask) == a_num;
            r |= ((b_num >>  3) & a_num_mask) == a_num;
            r |= ((b_num >>  4) & a_num_mask) == a_num;
            r |= ((b_num >>  5) & a_num_mask) == a_num;
            r |= ((b_num >>  6) & a_num_mask) == a_num;
            r |= ((b_num >>  7) & a_num_mask) == a_num;
            r |= ((b_num >>  8) & a_num_mask) == a_num;
            r |= ((b_num >>  9) & a_num_mask) == a_num;
            r |= ((b_num >> 10) & a_num_mask) == a_num;
            r |= ((b_num >> 11) & a_num_mask) == a_num;
            r |= ((b_num >> 12) & a_num_mask) == a_num;
            r |= ((b_num >> 13) & a_num_mask) == a_num;
            r |= ((b_num >> 14) & a_num_mask) == a_num;
            r |= ((b_num >> 15) & a_num_mask) == a_num;
            r |= ((b_num >> 16) & a_num_mask) == a_num;
            r |= ((b_num >> 17) & a_num_mask) == a_num;
            r |= ((b_num >> 18) & a_num_mask) == a_num;
            r |= ((b_num >> 19) & a_num_mask) == a_num;
            r |= ((b_num >> 20) & a_num_mask) == a_num;
            r |= ((b_num >> 21) & a_num_mask) == a_num;
            r |= ((b_num >> 22) & a_num_mask) == a_num;
            r |= ((b_num >> 23) & a_num_mask) == a_num;
            r |= ((b_num >> 24) & a_num_mask) == a_num;
            r |= ((b_num >> 25) & a_num_mask) == a_num;
            r |= ((b_num >> 26) & a_num_mask) == a_num;
            r |= ((b_num >> 27) & a_num_mask) == a_num;
            r |= ((b_num >> 28) & a_num_mask) == a_num;
            r |= ((b_num >> 29) & a_num_mask) == a_num;
            r |= ((b_num >> 30) & a_num_mask) == a_num;
            r |= ((b_num >> 31) & a_num_mask) == a_num;
            r |= ((b_num >> 32) & a_num_mask) == a_num;
            r |= ((b_num >> 33) & a_num_mask) == a_num;
            r |= ((b_num >> 34) & a_num_mask) == a_num;
            r |= ((b_num >> 35) & a_num_mask) == a_num;
            r |= ((b_num >> 36) & a_num_mask) == a_num;
            r |= ((b_num >> 37) & a_num_mask) == a_num;
            r |= ((b_num >> 38) & a_num_mask) == a_num;
            r |= ((b_num >> 39) & a_num_mask) == a_num;
            r |= ((b_num >> 40) & a_num_mask) == a_num;
            r |= ((b_num >> 41) & a_num_mask) == a_num;
            r |= ((b_num >> 42) & a_num_mask) == a_num;
            r |= ((b_num >> 43) & a_num_mask) == a_num;
            r |= ((b_num >> 44) & a_num_mask) == a_num;
            r |= ((b_num >> 45) & a_num_mask) == a_num;
            r |= ((b_num >> 46) & a_num_mask) == a_num;
            r |= ((b_num >> 47) & a_num_mask) == a_num;
            r |= ((b_num >> 48) & a_num_mask) == a_num;
            r |= ((b_num >> 49) & a_num_mask) == a_num;
            r |= ((b_num >> 50) & a_num_mask) == a_num;
            r |= ((b_num >> 51) & a_num_mask) == a_num;
            r |= ((b_num >> 52) & a_num_mask) == a_num;
            r |= ((b_num >> 53) & a_num_mask) == a_num;
            r |= ((b_num >> 54) & a_num_mask) == a_num;
            r |= ((b_num >> 55) & a_num_mask) == a_num;
            r |= ((b_num >> 56) & a_num_mask) == a_num;
            r |= ((b_num >> 57) & a_num_mask) == a_num;
            r |= ((b_num >> 58) & a_num_mask) == a_num;
            r |= ((b_num >> 59) & a_num_mask) == a_num;
            r |= ((b_num >> 60) & a_num_mask) == a_num;
            r |= ((b_num >> 61) & a_num_mask) == a_num;
            r |= ((b_num >> 62) & a_num_mask) == a_num;
            r |= ((b_num >> 63) & a_num_mask) == a_num;
            if (r == 0) {
                Py_RETURN_FALSE;
            }
            Py_RETURN_TRUE;
        }
    }
    size_t a_rem_length;
    a_ddigits = ((PyLongObject *)a)->ob_digit;
    size_t a_length = size_a * 30 + (a_rem_length = (size_t)32 - (size_t)__builtin_clz(a_ddigits[size_a]));
    b_ddigits = ((PyLongObject *)b)->ob_digit;
    size_t b_length = size_b * 30 + ((size_t)32 - (size_t)__builtin_clz(b_ddigits[size_b]));
    if (__builtin_expect(b_length < a_length, 0)) {
        Py_RETURN_FALSE;
    }
    size_t a_bytelength = (a_length >> 3) + ((a_length & 7) != 0);
    size_t size_window;
    digit *window = calloc((size_window = (a_length >> 5) + ((a_length & 31) != 0)), sizeof(digit));
    if (__builtin_expect(window == NULL, 0)) {
        PyErr_NoMemory();
        return NULL;
    }
    size_t last_window_index = size_window - 1;
    size_t lshift_amount = 30;
    size_t difference = 0;
    for (size_t i = 0; __builtin_expect(i < size_a, 1); i++) {
        if (__builtin_expect(lshift_amount == 0, 0)) {
            lshift_amount = 30;
            ++difference;
            continue;
        }
        window[i - difference] = ((b_ddigits[i] >> (30 - lshift_amount)) & 0x3fffffff) | ((b_ddigits[i + 1] & (1 << (32 - lshift_amount)) - 1) << lshift_amount);
        lshift_amount -= 2;
    }
    a_rem_length -= 30 - lshift_amount;
    digit a_rem_mask = (1 << (32 - __builtin_clz((digit)a_rem_length))) - 1;
    size_t bit_length, bit_index;
    if (__builtin_expect(lshift_amount != 0, 1)) {
        if (__builtin_expect(size_b > size_a, 0)) {
            window[size_a - difference] = ((b_ddigits[size_a] >> (30 - lshift_amount)) & 0x3fffffff)
                                           | ((b_ddigits[size_a + 1] & (1 << (32 - lshift_amount)) - 1) << lshift_amount);
            bit_length = ((size_t)difference * 32) + (bit_index = lshift_amount);
        }
        else {
            window[size_a - difference] = ((b_ddigits[size_a] >> (30 - lshift_amount)) & 0x3fffffff);
            bit_length = ((size_t)difference * 32) + (bit_index = lshift_amount);
        }
    }
    else {
        bit_length = (size_t)difference * 32;
        bit_index = 0;
    }
    while (bit_length <= b_length) {
        lshift_amount = 30;
        difference = 0;
        for (size_t i = 0; __builtin_expect(i < size_a, 1); i++) {
            if (__builtin_expect(lshift_amount == 0, 0)) {
                lshift_amount = 30;
                ++difference;
                continue;
            }
            if (__builtin_expect(window[i - difference] != (((a_ddigits[i] >> (30 - lshift_amount)) & 0x3fffffff)
                                                            | ((a_ddigits[i + 1] & (1 << (32 - lshift_amount)) - 1) << lshift_amount)), 1)) {
                goto comparison_false;
            }
            lshift_amount -= 2;
        }
        if (__builtin_expect(lshift_amount != 0, 1)) {
            if (__builtin_expect((window[size_a - difference] & a_rem_mask) != ((a_ddigits[size_a] >> (30 - lshift_amount)) & 0x3fffffff), 1)) {
                goto comparison_false;
            }
        }
        free(window);
        Py_RETURN_TRUE;
  comparison_false:
        for (size_t i = 0; i < last_window_index; ++i) {
            window[i] = (window[i] >> 1) | ((window[i+1] & 1) << 31);
        }
        if (__builtin_expect(bit_index == 30, 0)) {
            bit_index = 0;
            ++b_ddigits;
        }
        window[last_window_index] = (window[last_window_index] >> 1) | (((*b_ddigits & (1 << bit_index++)) != 0) << 31);
        ++bit_length;
    }
    free(window);
    Py_RETURN_FALSE;
}

static binaryfunc long_mod;

static Py_ssize_t
long_compare(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t sign = Py_SIZE(a) - Py_SIZE(b);
    if (__builtin_expect(sign == 0, 1)) {
        Py_ssize_t i = Py_ABS(Py_SIZE(a));
        sdigit diff = 0;
        while (i--) {
            diff = (sdigit) a->ob_digit[i] - (sdigit) b->ob_digit[i];
            if (diff) {
                break;
            }
        }
        sign = Py_SIZE(a) < 0 ? -diff : diff;
    }
    return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

/* Is this PyLong of size 1, 0 or -1? */
#define IS_MEDIUM_VALUE(x) (((size_t)Py_SIZE(x)) + 1U < 3U)

/* convert a PyLong of size 1, 0 or -1 to a C integer */
static inline stwodigits
medium_value(PyLongObject *x)
{
    assert(IS_MEDIUM_VALUE(x));
    return ((stwodigits)Py_SIZE(x)) * x->ob_digit[0];
}

#define IS_SMALL_INT(ival) (-_PY_NSMALLNEGINTS <= (ival) && (ival) < _PY_NSMALLPOSINTS)
#define IS_SMALL_UINT(ival) ((ival) < _PY_NSMALLPOSINTS)

static inline void
_Py_DECREF_INT(PyLongObject *op)
{
    assert(PyLong_CheckExact(op));
    _Py_DECREF_SPECIALIZED((PyObject *)op, (destructor)PyObject_Free);
}

static inline int
is_medium_int(stwodigits x)
{
    /* Take care that we are comparing unsigned values. */
    twodigits x_plus_mask = ((twodigits)x) + PyLong_MASK;
    return x_plus_mask < ((twodigits)PyLong_MASK) + PyLong_BASE;
}

static PyObject *
get_small_int(sdigit ival)
{
    assert(IS_SMALL_INT(ival));
    PyObject *v = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + ival];
    Py_INCREF(v);
    return v;
}

static PyLongObject *
maybe_small_long(PyLongObject *v)
{
    if (v && IS_MEDIUM_VALUE(v)) {
        stwodigits ival = medium_value(v);
        if (IS_SMALL_INT(ival)) {
            _Py_DECREF_INT(v);
            return (PyLongObject *)get_small_int((sdigit)ival);
        }
    }
    return v;
}

/* For int multiplication, use the O(N**2) school algorithm unless
 * both operands contain more than KARATSUBA_CUTOFF digits (this
 * being an internal Python int digit, in base BASE).
 */
#define KARATSUBA_CUTOFF 70
#define KARATSUBA_SQUARE_CUTOFF (2 * KARATSUBA_CUTOFF)

/* For exponentiation, use the binary left-to-right algorithm unless the
 ^ exponent contains more than HUGE_EXP_CUTOFF bits.  In that case, do
 * (no more than) EXP_WINDOW_SIZE bits at a time.  The potential drawback is
 * that a table of 2**(EXP_WINDOW_SIZE - 1) intermediate results is
 * precomputed.
 */
#define EXP_WINDOW_SIZE 5
#define EXP_TABLE_LEN (1 << (EXP_WINDOW_SIZE - 1))
/* Suppose the exponent has bit length e. All ways of doing this
 * need e squarings. The binary method also needs a multiply for
 * each bit set. In a k-ary method with window width w, a multiply
 * for each non-zero window, so at worst (and likely!)
 * ceiling(e/w). The k-ary sliding window method has the same
 * worst case, but the window slides so it can sometimes skip
 * over an all-zero window that the fixed-window method can't
 * exploit. In addition, the windowing methods need multiplies
 * to precompute a table of small powers.
 *
 * For the sliding window method with width 5, 16 precomputation
 * multiplies are needed. Assuming about half the exponent bits
 * are set, then, the binary method needs about e/2 extra mults
 * and the window method about 16 + e/5.
 *
 * The latter is smaller for e > 53 1/3. We don't have direct
 * access to the bit length, though, so call it 60, which is a
 * multiple of a long digit's max bit length (15 or 30 so far).
 */
#define HUGE_EXP_CUTOFF 60

#define SIGCHECK(PyTryBlock)                    \
    do {                                        \
        if (PyErr_CheckSignals()) PyTryBlock    \
    } while(0)

/* Normalize (remove leading zeros from) an int object.
   Doesn't attempt to free the storage--in most cases, due to the nature
   of the algorithms used, this could save at most be one word anyway. */

static PyLongObject *
long_normalize(PyLongObject *v)
{
    Py_ssize_t j = Py_ABS(Py_SIZE(v));
    Py_ssize_t i = j;

    while (i > 0 && v->ob_digit[i-1] == 0)
        --i;
    if (i != j) {
        Py_SET_SIZE(v, (Py_SIZE(v) < 0) ? -(i) : i);
    }
    return v;
}

static PyObject *
_PyLong_FromMedium(sdigit x)
{
    assert(!IS_SMALL_INT(x));
    assert(is_medium_int(x));
    /* We could use a freelist here */
    PyLongObject *v = PyObject_Malloc(sizeof(PyLongObject));
    if (v == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    Py_ssize_t sign = x < 0 ? -1: 1;
    digit abs_x = x < 0 ? -x : x;
    _PyObject_InitVar((PyVarObject*)v, &PyLong_Type, sign);
    v->ob_digit[0] = abs_x;
    return (PyObject*)v;
}

static PyObject *
_PyLong_FromLarge(stwodigits ival)
{
    twodigits abs_ival;
    int sign;
    assert(!is_medium_int(ival));

    if (ival < 0) {
        /* negate: can't write this as abs_ival = -ival since that
           invokes undefined behaviour when ival is LONG_MIN */
        abs_ival = 0U-(twodigits)ival;
        sign = -1;
    }
    else {
        abs_ival = (twodigits)ival;
        sign = 1;
    }
    /* Must be at least two digits */
    assert(abs_ival >> PyLong_SHIFT != 0);
    twodigits t = abs_ival >> (PyLong_SHIFT * 2);
    Py_ssize_t ndigits = 2;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    PyLongObject *v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        Py_SET_SIZE(v, ndigits * sign);
        t = abs_ival;
        while (t) {
            *p++ = Py_SAFE_DOWNCAST(
                t & PyLong_MASK, twodigits, digit);
            t >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Create a new int object from a C word-sized int */
static inline PyObject *
_PyLong_FromSTwoDigits(stwodigits x)
{
    if (IS_SMALL_INT(x)) {
        return get_small_int((sdigit)x);
    }
    assert(x != 0);
    if (is_medium_int(x)) {
        return _PyLong_FromMedium((sdigit)x);
    }
    return _PyLong_FromLarge(x);
}

/* If a freshly-allocated int is already shared, it must
   be a small integer, so negating it must go to PyLong_FromLong */
Py_LOCAL_INLINE(void)
_PyLong_Negate(PyLongObject **x_p)
{
    PyLongObject *x;

    x = (PyLongObject *)*x_p;
    if (Py_REFCNT(x) == 1) {
        Py_SET_SIZE(x, -Py_SIZE(x));
        return;
    }

    *x_p = (PyLongObject *)_PyLong_FromSTwoDigits(-medium_value(x));
    Py_DECREF(x);
}

/* x[0:m] and y[0:n] are digit vectors, LSD first, m >= n required.  x[0:n]
 * is modified in place, by adding y to it.  Carries are propagated as far as
 * x[m-1], and the remaining carry (0 or 1) is returned.
 */
static digit
v_iadd(digit *x, Py_ssize_t m, digit *y, Py_ssize_t n)
{
    Py_ssize_t i;
    digit carry = 0;

    assert(m >= n);
    for (i = 0; i < n; ++i) {
        carry += x[i] + y[i];
        x[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
        assert((carry & 1) == carry);
    }
    for (; carry && i < m; ++i) {
        carry += x[i];
        x[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
        assert((carry & 1) == carry);
    }
    return carry;
}

/* x[0:m] and y[0:n] are digit vectors, LSD first, m >= n required.  x[0:n]
 * is modified in place, by subtracting y from it.  Borrows are propagated as
 * far as x[m-1], and the remaining borrow (0 or 1) is returned.
 */
static digit
v_isub(digit *x, Py_ssize_t m, digit *y, Py_ssize_t n)
{
    Py_ssize_t i;
    digit borrow = 0;

    assert(m >= n);
    for (i = 0; i < n; ++i) {
        borrow = x[i] - y[i] - borrow;
        x[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1;            /* keep only 1 sign bit */
    }
    for (; borrow && i < m; ++i) {
        borrow = x[i] - borrow;
        x[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1;
    }
    return borrow;
}

/* Add the absolute values of two integers. */

static PyLongObject *
x_add(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a)), size_b = Py_ABS(Py_SIZE(b));
    PyLongObject *z;
    Py_ssize_t i;
    digit carry = 0;

    /* Ensure a is the larger of the two: */
    if (size_a < size_b) {
        { PyLongObject *temp = a; a = b; b = temp; }
        { Py_ssize_t size_temp = size_a;
            size_a = size_b;
            size_b = size_temp; }
    }
    z = _PyLong_New(size_a+1);
    if (z == NULL)
        return NULL;
    for (i = 0; i < size_b; ++i) {
        carry += a->ob_digit[i] + b->ob_digit[i];
        z->ob_digit[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
    }
    for (; i < size_a; ++i) {
        carry += a->ob_digit[i];
        z->ob_digit[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
    }
    z->ob_digit[i] = carry;
    return long_normalize(z);
}

/* Subtract the absolute values of two integers. */

static PyLongObject *
x_sub(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a)), size_b = Py_ABS(Py_SIZE(b));
    PyLongObject *z;
    Py_ssize_t i;
    int sign = 1;
    digit borrow = 0;

    /* Ensure a is the larger of the two: */
    if (size_a < size_b) {
        sign = -1;
        { PyLongObject *temp = a; a = b; b = temp; }
        { Py_ssize_t size_temp = size_a;
            size_a = size_b;
            size_b = size_temp; }
    }
    else if (size_a == size_b) {
        /* Find highest digit where a and b differ: */
        i = size_a;
        while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
            ;
        if (i < 0)
            return (PyLongObject *)PyLong_FromLong(0);
        if (a->ob_digit[i] < b->ob_digit[i]) {
            sign = -1;
            { PyLongObject *temp = a; a = b; b = temp; }
        }
        size_a = size_b = i+1;
    }
    z = _PyLong_New(size_a);
    if (z == NULL)
        return NULL;
    for (i = 0; i < size_b; ++i) {
        /* The following assumes unsigned arithmetic
           works module 2**N for some N>PyLong_SHIFT. */
        borrow = a->ob_digit[i] - b->ob_digit[i] - borrow;
        z->ob_digit[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1; /* Keep only one sign bit */
    }
    for (; i < size_a; ++i) {
        borrow = a->ob_digit[i] - borrow;
        z->ob_digit[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1; /* Keep only one sign bit */
    }
    assert(borrow == 0);
    if (sign < 0) {
        Py_SET_SIZE(z, -Py_SIZE(z));
    }
    return maybe_small_long(long_normalize(z));
}

PyObject *
_PyLong_Add(PyLongObject *a, PyLongObject *b)
{
    if (IS_MEDIUM_VALUE(a) && IS_MEDIUM_VALUE(b)) {
        return _PyLong_FromSTwoDigits(medium_value(a) + medium_value(b));
    }

    PyLongObject *z;
    if (Py_SIZE(a) < 0) {
        if (Py_SIZE(b) < 0) {
            z = x_add(a, b);
            if (z != NULL) {
                /* x_add received at least one multiple-digit int,
                   and thus z must be a multiple-digit int.
                   That also means z is not an element of
                   small_ints, so negating it in-place is safe. */
                assert(Py_REFCNT(z) == 1);
                Py_SET_SIZE(z, -(Py_SIZE(z)));
            }
        }
        else
            z = x_sub(b, a);
    }
    else {
        if (Py_SIZE(b) < 0)
            z = x_sub(a, b);
        else
            z = x_add(a, b);
    }
    return (PyObject *)z;
}

PyObject *
_PyLong_Subtract(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;

    if (IS_MEDIUM_VALUE(a) && IS_MEDIUM_VALUE(b)) {
        return _PyLong_FromSTwoDigits(medium_value(a) - medium_value(b));
    }
    if (Py_SIZE(a) < 0) {
        if (Py_SIZE(b) < 0) {
            z = x_sub(b, a);
        }
        else {
            z = x_add(a, b);
            if (z != NULL) {
                assert(Py_SIZE(z) == 0 || Py_REFCNT(z) == 1);
                Py_SET_SIZE(z, -(Py_SIZE(z)));
            }
        }
    }
    else {
        if (Py_SIZE(b) < 0)
            z = x_add(a, b);
        else
            z = x_sub(a, b);
    }
    return (PyObject *)z;
}

/* Grade school multiplication, ignoring the signs.
 * Returns the absolute value of the product, or NULL if error.
 */
static PyLongObject *
x_mul(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a));
    Py_ssize_t size_b = Py_ABS(Py_SIZE(b));
    Py_ssize_t i;

    z = _PyLong_New(size_a + size_b);
    if (z == NULL)
        return NULL;

    memset(z->ob_digit, 0, Py_SIZE(z) * sizeof(digit));
    if (a == b) {
        /* Efficient squaring per HAC, Algorithm 14.16:
         * http://www.cacr.math.uwaterloo.ca/hac/about/chap14.pdf
         * Gives slightly less than a 2x speedup when a == b,
         * via exploiting that each entry in the multiplication
         * pyramid appears twice (except for the size_a squares).
         */
        digit *paend = a->ob_digit + size_a;
        for (i = 0; i < size_a; ++i) {
            twodigits carry;
            twodigits f = a->ob_digit[i];
            digit *pz = z->ob_digit + (i << 1);
            digit *pa = a->ob_digit + i + 1;

            SIGCHECK({
                    Py_DECREF(z);
                    return NULL;
                });

            carry = *pz + f * f;
            *pz++ = (digit)(carry & PyLong_MASK);
            carry >>= PyLong_SHIFT;
            assert(carry <= PyLong_MASK);

            /* Now f is added in twice in each column of the
             * pyramid it appears.  Same as adding f<<1 once.
             */
            f <<= 1;
            while (pa < paend) {
                carry += *pz + *pa++ * f;
                *pz++ = (digit)(carry & PyLong_MASK);
                carry >>= PyLong_SHIFT;
                assert(carry <= (PyLong_MASK << 1));
            }
            if (carry) {
                /* See comment below. pz points at the highest possible
                 * carry position from the last outer loop iteration, so
                 * *pz is at most 1.
                 */
                assert(*pz <= 1);
                carry += *pz;
                *pz = (digit)(carry & PyLong_MASK);
                carry >>= PyLong_SHIFT;
                if (carry) {
                    /* If there's still a carry, it must be into a position
                     * that still holds a 0. Where the base
                     ^ B is 1 << PyLong_SHIFT, the last add was of a carry no
                     * more than 2*B - 2 to a stored digit no more than 1.
                     * So the sum was no more than 2*B - 1, so the current
                     * carry no more than floor((2*B - 1)/B) = 1.
                     */
                    assert(carry == 1);
                    assert(pz[1] == 0);
                    pz[1] = (digit)carry;
                }
            }
        }
    }
    else {      /* a is not the same as b -- gradeschool int mult */
        for (i = 0; i < size_a; ++i) {
            twodigits carry = 0;
            twodigits f = a->ob_digit[i];
            digit *pz = z->ob_digit + i;
            digit *pb = b->ob_digit;
            digit *pbend = b->ob_digit + size_b;

            SIGCHECK({
                    Py_DECREF(z);
                    return NULL;
                });

            while (pb < pbend) {
                carry += *pz + *pb++ * f;
                *pz++ = (digit)(carry & PyLong_MASK);
                carry >>= PyLong_SHIFT;
                assert(carry <= PyLong_MASK);
            }
            if (carry)
                *pz += (digit)(carry & PyLong_MASK);
            assert((carry >> PyLong_SHIFT) == 0);
        }
    }
    return long_normalize(z);
}

/* A helper for Karatsuba multiplication (k_mul).
   Takes an int "n" and an integer "size" representing the place to
   split, and sets low and high such that abs(n) == (high << size) + low,
   viewing the shift as being by digits.  The sign bit is ignored, and
   the return values are >= 0.
   Returns 0 on success, -1 on failure.
*/
static int
kmul_split(PyLongObject *n,
           Py_ssize_t size,
           PyLongObject **high,
           PyLongObject **low)
{
    PyLongObject *hi, *lo;
    Py_ssize_t size_lo, size_hi;
    const Py_ssize_t size_n = Py_ABS(Py_SIZE(n));

    size_lo = Py_MIN(size_n, size);
    size_hi = size_n - size_lo;

    if ((hi = _PyLong_New(size_hi)) == NULL)
        return -1;
    if ((lo = _PyLong_New(size_lo)) == NULL) {
        Py_DECREF(hi);
        return -1;
    }

    memcpy(lo->ob_digit, n->ob_digit, size_lo * sizeof(digit));
    memcpy(hi->ob_digit, n->ob_digit + size_lo, size_hi * sizeof(digit));

    *high = long_normalize(hi);
    *low = long_normalize(lo);
    return 0;
}

static PyLongObject *k_lopsided_mul(PyLongObject *a, PyLongObject *b);

/* Karatsuba multiplication.  Ignores the input signs, and returns the
 * absolute value of the product (or NULL if error).
 * See Knuth Vol. 2 Chapter 4.3.3 (Pp. 294-295).
 */
static PyLongObject *
k_mul(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t asize = Py_ABS(Py_SIZE(a));
    Py_ssize_t bsize = Py_ABS(Py_SIZE(b));
    PyLongObject *ah = NULL;
    PyLongObject *al = NULL;
    PyLongObject *bh = NULL;
    PyLongObject *bl = NULL;
    PyLongObject *ret = NULL;
    PyLongObject *t1, *t2, *t3;
    Py_ssize_t shift;           /* the number of digits we split off */
    Py_ssize_t i;

    /* (ah*X+al)(bh*X+bl) = ah*bh*X*X + (ah*bl + al*bh)*X + al*bl
     * Let k = (ah+al)*(bh+bl) = ah*bl + al*bh  + ah*bh + al*bl
     * Then the original product is
     *     ah*bh*X*X + (k - ah*bh - al*bl)*X + al*bl
     * By picking X to be a power of 2, "*X" is just shifting, and it's
     * been reduced to 3 multiplies on numbers half the size.
     */

    /* We want to split based on the larger number; fiddle so that b
     * is largest.
     */
    if (asize > bsize) {
        t1 = a;
        a = b;
        b = t1;

        i = asize;
        asize = bsize;
        bsize = i;
    }

    /* Use gradeschool math when either number is too small. */
    i = a == b ? KARATSUBA_SQUARE_CUTOFF : KARATSUBA_CUTOFF;
    if (asize <= i) {
        if (asize == 0)
            return (PyLongObject *)PyLong_FromLong(0);
        else
            return x_mul(a, b);
    }

    /* If a is small compared to b, splitting on b gives a degenerate
     * case with ah==0, and Karatsuba may be (even much) less efficient
     * than "grade school" then.  However, we can still win, by viewing
     * b as a string of "big digits", each of width a->ob_size.  That
     * leads to a sequence of balanced calls to k_mul.
     */
    if (2 * asize <= bsize)
        return k_lopsided_mul(a, b);

    /* Split a & b into hi & lo pieces. */
    shift = bsize >> 1;
    if (kmul_split(a, shift, &ah, &al) < 0) goto fail;
    assert(Py_SIZE(ah) > 0);            /* the split isn't degenerate */

    if (a == b) {
        bh = ah;
        bl = al;
        Py_INCREF(bh);
        Py_INCREF(bl);
    }
    else if (kmul_split(b, shift, &bh, &bl) < 0) goto fail;

    /* The plan:
     * 1. Allocate result space (asize + bsize digits:  that's always
     *    enough).
     * 2. Compute ah*bh, and copy into result at 2*shift.
     * 3. Compute al*bl, and copy into result at 0.  Note that this
     *    can't overlap with #2.
     * 4. Subtract al*bl from the result, starting at shift.  This may
     *    underflow (borrow out of the high digit), but we don't care:
     *    we're effectively doing unsigned arithmetic mod
     *    BASE**(sizea + sizeb), and so long as the *final* result fits,
     *    borrows and carries out of the high digit can be ignored.
     * 5. Subtract ah*bh from the result, starting at shift.
     * 6. Compute (ah+al)*(bh+bl), and add it into the result starting
     *    at shift.
     */

    /* 1. Allocate result space. */
    ret = _PyLong_New(asize + bsize);
    if (ret == NULL) goto fail;
#ifdef Py_DEBUG
    /* Fill with trash, to catch reference to uninitialized digits. */
    memset(ret->ob_digit, 0xDF, Py_SIZE(ret) * sizeof(digit));
#endif

    /* 2. t1 <- ah*bh, and copy into high digits of result. */
    if ((t1 = k_mul(ah, bh)) == NULL) goto fail;
    assert(Py_SIZE(t1) >= 0);
    assert(2*shift + Py_SIZE(t1) <= Py_SIZE(ret));
    memcpy(ret->ob_digit + 2*shift, t1->ob_digit,
           Py_SIZE(t1) * sizeof(digit));

    /* Zero-out the digits higher than the ah*bh copy. */
    i = Py_SIZE(ret) - 2*shift - Py_SIZE(t1);
    if (i)
        memset(ret->ob_digit + 2*shift + Py_SIZE(t1), 0,
               i * sizeof(digit));

    /* 3. t2 <- al*bl, and copy into the low digits. */
    if ((t2 = k_mul(al, bl)) == NULL) {
        Py_DECREF(t1);
        goto fail;
    }
    assert(Py_SIZE(t2) >= 0);
    assert(Py_SIZE(t2) <= 2*shift); /* no overlap with high digits */
    memcpy(ret->ob_digit, t2->ob_digit, Py_SIZE(t2) * sizeof(digit));

    /* Zero out remaining digits. */
    i = 2*shift - Py_SIZE(t2);          /* number of uninitialized digits */
    if (i)
        memset(ret->ob_digit + Py_SIZE(t2), 0, i * sizeof(digit));

    /* 4 & 5. Subtract ah*bh (t1) and al*bl (t2).  We do al*bl first
     * because it's fresher in cache.
     */
    i = Py_SIZE(ret) - shift;  /* # digits after shift */
    (void)v_isub(ret->ob_digit + shift, i, t2->ob_digit, Py_SIZE(t2));
    _Py_DECREF_INT(t2);

    (void)v_isub(ret->ob_digit + shift, i, t1->ob_digit, Py_SIZE(t1));
    _Py_DECREF_INT(t1);

    /* 6. t3 <- (ah+al)(bh+bl), and add into result. */
    if ((t1 = x_add(ah, al)) == NULL) goto fail;
    _Py_DECREF_INT(ah);
    _Py_DECREF_INT(al);
    ah = al = NULL;

    if (a == b) {
        t2 = t1;
        Py_INCREF(t2);
    }
    else if ((t2 = x_add(bh, bl)) == NULL) {
        Py_DECREF(t1);
        goto fail;
    }
    _Py_DECREF_INT(bh);
    _Py_DECREF_INT(bl);
    bh = bl = NULL;

    t3 = k_mul(t1, t2);
    _Py_DECREF_INT(t1);
    _Py_DECREF_INT(t2);
    if (t3 == NULL) goto fail;
    assert(Py_SIZE(t3) >= 0);

    /* Add t3.  It's not obvious why we can't run out of room here.
     * See the (*) comment after this function.
     */
    (void)v_iadd(ret->ob_digit + shift, i, t3->ob_digit, Py_SIZE(t3));
    _Py_DECREF_INT(t3);

    return long_normalize(ret);

  fail:
    Py_XDECREF(ret);
    Py_XDECREF(ah);
    Py_XDECREF(al);
    Py_XDECREF(bh);
    Py_XDECREF(bl);
    return NULL;
}

/* (*) Why adding t3 can't "run out of room" above.
Let f(x) mean the floor of x and c(x) mean the ceiling of x.  Some facts
to start with:
1. For any integer i, i = c(i/2) + f(i/2).  In particular,
   bsize = c(bsize/2) + f(bsize/2).
2. shift = f(bsize/2)
3. asize <= bsize
4. Since we call k_lopsided_mul if asize*2 <= bsize, asize*2 > bsize in this
   routine, so asize > bsize/2 >= f(bsize/2) in this routine.
We allocated asize + bsize result digits, and add t3 into them at an offset
of shift.  This leaves asize+bsize-shift allocated digit positions for t3
to fit into, = (by #1 and #2) asize + f(bsize/2) + c(bsize/2) - f(bsize/2) =
asize + c(bsize/2) available digit positions.
bh has c(bsize/2) digits, and bl at most f(size/2) digits.  So bh+hl has
at most c(bsize/2) digits + 1 bit.
If asize == bsize, ah has c(bsize/2) digits, else ah has at most f(bsize/2)
digits, and al has at most f(bsize/2) digits in any case.  So ah+al has at
most (asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 1 bit.
The product (ah+al)*(bh+bl) therefore has at most
    c(bsize/2) + (asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 2 bits
and we have asize + c(bsize/2) available digit positions.  We need to show
this is always enough.  An instance of c(bsize/2) cancels out in both, so
the question reduces to whether asize digits is enough to hold
(asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 2 bits.  If asize < bsize,
then we're asking whether asize digits >= f(bsize/2) digits + 2 bits.  By #4,
asize is at least f(bsize/2)+1 digits, so this in turn reduces to whether 1
digit is enough to hold 2 bits.  This is so since PyLong_SHIFT=15 >= 2.  If
asize == bsize, then we're asking whether bsize digits is enough to hold
c(bsize/2) digits + 2 bits, or equivalently (by #1) whether f(bsize/2) digits
is enough to hold 2 bits.  This is so if bsize >= 2, which holds because
bsize >= KARATSUBA_CUTOFF >= 2.
Note that since there's always enough room for (ah+al)*(bh+bl), and that's
clearly >= each of ah*bh and al*bl, there's always enough room to subtract
ah*bh and al*bl too.
*/

/* b has at least twice the digits of a, and a is big enough that Karatsuba
 * would pay off *if* the inputs had balanced sizes.  View b as a sequence
 * of slices, each with a->ob_size digits, and multiply the slices by a,
 * one at a time.  This gives k_mul balanced inputs to work with, and is
 * also cache-friendly (we compute one double-width slice of the result
 * at a time, then move on, never backtracking except for the helpful
 * single-width slice overlap between successive partial sums).
 */
static PyLongObject *
k_lopsided_mul(PyLongObject *a, PyLongObject *b)
{
    const Py_ssize_t asize = Py_ABS(Py_SIZE(a));
    Py_ssize_t bsize = Py_ABS(Py_SIZE(b));
    Py_ssize_t nbdone;          /* # of b digits already multiplied */
    PyLongObject *ret;
    PyLongObject *bslice = NULL;

    assert(asize > KARATSUBA_CUTOFF);
    assert(2 * asize <= bsize);

    /* Allocate result space, and zero it out. */
    ret = _PyLong_New(asize + bsize);
    if (ret == NULL)
        return NULL;
    memset(ret->ob_digit, 0, Py_SIZE(ret) * sizeof(digit));

    /* Successive slices of b are copied into bslice. */
    bslice = _PyLong_New(asize);
    if (bslice == NULL)
        goto fail;

    nbdone = 0;
    while (bsize > 0) {
        PyLongObject *product;
        const Py_ssize_t nbtouse = Py_MIN(bsize, asize);

        /* Multiply the next slice of b by a. */
        memcpy(bslice->ob_digit, b->ob_digit + nbdone,
               nbtouse * sizeof(digit));
        Py_SET_SIZE(bslice, nbtouse);
        product = k_mul(a, bslice);
        if (product == NULL)
            goto fail;

        /* Add into result. */
        (void)v_iadd(ret->ob_digit + nbdone, Py_SIZE(ret) - nbdone,
                     product->ob_digit, Py_SIZE(product));
        _Py_DECREF_INT(product);

        bsize -= nbtouse;
        nbdone += nbtouse;
    }

    _Py_DECREF_INT(bslice);
    return long_normalize(ret);

  fail:
    Py_DECREF(ret);
    Py_XDECREF(bslice);
    return NULL;
}

PyObject *
_PyLong_Multiply(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;

    /* fast path for single-digit multiplication */
    if (IS_MEDIUM_VALUE(a) && IS_MEDIUM_VALUE(b)) {
        stwodigits v = medium_value(a) * medium_value(b);
        return _PyLong_FromSTwoDigits(v);
    }

    z = k_mul(a, b);
    /* Negate if exactly one of the inputs is negative. */
    if (((Py_SIZE(a) ^ Py_SIZE(b)) < 0) && z) {
        _PyLong_Negate(&z);
        if (z == NULL)
            return NULL;
    }
    return (PyObject *)z;
}


static PyObject *
a_c_round_increment(PyObject *self,
                    PyObject *const *args,
                    Py_ssize_t nargs)
{
    PyObject *value, *increment;
    if (__builtin_expect(PyFloat_CheckExact(value = args[0]), 1)) {
        double a = ((PyFloatObject *)value)->ob_fval, b;
        if (__builtin_expect(PyFloat_CheckExact(increment = args[1]), 1)) {
            b = ((PyFloatObject *)increment)->ob_fval;
        }
        else if (PyLong_CheckExact(increment)) {
            b = PyLong_AsDouble(increment);
        }
        else {
            Py_INCREF(increment);
            return increment;
        }
        double minimum = floor(a / b) * b;
        b += minimum;
        a = (b - a) <= (a - minimum) ? b : minimum;
        if (__builtin_expect(modf(a, &b) != 0., 1)) {
            return PyFloat_FromDouble(a);
        }
        return PyLong_FromDouble(a);
    }
    else if (PyLong_CheckExact(value)) {
        if (PyFloat_CheckExact(increment = args[1])) {
            double a = PyLong_AsDouble(value), b = ((PyFloatObject *)increment)->ob_fval;
            double minimum = floor(a / b) * b;
            b += minimum;
            a = (b - a) <= (a - minimum) ? b : minimum;
            if (__builtin_expect(modf(a, &b) != 0., 1)) {
                return PyFloat_FromDouble(a);
            }
            return PyLong_FromDouble(a);
        }
        else if (PyLong_CheckExact(increment)) {
            PyLongObject *a, *b;
            PyObject *minimum = _PyLong_Subtract((PyLongObject *)value, a = (PyLongObject *)(*long_mod)(value, increment));
            Py_DECREF(a);
            PyObject *maximum = _PyLong_Add((PyLongObject *)minimum, (PyLongObject *)increment);
            PyObject *res = long_compare(a = (PyLongObject *)_PyLong_Subtract((PyLongObject *)maximum, (PyLongObject *)value),
                                         b = (PyLongObject *)_PyLong_Subtract((PyLongObject *)value, (PyLongObject *)minimum)) != 1 ?
                            maximum :
                            minimum;
            Py_DECREF(maximum);
            Py_DECREF(value);
            Py_DECREF(minimum);
            Py_DECREF(a);
            Py_DECREF(b);
            return res;
        }
        else {
            Py_INCREF(increment);
            return increment;
        }
    }
    else {
        Py_INCREF(value);
        return value;
    }
}

typedef struct {
    Py_ssize_t x;
    Py_ssize_t y;
} Point;

static PyTypeObject *pytype_str;

static PyObject *
a_c_AoC2022_d15_p2_cefqrn(PyObject *self,
                          PyObject *const *args,
                          Py_ssize_t nargs)
{
    PyObject *arg;
    const char *string;

    if (__builtin_expect(nargs == 1 &&
                         Py_TYPE(arg = args[0]) == pytype_str, 1))
    {
        string = (const char *)PyUnicode_DATA(arg);
        goto logic;
    }

    if (nargs != 1) {
        PyErr_Format(PyExc_TypeError,
                     "unbound method a_c.AoC2022_d15() takes exactly "
                     "one argument (%zd given)",
                     nargs);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method a_c.AoC2022_d15() expected str, "
                     "got '%.200s'",
                     Py_TYPE(arg)->tp_name);
    }
    return NULL;
  logic:;
    Point lines_a[50][2], lines_b[50][2];
    Point intersections[400];
    Point a, b, c, d;
    Py_ssize_t scanners[25][3];
    Py_ssize_t diffs_a[50];
    Py_ssize_t c_xs[50], c_ys[50];
    Py_ssize_t x, y, bx, by, dist, dist_y;
    int i, j, k, iidx = 0;

#define PARSE_INT(var, delim) \
    var = 0; \
    if (__builtin_expect(*string == '-', 0)) { \
        while (*++string != delim) { \
            var = var*10 - *string + '0'; \
        } \
    } \
    else { \
        while (*string != delim) { \
            var = var*10 + *string++ - '0'; \
        } \
    }
#define ADD_INTERSECT(var) \
    for (j = 0; j < iidx; ++j) { \
        Point intersect = intersections[j]; \
        if (intersect.x == var.x && intersect.y == var.y) { \
            break; \
        } \
    } \
    if (j == iidx) { \
        intersections[iidx++] = var; \
    }
    for (i = 0; i < 25; ++i) {
        string += 12;
        PARSE_INT(x, ',');
        string += 4;
        PARSE_INT(y, ':');
        string += 25;
        PARSE_INT(bx, ',');
        string += 4;
        by = 0;
        if (__builtin_expect(*string == '-', 0)) {
            while (*++string && *string != '\n') {
                by = by*10 - *string + '0';
            }
        }
        else {
            while (*string && *string != '\n') {
                by = by*10 + *string++ - '0';
            }
        }

        dist = (x >= bx ? x - bx : bx - x) + (y >= by ? y - by : by - y);
        a = (Point){ .x = x, .y = y + dist };
        b = (Point){ .x = x + dist, .y = y };
        c = (Point){ .x = x, .y = y - dist };
        d = (Point){ .x = x - dist, .y = y };

        scanners[i][0] = x;
        scanners[i][1] = y;
        scanners[i][2] = dist;

        j = i << 1;

        lines_a[j][0] = c;
        lines_b[j][0] = a;
        lines_a[j][1] = b;
        lines_b[j][1] = b;
        diffs_a[j] = x - c.y;
        c_xs[j] = x;
        c_ys[j] = a.y;
        lines_a[j + 1][0] = d;
        lines_b[j + 1][0] = d;
        lines_a[j + 1][1] = a;
        lines_b[j + 1][1] = c;
        diffs_a[j + 1] = d.x - y;
        c_xs[j + 1] = d.x;
        c_ys[j + 1] = y;

        ADD_INTERSECT(a);
        ADD_INTERSECT(b);
        ADD_INTERSECT(c);
        ADD_INTERSECT(d);

        ++string;
    }
#undef PARSE_INT
#undef ADD_INTERSECT

    Py_ssize_t diff_a, c_x, c_y;

    for (i = 0; i < 50; ++i) {
        diff_a = diffs_a[i];
        for (j = 0; j < 50; ++j) {
            c_x = c_xs[j];
            c_y = c_ys[j];
            if ((x = diff_a + c_x + c_y) & 1 ||
                (x >>= 1) > lines_b[j][1].x || x < c_x)
            {
                continue;
            }

            y = x - diff_a;
            for (k = 0; k < iidx; ++k) {
                Point intersect = intersections[k];
                if (intersect.x == x && intersect.y == y) {
                    break;
                }
            }
            if (k == iidx) {
                intersections[iidx++] = (Point){ .x = x, .y = y };
            }
        }
    }

    Py_ssize_t closest_x[100], closest_y[100];
    Py_ssize_t ranges_x[25], ranges_y[25];
    Py_ssize_t tprev_x = -1, tprev_y = -1, prev_x, prev_y, curr_x, curr_y;
    Py_ssize_t px, py, dr, dh, res;
    int lo, hi, mid, cidx, ridx = 0;

    for (i = 0; i < iidx; ++i) {
        x = intersections[i].x;
        y = intersections[i].y;

        cidx = 0;

        for (j = i + 1; j < iidx; ++j) {
            if ((x >= (bx = intersections[j].x) ? x - bx : bx - x)
                + (y >= (by = intersections[j].y) ? y - by : by - y)
                != 2)
            {
                continue;
            }

            closest_x[cidx] = bx;
            closest_y[cidx++] = by;
        }

        if (cidx == 3) {
            x += closest_x[0] + closest_x[1] + closest_x[2];
            y += closest_y[0] + closest_y[1] + closest_y[2];
            x >>= 2;
            y >>= 2;

            for (k = 0; k < 25; ++k) {
                if ((dh = (py = scanners[k][1]) >= y ? py - y : y - py) >
                    (dr = scanners[k][2]))
                {
                    continue;
                }

                dr -= dh;
                px = scanners[k][0];
                py = px + dr;
                px -= dr;
                lo = 0;
                if (__builtin_expect(ridx > 0, 1)) {
                    hi = ridx;
                    while (lo < hi) {
                        mid = (lo + hi) >> 1;
                        if (px > ranges_x[mid] ||
                            px == ranges_x[mid] && py >= ranges_y[mid])
                        {
                            lo = mid + 1;
                        }
                        else {
                            hi = mid;
                        }
                    }
                    for (hi = ridx; hi >= lo; --hi) {
                        ranges_x[hi + 1] = ranges_x[hi];
                        ranges_y[hi + 1] = ranges_y[hi];
                    }
                }
                ranges_x[lo] = px;
                ranges_y[lo] = py;
                ++ridx;
            }

            prev_x = ranges_x[0];
            prev_y = ranges_y[0];
            cidx = 0;
            for (k = 1; k < ridx; ++k) {
                if (prev_y < (curr_x = ranges_x[k]) - 1) {
                    if (prev_x < x && x <= prev_y) {
                        goto finn;
                    }
                    cidx = 1;
                    tprev_x = prev_x;
                    tprev_y = prev_y;
                    prev_x = curr_x;
                    prev_y = ranges_y[k];
                    continue;
                }
                prev_y = (
                    (tprev_y = prev_y) >= (curr_y = ranges_y[k]) ?
                    prev_y : curr_y
                );
            }
            if (prev_x >= x || x > prev_y) {
                res = x*4000000 + y;
                break;
            }
  finn:;
        }
    }

    return PyLong_FromSsize_t(res);
}

static PyObject *
a_c_test_refcnt(PyObject *self, PyObject *o)
{
    PyObject *s = PyTuple_GET_ITEM(o, 0);
    PyObject *non = PyTuple_GET_ITEM(o, 1);
    Py_INCREF(non);
    printf("%zd\n", Py_REFCNT(s));
    Py_DECREF(o);
    printf("%zd:%zd\n", Py_REFCNT(s), Py_REFCNT(o));
    PyGC_Collect();
    printf("%zd\n", Py_REFCNT(s));
    return non;
}

#define METH_O_NODOC(name) \
    {#name, (PyCFunction)(void(*)(void))a_c_ ## name, METH_O, ""},

#define METH_FASTCALL_KEYWORDS_NODOC(name) \
    {#name, (PyCFunction)(void(*)(void))a_c_ ## name, METH_FASTCALL | METH_KEYWORDS, ""},

#define METH_FASTCALL_NODOC(name) \
    {#name, (PyCFunction)(void(*)(void))a_c_ ## name, METH_FASTCALL, ""},

static PyMethodDef A_CMethods[] = {
    METH_O_NODOC(something_safe1)
    METH_O_NODOC(something_safe2)
    METH_O_NODOC(something_unsafe1)
    METH_O_NODOC(something_unsafe2)
    METH_O_NODOC(test_refcnt)
    METH_O_NODOC(factorial)
    
    METH_FASTCALL_KEYWORDS_NODOC(hello)
    METH_FASTCALL_KEYWORDS_NODOC(fast_min)
    METH_FASTCALL_KEYWORDS_NODOC(fast_max)

    METH_FASTCALL_NODOC(parse_exception_table)
    METH_FASTCALL_NODOC(compute_range_length)
    METH_FASTCALL_NODOC(is_even)
    METH_FASTCALL_NODOC(remove_dupes)
    METH_FASTCALL_NODOC(bitwise_in)
    METH_FASTCALL_NODOC(round_increment)
    METH_FASTCALL_NODOC(AoC2022_d15_p2_cefqrn)
    {NULL}
};

PyDoc_STRVAR(a_c___doc__,
             "Cython vs C extensions testing");

static struct PyModuleDef a_cmodule = {
    PyModuleDef_HEAD_INIT,
    "a_c",
    a_c___doc__,
    -1,
    A_CMethods,
};

PyMODINIT_FUNC
PyInit_a_c(void)
{
    zero = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS];
    one = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+1];
    false_or_true[0] = Py_False;
    false_or_true[1] = Py_True;
    long_mod = PyLong_Type.tp_as_number->nb_remainder;
    long_mul = PyLong_Type.tp_as_number->nb_multiply;
    long_type = &PyLong_Type;
    pytype_str = Py_TYPE(PyUnicode_New(0, 255));
    return PyModule_Create(&a_cmodule);
}
