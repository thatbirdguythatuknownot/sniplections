#define PY_SSIZE_T_CLEAN

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "Python.h"

/*
** Lua table interface as a Python C extension.
*/

typedef int8_t small_t;

#define CAST(TYPE, O)  ((TYPE)(O))
#define RCAST(O, TYPE)  ((TYPE)(O))

#define TRUE  -1
#define FALSE  0

#define CARRLENGTH(A)  (sizeof((A)) / sizeof((A)[0]))

#define FIELDOF(S, F) ((S){}).F

static inline small_t
in_bounds(const Py_ssize_t i, const Py_ssize_t size)
{
    return (size_t)i < (size_t)size;
}

#define TWOTO(N) (1ULL << (N))
#define TWOTO_MASK(N) (TWOTO((N)) - 1)

/* Funnily enough, this corresponds to a ceil(log2(b + 1)) operation. */
static inline uint8_t
byte_msb(uint8_t b)
{
#if (defined(__clang__) || defined(__GNUC__))
    return b ? 32 - CAST(uint8_t, __builtin_clz((unsigned int)b)) : 0;
#elif defined(_MSC_VER)
    unsigned long msb = 0;
    if (_BitScanReverse(&msb, exp)) {
        return CAST(uint8_t, msb) + 1;
    }
    return 0;
#else
    static const uint8_t _msb_set_table[256] = {
        0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };
    return _msb_set_table[b];
#endif
}

/* From Lua's /lobject.c */
static inline uint8_t
clg2(size_t x)
{
    uint8_t l = 0;
    x--;
    while (x >= 256) { l += 8; x >>= 8; }
    return l + byte_msb((uint8_t)x);
}


typedef Py_ssize_t _i2pow_res[2];

/*
** (Based from https://gist.github.com/orlp/3551590)
** !!!! This function can overflow. Only use when it's guaranteed not to.
**
** Calculates the integer pow of 2 bases with 1 exponent.
** Assigns 0 to result on overflow/underflow.
*/
static inline void
i2pow(Py_ssize_t b0, Py_ssize_t b1, uint8_t exp, _i2pow_res res)
{
    assert(res != NULL);
    Py_ssize_t r0 = 1, r1 = 1;

#define COMMON \
    if (exp & 1) { \
        r0 *= b0; \
        r1 *= b1; \
    } \
    exp >>= 1; \
    b0 *= b0; \
    b1 *= b1; \

    switch (byte_msb(exp)) {
    case 8:
    case 7:
        /* Handle base == 1/-1 cases and overflow/underflow. */
        res[0] = b0 == 1 ? 1
                 : b0 == -1 ? 1 - 2*(exp & 1)
                 : 0;
        res[1] = b1 == 1 ? 1
                 : b1 == -1 ? 1 - 2*(exp & 1)
                 : 0;
        break;
    case 6: COMMON
    case 5: COMMON
    case 4: COMMON
    case 3: COMMON
    case 2:
        if (exp & 1) {
            r0 *= b0;
            r1 *= b1;
        }
        b0 *= b0;
        b1 *= b1;
    case 1:
        r0 *= b0;
        r1 *= b1;
    default:
        res[0] = r0;
        res[1] = r1;
    }

#undef COMMON
}

/*
** (x + 1)/x = 1 + 1/x
** Consecutive natural number ratio.
*/
static inline long double
cnnr(uint64_t x)
{
    return 1 + 1/(long double)x;
}


typedef uint8_t flag_t;

#define VFLAG(FL)  (1U << (FL))
#define FLAG_MASK_BELOW(FL)  ((flag_t)(~(~0U << (FL))))

typedef enum {
    TM_INDEX,
    TM_NEWINDEX,
    TM_GC,
    TM_MODE_V,  /* /| Split into two for convenience reasons. */
    TM_MODE_K,  /* \|                                         */
    TM_LEN,
    TM_EQ,      /* last tag method with fast access */
    TM_NOFAST,
    TM_ADD = TM_NOFAST,
    TM_SUB,
    TM_MUL,
    TM_MOD,
    TM_POW,
    TM_DIV,
    TM_IDIV,
    TM_BAND,
    TM_BOR,
    TM_BXOR,
    TM_SHL,
    TM_SHR,
    TM_UNM,
    TM_BNOT,
    TM_LT,
    TM_LE,
    TM_CONCAT,
    TM_CALL,
    TM_CLOSE,
    TM_N,       /* number of elements in the enum */
} LTMS;

/* Mask for all the fast tagmethods. */
#define LTM_FASTMASK  FLAG_MASK_BELOW(TM_NOFAST)

typedef enum {
    FL_HASDUMMY = TM_EQ + 1,
} LFLAGS;

#define NODUMMY_MASK  FLAG_MASK_BELOW(FL_HASDUMMY)

/*
** A node can either be a normal object-key node or a special type node of
** two types: int and float. The type is determined by the 'spc_type' field,
** with the truthiness of the field determining whether a node is special or
** not. 'spc_type' can be checked using the 'TSPCKS' enum below.
**
** Normal object-key nodes have spc_type = SPCK_NONE (falsy 'spc_type'),
** with the key object in the 'key' field.
**
** Int-key nodes, used for ints whose absolute value fits in a Py_ssize_t,
** have spc_type = SPCK_INT, with the value of the key stored at 'key_i'.
**
** Float-key nodes, used for Python floats, have spc_type = SPCK_FLT, with
** the value of the key stored at 'key_f'.
**
** This slightly mimics Lua's type system and is used to make access of
** common small ints and all Python floats a bit faster.
*/
typedef enum {
    SPCK_NONE = 0,
    SPCK_INT = VFLAG(0),
    SPCK_FLT = VFLAG(1),
} TSPCKS;

typedef struct {
    PyObject *val;
    union {
        PyObject *key;
        Py_ssize_t key_i;
        double key_f;
    };
    TSPCKS spc_type : 2;
    Py_ssize_t next : sizeof(Py_ssize_t)*8 - 2;  /* for chaining */
} LNode;

#define IS_INTKEY(n) ((CAST(LNode *, n)->spc_type & SPCK_INT) != 0)
#define IS_FLTKEY(n) ((CAST(LNode *, n)->spc_type & SPCK_FLT) != 0)

static LNode _dummynode = {NULL, NULL, 0, 0};

#define DUMMYNODE (&_dummynode)

static inline small_t
is_nil(const PyObject *o)
{
    return (o == NULL) || Py_IsNone(o);
}
#define ISNIL(O)  (is_nil((O)))

static inline small_t
is_objnode(const LNode *n)
{
    assert(n != NULL);
    return n->spc_type == SPCK_NONE;
}
#define IS_OBJNODE(N)  (is_objnode((N)))

static inline small_t
node_empty(const LNode *n)
{
    assert(n != NULL);
    return IS_OBJNODE(n) && ISNIL(n->key) || ISNIL(n->val);
}
#define EMPTY(N)  (node_empty((N)))

static inline small_t
node_absent(const LNode *n)
{
    return (n == DUMMYNODE) || EMPTY(n);
}
#define ABSENT(N)  (node_absent((N)))

static inline LNode *
node_next(LNode *n)
{
    assert(n != NULL);
    return n + n->next;
}
#define NEXT(N)  (node_next((N)))

