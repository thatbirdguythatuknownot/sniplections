/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(algos_insertion_sort__doc__,
"insertion_sort($module, /, array, low=0, high=-1)\n"
"--\n"
"\n"
"Insertion sort implementation, sorting a list inplace.\n"
"\n"
"  array\n"
"    The array to sort. Must be a list.\n"
"  low\n"
"    Sort from this point in the array (inclusive). Must be positive and\n"
"    must be less than or equal to the \'high\' argument.\n"
"    0 is the default value.\n"
"  high\n"
"    Sort to this point in the array (exclusive). Must be positive.\n"
"    -1 is the default value, which will default to len(array).");

#define ALGOS_INSERTION_SORT_METHODDEF    \
    {"insertion_sort", (PyCFunction)(void(*)(void))algos_insertion_sort, METH_FASTCALL|METH_KEYWORDS, algos_insertion_sort__doc__},

static PyObject *
algos_insertion_sort_impl(PyObject *module, PyObject *array, Py_ssize_t low,
                          Py_ssize_t high);

static PyObject *
algos_insertion_sort(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"array", "low", "high", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "insertion_sort", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *array;
    Py_ssize_t low = 0;
    Py_ssize_t high = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    array = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            low = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        high = ival;
    }
skip_optional_pos:
    return_value = algos_insertion_sort_impl(module, array, low, high);

exit:
    return return_value;
}

PyDoc_STRVAR(algos_quick_sort__doc__,
"quick_sort($module, /, array, low=0, high=-1, length=-1)\n"
"--\n"
"\n"
"Quick-sort implementation, sorting a list inplace.\n"
"\n"
"  array\n"
"    The array to sort. Must be a list.\n"
"  low\n"
"    Sort from this point in the array (inclusive). Must be positive and\n"
"    must be less than or equal to the \'high\' argument.\n"
"    0 is the default value.\n"
"  high\n"
"    Sort to this point in the array (inclusive). Must be positive and\n"
"    must be less than the \'length\' argument.\n"
"    -1 is the default value, which will default to len(array) - 1.\n"
"  length\n"
"    Sort the array to array[:length]. Must be positive and\n"
"    if greater than len(array) will default to len(array).\n"
"    -1 is the default value, which will default to len(array).\n"
"    \'high\' - \'low\' + 1 must be greater than or equal to this value;\n"
"    If it is greater than, \'high\' is clamped.");

#define ALGOS_QUICK_SORT_METHODDEF    \
    {"quick_sort", (PyCFunction)(void(*)(void))algos_quick_sort, METH_FASTCALL|METH_KEYWORDS, algos_quick_sort__doc__},

static PyObject *
algos_quick_sort_impl(PyObject *module, PyObject *array, Py_ssize_t low,
                      Py_ssize_t high, Py_ssize_t length);

static PyObject *
algos_quick_sort(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"array", "low", "high", "length", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "quick_sort", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *array;
    Py_ssize_t low = 0;
    Py_ssize_t high = -1;
    Py_ssize_t length = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    array = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            low = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            high = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[3]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
skip_optional_pos:
    return_value = algos_quick_sort_impl(module, array, low, high, length);

exit:
    return return_value;
}

PyDoc_STRVAR(algos__s_AOC2021_1_1__doc__,
"_s_AOC2021_1_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @mustafaq#8791 (<@!378279228002664454>) for AoC 2021 day 1 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_1_1_METHODDEF    \
    {"_s_AOC2021_1_1", (PyCFunction)algos__s_AOC2021_1_1, METH_O, algos__s_AOC2021_1_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_1_2__doc__,
"_s_AOC2021_1_2($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @Eisenwave#7675 <@!162964325823283200> for AoC 2021 day 1 part 2\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_1_2_METHODDEF    \
    {"_s_AOC2021_1_2", (PyCFunction)algos__s_AOC2021_1_2, METH_O, algos__s_AOC2021_1_2__doc__},

PyDoc_STRVAR(algos_AOC2021_6_2__doc__,
"AOC2021_6_2($module, /, data)\n"
"--\n"
"\n"
"C-implemented version of the hardcoded values solution of @ConfusedReptile#6830 for AoC 2021 day 6 part 2\n"
"\n"
"  data\n"
"    Must be an iterable of ints with __iter__ implemented.");

#define ALGOS_AOC2021_6_2_METHODDEF    \
    {"AOC2021_6_2", (PyCFunction)(void(*)(void))algos_AOC2021_6_2, METH_FASTCALL|METH_KEYWORDS, algos_AOC2021_6_2__doc__},

static PyObject *
algos_AOC2021_6_2_impl(PyObject *module, PyObject *data);

static PyObject *
algos_AOC2021_6_2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "AOC2021_6_2", 0};
    PyObject *argsbuf[1];
    PyObject *data;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    data = args[0];
    return_value = algos_AOC2021_6_2_impl(module, data);

exit:
    return return_value;
}

