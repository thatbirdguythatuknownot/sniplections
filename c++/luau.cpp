#include <cmath>
#include <cfloat>
#include <string>
#include <iostream>
#include <memory>
#include <iomanip>
#include <type_traits>
#include <variant>

#if defined(__OPTIMIZE__)

/* ASSUME() definition */
#ifdef __clang__
#  define ASSUME(value) (void)__builtin_assume(value)
#elif defined(__GNUC__) && (__GNUC__ > 4) && (__GNUC_MINOR__ >= 5)
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79469
   `__builtin_object_size(( (void)(value), "" ), 2)` checks whether the
    expression is constant. */
#  define ASSUME(value) \
    (__builtin_object_size(( (void)(value), "" ), 2) \
        ? ((value) ? (void)0 : __builtin_unreachable()) \
        : (void)0 \
    )
#elif defined(_MSC_VER)
/* We don't have the "just to make sure it's constant" check here. */
#  define ASSUME(value) (void)__assume(value)
#else
#  define ASSUME(value) ((void)0)
#endif

#endif /* __OPTIMIZE__ */

#define DEFCONST(T, name, ...) \
template<> \
T T::name = T(__VA_ARGS__);

template<typename T>
static inline T fuzzyEq(T a, T b) {
    static_assert(std::is_floating_point<T>::value,
                  "can only use fuzzyEq() on floats");
    return (a == b) || (std::abs(a - b) <=
        DBL_EPSILON * std::max(std::abs(a), std::abs(b)));
}

class NormalId {
public:
    enum {
        Right,
        Top,
        Back,
        Left,
        Bottom,
        Front
    };
};

class Axis: public NormalId {
public:
    enum {
        X = NormalId::Right,
        Y = NormalId::Top,
        Z = NormalId::Back
    };
};

template <class number,
          std::enable_if_t<
            std::is_arithmetic<number>::value,
            bool
          > = true>
class Vector2Ti {
public:
    static Vector2Ti<number> zero;
    static Vector2Ti<number> one;
    static Vector2Ti<number> xAxis;
    static Vector2Ti<number> yAxis;

    number X;
    number Y;
    number Magnitude;

    Vector2Ti<number>(number x, number y)
        : X(x), Y(y), Magnitude(std::sqrt(x*x + y*y))
    {}

    Vector2Ti<number>(number n)
        : Vector2Ti<number>(n, n)
    {}

    Vector2Ti<number>()
        : Vector2Ti<number>(0, 0)
    {}

    Vector2Ti<number> operator +(const Vector2Ti<number> other) const {
        return Vector2Ti<number>(
            this->X + other.X,
            this->Y + other.Y
        );
    }

    void operator +=(const Vector2Ti<number> other) {
        this->X += other.X;
        this->Y += other.Y;
    }

    Vector2Ti<number> operator -(const Vector2Ti<number> other) const {
        return Vector2Ti<number>(
            this->X - other.X,
            this->Y - other.Y
        );
    }

    void operator -=(const Vector2Ti<number> other) {
        this->X -= other.X;
        this->Y -= other.Y;
    }

    Vector2Ti<number> operator *(const Vector2Ti<number> other) const {
        return Vector2Ti<number>(
            this->X * other.X,
            this->Y * other.Y
        );
    }

    void operator *=(const Vector2Ti<number> other) {
        this->X *= other.X;
        this->Y *= other.Y;
    }

    Vector2Ti<number> operator /(const Vector2Ti<number> other) const {
        return Vector2Ti<number>(
            this->X / other.X,
            this->Y / other.Y
        );
    }

    void operator /=(const Vector2Ti<number> other) {
        this->X /= other.X;
        this->Y /= other.Y;
    }

    Vector2Ti<number> operator *(const number other) const {
        return Vector2Ti<number>(
            this->X * other,
            this->Y * other
        );
    }

    void operator *=(const number other) {
        this->X *= other;
        this->Y *= other;
    }

    Vector2Ti<number> operator /(const number other) const {
        return Vector2Ti<number>(
            this->X / other,
            this->Y / other
        );
    }

    void operator /=(const number other) {
        this->X /= other;
        this->Y /= other;
    }

    number Cross(const Vector2Ti<number> other) const {
        return this->X*other.Y - this->Y*other.X;
    }

