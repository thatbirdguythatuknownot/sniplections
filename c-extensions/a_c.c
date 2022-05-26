#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define Py_BUILD_CORE
#include "internal/pycore_pystate.h"
#include "internal/pycore_runtime_init.h"
#include "internal/pycore_long.h"
#undef Py_BUILD_CORE

#include <malloc.h>
#include <math.h>
#include <stdint.h>

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
     register PyObject *picked_list, register Py_ssize_t *index,
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
parse_varint(uint_fast8_t *seq, Py_ssize_t *idx)
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

        ASSIGN_1DIGIT(tup_items[0], start, integer, parse_varint(seq, &idx))
        ASSIGN_1DIGIT(tup_items[1], end, integer, start + parse_varint(seq, &idx) + 1)
        parse_varint(seq, &idx);
        Py_INCREF(zero);
        tup_items[2] = zero;
        ASSIGN_1DIGIT(tup_items[3], depth, integer, (dl = parse_varint(seq, &idx)) >> 1)
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

static PyObject *one;

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
    Py_ssize_t length = Py_SIZE(arg = (PyListObject *)args[0]);
    els = arg->ob_item;
    PySetObject *passed = (PySetObject *)PyType_GenericAlloc(&PySet_Type, 0);
    set_fill = set_used = passed->fill = passed->used = 0;
    passed->mask = 7;
    passed->table = passed->smalltable;
    for (Py_ssize_t i = 0; __builtin_expect(i < length, 0);) {
        hash = __builtin_expect((tp = Py_TYPE(item = els[i]))->tp_hash != NULL, 1) ? (*tp->tp_hash)(item) : -1;
        if (__builtin_expect(hash == -1, 0)) {
            (void) PyType_Ready(tp);
            hash = (*tp->tp_hash)(item);
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
            size_t newsize = 8 <= (size_t)minused ? 1 << 64-__builtin_clzll((size_t)minused) : 8;
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
    
    METH_FASTCALL_KEYWORDS_NODOC(hello)

    METH_FASTCALL_NODOC(parse_exception_table)
    METH_FASTCALL_NODOC(compute_range_length)
    METH_FASTCALL_NODOC(is_even)
    METH_FASTCALL_NODOC(remove_dupes)
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
    return PyModule_Create(&a_cmodule);
}
