#include "win32_platform.h"
#include "resources.h"
#include <assert.h>

// NOTE: These file I/O functions should only be used for DEBUG purposes.
DEBUG_FREE_FILE(DEBUGFreeFileMemory)
{
	if (memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}
}

DEBUG_READ_FILE(DEBUGReadFile)
{
	debug_read_file_result result = {};
	HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			// Truncate 64 bit value to 32 bit because VirtualAlloc only takes 32bit value
			ASSERT(fileSize.QuadPart <= 0xFFFFFFFF);
			result.size = (uint32_t)fileSize.QuadPart;

			result.content = VirtualAlloc(0, result.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (result.content)
			{
				DWORD bytesRead;
				if (ReadFile(fileHandle, result.content, result.size, &bytesRead, 0) &&
					result.size == bytesRead)
				{
					// We read the file successfully
				}
				else
				{
					DEBUGFreeFileMemory(result.content);
					result = {};
				}
			}
		}
		CloseHandle(fileHandle);
	}
	// NOTE:  We can add logging in case these steps fail.
	return result;
}

DEBUG_WRITE_FILE(DEBUGWriteFile)
{
	bool result = false;
	HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD bytesWritten;
		if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
		{
			// We wrote into the file successfully
			result = (bytesWritten == memorySize);
		}
		CloseHandle(fileHandle);
	}
	// NOTE:  We can add logging in case these steps fail.
	return result;
}

static inline void Win32InitializeBackbuffer(win32_pixel_buffer &pixel_buffer, i16 width, i16 height)
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
	pixel_buffer.zBuffer = VirtualAlloc(0, pixel_buffer.size, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32ClearBackbuffer(win32_pixel_buffer &pixel_buffer)
{
	if (pixel_buffer.memory)
	{
		VirtualFree(pixel_buffer.memory, 0, MEM_RELEASE);
		VirtualFree(pixel_buffer.zBuffer, 0, MEM_RELEASE);
	}
	pixel_buffer.memory = VirtualAlloc(0, pixel_buffer.size, MEM_COMMIT, PAGE_READWRITE);
	pixel_buffer.zBuffer = VirtualAlloc(0, pixel_buffer.size, MEM_COMMIT, PAGE_READWRITE);
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
		Win32DisplayPixelBuffer(window->pixelBuffer, deviceContext);
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
		ASSERT("We got an input message from somewhere else and we did not handle it properly");
	}

	default:
		result = DefWindowProc(handle, message, wParam, lParam);
	}
	return result;
}

static inline void Win32InitializeWindow(win32_window &window, i16 width, i16 height, LPCSTR windowTitle)
{
	window.instance = GetModuleHandle(nullptr);

	// Set window class properties
	WNDCLASSA windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowProc;
	windowClass.lpszClassName = window.className;
	windowClass.hInstance = window.instance;

	windowClass.hIcon = LoadIcon(window.instance, MAKEINTRESOURCE(101));

	if (!RegisterClassA(&windowClass))
	{
		OutputDebugStringA("Window class wasn't registered.\n");
		return;
	}

	Win32InitializeBackbuffer(window.pixelBuffer, width, height);

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

static inline void Win32InitializeMemory(engine_memory &memory)
{
	LPVOID baseAddress = (LPVOID)TERABYTES(2);
	memory.permanentMemorySize = MEGABYTES(64);
	memory.transientMemorySize = GIGABYTES(2);
	u64 totalSize = memory.permanentMemorySize + memory.transientMemorySize;
	memory.permanentMemory = VirtualAlloc(baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	memory.transientMemory = (u8 *)memory.permanentMemory + memory.permanentMemorySize;
	memory.isInitialized = false;

	memory.DEBUGFreeFileMemory = DEBUGFreeFileMemory;
	memory.DEBUGReadFile = DEBUGReadFile;
	memory.DEBUGWriteFile = DEBUGWriteFile;
}

static void Win32Update(win32_window &window)
{
	HDC deviceContext = GetDC(window.handle);
	Win32DisplayPixelBuffer(window.pixelBuffer, deviceContext);
	Win32ClearBackbuffer(window.pixelBuffer);
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
	case 0x49:
		return I;
		break;
	case 0x4A:
		return J;
		break;
	case 0x4B:
		return K;
		break;
	case 0x4C:
		return L;
		break;
	case 0x55:
		return U;
		break;
	case 0x4F:
		return O;
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
	case 0x30:
		return ZERO;
		break;
	case 0x31:
		return ONE;
		break;
	case 0x32:
		return TWO;
		break;
	case 0x33:
		return THREE;
		break;
	case 0x34:
		return FOUR;
		break;
	case 0x35:
		return FIVE;
		break;
	case 0x36:
		return SIX;
		break;
	case 0x37:
		return SEVEN;
		break;
	case 0x38:
		return EIGHT;
		break;
	case 0x39:
		return NINE;
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
			p.y = window.dimensions.height - p.y;
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
			input.mouse.SetWheelDelta(input.mouse.WheelDelta() + GET_WHEEL_DELTA_WPARAM(message.wParam));
		case WM_MOUSELEAVE:
			POINTS p = MAKEPOINTS(message.lParam);
			input.mouse.SetPos(p.x, p.y);
			break;
		case WM_KILLFOCUS:
			input.keyboard.Clear();
			break;
		default:
			TranslateMessage(&message);
			DispatchMessage(&message);
			break;
		}
	}
	return true;
}

