#pragma once
#include "hy3d_types.h"
#include "math.h"

struct vec3
{
    vec3()
        : x(0.0f), y(0.0f), z(0.0f)
    {
    }
    vec3(r32 x, r32 y, r32 z)
        : x(x), y(y), z(z)
    {
    }
    vec3(r32 x, r32 y)
        : x(x), y(y), z(0.0f)
    {
    }

    inline vec3 operator=(vec3 b)
    {
        x = b.x;
        y = b.y;
        z = b.z;
        return *this;
    }

    inline vec3 operator/(r32 b)
    {
        return vec3(x / b, y / b, z / b);
    }

    inline r32 lengthSq()
    {
        return (x * x + y * y + z * z);
    }

    inline r32 length()
    {
        return sqrtf(lengthSq());
    }

    inline vec3 normal()
    {
        r32 l = length();
        if (l == 0.0f)
            return vec3();
        return (*this / l);
    }

    inline void normalize()
    {
        *this = this->normal();
    }

    r32 x, y, z;
};

// addition, subtraction
inline vec3 operator+(vec3 a, vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline vec3 operator-(vec3 a, vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline vec3 operator+=(vec3 &a, vec3 b)
{
    a = a + b;
    return a;
}

inline vec3 operator-=(vec3 &a, vec3 b)
{
    a = a - b;
    return a;
}

// vector * number
inline vec3 operator*(r32 a, vec3 b)
{
    return {a * b.x, a * b.y, a * b.z};
}

inline vec3 operator*(vec3 b, r32 a)
{
    return a * b;
}

inline vec3 operator*=(vec3 &a, r32 b)
{
    a = b * a;
    return a;
}

// negative of a vector
inline vec3 operator-(vec3 a)
{
    return {-a.x, -a.y, -a.z};
}

// dot product
inline r32 operator*(vec3 a, vec3 b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline r32 DotProduct(vec3 a, vec3 b)
{
    return (a * b);
}

inline vec3 CrossProduct(vec3 a, vec3 b)
{
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

// comparison
inline bool operator==(vec3 a, vec3 b)
{
    return (a.x == b.x && a.y == b.y && a.z == b.z);
}

inline bool operator!=(vec3 a, vec3 b)
{
    return !(a == b);
}