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

owl::Graphics::~Graphics()
{
    if (m_D3D11BlendState)
    {
        m_D3D11BlendState->Release();
        m_D3D11BlendState = nullptr;
    }

    if (m_D3D11RenderTargetView)
    {
        m_D3D11RenderTargetView->Release();
        m_D3D11RenderTargetView = nullptr;
    }

    // Direct3D can't close when in fullscreen mode
    if (m_DXGISwapChain)
    {
        m_DXGISwapChain->SetFullscreenState(false, nullptr);
        m_DXGISwapChain->Release();
        m_DXGISwapChain = nullptr;
    }

    if (m_D3D11Context)
    {
        m_D3D11Context->ClearState();
        m_D3D11Context->Release();
        m_D3D11Context = nullptr;
    }

    if (m_D3D11Device)
    {
        m_D3D11Device->Release();
        m_D3D11Device = nullptr;
    }
}

bool owl::Graphics::Initialize()
{
    // -------------------------------
    // Dispositivo Direct3D
    // -------------------------------

    uint CreateDeviceFlags = 0;

#ifdef WL_DEBUG
    // exibe mensagens de erro do Direct3D em modo de depuração
    CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // cria objeto para acessar dispositivo Direct3D
    if FAILED(
        D3D11CreateDevice(
            0,                              // adaptador de vídeo (NULL = adaptador padrão)
            D3D_DRIVER_TYPE_HARDWARE,       // tipo de driver D3D (Hardware, Reference ou Software)
            nullptr,                        // ponteiro para rasterizador em software
            CreateDeviceFlags,              // modo de depuração ou modo normal
            nullptr,                        // featureLevels do Direct3D (NULL = maior suportada)
            0,                              // tamanho do vetor featureLevels
            D3D11_SDK_VERSION,              // versão do SDK do Direct3D
            &m_D3D11Device,                 // guarda o dispositivo D3D criado
            &m_D3DFeatureLevel,             // versão do Direct3D utilizada
            &m_D3D11Context))               // contexto do dispositivo D3D

    {
        // sistema não suporta dispositivo D3D11
        // fazendo a criação de um WARP Device que 
        // implementa um rasterizador em software

        Debug::Message(Error, "Failed to create HAL device.");

        if FAILED(
            D3D11CreateDevice(0, D3D_DRIVER_TYPE_WARP, nullptr, CreateDeviceFlags, nullptr, 0, D3D11_SDK_VERSION, &m_D3D11Device, &m_D3DFeatureLevel, &m_D3D11Context))
            return false;

        Debug::Message(Warning, "There is no support for D3D11. Now you are using the WARP adapter.");
    }

    // -------------------------------
    // Cor de Fundo do Direct3D
    // -------------------------------

    // ajusta a cor de fundo do backbuffer
    // para a mesma cor de fundo da janela
    COLORREF Colour = Window::GetBackgroundColour();

    m_BackgroundColour[0] = GetRValue(Colour) / 255.0f; // Red
    m_BackgroundColour[1] = GetGValue(Colour) / 255.0f; // Green
    m_BackgroundColour[2] = GetBValue(Colour) / 255.0f; // Blue
    m_BackgroundColour[3] = 1.0f;                       // Alpha (1 = cor sólida)

    // -------------------------------
    // Interfaces DXGI
    // -------------------------------

    // cria objeto para a infraestrutura gráfica
    IDXGIDevice* DXGIDevice = nullptr;
    if FAILED(m_D3D11Device->QueryInterface(UID(IDXGIDevice), reinterpret_cast<void**>(&DXGIDevice)))
    {
        Debug::Message(Error, "Failed to create the DXGI device.");
        return false;
    }

    // cria objeto para adaptador de vídeo (placa gráfica)
    IDXGIAdapter* DXGIAdapter = nullptr;
    if FAILED(DXGIDevice->GetParent(UID(IDXGIAdapter), reinterpret_cast<void**>(&DXGIAdapter)))
    {
        Debug::Message(Error, "Failed to create the DXGI adapter.");
        return false;
    }

    // cria objeto para a fábrica DXGI
    IDXGIFactory* DXGIFactory = nullptr;
    if FAILED(DXGIAdapter->GetParent(UID(IDXGIFactory), reinterpret_cast<void**>(&DXGIFactory)))
    {
        Debug::Message(Error, "Failed to create the DXGI factory.");
        return false;
    }

    // -------------------------------
    // Swap Chain 
    // -------------------------------

    // descrição de uma swap chain
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = { 0 };
    SwapChainDesc.BufferDesc.Width = static_cast<uint>(Window::GetSize()[0]);  // largura do backbuffer
    SwapChainDesc.BufferDesc.Height = static_cast<uint>(Window::GetSize()[1]); // altura do backbuffer
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;                       // taxa de atualização em hertz 
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;                      // numerador é um inteiro e não um racional
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;              // formato de cores RGBA 8 bits
    SwapChainDesc.SampleDesc.Count = 1;                                        // amostras por pixel (antialiasing)
    SwapChainDesc.SampleDesc.Quality = 0;                                      // nível de qualidade da imagem
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;               // utilize superfície como RENDER-TARGET
    SwapChainDesc.BufferCount = 2;                                             // número de buffers (front + back)
    SwapChainDesc.OutputWindow = Window::GetInstance().m_hWindow;              // identificador da janela
    SwapChainDesc.Windowed = (Window::GetDisplayMode() != Fullscreen);         // modo janela ou tela cheia
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;                  // descarta superfície após apresentação
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;              // muda a resolução do monitor em tela cheia

    // cria uma swap chain
    if FAILED(DXGIFactory->CreateSwapChain(m_D3D11Device, &SwapChainDesc, &m_DXGISwapChain))
    {
        Debug::Message(Error, "Failed to create the swap chain.");
        return false;
    }

    // impede a DXGI de monitorar ALT-ENTER e alternar entre windowed/fullscreen
    if FAILED(DXGIFactory->MakeWindowAssociation(Window::GetInstance().m_hWindow, DXGI_MWA_NO_ALT_ENTER))
    {
        Debug::Message(Error, "Failed to associate the swap chain to the window.");
        return false;
    }

    // -------------------------------
    // Render Target
    // -------------------------------

    // pega a superfície backbuffer de uma swapchain
    ID3D11Texture2D* D3D11BackBuffer = nullptr;
    if FAILED(m_DXGISwapChain->GetBuffer(0, UID(D3D11BackBuffer), reinterpret_cast<void**>(&D3D11BackBuffer)))
    {
        Debug::Message(Error, "Failed to get the backbuffer.");
        return false;
    }

    // cria uma render-target view do backbuffer
    if FAILED(m_D3D11Device->CreateRenderTargetView(D3D11BackBuffer, nullptr, &m_D3D11RenderTargetView))
    {
        Debug::Message(Error, "Failed to create the render target view.");
        return false;
    }

    // liga uma render-target ao estágio output-merger
    m_D3D11Context->OMSetRenderTargets(1, &m_D3D11RenderTargetView, nullptr);

    // -------------------------------
    // Viewport / Rasterizer
    // -------------------------------

    // configura uma viewport
    m_D3D11Viewport.TopLeftX = 0;
    m_D3D11Viewport.TopLeftY = 0;
    m_D3D11Viewport.Width = static_cast<float>(Window::GetSize()[0]);
    m_D3D11Viewport.Height = static_cast<float>(Window::GetSize()[1]);
    m_D3D11Viewport.MinDepth = 0.0f;
    m_D3D11Viewport.MaxDepth = 1.0f;

    // liga a viewport ao estágio de rasterização
    m_D3D11Context->RSSetViewports(1, &m_D3D11Viewport);

    // ---------------------------------------------
    // Blend State
    // ---------------------------------------------

    // Equação de mistura de cores (blending):
    // finalColor = SrcColor * SrcBlend <OP> DestColor * DestBlend

    // Combinando superfícies transparentes (Alpha Blending)
    // finalColor = SrcColor * ScrAlpha + DestColor * (1-SrcAlpha)

    D3D11_BLEND_DESC BlendDesc = { 0 };
    BlendDesc.AlphaToCoverageEnable = false;                                // destaca a silhueta dos sprites
    BlendDesc.IndependentBlendEnable = false;                               // usa mesma mistura para todos os render targets
    BlendDesc.RenderTarget[0].BlendEnable = true;                           // habilita o blending
    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;             // fator de mistura da fonte 
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;        // destino da mistura RGB é o alpha invertido 
    BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;                 // operação de adição na mistura de cores
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;              // fonte da mistura Alpha é o alpha do pixel shader
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;            // destino da mistura Alpha é o alpha invertido
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;            // operação de adição na mistura de cores
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;                 // componentes de cada pixel que podem ser sobrescritos

    // cria a blend state
    if FAILED(m_D3D11Device->CreateBlendState(&BlendDesc, &m_D3D11BlendState))
    {
        Debug::Message(Error, "Failed to create the render state.");
        return false;
    }

    // liga a blend state ao estágio Output-Merger
    m_D3D11Context->OMSetBlendState(m_D3D11BlendState, nullptr, 0xffffffff);

    // -------------------------------
    // Libera interfaces DXGI
    // -------------------------------

    if (DXGIDevice)
    {
        DXGIDevice->Release();
        DXGIDevice = nullptr;
    }

    if (DXGIAdapter)
    {
        DXGIAdapter->Release();
        DXGIAdapter = nullptr;
    }

    if (DXGIFactory)
    {
        DXGIFactory->Release();
        DXGIFactory = nullptr;
    }

    if (D3D11BackBuffer)
    {
        D3D11BackBuffer->Release();
        D3D11BackBuffer = nullptr;
    }

    // inicialização bem sucedida
    return true;
}
