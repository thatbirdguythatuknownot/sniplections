#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#define Py_BUILD_CORE
#include "internal/pycore_pyerrors.h"
#include "internal/pycore_pystate.h"
#include "internal/pycore_long.h"
#undef Py_BUILD_CORE

/* miscellaneous declarations ***********************************************/

static PyObject *strings1[256];
static PyObject *uchar_longs[256];
static PyTypeObject *list_type, *tuple_type, *range_type, *enum_type,
                    *set_type, *frozenset_type, *dict_type;
static PyTypeObject *unicode_type, *bytes_type, *bytearray_type;
static PyTypeObject *long_type;

typedef struct {
    PyObject_HEAD
    PyObject *start;
    PyObject *stop;
    PyObject *step;
    PyObject *length;
} rangeobject;

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyObject *it_seq;
} seqiterobject;

typedef struct {
    Py_ssize_t curidx;
    Py_ssize_t size;
    ssizeargfunc func;
    Py_ssize_t extra_info;
} sized_info;

/* miscellaneous declarations END *******************************************/

/* utilities ****************************************************************/

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define get_index_from_values_order(v, i) ((char *)v)[-3-i]
#define DK_ENTRIES(dk) (PyDictKeyEntry*)(&((int8_t*)((dk)->dk_indices))[(size_t)1 << (dk)->dk_log2_index_bytes])
#define DK_UNICODE_ENTRIES(dk) (PyDictUnicodeEntry*)(&((int8_t*)((dk)->dk_indices))[(size_t)1 << (dk)->dk_log2_index_bytes])
#define DK_IS_UNICODE(dk) ((dk)->dk_kind != DICT_KEYS_GENERAL)

Py_LOCAL_INLINE(PyObject *)
unicode_char1(Py_UCS1 ch)
{
    return Py_NewRef(strings1[ch]);
}
#define unicode_char1(ch) (Py_NewRef(strings1[ch]))
#define UNICODE_CHAR(bytesize) \
    PyObject *unicode = PyUnicode_New(1, ch); \
    if (unlikely(unicode == NULL)) { \
        return NULL; \
    } \
    PyUnicode_ ## bytesize ## BYTE_DATA(unicode)[0] = ch; \
    return unicode;
Py_LOCAL_INLINE(PyObject *)
unicode_char2(Py_UCS2 ch)
{
    UNICODE_CHAR(2)
}
Py_LOCAL_INLINE(PyObject *)
unicode_char4(Py_UCS4 ch)
{
    UNICODE_CHAR(4)
}

typedef enum {
    LF_NONE,
    LF_SEQ,
    L_ARRAY,
    L_BYTEOBJ,
    L_STR,
    L_DICT,
    L_SET,
    L_AFTERLAZY,
    L_SEQITEREMPTY,
} SIZEDINFO_RET;

Py_LOCAL_INLINE(SIZEDINFO_RET)
get_sized_info(PyObject *it, sized_info *s)
{
    PyTypeObject *it_type = Py_TYPE(it);
    PySequenceMethods *sq;
    PyMappingMethods *mp;

    if (likely((sq = it_type->tp_as_sequence) && sq->sq_length)) {
        if (likely(sq->sq_item != NULL)) {
            s->size = sq->sq_length(it);
            s->func = NULL;
            if (PyList_Check(it) || PyTuple_Check(it)) {
                return L_ARRAY;
            }
            if (PyUnicode_Check(it)) {
                s->extra_info = PyUnicode_KIND(it);
                return L_STR;
            }
            if (PyBytes_Check(it)) {
                s->extra_info = 1;
                return L_BYTEOBJ;
            }
            if (PyByteArray_Check(it)) {
                s->extra_info = 0;
                return L_BYTEOBJ;
            }
            s->func = sq->sq_item;
            return LF_SEQ;
        }
        else if (it_type == set_type || it_type == frozenset_type) {
            s->size = sq->sq_length(it);
            s->func = NULL;
            return L_SET;
        }
    }
    else if ((mp = it_type->tp_as_mapping) && mp->mp_length) {
        s->size = mp->mp_length(it);
        s->func = NULL;
        return L_DICT;
    }
    else if (it_type == &PySeqIter_Type) {
        if (likely(((seqiterobject *)it)->it_seq != NULL)) {
            SIZEDINFO_RET res = get_sized_info(((seqiterobject *)it)->it_seq, s);
            if (likely(s->size >= 0)) {
                s->curidx = ((seqiterobject *)it)->it_index;
            }
            return res;
        }
        s->size = 0;
        s->func = NULL;
        return L_SEQITEREMPTY;
    }
    /* the other sized iterators will unfortunately have to go down the
       lazy iterator path */
    s->size = -1;
    s->func = NULL;
    return LF_NONE;
}

Py_LOCAL_INLINE(Py_ssize_t)
get_len_info(PyObject *it)
{
    PyTypeObject *it_type = Py_TYPE(it);
    PySequenceMethods *sq;
    PyMappingMethods *mp;

    if (likely((sq = it_type->tp_as_sequence) && sq->sq_length)) {
        return sq->sq_length(it);
    }
    if ((mp = it_type->tp_as_mapping) && mp->mp_length) {
        return mp->mp_length(it);
    }
    if (it_type == &PySeqIter_Type) {
        if (likely(((seqiterobject *)it)->it_seq != NULL)) {
            return (get_len_info(((seqiterobject *)it)->it_seq)
                    - ((seqiterobject *)it)->it_index);
        }
        return 0;
    }
    return -1;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

Py_LOCAL_INLINE(Py_ssize_t)
long_to_ssize(PyObject *o, int *oflow)
{
    uint64_t n_temp;
    Py_ssize_t n;
    const Py_ssize_t long_arg_size = Py_SIZE(o);

    if (likely(long_arg_size == 1)) {
        n = ((PyLongObject *)o)->ob_digit[0];
    }
    else if (long_arg_size == 0) {
        n = 0;
    }
    else {
        const digit *ob_digit = ((PyLongObject *)o)->ob_digit;
        switch (long_arg_size) {
        case 2:
            n = (uint64_t)ob_digit[0] | ((uint64_t)ob_digit[1] << PyLong_SHIFT);
#if SIZEOF_SIZE_T == 8
            break;
        case 3:
            n_temp = ((uint64_t)ob_digit[0]
                      | (((uint64_t)ob_digit[1]
                          | ((uint64_t)ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
#endif
            if (unlikely(n_temp > (uint64_t)PY_SSIZE_T_MAX ||
#if SIZEOF_SIZE_T == 8
                         ob_digit[2] >= 8))
#else
                         ob_digit[1] >= 2))
#endif
            {
                goto overflow;
            }
            n = Py_SAFE_DOWNCAST(n_temp, uint64_t, Py_ssize_t);
            break;
        default:
            goto overflow;
        }
    }

    *oflow = 0;
    return n;
  overflow:
    *oflow = 1;
    return -1;
}

/* utilities END ************************************************************/

/* .chunked() and chunked object ********************************************/

static PyTypeObject chunked_type;

typedef struct {
    PyObject_HEAD;
    PyObject *iterable;
    Py_ssize_t n;
    int strict;
    int done;
} chunkedobject;

static PyObject *
fast_itertools_chunked(PyObject *self,
                       PyObject *const *args,
                       Py_ssize_t nargs,
                       PyObject *kwnames)
{
    PyObject *iterable = NULL;
    int iterable_given = 0;
    Py_ssize_t n;
    PyObject *n_arg;
    int n_given = 0;
    int strict = 0;
    int strict_given = 0;
    Py_ssize_t nkwargs = 0;
    Py_ssize_t kwname_length;
    PyASCIIObject *kwname;
    void *kwname_data;
    Py_ssize_t i;
    int err;
    PyObject **kwnames_items;
    Py_ssize_t largs = nargs;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs < 2 || largs > 3)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.chunked() takes from 2 to 3 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 3:
        strict_given = 1;
        strict = PyObject_IsTrue(args[2]);
        if (unlikely(strict < 0)) {
            goto error;
        }
    case 2:
        n_given = 1;
        n_arg = args[1];
        if (likely(PyLong_Check(n_arg))) {
            n = long_to_ssize(n_arg, &err);
            if (err) {
                goto overflow_error;
            }
        }
        else if (n_arg == Py_None) {
            if (unlikely(strict)) {
                PyErr_SetString(PyExc_ValueError,
                                "fast_itertools.chunked(): 'n' must not be "
                                "None when using strict mode");
                goto error;
            }
            n = PY_SSIZE_T_MAX;
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'n' argument for fast_itertools.chunked() must be "
                         "None or an integer (0 <= n <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    case 1:
        iterable_given = 1;
        iterable = PyObject_GetIter(args[0]);
        if (unlikely(iterable == NULL)) {
            goto error;
        }
        break;
    }

    for (i = 0; i < nkwargs; i++) {
        kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length;
        kwname_data = PyUnicode_DATA(kwname);
        if (unlikely(
                kwname_length == 8 &&
                memcmp(kwname_data, "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.chunked() got multiple values "
                                "for argument 'iterable'");
                goto error;
            }
            iterable = PyObject_GetIter(args[nargs + i]);
            if (unlikely(iterable == NULL)) {
                goto error;
            }
        }
        else if (likely(
                    kwname_length == 1 &&
                    *((char *)kwname_data) == 'n'
                 ))
        {
            if (unlikely(n_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.chunked() got multiple values "
                                "for argument 'n'");
                goto error;
            }
            n_arg = args[nargs + i];
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
            }
            else if (n_arg == Py_None) {
                n = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.chunked() must be "
                             "None or an integer (0 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else if (likely(
                    kwname_length == 6 &&
                    memcmp(kwname_data, "strict", 6) == 0
                 ))
        {
            if (unlikely(strict_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.chunked() got multiple values "
                                "for argument 'strict'");
                goto error;
            }
            PyObject *arg = args[nargs + i];
            if (n_arg == Py_None) {
                PyErr_SetString(PyExc_ValueError,
                                "fast_itertools.chunked(): 'n' must not be "
                                "None when using strict mode");
                goto error;
            }
            strict = PyObject_IsTrue(arg);
            if (unlikely(strict < 0)) {
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "fast_itertools.chunked() got an unexpected "
                         "keyword argument '%U'",
                         kwname);
            goto error;
        }
    }

    chunkedobject *res = (chunkedobject *)chunked_type.tp_alloc(&chunked_type, 0);
    if (unlikely(res == NULL)) {
        PyErr_NoMemory();
        goto error;
    }
    res->iterable = iterable;
    res->n = n;
    res->strict = strict;
    res->done = 0;
    return (PyObject *)res;
overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.chunked() must "
                 "be None or a positive number <= %zd",
                 PY_SSIZE_T_MAX);
  error:
    Py_XDECREF(iterable);
    return NULL;
}

static void
chunked_dealloc(chunkedobject *o)
{
    PyObject_GC_UnTrack(o);
    Py_XDECREF(o->iterable);
    Py_TYPE(o)->tp_free(o);
}

static int
chunked_traverse(chunkedobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->iterable);
    return 0;
}

