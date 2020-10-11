#pragma once
#include "hy3d_windows.h"
#include "dx11_graphics.h"
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

class Window
{
public:
	Window(int width, int height, LPCSTR windowTitle);
	~Window();
	static bool ProcessMessages(int& quitMessage);

private:
	// Windows Messages Handling Functions
	static LRESULT CALLBACK HandleWindowCreation(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HandleMessageThunk(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

public:
	dx11_graphics *gfx;
	Keyboard kbd;
	Mouse mouse;

private:
	HINSTANCE instance;
	HWND window;
	int width;
	int height;
	
	LPCSTR windowClassName = "HEYYO3D_Window_Class";
};