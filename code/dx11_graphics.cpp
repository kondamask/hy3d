#include "dx11_graphics.h"

dx11_graphics::dx11_graphics(HWND window)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.RefreshRate = {};
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;

    swapChainDesc.OutputWindow = window;
    swapChainDesc.Windowed = TRUE;

    swapChainDesc.SwapEffect =DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    HRESULT createResult = D3D11CreateDeviceAndSwapChain(
    0,
    D3D_DRIVER_TYPE_HARDWARE,
    0,
    0,
    0,
    0,
    D3D11_SDK_VERSION,
    &swapChainDesc,
    &swapChain,
    &device,
    0,
    &deviceContext);

    if (createResult != S_OK)
    {
        // TODO: LOG
    }

    ID3D11Resource* backbuffer = 0;
    swapChain->GetBuffer(0, __uuidof(ID3D11Resource), (void**)&backbuffer);
    device->CreateRenderTargetView(backbuffer,0,&renderTargetView);
    backbuffer->Release();
}

dx11_graphics::~dx11_graphics()
{
    if(device) device->Release();
    if(swapChain) swapChain->Release();
    if(deviceContext) deviceContext->Release();
    if(renderTargetView) renderTargetView->Release();
}

void dx11_graphics::EndFrame()
{
    swapChain->Present(1u,0u);
}