typedef struct _LTable {
    PyObject_HEAD;
    flag_t flags;           /* if p <= TM_EQ and 1<<p is set,
                            ** then LTMS[p] is not present
                            */
    uint8_t lg2n_node;      /* log2 of number of slots of 'node' array */
    Py_ssize_t asize;       /* size of array part */
    PyObject **array;       /* dynamic array of objects;  */
    LNode *node;
    struct _LTable *meta;
    PyTypeObject *_astype;  /* singleton of table as heap type
                            ** (for metatable purposes)
                            */
} LTable;

static PyTypeObject *LTable_Type;

#define GET_FLAG(T, FL)  (CAST(LTable *, (T))->flags & VFLAG((FL)))
#define HAS_FLAG(T, FL)  (GET_FLAG((T), (FL)))
#define SET_FLAG(T, FL)  (CAST(LTable *, (T))->flags |= VFLAG((FL)))
#define DEL_FLAG(T, FL)  (CAST(LTable *, (T))->flags &= ~VFLAG((FL)))

#define HASDUMMY(T)  HAS_FLAG((T), FL_HASDUMMY)

/* Max sizes must fit inside a positive Py_ssize_t int. */

#define MAX_ABITS  (sizeof(Py_ssize_t)*8 - 1)
#define MAX_ASIZE \
    (CAST(Py_ssize_t, TWOTO_MASK(MAX_ABITS)) / (sizeof(PyObject *) + 1))
#define VALID_AIDX(I)  (in_bounds((I), MAX_ASIZE))
#define ELEMAT(T, I)  (CAST(LTable *, (T))->array[(I)])
#define INBOUNDS(T, I) (in_bounds((I), CAST(LTable *, (T))->asize))

#define MAX_HBITS  (MAX_ABITS - 1)
#define MAX_HSIZE  (CAST(Py_ssize_t, TWOTO_MASK(MAX_HBITS)) / sizeof(LNode))
#define HSIZE(T)  (1LL << CAST(LTable *, (T))->lg2n_node)
#define HMASK(T)  (HSIZE((T)) - 1)
#define NODEAT(T, I)  (&CAST(LTable *, (T))->node[(I)])

static inline LNode *
node_in(const LTable *t, const Py_ssize_t i)
{
    return NODEAT(t, i & HMASK(t));
}
#define NODEIN(T, I)  (node_in((T), (I)))

#define NODEIDX(T, N) (CAST(LNode *, N) - &CAST(LTable *, (T))->node)

/*
** Lua last-free system to optimize finding a free slot in large tables.
** 2^LFREE_LG2LIM (= 8) is the minimum size needed before the last-free
** system is utilized.
**
** If the last-free system is utilized, a pointer to a last-free slot
** in the 'node' array precedes the array.
*/
#define LFREE_LG2LIM  3

#define LFREEABLE(SIZE)  ((SIZE) >= LFREE_LG2LIM)

/*
** Ensure that the union 'LFreeBox' is properly aligned with the
** nodes following it in the 'node' array.
*/
struct _lf_aux { LNode *dummy; LNode nextnode; };

typedef union {
    LNode *lastfree;
    char _pad[offsetof(struct _lf_aux, nextnode)];
} LFreeBox;

#define LFBOX_PTR(T)  (CAST(LFreeBox *, RCAST((T), LTable *)->node) - 1)

#define HAS_LFREE(T)  LFREEABLE(CAST(LTable *, (T))->lg2n_node)
#define LASTFREE(T)  (LFBOX_PTR((T))->lastfree)

static inline void *
hashpart_baseptr(const LTable *t)
{
    return HAS_LFREE(t) ? (void *)LFBOX_PTR(t) : (void *)t->node;
}
#define HASHPART_PTR(T) (hashpart_baseptr((T)))

/*
** Length hint system. Placed before the 'array' pointer. 0-based indexing.
*/
#define LENHINT_PTR(T)  (CAST(Py_ssize_t *, RCAST((T), LTable *)->array) - 1)
#define LENHINT(T)  (*LENHINT_PTR((T)))

static inline Py_ssize_t
new_hint(LTable *t, const Py_ssize_t hint)
{
    assert(hint < t->asize);
    LENHINT(t) = hint;
    return hint + 1;
}
#define NEWHINT(T, N)  (new_hint((T), (N)))


static inline Py_hash_t
ssize_hash(const Py_ssize_t x)
{
    /* Emulate hash(x) as if it was done in Python. */
    Py_hash_t y = (Py_hash_t)(x >= 0 ? x : -x);
    if (y >= PyHASH_MODULUS) {
        y = (y & PyHASH_MODULUS) + (y >> PyHASH_BITS);
        if (y >= PyHASH_MODULUS) {
            y -= PyHASH_MODULUS;
        }
    }
    if (x >= 0) {
        return y;
    }
    return y == 1 ? -2 : -y;
}

static inline LNode *
ssize_main_pos(const LTable *t, const Py_ssize_t key)
{
    return NODEIN(t, ssize_hash(key));
}

static LNode *
get_ssizepos(const LTable *t, const Py_ssize_t key)
{
    LNode *n = ssize_main_pos(t, key);
    for (;;) {
        if (IS_INTKEY(n) && n->key_i == key) {
            return n;
        }
        if (n->next == 0) {
            break;
        }
        n = NEXT(n);
    }
    return DUMMYNODE;
}

/*
** Try to convert a long object into a Py_ssize_t.
** Returns 1 for success and 0 for fail.
*/
static inline small_t
try_int2ssize(PyObject *key, Py_ssize_t *resptr)
{
    assert(PyLong_CheckExact(key));
    Py_ssize_t res = PyLong_AsSsize_t(key);
    if (res == -1 && PyErr_Occurred()) {
        PyErr_Clear();
         return 0;
    }
    *resptr = res;
    return 1;
}

static Py_hash_t
dbl_hash(double key)
{
    /* DANGER!! This is relying on the fact that _Py_HashDouble()
     * only uses the instance for a generic pointer hash function.
     * It's fine now, but it might change later. */
    return _Py_HashDouble((PyObject *)&key, key);
}

static LNode *
dbl_main_pos(const LTable *t, const double key)
{
    return NODEIN(t, dbl_hash(key));
}

static hashfunc long_hash = NULL;
static hashfunc float_hash = NULL;

static inline Py_hash_t
generic_hash(PyObject *key)
{
    Py_hash_t hash = PyObject_Hash(key);
    if (hash == -1) {
        return Py_HashPointer(key);
    }
    return hash;
}

static LNode *
generic_main_pos(const LTable *t, PyObject *key)
{
    return NODEIN(t, generic_hash(key));
}

static LNode *
node_main_pos(const LTable *t, const LNode *n)
{
    if (IS_INTKEY(n)) {
        return ssize_main_pos(t, n->key_i);
    }
    if (IS_FLTKEY(n)) {
        return dbl_main_pos(t, n->key_f);
    }
    return NODEIN(t, generic_hash(n->key));
}


static LNode *
get_free_pos(LTable *t)
{
    if (HAS_LFREE(t)) {
        /* 'lastfree' exists; look for a spot
         * preceding it while also updating it. */
        while (LASTFREE(t) > t->node) {
            LNode *free = --LASTFREE(t);
            if (EMPTY(free)) {
                return free;
            }
        }
    }
    else {
        /* No 'lastfree' info; do a linear search. */
        Py_ssize_t i = HSIZE(t);
        while (i--) {
            LNode *free = NODEAT(t, i);
            if (EMPTY(free)) {
                return free;
            }
        }
    }
    /* Couldn't find a free place. */
    return NULL;
}

