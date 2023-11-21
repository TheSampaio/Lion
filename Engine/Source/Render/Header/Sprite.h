#pragma once

namespace Lion
{
	class Sprite
	{
    public:
        LION_API Sprite(std::string FilePath);
        LION_API Sprite(const class Texture* Texture);
        LION_API ~Sprite();

        // === MAIN methods ======

        // Draws the sprite
        void LION_API Draw(float X, float Y, float Z = 0.50f);

        // === GET methods ======

        // Gets sprites's size
        std::array<uint, 2> LION_API GetSize();

        // Gets sprites's center
        std::array<uint, 2> LION_API GetCenter();

    private:
        // Attributes
        struct Sinfo* m_Sinfo;
        bool m_bLocalImage;
        const class Texture* m_Texture;
	};
}
