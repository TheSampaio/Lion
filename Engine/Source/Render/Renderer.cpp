#include "Engine.h"
#include "Header/Renderer.h"

#include "Header/Graphics.h"
#include "Header/Sprite.h"
#include "../Core/Header/Window.h"
#include "../Event/Header/Debug.h"

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Colour;
    DirectX::XMFLOAT2 Texture;
};

// ---------------------------------------------------------------------------------

owl::Renderer::Renderer()
    : m_D3D11InputLayout(nullptr), m_D3D11VertexShader(nullptr), m_D3D11PixelShader(nullptr), m_D3D11RasterizerState(nullptr), m_D3D11SamplerState(nullptr),
      m_D3D11VertexBuffer(nullptr), m_D3D11IndexBuffer(nullptr), m_D3D11ConstantBuffer(nullptr)
{
    m_VertexBufferPosition = 0;
}

bool owl::Renderer::Initialize()
{
    //-------------------------------
    // Vertex Shader
    //-------------------------------

    // carrega bytecode do vertex shader (HLSL)
    ID3DBlob* D3DVertexShader = nullptr;
    if FAILED(D3DReadFileToBlob(L"Shader/DefaultVertex.cso", &D3DVertexShader))
    {
        Debug::Message(Error, "Failed to read the vertex shader.");
        return false;
    }

    // cria o vertex shader
    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateVertexShader(D3DVertexShader->GetBufferPointer(), D3DVertexShader->GetBufferSize(), nullptr, &m_D3D11VertexShader))
    {
        Debug::Message(Error, "Failed to compile the vertex shader.");
        return false;
    }

    //-------------------------------
    // Input Layout
    //-------------------------------

    // descreve o input layout dos vértices
    D3D11_INPUT_ELEMENT_DESC InputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // cria o input layout
    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateInputLayout(InputElementDesc, 3, D3DVertexShader->GetBufferPointer(), D3DVertexShader->GetBufferSize(), &m_D3D11InputLayout))
    {
        Debug::Message(Error, "Failed to create the input layout.");
        return false;
    }

    // libera bytecode
    D3DVertexShader->Release();

    //-------------------------------
    // Pixel Shader
    //-------------------------------

    // carrega bytecode do pixel shader (HLSL)
    ID3DBlob* D3DPixelShader = nullptr;
    if FAILED(D3DReadFileToBlob(L"Shader/DefaultPixel.cso", &D3DPixelShader))
    {
        Debug::Message(Error, "Failed to read the pixel shader.");
        return false;
    }

    // cria o pixel shader
    if FAILED(Graphics::GetInstance().m_D3D11Device->CreatePixelShader(D3DPixelShader->GetBufferPointer(), D3DPixelShader->GetBufferSize(), nullptr, &m_D3D11PixelShader))
    {
        Debug::Message(Error, "Failed to compile the pixel shader.");
        return false;
    }

    // libera bytecode
    D3DPixelShader->Release();

    //-------------------------------
    // Rasterizador
    //-------------------------------

    D3D11_RASTERIZER_DESC D3D11RasterizerDesc = {};
    D3D11RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    //rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    D3D11RasterizerDesc.CullMode = D3D11_CULL_NONE;
    D3D11RasterizerDesc.DepthClipEnable = true;

    // cria estado do rasterizador
    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateRasterizerState(&D3D11RasterizerDesc, &m_D3D11RasterizerState))
    {
        Debug::Message(Error, "Failed to create the rasterizer shader.");
        return false;
    }

    //-------------------------------
    // Vertex Buffer
    //-------------------------------

    D3D11_BUFFER_DESC D3D11VertexBufferDesc = {};
    D3D11VertexBufferDesc.ByteWidth = sizeof(Vertex) * m_VerticesPerSprite * m_MaxBatchSize;
    D3D11VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    D3D11VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateBuffer(&D3D11VertexBufferDesc, nullptr, &m_D3D11VertexBuffer))
    {
        Debug::Message(Error, "Failed to create the vertex buffer.");
        return false;
    }

    //-------------------------------
    // Index Buffer
    //-------------------------------

    D3D11_BUFFER_DESC D3D11IndexBufferDesc = { 0 };
    D3D11IndexBufferDesc.ByteWidth = sizeof(short) * m_IndicesPerSprite * m_MaxBatchSize;
    D3D11IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    D3D11IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    // gera índices para o número máximo de sprites suportados
    std::vector<short> Indices;
    Indices.reserve(static_cast<std::vector<short, std::allocator<short>>::size_type>(m_MaxBatchSize)* m_IndicesPerSprite);

    for (short i = 0; i < m_MaxBatchSize * m_VerticesPerSprite; i += m_VerticesPerSprite)
    {
        Indices.push_back(i);
        Indices.push_back(i + 1);
        Indices.push_back(i + 2);
        Indices.push_back(i + 1);
        Indices.push_back(i + 3);
        Indices.push_back(i + 2);
    }

    D3D11_SUBRESOURCE_DATA D3D11IndexData = { 0 };
    D3D11IndexData.pSysMem = &Indices.front();

    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateBuffer(&D3D11IndexBufferDesc, &D3D11IndexData, &m_D3D11IndexBuffer))
    {
        Debug::Message(Error, "Failed to create the index buffer.");
        return false;
    }

    //-------------------------------
    // Constant Buffer
    //-------------------------------

    D3D11_BUFFER_DESC D3D11ConstantBufferDesc = { 0 };
    D3D11ConstantBufferDesc.ByteWidth = sizeof(DirectX::XMMATRIX);
    D3D11ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    D3D11ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    // calcula a matriz de transformação
    float xScale = (Graphics::GetInstance().m_D3D11Viewport.Width > 0) ? 2.0f / Graphics::GetInstance().m_D3D11Viewport.Width : 0.0f;
    float yScale = (Graphics::GetInstance().m_D3D11Viewport.Height > 0) ? 2.0f / Graphics::GetInstance().m_D3D11Viewport.Height : 0.0f;

    // transforma para coordenadas da tela
    DirectX::XMMATRIX TransformMatrix
    (
        xScale, 0, 0, 0,
        0, -yScale, 0, 0,
        0, 0, 1, 0,
        -1, 1, 0, 1
    );

    D3D11_SUBRESOURCE_DATA D3D11ConstantData = { 0 };
    DirectX::XMMATRIX WorldViewProjection = DirectX::XMMatrixTranspose(TransformMatrix);
    D3D11ConstantData.pSysMem = &WorldViewProjection;

    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateBuffer(&D3D11ConstantBufferDesc, &D3D11ConstantData, &m_D3D11ConstantBuffer))
    {
        Debug::Message(Error, "Failed to create the constant buffer.");
        return false;
    }

    //-------------------------------
    // Texture Sampler
    //-------------------------------

    D3D11_SAMPLER_DESC D3D11SamplerStateDesc = {};
    D3D11SamplerStateDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    D3D11SamplerStateDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    D3D11SamplerStateDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    D3D11SamplerStateDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    D3D11SamplerStateDesc.MipLODBias = 0.0f;
    D3D11SamplerStateDesc.MaxAnisotropy = (Graphics::GetInstance().m_D3D11Device->GetFeatureLevel() > D3D_FEATURE_LEVEL_9_1) ? 16 : 2;
    D3D11SamplerStateDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    D3D11SamplerStateDesc.BorderColor[0] = 0.0f;
    D3D11SamplerStateDesc.BorderColor[1] = 0.0f;
    D3D11SamplerStateDesc.BorderColor[2] = 0.0f;
    D3D11SamplerStateDesc.BorderColor[3] = 0.0f;
    D3D11SamplerStateDesc.MinLOD = 0;
    D3D11SamplerStateDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // cria o amostrador da textura
    if FAILED(Graphics::GetInstance().m_D3D11Device->CreateSamplerState(&D3D11SamplerStateDesc, &m_D3D11SamplerState))
    {
        Debug::Message(Error, "Failed to create the sampler state.");
        return false;
    }

    //-------------------------------
    // Configura Pipeline Direct3D 
    //-------------------------------

    uint VertexStride = sizeof(Vertex);
    uint VertexOffset = 0;

    Graphics::GetInstance().m_D3D11Context->IASetVertexBuffers(0, 1, &m_D3D11VertexBuffer, &VertexStride, &VertexOffset);
    Graphics::GetInstance().m_D3D11Context->IASetIndexBuffer(m_D3D11IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    Graphics::GetInstance().m_D3D11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics::GetInstance().m_D3D11Context->IASetInputLayout(m_D3D11InputLayout);
    Graphics::GetInstance().m_D3D11Context->VSSetShader(m_D3D11VertexShader, nullptr, 0);
    Graphics::GetInstance().m_D3D11Context->VSSetConstantBuffers(0, 1, &m_D3D11ConstantBuffer);
    Graphics::GetInstance().m_D3D11Context->PSSetShader(m_D3D11PixelShader, nullptr, 0);
    Graphics::GetInstance().m_D3D11Context->PSSetSamplers(0, 1, &m_D3D11SamplerState);
    Graphics::GetInstance().m_D3D11Context->RSSetState(m_D3D11RasterizerState);

    // inicialização bem sucedida
    return true;
}

