#include "Engine.h"
#include "AudioPlayer.h"

#include <Lion/Core/Asset.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>

#include <cctype>

namespace Lion
{
	namespace
	{
		std::string BusName(AudioBus bus)
		{
			switch (bus)
			{
				case AudioBus::Master: return "Master";
				case AudioBus::Music:  return "Music";
				default:               return "SFX";
			}
		}
	}

	void AudioPlayer::OnAwake()
	{
		if (!mClipPath.empty())
			mClip = Asset::LoadAudio(mClipPath, mClipPath);

		if (mPlayOnAwake)
			Play();
	}

	void AudioPlayer::OnEnable()
	{
		if (mPlayOnAwake && !IsPlaying())
			Play();
	}

	void AudioPlayer::OnDisable()
	{
		if (mRestartOnSceneReload)
			Stop();
	}

	void AudioPlayer::OnDestroy()
	{
		if (mRestartOnSceneReload)
			Stop();
	}

	void AudioPlayer::Reflect(Reflector& reflector)
	{
		reflector.FieldAsset("Audio Clip", mClipPath);
		reflector.Field("Bus", mBus);
		reflector.Field("Volume", mVolume);
		reflector.Field("Pitch", mPitch);
		reflector.Field("Loop", mLoop);
		reflector.Field("Play On Awake", mPlayOnAwake);
		reflector.Field("Restart On Scene Reload", mRestartOnSceneReload);
	}

	bool AudioPlayer::Play()
	{
		if (!mClip && !mClipPath.empty())
			mClip = Asset::LoadAudio(mClipPath, mClipPath);

		if (!mClip)
			return false;

		mVoices.erase(std::remove_if(mVoices.begin(), mVoices.end(),
			[](AudioVoice voice) { return !Audio::IsPlaying(voice); }), mVoices.end());

		const AudioVoice voice = Audio::Play(mClip, Playback(),
			mRestartOnSceneReload ? std::string() : PersistenceKey());

		if (voice == kInvalidAudioVoice)
			return false;

		if (std::find(mVoices.begin(), mVoices.end(), voice) == mVoices.end())
			mVoices.push_back(voice);

		return true;
	}

	void AudioPlayer::Stop()
	{
		for (const AudioVoice voice : mVoices)
			Audio::Stop(voice);

		mVoices.clear();
	}

	bool AudioPlayer::IsPlaying() const
	{
		return std::any_of(mVoices.begin(), mVoices.end(),
			[](AudioVoice voice) { return Audio::IsPlaying(voice); });
	}

	void AudioPlayer::SetClipPath(const std::string& path)
	{
		if (path == mClipPath)
			return;

		Stop();
		mClipPath = path;
		mClip.reset();
	}

	void AudioPlayer::SetVolume(float32 volume)
	{
		mVolume = std::clamp(volume, 0.0f, 1.0f);

		for (const AudioVoice voice : mVoices)
			Audio::SetVolume(voice, mVolume);
	}

	void AudioPlayer::SetPitch(float32 pitch)
	{
		mPitch = std::clamp(pitch, 0.25f, 4.0f);

		for (const AudioVoice voice : mVoices)
			Audio::SetPitch(voice, mPitch);
	}

	AudioBus AudioPlayer::GetBus() const
	{
		std::string bus = mBus;
		std::transform(bus.begin(), bus.end(), bus.begin(),
			[](unsigned char character) { return static_cast<char8>(std::tolower(character)); });

		if (bus == "master") return AudioBus::Master;
		if (bus == "music")  return AudioBus::Music;
		return AudioBus::SFX;
	}

	void AudioPlayer::SetBus(AudioBus bus)
	{
		mBus = BusName(bus);
	}

	AudioPlayback AudioPlayer::Playback() const
	{
		AudioPlayback playback;
		playback.volume = std::clamp(mVolume, 0.0f, 1.0f);
		playback.pitch = std::clamp(mPitch, 0.25f, 4.0f);
		playback.loop = mLoop;
		playback.bus = GetBus();
		return playback;
	}

	std::string AudioPlayer::PersistenceKey() const
	{
		return GetOwner().GetName() + "|" + mClipPath + "|" + mBus;
	}

	LION_REGISTER_COMPONENT(AudioPlayer)
}
