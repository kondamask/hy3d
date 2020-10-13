#pragma once
#include "hy3d_windows.h"
#include <stdint.h>

struct Color
{
    uint8_t r, g, b;
};

class win32_graphics
{
public:
    void MakeDIBSection(int width, int height);
	void Update(HDC deviceContext, int windoWidth, int windowheight);
    void PutPixel(int x, int y, Color c);

private:
    BITMAPINFO bitmapInfo;
	void* bitmapMemory;
    int bitmapWidth;
    int bitmapHeight;
    int bytesPerPixel = 4;
};