void owl::Renderer::Finalize()
{
    // Releases all got components
    ReleaseCOM(m_D3D11ConstantBuffer);
    ReleaseCOM(m_D3D11IndexBuffer);
    ReleaseCOM(m_D3D11VertexBuffer);
    ReleaseCOM(m_D3D11SamplerState);
    ReleaseCOM(m_D3D11RasterizerState);
    ReleaseCOM(m_D3D11PixelShader);
    ReleaseCOM(m_D3D11VertexShader);
    ReleaseCOM(m_D3D11InputLayout);
}

// ---------------------------------------------------------------------------------

void owl::Renderer::RenderBatch(ID3D11ShaderResourceView* Texture, SpriteData** Sprites, uint Cont)
{
    // desenhe usando a seguinte textura
    Graphics::GetInstance().m_D3D11Context->PSSetShaderResources(0, 1, &Texture);

    while (Cont > 0)
    {
        // quantos sprites vamos desenhar
        uint BatchSize = Cont;

        // quantos sprites cabem no vertex buffer
        uint RemainingSpace = m_MaxBatchSize - m_VertexBufferPosition;

        // quantidade de sprite é maior do que o espaço disponível
        if (BatchSize > RemainingSpace)
        {
            // se o tamanho disponível é muito pequeno
            if (RemainingSpace < m_MinBatchSize)
            {
                // volte ao ínicio do buffer
                m_VertexBufferPosition = 0;
                BatchSize = (Cont < m_MaxBatchSize) ? Cont : m_MaxBatchSize;
            }
            else
            {
                // restrinja a quantidade de sprites pelo espaço sobrando
                BatchSize = RemainingSpace;
            }
        }

        // trava o vertex buffer para escrita
        D3D11_MAP D3D11MapType = (m_VertexBufferPosition == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
        D3D11_MAPPED_SUBRESOURCE D3D11MappedBuffer;
        Graphics::GetInstance().m_D3D11Context->Map(m_D3D11VertexBuffer, 0, D3D11MapType, 0, &D3D11MappedBuffer);

        // se posiciona dentro do vertex buffer
        Vertex* Vertices = static_cast<Vertex*>(D3D11MappedBuffer.pData) + m_VertexBufferPosition * m_VerticesPerSprite;

        // gera posições dos vértices de cada sprite que será desenhado nesse lote
        for (uint i = 0; i < BatchSize; ++i)
        {
            using namespace DirectX;

            // pega tamanho da textura
            XMVECTOR Size = XMVectorMergeXY(XMLoadInt(&Sprites[i]->Width), XMLoadInt(&Sprites[i]->Height));
            XMVECTOR TextureSize = XMConvertVectorUIntToFloat(Size, 0);
            XMVECTOR InverseTextureSize = XMVectorReciprocal(TextureSize);

            // organiza informações do sprite
            XMFLOAT2 PositionXY(Sprites[i]->X, Sprites[i]->Y);
            float Scale = Sprites[i]->Scale;
            XMFLOAT2 Center(0.0f, 0.0f);
            float Rotation = Sprites[i]->Rotation;
            float LayerDepth = Sprites[i]->Depth;

            // carrega informações do sprite em registros SIMD
            XMVECTOR Source = XMVectorSet(0, 0, 1, 1);
            XMVECTOR Destination = XMVectorPermute<0, 1, 4, 4>(XMLoadFloat2(&PositionXY), XMLoadFloat(&Scale));
            XMVECTOR Colour = XMVectorSet(1, 1, 1, 1);
            XMVECTOR OriginRotationDepth = XMVectorSet(Center.x, Center.y, Rotation, LayerDepth);

            // extrai os tamanhos de origem e destino em vetores separados
            XMVECTOR SourceSize = XMVectorSwizzle<2, 3, 2, 3>(Source);
            XMVECTOR DestinationSize = XMVectorSwizzle<2, 3, 2, 3>(Destination);

            // altera a escala do offset de origem pelo tamanho da fonte, tomando cuidado para evitar overflow se a região fonte for zero
            XMVECTOR IsZeroMask = XMVectorEqual(SourceSize, XMVectorZero());
            XMVECTOR NonZeroSourceSize = XMVectorSelect(SourceSize, g_XMEpsilon, IsZeroMask);

            XMVECTOR Origin = XMVectorDivide(OriginRotationDepth, NonZeroSourceSize);

            // converte a região fonte de texels para o formato de coordenadas de textura mod-1
            Origin *= InverseTextureSize;

            // se o tamanho de destino é relativo a região fonte, converte-o para pixels
            DestinationSize *= TextureSize;

            // calcula uma matriz de rotação 2x2
            XMVECTOR RotationMatrix1;
            XMVECTOR RotationMatrix2;

            if (Rotation != 0)
            {
                float Sin, Cos;

                XMScalarSinCos(&Sin, &Cos, Rotation);

                XMVECTOR SinV = XMLoadFloat(&Sin);
                XMVECTOR CosV = XMLoadFloat(&Cos);

                RotationMatrix1 = XMVectorMergeXY(CosV, SinV);
                RotationMatrix2 = XMVectorMergeXY(-SinV, CosV);
            }
            else
            {
                RotationMatrix1 = g_XMIdentityR0;
                RotationMatrix2 = g_XMIdentityR1;
            }

            // os quatro vértices do sprite são calculados a partir de transformações dessas posições unitárias
            static XMVECTORF32 CornerOffsets[m_VerticesPerSprite] =
            {
                { 0, 0 },
                { 1, 0 },
                { 0, 1 },
                { 1, 1 },
            };

            int MirrorBits = 0;

            // gere os quatro vértices 
            for (int i = 0; i < m_VerticesPerSprite; ++i)
            {
                // calcula posição
                XMVECTOR CornerOffset = (CornerOffsets[i] - Origin) * DestinationSize;

                // aplica matriz de rotação 2x2
                XMVECTOR Position1 = XMVectorMultiplyAdd(XMVectorSplatX(CornerOffset), RotationMatrix1, Destination);
                XMVECTOR Position2 = XMVectorMultiplyAdd(XMVectorSplatY(CornerOffset), RotationMatrix2, Position1);

                // insere componente z = depth
                XMVECTOR Position = XMVectorPermute<0, 1, 7, 6>(Position2, OriginRotationDepth);

                // Escreve posição como um Float4, mesmo sendo VertexPositionColor::position um XMFLOAT3.
                // Isso é mais rápido e inofensivo porque estamos apenas invalidando o primeiro elemento
                // do campo cor, que será imediatamente sobrescrito com seu valor correto.
                XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&Vertices[i].Position), Position);

                // insere a cor
                XMStoreFloat4(&Vertices[i].Colour, Colour);

                // computa e escreve as coordenadas da textura
                XMVECTOR textureCoordinate = XMVectorMultiplyAdd(CornerOffsets[i ^ MirrorBits], SourceSize, Source);

                XMStoreFloat2(&Vertices[i].Texture, textureCoordinate);
            }

            Vertices += m_VerticesPerSprite;
        }

        // destrava o vertex buffer
        Graphics::GetInstance().m_D3D11Context->Unmap(m_D3D11VertexBuffer, 0);

        // desenha sprites 
        uint StartIndex = static_cast<uint>(m_VertexBufferPosition) * m_IndicesPerSprite;
        uint IndexCount = static_cast<uint>(BatchSize) * m_IndicesPerSprite;
        Graphics::GetInstance().m_D3D11Context->DrawIndexed(IndexCount, StartIndex, 0);

        // avança a posição no vertex buffer
        m_VertexBufferPosition += BatchSize;

        // avança a posição no vetor de sprites
        Sprites += BatchSize;

        // foram desenhados batchSize sprites nessa passagem
        Cont -= BatchSize;
    }
}

