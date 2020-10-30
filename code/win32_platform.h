#pragma once
#include "hy3d_windows.h"
#include "hy3d_vector.h"
#include "stdint.h"

#define VK_CODE unsigned char

struct Dimensions
{
	int width, height;
};

struct Color
{
	uint8_t r, g, b;
};

struct Keyboard
{
	bool autoRepeatEnabled = false;
	uint64_t state[4] = {}; // 4*64=256 total states

	void Clear()
	{
		for (int i = 0; i < 4; i++)
			state[i] = 0;
	}

	void ToggleKey(VK_CODE code)
	{
		int group = code / 256;
		int i = code % 256;
		state[group] ^= 1ULL << i;
	}

	bool IsPressed(VK_CODE code)
	{
		int group = code / 256;
		int i = code % 256;
		return (state[group] & (1ULL << i));
	}
};

struct Mouse
{
	int x = 0;
	int y = 0;
	bool isInWindow = false;
	bool leftIsPressed = false;
	bool rightIsPressed = false;
	int wheelDelta = 0;

	void SetPos(int x_in, int y_in)
	{
		x = x_in;
		y = y_in;
	}
};

struct win32_graphics
{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int bytesPerPixel;
	int size;
};

struct Window
{
	win32_graphics graphics;
	Keyboard keyboard;
	Mouse mouse;
	HINSTANCE instance;
	HWND handle;
	Dimensions dimensions;
	LPCSTR className = "HY3D_WINDOW_CLASS";
};

static void InitializeWindow(Window &window, int width, int height, LPCSTR windowTitle);
static void Win32Update(Window &window);
static void PutPixel(win32_graphics &graphics, int x, int y, Color c);