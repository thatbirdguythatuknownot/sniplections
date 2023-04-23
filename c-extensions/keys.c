#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define Py_BUILD_CORE
#include "internal/pycore_global_objects.h"
#include "internal/pycore_runtime.h"
#undef Py_BUILD_CORE

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
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

#define CHECK_KEY_ASCII(i) \
    state = GetKeyState(i); \
    if (BUNLIKELY(state & 0x8000)) { \
        k = MapVirtualKeyA(i, 2); \
        s = (PyObject *)&_Py_SINGLETON(strings).ascii[k]; \
        Py_INCREF(s); \
        items[idx++] = s; \
    }

#define CHECK_KEY_ASCII_SET(i) \
    state = GetKeyState(i); \
    if (BUNLIKELY(state & 0x8000)) { \
        k = MapVirtualKeyA(i, 2); \
        if (LIKELY(!is_char_set[k])) { \
            is_char_set[k] = 1; \
            s = (PyObject *)&_Py_SINGLETON(strings).ascii[k]; \
            Py_INCREF(s); \
            items[idx++] = s; \
        } \
    }

#define CHECK_KEY(i, s) \
    state = GetKeyState(i); \
    if (BUNLIKELY(state & 0x8000)) { \
        Py_INCREF(s); \
        items[idx++] = s; \
    }

#define CHECK_KEY_SET(i, s, j) \
    if (!is_vk_set[j]) { \
        state = GetKeyState(i); \
        if (BUNLIKELY(state & 0x8000)) { \
            is_vk_set[j] = 1; \
            Py_INCREF(s); \
            items[idx++] = s; \
        } \
    }

static PyObject *Shift_o;
static PyObject *Ctrl_o;
static PyObject *LAlt_o;
static PyObject *Back_o;
static PyObject *Tab_o;
static PyObject *Enter_o;
static PyObject *Esc_o;
static PyObject *Caps_o;
static PyObject *PgUp_o;
static PyObject *PgDn_o;
static PyObject *End_o;
static PyObject *Home_o;
static PyObject *Left_o;
static PyObject *Up_o;
static PyObject *Right_o;
static PyObject *Down_o;
static PyObject *PrtSc_o;
static PyObject *Insert_o;
static PyObject *Del_o;
static PyObject *Win_o;
static PyObject *F1_o;
static PyObject *F2_o;
static PyObject *F3_o;
static PyObject *F4_o;
static PyObject *F5_o;
static PyObject *F6_o;
static PyObject *F7_o;
static PyObject *F8_o;
static PyObject *F9_o;
static PyObject *F10_o;
static PyObject *F11_o;
static PyObject *F12_o;
static PyObject *F13_o;
static PyObject *F14_o;
static PyObject *F15_o;
static PyObject *F16_o;
static PyObject *F17_o;
static PyObject *F18_o;
static PyObject *F19_o;
static PyObject *F20_o;
static PyObject *F21_o;
static PyObject *F22_o;
static PyObject *F23_o;
static PyObject *F24_o;
static PyObject *NumLock_o;
static PyObject *SclLock_o;
static PyObject *RAlt_o;