/*
** Prepares a hash's position in the hash table, using Brent's variation of
** scattering (the one Lua uses).
** 1. If the main position is free, return it.
** 2. If the colliding node at the main position is not in its own
**    main position, move it to another new position and return the
**    original position.
** 3. The colliding node has the same main position, so return a free
**    empty position.
** Returns 1 if it successfully prepared a free position or 0 if there was no
** space available.
*/
static small_t
prepare_pos(LTable *t, LNode **mpptr)
{
    assert(mpptr != NULL);
    LNode *mp = *mpptr;
    if (!ABSENT(mp) || HASDUMMY(t)) {
        /* Main position is taken. */
        LNode *fpos = get_free_pos(t);
        if (fpos == NULL) {
            /* No free position found, exit. */
            return 0;
        }
        /* Dummy node table path shouldn't have had a free position. */
        assert(!HASDUMMY(t));

        LNode *coll_mp = node_main_pos(t, mp);
        if (coll_mp != mp) {
            /* Colliding node is out of its main position; move it into
             * the free position. */
            while (NEXT(coll_mp) != mp) {  /* Find the colliding node's
                                            * previous link */
                coll_mp = NEXT(coll_mp);
            }

            /* Rechain the previous link to point to the colliding node's
             * new position, then copy the colliding node's data into the free
             * position. */
            coll_mp->next = (Py_ssize_t)(fpos - coll_mp);
            *fpos = *mp;

            if (mp->next != 0) {
                /* Correct the new position's 'next' field and clear the
                 * old position's 'next' data. */
                fpos->next += (Py_ssize_t)(mp - fpos);
                mp->next = 0;
            }

            /* Completely clear the data of the colliding node's
             * old position. */
            Py_CLEAR(mp->val);
        }
        else {
            /* Colliding node is in its own main position; new key
             * will go into the free position. */
            if (mp->next != 0) {
                /* Move the free position into the chain. */
                fpos->next = (Py_ssize_t)(NEXT(mp) - fpos);
            }
            else {
                /* The free position should be completely clear. */
                assert(fpos->next == 0);
            }

            /* Make the main position's 'next' field point to the free
             * position. */
            mp->next = (Py_ssize_t)(fpos - mp);
            *mpptr = mp = fpos;
        }
    }

    /* The position to insert to should be completely free now. */
    assert(ISNIL(mp->val));
    return 1;
}

/*
** Inserts an object key and value pair into the hash part.
** Steals the references to 'key' and 'val'.
** Returns 1 for success and 0 for fail.
*/
static small_t
insert_key(LTable *t, PyObject *key, PyObject *val)
{
    LNode *mp = generic_main_pos(t, key);
    if (!prepare_pos(t, &mp)) {
        return 0;
    }
    mp->spc_type = SPCK_NONE;
    mp->key = key;
    mp->val = val;
    return 1;
}

/*
** Inserts an object key and a value pair into the hash part, given the key's
** hash.
** Steals the references to 'key' and 'val'.
** Returns 1 for success and 0 for fail.
*/
static small_t
insert_key_hash(LTable *t, PyObject *key,
                const Py_hash_t hash, PyObject *val)
{
    LNode *mp = NODEIN(t, hash);
    if (!prepare_pos(t, &mp)) {
        return 0;
    }
    mp->spc_type = SPCK_NONE;
    mp->key = key;
    mp->val = val;
    return 1;
}

/*
** Inserts an int (Py_ssize_t) key and object value pair into the hash part.
** Steals the reference to 'val'.
** Returns 1 for success and 0 for fail.
*/
static small_t
insert_int_key(LTable *t, Py_ssize_t key, PyObject *val)
{
    LNode *mp = ssize_main_pos(t, key);
    if (!prepare_pos(t, &mp)) {
        return 0;
    }
    mp->spc_type = SPCK_INT;
    mp->key_i = key;
    mp->val = val;
    return 1;
}

/*
** Inserts a float (double) key and object value pair into the hash part.
** Steals the reference to 'val'.
** Returns 1 for success and 0 for fail.
*/
static small_t
insert_flt_key(LTable *t, double key, PyObject *val)
{
    LNode *mp = dbl_main_pos(t, key);
    if (!prepare_pos(t, &mp)) {
        return 0;
    }
    mp->spc_type = SPCK_FLT;
    mp->key_f = key;
    mp->val = val;
    return 1;
}

/*
** Inserts a new int key into a table 't', considering if it belongs in
** the array part of the table.
** Steals the reference to 'val'.
** Returns 1 for success and 0 for fail.
*/
static small_t
new_int_key(LTable *t, Py_ssize_t key, PyObject *val)
{
    if (INBOUNDS(t, key)) {
        /* Key is in the array part; set the value there. */
        ELEMAT(t, key) = val;
        return 1;
    }
    /* Key doesn't belong in the array; put it in the hash part. */
    return insert_int_key(t, key, val);
}

/*
** Specialize an object 'key' into one of the three types
** (object/non-specialized, int, and float) of a node.
**
** Steals the reference to 'key', and decrefs it if it is successfully
** converted into a specialized non-object key.
** The __index__() method of 'key' may be used, if it exists, and a new
** object is made; if so, it replaces 'key' as the used node key and decrefs
** it.
** The resulting info is stored in 'n'. An optional pointer 'hptr' used to
** store the object hash if the key is a non-specialized key.
*/
static void
specialize_obj(PyObject *key, LNode *n, Py_hash_t *hptr)
{
    assert(n != NULL);

    /* If the 'key' is not an int and has an __index__() method, use that. */
    if (!PyLong_CheckExact(key)) {
        PyObject *temp = PyNumber_Index(key);
        if (temp == NULL) {
            PyErr_Clear();
        }
        else {
            Py_SETREF(key, temp);
        }
    }

    if (PyLong_CheckExact(key)) {
        Py_ssize_t ival = -1;
        if (!try_int2ssize(key, &ival)) {
            /* Key doesn't fit in a Py_ssize_t; treat it as an object. */
            if (hptr) {
                *hptr = long_hash(key);
            }
            goto set_obj;
        }
        Py_DECREF(key);
        n->spc_type = SPCK_INT;
        n->key_i = ival;
        return;
    }

    if (PyFloat_CheckExact(key)) {
        double keyf = PyFloat_AS_DOUBLE(key);
        if (hptr) {
            *hptr = float_hash(key);
        }
        Py_DECREF(key);

        /* Check if 'keyf' is a valid special integer key. */
        if (isfinite(keyf) &&
            keyf >= (double)PY_SSIZE_T_MIN &&
            keyf <= (double)PY_SSIZE_T_MAX &&
            floor(keyf) == keyf
        ) {
            /* If it is, treat it like one. */
            n->spc_type = SPCK_INT;
            n->key_i = (Py_ssize_t)keyf;
        }
        else {
            /* If not, do the normal approach. */
            n->spc_type = SPCK_FLT;
            n->key_f = keyf;
        }
        return;
    }

    /* Non-specialized path. */
    if (hptr != NULL) {
        *hptr = generic_hash(key);
    }

set_obj:;
    n->spc_type = SPCK_NONE;
    n->key = key;
}

