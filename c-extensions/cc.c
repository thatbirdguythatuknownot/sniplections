#define PY_SSIZE_T_CLEAN
#define Py_BUILD_CORE
#include "Python.h"

#include "internal/pycore_pystate.h"
#include "internal/pycore_pyerrors.h"
#include "internal/pycore_long.h"
#undef Py_BUILD_CORE

#include <omp.h>

#define likely(x) __builtin_expect(x, 1)
#define unlikely(x) __builtin_expect(x, 0)

static inline void Py_INC_REFCNT(PyObject *ob, Py_ssize_t refcnt) {
    ob->ob_refcnt += refcnt;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_INC_REFCNT(ob, refcnt) Py_INC_REFCNT(_PyObject_CAST(ob), (refcnt))
#endif

static inline Py_ALWAYS_INLINE Py_hash_t
hash_noerror(PyObject *v)
{
    PyTypeObject *tp = Py_TYPE(v);
    if (tp->tp_hash != NULL)
        return (*tp->tp_hash)(v);

    if (tp->tp_dict == NULL) {
        if (PyType_Ready(tp) < 0)
            return -1;
        if (tp->tp_hash != NULL)
            return (*tp->tp_hash)(v);
    }

    return -1;
}

static PyObject *
cc_get_consec(PyObject *self, PyObject *const *args,
              Py_ssize_t nargs, PyObject *kwargs)
{
    PyObject *res = NULL, *item, *obj, **list_items, **res_items;
    PyObject *x = NULL, *y = NULL, *tup = NULL;
    Py_hash_t item_hash, obj_hash;
    Py_ssize_t list_size, index, threshold = 0, i = 0, j = 0;
    Py_ssize_t len_consec;
    int threshold_given = 0;
    int item_is_unicode, obj_is_unicode, obj_is_neghash = 0;
    PyThreadState *tstate = _PyThreadState_GET();
    if (unlikely(tstate == NULL)) {
        tstate = PyGILState_GetThisThreadState();
        if (unlikely(tstate == NULL)) {
            Py_FatalError("tstate not found");
        }
    }
    switch (nargs) {
    case 3:
        threshold = PyLong_AsSsize_t(args[2]);
        if (unlikely(threshold == -1 && PyErr_Occurred())) {
            return NULL;
        }
        threshold_given = 1;
    case 2:
        obj = args[1];
        if (!PyUnicode_CheckExact(obj) ||
            (obj_hash = _PyASCIIObject_CAST(obj)->hash) == -1)
        {
            obj_hash = hash_noerror(obj);
        }
        if (unlikely(obj_hash == -1)) {
            if (_PyErr_Occurred(tstate)) {
                _PyErr_Clear(tstate);
            }
            obj_is_neghash = 1;
        }
        obj_is_unicode = PyUnicode_CheckExact(obj);
        PyObject *list = args[0];
        if (unlikely((Py_TYPE(list)->tp_flags & Py_TPFLAGS_LIST_SUBCLASS) == 0)) {
            PyErr_Format(PyExc_TypeError,
                         "argument 1 to cc.get_consec() is "
                         "a '%.200s' object instead of a list",
                         Py_TYPE(list)->tp_name);
            return NULL;
        }
        list_items = ((PyListObject *)list)->ob_item;
        list_size = Py_SIZE(list);
        break;
    case 69:
        goto unreachable_state;
    default:
        PyErr_Format(PyExc_TypeError,
                     "expected 2 or 3 positional arguments to "
                     "cc.get_consec(), got %zd",
                     nargs);
        return NULL;
    }
    if (kwargs) {
        const char *data;
        if (!threshold_given &&
            Py_SIZE(kwargs) == 1 &&
            memcmp(data = PyUnicode_DATA(((PyTupleObject *)kwargs)->ob_item[0]), "threshold", 9) == 0)
        {
            threshold = PyLong_AsSsize_t(args[2]);
            if (unlikely(threshold == -1 && PyErr_Occurred())) {
                return NULL;
            }
        }
        else if (threshold_given) {
            PyErr_SetString(PyExc_TypeError,
                            "argument 'threshold' given as positional "
                            "argument 2 and as keyword argument");
            return NULL;
        }
        else if (Py_SIZE(kwargs) > 1) {
            PyErr_Format(PyExc_TypeError,
                         "expected 2 or 3 arguments to cc.get_consec(), "
                         "got %zd",
                         nargs + Py_SIZE(kwargs));
            return NULL;
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "'%.200s' is not a valid keyword argument to "
                         "cc.get_consec()",
                         data);
            return NULL;
        }
    }
    res = PyList_New((list_size >> 1) + 1);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    res_items = ((PyListObject *)res)->ob_item;
  find_obj_again:
    if (unlikely(i >= list_size)) {
        Py_SET_SIZE(res, j);
        return res;
    }
    if (unlikely((item = list_items[i++]) == obj)) {
        index = i - 1;
        goto find_end_consec;
    }
    if (!(item_is_unicode = PyUnicode_CheckExact(item)) ||
        (item_hash = _PyASCIIObject_CAST(item)->hash) == -1)
    {
        item_hash = hash_noerror(item);
    }
    if (unlikely(item_hash == -1)) {
        if (_PyErr_Occurred(tstate)) {
            _PyErr_Clear(tstate);
        }
        if (obj_is_neghash) {
            if (obj_is_unicode && item_is_unicode &&
                _PyUnicode_EQ(item, obj))
            {
                index = i - 1;
                goto find_end_consec;
            }
            int status = PyObject_RichCompareBool(item, obj, Py_EQ);
            if (unlikely(status < 0)) {
                goto error;
            }
            else if (status) {
                index = i - 1;
                goto find_end_consec;
            }
        }
    }
    else if (item_hash == obj_hash) {
        if (likely(obj_is_unicode && item_is_unicode &&
                   _PyUnicode_EQ(item, obj)))
        {
            index = i - 1;
            goto find_end_consec;
        }
        int status = PyObject_RichCompareBool(item, obj, Py_EQ);
        if (unlikely(status < 0)) {
            goto error;
        }
        else if (likely(status)) {
            index = i - 1;
            goto find_end_consec;
        }
    }
    goto find_obj_again;
  find_end_consec:
    if (unlikely(i >= list_size)) {
        if ((len_consec = i - index) >= threshold) {
            x = PyLong_FromSsize_t(index);
            if (unlikely(x == NULL)) {
                goto error;
            }
            y = PyLong_FromSsize_t(len_consec);
            if (unlikely(y == NULL)) {
                goto error;
            }
            res_items[j++] = tup = PyTuple_Pack(2, x, y);
            if (unlikely(tup == NULL)) {
                goto error;
            }
        }
        Py_SET_SIZE(res, j);
        return res;
    }
    if ((item = list_items[i++]) == obj) {
        goto find_end_consec;
    }
    if (!(item_is_unicode = PyUnicode_CheckExact(item)) ||
        (item_hash = _PyASCIIObject_CAST(item)->hash) == -1)
    {
        item_hash = hash_noerror(item);
    }
    if (unlikely(item_hash == -1)) {
        if (_PyErr_Occurred(tstate)) {
            _PyErr_Clear(tstate);
        }
        if (obj_is_neghash) {
            if (obj_is_unicode && item_is_unicode &&
                _PyUnicode_EQ(item, obj))
            {
                goto find_end_consec;
            }
            int status = PyObject_RichCompareBool(item, obj, Py_EQ);
            if (unlikely(status < 0)) {
                goto error;
            }
            else if (status) {
                goto find_end_consec;
            }
        }
    }
    else if (item_hash == obj_hash) {
        if (likely(obj_is_unicode && item_is_unicode &&
                   _PyUnicode_EQ(item, obj)))
        {
            goto find_end_consec;
        }
        int status = PyObject_RichCompareBool(item, obj, Py_EQ);
        if (unlikely(status < 0)) {
            goto error;
        }
        else if (likely(status)) {
            goto find_end_consec;
        }
    }
    if ((len_consec = i - index + 1) >= threshold) {
        x = PyLong_FromSsize_t(index);
        if (unlikely(x == NULL)) {
            goto error;
        }
        y = PyLong_FromSsize_t(len_consec);
        if (unlikely(y == NULL)) {
            goto error;
        }
        res_items[j++] = tup = PyTuple_Pack(2, x, y);
        if (unlikely(tup == NULL)) {
            goto error;
        }
    }
    goto find_obj_again;
  unreachable_state:
    Py_FatalError(
        "\nThe code somehow reached this path. I thought that\n"
        "the message in the current unreachable was too boring to\n"
        "read, so I decided to make my own out of thinking.\n\n"
        "I don't think I'm really creative with the message. It's\n"
        "just a little thing that I put my time into, wasting my\n"
        "precious time writing it instead of testing the code that\n"
        "I just wrote. I could've made an AI generate this, but\n"
        "I decided not to. It feels better to write it myself.\n\n"
        "Anyways, this message is reaching its end. I tried to write this\n"
        "as fast as I could. This is probably the most that I can get\n"
        "this to, before I stop wasting my time writing this and start\n"
        "doing other things. Farewell, and I hope the code doesn't\n"
        "fail again. Unless you activated the easter egg to achieve it.\n"
        "https://xkcd.com/2200/"
    );
  error:
    Py_DECREF(res);
    Py_XDECREF(x);
    Py_XDECREF(y);
    return NULL;
}

