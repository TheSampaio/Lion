#include "Engine.h"
#include "Header/Texture.h"

#include "Header/Loader.h"
#include "Header/Graphics.h"
#include "../Event/Header/Debug.h"
#include "../Kind/Header/Tools.h"

owl::Texture::Texture(std::string FilePath)
	: m_TextureView(nullptr)
{
	m_Size = { 0, 0 };
	m_Center = { 0, 0 };

    if FAILED(D3D11CreateTextureFromFile(Graphics::GetInstance().m_D3D11Device, Graphics::GetInstance().m_D3D11Context, FilePath.c_str(), nullptr, &m_TextureView, m_Size[0], m_Size[1]))
    {
        std::string Message = std::string("Failed to load \"") + FilePath.c_str() + std::string("\". The sprite will not appear in the game.");
        Debug::Message(Warning, Message.c_str());
    }

    m_Center = { m_Size[0] / 2, m_Size[1] / 2 };
}

owl::Texture::~Texture()
{
    if (m_TextureView)
    {
        // Gets the texture's content
        ID3D11Resource* Resource = nullptr;
        m_TextureView->GetResource(&Resource);

        // Releases the texture and its content
        ReleaseCOM(Resource);
        ReleaseCOM(m_TextureView);
    }
}
