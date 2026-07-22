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
        int32 entityId = -1;         // Owner entity id for editor picking (-1 = none).

        // What goes on top of what. Sprites are drawn in this order, low to high, and sprites sharing
        // one are drawn in the order the scene submitted them — which is the order the Hierarchy shows,
        // so moving a row down the list moves it in front. Setting it is overriding that.
        int32 order = 0;

        // Mirroring, which a sprite does by reading its texture backwards rather than by taking a
        // negative scale: a scale of -1 also flips the collider, the children and the maths.
        bool flipX = false;
        bool flipY = false;
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

        // Submits the sprite to the Renderer using the given transform. The optional entityId is
        // forwarded to the renderer for editor picking (-1 means "not associated with an entity").
        void LION_API Draw(const Reference<Transform>& transform, int32 entityId = -1);

        // Submits the sprite using an explicit world-space transform (used by SpriteRenderer, which
        // resolves the owner's transform through the parent chain).
        void LION_API Draw(const Vector& position, const Vector& rotation, const Vector& scale, int32 entityId = -1);

        // How this sprite is drawn, as opposed to where: what it goes on top of, and which way round it
        // is read. Set before Draw; they travel with the submission.
        void LION_API SetOrder(int32 order);
        void LION_API SetFlip(bool flipX, bool flipY);

    private:
        // Attributes
        Scope<SpriteInfo> mSpriteInfo;
        Reference<Texture> mTexture;
    };
}