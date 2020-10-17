#pragma once
#include "hy3d_windows.h"
#include "hy3d_vector.h"
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

class Graphics
{
public:
    void InitializeBackbuffer(int width, int height);
    void ClearBackbuffer();
    void DisplayPixelBuffer(HDC deviceContext);
    void PutPixel(int x, int y, Color c);

    void DrawBufferBounds();
    void DrawTest(int x, int y);
    void DrawLine(vec3 a, vec3 b, Color c);

    int Width() { return buffer.width; }
    int Height() { return buffer.height; }

private:
    PixelBuffer buffer;
};
