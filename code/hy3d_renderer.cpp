#include "hy3d_renderer.h"
#include <utility>

static void DrawLine(win32_graphics &graphics, vec3 a, vec3 b, Color c)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	if (dx == 0.0f && dy == 0.0f)
	{
		PutPixel(graphics, (int)a.x, (int)a.y, c);
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
			PutPixel(graphics, (int)x, (int)y, c);
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
			PutPixel(graphics, (int)x, (int)y, c);
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

static void DrawTriangle(win32_graphics &graphics, triangle t, Color c)
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
	bool isGenericTriangle = true;
	if (t.v0.y == t.v1.y)
	{
		isGenericTriangle = false;
		if (t.v0.x > t.v1.x)
			std::swap(t.v0, t.v1);
	}
	else if (t.v1.y == t.v2.y)
	{
		isGenericTriangle = false;
		if (t.v1.x > t.v2.x)
			std::swap(t.v1, t.v2);
	}
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
	float slope01 = -(t.v1.x - t.v0.x) / (t.v1.y - t.v0.y);
	float slope02 = -(t.v2.x - t.v0.x) / (t.v2.y - t.v0.y);
	float slope12 = -(t.v2.x - t.v1.x) / (t.v2.y - t.v1.y);

	// Top Half | Flat Bottom Triangle
	for (int y = yTop; y > ySplit; y--)
	{
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
			PutPixel(graphics, x, y, c);
		}
	}

	//Bottom Half | Flat Top
	for (int y = ySplit; y > yBottom; y--)
	{
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
			PutPixel(graphics, x, y, c);
		}
	}
}

#if 0
// TEST:
static void DrawTriangle(win32_graphics &graphics, triangle t, Color c)
{
	// Set t.a to be the top vertex and t.c to be the bottom
	if (t.v0.y < t.v1.y)
		std::swap(t.v0, t.v1);
	if (t.v2.y > t.v1.y)
		std::swap(t.v1, t.v2);
	if (t.v2.y > t.v0.y)
		std::swap(t.v0, t.v2);

	// If t.a.y and t.b.y are the same -> flat top
	if (t.v0.y == t.v1.y)
	{
		if (t.v0.x > t.v1.x)
			std::swap(t.v0, t.v1);
		DrawFlattopTriangle(graphics, t.v0, t.v1, t.v2, c);
	}
	if (t.v1.y == t.v2.y)
	{
		if (t.v1.x > t.v2.x)
			std::swap(t.v1, t.v2);
		DrawFlattopTriangle(graphics, t.v0, t.v1, t.v2, c);
	}

	// Find Splitting Vertex
	float alpha = (t.v1.y - t.v0.y) / (t.v2.y - t.v0.y);
	vec3 split = t.v0 + (t.v2 - t.v0) * alpha;

	if (split.x < t.v1.x)
	{
		std::swap(split, t.v1);
	}

	DrawFlatbottomTriangle(graphics, t.v0, t.v1, split, c);
	DrawFlattopTriangle(graphics, t.v1, split, t.v2, c);
}

static void DrawBufferBounds()
{
    Color c{0, 255, 0};
    uint32_t *pixel = (uint32_t *)graphics.memory;
    for (int y = 0; y < graphics.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + graphics.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += graphics.width;
    }
    pixel = (uint32_t *)graphics.memory + 1;
    for (int x = 0; x < graphics.width - 1; x++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + graphics.size / graphics.bytesPerPixel - graphics.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel++;
    }
}

static void DrawTest(int x_in, int y_in)
{
    uint32_t *pixel = (uint32_t *)graphics.memory;
    int strechX = graphics.width / 255;
    int strechY = graphics.height / 255;
    for (int y = 0; y < graphics.height; y++)
    {
        for (int x = 0; x < graphics.width; x++)
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