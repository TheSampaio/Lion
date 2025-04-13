#include "Engine.h"
#include "Sprite.h"

#include "Renderer.h"
#include "Texture.h"

#include "../Core/Log.h"

namespace Lion
{
    Sprite::Sprite(const std::string& filePath)
    {
        mTexture = MakeReference<Texture>(filePath);
        mSpriteInfo = MakeScope<SpriteInfo>();
        mSpriteInfo->texture = mTexture->GetId();
    }

    Sprite::Sprite(const Reference<Texture> texture)
        : mTexture(texture)
    {
        mSpriteInfo = MakeScope<SpriteInfo>();
        mSpriteInfo->texture = mTexture->GetId();
    }

    void Sprite::Draw(float32 x, float32 y, float32 z)
    {
        mSpriteInfo->x = x;
        mSpriteInfo->y = y;
        mSpriteInfo->scale = 1.0;
        mSpriteInfo->depth = z;
        mSpriteInfo->rotation = 0.0f;
        mSpriteInfo->width = mTexture->GetSize()[0];
        mSpriteInfo->height = mTexture->GetSize()[1];
        mSpriteInfo->texture = mTexture->GetId();

        Renderer::Submit(mSpriteInfo.get());
    }

    std::array<uint32, 2> Sprite::GetSize()
    {
        return mTexture->GetSize();
    }

    std::array<uint32, 2> Sprite::GetCenter()
    {
        return mTexture->GetCenter();
    }
}
