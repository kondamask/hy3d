#pragma once
#include "hy3d_types.h"
#include "hy3d_graphics.h"
#include "hy3d_vector.h"
#include "hy3d_matrix.h"
#include <math.h>

// TODO: add Z later
// NOTE:
// The HY3D space is a 3d space where
// Y IS UP
// X IS RIGHT
// Z IS INTO THE SCREEN
// The origin (0,0,0) is in the center of the screen.
// We normalize the coordinates so that the far right, left, top and down
// take values -1.0 and +1.0
struct space3d
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;
    f32 width;
    f32 height;
};

struct screen_transformer
{
    f32 xFactor;
    f32 yFactor;
};

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

inline vertex operator/(vertex a, f32 b)
{
    return {a.pos * (1.0f / b), a.uv * (1.0f / b)};
}

inline vertex operator*=(vertex &a, f32 b)
{
    a = b * a;
    return a;
}

inline vertex operator-(vertex a)
{
    return {-a.pos, -a.uv};
}

struct triangle
{
    vertex v0;
    vertex v1;
    vertex v2;
};

struct processed_triangle_result
{
    vertex split;
    vertex dv01;
    vertex dv02;
    vertex dv12;
    bool isLeftSideMajor;
};