    number Dot(const Vector2Ti<number> other) const {
        return this->X*other.X + this->Y*other.Y;
    }

    Vector2Ti<number> Lerp(const Vector2Ti<number> goal, const number alpha)
        const
    {
        return Vector2Ti<number>(
            this->X + (goal.X - this->X)*alpha,
            this->Y + (goal.X - this->Y)*alpha
        );
    }

    Vector2Ti<number> Max(const Vector2Ti<number> vector) const {
        return Vector2Ti<number>(
            std::max(this->X, vector.X),
            std::max(this->Y, vector.Y)
        );
    }

    Vector2Ti<number> Min(const Vector2Ti<number> vector) const {
        return Vector2Ti<number>(
            std::min(this->X, vector.X),
            std::min(this->Y, vector.Y)
        );
    }

    friend std::ostream &operator<<(std::ostream &out,
                                    const Vector2Ti<number> &vector)
    {
        out << vector.X << ", " << vector.Y;
        return out;
    }

    friend std::istream &operator<<(std::istream &in,
                                    Vector2Ti<number> &vector)
    {
        in >> vector.X >> vector.Y;
        return in;
    }
};

template <class number,
          std::enable_if_t<
            std::is_floating_point<number>::value,
            bool
          > = true>
class Vector2T: public Vector2Ti<number> {
private:
    Vector2T<number>(const char *_) {};

    std::shared_ptr<Vector2T<number>> unitVec() const {
        auto ret = std::shared_ptr<Vector2T<number>>(
            new Vector2T<number>("")
        );
        ret->X = this->X;
        ret->Y = this->Y;
        ret->Magnitude = this->Magnitude;
        ret->Unit = ret;
        return ret;
    }

public:
    static Vector2T<number> zero;
    static Vector2T<number> one;
    static Vector2T<number> xAxis;
    static Vector2T<number> yAxis;

    std::shared_ptr<Vector2T<number>> Unit;

    using Vector2Ti<number>::Vector2Ti;

    Vector2T<number>(number x, number y)
        : Vector2Ti<number>(x, y)
    {
        if (!fuzzyEq<number>(this->Magnitude, 1.) &&
            !fuzzyEq<number>(this->Magnitude, 0.))
        {
            this->Unit = std::shared_ptr<Vector2T<number>>(
                new Vector2T<number>(
                    x / this->Magnitude,
                    y / this->Magnitude
                )
            );
        }
        else {
            this->Unit = this->unitVec();
        }
    }

    number Angle(const Vector2T<number> other,
                 const Vector2T<number> axis = Vector2T<number>::zero) const
    {
        number res = std::acos(
            this->Dot(other) / (this->Magnitude * other.Magnitude)
        );
        return (axis.X > 0 || axis.Z < 0) ? -res : res;
    }

    bool FuzzyEq(const Vector2T<number> other, number epsilon = 0.) const {
        if (epsilon != 0.) {
            goto compare;
        }
        else {
            epsilon = 1. / 100000.;
        }
      compare:

#define CHECK(field) \
    if (this->field == other.field || \
            std::abs(this->field - other.field) <= \
            std::abs(this->field) * epsilon) \
    { \
        goto checked_ ## field; \
    } \
    else { \
        return false; \
    } \
  checked_ ## field:

        CHECK(X)
        CHECK(Y)

        return true;

#undef CHECK
    }
};

typedef Vector2T<double> Vector2;
DEFCONST(Vector2, zero);
DEFCONST(Vector2, one, 1.);
DEFCONST(Vector2, xAxis, 1., 0.);
DEFCONST(Vector2, yAxis, 0., 1.);

typedef Vector2Ti<signed short> Vector2int16;
DEFCONST(Vector2int16, zero);
DEFCONST(Vector2int16, one, 1);
DEFCONST(Vector2int16, xAxis, 1, 0);
DEFCONST(Vector2int16, yAxis, 0, 1);

template <class number,
          std::enable_if_t<
            std::is_arithmetic<number>::value,
            bool
          > = true>
class Vector3Ti {
public:
    static Vector3Ti<number> zero;
    static Vector3Ti<number> one;
    static Vector3Ti<number> xAxis;
    static Vector3Ti<number> yAxis;
    static Vector3Ti<number> zAxis;

