#include "Engine.h"
#include "Sprite.h"

#include "Renderer.h"
#include "SpriteInfo.h"
#include "Texture.h"


namespace Lion
{
    Sprite::Sprite(std::string filePath)
    {
        mTexture = new Texture(filePath);
        mLocalImage = true;

        mSpriteInfo = new SpriteInfo();
        mSpriteInfo->texture = mTexture->GetId();
    }

    Sprite::Sprite(const Texture* Texture)
    {
        mTexture = Texture;
        mLocalImage = false;

        mSpriteInfo = new SpriteInfo();
        mSpriteInfo->texture = mTexture->GetId();
    }

    Sprite::~Sprite()
    {
        if (mLocalImage)
            delete mTexture;

        delete mSpriteInfo;
    }

    void Sprite::Draw(float x, float y, float z)
    {
        mSpriteInfo->x = x;
        mSpriteInfo->y = y;
        mSpriteInfo->scale = 1.0f;
        mSpriteInfo->depth = z;
        mSpriteInfo->rotation = 0.0f;
        mSpriteInfo->width = mTexture->GetSize()[0];
        mSpriteInfo->height = mTexture->GetSize()[1];
        mSpriteInfo->texture = mTexture->GetId();

        Renderer::Submit(mSpriteInfo);
    }

    std::array<int, 2> Sprite::GetSize()
    {
        std::array<int, 2> Size = { mTexture->GetSize()[0], mTexture->GetSize()[1] };
        return Size;
    }

    std::array<int, 2> Sprite::GetCenter()
    {
        std::array<int, 2> Size = { mTexture->GetCenter()[0], mTexture->GetCenter()[1] };
        return Size;
    }
}
