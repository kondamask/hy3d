#pragma once
#include "WindowsDefines.h"
#include "Exception.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"

class Window
{
public:
	Window();
	Window(int width, int height, LPCSTR windowTitle);
	// Not necessary to have a destructor. Everything gets destroyed
	// if the window closes.
	// ~Window();
	// Window(const Window&) = delete;
	// Window& operator = (const Window&) = delete;

	dx11_graphics& Gfx() const;
	Keyboard kbd;
	Mouse mouse;

private:
	HINSTANCE instance;
	HWND window;
	int width;
	int height;
	dx11_graphics *gfx;
	
	static LRESULT CALLBACK HandleWindowCreation(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK HandleMessageThunk(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT CALLBACK HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);

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