#include "win32_window.h"

int CALLBACK WinMain(
	HINSTANCE instance, 
	HINSTANCE prevInstance, 
	LPSTR lpCmdLine, 
	int nShowCmd)
{
	Window window(1024, 512, "HY3D DEV");
	int quitMessage = -1;

	Color c{255,0,255};

	int x = 1024/2;
	int y = 512/2;

    while(Window::ProcessMessages(quitMessage))
    {
		window.graphics.DrawBufferBounds();

		if(window.keyboard.IsPressed(VK_UP))
			y += 1;
		if(window.keyboard.IsPressed(VK_DOWN))
			y -= 1;
		if(window.keyboard.IsPressed(VK_LEFT))
			x -= 1;
		if(window.keyboard.IsPressed(VK_RIGHT))
			x += 1;

		window.graphics.PutPixel(x,y, {255,0,0});
		window.Update();
    }
    return quitMessage;
}