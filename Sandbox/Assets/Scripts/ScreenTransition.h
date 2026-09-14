#pragma once

#include <Lion/Lion.h>

// Applies presentation settings to each scene and owns the shared fade between scene loads.
class ScreenTransition final : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void OnDestroy() override;
	bool UpdatesWhenPaused() const override { return true; }

	static void LoadScene(const std::string& path);

private:
	enum class State
	{
		FadeIn,
		Idle,
		FadeOut,
	};

	static inline ScreenTransition* sActive = nullptr;
	Lion::PostProcessingComponent* mPostProcessing = nullptr;
	State mState = State::FadeIn;
	std::string mPendingScene;
	Lion::float32 mFade = 1.0f;
	Lion::float32 mDuration = 0.32f;

	void ResolvePostProcessing();
};