static PyObject *
chunked_next(chunkedobject *o)
{
    PyObject *iterable = o->iterable, **items;
    Py_ssize_t n = o->n;
    int done = o->done;
    Py_ssize_t i;

    if (unlikely(o->done)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    PyObject *res = PyList_New(n);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    items = _PyList_ITEMS(res);
    iternextfunc it = Py_TYPE(iterable)->tp_iternext;
    for (i = 0; i < n; i++) {
        PyObject *item = it(iterable);
        if (unlikely(item == NULL)) {
            PyThreadState *tstate = _PyThreadState_GET();
            PyObject *exc;
            if ((exc = _PyErr_Occurred(tstate)) == NULL || exc &&
                PyErr_GivenExceptionMatches(exc, PyExc_StopIteration))
            {
                o->done = 1;
                if (i > 0) {
                    if (unlikely(o->strict)) {
                        _PyErr_Format(tstate, PyExc_ValueError,
                                      "next(fast_itertools.chunked()): iterable is not "
                                      "divisible by %zd",
                                      n);
                        goto error;
                    }
                    _PyErr_Restore(tstate, NULL, NULL, NULL);
                    break;
                }
            }
            goto error;
        }
        items[i] = item;
    }
    Py_SET_SIZE(res, i);
    return res;
  error:
    Py_DECREF(res);
    return NULL;
}

static PyTypeObject chunked_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.chunked([], 0).__class__",
    .tp_basicsize = sizeof(chunkedobject),
    .tp_dealloc = (destructor)chunked_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_doc = "Base class for the result of fast_itertools.chunked()",
    .tp_traverse = (traverseproc)chunked_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)chunked_next,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
};

PyDoc_STRVAR(chunked__doc__,
"fast_itertools.chunked(iterable, n, strict=False)\n\
    Break *iterable* into lists of length *n*:\n\
\n\
        >>> list(fast_itertools.chunked([1, 2, 3, 4, 5, 6], 3))\n\
        [[1, 2, 3], [4, 5, 6]]\n\
\n\
    By the default, the last yielded list will have fewer than *n* elements\n\
    if the length of *iterable* is not divisible by *n*:\n\
\n\
        >>> list(fast_itertools.chunked([1, 2, 3, 4, 5, 6, 7, 8], 3))\n\
        [[1, 2, 3], [4, 5, 6], [7, 8]]\n\
\n\
    To use a fill-in value instead, see the :func:`fast_itertools.grouper` recipe.\n\
\n\
    If the length of *iterable* is not divisible by *n* and *strict* is\n\
    ``True``, then ``ValueError`` will be raised before the last\n\
    list is yielded.\n");

/* .chunked() and chunked object END ****************************************/

/* .ichunked() and ichunked object ******************************************/

static PyTypeObject ichunk_type;

typedef struct {
    PyObject_HEAD;
    PyObject *iterable;
    Py_ssize_t n;
    Py_ssize_t idx;
    int *doneptr;
    int allocated_doneptr;
    PyObject **cache;
    Py_ssize_t cache_min_idx;
} ichunkobject;

static PyObject *
ichunk_new(PyTypeObject *type, PyObject *iterable,
           Py_ssize_t n, int *doneptr)
{
    ichunkobject *o = (ichunkobject *)type->tp_alloc(type, 0);
    if (unlikely(o == NULL)) {
        Py_DECREF(iterable);
        return NULL;
    }
    o->iterable = iterable;
    o->n = n;
    o->idx = 0;
    if (doneptr != NULL) {
        o->doneptr = doneptr;
        o->allocated_doneptr = 0;
    }
    else {
        o->doneptr = malloc(sizeof(int));
        if (unlikely(o->doneptr == NULL)) {
            o->allocated_doneptr = 0;
            Py_DECREF(o);
            PyErr_NoMemory();
            return NULL;
        }
        o->allocated_doneptr = 1;
    }
    o->cache = NULL;
    o->cache_min_idx = -1;
    return (PyObject *)o;
}

static PyObject *
ichunk__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *iterable, *n_arg;
    Py_ssize_t n;
    int strict;
    int err;

    Py_ssize_t nargs = 0;
    if (likely(args != NULL)) {
        nargs = PyTuple_GET_SIZE(args);
        if (likely(nargs == 2)) {
            n_arg = PyTuple_GET_ITEM(args, 1);
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
            }
            else if (n_arg == Py_None) {
                n = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.ichunk() must "
                             "be None or an integer (0 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                return NULL;
            }
            iterable = PyObject_GetIter(PyTuple_GET_ITEM(args, 0));
            if (unlikely(iterable == NULL)) {
                return NULL;
            }
        }
        else {
            goto incorrect_number_args;
        }
    }
    else {
  incorrect_number_args:
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.ichunk() takes 2 "
                     "positional arguments (got %zd)",
                     nargs);
        return NULL;
    }
    if (unlikely(kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.ichunk() takes no "
                     "keyword arguments (got %zd)",
                     PyDict_GET_SIZE(kwargs));
        return NULL;
    }

    return ichunk_new(type, iterable, n, NULL);
overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.ichunk() must "
                 "be None or a positive number <= %zd",
                 PY_SSIZE_T_MAX);
    return NULL;
}

Py_LOCAL_INLINE(int)
ichunk_fillcache_internal(ichunkobject *o)
{
    PyObject **cache, *iterable;
    Py_ssize_t n;

    if (unlikely(o->idx == (n = o->n) || o->cache != NULL)) {
        return 0;
    }
    iterable = o->iterable;

    o->cache = cache = PyMem_Calloc(n, sizeof(PyObject *));
    if (unlikely(cache == NULL)) {
        return -1;
    }
    iternextfunc it = Py_TYPE(iterable)->tp_iternext;
    for (Py_ssize_t i = o->cache_min_idx = o->idx; i < n; i++) {
        PyObject *item = it(iterable);
        if (unlikely(item == NULL)) {
            PyThreadState *tstate = _PyThreadState_GET();
            PyObject *exc;
            if ((exc = _PyErr_Occurred(tstate)) == NULL || exc &&
                PyErr_GivenExceptionMatches(exc, PyExc_StopIteration))
            {
                *o->doneptr = 1;
                if (i > o->cache_min_idx) {
                    _PyErr_Restore(tstate, NULL, NULL, NULL);
                    break;
                }
            }
            return -1;
        }
        cache[i] = item;
    }
    return 0;
}

static PyObject *
ichunk_fill_cache(ichunkobject *self, PyObject *Py_UNUSED(ignored))
{
    if (ichunk_fillcache_internal(self) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static void
ichunk_dealloc(ichunkobject *o)
{
    PyObject_GC_UnTrack(o);
    if (unlikely(o->allocated_doneptr)) {
        free(o->doneptr);
    }
    if (o->cache) {
        PyMem_Free(o->cache);
    }
    Py_TYPE(o)->tp_free(o);
}

static int
ichunk_traverse(ichunkobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->iterable);
    return 0;
}

static PyObject *
ichunk_next(ichunkobject *o)
{
    PyObject *iterable = o->iterable;
    PyObject **cache = o->cache;
    Py_ssize_t n = o->n;
    Py_ssize_t idx = o->idx;

    if (unlikely(idx == n)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (likely(cache != NULL)) {
        o->idx++;
        return cache[idx];
    }
    PyObject *item = Py_TYPE(iterable)->tp_iternext(iterable);
    if (unlikely(item == NULL)) {
        PyThreadState *tstate = _PyThreadState_GET();
        PyObject *exc;
        if ((exc = _PyErr_Occurred(tstate)) == NULL || exc &&
            PyErr_GivenExceptionMatches(exc, PyExc_StopIteration))
        {
            *o->doneptr = 1;
            o->idx = n;
        }
        goto error;
    }
    o->idx++;
    return item;
  error:
    Py_DECREF(item);
    return NULL;
}

static PyObject *
ichunk_reduce(ichunkobject *o, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("O(On)", Py_TYPE(o), o->n);
}

static PyObject *
ichunk_setstate(ichunkobject *o, PyObject *num)
{
    int err;
    Py_ssize_t n;

    if (unlikely(!PyLong_Check(num))) {
  invalid_argument:
        PyErr_Format(PyExc_ValueError,
                     "fast_itertools.ichunk().__setstate__(): "
                     "argument must be an integer <= %zd",
                     PY_SSIZE_T_MAX);
        return NULL;
    }

    if (unlikely(o->cache == NULL)) {
        PyErr_SetString(PyExc_TypeError,
                        "fast_itertools.ichunk().__setstate__() failed: "
                        "cache is non-existent (must do .fill_cache() first)");
        return NULL;
    }

    n = long_to_ssize(num, &err);
    if (err) {
        goto invalid_argument;
    }

    if (unlikely(n < o->cache_min_idx)) {
        PyErr_Format(PyExc_ValueError,
                     "fast_itertools.ichunk().__setstate__() failed: "
                     "cache index is below minimum index (%zd)",
                     o->cache_min_idx);
        return NULL;
    }
    o->idx = n;
    Py_RETURN_NONE;
}

static PyMethodDef ichunk_methods[] = {
    {"fill_cache", (PyCFunction)ichunk_fill_cache, METH_NOARGS,
     "Fill the internal cache."},
    {"__reduce__", (PyCFunction)ichunk_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)ichunk_setstate, METH_O, setstate_doc},
    {NULL}
};

static PyTypeObject ichunk_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.ichunk",
    .tp_basicsize = sizeof(ichunkobject),
    .tp_dealloc = (destructor)ichunk_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_doc = "fast_itertools.ichunk(iterable, n, /) [TODO: make them normal parameters]\n\
\n\
Base class for the generator result of next(fast_itertools.ichunked())",
    .tp_traverse = (traverseproc)ichunk_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)ichunk_next,
    .tp_methods = ichunk_methods,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = (newfunc)ichunk__new__,
    .tp_free = PyObject_GC_Del,
};

static PyTypeObject ichunked_type;

typedef struct {
    PyObject_HEAD;
    PyObject *iterable;
    Py_ssize_t n;
    int done;
} ichunkedobject;

static PyObject *
fast_itertools_ichunked(PyObject *self,
                        PyObject *const *args,
                        Py_ssize_t nargs,
                        PyObject *kwnames)
{
    PyObject *iterable = NULL;
    int iterable_given = 0;
    Py_ssize_t n;
    PyObject *n_arg;
    int n_given = 0;
    Py_ssize_t nkwargs = 0;
    Py_ssize_t kwname_length;
    PyASCIIObject *kwname;
    void *kwname_data;
    Py_ssize_t i;
    int err;
    PyObject **kwnames_items;
    Py_ssize_t largs = nargs;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs < 2 || largs > 2)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.ichunked() takes 2 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 2:
        n_given = 1;
        n_arg = args[1];
        if (likely(PyLong_Check(n_arg))) {
            n = long_to_ssize(n_arg, &err);
            if (err) {
                goto overflow_error;
            }
        }
        else if (n_arg == Py_None) {
            n = PY_SSIZE_T_MAX;
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'n' argument for fast_itertools.ichunked() must be "
                         "None or an integer (0 <= n <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    case 1:
        iterable_given = 1;
        iterable = PyObject_GetIter(args[0]);
        if (unlikely(iterable == NULL)) {
            goto error;
        }
        break;
    }

    for (i = 0; i < nkwargs; i++) {
        if (unlikely(
                (kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length) == 8 &&
                memcmp((kwname_data = PyUnicode_DATA(kwname)), "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.ichunked() got multiple values "
                                "for argument 'iterable'");
                goto error;
            }
            iterable = PyObject_GetIter(args[nargs + i]);
            if (unlikely(iterable == NULL)) {
                goto error;
            }
        }
        else if (likely(
                    kwname_length == 1 &&
                    *((char *)kwname_data) == 'n'
                 ))
        {
            if (unlikely(n_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.ichunked() got multiple values "
                                "for argument 'n'");
                goto error;
            }
            n_arg = args[nargs + i];
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
            }
            else if (n_arg == Py_None) {
                n = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.ichunked() must be "
                             "None or an integer (0 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "fast_itertools.ichunked() got an unexpected "
                         "keyword argument '%U'",
                         kwname);
            goto error;
        }
    }

    ichunkedobject *res = (ichunkedobject *)ichunked_type.tp_alloc(&ichunked_type, 0);
    if (unlikely(res == NULL)) {
        goto error;
    }
    res->iterable = iterable;
    res->n = n;
    res->done = 0;
    return (PyObject *)res;
overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.ichunked() must "
                 "be None or a positive number <= %zd",
                 PY_SSIZE_T_MAX);
  error:
    Py_XDECREF(iterable);
    return NULL;
}

static void
ichunked_dealloc(ichunkedobject *o)
{
    PyObject_GC_UnTrack(o);
    Py_XDECREF(o->iterable);
    Py_TYPE(o)->tp_free(o);
}

static int
ichunked_traverse(ichunkedobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->iterable);
    return 0;
}

static PyObject *
ichunked_next(ichunkedobject *o)
{
    if (unlikely(o->done)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    PyObject *res = ichunk_new(&ichunk_type, o->iterable, o->n, &o->done);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    if (unlikely(ichunk_fillcache_internal((ichunkobject *)res) < 0)) {
        Py_DECREF(res);
        return NULL;
    }
    return res;
}

static PyTypeObject ichunked_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.ichunked([], 0).__class__",
    .tp_basicsize = sizeof(ichunkedobject),
    .tp_dealloc = (destructor)ichunked_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_doc = "Base class for the result of fast_itertools.ichunked()",
    .tp_traverse = (traverseproc)ichunked_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)ichunked_next,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
};

PyDoc_STRVAR(ichunked__doc__,
"ichunked(iterable, n)\n\
    Break *iterable* into sub-iterables with *n* elements each.\n\
    :func:`fast_itertools.ichunked` is like :func:`fast_itertools.chunked`,\n\
    but it yields iterables instead of lists.\n\
\n\
    If the sub-iterables are read in order, the elements of *iterable*\n\
    won't be stored in memory.\n\
    If they are read out of order, :func:`itertools.tee` is used to cache\n\
    elements as necessary.\n\
\n\
    >>> import fast_itertools\n\
    >>> all_chunks = fast_itertools.ichunked(count(), 4)\n\
    >>> c_1, c_2, c_3 = next(all_chunks), next(all_chunks), next(all_chunks)\n\
    >>> list(c_2)  # c_1's elements have been cached; c_3's haven't been\n\
    [4, 5, 6, 7]\n\
    >>> list(c_1)\n\
    [0, 1, 2, 3]\n\
    >>> list(c_3)\n\
    [8, 9, 10, 11]\n");

/* .ichunked() and ichunked object END **************************************/

/* .chunked_even() and chunked_even object **********************************/

static PyTypeObject chunkedeven_type;

typedef struct _chunkedevenobject {
    PyObject_HEAD;
    PyObject *iterable;
    Py_ssize_t n;
    Py_ssize_t taken;
    union {
        /* arrange the structs in a way that these fields align:
                sized.size ~~ afterlazy.size
                sized.n_left ~~ afterlazy.n_left
                lazy.headidx ~~ afterlazy.idx
                lazy.bufsize ~~ afterlazy.bufsize
                lazy.buf ~~ afterlazy.buf
        */
        struct {
            Py_ssize_t size;
            Py_ssize_t n_left;
            ssizeargfunc func;
            Py_ssize_t extra_info;
        } sized;
        struct {
            iternextfunc func;
            Py_ssize_t tailidx;
            Py_ssize_t headidx;
            Py_ssize_t bufsize;
            PyObject **buf;
        } lazy;
        struct {
            Py_ssize_t size;
            Py_ssize_t n_left;
            Py_ssize_t idx;
            Py_ssize_t bufsize;
            PyObject **buf;
        } afterlazy;
    } info;
    PyObject *(*func)(struct _chunkedevenobject *);
    SIZEDINFO_RET type;
    int done;
} chunkedevenobject;

#define RES_INIT(n) \
    res = PyList_New(n); \
    if (unlikely(res == NULL)) { \
        return NULL; \
    } \
    res_items = _PyList_ITEMS(res);

PyObject *
chunkedeven_sizedwrapper(chunkedevenobject *o)
{
    ssizeargfunc func = o->info.sized.func;
    PyObject *res, *iterable = o->iterable;
    PyObject **res_items;
    Py_ssize_t i;

    RES_INIT(o->n)
    for (i = 0; i < o->n; ++o->taken) {
        res_items[i] = func(iterable, o->taken);
        if (unlikely(res_items[i++] == NULL)) {
            Py_DECREF(res);
            return NULL;
        }
    }
    return res;
}

PyObject *
chunkedeven_array_iternext(chunkedevenobject *o)
{
    PyObject *res;
    PyObject **res_items, **items = PySequence_Fast_ITEMS(o->iterable);
    Py_ssize_t i;

    RES_INIT(o->n)
    for (i = 0; i < o->n; ++i) {
        res_items[i] = items[o->taken++];
        Py_XINCREF(res_items[i]);
    }
    return res;
}

PyObject *
chunkedeven_str_iternext(chunkedevenobject *o)
{
    PyObject *res, **res_items;
    Py_ssize_t i;

    RES_INIT(o->n)
#define ALGO(bytesize) \
    case PyUnicode_##bytesize##BYTE_KIND: { \
        Py_UCS##bytesize *data = PyUnicode_##bytesize##BYTE_DATA(o->iterable); \
        for (i = 0; i < o->n; ++i) { \
            res_items[i] = unicode_char##bytesize(data[o->taken++]); \
            if (unlikely(res_items[i] == NULL)) { \
                Py_DECREF(res); \
                return NULL; \
            } \
        } \
        break; \
    }
    switch (o->info.sized.extra_info) {
        ALGO(1)
        ALGO(2)
        ALGO(4)
    }
    return res;
#undef ALGO
}

PyObject *
chunkedeven_byteobj_iternext(chunkedevenobject *o)
{
    PyObject *res, **res_items;
    char *ob_sval = (
        o->info.sized.extra_info ?
        PyBytes_AS_STRING(o->iterable) :
        PyByteArray_AS_STRING(o->iterable)
    );
    Py_ssize_t i;

    RES_INIT(o->n)
    for (i = 0; i < o->n; ++i) {
        res_items[i] = uchar_longs[ob_sval[o->taken++]];
        Py_INCREF(res_items[i]);
    }
    return res;
}

PyObject *
chunkedeven_dict_iternext(chunkedevenobject *o)
{
    PyObject *res, **res_items;
    Py_ssize_t i = 0;

    PyDictValues *v = ((PyDictObject *)o->iterable)->ma_values;
    PyDictKeysObject *k = ((PyDictObject *)o->iterable)->ma_keys;
    RES_INIT(o->n)
    if (v) {
        PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(k);
        while (i < o->n) {
            res_items[i] = entries[get_index_from_values_order(v, o->taken++)].me_key;
            Py_INCREF(res_items[i++]);
        }
    }
    else {
#define ITER_ALGO \
    entry_ptr += o->info.sized.extra_info; \
    for (; i < o->n; ++o->info.sized.extra_info) { \
        if (entry_ptr->me_value != NULL) { \
            res_items[i] = entry_ptr->me_key; \
            Py_INCREF(res_items[i++]); \
            ++o->taken; \
        } \
        ++entry_ptr; \
    }
        if (DK_IS_UNICODE(k)) {
            PyDictUnicodeEntry *entry_ptr = DK_UNICODE_ENTRIES(k);
            ITER_ALGO
        }
        else {
            PyDictKeyEntry *entry_ptr = DK_ENTRIES(k);
            ITER_ALGO
        }
#undef ITER_ALGO
    }
    return res;
}

PyObject *
chunkedeven_set_iternext(chunkedevenobject *o)
{
    setentry *entry = ((PySetObject *)o->iterable)->table;
    PyObject *key, *res, **res_items;
    Py_ssize_t i = 0;

    RES_INIT(o->n)
    while (likely(i < o->n)) {
        if ((key = entry[o->info.sized.extra_info++].key) != NULL &&
            key != _PySet_Dummy) {
            res_items[i++] = key;
            Py_INCREF(key);
            ++o->taken;
        }
    }
    return res;
}

PyObject *
chunkedeven_limitedgeneric_iternext(chunkedevenobject *o)
{
    PyObject *res, **res_items, **buf = o->info.afterlazy.buf;
    Py_ssize_t i, bufsize = o->info.afterlazy.bufsize;

    RES_INIT(o->n)
    for (i = 0; i < o->n; ++i) {
        res_items[i] = buf[o->info.afterlazy.idx++];
        if (unlikely(o->info.afterlazy.idx == bufsize)) {
            o->info.afterlazy.idx = 0;
        }
        ++o->taken;
    }
    return res;
}

PyObject *
chunkedeven_generic_iternext(chunkedevenobject *o)
{
    PyObject *res, *item, *iterable = o->iterable;
    PyObject **buf = o->info.lazy.buf;
    iternextfunc func = o->info.lazy.func;
    Py_ssize_t n = o->n, bufsize = o->info.lazy.bufsize;

    do {
        item = func(iterable);
        if (unlikely(item == NULL)) {
            if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
                PyErr_Clear();
                goto end_of_generator;
            }
        }
        Py_INCREF(item);
        buf[o->info.lazy.tailidx++] = item;
        if (unlikely(o->info.lazy.tailidx == bufsize)) {
            o->info.lazy.tailidx = 0;
        }
    } while (o->info.lazy.tailidx != o->info.lazy.headidx);

    res = PyList_New(n);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    PyObject **res_items = _PyList_ITEMS(res);
    Py_ssize_t size;
    if ((size = bufsize - o->info.lazy.headidx) < n) {
        memmove(res_items,
                buf + o->info.lazy.headidx,
                size * sizeof(PyObject *));
        memmove(res_items + size,
                buf,
                (o->info.lazy.headidx = n - size) * sizeof(PyObject *));
    }
    else {
        memmove(res_items, buf + o->info.lazy.headidx,
                n * sizeof(PyObject *));
        o->info.lazy.headidx = (o->info.lazy.headidx + n) % bufsize;
    }
    return res;
end_of_generator:
    o->func = chunkedeven_limitedgeneric_iternext;
    o->type = L_AFTERLAZY;
    size = o->info.lazy.tailidx - o->info.lazy.headidx;
    if (size < 0) {
        size += bufsize;
    }
    else if (unlikely(size == 0)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    o->info.afterlazy.size = size;
    Py_ssize_t quot = size / n;
    Py_ssize_t rem = size % n;
    Py_ssize_t i = quot + (rem != 0);
    o->n = size/i;
    o->info.afterlazy.n_left = size%i * o->n;
    if (unlikely(o->info.afterlazy.n_left == 0)) {
        o->info.afterlazy.n_left = size;
    }
    else {
        ++o->n;
    }
    return o->func(o);
}

#undef RES_INIT

