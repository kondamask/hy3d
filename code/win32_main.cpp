#include "hy3d_windows.h"
#include "hy3d_engine.h"

static bool ProcessMessages(int &quitMessage)
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
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

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	win32_window window;
	InitializeWin32Window(window, 512, 512, "HY3D");

	hy3d_engine engine;
	InitializeEngine(engine, window);
	
	int quitMessage = -1;
	while (ProcessMessages(quitMessage))
	{
		UpdateAndRender(engine);
		Win32Update(engine.window);
	}
	return quitMessage;
}