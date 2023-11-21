#pragma once

namespace Lion
{
	class Texture
	{
	public:
		LION_API Texture(std::string FilePath);
		LION_API ~Texture();

		// === GET methods ======

		// Gets the textures's size
		std::array<uint, 2> GetSize() const { return m_Size; }

		// Gets the textures's center
		std::array<uint, 2> GetCenter() const { return m_Center; }

		// === Friends ======

		friend class Sprite;

	private:
		// Attributes
		ID3D11ShaderResourceView* m_TextureView;
		std::array<uint, 2> m_Size;
		std::array<uint, 2> m_Center;

		// GET methods
		ID3D11ShaderResourceView* GetResourceView() const { return m_TextureView; }
	};
}
