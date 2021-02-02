#pragma once
#include "hy3d_vector.h"
#include "hy3d_matrix.h"
#include "hy3d_objects.h"
#include "hy3d_types.h"
#include <chrono>

#define ASSERT(expression)

#define KILOBYTES(val) (val * 1024LL)
#define MEGABYTES(val) (KILOBYTES(val) * 1024LL)
#define GIGABYTES(val) (MEGABYTES(val) * 1024LL)
#define TERABYTES(val) (GIGABYTES(val) * 1024LL)

struct engine_memory
{
    bool isInitialized;
    u64 permanentMemorySize;
    void *permanentMemory;
    u64 transientMemorySize;
    void *transientMemory;
};

struct read_file_result
{
    void *content;
    u32 size;
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
struct hy3d_space
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;
    f32 width;
    f32 height;
};

struct triangle
{
    vec3 v0, v1, v2;
};

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u16 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
};
#pragma pack(pop)

struct hy3d_screen_transformer
{
    f32 xFactor, yFactor;

    vec3 GetTransformed(vec3 v)
    {
        f32 zInv = 1 / v.z;
        v.x = (v.x * zInv + 1.0f) * xFactor;
        v.y = (v.y * zInv + 1.0f) * yFactor;
        return v;
    }

    void Transform(vec3 &v)
    {
        v = GetTransformed(v);
    }
};

struct pixel_buffer
{
    void *memory;
    i16 width;
    i16 height;
    i8 bytesPerPixel;
    i32 size;
};

enum KEYBOARD_BUTTON
{
    UP,
    LEFT,
    DOWN,
    RIGHT,
    W,
    A,
    S,
    D,
    Q,
    E,
    R,
    F,
    Z,
    X,
    C,
    V,
    SHIFT,
    CTRL,
    ALT,
    F4,
    COUNT,
    INVALID
};

struct keyboard
{
    // TODO: USE A SINGLE VARIABLE INSTEAD OF A BOOL ARRAY
    // WE ONLY NEED 1 BIT FOR A KEY
    bool autoRepeatEnabled = false;
    bool isPressed[KEYBOARD_BUTTON::COUNT];

    inline void Clear()
    {
        for (int i = 0; i < KEYBOARD_BUTTON::COUNT; i++)
            isPressed[i] = false;
    }

    inline void ToggleKey(KEYBOARD_BUTTON key)
    {
        isPressed[key] = !isPressed[key];
    }
};

struct mouse
{
    i16 x;
    i16 y;
    f32 wheelDelta;
    bool isInWindow;
    bool leftIsPressed;
    bool rightIsPressed;

    void SetPos(i16 x_, i16 y_)
    {
        x = x_;
        y = y_;
    }
};

struct engine_input
{
    mouse mouse;
    keyboard keyboard;
};

struct engine_state
{
    orientation cubeOrientation;
    f32 cubeZ;
    bool drawLines;
    axis3d cubeAxis;
    cube cube;
};

struct hy3d_engine
{
    pixel_buffer pixel_buffer;
    engine_input input;
    hy3d_space space;
    hy3d_screen_transformer screenTransformer;
    std::chrono::steady_clock::time_point frameStart;
};
