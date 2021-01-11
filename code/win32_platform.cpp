#include "win32_platform.h"
#include "resources.h"
#include <assert.h>

static void Win32InitializeBackbuffer(win32_pixel_buffer &pixel_buffer, i16 width, i16 height)
{
	if (pixel_buffer.memory)
	{
		VirtualFree(pixel_buffer.memory, 0, MEM_RELEASE);
	}

	pixel_buffer.width = width;
	pixel_buffer.height = height;
	pixel_buffer.bytesPerPixel = 4;

	pixel_buffer.info = {};
	pixel_buffer.info.bmiHeader.biSize = sizeof(pixel_buffer.info.bmiHeader);
	pixel_buffer.info.bmiHeader.biWidth = width;
	pixel_buffer.info.bmiHeader.biHeight = height; // bottom up y. "-height" fot top down y
	pixel_buffer.info.bmiHeader.biPlanes = 1;
	pixel_buffer.info.bmiHeader.biBitCount = 32;
	pixel_buffer.info.bmiHeader.biCompression = BI_RGB;

	pixel_buffer.size = pixel_buffer.width * pixel_buffer.height * pixel_buffer.bytesPerPixel;
	pixel_buffer.memory = VirtualAlloc(0, pixel_buffer.size, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32ClearBackbuffer(win32_pixel_buffer &pixel_buffer)
{
	if (pixel_buffer.memory)
	{
		VirtualFree(pixel_buffer.memory, 0, MEM_RELEASE);
	}
	pixel_buffer.memory = VirtualAlloc(0, pixel_buffer.size, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32DisplayPixelBuffer(win32_pixel_buffer &pixel_buffer, HDC deviceContext)
{
	StretchDIBits(
		deviceContext,
		0, 0, pixel_buffer.width, pixel_buffer.height,
		0, 0, pixel_buffer.width, pixel_buffer.height,
		pixel_buffer.memory,
		&pixel_buffer.info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

static LRESULT Win32MainWindowProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	// NOTE: This pointer doesn't need to be checked for null since it always gets a value
	// before we need to process ather messages. On application start we get:
	// 1st message: WM_GETMINMAXINFO
	// 2nd message: WM_NCCREATE -> sets window pointer in the windows api
	win32_window *window = (win32_window *)GetWindowLongPtr(handle, GWLP_USERDATA);

	//		 before the other messages.
	LRESULT result = 0;
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(handle, &paint);
		Win32DisplayPixelBuffer(window->pixel_buffer, deviceContext);
		EndPaint(handle, &paint);
		break;
	}

	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
		break;
	}

	case WM_DESTROY:
	{
		UnregisterClassA(window->className, window->instance);
		DestroyWindow(handle);
		break;
	}

	case WM_NCCREATE:
	{
		CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
		if (pCreate)
		{
			win32_window *pWindow = (win32_window *)(pCreate->lpCreateParams);
			// Set WinAPI-managed user data to store ptr to window class
			SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)(pWindow));
		}
	}

	// 	NOTE: KEYBOARD AND MOUSE EVENTS SHOULD NOT COME HERE!
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_MOUSELEAVE:
	case WM_KILLFOCUS:
	{
		assert("We got an input message from somewhere else and we did not handle it properly");
	}

	default:
		result = DefWindowProc(handle, message, wParam, lParam);
	}
	return result;
}

