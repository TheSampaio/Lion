#include "Engine.h"
#include "Header/Sprite.h"

#include "Header/Renderer.h"
#include "Header/Texture.h"

#include "../Kind/Header/Sinfo.h"

Lion::Sprite::Sprite(std::string FilePath)
{
    m_Texture = new Texture(FilePath);
    m_bLocalImage = true;

    m_Sinfo = new Sinfo();
    m_Sinfo->Texture = m_Texture->GetResourceView();
}

Lion::Sprite::Sprite(const Texture* Texture)
{
    m_Texture = Texture;
    m_bLocalImage = false;

    m_Sinfo = new Sinfo();
    m_Sinfo->Texture = m_Texture->GetResourceView();
}

Lion::Sprite::~Sprite()
{
    if (m_bLocalImage) delete m_Texture;
    delete m_Sinfo;
}

void Lion::Sprite::Draw(float X, float Y, float Z)
{
    m_Sinfo->X = X;
    m_Sinfo->Y = Y;
    m_Sinfo->Scale = 1.0f;
    m_Sinfo->Depth = Z;
    m_Sinfo->Rotation = 0.0f;
    m_Sinfo->Width = m_Texture->GetSize()[0];
    m_Sinfo->Height = m_Texture->GetSize()[1];

    Renderer::GetInstance().Draw(m_Sinfo);
}

std::array<uint, 2> Lion::Sprite::GetSize()
{
    std::array<uint, 2> Size = { m_Texture->GetSize()[0], m_Texture->GetSize()[1] };
    return Size;
}

std::array<uint, 2>LION_API Lion::Sprite::GetCenter()
{
    std::array<uint, 2> Size = { m_Texture->GetCenter()[0], m_Texture->GetCenter()[1] };
    return Size;
}
