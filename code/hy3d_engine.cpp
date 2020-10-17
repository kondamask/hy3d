#include "hy3d_engine.h"

hy3d_engine::hy3d_engine()
    : window(512, 512, "HY3D")
{
    space.left = -1.0f;
    space.right = 1.0f;
    space.top = 1.0f;
    space.bottom = -1.0f;
    space.width = space.right - space.left;
    space.height = space.top - space.bottom;

    screenTransformer.xFactor = window.graphics.Width() / space.width;
    screenTransformer.yFactor = window.graphics.Height() / space.height;
}

void hy3d_engine::Run()
{
    UpdateFrame();
    ComposeFrame();
    window.Update();
}

void hy3d_engine::UpdateFrame()
{
}

void hy3d_engine::ComposeFrame()
{

    cube cube = MakeCube(1.0f);
    for (int i = 0; i < cube.nVertices; i++)
    {
        cube.vertices[i] += {0.0f, 0.0f, 1.0f};
        screenTransformer.Transform(cube.vertices[i]);
    }
    for (int i = 0; i < 24; i += 2)
    {
        vec3 a = cube.vertices[cube.lines[i]];
        vec3 b = cube.vertices[cube.lines[i + 1]];
        window.graphics.DrawLine(a, b, {255, 255, 255});
    }
}