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

void GameplayLayer::OnRender()
{
	Renderer::RenderBegin(mCamera);

	mSpriteBackground->Draw(0.0f, 0.0f, Depth::Back);
	mSpriteBall->Draw(0.0f, -258.0f, Depth::Upper);
	mSpritePlayer->Draw(0.0f, -275.0f, Depth::Front);

	const float32 spacingX = 80.0f;
	const float32 spacingY = 40.0f;

	mSpriteBrickRed01    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 1.0f), Depth::Middle);
	mSpriteBrickGreen01  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 1.0f), Depth::Middle);
	mSpriteBrickBlue01   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 1.0f), Depth::Middle);
	mSpriteBrickYellow01 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 1.0f), Depth::Middle);
	mSpriteBrickPurple01 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 1.0f), Depth::Middle);

	mSpriteBrickRed02    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 2.0f), Depth::Middle);
	mSpriteBrickGreen02  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 2.0f), Depth::Middle);
	mSpriteBrickBlue02   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 2.0f), Depth::Middle);
	mSpriteBrickYellow02 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 2.0f), Depth::Middle);
	mSpriteBrickPurple02 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 2.0f), Depth::Middle);

	mSpriteBrickRed03    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 3.0f), Depth::Middle);
	mSpriteBrickGreen03  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 3.0f), Depth::Middle);
	mSpriteBrickBlue03   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 3.0f), Depth::Middle);
	mSpriteBrickYellow03 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 3.0f), Depth::Middle);
	mSpriteBrickPurple03 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 3.0f), Depth::Middle);

	mSpriteBrickRed04    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 4.0f), Depth::Middle);
	mSpriteBrickGreen04  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 4.0f), Depth::Middle);
	mSpriteBrickBlue04   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 4.0f), Depth::Middle);
	mSpriteBrickYellow04 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 4.0f), Depth::Middle);
	mSpriteBrickPurple04 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 4.0f), Depth::Middle);

	mSpriteBrickRed05    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 5.0f), Depth::Middle);
	mSpriteBrickGreen05  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 5.0f), Depth::Middle);
	mSpriteBrickBlue05   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 5.0f), Depth::Middle);
	mSpriteBrickYellow05 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 5.0f), Depth::Middle);
	mSpriteBrickPurple05 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 5.0f), Depth::Middle);

	mSpriteBrickRed06    -> Draw(-228.0f + (spacingX * 1.0f), 150.0f - (spacingY * 6.0f), Depth::Middle);
	mSpriteBrickGreen06  -> Draw(-228.0f + (spacingX * 2.0f), 150.0f - (spacingY * 6.0f), Depth::Middle);
	mSpriteBrickBlue06   -> Draw(-228.0f + (spacingX * 3.0f), 150.0f - (spacingY * 6.0f), Depth::Middle);
	mSpriteBrickYellow06 -> Draw(-228.0f + (spacingX * 4.0f), 150.0f - (spacingY * 6.0f), Depth::Middle);
	mSpriteBrickPurple06 -> Draw(-228.0f + (spacingX * 5.0f), 150.0f - (spacingY * 6.0f), Depth::Middle);

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
