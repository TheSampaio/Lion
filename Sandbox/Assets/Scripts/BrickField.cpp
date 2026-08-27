#include "BrickField.h"
#include "Ball.h"
#include "Brick.h"
#include "Paddle.h"
#include "SceneQuery.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void BrickField::OnAwake()
{
	mBall = FindInScene<Ball>(GetOwner().GetScene());
	mPaddle = FindInScene<Paddle>(GetOwner().GetScene());

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

	else if (Input::GetKeyTap(KeyCode::R))
		Restart();
}

void BrickField::Reflect(Reflector& reflector)
{
	reflector.Field("Lose Height", mLoseHeight);
}

void BrickField::CheckWinLose()
{
	if (CountActiveInScene<Brick>(GetOwner().GetScene()) == 0)
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
		Log::Console(LogLevel::Success, "[Game] You win! Press R to play again.");
	}
	else
	{
		mBall->SetVisible(false);  // The ball has left the screen; hide it.
		Window::SetBackgroundColor(0.25f, 0.0f, 0.0f);
		Log::Console(LogLevel::Warning, "[Game] Game over! Press R to play again.");
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