// ---------------------------------------------------------------------------------

void owl::Renderer::Render()
{
    // ordena sprites por profundidade
    std::sort(m_SpriteVector.begin(), m_SpriteVector.end(),
        [](SpriteData* a, SpriteData* b) -> bool
        { return a->Depth > b->Depth; });

    // quantidades de sprites a serem renderizados
    uint SpriteVectorSize = uint(m_SpriteVector.size());

    if (SpriteVectorSize == 0)
        return;

    ID3D11ShaderResourceView* BatchTexture = nullptr;
    uint BatchStart = 0;

    // junta sprites adjacentes que compartilham a mesma textura
    for (uint Position = 0; Position < SpriteVectorSize; ++Position)
    {
        ID3D11ShaderResourceView* Texture = m_SpriteVector[Position]->Texture;

        if (Texture != BatchTexture)
        {
            if (Position > BatchStart)
            {
                RenderBatch(BatchTexture, &m_SpriteVector[BatchStart], Position - BatchStart);
            }

            BatchTexture = Texture;
            BatchStart = Position;
        }
    }

    // desenha o grupo final de sprites
    RenderBatch(BatchTexture, &m_SpriteVector[BatchStart], SpriteVectorSize - BatchStart);

    // limpa a lista de desenho (atualizada a cada frame)
    m_SpriteVector.clear();
}

// ---------------------------------------------------------------------------------

void owl::Renderer::Draw(SpriteData* Sprite)
{
    m_SpriteVector.push_back(Sprite);
}
