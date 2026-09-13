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
        mSpriteInfo->size = mTexture->GetSize();
    }

    Sprite::Sprite(const Reference<Texture>& texture)
        : mSpriteInfo(MakeScope<SpriteInfo>()),
        mTexture(texture)
    {
        mSpriteInfo->texture = mTexture.get();
        mSpriteInfo->size = mTexture->GetSize();
    }

    void Sprite::Draw(const Reference<Transform>& transform, int32 entityId)
    {
        Draw(transform->mPosition, transform->mRotation, transform->mScale, entityId);
    }

    void Sprite::Draw(const Vector& position, const Vector& rotation, const Vector& scale, int32 entityId)
    {
        mSpriteInfo->position = position;
        mSpriteInfo->rotation = rotation;
        mSpriteInfo->scale = scale;
        mSpriteInfo->texture = mTexture.get();
        mSpriteInfo->entityId = entityId;

        Renderer::Submit(mSpriteInfo.get());
    }

    void Sprite::SetOrder(int32 order)
    {
        mSpriteInfo->order = order;
    }

    void Sprite::SetFlip(bool flipX, bool flipY)
    {
        mSpriteInfo->flipX = flipX;
        mSpriteInfo->flipY = flipY;
    }

    void Sprite::SetRegion(const Vector& uvMinimum, const Vector& uvMaximum, const Size& size)
    {
        mSpriteInfo->uvMinimum = uvMinimum;
        mSpriteInfo->uvMaximum = uvMaximum;
        mSpriteInfo->size = size;
    }

    void Sprite::SetColor(const Vector& color)
    {
        mSpriteInfo->color = color;
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