/*
** Inserts a node into a table with space.
** Steals the reference to 'val'. Uses the information from node 'n'
** to insert an entry into either the array or hash part of the table.
** Parameter 'hash' is only used for inserting a non-specialized
** object key into the hash part of the table. If it is needed but its
** value is -1, then the hash is calculated generically.
** Returns 1 for success and 0 for fail.
*/
static small_t
new_node_key_hash(LTable *t, LNode *n, PyObject *val, Py_hash_t hash)
{
    if (IS_INTKEY(n)) {
        return new_int_key(t, n->key_i, val);
    }

    if (IS_FLTKEY(n)) {
        return insert_flt_key(t, n->key_f, val);
    }

    assert(n->key != NULL);
    if (hash == -1) {
        hash = generic_hash(n->key);
    }
    assert(hash != -1);
    return insert_key_hash(t, n->key, hash, val);
}

/* Like new_node_key_hash(), but without the 'hash' parameter. */
static small_t
new_node_key(LTable *t, LNode *n, PyObject *val)
{
    return new_node_key_hash(t, n, val, -1);
}

/*
** Inserts a key into a table with space for the key.
** Steals the references to 'key' and 'val' and decrefs 'key' if the
** function succeeds with inserting a special-key node that doesn't
** need the key object.
** Returns 1 for success and 0 for fail.
*/
static small_t
new_key(LTable *t, PyObject *key, PyObject *val, small_t *is_int)
{
    Py_hash_t hash = -1;
    LNode info = {};
    specialize_obj(key, &info, &hash);
    if (is_int) {
        *is_int = IS_INTKEY(&info);
    }
    return new_node_key_hash(t, &info, val, hash);
}


/*
** Implement the same strategy Lua uses for getting the table length
** by finding the "border": an index 'i' where t[i] is present but t[i + 1]
** is absent, or 'asize - 1' if t[asize - 1] (the last element) is present.
** The true length is always 'border + 1'.
*/

/*
** (copied almost word-for-word from the official file at '/ltable.c')
** Try to find a boundary in the hash part of table 't'. From the caller, we
** know that 'j' is present. We want to find a larger key that is absent from
** the table, so that we can do a binary search between the two keys to find
** a boundary. We keep doubling 'j' until we get an absent index. If the
** doubling would overflow, we try PY_SSIZE_T_MAX - 1. If it is absent, we
** are ready for the binary search ('j', being max integer, is larger or
** equal to 'i', but it cannot be equal because it is absent while 'i' is
** present; so 'j > i'). Otherwise, 'j' is the boundary ('j + 1' cannot be a
** present int key because no Python collection should ever have that much
** elements. Maybe weird ones.)
*/
static Py_ssize_t
hash_search_bor(const LTable *t, Py_ssize_t j)
{
    Py_ssize_t i;
    do {
        i = j;  /* 'i' is a present index */
        if (j <= PY_SSIZE_T_MAX >> 1) {
            j <<= 1;
        }
        else {
            if (ABSENT(get_ssizepos(t, PY_SSIZE_T_MAX - 1))) {
                /* t[MAX - 1] is absent; break out of the loop. */
                j = PY_SSIZE_T_MAX - 1;
                break;
            }
            /* Weird... t[MAX - 1] is present? Tough system, maybe. */
            return PY_SSIZE_T_MAX;
        }
    } while (!ABSENT(get_ssizepos(t, j)));  /* repeat until an absent t[j] */

    /* 'i < j', t[i] is present, and t[j] is absent. Do a binary search. */
    while (j - i > 1) {
        Py_ssize_t m = (i + j) >> 1;
        if (ABSENT(get_ssizepos(t, m))) {
            j = m;
        }
        else {
            i = m;
        }
    }
    /* Return the true length. */
    return j;
}

static Py_ssize_t
bin_search_bor(const LTable *t, Py_ssize_t i, Py_ssize_t j)
{
    assert(i <= j);
    while (j - i > 1) {
        Py_ssize_t m = (i + j) >> 1;
        if (ISNIL(ELEMAT(t, m))) {
            j = m;
        }
        else {
            i = m;
        }
    }
    return i;
}

#define _MAXVICINITY  4

static int
LTable_len(LTable *t)
{
    Py_ssize_t asize = t->asize;
    if (asize > 0) {
        /* Array part is present. Start with limit: */
        Py_ssize_t limit = LENHINT(t);
        if (ISNIL(ELEMAT(t, limit))) {
            /* If t[limit] is absent, then there's a border before it;
             * first look for one in the vicinity of the hint. */
            for (Py_ssize_t i = 0; i < _MAXVICINITY && limit > 1; i++) {
                limit--;
                if (ELEMAT(t, limit) != NULL) {
                    /* Border found at 'limit'. */
                    return NEWHINT(t, limit);
                }
            }
            /* Nothing in the vicinity; search for a border in [0, limit). */
            return NEWHINT(t, bin_search_bor(t, 0, limit));
        }

        /* t[limit] is present, so the border is after it;
         * like the former case, look for one in the vicinity first. */
        for (Py_ssize_t i = 0;
             i < _MAXVICINITY && limit < asize;
             i++)
        {
            limit++;
            if (ISNIL(ELEMAT(t, limit))) {
                /* Border found at 'limit - 1'. */
                return NEWHINT(t, limit - 1);
            }
        }

        if (ISNIL(ELEMAT(t, asize - 1))) {
            /* If the last element is absent, then that must mean
             * the border is between 'limit' and the last element;
             * search for a border in [limit, asize). */
            return NEWHINT(t, bin_search_bor(t, limit, asize - 1));
        }

        /* The last element is present;
         * set a hint to speed up later iterations. */
        LENHINT(t) = asize - 1;
    }

    /* Either no array exists or the last element is present;
     * check the hash part. */
    assert(asize == 0 || !ISNIL(ELEMAT(t, asize - 1)));
    if (HASDUMMY(t) || ABSENT(get_ssizepos(t, asize))) {
        /* t[asize] is absent. */
        return asize;
    }
    /* t[asize] is also present. */
    return hash_search_bor(t, asize);
}

#undef _MAXVICINITY


/*
** !!!! Update 'MINI_COEFF' to always approximately equal the point where
** !!!! (1 + 1/MINI_FACTOR) ^ MINI_COEFF exceeds a doubling (x2) multiplier!
** !!!! Ex. (1 + 1/50) ^ 35 ~~ 1.9999
   !!!!     (1 + 1/50) ^ 36 ~~ 2.0399
   !!!!     So when MINI_FACTOR = 50, MINI_COEFF = 36.
**
** Used for the size of the slices to iterate over when computing the
** possible size of a table's array part during rehashing and saving some
** memory in large cases of resizing.
*/
#define MINI_FACTOR  10
#define MINI_COEFF  8
#define NUMS_SIZE  (MAX_ABITS - 1) * MINI_COEFF

/*
** For context:
** A slice is every index between powers of 2. They are the main checkpoints
** when computing size.
** A mini-slice is every index between the current iterative size and the
** size multiplied by 1 + 1/MINI_FACTOR. There are MINI_COEFF number of
** mini-slices between a slice and another slice.
*/