static void Win32InitializeWindow(win32_window &window, i16 width, i16 height, LPCSTR windowTitle)
{
	window.instance = GetModuleHandle(nullptr);

	// Set window class properties
	WNDCLASSA windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowProc;
	windowClass.lpszClassName = window.className;
	windowClass.hInstance = window.instance;

	// TODO: FIX THE FUCKING ICON.
	windowClass.hIcon = LoadIconA(window.instance, MAKEINTRESOURCEA(IDI_ICON1));

	if (!RegisterClassA(&windowClass))
	{
		OutputDebugStringA("Window class wasn't registered.\n");
		return;
	}

	Win32InitializeBackbuffer(window.pixel_buffer, width, height);

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
	window.dimensions.width = (i16)(rect.right - rect.left);
	window.dimensions.height = (i16)(rect.bottom - rect.top);

	// Create the window
	window.handle = CreateWindowA(
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

static void Win32Update(win32_window &window)
{
	HDC deviceContext = GetDC(window.handle);
	Win32DisplayPixelBuffer(window.pixel_buffer, deviceContext);
	Win32ClearBackbuffer(window.pixel_buffer);
	ReleaseDC(window.handle, deviceContext);
}

static KEYBOARD_BUTTON Win32TranslateKeyInput(VK_CODE code)
{
	switch (code)
	{
	case VK_UP:
		return UP;
		break;
	case VK_LEFT:
		return LEFT;
		break;
	case VK_DOWN:
		return DOWN;
		break;
	case VK_RIGHT:
		return RIGHT;
		break;
	case 0x57:
		return W;
		break;
	case 0x41:
		return A;
		break;
	case 0x53:
		return S;
		break;
	case 0x44:
		return D;
		break;
	case 0x51:
		return Q;
		break;
	case 0x45:
		return E;
		break;
	case 0x52:
		return R;
		break;
	case 0x46:
		return F;
		break;
	case 0x5A:
		return Z;
		break;
	case 0x58:
		return X;
		break;
	case 0x43:
		return C;
		break;
	case 0x56:
		return V;
		break;
	case VK_SHIFT:
		return SHIFT;
		break;
	case VK_CONTROL:
		return CTRL;
		break;
	case VK_MENU:
		return ALT;
		break;
	case VK_F4:
		return F4;
		break;

	default:
		return INVALID;
		break;
	}
}

static bool Win32ProcessMessages(win32_window &window, engine_input &input, i32 &quitMessage)
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		quitMessage = (i32)message.wParam;

		switch (message.message)
		{
		case WM_QUIT:
			return false;
			break;

		/***************** KEYBOARD EVENTS ****************/
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool wasDown = ((message.lParam >> 30) & 1) != 0;
			if (!wasDown || input.keyboard.autoRepeatEnabled)
			{
				KEYBOARD_BUTTON key = Win32TranslateKeyInput((VK_CODE)message.wParam);
				if (key < KEYBOARD_BUTTON::COUNT)
					input.keyboard.ToggleKey(key);
			}
			if (input.keyboard.isPressed[F4] && input.keyboard.isPressed[ALT])
			{
				PostQuitMessage(0);
				return 0;
			}
			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			KEYBOARD_BUTTON key = Win32TranslateKeyInput((VK_CODE)message.wParam);
			if (key < KEYBOARD_BUTTON::COUNT)
				input.keyboard.ToggleKey(key);
			break;
		}
		/**************************************************/

		/****************** MOUSE EVENTS ******************/
		case WM_MOUSEMOVE:
		{
			POINTS p = MAKEPOINTS(message.lParam);
			bool isInWindow =
				p.x >= 0 && p.x < window.dimensions.width &&
				p.y >= 0 && p.y < window.dimensions.height;
			if (isInWindow)
			{
				input.mouse.SetPos(p.x, p.y);
				if (!input.mouse.isInWindow) // if it wasn't in the window before
				{
					SetCapture(window.handle);
					input.mouse.isInWindow = true;
				}
			}
			else
			{
				if (input.mouse.leftIsPressed || input.mouse.rightIsPressed)
				{
					// mouse is of the window but we're holding a button
					input.mouse.SetPos(p.x, p.y);
				}
				else
				{
					// mouse is out of the window
					ReleaseCapture();
					input.mouse.isInWindow = false;
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
			input.mouse.leftIsPressed = true;
			break;
		case WM_RBUTTONDOWN:
			input.mouse.rightIsPressed = true;
			break;
		case WM_LBUTTONUP:
			input.mouse.leftIsPressed = false;
			break;
		case WM_RBUTTONUP:
			input.mouse.rightIsPressed = false;
			break;
		case WM_MOUSEWHEEL:
			input.mouse.wheelDelta += GET_WHEEL_DELTA_WPARAM(message.wParam);
			while (input.mouse.wheelDelta >= WHEEL_DELTA)
			{
				// wheel up action
				input.mouse.wheelDelta -= WHEEL_DELTA;
			}
			while (input.mouse.wheelDelta <= -WHEEL_DELTA)
			{
				// wheel down action
				input.mouse.wheelDelta += WHEEL_DELTA;
			}
		case WM_MOUSELEAVE:
			POINTS p = MAKEPOINTS(message.lParam);
			input.mouse.SetPos(p.x, p.y);
			break;
		case WM_KILLFOCUS:
		{
			input.keyboard.Clear();
		}

		default:
			TranslateMessage(&message);
			DispatchMessage(&message);
			break;
		}
	}
	return true;
}

static void Win32FreeFileMemory(void *memory)
{
	if (memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}
}

static void *Win32ReadFile(char *filename)
{
	void *result = 0;
	HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			// Truncate 64 bit value to 32 bit because VirtualAlloc only takes 32bit value
			assert(fileSize.QuadPart <= 0xFFFFFFFF);
			uint32_t fileSize32 = (uint32_t)fileSize.QuadPart;

			result = VirtualAlloc(0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			DWORD bytesRead;
			if (result)
			{
				if (ReadFile(fileHandle, result, fileSize32, &bytesRead, 0) &&
					fileSize32 == bytesRead)
				{
					// We read the file successfully
				}
				else
				{
					Win32FreeFileMemory(result);
				}
			}
		}
		CloseHandle(fileHandle);
	}
	// NOTE: We can add logging in case these steps fail.
	return result;
}

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	win32_window window;
	Win32InitializeWindow(window, 512, 512, "HY3D");

	hy3d_engine engine;
	InitializeEngine(engine, window.pixel_buffer.memory, window.pixel_buffer.width, window.pixel_buffer.height, window.pixel_buffer.bytesPerPixel, window.pixel_buffer.size);

	char *filename = __FILE__;
	Win32ReadFile(filename);

	i32 quitMessage = -1;
	while (Win32ProcessMessages(window, engine.input, quitMessage))
	{
		UpdateAndRender(engine);
		Win32Update(window);
	}
	return quitMessage;
}