    number X;
    number Y;
    number Z;
    number Magnitude;

    Vector3Ti<number>(number x, number y, number z)
        : X(x), Y(y), Z(z), Magnitude(std::sqrt(x*x + y*y + z*z))
    {}

    Vector3Ti<number>(number n)
        : Vector3Ti<number>(n, n, n)
    {}

    Vector3Ti<number>()
        : Vector3Ti<number>(0, 0, 0)
    {}

    static Vector3Ti<number> FromNormalId(const int normal) {
        switch (normal) {
        case NormalId::Top:
            return Vector3Ti<number>(0, 1, 0);
        case NormalId::Bottom:
            return Vector3Ti<number>(0, -1, 0);
        case NormalId::Back:
            return Vector3Ti<number>(0, 0, 1);
        case NormalId::Front:
            return Vector3Ti<number>(0, 0, -1);
        case NormalId::Right:
            return Vector3Ti<number>(1, 0, 0);
        case NormalId::Left:
            return Vector3Ti<number>(-1, 0, 0);
        default:
            ASSUME(0);
        }
    }

    static Vector3Ti<number> FromAxis(const int axis) {
        switch (axis) {
        case Axis::Y: // = Axis.Top
            return Vector3Ti<number>(0, 1, 0);
        case Axis::Bottom:
            return Vector3Ti<number>(0, -1, 0);
        case Axis::Z: // = Axis.Back
            return Vector3Ti<number>(0, 0, 1);
        case Axis::Front:
            return Vector3Ti<number>(0, 0, -1);
        case Axis::X: // = Axis.Right
            return Vector3Ti<number>(1, 0, 0);
        case Axis::Left:
            return Vector3Ti<number>(-1, 0, 0);
        default:
            ASSUME(0);
        }
    }

    Vector3Ti<number> operator +(const Vector3Ti<number> other) const {
        return Vector3Ti<number>(
            this->X + other.X,
            this->Y + other.Y,
            this->Z + other.Z
        );
    }

    void operator +=(const Vector3Ti<number> other) {
        this->X += other.X;
        this->Y += other.Y;
        this->Z += other.Z;
    }

    Vector3Ti<number> operator -(const Vector3Ti<number> other) const {
        return Vector3Ti<number>(
            this->X - other.X,
            this->Y - other.Y,
            this->Z - other.Z
        );
    }

    void operator -=(const Vector3Ti<number> other) {
        this->X -= other.X;
        this->Y -= other.Y;
        this->Z -= other.Z;
    }

    Vector3Ti<number> operator *(const Vector3Ti<number> other) const {
        return Vector3Ti<number>(
            this->X * other.X,
            this->Y * other.Y,
            this->Z * other.Z
        );
    }

    void operator *=(const Vector3Ti<number> other) {
        this->X *= other.X;
        this->Y *= other.Y;
        this->Z *= other.Z;
    }

    Vector3Ti<number> operator /(const Vector3Ti<number> other) const {
        return Vector3Ti<number>(
            this->X / other.X,
            this->Y / other.Y,
            this->Z / other.Z
        );
    }

    void operator /=(const Vector3Ti<number> other) {
        this->X /= other.X;
        this->Y /= other.Y;
        this->Z /= other.Z;
    }

    Vector3Ti<number> operator *(const number other) const {
        return Vector3Ti<number>(
            this->X * other,
            this->Y * other,
            this->Z * other
        );
    }

    void operator *=(const number other) {
        this->X *= other;
        this->Y *= other;
        this->Z *= other;
    }

    Vector3Ti<number> operator /(const number other) const {
        return Vector3Ti<number>(
            this->X / other,
            this->Y / other,
            this->Z / other
        );
    }

    void operator /=(const number other) {
        this->X /= other;
        this->Y /= other;
        this->Z /= other;
    }

    Vector3Ti<number> Cross(const Vector3Ti<number> other) const {
        return Vector3Ti<number>(
            this->Y*other.Z - this->Z*other.Y,
            this->Z*other.X - this->X*other.Z,
            this->X*other.Y - this->Y*other.X
        );
    }

    number Dot(const Vector3Ti<number> other) const {
        return this->X*other.X + this->Y*other.Y + this->Z*other.Z;
    }

