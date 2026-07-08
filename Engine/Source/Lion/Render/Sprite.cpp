#include "Engine.h"
#include "Sprite.h"

#include <Lion/Render/Renderer.h>
#include <Lion/Render/Texture.h>

#include <Lion/Math/Transform.h>

namespace Lion
{
    Sprite::Sprite(const std::string& filePath)
        : mSpriteInfo(MakeScope<SpriteInfo>()),
        mTexture(Texture::Create(filePath))
    {
        mSpriteInfo->texture = mTexture.get();
    }

    Sprite::Sprite(const Reference<Texture>& texture)
        : mSpriteInfo(MakeScope<SpriteInfo>()),
        mTexture(texture)
    {
        mSpriteInfo->texture = mTexture.get();
    }

    void Sprite::Draw(const Reference<Transform>& transform, int32 entityId)
    {
        mSpriteInfo->position = transform->mPosition;
        mSpriteInfo->rotation = transform->mRotation;
        mSpriteInfo->scale = transform->mScale;
        mSpriteInfo->size = mTexture->GetSize();
        mSpriteInfo->texture = mTexture.get();
        mSpriteInfo->entityId = entityId;

        Renderer::Submit(mSpriteInfo.get());
    }

    Size Sprite::GetSize()
    {
        return mTexture->GetSize();
    }

    Size Sprite::GetCenter()
    {
        return mTexture->GetCenter();
    }
}
