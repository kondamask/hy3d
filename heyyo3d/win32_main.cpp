#include "win32_window.h"
#include "Timer.h"

static bool ProcessMessages(int& quitMessage)
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

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	Window window;
	Timer timer;

	// Message loop
	int quitMessage = -1;
	while (ProcessMessages(quitMessage))
	{
		float t = timer.Peek();
		char title[50];
		sprintf(title, "Time Elapsed: %.2f", t);
		window.SetTitle(title);
	}

	return quitMessage;
}