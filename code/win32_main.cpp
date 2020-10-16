#include "win32_window.h"
#include "hy3d_vector.h"

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	Window window(1024, 512, "HY3D DEV");
	int quitMessage = -1;

	Color c{255, 0, 255};
	vec3 pos{0.0f, 0.0f, 0.0f};
	float speed = 3.0f;

	while (Window::ProcessMessages(quitMessage))
	{
		// Test Drawing
		vec3 dir;
		window.graphics.DrawBufferBounds();
		if (window.keyboard.IsPressed(VK_UP))
			dir.y = -1.0f;
 		if (window.keyboard.IsPressed(VK_DOWN))
			dir.y = 1.0f;
		if (window.keyboard.IsPressed(VK_LEFT))
			dir.x = 1.0f;
		if (window.keyboard.IsPressed(VK_RIGHT))
			dir.x = -1.0f;
		
		pos += dir.normal() * speed;
		
		window.graphics.DrawTest((int)pos.x,(int)pos.y);
		// End Test Drawing

		window.Update();
	}
	return quitMessage;
}