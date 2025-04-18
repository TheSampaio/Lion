#include "Engine.h"
#include "Asset.h"

#include "Log.h"

namespace Lion
{
	Asset::~Asset()
	{
		sTextures.clear();
	}

	Reference<Texture> Asset::LoadTexture(const std::string& name)
	{
		auto it = sTextures.find(name);

		if (it != sTextures.end())
			return it->second;

		Log::Console(LogLevel::Error, LN_LOG_FORMAT("[Asset] Texture '{}' not found in cache.", name));
		return nullptr;
	}

	Reference<Texture> Asset::LoadTexture(const std::string& name, const std::string& filepath)
	{
		auto it = sTextures.emplace(name, MakeReference<Texture>(filepath));

		if (!it.first->second->GetId())
		{
			Log::Console(LogLevel::Error, LN_LOG_FORMAT("[Asset] Failed to load texture '{}', path: '{}'.", name, filepath));
			return nullptr;
		}

		return it.first->second;
	}
}
