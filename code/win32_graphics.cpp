#include "win32_graphics.h"

void Graphics::InitializeBackbuffer(int width, int height)
{
    if (buffer.memory)
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

void Graphics::ClearBackbuffer()
{
    if (buffer.memory)
    {
        VirtualFree(buffer.memory, 0, MEM_RELEASE);
    }
    buffer.memory = VirtualAlloc(0, buffer.size, MEM_COMMIT, PAGE_READWRITE);
}

void Graphics::DisplayPixelBuffer(HDC deviceContext)
{
    StretchDIBits(
        deviceContext,
        0, 0, buffer.width, buffer.height,
        0, 0, buffer.width, buffer.height,
        buffer.memory,
        &buffer.info,
        DIB_RGB_COLORS,
        SRCCOPY);
}

void Graphics::DrawBufferBounds()
{
    Color c{0, 255, 0};
    uint32_t *pixel = (uint32_t *)buffer.memory;
    for (int y = 0; y < buffer.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += buffer.width;
    }
    pixel = (uint32_t *)buffer.memory + 1;
    for (int x = 0; x < buffer.width - 1; x++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + buffer.size / buffer.bytesPerPixel - buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel++;
    }
}

void Graphics::DrawTest(int x_in, int y_in)
{
    uint32_t *pixel = (uint32_t *)buffer.memory;
    int strechX = buffer.width / 255;
    int strechY = buffer.height / 255;
    for (int y = 0; y < buffer.height; y++)
    {
        for (int x = 0; x < buffer.width; x++)
        {
            uint8_t r = (uint8_t)((x + x_in) / strechX);
            uint8_t g = (uint8_t)(x / strechX);
            uint8_t b = (uint8_t)((y + y_in) / strechY);
            Color c = Color{r, g, b};
            *pixel++ = (c.r << 16) | (c.g << 8) | (c.b);
        }
    }
}

void Graphics::DrawLine(vec3 a, vec3 b, Color c)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    if (dx == 0.0f && dy == 0.0f)
    {
        PutPixel((int)a.x, (int)a.y, c);
    }
    else if (abs(dy) >= abs(dx))
    {
        if (dy < 0.0f)
        {
            vec3 temp = a;
            a = b;
            b = temp;
        }

        float m = dx / dy;
        for (float x = a.x, y = a.y;
             y < b.y;
             y += 1.0f, x += m)
        {
            PutPixel((int)x, (int)y, c);
        }
    }
    else
    {
        if (dx < 0.0f)
        {
            vec3 temp = a;
            a = b;
            b = temp;
        }

        float m = dy / dx;
        for (float x = a.x, y = a.y;
             x < b.x;
             x += 1.0f, y += m)
        {
            PutPixel((int)x, (int)y, c);
        }
    }
}

void Graphics::PutPixel(int x, int y, Color c)
{
    // Pixel 32 bits
    // Memory:      BB GG RR xx
    // Register:    xx RR GG BB

    uint32_t *pixel = (uint32_t *)buffer.memory + y * buffer.width + x;
    bool isInBuffer =
        y >= 0 &&
        y < buffer.height &&
        x >= 0 &&         // left
        x < buffer.width; // right
    if (isInBuffer)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
    }
}