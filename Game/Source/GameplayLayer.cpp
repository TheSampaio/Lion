#include "GameplayLayer.h"

using namespace Lion;

void GameplayLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();

	mSpriteBackground = MakeScope<Sprite>("Resource/Sprite/Brickout/Background.jpg");
	mSpritePlayer = MakeScope<Sprite>("Resource/Sprite/Brickout/Player.png");
	mSpriteBall = MakeScope<Sprite>("Resource/Sprite/Brickout/Ball.png");

	mTextureBrickRed = MakeReference<Texture>("Resource/Sprite/Brickout/tile-3.png");
	mTextureBrickGreen = MakeReference<Texture>("Resource/Sprite/Brickout/tile-1.png");
	mTextureBrickBlue = MakeReference<Texture>("Resource/Sprite/Brickout/tile-2.png");
	mTextureBrickYellow = MakeReference<Texture>("Resource/Sprite/Brickout/tile-5.png");
	mTextureBrickPurple = MakeReference<Texture>("Resource/Sprite/Brickout/tile-4.png");

	mSpriteBrickRed01 = MakeReference<Sprite>(mTextureBrickRed);
	mSpriteBrickGreen01 = MakeReference<Sprite>(mTextureBrickGreen);
	mSpriteBrickBlue01 = MakeReference<Sprite>(mTextureBrickBlue);
	mSpriteBrickYellow01 = MakeReference<Sprite>(mTextureBrickYellow);
	mSpriteBrickPurple01 = MakeReference<Sprite>(mTextureBrickPurple);

	mSpriteBrickRed02 = MakeReference<Sprite>(mTextureBrickRed);
	mSpriteBrickGreen02 = MakeReference<Sprite>(mTextureBrickGreen);
	mSpriteBrickBlue02 = MakeReference<Sprite>(mTextureBrickBlue);
	mSpriteBrickYellow02 = MakeReference<Sprite>(mTextureBrickYellow);
	mSpriteBrickPurple02 = MakeReference<Sprite>(mTextureBrickPurple);

	mSpriteBrickRed03 = MakeReference<Sprite>(mTextureBrickRed);
	mSpriteBrickGreen03 = MakeReference<Sprite>(mTextureBrickGreen);
	mSpriteBrickBlue03 = MakeReference<Sprite>(mTextureBrickBlue);
	mSpriteBrickYellow03 = MakeReference<Sprite>(mTextureBrickYellow);
	mSpriteBrickPurple03 = MakeReference<Sprite>(mTextureBrickPurple);

	mSpriteBrickRed04 = MakeReference<Sprite>(mTextureBrickRed);
	mSpriteBrickGreen04 = MakeReference<Sprite>(mTextureBrickGreen);
	mSpriteBrickBlue04 = MakeReference<Sprite>(mTextureBrickBlue);
	mSpriteBrickYellow04 = MakeReference<Sprite>(mTextureBrickYellow);
	mSpriteBrickPurple04 = MakeReference<Sprite>(mTextureBrickPurple);

	mSpriteBrickRed05 = MakeReference<Sprite>(mTextureBrickRed);
	mSpriteBrickGreen05 = MakeReference<Sprite>(mTextureBrickGreen);
	mSpriteBrickBlue05 = MakeReference<Sprite>(mTextureBrickBlue);
	mSpriteBrickYellow05 = MakeReference<Sprite>(mTextureBrickYellow);
	mSpriteBrickPurple05 = MakeReference<Sprite>(mTextureBrickPurple);

	mSpriteBrickRed06 = MakeReference<Sprite>(mTextureBrickRed);
	mSpriteBrickGreen06 = MakeReference<Sprite>(mTextureBrickGreen);
	mSpriteBrickBlue06 = MakeReference<Sprite>(mTextureBrickBlue);
	mSpriteBrickYellow06 = MakeReference<Sprite>(mTextureBrickYellow);
	mSpriteBrickPurple06 = MakeReference<Sprite>(mTextureBrickPurple);
}

