#pragma once

#include <Lion/Lion.h>

#include <array>

// Shared game-audio entry points keep clip paths, mixer routing and playback policy in one place.
class GameAudio final
{
public:
	static void PlaySfx(const std::string& path, Lion::float32 volume = 1.0f,
		Lion::float32 pitch = 1.0f);
	static void PlayUiHover();
	static void PlayUiSelect();
	static void PlayPower(const std::string& power);
	static void EnsureMusic(Lion::int32 level, bool overdrive = false);
	static void StopMusic();

private:
	static inline Lion::Reference<Lion::AudioClip> sMenuMusic;
	static inline std::array<Lion::Reference<Lion::AudioClip>, 20> sThemeMusic;
	static inline Lion::Reference<Lion::AudioClip> sOverdriveMusic;
	static inline Lion::AudioVoice sMusicVoice = Lion::kInvalidAudioVoice;
	static inline Lion::int32 sPlayingMusic = -1;
};
