#pragma once
#include "hy3d_vector.h"
#include "hy3d_matrix.h"
#include "hy3d_objects.h"
#include "hy3d_types.h"
#include <chrono>

// TODO: Make this an actual assetion
#if 1
#define ASSERT(Expression) \
    if (!(Expression))     \
    {                      \
        *(int *)0 = 0;     \
    }
#else
#define ASSERT(Expression)
#endif

#define KILOBYTES(val) (val * 1024LL)
#define MEGABYTES(val) (KILOBYTES(val) * 1024LL)
#define GIGABYTES(val) (MEGABYTES(val) * 1024LL)
#define TERABYTES(val) (GIGABYTES(val) * 1024LL)

struct debug_read_file_result
{
    void *content;
    u32 size;
};

#define DEBUG_READ_FILE(name) debug_read_file_result name(char *filename)
typedef DEBUG_READ_FILE(debug_read_file);

#define DEBUG_WRITE_FILE(name) bool name(char *filename, u32 memorySize, void *memory)
typedef DEBUG_WRITE_FILE(debug_write_file);

#define DEBUG_FREE_FILE(name) void name(void *memory)
typedef DEBUG_FREE_FILE(debug_free_file);

struct engine_memory
{
    bool isInitialized;
    u64 permanentMemorySize;
    void *permanentMemory;
    u64 transientMemorySize;
    void *transientMemory;

    debug_read_file *DEBUGReadFile;
    debug_write_file *DEBUGWriteFile;
    debug_free_file *DEBUGFreeFileMemory;
};

struct pixel_buffer
{
    void *memory;
    i16 width;
    i16 height;
    i8 bytesPerPixel;
    i32 size;
};

struct loaded_bitmap
{
    i32 width;
    i32 height;
    u32 *pixels;
};

struct engine_state
{
    orientation cubeOrientation;
    f32 cubeZ;
    bool drawLines;
    axis3d cubeAxis;
    cube cube;

    loaded_bitmap wall;
    loaded_bitmap peepo;
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

    inline void SetPos(i16 x_, i16 y_)
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

struct hy3d_engine
{
    pixel_buffer pixelBuffer;
    engine_input input;
    space3d space;
    screen_transformer screenTransformer;
    std::chrono::steady_clock::time_point frameStart;

    void Initialize(void *pixelBufferMemory, i16 width, i16 height, i8 bytesPerPixel, i32 bufferSize)
    {
        pixelBuffer = {};
        pixelBuffer.memory = pixelBufferMemory;
        pixelBuffer.width = width;
        pixelBuffer.height = height;
        pixelBuffer.bytesPerPixel = bytesPerPixel;
        pixelBuffer.size = width * height * bytesPerPixel;

        input = {};

        space.left = -1.0f;
        space.right = 1.0f;
        space.top = 1.0f;
        space.bottom = -1.0f;
        space.width = space.right - space.left;
        space.height = space.top - space.bottom;

        screenTransformer.xFactor = width / space.width;
        screenTransformer.yFactor = height / space.height;

        frameStart = std::chrono::steady_clock::now();
    }
};

// IMPORTANT:
// This is the way to create to dynamicaly load your code. In short it
// says that there is a function of type X with THESE parameters and someone
// is goind to call it with a function pointer at some point. So we just need
// that function pointer to load our code whenever we want.
// https://hero.handmade.network/episode/code/day021/

// 1. Make a macro that defines a function with the name we pass it
#define UPDATE_AND_RENDER(name) void name(hy3d_engine &e, engine_memory *memory)
// 2. Create a typedef that says: there is a function of type void (the actual type of the function)
//    I want to replace it with <name>. So now the function is of type update_and_render.
//    I can use this to get a pointer to said function.
typedef UPDATE_AND_RENDER(update_and_render);
// This translates to:
// typedef void update_and_render(hy3d_engine &e, engine_memory *memory);
// 3. Create a stub function that prevents the program from crashing it.
UPDATE_AND_RENDER(UpdateAndRenderStub) {}
