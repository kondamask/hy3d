#include "hy3d_app.h"

App::App()
    : window(720, 480, "HY3D DEV")
{
}

int App::Go()
{
    int quitMessage = -1;
    while(Window::ProcessMessages(quitMessage))
    {
        DoFrame();
    }
    return quitMessage;
}

void App::DoFrame()
{
    window.graphics->RenderBackground(1.0f,0.1f,0.9f);
    window.graphics->EndFrame();
}