PyDoc_STRVAR(cc_get_consec__doc__, "\
cc.get_consec(list_object, object, /, threshold=1)\n\
    Get the consecutive occurences of *object* from *list_object*\n\
    in (idx, len) pairs. If the length of the occurence is less than\n\
    *threshold* then it is filtered out.\n\
");

static PyObject *
cc_split(PyObject *self,
        PyObject *const *args,
        Py_ssize_t nargs,
        PyObject *kwargs)
{
    PyObject *kwarg;
    PyObject *_num, *_n, *quot_obj, *quot1_obj, *list;
    PyObject **list_items;
    size_t num, n, quot, rem;
    Py_ssize_t nkwargs, lenkwarg, total, maxiter, j, i = 0;
    int thread_num;

    total = nargs + (nkwargs = kwargs == NULL ? 0 : PyTuple_GET_SIZE(kwargs));
    if (likely(total == 2)) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method cc.split() missing "
                        "1 required positional argument: 'self'");
    }
    else if (total == 1) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method cc.split() missing "
                        "1 required argument: 'n'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method cc.split() got "
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
                         "unbound method cc.split() got "
                         "an unexpected keyword argument '%U'",
                         kwarg);
            return NULL;
        }
    }
    num = PyLong_AsSize_t(args[0]);
    if (unlikely(num == (size_t)-1 && PyErr_Occurred())) {
        return NULL;
    }
    n = PyLong_AsSize_t(args[1]);
    if (unlikely(n == (size_t)-1 && PyErr_Occurred())) {
        return NULL;
    }
    else if (unlikely(n == 0)) {
        PyErr_SetString(PyExc_ValueError,
                        "unbound method cc.split() argument 2 "
                        "must not be 0");
        return NULL;
    }
    quot = num / n;
    rem = num % n;
    list = PyList_New(n);
    if (unlikely(list == NULL)) {
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
        if (rem > 7) {
            i = (Py_ssize_t)rem >> 3;
#pragma omp parallel num_threads(8) private(thread_num, i, j, maxiter)
            {
                if (likely((thread_num = omp_get_thread_num()) < 7)) {
                    maxiter = i + i * thread_num;
                }
                else {
                    maxiter = rem;
                }
#pragma omp for
                for (j = maxiter - i; j < maxiter; j++) {
                    list_items[j] = quot1_obj;
                }
            }
        }
        else {
#pragma omp parallel for num_threads(rem)
            for (i = 0; i < rem; i++) {
                list_items[i] = quot1_obj;
            }
        }
    }
  quot_obj_init:
    quot_obj = PyLong_FromSize_t(quot);
    if (unlikely(quot_obj == NULL)) {
        Py_DECREF(list);
        return NULL;
    }
    Py_INC_REFCNT(quot_obj, (i = n - rem) - 1);
    if (likely(i > 7)) {
        j = i >> 3;
        maxiter = rem;
#pragma omp parallel num_threads(8) private(thread_num)
        {
            Py_ssize_t new_maxiter;
            if (likely((thread_num = omp_get_thread_num()) < 7)) {
                new_maxiter = rem + j + j * thread_num;
            }
            else {
                new_maxiter = n;
            }
#pragma omp for
            for (Py_ssize_t k = maxiter; k < new_maxiter; k++) {
                list_items[k] = quot_obj;
            }
            maxiter = new_maxiter;
        }
    }
    else {
#pragma omp parallel for num_threads(i)
        for (j = rem; j < i; j++) {
            list_items[j] = quot_obj;
        }
    }
    return list;
}

