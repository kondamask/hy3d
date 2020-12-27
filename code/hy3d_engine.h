#pragma once
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

struct triangle
{
    vec3 v0, v1, v2;
};

struct hy3d_screen_transformer
{
    float xFactor, yFactor;

    vec3 GetTransformed(vec3 v)
    {
        float zInv = 1 / v.z;
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
    int width;
    int height;
    int bytesPerPixel;
    int size;
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
    COUNT
};

struct keyboard
{
    bool autoRepeatEnabled = false;
    bool isPressed[KEYBOARD_BUTTON::COUNT];

    void Clear()
    {
        for (int i = 0; i < KEYBOARD_BUTTON::COUNT; i++)
            isPressed[i] = false;
    }

    void ToggleKey(KEYBOARD_BUTTON key)
    {
        isPressed[key] = !isPressed[key];
    }
};

struct mouse
{
    int x;
    int y;
    bool isInWindow;
    bool leftIsPressed;
    bool rightIsPressed;
    float wheelDelta;

    void SetPos(int x_, int y_)
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
    // TEST:
    orientation cubeOrientation{0.0f, 0.0f, 0.0f};
    float cubeZ = 2.0f;
    bool drawLines = true;
};

struct hy3d_engine
{
    pixel_buffer pixel_buffer;
    engine_state state;
    engine_input input;
    hy3d_space space;
    hy3d_screen_transformer screenTransformer;
    std::chrono::steady_clock::time_point frameStart;
};
