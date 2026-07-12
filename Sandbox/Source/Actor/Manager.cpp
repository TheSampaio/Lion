#include "Manager.h"
#include "Ball.h"
#include "Brick.h"

using namespace Lion;

Manager::Manager(Reference<Ball> ball)
	: mBall(std::move(ball))
{
}

void Manager::OnAwake()
{
	// The manager owns the background sprite through its own transform and component.
	GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Back));
	AddComponent<SpriteRenderer>("Sprites/Brickout/background.jpg");

	// Losing happens once the ball falls below the bottom edge of the screen.
	mLoseThresholdY = -(Window::GetSize().height / 2.0f);

	SpawnBricks();
}

void Manager::OnUpdate()
{
	if (mState == State::Playing)
	{
		RemoveDestroyedBricks();
		CheckWinLose();
	}
	else if (Input::GetKeyTap(KeyCode::R))
	{
		Restart();
	}
}

void Manager::SpawnBricks()
{
	const float32 spacingX = 80.0f;
	const float32 spacingY = 40.0f;
	const int32 colCount = 8;
	const int32 rowCount = 5; // Max rows are 5, don't increase!

	const float32 positionOffset = 10.0f;
	const float32 positionX = -(colCount * (spacingX - positionOffset)) / 2.0f; // Center all bricks
	const float32 positionY = 200.0f;

	// World coordinate system: (0,0) is the center of the screen
	for (int32 row = 0; row < rowCount; row++)
	{
		for (int32 col = 0; col < colCount; col++)
		{
			Reference<Texture> texture;

			switch (row)
			{
				case 0: texture = Asset::LoadTexture("brick_red",    "Sprites/Brickout/tile-3.png"); break;
				case 1: texture = Asset::LoadTexture("brick_green",  "Sprites/Brickout/tile-1.png"); break;
				case 2: texture = Asset::LoadTexture("brick_blue",   "Sprites/Brickout/tile-2.png"); break;
				case 3: texture = Asset::LoadTexture("brick_yellow", "Sprites/Brickout/tile-5.png"); break;
				case 4: texture = Asset::LoadTexture("brick_purple", "Sprites/Brickout/tile-4.png"); break;
			}

			const float32 x = positionX + col * spacingX;
			const float32 y = positionY - row * spacingY;

			auto brick = MakeReference<Brick>(texture, Vector(x, y, Depth::Middle));
			mBricks.push_back(brick);
			GetScene()->Add(brick);
		}
	}
}

void Manager::RemoveDestroyedBricks()
{
	for (auto it = mBricks.begin(); it != mBricks.end();)
	{
		if ((*it)->IsDestroyed())
		{
			GetScene()->Remove(*it);
			it = mBricks.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Manager::CheckWinLose()
{
	if (mBricks.empty())
		EndGame(State::Won);

	else if (mBall->GetTransform()->GetPosition().y < mLoseThresholdY)
		EndGame(State::Lost);
}

void Manager::EndGame(State state)
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
		mBall->SetVisible(false); // The ball has left the screen; hide it.
		Window::SetBackgroundColor(0.25f, 0.0f, 0.0f);
		Log::Console(LogLevel::Warning, "[Game] Game over! Press R to play again.");
	}
}

void Manager::Restart()
{
	for (auto& brick : mBricks)
		GetScene()->Remove(brick);

	mBricks.clear();
	SpawnBricks();

	mBall->Reset();

	Window::SetBackgroundColor(0.05f, 0.05f, 0.05f);
	mState = State::Playing;
}
