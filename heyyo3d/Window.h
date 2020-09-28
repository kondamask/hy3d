#pragma once
#include "HY3D_Windows.h"
#include "HY3D_Exception.h"

class Window
{
public:
	class Exception : public HY3D_Exception
	{
	public:
		Exception(int line, const char* file, HRESULT hr) noexcept;
		const char* what() const noexcept override;
		virtual const char* GetType() const noexcept override;
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;

	private:
		HRESULT hr;
	};

private:
	// This is a singleton class meaning:
	// The class contains a member variable of the same
	// type which ensures that we can only create ONE
	// instance of it.
	class WindowClass
	{
	public:
		static LPCSTR GetName() noexcept;
		static HINSTANCE GetInstance() noexcept;
	private:
		WindowClass() noexcept;
		~WindowClass();
		static WindowClass wndClass;
		static constexpr LPCSTR name = "HEYYO3D";
		HINSTANCE instance;

		// Delete the copy constuctor and assignment operation
		// for extra safety.
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator = (const WindowClass&) = delete;
	};

public:
	Window(int width, int height, LPCSTR title);
	~Window();
	
private:
	static LRESULT CALLBACK HandleWindowCreation(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK HandleMessageThunk(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT CALLBACK HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

	HWND handle;
	int width;
	int height;

	Window(const Window&) = delete;
	Window& operator = (const Window&) = delete;
};


// error exception helper macro
#define HY3D_WND_EXCEPT( hr ) Window::Exception( __LINE__,__FILE__,hr )
#define HY3D_WND_LAST_EXCEPT() Window::Exception( __LINE__,__FILE__,GetLastError() )