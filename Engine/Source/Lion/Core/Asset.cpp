#include "Engine.h"
#include "Asset.h"

#include <Lion/Core/Log.h>

namespace Lion
{
	Asset::~Asset()
	{
		sAudioClips.clear();
		sTextures.clear();
	}

	Reference<Texture> Asset::LoadTexture(const std::string& name)
	{
		auto it = sTextures.find(name);

		if (it != sTextures.end())
			return it->second;

		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Asset] Texture '{}' not found in cache.", name));
		return nullptr;
	}

	Reference<Texture> Asset::LoadTexture(const std::string& name, const std::string& filepath)
	{
		auto it = sTextures.find(name);

		if (it != sTextures.end())
			return it->second;

		// Load and cache it
		auto texture = Texture::Create(filepath);
		sTextures[name] = texture;

		return texture;
	}

	Reference<AudioClip> Asset::LoadAudio(const std::string& name)
	{
		const auto found = sAudioClips.find(name);

		if (found != sAudioClips.end())
			return found->second;

		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Asset] Audio clip '{}' not found in cache.", name));
		return nullptr;
	}

	Reference<AudioClip> Asset::LoadAudio(const std::string& name, const std::string& filePath)
	{
		const auto found = sAudioClips.find(name);

		if (found != sAudioClips.end())
			return found->second;

		Reference<AudioClip> clip = AudioClip::Create(filePath);

		if (clip)
			sAudioClips[name] = clip;

		return clip;
	}
}
