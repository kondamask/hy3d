#include "hy3d_windows.h"
#include "d3d11.h"

class dx11_graphics
{
private:
    IDXGISwapChain* swapChain;
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
    ID3D11RenderTargetView* renderTargetView;
public:
    dx11_graphics(HWND window);
    ~dx11_graphics();
    dx11_graphics(const dx11_graphics&) = delete;
	dx11_graphics& operator = (const dx11_graphics&) = delete;
    void EndFrame();

    // test
    void RenderBackground(float r, float g, float b)
    {
        const float color[] = {r,g,b,1.0f};
        deviceContext->ClearRenderTargetView(renderTargetView, color);
    }
};
