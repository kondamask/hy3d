#pragma once

#include "hy3d_math.h"

struct vertex
{
    vec3 pos;      // world position
    vec2 texCoord; // texture coordinate
    vec3 normal;

    inline vertex interpolateTo(vertex A, vertex B)
    {
        f32 alpha = (A.pos.y - pos.y) / (B.pos.y - pos.y);
        return {
            lerp(pos, A.pos, B.pos, alpha),
            lerp(texCoord, A.texCoord, B.texCoord, alpha),
            normal};
    }
};

inline vertex operator+(vertex a, vertex b)
{
    return {a.pos + b.pos, a.texCoord + b.texCoord, a.normal + b.normal};
}

inline vertex operator-(vertex a, vertex b)
{
    return {a.pos - b.pos, a.texCoord - b.texCoord, a.normal - b.normal};
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
    return {a * b.pos, a * b.texCoord, a * b.normal};
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
    b = 1.0f / b;
    return {a.pos * b, a.texCoord * b, a.normal * b};
}

inline vertex operator-(vertex a)
{
    return {-a.pos, -a.texCoord, -a.normal};
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

struct vertex_smooth
{
    vec3 pos;      // world position
    vec2 texCoord; // texture coordinate
    vec3 color;
    vec3 normal;

    inline vertex_smooth interpolateTo(vertex_smooth A, vertex_smooth B)
    {
        f32 alpha = (A.pos.y - pos.y) / (B.pos.y - pos.y);
        return {
            lerp(pos, A.pos, B.pos, alpha),
            lerp(texCoord, A.texCoord, B.texCoord, alpha),
            lerp(color, A.color, B.color, alpha),
            lerp(normal, A.normal, B.normal, alpha)};
    }
};

inline vertex_smooth GetSmoothVertex(vertex a)
{
    vertex_smooth result = {};
    result.pos = a.pos;
    result.texCoord = a.texCoord;
    result.normal = a.normal;
    return result;
}

inline vertex_smooth operator+(vertex_smooth a, vertex_smooth b)
{
    return {a.pos + b.pos, a.texCoord + b.texCoord, a.color + b.color, a.normal + b.normal};
}

inline vertex_smooth operator+=(vertex_smooth &a, vertex_smooth b)
{
    a = a + b;
    return a;
}

inline vertex_smooth operator-(vertex_smooth a, vertex_smooth b)
{
    return {a.pos - b.pos, a.texCoord - b.texCoord, a.color - b.color, a.normal - b.normal};
}

inline vertex_smooth operator-=(vertex_smooth &a, vertex_smooth b)
{
    a = a - b;
    return a;
}

inline vertex_smooth operator-(vertex_smooth a)
{
    return {-a.pos, -a.texCoord, a.color, -a.normal};
}

inline vertex_smooth operator*(f32 a, vertex_smooth b)
{
    return {a * b.pos, a * b.texCoord, b.color, a * b.normal};
}

inline vertex_smooth operator*(vertex_smooth b, f32 a)
{
    return a * b;
}

inline vertex_smooth operator*=(vertex_smooth &a, f32 b)
{
    a = b * a;
    return a;
}

inline vertex_smooth operator/(vertex_smooth a, f32 b)
{
    b = 1.0f / b;
    return {a.pos * b, a.texCoord * b, a.color * b, a.normal * b};
}

inline vertex_smooth VertexSlopeX(vertex_smooth a, vertex_smooth b)
{
    return (-(b - a) / (b.pos.x - a.pos.x));
}

inline vertex_smooth VertexSlopeY(vertex_smooth a, vertex_smooth b)
{
    return (-(b - a) / (b.pos.y - a.pos.y));
}