/*
** Mini-slice step, modifying 'nums_i'.
** Returns the next limit or -1 if it should move to the bigger slice.
*/
static inline Py_ssize_t
mini_step(Py_ssize_t curlim, uint16_t *nums_i)
{
    assert(nums_i != NULL && curlim > 0);
    Py_ssize_t orig = curlim;
    curlim += curlim/MINI_FACTOR;
    if (curlim == orig) {
        /*
        ** Where 'x = lim' and 'r = MINI_FACTOR', solve for 'n' in:
        **     x*(1 + 1/r)**n - x >= 1
        **     x*(1 + 1/r)**n     >= 1 + x
        **       (1 + 1/r)**n     >= (1 + x) / x
        **                           1 + 1/x
        **                      n >= log(1 + 1/x) with base (1 + 1/r)
        **                           log(1 + 1/x) / log(1 + 1/r)
        ** In order to turn 'n' into an integer, a function returning
        ** an integer >= the original number is used: ceil.
        ** So all in all, we have this equation:
        **     n = ceil(log(1 + 1/x) / log(1 + 1/r))
        */
        long double n = ceill(
            logl(cnnr(CAST(size_t, curlim))) / logl(cnnr(MINI_FACTOR))
        );
        uint16_t diff = MINI_COEFF - (*nums_i % MINI_COEFF);
        if (diff <= (uint16_t)n) {
            *nums_i += diff;
            return -1;
        }
        *nums_i += CAST(uint16_t, n);
        return CAST(Py_ssize_t, curlim * powl(cnnr(MINI_FACTOR), n));
    }
    return curlim;
}

/*
** Structure adapted from Lua to count the keys in a table.
** 'total' is the total number of keys in the table.
** 'na' is the number of possible array indices in the table.
** 'deld' is set if there are deleted nodes in the hash part.
** 'nums' is a "count array" to count elements in mini-slices of possible
** array indices. Note that 'na' is the summation of 'nums'.
*/
typedef struct {
    Py_ssize_t total;
    Py_ssize_t na : sizeof(Py_ssize_t)*8 - 1;
    small_t deld : 1;
    Py_ssize_t nums[NUMS_SIZE];
} Counters;

static void
count_int(const Py_ssize_t key, Counters *ct)
{
    if (VALID_AIDX(key)) {
        /* Key is a valid array index; count as such. */
        small_t lg = clg2(key + 1);  /* current power */
        Py_ssize_t pval = 1 << lg;  /* power-of-2 value of 'lg' (= 2^lg) */
        uint16_t i;  /* index of the current slice
                      * (including mini-slices)
                      */
        Py_ssize_t lim;  /* current limit to iterate over */

        if (lg != 0) {
            /* Traverse each mini-slice to find where the key belongs. */
            for (i = 1, lim = pval >> 1 ; ; i++) {
                if (i >= MINI_COEFF || lim >= pval) {
                    /* We've ran out of mini-slices; it's probably
                     * at the last one. */
                    i = MINI_COEFF;
                    break;
                }
                lim = mini_step(lim, &i);
                if (lim == -1) {
                    /* Ran out of mini-slices. */
                    break;
                }
                if (key < lim) {
                    /* We found the slice; break out. */
                    break;
                }
            }
        }
        Py_ssize_t idx = lg ? CAST(Py_ssize_t, lg - 1) * MINI_COEFF + i : 0;
        ct->nums[idx]++;
        ct->na++;
    }
}

/*
** Count keys in the array part of table 't'.
*/
static void
count_arr(const LTable *t, Counters *ct)
{
    small_t lg;  /* current power */
    Py_ssize_t pval;  /* power-of-2 value of 'lg' (= 2^lg) */
    uint16_t nums_i;  /* index of the current slice
                       * (including mini-slices)
                       */
    Py_ssize_t lim;  /* current limit to iterate over */
    Py_ssize_t ause = 0;  /* summation of 'nums' */
    Py_ssize_t i = 0;  /* index to traverse the array */

    /* Traverse each slice. */
    for (lg = lim = nums_i = 0, pval = 1;
         lim < t->asize;
         nums_i++)
    {
        if (nums_i % MINI_COEFF == 0) {
move_slice:;
            /* Move to the next slice. */
            if (++lg >= MAX_ABITS) {
                break;
            }
            lim = pval;
            pval <<= 1;
        }
        else {
            lim = mini_step(lim, &nums_i);
            if (lim == -1) {
                /* No more distinct mini-slice step; continue
                 * to a bigger slice. */
                assert(nums_i % MINI_COEFF == 0);
                goto move_slice;
            }
        }

        /* It shouldn't be possible to reach 'NUMS_SIZE'. */
        assert(nums_i < NUMS_SIZE);

        Py_ssize_t lc = 0;  /* local counter */
        if (lim > t->asize) {
            lim = t->asize;
            if (i >= lim) {
                /* No more elements to count; break. */
                break;
            }
        }

        /* Count elements in the mini-slice. */
        for (; i < lim; i++) {
            if (!ISNIL(ELEMAT(t, i))) {
                lc++;
            }
        }

        /* Add the local counter to the current (mini-)slice and the
         * total used by the array itself. */
        ct->nums[nums_i] += lc;
        ause += lc;
    }

    /* Add the total number of array elements to the total
     * number of entries in both parts and the supposed number of
     * possible array entries. */
    ct->total += ause;
    ct->na += ause;
}

/*
** Count keys in the hash part of table 't'. Assumes that it's always
** called during a rehash when all nodes have been used.
*/
static void
count_hash(const LTable *t, Counters *ct)
{
    Py_ssize_t i = HSIZE(t);
    Py_ssize_t total = 0;

    while (i--) {
        LNode *n = NODEAT(t, i);
        if (ABSENT(n)) {
            /* Deleted entry; signal its existence. */
            ct->deld = TRUE;
        }
        else {
            total++;
            if (IS_INTKEY(n)) {
                /* Might be a valid array index. */
                count_int(n->key_i, ct);
            }
        }
    }

    /* Add the total number of hash entries to the total number
     * of entries in both parts. */
    ct->total += total;
}

/*
** Checks whether it is worth to use 'NA' array entries
** over 'NH' hash nodes.
*/
#define ARRXHASH(NA, NH) \
    ((NA)/sizeof(LNode) <= (NH)/sizeof(*FIELDOF(LTable, array)))

/*
** (Copied almost word-for-word from Lua's /ltable.c)
** Compute the optimal size for the array part of table 't'.
**
** The optimal size maximizes the number of entries going to the array part
** while also considering 'ARRXHASH' if all those entries were placed in the
** hash part.
** 'ct->na' enters with the total number of array indices in the table and
** leaves with the number of entries that will go to the array part;
** The return value is the optimal size for the array part.
*/
static Py_ssize_t
compute_optimal(Counters *ct)
{
    small_t lg;
    Py_ssize_t pval;  /* power-of-2 value of 'lg' (=2^lg) */
    Py_ssize_t lim;  /* upper size of the current (mini-)slice */
    uint16_t i;  /* index of the current (mini-)slice */
    Py_ssize_t a = 0;  /* number of entries smaller than 'lim' */
    Py_ssize_t na = 0;  /* number of entries going to array part */
    Py_ssize_t optm = 0;  /* optimal size for array part */

    /* Traverse mini-slices until 'i' reaches the end index of
     * 'ct->nums' (and 'lg' exceeds 'MAX_ABITS' and 'pval' overflows) or until
     * the total number of array entries don't satisfy 'ARRXHASH' against
     * the possible number of array indices converted to hash nodes. */
    for (lg = lim = i = 0, pval = 1;
         ARRXHASH(lim, ct->na);
         i++)
    {
        /* Logic copied from count_arr(). */
        if (i % MINI_COEFF == 0) {
move_slice:;
            if (++lg >= MAX_ABITS) {
                break;
            }
            lim = pval;
            pval <<= 1;
        }
        else {
            lim = mini_step(lim, &i);
            if (lim == -1) {
                assert(i % MINI_COEFF == 0);
                goto move_slice;
            }
        }

        /* It shouldn't be possible to reach 'NUMS_SIZE'. */
        assert(i < NUMS_SIZE);

        Py_ssize_t inslice = ct->nums[i];
        a += inslice;
        if (inslice > 0 &&  /* If it gets more entries... */
            ARRXHASH(lim, a))  /* ...while using "less memory"... */
        {
            /* ...then set a new optimal size. */
            optm = lim;
            na = a;  /* all elements up to 'optm' will go to the array part */
        }
    }

    /* Assign the number of entries below 'optm'. */
    ct->na = na;
    return optm;
}


