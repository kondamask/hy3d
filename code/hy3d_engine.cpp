#include "hy3d_engine.h"

static void PutPixel(pixel_buffer &pixel_buffer, int x, int y, Color c)
{
    // Pixel 32 bits
    // Memory:      BB GG RR xx
    // Register:    xx RR GG BB

    uint32_t *pixel = (uint32_t *)pixel_buffer.memory + y * pixel_buffer.width + x;
    bool isInBuffer =
        y >= 0 &&
        y < pixel_buffer.height &&
        x >= 0 &&               // left
        x < pixel_buffer.width; // right
    if (isInBuffer)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
    }
}

static void DrawLine(pixel_buffer &pixel_buffer, vec3 a, vec3 b, Color c)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    if (dx == 0.0f && dy == 0.0f)
    {
        PutPixel(pixel_buffer, (int)a.x, (int)a.y, c);
    }
    else if (fabsf(dy) >= fabsf(dx))
    {
        if (dy < 0.0f)
        {
            vec3 temp = a;
            a = b;
            b = temp;
        }

        float m = dx / dy;
        for (float x = a.x, y = a.y;
             y < b.y;
             y += 1.0f, x += m)
        {
            PutPixel(pixel_buffer, (int)x, (int)y, c);
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

        float m = dy / dx;
        for (float x = a.x, y = a.y;
             x < b.x;
             x += 1.0f, y += m)
        {
            PutPixel(pixel_buffer, (int)x, (int)y, c);
        }
    }
}

/*  NOTE:
	How a triangle might look like:

	Case 1:
	             v0
	             *
		      * *
		   *   *
	    *     *
  v1 *       *
	  *     *
	   *   *
	    * *
		 * v2

	Case 2:
	 v0 *
	 	 *  *
		  *    *
		   *       *
		    *         *
			 *           * v1
			  *        *
			   *     *
			    *  *
				 * v2
*/

static void DrawTriangle(pixel_buffer &pixel_buffer, triangle t, Color c)
{
    // Sort by y: v0 is at the top, v2 at the bottom
    if (t.v0.y < t.v1.y)
        std::swap(t.v0, t.v1);
    if (t.v1.y < t.v2.y)
        std::swap(t.v1, t.v2);
    if (t.v0.y < t.v1.y)
        std::swap(t.v0, t.v1);

    // Sort by x:
    // if v1.y is the same as v0.y, it should be to the right
    // if v1.y is the same as v2.y, it should be to the left
    if (t.v0.y == t.v1.y && t.v0.x > t.v1.x)
        std::swap(t.v0, t.v1);
    else if (t.v1.y == t.v2.y && t.v1.x > t.v2.x)
        std::swap(t.v1, t.v2);

    float alphaSplit = (t.v1.y - t.v0.y) / (t.v2.y - t.v0.y);
    vec3 split = t.v0 + (t.v2 - t.v0) * alphaSplit;
    bool isLeftSideMajor = t.v1.x > split.x;

    int yTop = (int)ceilf(t.v0.y - 0.5f);
    int ySplit = (int)ceilf(split.y - 0.5f);
    int yBottom = (int)ceilf(t.v2.y - 0.5f);

    float xLeftF, xRightF;
    int xLeft, xRight;

    // NOTE:
    // It looks like that multiplication with the negative slope and the negative
    // Dy give more precise results in comparison with the positive slope and Dy.
    float slope02 = -(t.v2.x - t.v0.x) / (t.v2.y - t.v0.y);

    // Top Half | Flat Bottom Triangle
    for (int y = yTop; y > ySplit; y--)
    {
        float slope01 = -(t.v1.x - t.v0.x) / (t.v1.y - t.v0.y);
        if (isLeftSideMajor)
        {
            xLeftF = slope02 * (t.v0.y - (float)y + 0.5f) + t.v0.x;
            xRightF = slope01 * (t.v0.y - (float)y + 0.5f) + t.v0.x;
        }
        else
        {
            xLeftF = slope01 * (t.v0.y - (float)y + 0.5f) + t.v0.x;
            xRightF = slope02 * (t.v0.y - (float)y + 0.5f) + t.v0.x;
        }
        xLeft = (int)ceilf(xLeftF - 0.5f);
        xRight = (int)ceilf(xRightF - 0.5f);

        for (int x = xLeft; x < xRight; x++)
        {
            PutPixel(pixel_buffer, x, y, c);
        }
    }

    //Bottom Half | Flat Top
    for (int y = ySplit; y > yBottom; y--)
    {
        float slope12 = -(t.v2.x - t.v1.x) / (t.v2.y - t.v1.y);
        if (isLeftSideMajor)
        {
            xLeftF = slope02 * (t.v0.y - (float)y + 0.5f) + t.v0.x;
            xRightF = slope12 * (t.v1.y - (float)y + 0.5f) + t.v1.x;
        }
        else
        {
            xLeftF = slope12 * (t.v1.y - (float)y + 0.5f) + t.v1.x;
            xRightF = slope02 * (t.v0.y - (float)y + 0.5f) + t.v0.x;
        }
        xLeft = (int)ceilf(xLeftF - 0.5f);
        xRight = (int)ceilf(xRightF - 0.5f);
        for (int x = xLeft; x < xRight; x++)
        {
            PutPixel(pixel_buffer, x, y, c);
        }
    }
}