static PyObject *
keys_get_keys(PyObject *self, PyObject *Py_UNUSED(o))
{
    PyObject *list;
#ifdef _WIN32
    PyObject *s, **items;
    unsigned short state;
    unsigned char k;
    Py_ssize_t idx = 0;
    int is_char_set[128] = {0};
    int is_vk_set[256] = {0};

    list = PyList_New(112);
    if (UNLIKELY(list == NULL)) {
        return NULL;
    }
    items = ((PyListObject *)list)->ob_item;

    CHECK_KEY(0x08, Back_o);
    CHECK_KEY(0x09, Tab_o);
    CHECK_KEY(0x0d, Enter_o);
    CHECK_KEY(0x10, Shift_o);
    CHECK_KEY(0x11, Ctrl_o);
    CHECK_KEY(0x14, Caps_o);
    CHECK_KEY(0x1b, Esc_o);

    CHECK_KEY_ASCII(0x20);

    CHECK_KEY(0x21, PgUp_o);
    CHECK_KEY(0x22, PgDn_o);
    CHECK_KEY(0x23, End_o);
    CHECK_KEY(0x24, Home_o);
    CHECK_KEY(0x25, Left_o);
    CHECK_KEY(0x26, Up_o);
    CHECK_KEY(0x27, Right_o);
    CHECK_KEY(0x28, Down_o);
    CHECK_KEY(0x2c, PrtSc_o);
    CHECK_KEY(0x2d, Insert_o);
    CHECK_KEY(0x2e, Del_o);

    CHECK_KEY_ASCII_SET(0x30);
    CHECK_KEY_ASCII_SET(0x31);
    CHECK_KEY_ASCII_SET(0x32);
    CHECK_KEY_ASCII_SET(0x33);
    CHECK_KEY_ASCII_SET(0x34);
    CHECK_KEY_ASCII_SET(0x35);
    CHECK_KEY_ASCII_SET(0x36);
    CHECK_KEY_ASCII_SET(0x37);
    CHECK_KEY_ASCII_SET(0x38);
    CHECK_KEY_ASCII_SET(0x39);

    CHECK_KEY_ASCII(0x41);
    CHECK_KEY_ASCII(0x42);
    CHECK_KEY_ASCII(0x43);
    CHECK_KEY_ASCII(0x44);
    CHECK_KEY_ASCII(0x45);
    CHECK_KEY_ASCII(0x46);
    CHECK_KEY_ASCII(0x47);
    CHECK_KEY_ASCII(0x48);
    CHECK_KEY_ASCII(0x49);
    CHECK_KEY_ASCII(0x4a);
    CHECK_KEY_ASCII(0x4b);
    CHECK_KEY_ASCII(0x4c);
    CHECK_KEY_ASCII(0x4d);
    CHECK_KEY_ASCII(0x4e);
    CHECK_KEY_ASCII(0x4f);
    CHECK_KEY_ASCII(0x50);
    CHECK_KEY_ASCII(0x51);
    CHECK_KEY_ASCII(0x52);
    CHECK_KEY_ASCII(0x53);
    CHECK_KEY_ASCII(0x54);
    CHECK_KEY_ASCII(0x55);
    CHECK_KEY_ASCII(0x56);
    CHECK_KEY_ASCII(0x57);
    CHECK_KEY_ASCII(0x58);
    CHECK_KEY_ASCII(0x59);
    CHECK_KEY_ASCII(0x5a);

    CHECK_KEY_SET(0x5b, Win_o, 0x5b);
    CHECK_KEY_SET(0x5c, Win_o, 0x5b);

    CHECK_KEY_ASCII_SET(0x60);
    CHECK_KEY_ASCII_SET(0x61);
    CHECK_KEY_ASCII_SET(0x62);
    CHECK_KEY_ASCII_SET(0x63);
    CHECK_KEY_ASCII_SET(0x64);
    CHECK_KEY_ASCII_SET(0x65);
    CHECK_KEY_ASCII_SET(0x66);
    CHECK_KEY_ASCII_SET(0x67);
    CHECK_KEY_ASCII_SET(0x68);
    CHECK_KEY_ASCII_SET(0x69);

    CHECK_KEY_ASCII(0x6a);
    CHECK_KEY_ASCII(0x6b);
    CHECK_KEY_ASCII(0x6d);
    CHECK_KEY_ASCII(0x6e);
    CHECK_KEY_ASCII(0x6f);

    CHECK_KEY(0x70, F1_o);
    CHECK_KEY(0x71, F2_o);
    CHECK_KEY(0x72, F3_o);
    CHECK_KEY(0x73, F4_o);
    CHECK_KEY(0x74, F5_o);
    CHECK_KEY(0x75, F6_o);
    CHECK_KEY(0x76, F7_o);
    CHECK_KEY(0x77, F8_o);
    CHECK_KEY(0x78, F9_o);
    CHECK_KEY(0x79, F10_o);
    CHECK_KEY(0x7a, F11_o);
    CHECK_KEY(0x7b, F12_o);
    CHECK_KEY(0x7c, F13_o);
    CHECK_KEY(0x7d, F14_o);
    CHECK_KEY(0x7e, F15_o);
    CHECK_KEY(0x7f, F16_o);
    CHECK_KEY(0x80, F17_o);
    CHECK_KEY(0x81, F18_o);
    CHECK_KEY(0x82, F19_o);
    CHECK_KEY(0x83, F20_o);
    CHECK_KEY(0x84, F21_o);
    CHECK_KEY(0x85, F22_o);
    CHECK_KEY(0x86, F23_o);
    CHECK_KEY(0x87, F24_o);
    CHECK_KEY(0x90, NumLock_o);
    CHECK_KEY(0x90, SclLock_o);
    CHECK_KEY(0xa4, LAlt_o);
    CHECK_KEY(0xa5, RAlt_o);

    CHECK_KEY_ASCII(0xba);
    CHECK_KEY_ASCII(0xbb);
    CHECK_KEY_ASCII(0xbc);
    CHECK_KEY_ASCII(0xbd);
    CHECK_KEY_ASCII(0xbe);
    CHECK_KEY_ASCII(0xbf);
    CHECK_KEY_ASCII(0xc0);
    CHECK_KEY_ASCII(0xdb);
    CHECK_KEY_ASCII(0xdc);
    CHECK_KEY_ASCII(0xdd);
    CHECK_KEY_ASCII(0xde);
    CHECK_KEY_ASCII(0xe2);

    Py_SET_SIZE(list, idx);
#else
    list = PyList_New(0);
#endif
    return list;
}

