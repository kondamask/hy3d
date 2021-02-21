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

    f32 alpha = (t.v1.y - t.v0.y) / (t.v2.y - t.v0.y);
    vec3 split = lerp(t.v0, t.v1, t.v2, alpha);
    bool isLeftSideMajor = t.v1.x > split.x;

    i16 yTop = RoundDownF32toI16(t.v0.y);
    i16 ySplit = RoundDownF32toI16(split.y);
    i16 yBottom = RoundDownF32toI16(t.v2.y);

    f32 xLeftF, xRightF;
    i16 xLeft, xRight;

    // NOTE:
    // It looks like that multiplication with the negative slope and the negative
    // Dy give more precise results in comparison with the positive slope and Dy.
    f32 slope02 = -(t.v2.x - t.v0.x) / (t.v2.y - t.v0.y);

    // Top Half | Flat Bottom Triangle
    f32 slope01 = -(t.v1.x - t.v0.x) / (t.v1.y - t.v0.y);
    for (i16 y = yTop; y > ySplit; y--)
    {
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
        xLeft = RoundDownF32toI16(xLeftF);
        xRight = RoundDownF32toI16(xRightF);

        for (i16 x = xLeft; x < xRight; x++)
        {
            PutPixel(pixelBuffer, x, y, c);
        }
    }

    //Bottom Half | Flat Top
    f32 slope12 = -(t.v2.x - t.v1.x) / (t.v2.y - t.v1.y);
    for (i16 y = ySplit; y > yBottom; y--)
    {
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

static void DrawTriangleTexture(pixel_buffer *pixelBuffer, textured_triangle t, loaded_bitmap bmp)
{
    // Sort by y: v0 is at the top, v2 at the bottom
    if (t.v0.pos.y < t.v1.pos.y)
        std::swap(t.v0, t.v1);
    if (t.v1.pos.y < t.v2.pos.y)
        std::swap(t.v1, t.v2);
    if (t.v0.pos.y < t.v1.pos.y)
        std::swap(t.v0, t.v1);

    // Sort by x:
    // if v1.pos.y is the same as v0.pos.y, v1 should be to the right
    // if v1.pos.y is the same as v2.pos.y, v1 it should be to the left
    if (t.v0.pos.y == t.v1.pos.y && t.v0.pos.x > t.v1.pos.x)
        std::swap(t.v0, t.v1);
    else if (t.v1.pos.y == t.v2.pos.y && t.v1.pos.x > t.v2.pos.x)
        std::swap(t.v1, t.v2);

    // Find point where triangle is split in 2 parts: Flat Top and Flat Bottom
    texel split = t.v0.interpolateTo(t.v1, t.v2);
    i16 yTop = RoundDownF32toI16(t.v0.pos.y);
    i16 ySplit = RoundDownF32toI16(split.pos.y);
    i16 yBottom = RoundDownF32toI16(t.v2.pos.y);

    // Check if hypotinus is on the left
    bool isLeftSideMajor = t.v1.pos.x > split.pos.x;

    // Calculate Slopes
    // NOTE:
    // It looks like that multiplication with the negative slope and the negative
    // Dy give more precise results in comparison with the positive slope and Dy.
    f32 slope01 = -(t.v1.pos.x - t.v0.pos.x) / (t.v1.pos.y - t.v0.pos.y);
    f32 slope02 = -(t.v2.pos.x - t.v0.pos.x) / (t.v2.pos.y - t.v0.pos.y);
    f32 slope12 = -(t.v2.pos.x - t.v1.pos.x) / (t.v2.pos.y - t.v1.pos.y);

    f32 xLeftF, xRightF = 0.0f;
    i16 xLeft, xRight;

    vec2 texLeft = {};
    vec2 texRight = {};
    vec2 texStep01 = (t.v1.coord - t.v0.coord) / (t.v1.pos.y - t.v0.pos.y);
    vec2 texStep02 = (t.v2.coord - t.v0.coord) / (t.v2.pos.y - t.v0.pos.y);
    vec2 texStep12 = (t.v2.coord - t.v1.coord) / (t.v2.pos.y - t.v1.pos.y);
    vec2 texScanStep = {};
    vec2 texCoord = {};

    // Top Half | Flat Bottom Triangle
    texLeft = t.v0.coord;
    texRight = t.v0.coord;
    if (isLeftSideMajor)
    {
        for (i16 y = yTop; y > ySplit; y--)
        {
            xLeftF = slope02 * (t.v0.pos.y - (f32)y + 0.5f) + t.v0.pos.x;
            xRightF = slope01 * (t.v0.pos.y - (f32)y + 0.5f) + t.v0.pos.x;
            xLeft = RoundDownF32toI16(xLeftF);
            xRight = RoundDownF32toI16(xRightF);
            texScanStep = (texRight - texLeft) / (xRightF - xLeftF);
            texCoord = texLeft + texScanStep * ((f32)xLeft + 0.5f - xLeftF);
            for (i16 x = xLeft; x < xRight; x++, texCoord += texScanStep)
            {
                Color c = bmp.GetColor((i32)(texCoord.x * bmp.width), (i32)(texCoord.y * bmp.height));
                PutPixel(pixelBuffer, x, y, c);
            }
            texLeft -= texStep02;
            texRight -= texStep01;
        }
    }
    else
    {
        for (i16 y = yTop; y > ySplit; y--)
        {
            xLeftF = slope01 * (t.v0.pos.y - (f32)y + 0.5f) + t.v0.pos.x;
            xRightF = slope02 * (t.v0.pos.y - (f32)y + 0.5f) + t.v0.pos.x;
            xLeft = RoundDownF32toI16(xLeftF);
            xRight = RoundDownF32toI16(xRightF);
            texScanStep = (texRight - texLeft) / (xRightF - xLeftF);
            texCoord = texLeft + texScanStep * ((f32)xLeft + 0.5f - xLeftF);

            for (i16 x = xLeft; x < xRight; x++, texCoord += texScanStep)
            {
                Color c = bmp.GetColor((i32)(texCoord.x * bmp.width), (i32)(texCoord.y * bmp.height));
                PutPixel(pixelBuffer, x, y, c);
            }
            texLeft -= texStep01;
            texRight -= texStep02;
        }
    }

    //Bottom Half | Flat Top
    if (isLeftSideMajor)
    {
        texLeft = split.coord;
        texRight = t.v1.coord;
        for (i16 y = ySplit; y > yBottom; y--)
        {
            xLeftF = slope02 * (t.v0.pos.y - (f32)y + 0.5f) + t.v0.pos.x;
            xRightF = slope12 * (t.v1.pos.y - (f32)y + 0.5f) + t.v1.pos.x;
            xLeft = RoundDownF32toI16(xLeftF);
            xRight = RoundDownF32toI16(xRightF);
            texScanStep = (texRight - texLeft) / (xRightF - xLeftF);
            texCoord = texLeft + texScanStep * ((f32)xLeft + 0.5f - xLeftF);
            for (i16 x = xLeft; x < xRight; x++, texCoord += texScanStep)
            {
                Color c = bmp.GetColor((i32)(texCoord.x * bmp.width), (i32)(texCoord.y * bmp.height));
                PutPixel(pixelBuffer, x, y, c);
            }
            texLeft -= texStep02;
            texRight -= texStep12;
        }
    }
    else
    {
        texRight = split.coord;
        texLeft = t.v1.coord;
        for (i16 y = ySplit; y > yBottom; y--)
        {
            xLeftF = slope12 * (t.v1.pos.y - (f32)y + 0.5f) + t.v1.pos.x;
            xRightF = slope02 * (t.v0.pos.y - (f32)y + 0.5f) + t.v0.pos.x;
            xLeft = RoundDownF32toI16(xLeftF);
            xRight = RoundDownF32toI16(xRightF);
            texScanStep = (texRight - texLeft) / (xRightF - xLeftF);
            texCoord = texLeft + texScanStep * ((f32)xLeft + 0.5f - xLeftF);
            for (i16 x = xLeft; x < xRight; x++, texCoord += texScanStep)
            {
                Color c = bmp.GetColor((i32)(texCoord.x * bmp.width), (i32)(texCoord.y * bmp.height));
                PutPixel(pixelBuffer, x, y, c);
            }
            texLeft -= texStep12;
            texRight -= texStep02;
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
        bmp->height = (i16)header->height;
        bmp->width = (i16)header->width;

        if (header->compression == 3)
        {
            u32 alphaMask = ~(header->redMask | header->greenMask | header->blueMask);
            u32 redShift, greenShift, blueShift, alphaShift;
            _BitScanForward((unsigned long *)&redShift, header->redMask);
            _BitScanForward((unsigned long *)&greenShift, header->greenMask);
            _BitScanForward((unsigned long *)&blueShift, header->blueMask);
            _BitScanForward((unsigned long *)&alphaShift, alphaMask);
            if (alphaShift == 24 && redShift == 16 && greenShift == 8 && blueShift == 0)
                return;
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
    i32 clipLeft = 0;
    i32 clipTop = 0;
    i32 clipRight = 0;
    i32 clipBottom = 0;

    if (minX < 0)
    {
        clipLeft -= minX;
        minX = 0;
    }
    if (minY < 0)
    {
        clipBottom -= minY;
        minY = 0;
    }
    if (maxX > pixelBuffer->width)
    {
        clipRight = maxX - pixelBuffer->width;
        maxX = pixelBuffer->width;
    }
    if (maxY > pixelBuffer->height)
    {
        clipTop = maxY - pixelBuffer->height;
        maxY = pixelBuffer->height;
    }
    u32 *source = (u32 *)bmp->pixels + clipLeft + clipBottom * bmp->width;
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
        dest += pixelBuffer->width - bmp->width + clipLeft + clipRight; // - bmp->width + maxXsource;
        source += clipLeft + clipRight;
    }
}

static void Initialize(hy3d_engine *e, engine_state *state, engine_memory *memory)
{
    e->input = {};

    e->space.left = -1.0f;
    e->space.right = 1.0f;
    e->space.top = 1.0f;
    e->space.bottom = -1.0f;
    e->space.width = e->space.right - e->space.left;
    e->space.height = e->space.top - e->space.bottom;
    e->screenTransformer.xFactor = e->pixelBuffer.width / e->space.width;
    e->screenTransformer.yFactor = e->pixelBuffer.width / e->space.height;

    state->cube.orientation = {0.0f, 0.0f, 0.0f};
    state->cubeZ = 2.0f;
    state->drawCubeOutline = false;

    LoadBitmap(&state->background, memory->DEBUGReadFile, "city_bg_purple.bmp");
    state->background.opacity = 1.0f;
    LoadBitmap(&state->logo, memory->DEBUGReadFile, "hy3d_gimp.bmp");
    state->logo.opacity = 0.3f;
    state->logoVelX = 100.0f;
    state->logoVelY = 80.0f;
    LoadBitmap(&state->texture, memory->DEBUGReadFile, "crate.bmp");
    state->texture.opacity = 1.0f;

    memory->isInitialized = true;
    e->frameStart = std::chrono::steady_clock::now();
}

static void Update(hy3d_engine *e, engine_state *state)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<f32> frameTime = frameEnd - e->frameStart;
    f32 dt = frameTime.count();
    e->frameStart = frameEnd;

    // Cube Control
    f32 speed = 1.5f * dt;
    if (e->input.keyboard.isPressed[UP])
    {
        state->cube.orientation.thetaX += speed;
    }
    if (e->input.keyboard.isPressed[DOWN])
    {
        state->cube.orientation.thetaX -= speed;
    }
    if (e->input.keyboard.isPressed[LEFT])
    {
        state->cube.orientation.thetaY += speed;
    }
    if (e->input.keyboard.isPressed[RIGHT])
    {
        state->cube.orientation.thetaY -= speed;
    }
    if (e->input.keyboard.isPressed[Q])
        state->cube.orientation.thetaZ += speed;
    if (e->input.keyboard.isPressed[W])
        state->cube.orientation.thetaZ -= speed;

    // Logo Control
    if (state->logo.posX < 0 || (state->logo.posX + state->logo.width) > e->pixelBuffer.width)
        state->logoVelX = -state->logoVelX;
    if (state->logo.posY < 0 || (state->logo.posY + state->logo.height) > e->pixelBuffer.height)
        state->logoVelY = -state->logoVelY;
    state->logo.posX += state->logoVelX * dt;
    state->logo.posY += state->logoVelY * dt;

    state->logo.opacity += (f32)e->input.mouse.WheelDelta() * 0.0005f;
    if (state->logo.opacity > 1.0f)
        state->logo.opacity = 1.0f;
    if (state->logo.opacity < 0.0f)
        state->logo.opacity = 0.0f;

    if (e->input.keyboard.isPressed[R])
    {
        state->cube.orientation.thetaX = 0.0f;
        state->cube.orientation.thetaY = 0.0f;
        state->cube.orientation.thetaZ = 0.0f;
    }
    f32 offsetZ = 1.0f * dt;
    if (e->input.keyboard.isPressed[Z])
        state->cubeZ -= offsetZ;
    if (e->input.keyboard.isPressed[X])
        state->cubeZ += offsetZ;
    state->drawCubeOutline = e->input.keyboard.isPressed[CTRL];
}

static void Render(hy3d_engine *e, engine_state *state)
{
    DrawBitmap(&state->background, 0, 0, &e->pixelBuffer);
    DrawBitmap(&state->logo, (i32)(state->logo.posX), (i32)(state->logo.posY), &e->pixelBuffer);

    state->cubeAxis = MakeAxis3D({-0.0f, -0.0f, -0.0f}, 1.0f, state->cube.orientation);
    state->cube = MakeCube(1.0f, state->cube.orientation);

    // NOTE:  Apply Transformations
    mat3 transformation = RotateX(state->cube.orientation.thetaX) *
                          RotateY(state->cube.orientation.thetaY) *
                          RotateZ(state->cube.orientation.thetaZ);
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

    // NOTE:  Find Visible Triangles
    for (int i = 0; i < state->cube.nTrianglesVertices; i += 3)
    {
        triangle t{
            state->cube.vertices[state->cube.triangles[i]],
            state->cube.vertices[state->cube.triangles[i + 1]],
            state->cube.vertices[state->cube.triangles[i + 2]],
        };
        vec3 normal = CrossProduct(t.v1 - t.v0, t.v2 - t.v0);
        state->cube.isTriangleVisible[i / 3] = (normal * t.v0 <= 0);
    }

    // NOTE:  Transform to sceen
    for (int i = 0; i < state->cubeAxis.nVertices; i++)
        e->screenTransformer.Transform(state->cubeAxis.vertices[i]);
    for (int i = 0; i < state->cube.nVertices; i++)
        e->screenTransformer.Transform(state->cube.vertices[i]);

    // NOTE:  Draw
    for (int i = 0; i < state->cube.nTrianglesVertices; i += 3)
    {
        if (state->cube.isTriangleVisible[i / 3])
        {
            textured_triangle t{
                {state->cube.vertices[state->cube.triangles[i]], state->cube.texCoord[state->cube.triangles[i]]},
                {state->cube.vertices[state->cube.triangles[i + 1]], state->cube.texCoord[state->cube.triangles[i + 1]]},
                {state->cube.vertices[state->cube.triangles[i + 2]], state->cube.texCoord[state->cube.triangles[i + 2]]}};
            DrawTriangleTexture(&e->pixelBuffer, t, state->texture);
        }
    }
    if (state->drawCubeOutline)
    {
        for (int i = 0; i < state->cube.nLinesVertices; i += 2)
        {
            vec3 a = state->cube.vertices[state->cube.lines[i]];
            vec3 b = state->cube.vertices[state->cube.lines[i + 1]];
            DrawLine(&e->pixelBuffer, a, b, {255, 255, 255});
        }
    }
    for (int i = 0; i < state->cubeAxis.nLinesVertices; i += 2)
    {
        vec3 a = state->cubeAxis.vertices[state->cubeAxis.lines[i]];
        vec3 b = state->cubeAxis.vertices[state->cubeAxis.lines[i + 1]];
        DrawLine(&e->pixelBuffer, a, b, state->cubeAxis.colors[i / 2]);
    }
}

extern "C" UPDATE_AND_RENDER(UpdateAndRender)
{
    engine_state *state = (engine_state *)memory->permanentMemory;

    if (!memory->isInitialized)
        Initialize(&e, state, memory);

    Update(&e, state);
    Render(&e, state);
}
