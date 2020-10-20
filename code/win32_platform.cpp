#include "win32_platform.h"
//#include "resources.h"

static void InitializeBackbuffer(win32_graphics &graphics, int width, int height)
{
	if (graphics.memory)
	{
		VirtualFree(graphics.memory, 0, MEM_RELEASE);
	}

	graphics.width = width;
	graphics.height = height;
	graphics.bytesPerPixel = 4;

	graphics.info = {};
	graphics.info.bmiHeader.biSize = sizeof(graphics.info.bmiHeader);
	graphics.info.bmiHeader.biWidth = width;
	graphics.info.bmiHeader.biHeight = height; // bottom up y. "-height" fot top down y
	graphics.info.bmiHeader.biPlanes = 1;
	graphics.info.bmiHeader.biBitCount = 32;
	graphics.info.bmiHeader.biCompression = BI_RGB;

	graphics.size = graphics.width * graphics.height * graphics.bytesPerPixel;
	graphics.memory = VirtualAlloc(0, graphics.size, MEM_COMMIT, PAGE_READWRITE);
}

void InitializeWindow(Window &window, int width, int height, LPCSTR windowTitle)
{
	window.instance = GetModuleHandle(nullptr);

	// Set window class properties
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Window::CreateWindowProc;
	windowClass.lpszClassName = window.className;
	windowClass.hInstance = window.instance;

	// TODO: FIX THE FUCKING ICON.
	// windowClass.hIcon = LoadIcon(instance, MAKEINTRESOURCE(HY3D_ICON));

	if (!RegisterClass(&windowClass))
	{
		OutputDebugString("Window class wasn't registered.\n");
		return;
	}

	InitializeBackbuffer(window.graphics, width, height);

	window.dimensions.width = width;
	window.dimensions.height = height;
	// Declare the window client size
	RECT rect = {0};
	rect.left = 100;
	rect.top = 100;
	rect.right = rect.left + window.dimensions.width;
	rect.bottom = rect.top + window.dimensions.height;

	// Adjuct the window size according to the style we
	// have for out window, while keeping the client size
	// the same.
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);
	window.dimensions.width = rect.right - rect.left;
	window.dimensions.height = rect.bottom - rect.top;

	// Create the window
	window.handle = CreateWindow(
		window.className,
		windowTitle,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, // X, Y
		window.dimensions.width,
		window.dimensions.height,
		nullptr, nullptr,
		window.instance,
		&window // * See note bellow
	);

	// NOTE: A value to be passed to the window through the
	// CREATESTRUCT structure pointed to by the lParam of
	// the WM_CREATE message. This message is sent to the
	// created window by this function before it returns.

	ShowWindow(window.handle, SW_SHOWDEFAULT);
}

static void ClearBackbuffer(win32_graphics &graphics)
{
	if (graphics.memory)
	{
		VirtualFree(graphics.memory, 0, MEM_RELEASE);
	}
	graphics.memory = VirtualAlloc(0, graphics.size, MEM_COMMIT, PAGE_READWRITE);
}

static void DisplayPixelBuffer(win32_graphics &graphics, HDC deviceContext)
{
	StretchDIBits(
		deviceContext,
		0, 0, graphics.width, graphics.height,
		0, 0, graphics.width, graphics.height,
		graphics.memory,
		&graphics.info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

static void Win32Update(Window &window)
{
	HDC deviceContext = GetDC(window.handle);
	DisplayPixelBuffer(window.graphics, deviceContext);
	ClearBackbuffer(window.graphics);
	ReleaseDC(window.handle, deviceContext);
}

static void PutPixel(win32_graphics &graphics, int x, int y, Color c)
{
	// Pixel 32 bits
	// Memory:      BB GG RR xx
	// Register:    xx RR GG BB

	uint32_t *pixel = (uint32_t *)graphics.memory + y * graphics.width + x;
	bool isInBuffer =
		y >= 0 &&
		y < graphics.height &&
		x >= 0 &&			// left
		x < graphics.width; // right
	if (isInBuffer)
	{
		*pixel = (c.r << 16) | (c.g << 8) | (c.b);
	}
}

static void DrawLine(win32_graphics &graphics, vec3 a, vec3 b, Color c)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	if (dx == 0.0f && dy == 0.0f)
	{
		PutPixel(graphics, (int)a.x, (int)a.y, c);
	}
	else if (fabsf(dy) >= fabsf(dx))
	{
		if (dy < 0.0f)
		{
			vec3 temp = a;
			a = b;
			b = temp;
		}

		float m = dx / dy;
		for (float x = a.x, y = a.y;
			 y < b.y;
			 y += 1.0f, x += m)
		{
			PutPixel(graphics, (int)x, (int)y, c);
		}
	}
	else
	{
		if (dx < 0.0f)
		{
			vec3 temp = a;
			a = b;
			b = temp;
		}

		float m = dy / dx;
		for (float x = a.x, y = a.y;
			 x < b.x;
			 x += 1.0f, y += m)
		{
			PutPixel(graphics, (int)x, (int)y, c);
		}
	}
}

// NOTE: WINDOWS MESSAGE HANDLERS
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
		DisplayPixelBuffer(graphics, deviceContext);
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

// TEST:
#if 0
static void DrawBufferBounds()
{
    Color c{0, 255, 0};
    uint32_t *pixel = (uint32_t *)graphics.memory;
    for (int y = 0; y < graphics.height; y++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + graphics.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel += graphics.width;
    }
    pixel = (uint32_t *)graphics.memory + 1;
    for (int x = 0; x < graphics.width - 1; x++)
    {
        *pixel = (c.r << 16) | (c.g << 8) | (c.b);
        *(pixel + graphics.size / graphics.bytesPerPixel - graphics.width - 1) = (c.r << 16) | (c.g << 8) | (c.b);
        pixel++;
    }
}

static void DrawTest(int x_in, int y_in)
{
    uint32_t *pixel = (uint32_t *)graphics.memory;
    int strechX = graphics.width / 255;
    int strechY = graphics.height / 255;
    for (int y = 0; y < graphics.height; y++)
    {
        for (int x = 0; x < graphics.width; x++)
        {
            uint8_t r = (uint8_t)((x + x_in) / strechX);
            uint8_t g = (uint8_t)(x / strechX);
            uint8_t b = (uint8_t)((y + y_in) / strechY);
            Color c = Color{r, g, b};
            *pixel++ = (c.r << 16) | (c.g << 8) | (c.b);
        }
    }
}
#endif