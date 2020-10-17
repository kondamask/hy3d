#pragma once
#include "win32_window.h"
#include "hy3d_vector.h"
#include "hy3d_matrix.h"

// NOTE:
// The HY3D space is a 3d space where
// Y IS UP
// X IS RIGHT
// Z IS INTO THE SCREEN
// The origin (0,0,0) is in the center of the screen.
// We normalize the coordinates so that the far right, left, top and down
// take values -1.0 and +1.0

struct hy3d_space
{
    float left;
    float right;
    float top;
    float bottom;
    // TODO: add Z later
    float width;
    float height;
};

struct hy3d_screen_transformer
{
    float xFactor, yFactor;

    vec3 GetTransformed(vec3 v)
    {
        v.x = (v.x + 1.0f) * xFactor;
        v.y = (v.y + 1.0f) * yFactor;
        return v;
    }

    void Transform(vec3 &v)
    {
        v = GetTransformed(v);
    }
};

class hy3d_engine
{
public:
    hy3d_engine();
    void Run();

private:
    void UpdateFrame();
    void ComposeFrame();

private:
    Window window;
    hy3d_space space;
    hy3d_screen_transformer screenTransformer;

    // TEST:
    float thetaX = 0.0f;
    float thetaY = 0.0f;
    float thetaZ = 0.0f;
    float side = 1.0f;
};

// TEST:
struct cube
{
    float thetaX, thetaY, thetaZ;
    int nVertices = 8;
    int nLines = 12;
    vec3 vertices[8];
    int lines[24] = {
			0,1,  1,3,  3,2,  2,0,
			0,4,  1,5,	3,7,  2,6,
			4,5,  5,7,	7,6,  6,4 };
};

// TEST:
cube MakeCube(float side, float thetaX, float thetaY, float thetaZ)
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
    result.thetaX = thetaX;
    result.thetaY = thetaY;
    result.thetaZ = thetaZ;
    return result;
}