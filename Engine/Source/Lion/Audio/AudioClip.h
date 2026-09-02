#pragma once

namespace Lion
{
	class Audio;

	// Decoded PCM audio data loaded from a Waveform Audio File Format resource.
	//
	// An AudioClip owns immutable sample data and can be played any number of times. Loading is separate
	// from playback so gameplay can cache a sound once and trigger inexpensive one-shot voices later.
	class AudioClip final
	{
	public:
		LION_API ~AudioClip();

		AudioClip(const AudioClip&) = delete;
		AudioClip& operator=(const AudioClip&) = delete;

		// Loads a PCM or IEEE-float .wav resource. Both project files and Vault-sealed Shipping assets are
		// accepted through the same path.
		static LION_API Reference<AudioClip> Create(const std::string& filePath);

		LION_API bool IsValid() const { return !mSamples.empty(); }
		LION_API uint16 GetChannelCount() const { return mChannels; }
		LION_API uint32 GetSampleRate() const { return mSampleRate; }
		LION_API float32 GetDuration() const;
		LION_API const std::string& GetFilePath() const { return mFilePath; }

	private:
		AudioClip() = default;

		std::string mFilePath;
		std::vector<byte> mSamples;
		uint16 mFormatTag = 0;
		uint16 mChannels = 0;
		uint32 mSampleRate = 0;
		uint32 mAverageBytesPerSecond = 0;
		uint16 mBlockAlign = 0;
		uint16 mBitsPerSample = 0;

		friend Audio;
	};
}
