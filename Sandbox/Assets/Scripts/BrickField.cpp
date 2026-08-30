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

	if (!mBall || !mPaddle)
	{
		Log::Console(LogLevel::Error, "[BrickField] Requires a Ball and Paddle in the same scene.");
		SetEnabled(false);
	}
}

void BrickField::OnUpdate()
{
	if (mState == State::Playing)
		CheckWinLose();

	else if (Input::GetActionTap("player_reset"))
		Restart();
}

void BrickField::Reflect(Reflector& reflector)
{
	reflector.Field("Lose Height", mLoseHeight);
}

void BrickField::CheckWinLose()
{
	if (GetOwner().GetScene()->CountActiveComponents<Brick>() == 0)
		EndRound(State::Won);

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
		Log::Console(LogLevel::Success, "[Game] You win! Press R or Select to play again.");
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
	for (const auto& entity : GetOwner().GetScene()->GetEntities())
		if (entity->HasComponent<Brick>())
		{
			entity->SetVisible(true);
			entity->SetEnabled(true);
		}

	mPaddle->Reset();
	mBall->Reset();

	Window::SetBackgroundColor(0.05f, 0.05f, 0.05f);
	mState = State::Playing;
}

LION_REGISTER_COMPONENT(BrickField)
