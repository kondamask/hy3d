#include "Window.h"
#include "resource.h"
#include <sstream>

Window::Window()
	:
	Window(720, 480, "HY3D DEV")
{
}

Window::Window(int width, int height, LPCSTR windowTitle)
	:
	width(width), height(height)
{
	instance = GetModuleHandle(nullptr);

	LPCSTR windowClassName = "HEYYO3D_Window_Class";
	WNDCLASSA windowClass = {};

	// Set window class properties
	windowClass.style = CS_OWNDC;
	windowClass.lpfnWndProc = HandleWindowCreation;
	windowClass.lpszClassName = windowClassName;
	windowClass.hInstance = instance;
	windowClass.hbrBackground = 0;
	windowClass.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	
	if (!RegisterClassA(&windowClass))
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
	window = CreateWindowA(
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

	ShowWindow(window, SW_SHOW);
	gfx = new dx11_graphics(window);
}

// Window::~Window()
// {
// 	delete gfx;
// 	UnregisterClassA(windowClassName, instance);
// 	DestroyWindow(window);
// }

dx11_graphics& Window::Gfx() const
{
	return *gfx;
}

LRESULT Window::HandleWindowCreation(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	if(message == WM_CREATE)
	{
		// extract pointer to window class from creation data
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		if (pCreate)
		{
			Window* pWindow = (Window*)(pCreate->lpCreateParams);
			// Set WinAPI-managed user data to store ptr to window class
			SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)(pWindow));
			// Set message proc to normal handler
			SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR)(&Window::HandleMessageThunk));
			// forward message to window class handler
			return pWindow->HandleMessage(handle, message, wParam, lParam);
		}
	}
	return DefWindowProc(handle, message, wParam, lParam);;
}

LRESULT Window::HandleMessageThunk(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	Window* pWindow = (Window*)GetWindowLongPtr(handle, GWLP_USERDATA);
	return pWindow->HandleMessage(handle, message, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
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

	case WM_KILLFOCUS:
	{
		kbd.ClearState();
	} break;

	/***************** KEYBOARD EVENTS ****************/
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		bool wasDown = ((lParam >> 30) & 1) != 0;
		if (!wasDown || kbd.AutoRepeatEnabled())
		{
			kbd.OnPress((VK_CODE)wParam);
		}
		break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		kbd.OnRelease((VK_CODE)wParam);
		break;
	}

	case WM_CHAR:
	{
		kbd.OnChar((unsigned char)wParam);
		break;
	}
	/**************************************************/
	
	/****************** MOUSE EVENTS ******************/
	case WM_MOUSEMOVE:
	{
		POINTS p = MAKEPOINTS(lParam);

		bool isInWindow = p.x >= 0 && p.x < width && p.y >= 0 && p.y < height;
		if (isInWindow)
		{
			mouse.OnMove(p.x, p.y);
			if(!mouse.IsInWindow())
			{
				SetCapture(handle);
				mouse.OnEnter();
			}
		}
		else
		{
			if (mouse.LeftIsPressed() || mouse.RightIsPressed())
			{
				mouse.OnMove(p.x, p.y);
			}
			else
			{
				ReleaseCapture();
				mouse.OnLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.OnLeftPress(p.x, p.y);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.OnRightPress(p.x, p.y);
		break;
	}
	case WM_LBUTTONUP:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.OnLeftRelease(p.x, p.y);
		break;
	}
	case WM_RBUTTONUP:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.OnRightRelease(p.x, p.y);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.OnWheelDelta(p.x, p.y, GET_WHEEL_DELTA_WPARAM(wParam));
		break;
	}
	case WM_MOUSELEAVE:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.OnMove(p.x, p.y);
		break;
	}
	/**************************************************/

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
	if (FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)(&pBuffer),
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
