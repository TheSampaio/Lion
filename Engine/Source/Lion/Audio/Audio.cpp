#include "Engine.h"
#include "Audio.h"

#include <Lion/Core/Log.h>

#include <xaudio2.h>

namespace Lion
{
	namespace
	{
		size_t BusIndex(AudioBus bus)
		{
			return static_cast<size_t>(bus);
		}

		float32 MixedVolume(const std::array<float32, 3>& buses, const AudioPlayback& playback)
		{
			const float32 master = buses[BusIndex(AudioBus::Master)];
			const float32 bus = playback.bus == AudioBus::Master ? 1.0f : buses[BusIndex(playback.bus)];
			return std::clamp(playback.volume, 0.0f, 1.0f) * master * bus;
		}
	}

	struct Audio::State
	{
		struct Voice
		{
			AudioVoice id = kInvalidAudioVoice;
			IXAudio2SourceVoice* source = nullptr;
			Reference<AudioClip> clip;
			AudioPlayback playback;
			std::string persistenceKey;
		};

		IXAudio2* engine = nullptr;
		IXAudio2MasteringVoice* masteringVoice = nullptr;
		std::vector<Voice> voices;
		std::array<float32, 3> busVolumes{ 1.0f, 1.0f, 1.0f };
		AudioVoice nextVoice = 1;
		bool ownsComApartment = false;
	};

	Audio::State* Audio::sState = nullptr;

	void Audio::New()
	{
		sState = new State();

		const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		sState->ownsComApartment = SUCCEEDED(com);

		HRESULT result = XAudio2Create(&sState->engine, 0, XAUDIO2_DEFAULT_PROCESSOR);

		if (SUCCEEDED(result))
			result = sState->engine->CreateMasteringVoice(&sState->masteringVoice);

		if (FAILED(result))
		{
			Log::Console(LogLevel::Warning, "[Audio] XAudio2 is unavailable; playback is disabled.");
			return;
		}

		Log::Console(LogLevel::Success, "[Application] Audio initialized successfully.");
	}

	void Audio::Delete()
	{
		if (!sState)
			return;

		StopAll();

		if (sState->masteringVoice)
			sState->masteringVoice->DestroyVoice();

		if (sState->engine)
			sState->engine->Release();

		if (sState->ownsComApartment)
			CoUninitialize();

		delete sState;
		sState = nullptr;
	}

	void Audio::Update()
	{
		if (!IsAvailable())
			return;

		for (auto voice = sState->voices.begin(); voice != sState->voices.end();)
		{
			XAUDIO2_VOICE_STATE state{};
			voice->source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

			if (state.BuffersQueued != 0)
			{
				++voice;
				continue;
			}

			voice->source->DestroyVoice();
			voice = sState->voices.erase(voice);
		}
	}

	bool Audio::IsAvailable()
	{
		return sState && sState->engine && sState->masteringVoice;
	}

	AudioVoice Audio::Play(const Reference<AudioClip>& clip, const AudioPlayback& playback,
		const std::string& persistenceKey)
	{
		if (!IsAvailable() || !clip || !clip->IsValid())
			return kInvalidAudioVoice;

		if (!persistenceKey.empty())
		{
			const auto existing = std::find_if(sState->voices.begin(), sState->voices.end(),
				[&](const State::Voice& voice) { return voice.persistenceKey == persistenceKey; });

			if (existing != sState->voices.end())
			{
				existing->playback = playback;
				existing->source->SetVolume(MixedVolume(sState->busVolumes, playback));
				existing->source->SetFrequencyRatio(std::clamp(playback.pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO));
				return existing->id;
			}
		}

		WAVEFORMATEX format{};
		format.wFormatTag = clip->mFormatTag;
		format.nChannels = clip->mChannels;
		format.nSamplesPerSec = clip->mSampleRate;
		format.nAvgBytesPerSec = clip->mAverageBytesPerSecond;
		format.nBlockAlign = clip->mBlockAlign;
		format.wBitsPerSample = clip->mBitsPerSample;

		IXAudio2SourceVoice* source = nullptr;
		HRESULT result = sState->engine->CreateSourceVoice(&source, &format);

		if (FAILED(result))
			return kInvalidAudioVoice;

		XAUDIO2_BUFFER buffer{};
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		buffer.AudioBytes = static_cast<UINT32>(clip->mSamples.size());
		buffer.pAudioData = clip->mSamples.data();
		buffer.LoopCount = playback.loop ? XAUDIO2_LOOP_INFINITE : 0;

		result = source->SetVolume(MixedVolume(sState->busVolumes, playback));
		if (SUCCEEDED(result))
			result = source->SetFrequencyRatio(std::clamp(playback.pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO));
		if (SUCCEEDED(result))
			result = source->SubmitSourceBuffer(&buffer);
		if (SUCCEEDED(result))
			result = source->Start();

		if (FAILED(result))
		{
			source->DestroyVoice();
			return kInvalidAudioVoice;
		}

		const AudioVoice voice = sState->nextVoice++;
		sState->voices.push_back({ voice, source, clip, playback, persistenceKey });
		return voice;
	}

	bool Audio::PlayOneShot(const Reference<AudioClip>& clip, float32 volume)
	{
		AudioPlayback playback;
		playback.volume = volume;
		return Play(clip, playback) != kInvalidAudioVoice;
	}

	void Audio::Stop(AudioVoice voice)
	{
		if (!sState || voice == kInvalidAudioVoice)
			return;

		const auto found = std::find_if(sState->voices.begin(), sState->voices.end(),
			[voice](const State::Voice& entry) { return entry.id == voice; });

		if (found == sState->voices.end())
			return;

		found->source->Stop();
		found->source->DestroyVoice();
		sState->voices.erase(found);
	}

	bool Audio::IsPlaying(AudioVoice voice)
	{
		if (!sState || voice == kInvalidAudioVoice)
			return false;

		return std::any_of(sState->voices.begin(), sState->voices.end(),
			[voice](const State::Voice& entry) { return entry.id == voice; });
	}

	void Audio::SetVolume(AudioVoice voice, float32 volume)
	{
		if (!sState)
			return;

		for (State::Voice& entry : sState->voices)
			if (entry.id == voice)
			{
				entry.playback.volume = volume;
				entry.source->SetVolume(MixedVolume(sState->busVolumes, entry.playback));
				return;
			}
	}

	void Audio::SetPitch(AudioVoice voice, float32 pitch)
	{
		if (!sState)
			return;

		for (State::Voice& entry : sState->voices)
			if (entry.id == voice)
			{
				entry.playback.pitch = pitch;
				entry.source->SetFrequencyRatio(std::clamp(pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO));
				return;
			}
	}

	void Audio::SetBusVolume(AudioBus bus, float32 volume)
	{
		if (!sState)
			return;

		sState->busVolumes[BusIndex(bus)] = std::clamp(volume, 0.0f, 1.0f);

		for (State::Voice& voice : sState->voices)
			voice.source->SetVolume(MixedVolume(sState->busVolumes, voice.playback));
	}

	float32 Audio::GetBusVolume(AudioBus bus)
	{
		return sState ? sState->busVolumes[BusIndex(bus)] : 0.0f;
	}

	void Audio::StopAll()
	{
		if (!sState)
			return;

		for (State::Voice& voice : sState->voices)
		{
			voice.source->Stop();
			voice.source->DestroyVoice();
		}

		sState->voices.clear();
	}
}
