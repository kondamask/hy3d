#pragma once
#include "hy3d_vector.h"
#include "hy3d_types.h"

struct triangle
{
    vec3 v0, v1, v2;
};

struct texel
{
    vec3 pos;
    vec2 coord;

    inline texel interpolateTo(texel A, texel B)
    {
        f32 alpha = (A.pos.y - pos.y) / (B.pos.y - pos.y);
        return {
            lerp(pos, A.pos, B.pos, alpha),
            lerp(coord, A.coord, B.coord, alpha)};
    }
};

struct textured_triangle
{
    texel v0, v1, v2;
};

struct color
{
    u8 r, g, b;
};

struct orientation
{
    f32 thetaX, thetaY, thetaZ;
};

struct cube
{
    orientation orientation;
    i8 nVertices = 8;
    i8 nLinesVertices = 24;
    i8 nTrianglesVertices = 36;
    vec3 vertices[8];
    vec2 texCoord[8];
    i8 lines[24] = {
        0, 1, 1, 3, 3, 2, 2, 0,
        0, 4, 1, 5, 3, 7, 2, 6,
        4, 5, 5, 7, 7, 6, 6, 4};
    i8 triangles[36] = {
        0, 2, 1, 2, 3, 1,
        1, 3, 5, 3, 7, 5,
        2, 6, 3, 3, 6, 7,
        4, 5, 7, 4, 7, 6,
        0, 4, 2, 2, 4, 6,
        0, 1, 4, 1, 5, 4};
    bool isTriangleVisible[12] = {false}; // triangles vertices / 3. the number of triangles
    color colors[6] = {
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 0},
        {255, 0, 255},
        {0, 255, 255}};
};

static cube MakeCube(f32 side, orientation o, f32 texSide)
{
    cube result;
    side /= 2.0f;
    result.vertices[0] = {-side, -side, -side};
    result.vertices[1] = {side, -side, -side};
    result.vertices[2] = {-side, side, -side};
    result.vertices[3] = {side, side, -side};
    result.vertices[4] = {-side, -side, side};
    result.vertices[5] = {side, -side, side};
    result.vertices[6] = {-side, side, side};
    result.vertices[7] = {side, side, side};
    result.texCoord[0] = {0.0f, -texSide};
    result.texCoord[1] = {texSide, -texSide};
    result.texCoord[2] = {0.0f, 0.0f};
    result.texCoord[3] = {texSide, 0.0f};
    result.texCoord[4] = {texSide, -texSide};
    result.texCoord[5] = {0.0f, -texSide};
    result.texCoord[6] = {texSide, 0.0f};
    result.texCoord[7] = {0.0f, 0.0f};

    result.orientation.thetaX = o.thetaX;
    result.orientation.thetaY = o.thetaY;
    result.orientation.thetaZ = o.thetaZ;
    return result;
}

struct cube_skinned
{
    orientation orientation;
    i8 nVertices = 14;
    i8 nLinesVertices = 24;
    i8 nTrianglesVertices = 36;
    vec3 vertices[14];
    vec2 texCoord[14];
    i8 lines[24] = {
        0, 1, 1, 3, 3, 2, 2, 0,
        0, 4, 1, 5, 3, 7, 2, 6,
        4, 5, 5, 7, 7, 6, 6, 4};
    // 1 Triangle every three vertices
    i8 triangles[36] = {
        0, 2, 1, 2, 3, 1,
        4, 0, 5, 0, 1, 5,
        6, 4, 7, 4, 5, 7,
        8, 6, 9, 6, 7, 9,
        10, 11, 4, 11, 0, 4,
        5, 1, 12, 1, 13, 12};
    bool isTriangleVisible[12] = {false}; // triangles vertices / 3. the number of triangles
    color colors[6] = {
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 0},
        {255, 0, 255},
        {0, 255, 255}};
};

static inline vec2 ConvertSkinToTextureCoord(f32 u, f32 v)
{
    return {u / 3.0f, v / 4.0f};
}

static cube_skinned MakeCubeSkinned(f32 side, orientation o)
{
    cube_skinned result;
    side /= 2.0f;

    result.vertices[0] = {-side, -side, -side}; // 0
    result.texCoord[0] = {ConvertSkinToTextureCoord(1.0f, 3.0f)};
    result.vertices[1] = {side, -side, -side}; // 1
    result.texCoord[1] = {ConvertSkinToTextureCoord(2.0f, 3.0f)};
    result.vertices[2] = {-side, side, -side}; // 2
    result.texCoord[2] = {ConvertSkinToTextureCoord(1.0f, 4.0f)};
    result.vertices[3] = {side, side, -side}; // 3
    result.texCoord[3] = {ConvertSkinToTextureCoord(2.0f, 4.0f)};
    result.vertices[4] = {-side, -side, side}; // 4
    result.texCoord[4] = {ConvertSkinToTextureCoord(1.0f, 2.0f)};
    result.vertices[5] = {side, -side, side}; // 5
    result.texCoord[5] = {ConvertSkinToTextureCoord(2.0f, 2.0f)};
    result.vertices[6] = {-side, side, side}; // 6
    result.texCoord[6] = {ConvertSkinToTextureCoord(1.0f, 1.0f)};
    result.vertices[7] = {side, side, side}; // 7
    result.texCoord[7] = {ConvertSkinToTextureCoord(2.0f, 1.0f)};
    result.vertices[8] = {-side, side, -side}; // 8
    result.texCoord[8] = {ConvertSkinToTextureCoord(1.0f, 0.0f)};
    result.vertices[9] = {side, side, -side}; // 9
    result.texCoord[9] = {ConvertSkinToTextureCoord(2.0f, 0.0f)};
    result.vertices[10] = {-side, side, side}; // 10
    result.texCoord[10] = {ConvertSkinToTextureCoord(0.0f, 2.0f)};
    result.vertices[11] = {-side, side, -side}; // 11
    result.texCoord[11] = {ConvertSkinToTextureCoord(0.0f, 3.0f)};
    result.vertices[12] = {side, side, side}; // 12
    result.texCoord[12] = {ConvertSkinToTextureCoord(3.0f, 2.0f)};
    result.vertices[13] = {side, side, -side}; // 13
    result.texCoord[13] = {ConvertSkinToTextureCoord(3.0f, 3.0f)};

    result.orientation.thetaX = o.thetaX;
    result.orientation.thetaY = o.thetaY;
    result.orientation.thetaZ = o.thetaZ;
    return result;
}

struct axis3d
{
    i8 nVertices = 4;
    vec3 vertices[4];
    i8 nLinesVertices = 6;
    i8 lines[6] = {
        0, 1,
        0, 2,
        0, 3};
    color colors[3] = {
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255}};
    orientation orientation;
};

static axis3d MakeAxis3D(vec3 center, f32 length, orientation o)
{
    axis3d result;
    result.vertices[0] = center;
    result.vertices[1] = {length, 0.0f, 0.0f};
    result.vertices[1] += center;
    result.vertices[2] = {0.0f, length, 0.0f};
    result.vertices[2] += center;
    result.vertices[3] = {0.0f, 0.0f, length};
    result.vertices[3] += center;
    result.orientation.thetaX = o.thetaX;
    result.orientation.thetaY = o.thetaY;
    result.orientation.thetaZ = o.thetaZ;
    return result;
}