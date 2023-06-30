#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/ndarraytypes.h"
#include "numpy/npy_3kcompat.h"

#define NPY_DTYPE(descr) ((PyArray_DTypeMeta *)Py_TYPE(descr))

#if defined(__OPTIMIZE__)

/* LIKELY() definition */
/* Checks taken from
    https://github.com/python/cpython/blob/main/Objects/obmalloc.c */
#if defined(__GNUC__) && (__GNUC__ > 2)
#  define UNLIKELY(value) __builtin_expect((value), 0)
#else
#  define UNLIKELY(value) (value)
#endif

#else

#define UNLIKELY(value) (value)

#endif /* __OPTIMIZE__ */

PyArray_Descr *DOUBLE_Descr;

Py_LOCAL_INLINE(void)
pdist_cext_impl(const char *A,
                const char *B,
                char *out,
                const size_t n,
                const size_t k,
                const size_t m,
                const size_t sk,
                const size_t sm)
{
    size_t i, j, l;
    double tmp, tmp2;
    const double *Adata, *Bdata;
    double *outdata;

    for (i = 0; i < n; ++i) {
        Adata = (const double *)(A + i * sk);
        outdata = (double *)(out + i * sm);
        for (j = 0; j < m; ++j) {
            Bdata = (double *)(B + j * sk);
            tmp = 0;
            for (l = 0; l < k; ++l) {
                tmp2 = Adata[l] - Bdata[l];
                tmp += tmp2 * tmp2;
            }
            outdata[j] = sqrt(tmp);
        }
    }
}

static PyObject *
pdist_cext_main(PyObject *Py_UNUSED(_),
                PyObject *const *args,
                const Py_ssize_t nargs)
{
    PyArrayObject *As = NULL, *Bs = NULL, *res;
    size_t n, m, k;

    if (UNLIKELY(nargs != 2)) {
        PyErr_Format(PyExc_TypeError,
                     "pdist_cext.main() expected exactly 2 positional "
                     "arguments, got %zd",
                     nargs);
        return NULL;
    }

    Py_INCREF(DOUBLE_Descr);
    As = (PyArrayObject *)PyArray_CheckFromAny(
        args[0], DOUBLE_Descr,
        2, 2,
        NPY_ARRAY_CARRAY_RO | NPY_ARRAY_NOTSWAPPED,
        NULL);
    if (UNLIKELY(As == NULL)) return NULL;
    Py_INCREF(DOUBLE_Descr);
    Bs = (PyArrayObject *)PyArray_CheckFromAny(
        args[1], DOUBLE_Descr,
        2, 2,
        NPY_ARRAY_CARRAY_RO | NPY_ARRAY_NOTSWAPPED,
        NULL);
    if (UNLIKELY(Bs == NULL)) goto error;

    n = PyArray_DIM(As, 0);
    m = PyArray_DIM(Bs, 0);
    const npy_intp dims[2] = {n, m};
    Py_INCREF(DOUBLE_Descr);
    res = (PyArrayObject *)PyArray_NewFromDescr(
        &PyArray_Type, DOUBLE_Descr, 2,
        dims, NULL, NULL, 0, NULL
    );
    if (UNLIKELY(res == NULL)) goto error;

    k = PyArray_DIM(As, 1);

    pdist_cext_impl(PyArray_BYTES(As),
                    PyArray_BYTES(Bs),
                    PyArray_BYTES(res),
                    n, k, m,
                    sizeof(double) * k,
                    sizeof(double) * m);
    Py_DECREF(As);
    Py_DECREF(Bs);

    return (PyObject *)res;
  error:
    Py_XDECREF(As);
    Py_XDECREF(Bs);
    return NULL;
}

static PyMethodDef pdist_cext_methods[] = {
    {"main", (PyCFunction)pdist_cext_main, METH_FASTCALL, ""},
    {NULL}
};

static struct PyModuleDef pdist_cextmodule = {
    PyModuleDef_HEAD_INIT,
    "pdist_cext",
    "",
    0,
    pdist_cext_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_pdist_cext(void)
{
    import_array();

    DOUBLE_Descr = PyArray_DescrFromType(NPY_DOUBLE);

    return PyModule_Create(&pdist_cextmodule);
}
