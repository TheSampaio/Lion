#pragma once

#include <Lion/Math/Vector.h>
#include <Lion/Type/Size.h>

namespace Lion
{
    struct SpriteInfo
    {
        Vector position = {};
        Vector rotation = {};
        Vector scale = {};
        Size size = {};
        uint32 texture = 0;
    };

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
        Size LION_API GetSize();

        // Gets sprites's center
        Size LION_API GetCenter();

        // Draws the sprite
        void LION_API Draw(const Reference<Transform> transform);

    private:
        // Attributes
        Scope<SpriteInfo> mSpriteInfo;
        Reference<Texture> mTexture;
    };
}