static void
clear_arr_slice(const LTable *t, Py_ssize_t i, Py_ssize_t j)
{
    assert(t->array && i <= j);
    for (; i < j; i++) {
        Py_CLEAR(t->array[i]);
    }
}

static PyObject **
resize_array(LTable *t, Py_ssize_t newsize)
{
    if (t->asize == newsize) {
        /* Nothing to do. */
        return t->array;
    }

    if (newsize == 0) {
        /* Clear the entire array. */
        if (t->array) {
            clear_arr_slice(t, 0, t->asize);
            PyObject_Free(LENHINT_PTR(t));
        }
        return NULL;
    }

    /* The case where the new size is lesser than the original size
     * is handled in the resizing function for both parts. */

    PyObject **newptr = (PyObject **)PyObject_Realloc(
        t->array ? LENHINT_PTR(t->array) : NULL,
        (newsize + 1) * sizeof(*newptr));
    if (newptr == NULL) {
        return NULL;
    }

    /* Adjust for the length hint pointer at the start. */
    newptr += 1;

    if (newsize > t->asize) {
        /* If the new size is bigger, zero-initialize the new space. */
        memset(newptr + t->asize, 0,
               sizeof(*newptr) * (newsize - t->asize));
    }

    return newptr;
}

static int
new_nodearr(LTable *t, Py_ssize_t size)
{
    assert(size >= 0);
    if (size == 0) {
        /* No elements; set hash part to dummy node and set the table flag
         * indicating so. */
        t->node = DUMMYNODE;
        t->lg2n_node = 0;
        SET_FLAG(t, FL_HASDUMMY);
        return 0;
    }
    else {
        /* Get the nearest power of 2 above given size;
         * resulting value must not exceed certain limits. */
        uint8_t lg2_size = clg2(size);
        if (lg2_size > MAX_HBITS) {
            goto nomem;
        }

        Py_ssize_t size = 1LL << lg2_size;
        if (size > MAX_HSIZE) {
            goto nomem;
        }

        if (!LFREEABLE(lg2_size)) {
            /* The size is too small to need last-free info. */
            t->node = (LNode *)PyMem_Calloc(size, sizeof(LNode));
            if (t->node == NULL) {
                goto nomem;
            }
        }
        else {
            /* Allocate the size plus last-free info. */
            size_t bsize = size*sizeof(LNode) + sizeof(LFreeBox);
            char *block = (char *)PyMem_Malloc(bsize);
            if (block == NULL) {
                goto nomem;
            }
            memset(block, 0, bsize);
            t->node = CAST(LNode *, block + sizeof(LFreeBox));

            /* All slots are free; set last-free info to point to the end
             * of the hash table. */
            LASTFREE(t) = NODEAT(t, size);
        }

        /* Set the table's log2 size field to its new value. */
        t->lg2n_node = lg2_size;
        DEL_FLAG(t, FL_HASDUMMY);
    }
    return 0;
nomem:
    PyErr_NoMemory();
    return -1;
}

/*
** Exchanges the hash parts of 't1' and 't2'.
**
** Only the dummy bit is exchanged in the flags part since the other
** flags are irrelevant to the element insertion process.
*/
static void
xch_hashpart(LTable *t1, LTable *t2)
{
    uint8_t t1_lg2n = t1->lg2n_node;
    t1->lg2n_node = t2->lg2n_node;
    t2->lg2n_node = t1_lg2n;

    LNode *t1_node = t1->node;
    t1->node = t2->node;
    t2->node = t1_node;

    flag_t t1_bitdummy = GET_FLAG(t1, FL_HASDUMMY);
    t1->flags = (t1->flags & NODUMMY_MASK) | GET_FLAG(t2, FL_HASDUMMY);
    t2->flags = (t2->flags & NODUMMY_MASK) | t1_bitdummy;
}

/*
** Re-insert the elements from the vanishing part of the table array
** into the hash part of the table.
*/
static void
move_arr_hash(LTable *t, Py_ssize_t new_size)
{
    for (Py_ssize_t i = new_size; i < t->asize; i++) {
        PyObject *val = ELEMAT(t, i);
        if (!ISNIL(val)) {
            /* There's a non-empty item, so insert it into the hash part;
             * it *should* have enough space. */
            assert(insert_int_key(t, i, val) >= 0);
        }
    }
}

/*
** Frees the hash part of table 't'.
**
** The elements inserted into the hash all have stolen references,
** so the hash part is simply freed (if the table isn't a dummy one).
*/
static void
free_hash(LTable *t)
{
    if (!HASDUMMY(t)) {
        PyMem_Free(HASHPART_PTR(t));
    }
}

/*
** (Re)insert all elements from old table 'ot' into new table 'nt'.
** 'nt' *should* have enough space for all elements being reinserted.
*/
static void
reinsert_hash(LTable *ot, LTable *nt)
{
    Py_ssize_t length = HSIZE(ot);
    for (Py_ssize_t i = 0; i < length; i++) {
        LNode *old = NODEAT(ot, i);
        if (!ABSENT(old)) {
            if (IS_INTKEY(old)) {
                assert(new_int_key(nt, old->key_i, old->val) >= 0);
                continue;
            }
            if (IS_FLTKEY(old)) {
                assert(insert_flt_key(nt, old->key_f, old->val) >= 0);
                continue;
            }
            assert(insert_key(nt, old->key, old->val) >= 0);
        }
    }
}

/*
** Move all the hash part's int keys that could work as array indices into
** the array part.
*/
static void
move_needed(LTable *t)
{
    Py_ssize_t length = HSIZE(t);
    LNode **lfree_ptr = NULL;
    if (HAS_LFREE(t)) {
        lfree_ptr = &LASTFREE(t);
    }
    for (Py_ssize_t i = 0; i < length; i++) {
        LNode *node = NODEAT(t, i);
        if (!ABSENT(node) &&
            IS_INTKEY(node) &&
            INBOUNDS(t, node->key_i)
        ) {
            ELEMAT(t, node->key_i) = node->val;
            if (node->next) {
                /* If the node has a chain, make the next link occupy the
                 * space by copying it, then make its old space available for
                 * last-free. */
                Py_ssize_t off = node->next;
                LNode *nx = NEXT(node);
                *node = *nx;
                if (node->next != 0) {
                    /* If the copied link has a next-link,
                     * adjust its offset. */
                    node->next += off;
                }
                node = nx;  /* reassign variable for last-free update below */
            }
            /* Update where last-free points to (if it exists). */
            if (lfree_ptr && node > *lfree_ptr) {
                *lfree_ptr = node;
            }
            /* Clear node contents. */
            node->val = NULL;
            node->spc_type = SPCK_NONE;
            node->key = NULL;
            node->next = 0;
        }
    }
}

