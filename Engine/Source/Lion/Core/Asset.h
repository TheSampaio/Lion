#pragma once

#include <Lion/Audio/AudioClip.h>
#include <Lion/Render/Texture.h>

namespace Lion
{
	class BitmapFont;

	class Asset
	{
	public:
		Asset() = default;
		~Asset();

		Asset(const Asset&) = delete;
		Asset& operator=(const Asset&) = delete;

		static LION_API Reference<Texture> LoadTexture(const std::string& name);

		static LION_API Reference<Texture> LoadTexture(const std::string& name, const std::string& filepath);

		static LION_API Reference<AudioClip> LoadAudio(const std::string& name);
		static LION_API Reference<AudioClip> LoadAudio(const std::string& name, const std::string& filePath);

		static LION_API Reference<BitmapFont> LoadFont(const std::string& name);
		static LION_API Reference<BitmapFont> LoadFont(const std::string& name, const std::string& filePath);

	private:
		static inline std::unordered_map<std::string, Reference<Texture>> sTextures;
		static inline std::unordered_map<std::string, Reference<AudioClip>> sAudioClips;
		static inline std::unordered_map<std::string, Reference<BitmapFont>> sFonts;
	};
}
