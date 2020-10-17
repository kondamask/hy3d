#pragma once
#include "hy3d_windows.h"
#include "win32_graphics.h"
#include <bitset>

using VK_CODE = unsigned char;

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

struct Dimensions
{
	int width, height;
};

class Window
{
public:
	Window(int width, int height, LPCSTR windowTitle);
	~Window();
	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	void Update();

private:
	// NOTE:
	// On window creation we set the proc function to be the CreateWindowProc which
	// handles the window creation when we get a WM_CREATE message.
	// After that we change the window proc function to ForwardMessageToClassHandler
	// which gets called every time we get a message and forwards it to the class
	// message handling function from where we can call our class functions.
	static LRESULT CALLBACK CreateWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ForwardMessageToClassHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

public:
	Graphics graphics;
	Keyboard keyboard;
	Mouse mouse;

private:
	HINSTANCE instance;
	HWND window;
	Dimensions dimensions;
	LPCSTR windowClassName = "HEYYO3D_Window_Class";
};