static PyObject *
fast_itertools_chunked_even(PyObject *self,
                            PyObject *const *args,
                            Py_ssize_t nargs,
                            PyObject *kwnames)
{
    PyObject *iterable = NULL;
    int iterable_given = 0;
    Py_ssize_t n, quot, rem;
    PyObject *n_arg;
    int n_given = 0;
    Py_ssize_t nkwargs = 0;
    Py_ssize_t kwname_length;
    PyASCIIObject *kwname;
    void *kwname_data;
    Py_ssize_t i;
    int err;
    PyObject **kwnames_items;
    Py_ssize_t largs = nargs;
    sized_info s;
    chunkedevenobject *res;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs != 2)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.chunked_even() takes 2 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 2:
        n_given = 1;
        n_arg = args[1];
        if (likely(PyLong_Check(n_arg))) {
            n = long_to_ssize(n_arg, &err);
            if (err) {
                goto overflow_error;
            }
        }
        else if (n_arg == Py_None) {
            n = PY_SSIZE_T_MAX;
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'n' argument for fast_itertools.chunked_even() "
                         "must be None or an integer (1 <= n <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    case 1:
        iterable_given = 1;
        iterable = args[0];
        break;
    }

    for (i = 0; i < nkwargs; i++) {
        kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length;
        kwname_data = PyUnicode_DATA(kwname);
        if (unlikely(
                kwname_length == 8 &&
                memcmp(kwname_data, "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.chunked_even() got multiple values "
                                "for argument 'iterable'");
                goto error;
            }
            iterable = args[nargs + i];
        }
        else if (likely(
                    kwname_length == 1 &&
                    *((char *)kwname_data) == 'n'
                 ))
        {
            if (unlikely(n_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.chunked_even() got multiple values "
                                "for argument 'n'");
                goto error;
            }
            n_arg = args[nargs + i];
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
            }
            else if (n_arg == Py_None) {
                n = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.chunked_even() "
                             "must be None or an integer (1 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
    }

    res = (chunkedevenobject *)chunkedeven_type.tp_alloc(&chunkedeven_type, 0);
    if (unlikely(res == NULL)) {
        PyErr_NoMemory();
        goto error;
    }
    res->iterable = Py_NewRef(iterable);
    res->n = n;
    res->done = 0;
    SIZEDINFO_RET status = get_sized_info(iterable, &s);
    res->type = status;
    if (status) {
        /* also covers status == L_SEQITEREMPTY */
        if (unlikely(s.size == 0)) {
            res->done = 1;
            return (PyObject *)res;
        }
        res->info.sized.size = s.size;
        quot = s.size / n;
        rem = s.size % n;
        i = quot + (rem != 0);
        res->n = s.size/i;
        res->info.sized.n_left = s.size%i * res->n;
        if (unlikely(res->info.sized.n_left == 0)) {
            res->info.sized.n_left = s.size;
        }
        else {
            ++res->n;
        }
        res->info.sized.extra_info = s.extra_info;
        if (status == L_ARRAY) {
            res->func = chunkedeven_array_iternext;
        }
        else if (status == L_STR) {
            res->func = chunkedeven_str_iternext;
        }
        else if (status == L_BYTEOBJ) {
            res->func = chunkedeven_byteobj_iternext;
        }
        else if (status == LF_SEQ) {
            res->info.sized.func = s.func;
            res->func = chunkedeven_sizedwrapper;
        }
        else if (status == L_DICT) {
            res->info.sized.extra_info = 0;
            res->func = chunkedeven_dict_iternext;
        }
        else { /* status == L_SET */
            res->info.sized.extra_info = 0;
            res->func = chunkedeven_set_iternext;
        }
    }
    else {
        res->taken = 0;
        n *= n - 2;
        if (unlikely(n > PY_SSIZE_T_MAX/sizeof(PyObject *) - 2)) {
            PyErr_NoMemory();
            goto error;
        }
        res->info.lazy.bufsize = n + 2;
        res->info.lazy.buf = PyObject_Calloc(n + 2, sizeof(PyObject *));
        if (unlikely(res->info.lazy.buf == NULL)) {
            PyErr_NoMemory();
            goto error;
        }
        res->info.lazy.headidx = res->info.lazy.tailidx = 0;
        res->info.lazy.func = Py_TYPE(iterable)->tp_iternext;
        if (unlikely(res->info.lazy.func == NULL)) {
            PyErr_SetString(PyExc_ValueError,
                            "expected iterable for 'iterable' argument of "
                            "fast_itertools.chunked_even()");
            goto error;
        }
        res->func = chunkedeven_generic_iternext;
    }
    return (PyObject *)res;
  overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.chunked_even() must be "
                 "None or 1 <= n <= %zd",
                 PY_SSIZE_T_MAX);
  error:
    Py_XDECREF(res);
    return NULL;
}

static void
chunkedeven_dealloc(chunkedevenobject *o)
{
    PyObject_GC_UnTrack(o);
    Py_XDECREF(o->iterable);
    if (o->type == LF_NONE || o->type == L_AFTERLAZY) {
        /* this works in both cases since the `.afterlazy.buf` field
           is aligned with the `.lazy.buf` field */
        PyObject_Free(o->info.lazy.buf);
    }
    Py_TYPE(o)->tp_free(o);
}

static int
chunkedeven_traverse(chunkedevenobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->iterable);
    return 0;
}

static PyObject *
chunkedeven_next(chunkedevenobject *o)
{
    int C;
    if (!o->done) {
        PyObject *res = o->func(o);
        if (unlikely(res == NULL)) {
            if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
                o->done = 1;
            }
            return NULL;
        }
        if (o->type != LF_NONE) {
            if (o->taken >= o->info.sized.n_left) {
                if (o->info.sized.n_left == o->info.sized.size) {
                    o->done = 1;
                }
                else {
                    o->info.sized.n_left = o->info.sized.size;
                    --o->n;
                }
            }
        }
        return res;
    }

    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
}

#define READONLY_MEMBER(field, type) \
    {#field, type, offsetof(chunkedevenobject, field), READONLY},

static PyMemberDef chunkedeven_members[] = {
    READONLY_MEMBER(iterable, T_OBJECT)
    READONLY_MEMBER(n, T_PYSSIZET)
    READONLY_MEMBER(taken, T_PYSSIZET)
    READONLY_MEMBER(type, T_INT)
    READONLY_MEMBER(done, T_BOOL)
    {NULL}
};

#undef READONLY_MEMBER

static PyTypeObject chunkedeven_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.chunked_even([], 0).__class__",
    .tp_basicsize = sizeof(chunkedevenobject),
    .tp_dealloc = (destructor)chunkedeven_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_doc = "Base class for the result of fast_itertools.chunked_even()",
    .tp_traverse = (traverseproc)chunkedeven_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)chunkedeven_next,
    .tp_members = chunkedeven_members,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
};

PyDoc_STRVAR(chunked_even__doc__,
"fast_itertools.chunked_even(iterable, n)\n\
    Break *iterable* into lists of approximately length *n*.\n\
    Items are distributed such the lengths of the lists differ by at most\n\
    1 item.\n\
\n\
    >>> from fast_itertools import chunked_even\n\
    >>> iterable = [1, 2, 3, 4, 5, 6, 7]\n\
    >>> n = 3\n\
    >>> list(chunked_even(iterable, n))  # List lengths: 3, 2, 2\n\
    [[1, 2, 3], [4, 5], [6, 7]]\n\
    >>> list(chunked(iterable, n))  # List lengths: 3, 3, 1\n\
    [[1, 2, 3], [4, 5, 6], [7]]\n");

/* .chunked_even() and chunked_even object END ******************************/

/* .sliced() and sliced object **********************************************/

static PyTypeObject sliced_type;

typedef struct _slicedobject {
    PyObject_HEAD;
    PyObject *iterable;
    PyObject *(*func)(struct _slicedobject *);
    Py_ssize_t n;
    Py_ssize_t idx;
    int strict;
    int done;
    union {
        binaryfunc getitem;
        struct {
            PyObject **items;
            PyObject *(*seqnew)(Py_ssize_t);
        } seq;
    } extra_info;
} slicedobject;

PyObject *
sliced_seq_iternext(slicedobject *o)
{
    PyObject *res, **dest, **src = o->extra_info.seq.items;
    Py_ssize_t i, n = Py_MIN(o->n, Py_SIZE(o->iterable) - o->idx);
    src += o->idx;

    res = o->extra_info.seq.seqnew(n);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    if (n) {
        dest = PyList_CheckExact(res) ?
               _PyList_ITEMS(res) :
               _PyTuple_ITEMS(res);

        for (i = 0; i < n; ++i) {
            dest[i] = src[i];
            Py_INCREF(dest[i]);
        }
    }

    return res;
}

PyObject *
sliced_generic_iternext(slicedobject *o)
{
    PyObject *slice = _PySlice_FromIndices(o->idx, o->idx + o->n);
    if (unlikely(slice == NULL)) {
        return NULL;
    }
    PyObject *res = o->extra_info.getitem(o->iterable, slice);
    Py_DECREF(slice);
    return res;
}

static PyObject *
fast_itertools_sliced(PyObject *self,
                       PyObject *const *args,
                       Py_ssize_t nargs,
                       PyObject *kwnames)
{
    PyObject *iterable = NULL;
    int iterable_given = 0;
    Py_ssize_t n;
    PyObject *n_arg;
    int n_given = 0;
    int strict = 0;
    int strict_given = 0;
    Py_ssize_t nkwargs = 0;
    Py_ssize_t kwname_length;
    PyASCIIObject *kwname;
    void *kwname_data;
    Py_ssize_t i;
    int err;
    PyObject **kwnames_items;
    Py_ssize_t largs = nargs;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs < 2 || largs > 3)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.sliced() takes from 2 to 3 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 3:
        strict_given = 1;
        strict = PyObject_IsTrue(args[2]);
        if (unlikely(strict < 0)) {
            goto error;
        }
    case 2:
        n_given = 1;
        n_arg = args[1];
        if (likely(PyLong_Check(n_arg))) {
            n = long_to_ssize(n_arg, &err);
            if (err) {
                goto overflow_error;
            }
        }
        else if (n_arg == Py_None) {
            if (unlikely(strict)) {
                PyErr_SetString(PyExc_ValueError,
                                "fast_itertools.sliced(): 'n' must not be "
                                "None when using strict mode");
                goto error;
            }
            n = PY_SSIZE_T_MAX;
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'n' argument for fast_itertools.sliced() must be "
                         "None or an integer (0 <= n <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    case 1:
        iterable_given = 1;
        iterable = args[0];
        break;
    }

    for (i = 0; i < nkwargs; i++) {
        kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length;
        kwname_data = PyUnicode_DATA(kwname);
        if (unlikely(
                kwname_length == 8 &&
                memcmp(kwname_data, "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.sliced() got multiple values "
                                "for argument 'iterable'");
                goto error;
            }
            iterable = args[nargs + i];
        }
        else if (likely(
                    kwname_length == 1 &&
                    *((char *)kwname_data) == 'n'
                 ))
        {
            if (unlikely(n_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.sliced() got multiple values "
                                "for argument 'n'");
                goto error;
            }
            n_arg = args[nargs + i];
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
            }
            else if (n_arg == Py_None) {
                n = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.sliced() must be "
                             "None or an integer (0 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else if (likely(
                    kwname_length == 6 &&
                    memcmp(kwname_data, "strict", 6) == 0
                 ))
        {
            if (unlikely(strict_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.sliced() got multiple values "
                                "for argument 'strict'");
                goto error;
            }
            PyObject *arg = args[nargs + i];
            if (n_arg == Py_None) {
                PyErr_SetString(PyExc_ValueError,
                                "fast_itertools.sliced(): 'n' must not be "
                                "None when using strict mode");
                goto error;
            }
            strict = PyObject_IsTrue(arg);
            if (unlikely(strict < 0)) {
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "fast_itertools.sliced() got an unexpected "
                         "keyword argument '%U'",
                         kwname);
            goto error;
        }
    }

    slicedobject *res = (slicedobject *)sliced_type.tp_alloc(&sliced_type, 0);
    if (unlikely(res == NULL)) {
        PyErr_NoMemory();
        goto error;
    }
    res->iterable = Py_NewRef(iterable);
    if (PyList_CheckExact(iterable)) {
        res->func = sliced_seq_iternext;
        res->extra_info.seq.items = _PyList_ITEMS(iterable);
        res->extra_info.seq.seqnew = PyList_New;
    }
    else if (PyTuple_CheckExact(iterable)) {
        res->func = sliced_seq_iternext;
        res->extra_info.seq.items = _PyTuple_ITEMS(iterable);
        res->extra_info.seq.seqnew = PyTuple_New;
    }
    else {
        PyMappingMethods *asmap = Py_TYPE(iterable)->tp_as_mapping;
        if (asmap && asmap->mp_subscript) {
            res->func = sliced_generic_iternext;
            res->extra_info.getitem = asmap->mp_subscript;
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                            "expected sliceable & sized for 'iterable' "
                            "argument of fast_itertools.sliced()");
            goto error;
        }
    }
    res->n = n;
    res->strict = strict;
    res->done = 0;
    return (PyObject *)res;
overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.sliced() must "
                 "be None or a positive number <= %zd",
                 PY_SSIZE_T_MAX);
  error:
    return NULL;
}

static void
sliced_dealloc(slicedobject *o)
{
    PyObject_GC_UnTrack(o);
    Py_XDECREF(o->iterable);
    Py_TYPE(o)->tp_free(o);
}

static int
sliced_traverse(slicedobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->iterable);
    return 0;
}

static PyObject *
sliced_next(slicedobject *o)
{
    if (unlikely(o->done)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    PyObject *res = o->func(o);
    if (unlikely(res == NULL)) {
        return NULL;
    }

    Py_ssize_t res_size =
        PyList_CheckExact(res) || PyTuple_CheckExact(res) ?
        Py_SIZE(res) :
        PySequence_Size(res);
    if (unlikely(res_size == -1)) {
        PyErr_Format(PyExc_ValueError,
                     "next(fast_itertools.sliced()): '%.200s' object has "
                     "no length",
                     Py_TYPE(res)->tp_name);
        goto error;
    }
    else if (unlikely(o->strict && res_size != o->n)) {
        PyErr_Format(PyExc_ValueError,
                     "next(fast_itertools.sliced()): iterable is not "
                     "divisible by %zd",
                     o->n);
        goto error;
    }

    Py_ssize_t length =
        PyList_CheckExact(o->iterable) || PyTuple_CheckExact(o->iterable) ?
        Py_SIZE(o->iterable) :
        PySequence_Size(o->iterable);
    if (unlikely((o->idx += o->n) >= length)) {
        o->done = 1;
    }
    return res;
  error:
    Py_DECREF(res);
    return NULL;
}

static PyTypeObject sliced_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.sliced([], 0).__class__",
    .tp_basicsize = sizeof(slicedobject),
    .tp_dealloc = (destructor)sliced_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_doc = "Base class for the result of fast_itertools.sliced()",
    .tp_traverse = (traverseproc)sliced_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)sliced_next,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
};

PyDoc_STRVAR(sliced__doc__,
"fast_itertools.sliced(iterable, n, strict=False)\n\
    Yield slices of length *n* from the sequence *seq*.\n\
\n\
    >>> list(sliced((1, 2, 3, 4, 5, 6), 3))\n\
    [(1, 2, 3), (4, 5, 6)]\n\
\n\
    By the default, the last yielded slice will have fewer than *n* elements\n\
    if the length of *seq* is not divisible by *n*:\n\
\n\
    >>> list(sliced((1, 2, 3, 4, 5, 6, 7, 8), 3))\n\
    [(1, 2, 3), (4, 5, 6), (7, 8)]\n\
\n\
    If the length of *seq* is not divisible by *n* and *strict* is\n\
    ``True``, then ``ValueError`` will be raised before the last\n\
    slice is yielded.\n\
\n\
    This function will only work for iterables that support slicing.\n\
    For non-sliceable iterables, see :func:`fast_itertools.chunked`.\n");

/* .sliced() and sliced object END ******************************************/

/* .constrained_batches() and constrained_batches object ********************/

static PyTypeObject conbatches_type;

typedef struct _conbatchesobject {
    PyObject_HEAD;
    PyObject *iterable;
    PyObject *nextitem;
    Py_ssize_t len_nextitem;
    Py_ssize_t max_size;
    Py_ssize_t max_count;
    Py_ssize_t batch_size;
    Py_ssize_t (*get_len)(struct _conbatchesobject *, PyObject *);
    PyObject *supplied_len;
    int strict;
    int done;
} conbatchesobject;

Py_ssize_t
len_default(conbatchesobject *Py_UNUSED(o), PyObject *el)
{
    Py_ssize_t n = get_len_info(el);
    if (unlikely(n < 0)) {
        PyErr_Format(PyExc_TypeError,
                     "object of type '%.200s' has no len()",
                     Py_TYPE(el)->tp_name);
        return -1;
    }
    return n;
}

Py_ssize_t
len_supplied(conbatchesobject *o, PyObject *el)
{
    PyObject *n_o = PyObject_CallOneArg(o->supplied_len, el);
    if (unlikely(n_o == NULL)) {
        return -1;
    }
    int err;
    Py_ssize_t n = long_to_ssize(n_o, &err);
    if (unlikely(n < 0)) {
        if (err) {
            PyErr_SetString(PyExc_OverflowError,
                            "overflow while getting length");
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "object of type '%.200s' has invalid length",
                         Py_TYPE(o)->tp_name);
        }
        return -1;
    }
    return n;
}

