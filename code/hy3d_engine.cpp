#include "hy3d_engine.h"

// TEST:
static orientation cubeOrientation{0.0f, 0.0f, 0.0f};
static float cubeZ = 2.0f;

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
        cubeOrientation.thetaX += rotSpeed;
    if (e.window.keyboard.IsPressed(VK_DOWN))
        cubeOrientation.thetaX -= rotSpeed;
    if (e.window.keyboard.IsPressed(VK_LEFT))
        cubeOrientation.thetaY += rotSpeed;
    if (e.window.keyboard.IsPressed(VK_RIGHT))
        cubeOrientation.thetaY -= rotSpeed;
    if (e.window.keyboard.IsPressed('Q'))
        cubeOrientation.thetaZ += rotSpeed;
    if (e.window.keyboard.IsPressed('W'))
        cubeOrientation.thetaZ -= rotSpeed;

    if (e.window.keyboard.IsPressed('R'))
    {
        cubeOrientation.thetaX = 0.0f;
        cubeOrientation.thetaY = 0.0f;
        cubeOrientation.thetaZ = 0.0f;
    }

    float offsetZ = 1.0f * dt;
    if (e.window.keyboard.IsPressed('Z'))
        cubeZ -= offsetZ;
    if (e.window.keyboard.IsPressed('X'))
        cubeZ += offsetZ;
}

static void ComposeFrame(hy3d_engine &e)
{
    cube cube = MakeCube(1.0, cubeOrientation);
    mat3 transformation = RotateX(cubeOrientation.thetaX) *
                          RotateY(cubeOrientation.thetaY) *
                          RotateZ(cubeOrientation.thetaZ);
    for (int i = 0; i < cube.nVertices; i++)
    {
        cube.vertices[i] *= transformation;
        cube.vertices[i] += {0.0f, 0.0f, cubeZ};
        e.screenTransformer.Transform(cube.vertices[i]);
    }
    Color c = {200,200,200};
    //for (int i = 0; i < cube.nTrianglesVertices; i += 3)
    for (int i = 0; i < cube.nTrianglesVertices; i += 3)
    {
        triangle t{
            cube.vertices[cube.triangles[i]],
            cube.vertices[cube.triangles[i + 1]],
            cube.vertices[cube.triangles[i + 2]],
        };
        DrawTriangle(e.window.graphics, t, c);
        c.r += 25;
        c.g += 50;
        c.b += 100;
    }

    //Draw Lines
    for (int i = 0; i < cube.nLinesVertices; i += 2)
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