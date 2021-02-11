#include "hy3d_engine.h"
#include <intrin.h>

static void PutPixel(pixel_buffer *pixelBuffer, i16 x, i16 y, Color c)
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
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
    }
}

static void DrawLine(pixel_buffer *pixelBuffer, vec3 a, vec3 b, Color c)
{
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    if (dx == 0.0f && dy == 0.0f)
    {
        PutPixel(pixelBuffer, (i16)a.x, (i16)a.y, c);
    }
    else if (fabsf(dy) >= fabsf(dx))
    {
        if (dy < 0.0f)
        {
            vec3 temp = a;
            a = b;
            b = temp;
        }

        f32 m = dx / dy;
        for (f32 x = a.x, y = a.y;
             y < b.y;
             y += 1.0f, x += m)
        {
            PutPixel(pixelBuffer, (i16)x, (i16)y, c);
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

        f32 m = dy / dx;
        for (f32 x = a.x, y = a.y;
             x < b.x;
             x += 1.0f, y += m)
        {
            PutPixel(pixelBuffer, (i16)x, (i16)y, c);
        }
    }
}

static void DrawTriangle(pixel_buffer *pixelBuffer, triangle t, Color c)
{
    // Sort by y: v0 is at the top, v2 at the bottom
    if (t.v0.y < t.v1.y)
        std::swap(t.v0, t.v1);
    if (t.v1.y < t.v2.y)
        std::swap(t.v1, t.v2);
    if (t.v0.y < t.v1.y)
        std::swap(t.v0, t.v1);

    // Sort by x:
    // if v1.y is the same as v0.y, it should be to the right
    // if v1.y is the same as v2.y, it should be to the left
    if (t.v0.y == t.v1.y && t.v0.x > t.v1.x)
        std::swap(t.v0, t.v1);
    else if (t.v1.y == t.v2.y && t.v1.x > t.v2.x)
        std::swap(t.v1, t.v2);

    f32 alphaSplit = (t.v1.y - t.v0.y) / (t.v2.y - t.v0.y);
    vec3 split = t.v0 + (t.v2 - t.v0) * alphaSplit;
    bool isLeftSideMajor = t.v1.x > split.x;

    i16 yTop = (i16)ceilf(t.v0.y - 0.5f);
    i16 ySplit = (i16)ceilf(split.y - 0.5f);
    i16 yBottom = (i16)ceilf(t.v2.y - 0.5f);

    f32 xLeftF, xRightF;
    i16 xLeft, xRight;

    // NOTE:
    // It looks like that multiplication with the negative slope and the negative
    // Dy give more precise results in comparison with the positive slope and Dy.
    f32 slope02 = -(t.v2.x - t.v0.x) / (t.v2.y - t.v0.y);

    // Top Half | Flat Bottom Triangle
    for (i16 y = yTop; y > ySplit; y--)
    {
        f32 slope01 = -(t.v1.x - t.v0.x) / (t.v1.y - t.v0.y);
        if (isLeftSideMajor)
        {
            xLeftF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
            xRightF = slope01 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
        }
        else
        {
            xLeftF = slope01 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
            xRightF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
        }
        xLeft = (i16)ceilf(xLeftF - 0.5f);
        xRight = (i16)ceilf(xRightF - 0.5f);

        for (i16 x = xLeft; x < xRight; x++)
        {
            PutPixel(pixelBuffer, x, y, c);
        }
    }

    //Bottom Half | Flat Top
    for (i16 y = ySplit; y > yBottom; y--)
    {
        f32 slope12 = -(t.v2.x - t.v1.x) / (t.v2.y - t.v1.y);
        if (isLeftSideMajor)
        {
            xLeftF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
            xRightF = slope12 * (t.v1.y - (f32)y + 0.5f) + t.v1.x;
        }
        else
        {
            xLeftF = slope12 * (t.v1.y - (f32)y + 0.5f) + t.v1.x;
            xRightF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
        }
        xLeft = (i16)ceilf(xLeftF - 0.5f);
        xRight = (i16)ceilf(xRightF - 0.5f);
        for (i16 x = xLeft; x < xRight; x++)
        {
            PutPixel(pixelBuffer, x, y, c);
        }
    }
}

#if 0
// TEST:
static void DrawBufferBounds()
{
    Color c{0, 255, 0};
    u32 *pixel = (u32 *)pixelBuffer.memory;
    for (int y = 0; y < pixelBuffer.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + pixelBuffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += pixelBuffer.width;
    }
    pixel = (u32 *)pixelBuffer.memory + 1;
    for (int x = 0; x < pixelBuffer.width - 1; x++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + pixelBuffer.size / pixelBuffer.bytesPerPixel - pixelBuffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel++;
    }
}

static void DrawTest(int x_in, int y_in)
{
    u32 *pixel = (u32 *)pixelBuffer.memory;
    int strechX = pixelBuffer.width / 255;
    int strechY = pixelBuffer.height / 255;
    for (int y = 0; y < pixelBuffer.height; y++)
    {
        for (int x = 0; x < pixelBuffer.width; x++)
        {
            uint8_t r = (uint8_t)((x + x_in) / strechX);
            uint8_t g = (uint8_t)(x / strechX);
            uint8_t b = (uint8_t)((y + y_in) / strechY);
            Color c = Color{r, g, b};
            *pixel++ = (c.r << 16) | (c.g << 8) | (c.b);
        }
    }
}
#endif

#pragma pack(push, 1)
struct bitmap_header
{
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmapOffset;
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bitsPerPixel;
    u32 compression;
    u32 sizeOfBitmap;
    i32 horzResolution;
    i32 vertResolution;
    u32 colorsUsed;
    u32 colorsImportant;

    u32 redMask;
    u32 greenMask;
    u32 blueMask;
};
#pragma pack(pop)

static void LoadBitmap(loaded_bitmap *bmp, debug_read_file *ReadFile, char *filename)
{
    debug_read_file_result file = ReadFile(filename);
    if (file.size != 0 && file.content)
    {
        bitmap_header *header = (bitmap_header *)file.content;
        u8 *address = (u8 *)file.content + header->bitmapOffset;
        u32 *pixels = (u32 *)address;
        bmp->pixels = pixels;
        bmp->height = header->height;
        bmp->width = header->width;

        if (header->compression == 3)
        {
            u32 alphaMask = ~(header->redMask | header->greenMask | header->blueMask);
            u32 redShift, greenShift, blueShift, alphaShift;
            _BitScanForward((unsigned long *)&redShift, header->redMask);
            _BitScanForward((unsigned long *)&greenShift, header->greenMask);
            _BitScanForward((unsigned long *)&blueShift, header->blueMask);
            _BitScanForward((unsigned long *)&alphaShift, alphaMask);
            u32 *dest = pixels;
            for (i32 y = 0; y < header->height; y++)
            {
                for (i32 x = 0; x < header->width; x++)
                {
                    u32 c = *dest;
                    *dest++ = ((((c >> alphaShift) & 0xFF) << 24) |
                               (((c >> redShift) & 0xFF) << 16) |
                               (((c >> greenShift) & 0xFF) << 8) |
                               (((c >> blueShift) & 0xFF) << 0));
                }
            }
        }
    }
}

static void DrawBitmap(loaded_bitmap *bmp, i32 xPos, i32 yPos, pixel_buffer *pixelBuffer)
{
    i32 minX = xPos;
    i32 minY = yPos;
    i32 maxX = xPos + bmp->width;
    i32 maxY = yPos + bmp->height;

    if (minX < 0)
        minX = 0;
    if (minY < 0)
        minY = 0;
    if (maxX > pixelBuffer->width)
        maxX = pixelBuffer->width;
    if (maxY > pixelBuffer->height)
        maxY = pixelBuffer->height;

    u32 *source = (u32 *)bmp->pixels;
    u32 *dest = (u32 *)pixelBuffer->memory + minY * pixelBuffer->width + minX;
    for (i32 y = minY; y < maxY; y++)
    {
        for (i32 x = minX; x < maxX; x++)
        {
            f32 A = (f32)((*source >> 24) & 0xFF) / 255.0f;
            A *= bmp->opacity;
            f32 SR = (f32)((*source >> 16) & 0xFF);
            f32 SG = (f32)((*source >> 8) & 0xFF);
            f32 SB = (f32)((*source >> 0) & 0xFF);

            f32 DR = (f32)((*dest >> 16) & 0xFF);
            f32 DG = (f32)((*dest >> 8) & 0xFF);
            f32 DB = (f32)((*dest >> 0) & 0xFF);

            // TODO(casey): Someday, we need to talk about premultiplied alpha!
            // (this is not premultiplied alpha)
            f32 R = (1.0f - A) * DR + A * SR;
            f32 G = (1.0f - A) * DG + A * SG;
            f32 B = (1.0f - A) * DB + A * SB;

            *dest = (((u32)(R + 0.5f) << 16) |
                     ((u32)(G + 0.5f) << 8) |
                     ((u32)(B + 0.5f) << 0));
            dest++;
            source++;
        }
        dest += pixelBuffer->width - bmp->width;
    }
}

extern "C" UPDATE_AND_RENDER(UpdateAndRender)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<f32> frameTime = frameEnd - e.frameStart;
    f32 dt = frameTime.count();
    e.frameStart = frameEnd;

    engine_state *state = (engine_state *)memory->permanentMemory;

    if (!memory->isInitialized)
    {
        state->cubeOrientation = {0.0f, 0.0f, 0.0f};
        state->cubeZ = 2.0f;
        state->drawLines = false;
        memory->isInitialized = true;

        LoadBitmap(&state->background, memory->DEBUGReadFile, "city_bg_purple.bmp");
        state->background.opacity = 1.7f;
        LoadBitmap(&state->logo, memory->DEBUGReadFile, "hy3d.bmp");
        state->logo.opacity = 1.0f;
    }

    // NOTE:  UPDATE
    // Cube Control
    f32 rotSpeed = 1.5f * dt;
    if (e.input.keyboard.isPressed[UP])
        state->cubeOrientation.thetaX += rotSpeed;
    if (e.input.keyboard.isPressed[DOWN])
        state->cubeOrientation.thetaX -= rotSpeed;
    if (e.input.keyboard.isPressed[LEFT])
        state->cubeOrientation.thetaY += rotSpeed;
    if (e.input.keyboard.isPressed[RIGHT])
        state->cubeOrientation.thetaY -= rotSpeed;
    if (e.input.keyboard.isPressed[Q])
        state->cubeOrientation.thetaZ += rotSpeed;
    if (e.input.keyboard.isPressed[W])
        state->cubeOrientation.thetaZ -= rotSpeed;

    f32 opacity = (f32)e.input.mouse.y / (f32)e.pixelBuffer.height;
    if (opacity > 1.0f)
        opacity = 1.0f;
    if (opacity < 0.0f)
        opacity = 0.0f;
    state->logo.opacity = opacity;
    state->background.opacity = opacity;

    if (e.input.keyboard.isPressed[R])
    {
        state->cubeOrientation.thetaX = 0.0f;
        state->cubeOrientation.thetaY = 0.0f;
        state->cubeOrientation.thetaZ = 0.0f;
    }
    f32 offsetZ = 1.0f * dt;
    if (e.input.keyboard.isPressed[Z])
        state->cubeZ -= offsetZ;
    if (e.input.keyboard.isPressed[X])
        state->cubeZ += offsetZ;
    state->drawLines = e.input.keyboard.isPressed[CTRL];

    // NOTE:  RENDER
    DrawBitmap(&state->background, 0, 0, &e.pixelBuffer);

    state->cubeAxis = MakeAxis3D({-0.5f, -0.5f, -0.5f}, 1.5f, state->cubeOrientation);
    state->cube = MakeCube(1.0f, state->cubeOrientation);

    // Apply Transformations
    mat3 transformation = RotateX(state->cubeOrientation.thetaX) *
                          RotateY(state->cubeOrientation.thetaY) *
                          RotateZ(state->cubeOrientation.thetaZ);
    for (int i = 0; i < state->cubeAxis.nVertices; i++)
    {
        state->cubeAxis.vertices[i] *= transformation;
        state->cubeAxis.vertices[i] += {0.0f, 0.0f, state->cubeZ};
    }
    for (int i = 0; i < state->cube.nVertices; i++)
    {
        state->cube.vertices[i] *= transformation;
        state->cube.vertices[i] += {0.0f, 0.0f, state->cubeZ};
    }

    for (int i = 0; i < state->cube.nTrianglesVertices; i += 3)
    {
        triangle t{
            state->cube.vertices[state->cube.triangles[i]],
            state->cube.vertices[state->cube.triangles[i + 1]],
            state->cube.vertices[state->cube.triangles[i + 2]],
        };
        vec3 normal = CrossProduct(t.v1 - t.v0, t.v2 - t.v0);
        state->cube.isTriangleVisible[i / 3] = normal * t.v0 <= 0;
    }

    // Transform to sceen
    for (int i = 0; i < state->cubeAxis.nVertices; i++)
    {
        e.screenTransformer.Transform(state->cubeAxis.vertices[i]);
    }
    for (int i = 0; i < state->cube.nVertices; i++)
    {
        e.screenTransformer.Transform(state->cube.vertices[i]);
    }

    //Draw Triangles
    for (int i = 0; i < state->cube.nTrianglesVertices; i += 3)
    {
        if (state->cube.isTriangleVisible[i / 3])
        {
            triangle t{
                state->cube.vertices[state->cube.triangles[i]],
                state->cube.vertices[state->cube.triangles[i + 1]],
                state->cube.vertices[state->cube.triangles[i + 2]],
            };
            DrawTriangle(&e.pixelBuffer, t, state->cube.colors[i / 6]);
        }
    }

    //Draw Lines
    if (state->drawLines)
    {
        for (int i = 0; i < state->cube.nLinesVertices; i += 2)
        {
            vec3 a = state->cube.vertices[state->cube.lines[i]];
            vec3 b = state->cube.vertices[state->cube.lines[i + 1]];
            DrawLine(&e.pixelBuffer, a, b, {255, 255, 255});
        }
    }

    for (int i = 0; i < state->cubeAxis.nLinesVertices; i += 2)
    {
        vec3 a = state->cubeAxis.vertices[state->cubeAxis.lines[i]];
        vec3 b = state->cubeAxis.vertices[state->cubeAxis.lines[i + 1]];
        DrawLine(&e.pixelBuffer, a, b, {100, 100, 0});
    }

    // TEST:  Draw bitmap
    DrawBitmap(&state->logo, 464, 464, &e.pixelBuffer);

    state->worldAxis = MakeAxis3D({-3.9f, -3.9f, 2.0f}, 0.3f, {0.0f, 0.0f, 0.0f});
    for (int i = 0; i < state->worldAxis.nVertices; i++)
    {
        state->worldAxis.vertices[i] += {0.0f, 0.0f, state->cubeZ};
    }
    for (int i = 0; i < state->worldAxis.nVertices; i++)
    {
        e.screenTransformer.Transform(state->worldAxis.vertices[i]);
    }
    for (int i = 0; i < state->worldAxis.nLinesVertices; i += 2)
    {
        vec3 a = state->worldAxis.vertices[state->worldAxis.lines[i]];
        vec3 b = state->worldAxis.vertices[state->worldAxis.lines[i + 1]];
        DrawLine(&e.pixelBuffer, a, b, {255, 255, 255});
    }
}
