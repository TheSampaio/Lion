#pragma once

#include "SpriteInfo.h"

namespace Lion
{
    class Texture;
    class Transform;

    struct Depth;
    struct SpriteInfo;

    class Sprite
    {
    public:
        LION_API Sprite(const std::string& filePath);
        LION_API Sprite(const Reference<Texture> texture);

        // Gets sprites's size
        std::array<uint32, 2> LION_API GetSize();

        // Gets sprites's center
        std::array<uint32, 2> LION_API GetCenter();

        // Draws the sprite
        void LION_API Draw(const Reference<Transform> transform);

    private:
        // Attributes
        Scope<SpriteInfo> mSpriteInfo;
        Reference<Texture> mTexture;
    };
}