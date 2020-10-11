#include "win32_window.h"
#include <sstream>

Window::Window(int width, int height, LPCSTR windowTitle)
	:
	width(width), height(height)
{
	instance = GetModuleHandle(nullptr);
	
	WNDCLASSA windowClass = {};

	// Set window class properties
	windowClass.style = CS_OWNDC;
	windowClass.lpfnWndProc = HandleWindowCreation;
	windowClass.lpszClassName = windowClassName;
	windowClass.hInstance = instance;
	windowClass.hbrBackground = 0;
	windowClass.hCursor = LoadCursor(instance, IDC_ARROW);
	windowClass.hIcon = (HICON)LoadIconA(instance, "hy3d.ico");

	
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
	
	gfx = new dx11_graphics(window);

	ShowWindow(window, SW_SHOWDEFAULT);
}

Window::~Window()
{
	delete gfx;
	UnregisterClassA(windowClassName, instance);
	DestroyWindow(window);
}

bool Window::ProcessMessages(int& quitMessage)
{
	MSG message;
	while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
	{
		quitMessage = (int)message.wParam;
		if (message.message == WM_QUIT)
		{
			return false;
		} 
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return true;
}

LRESULT Window::HandleWindowCreation(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_CREATE)
	{
		// extract pointer to window class from creation data
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		if (pCreate)
		{
			Window* pWindow = (Window*)(pCreate->lpCreateParams);
			// Set WinAPI-managed user data to store ptr to window class
			SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)(pWindow));
			// Set message proc to normal handler
			SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(&Window::HandleMessageThunk));
			// forward message to window class handler
			return pWindow->HandleMessage(window, message, wParam, lParam);
		}
	}
	return DefWindowProc(window, message, wParam, lParam);;
}

LRESULT Window::HandleMessageThunk(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window* pWindow = (Window*)GetWindowLongPtr(window, GWLP_USERDATA);
	return pWindow->HandleMessage(window, message, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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
		kbd.Clear();
		break;
	}

	/***************** KEYBOARD EVENTS ****************/
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		bool wasDown = ((lParam >> 30) & 1) != 0;
		if (!wasDown || kbd.autoRepeatEnabled)
		{
			kbd.Press((VK_CODE)wParam);
		}
		break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		kbd.Release((VK_CODE)wParam);
		break;
	}

	case WM_CHAR:
	{
		kbd.SetChar((unsigned char)wParam);
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
			mouse.SetPos(p.x, p.y);
			if(!mouse.isInWindow) // if it wasn't in the window before
			{
				SetCapture(window);
				mouse.isInWindow = true;
			}
		}
		else
		{
			if (mouse.leftIsPressed || mouse.rightIsPressed)
			{
				// mouse is of the window but we're holding a button
				mouse.SetPos(p.x, p.y);
			}
			else
			{
				// mouse is out of the window
				ReleaseCapture();
				mouse.isInWindow = false;
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		mouse.leftIsPressed = true;
		break;
	}
	case WM_RBUTTONDOWN:
	{
		mouse.rightIsPressed = true;
		break;
	}
	case WM_LBUTTONUP:
	{
		mouse.leftIsPressed = false;
		break;
	}
	case WM_RBUTTONUP:
	{
		mouse.rightIsPressed = false;
		break;
	}
	case WM_MOUSEWHEEL:
	{
		mouse.wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
		while (mouse.wheelDelta >= WHEEL_DELTA)
		{
			mouse.wheelDelta -= WHEEL_DELTA;
			// wheel up action
		}
		while (mouse.wheelDelta <= -WHEEL_DELTA)
		{
			mouse.wheelDelta += WHEEL_DELTA;
			// wheel down action
		}
	}
	case WM_MOUSELEAVE:
	{
		POINTS p = MAKEPOINTS(lParam);
		mouse.SetPos(p.x, p.y);
		break;
	}
	/**************************************************/

	case WM_DESTROY:
	{
		break;
	}

	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
	}
	}
	return result;
}