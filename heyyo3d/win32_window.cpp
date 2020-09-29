#include "win32_window.h"
#include "resource.h"
#include <sstream>

Window::Window()
	:
	Window(720, 480, "HY3D DEV")
{
}

Window::Window(int width, int height, LPCSTR windowTitle)
{
	instance = GetModuleHandle(nullptr);

	// Set window class properties
	WNDCLASS windowClass = { 0 };
	windowClass.style = CS_OWNDC;
	windowClass.lpfnWndProc = HandleMessage;
	windowClass.lpszClassName = windowClassName;
	windowClass.hInstance = instance;
	windowClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 0));
	windowClass.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	
	if (!RegisterClass(&windowClass))
	{
		return;
	}

	// Declare the _client_ size
	RECT rect = { 0 };
	rect.left = 100;
	rect.top = 100;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	// Adjuct the window size according to the style we
	// have for out window, while keeping the client size
	// the same.
	AdjustWindowRect(&rect, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the window
	handle = CreateWindow(
		windowClassName, 
		windowTitle,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, // X, Y
		width, height,
		nullptr, nullptr, 
		instance, 
		this // * See comment bellow
	); 

	// *
	// A value to be passed to the window through the 
	// CREATESTRUCT structure pointed to by the lParam of 
	// the WM_CREATE message. This message is sent to the
	// created window by this function before it returns.

	ShowWindow(handle, SW_SHOW);
}

Window::~Window()
{
	UnregisterClass(windowClassName, instance);
	DestroyWindow(handle);
}

LRESULT Window::HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
		break;
	}

	case WM_CREATE:
	{
		// extract pointer to window class from creation data
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		if (pCreate)
		{
			Window* pWindow = (Window*)(pCreate->lpCreateParams);
			// Set WinAPI-managed user data to store ptr to window class
			SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)(pWindow));
		}
		break;
	}

	case WM_DESTROY:
	{
		break;
	}

	default:
	{
		result = DefWindowProc(handle, message, wParam, lParam);
	}
	}
	return result;
}

Window::Exception::Exception(int line, const char * file, HRESULT hr) noexcept
	:
	HY3D_Exception(line, file),
	hr(hr)
{
}

const char * Window::Exception::what() const noexcept
{
	std::ostringstream result;
	result << GetType() << std::endl
		<< "Error: " << GetErrorCode() << std::endl
		<< "Description: " << GetErrorString() << std::endl
		<< GetOriginString();

	// after this function, the stringstream dies, so we need to save the
	// message into a provided buffer.
	whatBuffer = result.str();
	return whatBuffer.c_str();
}

const char * Window::Exception::GetType() const noexcept
{
	return "HY3D Window Exception";
}

std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept
{
	char *pBuffer = nullptr;
	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pBuffer),
		0, nullptr))
	{
		std::string error = pBuffer;
		LocalFree(pBuffer);
		return error;
	}
	return "Unidentified error code";
}

HRESULT Window::Exception::GetErrorCode() const noexcept
{
	return hr;
}

std::string Window::Exception::GetErrorString() const noexcept
{
	return TranslateErrorCode(hr);
}
