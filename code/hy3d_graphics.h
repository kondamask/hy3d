#pragma once
#include "hy3d_types.h"

struct pixel_buffer
{
    void *memory;
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

static void PutPixel(pixel_buffer *pixelBuffer, i16 x, i16 y, color c)
{
    // Pixel 32 bits
    // Memory:      BB GG RR xx
    // Register:    xx RR GG BB

    u32 *pixel = (u32 *)pixelBuffer->memory + y * pixelBuffer->width + x;
    bool isInBuffer =
        y >= 0 &&
        y < pixelBuffer->height &&
        x >= 0 &&               // left
        x < pixelBuffer->width; // right
    if (isInBuffer)
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
}

static void PutPixel(pixel_buffer *pixelBuffer, i16 x, i16 y, u32 c)
{
    // Pixel 32 bits
    // Memory:      BB GG RR xx
    // Register:    xx RR GG BB

    u32 *pixel = (u32 *)pixelBuffer->memory + y * pixelBuffer->width + x;
    bool isInBuffer =
        y >= 0 &&
        y < pixelBuffer->height &&
        x >= 0 &&               // left
        x < pixelBuffer->width; // right
    if (isInBuffer)
        *pixel = c;
}