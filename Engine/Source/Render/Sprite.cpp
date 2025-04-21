#include "Engine.h"
#include "Sprite.h"

#include "Renderer.h"
#include "Texture.h"

#include "../Core/Log.h"
#include "../Math/Transform.h"
#include "../Type/Depth.h"

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

    void Sprite::Draw(const Reference<Transform> transform)
    {
        const auto& size = mTexture->GetSize();

        mSpriteInfo->position = transform->mPosition;
        mSpriteInfo->rotation = transform->mRotation;
        mSpriteInfo->scale = transform->mScale;
        mSpriteInfo->size = size;
        mSpriteInfo->texture = mTexture->GetId();

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
