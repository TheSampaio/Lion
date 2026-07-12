#include "BrickField.h"
#include "Ball.h"
#include "Brick.h"
#include "SceneQuery.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void BrickField::OnAwake()
{
	mBall = FindInScene<Ball>(GetOwner().GetScene());

	// Losing happens once the ball falls below the bottom edge of the screen.
	mLoseThresholdY = -(Window::GetSize().height / 2.0f);

	SpawnBricks();
}

void BrickField::OnUpdate()
{
	if (mState == State::Playing)
		CheckWinLose();

	else if (Input::GetKeyTap(KeyCode::R))
		Restart();
}

void BrickField::SpawnBricks()
{
	const float32 spacingX = 80.0f;
	const float32 spacingY = 40.0f;
	const int32 colCount = 8;
	const int32 rowCount = 5;

	const float32 positionOffset = 10.0f;
	const float32 positionX = -(colCount * (spacingX - positionOffset)) / 2.0f;  // Centers the whole field.
	const float32 positionY = 200.0f;

	const Reference<Scene> scene = GetOwner().GetScene();

	// World coordinate system: (0,0) is the center of the screen.
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

			auto entity = MakeReference<Entity>();
			entity->SetName("Brick");
			entity->GetTransform()->SetPosition(Vector(positionX + col * spacingX, positionY - row * spacingY, Depth::Middle));

			SpriteRenderer* renderer = entity->AddComponent<SpriteRenderer>(texture);
			const Size size = renderer->GetSize();

			entity->AddComponent<RigidBody2D>(BodyType::Static);
			entity->AddComponent<BoxCollider2D>(size.width, size.height, 1.0f, 0.0f, 1.0f);
			entity->AddComponent<Brick>();

			scene->Add(entity);
		}
	}
}

void BrickField::CheckWinLose()
{
	if (CountInScene<Brick>(GetOwner().GetScene()) == 0)
		EndRound(State::Won);

	else if (mBall->GetTransform()->GetPosition().y < mLoseThresholdY)
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
	const Reference<Scene> scene = GetOwner().GetScene();

	// Whatever bricks survived the round go with it. Removal is deferred, so the list being iterated is
	// not the one being changed.
	for (const auto& entity : scene->GetEntities())
		if (entity->HasComponent<Brick>())
			entity->RemoveFromScene();

	scene->FlushRemovals();

	SpawnBricks();
	mBall->Reset();

	Window::SetBackgroundColor(0.05f, 0.05f, 0.05f);
	mState = State::Playing;
}

LION_REGISTER_COMPONENT(BrickField)
