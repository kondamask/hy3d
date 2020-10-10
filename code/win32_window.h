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
	Window();
	Window(int width, int height, LPCSTR windowTitle);

	dx11_graphics *gfx;
	Keyboard kbd;
	Mouse mouse;

private:
	HINSTANCE instance;
	HWND window;
	int width;
	int height;
	
	static LRESULT CALLBACK HandleWindowCreation(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HandleMessageThunk(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
};

////////////	EXCEPTIONS	///////////////

/*
public:
	class Exception : public HY3D_Exception
	{
	public:
		Exception(int line, const char* file, HRESULT hr) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;

	private:
		HRESULT hr;
	};
};


// error exception helper macro
#define HY3D_WND_EXCEPT( hr ) Window::Exception( __LINE__,__FILE__,hr )
#define HY3D_WND_LAST_EXCEPT() Window::Exception( __LINE__,__FILE__,GetLastError() )
*/