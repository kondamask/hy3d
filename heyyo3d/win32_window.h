#pragma once
#include "hy3d_windows_defines.h"
#include "HY3D_Exception.h"
#include "Keyboard.h"

class Window
{
public:
	Window();
	Window(int width, int height, LPCSTR windowTitle);
	~Window();

	Keyboard kbd;

private:
	LPCSTR windowClassName = "HEYYO3D_Window_Class";
	HINSTANCE instance;
	HWND handle;
	int width;
	int height;
	
	static LRESULT CALLBACK HandleWindowCreation(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK HandleMessageThunk(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT CALLBACK HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);

	// Delete the copy constuctor and assignment operation
	// for extra safety.
	Window(const Window&) = delete;
	Window& operator = (const Window&) = delete;

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