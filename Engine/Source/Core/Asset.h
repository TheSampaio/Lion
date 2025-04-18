#pragma once

#include "../Render/Texture.h"

namespace Lion
{
	class Asset
	{
	public:
		Asset() = default;
		~Asset();

		Asset(const Asset&) = delete;
		Asset& operator=(const Asset&) = delete;

		static LION_API Reference<Texture> LoadTexture(const std::string& name);

		static LION_API Reference<Texture> LoadTexture(const std::string& name, const std::string& filepath);

		// TODO: LoadSound(), GetShader(), LoadFont() etc.

	private:
		static inline std::unordered_map<std::string, Reference<Texture>> sTextures;
	};
}
