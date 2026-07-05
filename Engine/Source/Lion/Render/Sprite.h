#pragma once

#include <Lion/Math/Vector.h>
#include <Lion/Type/Size.h>

namespace Lion
{
    class Texture;
    class Transform;

    struct SpriteInfo
    {
        Vector position = {};
        Vector rotation = {};
        Vector scale = {};
        Size size = {};
        Texture* texture = nullptr;  // Non-owning; the Sprite keeps the texture alive.
    };

    // Lightweight drawable: owns a texture and submits a textured quad to the Renderer.
    class Sprite
    {
    public:
        LION_API Sprite(const std::string& filePath);
        LION_API Sprite(const Reference<Texture>& texture);

        // Returns the sprite's pixel size.
        Size LION_API GetSize();

        // Returns the sprite's pixel center.
        Size LION_API GetCenter();

        // Submits the sprite to the Renderer using the given transform.
        void LION_API Draw(const Reference<Transform>& transform);

    private:
        // Attributes
        Scope<SpriteInfo> mSpriteInfo;
        Reference<Texture> mTexture;
    };
}