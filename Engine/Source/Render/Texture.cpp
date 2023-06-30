#include "Engine.h"
#include "Header/Texture.h"

#include "Header/Loader.h"
#include "Header/Graphics.h"
#include "../Kind/Header/Tools.h"

owl::Texture::Texture(std::string FilePath)
	: m_TextureView(nullptr)
{
	m_Size = { 0, 0 };

    D3D11CreateTextureFromFile(
        Graphics::GetInstance().m_D3D11Device,
        Graphics::GetInstance().m_D3D11Context,
        FilePath.c_str(),
        nullptr,
        &m_TextureView,
        m_Size[0],
        m_Size[1]);
}

owl::Texture::~Texture()
{
    if (m_TextureView)
    {
        ID3D11Resource* Resource = nullptr;
        m_TextureView->GetResource(&Resource);

        ReleaseCOM(Resource);
        ReleaseCOM(m_TextureView);
    }
}