PyDoc_STRVAR(cc_split__doc__, "\
cc.split(self, /, n)\n\
    Split an integer into n groups. Returns a list sorted from greatest\n\
    to least.\n\
\n\
      n\n\
        The number of groups.\n\
");

PyObject *zero;

#define IS_MEDIUM_VALUE(x) (((size_t)Py_SIZE(x)) + 1U < 3U)
#define IS_SMALL_INT(ival) (-_PY_NSMALLNEGINTS <= (ival) && (ival) < _PY_NSMALLPOSINTS)

static PyObject *
cc_int_brstrip(PyObject *self,
               PyObject *const *args,
               Py_ssize_t nargs)
{
    PyObject *err;
    PyLongObject *num;
    Py_ssize_t _fast_num = 0;
    Py_ssize_t real_size_num, size_num, newsize, j, i = 0;
    size_t fast_num;
    unsigned mergeshift, shift;
    digit dig;
    twodigits accum = 0;

    if (likely(nargs == 1)) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method cc.int_brstrip() missing "
                        "1 required positional argument: 'self'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method cc.brstrip() got "
                     "more arguments than accepted (%zd > 1)",
                     nargs);
    }
    return NULL;

  valid_args:
    num = (PyLongObject *)args[0];
    if (unlikely(num->ob_digit[0] & 1)) {
        Py_INCREF(num);
        return (PyObject *)num;
    }
    _fast_num = PyLong_AsSsize_t((PyObject *)num);
    if (unlikely(_fast_num == -1 && (err = PyErr_Occurred()))) {
        if (PyErr_GivenExceptionMatches(err, PyExc_OverflowError)) {
            PyErr_Clear();
            _fast_num = 0;
            goto slow_path;
        }
        else {
            return NULL;
        }
    }
    fast_num = (size_t)_fast_num;
    fast_num >>= __builtin_ctzll(fast_num);
    if (_fast_num > 0) {
        return PyLong_FromSize_t(fast_num);
    }
    if (_fast_num < 0) {
        return PyLong_FromSsize_t((Py_ssize_t)fast_num);
    }
    Py_INCREF(zero);
    return zero;
  slow_path:
    for (; unlikely(num->ob_digit[i] == 0); i++);
    shift = __builtin_ctz(dig = num->ob_digit[i]);
    mergeshift = PyLong_SHIFT - shift;
    accum = dig >> shift;
    real_size_num = Py_SIZE(num);
    size_num = Py_ABS(real_size_num);
    newsize = size_num - i;
    switch (newsize) {
        case 2:
            _fast_num = (Py_ssize_t)num->ob_digit[i + 1] << mergeshift;
        case 1:
            _fast_num |= accum;
            _fast_num = real_size_num > 0 ? _fast_num : -_fast_num;
            return PyLong_FromSsize_t(_fast_num);
        default:
            break;
    }
    PyLongObject *z = _PyLong_New(newsize);
    if (unlikely(z == NULL)) {
        return NULL;
    }
    for (j = i + 1, i = 0; j < size_num;) {
        accum |= (twodigits)num->ob_digit[j++] << mergeshift;
        z->ob_digit[i++] = (digit)(accum & PyLong_MASK);
        accum >>= PyLong_SHIFT;
    }
    if (likely(accum != 0)) {
        z->ob_digit[newsize - 1] = (digit)accum;
    }
    else {
        Py_SET_SIZE(z, newsize = newsize - 1);
    }
    if (unlikely(real_size_num < 0)) {
        Py_SET_SIZE(z, -newsize);
    }
    return (PyObject *)z;
}

