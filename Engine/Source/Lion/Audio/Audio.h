#pragma once

#include <Lion/Audio/AudioClip.h>

namespace Lion
{
	class Application;

	enum class AudioBus
	{
		Master,
		SFX,
		Music,
	};

	using AudioVoice = uint64;
	constexpr AudioVoice kInvalidAudioVoice = 0;

	struct AudioPlayback
	{
		float32 volume = 1.0f;
		float32 pitch = 1.0f;
		bool loop = false;
		AudioBus bus = AudioBus::SFX;
	};

	// Runtime audio mixer. Voices play independently and route through Master, SFX or Music volume buses.
	class Audio
	{
	public:
		static LION_API bool IsAvailable();
		static LION_API AudioVoice Play(const Reference<AudioClip>& clip, const AudioPlayback& playback = {},
			const std::string& persistenceKey = {});
		static LION_API bool PlayOneShot(const Reference<AudioClip>& clip, float32 volume = 1.0f);
		static LION_API void Stop(AudioVoice voice);
		static LION_API bool IsPlaying(AudioVoice voice);
		static LION_API void SetVolume(AudioVoice voice, float32 volume);
		static LION_API void SetPitch(AudioVoice voice, float32 pitch);
		static LION_API void SetBusVolume(AudioBus bus, float32 volume);
		static LION_API float32 GetBusVolume(AudioBus bus);
		static LION_API void StopAll();

		friend Application;

	private:
		struct State;
		static State* sState;

		static void New();
		static void Delete();
		static void Update();
	};
}
