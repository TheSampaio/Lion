#pragma once

namespace owl
{
    struct OWL_API SpriteData
    {
        float X, Y;
        float Scale;
        float Depth;
        float Rotation;
        uint  Width;
        uint  Height;
        ID3D11ShaderResourceView* Texture;
    };

    struct OWL_API Layer
    {
        static const float Front;
        static const float Upper;
        static const float Middle;
        static const float Lower;
        static const float Back;
    };

	class OWL_API Sprite
	{
    public:
        Sprite(std::string FilePath);
        Sprite(const class Texture* Texture);
        ~Sprite();

        void Draw(float X, float Y, float Z = Layer::Middle);

        std::array<uint, 2> GetSize();

    private:
        SpriteData m_Sprite;
        bool m_bLocalImage;
        const class Texture* m_Texture;
	};
}
