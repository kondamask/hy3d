#pragma once
#include "hy3d_math.h"

typedef i32 triangle_index;

static inline vec2 ConvertSkinToTextureCoord(f32 u, f32 v)
{
    return {u / 3.0f, v / 4.0f};
}

struct orientation
{
    f32 thetaX;
    f32 thetaY;
    f32 thetaZ;
};

struct mesh
{
    i32 nVertices;
    i32 nIndices;
    vertex *vertices;
    triangle_index *triangleIndices;
};

struct cube
{
    mesh *mesh;
    orientation orientation;
    vec3 pos;
    f32 side;
};

struct square_plane
{
    mesh *mesh;
    orientation orientation;
    vec3 pos;
    f32 side;
    i32 divisions;
};

static void LoadUnfoldedCubeMesh(mesh *mesh, f32 side)
{
    mesh->nVertices = 14;
    mesh->nIndices = 36;
    side /= 2.0f;

    mesh->vertices[0].pos = {-side, -side, -side}; // 0
    mesh->vertices[0].uv = {ConvertSkinToTextureCoord(1.0f, 3.0f)};
    mesh->vertices[1].pos = {side, -side, -side}; // 1
    mesh->vertices[1].uv = {ConvertSkinToTextureCoord(2.0f, 3.0f)};
    mesh->vertices[2].pos = {-side, side, -side}; // 2
    mesh->vertices[2].uv = {ConvertSkinToTextureCoord(1.0f, 4.0f)};
    mesh->vertices[3].pos = {side, side, -side}; // 3
    mesh->vertices[3].uv = {ConvertSkinToTextureCoord(2.0f, 4.0f)};
    mesh->vertices[4].pos = {-side, -side, side}; // 4
    mesh->vertices[4].uv = {ConvertSkinToTextureCoord(1.0f, 2.0f)};
    mesh->vertices[5].pos = {side, -side, side}; // 5
    mesh->vertices[5].uv = {ConvertSkinToTextureCoord(2.0f, 2.0f)};
    mesh->vertices[6].pos = {-side, side, side}; // 6
    mesh->vertices[6].uv = {ConvertSkinToTextureCoord(1.0f, 1.0f)};
    mesh->vertices[7].pos = {side, side, side}; // 7
    mesh->vertices[7].uv = {ConvertSkinToTextureCoord(2.0f, 1.0f)};
    mesh->vertices[8].pos = {-side, side, -side}; // 8
    mesh->vertices[8].uv = {ConvertSkinToTextureCoord(1.0f, 0.0f)};
    mesh->vertices[9].pos = {side, side, -side}; // 9
    mesh->vertices[9].uv = {ConvertSkinToTextureCoord(2.0f, 0.0f)};
    mesh->vertices[10].pos = {-side, side, side}; // 10
    mesh->vertices[10].uv = {ConvertSkinToTextureCoord(0.0f, 2.0f)};
    mesh->vertices[11].pos = {-side, side, -side}; // 11
    mesh->vertices[11].uv = {ConvertSkinToTextureCoord(0.0f, 3.0f)};
    mesh->vertices[12].pos = {side, side, side}; // 12
    mesh->vertices[12].uv = {ConvertSkinToTextureCoord(3.0f, 2.0f)};
    mesh->vertices[13].pos = {side, side, -side}; // 13
    mesh->vertices[13].uv = {ConvertSkinToTextureCoord(3.0f, 3.0f)};

    mesh->triangleIndices[0] = 0;
    mesh->triangleIndices[1] = 2;
    mesh->triangleIndices[2] = 1;
    mesh->triangleIndices[3] = 2;
    mesh->triangleIndices[4] = 3;
    mesh->triangleIndices[5] = 1;
    mesh->triangleIndices[6] = 4;
    mesh->triangleIndices[7] = 0;
    mesh->triangleIndices[8] = 5;
    mesh->triangleIndices[9] = 0;
    mesh->triangleIndices[10] = 1;
    mesh->triangleIndices[11] = 5;
    mesh->triangleIndices[12] = 6;
    mesh->triangleIndices[13] = 4;
    mesh->triangleIndices[14] = 7;
    mesh->triangleIndices[15] = 4;
    mesh->triangleIndices[16] = 5;
    mesh->triangleIndices[17] = 7;
    mesh->triangleIndices[18] = 8;
    mesh->triangleIndices[19] = 6;
    mesh->triangleIndices[20] = 9;
    mesh->triangleIndices[21] = 6;
    mesh->triangleIndices[22] = 7;
    mesh->triangleIndices[23] = 9;
    mesh->triangleIndices[24] = 10;
    mesh->triangleIndices[25] = 11;
    mesh->triangleIndices[26] = 4;
    mesh->triangleIndices[27] = 11;
    mesh->triangleIndices[28] = 0;
    mesh->triangleIndices[29] = 4;
    mesh->triangleIndices[30] = 5;
    mesh->triangleIndices[31] = 1;
    mesh->triangleIndices[32] = 12;
    mesh->triangleIndices[33] = 1;
    mesh->triangleIndices[34] = 13;
    mesh->triangleIndices[35] = 12;

    /*i8 lines[24] = {
        0, 1, 1, 3, 3, 2, 2, 0,
        0, 4, 1, 5, 3, 7, 2, 6,
        4, 5, 5, 7, 7, 6, 6, 4};*/
}

static cube MakeCubeUnfolded(mesh *mesh, orientation orientation, vec3 pos, f32 side)
{
    cube result;
    result.mesh = mesh;
    result.orientation = orientation;
    result.pos = pos;
    result.side = side;
    return result;
}

static void LoadSquarePlaneMesh(mesh *mesh, f32 side, i32 divisions)
{
    f32 posStep = side / (f32)(divisions);
    f32 texStep = 1.0f / (f32)(divisions);

    // Start at bottom left and map texture to vertices
    vec3 pos = {-side / 2.0f, -side / 2.0f, 0.0f};
    vec2 uv = {};
    for (i32 row = 0; row <= divisions; row++)
    {
        for (i32 col = 0; col <= divisions; col++)
        {
            mesh->vertices[row * divisions + row + col].pos = pos;
            mesh->vertices[row * divisions + row + col].uv = uv;
            pos.x += posStep;
            uv.x += texStep;
        }
        pos.x = -side / 2.0f;
        pos.y = (f32)(row + 1) * posStep - side / 2.0f;

        uv.x = 0.0f;
        uv.y = (f32)(row + 1) * texStep;
    }

    // Initialize triangle index list
    i32 ti = 0;
    i32 vi = 0;
    i32 row = 0;
    while ((ti + 5 < mesh->nIndices) && (vi + divisions + 2 < mesh->nVertices))
    {
        mesh->triangleIndices[ti] = vi;
        mesh->triangleIndices[ti + 1] = vi + divisions + 1;
        mesh->triangleIndices[ti + 2] = vi + 1;
        ti += 3;

        mesh->triangleIndices[ti] = vi + divisions + 1;
        mesh->triangleIndices[ti + 1] = vi + divisions + 2;
        mesh->triangleIndices[ti + 2] = vi + 1;
        ti += 3;
        vi++;
        if (vi % divisions == row)
        {
            vi++;
            row++;
        }
    }
}

static square_plane MakeSquarePlane(mesh *mesh, orientation orientation, vec3 pos, f32 side, i32 divisions)
{
    square_plane result;
    result.mesh = mesh;
    result.orientation = orientation;
    result.pos = pos;
    result.side = side;
    result.divisions = divisions;
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