    Vector3Ti<number> Lerp(const Vector3Ti<number> goal, const number alpha)
        const
    {
        return Vector3Ti<number>(
            this->X + (goal.X - this->X)*alpha,
            this->Y + (goal.X - this->Y)*alpha,
            this->Z + (goal.X - this->Z)*alpha
        );
    }

    Vector3Ti<number> Max(const Vector3Ti<number> vector) const {
        return Vector3Ti<number>(
            std::max(this->X, vector.X),
            std::max(this->Y, vector.Y),
            std::max(this->Z, vector.Z)
        );
    }

    Vector3Ti<number> Min(const Vector3Ti<number> vector) const {
        return Vector3Ti<number>(
            std::min(this->X, vector.X),
            std::min(this->Y, vector.Y),
            std::min(this->Z, vector.Z)
        );
    }

    friend std::ostream &operator<<(std::ostream &out,
                                    const Vector3Ti<number> &vector)
    {
        out << vector.X << ", " << vector.Y << ", " << vector.Z;
        return out;
    }

    friend std::istream &operator<<(std::istream &in,
                                    Vector3Ti<number> &vector)
    {
        in >> vector.X >> vector.Y >> vector.Z;
        return in;
    }
};

template <class number,
          std::enable_if_t<
            std::is_floating_point<number>::value,
            bool
          > = true>
class Vector3T: public Vector3Ti<number> {
private:
    Vector3T<number>(const char *_) {};

    std::shared_ptr<Vector3T<number>> unitVec() const {
        auto ret = std::shared_ptr<Vector3T<number>>(
            new Vector3T<number>("")
        );
        ret->X = this->X;
        ret->Y = this->Y;
        ret->Z = this->Z;
        ret->Magnitude = this->Magnitude;
        ret->Unit = ret;
        return ret;
    }

public:
    static Vector3T<number> zero;
    static Vector3T<number> one;
    static Vector3T<number> xAxis;
    static Vector3T<number> yAxis;
    static Vector3T<number> zAxis;

    std::shared_ptr<Vector3T<number>> Unit;

    using Vector3Ti<number>::Vector3Ti;

    Vector3T<number>(number x, number y, number z)
        : Vector3Ti<number>(x, y, z)
    {
        if (!fuzzyEq<number>(this->Magnitude, 1.) &&
            !fuzzyEq<number>(this->Magnitude, 0.))
        {
            this->Unit = std::shared_ptr<Vector3T<number>>(new Vector3T<number>(
                x / this->Magnitude,
                y / this->Magnitude,
                z / this->Magnitude
            ));
        }
        else {
            this->Unit = this->unitVec();
        }
    }

    number Angle(const Vector3T<number> other,
                 const Vector3T<number> axis = Vector3T<number>::zero) const
    {
        number res = std::acos(
            this->Dot(other) / (this->Magnitude * other.Magnitude)
        );
        return (axis.X > 0 || axis.Z < 0) ? -res : res;
    }

    bool FuzzyEq(const Vector3T<number> other, number epsilon = 0.) const {
        if (epsilon != 0.) {
            goto compare;
        }
        else {
            epsilon = 1. / 100000.;
        }
      compare:

#define CHECK(field) \
    if (this->field == other.field || \
            std::abs(this->field - other.field) <= \
            std::abs(this->field) * epsilon) \
    { \
        goto checked_ ## field; \
    } \
    else { \
        return false; \
    } \
  checked_ ## field:

        CHECK(X)
        CHECK(Y)
        CHECK(Z)

        return true;

#undef CHECK
    }
};

typedef Vector3T<double> Vector3;
DEFCONST(Vector3, zero);
DEFCONST(Vector3, one, 1.);
DEFCONST(Vector3, xAxis, 1., 0., 0.);
DEFCONST(Vector3, yAxis, 0., 1., 0.);
DEFCONST(Vector3, zAxis, 0., 0., 1.);

typedef Vector3Ti<signed short> Vector3int16;
DEFCONST(Vector3int16, zero);
DEFCONST(Vector3int16, one, 1);
DEFCONST(Vector3int16, xAxis, 1, 0, 0);
DEFCONST(Vector3int16, yAxis, 0, 1, 0);
DEFCONST(Vector3int16, zAxis, 0, 0, 1);
