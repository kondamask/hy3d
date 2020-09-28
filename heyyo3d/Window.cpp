#include "Window.h"
#include "resource.h"

Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() noexcept
	:
	instance(GetModuleHandle(nullptr))
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleWindowCreation;
	wc.lpszClassName = GetName();
	wc.hInstance = GetInstance();
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = nullptr;
	wc.hCursor = nullptr;
	wc.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	wc.hIconSm = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	wc.lpszMenuName = nullptr;
	RegisterClassEx(&wc);
}

Window::WindowClass::~WindowClass()
{
	UnregisterClass(GetName(), GetInstance());
}

LPCSTR Window::WindowClass::GetName() noexcept
{
	return name;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.instance;
}

Window::Window(int width, int height, LPCSTR title)  noexcept
{
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
		WindowClass::GetName(), title,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, // X, Y
		width, height,
		nullptr, nullptr, 
		WindowClass::GetInstance(), 
		this // * See comment bellow
	); 

	// *
	// A value to be passed to the window through the 
	// CREATESTRUCT structure pointed to by the lParam of 
	// the WM_CREATE message. This message is sent to the
	// created window by this function before it returns.

	ShowWindow(handle, SW_SHOWDEFAULT);
}

Window::~Window()
{
	DestroyWindow(handle);
}

LRESULT Window::HandleWindowCreation(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	// use create parametre passed in from CreateWindow() to store our
	// window class point at WinAPI

	// The lParam of the message is a pointer to the CREATESTRUCT structure
	// that contains information about the window being created.
	if (message == WM_NCCREATE)
	{
		// extract pointer to window class from creation data
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWindow = static_cast<Window*>(pCreate->lpCreateParams);

		// Set WinAPI-manages user data to store ptr to window class
		SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));

		// Set mesage proc to a normal message handler (not the one responsible for the window creation)
		SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMessageThunk));

		// forward message to window class handler
		return pWindow->HandleMessage(handle, message, wParam, lParam);
	}

	// use default handler for any message before window creation
	return DefWindowProc(handle, message, wParam, lParam);
}

LRESULT Window::HandleMessageThunk(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	// get pointer to our window class
	Window* pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));

	// forward message to window class handler
	return pWindow->HandleMessage(handle, message, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(handle, message, wParam, lParam);
}