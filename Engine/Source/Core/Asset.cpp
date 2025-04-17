#include "Engine.h"
#include "Asset.h"

#include "Log.h"

namespace Lion
{
	Asset::~Asset()
	{
		sTextures.clear();
	}

	Reference<Texture> Asset::LoadTexture(const std::string& name, const std::string& filepath)
	{
		auto it = sTextures.find(name);

		if (it != sTextures.end())
			return it->second;

		// Load and cache it
		auto texture = MakeReference<Texture>(filepath);
		sTextures[name] = texture;

		return texture;
	}
}
