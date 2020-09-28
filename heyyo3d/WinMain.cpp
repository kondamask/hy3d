#include "Window.h"

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	try
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
	}
	catch (const HY3D_Exception& e)
	{
		MessageBox(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
	}
	catch (const std::exception& e)
	{
		MessageBox(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		MessageBox(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	return -1;
}