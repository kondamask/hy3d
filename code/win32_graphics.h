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
	void* memory;
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

    void DrawBufferBounds()
    {
        for(int x=0; x<buffer.width; x++)
    	{
    	    PutPixel(x, 0, {0, 255,0});
            PutPixel(x, buffer.height-1, {0, 255,0});
    	}
    	for(int y=0; y<buffer.height; y++)
    	{
            PutPixel(0, y, {0, 255,0});
    	    PutPixel(buffer.width-1, y, {0, 255,0});
    	}
    }

private:
    PixelBuffer buffer;    
};