/*
** Resize table 't', given a new array size and a new hash table size.
** This function ensures the data of both parts remain the same in case
** reallocations may fail, similar to Lua's behavior.
**
** If the hash table reallocation fails, the function simply exits.
**
** If the array is shrinking, it moves the items from the vanishing part
** of the array into the new hash part. If the array reallocation fails,
** the original table is not affected and the new hash part is freed.
**
** If the (re)allocations are a success, the elements from the old hash
** part are reinserted into the new hash part.
** Returns 1 if the table changes, 0 if the table was left unchanged, and
** -1 for error.
*/
static small_t
resize_table(LTable *t, Py_ssize_t new_asize, Py_ssize_t new_hsize)
{
    if (new_asize > MAX_ASIZE) {
        PyErr_NoMemory();
        return -1;
    }

    Py_ssize_t old_asize = t->asize;
    LTable newt = {};
    small_t no_new_hash = 0;

    /* If the size of the hash part changed, create a new hash part with
     * appropriate size in 'newt'. */
    if (t->lg2n_node == clg2(new_hsize)) {  /* Does the hash part's size
                                             * change? */
        if (old_asize == new_asize) {
            /* The array part's size is also the same; nothing changes. */
            return 0;
        } else if (old_asize < new_asize) {
            /* If the array grows and the hash part size remains the same,
             * then we won't need to reallocate the hash part to prevent
             * any losses. */
            no_new_hash = 1;
        }
    }

    if (!no_new_hash && new_nodearr(&newt, new_hsize) < 0) {
        return -1;
    }

    if (new_asize < old_asize) {
        /* Array will shrink; insert vanishing items into new hash. */
        assert(!no_new_hash);
        xch_hashpart(t, &newt);  /* pretend 't' has the new hash part */
        move_arr_hash(t, new_asize);
        xch_hashpart(t, &newt);  /* restore old hash part in case of errors */
    }

    PyObject **new_array = resize_array(t, new_asize);
    if (new_array == NULL && new_asize > 0) {
        /* Allocation failed; free new hash part and set an error. */
        free_hash(&newt);
        PyErr_NoMemory();
        return -1;
    }

    /* All allocations OK, move new data into the original table. */
    t->array = new_array;  /* assign new array and size */
    t->asize = new_asize;
    if (new_array != NULL) {
        /* Set an initial hint. */
        LENHINT(t) = new_asize < old_asize ? new_asize : old_asize;
    }

    if (no_new_hash) {
        /* Move all the keys we can from the hash part into the array. */
        move_needed(t);
    }
    else {
        xch_hashpart(t, &newt);  /* exchange hash parts */
        /* 'newt' should now have the old hash part here; transfer its entries
         * into the new table and free the old one. */
        reinsert_hash(&newt, t);
        free_hash(&newt);
    }

    return 1;
}

/*
** Rehash table 't' with pointer info of a to-be-inserted extra entry 'en'.
**
** First, count entries in its hash part. If it has keys that could be valid
** array indices (that is, valid array indices outside of the array part),
** then count the entries in the array part and recompute the optimal size
** for the array.
** If it doesn't have such keys, then the hash and array parts are distinct,
** and the array size is left unchanged.
** After counting, the table is resized.
** Returns 1 if the table was resized, 0 if it is left unchanged, and
** -1 for error.
*/
static small_t
rehash_table(LTable *t, LNode *en)
{
    Py_ssize_t asize;  /* optimal size for array part */
    Py_ssize_t hsize;  /* size for the hash part */
    Counters ct = {};

    /* Count extra entry (if any). */
    if (en != NULL) {
        ct.total = 1;
        if (IS_INTKEY(en)) {
            count_int(en->key_i, &ct);
        }
    }

    count_hash(t, &ct);  /* count hash part entries */
    if (ct.na == 0) {
        /* No new entries in the array part; don't change it. */
        asize = t->asize;
    }
    else {
        count_arr(t, &ct);  /* count array part entries */
        asize = compute_optimal(&ct);  /* compute the best size */
    }

    /* All keys not in the array part go to the hash part. */
    hsize = ct.total - ct.na;
    if (ct.deld) {
        /* Table has deleted entries; give hash part some extra size
         * to avoid repeated resizings. */
        hsize += hsize >> 2;
    }

    /* Finally, resize the table. */
    return resize_table(t, asize, hsize);
}

/*
** Copy the array of objects 'arr' and put it into table 't',
** given 'size' as its length and 'start' as the index from which to place
** the elements in the table. Returns 0 on success and -1 on error.
*/
static small_t
copy_arr(LTable *t, PyObject **arr, Py_ssize_t size, Py_ssize_t start)
{
    /* Resize the table's array part. */
    if (size > t->asize && resize_table(t, size, HSIZE(t)) < 0) {
        return -1;
    }

    /* Copy all the elements from 'arr' into the array part of 't'. */
    for (Py_ssize_t i = start; i < size; i++) {
        ELEMAT(t, i) = Py_XNewRef(arr[i - start]);
    }

    return 0;
}

/*
** Copy a mapping into table 't' with enough space for it.
** Returns 0 on success and -1 on error.
*/
static small_t
copy_map(LTable *t, PyObject *mapping)
{
    small_t do_rehash = FALSE;
    if (PyDict_Check(mapping)) {
        /* Fast path for dicts. */
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(mapping, &pos, &key, &value)) {
            /* There should be enough space. */
            small_t is_int = FALSE;
            assert(new_key(
                t, Py_NewRef(key), Py_NewRef(value), &is_int) >= 0);
            do_rehash |= is_int;
        }

        if (do_rehash) {
            goto do_rehash_label;
        }
        return 0;
    }

    PyObject *items_o = PyMapping_Items(mapping);
    if (items_o == NULL) {
        return -1;
    }

    PyObject **items = PySequence_Fast_ITEMS(items_o);
    Py_ssize_t size = PySequence_Fast_GET_SIZE(items_o);
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject *pair = items[i];
        if (!PyTuple_Check(pair) || PyTuple_GET_SIZE(pair) != 2) {
            PyErr_SetString(PyExc_TypeError,
                            "key-value pair is not a tuple of size 2");
            Py_DECREF(items_o);
            return -1;
        }
        PyObject *key = PyTuple_GET_ITEM(pair, 0);
        PyObject *value = PyTuple_GET_ITEM(pair, 1);

        /* There should be enough space. */
        small_t is_int = FALSE;
        assert(new_key(t, Py_NewRef(key), Py_NewRef(value), &is_int) >= 0);
        do_rehash |= is_int;
    }

    Py_DECREF(items_o);
    if (do_rehash) {
        goto do_rehash_label;
    }
    return 0;

do_rehash_label:;
    return rehash_table(t, NULL) < 0 ? -1 : 0;
}

static PyType_Spec LTableType_spec;

static small_t
LTable_new_meta(LTable *meta, PyTypeObject **type)
{
    assert(type != NULL);
    if (meta->_astype) {
        *type = meta->_astype;
        return 0;
    }
    PyType_Spec meta_spec = LTableType_spec;
    PyTypeObject *new_type = (PyTypeObject *)PyType_FromMetaclass(
        Py_TYPE(meta), NULL, &meta_spec, (PyObject *)*type);
    if (new_type == NULL) {
        return -1;
    }
    meta->_astype = new_type;
    *type = new_type;
    return 0;
}

