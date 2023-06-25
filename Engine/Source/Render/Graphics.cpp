#include "Engine.h"
#include "Header/Graphics.h"

#include "../Core/Header/Window.h"
#include "../Event/Header/Debug.h"

owl::Graphics::Graphics()
	: m_Device(nullptr), m_Context(nullptr), m_SwapChain(nullptr), m_RenderTargetView(nullptr), m_BlendState(nullptr)
{
    m_Viewport = { 0 };
	m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	m_VSyncMode = Disabled;

	m_BackgroundColour[0] = 0.0f;
	m_BackgroundColour[1] = 0.0f;
	m_BackgroundColour[2] = 0.0f;
	m_BackgroundColour[3] = 0.0f;
}

owl::Graphics::~Graphics()
{
    if (m_BlendState)
    {
        m_BlendState->Release();
        m_BlendState = nullptr;
    }

    if (m_RenderTargetView)
    {
        m_RenderTargetView->Release();
        m_RenderTargetView = nullptr;
    }

    // Direct3D can't close when in fullscreen mode
    if (m_SwapChain)
    {
        m_SwapChain->SetFullscreenState(false, nullptr);
        m_SwapChain->Release();
        m_SwapChain = nullptr;
    }

    if (m_Context)
    {
        m_Context->ClearState();
        m_Context->Release();
        m_Context = nullptr;
    }

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
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
            0,                        // adaptador de vídeo (NULL = adaptador padrão)
            D3D_DRIVER_TYPE_HARDWARE,       // tipo de driver D3D (Hardware, Reference ou Software)
            nullptr,                        // ponteiro para rasterizador em software
            CreateDeviceFlags,              // modo de depuração ou modo normal
            nullptr,                        // featureLevels do Direct3D (NULL = maior suportada)
            0,                              // tamanho do vetor featureLevels
            D3D11_SDK_VERSION,              // versão do SDK do Direct3D
            &m_Device,                      // guarda o dispositivo D3D criado
            &m_FeatureLevel,                // versão do Direct3D utilizada
            &m_Context))                    // contexto do dispositivo D3D

    {
        // sistema não suporta dispositivo D3D11
        // fazendo a criação de um WARP Device que 
        // implementa um rasterizador em software

        Debug::Message(Error, "Failed to create HAL device.");

        if FAILED(
            D3D11CreateDevice(0, D3D_DRIVER_TYPE_WARP, nullptr, CreateDeviceFlags, nullptr, 0, D3D11_SDK_VERSION, &m_Device, &m_FeatureLevel, &m_Context))
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
    if FAILED(m_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice))
    {
        Debug::Message(Error, "Failed to create the DXGI device.");
        return false;
    }

    // cria objeto para adaptador de vídeo (placa gráfica)
    IDXGIAdapter* DXGIAdapter = nullptr;
    if FAILED(DXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&DXGIAdapter))
    {
        Debug::Message(Error, "Failed to create the DXGI adapter.");
        return false;
    }

    // cria objeto para a fábrica DXGI
    IDXGIFactory* DXGIFactory = nullptr;
    if FAILED(DXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&DXGIFactory))
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
    if FAILED(DXGIFactory->CreateSwapChain(m_Device, &SwapChainDesc, &m_SwapChain))
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
    ID3D11Texture2D* BackBuffer = nullptr;
    if FAILED(m_SwapChain->GetBuffer(0, __uuidof(BackBuffer), (void**)(&BackBuffer)))
    {
        Debug::Message(Error, "Failed to get the backbuffer.");
        return false;
    }

    // cria uma render-target view do backbuffer
    if FAILED(m_Device->CreateRenderTargetView(BackBuffer, nullptr, &m_RenderTargetView))
    {
        Debug::Message(Error, "Failed to create the render target view.");
        return false;
    }

    // liga uma render-target ao estágio output-merger
    m_Context->OMSetRenderTargets(1, &m_RenderTargetView, nullptr);

    // -------------------------------
    // Viewport / Rasterizer
    // -------------------------------

    // configura uma viewport
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = static_cast<float>(Window::GetSize()[0]);
    m_Viewport.Height = static_cast<float>(Window::GetSize()[1]);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    // liga a viewport ao estágio de rasterização
    m_Context->RSSetViewports(1, &m_Viewport);

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
    if FAILED(m_Device->CreateBlendState(&BlendDesc, &m_BlendState))
    {
        Debug::Message(Error, "Failed to create the render state.");
        return false;
    }

    // liga a blend state ao estágio Output-Merger
    m_Context->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);

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

    if (BackBuffer)
    {
        BackBuffer->Release();
        BackBuffer = nullptr;
    }

    // inicialização bem sucedida
    return true;
}
