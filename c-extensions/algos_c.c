#define PY_SSIZE_T_CLEAN
#include <stdbool.h>
#include <search.h>
#include <stdlib.h>
#include "Python.h"

/*[clinic input]
module algos
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d79fe216a0c66166]*/

static const Py_ssize_t CUTOFF = 8;
static const Py_ssize_t _CONSTANT_1 = 40;

#ifdef USE_STATIC_INLINES
static inline const void
SWAP(register void **a, register void **b)
{
    const register void *t = *a;
    *a = *b;
    *b = t;
}
#endif

static inline const Py_ssize_t
MEDIAN(register PyObject **array, const register Py_ssize_t i,
       const register Py_ssize_t j, const register Py_ssize_t k)
{
    register int is_less;

    is_less = PyObject_RichCompareBool(array[i], array[j], Py_LT);
    if (is_less == -1) {
        return (const Py_ssize_t)-1;
    }
    if (is_less) {
        is_less = PyObject_RichCompareBool(
            array[j],
            array[k],
            Py_LT
        );
        if (is_less == -1) {
            return (const Py_ssize_t)-1;
        }
        if (is_less) {
            return j;
        }
        is_less = PyObject_RichCompareBool(
            array[i],
            array[k],
            Py_LT
        );
        if (is_less == -1) {
            return (const Py_ssize_t)-1;
        }
        if (is_less) {
            return k;
        }
        return i;
    }
    is_less = PyObject_RichCompareBool(array[k], array[j], Py_LT);
    if (is_less == -1) {
        return (const Py_ssize_t)-1;
    }
    if (is_less) {
        return j;
    }
    is_less = PyObject_RichCompareBool(array[k], array[i], Py_LT);
    if (is_less == -1) {
        return (const Py_ssize_t)-1;
    }
    if (is_less) {
        return k;
    }
    return i;
}

static const bool
insertion_sort(register PyObject **array,
               const register Py_ssize_t low,
               const register Py_ssize_t high)
{
    register Py_ssize_t i, j;
    register int is_less;
    register PyObject *key;

    for (i = low + 1; i < high; ++i) {
        key = array[i];
        j = i - 1;
        is_less = PyObject_RichCompareBool(
            key,
            array[j],
            Py_LT
        );
        if (is_less == -1) {
            return true;
        }
        for (;is_less;) {
            array[j + 1] = array[j];
            if (--j < low) {
                break;
            }
            is_less = PyObject_RichCompareBool(
                key,
                array[j],
                Py_LT
            );
            if (is_less == -1) {
                return true;
            }
        }
        array[j + 1] = key;
    }
    return false;
}

static const bool
quick_sort(register PyObject **array,
           const register Py_ssize_t low,
           const register Py_ssize_t high)
{
    register Py_ssize_t length, eps, mid;
    register Py_ssize_t m_first, m_mid, m_last;
    register Py_ssize_t median, median2;
#ifndef USE_STATIC_INLINES
    register PyObject *t;
#endif
    register Py_ssize_t i, j, k, p, q;
    register int is_less, is_equal;
    register PyObject *v;

    length = high - low + 1;
    if (length <= CUTOFF) {
        return insertion_sort(array, low, high);
    }
    else if (length <= _CONSTANT_1) {
        median = MEDIAN(array, low, length/2 + low, high);
#ifdef USE_STATIC_INLINES
        SWAP(&array[median], &array[low]);
#else
        t = array[median];
        array[median] = array[low];
        array[low] = t;
#endif
    }
    else {
        eps = length / CUTOFF;
        mid = length/2 + low;
        m_first = MEDIAN(array, low, low + eps, low + eps + eps);
        m_mid = MEDIAN(array, mid - eps, mid, mid + eps);
        m_last = MEDIAN(array, high - eps - eps, high - eps, high);
        median2 = MEDIAN(array, m_first, m_mid, m_last);
#ifdef USE_STATIC_INLINES
        SWAP(&array[median2], &array[low]);
#else
        t = array[median2];
        array[median2] = array[low];
        array[low] = t;
#endif
    }
    p = i = low;
    q = j = high + 1;
    v = array[low];
    for (;;) {
        is_less = PyObject_RichCompareBool(array[++i], v, Py_LT);
        if (is_less == -1) {
            return true;
        }
        while (is_less && i < high) {
            is_less = PyObject_RichCompareBool(
                array[++i],
                v,
                Py_LT
            );
            if (is_less == -1) {
                return true;
            }
        }
        is_less = PyObject_RichCompareBool(v, array[--j], Py_LT);
        if (is_less == -1) {
            return true;
        }
        while (is_less && j > low) {
            is_less = PyObject_RichCompareBool(
                v,
                array[--j],
                Py_LT
            );
            if (is_less == -1) {
                return true;
            }
        }
        if (i >= j) {
            break;
        }
#ifdef USE_STATIC_INLINES
        SWAP(&array[i], &array[j]);
#else
        t = array[i];
        array[i] = array[j];
        array[j] = t;
#endif
        is_equal = PyObject_RichCompareBool(array[i], v, Py_EQ);
        if (is_equal == -1) {
            return true;
        }
        else if (is_equal) {
#ifdef USE_STATIC_INLINES
            SWAP(&array[++p], &array[i]);
#else
            t = array[++p];
            array[p] = array[i];
            array[i] = t;
#endif
        }
        is_equal = PyObject_RichCompareBool(array[j], v, Py_EQ);
        if (is_equal == -1) {
            return true;
        }
        else if (is_equal) {
#ifdef USE_STATIC_INLINES
            SWAP(&array[--q], &array[j]);
#else
            t = array[--q];
            array[q] = array[j];
            array[j] = t;
#endif
        }
    }
#ifdef USE_STATIC_INLINES
    SWAP(&array[low], &array[j]);
#else
    t = array[low];
    array[low] = array[j];
    array[j] = t;
#endif
    ++i;
    --j;
    for (k = low + 1; k <= p; ++k) {
#ifdef USE_STATIC_INLINES
            SWAP(&array[k], &array[j--]);
#else
            t = array[k];
            array[k] = array[j];
            array[j--] = t;
#endif
    }
    for (k = high; k >= q; --k) {
#ifdef USE_STATIC_INLINES
            SWAP(&array[k], &array[i++]);
#else
            t = array[k];
            array[k] = array[i];
            array[i++] = t;
#endif
    }
    if (quick_sort(array, low, j)) {
        return true;
    }
    return quick_sort(array, i, high);
}

#include "clinic/algos_c.c.h"

/*[clinic input]
algos.insertion_sort

    array: object
        The array to sort. Must be a list.
    low: Py_ssize_t = 0
        Sort from this point in the array (inclusive). Must be positive and
        must be less than or equal to the 'high' argument.
        0 is the default value.
    high: Py_ssize_t = -1
        Sort to this point in the array (exclusive). Must be positive.
        -1 is the default value, which will default to len(array).

Insertion sort implementation, sorting a list inplace.
[clinic start generated code]*/

static PyObject *
algos_insertion_sort_impl(PyObject *module, PyObject *array, Py_ssize_t low,
                          Py_ssize_t high)
/*[clinic end generated code: output=404d49a17e5a4696 input=ee16636692de8454]*/
{
    if (!PyList_Check(array)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument 1 to algos.insertion_sort must be "
                        "a list or a subclass of 'list'"
        );
        return NULL;
    }
    if (low < 0) {
        PyErr_Format(PyExc_ValueError,
                     "argument 2 to algos.insertion_sort must be "
                     "positive, instead it's %zd",
                     low);
        return NULL;
    }
    if (high == -1) {
        high += PyObject_Length(array);
        if (high == -2) {
            return NULL;
        }
    }
    if (high < 0) {
        PyErr_Format(PyExc_ValueError,
                     "argument 3 to algos.insertion_sort must be "
                     "positive, instead it's %zd",
                     high);
        return NULL;
    }
    if (low > high) {
        PyErr_Format(PyExc_ValueError,
                     "argument 'low' to algos.insertion_sort must be "
                     "less than argument 'high', instead it's %zd",
                     low);
        return NULL;
    }
    if (low == high) {
        goto done;
    }
    if (insertion_sort(((PyListObject *)array)->ob_item, low, high)) {
        return NULL;
    }
  done:
    Py_RETURN_NONE;
}

