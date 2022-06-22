#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define Py_BUILD_CORE
#include "internal/pycore_pyerrors.h"
#include "internal/pycore_pystate.h"
#undef Py_BUILD_CORE

/* utilities ****************************************************************/

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

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
    size_t n_temp;
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
            Py_ssize_t long_arg_size = Py_SIZE(n_arg);
            digit *ob_digit;
            switch (long_arg_size) {
            case 0:
                n = 0;
                break;
            case 1:
                n = ((PyLongObject *)n_arg)->ob_digit[0];
                break;
            case 2:
                ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                n = ob_digit[0] | (ob_digit[1] << PyLong_SHIFT);
                break;
            case 3:
                ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                n_temp = (ob_digit[0]
                          | ((ob_digit[1] | (ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
                if (unlikely(n_temp > (size_t)PY_SSIZE_T_MAX)) {
                    PyErr_Format(PyExc_OverflowError,
                                 "'n' argument for fast_itertools.chunked() must be "
                                 "None or a positive number <= %zd",
                                 PY_SSIZE_T_MAX);
                    goto error;
                }
                n = Py_SAFE_DOWNCAST(n_temp, size_t, Py_ssize_t);
                break;
            default:
                PyErr_Format(PyExc_OverflowError,
                             "'n' argument for fast_itertools.chunked() must be "
                             "None or a positive number <= %zd",
                             PY_SSIZE_T_MAX);
                goto error;
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
                Py_ssize_t long_arg_size = Py_SIZE(n_arg);
                digit *ob_digit;
                switch (long_arg_size) {
                case 0:
                    n = 0;
                    break;
                case 1:
                    n = ((PyLongObject *)n_arg)->ob_digit[0];
                    break;
                case 2:
                    ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                    n = ob_digit[0] | (ob_digit[1] << PyLong_SHIFT);
                    break;
                case 3:
                    ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                    n_temp = (ob_digit[0]
                              | ((ob_digit[1] | (ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
                    if (unlikely(n_temp > (size_t)PY_SSIZE_T_MAX)) {
                        PyErr_Format(PyExc_OverflowError,
                                     "'n' argument for fast_itertools.chunked() must be "
                                     "None or a positive number <= %zd",
                                     PY_SSIZE_T_MAX);
                        goto error;
                    }
                    n = Py_SAFE_DOWNCAST(n_temp, size_t, Py_ssize_t);
                    break;
                default:
                    PyErr_Format(PyExc_OverflowError,
                                 "'n' argument for fast_itertools.chunked() must be "
                                 "None or a positive number <= %zd",
                                 PY_SSIZE_T_MAX);
                    goto error;
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
    PyObject *iterable = o->iterable;
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
                    _PyErr_Clear(tstate);
                    break;
                }
            }
            goto error;
        }
        PyList_SET_ITEM(res, i, item);
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
    size_t n_temp;

    Py_ssize_t nargs = 0;
    if (likely(args != NULL)) {
        nargs = PyTuple_GET_SIZE(args);
        if (likely(nargs == 2)) {
            n_arg = PyTuple_GET_ITEM(args, 1);
            if (likely(PyLong_Check(n_arg))) {
                Py_ssize_t long_arg_size = Py_SIZE(n_arg);
                digit *ob_digit;
                switch (long_arg_size) {
                case 0:
                    n = 0;
                    break;
                case 1:
                    n = ((PyLongObject *)n_arg)->ob_digit[0];
                    break;
                case 2:
                    ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                    n = ob_digit[0] | (ob_digit[1] << PyLong_SHIFT);
                    break;
                case 3:
                    ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                    n_temp = (ob_digit[0]
                              | ((ob_digit[1] | (ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
                    if (unlikely(n_temp > (size_t)PY_SSIZE_T_MAX)) {
                        PyErr_Format(PyExc_OverflowError,
                                     "'n' argument for fast_itertools.ichunk() must "
                                     "be None or a positive number <= %zd",
                                     PY_SSIZE_T_MAX);
                        return NULL;
                    }
                    n = Py_SAFE_DOWNCAST(n_temp, size_t, Py_ssize_t);
                    break;
                default:
                    PyErr_Format(PyExc_OverflowError,
                                 "'n' argument for fast_itertools.ichunk() must "
                                 "be None or a positive number <= %zd",
                                 PY_SSIZE_T_MAX);
                    return NULL;
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
                    _PyErr_Clear(tstate);
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
    size_t n_temp;
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

    Py_ssize_t long_arg_size = Py_SIZE(num);
    digit *ob_digit;
    switch (long_arg_size) {
    case 0:
        n = 0;
        break;
    case 1:
        n = ((PyLongObject *)num)->ob_digit[0];
        break;
    case 2:
        ob_digit = ((PyLongObject *)num)->ob_digit;
        n = ob_digit[0] | (ob_digit[1] << PyLong_SHIFT);
        break;
    case 3:
        ob_digit = ((PyLongObject *)num)->ob_digit;
        n_temp = (ob_digit[0]
                  | ((ob_digit[1] | (ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
        if (unlikely(n_temp > (size_t)PY_SSIZE_T_MAX)) {
            goto invalid_argument;
        }
        n = Py_SAFE_DOWNCAST(n_temp, size_t, Py_ssize_t);
        break;
    default:
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
    size_t n_temp;
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
            Py_ssize_t long_arg_size = Py_SIZE(n_arg);
            digit *ob_digit;
            switch (long_arg_size) {
            case 0:
                n = 0;
                break;
            case 1:
                n = ((PyLongObject *)n_arg)->ob_digit[0];
                break;
            case 2:
                ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                n = ob_digit[0] | (ob_digit[1] << PyLong_SHIFT);
                break;
            case 3:
                ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                n_temp = (ob_digit[0]
                          | ((ob_digit[1] | (ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
                if (unlikely(n_temp > (size_t)PY_SSIZE_T_MAX)) {
                    PyErr_Format(PyExc_OverflowError,
                                 "'n' argument for fast_itertools.ichunked() must be "
                                 "None or a positive number <= %zd",
                                 PY_SSIZE_T_MAX);
                    goto error;
                }
                n = Py_SAFE_DOWNCAST(n_temp, size_t, Py_ssize_t);
                break;
            default:
                PyErr_Format(PyExc_OverflowError,
                             "'n' argument for fast_itertools.ichunked() must be "
                             "None or a positive number <= %zd",
                             PY_SSIZE_T_MAX);
                goto error;
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
                Py_ssize_t long_arg_size = Py_SIZE(n_arg);
                digit *ob_digit;
                switch (long_arg_size) {
                case 0:
                    n = 0;
                    break;
                case 1:
                    n = ((PyLongObject *)n_arg)->ob_digit[0];
                    break;
                case 2:
                    ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                    n = ob_digit[0] | (ob_digit[1] << PyLong_SHIFT);
                    break;
                case 3:
                    ob_digit = ((PyLongObject *)n_arg)->ob_digit;
                    n_temp = (ob_digit[0]
                              | ((ob_digit[1] | (ob_digit[2] << PyLong_SHIFT)) << PyLong_SHIFT));
                    if (unlikely(n_temp > (size_t)PY_SSIZE_T_MAX)) {
                        PyErr_Format(PyExc_OverflowError,
                                     "'n' argument for fast_itertools.ichunked() must be "
                                     "None or a positive number <= %zd",
                                     PY_SSIZE_T_MAX);
                        goto error;
                    }
                    n = Py_SAFE_DOWNCAST(n_temp, size_t, Py_ssize_t);
                    break;
                default:
                    PyErr_Format(PyExc_OverflowError,
                                 "'n' argument for fast_itertools.ichunked() must be "
                                 "None or a positive number <= %zd",
                                 PY_SSIZE_T_MAX);
                    goto error;
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
    :func:`ichunked` is like :func:`chunked`, but it yields iterables\n\
    instead of lists.\n\
\n\
    If the sub-iterables are read in order, the elements of *iterable*\n\
    won't be stored in memory.\n\
    If they are read out of order, :func:`itertools.tee` is used to cache\n\
    elements as necessary.\n\
\n\
    >>> import itertools \n\
    >>> all_chunks = fast_itertools.ichunked(count(), 4)\n\
    >>> c_1, c_2, c_3 = next(all_chunks), next(all_chunks), next(all_chunks)\n\
    >>> list(c_2)  # c_1's elements have been cached; c_3's haven't been\n\
    [4, 5, 6, 7]\n\
    >>> list(c_1)\n\
    [0, 1, 2, 3]\n\
    >>> list(c_3)\n\
    [8, 9, 10, 11]\n");

/* .ichunked() and ichunked object END **************************************/



static PyMethodDef fast_itertools_methods[] = {
    {"chunked", (PyCFunction)fast_itertools_chunked, METH_FASTCALL | METH_KEYWORDS,
     chunked__doc__},
    {"ichunked", (PyCFunction)fast_itertools_ichunked, METH_FASTCALL | METH_KEYWORDS,
     ichunked__doc__},
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
    PyObject *m = PyModule_Create(&fast_itertoolsmodule);
    if (unlikely(PyModule_AddType(m, &ichunk_type) < 0)) {
        return NULL;
    }
    return m;
}
