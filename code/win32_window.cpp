#include "win32_window.h"
//#include "resources.h"

Window::Window(int width, int height, LPCSTR windowTitle)
	: dimensions({width, height})
{
	instance = GetModuleHandle(nullptr);

	// Set window class properties
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = CreateWindowProc;
	windowClass.lpszClassName = windowClassName;
	windowClass.hInstance = instance;

	// TODO: FIX THE FUCKING ICON.
	// windowClass.hIcon = LoadIcon(instance, MAKEINTRESOURCE(HY3D_ICON));

	if (!RegisterClass(&windowClass))
	{
		OutputDebugString("Window class wasn't registered.\n");
		return;
	}

	graphics.InitializeBackbuffer(dimensions.width, dimensions.height);

	// Declare the window client size
	RECT rect = {0};
	rect.left = 100;
	rect.top = 100;
	rect.right = rect.left + dimensions.width;
	rect.bottom = rect.top + dimensions.height;

	// Adjuct the window size according to the style we
	// have for out window, while keeping the client size
	// the same.
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);
	dimensions.width = rect.right - rect.left;
	dimensions.height = rect.bottom - rect.top;

	// Create the window
	window = CreateWindow(
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, // X, Y
		dimensions.width, dimensions.height,
		nullptr, nullptr,
		instance,
		this // * See note bellow
	);

	// NOTE: A value to be passed to the window through the
	// CREATESTRUCT structure pointed to by the lParam of
	// the WM_CREATE message. This message is sent to the
	// created window by this function before it returns.

	ShowWindow(window, SW_SHOWDEFAULT);
}

Window::~Window()
{
	UnregisterClass(windowClassName, instance);
	DestroyWindow(window);
}

void Window::Update()
{
	HDC deviceContext = GetDC(window);
	graphics.DisplayPixelBuffer(GetDC(window));
	graphics.ClearBackbuffer();
	ReleaseDC(window, deviceContext);
}

LRESULT Window::CreateWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CREATE)
	{
		// extract pointer to window class from creation data
		CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
		if (pCreate)
		{
			Window *pWindow = (Window *)(pCreate->lpCreateParams);
			// Set WinAPI-managed user data to store ptr to window class
			SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)(pWindow));
			// Set message proc to normal handler
			SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(&Window::ForwardMessageToClassHandler));
			// forward message to window class handler
			return pWindow->HandleMessage(window, message, wParam, lParam);
		}
	}

	return DefWindowProc(window, message, wParam, lParam);
	;
}

LRESULT Window::ForwardMessageToClassHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window *pWindow = (Window *)GetWindowLongPtr(window, GWLP_USERDATA);
	return pWindow->HandleMessage(window, message, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		graphics.DisplayPixelBuffer(deviceContext);
		EndPaint(window, &paint);
		break;
	}

	/***************** KEYBOARD EVENTS ****************/
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		bool wasDown = ((lParam >> 30) & 1) != 0;
		if (!wasDown || keyboard.autoRepeatEnabled)
		{
			keyboard.Press((VK_CODE)wParam);
		}
		if (keyboard.IsPressed(VK_F4) && keyboard.IsPressed(VK_MENU))
		{
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		keyboard.Release((VK_CODE)wParam);
		break;
	}

	case WM_CHAR:
	{
		keyboard.SetChar((unsigned char)wParam);
		break;
	}
	/**************************************************/

	/****************** MOUSE EVENTS ******************/
	case WM_MOUSEMOVE:
	{
		POINTS p = MAKEPOINTS(lParam);
		bool isInWindow =
			p.x >= 0 && p.x < dimensions.width &&
			p.y >= 0 && p.y < dimensions.height;
		if (isInWindow)
		{
			mouse.SetPos(p.x, p.y);
			if (!mouse.isInWindow) // if it wasn't in the window before
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

	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
		break;
	}

	case WM_KILLFOCUS:
	{
		keyboard.Clear();
		break;
	}

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