#pragma once
#include "hy3d_windows.h"
#include "hy3d_vector.h"
#include <bitset>

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
	char c = '\0';
	bool autoRepeatEnabled = false;
	std::bitset<256> keyStates;

	void Clear()
	{
		c = '\0';
	}

	void SetChar(char c_in)
	{
		c = c_in;
	}

	void Press(VK_CODE code)
	{
		keyStates[code] = 1;
	}

	void Release(VK_CODE code)
	{
		keyStates[code] = 0;
	}

	bool IsPressed(VK_CODE code)
	{
		return (keyStates[code] == 1);
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
	LPCSTR className = "HEYYO3D_Window_Class";

	// NOTE:
	// On window creation we set the proc function to be the CreateWindowProc which
	// handles the window creation when we get a WM_CREATE message.
	// After that we change the window proc function to ForwardMessageToClassHandler
	// which gets called every time we get a message and forwards it to the class
	// message handling function from where we can call our class functions.
	static LRESULT CALLBACK CreateWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ForwardMessageToClassHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
};

static void InitializeWindow(Window &window, int width, int height, LPCSTR windowTitle);
static void DrawLine(win32_graphics &graphics, vec3 a, vec3 b, Color c);