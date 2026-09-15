#pragma once

#include <Lion/Lion.h>

// Shared game-audio entry points keep clip paths, mixer routing and playback policy in one place.
class GameAudio final
{
public:
	static void PlaySfx(const std::string& path, Lion::float32 volume = 1.0f,
		Lion::float32 pitch = 1.0f);
	static void PlayUiHover();
	static void PlayUiSelect();
	static void PlayPower(const std::string& power);
};