static PyObject *
fast_itertools_constrained_batches(PyObject *self,
                                   PyObject *const *args,
                                   Py_ssize_t nargs,
                                   PyObject *kwnames)
{
    PyObject *iterable = NULL;
    int iterable_given = 0;
    Py_ssize_t maxs, maxc = PY_SSIZE_T_MAX;
    PyObject *maxs_arg, *maxc_arg = NULL;
    int maxs_given = 0, maxc_given = 0;
    PyObject *getlen_arg = NULL;
    int getlen_given = 0;
    int strict = 1;
    int strict_given = 0;
    Py_ssize_t nkwargs = 0;
    Py_ssize_t kwname_length;
    PyASCIIObject *kwname;
    void *kwname_data;
    Py_ssize_t i;
    int err;
    PyObject **kwnames_items;
    Py_ssize_t largs = nargs;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs < 2 || largs > 5)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.constrained_batches() takes from 2 to 5 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 5:
        strict_given = 1;
        strict = PyObject_IsTrue(args[4]);
        if (unlikely(strict < 0)) {
            goto error;
        }
    case 4:
        getlen_given = 1;
        getlen_arg = args[3];
        if (unlikely(!PyCallable_Check(getlen_arg))) {
        PyErr_Format(PyExc_TypeError,
                     "'get_len' argument for "
                     "fast_itertools.constrained_batches() is not a callable",
                     largs);
            goto error;
        }
    case 3:
        maxc_given = 1;
        maxc_arg = args[2];
        if (likely(PyLong_Check(maxc_arg))) {
            maxc = long_to_ssize(maxc_arg, &err);
            if (err) {
                PyErr_Format(PyExc_ValueError,
                             "'max_count' argument for "
                             "fast_itertools.constrained_batches() "
                             "must be None or an integer "
                             "(0 <= max_count <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else if (maxc_arg == Py_None) {
            maxc = PY_SSIZE_T_MAX;
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'max_count' argument for "
                         "fast_itertools.constrained_batches() "
                         "must be None or an integer (0 <= max_count <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    case 2:
        maxs_given = 1;
        maxs_arg = args[1];
        if (likely(PyLong_Check(maxs_arg))) {
            maxs = long_to_ssize(maxs_arg, &err);
            if (err || maxs == 0) {
                PyErr_Format(PyExc_ValueError,
                             "'max_size' argument for "
                             "fast_itertools.constrained_batches() must be "
                             "an integer (1 <= max_size <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'max_size' argument for "
                         "fast_itertools.constrained_batches() must be "
                         "an integer (1 <= max_size <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    case 1:
        iterable_given = 1;
        iterable = PyObject_GetIter(args[0]);
        if (unlikely(iterable == NULL)) {
            goto error;
        }
        break;
    }

    for (i = 0; i < nkwargs; i++) {
        kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length;
        kwname_data = PyUnicode_DATA(kwname);
        if (unlikely(
                kwname_length == 8 &&
                memcmp(kwname_data, "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.constrained_batches() got "
                                "multiple values for argument 'iterable'");
                goto error;
            }
            iterable = PyObject_GetIter(args[nargs + i]);
            if (unlikely(iterable == NULL)) {
                goto error;
            }
        }
        else if (
                kwname_length == 8 &&
                memcmp(kwname_data, "max_size", 8) == 0
            )
        {
            if (unlikely(maxs_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.constrained_batches() got "
                                "multiple values for argument 'max_size'");
                goto error;
            }
            maxs_arg = args[nargs + i];
            if (likely(PyLong_Check(maxs_arg))) {
                maxs = long_to_ssize(maxs_arg, &err);
                if (err || maxs == 0) {
                    PyErr_Format(PyExc_ValueError,
                                 "'max_size' argument for "
                                 "fast_itertools.constrained_batches() must be "
                                 "an integer (1 <= max_size <= %zd)",
                                 PY_SSIZE_T_MAX);
                    goto error;
                }
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'max_size' argument for "
                             "fast_itertools.constrained_batches() must be "
                             "an integer (1 <= max_size <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else if (likely(
                kwname_length == 9 &&
                memcmp(kwname_data, "max_count", 9) == 0
            ))
        {
            if (unlikely(maxc_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.constrained_batches() got "
                                "multiple values for argument 'max_count'");
                goto error;
            }
            maxc_arg = args[nargs + i];
            if (likely(PyLong_Check(maxc_arg))) {
                maxc = long_to_ssize(maxc_arg, &err);
                if (err) {
                    PyErr_Format(PyExc_ValueError,
                                 "'max_count' argument for "
                                 "fast_itertools.constrained_batches() "
                                 "must be None or an integer "
                                 "(0 <= max_count <= %zd)",
                                 PY_SSIZE_T_MAX);
                    goto error;
                }
            }
            else if (maxc_arg == Py_None) {
                maxc = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'max_count' argument for "
                             "fast_itertools.constrained_batches() "
                             "must be None or an integer (0 <= max_count <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else if (likely(
                kwname_length == 7 &&
                memcmp(kwname_data, "get_len", 7) == 0
            ))
        {
            if (unlikely(getlen_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.constrained_batches() got "
                                "multiple values for argument 'get_len'");
                goto error;
            }
            getlen_arg = args[nargs + i];
            if (unlikely(!PyCallable_Check(getlen_arg))) {
            PyErr_Format(PyExc_TypeError,
                         "'get_len' argument for "
                         "fast_itertools.constrained_batches() is not a callable",
                         largs);
                goto error;
            }
        }
        else if (likely(
                    kwname_length == 6 &&
                    memcmp(kwname_data, "strict", 6) == 0
                 ))
        {
            if (unlikely(strict_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.constrained_batches() got "
                                "multiple values for argument 'strict'");
                goto error;
            }
            strict = PyObject_IsTrue(args[nargs + i]);
            if (unlikely(strict < 0)) {
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "fast_itertools.constrained_batches() got an "
                         "unexpected keyword argument '%U'",
                         kwname);
            goto error;
        }
    }

    conbatchesobject *res =
        (conbatchesobject *)conbatches_type.tp_alloc(&conbatches_type, 0);
    if (unlikely(res == NULL)) {
        PyErr_NoMemory();
        goto error;
    }
    res->iterable = iterable;
    res->nextitem = NULL;
    res->max_size = maxs;
    res->max_count = maxc;
    res->batch_size = maxc_arg ? maxc : maxs;
    if (getlen_arg) {
        res->get_len = len_supplied;
        res->supplied_len = Py_NewRef(getlen_arg);
    }
    else {
        res->get_len = len_default;
        res->supplied_len = NULL;
    }
    res->strict = strict;
    res->done = 0;
    return (PyObject *)res;
  error:
    Py_XDECREF(iterable);
    return NULL;
}

static void
conbatches_dealloc(conbatchesobject *o)
{
    PyObject_GC_UnTrack(o);
    Py_XDECREF(o->iterable);
    Py_XDECREF(o->supplied_len);
    Py_TYPE(o)->tp_free(o);
}

static int
conbatches_traverse(conbatchesobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->iterable);
    if (o->supplied_len) {
        Py_VISIT(o->supplied_len);
    }
    return 0;
}

static PyObject *
conbatches_next(conbatchesobject *o)
{
    PyObject *item, *iterable = o->iterable, **items;
    Py_ssize_t m = o->max_size, n = o->batch_size;
    int done = o->done;
    Py_ssize_t i, j;
    Py_ssize_t (*len)(struct _conbatchesobject *, PyObject *) = o->get_len;

    if (unlikely(o->done)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    PyObject *res = PyTuple_New(n);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    items = _PyTuple_ITEMS(res);
    if (o->nextitem) {
        items[0] = o->nextitem;
        i = 1;
        j = o->len_nextitem;
        o->nextitem = NULL;
        o->len_nextitem = 0;
    }
    else {
        i = 0;
        j = 0;
    }
    iternextfunc it = Py_TYPE(iterable)->tp_iternext;
    for (; i < n; i++) {
        PyObject *item = it(iterable);
        if (unlikely(item == NULL)) {
            PyThreadState *tstate = _PyThreadState_GET();
            PyObject *exc;
            if ((exc = _PyErr_Occurred(tstate)) == NULL || exc &&
                PyErr_GivenExceptionMatches(exc, PyExc_StopIteration))
            {
                o->done = 1;
                if (i > 0) {
                    if (unlikely(_PyTuple_Resize(&res, i) == -1)) {
                        return NULL;
                    }
                    _PyErr_Restore(tstate, NULL, NULL, NULL);
                    break;
                }
            }
            goto error;
        }
        Py_ssize_t length = len(o, item);
        if (unlikely(length == -1)) {
            goto error;
        }
        if (o->strict && length > m) {
            PyErr_SetString(PyExc_ValueError,
                            "next(fast_itertools.constrained_batches()): "
                            "item size exceeds maximum size");
            goto error;
        }
        j += length;
        if (unlikely(i && (i == n || j > m))) {
            o->nextitem = item;
            o->len_nextitem = length;
            if (unlikely(_PyTuple_Resize(&res, i) == -1)) {
                return NULL;
            }
            break;
        }
        items[i] = item;
    }
    return res;
  error:
    Py_DECREF(res);
    return NULL;
}

static PyTypeObject conbatches_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.constrained_batches([], 0).__class__",
    .tp_basicsize = sizeof(conbatchesobject),
    .tp_dealloc = (destructor)conbatches_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_doc = "Base class for the result of fast_itertools.constrained_batches()",
    .tp_traverse = (traverseproc)conbatches_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)conbatches_next,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = PyType_GenericNew,
    .tp_free = PyObject_GC_Del,
};

PyDoc_STRVAR(constrained_batches__doc__,
"constrained_batches(iterable, max_size, max_count=None, get_len=len, strict=True)\n\
    Yield batches of items from *iterable* with a combined size limited by\n\
    *max_size*.\n\
\n\
    >>> from fast_itertools import constrained_batches\n\
    >>> iterable = [b'12345', b'123', b'12345678', b'1', b'1', b'12', b'1']\n\
    >>> list(constrained_batches(iterable, 10))\n\
    [(b'12345', b'123'), (b'12345678', b'1', b'1'), (b'12', b'1')]\n\
\n\
    If a *max_count* is supplied, the number of items per batch is also\n\
    limited:\n\
\n\
    >>> iterable = [b'12345', b'123', b'12345678', b'1', b'1', b'12', b'1']\n\
    >>> list(constrained_batches(iterable, 10, max_count = 2))\n\
    [(b'12345', b'123'), (b'12345678', b'1'), (b'1', b'12'), (b'1',)]\n\
\n\
    If a *get_len* function is supplied, use that instead of :func:`len` to\n\
    determine item size.\n\
\n\
    If *strict* is ``True``, raise ``ValueError`` if any single item is bigger\n\
    than *max_size*. Otherwise, allow single items to exceed *max_size*.\n");

/* .constrained_batches() and constrained_batches object END ****************/

/* .distribute() and distributed object *************************************/

static PyTypeObject distributed_type;

typedef struct _distobj distributedobject;

struct _distributed_state {
    PyObject *iterable;
    Py_ssize_t i; /* distobj idx where next item of iterable belongs to */
    Py_ssize_t n; /* num of distobjs */
    struct _distobj **objs;
};

typedef struct _distobj {
    PyObject_HEAD;
    int done;
    Py_ssize_t id;
    struct _distributed_state *state;
    Py_ssize_t alloc_cache;
    Py_ssize_t len_cache;
    Py_ssize_t li;
    Py_ssize_t ri;
    PyObject **cache; /* dynamic circular queue */
} distributedobject;

static void
distobj_dealloc(distributedobject *o)
{
    Py_ssize_t i, n = o->state->n;
    Py_ssize_t actives = 0;
    for (i = 0; i < o->id; ++i) {
        actives += o->state->objs[i] != NULL;
    }
    while (++i < n) {
        actives += o->state->objs[i] != NULL;
    }
    if (actives) {
        o->state->objs[o->id] = NULL;
    }
    else {
        Py_XDECREF(o->state->iterable);
        PyMem_Free(o->state->objs);
        PyMem_Free(o->state);
    }
    if (o->len_cache) {
        i = o->li;
        if (o->ri >= o->li) {
            for (; i <= o->ri; ++i) {
                Py_DECREF(o->cache[i]);
            }
        }
        else {
            for (; i < o->alloc_cache; ++i) {
                Py_DECREF(o->cache[i]);
            }
            for (i = 0; i <= o->ri; ++i) {
                Py_DECREF(o->cache[i]);
            }
        }
        PyMem_Free(o->cache);
    }
    PyObject_GC_UnTrack(o);
    Py_TYPE(o)->tp_free(o);
}

static int
distobj_traverse(distributedobject *o, visitproc visit, void *arg)
{
    Py_VISIT(o->state->iterable);
    return 0;
}

Py_LOCAL_INLINE(int)
cache_resize(distributedobject *o, Py_ssize_t newsize)
{
    /* Follows the Python list growth pattern. */
    size_t new_alloc;
    PyObject **items;
    int compress = 0;

    if (o->alloc_cache >= newsize) {
        if (newsize >= (o->alloc_cache >> 2)) {
            o->len_cache = newsize;
            return 0;
        }
        compress = 1;
    }

    if (unlikely(newsize == 0)) {
        new_alloc = 0;
    }
    else {
        new_alloc = ((size_t)newsize + (newsize >> 3) + 6) & ~(size_t)3;
        if (newsize - o->len_cache > (Py_ssize_t)(new_alloc - newsize)) {
            new_alloc = ((size_t)newsize + 3) & ~(size_t)3;
        }
    }
    if (new_alloc <= (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *)) {
        if (new_alloc == o->alloc_cache) {
            o->len_cache = newsize;
            return 0;
        }
        if (new_alloc && o->cache) {
            /* Compress/expand the cache. */
            if (o->li) {
                if (o->ri >= o->li) {
                    o->ri -= o->li;
                    memmove(o->cache,
                            o->cache + o->li,
                            (o->ri + 1) * sizeof(PyObject *));
                    o->li = 0;
                }
                else if (compress) {
                    if (o->ri < o->li - 1) {
                        memmove(o->cache + o->ri + 1,
                                o->cache + o->li,
                                (o->alloc_cache - o->li) * sizeof(PyObject *));
                        o->li = o->ri + 1;
                    }
                }
                else {
                    Py_ssize_t prev_li = o->li;
                    o->li += new_alloc - o->alloc_cache;
                    memmove(o->cache + o->li,
                            o->cache + prev_li,
                            (o->alloc_cache - prev_li) * sizeof(PyObject *));
                }
            }
            items = PyMem_Realloc(o->cache, new_alloc * sizeof(PyObject *));
        }
        else {
            items = PyMem_Malloc(new_alloc * sizeof(PyObject *));
        }
    }
    else {
        items = NULL;
    }
    if (unlikely(items == NULL)) {
        return 1;
    }

    o->cache = items;
    o->len_cache = newsize;
    o->alloc_cache = new_alloc;
    return 0;
}

Py_LOCAL_INLINE(int)
cache_pop(distributedobject *o, PyObject **item)
{
    if (o->len_cache) {
        Py_ssize_t prev_li = o->li;
        *item = o->cache[o->li++];
        if (o->li == o->alloc_cache) {
            o->li = 0;
        }
        assert(o->li < o->alloc_cache);
        if (cache_resize(o, o->len_cache - 1)) {
            o->li = prev_li;
            *item = NULL;
            return 1;
        }
        return 0;
    }
    *item = NULL;
    return 0;
}

Py_LOCAL_INLINE(int)
cache_push(distributedobject *o, PyObject *item)
{
    if (cache_resize(o, o->len_cache + 1)) {
        return 1;
    }
    if (o->ri == o->alloc_cache) {
        o->ri = 0;
    }
    assert(o->ri < o->alloc_cache);
    o->cache[o->ri++] = item;
    return 0;
}

PyObject *
distobj_next(distributedobject *o)
{
    PyObject *item;

    if (unlikely(o->done)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (cache_pop(o, &item)) {
        PyErr_NoMemory();
        return NULL;
    }
    else if (item) {
        return item;
    }

    struct _distributed_state *s = o->state;

    Py_ssize_t i = s->i;
    PyObject *it = s->iterable;

    if (unlikely(it == NULL)) {
        o->done = 1;
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    iternextfunc func = Py_TYPE(it)->tp_iternext;

#define GET_ITEM \
    item = func(it); \
    if (unlikely(item == NULL)) { \
        PyThreadState *tstate = _PyThreadState_GET(); \
        PyObject *exc; \
        if ((exc = _PyErr_Occurred(tstate)) == NULL || exc && \
            PyErr_GivenExceptionMatches(exc, PyExc_StopIteration)) \
        { \
            o->done = 1; \
            Py_DECREF(s->iterable); \
            s->iterable = NULL; \
        } \
        return NULL; \
    } \

    if (i > o->id) {
        for (; i < s->n; ++i) {
            GET_ITEM
            distributedobject *x = s->objs[i];
            if (unlikely(x == NULL)) {
                Py_DECREF(item);
                continue;
            }
            if (cache_push(x, item)) {
                Py_DECREF(item);
                PyErr_NoMemory();
                return NULL;
            }
        }
        i = 0;
    }

    for (; i < o->id; ++i) {
        GET_ITEM
        distributedobject *x = s->objs[i];
        if (unlikely(x == NULL)) {
            Py_DECREF(item);
            continue;
        }
        if (cache_push(x, item)) {
            Py_DECREF(item);
            PyErr_NoMemory();
            return NULL;
        }
    }

    GET_ITEM
    s->i = (o->id + 1) % s->n;
    return item;
#undef GET_ITEM
}

static PyTypeObject distributed_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fast_itertools.distribute(1, [])[0].__class__",
    .tp_basicsize = sizeof(distributedobject),
    .tp_dealloc = (destructor)distobj_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE |
                    Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_doc = "Base class for the iterators in the result of "
              "fast_itertools.distribute()",
    .tp_traverse = (traverseproc)distobj_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)distobj_next,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = NULL,
    .tp_free = PyObject_GC_Del,
};

static PyObject *
fast_itertools_distribute(PyObject *self,
                          PyObject *const *args,
                          Py_ssize_t nargs,
                          PyObject *kwnames)
{
    PyObject *res = NULL, *iterable = NULL;
    int iterable_given = 0;
    Py_ssize_t n;
    PyObject *n_arg;
    int n_given = 0;
    struct _distributed_state *global_state = NULL;
    Py_ssize_t nkwargs = 0;
    Py_ssize_t kwname_length;
    PyASCIIObject *kwname;
    void *kwname_data;
    Py_ssize_t i;
    int err;
    PyObject **kwnames_items;
    Py_ssize_t largs = nargs;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs != 2)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.distribute() takes 2 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 2:
        iterable_given = 1;
        iterable = PyObject_GetIter(args[1]);
        if (unlikely(iterable == NULL)) {
            goto error;
        }
    case 1:
        n_given = 1;
        n_arg = args[0];
        if (likely(PyLong_Check(n_arg))) {
            n = long_to_ssize(n_arg, &err);
            if (err) {
                goto overflow_error;
            }
            if (unlikely(n < 1)) {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.distribute() "
                             "must be an integer (1 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'n' argument for fast_itertools.distribute() must "
                         "be an integer (1 <= n <= %zd)",
                         PY_SSIZE_T_MAX);
            goto error;
        }
    }

    for (i = 0; i < nkwargs; i++) {
        kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length;
        kwname_data = PyUnicode_DATA(kwname);
        if (unlikely(
                kwname_length == 8 &&
                memcmp(kwname_data, "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.chunked() got multiple values "
                                "for argument 'iterable'");
                goto error;
            }
            iterable = PyObject_GetIter(args[nargs + i]);
            if (unlikely(iterable == NULL)) {
                goto error;
            }
        }
        else if (likely(
                    kwname_length == 1 &&
                    *((char *)kwname_data) == 'n'
                 ))
        {
            if (unlikely(n_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.distribute() got multiple values "
                                "for argument 'n'");
                goto error;
            }
            n_arg = args[nargs + i];
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
                if (unlikely(n < 1)) {
                    PyErr_Format(PyExc_ValueError,
                                 "'n' argument for fast_itertools.distribute() "
                                 "must be an integer (1 <= n <= %zd)",
                                 PY_SSIZE_T_MAX);
                    goto error;
                }
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.distribute() must "
                             "be an integer (1 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "fast_itertools.distribute() got an unexpected "
                         "keyword argument '%U'",
                         kwname);
            goto error;
        }
    }

    res = PyList_New(n);
    if (unlikely(res == NULL)) {
        PyErr_NoMemory();
        goto error;
    }
    PyObject **items = _PyList_ITEMS(res);

    global_state = PyMem_Malloc(sizeof *global_state);
    if (unlikely(global_state == NULL)) {
        PyErr_NoMemory();
        goto error;
    }
    global_state->iterable = iterable;
    global_state->i = 0;
    global_state->n = n;
    global_state->objs = PyMem_Malloc(n * sizeof(distributedobject *));
    if (unlikely(global_state->objs == NULL)) {
        PyErr_NoMemory();
        goto error;
    }

    for (i = 0; i < n; ++i) {
        distributedobject *item =
            (distributedobject *)distributed_type.tp_alloc(&distributed_type, 0);
        if (unlikely(item == NULL)) {
            PyErr_NoMemory();
            goto error;
        }
        item->done = 0;
        item->id = i;
        item->state = global_state;
        item->alloc_cache = 0;
        item->len_cache = 0;
        item->li = 0;
        item->ri = 0;
        item->cache = NULL;
        global_state->objs[i] = item; /* Steal the reference to avoid
                                         memory leaking after the list is
                                         dealloc'd */
        items[i] = (PyObject *)item;
    }

    return res;
overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.distribute() must "
                 "be a positive number <= %zd and >= 1",
                 PY_SSIZE_T_MAX);
  error:
    Py_XDECREF(iterable);
    if (global_state) {
        if (global_state->objs) {
            PyMem_Free(global_state->objs);
        }
        PyMem_Free(global_state);
    }
    Py_XDECREF(res);
    return NULL;
}

PyDoc_STRVAR(distribute__doc__,
"distribute(n, iterable)\n\
    Distribute the items from *iterable* among *n* smaller iterables.\n\
\n\
        >>> from fast_itertools import distribute\n\
        >>> group_1, group_2 = distribute(2, [1, 2, 3, 4, 5, 6])\n\
        >>> list(group_1)\n\
        [1, 3, 5]\n\
        >>> list(group_2)\n\
        [2, 4, 6]\n\
\n\
    If the length of *iterable* is not evenly divisible by *n*, then the\n\
    length of the returned iterables will not be identical:\n\
\n\
        >>> children = distribute(3, [1, 2, 3, 4, 5, 6, 7])\n\
        >>> [list(c) for c in children]\n\
        [[1, 4, 7], [2, 5], [3, 6]]\n\
\n\
    If the length of *iterable* is smaller than *n*, then the last returned\n\
    iterables will be empty:\n\
\n\
        >>> children = distribute(5, [1, 2, 3])\n\
        >>> [list(c) for c in children]\n\
        [[1], [2], [3], [], []]\n\
\n\
    This function uses a cache for values and may require significant\n\
    storage. If you need the order items in the smaller iterables to match the\n\
    original iterable, see :func:`divide`.\n");

/* .distribute() and distributed object END *********************************/

/* .take() ******************************************************************/

static PyObject *
fast_itertools_take(PyObject *self,
                        PyObject *const *args,
                        Py_ssize_t nargs,
                        PyObject *kwnames)
{
    PyObject *iterable = NULL, *res = NULL, *n_arg, **kwnames_items, **res_items;
    PyObject *tmp, *diff, *endcalc;
    PyTypeObject *iterabletype = NULL;
    PyASCIIObject *kwname;
    PyBufferProcs *pb;
    void *kwname_data;
    int iterable_given = 0, n_given = 0, is_list = 0;
    int cmp0, cmp1;
    Py_ssize_t i, j, n, kwname_length, nkwargs = 0, largs = nargs;
    int err;
    size_t size;

    if (kwnames) {
        largs += (nkwargs = PyTuple_GET_SIZE(kwnames));
        kwnames_items = ((PyTupleObject *)kwnames)->ob_item;
    }
    if (unlikely(largs < 2 || largs > 2)) {
        PyErr_Format(PyExc_TypeError,
                     "fast_itertools.take() takes 2 "
                     "arguments (given %zd)",
                     largs);
        goto error;
    }

    switch (nargs) {
    case 2:
        n_given = 1;
        n_arg = args[1];
        if (likely(PyLong_Check(n_arg))) {
            n = long_to_ssize(n_arg, &err);
            if (err) {
                goto overflow_error;
            }
        }
        else if (n_arg == Py_None) {
            n = PY_SSIZE_T_MAX;
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'n' argument for fast_itertools.take() must be "
                         "None or an integer (0 <= n <= %zd)",
                         PY_SSIZE_T_MAX);
            goto overflow_error;
        }
    case 1:
        iterable_given = 1;
        iterable = args[0];
        break;
    }

    for (i = 0; i < nkwargs; i++) {
        if (unlikely(
                (kwname_length = (kwname = (PyASCIIObject *)kwnames_items[i])->length) == 8 &&
                memcmp((kwname_data = PyUnicode_DATA(kwname)), "iterable", 8) == 0
            ))
        {
            if (unlikely(iterable_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.take() got multiple values "
                                "for argument 'iterable'");
                goto error;
            }
            iterable = args[nargs + i];
        }
        else if (likely(
                    kwname_length == 1 &&
                    *((char *)kwname_data) == 'n'
                 ))
        {
            if (unlikely(n_given == 1)) {
                PyErr_SetString(PyExc_TypeError,
                                "fast_itertools.take() got multiple values "
                                "for argument 'n'");
                goto error;
            }
            n_arg = args[nargs + i];
            if (likely(PyLong_Check(n_arg))) {
                n = long_to_ssize(n_arg, &err);
                if (err) {
                    goto overflow_error;
                }
            }
            else if (n_arg == Py_None) {
                n = PY_SSIZE_T_MAX;
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "'n' argument for fast_itertools.take() must be "
                             "None or an integer (0 <= n <= %zd)",
                             PY_SSIZE_T_MAX);
                goto error;
            }
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "fast_itertools.take() got an unexpected "
                         "keyword argument '%U'",
                         kwname);
            goto error;
        }
    }
    i = j = 0;
#define RES_INIT(x) \
    n = Py_MIN(n, (x)); \
    res = PyList_New(n); \
    if (unlikely(res == NULL)) { \
        goto error; \
    } \
    else if (unlikely(n == 0)) { \
        return res; \
    } \
    res_items = ((PyListObject *)res)->ob_item;
    if (likely((iterabletype = Py_TYPE(iterable)) == list_type || iterabletype == tuple_type)) {
        RES_INIT(Py_SIZE(iterable));
        PyObject **iterable_items = ((PyListObject *)iterable)->ob_item;
        for (; likely(i < n); ++i) {
            Py_XINCREF(iterable_items[i]);
            res_items[i] = iterable_items[i];
        }
    }
    else if (iterabletype == unicode_type) {
#define CASE_ALGO(bytesize) \
    case PyUnicode_ ## bytesize ## BYTE_KIND: { \
        Py_UCS ## bytesize *data = PyUnicode_ ## bytesize ## BYTE_DATA(iterable); \
        for (; likely(i < n); ++i) { \
            res_items[i] = unicode_char##bytesize(data[i]); \
            Py_INCREF(res_items[i]); \
        } \
        break; \
    }
        RES_INIT(((PyASCIIObject *)iterable)->length);
        switch (PyUnicode_KIND(iterable)) {
            CASE_ALGO(1)
            CASE_ALGO(2)
            CASE_ALGO(4)
        }
