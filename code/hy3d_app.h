#pragma once
#include "win32_window.h"

class App
{
public:
    App();
    int Go();
private:
    void DoFrame();
    Window window;
};