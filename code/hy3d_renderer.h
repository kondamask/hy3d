#pragma once
#include "win32_platform.h"
#include "hy3d_vector.h"

struct triangle 
{
    vec3 v0, v1, v2;
};

static void DrawLine(win32_graphics &graphics, vec3 a, vec3 b, Color c);
static void DrawTriangle(win32_graphics &graphics, triangle t, Color c);