void GameplayLayer::OnUpdate()
{
	// Player
	if (Input::GetKeyPress(KeyCode::D))
		mPlayerPosition.x += mPlayerVelocity * Clock::GetDeltaTime();

	else if (Input::GetKeyPress(KeyCode::A))
		mPlayerPosition.x -= mPlayerVelocity * Clock::GetDeltaTime();

	// Ball
	mBallPosition.x += mBallVelocity.x * Clock::GetDeltaTime();
	mBallPosition.y += mBallVelocity.y * Clock::GetDeltaTime();

	// Limits
	const auto& windowSize = Window::GetSize();
	const float32 maxWidth = (windowSize[0] / 2.0f);
	const float32 maxHeight = (windowSize[1] / 2.0f);

	// Player (X)
	if (mPlayerPosition.x + (mSpritePlayer->GetSize()[0] / 2) >= maxWidth)
		mPlayerPosition.x = maxWidth - (mSpritePlayer->GetSize()[0] / 2);

	else if (mPlayerPosition.x - (mSpritePlayer->GetSize()[0] / 2) <= -maxWidth)
		mPlayerPosition.x = -maxWidth + (mSpritePlayer->GetSize()[0] / 2);

	// Ball (X / Y)
	if (mBallPosition.x + (mSpriteBall->GetSize()[0] / 2) >= maxWidth)
	{
		mBallPosition.x = maxWidth - (mSpriteBall->GetSize()[0] / 2);
		mBallVelocity.x *= -1.0;
	}

	else if (mBallPosition.x - (mSpriteBall->GetSize()[0] / 2) <= -maxWidth)
	{
		mBallPosition.x = -maxWidth + (mSpriteBall->GetSize()[0] / 2);
		mBallVelocity.x *= -1.0;
	}

	if (mBallPosition.y + (mSpriteBall->GetSize()[1] / 2) >= maxHeight)
	{
		mBallPosition.y = maxHeight - (mSpriteBall->GetSize()[1] / 2);
		mBallVelocity.y *= -1.0;
	}

	else if (mBallPosition.y - (mSpriteBall->GetSize()[1] / 2) <= -maxHeight)
	{
		mBallPosition.y = -maxHeight + (mSpriteBall->GetSize()[1] / 2);
		mBallVelocity.y *= -1.0;
	}
}

void GameplayLayer::OnRender()
{
	Renderer::RenderBegin(mCamera);

	mSpriteBackground->Draw(0.0f, 0.0f, Depth::Back);
	mSpritePlayer->Draw(mPlayerPosition.x, mPlayerPosition.y, Depth::Front);
	mSpriteBall->Draw(mBallPosition.x, mBallPosition.y, Depth::Middle);

	const float32 spacingX = 80.0f;
	const float32 spacingY = 40.0f;

	mSpriteBrickRed01    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 1.0f), Depth::Upper);
	mSpriteBrickGreen01  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 1.0f), Depth::Upper);
	mSpriteBrickBlue01   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 1.0f), Depth::Upper);
	mSpriteBrickYellow01 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 1.0f), Depth::Upper);
	mSpriteBrickPurple01 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 1.0f), Depth::Upper);

	mSpriteBrickRed02    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 2.0f), Depth::Upper);
	mSpriteBrickGreen02  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 2.0f), Depth::Upper);
	mSpriteBrickBlue02   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 2.0f), Depth::Upper);
	mSpriteBrickYellow02 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 2.0f), Depth::Upper);
	mSpriteBrickPurple02 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 2.0f), Depth::Upper);

	mSpriteBrickRed03    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 3.0f), Depth::Upper);
	mSpriteBrickGreen03  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 3.0f), Depth::Upper);
	mSpriteBrickBlue03   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 3.0f), Depth::Upper);
	mSpriteBrickYellow03 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 3.0f), Depth::Upper);
	mSpriteBrickPurple03 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 3.0f), Depth::Upper);

	mSpriteBrickRed04    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 4.0f), Depth::Upper);
	mSpriteBrickGreen04  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 4.0f), Depth::Upper);
	mSpriteBrickBlue04   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 4.0f), Depth::Upper);
	mSpriteBrickYellow04 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 4.0f), Depth::Upper);
	mSpriteBrickPurple04 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 4.0f), Depth::Upper);

	mSpriteBrickRed05    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 5.0f), Depth::Upper);
	mSpriteBrickGreen05  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 5.0f), Depth::Upper);
	mSpriteBrickBlue05   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 5.0f), Depth::Upper);
	mSpriteBrickYellow05 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 5.0f), Depth::Upper);
	mSpriteBrickPurple05 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 5.0f), Depth::Upper);

	mSpriteBrickRed06    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 6.0f), Depth::Upper);
	mSpriteBrickGreen06  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 6.0f), Depth::Upper);
	mSpriteBrickBlue06   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 6.0f), Depth::Upper);
	mSpriteBrickYellow06 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 6.0f), Depth::Upper);
	mSpriteBrickPurple06 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 6.0f), Depth::Upper);

	Renderer::RenderEnd();
}

void GameplayLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowResize>(LN_EVENT_BIND(GameplayLayer::OnEventWindowResize));
}

bool GameplayLayer::OnEventWindowResize(const EventWindowResize& event)
{
	mCamera->OnResize(static_cast<float32>(event.GetWidth()), static_cast<float32>(event.GetHeight()));
	return false;
}
