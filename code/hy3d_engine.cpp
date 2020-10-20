#include "hy3d_engine.h"

static void InitializeEngine(hy3d_engine &e)
{
    InitializeWindow(e.window, 512, 512, "HY3D");

    e.space.left = -1.0f;
    e.space.right = 1.0f;
    e.space.top = 1.0f;
    e.space.bottom = -1.0f;
    e.space.width = e.space.right - e.space.left;
    e.space.height = e.space.top - e.space.bottom;

    e.screenTransformer.xFactor = e.window.graphics.width / e.space.width;
    e.screenTransformer.yFactor = e.window.graphics.height / e.space.height;

    e.frameStart = std::chrono::steady_clock::now();
}

static void UpdateFrame(hy3d_engine &e)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<float> frameTime = frameEnd - e.frameStart;
    float dt = frameTime.count();
    e.frameStart = frameEnd;

    float rotSpeed = 1.5f * dt;
    if (e.window.keyboard.IsPressed(VK_UP))
        e.cubeOrientation.thetaX -= rotSpeed;
    if (e.window.keyboard.IsPressed(VK_DOWN))
        e.cubeOrientation.thetaX += rotSpeed;
    if (e.window.keyboard.IsPressed(VK_LEFT))
        e.cubeOrientation.thetaY -= rotSpeed;
    if (e.window.keyboard.IsPressed(VK_RIGHT))
        e.cubeOrientation.thetaY += rotSpeed;
    if (e.window.keyboard.IsPressed('Q'))
        e.cubeOrientation.thetaZ += rotSpeed;
    if (e.window.keyboard.IsPressed('W'))
        e.cubeOrientation.thetaZ -= rotSpeed;

    if (e.window.keyboard.IsPressed('R'))
    {
        e.cubeOrientation.thetaX = 0.0f;
        e.cubeOrientation.thetaY = 0.0f;
        e.cubeOrientation.thetaZ = 0.0f;
    }

    float dSize = 1.0f * dt;
    if (e.window.keyboard.IsPressed('Z'))
        e.cubeSide -= dSize;
    if (e.window.keyboard.IsPressed('X'))
        e.cubeSide += dSize;
}

static void ComposeFrame(hy3d_engine &e)
{
    cube cube = MakeCube(e.cubeSide, e.cubeOrientation);
    mat3 rotation = RotateX(e.cubeOrientation.thetaX) *
                    RotateY(e.cubeOrientation.thetaY) *
                    RotateZ(e.cubeOrientation.thetaZ);
    for (int i = 0; i < cube.nVertices; i++)
    {
        cube.vertices[i] *= rotation;
        cube.vertices[i] += {0.0f, 0.0f, 1.0f};
        e.screenTransformer.Transform(cube.vertices[i]);
    }
    for (int i = 0; i < 24; i += 2)
    {
        vec3 a = cube.vertices[cube.lines[i]];
        vec3 b = cube.vertices[cube.lines[i + 1]];
        DrawLine(e.window.graphics, a, b, {255, 255, 255});
    }
}

static void Run(hy3d_engine &e)
{
    UpdateFrame(e);
    ComposeFrame(e);
    Win32Update(e.window);
}