#pragma once

#include "hy3d_math.h"

struct vertex
{
    vec3 pos; // world position
    vec2 uv;  // texture coordinate

    inline vertex interpolateTo(vertex A, vertex B)
    {
        f32 alpha = (A.pos.y - pos.y) / (B.pos.y - pos.y);
        return {
            lerp(pos, A.pos, B.pos, alpha),
            lerp(uv, A.uv, B.uv, alpha)};
    }
};

inline vertex operator+(vertex a, vertex b)
{
    return {a.pos + b.pos, a.uv + b.uv};
}

inline vertex operator-(vertex a, vertex b)
{
    return {a.pos - b.pos, a.uv - b.uv};
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
    return {a * b.pos, a * b.uv};
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
    return {a.pos * (1.0f / b), a.uv * (1.0f / b)};
}

inline vertex operator-(vertex a)
{
    return {-a.pos, -a.uv};
}
