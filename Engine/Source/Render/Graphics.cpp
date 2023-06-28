#include "Engine.h"
#include "Header/Graphics.h"

#include "../Core/Header/Window.h"
#include "../Event/Header/Debug.h"

owl::Graphics::Graphics()
	: m_D3D11Device(nullptr), m_D3D11Context(nullptr), m_DXGISwapChain(nullptr), m_D3D11RenderTargetView(nullptr), m_D3D11BlendState(nullptr)
{
    m_D3D11Viewport = { 0 };
	m_D3DFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	m_VSyncMode = Disabled;

	m_BackgroundColour[0] = 0.0f;
	m_BackgroundColour[1] = 0.0f;
	m_BackgroundColour[2] = 0.0f;
	m_BackgroundColour[3] = 0.0f;
}

bool owl::Graphics::Initialize()
{
    // Default D3D11 Device's flag
    uint CreateDeviceFlags = 0;

#ifdef WL_DEBUG
    // exibe mensagens de erro do Direct3D em modo de depuração
    CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Creates a device to access the D3D11 Device
    if FAILED(
        D3D11CreateDevice(
            nullptr,                        // Graphics device slot (nullptr = Default graphics device)
            D3D_DRIVER_TYPE_HARDWARE,       // D3D driver's type (Hardware, Reference or Software)
            nullptr,                        // Pointer to a software rasterizer
            CreateDeviceFlags,              // D3D device's flags (Debug or Release)
            nullptr,                        // D3D's version (nullptr = Greater supported)
            0,                              // Features level's vector
            D3D11_SDK_VERSION,              // D3D's SDK's version
            &m_D3D11Device,                 // Stores the D3D's device
            &m_D3DFeatureLevel,             // Stores the D3D's version
            &m_D3D11Context))               // Stores the D3D's context

    {
        // Failed to creates a hardware graphics device reference
        Debug::Message(Error, "Failed to create the HAL device.");

        if FAILED(
            D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, CreateDeviceFlags, nullptr, 0, D3D11_SDK_VERSION, &m_D3D11Device, &m_D3DFeatureLevel, &m_D3D11Context))
            return false;

        Debug::Message(Warning, "There is no support for D3D11. Now you are using the WARP adapter.");
    }

    // Adjusts the background's colour
    COLORREF Colour = Window::GetBackgroundColour();

    m_BackgroundColour[0] = GetRValue(Colour) / 255.0f; // Red
    m_BackgroundColour[1] = GetGValue(Colour) / 255.0f; // Green
    m_BackgroundColour[2] = GetBValue(Colour) / 255.0f; // Blue
    m_BackgroundColour[3] = 1.0f;                       // Alpha (1 = cor sólida)

    // Creates a DXGI's device
    IDXGIDevice* DXGIDevice = nullptr;
    if FAILED(m_D3D11Device->QueryInterface(UUID(IDXGIDevice), reinterpret_cast<void**>(&DXGIDevice)))
    {
        Debug::Message(Error, "Failed to create the DXGI's device.");
        return false;
    }

    // Creates a DXGI's adapter
    IDXGIAdapter* DXGIAdapter = nullptr;
    if FAILED(DXGIDevice->GetParent(UUID(IDXGIAdapter), reinterpret_cast<void**>(&DXGIAdapter)))
    {
        Debug::Message(Error, "Failed to create the DXGI's adapter.");
        return false;
    }

    // Creates a DXGI's factory
    IDXGIFactory* DXGIFactory = nullptr;
    if FAILED(DXGIAdapter->GetParent(UUID(IDXGIFactory), reinterpret_cast<void**>(&DXGIFactory)))
    {
        Debug::Message(Error, "Failed to create the DXGI's factory.");
        return false;
    }

    // Set-up a swap chain
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = { 0 };
    SwapChainDesc.BufferDesc.Width = static_cast<uint>(Window::GetSize()[0]);  // Backbuffer's width
    SwapChainDesc.BufferDesc.Height = static_cast<uint>(Window::GetSize()[1]); // Backbuffer's height
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;                       // Refresh rate in hertz
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;                      // Numerator is an integer and not a rational
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;              // RGBA 8 bits for colour's format
    SwapChainDesc.SampleDesc.Count = 1;                                        // Antialiasing's samples
    SwapChainDesc.SampleDesc.Quality = 0;                                      // Possible image's quality
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;               // Specify backbuffer's usage
    SwapChainDesc.BufferCount = 2;                                             // Numbers of buffers (front + back)
    SwapChainDesc.OutputWindow = Window::GetInstance().m_hWindow;              // Window's id
    SwapChainDesc.Windowed = (Window::GetDisplayMode() != Fullscreen);         // Window's display mode
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;                  // Discards the framebuffer's content after swaps
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;              // Changes the monitor's resolution when in fullscreen mode

    // Creates a DXGI's swap chain
    if FAILED(DXGIFactory->CreateSwapChain(m_D3D11Device, &SwapChainDesc, &m_DXGISwapChain))
    {
        Debug::Message(Error, "Failed to create the DXGI's swap chain.");
        return false;
    }

    // Blocks "Alt + Enter" hotkey (Fullscreen)
    if FAILED(DXGIFactory->MakeWindowAssociation(Window::GetInstance().m_hWindow, DXGI_MWA_NO_ALT_ENTER))
    {
        Debug::Message(Error, "Failed to associate the swap chain to the window.");
        return false;
    }

    // Gets the D3D11's backbuffer's surface of a swap chain
    ID3D11Texture2D* D3D11BackBuffer = nullptr;
    if FAILED(m_DXGISwapChain->GetBuffer(0, UUID(D3D11BackBuffer), reinterpret_cast<void**>(&D3D11BackBuffer)))
    {
        Debug::Message(Error, "Failed to get the D3D11's backbuffer.");
        return false;
    }

    // Creates a render target view for the backbuffer
    if FAILED(m_D3D11Device->CreateRenderTargetView(D3D11BackBuffer, nullptr, &m_D3D11RenderTargetView))
    {
        Debug::Message(Error, "Failed to create the D3D11's render target view.");
        return false;
    }

    // Links a D3D11's render target view to the output merger's state
    m_D3D11Context->OMSetRenderTargets(1, &m_D3D11RenderTargetView, nullptr);

    // Set-up the viewport
    m_D3D11Viewport.TopLeftX = 0;
    m_D3D11Viewport.TopLeftY = 0;
    m_D3D11Viewport.Width = static_cast<float>(Window::GetSize()[0]);
    m_D3D11Viewport.Height = static_cast<float>(Window::GetSize()[1]);
    m_D3D11Viewport.MinDepth = 0.0f;
    m_D3D11Viewport.MaxDepth = 1.0f;

    // Links the viewport to the rasterizer's state
    m_D3D11Context->RSSetViewports(1, &m_D3D11Viewport);

    // Set-up the blend between transparent objects
    D3D11_BLEND_DESC BlendDesc = { 0 };
    BlendDesc.AlphaToCoverageEnable = false;                                // Discards sprite's border
    BlendDesc.IndependentBlendEnable = false;                               // Uniform blend for all render targets
    BlendDesc.RenderTarget[0].BlendEnable = true;                           // Enables blending
    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;             // Source blending factor
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;        // RGB mix destination is alpha inverted
    BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;                 // Addition operation in color mixing
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;              // Alpha blend font is the pixel shader's alpha
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;            // Alpha mix destination is the inverted alpha
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;            // Addition operation in color mixing
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;                 // Components of each pixel that can be overwritten

    // Creates a D3D11's blend state
    if FAILED(m_D3D11Device->CreateBlendState(&BlendDesc, &m_D3D11BlendState))
    {
        Debug::Message(Error, "Failed to create the D3D11's blend state.");
        return false;
    }

    // Links a blend state to the output merger's state
    m_D3D11Context->OMSetBlendState(m_D3D11BlendState, nullptr, 0xffffffff);

    // Release everything got
    ReleaseCOM(DXGIDevice);
    ReleaseCOM(DXGIAdapter);
    ReleaseCOM(DXGIFactory);
    ReleaseCOM(D3D11BackBuffer);

    // Returns true if succeed
    return true;
}

void owl::Graphics::Finalize()
{
    // Release everything got
    ReleaseCOM(m_D3D11BlendState);
    ReleaseCOM(m_D3D11RenderTargetView);

    if (m_DXGISwapChain)
    {
        // D3D can't close when in fullscreen mode
        m_DXGISwapChain->SetFullscreenState(false, nullptr);
        ReleaseCOM(m_DXGISwapChain);
    }

    if (m_D3D11Context)
    {
        // Restores to original state
        m_D3D11Context->ClearState();
        ReleaseCOM(m_D3D11Context);
    }

    ReleaseCOM(m_D3D11Device);
}
