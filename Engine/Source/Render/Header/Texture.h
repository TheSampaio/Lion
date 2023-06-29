#pragma once

namespace owl
{
	class OWL_API Texture
	{
	public:
		Texture(std::string FilePath);
		~Texture();

		ID3D11ShaderResourceView* GetResourceView() const { return m_TextureView; }
		std::array<uint, 2> GetSize() const { return m_Size; }

	private:
		ID3D11ShaderResourceView* m_TextureView;
		std::array<uint, 2> m_Size;
	};
}
