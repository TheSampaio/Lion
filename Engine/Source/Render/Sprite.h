#pragma once

namespace Lion
{
    class Texture;
    struct SpriteInfo;

    class Sprite
    {
    public:
        LION_API Sprite(std::string filePath);
        LION_API Sprite(const Texture* texture);
        LION_API ~Sprite();

        // Gets sprites's size
        std::array<int, 2> LION_API GetSize();

        // Gets sprites's center
        std::array<int, 2> LION_API GetCenter();

        // Draws the sprite
        void LION_API Draw(float x, float y, float z = 0.50f);

    private:
        // Attributes
        struct SpriteInfo* mSpriteInfo;
        const class Texture* mTexture;
        bool mLocalImage;
    };
}