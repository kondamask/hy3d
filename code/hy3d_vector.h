#pragma once
#include "hy3d_types.h"
#include "math.h"

struct vec3
{
    f32 x, y, z;

    inline vec3 operator=(vec3 b)
    {
        x = b.x;
        y = b.y;
        z = b.z;
        return *this;
    }

    inline vec3 operator/(f32 b)
    {
        return vec3{x / b, y / b, z / b};
    }

    inline f32 lengthSq()
    {
        return (x * x + y * y + z * z);
    }

    inline f32 length()
    {
        return sqrtf(lengthSq());
    }

    inline vec3 normal()
    {
        f32 l = length();
        if (l == 0.0f)
            return {};
        return (*this / l);
    }

    inline void normalize()
    {
        *this = this->normal();
    }
};

// NOTE:  addition, subtraction
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

// NOTE:  vector * number
inline vec3 operator*(f32 a, vec3 b)
{
    return {a * b.x, a * b.y, a * b.z};
}

inline vec3 operator*(vec3 b, f32 a)
{
    return a * b;
}

inline vec3 operator*=(vec3 &a, f32 b)
{
    a = b * a;
    return a;
}

// NOTE:  negative of a vector
inline vec3 operator-(vec3 a)
{
    return {-a.x, -a.y, -a.z};
}

// NOTE:  dot product
inline f32 operator*(vec3 a, vec3 b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline f32 DotProduct(vec3 a, vec3 b)
{
    return (a * b);
}

inline vec3 CrossProduct(vec3 a, vec3 b)
{
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

// NOTE:  comparison
inline bool operator==(vec3 a, vec3 b)
{
    return (a.x == b.x && a.y == b.y && a.z == b.z);
}

inline bool operator!=(vec3 a, vec3 b)
{
    return !(a == b);
}

inline vec3 lerp(vec3 P, vec3 A, vec3 B, f32 alpha)
{
    return P + (B - P) * alpha;
}

// ----------------------------------------------------------------------------------------
// NOTE:  VEC2 ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

struct vec2
{
    f32 x, y;

    inline vec2 operator=(vec2 b)
    {
        x = b.x;
        y = b.y;
        return *this;
    }

    inline vec2 operator-(vec2 b)
    {
        return vec2{x - b.x, y - b.y};
    }

    inline vec2 operator/(f32 b)
    {
        return vec2{x / b, y / b};
    }

    inline f32 lengthSq()
    {
        return (x * x + y * y);
    }

    inline f32 length()
    {
        return sqrtf(lengthSq());
    }

    inline vec2 normal()
    {
        f32 l = length();
        if (l == 0.0f)
            return {};
        return (*this / l);
    }

    inline void normalize()
    {
        *this = this->normal();
    }
};

// NOTE:  addition, subtraction
inline vec2 operator+(vec2 a, vec2 b)
{
    return {a.x + b.x, a.y + b.y};
}

inline vec2 operator+=(vec2 &a, vec2 b)
{
    a = a + b;
    return a;
}

inline vec2 operator-=(vec2 &a, vec2 b)
{
    a = a - b;
    return a;
}

// NOTE:  vector * number
inline vec2 operator*(f32 a, vec2 b)
{
    return {a * b.x, a * b.y};
}

inline vec2 operator*(vec2 b, f32 a)
{
    return a * b;
}

inline vec2 operator*=(vec2 &a, f32 b)
{
    a = b * a;
    return a;
}

// NOTE:  negative of a vector
inline vec2 operator-(vec2 a)
{
    return {-a.x, -a.y};
}

// NOTE:  dot product
inline f32 operator*(vec2 a, vec2 b)
{
    return (a.x * b.x + a.y * b.y);
}

inline f32 DotProduct(vec2 a, vec2 b)
{
    return (a * b);
}

// NOTE:  comparison
inline bool operator==(vec2 a, vec2 b)
{
    return (a.x == b.x && a.y == b.y);
}

inline bool operator!=(vec2 a, vec2 b)
{
    return !(a == b);
}

inline vec2 lerp(vec2 P, vec2 A, vec2 B, f32 alpha)
{
    return P + (B - P) * alpha;
}