/*[clinic input]
algos.quick_sort

    array: object
        The array to sort. Must be a list.
    low: Py_ssize_t = 0
        Sort from this point in the array (inclusive). Must be positive and
        must be less than or equal to the 'high' argument.
        0 is the default value.
    high: Py_ssize_t = -1
        Sort to this point in the array (inclusive). Must be positive and
        must be less than the 'length' argument.
        -1 is the default value, which will default to len(array) - 1.
    length: Py_ssize_t = -1
        Sort the array to array[:length]. Must be positive and
        if greater than len(array) will default to len(array).
        -1 is the default value, which will default to len(array).
        'high' - 'low' + 1 must be greater than or equal to this value;
        If it is greater than, 'high' is clamped.

Quick-sort implementation, sorting a list inplace.
[clinic start generated code]*/

static PyObject *
algos_quick_sort_impl(PyObject *module, PyObject *array, Py_ssize_t low,
                      Py_ssize_t high, Py_ssize_t length)
/*[clinic end generated code: output=4f83275f69747d6e input=e4d86946f24f963a]*/
{
    register Py_ssize_t add;
    register Py_ssize_t high_low;

    if (!PyList_Check(array)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument 1 to algos.quick_sort must be "
                        "a list or a subclass of 'list'"
        );
        return NULL;
    }
    if (low < 0) {
        PyErr_Format(PyExc_ValueError,
                     "argument 2 to algos.quick_sort must be "
                     "positive, instead it's %zd",
                     low);
        return NULL;
    }
    if (high == -1) {
        high += PyObject_Length(array);
        if (high == -2) {
            return NULL;
        }
    }
    if (high < 0) {
        PyErr_Format(PyExc_ValueError,
                     "argument 3 to algos.quick_sort must be "
                     "positive, instead it's %zd",
                     high);
        return NULL;
    }
    if (low > high) {
        PyErr_Format(PyExc_ValueError,
                     "argument 'low' to algos.quick_sort must be "
                     "less than argument 'high', instead it's %zd",
                     low);
        return NULL;
    }
    if (length == -1) {
        length = PyObject_Length(array);
    }
    if (length < 0) {
        PyErr_Format(PyExc_ValueError,
                     "argument 'length' to algos.quick_sort "
                     "must be positive, instead it's %zd",
                     length);
        return NULL;
    }
    if (high >= length) {
        PyErr_Format(PyExc_ValueError,
                     "argument 'high' to algos.quick_sort "
                     "must be less than argument 'length', "
                     "instead it's %zd",
                     high);
        return NULL;
    }
    if (low == high) {
        goto done;
    }
    if (length > Py_SIZE(array)) {
        length = Py_SIZE(array);
    }
    add = length - high - 1;
    low += add;
    high += add;
    high_low = high - low + 1;
    if (high_low < length) {
        PyErr_Format(PyExc_ValueError,
                     "'high' (%zd) - 'low' (%zd) + 1 < 'length' (%zd)",
                     high, low, length);
        return NULL;
    }
    else if (high_low > length) {
        high -= length - high_low;
    }
    if (quick_sort(((PyListObject *)array)->ob_item, low, high)) {
        return NULL;
    }
  done:
    Py_RETURN_NONE;
}

static PyObject *stop_STR;
static PyObject *initial_STR;
static PyObject *start_STR;
static PyObject *step_STR;
static PyObject *use_objects_STR;

static inline const Py_ssize_t
long_compare(PyLongObject *a, PyLongObject *b)
{
    register Py_ssize_t a_size, b_size;
    register Py_ssize_t sign = (a_size = Py_SIZE(a)) - (b_size = Py_SIZE(b));
    const register sdigit *a_dig = (sdigit *)a->ob_digit;
    const register sdigit *b_dig = (sdigit *)b->ob_digit;
    if (sign == 0) {
        Py_ssize_t i = a_size < 0 ? -a_size : a_size;
        sdigit diff = 0;
        while (--i >= 0) {
            diff = a_dig[i] - b_dig[i];
            if (diff) {
                break;
            }
        }
        sign = Py_SIZE(a) < 0 ? -diff : diff;
    }
    return sign;
}

static PyObject *
algos_adder_py(register PyObject *module,
               register PyObject *const *args,
               register Py_ssize_t nargs,
               register PyObject *_kwnames)
{
    register PyLongObject *_stop, *_initial, *_start, *_step;
    _stop = _initial = _start = (PyLongObject *)PyLong_FromLong(0);
    _step = (PyLongObject *)PyLong_FromLong(1);
    register PyObject *_use_objects = NULL;
    if (nargs > 4) {
        _use_objects = args[4];
    }
    if (nargs > 3) {
        _step = (PyLongObject *)args[3];
    }
    if (nargs > 2) {
        _start = (PyLongObject *)args[2];
    }
    if (nargs > 1) {
        _initial = (PyLongObject *)args[1];
    }
    if (nargs) {
        _stop = (PyLongObject *)args[0];
    }
    register Py_ssize_t nkwargs;
    if (_kwnames && (nkwargs = Py_SIZE(_kwnames))) {
        register PyObject *const *kwstack = args + nargs;
        register PyObject *const *kwnames = ((PyTupleObject *)_kwnames)->ob_item;
        register PyObject *kwname;
        register Py_ssize_t to_match = 5;
        for (register Py_ssize_t i = 0; to_match && i < nkwargs; ++i) {
            if ((kwname = kwnames[i]) == stop_STR ||
                _PyUnicode_EQ(kwname, stop_STR))
            {
                _stop = (PyLongObject *)kwstack[i];
                --to_match;
            }
            else if ((kwname = kwnames[i]) == initial_STR ||
                     _PyUnicode_EQ(kwname, initial_STR))
            {
                _initial = (PyLongObject *)kwstack[i];
                --to_match;
            }
            else if ((kwname = kwnames[i]) == start_STR ||
                     _PyUnicode_EQ(kwname, start_STR))
            {
                _start = (PyLongObject *)kwstack[i];
                --to_match;
            }
            else if ((kwname = kwnames[i]) == step_STR ||
                     _PyUnicode_EQ(kwname, step_STR))
            {
                _step = (PyLongObject *)kwstack[i];
                --to_match;
            }
            else if ((kwname = kwnames[i]) == use_objects_STR ||
                     _PyUnicode_EQ(kwname, use_objects_STR))
            {
                _use_objects = kwstack[i];
                --to_match;
            }
        }
    }
    if (_use_objects != NULL) {
        if (_use_objects->ob_type == &PyLong_Type &&
            ((PyVarObject *)_use_objects)->ob_size)
        {
            goto use_objects;
        }
        else if (_use_objects == Py_True) {
            goto use_objects;
        }
    }
    register Py_ssize_t stop = 0, initial = 0, start = 0, step = 1;
    if (_stop != NULL) {
        switch (Py_SIZE(_stop)) {
        case -3:
            stop = (_stop->ob_digit[2] << PyLong_SHIFT) | _stop->ob_digit[1];
            stop = -((stop << PyLong_SHIFT) | _stop->ob_digit[0]);
            break;
        case -2:
            stop = -(((sdigit)_stop->ob_digit[1] << PyLong_SHIFT) | (sdigit)_stop->ob_digit[0]);
            break;
        case -1:
            stop = -(sdigit)_stop->ob_digit[0];
            break;
        case 0:
            break;
        case 1:
            stop = _stop->ob_digit[0];
            break;
        case 2:
            stop = (_stop->ob_digit[1] << PyLong_SHIFT) | _stop->ob_digit[0];
            break;
        case 3:
            stop = (_stop->ob_digit[2] << PyLong_SHIFT) | _stop->ob_digit[1];
            stop = (stop << PyLong_SHIFT) | _stop->ob_digit[0];
            break;
        default:
            goto use_objects;
            break;
        }
    }
    if (_initial != NULL) {
        switch (Py_SIZE(_initial)) {
        case -3:
            initial = (_initial->ob_digit[2] << PyLong_SHIFT) | _initial->ob_digit[1];
            initial = -((initial << PyLong_SHIFT) | _initial->ob_digit[0]);
            break;
        case -2:
            initial = -(((sdigit)_initial->ob_digit[1] << PyLong_SHIFT) | (sdigit)_initial->ob_digit[0]);
            break;
        case -1:
            initial = -(sdigit)_initial->ob_digit[0];
            break;
        case 0:
            break;
        case 1:
            initial = _initial->ob_digit[0];
            break;
        case 2:
            initial = (_initial->ob_digit[1] << PyLong_SHIFT) | _initial->ob_digit[0];
            break;
        case 3:
            initial = (_initial->ob_digit[2] << PyLong_SHIFT) | _initial->ob_digit[1];
            initial = (initial << PyLong_SHIFT) | _initial->ob_digit[0];
            break;
        default:
            goto use_objects;
            break;
        }
    }
    if (_start != NULL) {
        switch (Py_SIZE(_start)) {
        case -3:
            start = (_start->ob_digit[2] << PyLong_SHIFT) | _start->ob_digit[1];
            start = -((start << PyLong_SHIFT) | _start->ob_digit[0]);
            break;
        case -2:
            start = -(((sdigit)_start->ob_digit[1] << PyLong_SHIFT) | (sdigit)_start->ob_digit[0]);
            break;
        case -1:
            start = -(sdigit)_start->ob_digit[0];
            break;
        case 0:
            break;
        case 1:
            start = _start->ob_digit[0];
            break;
        case 2:
            start = (_start->ob_digit[1] << PyLong_SHIFT) | _start->ob_digit[0];
            break;
        case 3:
            start = (_start->ob_digit[2] << PyLong_SHIFT) | _start->ob_digit[1];
            start = (start << PyLong_SHIFT) | _start->ob_digit[0];
            break;
        default:
            goto use_objects;
            break;
        }
    }
    if (_step != NULL) {
        switch (Py_SIZE(_step)) {
        case -3:
            step = (_step->ob_digit[2] << PyLong_SHIFT) | _step->ob_digit[1];
            step = -((step << PyLong_SHIFT) | _step->ob_digit[0]);
            break;
        case -2:
            step = -(((sdigit)_step->ob_digit[1] << PyLong_SHIFT) | (sdigit)_step->ob_digit[0]);
            break;
        case -1:
            step = -(sdigit)_step->ob_digit[0];
            break;
        case 0:
            break;
        case 1:
            step = _step->ob_digit[0];
            break;
        case 2:
            step = (_step->ob_digit[1] << PyLong_SHIFT) | _step->ob_digit[0];
            break;
        case 3:
            step = (_step->ob_digit[2] << PyLong_SHIFT) | _step->ob_digit[1];
            step = (step << PyLong_SHIFT) | _step->ob_digit[0];
            break;
        default:
            goto use_objects;
            break;
        }
    }
    while (start < stop) {
        initial += start;
        start += step;
    }
    return PyLong_FromSsize_t(initial);
  use_objects:
    while (long_compare(_start, _stop) < 0) {
        _initial = (PyLongObject *)PyNumber_Add((PyObject *)_initial, (PyObject *)_start);
        _start = (PyLongObject *)PyNumber_Add((PyObject *)_start, (PyObject *)_step);
    }
    return (PyObject *)_initial;
}

