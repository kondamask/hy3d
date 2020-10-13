#include "win32_graphics.h"

void win32_graphics::MakeDIBSection(int width, int height)
{    
    if(bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

	bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;
    int bitmapMemorySize = width* height * bytesPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    
    PutPixel(0, 0, {0, 255,0});
    PutPixel(1, 0, {0,0,255});
}

void win32_graphics::Update(HDC deviceContext, int windoWidth, int windowheight)
{
	StretchDIBits(
		deviceContext,
		0, 0, windoWidth, windowheight,
		0, 0, bitmapWidth, bitmapHeight,
		bitmapMemory,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

void win32_graphics::PutPixel(int x, int y, Color c)
{
    int pitch = bitmapWidth*bytesPerPixel;
    uint8_t *pixel = (uint8_t *)bitmapMemory + y * pitch + x * bytesPerPixel;
    pixel[2] = c.r; // RED
    pixel[1] = c.g; // GREEN
    pixel[0] = c.b; // BLUE
}