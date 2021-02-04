#include "hy3d_engine.h"

static inline void InitializeEngine(hy3d_engine &e, void *pixel_buffer_memory, i16 width, i16 height, i8 bytesPerPixel, i32 buffer_size)
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

static void PutPixel(pixel_buffer *pixel_buffer, i16 x, i16 y, Color c)
{
    // Pixel 32 bits
    // Memory:      BB GG RR xx
    // Register:    xx RR GG BB

    u32 *pixel = (u32 *)pixel_buffer->memory + y * pixel_buffer->width + x;
    bool isInBuffer =
        y >= 0 &&
        y < pixel_buffer->height &&
        x >= 0 &&                // left
        x < pixel_buffer->width; // right
    if (isInBuffer)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
    }
}

static void DrawLine(pixel_buffer *pixel_buffer, vec3 a, vec3 b, Color c)
{
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    if (dx == 0.0f && dy == 0.0f)
    {
        PutPixel(pixel_buffer, (i16)a.x, (i16)a.y, c);
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
            PutPixel(pixel_buffer, (i16)x, (i16)y, c);
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
            PutPixel(pixel_buffer, (i16)x, (i16)y, c);
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

static void DrawTriangle(pixel_buffer *pixel_buffer, triangle t, Color c)
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

    f32 alphaSplit = (t.v1.y - t.v0.y) / (t.v2.y - t.v0.y);
    vec3 split = t.v0 + (t.v2 - t.v0) * alphaSplit;
    bool isLeftSideMajor = t.v1.x > split.x;

    i16 yTop = (i16)ceilf(t.v0.y - 0.5f);
    i16 ySplit = (i16)ceilf(split.y - 0.5f);
    i16 yBottom = (i16)ceilf(t.v2.y - 0.5f);

    f32 xLeftF, xRightF;
    i16 xLeft, xRight;

    // NOTE:
    // It looks like that multiplication with the negative slope and the negative
    // Dy give more precise results in comparison with the positive slope and Dy.
    f32 slope02 = -(t.v2.x - t.v0.x) / (t.v2.y - t.v0.y);

    // Top Half | Flat Bottom Triangle
    for (i16 y = yTop; y > ySplit; y--)
    {
        f32 slope01 = -(t.v1.x - t.v0.x) / (t.v1.y - t.v0.y);
        if (isLeftSideMajor)
        {
            xLeftF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
            xRightF = slope01 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
        }
        else
        {
            xLeftF = slope01 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
            xRightF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
        }
        xLeft = (i16)ceilf(xLeftF - 0.5f);
        xRight = (i16)ceilf(xRightF - 0.5f);

        for (i16 x = xLeft; x < xRight; x++)
        {
            PutPixel(pixel_buffer, x, y, c);
        }
    }

    //Bottom Half | Flat Top
    for (i16 y = ySplit; y > yBottom; y--)
    {
        f32 slope12 = -(t.v2.x - t.v1.x) / (t.v2.y - t.v1.y);
        if (isLeftSideMajor)
        {
            xLeftF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
            xRightF = slope12 * (t.v1.y - (f32)y + 0.5f) + t.v1.x;
        }
        else
        {
            xLeftF = slope12 * (t.v1.y - (f32)y + 0.5f) + t.v1.x;
            xRightF = slope02 * (t.v0.y - (f32)y + 0.5f) + t.v0.x;
        }
        xLeft = (i16)ceilf(xLeftF - 0.5f);
        xRight = (i16)ceilf(xRightF - 0.5f);
        for (i16 x = xLeft; x < xRight; x++)
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
    u32 *pixel = (u32 *)pixel_buffer.memory;
    for (int y = 0; y < pixel_buffer.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + pixel_buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += pixel_buffer.width;
    }
    pixel = (u32 *)pixel_buffer.memory + 1;
    for (int x = 0; x < pixel_buffer.width - 1; x++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + pixel_buffer.size / pixel_buffer.bytesPerPixel - pixel_buffer.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel++;
    }
}

static void DrawTest(int x_in, int y_in)
{
    u32 *pixel = (u32 *)pixel_buffer.memory;
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

/*
static u32* LoadBitmap(debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
    uint32 *Result = 0;
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
        Result = Pixels;
    }

    return(Result);
}
*/

extern "C" UPDATE_AND_RENDER(UpdateAndRender)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<f32> frameTime = frameEnd - e.frameStart;
    f32 dt = frameTime.count();
    e.frameStart = frameEnd;

    engine_state *state = (engine_state *)memory->permanentMemory;

    if (!memory->isInitialized)
    {
        state->cubeOrientation = {0.0f, 0.0f, 0.0f};
        state->cubeZ = 2.0f;
        state->drawLines = false;
        memory->isInitialized = true;
    }

    // NOTE:  UPDATE
    // Cube Control
    f32 rotSpeed = 1.5f * dt;
    if (e.input.keyboard.isPressed[UP])
        state->cubeOrientation.thetaX += rotSpeed;
    if (e.input.keyboard.isPressed[DOWN])
        state->cubeOrientation.thetaX -= rotSpeed;
    if (e.input.keyboard.isPressed[LEFT])
        state->cubeOrientation.thetaY += rotSpeed;
    if (e.input.keyboard.isPressed[RIGHT])
        state->cubeOrientation.thetaY -= rotSpeed;
    if (e.input.keyboard.isPressed[Q])
        state->cubeOrientation.thetaZ += rotSpeed;
    if (e.input.keyboard.isPressed[W])
        state->cubeOrientation.thetaZ -= rotSpeed;

    if (e.input.keyboard.isPressed[R])
    {
        state->cubeOrientation.thetaX = 0.0f;
        state->cubeOrientation.thetaY = 0.0f;
        state->cubeOrientation.thetaZ = 0.0f;

        f32 offsetZ = 1.0f * dt;
        if (e.input.keyboard.isPressed[Z])
            state->cubeZ -= offsetZ;
        if (e.input.keyboard.isPressed[X])
            state->cubeZ += offsetZ;
    }
    state->drawLines = e.input.keyboard.isPressed[CTRL];

    // NOTE:  RENDER
    state->cubeAxis = MakeAxis3D({-0.5f, -0.5f, -0.5f}, 1.5f, state->cubeOrientation);
    state->cube = MakeCube(1.0f, state->cubeOrientation);

    // Apply Transformations
    mat3 transformation = RotateX(state->cubeOrientation.thetaX) *
                          RotateY(state->cubeOrientation.thetaY) *
                          RotateZ(state->cubeOrientation.thetaZ);
    for (int i = 0; i < state->cubeAxis.nVertices; i++)
    {
        state->cubeAxis.vertices[i] *= transformation;
        state->cubeAxis.vertices[i] += {0.0f, 0.0f, state->cubeZ};
    }
    for (int i = 0; i < state->cube.nVertices; i++)
    {
        state->cube.vertices[i] *= transformation;
        state->cube.vertices[i] += {0.0f, 0.0f, state->cubeZ};
    }

    for (int i = 0; i < state->cube.nTrianglesVertices; i += 3)
    {
        triangle t{
            state->cube.vertices[state->cube.triangles[i]],
            state->cube.vertices[state->cube.triangles[i + 1]],
            state->cube.vertices[state->cube.triangles[i + 2]],
        };
        vec3 normal = CrossProduct(t.v1 - t.v0, t.v2 - t.v0);
        state->cube.isTriangleVisible[i / 3] = normal * t.v0 <= 0;
    }

    // Transform to sceen
    for (int i = 0; i < state->cubeAxis.nVertices; i++)
    {
        e.screenTransformer.Transform(state->cubeAxis.vertices[i]);
    }
    for (int i = 0; i < state->cube.nVertices; i++)
    {
        e.screenTransformer.Transform(state->cube.vertices[i]);
    }

    //Draw Triangles
    for (int i = 0; i < state->cube.nTrianglesVertices; i += 3)
    {
        if (state->cube.isTriangleVisible[i / 3])
        {
            triangle t{
                state->cube.vertices[state->cube.triangles[i]],
                state->cube.vertices[state->cube.triangles[i + 1]],
                state->cube.vertices[state->cube.triangles[i + 2]],
            };
            DrawTriangle(&e.pixel_buffer, t, state->cube.colors[0]);
        }
    }

    //Draw Lines
    if (state->drawLines)
    {
        for (int i = 0; i < state->cube.nLinesVertices; i += 2)
        {
            vec3 a = state->cube.vertices[state->cube.lines[i]];
            vec3 b = state->cube.vertices[state->cube.lines[i + 1]];
            DrawLine(&e.pixel_buffer, a, b, {255, 255, 255});
        }
    }

    for (int i = 0; i < state->cubeAxis.nLinesVertices; i += 2)
    {
        vec3 a = state->cubeAxis.vertices[state->cubeAxis.lines[i]];
        vec3 b = state->cubeAxis.vertices[state->cubeAxis.lines[i + 1]];
        DrawLine(&e.pixel_buffer, a, b, {0, 255, 0});
    }
}

#include "Windows.h"
BOOL WINAPI DllMain(
    _In_  HINSTANCE hinstDLL,
    _In_  DWORD fdwReason,
    _In_  LPVOID lpvReserved
                    )
{
    return(TRUE);
}