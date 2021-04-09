#pragma once

#include "hy3d_math.h"

struct vertex
{
    vec3 pos; // world position
    vec2 texCoord;  // texture coordinate
    vec3 normal;

    inline vertex interpolateTo(vertex A, vertex B)
    {
        f32 alpha = (A.pos.y - pos.y) / (B.pos.y - pos.y);
        return {
            lerp(pos, A.pos, B.pos, alpha),
            lerp(texCoord, A.texCoord, B.texCoord, alpha)};
    }
};

inline vertex operator+(vertex a, vertex b)
{
    return {a.pos + b.pos, a.texCoord + b.texCoord};
}

inline vertex operator-(vertex a, vertex b)
{
    return {a.pos - b.pos, a.texCoord - b.texCoord};
}

inline vertex operator+=(vertex &a, vertex b)
{
    a = a + b;
    return a;
}

inline vertex operator-=(vertex &a, vertex b)
{
    a = a - b;
    return a;
}

inline vertex operator*(f32 a, vertex b)
{
    return {a * b.pos, a * b.texCoord};
}

inline vertex operator*(vertex b, f32 a)
{
    return a * b;
}

inline vertex operator*=(vertex &a, f32 b)
{
    a = b * a;
    return a;
}

inline vertex operator/(vertex a, f32 b)
{
    return {a.pos * (1.0f / b), a.texCoord * (1.0f / b)};
}

inline vertex operator-(vertex a)
{
    return {-a.pos, -a.texCoord};
}

// NOTE:  It looks like that returning the negative slope is more accurate.
inline vertex VertexSlopeX(vertex a, vertex b)
{
    return (-(b - a) / (b.pos.x - a.pos.x));
}

inline vertex VertexSlopeY(vertex a, vertex b)
{
    return (-(b - a) / (b.pos.y - a.pos.y));
}
