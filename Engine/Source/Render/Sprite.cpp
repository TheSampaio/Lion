#include "Engine.h"
#include "Header/Sprite.h"

#include "Header/Renderer.h"
#include "Header/Texture.h"

const float owl::Layer::Front = 0.00f;
const float owl::Layer::Upper = 0.25f;
const float owl::Layer::Middle = 0.50f;
const float owl::Layer::Lower = 0.75f;
const float owl::Layer::Back = 0.99f;

owl::Sprite::Sprite(std::string FilePath)
{
    m_Texture = new Texture(FilePath);
    m_bLocalImage = true;

    m_Sprite.Texture = m_Texture->GetResourceView();
}

owl::Sprite::Sprite(const Texture* Texture)
{
    m_Texture = Texture;
    m_bLocalImage = false;

    m_Sprite.Texture = m_Texture->GetResourceView();
}

owl::Sprite::~Sprite()
{
    if (m_bLocalImage) delete m_Texture;
}

void owl::Sprite::Draw(float X, float Y, float Z)
{
    m_Sprite.X = X;
    m_Sprite.Y = Y;
    m_Sprite.Scale = 1.0f;
    m_Sprite.Depth = Z;
    m_Sprite.Rotation = 0.0f;
    m_Sprite.Width = m_Texture->GetSize()[0];
    m_Sprite.Height = m_Texture->GetSize()[1];

    Renderer::GetInstance().Draw(&m_Sprite);
}

std::array<uint, 2> owl::Sprite::GetSize()
{
    std::array<uint, 2> Size = { m_Texture->GetSize()[0], m_Texture->GetSize()[1] };
    return Size;
}