PyDoc_STRVAR(algos_adder_py__doc__,
"adder_py($module, /, stop=0, initial=0, start=0, step=1, use_objects=False)"
"\n"
"--\n"
"\n"
"Very unsafe C extension implementation of the adder_py function made "
"by @SouvikSSG#8443 <@!528519445149122570> in Cython\n"
"\n"
"  stop\n"
"    Assumed a python int and is the limit (exclusive) of the range "
"to add\n"
"  initial\n"
"    Assumed a python int and is the initial value to add to\n"
"  start\n"
"    Assumed a python int and is the starting point (inclusive) of the"
" range to add\n"
"  step\n"
"    Assumed a python int and is the step of the range to add\n"
"  use_objects\n"
"    Either a python int or python bool to tell the function to use "
"python objects instead of internal C data types to prevent overflow; "
"if it is False or a false-y int, the function will automatically "
"determine whether to use objects or not");

static inline const Py_ssize_t
strdigs_length3(register const char *string)
{
    return (string[0] << 12) | (string[1] << 6) | string[2];
}

static inline const Py_ssize_t
strdigs_length4(register const char *string)
{
    return (string[0]<<18) | (string[1]<<12) | (string[2]<<6) | string[3];
}

PyObject *string_cache_1_1;

/*[clinic input]
algos._s_AOC2021_1_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @mustafaq#8791 (<@!378279228002664454>) for AoC 2021 day 1 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_1_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=fe8399611a8c3cf2 input=c313c7c8ffec2bf2]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_1_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    register char curchar = '\n';
    register Py_ssize_t prev, count = 0, temp = strdigs_length3(string);
    string += 3;
    while (curchar == '\n') {
        prev = temp;
        ++string;
        count += prev < (temp = strdigs_length3(string));
        string += 3;
        curchar = *string;
    }
    if (prev >= temp) {
        ++count;
    }
    temp = (temp << 6) | curchar;
    ++string;
    while (curchar = *string) {
        prev = temp;
        ++string;
        count += prev < (temp = strdigs_length4(string));
        string += 4;
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)count;
    _PyDict_SetItem_KnownHash(string_cache_1_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

PyObject *string_cache_1_2;

/*[clinic input]
algos._s_AOC2021_1_2

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @Eisenwave#7675 <@!162964325823283200> for AoC 2021 day 1 part 2
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_1_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=7cbfe6d075f91a9c input=5a7b9fc04f9ea3e0]*/
{
    const register int_fast64_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_1_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    char *string = (char*)((PyASCIIObject *)data + 1);
    int_fast32_t line[4] = {0};
    int_fast32_t incptr;
    register int_fast32_t count = 0, i = 0;
    while (sscanf(string, "%d\n", line+i++%4, &incptr)>0) {
        count += i>3 && line[i-1&3] > line[i&3];
        string += incptr;
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)count;
    _PyDict_SetItem_KnownHash(string_cache_1_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static const Py_ssize_t AOC2021_6_2_hardcoded_values[9] = {
    6703087164LL, 6206821033LL, 5617089148LL, 5217223242LL, 4726100874LL,
        4368232009LL, 3989468462LL, 3649885552LL, 3369186778LL};

/*[clinic input]
algos.AOC2021_6_2

    data: object
        Must be an iterable of ints with __iter__ implemented.

C-implemented version of the hardcoded values solution of @ConfusedReptile#6830 for AoC 2021 day 6 part 2
[clinic start generated code]*/

static PyObject *
algos_AOC2021_6_2_impl(PyObject *module, PyObject *data)
/*[clinic end generated code: output=b7d5016ede2e546e input=af44e8df92466edd]*/
{
    PyObject **C_data;
    PyLongObject *v;
    Py_ssize_t i, length, sum = 0;
    int item;

    data = PySequence_Fast(data,
                           "algos.AOC2021_6_2() argument 1 must "
                           "be an iterable");
    if (data == NULL) {
        return NULL;
    }
    length = PySequence_Fast_GET_SIZE(data);
    C_data = PySequence_Fast_ITEMS(data);
    for (i = 0; i < length; ++i) {
        item = ((PyLongObject *)C_data[i])->ob_digit[0];
        if (PyErr_Occurred()) {
            Py_DECREF(data);
            return NULL;
        }
        sum += AOC2021_6_2_hardcoded_values[item];
    }
    Py_DECREF(data);
    v = _PyLong_New(2);
    if (v != NULL) {
        v->ob_digit[0] = (digit)(sum & PyLong_MASK);
        v->ob_digit[1] = (digit)(sum >> PyLong_SHIFT);
    }
    
    return (PyObject *)v;
}

/*[clinic input]
algos._l_AOC2021_6_2

    data: object
        Must be a list, but it isn't really checked.
    /

Raw version of algos.AOC2021_6_2 but it only takes lists, very unsafe (no error checking)
[clinic start generated code]*/

static PyObject *
algos__l_AOC2021_6_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=d6470fbd87faa1fc input=074572f0d26ca124]*/
{
    const register Py_ssize_t length = _PyVarObject_CAST_CONST(data)->ob_size;
    const register PyObject **C_data = ((PyListObject *)data)->ob_item;
    register Py_ssize_t sum = 0;
    for (register Py_ssize_t i = 0; i < length; ++i) {
        sum += AOC2021_6_2_hardcoded_values[
            (const int)(((const PyLongObject *)C_data[i])->ob_digit[0])
        ];
    }
    register PyLongObject *v = _PyLong_New(2);
    v->ob_digit[0] = (digit)(sum & PyLong_MASK);
    v->ob_digit[1] = (digit)(sum >> PyLong_SHIFT);
    return (PyObject *)v;
}

static const Py_ssize_t AOC2021_6_2_hardcoded_v2[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,
    6703087164LL, 6206821033LL, 5617089148LL, 5217223242LL, 4726100874LL,
        4368232009LL, 3989468462LL, 3649885552LL, 3369186778LL};

static PyObject *string_cache_6_2;

/*[clinic input]
algos._s_AOC2021_6_2

    data: object
        Must be an ASCII string, but it isn't really checked.
    /

Raw version of algos.AOC2021_6_2 but it takes a string, very unsafe (no error checking)
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_6_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=d28fa0ec40da71a9 input=be1076bb8a023a2e]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_6_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    register Py_ssize_t sum = 0;
    for (register Py_ssize_t i = 0; i < 600; i += 2) {
        sum += AOC2021_6_2_hardcoded_v2[string[i]];
    }
    register PyLongObject *v = _PyLong_New(2);
    v->ob_digit[0] = (digit)(sum & PyLong_MASK);
    v->ob_digit[1] = (digit)(sum >> PyLong_SHIFT);
    _PyDict_SetItem_KnownHash(string_cache_6_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *bytes_cache_6_2;

/*[clinic input]
algos._b_AOC2021_6_2

    data: object
        Must be a bytes object, but it isn't really checked.
    /

Raw version of algos.AOC2021_6_2 but it takes a bytes object, very unsafe (no error checking)
[clinic start generated code]*/

static PyObject *
algos__b_AOC2021_6_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=84e714e713b61ab4 input=7b9c12bccf3b5f32]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(bytes_cache_6_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = ((PyBytesObject *)data)->ob_sval;
    register Py_ssize_t sum = 0;
    for (register Py_ssize_t i = 0; i < 600; i += 2) {
        sum += AOC2021_6_2_hardcoded_v2[string[i]];
    }
    register PyLongObject *v = _PyLong_New(2);
    v->ob_digit[0] = (digit)(sum & PyLong_MASK);
    v->ob_digit[1] = (digit)(sum >> PyLong_SHIFT);
    _PyDict_SetItem_KnownHash(bytes_cache_6_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static inline const int
compare_less(const void **_a, const void **_b) {
    const register Py_ssize_t a = (const Py_ssize_t)*_a;
    const register Py_ssize_t b = (const Py_ssize_t)*_b;
    return (a > b) - (a < b);
}

static inline Py_ssize_t
absolute_value(const register Py_ssize_t x)
{
    return x < 0 ? -x : x;
}

static PyObject *string_cache_7_1;

/*[clinic input]
algos._s_AOC2021_7_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @salt#2080 (<@!431341013479718912>) for AoC 2021 day 7 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_7_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=09d02d53c7051a8e input=a2840f09941773df]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_7_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    register Py_ssize_t i = -1, j = 0;
    register Py_ssize_t num = 0;
    register char curchr;
    Py_ssize_t array[1000];
    do {
        while ((curchr = string[++i]) && curchr != ',') {
            num = num*10 + (curchr - '0');
        }
        array[j++] = num;
        num = 0;
    } while (string[i]);
    qsort(array, 1001, sizeof(Py_ssize_t), compare_less);
    const register Py_ssize_t median = array[499];
    register Py_ssize_t sum = 0;
    for (i = 0; i < 1000; ++i) {
        sum += absolute_value(array[i] - median);
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(sum);
    _PyDict_SetItem_KnownHash(string_cache_7_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static const Py_ssize_t additions[9] = {0, 0, 1, 1, 1, 0, 0, 1, 0};

static PyObject *string_cache_8_1;

/*[clinic input]
algos._s_AOC2021_8_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @dementati#4691 (<@!181803748262281216>) and @mustafaq#8791 (<@!378279228002664454>) for AoC 2021 day 8 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_8_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=62947f88376a25bf input=cad3f9769adcb51a]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_8_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    register Py_ssize_t i, j = -1, k, l;
    register Py_ssize_t num = 0;
    register char curchr;
    for (i = 0; i < 200; ++i) {
        while (string[++j] != '|');
        ++j;
        for (k = 0; k < 4; ++k) {
            for (l = 0; (curchr = string[++j]) && curchr != ' ' && curchr != '\n'; ++l);
            num += additions[l];
        }
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(num);
    _PyDict_SetItem_KnownHash(string_cache_8_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *bytes_cache_8_1;

/*[clinic input]
algos._b_AOC2021_8_1

    data: object
        Must be a bytes object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @dementati#4691 (<@!181803748262281216>) and @mustafaq#8791 (<@!378279228002664454>) for AoC 2021 day 8 part 1
[clinic start generated code]*/

static PyObject *
algos__b_AOC2021_8_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=f2cf84cafac6dafb input=370bda3dc6697c5b]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(bytes_cache_8_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = ((PyBytesObject *)data)->ob_sval;
    register Py_ssize_t i, j = -1, k, l;
    register Py_ssize_t num = 0;
    register char curchr;
    for (i = 0; i < 200; ++i) {
        while (string[++j] != '|');
        ++j;
        for (k = 0; k < 4; ++k) {
            for (l = 0; (curchr = string[++j]) && curchr != ' ' && curchr != '\n'; ++l);
            num += additions[l];
        }
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(num);
    _PyDict_SetItem_KnownHash(bytes_cache_8_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static const Py_ssize_t number_list[10] = {4, 7, 2, 5, 3, 6, 0, 9, 1, 8};

static PyObject *string_cache_8_2;

/*[clinic input]
algos._s_AOC2021_8_2

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution posted by @tessaract#5963 <@!417374125015695373> for AoC 2021 day 8 part 2
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_8_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=73d7a11e2974de6a input=d58bb9ec41ce610e]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_8_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    register Py_ssize_t i, j = -1, k;
    register char curchr;
    register Py_ssize_t sum, num = 0;
    Py_ssize_t counts[104];
    for (i = 0; i < 200; ++i) {
        for (k = 97; k < 104; ++k) {
            counts[k] = 0;
        }
        for (k = 0; k < 10; ++k) {
            while ((curchr = string[++j]) && curchr != ' ') {
                ++counts[curchr];
            }
        }
        j += 2;
        register Py_ssize_t mini_sum = 0;
        while ((curchr = string[++j]) && curchr != ' ') {
            mini_sum += counts[curchr];
        }
        sum = number_list[(mini_sum>>1) % 15 % 11];
        for (k = 0; k < 2; ++k) {
            mini_sum = 0;
            while ((curchr = string[++j]) && curchr != ' ') {
                mini_sum += counts[curchr];
            }
            sum = sum * 10 + number_list[(mini_sum>>1) % 15 % 11];
        }
        mini_sum = 0;
        while ((curchr = string[++j]) && curchr != '\n') {
            mini_sum += counts[curchr];
        }
        num += sum * 10 + number_list[(mini_sum>>1) % 15 % 11];
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(num);
    _PyDict_SetItem_KnownHash(string_cache_8_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *bytes_cache_8_2;

/*[clinic input]
algos._b_AOC2021_8_2

    data: object
        Must be a bytes object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution posted by @tessaract#5963 <@!417374125015695373> for AoC 2021 day 8 part 2
[clinic start generated code]*/

static PyObject *
algos__b_AOC2021_8_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=51fbf4d30099041c input=f05bfe83909de46c]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(bytes_cache_8_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = ((PyBytesObject *)data)->ob_sval;
    register Py_ssize_t i, j = -1, k;
    register char curchr;
    register Py_ssize_t sum, num = 0;
    Py_ssize_t counts[104];
    for (i = 0; i < 200; ++i) {
        for (k = 97; k < 104; ++k) {
            counts[k] = 0;
        }
        for (k = 0; k < 10; ++k) {
            while ((curchr = string[++j]) && curchr != ' ') {
                ++counts[curchr];
            }
        }
        j += 2;
        register Py_ssize_t mini_sum = 0;
        while ((curchr = string[++j]) && curchr != ' ') {
            mini_sum += counts[curchr];
        }
        sum = number_list[(mini_sum>>1) % 15 % 11];
        for (k = 0; k < 2; ++k) {
            mini_sum = 0;
            while ((curchr = string[++j]) && curchr != ' ') {
                mini_sum += counts[curchr];
            }
            sum = sum * 10 + number_list[(mini_sum>>1) % 15 % 11];
        }
        mini_sum = 0;
        while ((curchr = string[++j]) && curchr != '\n') {
            mini_sum += counts[curchr];
        }
        num += sum * 10 + number_list[(mini_sum>>1) % 15 % 11];
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(num);
    _PyDict_SetItem_KnownHash(bytes_cache_8_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *string_cache_9_1;

/*[clinic input]
algos._s_AOC2021_9_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by various people for AoC 2021 day 9 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_9_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=c0229a40e04e04c5 input=0c9f1af5761177cc]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_9_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    register char curchr = string[0];
    register Py_ssize_t sum = curchr < string[1] && curchr < string[101] && curchr - '/';
    register Py_ssize_t k;
    for (k = 1; k < 99; ++k) {
        curchr = string[k];
        if (curchr < string[k - 1] &&
            curchr < string[k + 1] &&
            curchr < string[k + 101])
        {
            sum += curchr - '/';
        }
    }
    curchr = string[k];
    if (curchr < string[++k - 2] && curchr < string[k + 100]) {
        sum += curchr - '/';
    }
    ++k;
    register Py_ssize_t i, j;
    for (i = 0; i < 98; ++i) {
        curchr = string[k];
        if (curchr < string[++k] &&
            curchr < string[k + 100] &&
            curchr < string[k - 102])
        {
            sum += curchr - '/';
        }
        for (j = 1; j < 99; ++j) {
            curchr = string[k];
            if (curchr < string[++k] &&
                curchr < string[k - 2] &&
                curchr < string[k - 102] &&
                curchr < string[k + 100])
            {
                sum += curchr - '/';
            }
        }
        curchr = string[k];
        if (curchr < string[++k - 2] &&
            curchr < string[k - 102] &&
            curchr < string[k + 100])
        {
            sum += curchr - '/';
        }
        ++k;
    }
    for (j = 1; j < 99; ++j) {
        curchr = string[k];
        if (curchr < string[++k] &&
            curchr < string[k - 2] &&
            curchr < string[k - 102])
        {
            sum += curchr - '/';
        }
    }
    curchr = string[k];
    if (curchr < string[k - 1] && curchr < string[k - 101]) {
        sum += curchr - '/';
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(sum);
    _PyDict_SetItem_KnownHash(string_cache_9_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *bytes_cache_9_1;

/*[clinic input]
algos._b_AOC2021_9_1

    data: object
        Must be a bytes object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by various people for AoC 2021 day 9 part 1
[clinic start generated code]*/

static PyObject *
algos__b_AOC2021_9_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=9cd959dc0438a191 input=708f6d8505cccb89]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(bytes_cache_9_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = ((PyBytesObject *)data)->ob_sval;
    register char curchr = string[0];
    register Py_ssize_t sum = curchr < string[1] && curchr < string[101] && curchr - '/';
    register Py_ssize_t k;
    for (k = 1; k < 99; ++k) {
        curchr = string[k];
        if (curchr < string[k - 1] &&
            curchr < string[k + 1] &&
            curchr < string[k + 101])
        {
            sum += curchr - '/';
        }
    }
    curchr = string[k];
    if (curchr < string[++k - 2] && curchr < string[k + 100]) {
        sum += curchr - '/';
    }
    ++k;
    register Py_ssize_t i, j;
    for (i = 0; i < 98; ++i) {
        curchr = string[k];
        if (curchr < string[++k] &&
            curchr < string[k + 100] &&
            curchr < string[k - 102])
        {
            sum += curchr - '/';
        }
        for (j = 1; j < 99; ++j) {
            curchr = string[k];
            if (curchr < string[++k] &&
                curchr < string[k - 2] &&
                curchr < string[k - 102] &&
                curchr < string[k + 100])
            {
                sum += curchr - '/';
            }
        }
        curchr = string[k];
        if (curchr < string[++k - 2] &&
            curchr < string[k - 102] &&
            curchr < string[k + 100])
        {
            sum += curchr - '/';
        }
        ++k;
    }
    for (j = 1; j < 99; ++j) {
        curchr = string[k];
        if (curchr < string[++k] &&
            curchr < string[k - 2] &&
            curchr < string[k - 102])
        {
            sum += curchr - '/';
        }
    }
    curchr = string[k];
    if (curchr < string[k - 1] && curchr < string[k - 101]) {
        sum += curchr - '/';
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(sum);
    _PyDict_SetItem_KnownHash(bytes_cache_9_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static Py_ssize_t lookup[126] = {0};
static Py_ssize_t lookup2[126] = {0};
static Py_ssize_t equivs2[126] = {0};

static PyObject *string_cache_10_1;

/*[clinic input]
algos._s_AOC2021_10_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @SaiMonYo#0234 <@!447128951169482752> for AoC 2021 day 10 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_10_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=c5f5751e834d2af4 input=5393a6abf0c96f2f]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_10_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    char lasts[256] = {0};
    register Py_ssize_t t = 0, stackp = -1;
    register char curchar;
    for (; (curchar = *string); ++string) {
        if (curchar == '\n') {
            stackp = 0;
            lasts[0] = 0;
            continue;
        }
        if (lookup2[curchar]) {
            lasts[++stackp] = curchar;
        }
        else {
            if (lasts[stackp] != equivs2[curchar]) {
                t += lookup[curchar];
                while ((curchar = *++string) && curchar != '\n');
                if (curchar == '\0') {
                    break;
                }
                stackp = 0;
                lasts[0] = 0;
            }
            else {
                --stackp;
            }
        }
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(t);
    _PyDict_SetItem_KnownHash(string_cache_10_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *string_cache_10_2;

/*[clinic input]
algos._s_AOC2021_10_2

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @SaiMonYo#0234 <@!447128951169482752> for AoC 2021 day 10 part 2
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_10_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=aa8c889d3f5c1688 input=4524c615fa7310a4]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_10_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    char lasts[256] = {0};
    Py_ssize_t scores[256] = {0};
    register Py_ssize_t stackp = -1, scorep = 0, s = 0;
    register char curchar;
    for (; (curchar = *string); ++string) {
        if (curchar == '\n') {
            s = lookup2[lasts[stackp--]];
            while (stackp) {
                s = s * 5 + lookup2[lasts[stackp--]];
            }
            scores[scorep++] = s;
            stackp = 0;
            lasts[0] = 0;
            continue;
        }
        switch (curchar) {
        case '(':
        case '[':
        case '{':
        case '<':
            lasts[++stackp] = curchar;
            continue;
        }
        if (lasts[stackp] != equivs2[curchar]) {
            while ((curchar = *++string) && curchar != '\n');
            if (curchar == '\0') {
                break;
            }
            stackp = 0;
            lasts[0] = 0;
        }
        else {
            --stackp;
        }
    }
    qsort(scores, scorep + 1, sizeof(Py_ssize_t), compare_less);
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(scores[scorep >> 1]);
    _PyDict_SetItem_KnownHash(string_cache_10_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static inline const Py_ssize_t
flash_at(Py_ssize_t map[100],
         const register Py_ssize_t y,
         const register Py_ssize_t x)
{
    if (++map[y * 10 + x] != ':') {
        return 0;
    }
    register Py_ssize_t sum = 1;
    register Py_ssize_t x__dx, y__dy;
    for (register Py_ssize_t dy = -1; dy < 2; ++dy) {
        if ((y__dy = y + dy) >= 0 && y__dy < 10) {
            if (dy) {
                sum += flash_at(map, y__dy, x);
            }
            if ((x__dx = x - 1) >= 0) {
                sum += flash_at(map, y__dy, x__dx);
            }
            if ((x__dx = x + 1) < 10) {
                sum += flash_at(map, y__dy, x__dx);
            }
        }
    }
    return sum;
}

static PyObject *string_cache_11_1;

/*[clinic input]
algos._s_AOC2021_11_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @Eisenwave#7675 <@!162964325823283200> for AoC 2021 day 11 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_11_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=11e066b0442e731b input=2cceb736e29ac819]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_11_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    Py_ssize_t map[100];
    register Py_ssize_t j = 0;
    for (register Py_ssize_t i = 0; i < 109; ++i) {
        if (string[i] != '\n') {
            map[j++] = string[i];
        }
    }
    register Py_ssize_t flashes100 = 0;
    for (j = 0; j < 100; ++j) {
        for (register Py_ssize_t y = 0; y < 10; ++y) {
            for (register Py_ssize_t x = 0; x < 10; ++x) {
                flashes100 += flash_at(map, y, x);
            }
        }
        for (register Py_ssize_t k = 0; k < 100; ++k) {
            if (map[k] > '9') {
                map[k] = '0';
            }
        }
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(flashes100);
    _PyDict_SetItem_KnownHash(string_cache_11_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *string_cache_11_2;

/*[clinic input]
algos._s_AOC2021_11_2

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @Eisenwave#7675 <@!162964325823283200> for AoC 2021 day 11 part 2
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_11_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=ab4e2a9c89ee5cc5 input=a9f93e6d25e62555]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_11_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1);
    Py_ssize_t map[100];
    register Py_ssize_t j = 0;
    for (register Py_ssize_t i = 0; i < 109; ++i) {
        if (string[i] != '\n') {
            map[j++] = string[i];
        }
    }
    register Py_ssize_t f = 0;
    for (j = 0;; ++j) {
        f = 0;
        for (register Py_ssize_t y = 0; y < 10; ++y) {
            for (register Py_ssize_t x = 0; x < 10; ++x) {
                f += flash_at(map, y, x);
            }
        }
        if (f == 100) {
            break;
        }
        for (register Py_ssize_t k = 0; k < 100; ++k) {
            if (map[k] > '9') {
                map[k] = '0';
            }
        }
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(j + 1);
    _PyDict_SetItem_KnownHash(string_cache_11_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static Py_ssize_t nodes[128];
static int visited[128] = {0};
static Py_ssize_t mat[128][128] = {0};
static Py_ssize_t start_idx, end_idx, nodes_size = 0;

static inline const Py_ssize_t
insert_and_get_index(register const Py_ssize_t a)
{
    if (nodes_size) {
        for (register Py_ssize_t i = 0; i < nodes_size; ++i) {
            if (nodes[i] == a) {
                return i;
            }
        }
    }
    nodes[nodes_size] = a;
    return nodes_size++;
}

static inline const void
DFS(register const Py_ssize_t cur,
    register Py_ssize_t *count1,
    register Py_ssize_t *count2,
    int twice)
{
    if (cur == end_idx) {
        if (count1) {
            ++*count1;
        }
        ++*count2;
        return;
    }
    for (register Py_ssize_t n = 0; n < nodes_size; ++n) {
        if (!mat[cur][n]) {
            continue;
        }
        if (!visited[n]) {
            visited[n] = nodes[n] > 24928;
            DFS(n, count1, count2, twice);
            visited[n] = 0;
        }
        else if (!twice && n != start_idx) {
            DFS(n, NULL, count2, 1);
        }
    }
}

static PyObject *string_cache_12;

/*[clinic input]
algos._s_AOC2021_12

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @mustafaq#8791 <@!378279228002664454> for AoC 2021 day 12
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_12(PyObject *module, PyObject *data)
/*[clinic end generated code: output=299c481fe65a4434 input=59caff92287f7045]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_12, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register char *string = (char*)((PyASCIIObject *)data + 1);
    register Py_ssize_t charval, charval_2, i, j;
    register char curchar = *string;
    while (1) {
        charval = curchar;
        if ((curchar = *++string) != '-') {
            charval = (charval << 8) | curchar;
            if ((curchar = *++string) != '-') {
                if (charval == 25966) {
                    charval = 31355;
                    end_idx = nodes_size;
                }
                else {
                    charval = 1;
                    start_idx = nodes_size;
                }
                while ((curchar = *++string) != '-');
            }
        }
        charval_2 = curchar;
        if ((curchar = *++string) && curchar != '\n') {
            charval_2 = (charval_2 << 8) | curchar;
            if ((curchar = *++string) && curchar != '\n') {
                if (charval_2 == 25966) {
                    charval_2 = 31355;
                    end_idx = nodes_size;
                }
                else {
                    charval_2 = 1;
                    start_idx = nodes_size;
                }
                while ((curchar = *++string) && curchar != '\n');
                if (!curchar) {
                    break;
                }
                ++string;
            }
        }
        i = insert_and_get_index(charval);
        j = insert_and_get_index(charval_2);
        mat[i][j] = 1;
        mat[j][i] = 1;
    }
    Py_ssize_t count1 = 0, count2 = 0;
    visited[start_idx] = 1;
    DFS(start_idx, &count1, &count2, 0);
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)(count1);
    register PyLongObject *w = _PyLong_New(1);
    w->ob_digit[0] = (digit)(count2);
    register PyTupleObject *res = (PyTupleObject *)PyTuple_New(2);
    res->ob_item[0] = (PyObject *)v;
    res->ob_item[1] = (PyObject *)w;
    _PyDict_SetItem_KnownHash(string_cache_12, data, (PyObject *)res, hash);
    return (PyObject *)res;
}

static PyObject *string_cache_17_1;

/*[clinic input]
algos._s_AOC2021_17_1

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @RocketRace#0798 <@!156021301654454272> for AoC 2021 day 17 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_17_1(PyObject *module, PyObject *data)
/*[clinic end generated code: output=c0d872885530f5b0 input=5699c0c6e26e24b2]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_17_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1) + 26;
    switch (*string) {
    case '=':
        string += 2;
        break;
    case '-':
        ++string;
    }
    register digit res = (*string++ - '0') * 10;
    res += *string++ - '0';
    if (*string != '.') {
        res = res * 10 + (*string - '0');
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = res * (res - 1) / 2;
    _PyDict_SetItem_KnownHash(string_cache_17_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static PyObject *string_cache_17_2;

static inline float
_sqrt(const float number)
{
    const int_fast32_t i = 0x5f3759df - ((*(int_fast32_t *)&number)>>1);
    register float y = *(float *)&i;
    y = y * (1.5F - ( number * 0.5F * y * y ));
    return 1/y;
}

static inline const int
compare_vxel(const register int_fast32_t *a,
             const register int_fast32_t *b)
{
    register int_fast32_t a1, a2, b1, b2;
    return ((a1 = *a >> 12) >= (b1 = *b >> 12) &&
            (a2 = *a & 0xfff) > (b2 = *b & 0xfff)) - (a1 <= b1 && a2 < b2);
}

static inline const int_fast32_t
max_int_fast32_t(const register int_fast32_t a,
                 const register int_fast32_t b)
{
    return a > b ? a : b;
}

/*[clinic input]
algos._s_AOC2021_17_2

    data: object
        Must be a string object, but it isn't really checked.
    /

Very unsafe (no error checking) C-implemented solution by @Story#3481 <@!874118685940387881> for AoC 2021 day 17 part 2
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_17_2(PyObject *module, PyObject *data)
/*[clinic end generated code: output=f62a0c55384a5566 input=3c36ad3f1baa8eaf]*/
{
    const register Py_hash_t hash = PyObject_Hash(data);
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_17_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char*)((PyASCIIObject *)data + 1) + 15;
    register int_fast32_t min_x = *string++ * 10;
    min_x += *string++ - 528;
    register int_fast32_t max_x;
    if (*string != '.') {
        min_x = min_x * 10 + (*string++ - '0');
        string += 2;
        max_x = *string++ * 10;
        max_x += *string++;
        max_x = max_x * 10 + *string++ - 5328;
    }
    else {
        string += 2;
        max_x = *string++ * 10;
        max_x += *string++ - 528;
    }
    string += 5;
    register int_fast32_t min_y = *string++ * -10;
    min_y -= *string++ - 528;
    if (*string != '.') {
        min_y = min_y * 10 - (*string++ - '0');
    }
    string += 3;
    register int_fast32_t max_y = *string++ * -10;
    max_y -= *string++ - 528;
    if (*string) {
        max_y = max_y * 10 - (*string - '0');
    }
    register int_fast32_t ans = 0;
    const register int_fast32_t COMPARE_TO = 1-min_y;
    register float _sqrt_1 = (_sqrt(1.0F+8.0F*min_x) - 1.0F) / 2.0F;
    const register int_fast32_t ceil_const = (int_fast32_t)_sqrt_1 + ((int_fast32_t)(_sqrt_1 * 10) % 10 != 0);
    const register int_fast32_t floor_const = (int_fast32_t)((_sqrt(1.0F+8.0F*max_x) - 1.0F) / 2.0F);
    for (register int_fast32_t _vy = min_y; _vy < COMPARE_TO; ++_vy) {
        register int_fast32_t vy = _vy;
        register int_fast32_t vx_size = 0;
        int_fast32_t vx[4096] = {0};
        register int_fast32_t y = 0, i = 0;
        while (min_y <= y) {
            if (y <= max_y) {
                register int_fast32_t tri = i * (i+1) / 2;
                register int_fast32_t lo_vx, hi_vx;
                if (tri >= min_x) {
                    lo_vx = ceil_const;
                    hi_vx = tri > max_x ? floor_const : i + (max_x - tri)/i;
                }
                else {
                    lo_vx = i + (min_x-tri + i-1)/i;
                    hi_vx = i + (max_x - tri)/i;
                }
                vx[vx_size++] = (lo_vx << 12) | hi_vx;
            }
            ++i;
            y += vy;
            --vy;
        }
        if (vx_size == 0) {
            continue;
        }
        qsort(vx, vx_size, sizeof(int_fast32_t), compare_vxel);
        /*
        if (vx_size > 1) {
            const register int_fast32_t temp = vx[vx_size - 2];
            vx[vx_size - 2] = vx[vx_size - 1];
            vx[vx_size - 1] = temp;
        }
        */
        register int_fast32_t mn, mx;
        mx = vx[0] & 0xfff;
        mn = vx[0] >> 12;
        register int_fast32_t ans_1 = 0;
        for (i = 1; i < vx_size; ++i) {
            const register int_fast32_t q = vx[i] & 0xfff;
            const register int_fast32_t p = vx[i] >> 12;
            if (p <= mx) {
                mx = max_int_fast32_t(mx, q);
            }
            else {
                ans_1 += mx-mn + 1;
                mn, mx = p, q;
            }
        }
        ans += ans_1 + mx-mn + 1;
    }
    register PyLongObject *v = _PyLong_New(1);
    v->ob_digit[0] = (digit)ans;
    _PyDict_SetItem_KnownHash(string_cache_17_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static int_fast64_t AOC2021_21_1_values[11][11] = {
    {0,      0,      0,        0,      0,      0,      0,      0,        0,      0,      0},
    {0, 594042, 594042,   897798, 594042, 428736, 594042, 594042,   897798, 594042, 428736},
    {0, 790608, 790608,  1196172, 790608, 571032, 790608, 790608,  1196172, 790608, 571032},
    {0, 987174, 987174,  1073709, 987174, 713328, 987174, 987174,  1067724, 987174, 713328},
    {0, 920079, 916083,   742257, 908091, 855624, 900099, 906093,   752247, 918081, 855624},
    {0, 978486, 978486,  1073709, 978486, 711480, 978486, 978486,  1067724, 978486, 711480},
    {0, 920079, 916083,   742257, 908091, 853776, 900099, 906093,   752247, 918081, 853776},
    {0, 684495, 678468,   551901, 675024, 798147, 671580, 674163,   556206, 679329, 802452},
    {0, 518418, 513936,   412344, 504972, 597600, 503478, 506466,   419814, 512442, 605070},
    {0, 982830, 982830,  1073709, 982830, 707784, 982830, 982830,  1067724, 982830, 707784},
    {0, 920079, 916083,   742257, 908091, 850080, 900099, 906093,   752247, 918081, 850080}
};

static PyObject *string_cache_21_1;

/*[clinic input]
algos._s_AOC2021_21_1

    data: object
        Must be a string, but it isn't really checked

C-implemented version of the hardcoded solution by @RocketRace#0798 <@!156021301654454272> for AoC day 21 part 1
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_21_1_impl(PyObject *module, PyObject *data)
/*[clinic end generated code: output=0a65ffc8b5b96caa input=52d3aac1b56ad38b]*/
{
    const register Py_hash_t hash = ((PyASCIIObject *)data)->hash;
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_21_1, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char *)((PyASCIIObject *)data + 1) + 28;
    register int_fast64_t a = *string++;
    if (*string != '\n') {
        a = (a*10 + *string++) - 528;
    }
    else {
        a -= '0';
    }
    string += 29;
    register int_fast64_t b = *string++;
    if (*string) {
        b = (b*10 + *string++) - 528;
    }
    else {
        b -= '0';
    }
    const register int_fast64_t res = AOC2021_21_1_values[a][b];
    register PyLongObject *v = _PyLong_New(2);
    if (res > (int_fast64_t)1073741823) {
        v->ob_digit[0] = (digit)(res & (int_fast64_t)1073741823);
        v->ob_digit[1] = (digit)(res >> 30);
    }
    else {
        v->ob_digit[0] = (digit)res;
        v->ob_digit[1] = 0;
    }
    _PyDict_SetItem_KnownHash(string_cache_21_1, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static int_fast64_t AOC2021_21_2_values[11][11] = {
    {0,               0,               0,               0,               0,               0,                 0,               0,               0,               0,               0},
    {0,  32491093007709,  27674034218179,  48868319769358,  97774467368562, 138508043837521,   157253621231420, 141740702114011, 115864149937553,  85048040806299,  57328067654557},
    {0,  27464148626406,  24411161361207,  45771240990345,  93049942628388, 131888061854776,   149195946847792, 133029050096658, 106768284484217,  76262326668116,  49975322685009},
    {0,  51863007694527,  45198749672670,  93013662727308, 193753136998081, 275067741811212,   309991007938181, 273042027784929, 214368059463212, 147573255754448,  92399285032143},
    {0, 110271560863819,  91559198282731, 193170338541590, 404904579900696, 575111835924670,   647608359455719, 568867175661958, 444356092776315, 303121579983974, 187451244607486},
    {0, 156667189442502, 129742452789556, 274195599086465, 575025114466224, 816800855030343,   919758187195363, 807873766901514, 630947104784464, 430229563871565, 265845890886828},
    {0, 175731756652760, 146854918035875, 309196008717909, 647920021341197, 920342039518611,  1036584236547450, 911090395997650, 712381680443927, 486638407378784, 301304993766094},
    {0, 152587196649184, 131180774190079, 272847859601291, 570239341223618, 809953813657517,   912857726749764, 803934725594806, 630797200227453, 433315766324816, 270005289024391},
    {0, 116741133558209, 105619718613031, 214924284932572, 446968027750017, 634769613696613,   716241959649754, 632979211251440, 499714329362294, 346642902541848, 218433063958910},
    {0,  83778196139157,  75823864479001, 148747830493442, 306621346123766, 435288918824107,   492043106122795, 437256456198320, 348577682881276, 245605000281051, 157595953724471},
    {0,  56852759190649,  49982165861983,  93726416205179, 190897246590017, 270803396243039,   306719685234774, 274291038026362, 221109915584112, 158631174219251, 104001566545663}
};

static PyObject *string_cache_21_2;

/*[clinic input]
algos._s_AOC2021_21_2

    data: object
        Must be a string, but it isn't really checked

C-implemented version of the hardcoded solution by @RocketRace#0798 <@!156021301654454272> for AoC day 21 part 2
[clinic start generated code]*/

static PyObject *
algos__s_AOC2021_21_2_impl(PyObject *module, PyObject *data)
/*[clinic end generated code: output=f60112850d716134 input=693b4e1f5a966558]*/
{
    const register Py_hash_t hash = ((PyASCIIObject *)data)->hash;
    register PyObject *result = _PyDict_GetItem_KnownHash(string_cache_21_2, data, hash);
    if (result) {
        Py_INCREF(result);
        return result;
    }
    register const char *string = (const char *)((PyASCIIObject *)data + 1) + 28;
    register int_fast64_t a = *string++;
    if (*string != '\n') {
        a = (a*10 + *string++) - 528;
    }
    else {
        a -= '0';
    }
    string += 29;
    register int_fast64_t b = *string++;
    if (*string) {
        b = (b*10 + *string++) - 528;
    }
    else {
        b -= '0';
    }
    const register int_fast64_t res = AOC2021_21_2_values[a][b];
    register PyLongObject *v = _PyLong_New(2);
    v->ob_digit[0] = (digit)(res & (int_fast64_t)1073741823);
    v->ob_digit[1] = (digit)(res >> 30);
    _PyDict_SetItem_KnownHash(string_cache_21_2, data, (PyObject *)v, hash);
    return (PyObject *)v;
}

static const uint32_t M1 = 0x55555555;
static const uint32_t M2 = 0x33333333;
static const uint32_t M4 = 0x0F0F0F0F;
static const uint32_t SUM = 0x01010101;

static const inline uint32_t
bit_count_32(register uint32_t x)
{
    x -= ((x >> 1) & M1);
    x = (x & M2) + ((x >> 2) & M2);
    x = (x + (x >> 4)) & M4;
    return (uint32_t)((uint64_t)x * (uint64_t)SUM) >> 24;
}

/*[clinic input]
algos._bit_count

    integer: object
        Must be a python int, but it isn't really checked.
    /

Raw version of int.bit_count, very unsafe (no error checking)
[clinic start generated code]*/

static PyObject *
algos__bit_count(PyObject *module, PyObject *integer)
/*[clinic end generated code: output=a989aca8ad9e32d1 input=bcc200c6c3840e61]*/
{
    register size_t sum = 0;
    const register Py_ssize_t size_int = absolute_value(
        ((const PyVarObject *)integer)->ob_size
    );
    const digit *_int = ((PyLongObject *)integer)->ob_digit;
    for (register Py_ssize_t i = 0; i < size_int; ++i) {
        sum += (size_t)bit_count_32(_int[i]);
    }
    return PyLong_FromSize_t(sum);
}

#define METH(T, D, P) \
    ALGOS__ ## T ## _AOC2021_ ## D ## _ ## P ## _METHODDEF

#define METH_D(T, D) \
    ALGOS__ ## T ## _AOC2021_ ## D ## _METHODDEF

static PyMethodDef AlgosMethods[] = {
    ALGOS_INSERTION_SORT_METHODDEF
    ALGOS_QUICK_SORT_METHODDEF
    {"adder_py", (PyCFunction)(void(*)(void))algos_adder_py,
        METH_FASTCALL|METH_KEYWORDS, algos_adder_py__doc__},
    METH(S, 1, 1)
    METH(S, 1, 2)
    ALGOS_AOC2021_6_2_METHODDEF
    METH(L, 6, 2)
    METH(S, 6, 2)
    METH(B, 6, 2)
    METH(S, 7, 1)
    METH(S, 8, 1)
    METH(B, 8, 1)
    METH(S, 8, 2)
    METH(B, 8, 2)
    METH(S, 9, 1)
    METH(B, 9, 1)
    METH(S, 10, 1)
    METH(S, 10, 2)
    METH(S, 11, 1)
    METH(S, 11, 2)
    METH_D(S, 12)
    METH(S, 17, 1)
    METH(S, 17, 2)
    METH(S, 21, 1)
    METH(S, 21, 2)
    ALGOS__BIT_COUNT_METHODDEF
    {NULL}
};

#define CLEAR_DICT(name) \
    PyDict_Type.tp_clear(name);
static void
algos_clear()
{
    CLEAR_DICT(string_cache_1_1)
    CLEAR_DICT(string_cache_1_2)
    CLEAR_DICT(string_cache_6_2)
    CLEAR_DICT(bytes_cache_6_2)
    CLEAR_DICT(string_cache_7_1)
    CLEAR_DICT(string_cache_8_1)
    CLEAR_DICT(bytes_cache_8_1)
    CLEAR_DICT(string_cache_8_2)
    CLEAR_DICT(bytes_cache_8_2)
    CLEAR_DICT(string_cache_9_1)
    CLEAR_DICT(bytes_cache_9_1)
    CLEAR_DICT(string_cache_10_1)
    CLEAR_DICT(string_cache_10_2)
    CLEAR_DICT(string_cache_11_1)
    CLEAR_DICT(string_cache_11_2)
    CLEAR_DICT(string_cache_12)
    CLEAR_DICT(string_cache_17_1)
    CLEAR_DICT(string_cache_17_2)
    CLEAR_DICT(string_cache_21_1)
    CLEAR_DICT(string_cache_21_2)
}
#undef CLEAR_DICT

PyDoc_STRVAR(algos___doc__,
             "Module for implemented algorithms in C.");

static struct PyModuleDef algosmodule = {
    PyModuleDef_HEAD_INIT,
    "algos",
    algos___doc__,
    -1,
    AlgosMethods,
    0,
    0,
    0,
    algos_clear,
};

#define NEW_DICT(name) \
    name = PyDict_New(); \
    if (name == NULL) { \
        return NULL; \
    }
#define NEW_STR_VAR(name) \
    name ## _STR = PyUnicode_FromString(#name);
PyMODINIT_FUNC
PyInit_algos_c(void)
{
    lookup[')'] = 3;
    lookup['>'] = 25137;
    lookup[']'] = 57;
    lookup['}'] = 1197;
    lookup2['('] = 1;
    lookup2['<'] = 4;
    lookup2['['] = 2;
    lookup2['{'] = 3;
    equivs2[')'] = '(';
    equivs2['>'] = '<';
    equivs2[']'] = '[';
    equivs2['}'] = '{';
    NEW_STR_VAR(stop)
    NEW_STR_VAR(initial)
    NEW_STR_VAR(start)
    NEW_STR_VAR(step)
    NEW_STR_VAR(use_objects)
    NEW_DICT(string_cache_1_1)
    NEW_DICT(string_cache_1_2)
    NEW_DICT(string_cache_6_2)
    NEW_DICT(bytes_cache_6_2)
    NEW_DICT(string_cache_7_1)
    NEW_DICT(string_cache_8_1)
    NEW_DICT(bytes_cache_8_1)
    NEW_DICT(string_cache_8_2)
    NEW_DICT(bytes_cache_8_2)
    NEW_DICT(string_cache_9_1)
    NEW_DICT(bytes_cache_9_1)
    NEW_DICT(string_cache_10_1)
    NEW_DICT(string_cache_10_2)
    NEW_DICT(string_cache_11_1)
    NEW_DICT(string_cache_11_2)
    NEW_DICT(string_cache_12)
    NEW_DICT(string_cache_17_1)
    NEW_DICT(string_cache_17_2)
    NEW_DICT(string_cache_21_1)
    NEW_DICT(string_cache_21_2)
    return PyModule_Create(&algosmodule);
}
#undef NEW_DICT
