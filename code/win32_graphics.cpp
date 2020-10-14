#include "win32_graphics.h"

void win32_graphics::InitializeBackbuffer(int width, int height)
{    
    if(buffer.memory)
    {
        VirtualFree(buffer.memory, 0, MEM_RELEASE);
    }

    buffer.width = width;
    buffer.height = height;
    buffer.bytesPerPixel = 4;

	buffer.info = {};
	buffer.info.bmiHeader.biSize = sizeof(buffer.info.bmiHeader);
	buffer.info.bmiHeader.biWidth = width;
	buffer.info.bmiHeader.biHeight = height; // bottom up y. "-height" fot top down y
	buffer.info.bmiHeader.biPlanes = 1;
	buffer.info.bmiHeader.biBitCount = 32;
	buffer.info.bmiHeader.biCompression = BI_RGB;

    buffer.size = buffer.width * buffer.height * buffer.bytesPerPixel;
    buffer.memory = VirtualAlloc(0, buffer.size, MEM_COMMIT, PAGE_READWRITE);
}

void win32_graphics::ClearBackbuffer()
{
    if(buffer.memory)
    {
        VirtualFree(buffer.memory, 0, MEM_RELEASE);
    }
    buffer.memory = VirtualAlloc(0, buffer.size, MEM_COMMIT, PAGE_READWRITE);
}

void win32_graphics::DisplayPixelBuffer(HDC deviceContext)
{
	StretchDIBits(
		deviceContext,
		0, 0, buffer.width, buffer.height,
		0, 0, buffer.width, buffer.height,
		buffer.memory,
        &buffer.info,
        DIB_RGB_COLORS,
        SRCCOPY);
    ClearBackbuffer();
}

void win32_graphics::DrawBufferBounds()
{
    Color c{0,255,0};
    uint32_t *pixel = (uint32_t*)buffer.memory;
    for(int y=0; y<buffer.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel+buffer.width-1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += buffer.width;
    }
	pixel = (uint32_t*)buffer.memory + 1;
	for (int x = 0; x < buffer.width - 1; x++)
	{
		*pixel = (c.r << 16) | (c.g << 8) | (c.b);
		*(pixel + buffer.size/buffer.bytesPerPixel - buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
		pixel++;
	}
}

void win32_graphics::PutPixel(int x, int y, Color c)
{
    // Pixel 32 bits
    // Memory:      BB GG RR xx
    // Register:    xx RR GG BB   

	uint32_t *pixel = (uint32_t *)buffer.memory + y * buffer.width + x;
    bool isInBuffer = 
        y >= 0 &&
        y < buffer.height &&
        x >= 0 && // left
        x < buffer.width; // right
    if(isInBuffer)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
    }
}