#undef CASE_ALGO
    }
    else if (iterabletype == bytes_type) {
        RES_INIT(Py_SIZE(iterable))
        char *ob_sval = ((PyBytesObject *)iterable)->ob_sval;
        for (; likely(i < n); ++i) {
            res_items[i] = uchar_longs[ob_sval[i]];
            Py_INCREF(res_items[i]);
        }
    }
    else if (iterabletype == bytearray_type) {
        RES_INIT(Py_SIZE(iterable))
        if (likely(n != 0)) {
            char *ob_start = ((PyByteArrayObject *)iterable)->ob_start;
            for (; likely(i < n); ++i) {
                res_items[i] = uchar_longs[ob_start[i]];
                Py_INCREF(res_items[i]);
            }
        }
    }
    else if (iterabletype == range_type) {
        PyObject *ostart = ((rangeobject *)iterable)->start;
        PyObject *ostop = ((rangeobject *)iterable)->stop;
        PyObject *ostep = ((rangeobject *)iterable)->step;
        Py_ssize_t lstart, lstop, lstep;
        int ispos = 1;
        lstart = PyLong_AsSsize_t(ostart);
        if (unlikely(lstart == -1 && PyErr_Occurred())) {
            PyErr_Clear();
            goto long_range;
        }
        lstop = PyLong_AsSsize_t(ostop);
        if (unlikely(lstop == -1 && PyErr_Occurred())) {
            PyErr_Clear();
            goto long_range;
        }
        lstep = PyLong_AsSsize_t(ostep);
        if (unlikely(lstep == -1 && PyErr_Occurred())) {
            PyErr_Clear();
            goto long_range;
        }
        size = 0;
        if (likely(lstep > 0 && lstart < lstop)) {
            size = (size_t)1 + (lstop - (size_t)1 - lstart) / lstep;
        }
        else if (lstep < 0 && lstart > lstop) {
            ispos = 0;
            size = (size_t)1 + (lstart - (size_t)1 - lstop) / ((size_t)-lstep);
        }
        else {
            return PyList_New(0);
        }
        if (unlikely(size > (size_t)PY_SSIZE_T_MAX)) {
            PyErr_NoMemory();
            goto error;
        }
        RES_INIT(size);
        for (i = 0; likely(i < n); lstart += lstep, i++) {
            if (unlikely((res_items[i] = PyLong_FromSsize_t(lstart)) == NULL))
            {
                goto error;
            }
        }
        goto finn;
  long_range:
        size = 0;
        cmp0 = PyObject_RichCompareBool(ostep, uchar_longs[0], Py_GT);
        if (unlikely(cmp0 == -1)) {
            goto error;
        }
        cmp1 = PyObject_RichCompareBool(ostart, ostop, Py_LE);
        if (unlikely(cmp1 == -1)) {
            goto error;
        }
        if (likely(cmp0 && cmp1)) {
            tmp = PyLong_Type.tp_as_number->nb_subtract(ostop, ostart);
            if (unlikely(tmp == NULL)) {
                goto error;
            }
            diff = PyLong_Type.tp_as_number->nb_subtract(tmp, uchar_longs[1]);
            Py_DECREF(tmp);
            if (unlikely(diff == NULL)) {
                goto error;
            }
            endcalc = PyLong_Type.tp_as_number->nb_floor_divide(diff, ostep);
            Py_DECREF(diff);
            if (unlikely(endcalc == NULL)) {
                goto error;
            }
            size = (size_t)PyLong_AsSsize_t(endcalc);
            if (unlikely(size == (size_t)-1 && PyErr_Occurred())) {
                goto error;
            }
            else if (unlikely(size == PY_SSIZE_T_MAX)) {
                PyErr_NoMemory();
                goto error;
            }
            else if (unlikely(size == (size_t)-1)) {
                return PyList_New(0);
            }
        }
        else {
            tmp = PyLong_Type.tp_as_number->nb_subtract(ostart, ostop);
            if (unlikely(tmp == NULL)) {
                goto error;
            }
            diff = PyLong_Type.tp_as_number->nb_subtract(tmp, uchar_longs[1]);
            Py_DECREF(tmp);
            if (unlikely(diff == NULL)) {
                goto error;
            }
            tmp = PyLong_Type.tp_as_number->nb_negative(ostep);
            if (unlikely(tmp == NULL)) {
                goto error;
            }
            endcalc = PyLong_Type.tp_as_number->nb_floor_divide(diff, tmp);
            Py_DECREF(diff);
            Py_DECREF(tmp);
            if (unlikely(endcalc == NULL)) {
                goto error;
            }
            size = (size_t)PyLong_AsSsize_t(endcalc);
            if (unlikely(size == (size_t)-1 && PyErr_Occurred())) {
                goto error;
            }
            else if (unlikely(size == PY_SSIZE_T_MAX)) {
                PyErr_NoMemory();
                goto error;
            }
        }
        RES_INIT(size + 1);
        binaryfunc func = PyLong_Type.tp_as_number->nb_add;
        i = 0;
        while (likely(i < n)) {
            res_items[i++] = ostart;
            if (unlikely((ostart = func(ostart, ostep)) == NULL)) {
                goto error;
            }
        }
    }
    else if (iterabletype == set_type || iterabletype == frozenset_type) {
        RES_INIT(((PySetObject *)iterable)->mask);
        setentry *entry = ((PySetObject *)iterable)->table;
        PyObject *key;
        while (likely(j <= n)) {
            if ((key = entry[j++].key) != NULL && key != _PySet_Dummy) {
                res_items[i++] = key;
                Py_INCREF(key);
            }
        }
    }
    else if (iterabletype == dict_type) {
        PyDictValues *v = ((PyDictObject *)iterable)->ma_values;
        PyDictKeysObject *k = ((PyDictObject *)iterable)->ma_keys;
        if (v) {
            RES_INIT(((PyDictObject *)iterable)->ma_used);
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(k);
            while (likely(j < n)) {
                res_items[i] = entries[get_index_from_values_order(v, j++)].me_key;
                Py_INCREF(res_items[i++]);
            }
        }
        else {
#define ITER_ALGO \
    for (; likely(j < n); ++j) { \
        if (entry_ptr->me_value != NULL) { \
            res_items[i] = entry_ptr->me_key; \
            Py_INCREF(res_items[i++]); \
        } \
        ++entry_ptr; \
    }
            RES_INIT(k->dk_nentries);
            if (DK_IS_UNICODE(k)) {
                PyDictUnicodeEntry *entry_ptr = DK_UNICODE_ENTRIES(k);
                ITER_ALGO
            }
            else {
                PyDictKeyEntry *entry_ptr = DK_ENTRIES(k);
                ITER_ALGO
            }
#undef ITER_ALGO
        }
    }
    else if (iterabletype == long_type) {
        RES_INIT(Py_SIZE(iterable));
        digit *ob_digit = ((PyLongObject *)iterable)->ob_digit;
        for (; likely(j < n); ++j) {
            res_items[i] = PyLong_FromUnsignedLong(ob_digit[j]);
            if (unlikely(res_items[i++] == NULL)) {
                goto error;
            }
        }
    }
    else {
        res = PyList_New(n);
        if (unlikely(res == NULL)) {
            goto error;
        }
        res_items = ((PyListObject *)res)->ob_item;
        iterable = PyObject_GetIter(iterable);
        if (unlikely(iterable == NULL)) {
            goto error;
        }
        iternextfunc it = Py_TYPE(iterable)->tp_iternext;
        for (; likely(i < n); i++) {
            PyObject *item = it(iterable);
            if (unlikely(item == NULL)) {
                PyThreadState *tstate = _PyThreadState_GET();
                PyObject *exc;
                if ((exc = _PyErr_Occurred(tstate)) == NULL || exc &&
                    PyErr_GivenExceptionMatches(exc, PyExc_StopIteration))
                {
                    _PyErr_Restore(tstate, NULL, NULL, NULL);
                    Py_SET_SIZE(res, i);
                    break;
                }
                goto error;
            }
            res_items[i] = item;
        }
    }
  finn:
    return res;
  overflow_error:
    PyErr_Format(PyExc_OverflowError,
                 "'n' argument for fast_itertools.take() must be "
                 "None or a positive number <= %zd",
                 PY_SSIZE_T_MAX);
  error:
    Py_XDECREF(res);
    return NULL;
#undef RES_INIT
}

PyDoc_STRVAR(take__doc__,
"take(iterable, n)\n\
    Return first *n* items of the iterable as a list.\n\
\n\
        >>> import fast_itertools\n\
        >>> fast_itertools.take(range(10), 3)\n\
        [0, 1, 2]\n\
\n\
    If there are fewer than *n* items in the iterable, all of them are\n\
    returned.\n\
\n\
        >>> fast_itertools.take(range(3), 10)\n\
        [0, 1, 2]\n");

/* .take() END **************************************************************/

static PyMethodDef fast_itertools_methods[] = {
    {"chunked", (PyCFunction)fast_itertools_chunked, METH_FASTCALL | METH_KEYWORDS,
     chunked__doc__},
    {"ichunked", (PyCFunction)fast_itertools_ichunked, METH_FASTCALL | METH_KEYWORDS,
     ichunked__doc__},
    {"chunked_even", (PyCFunction)fast_itertools_chunked_even, METH_FASTCALL | METH_KEYWORDS,
     chunked_even__doc__},
    {"sliced", (PyCFunction)fast_itertools_sliced, METH_FASTCALL | METH_KEYWORDS,
     sliced__doc__},
    {"constrained_batches", (PyCFunction)fast_itertools_constrained_batches, METH_FASTCALL | METH_KEYWORDS,
     constrained_batches__doc__},
    {"distribute", (PyCFunction)fast_itertools_distribute, METH_FASTCALL | METH_KEYWORDS,
     distribute__doc__},
    {"take", (PyCFunction)fast_itertools_take, METH_FASTCALL | METH_KEYWORDS,
     take__doc__},
    {NULL}
};

static struct PyModuleDef fast_itertoolsmodule = {
    PyModuleDef_HEAD_INIT,
    "fast_itertools",
    "More (faster) routines for operating on iterables, beyond itertools",
    0,
    fast_itertools_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_fast_itertools(void)
{
    Py_ssize_t i = 0;
    for (i = 0; i < 128; ++i) {
        strings1[i] = (PyObject *)&_Py_SINGLETON(strings).ascii[i];
    }
    for (i = 0; i < 128; ++i) {
        strings1[i + 128] = (PyObject *)&_Py_SINGLETON(strings).latin1[i];
    }
    for (i = 0; i < 256; ++i) {
        uchar_longs[i] = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + i];
    }
    list_type = &PyList_Type;
    tuple_type = &PyTuple_Type;
    unicode_type = &PyUnicode_Type;
    bytes_type = &PyBytes_Type;
    bytearray_type = &PyByteArray_Type;
    range_type = &PyRange_Type;
    enum_type = &PyEnum_Type;
    set_type = &PySet_Type;
    frozenset_type = &PyFrozenSet_Type;
    dict_type = &PyDict_Type;
    long_type = &PyLong_Type;
    PyObject *m = PyModule_Create(&fast_itertoolsmodule);
    if (unlikely(PyModule_AddType(m, &ichunk_type) < 0)) {
        return NULL;
    }
    return m;
}
