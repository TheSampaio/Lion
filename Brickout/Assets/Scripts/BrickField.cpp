#include "BrickField.h"
#include "Ball.h"
#include "Brick.h"
#include "Paddle.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void BrickField::OnAwake()
{
	mBall = GetOwner().GetScene()->FindComponent<Ball>();
	mPaddle = GetOwner().GetScene()->FindComponent<Paddle>();

	if (!mBall || !mPaddle || GetOwner().GetScene()->CountActiveComponents<Brick>() == 0)
	{
		Log::Console(LogLevel::Error, "[BrickField] Requires a Ball, Paddle and authored Bricks in the same scene.");
		SetEnabled(false);
		return;
	}

	Log::Console(LogLevel::Information,
		LION_FORMAT_TEXT("[Game] Level {}/5: {} ({} bricks).", mLevel, mDifficulty,
			GetOwner().GetScene()->CountActiveComponents<Brick>()));
}

void BrickField::OnUpdate()
{
	HandleDebugLevelKeys();

	if (mState == State::Playing)
		CheckWinLose();

	else if (Input::GetActionTap("player_reset"))
		Restart();
}

void BrickField::Reflect(Reflector& reflector)
{
	reflector.Field("Level", mLevel);
	reflector.Field("Difficulty", mDifficulty);
	reflector.FieldAsset("Previous Scene", mPreviousScene);
	reflector.FieldAsset("Next Scene", mNextScene);
	reflector.Field("Lose Height", mLoseHeight);
}

void BrickField::CheckWinLose()
{
	if (GetOwner().GetScene()->CountActiveComponents<Brick>() == 0)
	{
		if (!mNextScene.empty())
			SceneManager::LoadScene(mNextScene);
		else
			EndRound(State::Won);
	}

	else if (mBall->GetOwner().GetWorldPosition().y < mLoseHeight)
		EndRound(State::Lost);
}

void BrickField::EndRound(State state)
{
	mState = state;
	mBall->Stop();

	if (state == State::Won)
	{
		Window::SetBackgroundColor(0.0f, 0.25f, 0.0f);
		Log::Console(LogLevel::Success, "[Game] All five levels complete! Press R or Select to play again.");
	}
	else
	{
		mBall->SetVisible(false);  // The ball has left the screen; hide it.
		Window::SetBackgroundColor(0.25f, 0.0f, 0.0f);
		Log::Console(LogLevel::Warning, "[Game] Game over! Press R or Select to play again.");
	}
}

void BrickField::Restart()
{
	SceneManager::ReloadScene();
}

void BrickField::HandleDebugLevelKeys()
{
#ifndef LN_SHIPPING
	if (!Application::IsEditor() || !Input::GetKeyPress(KeyCode::Shift))
		return;

	if (Input::GetKeyTap(KeyCode::Period) && !mNextScene.empty())
		SceneManager::LoadScene(mNextScene);
	else if (Input::GetKeyTap(KeyCode::Comma) && !mPreviousScene.empty())
		SceneManager::LoadScene(mPreviousScene);
#endif
}

LION_REGISTER_COMPONENT(BrickField)