PyDoc_STRVAR(cc_int_brstrip__doc__, "\
cc.int_brstrip(self, /)\n\
    Strip an integer of trailing zero bits.\n\
");

static PyObject *
cc_int_btshift(PyObject *self,
               PyObject *const *args,
               Py_ssize_t nargs)
{
    PyObject *err, *res, *e0, *e1;
    PyLongObject *num, *z;
    Py_ssize_t _fast_num = 0;
    Py_ssize_t real_size_num, size_num, newsize, j, i = 0;
    size_t fast_num, skipped;
    unsigned long mergeshift, shift;
    digit dig;
    twodigits accum = 0;

    if (likely(nargs == 1)) {
        goto valid_args;
    }

    if (unlikely(nargs == 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "unbound method cc.int_btshift() missing "
                        "1 required positional argument: 'self'");
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "unbound method cc.btshift() got "
                     "more arguments than accepted (%zd > 1)",
                     nargs);
    }
    return NULL;

  valid_args:
    num = (PyLongObject *)args[0];
    if (unlikely(num->ob_digit[0] & 1)) {
        Py_INCREF(num);
        return (PyObject *)num;
    }
    _fast_num = PyLong_AsSsize_t((PyObject *)num);
    if (unlikely(_fast_num == -1 && (err = PyErr_Occurred()))) {
        if (PyErr_GivenExceptionMatches(err, PyExc_OverflowError)) {
            PyErr_Clear();
            _fast_num = 0;
            goto slow_path;
        }
        else {
            return NULL;
        }
    }
    fast_num = (size_t)_fast_num;
    fast_num >>= (shift = __builtin_ctzll(fast_num));
    res = PyTuple_New(2);
    if (unlikely(res == NULL)) {
        return NULL;
    }
    if (_fast_num > 0) {
        e0 = PyLong_FromSize_t(fast_num);
        if (unlikely(e0 == NULL)) {
            Py_DECREF(res);
            return NULL;
        }
        e1 = PyLong_FromUnsignedLong(shift);
        if (unlikely(e1 == NULL)) {
            Py_DECREF(res);
            Py_DECREF(e0);
            return NULL;
        }
    }
    else if (_fast_num < 0) {
        e0 = PyLong_FromSsize_t((Py_ssize_t)fast_num);
        if (unlikely(e0 == NULL)) {
            Py_DECREF(res);
            return NULL;
        }
        e1 = PyLong_FromUnsignedLong(shift);
        if (unlikely(e1 == NULL)) {
            Py_DECREF(res);
            Py_DECREF(e0);
            return NULL;
        }
    }
    else {
        Py_INCREF(zero);
        Py_INCREF(zero);
        e0 = e1 = zero;
    }
    PyTuple_SET_ITEM(res, 0, e0);
    PyTuple_SET_ITEM(res, 1, e1);
    return res;
  slow_path:
    for (; unlikely(num->ob_digit[i] == 0); i++);
    skipped = i;
    shift = __builtin_ctz(dig = num->ob_digit[i]);
    mergeshift = PyLong_SHIFT - shift;
    accum = dig >> shift;
    real_size_num = Py_SIZE(num);
    size_num = Py_ABS(real_size_num);
    newsize = size_num - i;
    switch (newsize) {
        case 2:
            _fast_num = (Py_ssize_t)num->ob_digit[i + 1] << mergeshift;
        case 1:
            _fast_num |= accum;
            _fast_num = real_size_num > 0 ? _fast_num : -_fast_num;
            res = PyTuple_New(2);
            if (unlikely(res == NULL)) {
                return NULL;
            }
            e0 = PyLong_FromSsize_t(_fast_num);
            if (unlikely(e0 == NULL)) {
                Py_DECREF(res);
                return NULL;
            }
            e1 = PyLong_FromUnsignedLong(skipped*30 + shift);
            if (unlikely(e1 == NULL)) {
                Py_DECREF(res);
                Py_DECREF(e0);
                return NULL;
            }
            PyTuple_SET_ITEM(res, 0, e0);
            PyTuple_SET_ITEM(res, 1, e1);
            return res;
        default:
            break;
    }
    z = _PyLong_New(newsize);
    if (unlikely(z == NULL)) {
        return NULL;
    }
    for (j = i + 1, i = 0; j < size_num;) {
        accum |= (twodigits)num->ob_digit[j++] << mergeshift;
        z->ob_digit[i++] = (digit)(accum & PyLong_MASK);
        accum >>= PyLong_SHIFT;
    }
    if (likely(accum != 0)) {
        z->ob_digit[newsize - 1] = (digit)accum;
    }
    else {
        Py_SET_SIZE(z, newsize = newsize - 1);
    }
    if (unlikely(real_size_num < 0)) {
        Py_SET_SIZE(z, -newsize);
    }
    res = PyTuple_New(2);
    if (unlikely(res == NULL)) {
        Py_DECREF(z);
        return NULL;
    }
    e1 = PyLong_FromUnsignedLong(skipped*30 + shift);
    if (unlikely(e1 == NULL)) {
        Py_DECREF(res);
        Py_DECREF(z);
        return NULL;
    }
    PyTuple_SET_ITEM(res, 0, (PyObject *)z);
    PyTuple_SET_ITEM(res, 1, e1);
    return res;
}

