#include "hy3d_renderer.h"

static inline void TransformVertexToScreen(screen_transformer *st, vec3 *v)
{
    f32 zInv = 1.0f / v->z;
    v->x = (v->x * zInv + 1.0f) * st->xFactor;
    v->y = (v->y * zInv + 1.0f) * st->yFactor;
}

static vertex Prestep(i32 rounded, f32 original, vertex step)
{
    return (step * (original - (f32)rounded + 0.5f));
}

static inline u32 GetTextureColorU32(loaded_bitmap *bmp, vec2 coord)
{
    f32 x = coord.x * bmp->width;
    f32 y = coord.y * bmp->height;
    f32 xMax = (f32)bmp->width - 1.0f;
    f32 yMax = (f32)bmp->height - 1.0f;
    if (x < 0.0f)
        x = 0.0f;
    else if (xMax < x)
        x = bmp->width - 1.0f;
    if (y < 0.0f)
        y = 0.0f;
    else if (yMax < y)
        y = bmp->height - 1.0f;

    return bmp->GetColorU32((i32)x, (i32)y);
}

static inline u32 GetTextureWrapColorU32(loaded_bitmap *bmp, vec2 coord)
{
    f32 x = coord.x * (f32)bmp->width;
    f32 y = coord.y * (f32)bmp->height;
    f32 xMax = (f32)bmp->width - 1.0f;
    f32 yMax = (f32)bmp->height - 1.0f;
    x = fmodf(x, xMax);
    y = fmodf(y, yMax);
    if (x < 0.0f)
        x += xMax;
    if (y < 0.0f)
        y += yMax;

    return bmp->GetColorU32((i32)x, (i32)y);
}

static processed_triangle_result ProcessTriangle(triangle *t)
{
    processed_triangle_result result = {};

    // Sort by y: v0 is at the top, v2 at the bottom
    if (t->v0.pos.y < t->v1.pos.y)
        std::swap(t->v0, t->v1);
    if (t->v1.pos.y < t->v2.pos.y)
        std::swap(t->v1, t->v2);
    if (t->v0.pos.y < t->v1.pos.y)
        std::swap(t->v0, t->v1);

    // Sort by x:
    // if v1.pos.y is the same as v0.pos.y, v1 should be to the right
    // if v1.pos.y is the same as v2.pos.y, v1 it should be to the left
    if (t->v0.pos.y == t->v1.pos.y && t->v0.pos.x > t->v1.pos.x)
        std::swap(t->v0, t->v1);
    else if (t->v1.pos.y == t->v2.pos.y && t->v1.pos.x > t->v2.pos.x)
        std::swap(t->v1, t->v2);

    // Find point where triangle is split in 2 parts: Flat Top and Flat Bottom
    // same y as v1 but on the opposite side of the triangle
    result.split = t->v0.interpolateTo(t->v1, t->v2);

    // Check if hypotinus is on the left
    result.isLeftSideMajor = t->v1.pos.x > result.split.pos.x;

    // Calculate Slopes and Texture Lookup steps
    result.dv01 = (t->v1 - t->v0) / (t->v1.pos.y - t->v0.pos.y);
    result.dv02 = (t->v2 - t->v0) / (t->v2.pos.y - t->v0.pos.y);
    result.dv12 = (t->v2 - t->v1) / (t->v2.pos.y - t->v1.pos.y);

    return result;
}

static void DrawLine(pixel_buffer *pixelBuffer, vec3 a, vec3 b, color c)
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

static void DrawFlatTriangle(
    pixel_buffer *pixelBuffer, color c,
    vertex leftStart, vertex rightStart,
    vertex dvLeft, vertex dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex left = Prestep(yTop, yTopF32, -dvLeft) + leftStart;
    vertex right = Prestep(yTop, yTopF32, -dvRight) + rightStart;

    for (i16 y = yTop; y > yBottom; y--)
    {
        xLeft = RoundF32toI16(left.pos.x);
        xRight = RoundF32toI16(right.pos.x);
        for (i16 x = xLeft; x < xRight; x++)
            PutPixel(pixelBuffer, x, y, c);
        left -= dvLeft;
        right -= dvRight;
    }
}