static LTable *
LTable_create(PyTypeObject *type,
              PyObject *initobj,
              PyObject *extra,
              PyObject *meta)
{
    LTable *self = NULL;
    PyObject *temp = NULL;
    PyObject *extra_temp = NULL;
    small_t temp_meta = 0, add_dict = 0;
    Py_ssize_t dict_size = 0;

    if (!ISNIL(meta) && !PyType_IsSubtype(Py_TYPE(meta), type)) {
        meta = (PyObject *)LTable_create(type, meta, NULL, NULL);
        if (meta == NULL) {
            return NULL;
        }
        temp_meta = 1;
    }
    if (meta) {
        if (LTable_new_meta((LTable *)meta, &type) < 0) {
            goto error;
        }
    }

    if (!ISNIL(initobj)) {
        if (PySequence_Check(initobj) || !PyMapping_Check(initobj)) {
            temp = PySequence_Fast(initobj, "invalid table initializer");
            if (temp == NULL) {
                goto error;
            }
        }
        else {
            add_dict = 1;
            dict_size = PyMapping_Size(initobj);
            if (dict_size == -1) {
                goto error_no_tempseq;
            }
        }
    }

    if (!ISNIL(extra)) {
        if (PySequence_Check(extra) || !PyMapping_Check(extra)) {
            extra_temp = PySequence_Fast(
                extra, "invalid table extra initializer");
            if (extra_temp == NULL) {
                goto error;
            }
        }
        else {
            add_dict |= 2;
            Py_ssize_t extra_size = PyMapping_Size(extra);
            if (extra_size == -1) {
                goto error;
            }
            dict_size += extra_size;
        }
    }

    self = (LTable *)type->tp_alloc(type, 0);
    if (self == NULL) {
        goto error;
    }

    self->flags = LTM_FASTMASK;
    self->asize = 0;
    self->array = NULL;
    self->meta = (LTable *)meta;
    temp_meta = 0;
    self->_astype = NULL;
    new_nodearr(self, dict_size);

    if (temp != NULL) {
        small_t ret = 0;
        Py_ssize_t size = PySequence_Fast_GET_SIZE(temp);
        if (extra_temp != NULL) {
            /* Do this one first because it allocates a larger space. */
            Py_ssize_t extra_size = size;
            extra_size += PySequence_Fast_GET_SIZE(extra_temp);
            ret = copy_arr(
                self, PySequence_Fast_ITEMS(extra_temp),
                extra_size, size);
            Py_SETREF(extra_temp, NULL);
        }
        if (ret >= 0) {
            ret = copy_arr(
                self, PySequence_Fast_ITEMS(temp), size, 0);
        }
        Py_SETREF(temp, NULL);
        if (ret < 0) {
            goto error_no_tempseq;
        }
    }
    else if (extra_temp != NULL) {
        small_t ret = copy_arr(
            self, PySequence_Fast_ITEMS(extra_temp),
             PySequence_Fast_GET_SIZE(extra_temp), 0);
        Py_SETREF(extra_temp, NULL);
         if (ret < 0) {
            goto error_no_tempseq;
        }
    }

    if (add_dict != 0) {
        assert(dict_size != -1);
        if ((add_dict & 1) != 0 && copy_map(self, initobj) < 0) {
            goto error_no_tempseq;
        }
        if ((add_dict & 2) != 0 && copy_map(self, extra) < 0) {
            goto error_no_tempseq;
        }
    }

    return (LTable *)self;

error:;
    Py_XDECREF(temp);
    Py_XDECREF(extra_temp);
error_no_tempseq:;
    if (temp_meta) {
        Py_DECREF(meta);
    }
    Py_XDECREF(self);
    return NULL;
}

static LTable *
LTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *initobj = NULL;
    PyObject *extra = NULL;
    PyObject *meta = NULL;
    static char *kwnames[] = {"", "extra", "metatables", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwds,
            "|OOO:luatable.__new__",
            kwnames,
            &initobj, &extra, &meta))
    {
        return NULL;
    }
    return LTable_create(type, initobj, extra, meta);
}

static int
LTable_traverse(LTable *self, visitproc visit, void *arg)
{
    for (Py_ssize_t i = 0; i < self->asize; i++) {
        Py_VISIT(ELEMAT(self, i));
    }

    small_t weak_v = self->meta && HAS_FLAG(self->meta, TM_MODE_V);
    small_t weak_k = self->meta && HAS_FLAG(self->meta, TM_MODE_K);

    Py_ssize_t length = HSIZE(self);
    for (Py_ssize_t i = 0; i < length; i++) {
        LNode *node = NODEAT(self, i);
        Py_VISIT(node->val);
        Py_VISIT(node->key);

        Py_ssize_t val_refn = 0;
        if (weak_v && !ISNIL(node->val)) {
            val_refn++;
        }

        Py_ssize_t key_refn = 0;
        if (weak_k && IS_OBJNODE(node) && !ISNIL(node->key)) {
            key_refn++;
        }

        if (val_refn == 0 && key_refn == 0) {
            continue;
        }

        for (Py_ssize_t j = i + 1; j < length; j++) {
            LNode *subnode = NODEAT(node, j);
            if (val_refn && node->val == subnode->val) {
                val_refn++;
            }
            if (key_refn && node->key == subnode->key) {
                key_refn++;
            }
        }
        if (
            val_refn && Py_REFCNT(node->val) == val_refn
            || key_refn && Py_REFCNT(node->key) == key_refn
        ) {
            Py_CLEAR(node->val);
            Py_CLEAR(node->key);
        }
    }

    Py_VISIT(self->meta);
    Py_VISIT(self->_astype);
    return 0;
}

static int
LTable_clear(LTable *self)
{
    for (Py_ssize_t i = 0; i < self->asize; i++) {
        Py_CLEAR(ELEMAT(self, i));
    }
    Py_ssize_t length = HSIZE(self);
    for (Py_ssize_t i = 0; i < length; i++) {
        LNode *node = NODEAT(self, i);
        Py_CLEAR(node->val);
        if (IS_OBJNODE(node)) {
            Py_CLEAR(node->key);
        }
    }
    Py_CLEAR(self->meta);
    Py_CLEAR(self->_astype);
    return 0;
}

static void
LTable_dealloc(LTable *self)
{
    PyObject_GC_UnTrack(self);
    LTable_clear(self);
    if (self->array) {
        PyObject_Free(LENHINT_PTR(self));
    }
    if (!HASDUMMY(self)) {
        PyMem_Free(HASHPART_PTR(self));
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyType_Slot LTableType_slots[] = {
    {Py_tp_doc, "Lua table implementation as a Python C extension."},
    {Py_tp_new, LTable_new},
    {Py_tp_traverse, LTable_traverse},
    {Py_tp_clear, LTable_clear},
    {Py_tp_dealloc, LTable_dealloc},
    {0, NULL}
};

static PyType_Spec LTableType_spec = {
    .name = "luatablec.luatable",
    .basicsize = sizeof(LTable),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = LTableType_slots,
};

static int
luatablec_mod_exec(PyObject *m)
{
    LTable_Type = (PyTypeObject *)PyType_FromSpec(&LTableType_spec);
    if (LTable_Type == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "LuaTable", (PyObject *)LTable_Type) < 0) {
        return -1;
    }
    long_hash = PyLong_Type.tp_hash;
    float_hash = PyFloat_Type.tp_hash;
    return 0;
}

static PyModuleDef_Slot luatablec_modslots[] = {
    {Py_mod_exec, luatablec_mod_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};

static PyModuleDef luatablec_mod = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "luatablec",
    .m_doc = "Lua table C extension!!",
    .m_size = 0,
    .m_slots = luatablec_modslots,
};

PyMODINIT_FUNC
PyInit_luatablec(void)
{
    return PyModuleDef_Init(&luatablec_mod);
}