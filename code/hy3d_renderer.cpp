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

static void ClearZBuffer(pixel_buffer *pixelBuffer)
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

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTE:  Effects
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static inline void VertexShaderWave(vertex *v, void *properties)
{
    vertex_shader_wave *wave = (vertex_shader_wave *)properties;
    v->pos.y += f32(wave->amplitude * sin(wave->time * wave->scrollFreq + v->pos.x * wave->waveFreq));
}

static vec3 FlatShading(diffuse d, ambient a, vec3 n, material m)
{
    n.normalize();
    d.intensity = d.intensity * maxF32(0.0f, -n * d.direction);
    vec3 result = Saturated(HadamardProduct(m, d.intensity + a));
    return result;
}

static vec3 CellShading(diffuse d, ambient a, vec3 n, material m, f32 threshold, f32 shadeFactor)
{
    n.normalize();
    d.intensity = d.intensity * maxF32(0.0f, -n * d.direction);
    vec3 shade = d.intensity + a;
    f32 avgShade = (a.r + shade.g + shade.b) / 3.0f;
    if (avgShade > threshold)
        return m;
    shade = {shadeFactor, shadeFactor, shadeFactor};
    vec3 result = Saturated(HadamardProduct(m, shade));
    return result;
}

static void GouraudShading(triangle_smooth *t, diffuse d, ambient a, material m, mat3 r)
{
    for (i8 i = 0; i < 3; i++)
    {
        vec3 n = t->v[i].normal * r;
        vec3 dif = d.intensity * maxF32(0.0f, -n * d.direction);
        t->v[i].color = Saturated(HadamardProduct(m, dif + a));
    }
}
/*
static vec3 PhongShading(vertex_smooth v, lighting l, material m)
{
    vec3 vToL = l.p.pos - v.color;
    f32 dist = vToL.length();
    vec3 dir = vToL / dist;
    f32 attenuation = 1.0f / (l.p.constantAttenuation + l.p.linearAttenuation * dist + l.p.quadradicAttenuation * Squared(dist));
    vec3 dif = l.d.intensity * attenuation * maxF32(0.0f, v.normal.normalized() * l.d.direction);
    return Saturated(HadamardProduct(m, dif + l.a));
}
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTE:  FLAT Triangle Rendering
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static inline vertex Prestep(i32 rounded, f32 original, vertex step)
{
    return (step * (original - (f32)rounded + 0.5f));
}

static inline vertex_smooth Prestep(i32 rounded, f32 original, vertex_smooth step)
{
    vertex_smooth result = step * (original - (f32)rounded + 0.5f);
    return result;
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

static color GetTextureColorRGB(loaded_bitmap *bmp, vec2 coord)
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

static void DrawFlatTriangleSmooth(
    pixel_buffer *pixelBuffer,
    vertex_smooth leftStart, vertex_smooth rightStart,
    vertex_smooth dvLeft, vertex_smooth dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex_smooth left = leftStart + Prestep(yTop, yTopF32, -dvLeft);
    vertex_smooth right = rightStart + Prestep(yTop, yTopF32, -dvRight);
    vertex_smooth leftToRightStep;
    vertex_smooth inTriangleCoord;
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
                color c = Vec3ToRGB(inTriangleCoord.color);
                PutPixel(pixelBuffer, x, y, c);
            }
        }
        left -= dvLeft;
        right -= dvRight;
    }
}
/*
static void DrawFlatTrianglePhong(
    pixel_buffer *pixelBuffer, lighting l, material m,
    vertex_smooth leftStart, vertex_smooth rightStart,
    vertex_smooth dvLeft, vertex_smooth dvRight,
    f32 yTopF32, f32 yBottomF32)
{
    i16 xLeft;
    i16 xRight;
    i16 yTop = RoundF32toI16(yTopF32);
    i16 yBottom = RoundF32toI16(yBottomF32);
    vertex_smooth left = leftStart + Prestep(yTop, yTopF32, -dvLeft);
    vertex_smooth right = rightStart + Prestep(yTop, yTopF32, -dvRight);
    vertex_smooth leftToRightStep;
    vertex_smooth inTriangleCoord;
    f32 worldZ;
    color c;

    for (i16 y = yTop; y > yBottom; y--)
    {
        xLeft = RoundF32toI16(left.pos.x);
        xRight = RoundF32toI16(right.pos.x);
        leftToRightStep = VertexSlopeX(left, right);
        inTriangleCoord = left + Prestep(xLeft, left.pos.x, leftToRightStep);

        for (i16 x = xLeft; x < xRight; x++, inTriangleCoord -= leftToRightStep)
        {
            worldZ = 1.0f / inTriangleCoord.pos.z;
            if (UpdateZBuffer(pixelBuffer, x, y, worldZ))
            {
                vertex_smooth attr = inTriangleCoord * worldZ;
                c = Vec3ToRGB(PhongShading(attr, l, m));
                PutPixel(pixelBuffer, x, y, c);
            }
        }
        left -= dvLeft;
        right -= dvRight;
    }
}
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTE:  GENERIC Triangle Rendering
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static processed_triangle ProcessTriangle(triangle *t)
{
    processed_triangle result = {};

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

static void DrawTriangleSolid(pixel_buffer *pixelBuffer, triangle t, color c)
{
    processed_triangle p = ProcessTriangle(&t);

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

static void DrawTriangleTextured(pixel_buffer *pixelBuffer, triangle t, loaded_bitmap *bmp, vec3 shade)
{
    processed_triangle p = ProcessTriangle(&t);

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

static processed_smooth_triangle ProcessSmoothTriangle(triangle_smooth *t)
{
    processed_smooth_triangle result = {};

    // Sort by y: v0 is at the top, v2 at the bottom
    if (t->v0.pos.y < t->v1.pos.y)
    {
        std::swap(t->v0, t->v1);
    }
    if (t->v1.pos.y < t->v2.pos.y)
    {
        std::swap(t->v1, t->v2);
    }
    if (t->v0.pos.y < t->v1.pos.y)
    {
        std::swap(t->v0, t->v1);
    }

    // Sort by x:
    // if v1.pos.y is the same as v0.pos.y, v1 should be to the right
    // if v1.pos.y is the same as v2.pos.y, v1 it should be to the left
    if (t->v0.pos.y == t->v1.pos.y && t->v0.pos.x > t->v1.pos.x)
    {
        std::swap(t->v0, t->v1);
    }
    else if (t->v1.pos.y == t->v2.pos.y && t->v1.pos.x > t->v2.pos.x)
    {
        std::swap(t->v1, t->v2);
    }

    result.split = t->v0.interpolateTo(t->v1, t->v2);

    // Find point where triangle is split in 2 parts: Flat Top and Flat Bottom
    // same y as v1 but on the opposite side of the triangle

    // Check if hypotinus is on the left
    result.isLeftSideMajor = t->v1.pos.x > result.split.pos.x;

    // Calculate Slopes and Texture Lookup steps
    result.dv01 = -VertexSlopeY(t->v0, t->v1);
    result.dv02 = -VertexSlopeY(t->v0, t->v2);
    result.dv12 = -VertexSlopeY(t->v1, t->v2);

    return result;
}

static void DrawTriangleGouraudShaded(pixel_buffer *pixelBuffer, triangle_smooth t)
{
    processed_smooth_triangle p = ProcessSmoothTriangle(&t);

    // Top Half | Flat Bottom Triangle
    if (p.isLeftSideMajor)
        DrawFlatTriangleSmooth(pixelBuffer, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTriangleSmooth(pixelBuffer, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTriangleSmooth(pixelBuffer, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTriangleSmooth(pixelBuffer, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
}
/*
static void DrawTrianglePhongShaded(pixel_buffer *pixelBuffer, triangle_smooth t, lighting l, material m)
{
    processed_smooth_triangle p = ProcessSmoothTriangle(&t);

    // Top Half | Flat Bottom Triangle
    if (p.isLeftSideMajor)
        DrawFlatTrianglePhong(pixelBuffer, l, m, t.v0, t.v0, p.dv02, p.dv01, t.v0.pos.y, t.v1.pos.y);
    else
        DrawFlatTrianglePhong(pixelBuffer, l, m, t.v0, t.v0, p.dv01, p.dv02, t.v0.pos.y, t.v1.pos.y);

    //Bottom Half | Flat Top
    if (p.isLeftSideMajor)
        DrawFlatTrianglePhong(pixelBuffer, l, m, p.split, t.v1, p.dv02, p.dv12, t.v1.pos.y, t.v2.pos.y);
    else
        DrawFlatTrianglePhong(pixelBuffer, l, m, t.v1, p.split, p.dv12, p.dv02, t.v1.pos.y, t.v2.pos.y);
}
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTE:  Generic Mesh & Bitmap Rendering
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define TransformVertexToScreen_(st, vertex, vertex_type) \
    TransformVertexToScreen(st, (vertex_type *)vertex)

static inline void TransformVertexToScreen(screen_transformer *st, vertex *v)
{
    f32 zInv = 1.0f / v->pos.z;
    *v *= zInv;
    v->pos.x = (v->pos.x + 1.0f) * st->xFactor;
    v->pos.y = (v->pos.y + 1.0f) * st->yFactor;
    v->pos.z = zInv;
}

static inline void TransformVertexToScreen(screen_transformer *st, vertex_smooth *v)
{
    f32 zInv = 1.0f / v->pos.z;
    v->pos *= zInv;
    v->texCoord *= zInv;
    v->pos.x = (v->pos.x + 1.0f) * st->xFactor;
    v->pos.y = (v->pos.y + 1.0f) * st->yFactor;
    v->pos.z = zInv;
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
            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);
            vec3 shadeFactor = FlatShading(d, a, normal, m);
            DrawTriangleTextured(pixelBuffer, t, bmp, shadeFactor);
        }
    }

    FreeMeshCopy();
}

static void DrawObjectSolid(object *o, mat3 rot, vec3 trans,
                            pixel_buffer *pb, screen_transformer *st)
{
    triangle t;
    vec3 normal;
    color c;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        t.v[0] = o->vertices[i];
        t.v[1] = o->vertices[i + 1];
        t.v[2] = o->vertices[i + 2];

        t.v0.pos = t.v0.pos * rot + trans;
        t.v1.pos = t.v1.pos * rot + trans;
        t.v2.pos = t.v2.pos * rot + trans;

        normal = CrossProduct(t.v2.pos - t.v0.pos, t.v1.pos - t.v0.pos);
        if ((normal * t.v0.pos) <= 0) // is visible
        {

            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);
            c = Vec3ToRGB(o->mat);
            DrawTriangleSolid(pb, t, c);
        }
    }
}

static void DrawObjectFlatShaded(object *o, mat3 rot, vec3 trans, diffuse d, ambient a,
                                 pixel_buffer *pb, screen_transformer *st)
{
    triangle t;
    vec3 normal;
    color c;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        t.v[0] = o->vertices[i];
        t.v[1] = o->vertices[i + 1];
        t.v[2] = o->vertices[i + 2];

        t.v0.pos = t.v0.pos * rot + trans;
        t.v1.pos = t.v1.pos * rot + trans;
        t.v2.pos = t.v2.pos * rot + trans;

        normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        if ((normal * t.v0.pos) <= 0) // is visible
        {

            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);
            c = Vec3ToRGB(FlatShading(d, a, normal, o->mat));
            DrawTriangleSolid(pb, t, c);
        }
    }
}

static void DrawObjectTexturedFlatShaded(object *o, mat3 rot, vec3 trans, diffuse d, ambient a,
                                         pixel_buffer *pb, screen_transformer *st)
{
    triangle t;
    vec3 normal;
    vec3 shade;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        t.v[0] = o->vertices[i];
        t.v[1] = o->vertices[i + 1];
        t.v[2] = o->vertices[i + 2];

        t.v0.pos = t.v0.pos * rot + trans;
        t.v1.pos = t.v1.pos * rot + trans;
        t.v2.pos = t.v2.pos * rot + trans;

        normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        if ((normal * t.v0.pos) <= 0) // is visible
        {

            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);
            shade = FlatShading(d, a, normal, o->mat);
            DrawTriangleTextured(pb, t, o->texture, shade);
        }
    }
}

static void DrawObjectCellShaded(object *o, mat3 rot, vec3 trans, diffuse d, ambient a, f32 th, f32 sf,
                                 pixel_buffer *pb, screen_transformer *st)
{
    color c;
    triangle t;
    vec3 normal;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        t.v[0] = o->vertices[i];
        t.v[1] = o->vertices[i + 1];
        t.v[2] = o->vertices[i + 2];

        t.v0.pos = t.v0.pos * rot + trans;
        t.v1.pos = t.v1.pos * rot + trans;
        t.v2.pos = t.v2.pos * rot + trans;

        normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        if ((normal * t.v0.pos) <= 0) // is visible
        {
            TransformVertexToScreen(st, &t.v0);
            TransformVertexToScreen(st, &t.v1);
            TransformVertexToScreen(st, &t.v2);
            c = Vec3ToRGB(CellShading(d, a, normal, o->mat, th, sf));
            DrawTriangleSolid(pb, t, c);
        }
    }
}

static void TransformAndAssemble(vertex v0, vertex v1, vertex v2, mat3 rot, vec3 tran, triangle_smooth *tOut)
{
    v0.pos = v0.pos * rot + tran;
    v1.pos = v1.pos * rot + tran;
    v2.pos = v2.pos * rot + tran;

    tOut->v0.pos = v0.pos;
    tOut->v0.normal = v0.normal;
    tOut->v0.texCoord = v0.texCoord;

    tOut->v1.pos = v1.pos;
    tOut->v1.normal = v1.normal;
    tOut->v1.texCoord = v1.texCoord;

    tOut->v2.pos = v2.pos;
    tOut->v2.normal = v2.normal;
    tOut->v2.texCoord = v2.texCoord;
}

static void DrawObjectGouraudShaded(object *o, mat3 rot, vec3 trans, diffuse d, ambient a,
                                    pixel_buffer *pb, screen_transformer *st)
{
    triangle_smooth t = {};
    vec3 normal;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        TransformAndAssemble(o->vertices[i], o->vertices[i + 1], o->vertices[i + 2], rot, trans, &t);
        normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        if ((normal * t.v0.pos) <= 0) // is visible
        {
            for (i8 vi = 0; vi < 3; vi++)
                TransformVertexToScreen(st, &t.v[vi]);
            GouraudShading(&t, d, a, o->mat, rot);
            DrawTriangleGouraudShaded(pb, t);
        }
    }
}
/*
static void DrawObjectPhongShaded(object *o, mat3 rot, vec3 trans, diffuse d, ambient a, point_light p,
                                  pixel_buffer *pb, screen_transformer *st)
{
    triangle_smooth t = {};
    vec3 normal;
    for (i32 i = 0; i + 2 < o->nVertices; i += 3)
    {
        TransformAndAssemble(o->vertices[i], o->vertices[i + 1], o->vertices[i + 2], rot, trans, &t);
        normal = CrossProduct(t.v1.pos - t.v0.pos, t.v2.pos - t.v0.pos);
        if ((normal * t.v0.pos) <= 0) // is visible
        {
            for (i8 vi = 0; vi < 3; vi++)
                TransformVertexToScreen(st, &t.v[vi]);
            DrawTrianglePhongShaded(pb, t, {d, a, p}, o->mat);
        }
    }
}
*/
static void DrawObject(object *o, diffuse d, ambient a, point_light l, shade_type shade,
                       pixel_buffer *pb, screen_transformer *st)
{
    mat3 rotation = RotateX(o->orientation.thetaX) *
                    RotateY(o->orientation.thetaY) *
                    RotateZ(o->orientation.thetaZ);
    vec3 translation = o->pos;

    if (o->texture)
    {
        if (shade == shade_type::GOURAUD && o->hasNormals)
            DrawObjectGouraudShaded(o, rotation, translation, d, a, pb, st);
        else if (shade == shade_type::FLAT || (shade == shade_type::GOURAUD && !o->hasNormals))
            DrawObjectTexturedFlatShaded(o, rotation, translation, d, a, pb, st);
        else if (shade == shade_type::CELL)
            DrawObjectCellShaded(o, rotation, translation, d, a, 0.6f, 0.7f, pb, st);
    }
    else
    {
        if (shade == shade_type::SOLID)
            DrawObjectSolid(o, rotation, translation, pb, st);
        else if (shade == shade_type::GOURAUD && o->hasNormals)
            DrawObjectGouraudShaded(o, rotation, translation, d, a, pb, st);
        else if (shade == shade_type::FLAT || (shade == shade_type::GOURAUD && !o->hasNormals))
            DrawObjectFlatShaded(o, rotation, translation, d, a, pb, st);
        else if (shade == shade_type::CELL)
            DrawObjectCellShaded(o, rotation, translation, d, a, 0.6f, 0.7f, pb, st);
        //else if (shade == shade_type::PHONG && o->hasNormals)
        //    DrawObjectPhongShaded(o, rotation, translation, d, a, l, pb, st);
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

/*
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
    processed_triangle p = ProcessTriangle(&t);
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

static u32 GetTextureColorU32(loaded_bitmap *bmp, vec2 coord)
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
}*/
