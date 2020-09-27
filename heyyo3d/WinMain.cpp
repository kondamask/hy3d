#include <Windows.h>

LRESULT CALLBACK WndProc(
	HWND window,        // handle to window
	UINT message,        // message identifier
	WPARAM wParam,    // first message parameter
	LPARAM lParam)    // second message parameter
{

	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(420);
		break;
	}
	return DefWindowProc(window, message, wParam, lParam);
}


int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	auto className = "heyyo3d";

	// Registering window class
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_OWNDC;
	wndClass.lpfnWndProc = WndProc;
	wndClass.lpszClassName = className;
	wndClass.hInstance = instance;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = nullptr;
	wndClass.hCursor = nullptr;
	wndClass.hIcon = nullptr;
	wndClass.hIconSm = nullptr;
	wndClass.lpszMenuName = nullptr;
	RegisterClassEx(&wndClass);

	// Creating window instance
	HWND window = CreateWindowEx(
		0, className, "HEYYO3D",
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		0, 0, 640, 480, // X, Y, Width, Height
		nullptr, nullptr, instance, nullptr);

	ShowWindow(window, SW_SHOW);

	// Message loop
	MSG message;
	BOOL gResult = GetMessage(&message, nullptr, 0, 0);
	while (gResult > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
		gResult = GetMessage(&message, nullptr, 0, 0);
	}

	if (gResult == -1)
	{
		return -1;
	}
	else
	{
		return message.wParam;
	}
}