static FILETIME Win32GetWriteTime(char *filename)
{
	FILETIME result = {};

	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFileA(filename, &data);
	if (handle != INVALID_HANDLE_VALUE)
	{
		result = data.ftLastWriteTime;
		FindClose(handle);
	}
	return result;
}

static void Win32LoadEngineCode(win32_engine_code *engineCode, char *sourceFilename, char *sourceFilenameCopy)
{
	// NOTE:  We need to add a sleep in order to wait for the dll compilation.
	Sleep(800);
	engineCode->writeTime = Win32GetWriteTime(sourceFilename);
	CopyFileA(sourceFilename, sourceFilenameCopy, FALSE);
	engineCode->dll = LoadLibraryA(sourceFilenameCopy);
	if (engineCode->dll)
	{
		engineCode->UpdateAndRender = (update_and_render *)GetProcAddress(engineCode->dll, "UpdateAndRender");
		engineCode->isValid = engineCode->UpdateAndRender;
	}
	if (!engineCode->isValid)
	{
		engineCode->UpdateAndRender = UpdateAndRenderStub;
	}
}

static void Win32UnloadEngineCode(win32_engine_code *engineCode)
{
	if (engineCode->dll)
	{
		FreeLibrary(engineCode->dll);
		engineCode->dll = 0;
	}
	engineCode->isValid = false;
	engineCode->UpdateAndRender = UpdateAndRenderStub;
}

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{

	win32_window window;
	Win32InitializeWindow(window, 512, 512, "HY3D");
	// TODO:  We don't have proper focus of the window upon creation.
	// We can't move it unless we alt-tab from it.
	if (window.handle)
	{
		engine_memory engineMemory;
		Win32InitializeMemory(engineMemory);

		if (engineMemory.permanentMemory && engineMemory.transientMemory)
		{
			win32_engine_code engineCode = {};
			char *sourceDLLPath = "W:\\hy3d\\build\\hy3d_engine.dll";
			char *sourceDLLCopyPath = "W:\\hy3d\\build\\hy3d_engine_copy.dll";
			Win32LoadEngineCode(&engineCode, sourceDLLPath, sourceDLLCopyPath);

			hy3d_engine engine = {};
			engine.InitializePixelBuffer(window.pixelBuffer.memory, (f32 *)window.pixelBuffer.zBuffer,
										 window.pixelBuffer.width, window.pixelBuffer.height,
										 window.pixelBuffer.bytesPerPixel, window.pixelBuffer.size);

			i32 quitMessage = -1;
			while (Win32ProcessMessages(window, engine.input, quitMessage))
			{
				FILETIME newWriteTime = Win32GetWriteTime(sourceDLLPath);
				if (CompareFileTime(&newWriteTime, &engineCode.writeTime) == 1)
				{
					Win32UnloadEngineCode(&engineCode);
					Win32LoadEngineCode(&engineCode, sourceDLLPath, sourceDLLCopyPath);
				}
				engineCode.UpdateAndRender(engine, &engineMemory);
				Win32Update(window);
			}
			return quitMessage;
		}
	}
	return 0;
}