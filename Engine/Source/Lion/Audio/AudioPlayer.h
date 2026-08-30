#pragma once

#include <Lion/Audio/Audio.h>
#include <Lion/Logic/Component.h>

namespace Lion
{
	// Scene-authored audio playback, comparable to an Audio Source or AudioStreamPlayer.
	class AudioPlayer final : public Component
	{
	public:
		LION_API void OnAwake() override;
		LION_API void OnEnable() override;
		LION_API void OnDisable() override;
		LION_API void OnDestroy() override;
		LION_API void Reflect(Reflector& reflector) override;

		LION_API bool Play();
		LION_API void Stop();
		LION_API bool IsPlaying() const;

		LION_API const std::string& GetClipPath() const { return mClipPath; }
		LION_API void SetClipPath(const std::string& path);
		LION_API float32 GetVolume() const { return mVolume; }
		LION_API void SetVolume(float32 volume);
		LION_API float32 GetPitch() const { return mPitch; }
		LION_API void SetPitch(float32 pitch);
		LION_API bool IsLooping() const { return mLoop; }
		LION_API void SetLooping(bool loop) { mLoop = loop; }
		LION_API bool PlaysOnAwake() const { return mPlayOnAwake; }
		LION_API void SetPlayOnAwake(bool play) { mPlayOnAwake = play; }
		LION_API bool RestartsOnSceneReload() const { return mRestartOnSceneReload; }
		LION_API void SetRestartOnSceneReload(bool restart) { mRestartOnSceneReload = restart; }
		LION_API AudioBus GetBus() const;
		LION_API void SetBus(AudioBus bus);

	private:
		AudioPlayback Playback() const;
		std::string PersistenceKey() const;

		std::string mClipPath;
		std::string mBus = "SFX";
		float32 mVolume = 1.0f;
		float32 mPitch = 1.0f;
		bool mLoop = false;
		bool mPlayOnAwake = true;
		bool mRestartOnSceneReload = true;

		Reference<AudioClip> mClip;
		std::vector<AudioVoice> mVoices;
	};
}
