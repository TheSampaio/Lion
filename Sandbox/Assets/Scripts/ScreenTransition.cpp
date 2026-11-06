#include "ScreenTransition.h"
#include "GameSettings.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void ScreenTransition::OnAwake()
{
	sActive = this;
}

void ScreenTransition::OnUpdate()
{
	ResolvePostProcessing();
	if (!mPostProcessing)
		return;

	const float32 step = Clock::GetDeltaTime() / std::max(mDuration, 0.01f);
	if (mState == State::FadeIn)
	{
		mFade = std::max(0.0f, mFade - step);
		mPostProcessing->SetFade(mFade);
		if (mFade <= 0.0f)
			mState = State::Idle;
	}
	else if (mState == State::FadeOut)
	{
		mFade = std::min(1.0f, mFade + step);
		mPostProcessing->SetFade(mFade);
		if (mFade >= 1.0f)
		{
			const std::string destination = mPendingScene;
			mPendingScene.clear();
			SceneManager::LoadScene(destination);
		}
	}
}

void ScreenTransition::OnDestroy()
{
	if (sActive == this)
		sActive = nullptr;
}

void ScreenTransition::LoadScene(const std::string& path)
{
	if (!sActive || !sActive->mPostProcessing)
	{
		SceneManager::LoadScene(path);
		return;
	}

	if (sActive->mState == State::FadeOut)
		return;

	sActive->mPendingScene = path;
	sActive->mState = State::FadeOut;
}

void ScreenTransition::ResolvePostProcessing()
{
	if (mPostProcessing)
		return;

	const Reference<Scene> scene = GetOwner().GetScene();
	mPostProcessing = scene ? scene->FindComponent<PostProcessingComponent>() : nullptr;
	if (!mPostProcessing)
		return;

	GameSettings::Apply(*mPostProcessing);
	mPostProcessing->SetFade(mFade);
}

LION_REGISTER_COMPONENT(ScreenTransition)
