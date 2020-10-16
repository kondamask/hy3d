#pragma once
#include "hy3d_windows.h"
#include <stdint.h>

struct Color
{
    uint8_t r, g, b;
};

struct PixelBuffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int bytesPerPixel;
    int size;
};

class win32_graphics
{
public:
    void InitializeBackbuffer(int width, int height);
    void ClearBackbuffer();
    void DisplayPixelBuffer(HDC deviceContext);
    void PutPixel(int x, int y, Color c);

    void DrawBufferBounds();
    void DrawTest(int x, int y);

private:
    PixelBuffer buffer;
};
