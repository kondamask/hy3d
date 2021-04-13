#pragma once
#include "hy3d_types.h"
#include "hy3d_math.h"
#include "hy3d_vertex.h"
#include <math.h>

struct pixel_buffer
{
    void *memory;
    f32 *zBuffer;
    i16 width;
    i16 height;
    i8 bytesPerPixel;
    i32 size;
};

struct color
{
    u8 r;
    u8 g;
    u8 b;
};

static inline color Vec3ToRGB(vec3 color_in)
{
    color result;
    result.r = RoundF32toI8(color_in.r * 255.0f);
    result.g = RoundF32toI8(color_in.g * 255.0f);
    result.b = RoundF32toI8(color_in.b * 255.0f);
    return result;
}

struct triangle
{
    union
    {
        struct
        {
            vertex v0;
            vertex v1;
            vertex v2;
        };
        vertex v[3];
    };
};

struct triangle_smooth
{
    union
    {
        struct
        {
            vertex_smooth v0;
            vertex_smooth v1;
            vertex_smooth v2;
        };
        vertex_smooth v[3];
    };
};

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

struct loaded_bitmap
{
    i16 width;
    i16 height;
    f32 posX;
    f32 posY;
    f32 opacity;
    u32 *pixels;

    u32 GetColorU32(i32 x, i32 y)
    {
        ASSERT(x >= 0 && x < width && y >= 0 && y < height)
        return pixels[x + y * width];
    }

    color GetColorRGB(i32 x, i32 y)
    {
        ASSERT(x >= 0 && x < width && y >= 0 && y < height)
        u32 c = *(pixels + y * width + x);
        u8 r = (c >> 16) & 0xFF;
        u8 g = (c >> 8) & 0xFF;
        u8 b = (c >> 0) & 0xFF;
        return {r, g, b};
    }
};

struct processed_triangle
{
    vertex split;
    vertex dv01;
    vertex dv02;
    vertex dv12;
    bool isLeftSideMajor;
};

struct processed_smooth_triangle
{
    vertex_smooth split;
    vertex_smooth dv01;
    vertex_smooth dv02;
    vertex_smooth dv12;
    bool isLeftSideMajor;
};

struct vertex_shader_wave
{
    f32 amplitude;
    f32 waveFreq;
    f32 scrollFreq;
    f32 time;

    void Initialize(f32 amplitudeIn, f32 waveFreqIn, f32 scrollFreqIn)
    {
        amplitude = amplitudeIn;
        waveFreq = waveFreqIn;
        scrollFreq = scrollFreqIn;
        time = 0.0f;
    }
};

struct diffuse
{
    vec3 intensity;
    vec3 direction;
};

typedef vec3 ambient;
typedef vec3 material;
