#include "hy3d_renderer.h"
#include "stdlib.h"

static void PutPixel(pixel_buffer *pixelBuffer, i16 x, i16 y, color c)
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
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
}

static void PutPixel(pixel_buffer *pixelBuffer, i16 x, i16 y, u32 c)
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
        *pixel = c;
}

static inline void ClearZBuffer(pixel_buffer *pixelBuffer)
{
    i32 zBufferSize = pixelBuffer->size / pixelBuffer->bytesPerPixel;
    for (i32 i = 0; i < zBufferSize; i++)
        pixelBuffer->zBuffer[i] = FLT_MAX;
}

static bool UpdateZBuffer(pixel_buffer *pixelBuffer, i32 x, i32 y, f32 value)
{
    if (x >= 0 && x < pixelBuffer->width && y >= 0 && y < pixelBuffer->height)
    {
        f32 *old = &pixelBuffer->zBuffer[x + y * pixelBuffer->width];
        if (*old > value)
        {
            *old = value;
            return true;
        }
    }
    return false;
}

static inline void TransformVertexToScreen(screen_transformer *st, vertex *v)
{
    f32 zInv = 1.0f / v->pos.z;
    *v *= zInv;
    v->pos.x = (v->pos.x + 1.0f) * st->xFactor;
    v->pos.y = (v->pos.y + 1.0f) * st->yFactor;
    v->pos.z = zInv;
}

static inline vertex Prestep(i32 rounded, f32 original, vertex step)
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

static inline color GetTextureColorRGB(loaded_bitmap *bmp, vec2 coord)
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

    return bmp->GetColorRGB((i32)x, (i32)y);
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
    result.dv01 = -VertexSlopeY(t->v0, t->v1);
    result.dv02 = -VertexSlopeY(t->v0, t->v2);
    result.dv12 = -VertexSlopeY(t->v1, t->v2);

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

static inline color GetShadedColor(color c, vec3 shade)
{
    u8 r = (u8)((f32)c.r * shade.x);
    u8 g = (u8)((f32)c.g * shade.y);
    u8 b = (u8)((f32)c.b * shade.z);
    return {r, g, b};
}

static void DrawFlatTriangle(
    pixel_buffer *pixelBuffer, color c,
    vertex leftStart, vertex rightStart, vertex dvLeft, vertex dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex left = Prestep(yTop, yTopF32, -dvLeft) + leftStart;
    vertex right = Prestep(yTop, yTopF32, -dvRight) + rightStart;
    vertex leftToRightStep;
    vertex inTriangleCoord;
    f32 objectSpazeZ;

    for (i16 y = yTop; y > yBottom; y--)
    {
        xLeft = RoundF32toI16(left.pos.x);
        xRight = RoundF32toI16(right.pos.x);
        leftToRightStep = VertexSlopeX(left, right);
        inTriangleCoord = left + Prestep(xLeft, left.pos.x, leftToRightStep);

        for (i16 x = xLeft; x < xRight; x++, inTriangleCoord -= leftToRightStep)
        {
            objectSpazeZ = 1.0f / inTriangleCoord.pos.z;
            if (UpdateZBuffer(pixelBuffer, x, y, objectSpazeZ))
            {

                PutPixel(pixelBuffer, x, y, c);
            }
        }
        left -= dvLeft;
        right -= dvRight;
    }
}

static void DrawTriangleSolid(pixel_buffer *pixelBuffer, triangle t, color c)
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
    pixel_buffer *pixelBuffer, loaded_bitmap *bmp, vec3 shade,
    vertex leftStart, vertex rightStart, vertex dvLeft, vertex dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex left = leftStart + Prestep(yTop, yTopF32, -dvLeft);
    vertex right = rightStart + Prestep(yTop, yTopF32, -dvRight);
    vertex leftToRightStep;
    vertex inTriangleCoord;
    f32 objectSpazeZ;
    for (i16 y = yTop; y > yBottom; y--)
    {
        xLeft = RoundF32toI16(left.pos.x);
        xRight = RoundF32toI16(right.pos.x);
        leftToRightStep = VertexSlopeX(left, right);
        inTriangleCoord = left + Prestep(xLeft, left.pos.x, leftToRightStep);

        for (i16 x = xLeft; x < xRight; x++, inTriangleCoord -= leftToRightStep)
        {
            objectSpazeZ = 1.0f / inTriangleCoord.pos.z;
            if (UpdateZBuffer(pixelBuffer, x, y, objectSpazeZ))
            {
                vertex attr = inTriangleCoord * objectSpazeZ;
                color c = GetShadedColor(GetTextureColorRGB(bmp, attr.texCoord), shade);
                PutPixel(pixelBuffer, x, y, c);
            }
        }
        left -= dvLeft;
        right -= dvRight;
    }
}

