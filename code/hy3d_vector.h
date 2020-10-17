#include "math.h"

struct vec3
{
    vec3()
        : x(0.0f), y(0.0f), z(0.0f)
    {
    }
    vec3(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }
    vec3(float x, float y)
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

    inline vec3 operator/(float b)
    {
        return vec3(x / b, y / b, z / b);
    }

    inline float lengthSq()
    {
        return (x * x + y * y + z * z);
    }

    inline float length()
    {
        return sqrtf(lengthSq());
    }

    inline vec3 normal()
    {
        float l = length();
        if (l == 0.0f)
            return vec3();
        return (*this / l);
    }

    inline void normalize()
    {
        *this = this->normal();
    }

    float x, y, z;
};

// addition, subtraction
inline vec3 operator+(vec3 a, vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline vec3 operator-(vec3 a, vec3 b)
{
    return {a.x - b.x, a.x - b.y, a.z - b.z};
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
inline vec3 operator*(float a, vec3 b)
{
    return {a * b.x, a * b.y, a * b.z};
}

inline vec3 operator*(vec3 b, float a)
{
    return a * b;
}

inline vec3 operator*=(vec3 &a, float b)
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
inline float operator*(vec3 a, vec3 b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z);
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