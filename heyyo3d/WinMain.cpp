#include "Window.h"

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	Window a(700, 500, "HEYYO3D");

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
	
	return message.wParam;
}