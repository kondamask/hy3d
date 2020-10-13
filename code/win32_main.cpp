#include "win32_window.h"

int CALLBACK WinMain(
	HINSTANCE instance, 
	HINSTANCE prevInstance, 
	LPSTR lpCmdLine, 
	int nShowCmd)
{
	Window window(720, 480, "HY3D DEV");
	int quitMessage = -1;
    while(Window::ProcessMessages(quitMessage))
    {
		// do stuff
    }
    return quitMessage;
}