static PyMethodDef keysmethods[] = {
    {"get_keys", (PyCFunction)(void(*)(void))keys_get_keys, METH_NOARGS, "keys!"},
    {NULL}
};

static int
keys_traverse(PyObject *module, visitproc visit, void *arg)
{
    Py_VISIT(Shift_o);
    Py_VISIT(Ctrl_o);
    Py_VISIT(LAlt_o);
    Py_VISIT(Back_o);
    Py_VISIT(Tab_o);
    Py_VISIT(Enter_o);
    Py_VISIT(Esc_o);
    Py_VISIT(Caps_o);
    Py_VISIT(PgUp_o);
    Py_VISIT(PgDn_o);
    Py_VISIT(End_o);
    Py_VISIT(Home_o);
    Py_VISIT(Left_o);
    Py_VISIT(Up_o);
    Py_VISIT(Right_o);
    Py_VISIT(Down_o);
    Py_VISIT(PrtSc_o);
    Py_VISIT(Insert_o);
    Py_VISIT(Del_o);
    Py_VISIT(Win_o);
    Py_VISIT(F1_o);
    Py_VISIT(F2_o);
    Py_VISIT(F3_o);
    Py_VISIT(F4_o);
    Py_VISIT(F5_o);
    Py_VISIT(F6_o);
    Py_VISIT(F7_o);
    Py_VISIT(F8_o);
    Py_VISIT(F9_o);
    Py_VISIT(F10_o);
    Py_VISIT(F11_o);
    Py_VISIT(F12_o);
    Py_VISIT(F13_o);
    Py_VISIT(F14_o);
    Py_VISIT(F15_o);
    Py_VISIT(F16_o);
    Py_VISIT(F17_o);
    Py_VISIT(F18_o);
    Py_VISIT(F19_o);
    Py_VISIT(F20_o);
    Py_VISIT(F21_o);
    Py_VISIT(F22_o);
    Py_VISIT(F23_o);
    Py_VISIT(F24_o);
    Py_VISIT(NumLock_o);
    Py_VISIT(SclLock_o);
    Py_VISIT(RAlt_o);
    return 0;
}

