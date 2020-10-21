#pragma once
#include "win32_platform.h"
#include "hy3d_vector.h"
#include "hy3d_matrix.h"
#include "hy3d_objects.h"
#include <chrono>

// NOTE:
// The HY3D space is a 3d space where
// Y IS UP
// X IS RIGHT
// Z IS INTO THE SCREEN
// The origin (0,0,0) is in the center of the screen.
// We normalize the coordinates so that the far right, left, top and down
// take values -1.0 and +1.0

// TODO: add Z later
struct hy3d_space
{
    float left;
    float right;
    float top;
    float bottom;
    float width;
    float height;
};

struct hy3d_screen_transformer
{
    float xFactor, yFactor;

    vec3 GetTransformed(vec3 v)
    {
        v.x = (v.x + 1.0f) * xFactor;
        v.y = (v.y + 1.0f) * yFactor;
        return v;
    }

    void Transform(vec3 &v)
    {
        v = GetTransformed(v);
    }
};

struct hy3d_engine
{
    Window window;
    hy3d_space space;
    hy3d_screen_transformer screenTransformer;
    std::chrono::steady_clock::time_point frameStart;
};

static void Run(hy3d_engine &e);