#if 0
// TEST:
static void DrawBufferBounds()
{
    Color c{0, 255, 0};
    uint32_t *pixel = (uint32_t *)pixel_buffer.memory;
    for (int y = 0; y < pixel_buffer.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + pixel_buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += pixel_buffer.width;
    }
    pixel = (uint32_t *)pixel_buffer.memory + 1;
    for (int x = 0; x < pixel_buffer.width - 1; x++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + pixel_buffer.size / pixel_buffer.bytesPerPixel - pixel_buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel++;
    }
}

static void DrawTest(int x_in, int y_in)
{
    uint32_t *pixel = (uint32_t *)pixel_buffer.memory;
    int strechX = pixel_buffer.width / 255;
    int strechY = pixel_buffer.height / 255;
    for (int y = 0; y < pixel_buffer.height; y++)
    {
        for (int x = 0; x < pixel_buffer.width; x++)
        {
            uint8_t r = (uint8_t)((x + x_in) / strechX);
            uint8_t g = (uint8_t)(x / strechX);
            uint8_t b = (uint8_t)((y + y_in) / strechY);
            Color c = Color{r, g, b};
            *pixel++ = (c.r << 16) | (c.g << 8) | (c.b);
        }
    }
}
#endif

static void InitializeEngine(hy3d_engine &e, void *pixel_buffer_memory, int width, int height, int bytesPerPixel, int buffer_size)
{
    e.pixel_buffer = {};
    e.pixel_buffer.memory = pixel_buffer_memory;
    e.pixel_buffer.width = width;
    e.pixel_buffer.height = height;
    e.pixel_buffer.bytesPerPixel = bytesPerPixel;
    e.pixel_buffer.size = width * height * bytesPerPixel;

    e.input = {};

    e.space.left = -1.0f;
    e.space.right = 1.0f;
    e.space.top = 1.0f;
    e.space.bottom = -1.0f;
    e.space.width = e.space.right - e.space.left;
    e.space.height = e.space.top - e.space.bottom;

    e.screenTransformer.xFactor = width / e.space.width;
    e.screenTransformer.yFactor = height / e.space.height;

    e.frameStart = std::chrono::steady_clock::now();
}

static void UpdateFrame(hy3d_engine &e)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<float> frameTime = frameEnd - e.frameStart;
    float dt = frameTime.count();
    e.frameStart = frameEnd;

    // Cube Control
    float rotSpeed = 1.5f * dt;
    if (e.input.keyboard.isPressed[UP])
        e.state.cubeOrientation.thetaX += rotSpeed;
    if (e.input.keyboard.isPressed[DOWN])
        e.state.cubeOrientation.thetaX -= rotSpeed;
    if (e.input.keyboard.isPressed[LEFT])
        e.state.cubeOrientation.thetaY += rotSpeed;
    if (e.input.keyboard.isPressed[RIGHT])
        e.state.cubeOrientation.thetaY -= rotSpeed;
    if (e.input.keyboard.isPressed[Q])
        e.state.cubeOrientation.thetaZ += rotSpeed;
    if (e.input.keyboard.isPressed[W])
        e.state.cubeOrientation.thetaZ -= rotSpeed;

    if (e.input.keyboard.isPressed[R])
    {
        e.state.cubeOrientation.thetaX = 0.0f;
        e.state.cubeOrientation.thetaY = 0.0f;
        e.state.cubeOrientation.thetaZ = 0.0f;
    }

    float offsetZ = 1.0f * dt;
    if (e.input.keyboard.isPressed[Z])
        e.state.cubeZ -= offsetZ;
    if (e.input.keyboard.isPressed[X])
        e.state.cubeZ += offsetZ;

    e.state.drawLines = e.input.keyboard.isPressed[CTRL];
}
static void ComposeFrame(hy3d_engine &e)
{
    cube cube = MakeCube(1.0, e.state.cubeOrientation);

    // Apply Transformations
    mat3 transformation = RotateX(e.state.cubeOrientation.thetaX) *
                          RotateY(e.state.cubeOrientation.thetaY) *
                          RotateZ(e.state.cubeOrientation.thetaZ);

    for (int i = 0; i < cube.nVertices; i++)
    {
        cube.vertices[i] *= transformation;
        cube.vertices[i] += {0.0f, 0.0f, e.state.cubeZ};
    }

    for (int i = 0; i < cube.nTrianglesVertices; i += 3)
    {
        triangle t{
            cube.vertices[cube.triangles[i]],
            cube.vertices[cube.triangles[i + 1]],
            cube.vertices[cube.triangles[i + 2]],
        };
        vec3 normal = CrossProduct(t.v1 - t.v0, t.v2 - t.v0);
        cube.isTriangleVisible[i / 3] = normal * t.v0 <= 0;
    }

    // Transform to sceen
    for (int i = 0; i < cube.nVertices; i++)
    {
        e.screenTransformer.Transform(cube.vertices[i]);
    }

    // Draw Triangles
    for (int i = 0; i < cube.nTrianglesVertices; i += 3)
    {
        if (cube.isTriangleVisible[i / 3])
        {
            triangle t{
                cube.vertices[cube.triangles[i]],
                cube.vertices[cube.triangles[i + 1]],
                cube.vertices[cube.triangles[i + 2]],
            };
            DrawTriangle(e.pixel_buffer, t, cube.colors[i / 6]);
        }
    }

    //Draw Lines
    if (e.state.drawLines)
    {
        for (int i = 0; i < cube.nLinesVertices; i += 2)
        {
            vec3 a = cube.vertices[cube.lines[i]];
            vec3 b = cube.vertices[cube.lines[i + 1]];
            DrawLine(e.pixel_buffer, a, b, {255, 255, 255});
        }
    }
}

static void UpdateAndRender(hy3d_engine &e)
{
    UpdateFrame(e);
    ComposeFrame(e);
}