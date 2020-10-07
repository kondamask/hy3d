#include "Window.h"
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

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	Window window;
	Timer timer;

	// Message loop
	int quitMessage = -1;
	while (ProcessMessages(quitMessage))
	{
		float r = sin(timer.Peek()) / 2.0f + 0.5f;
		float g = sin(timer.Peek()) / 4.0f + 0.5f;
		float b = sin(timer.Peek()) / 6.0f + 0.5f;

		window.gfx->RenderBackground(r,g,b);
		window.gfx->EndFrame();
	}

	return quitMessage;
}