static int
keys_clear(PyObject *module)
{
    Py_CLEAR(Shift_o);
    Py_CLEAR(Ctrl_o);
    Py_CLEAR(LAlt_o);
    Py_CLEAR(Back_o);
    Py_CLEAR(Tab_o);
    Py_CLEAR(Enter_o);
    Py_CLEAR(Esc_o);
    Py_CLEAR(Caps_o);
    Py_CLEAR(PgUp_o);
    Py_CLEAR(PgDn_o);
    Py_CLEAR(End_o);
    Py_CLEAR(Home_o);
    Py_CLEAR(Left_o);
    Py_CLEAR(Up_o);
    Py_CLEAR(Right_o);
    Py_CLEAR(Down_o);
    Py_CLEAR(PrtSc_o);
    Py_CLEAR(Insert_o);
    Py_CLEAR(Del_o);
    Py_CLEAR(Win_o);
    Py_CLEAR(F1_o);
    Py_CLEAR(F2_o);
    Py_CLEAR(F3_o);
    Py_CLEAR(F4_o);
    Py_CLEAR(F5_o);
    Py_CLEAR(F6_o);
    Py_CLEAR(F7_o);
    Py_CLEAR(F8_o);
    Py_CLEAR(F9_o);
    Py_CLEAR(F10_o);
    Py_CLEAR(F11_o);
    Py_CLEAR(F12_o);
    Py_CLEAR(F13_o);
    Py_CLEAR(F14_o);
    Py_CLEAR(F15_o);
    Py_CLEAR(F16_o);
    Py_CLEAR(F17_o);
    Py_CLEAR(F18_o);
    Py_CLEAR(F19_o);
    Py_CLEAR(F20_o);
    Py_CLEAR(F21_o);
    Py_CLEAR(F22_o);
    Py_CLEAR(F23_o);
    Py_CLEAR(F24_o);
    Py_CLEAR(NumLock_o);
    Py_CLEAR(SclLock_o);
    Py_CLEAR(RAlt_o);
    return 0;
}

static void
keys_free(void *module)
{
    (void)keys_clear((PyObject *)module);
}

static PyModuleDef keysmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "keys",
    .m_doc = "keys!",
    .m_size = -1,
    .m_methods = keysmethods,
    .m_traverse = keys_traverse,
    .m_clear = keys_clear,
    .m_free = keys_free,
};

#define NEW_STRING(name) \
    name ## _o = PyUnicode_DecodeUTF8Stateful(#name, sizeof(#name) - 1, NULL, NULL); \
    if (UNLIKELY(name ## _o == NULL)) { \
        return NULL; \
    } \

PyMODINIT_FUNC
PyInit_keys(void)
{
    NEW_STRING(Shift);
    NEW_STRING(Ctrl);
    NEW_STRING(LAlt);
    NEW_STRING(Back);
    NEW_STRING(Tab);
    NEW_STRING(Enter);
    NEW_STRING(Esc);
    NEW_STRING(Caps);
    NEW_STRING(PgUp);
    NEW_STRING(PgDn);
    NEW_STRING(End);
    NEW_STRING(Home);
    NEW_STRING(Left);
    NEW_STRING(Up);
    NEW_STRING(Right);
    NEW_STRING(Down);
    NEW_STRING(PrtSc);
    NEW_STRING(Insert);
    NEW_STRING(Del);
    NEW_STRING(Win);
    NEW_STRING(F1);
    NEW_STRING(F2);
    NEW_STRING(F3);
    NEW_STRING(F4);
    NEW_STRING(F5);
    NEW_STRING(F6);
    NEW_STRING(F7);
    NEW_STRING(F8);
    NEW_STRING(F9);
    NEW_STRING(F10);
    NEW_STRING(F11);
    NEW_STRING(F12);
    NEW_STRING(F13);
    NEW_STRING(F14);
    NEW_STRING(F15);
    NEW_STRING(F16);
    NEW_STRING(F17);
    NEW_STRING(F18);
    NEW_STRING(F19);
    NEW_STRING(F20);
    NEW_STRING(F21);
    NEW_STRING(F22);
    NEW_STRING(F23);
    NEW_STRING(F24);
    NEW_STRING(NumLock);
    NEW_STRING(SclLock);
    NEW_STRING(RAlt);
    return PyModule_Create(&keysmodule);
}