PyDoc_STRVAR(algos__l_AOC2021_6_2__doc__,
"_l_AOC2021_6_2($module, data, /)\n"
"--\n"
"\n"
"Raw version of algos.AOC2021_6_2 but it only takes lists, very unsafe (no error checking)\n"
"\n"
"  data\n"
"    Must be a list, but it isn\'t really checked.");

#define ALGOS__L_AOC2021_6_2_METHODDEF    \
    {"_l_AOC2021_6_2", (PyCFunction)algos__l_AOC2021_6_2, METH_O, algos__l_AOC2021_6_2__doc__},

PyDoc_STRVAR(algos__s_AOC2021_6_2__doc__,
"_s_AOC2021_6_2($module, data, /)\n"
"--\n"
"\n"
"Raw version of algos.AOC2021_6_2 but it takes a string, very unsafe (no error checking)\n"
"\n"
"  data\n"
"    Must be an ASCII string, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_6_2_METHODDEF    \
    {"_s_AOC2021_6_2", (PyCFunction)algos__s_AOC2021_6_2, METH_O, algos__s_AOC2021_6_2__doc__},

PyDoc_STRVAR(algos__b_AOC2021_6_2__doc__,
"_b_AOC2021_6_2($module, data, /)\n"
"--\n"
"\n"
"Raw version of algos.AOC2021_6_2 but it takes a bytes object, very unsafe (no error checking)\n"
"\n"
"  data\n"
"    Must be a bytes object, but it isn\'t really checked.");

#define ALGOS__B_AOC2021_6_2_METHODDEF    \
    {"_b_AOC2021_6_2", (PyCFunction)algos__b_AOC2021_6_2, METH_O, algos__b_AOC2021_6_2__doc__},

PyDoc_STRVAR(algos__s_AOC2021_7_1__doc__,
"_s_AOC2021_7_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @salt#2080 (<@!431341013479718912>) for AoC 2021 day 7 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_7_1_METHODDEF    \
    {"_s_AOC2021_7_1", (PyCFunction)algos__s_AOC2021_7_1, METH_O, algos__s_AOC2021_7_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_8_1__doc__,
"_s_AOC2021_8_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @dementati#4691 (<@!181803748262281216>) and @mustafaq#8791 (<@!378279228002664454>) for AoC 2021 day 8 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_8_1_METHODDEF    \
    {"_s_AOC2021_8_1", (PyCFunction)algos__s_AOC2021_8_1, METH_O, algos__s_AOC2021_8_1__doc__},

PyDoc_STRVAR(algos__b_AOC2021_8_1__doc__,
"_b_AOC2021_8_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @dementati#4691 (<@!181803748262281216>) and @mustafaq#8791 (<@!378279228002664454>) for AoC 2021 day 8 part 1\n"
"\n"
"  data\n"
"    Must be a bytes object, but it isn\'t really checked.");

#define ALGOS__B_AOC2021_8_1_METHODDEF    \
    {"_b_AOC2021_8_1", (PyCFunction)algos__b_AOC2021_8_1, METH_O, algos__b_AOC2021_8_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_8_2__doc__,
"_s_AOC2021_8_2($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution posted by @tessaract#5963 <@!417374125015695373> for AoC 2021 day 8 part 2\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_8_2_METHODDEF    \
    {"_s_AOC2021_8_2", (PyCFunction)algos__s_AOC2021_8_2, METH_O, algos__s_AOC2021_8_2__doc__},

PyDoc_STRVAR(algos__b_AOC2021_8_2__doc__,
"_b_AOC2021_8_2($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution posted by @tessaract#5963 <@!417374125015695373> for AoC 2021 day 8 part 2\n"
"\n"
"  data\n"
"    Must be a bytes object, but it isn\'t really checked.");

#define ALGOS__B_AOC2021_8_2_METHODDEF    \
    {"_b_AOC2021_8_2", (PyCFunction)algos__b_AOC2021_8_2, METH_O, algos__b_AOC2021_8_2__doc__},

PyDoc_STRVAR(algos__s_AOC2021_9_1__doc__,
"_s_AOC2021_9_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by various people for AoC 2021 day 9 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_9_1_METHODDEF    \
    {"_s_AOC2021_9_1", (PyCFunction)algos__s_AOC2021_9_1, METH_O, algos__s_AOC2021_9_1__doc__},

PyDoc_STRVAR(algos__b_AOC2021_9_1__doc__,
"_b_AOC2021_9_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by various people for AoC 2021 day 9 part 1\n"
"\n"
"  data\n"
"    Must be a bytes object, but it isn\'t really checked.");

#define ALGOS__B_AOC2021_9_1_METHODDEF    \
    {"_b_AOC2021_9_1", (PyCFunction)algos__b_AOC2021_9_1, METH_O, algos__b_AOC2021_9_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_10_1__doc__,
"_s_AOC2021_10_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @SaiMonYo#0234 <@!447128951169482752> for AoC 2021 day 10 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_10_1_METHODDEF    \
    {"_s_AOC2021_10_1", (PyCFunction)algos__s_AOC2021_10_1, METH_O, algos__s_AOC2021_10_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_10_2__doc__,
"_s_AOC2021_10_2($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @SaiMonYo#0234 <@!447128951169482752> for AoC 2021 day 10 part 2\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_10_2_METHODDEF    \
    {"_s_AOC2021_10_2", (PyCFunction)algos__s_AOC2021_10_2, METH_O, algos__s_AOC2021_10_2__doc__},

PyDoc_STRVAR(algos__s_AOC2021_11_1__doc__,
"_s_AOC2021_11_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @Eisenwave#7675 <@!162964325823283200> for AoC 2021 day 11 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_11_1_METHODDEF    \
    {"_s_AOC2021_11_1", (PyCFunction)algos__s_AOC2021_11_1, METH_O, algos__s_AOC2021_11_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_11_2__doc__,
"_s_AOC2021_11_2($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @Eisenwave#7675 <@!162964325823283200> for AoC 2021 day 11 part 2\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_11_2_METHODDEF    \
    {"_s_AOC2021_11_2", (PyCFunction)algos__s_AOC2021_11_2, METH_O, algos__s_AOC2021_11_2__doc__},

PyDoc_STRVAR(algos__s_AOC2021_12__doc__,
"_s_AOC2021_12($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @mustafaq#8791 <@!378279228002664454> for AoC 2021 day 12\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_12_METHODDEF    \
    {"_s_AOC2021_12", (PyCFunction)algos__s_AOC2021_12, METH_O, algos__s_AOC2021_12__doc__},

PyDoc_STRVAR(algos__s_AOC2021_17_1__doc__,
"_s_AOC2021_17_1($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @RocketRace#0798 <@!156021301654454272> for AoC 2021 day 17 part 1\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_17_1_METHODDEF    \
    {"_s_AOC2021_17_1", (PyCFunction)algos__s_AOC2021_17_1, METH_O, algos__s_AOC2021_17_1__doc__},

PyDoc_STRVAR(algos__s_AOC2021_17_2__doc__,
"_s_AOC2021_17_2($module, data, /)\n"
"--\n"
"\n"
"Very unsafe (no error checking) C-implemented solution by @Story#3481 <@!874118685940387881> for AoC 2021 day 17 part 2\n"
"\n"
"  data\n"
"    Must be a string object, but it isn\'t really checked.");

#define ALGOS__S_AOC2021_17_2_METHODDEF    \
    {"_s_AOC2021_17_2", (PyCFunction)algos__s_AOC2021_17_2, METH_O, algos__s_AOC2021_17_2__doc__},

PyDoc_STRVAR(algos__s_AOC2021_21_1__doc__,
"_s_AOC2021_21_1($module, /, data)\n"
"--\n"
"\n"
"C-implemented version of the hardcoded solution by @RocketRace#0798 <@!156021301654454272> for AoC day 21 part 1\n"
"\n"
"  data\n"
"    Must be a string, but it isn\'t really checked");

#define ALGOS__S_AOC2021_21_1_METHODDEF    \
    {"_s_AOC2021_21_1", (PyCFunction)(void(*)(void))algos__s_AOC2021_21_1, METH_FASTCALL|METH_KEYWORDS, algos__s_AOC2021_21_1__doc__},

static PyObject *
algos__s_AOC2021_21_1_impl(PyObject *module, PyObject *data);

static PyObject *
algos__s_AOC2021_21_1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_s_AOC2021_21_1", 0};
    PyObject *argsbuf[1];
    PyObject *data;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    data = args[0];
    return_value = algos__s_AOC2021_21_1_impl(module, data);

exit:
    return return_value;
}

PyDoc_STRVAR(algos__s_AOC2021_21_2__doc__,
"_s_AOC2021_21_2($module, /, data)\n"
"--\n"
"\n"
"C-implemented version of the hardcoded solution by @RocketRace#0798 <@!156021301654454272> for AoC day 21 part 2\n"
"\n"
"  data\n"
"    Must be a string, but it isn\'t really checked");

#define ALGOS__S_AOC2021_21_2_METHODDEF    \
    {"_s_AOC2021_21_2", (PyCFunction)(void(*)(void))algos__s_AOC2021_21_2, METH_FASTCALL|METH_KEYWORDS, algos__s_AOC2021_21_2__doc__},

static PyObject *
algos__s_AOC2021_21_2_impl(PyObject *module, PyObject *data);

static PyObject *
algos__s_AOC2021_21_2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_s_AOC2021_21_2", 0};
    PyObject *argsbuf[1];
    PyObject *data;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    data = args[0];
    return_value = algos__s_AOC2021_21_2_impl(module, data);

exit:
    return return_value;
}

PyDoc_STRVAR(algos__bit_count__doc__,
"_bit_count($module, integer, /)\n"
"--\n"
"\n"
"Raw version of int.bit_count, very unsafe (no error checking)\n"
"\n"
"  integer\n"
"    Must be a python int, but it isn\'t really checked.");

#define ALGOS__BIT_COUNT_METHODDEF    \
    {"_bit_count", (PyCFunction)algos__bit_count, METH_O, algos__bit_count__doc__},
/*[clinic end generated code: output=817630d02c1e2b49 input=a9049054013a1b77]*/