PyDoc_STRVAR(cc_int_btshift__doc__, "\
cc.int_btshift(self, /)\n\
    Strip an integer of trailing zero bits. Returns\n\
    (stripped integer, shift).\n\
");

#define METH_FASTCALL_DOC(name) \
    {#name, (PyCFunction)(void(*)(void))cc_ ## name, METH_FASTCALL, cc_ ## name ## __doc__},

#define METH_FASTCALL_KEYWORDS_DOC(name) \
    {#name, (PyCFunction)(void(*)(void))cc_ ## name, METH_FASTCALL | METH_KEYWORDS, cc_ ## name ## __doc__},

static PyMethodDef CCMethods[] = {
    METH_FASTCALL_KEYWORDS_DOC(get_consec)
    METH_FASTCALL_KEYWORDS_DOC(split)

    METH_FASTCALL_DOC(int_brstrip)
    METH_FASTCALL_DOC(int_btshift)

    {NULL}
};

static struct PyModuleDef ccmodule = {
    PyModuleDef_HEAD_INIT,
    "cc",
    "C extension 2",
    -1,
    CCMethods,
};

Py_EXPORTED_SYMBOL PyObject *
PyInit_cc(void)
{
    zero = _PyLong_GetZero();
    return PyModule_Create(&ccmodule);
}