static void DrawTriangleTextured(pixel_buffer *pixelBuffer, triangle t, loaded_bitmap *bmp, vec3 shade)
{
    processed_triangle_result p = ProcessTriangle(&t);

    // Top Half | Flat Bottom Triangle
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextured(pixelBuffer, bmp, shade, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTriangleTextured(pixelBuffer, bmp, shade, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextured(pixelBuffer, bmp, shade, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTriangleTextured(pixelBuffer, bmp, shade, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
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
            u32 c = GetTextureWrapColorU32(bmp, texCoord.texCoord);
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
        DrawFlatTriangleTextureWrap(pixelBuffer, bmp, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTriangleTextureWrap(pixelBuffer, bmp, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTriangleTextureWrap(pixelBuffer, bmp, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTriangleTextureWrap(pixelBuffer, bmp, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
}

#define GetMeshCopy()        \
    vertex *vertices;        \
    triangle_index *indices; \
    GetMeshCopy_(mesh, &vertices, &indices)
static void GetMeshCopy_(mesh mesh, vertex **verticesOut, triangle_index **triangleIndicesOut)
{
    *verticesOut = (vertex *)calloc(mesh.nVertices, sizeof(vertex));
    *triangleIndicesOut = (triangle_index *)calloc(mesh.nIndices, sizeof(triangle_index));

    memcpy(*verticesOut, mesh.vertices, mesh.nVertices * sizeof(vertex));
    memcpy(*triangleIndicesOut, mesh.indices, mesh.nIndices * sizeof(triangle_index));
}

#define FreeMeshCopy() FreeMeshCopy_(vertices, indices)
static void FreeMeshCopy_(vertex *vertices, triangle_index *indices)
{
    free(vertices);
    free(indices);
}

static inline void VertexShaderWave(vertex *v, void *properties)
{
    vertex_shader_wave *wave = (vertex_shader_wave *)properties;
    v->pos.y += wave->amplitude * std::sin(wave->time * wave->scrollFreq + v->pos.x * wave->waveFreq);
}

static vec3 FlatShading(vec3 normal, diffuse d, ambient a, material m)
{
    normal.normalize();
    d.intensity = d.intensity * maxF32(0.0f, -normal * d.direction);
    vec3 result = Saturated(HadamardProduct(m, d.intensity + a));
    return result;
}

static vec3 GouraudShading(vec3 normal, mat3 rotation, vec3 diffuse, vec3 dir, vec3 ambient, vec3 material)
{
    diffuse = diffuse * maxF32(0.0f, -(normal * rotation) * dir);
    vec3 result = Saturated(HadamardProduct(material, diffuse + ambient));
    return result;
}

static void DrawObjectTextured(
    mesh mesh, mat3 rotation, vec3 translation,
    diffuse d, ambient a, material m, loaded_bitmap *bmp,
    void (*VertexShader)(vertex *, void *), void *vertexShaderProperties,
    pixel_buffer *pixelBuffer, screen_transformer *st)
{
    GetMeshCopy();

    // Apply Transformations
    if (VertexShader)
    {
        for (i32 i = 0; i < mesh.nVertices; i++)
        {
            vertices[i].pos = vertices[i].pos * rotation + translation;
            VertexShader(&vertices[i], vertexShaderProperties);
        }
    }
    else
    {
        for (i32 i = 0; i < mesh.nVertices; i++)
        {
            vertices[i].pos = vertices[i].pos * rotation + translation;
        }
    }

    // Find and Draw Visible Triangles
    for (i32 i = 0; i + 2 < mesh.nIndices; i += 3)
    {
        triangle t = {vertices[indices[i]],
                      vertices[indices[i + 1]],
                      vertices[indices[i + 2]]};
        vec3 normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        bool isVisible = (normal * t.v0.pos) <= 0;
        if (isVisible)
        {
            vec3 shadeFactor = FlatShading(normal, d, a, m);

            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);

            DrawTriangleTextured(pixelBuffer, t, bmp, shadeFactor);
        }
    }

    FreeMeshCopy();
}

static inline color Vec3ToRGB(vec3 color_in)
{
    color result;
    result.r = RoundF32toI8(color_in.r * 255.0f);
    result.g = RoundF32toI8(color_in.g * 255.0f);
    result.b = RoundF32toI8(color_in.b * 255.0f);
    return result;
} 

static void DrawObject_Flat(object *o, diffuse d, ambient a,
                           pixel_buffer *pixelBuffer, screen_transformer *st)
{
    vertex *vertices = (vertex *)calloc(o->nVertices, sizeof(vertex));
    memcpy(vertices, o->vertices, o->nVertices * sizeof(vertex));

    mat3 rotation = RotateX(o->orientation.thetaX) *
                    RotateY(o->orientation.thetaY) *
                    RotateZ(o->orientation.thetaZ);
    vec3 translation = o->pos;

    for (i32 i = 0; i < o->nVertices; i++)
        vertices[i].pos = vertices[i].pos * rotation + translation;

    // Find and Draw Visible Triangles
    color c;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        triangle t = {vertices[i], vertices[i + 1], vertices[i + 2]};
        vec3 normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        bool isVisible = (normal * t.v0.pos) <= 0;
        if (isVisible)
        {
            c = Vec3ToRGB(FlatShading(normal, d, a, o->mat));
            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);

            DrawTriangleSolid(pixelBuffer, t, c);
        }
    }

    free(vertices);
}

static void DrawObjectOutline(
    vertex *vertices, i32 nVertices, i8 *lines, i8 nLineIndices, color c,
    mat3 rotation, vec3 translation, pixel_buffer *pixelBuffer, screen_transformer *st)
{
    // Apply Transformations
    for (i32 i = 0; i < nVertices; i++)
    {
        TransformVertexToScreen(st, &vertices[i]);
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
    vertex *vertices, i32 nVertices, i8 *lines, i8 nLineIndices, color *colors,
    mat3 rotation, vec3 translation, pixel_buffer *pixelBuffer, screen_transformer *st)
{
    // Apply Transformations
    for (i32 i = 0; i < nVertices; i++)
    {
        vertices[i].pos = vertices[i].pos * rotation + translation;
        TransformVertexToScreen(st, &vertices[i]);
    }

    // Find and Draw Visible Triangles
    for (i32 i = 0; i < nLineIndices; i += 2)
    {
        vec3 a = vertices[lines[i]].pos;
        vec3 b = vertices[lines[i + 1]].pos;
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
