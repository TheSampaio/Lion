#include "Engine.h"
#include "AudioClip.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Vault.h>

#include <cstring>

namespace Lion
{
	namespace
	{
		template<typename T>
		bool ReadValue(const std::string& bytes, size_t offset, T& value)
		{
			if (offset + sizeof(T) > bytes.size())
				return false;

			std::memcpy(&value, bytes.data() + offset, sizeof(T));
			return true;
		}

		bool HasTag(const std::string& bytes, size_t offset, const char8* tag)
		{
			return offset + 4 <= bytes.size() && std::memcmp(bytes.data() + offset, tag, 4) == 0;
		}
	}

	AudioClip::~AudioClip() = default;

	Reference<AudioClip> AudioClip::Create(const std::string& filePath)
	{
		std::ifstream file(ResolveResourcePath(filePath), std::ios::binary);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[Audio] Could not locate '{}'.", filePath));
			return nullptr;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		const std::string bytes = Vault::Unseal(buffer.str());

		if (bytes.size() < 12 || !HasTag(bytes, 0, "RIFF") || !HasTag(bytes, 8, "WAVE"))
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Audio] '{}' is not a valid WAV file.", filePath));
			return nullptr;
		}

		auto clip = Reference<AudioClip>(new AudioClip());
		clip->mFilePath = filePath;
		bool foundFormat = false;
		bool foundSamples = false;

		for (size_t chunk = 12; chunk + 8 <= bytes.size();)
		{
			uint32 chunkSize = 0;
			ReadValue(bytes, chunk + 4, chunkSize);
			const size_t data = chunk + 8;

			if (data + chunkSize > bytes.size())
				break;

			if (HasTag(bytes, chunk, "fmt ") && chunkSize >= 16)
			{
				foundFormat = ReadValue(bytes, data + 0, clip->mFormatTag)
					&& ReadValue(bytes, data + 2, clip->mChannels)
					&& ReadValue(bytes, data + 4, clip->mSampleRate)
					&& ReadValue(bytes, data + 8, clip->mAverageBytesPerSecond)
					&& ReadValue(bytes, data + 12, clip->mBlockAlign)
					&& ReadValue(bytes, data + 14, clip->mBitsPerSample);
			}
			else if (HasTag(bytes, chunk, "data") && chunkSize > 0)
			{
				const byte* first = reinterpret_cast<const byte*>(bytes.data() + data);
				clip->mSamples.assign(first, first + chunkSize);
				foundSamples = true;
			}

			chunk = data + chunkSize + (chunkSize & 1u);
		}

		constexpr uint16 kPcm = 1;
		constexpr uint16 kIeeeFloat = 3;
		const bool supportedFormat = clip->mFormatTag == kPcm || clip->mFormatTag == kIeeeFloat;

		if (!foundFormat || !foundSamples || !supportedFormat || clip->mChannels == 0
			|| clip->mSampleRate == 0 || clip->mBlockAlign == 0 || clip->mBitsPerSample == 0)
		{
			Log::Console(LogLevel::Error,
				LION_FORMAT_TEXT("[Audio] '{}' uses an unsupported or incomplete WAV format.", filePath));
			return nullptr;
		}

		Log::Console(LogLevel::Trace,
			LION_FORMAT_TEXT("[Audio] Loaded '{}' ({:.2f}s, {} Hz, {} channel(s)).",
				filePath, clip->GetDuration(), clip->mSampleRate, clip->mChannels));
		return clip;
	}

	float32 AudioClip::GetDuration() const
	{
		return mAverageBytesPerSecond == 0
			? 0.0f
			: static_cast<float32>(mSamples.size()) / static_cast<float32>(mAverageBytesPerSecond);
	}
}
