#pragma once

#include "SpriteInfo.h"
#include "../Kind/Depth.h"

namespace Lion
{
    class Texture;
    struct Depth;

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
        void LION_API Draw(float32 x, float32 y, float32 z = Depth::Middle);

    private:
        // Attributes
        Scope<SpriteInfo> mSpriteInfo;
        Reference<Texture> mTexture;
    };
}