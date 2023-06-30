#pragma once

namespace owl
{
	class Texture
	{
	public:
		OWL_API Texture(std::string FilePath);
		OWL_API ~Texture();

		ID3D11ShaderResourceView* GetResourceView() const { return m_TextureView; }
		std::array<uint, 2> GetSize() const { return m_Size; }

	private:
		ID3D11ShaderResourceView* m_TextureView;
		std::array<uint, 2> m_Size;
	};
}
