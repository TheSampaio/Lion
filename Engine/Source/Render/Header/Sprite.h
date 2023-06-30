#pragma once

namespace owl
{
	class Sprite
	{
    public:
        OWL_API Sprite(std::string FilePath);
        OWL_API Sprite(const class Texture* Texture);
        OWL_API ~Sprite();

        void OWL_API Draw(float X, float Y, float Z = 0.50f);

        std::array<uint, 2> OWL_API GetSize();

    private:
        struct Sinfo* m_Sinfo;
        bool m_bLocalImage;
        const class Texture* m_Texture;
	};
}
