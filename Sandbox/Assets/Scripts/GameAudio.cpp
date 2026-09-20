#include "GameAudio.h"

#include <Lion/Core/Asset.h>

using namespace Lion;

void GameAudio::PlaySfx(const std::string& path, float32 volume, float32 pitch)
{
	const Reference<AudioClip> clip = Asset::LoadAudio(path, path);
	if (!clip)
		return;

	AudioPlayback playback;
	playback.volume = std::clamp(volume, 0.0f, 1.0f);
	playback.pitch = std::clamp(pitch, 0.25f, 4.0f);
	playback.bus = AudioBus::SFX;
	Audio::Play(clip, playback);
}

void GameAudio::PlayUiHover()
{
	PlaySfx("Sounds/ui-hover.wav", 0.34f);
}

void GameAudio::PlayUiSelect()
{
	PlaySfx("Sounds/ui-select.wav", 0.48f);
}

void GameAudio::PlayPower(const std::string& power)
{
	if (power == "Extra Life") PlaySfx("Sounds/power-life.wav", 0.72f);
	else if (power == "Multiball") PlaySfx("Sounds/power-multiball.wav", 0.72f);
	else if (power == "Bomb") PlaySfx("Sounds/power-bomb.wav", 0.82f);
	else if (power == "Wide Paddle") PlaySfx("Sounds/power-wide.wav", 0.68f);
	else if (power == "Duplicate Paddle") PlaySfx("Sounds/power-duplicate.wav", 0.68f);
}

void GameAudio::EnsureMusic(bool gameplay)
{
	if (sMusicVoice != kInvalidAudioVoice && Audio::IsPlaying(sMusicVoice)
		&& gameplay == sPlayingGameplayMusic)
		return;

	Audio::Stop(sMusicVoice);
	sMusicVoice = kInvalidAudioVoice;
	sPlayingGameplayMusic = gameplay;
	Reference<AudioClip>& clip = gameplay ? sGameMusic : sMenuMusic;
	const std::string path = gameplay ? "Sounds/music-game.wav" : "Sounds/music-menu.wav";
	if (!clip)
		clip = Asset::LoadAudio(path, path);
	if (!clip)
	{
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[GameAudio] Could not load music '{}'.", path));
		return;
	}

	AudioPlayback playback;
	playback.volume = gameplay ? 0.68f : 0.58f;
	playback.loop = true;
	playback.bus = AudioBus::Music;
	sMusicVoice = Audio::Play(clip, playback, "brickout-music");
	if (sMusicVoice == kInvalidAudioVoice)
		Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[GameAudio] Could not start music '{}'.", path));
}

void GameAudio::StopMusic()
{
	Audio::Stop(sMusicVoice);
	sMusicVoice = kInvalidAudioVoice;
}
