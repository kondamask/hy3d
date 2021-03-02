#pragma once
#include "hy3d_math.h"

struct orientation
{
    f32 thetaX;
    f32 thetaY;
    f32 thetaZ;
};

struct cube
{
    orientation orientation;
    i8 nVertices = 8;
    i8 nLinesVertices = 24;
    i8 nIndices = 36;
    vec3 vertices[8];
    vec2 texCoord[8];
    i8 lines[24] = {
        0, 1, 1, 3, 3, 2, 2, 0,
        0, 4, 1, 5, 3, 7, 2, 6,
        4, 5, 5, 7, 7, 6, 6, 4};
    i8 indices[36] = {
        0, 2, 1, 2, 3, 1,
        1, 3, 5, 3, 7, 5,
        2, 6, 3, 3, 6, 7,
        4, 5, 7, 4, 7, 6,
        0, 4, 2, 2, 4, 6,
        0, 1, 4, 1, 5, 4};
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
    i8 nLineIndices = 24;
    i8 nIndices = 36;
    vertex vertices[14];
    i8 lines[24] = {
        0, 1, 1, 3, 3, 2, 2, 0,
        0, 4, 1, 5, 3, 7, 2, 6,
        4, 5, 5, 7, 7, 6, 6, 4};
    i8 indices[36] = {
        0, 2, 1, 2, 3, 1,
        4, 0, 5, 0, 1, 5,
        6, 4, 7, 4, 5, 7,
        8, 6, 9, 6, 7, 9,
        10, 11, 4, 11, 0, 4,
        5, 1, 12, 1, 13, 12};
};

static inline vec2 ConvertSkinToTextureCoord(f32 u, f32 v)
{
    return {u / 3.0f, v / 4.0f};
}

static cube_skinned MakeCubeSkinned(f32 side, orientation o)
{
    cube_skinned result;
    side /= 2.0f;

    result.vertices[0].pos = {-side, -side, -side}; // 0
    result.vertices[0].uv = {ConvertSkinToTextureCoord(1.0f, 3.0f)};
    result.vertices[1].pos = {side, -side, -side}; // 1
    result.vertices[1].uv = {ConvertSkinToTextureCoord(2.0f, 3.0f)};
    result.vertices[2].pos = {-side, side, -side}; // 2
    result.vertices[2].uv = {ConvertSkinToTextureCoord(1.0f, 4.0f)};
    result.vertices[3].pos = {side, side, -side}; // 3
    result.vertices[3].uv = {ConvertSkinToTextureCoord(2.0f, 4.0f)};
    result.vertices[4].pos = {-side, -side, side}; // 4
    result.vertices[4].uv = {ConvertSkinToTextureCoord(1.0f, 2.0f)};
    result.vertices[5].pos = {side, -side, side}; // 5
    result.vertices[5].uv = {ConvertSkinToTextureCoord(2.0f, 2.0f)};
    result.vertices[6].pos = {-side, side, side}; // 6
    result.vertices[6].uv = {ConvertSkinToTextureCoord(1.0f, 1.0f)};
    result.vertices[7].pos = {side, side, side}; // 7
    result.vertices[7].uv = {ConvertSkinToTextureCoord(2.0f, 1.0f)};
    result.vertices[8].pos = {-side, side, -side}; // 8
    result.vertices[8].uv = {ConvertSkinToTextureCoord(1.0f, 0.0f)};
    result.vertices[9].pos = {side, side, -side}; // 9
    result.vertices[9].uv = {ConvertSkinToTextureCoord(2.0f, 0.0f)};
    result.vertices[10].pos = {-side, side, side}; // 10
    result.vertices[10].uv = {ConvertSkinToTextureCoord(0.0f, 2.0f)};
    result.vertices[11].pos = {-side, side, -side}; // 11
    result.vertices[11].uv = {ConvertSkinToTextureCoord(0.0f, 3.0f)};
    result.vertices[12].pos = {side, side, side}; // 12
    result.vertices[12].uv = {ConvertSkinToTextureCoord(3.0f, 2.0f)};
    result.vertices[13].pos = {side, side, -side}; // 13
    result.vertices[13].uv = {ConvertSkinToTextureCoord(3.0f, 3.0f)};

    result.orientation.thetaX = o.thetaX;
    result.orientation.thetaY = o.thetaY;
    result.orientation.thetaZ = o.thetaZ;
    return result;
}

struct axis3d
{
    i8 nVertices = 4;
    vertex vertices[4];
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
    result.vertices[0].pos = center;
    result.vertices[1].pos = {length, 0.0f, 0.0f};
    result.vertices[1].pos += center;
    result.vertices[2].pos = {0.0f, length, 0.0f};
    result.vertices[2].pos += center;
    result.vertices[3].pos = {0.0f, 0.0f, length};
    result.vertices[3].pos += center;
    result.orientation.thetaX = o.thetaX;
    result.orientation.thetaY = o.thetaY;
    result.orientation.thetaZ = o.thetaZ;
    return result;
}

struct square_plane10
{
    i8 trianglesPerSide = 10;
    i8 nVertices = trianglesPerSide * trianglesPerSide;
    vertex vertices[100];
    i16 nIndices = trianglesPerSide * trianglesPerSide * 3 * 2;
    i8 indices[600];
    orientation orientation;
};

static square_plane10 MakeSquarePlane(f32 side, orientation o)
{
    square_plane10 result = {};
    f32 posStep = side / ((f32)(result.trianglesPerSide - 1));
    f32 texStep = 1.0f / ((f32)(result.trianglesPerSide - 1));
    vec3 pos = {-side / 2.0f, -side / 2.0f, 0.0f}; // bottom left
    vec2 uv = {};
    for (i8 row = 0; row < result.trianglesPerSide; row++)
    {
        for (i8 col = 0; col < result.trianglesPerSide; col++)
        {

            result.vertices[row * result.trianglesPerSide + col].pos = pos;
            result.vertices[row * result.trianglesPerSide + col].uv = uv;
            pos.x += posStep;
            uv.x += texStep;
        }
        pos.x = -side / 2.0f;
        pos.y = (f32)(row + 1) * posStep - side / 2.0f;

        uv.x = 0.0f;
        uv.y = (f32)(row + 1) * texStep;
    }
    int tIndex = 0;
    i8 vIndex = 0;
    while ((tIndex + 5 < result.nIndices) && (vIndex + 11 < result.nVertices))
    {
        result.indices[tIndex] = vIndex;
        result.indices[tIndex + 1] = vIndex + 10;
        result.indices[tIndex + 2] = vIndex + 1;
        tIndex += 3;

        result.indices[tIndex] = vIndex + 10;
        result.indices[tIndex + 1] = vIndex + 11;
        result.indices[tIndex + 2] = vIndex + 1;
        tIndex += 3;
        vIndex++;
    }
    result.orientation = o;
    return result;
}