static void DrawTriangleSolidColor(pixel_buffer *pixelBuffer, triangle t, color c)
{
    processed_triangle_result p = ProcessTriangle(&t);

    // Top Half | Flat Bottom Triangle
    if (p.isLeftSideMajor)
        DrawFlatTriangle(pixelBuffer, c, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTriangle(pixelBuffer, c, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTriangle(pixelBuffer, c, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTriangle(pixelBuffer, c, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
}

static void DrawFlatTriangleTextured(
    pixel_buffer *pixelBuffer, loaded_bitmap *bmp,
    vertex leftStart, vertex rightStart,
    vertex dvLeft, vertex dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex texCoord;
    vertex left = leftStart + Prestep(yTop, yTopF32, -dvLeft);
    vertex right = rightStart + Prestep(yTop, yTopF32, -dvRight);
    vertex leftToRightStep;
    for (i16 y = yTop; y > yBottom; y--)
    {
        xLeft = RoundF32toI16(left.pos.x);
        xRight = RoundF32toI16(right.pos.x);
        leftToRightStep = (right - left) / (right.pos.x - left.pos.x);
        texCoord = left + Prestep(xLeft, left.pos.x, leftToRightStep);

        for (i16 x = xLeft; x < xRight; x++, texCoord += leftToRightStep)
        {
            u32 c = GetTextureColorU32(bmp, texCoord.uv);
            PutPixel(pixelBuffer, x, y, c);
        }
        left -= dvLeft;
        right -= dvRight;
    }
}

static void DrawTriangleTextured(pixel_buffer *pixelBuffer, triangle t, loaded_bitmap *bmp)
{
    processed_triangle_result p = ProcessTriangle(&t);

    // Top Half | Flat Bottom Triangle
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextured(pixelBuffer, bmp, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTriangleTextured(pixelBuffer, bmp, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextured(pixelBuffer, bmp, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTriangleTextured(pixelBuffer, bmp, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
}

static void DrawFlatTriangleTextureWrap(
    pixel_buffer *pixelBuffer, loaded_bitmap *bmp,
    vertex leftStart, vertex rightStart,
    vertex dvLeft, vertex dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex texCoord;
    vertex left = leftStart + Prestep(yTop, yTopF32, -dvLeft);
    vertex right = rightStart + Prestep(yTop, yTopF32, -dvRight);
    vertex leftToRightStep;
    for (i16 y = yTop; y > yBottom; y--)
    {
        xLeft = RoundF32toI16(left.pos.x);
        xRight = RoundF32toI16(right.pos.x);
        leftToRightStep = (right - left) / (right.pos.x - left.pos.x);
        texCoord = left + Prestep(xLeft, left.pos.x, leftToRightStep);

        for (i16 x = xLeft; x < xRight; x++, texCoord += leftToRightStep)
        {
            u32 c = GetTextureWrapColorU32(bmp, texCoord.uv);
            PutPixel(pixelBuffer, x, y, c);
        }
        left -= dvLeft;
        right -= dvRight;
    }
}

static void DrawTriangleTextureWrap(pixel_buffer *pixelBuffer, triangle t, loaded_bitmap *bmp)
{
    processed_triangle_result p = ProcessTriangle(&t);
    // Top Half | Flat Bottom Triangle
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextured(pixelBuffer, bmp, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTriangleTextured(pixelBuffer, bmp, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextureWrap(pixelBuffer, bmp, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTriangleTextureWrap(pixelBuffer, bmp, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
}

static void DrawObjectTextured(
    vertex *vertices, i32 nVertices, i8 *indices, i32 nIndices,
    mat3 rotation, vec3 translation, loaded_bitmap *bmp,
    pixel_buffer *pixelBuffer, screen_transformer *st)
{
    // Apply Transformations
    for (i32 i = 0; i < nVertices; i++)
        vertices[i].pos = vertices[i].pos * rotation + translation;

    // Find and Draw Visible Triangles
    for (i32 i = 0; i < nIndices; i += 3)
    {
        triangle t = {vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]};
        bool isVisible = (CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos) * t.v0.pos) <= 0;
        if (isVisible)
        {
            TransformVertexToScreen(st, &t.v0.pos);
            TransformVertexToScreen(st, &t.v1.pos);
            TransformVertexToScreen(st, &t.v2.pos);

            DrawTriangleTextured(pixelBuffer, t, bmp);
        }
    }
}

static void DrawObjectOutline(
    vertex *vertices, i32 nVertices, i8 *lines, i8 nLineIndices, color c,
    mat3 rotation, vec3 translation, pixel_buffer *pixelBuffer, screen_transformer *st)
{
    // Apply Transformations
    for (i32 i = 0; i < nVertices; i++)
    {
        TransformVertexToScreen(st, &vertices[i].pos);
    }

    // Find and Draw Visible Triangles
    for (i32 i = 0; i < nLineIndices; i += 2)
    {
        vec3 a = vertices[lines[i]].pos;
        vec3 b = vertices[lines[i + 1]].pos;
        DrawLine(pixelBuffer, a, b, c);
    }
}

static void DrawAxis3D(
    vec3 *vertices, i32 nVertices, i8 *lines, i8 nLineIndices, color *colors,
    mat3 rotation, vec3 translation, pixel_buffer *pixelBuffer, screen_transformer *st)
{
    // Apply Transformations
    for (i32 i = 0; i < nVertices; i++)
    {
        vertices[i] = vertices[i] * rotation + translation;
        TransformVertexToScreen(st, &vertices[i]);
    }

    // Find and Draw Visible Triangles
    for (i32 i = 0; i < nLineIndices; i += 2)
    {
        vec3 a = vertices[lines[i]];
        vec3 b = vertices[lines[i + 1]];
        DrawLine(pixelBuffer, a, b, colors[i / 2]);
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
